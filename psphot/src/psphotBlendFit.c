# include "psphotInternal.h"
bool psphotBlendFitSetSource(pmSource *source, psImage *IDimage, float sigSigma);
bool psphotBlendSetSourceSatstar (psImage *image, pmSource *source);
float InterpolateValues (float X0, float Y0, float X1, float Y1, float X);

// for now, let's store the detections on the readout->analysis for each readout
bool psphotBlendFit (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Fit Sources (Non-Linear) ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (!psphotBlendFitReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to fit sources (non-linear) for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// XXX I don't like this name
bool psphotBlendFitReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    int Nfit = 0;
    int Npsf = 0;
    int Next = 0;
    int Nfail = 0;
    bool status;

    psTimerStart ("psphot.fit.nonlinear");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping blend fit");
        return true;
    }

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // do not thread if we are trying to study the fitting process
    if (psTraceGetLevel ("psphot.psphotFitEXT") >= 6) {
      nThreads = 0;
    }

    // Define source fitting parameters for extended source fits
    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();

    fitOptions->mode          = PM_SOURCE_FIT_PSF;

    fitOptions->covarFactor   = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix

    fitOptions->nIter         = psMetadataLookupS32(&status, recipe, "EXT_FIT_ITER"); // Max number of fit iterations
    assert (status && fitOptions->nIter > 0); 

    fitOptions->minTol = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MIN_TOL"); // Fit tolerance
    if (!status || !isfinite(fitOptions->minTol) || fitOptions->minTol <= 0) {
	fitOptions->minTol = psMetadataLookupF32 (&status, recipe, "PSF_FIT_TOL"); // Fit tolerance
	if (!status || !isfinite(fitOptions->minTol) || fitOptions->minTol <= 0) {
	    psAbort("PSF_FIT_MIN_TOL (and PSF_FIT_TOL) not defined or positive");
	}
    }

    fitOptions->maxTol = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MAX_TOL"); // Fit tolerance
    if (!status || !isfinite(fitOptions->maxTol) || fitOptions->maxTol <= 0) { fitOptions->maxTol = 1.0; }

    fitOptions->chisqConvergence = psMetadataLookupBool (&status, recipe, "LMM_FIT_CHISQ_CONVERGENCE"); // Fit tolerance
    if (!status) { fitOptions->chisqConvergence = true; } // default to the old method (chisqConvergence)

    fitOptions->useReweighting = psMetadataLookupBool (&status, recipe, "LMM_FIT_USE_REWEIGHTING"); // Fit tolerance
    if (!status) { fitOptions->useReweighting = false; }

    fitOptions->gainFactorMode = psMetadataLookupS32 (&status, recipe, "LMM_FIT_GAIN_FACTOR_MODE"); // Fit tolerance
    if (!status) { fitOptions->gainFactorMode = 0; } // default to the old method

    fitOptions->poissonErrors = psMetadataLookupBool(&status, recipe, "POISSON.ERRORS.PHOT.LMM"); // Poisson errors?
    assert (status);

    fitOptions->maxChisqDOF = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MAX_CHISQ"); // Fit tolerance

    float skySig = psMetadataLookupF32(&status, recipe, "SKY_SIG");
    assert (status && isfinite(skySig) && skySig > 0);
    fitOptions->weight = PS_SQR(skySig);

# if (0)
    // **** test block : generate an ID image where pixels are set based the source models (flux > 0.1 peak && flux > 2.0 skySig)
    psImage *IDimage = psImageAlloc (readout->image->numCols, readout->image->numRows, PS_TYPE_S32);
    psImageInit (IDimage, 0);

    // start with the currently known moments (Mxx, Myy, Mxy) and generate a window image
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
	psphotBlendFitSetSource (source, IDimage, skySig);
    }
    psphotSaveImage (NULL, IDimage, "idimage.fits");
# endif

    psphotInitLimitsPSF (recipe, readout);
    psphotInitLimitsEXT (recipe, readout);
    psphotInitRadiusPSF (recipe, readout);

    // starts the timer, sets up the array of fitSets
    psphotFitInit (nThreads);

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);
    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping blend");
	psFree (fitOptions);
        return true;
    }

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_BLEND_FIT");
            psArray *newSources = psArrayAllocEmpty(16);

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, recipe);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, psf);
            psArrayAdd(job->args, 1, newSources); // return for new sources
            psArrayAdd(job->args, 1, fitOptions); // default fit options
            psFree (newSources);

            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfit
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Npsf
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Next
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfail

            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree (fitOptions);
                return NULL;
            }
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
	    psFree (fitOptions);
            return NULL;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
                psScalar *scalar = NULL;
                scalar = job->args->data[6];
                Nfit += scalar->data.S32;
                scalar = job->args->data[7];
                Npsf += scalar->data.S32;
                scalar = job->args->data[8];
                Next += scalar->data.S32;
                scalar = job->args->data[9];
                Nfail += scalar->data.S32;

                // add these back onto sources
                psArray *newSources = job->args->data[4];
                for (int j = 0; j < newSources->n; j++) {
                    psArrayAdd (sources, 16, newSources->data[j]);
                }
            }
            psFree(job);
	}
    }
    psFree (cellGroups);

    if (psTraceGetLevel("psphot") >= 6) {
      psphotSaveImage (NULL, readout->image,  "image.v2.fits");
    }
    psFree (fitOptions);

    psLogMsg ("psphot.psphotBlendFit", PS_LOG_WARN, "fit models: %f sec for %d objects (%d psf, %d ext, %d failed, %ld skipped)\n", psTimerMark ("psphot.fit.nonlinear"), Nfit, Npsf, Next, Nfail, sources->n - Nfit);
    psphotFitSummary ();

    psphotVisualShowResidualImage (readout, false);
    psphotVisualShowObjectRegions (readout, recipe, sources);
    psphotVisualShowFlags (sources);

    return true;
}

bool psphotBlendFit_Threaded (psThreadJob *job) {

    bool status = false;
    int Nfit = 0;
    int Npsf = 0;
    int Next = 0;
    int Nfail = 0;
    psScalar *scalar = NULL;

    pmReadout *readout  	   = job->args->data[0];
    psMetadata *recipe  	   = job->args->data[1];
    psArray *sources    	   = job->args->data[2];
    pmPSF *psf          	   = job->args->data[3];
    psArray *newSources 	   = job->args->data[4];
    pmSourceFitOptions *fitOptions = job->args->data[5];

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // S/N limit to perform full non-linear fits
    float FIT_SN_LIM = psMetadataLookupF32 (&status, recipe, "FULL_FIT_SN_LIM");

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion AnalysisRegion = psRegionForImage (readout->image, psRegionFromString (region));
    if (psRegionIsNaN (AnalysisRegion)) psAbort("analysis region mis-defined");

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

# if (PS_TRACE_ON)
# define TEST_X 653
# define TEST_Y 466
# define TESTING 0
	int TEST_ON = false;
	if (TESTING && (fabs(source->peak->xf - TEST_X) < 5) && (fabs(source->peak->yf - TEST_Y) < 5)) {
	    fprintf (stderr, "test object\n");
	    // psTraceSetLevel("psLib.math.psMinimizeLMChi2", 5);
	    TEST_ON = true;
	}
# undef TEST_X
# undef TEST_Y
# endif

        // skip non-astronomical objects (very likely defects)
        if (source->mode &  PM_SOURCE_MODE_BLEND) goto skip_blend;
        if (source->mode &  PM_SOURCE_MODE_CR_LIMIT) goto skip_cr;
        if (source->type == PM_SOURCE_TYPE_DEFECT) goto skip_defect;
        if (source->type == PM_SOURCE_TYPE_SATURATED) goto skip_sat;

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) goto skip_sat;

        // skip DBL second sources (ie, added by psphotFitBlob
        if (source->mode &  PM_SOURCE_MODE_PAIR) goto skip_blend;

	// do not include MOMENTS_FAILURES in the fit
        if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) goto skip_generic;

        // limit selection to some SN limit
        if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
	    if (source->moments->KronFlux < FIT_SN_LIM * source->moments->KronFluxErr) goto skip_generic;
	} else {
	    if (sqrt(source->peak->detValue) < FIT_SN_LIM) goto skip_generic;
	}
        // exclude sources outside optional analysis region
        if (source->peak->xf < AnalysisRegion.x0) goto skip_generic;
        if (source->peak->yf < AnalysisRegion.y0) goto skip_generic;
        if (source->peak->xf > AnalysisRegion.x1) goto skip_generic;
        if (source->peak->yf > AnalysisRegion.y1) goto skip_generic;

        // if model is NULL, we don't have a starting guess
        if (source->modelPSF == NULL) goto skip_generic;

        // skip sources which are insignificant flux?
        // XXX this is somewhat ad-hoc
        if (source->modelPSF->params->data.F32[1] < 0.1) {
            psTrace ("psphot", 5, "skipping near-zero source: %f, %f : %f\n",
                     source->modelPSF->params->data.F32[1],
                     source->modelPSF->params->data.F32[2],
                     source->modelPSF->params->data.F32[3]);
            goto skip_generic;
        }

        // replace object in image & remove excess noise
        if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
            pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
        }
        Nfit ++;

	if (0) {
	    psF32 Mxx = source->moments->Mxx;
	    psF32 Myy = source->moments->Myy;
	    fprintf (stderr, "1: Mxx: %f, Myy: %f\n", Mxx, Myy);
	}

        // try fitting PSFs or extended sources depending on source->mode
        // these functions subtract the resulting fitted source
        if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
            if (psphotFitBlob (readout, source, newSources, psf, fitOptions, maskVal, markVal)) {
# if (PS_TRACE_ON)
		if (TEST_ON) {
		  // psTraceSetLevel("psLib.math.psMinimizeLMChi2", 0);
		}
# endif
                psTrace ("psphot", 5, "source at %7.1f, %7.1f is ext", source->peak->xf, source->peak->yf);
                Next ++;
                continue;
            }
        } else {
            if (psphotFitBlend (readout, source, psf, fitOptions, maskVal, markVal)) {
# if (PS_TRACE_ON)
		if (TEST_ON) {
		  // psTraceSetLevel("psLib.math.psMinimizeLMChi2", 0);
		}
# endif
                source->type = PM_SOURCE_TYPE_STAR;
                psTrace ("psphot", 5, "source at %7.1f, %7.1f is psf", source->peak->xf, source->peak->yf);
                Npsf ++;
                continue;
            }
        }

	if (0) {
	    psF32 Mxx = source->moments->Mxx;
	    psF32 Myy = source->moments->Myy;
	    fprintf (stderr, "2: Mxx: %f, Myy: %f\n", Mxx, Myy);
	}

        psTrace ("psphot", 5, "source at %7.1f, %7.1f failed", source->peak->xf, source->peak->yf);
        Nfail ++;

        // re-subtract the object, leave local sky, re-bump noise
        pmSourceCacheModel (source, maskVal);
        pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
	continue;

    skip_blend:
        psTrace ("psphot", 5, "source at %7.1f, %7.1f skipped", source->peak->xf, source->peak->yf);
	continue;
    skip_cr:
        psTrace ("psphot", 5, "source at %7.1f, %7.1f skipped", source->peak->xf, source->peak->yf);
	continue;
    skip_defect:
        psTrace ("psphot", 5, "source at %7.1f, %7.1f skipped", source->peak->xf, source->peak->yf);
	continue;
    skip_sat:
        psTrace ("psphot", 5, "source at %7.1f, %7.1f skipped", source->peak->xf, source->peak->yf);
	continue;
    skip_generic:
        psTrace ("psphot", 5, "source at %7.1f, %7.1f skipped", source->peak->xf, source->peak->yf);
	continue;
    }

    // change the value of a scalar on the array (wrap this and put it in psArray.h)
    scalar = job->args->data[6];
    scalar->data.S32 = Nfit;

    scalar = job->args->data[7];
    scalar->data.S32 = Npsf;

    scalar = job->args->data[8];
    scalar->data.S32 = Next;

    scalar = job->args->data[9];
    scalar->data.S32 = Nfail;

    return true;
}

bool psphotBlendFitSetSource(pmSource *source, psImage *IDimage, float skySigma) {

    if (!source) return false;
    if (!source->peak) return false; // XXX how can we have a peak-less source?

# if (0)
# define TEST_X 653
# define TEST_Y 466
	if ((fabs(source->peak->xf - TEST_X) < 5) && (fabs(source->peak->yf - TEST_Y) < 5)) {
	    fprintf (stderr, "test object\n");
	}
# undef TEST_X
# undef TEST_Y
# endif

    if (!source->moments) return false;
    if (source->type == PM_SOURCE_TYPE_DEFECT) return false;
    if (source->type == PM_SOURCE_TYPE_SATURATED) return false;
    if (source->mode2 == PM_SOURCE_MODE2_MATCHED) return false;
    psAssert(IDimage, "need a window");

    if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) {
	// find the radius at which we hit something like 0.1*SATURATION
	psphotBlendSetSourceSatstar (IDimage, source);
	return true;
    }

    if (!isfinite(source->moments->Mrf) || source->moments->Mrf < 0 ) return false;

    // we have a source with moments Mx, My, Mxx, Mxy, Myy.  we just need to define a Gaussian that has 
    // these values (and peak of 1.0)

    int Nx = IDimage->numCols;
    int Ny = IDimage->numRows;

    float Xo = source->moments->Mx;
    float Yo = source->moments->My;

    psEllipseMoments moments;
    moments.x2 = source->moments->Mxx;
    moments.y2 = source->moments->Myy;
    moments.xy = source->moments->Mxy;

    psEllipseAxes axes = psEllipseMomentsToAxes(moments, 20.0);
    if (! (isfinite(axes.major) && isfinite(axes.minor) && isfinite(axes.theta)) ) {
        fprintf (stderr, "invalid axes found id: %4d major: %f minor: %f theta: %f\n", source->id, axes.major, axes.minor, axes.theta);
        return false;
    }

    // Use 1st radial moment as size, not sigma.  Why this factor of 0.5 ?
    float scale = 0.5 * source->moments->Mrf / axes.major;
    axes.major *= scale;
    axes.minor *= scale;

    psEllipseShape shape = psEllipseAxesToShape(axes);
    if (! (isfinite(shape.sx) && isfinite(shape.sy) && isfinite(shape.sxy)) ) {
        fprintf (stderr, "invalid shape found id: %d sx: %f sy %f sxy: %f\n", source->id, shape.sx, shape.sy, shape.sxy);
        return false;
    }

    float Smajor = axes.major;

    // we want to go to 2.15 sigma (0.1 Io)
    int minX = PS_MIN(PS_MAX(Xo - 2.15*Smajor, 0), Nx - 1);
    int maxX = PS_MIN(PS_MAX(Xo + 2.15*Smajor, 0), Nx - 1);
    int minY = PS_MIN(PS_MAX(Yo - 2.15*Smajor, 0), Ny - 1);
    int maxY = PS_MIN(PS_MAX(Yo + 2.15*Smajor, 0), Ny - 1);

    float rMxx = 0.5 / PS_SQR(shape.sx);
    float rMyy = 0.5 / PS_SQR(shape.sy);
    float Sxy = shape.sxy;    // factor of -1 is included to match the previous window function
    // implementation. XXX: Is this correct?

    int ID = source->id;
    float Io = source->peak->rawFlux;
    for (int iy = minY; iy < maxY; iy++) {
	for (int ix = minX; ix < maxX; ix++) {

	    float dX = (ix + 0.5 - Xo);
	    float dY = (iy + 0.5 - Yo);

	    float z = rMxx * PS_SQR(dX) + rMyy * PS_SQR(dY) + Sxy*dX*dY;
	    if (z > 2.311) continue;

	    float f = Io*exp(-z);
	    
	    if (f < 2.0*skySigma) continue;
            IDimage->data.S32[iy][ix] = ID;
	}
    }

    return true;
}

bool psphotBlendSetSourceSatstar (psImage *IDimage, pmSource *source) {

    float Xo = source->satstar->Xo;
    float Yo = source->satstar->Yo;
    psVector *logRmodel = source->satstar->logRmodel;
    psVector *logFmodel = source->satstar->logFmodel;

    float lPeak = logFmodel->data.F32[0];
    float fPeak = pow(10.0, lPeak);
    float threshold = 0.1*fPeak;
    float logThresh = log10(threshold);

    float radius = NAN;

    // what is the radius for a specific peak fraction?
    for (int i = 1; i < logFmodel->n; i++) {
	float logF = logFmodel->data.F32[i];
	if (!isfinite(logF)) continue;

	float flux = pow(10.0, logF);
	if (flux > threshold) continue;
	
	float logF0 = logFmodel->data.F32[i - 1];
	float logR0 = logRmodel->data.F32[i - 1];

	float logF1 = logFmodel->data.F32[i];
	float logR1 = logRmodel->data.F32[i];

	float logR = InterpolateValues (logF0, logR0, logF1, logR1, logThresh);
	radius = pow(10.0, logR);
	break;
    }

    if (!isfinite(radius)) {
	for (int i = logFmodel->n - 1; i >= 0; i--) {
	    if (!isfinite(logFmodel->data.F32[i])) continue;
	    float logR = logRmodel->data.F32[i];
	    radius = pow(10.0, logR);
	    break;
	}
    }

    if (!isfinite(radius)) return false;

    int Nx = IDimage->numCols;
    int Ny = IDimage->numRows;

    // region to mask
    int minX = PS_MIN(PS_MAX(Xo - radius, 0), Nx - 1);
    int maxX = PS_MIN(PS_MAX(Xo + radius, 0), Nx - 1);
    int minY = PS_MIN(PS_MAX(Yo - radius, 0), Ny - 1);
    int maxY = PS_MIN(PS_MAX(Yo + radius, 0), Ny - 1);

    int ID = source->id;

    fprintf (stderr, "Xo,Yo: %f,%f; radius: %f\n", Xo, Yo, radius);

    for (int iy = minY; iy < maxY; iy++) {
	for (int ix = minX; ix < maxX; ix++) {

	    float dX = (ix - Xo);
	    float dY = (iy - Yo);
	    float R = hypot (dX, dY) ;
	    if (R > radius) continue;

            IDimage->data.S32[iy][ix] = ID;
	}
    }
    return true;
}


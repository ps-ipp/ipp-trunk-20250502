# include "psphotInternal.h"

// fit flux (and optionally sky model) to all reasonable sources
// with the linear fitting process.  sources must have an associated
// model with selected pixels, and the fit radius must be defined

// given the set of sources, each of which points to the pixels in the
// science image, we construct a set of simulated sources with their own pixels.
// these are used to determine the simultaneous linear fit of fluxes.
// the analysis is performed wrt the simulated pixel values

static bool SetBorderMatrixElements (psSparseBorder *border, pmReadout *readout, psArray *sources, bool constant_weights, int SKY_FIT_ORDER, psImageMaskType markVal);

// for now, let's store the detections on the readout->analysis for each readout
bool psphotFitSourcesLinear (pmConfig *config, const pmFPAview *view, const char *filerule, bool final, bool skipNegativeFluxSources)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Fit Source (Linear) ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    pmSourceFitVarMode fitVarMode = psphotGetFitVarMode (recipe);
    if (!fitVarMode) {
	psError (PSPHOT_ERR_CONFIG, true, "failed to get LINEAR_FIT_VARIANCE_MODE");
	return false;
    }
    // MODEL_VAR requires 2 passes -- in the first, we get the rough fluxes; in the second, we
    // use the flux to define the model variance before fitting the objects.  Other modes only
    // do a single pass.
    pmSourceFitVarMode fitVarModePass1 = (fitVarMode == PM_SOURCE_PHOTFIT_MODEL_VAR) ? PM_SOURCE_PHOTFIT_CONST : fitVarMode;

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        if (psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
            psLogMsg ("psphot", PS_LOG_DETAIL, "skipping fit sources for input file %d", i);
            continue;
        }


        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->allSources;
        psAssert (sources, "missing sources?");

        pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
        psAssert (psf, "missing psf?");

        if (!psphotFitSourcesLinearReadout (recipe, readout, sources, psf, final, fitVarModePass1, skipNegativeFluxSources)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to fit sources (linear) for %s entry %d", filerule, i);
            return false;
        }

	// the MODEL_VAR weighting scheme requires knowledge of the model fluxes to generate the variance
	// after we have determined the initial set of fits, then we can generate the variance image and 
	// re-run the fit against that variance.
	if (fitVarMode == PM_SOURCE_PHOTFIT_MODEL_VAR) {
	    // generate the model variance image & source pointers
	    if (!psphotGenerateModelVariance (config, view, file, i, recipe, readout, sources)) {
		psError (PSPHOT_ERR_CONFIG, false, "failed to fit sources (linear) for %s entry %d", filerule, i);
		return false;
	    }

	    // replace all sources (use TMPF_SUBTRACTED as test)
	    psphotReplaceAllSourcesReadout (config, view, filerule, i, recipe, false);

	    // rerun fit with correct fitVarMode
            // XXX: does skipNegativeFlux work here?
	    if (!psphotFitSourcesLinearReadout (recipe, readout, sources, psf, final, fitVarMode, skipNegativeFluxSources)) {
		psError (PSPHOT_ERR_CONFIG, false, "failed to fit sources (linear) for %s entry %d", filerule, i);
		return false;
	    }

	    // free the model variance image & source pointers
	    if (!psphotFreeModelVariance (readout, sources)) {
		psError (PSPHOT_ERR_CONFIG, false, "failed to fit sources (linear) for %s entry %d", filerule, i);
		return false;
	    }
	}

	psphotVisualShowResidualImage (readout, (num > 0)); 
	psphotVisualShowPeaks (detections);
	psphotVisualShowObjectRegions (readout, recipe, sources);
    }
    return true;
}

// look up the fit variance mode from the recipe; older recipes do not have the value
// 'LINEAR_FIT_VARIANCE_MODE'; in those cases, look for 'CONSTANT_PHOTOMETRIC_WEIGHTS' as a boolean and
// set the value to either CONST or IMAGE_VAR
pmSourceFitVarMode psphotGetFitVarMode (psMetadata *recipe) {

    bool status = false;

    char *fitVarModeString = psMetadataLookupStr(&status, recipe, "LINEAR_FIT_VARIANCE_MODE");
    if (!status) {
	bool CONSTANT_PHOTOMETRIC_WEIGHTS = psMetadataLookupBool(&status, recipe, "CONSTANT_PHOTOMETRIC_WEIGHTS");
	if (!status) {
	    psAbort("You must provide a value for LINEAR_FIT_VARIANCE_MODE or CONSTANT_PHOTOMETRIC_WEIGHTS");
	}
	pmSourceFitVarMode fitVarMode = CONSTANT_PHOTOMETRIC_WEIGHTS ? PM_SOURCE_PHOTFIT_CONST : PM_SOURCE_PHOTFIT_IMAGE_VAR;
	return fitVarMode;
    } 
    if (!strcasecmp(fitVarModeString, "CONSTANT") || !strcasecmp(fitVarModeString, "CONST")) {
	return PM_SOURCE_PHOTFIT_CONST;
    }
    if (!strcasecmp(fitVarModeString, "IMAGE") || !strcasecmp(fitVarModeString, "IMAGE_VAR")) {
	return PM_SOURCE_PHOTFIT_IMAGE_VAR;
    }
    if (!strcasecmp(fitVarModeString, "SKY")   || !strcasecmp(fitVarModeString, "MODEL_SKY")) {
	return PM_SOURCE_PHOTFIT_MODEL_SKY;
    }
    if (!strcasecmp(fitVarModeString, "MODEL") || !strcasecmp(fitVarModeString, "MODEL_VAR")) {
	return PM_SOURCE_PHOTFIT_MODEL_VAR;
    }
    psError (PSPHOT_ERR_CONFIG, false, "Invalid value for LINEAR_FIT_VARIANCE_MODE (%s)", fitVarModeString);
    return PM_SOURCE_PHOTFIT_NONE;
}

bool psphotFitSourcesLinearReadout (psMetadata *recipe, pmReadout *readout, psArray *sources, pmPSF *psf, bool final, pmSourceFitVarMode fitVarMode, bool skipNegativeFluxSources) {
    bool status;
    float x;
    float y;
    float f;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping linear fit");
        return true;
    }

    psTimerStart ("psphot.linear");

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // source analysis is done in spatial order
    sources = psArraySort (sources, pmSourceSortByY);

    // storage array for fitSources
    psArray *fitSources = psArrayAllocEmpty (sources->n);

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion AnalysisRegion = psRegionFromString (region);
    AnalysisRegion = psRegionForImage (readout->image, AnalysisRegion);
    if (psRegionIsNaN (AnalysisRegion)) psAbort("analysis region mis-defined");

    int SKY_FIT_ORDER = psMetadataLookupS32(&status, recipe, "SKY_FIT_ORDER");
    if (!status) {
        SKY_FIT_ORDER = 0;
    }
    bool SKY_FIT_LINEAR = psMetadataLookupBool(&status, recipe, "SKY_FIT_LINEAR");
    if (!status) {
        SKY_FIT_LINEAR = false;
    }
    
    float MIN_VALID_FLUX = psMetadataLookupF32(&status, recipe, "PSF_FIT_MIN_VALID_FLUX");
    if (!status) {
        MIN_VALID_FLUX = 0.0;
    }
    if (skipNegativeFluxSources && MIN_VALID_FLUX < 0) {
        MIN_VALID_FLUX = 0.0;
    }
    float MAX_VALID_FLUX = psMetadataLookupF32(&status, recipe, "PSF_FIT_MAX_VALID_FLUX");
    if (!status) {
        MAX_VALID_FLUX = 1e+8;
    }

    float cutModelSum = psMetadataLookupF32(&status, recipe, "PSF_FIT_MODEL_SUM_FRAC_CUT");
    float cutMaskedSum = psMetadataLookupF32(&status, recipe, "PSF_FIT_MASKED_SUM_FRAC_CUT");

    // XXX test: choose a larger-than expected radius:
    float covarFactor = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "covariance factor: %f\n", covarFactor);

    // XXX do not apply covarFactor for the moment...
    // covarFactor = 1.0;

    bool use_covarfactor = psMetadataLookupBool(&status, recipe, "LINEAR_FIT_COVAR_FACTOR");
    if (!status) {
        use_covarfactor = true;
    }

    if (!use_covarfactor) {
        covarFactor = 1.0;
        psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "Manually reset covariance factor: %f\n", covarFactor);

    }

    int Nsat = 0;

    // track number kept at several stages of the analysis
    int Nstep_0 = 0;
    int Nstep_1 = 0;
    int Nstep_2 = 0;
    int Nstep_3 = 0;
    int Nstep_4 = 0;
    int Nstep_5 = 0;

    // select the sources which will be used for the fitting analysis
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

	// psAssert (source->peak, "source without peak??");
	// psAssert (source->peak->footprint, "peak without footprint??");

        if (final) {
            if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) continue;
        } else {
            // if (source->mode & PM_SOURCE_MODE_BLEND) continue;
        }

        // turn this bit off and turn it on again if we pass this test
        source->mode &= ~PM_SOURCE_MODE_LINEAR_FIT;

        // skip non-astronomical objects (very likely defects)
        if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
        if (source->type == PM_SOURCE_TYPE_SATURATED) continue;

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

        // do not include CRs in the full ensemble fit
        if (source->mode & PM_SOURCE_MODE_CR_LIMIT) continue;

	// do not include MOMENTS_FAILURES in the fit
        if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;

	Nstep_0++;

	// XXX count saturated stars
        if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    Nsat ++;
	}

        // generate model for sources without, or skip if we can't
        if (!source->modelFlux) {
            if (!pmSourceCacheModel (source, maskVal)) continue;
        }
	Nstep_1++;

        // save the original coords
        x = source->peak->xf;
        y = source->peak->yf;

        // is the source in the region of interest?
        if (x < AnalysisRegion.x0) continue;
        if (y < AnalysisRegion.y0) continue;
        if (x > AnalysisRegion.x1) continue;
        if (y > AnalysisRegion.y1) continue;
	Nstep_2++;

	// check the integral of the model : is it large enough?
	// apply mask?
	float modelSum = 0.0;
	float maskedSum = 0.0;
	for (int iy = 0; iy < source->modelFlux->numRows; iy++) {
	    for (int ix = 0; ix < source->modelFlux->numCols; ix++) {
		modelSum += source->modelFlux->data.F32[iy][ix];
		if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) continue;
		maskedSum += source->modelFlux->data.F32[iy][ix];
	    }
	}
        if (modelSum == 0 ||  maskedSum == 0) {
            // no pixels at all drop the modelPSF (probably only an issue for external sources
            // which are created from a fake peak (full force))
            psFree(source->modelPSF);
            continue;
        }
	Nstep_3++;

	bool isPSF = false;
        pmModel *model = pmSourceGetModel (&isPSF, source);

        float Io = model->params->data.F32[PM_PAR_I0];
        model->params->data.F32[PM_PAR_I0] = 1.0;
        float normFlux = model->class->modelFlux(model->params);
        model->params->data.F32[PM_PAR_I0] = Io;

        // printf("%5d %4.3f %4.3f %4.3f\n", source->seq, normFlux, modelSum, maskedSum);
        //float cut = .85;
        //if (modelSum  < cut * normFlux) continue;
        //if (maskedSum < cut * normFlux) continue;

        // MEH add the log comments back in as trace instead, psphot 5?  
        if (modelSum < 0.8*normFlux) {
           psTrace ("psphot", 5, "low-sig model @ %f, %f (%f masked sum, %f sum, %f normFlux, %f peak)\n",
                   source->peak->xf, source->peak->yf, maskedSum, modelSum, normFlux, source->peak->rawFlux);
        }
        if (maskedSum < 0.5*normFlux) {
           psTrace ("psphot", 5, "worrying model @ %f, %f (%f masked sum, %f sum, %f normFlux, %f peak)\n",
                   source->peak->xf, source->peak->yf, maskedSum, modelSum, normFlux, source->peak->rawFlux);
        } 
        if (modelSum  < cutModelSum * normFlux) continue;
	Nstep_4++;

        if (maskedSum < cutMaskedSum * normFlux) continue;
	Nstep_5++;

	// clear the 'mark' pixels and remask on the fit aperture 
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));
        psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, model->fitRadius, "OR", markVal);

	// we call this function multiple times. for the first time, we have only PSF models for all objects
	// the second time has extended sources.  If we ever fit the PSF model, we should raise this bit
	source->mode |= PM_SOURCE_MODE_LINEAR_FIT;
	if (isPSF) {
	    source->mode |= PM_SOURCE_MODE_PSFMODEL;
	}	    

        psArrayAdd (fitSources, 100, source);
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "built fitSources: %f sec (%ld objects)\n", psTimerMark ("psphot.linear"), sources->n);
    psLogMsg ("psphot.ensemble", PS_LOG_INFO, "Number of sources kept at each stage: %d %d %d %d %d %d\n", Nstep_0, Nstep_1, Nstep_2, Nstep_3, Nstep_4, Nstep_5);

    if (fitSources->n == 0) {
        psFree(fitSources);
        return true;
    }

    // vectors to store stats for each object
    // psVector *variance = psVectorAlloc (fitSources->n, PS_TYPE_F32);
    psVector *errors = psVectorAlloc (fitSources->n, PS_TYPE_F32);

    // create the border matrix (includes the sparse matrix)
    // for just sky: 1 row; for x,y terms: 3 rows
    psSparse *sparse = psSparseAlloc (fitSources->n, 100);
    int nBorder = (SKY_FIT_ORDER == 0) ? 1 : 3;
    psSparseBorder *border = psSparseBorderAlloc (sparse, nBorder);

    // if fitVarMode is MODEL_VAR, then we need to generate the model image variance
    // XXX we have two possibilities here: 

    // 1) do 2 passes, where in the first case we use the CONST weighting, and in the second
    // use the fitted model values to define the model

    // 2) do a single pass, and use the model guess to define the model variance (but do I trust the Model Guess?)

    // fill out the sparse matrix elements and border elements (B)
    // SRCi is the current source of interest
    // SRCj is a possibly overlapping source
    for (int i = 0; i < fitSources->n; i++) {
        pmSource *SRCi = fitSources->data[i];

        // diagonal elements of the sparse matrix (auto-cross-product)
        float MM = pmSourceModelDotModel (SRCi, SRCi, fitVarMode, covarFactor, maskVal);
        psSparseMatrixElement (sparse, i, i, MM);

        // if we have used CONSTANT errors, then we need to re-calculate the value of the parameter error
        if (fitVarMode != PM_SOURCE_PHOTFIT_IMAGE_VAR) {
            float var = pmSourceModelDotModel (SRCi, SRCi, PM_SOURCE_PHOTFIT_IMAGE_VAR, covarFactor, maskVal);
            errors->data.F32[i] = 1.0 / sqrt(var);
        } else {
            errors->data.F32[i] = 1.0 / sqrt(MM);
        }

        // find the image x model value
        float FM = pmSourceDataDotModel (SRCi, SRCi, fitVarMode, covarFactor, maskVal);
        psSparseVectorElement (sparse, i, FM);

        // add the per-source variances (border region)
        switch (SKY_FIT_ORDER) {
          case 1:
            f = pmSourceModelWeight (SRCi, 1, fitVarMode, covarFactor, maskVal);
            psSparseBorderElementB (border, i, 1, f);
            f = pmSourceModelWeight (SRCi, 2, fitVarMode, covarFactor, maskVal);
            psSparseBorderElementB (border, i, 2, f);

          case 0:
            f = pmSourceModelWeight (SRCi, 0, fitVarMode, covarFactor, maskVal);
            psSparseBorderElementB (border, i, 0, f);
            break;

          default:
            psAbort("Invalid SKY_FIT_ORDER %d\n", SKY_FIT_ORDER);
            break;
        }

        // loop over all other stars following this one
        for (int j = i + 1; j < fitSources->n; j++) {
            pmSource *SRCj = fitSources->data[j];

            // skip over disjoint source images, break after last possible overlap
            if (SRCi->pixels->row0 + SRCi->pixels->numRows < SRCj->pixels->row0) break;
            if (SRCj->pixels->row0 + SRCj->pixels->numRows < SRCi->pixels->row0) continue;
            if (SRCi->pixels->col0 + SRCi->pixels->numCols < SRCj->pixels->col0) continue;
            if (SRCj->pixels->col0 + SRCj->pixels->numCols < SRCi->pixels->col0) continue;

            // got an overlap; calculate cross-product and add to output array
            f = pmSourceModelDotModel (SRCi, SRCj, fitVarMode, covarFactor, maskVal);
            psSparseMatrixElement (sparse, j, i, f);
        }
    }

# if (0)
    static int npass = 0;
    char name[128];
    FILE *f1 = NULL;
    int fd = -1;

    snprintf (name, 128, "sparse.Aij.%02d.dat", npass);
    f1 = fopen (name, "w");
    psAssert (f1, "failed to open file\n");
    fd = fileno (f1);
    p_psVectorPrint (fd, sparse->Aij, "Aij");
    fclose (f1);

    snprintf (name, 128, "sparse.Bfj.%02d.dat", npass);
    f1 = fopen (name, "w");
    psAssert (f1, "failed to open file\n");
    fd = fileno (f1);
    p_psVectorPrint (fd, sparse->Bfj, "Bfj");
    fclose (f1);

    snprintf (name, 128, "sparse.Qii.%02d.dat", npass);
    f1 = fopen (name, "w");
    psAssert (f1, "failed to open file\n");
    fd = fileno (f1);
    p_psVectorPrint (fd, sparse->Qii, "Qii");
    fclose (f1);
    npass ++;
# endif

    psSparseResort (sparse);
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "built matrix: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    // set the sky, sky_x, sky_y components of border matrix
    SetBorderMatrixElements (border, readout, fitSources, (fitVarMode == PM_SOURCE_PHOTFIT_CONST), SKY_FIT_ORDER, markVal);

    psSparseConstraint constraint;
    constraint.paramMin   = MIN_VALID_FLUX;
    constraint.paramMax   = MAX_VALID_FLUX;
    constraint.paramDelta = 1e7;

    // solve for normalization terms (need include local sky?)
    psVector *norm = NULL;
    psVector *skyfit = NULL;
    if (SKY_FIT_LINEAR) {
        psSparseBorderSolve (&norm, &skyfit, constraint, border, 5);
        psLogMsg ("psphot", PS_LOG_MINUTIA, "skyfit: %f\n", skyfit->data.F32[0]);
    } else {
        norm = psSparseSolve (NULL, constraint, sparse, 5);
        skyfit = NULL;
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "solve matrix: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    // Philosophical question: we measure bright objects in three passes: 1) linear fit; 2)
    // non-linear fit; 3) linear fit: should we retain the chisq and errors from the
    // intermediate non-linear fit?  the non-linear fit provides better values for the position
    // errors, and for extended sources, the shape errors

    // adjust I0 for fitSources and subtract
    for (int i = 0; i < fitSources->n; i++) {
        pmSource *source = fitSources->data[i];
        pmModel *model = pmSourceGetModel (NULL, source);

        // assign linearly-fitted normalization
        if (isnan(norm->data.F32[i])) {
            psAbort("linear fitted source is nan");
        }

        model->params->data.F32[PM_PAR_I0] = norm->data.F32[i];
        model->dparams->data.F32[PM_PAR_I0] = errors->data.F32[i];

	if (norm->data.F32[i] < MIN_VALID_FLUX) {
	  fprintf (stderr, "fit out of bounds for %f,%f : %f\n", source->peak->xf, source->peak->yf, norm->data.F32[i]);
	  model->params->data.F32[PM_PAR_I0] = MIN_VALID_FLUX;
	}

	// clear the 'mark' pixels so the subtraction covers the full window
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

        // subtract object & add noise
        pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "sub models: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    // mean stats on fit windows
    float sumRadius = 0.0;
    float sumPixels = 0.0;
    float sumSource = 0.0;

    // measure chisq for each source
    // for (int i = 0; final && (i < fitSources->n); i++) {
    for (int i = 0; i < fitSources->n; i++) {
        pmSource *source = fitSources->data[i];
        pmModel *model = pmSourceGetModel (NULL, source);

	// accumulate fit windows statistics
        sumRadius += model->fitRadius;
	sumPixels += M_PI*PS_SQR(model->fitRadius);
	sumSource += 1.0;

        if (!(source->mode & PM_SOURCE_MODE_NONLINEAR_FIT)) {
	    model->nPar = 1; // LINEAR-only sources have 1 parameter; NONLINEAR sources have their original value
	}
        pmSourceChisq (model, source->pixels, source->maskObj, source->variance, maskVal);
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "get chisqs: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    float meanRadius = sumRadius / sumSource;
    float meanPixels = sumPixels / sumSource;

    // psFree (index);
    psFree (sparse);
    psFree (fitSources);
    psFree (norm);
    psFree (skyfit);
    psFree (errors);
    psFree (border);

    psLogMsg ("psphot.ensemble", PS_LOG_WARN, "measure ensemble of PSFs (mean radius = %f pixels, mean area = %f pixels: %f sec\n", meanRadius, meanPixels, psTimerMark ("psphot.linear"));

    psphotVisualPlotChisq (sources);
    // psphotVisualShowFlags (sources);

    // We have to place this visualization here because the models are not realized until
    // psphotGuessModels or fitted until psphotFitSourcesLinear.
    // psphotVisualShowPSFStars (recipe, psf, sources);

    return true;
}

// Calculate the weight terms for the sky fit component of the matrix.  This function operates
// on the pixels which correspond to all of the sources of interest.  These elements fill in
// the border matrix components in the sparse matrix equation.
static bool SetBorderMatrixElements (psSparseBorder *border, pmReadout *readout, psArray *sources, bool constant_weights, int SKY_FIT_ORDER, psImageMaskType markVal) {

    // generate the image-wide weight terms
    // turn on MARK for all image pixels
    psRegion fullArray = psRegionSet (0, 0, 0, 0);
    fullArray = psRegionForImage (readout->mask, fullArray);
    psImageMaskRegion (readout->mask, fullArray, "OR", markVal);

    // turn off MARK for all object pixels
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        pmModel *model = pmSourceGetModel (NULL, source);
        if (model == NULL) continue;
        float x = model->params->data.F32[PM_PAR_XPOS];
        float y = model->params->data.F32[PM_PAR_YPOS];
        psImageMaskCircle (source->maskView, x, y, model->fitRadius, "AND", PS_NOT_IMAGE_MASK(markVal));
    }

    // accumulate the image statistics from the masked regions
    psF32 **image  = readout->image->data.F32;
    psF32 **variance = readout->variance->data.F32;
    psImageMaskType  **mask   = readout->mask->data.PS_TYPE_IMAGE_MASK_DATA;

    double w, x, y, x2, xy, y2, xc, yc, wt, f, fo, fx, fy;
    w = x = y = x2 = xy = y2 = fo = fx = fy = 0;

    int col0 = readout->image->col0;
    int row0 = readout->image->row0;

    for (int j = 0; j < readout->image->numRows; j++) {
        for (int i = 0; i < readout->image->numCols; i++) {
            if (mask[j][i]) continue;
            if (constant_weights) {
                wt = 1.0;
            } else {
                wt = variance[j][i];
            }
            f = image[j][i];
            w   += 1/wt;
            fo  += f/wt;
            if (SKY_FIT_ORDER == 0) continue;

            xc  = i + col0;
            yc  = j + row0;
            x  +=    xc/wt;
            y  +=    yc/wt;
            x2 += xc*xc/wt;
            xy += xc*yc/wt;
            y2 += yc*yc/wt;
            fx +=  f*xc/wt;
            fy +=  f*yc/wt;
        }
    }

    // turn off MARK for all image pixels
    psImageMaskRegion (readout->mask, fullArray, "AND", PS_NOT_IMAGE_MASK(markVal));

    // set the Border T elements
    psSparseBorderElementG (border, 0, fo);
    psSparseBorderElementT (border, 0, 0, w);
    if (SKY_FIT_ORDER > 0) {
        psSparseBorderElementG (border, 0, fx);
        psSparseBorderElementG (border, 0, fy);
        psSparseBorderElementT (border, 1, 0, x);
        psSparseBorderElementT (border, 2, 0, y);
        psSparseBorderElementT (border, 0, 1, x);
        psSparseBorderElementT (border, 1, 1, x2);
        psSparseBorderElementT (border, 2, 1, xy);
        psSparseBorderElementT (border, 0, 2, y);
        psSparseBorderElementT (border, 1, 2, xy);
        psSparseBorderElementT (border, 2, 2, y2);
    }

    return true;
}

bool psphotModelBackgroundReadout(psImage *model,  // Model image
				  psImage *modelStdev, // Model stdev image
				  psMetadata *analysis, // Analysis metadata for outputs
				  pmReadout *readout, // Readout for which to generate a background model
				  psImageBinning *binning, // Binning parameters
				  const pmConfig *config,// Configuration
				  bool useVarianceImage
    );

 bool psphotGenerateModelVariance (pmConfig *config, const pmFPAview *view, pmFPAfile *file, int index, psMetadata *recipe, pmReadout *readout, psArray *sources) {

    bool status = false;
    psRegion fullRegion = psRegionSet (0, 0, 0, 0);

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // create a model variance image
    psImage *modelVar = psImageCopy (NULL, readout->variance, PS_TYPE_F32);

    // find the binning information
    psImageBinning *backBinning = psphotBackgroundBinning (modelVar, config);
    assert (backBinning);
    
    psImage *varModel = psImageAlloc(backBinning->nXruff, backBinning->nYruff, PS_TYPE_F32); // Background model
    psImage *varModelStdev = psImageAlloc(backBinning->nXruff, backBinning->nYruff, PS_TYPE_F32); // Background model

    // generate an image of the mean variance image in DN
    if (!psphotModelBackgroundReadout(varModel, varModelStdev, NULL, readout, backBinning, config, true)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to generate background model");
	psFree (varModel);
	psFree (varModelStdev);
	return false;
    }

    // linear interpolation to full-scale
    if (!psImageUnbin (modelVar, varModel, backBinning)) {
	psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for unbinning");
	psFree (varModel);
	psFree (varModelStdev);
	return false;
    }
    psFree (varModel);
    psFree (varModelStdev);

    float gain = 1.0;  // accept 1.0 as a default since it is not critical to the analysis
    pmCell *cell = readout->parent; // The parent cell
    if (cell) {
      gain = psMetadataLookupF32(&status, cell->concepts, "CELL.GAIN"); // Cell gain
      if (!status) {
	gain = 1.0;	      // set note above
      }
    }
    if (gain > 2.0) { /* warn? */ }
    // XXX we are not actually using the gain, but need to test it to avoid gcc pedantic warnings

    // insert all of the source models
    for (int i = 0; i < sources->n; i++) {

	// source of interest
	pmSource *source = sources->data[i];

	// skip sources which were not fitted already
	if (!(source->mode & PM_SOURCE_MODE_LINEAR_FIT)) continue;

	// pixel region appropriate for the source
	psRegion region = psRegionForImage (source->pixels, fullRegion);

	// define the source->modelVar pixels (view on modelVar image)
	psAssert (!source->modelVar, "programming error : modelVar should be NULL here");
	psAssert (source->modelFlux, "programming error : modelFlux should not be NULL here");
	psAssert (source->modelFlux->data.F32, "programming error : modelFlux should not be NULL here");
	source->modelVar = psImageSubset(modelVar, region);

	// add the source model to the model variance image
	// XXX note that this should be added with gain applied
	// var_DN = flux_DN / gain [e/DN]
	// to do this requires an API upgrade...
	pmSourceAdd (source, PM_MODEL_OP_MODELVAR, maskVal);
    }

    // we save the model variance for future reference
    psMetadataAddImage(readout->analysis, PS_LIST_TAIL, "PSPHOT.MODEL.VAR", PS_META_REPLACE, "model variance", modelVar);
    psFree (modelVar);

    return true;
}

bool psphotFreeModelVariance (pmReadout *readout, psArray *sources) {

    bool status = false;

    // find the binning information
    psImage *modelVar = psMetadataLookupPtr(&status, readout->analysis, "PSPHOT.MODEL.VAR");
    assert (modelVar);

    if (modelVar) {
      psMetadataRemoveKey (readout->analysis, "PSPHOT.MODEL.VAR"); 
    }

    // clear modelVar pointers for all of the source models
    for (int i = 0; i < sources->n; i++) {

	// source of interest
	pmSource *source = sources->data[i];
	psFree (source->modelVar);
    }

    return true;
}

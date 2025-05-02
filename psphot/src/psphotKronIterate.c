# include "psphotInternal.h"

bool psphotKronRadiusMeasure (pmSource *source, float radius, float minKronRadius, psImageMaskType maskVal, psImage *smoothedPixels);
bool psphotKronFluxMeasure (pmSource *source, psImageMaskType maskVal);


bool psphotKronIterate (pmConfig *config, const pmFPAview *view, const char *filerule, int pass) {
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Kron Iterate ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

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

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->newSources ? detections->newSources : detections->allSources;
        psAssert (sources, "missing sources?");

        pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
        // psAssert (psf, "missing psf?");

        if (!psphotKronIterateReadout (config, recipe, view, filerule, readout, sources, psf, i, pass)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

int psphotKapaChannel (int channel);
bool psphotVisualShowMask (int kapaFD, psImage *inImage, const char *name, int channel);
bool psphotVisualRangeImage (int kapaFD, psImage *inImage, const char *name, int channel, float min, float max);

#ifdef DUMP_KRS
FILE *dumpFile = NULL;
#endif

bool psphotKronIterateReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char * filerule, pmReadout *readout, psArray *sources, pmPSF *psf, int index, int pass) {

    bool status = false;

#ifdef DUMP_KRS
    if (!dumpFile) {
        dumpFile = fopen("kr.txt", "w");
        psAssert (dumpFile, "failed to open kr.txt");
    }
    fprintf(dumpFile, "\n\n Input %d\n", index);
#endif

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping masked kron");
        return true;
    }

    psTimerStart ("psphot.kron");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    float RADIUS = psMetadataLookupF32 (&status, readout->analysis, "PSF_MOMENTS_RADIUS");
    if (!status) {
        RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_MOMENTS_RADIUS");
    }

    float MIN_KRON_RADIUS = psMetadataLookupF32 (&status, readout->analysis, "MOMENTS_MIN_KRON");
    if (!status) {
        MIN_KRON_RADIUS = 0.25*RADIUS;
    }

    int KRON_ITERATIONS = psMetadataLookupS32 (&status, recipe, "KRON_ITERATIONS");
    if (!status) {
        KRON_ITERATIONS = 1;
    }

    bool KRON_SMOOTH = psMetadataLookupBool (&status, recipe, "KRON_SMOOTH");
    if (!status) {
        KRON_SMOOTH = false;
    }
    float KRON_SMOOTH_SIGMA = psMetadataLookupF32 (&status, recipe, "KRON_SMOOTH_SIGMA");
    if (!status) {
        KRON_SMOOTH_SIGMA = 1.7;
    }
    float KRON_SMOOTH_NSIGMA = psMetadataLookupS32 (&status, recipe, "KRON_SMOOTH_NSIGMA");
    if (!status) {
        KRON_SMOOTH_NSIGMA = 2;
    }
    /*
     *  Parameter for calculating maximum integration radius based on source's surface
     *  brightness
     *  Given minimum surface brightness SBmin and a flux the maximum radius is found from
     *
     *  SBmin = source->flux / (pi * Rmax**2)
     *  Rmax = sqrt (source->flux ) / sqrt (SBmin * pi)
     *
     *  Now what do we use for SBmin?
     *
     *  SBmin = ( some flux ) / (some area)
     *  some flux ~ flux of Ns sigma PSF source
     *  some area ~ K times area of a PSF
     *  
     * flux of Ns sigma source ~ Ns * SKY_STDEV * PSF_EFFECTIVE_AREA
     * PSF_EFFECTIVE_AREA = 4 pi sigma_PSF^2
     * (the 4 accounts for the fact that 1 sigma is not the total area, it is
     * actually a larger region).
     *
     *   SBmin = Ns * SKY_STDEV * 4 * pi * sigma_PSF^2 / (K * pi * sigma_PSF^2) 
     *         = Ns * SKY_STDEV * 4 / K
     *
     * We combine the two parameters Ns and K, and the constant 4 into a single recipe value
     * KRON_SB_MIN_FACTOR with Ns = 5 and K = 2 the corresponding value 
     * for KRON_SB_MIN_FACTOR is 5 * 4 / 2 = 10
     */
    float KRON_SB_MIN_FACTOR = psMetadataLookupF32 (&status, recipe, "KRON_SB_MIN_FACTOR");
    if (!status) {
        KRON_SB_MIN_FACTOR = 10;
    }
    float SKY_STDEV = psMetadataLookupF32 (&status, readout->analysis, "MSKY_DEV");
    float KRON_SB_MIN_DIVISOR = sqrt ( M_PI * KRON_SB_MIN_FACTOR * SKY_STDEV );

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);
    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping blend");
        return true;
    }

    // pass 3 we only measure fluxes for matched sources so we don't need smooth pixels or multiple iterations
    if (pass == 3) {
        KRON_SMOOTH = false;
        KRON_ITERATIONS = 1;
    }

# if (PS_TRACE_ON)
    fprintf (stderr, "--- starting KRON ---\n");
#endif

    // We measure the Kron Radius on a smoothed copy of the readout image
    psImage *smoothedImage = NULL;
    if (KRON_SMOOTH) {
        psTimerStart ("psphot.kron.smooth");

        // Build the smoothed source image
        // Replace the subtracted sources
        psphotReplaceAllSourcesReadout(config, view, filerule, index, recipe, false);
        psLogMsg ("psphot.kron", PS_LOG_INFO, "replaced %ld raw sources %f sec\n", sources->n, psTimerMark ("psphot.kron.smooth"));

        // smoothedImage = psImageCopy(NULL, readout->image, PS_TYPE_F32);
        // psImageSmooth(smoothedImage, KRON_SMOOTH_SIGMA, KRON_SMOOTH_NSIGMA);
	
        // Copy the image and smooth
        psTimerStart ("psphot.kron.smooth");
	bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve
        smoothedImage = psImageSmoothNoMask_Threaded (NULL, readout->image, KRON_SMOOTH_SIGMA, KRON_SMOOTH_NSIGMA, 0.25);
	psImageConvolveSetThreads(oldThreads);
        psLogMsg ("psphot.kron", PS_LOG_INFO, "smoothed image %f sec\n", psTimerMark ("psphot.kron.smooth"));

        // remove the sources
        psTimerStart ("psphot.kron.smooth");
        psphotRemoveAllSourcesReadout( config, view, filerule, index, recipe, false );
        psLogMsg ("psphot.kron", PS_LOG_INFO, "removed %ld raw sources %f sec\n", sources->n, psTimerMark ("psphot.kron.smooth"));

        // Now subtract smooth versions of the sources from the smoothed image
        psTimerStart ("psphot.kron.smooth.sources");
        for (int i=0; i< sources->n; i++) {
            pmSource *source = sources->data[i];
            // If source has been subtracted from the readout image subtract a "smoothed" version from the smoothedImage
	    if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
                // cache copy of smoothedPixels in the source
                // tmpPtr is for use by a single "module" and must be null otherwise
                psAssert(source->tmpPtr == NULL, "source->tmpPtr is not null!");

                psImage *smoothedPixels = psImageSubset(smoothedImage, source->region);
                source->tmpPtr = (psPtr) smoothedPixels;
                pmSourceSmoothOp(source, PM_MODEL_OP_FUNC, smoothedPixels, KRON_SMOOTH_SIGMA, false, maskVal, 0, 0);
            }
        }
        psLogMsg ("psphot.kron", PS_LOG_INFO, "removed %ld smoothed sources %f sec\n", sources->n, psTimerMark ("psphot.kron.smooth.sources"));

    }

    // threaded measurement of the source magnitudes
    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_KRON_ITERATE");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, smoothedImage);
            PS_ARRAY_ADD_SCALAR(job->args, markVal,            PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,            PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, RADIUS,             PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, MIN_KRON_RADIUS,    PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, KRON_ITERATIONS,    PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, KRON_SMOOTH_SIGMA,  PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, KRON_SB_MIN_DIVISOR,PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, pass,               PS_TYPE_S32);

// set this to 0 to run without threading
# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
# else
	    if (!psphotKronIterate_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		// psFree(AnalysisRegion);
		return false;
	    }
	    psFree(job);
# endif
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            }
            psFree(job);
        }
    }
    psFree (cellGroups);
    if (KRON_SMOOTH) {
        for (int i = 0; i < sources->n; i++) {
            pmSource *source = sources->data[i];
            psFree(source->tmpPtr);
            source->tmpPtr = NULL;
        }
    }
    psFree (smoothedImage);

    psLogMsg ("psphot.kron", PS_LOG_WARN, "measure masked kron magnitudes : %f sec for %ld objects\n", psTimerMark ("psphot.kron"), sources->n);
    return true;
}

bool psphotKronIterate_Threaded (psThreadJob *job) {

    pmReadout *readout              = job->args->data[0];
    psArray *sources                = job->args->data[1];
    psImage *smoothedImage          = job->args->data[2];
    psImageMaskType markVal         = PS_SCALAR_VALUE(job->args->data[3],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);
    float RADIUS                    = PS_SCALAR_VALUE(job->args->data[5],F32);
    float MIN_KRON_RADIUS           = PS_SCALAR_VALUE(job->args->data[6],F32);
    int KRON_ITERATIONS             = PS_SCALAR_VALUE(job->args->data[7],S32);
    float KRON_SMOOTH_SIGMA         = PS_SCALAR_VALUE(job->args->data[8],F32);
    float KRON_SB_MIN_DIVISOR       = PS_SCALAR_VALUE(job->args->data[9],F32);
    int pass                        = PS_SCALAR_VALUE(job->args->data[10],S32);

    bool measureRadius = true;
    if (pass == 3) {
        measureRadius = false;
    }

    for (int j = 0; j < KRON_ITERATIONS; j++) {
	for (int i = 0; i < sources->n; i++) {

	    pmSource *source = sources->data[i];
	    if (!source->peak) continue; // XXX how can we have a peak-less source?
            if (!source->moments) continue;
            if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
            if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	    // skip saturated stars modeled with a radial profile 
	    if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

            // in pass 3 we only measure the flux for matched sources
            if ((pass == 3) && !(source->mode2 & PM_SOURCE_MODE2_MATCHED)) continue;

# if (0)
# define TEST_X 653
# define TEST_Y 466
            if ((fabs(source->peak->xf - TEST_X) < 5) && (fabs(source->peak->yf - TEST_Y) < 5)) {
                fprintf (stderr, "test object\n");
            }
# undef TEST_X
# undef TEST_Y
# endif

            if (measureRadius) {
                // check status of this source's moments
                // XXX: I don't think that we have to apply these restrictions since we dropped the window function
                if (!(source->tmpFlags & PM_SOURCE_TMPF_MOMENTS_MEASURED)) continue;
                if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;
            }

            if (!isfinite(source->moments->Mrf)) {
                // Once we save a bad Mrf measurement we give up on this source
                // XXX: is this the right thing to do?
                continue;
            }

	    // replace object in image
	    bool reSubtract = false;
            psImage *smoothedPixels = NULL;
	    if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
                pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
                smoothedPixels = (psImage *) source->tmpPtr;
                if (smoothedPixels) {
                    pmSourceSmoothOp(source, PM_MODEL_OP_FUNC, smoothedPixels, KRON_SMOOTH_SIGMA, true, maskVal, 0, 0);
                }
		reSubtract = true;
	    }

            // On first iteration set window radius to sky radius (if valid). We also use this on subsequent
            // iterations if we cannot find a better limit
	    float maxWindow;
            if (measureRadius) {
                maxWindow = isfinite(source->skyRadius) ? source->skyRadius : RADIUS;
            } else {
                maxWindow = source->moments->Mrf;
            }
            if (j > 0) {
                // on subsequent iterations we use a factor times the previous radial moment value
                // limited to a maximum value that depends on the surface brightness of the source
                if (KRON_SB_MIN_DIVISOR) {
                    // Limit window radius based on surface brightness if we have a good measurement of kron flux
                    if (isfinite(source->moments->KronFlux)  && (source->moments->KronFlux > 0)) {
                        float Rmax = sqrt(source->moments->KronFlux) / KRON_SB_MIN_DIVISOR;

                        if (isfinite(source->moments->Mrf) && source->moments->Mrf > 0) {
                            maxWindow = PS_MIN(6.0*source->moments->Mrf, Rmax);
                        } else {
                            maxWindow = PS_MIN(Rmax, maxWindow);
                        }
                    }
                } else {
                    // old recipe, no surface brightness cut
                    maxWindow = isfinite(source->moments->Mrf) ? 6.0*source->moments->Mrf : RADIUS;
                }
            }
	    float windowRadius = PS_MAX(RADIUS, maxWindow);

	    // re-allocate image, weight, mask arrays for each peak with box big enough to fit BIG_RADIUS
	    bool extend = pmSourceRedefinePixels (source, readout, source->peak->x, source->peak->y, windowRadius + 2);
	    psAssert (source->pixels, "redefine pixels failed?");
            if (extend && smoothedPixels) {
                psFree(source->tmpPtr);
                smoothedPixels = psImageSubset(smoothedImage, source->region);
                psAssert (smoothedPixels, "redefine smoothed pixels failed?");
                source->tmpPtr = (psPtr) smoothedPixels ;
            }

            bool measureFlux = true;
            if (measureRadius) {
                measureFlux = psphotKronRadiusMeasure (source, windowRadius, MIN_KRON_RADIUS, maskVal, smoothedPixels);
            }

            if (measureFlux) {
                // Make sure the sources images are large enough for the measured Kron Radius
                bool extend = pmSourceRedefinePixels (source, readout, source->peak->x, source->peak->y, 
                    2.5 * source->moments->Mrf + 2);

                psAssert (source->pixels, "redefine pixels failed?");
                if (extend && smoothedPixels) {
                    psFree(source->tmpPtr);
                    smoothedPixels = psImageSubset(smoothedImage, source->region);
                    psAssert (smoothedPixels, "redefine smoothed pixels failed?");
                    source->tmpPtr = (psPtr) smoothedPixels ;
                }

                // this function populates moments->Mrf,KronFlux,KronFluxErr, KronFinner, and KronFouter
                psphotKronFluxMeasure (source, maskVal);
            }

            psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

#ifdef DUMP_KRS
            fprintf(dumpFile, "%7d %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %2d\n", source->id, source->moments->Mrf, MrfPrior, maxWindow, windowRadius, source->peak->xf, source->peak->yf, source->imageID);
#endif

	    // if we subtracted it above, re-subtract the object, leave local sky
	    if (reSubtract) {
                pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
                if (smoothedPixels) {
                    pmSourceSmoothOp(source, PM_MODEL_OP_FUNC, smoothedPixels, KRON_SMOOTH_SIGMA, false, maskVal, 0, 0);
                }
	    }
	}
    }

    return true;
}

bool psphotKronRadiusMeasure (pmSource *source, float radius, float minKronRadius, psImageMaskType maskVal,
    psImage *smoothedPixels) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_FLOAT_LARGER_THAN(radius, 0.0, false);

    psF32 R2 = PS_SQR(radius);

    // a note about coordinates: coordinates of objects throughout psphot refer to the primary
    // image coordinates.  the source->pixels image has an offset relative to its parent of
    // col0,row0: a pixel (x,y) in the primary image has coordinates of (x-col0, y-row0) in
    // this subimage.  we subtract off the peak coordinates, adjusted to this subimage, to have
    // minimal round-off error in the sums.  since these values are subtracted just to minimize
    // the dynamic range and are added back below, the exact value does not matter. these are
    // (int) so they can be used in the image index below.

    // Now calculate higher-order moments, using the above-calculated first moments to adjust coordinates
    // Xn  = SUM (x - xc)^n * (z - sky)

    psF32 RF = 0.0;
    psF32 RS = 0.0;

    // the peak position is less accurate but less subject to extreme deviations
    float dX = source->moments->Mx - source->peak->xf;
    float dY = source->moments->My - source->peak->yf;
    float dR = hypot(dX, dY);
    float Xo = (dR < 2.0) ? source->moments->Mx : source->peak->xf;
    float Yo = (dR < 2.0) ? source->moments->My : source->peak->yf;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    psF32 xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    psF32 yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    psF32 **vPix;
    if (smoothedPixels) {
        vPix = smoothedPixels->data.F32;
    } else {
        vPix = source->pixels->data.F32;
    }
    
    psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

    {
        for (psS32 row = 0; row < source->pixels->numRows ; row++) {

            psF32 yDiff = row - yCM;
            if (fabs(yDiff) > radius) continue;

            // coordinate of mirror pixel
            int yFlip = yCM - yDiff;
            if (yFlip < 0) continue;
            if (yFlip >= source->pixels->numRows) continue;

            for (psS32 col = 0; col < source->pixels->numCols ; col++) {
                // check mask and value for this pixel
                if (vMsk && (vMsk[row][col] & maskVal)) continue;
                if (isnan(vPix[row][col])) continue;

                psF32 xDiff = col - xCM;
                if (fabs(xDiff) > radius) continue;

                // coordinate of mirror pixel
                int xFlip = xCM - xDiff;
                if (xFlip < 0) continue;
                if (xFlip >= source->pixels->numCols) continue;

                // check mask and value for mirror pixel
                if (vMsk && (vMsk[yFlip][xFlip] & maskVal)) continue;
                if (isnan(vPix[yFlip][xFlip])) continue;

                // radius is just a function of (xDiff, yDiff)
                psF32 r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
                if (r2 > R2) continue;

                float fDiff1 = vPix[row][col];
                float fDiff2 = vPix[yFlip][xFlip];

                float pDiff = (fDiff1 > 0.0) ? sqrt(fabs(fDiff1*fDiff2)) : -sqrt(fabs(fDiff1*fDiff2));

                // Kron Flux uses the 1st radial moment
                psF32 rf = pDiff * sqrt(r2);
                psF32 rs = 0.5 * (fDiff1 + fDiff2);

                RF  += rf;
                RS  += rs;
            }
        }

        float MrfTry = RF/RS;
        if (RF <= 0. || RS <= 0 || !isfinite(MrfTry)) {
            // We did not get a good measurement
            source->moments->Mrf = NAN;
            source->moments->KronFlux  = NAN;
            source->moments->KronFluxErr  = NAN;
            source->moments->KronFinner = NAN;
            source->moments->KronFouter = NAN;
            return false;
        }

        float Mrf = MAX(minKronRadius, MrfTry);
        // Saturate the 1st radial moment
        if (sqrt(source->peak->detValue) < 10.0) {
            Mrf = MIN (radius, Mrf);
        }
        source->moments->Mrf = Mrf;
    }
    return true;
}

bool psphotKronFluxMeasure(pmSource *source, psImageMaskType maskVal) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);

    // a note about coordinates: coordinates of objects throughout psphot refer to the primary
    // image coordinates.  the source->pixels image has an offset relative to its parent of
    // col0,row0: a pixel (x,y) in the primary image has coordinates of (x-col0, y-row0) in
    // this subimage.  we subtract off the peak coordinates, adjusted to this subimage, to have
    // minimal round-off error in the sums.  since these values are subtracted just to minimize
    // the dynamic range and are added back below, the exact value does not matter. these are
    // (int) so they can be used in the image index below.

    // Now calculate higher-order moments, using the above-calculated first moments to adjust coordinates
    // Xn  = SUM (x - xc)^n * (z - sky)

    // the peak position is less accurate but less subject to extreme deviations
    float dX = source->moments->Mx - source->peak->xf;
    float dY = source->moments->My - source->peak->yf;
    float dR = hypot(dX, dY);
    float Xo = (dR < 2.0) ? source->moments->Mx : source->peak->xf;
    float Yo = (dR < 2.0) ? source->moments->My : source->peak->yf;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    psF32 xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    psF32 yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    psF32 **vPix = source->pixels->data.F32;
    psF32 **vWgt = source->variance->data.F32;
    
    psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

    source->moments->KronFlux  = NAN;
    source->moments->KronFluxErr  = NAN;
    source->moments->KronFinner = NAN;
    source->moments->KronFouter = NAN;

    // Calculate the Kron fluxes
    float radKinner = 1.0 * source->moments->Mrf;
    float radKron   = 2.5 * source->moments->Mrf;
    float radKouter = 4.0 * source->moments->Mrf;

    float limitRadius = MIN (radKouter, source->windowRadius);
    if (radKouter > source->windowRadius) {
        // This happens but the measurement isn't important enough to allocate the extra pixels
        // psWarning ("outer kron radius: %f is larger than windowRadius: %f for %d\n", 
        //     radKouter, source->windowRadius, source->id);
        limitRadius = MIN (radKron, source->windowRadius);
    }
    if (radKron > source->windowRadius) {
        // caller should have prevented this from happening
        psWarning ("kron radius: %f is larger than windowRadius: %f for %d\n", 
            radKron, source->windowRadius, source->id);
        return false;
    }

    float Sum = 0.0;
    float Var = 0.0;
    float SumInner = 0.0;
    float SumOuter = 0.0;

    // set vPix to the source pixels (it may have been set to the smoothed image above)
    vPix = source->pixels->data.F32;

    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	psF32 yDiff = row - yCM;
	if (fabs(yDiff) > limitRadius) continue;

	for (psS32 col = 0; col < source->pixels->numCols ; col++) {
	    // check mask and value for this pixel
	    if (vMsk && (vMsk[row][col] & maskVal)) continue;
	    if (isnan(vPix[row][col])) continue;

	    psF32 xDiff = col - xCM;
	    if (fabs(xDiff) > limitRadius) continue;

	    psF32 r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
            psF32 r = sqrt(r2);

	    float pDiff = vPix[row][col];
	    psF32 wDiff = vWgt[row][col];

            if (r > radKinner && r < radKron) {
                SumInner += pDiff;
            }
            if (r < radKron) {
                Sum += pDiff;
                Var += wDiff;
            }
            if (r > radKron && r < radKouter) {
                SumOuter += pDiff;
            }
	}
    }

    source->moments->KronFlux    = Sum;
    source->moments->KronFluxErr = sqrt(Var);
    source->moments->KronFinner  = SumInner;

    // only save radKouter if the radius is inside the integration radius limit
    if (radKouter <= limitRadius) {
        source->moments->KronFouter = SumOuter;
    }

    return true;
}

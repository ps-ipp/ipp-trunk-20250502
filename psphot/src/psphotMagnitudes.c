# include "psphotInternal.h"

bool psphotMagnitudes (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Magnitudes ---");

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

        psArray *sources = detections->allSources;
        psAssert (sources, "missing sources?");

        pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
        psAssert (psf, "missing psf?");

        if (!psphotMagnitudesReadout (config, recipe, view, readout, sources, psf, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotMagnitudesReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, pmReadout *readout, psArray *sources, pmPSF *psf, int index) {

    bool status = false;
    int Nap = 0;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source magnitudes");
        return true;
    }

    psTimerStart ("psphot.mags");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // if backModel or backStdev are missing, the values of sky and/or skyErr will be set to NAN
    pmReadout *backModel = psphotSelectBackground (config, view, index);
    pmReadout *backStdev = psphotSelectBackgroundStdev (config, view, index);

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    pmSourceMagnitudesInit (config, recipe);

    // the binning details are saved on the analysis metadata
    psImageBinning *binning = NULL;
    if (backModel) {
        binning = psMetadataLookupPtr(&status, backModel->analysis, "PSPHOT.BACKGROUND.BINNING");
    }

    bool IGNORE_GROWTH  = psMetadataLookupBool (&status, recipe, "IGNORE_GROWTH");
    bool INTERPOLATE_AP = psMetadataLookupBool (&status, recipe, "INTERPOLATE_AP");
    bool DIFF_STATS     = psMetadataLookupBool (&status, recipe, "DIFF_STATS");

    pmSourcePhotometryMode photMode = PM_SOURCE_PHOT_APCORR | PM_SOURCE_PHOT_WEIGHT;
    if (!IGNORE_GROWTH) photMode |= PM_SOURCE_PHOT_GROWTH;
    if (INTERPOLATE_AP) photMode |= PM_SOURCE_PHOT_INTERP;
    if (DIFF_STATS)     photMode |= PM_SOURCE_PHOT_DIFFSTATS;

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_MAGNITUDES");

            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, psf);
            psArrayAdd(job->args, 1, binning);
            psArrayAdd(job->args, 1, backModel);
            psArrayAdd(job->args, 1, backStdev);

            PS_ARRAY_ADD_SCALAR(job->args, photMode, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, 0,        PS_TYPE_S32); // this is used as a return value for nAp

            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
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
            } else {
                psScalar *scalar = job->args->data[8];
                Nap += scalar->data.S32;
            }
            psFree(job);
        }
    }

    psFree (cellGroups);

    psLogMsg ("psphot.magnitudes", PS_LOG_WARN, "measure magnitudes : %f sec for %ld objects (%d with apertures)\n", psTimerMark ("psphot.mags"), sources->n, Nap);
    return true;
}

bool psphotMagnitudes_Threaded (psThreadJob *job) {

    bool status;
    int Nap = 0;

    psArray *sources                = job->args->data[0];
    pmPSF *psf                      = job->args->data[1];
    psImageBinning *binning         = job->args->data[2];
    pmReadout *backModel            = job->args->data[3];
    pmReadout *backStdev            = job->args->data[4];
    pmSourcePhotometryMode photMode = PS_SCALAR_VALUE(job->args->data[5],S32);
    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[6],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal         = PS_SCALAR_VALUE(job->args->data[7],PS_TYPE_IMAGE_MASK_DATA);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

	bool saveTest = false;
	psImage *testImage = NULL;
# if (0)
	if ((fabs(source->peak->xf-3518) < 5) && (fabs(source->peak->yf-3178) < 5)) {
	    saveTest = true;
	    psRegion subregion = psRegionSet (source->peak->xf - 200, source->peak->xf + 200, source->peak->yf - 200, source->peak->yf + 200);
	    testImage = psImageSubset ((psImage *) source->pixels->parent, subregion);
	}
# endif

	if (saveTest) {
	    psphotSaveImage(NULL, testImage, "test.image.1.fits");
	}

	// satstars modeled as a radial profile need special handling
	if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) {
	    psphotSatstarPhotometry (source);
	    continue;
	}

        // replace object in image
        if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
            pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
        }

	if (saveTest) {
	    psphotSaveImage(NULL, testImage, "test.image.2.fits");
	}

	float xPos = source->peak->xf;
	float yPos = source->peak->yf;

	pmModel *model = source->modelPSF;
	if (model) {
	    xPos = model->params->data.F32[PM_PAR_XPOS];
	    yPos = model->params->data.F32[PM_PAR_YPOS];
	} else {
	    bool useMoments = pmSourcePositionUseMoments(source);
	    if (useMoments) {
		xPos = source->moments->Mx;
		yPos = source->moments->My;
	    }
	}

        // clear the mask bit and set the circular mask pixels
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));
        psImageKeepCircle (source->maskObj, xPos, yPos, source->apRadius, "OR", markVal);

        status = pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, source->apRadius);
        if (status && isfinite(source->apFlux)) {
	    Nap ++;
	} 

        // clear the mask bit
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

        // re-subtract the object, leave local sky
        pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

	if (saveTest) {
	    psphotSaveImage(NULL, testImage, "test.image.3.fits");
	}

        if (backModel) {
            psAssert (binning, "if backModel is defined, so should binning be");
            source->sky = psImageUnbinPixel(xPos, yPos, backModel->image, binning);
            if (isnan(source->sky) && false) {
                psLogMsg ("psphot.magnitudes", PS_LOG_DETAIL, "error setting pmSource.sky");
            }
        } else {
            source->sky = NAN;
        }

        if (backStdev) {
            psAssert (binning, "if backStdev is defined, so should binning be");
            source->skyErr = psImageUnbinPixel(xPos, yPos, backStdev->image, binning);
            if (isnan(source->skyErr) && false) {
                psLogMsg ("psphot.magnitudes", PS_LOG_DETAIL, "error setting pmSource.skyErr");
            }
        } else {
            source->skyErr = NAN;
        }
    }

    // change the value of a scalar on the array (wrap this and put it in psArray.h)
    psScalar *scalar = job->args->data[8];
    scalar->data.S32 = Nap;

    return true;
}

bool psphotPSFWeights(pmConfig *config, pmReadout *readout, const pmFPAview *view, psArray *sources) {

    bool status = false;

    psTimerStart ("psphot.mags");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }
    nThreads = 0; // XXX until testing is complete, do not thread this function

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_PSF_WEIGHTS");

            psArrayAdd(job->args, 1, cells->data[j]); // sources
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal,  PS_TYPE_IMAGE_MASK);

            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
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

    psLogMsg ("psphot", PS_LOG_DETAIL, "measure psf weights : %f sec for %ld objects\n", psTimerMark ("psphot.mags"), sources->n);
    return true;
}

bool psphotPSFWeights_Threaded (psThreadJob *job) {

    bool status;
    bool isPSF;

    psArray *sources                = job->args->data[0];
    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[1],PS_TYPE_IMAGE_MASK_DATA);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

        // we must have a valid model
        pmModel *model = pmSourceGetModel (&isPSF, source);
        if (model == NULL) {
          psTrace ("psphot", 3, "fail mag : no valid model");
          source->pixWeightNotBad = NAN;
          source->pixWeightNotPoor = NAN;
          continue;
        }

        status = pmSourcePixelWeight (source, model, source->maskObj, maskVal, source->apRadius);
        if (!status) {
          psTrace ("psphot", 3, "fail to measure pixel weight");
          source->pixWeightNotBad = NAN;
          source->pixWeightNotPoor = NAN;
          continue;
        }

    }

    return true;
}

# include "psphotInternal.h"

// NOTE : element 0 of fwhmValues if the unmatched image,  

int psphotStackMatchPSFsEntries (pmConfig *config, const pmFPAview *view, const char *filerule) {

    int nRadialEntries = 0;

    // find the numer of fwhmValues in the first non-skipped input
    int num = psphotFileruleCount(config, filerule);
    for (int i=0; i<num; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");
    
        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");
        bool status = false;
        if (!psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
            psVector *fwhmValues = psMetadataLookupVector(&status, readout->analysis, "STACK.PSF.FWHM.VALUES");
            if (fwhmValues) {
                nRadialEntries = fwhmValues->n;
            } else {
                nRadialEntries = 1;
            }
            break;
        }
    }
    return nRadialEntries;
}

// smooth the input image to match the next target PSF
// this function assumes the image has already been smoothed to match the first value (array element 0),
// and that the smoothing can use a 1D Gaussian kernel of width sqrt(TARGET^2 - CURRENT^2)
// each subsequent call
bool psphotStackMatchPSFsNext(pmConfig *config, const pmFPAview *view, const char *filerule, int lastSize)
{
    int num = psphotFileruleCount(config, filerule);

    // smooth the image and variance map
    bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotStackMatchPSFsNextReadout (config, view, filerule, i, lastSize)) {
	    psLogMsg ("psphot", PS_LOG_INFO, "failed to smooth image %s (%d) to target PSF", filerule, i);
	    psImageConvolveSetThreads(oldThreads);
	    return false;
	}
    }

    psImageConvolveSetThreads(oldThreads);
    return true;
}

bool psphotStackMatchPSFsNextReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, int lastSize) {

    bool status = false;

    psTimerStart ("psphot.smooth");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    if (psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
        psLogMsg("psphot", PS_LOG_INFO, "skipping smooth %d to next psf", index);
        return true;
    }

    psLogMsg("psphot", PS_LOG_INFO, "smooth %d to next psf", index);
    psphotVisualShowImage(readout);

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");
    psImageMaskType maskSat = psMetadataLookupImageMask(&status, recipe, "MASK.SAT"); // Mask value for bad pixels
    psAssert (maskSat, "missing mask value?");


    float minGauss = psMetadataLookupF32(NULL, recipe, "PEAKS_MIN_GAUSS"); // Minimum valid fraction of kernel
    if (!isfinite(minGauss)) {
        psWarning("PEAKS_MIN_GAUSS is not set in recipe; using default value");
        minGauss = 0.5;
    }

    psVector *fwhmValues = psMetadataLookupVector(&status, readout->analysis, "STACK.PSF.FWHM.VALUES");
    psAssert (fwhmValues, "need target PSFs");

    if (lastSize + 1 >= fwhmValues->n) {
	return true;
    }

    float targetFWHM = fwhmValues->data.F32[lastSize + 1];
    float currentFWHM = fwhmValues->data.F32[lastSize];

    if (targetFWHM <= currentFWHM) {
	// psError (PSPHOT_ERR_CONFIG, true, "target FWHM cannot be smaller than current FWHM");
	psLogMsg ("psphot", PS_LOG_INFO, "target FWHM (%f) is smaller than current FWHM (%f), not smoothing\n", targetFWHM, currentFWHM);
	fwhmValues->data.F32[lastSize + 1] = currentFWHM;
        pmReadoutMaskInvalid(readout, maskVal, maskSat);
	return false;
    }

    float smoothFWHM  = sqrt(PS_SQR(targetFWHM) - PS_SQR(currentFWHM));
    float SIGMA_SMTH  = smoothFWHM / (2.0*sqrt(2.0*log(2.0)));
    float NSIGMA_SMTH = 3.0;
    
    // record the actual smoothing sigma
    // psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SIGMA_SMOOTH", PS_META_REPLACE, "Smoothing sigma for detections", SIGMA_SMTH);

    // smooth the image, applying the mask as we go
    psImageSmoothMask_Threaded(readout->image, readout->image, readout->mask, maskVal, SIGMA_SMTH, NSIGMA_SMTH, minGauss);
    psLogMsg("psphot", PS_LOG_MINUTIA, "smooth image: %f sec\n", psTimerMark("psphot.smooth"));

    // Smooth the variance, applying the mask as we go.  The variance is smoothed by the PSF^2,
    // renomalized to maintain the input level of the variance.  We achieve this by smoothing
    // with a Gaussian with sigma = SIGMA_SMTH/sqrt(2) with unity normalization.  Note that
    // this process yields a smoothed image with correlated errors.  The pixel-to-pixel
    // variations in smooth_im will be decreased by a factor of 4*pi*SIGMA_SMTH^2, but for
    // measurements based on apertures comparable to or larger than the smoothing kernel, the
    // effective per-pixel variance is maintained.
    psImageSmoothMask_Threaded(readout->variance, readout->variance, readout->mask, maskVal, SIGMA_SMTH * M_SQRT1_2, NSIGMA_SMTH, minGauss);
    psLogMsg("psphot", PS_LOG_MINUTIA, "smooth variance: %f sec\n", psTimerMark("psphot.smooth"));

    // Insure that invalid pixels are masked
    // XXX: the smoothing seems to generate nan pixels in the variance image
    // XXX: We may need to loop over the cached sources and redefine the maskObj images...
    pmReadoutMaskInvalid(readout, maskVal, maskSat);
    {
        // Now go rebuild the sources' copies of the mask
        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->allSources;

        if (sources && sources->n) {
            for (int i = 0 ; i < sources->n; i++) {
                // XXX: move this to a function in pmSource.c
                pmSource *source = sources->data[i];
                if (source->maskObj && source->maskView) {
                    psFree(source->maskObj);
                    source->maskObj = psImageCopy (source->maskObj, source->maskView, PS_TYPE_IMAGE_MASK);
                }
            }
        }
    }

    psLogMsg("psphot", PS_LOG_INFO, "smoothed");
    psphotVisualShowImage(readout);

    // optionally save example images under trace
    if (psTraceGetLevel("psphot") > 5) {
	static int pass = 0;
        char name[64];
        sprintf (name, "stksm.v%d.fits", pass);
        psphotSaveImage(NULL, readout->image, name);
        sprintf (name, "stkwt.v%d.fits", pass);
        psphotSaveImage(NULL, readout->variance, name);
	pass ++;
    }

    // XXX need to apply this to the radial apertures somehow.
    // Calculate correction factor for the covariance produced by the (potentially multiple) smoothing
    // psKernel *kernel = psImageSmoothKernel(SIGMA_SMTH, NSIGMA_SMTH); // Kernel used for smoothing
    // double sum2 = 0.0;                                               // Sum of kernel squared
    // for (int y = kernel->yMin; y <= kernel->yMax; y++) {
    //     for (int x = kernel->xMin; x <= kernel->xMax; x++) {
    //         sum2 += PS_SQR(kernel->kernel[y][x]);
    //     }
    // }
    // float factor = 1.0 / (sum2 * psImageCovarianceCalculateFactor(kernel, readout->covariance));
    // psFree(kernel);

    // record the effective area and significance scaling factor
    float effArea = 8.0 * M_PI * PS_SQR(SIGMA_SMTH);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "EFFECTIVE_AREA", PS_META_REPLACE, "Effective Area", effArea);
    // psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SIGNIFICANCE_SCALE_FACTOR", PS_META_REPLACE, "Signicance scale factor", factor);

    // do not generate a PSF if we already were supplied one
    pmPSF *psfOld = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    if (psfOld) {
	// save PSF on readout->analysis
	char psfEntry[64];
	snprintf (psfEntry, 64, "PSPHOT.PSF.V%d", lastSize);
	if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, psfEntry, PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot psf model", psfOld)) {
	    psError (PSPHOT_ERR_UNKNOWN, false, "problem saving sources on readout");
	    return false;
	}
	psMetadataRemoveKey(readout->analysis, "PSPHOT.PSF");
    }

    return true;
}

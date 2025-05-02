# include "psphotInternal.h"

// In this function, we smooth the image and variance, then generate the significance image :
// (S/N)^2.  If FWMH_X,Y have been recorded, use them, otherwise use PEAKS_SMOOTH_SIGMA for the
// smoothing kernel.
pmReadout *psphotSignificanceImage (pmReadout *readout, psMetadata *recipe, psImageMaskType maskVal) {

    float SIGMA_SMTH, NSIGMA_SMTH;
    bool status = false;

    // smooth the image and variance map
    psTimerStart ("psphot.smooth");
    bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve

    // XXX we can a) choose fft to convolve if needed and b) multithread fftw

    float minGauss = psMetadataLookupF32(NULL, recipe, "PEAKS_MIN_GAUSS"); // Minimum valid fraction of kernel
    if (!isfinite(minGauss)) {
        psWarning("PEAKS_MIN_GAUSS is not set in recipe; using default value");
        minGauss = 0.5;
    }

    // NOTE: for a faint extended-source detection pass, we over-smooth by SOMETHING

    // if we have already determined the PSF model, then we have a better idea how to smooth this image
    bool statusMajor, statusMinor;
    float fwhmMajor = psMetadataLookupF32(&statusMajor, readout->analysis, "FWHM_MAJ");
    float fwhmMinor = psMetadataLookupF32(&statusMinor, readout->analysis, "FWHM_MIN");
    if (statusMajor && statusMinor) {
        // if we know the FHWM, use that to set the smoothing kernel (XXX allow an optional override?)
        if (!isfinite(fwhmMajor) || !isfinite(fwhmMinor) || fwhmMajor == 0.0 || fwhmMinor == 0.0) {
            psWarning("fwhmMajor (%f) or fwhmMinor (%f) is bad!", fwhmMajor, fwhmMinor);
        }
        SIGMA_SMTH  = 0.5*(fwhmMajor + fwhmMinor) / (2.0*sqrt(2.0*log(2.0)));
        NSIGMA_SMTH = psMetadataLookupF32 (&status, recipe, "PEAKS_SMOOTH_NSIGMA");
    } else {
        // if we do not know the FWHM, use the guess smoothing kernel supplied.
        // it is a configuration error if these are not supplied
        SIGMA_SMTH  = psMetadataLookupF32 (&status, recipe, "PEAKS_SMOOTH_SIGMA");
        PS_ASSERT (status, NULL);
        NSIGMA_SMTH = psMetadataLookupF32 (&status, recipe, "PEAKS_SMOOTH_NSIGMA");
        PS_ASSERT (status, NULL);
    }
    // record the actual smoothing sigma
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SIGMA_SMOOTH", PS_META_REPLACE, "Smoothing sigma for detections", SIGMA_SMTH);

    // smooth the image, applying the mask as we go
    psImage *smooth_im = psImageCopy(NULL, readout->image, PS_TYPE_F32);
    psImageSmoothMask_Threaded(smooth_im, smooth_im, readout->mask, maskVal, SIGMA_SMTH, NSIGMA_SMTH, minGauss);
    psLogMsg("psphot", PS_LOG_MINUTIA, "smooth image: %f sec\n", psTimerMark("psphot.smooth"));

    // Smooth the variance, applying the mask as we go.  The variance is smoothed by the PSF^2,
    // renomalized to maintain the input level of the variance.  We achieve this by smoothing
    // with a Gaussian with sigma = SIGMA_SMTH/sqrt(2) with unity normalization.  Note that
    // this process yields a smoothed image with correlated errors.  The pixel-to-pixel
    // variations in smooth_im will be decreased by a factor of 4*pi*SIGMA_SMTH^2, but for
    // measurements based on apertures comparable to or larger than the smoothing kernel, the
    // effective per-pixel variance is maintained.
    psImage *smooth_wt = psImageCopy(NULL, readout->variance, PS_TYPE_F32);
    psImageSmoothMask_Threaded(smooth_wt, smooth_wt, readout->mask, maskVal, SIGMA_SMTH * M_SQRT1_2, NSIGMA_SMTH, minGauss);
    psLogMsg("psphot", PS_LOG_MINUTIA, "smooth variance: %f sec\n", psTimerMark("psphot.smooth"));

    psImage *mask = readout->mask;

    // optionally save example images under trace
    // XXX change these to recipe value checks
    if (psTraceGetLevel("psphot") > 5) {
	static int pass = 0;
        char name[64];
        sprintf (name, "imsmooth.v%d.fits", pass);
        psphotSaveImage(NULL, smooth_im, name);
        sprintf (name, "wtsmooth.v%d.fits", pass);
        psphotSaveImage(NULL, smooth_wt, name);
	pass ++;
    }

    // Calculate correction factor for the covariance produced by the (potentially multiple) smoothing
    psKernel *kernel = psImageSmoothKernel(SIGMA_SMTH, NSIGMA_SMTH); // Kernel used for smoothing
    double sum2 = 0.0;                                               // Sum of kernel squared
    for (int y = kernel->yMin; y <= kernel->yMax; y++) {
        for (int x = kernel->xMin; x <= kernel->xMax; x++) {
            sum2 += PS_SQR(kernel->kernel[y][x]);
        }
    }
    float factor = 1.0 / (sum2 * psImageCovarianceCalculateFactor(kernel, readout->covariance));
    psFree(kernel);

    // record the effective area and significance scaling factor
    float effArea = 8.0 * M_PI * PS_SQR(SIGMA_SMTH);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "EFFECTIVE_AREA", PS_META_REPLACE, "Effective Area", effArea);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SIGNIFICANCE_SCALE_FACTOR", PS_META_REPLACE, "Signicance scale factor", factor);

    // we are going to return both the image and the weight here: the image contains the signal
    // while the 'weight' will contain the significance (NOTE the deviation from the usual
    // definition)

    // save the smoothed significance image in the weight array
    for (int j = 0; j < smooth_im->numRows; j++) {
        for (int i = 0; i < smooth_im->numCols; i++) {
            float value = smooth_im->data.F32[j][i];
            if (value < 0 || smooth_wt->data.F32[j][i] <= 0 || (mask->data.PS_TYPE_IMAGE_MASK_DATA[j][i] & maskVal)) {
                smooth_wt->data.F32[j][i] = 0.0;
            } else {
	      // XXX the value of 100 here (or 1000 before) must depend on the FWHM of the smoothing kernel, right??
	      //	      float v2 = value + PS_SQR(value/100.0);
	      // CZW 2013-06-20: I don't think this hack was helping.
	      // EAM 2021-12-21: see note below about divots in the significance image
	      float v2 = value;
	      smooth_wt->data.F32[j][i] = factor * PS_SQR(v2) / smooth_wt->data.F32[j][i];
            }
        }
    }
    psLogMsg ("psphot", PS_LOG_INFO, "built smoothed signficance image: %f sec\n", psTimerMark("psphot.smooth"));

    // optionally save example images under trace
    // XXX change these to recipe value checks
    if (psTraceGetLevel("psphot") > 5) {
        char name[64];
	static int pass = 0;
        sprintf (name, "snsmooth.v%d.fits", pass);
        psphotSaveImage (NULL, smooth_wt, name);
	pass ++;
    }
    psImageConvolveSetThreads(oldThreads);

    // We now have the significance image and the signal image.  In some cases (e.g.,
    // stacks), the variance on pixels in the cores of stars is elevated compared to pure
    // poisson statistics.  In this case, especially at high signal levels, the ratio of
    // signal / noise in the core of the star can be lower than the surrounding ring of
    // pixels.  This results in a divot in the center of the star in the significance
    // image.  the apparent peak of the significance is then not centered on the star and
    // chaos ensues.  A possible fix is to use the signal image for signficance for
    // the high S/N detection pass.

    pmReadout *significanceRO = pmReadoutAlloc(NULL);
    significanceRO->variance = smooth_wt;    
    significanceRO->image = smooth_im;    

    return significanceRO;
}

# if (0)
{
    // threadingdemo
    // smooth the image, applying the mask as we go
    psImage *smooth_im = psImageCopy(NULL, readout->image, PS_TYPE_F32);
    psImageSmoothMask(smooth_im, smooth_im, readout->mask, maskVal, SIGMA_SMTH, NSIGMA_SMTH, minGauss);

    psLogMsg("psphot", PS_LOG_INFO, "smooth image: %f sec\n", psTimerMark("psphot.smooth"));

    psImageConvolveSetThreads(true);

    psTimerStart ("psphot.smooth");

    // XXX a quick test of the threaded version of the function:
    psImage *smooth_test = psImageCopy(NULL, readout->image, PS_TYPE_F32);
    psImageSmoothMask_Threaded(smooth_test, smooth_test, readout->mask, maskVal, SIGMA_SMTH, NSIGMA_SMTH, minGauss);

    psLogMsg("psphot", PS_LOG_INFO, "smooth_threaded image: %f sec\n", psTimerMark("psphot.smooth"));

    psphotSaveImage (NULL, smooth_im, "smooth_im.fits");
    psphotSaveImage (NULL, smooth_test, "smooth_test.fits");
    exit (0);
}
# endif

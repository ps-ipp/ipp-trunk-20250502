# include "psphotInternal.h"

#define MODEL_MASK (PM_MODEL_STATUS_NONCONVERGE | PM_MODEL_STATUS_OFFIMAGE | \
                    PM_MODEL_STATUS_BADARGS | PM_MODEL_STATUS_LIMITS) // Mask to apply to models

// Calculate the limiting magnitude for an image
//
// We limit ourselves to calculating the peak flux in the smoothed image, which should be close (modulo
// non-Gaussian PSF) to the limiting flux in the un-smoothed original image.
static bool effLimit(float *magLim,           // Limiting magntiude, to return
                     int *radius,             // Radius for fake sources, to return
                     float *minFlux,          // Minimum flux for fake sources, to return
                     float *norm,             // Normalisation of PSF (conversion: peak --> integrated flux)
                     float *covarFactor,// Covariance factor
                     const pmReadout *ro,     // Readout of interest
                     pmPSF *psf,              // Point-spread function
                     float thresh,            // Threshold for source identification
                     float smoothSigma,       // Gaussian smoothing sigma
                     float smoothNsigma,      // Smoothing limit
                     psImageMaskType maskVal  // Value to mask
                     )
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_VARIANCE(ro, false);
    assert(magLim);
    assert(radius);
    assert(minFlux);
    assert(norm);
    assert(covarFactor);

    // apply the amplitude of kernel^2 to the covarFactor (why?)
    // XXX this is simply undoing the scale scalculate in psImageCovarianceCalculateFactor
    // if this is the right solution, make this extra calculation optional (and explain...)
    psKernel *kernel = psImageSmoothKernel(smoothSigma, smoothNsigma); // Kernel used for smoothing
    double sum2 = 0.0;                                               // Sum of kernel squared
    for (int y = kernel->yMin; y <= kernel->yMax; y++) {
        for (int x = kernel->xMin; x <= kernel->xMax; x++) {
            sum2 += PS_SQR(kernel->kernel[y][x]);
        }
    }
    *covarFactor = sum2 * psImageCovarianceCalculateFactor(kernel, ro->covariance);
    psFree(kernel);

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for variance
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);        // Random number generator
    if (!psImageBackground(stats, NULL, ro->variance, ro->mask, maskVal, rng) ||
        !isfinite(stats->robustMedian)) {
        psError(PSPHOT_ERR_DATA, false, "Unable to determine mean variance");
        psFree(stats);
        psFree(rng);
        return false;
    }
    psFree(rng);
    float meanVar = stats->robustMedian; // Mean variance
    psFree(stats);

#ifndef USE_SINGLE_POINT_FOR_MODEL
    // Need to normalise out difference between Gaussian and real PSF
    int sizeX = ro->variance->numCols / 16;
    int sizeY = ro->variance->numRows / 16;
    int numPoints = 0;
    float sum =  0;
    for (int y = sizeY * 0.5 ; y < ro->variance->numRows; y += sizeY) {
        for (int x = sizeX * 0.5 ; x < ro->variance->numCols; x += sizeX) {
            pmModel *normModel = pmModelFromPSFforXY(psf, (float) x, (float) y, 1.0); // model for normalization

            if (!normModel || (normModel->flags & MODEL_MASK)) {
                psFree(normModel);
                continue;
            }
            float flux = normModel->class->modelFlux(normModel->params); // Total flux for peak of 1.0
            psFree(normModel);
            if (!isfinite(flux)) {
                continue;
            }
            numPoints++;
            sum += flux;
        }
    }
    if (!isfinite(sum) || numPoints == 0) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate PSF model for any of %d points.", sizeX * sizeY);
        return false;
    }
    *norm = sum / numPoints;
#else
    // This can fail for readout that has few good pixels. It's better to sample many points.
    pmModel *normModel = pmModelFromPSFforXY(psf, 0.5 * ro->variance->numCols,
                                         0.5 * ro->variance->numRows, 1.0); // Model for normalisation
    psFree(normModel);
    if (!normModel || (normModel->flags & MODEL_MASK)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate PSF model.");
        psFree(normModel);
        return false;
    }
    *norm = normModel->modelFlux(normModel->params); // Total flux for peak of 1.0
    psFree(normModel);
#endif

    // The signal-to-noise of the smoothed image is: S/N ~ I_smooth / sqrt(variance * factor)
    // since the variance factor tells us the variance in the smoothed image.  Now, the trick is working
    // out what the intensity in the smoothed image is, and how it is related to the flux.  We are
    // convolving Io G_* with G_PSF/2pi.w^2, where G_* is the approximately-Gaussian PSF of the star,
    // G_PSF is the pretty-close-matching Gaussian of the convolution kernel, Io is the peak flux in the
    // unsmoothed image, and w is the width of the Gaussian.  Now, a normalised 2D Gaussian convolved
    // with itself has a normalisation of 1/2pi(w^2+w^2) = 1/4pi.w^2.  Therefore:
    // I_smooth = Flux / 2*norm.
    float peakLim = thresh * sqrtf(meanVar * *covarFactor); // Limiting peak value in smoothed image
    float fluxLim = 2.0 * *norm * peakLim; // Limiting flux in original
    *magLim = -2.5 * log10f(fluxLim);
    psTrace("psphot.fake", 1, "Covar Factor:  %f\n", *covarFactor);
    psTrace("psphot.fake", 1, "Limiting peak: %f\n", peakLim);
    psTrace("psphot.fake", 1, "Limiting flux: %f\n", fluxLim);
    psTrace("psphot.fake", 1, "Limiting mag: %f\n", *magLim);
    psLogMsg("psphot", PS_LOG_INFO,
             "Detection efficiency:\n"
             "  Mean variance: %f * %f\n"
             "  Threshold: %f\n"
             "  Normalisation: %f\n"
             "  Limiting magnitude: %f\n",
             meanVar, *covarFactor, thresh, *norm, *magLim);

    *radius = 5 * smoothSigma * smoothNsigma;

    *minFlux = 0.1 * sqrtf(meanVar);

    psLogMsg ("psphot", PS_LOG_INFO, "Detection efficiency PSF radius: %d, minFlux: %f\n", *radius, *minFlux);

    return true;
}

/// Generate a fake image and add it in to the existing readout
static pmReadout *effGenerate(psImage **xSrc, psImage **ySrc, // Positions of sources
                        const pmReadout *ro,            // Readout of interest
                        const pmPSF *psf,               // Point-spread function
                        const psVector *magOffsets,     // Magnitude offsets for fake sources
                        int numSources,                 // Number of fake sources for each bin
                        float refMag,                   // Reference magnitude
                        int radius,                     // Radius for fake sources
                        float minFlux                   // Minimum flux for fake sources
                        )
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);
    PS_ASSERT_VECTOR_NON_NULL(magOffsets, false);
    PS_ASSERT_VECTOR_TYPE(magOffsets, PS_TYPE_F32, false);
    assert(xSrc);
    assert(ySrc);

    int numBins = magOffsets->n;                                    // Number of bins
    int numCols = ro->image->numCols, numRows = ro->image->numRows; // Size of image

    *xSrc = psImageRecycle(*xSrc, numSources, numBins, PS_TYPE_F32);
    *ySrc = psImageRecycle(*ySrc, numSources, numBins, PS_TYPE_F32);

    psVector *xAll = psVectorAlloc(numBins * numSources, PS_TYPE_F32);
    psVector *yAll = psVectorAlloc(numBins * numSources, PS_TYPE_F32);
    psVector *magAll = psVectorAlloc(numBins * numSources, PS_TYPE_F32);

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    for (int i = 0, index = 0; i < numBins; i++) {
        float mag = refMag + magOffsets->data.F32[i]; // Instrumental magnitude of sources

        for (int j = 0; j < numSources; j++, index++) {
            xAll->data.F32[index] = psRandomUniform(rng) * numCols;
            yAll->data.F32[index] = psRandomUniform(rng) * numRows;
            magAll->data.F32[index] = mag;
        }
        memcpy((*xSrc)->data.F32[i], &xAll->data.F32[i * numSources],
               numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        memcpy((*ySrc)->data.F32[i], &yAll->data.F32[i * numSources],
               numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    }
    psFree(rng);

    bool oldThreads = pmReadoutFakeThreads(true); // Old threading status

    pmReadout *fakeRO = pmReadoutAlloc(NULL); // Fake readout
    if (!pmReadoutFakeFromVectors(fakeRO, numCols, numRows, xAll, yAll, magAll,
                                  NULL, NULL, psf, minFlux, radius, false, true)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake image");
        psFree(fakeRO);
        psFree(magAll);
        psFree(xAll);
        psFree(yAll);
        return NULL;
    }
    psFree(magAll);
    psFree(xAll);
    psFree(yAll);

    pmReadoutFakeThreads(oldThreads);

    psBinaryOp(ro->image, ro->image, "+", fakeRO->image);

    // return the readout so we can subtract it later
    return fakeRO;
}

bool psphotEfficiency (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Efficiency ---");

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
	if (!psphotEfficiencyReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure detection efficiency for %s entry %d", filerule, i);
            psErrorStackPrint(stderr, " ");
            psErrorClear();
            // send message to log as well
            psLogMsg ("psphot", PS_LOG_WARN, "failed to measure detection efficiency for %s entry %d", filerule, i);
        }
    }
    return true;
}

// Determine detection efficiency
bool psphotEfficiencyReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    bool status = true;

    psTimerStart("psphot.fake");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *realSources = detections->allSources;
    psAssert (realSources, "missing sources?");

    // XXX do we need to skip this step if we do not have a psf?
    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);
    PS_ASSERT_PTR_NON_NULL(psf, false);
    PS_ASSERT_METADATA_NON_NULL(recipe, false);

    // Collect recipe information
    float smoothNsigma = psMetadataLookupF32(&status, recipe, "PEAKS_SMOOTH_NSIGMA"); // Smoothing limit
    psAssert (status && isfinite(smoothNsigma), "Unable to find PEAKS_SMOOTH_NSIGMA in recipe (or invalid value)");

    float thresh = psMetadataLookupF32(&status, recipe, "PEAKS_NSIGMA_LIMIT_2");
    psAssert (status && isfinite(thresh), "Unable to find PEAKS_NSIGMA_LIMIT_2 in recipe (or invalid value)");

    psVector *magOffsets = psMetadataLookupVector(&status, recipe, "EFF.MAG"); // Magnitude offsets
    psAssert (status, "Unable to find EFF.MAG F32 vector in recipe");
    psAssert (magOffsets->type.type == PS_TYPE_F32, "Unable to find EFF.MAG F32 vector in recipe");

    int numSources = psMetadataLookupS32(&status, recipe, "EFF.NUM"); // Number of sources for each bin
    psAssert (status && (numSources > 0), "Unable to find EFF.NUM in recipe (or invalid value)");

    float minGauss = psMetadataLookupF32(&status, recipe, "PEAKS_MIN_GAUSS"); // Minimum valid fraction of kernel
    psAssert (status && isfinite(minGauss), "PEAKS_MIN_GAUSS is not set in recipe (or invalid)");

    // find the PSF size information (why is this not part of the psf structure?)
    float fwhmMajor = psMetadataLookupF32(NULL, readout->analysis, "FWHM_MAJ"); // PSF size in x
    float fwhmMinor = psMetadataLookupF32(NULL, readout->analysis, "FWHM_MIN"); // PSF size in y
    if (!isfinite(fwhmMajor) || !isfinite(fwhmMinor) || fwhmMajor == 0.0 || fwhmMinor == 0.0) {
        psError(PSPHOT_ERR_CONFIG, false, "Unable to find FWHM_MAJ and FWHM_MIN in readout->analysis");
        return false;
    }

    float smoothSigma = 0.5*(fwhmMajor + fwhmMinor) / (2.0*sqrtf(2.0*log(2.0))); // Gaussian smoothing sigma
    int numCols = readout->image->numCols, numRows = readout->image->numRows; // Size of image
    int numBins = magOffsets->n;                          // Number of bins

    psImageMaskType maskVal = psMetadataLookupImageMask(NULL, recipe, "MASK.PSPHOT"); // Value to mask

    // remove all sources, adding noise for subtracted sources
    psphotRemoveAllSourcesByArray(realSources, recipe);

# define TESTING 0
#if TESTING
    {
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                readout->image->data.F32[y][x] = psRandomGaussian(rng);
                readout->variance->data.F32[y][x] = 1.0;
                readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = 0;
            }
        }
        psFree(readout->covariance);
        readout->covariance = NULL;
        psFree(rng);
    }
#endif



    float magLim;                       // Guess at limiting magnitude
    int radius;                         // Radius for fake sources
    float minFlux;                      // Minimum flux for fake sources
    float norm;                         // Normalisation of PSF
    float covarFactor;                  // Covariance factor
    if (!effLimit(&magLim, &radius, &minFlux, &norm, &covarFactor, readout,
                  psf, thresh, smoothSigma, smoothNsigma, maskVal)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine limits for image");
        return false;
    }

#if TESTING
    psphotSaveImage(NULL, readout->image, "orig_image.fits");
    psphotSaveImage(NULL, readout->variance, "orig_variance.fits");
    psphotSaveImage(NULL, readout->mask, "orig_mask.fits");
#endif

    psImage *xFake = NULL, *yFake = NULL; // Coordinates of sources, each bin in a row
    pmReadout *fakeRO = effGenerate(&xFake, &yFake, readout, psf, magOffsets, numSources, magLim, radius, minFlux);
    if (!fakeRO) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake sources");
        psFree(xFake);
        psFree(yFake);
        return false;
    }

#if TESTING
    psphotSaveImage(NULL, readout->image, "fake_image.fits");
    psphotSaveImage(NULL, readout->variance, "fake_variance.fits");
    psphotSaveImage(NULL, readout->mask, "fake_mask.fits");
#endif
#undef TESTING

    // XXX Could speed this up significantly by only convolving the central pixels of each fake source
    psVector *significance = NULL;       // Significance image
    {
        int num = numSources * numBins; // Total number of sources
        // Pixel coordinates of sources
        psVector *xPix = psVectorAlloc(num, PS_TYPE_S32);
        psVector *yPix = psVectorAlloc(num, PS_TYPE_S32);
        for (int i = 0, index = 0; i < numBins; i++) {
            for (int j = 0; j < numSources; j++, index++) {
                float x = xFake->data.F32[i][j];
                float y = yFake->data.F32[i][j];
                xPix->data.S32[index] = PS_MIN(x + 0.5, numCols - 1);
                yPix->data.S32[index] = PS_MIN(y + 0.5, numRows - 1);
            }
        }

        psVector *convImage = psImageSmoothMaskPixels(readout->image, readout->mask, maskVal, xPix, yPix,
                                                      smoothSigma, smoothNsigma, minGauss); // Convolved image
        if (!convImage) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to smooth image pixels");
            psFree(xPix);
            psFree(yPix);
            psFree(xFake);
            psFree(yFake);
            return false;
        }
        psVector *convVar = psImageSmoothMaskPixels(readout->variance, readout->mask, maskVal, xPix, yPix,
                                                    smoothSigma, smoothNsigma, minGauss); // Convolved varianc
        if (!convVar) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to smooth variance pixels");
            psFree(xPix);
            psFree(yPix);
            psFree(xFake);
            psFree(yFake);
            return false;
        }

        float factor = 1.0 / covarFactor; // Correction for covariance

        const psImage *mask = readout->mask;  // Mask for readout
        for (int i = 0; i < num; i++) {
            float imageVal = convImage->data.F32[i]; // Image value
            float varVal = convVar->data.F32[i]; // Variance value
            int x = xPix->data.S32[i], y = yPix->data.S32[i]; // Coordinates
            if (imageVal < 0 || varVal <= 0 || (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) {
                convImage->data.F32[i] = 0.0;
            } else {
                convImage->data.F32[i] = factor * PS_SQR(imageVal) / varVal;
            }
        }

        psFree(xPix);
        psFree(yPix);

        significance = convImage;
        psFree(convVar);
    }

    thresh *= thresh;                   // "Significance" is actually significance-squared

    psVector *count = psVectorAlloc(numBins, PS_TYPE_S32); // Number of sources found in each bin
    psArray *fakeSources = psArrayAlloc(numSources);            // Fake sources in each bin
    psArray *fakeSourcesAll = psArrayAllocEmpty(numSources * numBins); // All fake sources
    for (int i = 0, index = 0; i < numBins; i++) {
        int numFound = 0;               // Number found

        // Determine extraction size
        float mag = magLim + magOffsets->data.F32[i]; // Magnitude for bin
        float peak = powf(10.0, -0.4 * mag) / norm;   // Peak flux
#ifndef USE_SINGLE_POINT_FOR_MODEL
        int sizeX = numCols / 16;
        int sizeY = numRows / 16;
        int numPoints = 0;
        float sum =  0;
        for (int y = sizeY * 0.5 ; y < numRows; y += sizeY) {
            for (int x = sizeX * 0.5 ; x < numCols; x += sizeX) {
                // Need to normalise out difference between Gaussian and real PSF
                pmModel *model = pmModelFromPSFforXY(psf, (float) x, (float) y, peak); 

                if (!model || (model->flags & MODEL_MASK)) {
                    psFree(model);
                    continue;
                }

		// XXX this radius is too small : for faint sources the modelSum < 0.8*modelNorm and the
		// source is rejected.  Add a factor of 2.0 to be sure we get value fluxes
                float sourceRadius = PS_MAX(radius, model->class->modelRadius(model->params, minFlux)); // Radius for source
                psFree(model);
                if (!isfinite(sourceRadius)) {
                    continue;
                }
                numPoints++;
                sum += sourceRadius;
            }
        }
        if (!isfinite(sum) || numPoints == 0) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate PSF model for any of %d points.", sizeX * sizeY);
            return false;
        }
        float sourceRadius = sum / numPoints;
#else
        // This old way can fail for heavily masked images
        pmModel *model = pmModelFromPSFforXY(psf, numCols / 2.0, numRows / 2.0, peak); // Model for source
        if (!model || (model->flags & MODEL_MASK)) {
            psError(PS_ERR_UNKNOWN, true, "Unable to generate model for bin %d", i);
            psFree(model);
            return false;
        }
        float sourceRadius = PS_MAX(radius, model->modelRadius(model->params, minFlux)); // Radius for source
        psFree(model);
#endif

        psArray *sources = psArrayAllocEmpty(numSources); // Sources in this bin
        for (int j = 0; j < numSources; j++, index++) {
            // Coordinates of interest
            float sig = significance->data.F32[index]; // Significance of pixel
            if (sig > thresh) {
                pmSource *source = pmSourceAlloc(); // Fake source
                float x = xFake->data.F32[i][j];
                float y = yFake->data.F32[i][j];
                source->peak = pmPeakAlloc(x, y, sig, PM_PEAK_LONE);
                if (!pmSourceDefinePixels(source, readout, x, y, sourceRadius)) {
                    psErrorClear();
                    continue;
                }
                source->peak->xf = x;
                source->peak->yf = y;
                source->modelPSF = pmModelFromPSFforXY(psf, x, y, 2.0 * sqrtf(sig));
                if (source->modelPSF) {
                    source->type = PM_SOURCE_TYPE_STAR;

                    source->modelPSF->fitRadius = sourceRadius;
                    source->apRadius = sourceRadius;

                    numFound++;
                    psArrayAdd(sources, sources->n, source);
                    psArrayAdd(fakeSourcesAll, fakeSourcesAll->n, source);
                } else {
                    psLogMsg("psphot", PS_LOG_WARN, "pmModelFromPSFforXY failed for sig: %f x: %6.1f y: %6.1f\n", 
                        sig, x, y);
                }
                psFree(source);
            }
        }
        fakeSources->data[i] = sources;
        count->data.S32[i] = numFound;
    }
    psFree(xFake);
    psFree(yFake);
    psFree(significance);

    // psphotFitSourcesLinearReadout subtracts the model fits
    if (!psphotFitSourcesLinearReadout(recipe, readout, fakeSourcesAll, psf, true, PM_SOURCE_PHOTFIT_CONST, false)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to perform linear fit on fake sources.");
        psFree(fakeSources);
        psFree(count);
        return false;
    }

    // Disable aperture corrections (save current value)
    pmTrend2D *apTrend = psf->ApTrend;  // Aperture trend
    psf->ApTrend = NULL;

    // measure the magnitudes and fluxes for the sources
    if (!psphotMagnitudesReadout(config, recipe, view, readout, fakeSourcesAll, psf, index)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to measure magnitudes of fake sources.");
        psFree(fakeSources);
        psFree(count);
        psf->ApTrend = apTrend; // Casting away const!
        return false;
    }

    // replace the subtracted model fits
    for (int i = 0; i < fakeSourcesAll->n; i++) {
	pmSource *source = fakeSourcesAll->data[i];

	// replace other sources?
	if (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) continue;
	
	pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
    }
    psFree(fakeSourcesAll);

    // Replace aperture corrections
    psf->ApTrend = apTrend;

    psVector *magDiffMean = psVectorAlloc(numBins, PS_TYPE_F32); // Mean difference in magnitude for each bin
    psVector *magDiffStdev = psVectorAlloc(numBins, PS_TYPE_F32); // Stdev of diff in magnitude for each bin
    psVector *magErrMean = psVectorAlloc(numBins, PS_TYPE_F32); // Mean error in magnitude for each bin
    psVector *magDiff = psVectorAlloc(numSources, PS_TYPE_F32);   // Magnitude differences
    psVector *magErr = psVectorAlloc(numSources, PS_TYPE_F32);   // Magnitude errors
    psVector *magMask = psVectorAlloc(numSources, PS_TYPE_VECTOR_MASK); // Mask for magnitude errors
    psStats *magStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV |
                                     PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics
    for (int i = 0; i < numBins; i++) {
        psStatsInit(magStats);
        psVectorInit(magMask, 0);

# define TESTING 0
#if TESTING
        psString name = NULL;
        psStringAppend(&name, "fake_%d.dat", i);
        FILE *file = fopen(name, "w");
        psFree(name);
#endif

        float magRef = magLim + magOffsets->data.F32[i]; // Reference magnitude
        psArray *sources = fakeSources->data[i];         // Sources in bin
        for (int j = 0; j < sources->n; j++) {
            pmSource *source = sources->data[j]; // Source of interest
            if (!source || !isfinite(source->psfMag)) {
                magMask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 0xFF;
                continue;
            }

#if TESTING
            fprintf(file, "%f %f %f %f %f %f %f\n", source->peak->xf, source->peak->yf,
                    source->modelPSF->params->data.F32[PM_PAR_XPOS],
                    source->modelPSF->params->data.F32[PM_PAR_YPOS],
                    magRef, source->psfMag, source->psfMagErr);
#endif
            magDiff->data.F32[j] = source->psfMag - magRef;
            magErr->data.F32[j] = source->psfMagErr;
        }
        magDiff->n = sources->n;
        magMask->n = sources->n;
        magErr->n = sources->n;
        if (!psVectorStats(magStats, magDiff, NULL, magMask, 0xFF)) {
            // Probably because we don't have enough sources
            psErrorClear();
        }

        if (isfinite(magStats->robustMedian) && isfinite(magStats->robustStdev)) {
            magDiffMean->data.F32[i] = magStats->robustMedian;
            magDiffStdev->data.F32[i] = magStats->robustStdev;
        } else {
            magDiffMean->data.F32[i] = magStats->sampleMean;
            magDiffStdev->data.F32[i] = magStats->sampleStdev;
        }

        if (!psVectorStats(magStats, magErr, NULL, magMask, 0xFF)) {
            // Probably because we don't have enough sources
            psErrorClear();
        }

        if (isfinite(magStats->robustMedian)) {
            magErrMean->data.F32[i] = magStats->robustMedian;
        } else {
            magErrMean->data.F32[i] = magStats->sampleMean;
        }

#if TESTING
        fclose(file);
#endif
    }

    psFree(magStats);
    psFree(magDiff);
    psFree(magMask);
    psFree(magErr);

    psFree(fakeSources);

    // subtract the faked sources from the original image
    psBinaryOp(readout->image, readout->image, "-", fakeRO->image);
    psFree(fakeRO);

    pmDetEff *de = pmDetEffAlloc(magLim, numSources, numBins); // Detection efficiency
    de->magOffsets = psVectorCopy(NULL, magOffsets, PS_TYPE_F32);
    de->counts = count;
    de->magDiffMean = magDiffMean;
    de->magDiffStdev = magDiffStdev;
    de->magErrMean = magErrMean;

    psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, PM_DETEFF_ANALYSIS, PS_META_REPLACE | PS_DATA_UNKNOWN,
                     "Detection efficiency", de);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "DETEFF.MAGREF", 
        PS_META_REPLACE, "Magnitude reference", magLim);

    psFree(de);

    psLogMsg("psphot", PS_LOG_WARN, "Detection efficiency: %lf sec\n", psTimerClear("psphot.fake"));

    return true;
}

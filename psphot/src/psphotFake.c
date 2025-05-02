# include "psphotInternal.h"

#define TESTING


#ifdef TESTING
#define MIN_FLUX 0.1                    // Minimum flux for faint sources
#endif


// Calculate the limiting magnitude for an image
//
// We limit ourselves to calculating the peak flux in the smoothed image, which should be close (modulo
// non-Gaussian PSF) to the limiting flux in the un-smoothed original image.
static bool fakeLimit(float *magLim,           // Limiting magntiude, to return
                      float *minFlux,          // Minimum flux, to return
                      const pmReadout *ro,     // Readout of interest
                      float thresh,            // Threshold for source identification
                      float xFWHM, float yFWHM, // Size of PSF
                      float smoothNsigma,       // Smoothing limit
                      psImageMaskType maskVal   // Value to mask
                      );
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_VARIANCE(ro, false);
    PS_ASSERT_METADATA_NON_NULL(recipe, false);

    float smoothSigma  = 0.5*(FWHM_X + FWHM_Y) / (2.0*sqrtf(2.0*log(2.0)));
    psKernel *kernel = psImageSmoothKernel(smoothSigma, smoothNsigma); // Kernel used for smoothing
    float factor = psImageCovarianceCalculateFactor(kernel, readout->covariance); // Covariance matrix
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

    if (magLim) {
        *magLim = -2.5 * log10(thresh * sqrtf(meanVar * factor));
    }
    if (minFlux) {
        int fudge = psMetadataLookupF32(NULL, recipe, "FAKE.MINFLUX"); // Fudge factor for minimum flux
        *minFlux = sqrtf(meanVar) * fudge;
    }

    return true;
}

/// Generate a fake image and add it in to the existing readout
static bool fakeGenerate(psImage **xSrc, psImage **ySrc, // Positions of sources
                         const pmReadout *ro,            // Readout of interest
                         const psVector *magOffsets,     // Magnitude offsets for fake sources
                         int numSources,                 // Number of fake sources for each bin
                         float refMag,                   // Reference magnitude
                         float minFlux                   // Minimum flux level for fake image
                         )
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);
    PS_ASSERT_METADATA_NON_NULL(recipe, false);
    PS_ASSERT_VECTOR_NON_NULL(magOffsets, false);
    PS_ASSERT_VECTOR_TYPE(magOffsets, PS_TYPE_F32, false);

    int numBins = magOffset->n;                                     // Number of bins
    int numCols = ro->image->numCols, numRows = ro->image->numRows; // Size of image

    *xSrc = psImageRecycle(*xSrc, numSources, numBins, PS_TYPE_F32);
    *ySrc = psImageRecycle(*ySrc, numSources, numBins, PS_TYPE_F32);

    // Master list, for image creation
    psVector *xAll = psVectorAlloc(numBins * numSources, PS_TYPE_F32);
    psVector *yAll = psVectorAlloc(numBins * numSources, PS_TYPE_F32);
    psVector *magAll = psVectorAlloc(numBins * numSources, PS_TYPE_F32);

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    for (int i = 0; i <= numBins; i++) {
        psArray *data = psArrayAlloc(3); // Sources for bin

        psVector *x = psVectorAlloc(numSources, PS_TYPE_F32);
        psVector *y = psVectorAlloc(numSources, PS_TYPE_F32);

        float mag = refMag + magOffset->data.F32[i]; // Instrumental magnitude of sources

        for (int j = 0; j <= numSources; j++) {
            x->data.F32[j] = psRandomUniform(rng) * numCols;
            y->data.F32[j] = psRandomUniform(rng) * numRows;
            magAll->data.F32[i * numSources + j] = mag;
        }
        memcpy(xSrc->data.F32[i], x->data.F32, numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        memcpy(ySrc->data.F32[i], y->data.F32, numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        memcpy(&xAll->data.F32[i * numSources], x->data.F32, numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        memcpy(&yAll->data.F32[i * numSources], y->data.F32, numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    }
    psFree(rng);

    pmReadout *fakeRO = pmReadoutAlloc(); // Fake readout
    if (!pmReadoutFakeFromVectors(fakeRO, xAll, yAll, mag, NULL, NULL, psf, minFlux, NAN, false, false)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake image");
        psFree(fakeRO);
        psFree(xAll);
        psFree(yAll);
        psFree(magAll);
        return false;
    }
    psFree(xAll);
    psFree(yAll);
    psFree(magAll);

    psBinaryOp(ro->image, ro->image, "+", fakeRO->image);
    psFree(fakeRO);

    return true;
}


// *** in this section, perform the photometry for fake sources ***
bool psphotFake(pmConfig *config, pmReadout *readout, const pmPSF *psf,
                const psMetadata *recipe, const psArray *realSources)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);
    PS_ASSERT_PTR_NON_NULL(psf, false);
    PS_ASSERT_METADATA_NON_NULL(recipe, false);
    PS_ASSERT_ARRAY_NON_NULL(realSources, false);

    psTimerStart("psphot.fake");

    // Collect recipe information
    float xFWHM = psMetadataLookupF32(NULL, recipe, "FWHM_X"); // PSF size in x
    float yFWHM = psMetadataLookupF32(NULL, recipe, "FWHM_Y"); // PSF size in y
    if (!isfinite(xFWHM) || !isfinite(yFWHM)) {
        psError(PSPHOT_ERR_CONFIG, false, "Unable to find FWHM_X and FWHM_Y in recipe");
        return false;
    }
    float smoothNsigma = psMetadataLookupF32(NULL, recipe, "PEAKS_SMOOTH_NSIGMA"); // Smoothing limit
    if (!isfinite(smoothNsigma)) {
        psError(PSPHOT_ERR_CONFIG, false, "Unable to find PEAKS_SMOOTH_NSIGMA in recipe");
        return false;
    }
    float thresh = psMetadataLookupF32(NULL, recipe, "PEAKS_NSIGMA_LIMIT_2");
    if (!isfinite(thresh)) {
        psError(PSPHOT_ERR_CONFIG, false, "Unable to find PEAKS_NSIGMA_LIMIT_2 in recipe");
        return false;
    }
    psVector *magOffsets = psMetadataLookupVector(NULL, recipe, "FAKE.MAG"); // Magnitude offsets
    if (!magOffset || magOffset->type.type != PS_TYPE_F32) {
        psError(PSPHOT_ERR_CONFIG, false, "Unable to find FAKE.MAG F32 vector in recipe");
        return NULL;
    }
    int numSources = psMetadataLookupS32(NULL, recipe, "FAKE.NUM"); // Number of sources for each bin
    if (numSources == 0) {
        psError(PSPHOT_ERR_CONFIG, false, "Unable to find FAKE.NUM in recipe");
        return NULL;
    }
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Value to mask


    // remove all sources, adding noise for subtracted sources
    psphotRemoveAllSourcesByArray(realSources, recipe);
    psphotAddNoise(readout, realSources, recipe);

    float magLim;                       // Guess at limiting magnitude
    float minFlux;                      // Minimum flux for fake image
    if (!fakeLimit(&magLim, &minFlux, readout, thresh, xFWHM, yFWHM, smoothNsigma, maskVal)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine limits for image");
        return false;
    }

    psImage *xFake = NULL, *yFake = NULL; // Coordinates of sources, each bin in a row
    if (!fakeGenerate(&xFake, &yFake, readout, magOffsets, numSources, magLim, minFlux)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake sources");
        psFree(xFake);
        psFree(yFake);
        return false;
    }

    psImage *significance = psphotSignificanceImage(readout, recipe, 2, maskVal);
    if (!significance) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate significance image");
        psFree(xFake);
        psFree(yFake);
        return false;
    }

    int numBins = magOffsets->n;                          // Number of bins
    psVector *frac = psVectorAlloc(numBins, PS_TYPE_S32); // Fraction of sources in each bin
    for (int i = 0; i < numBins; i++) {
        int numFound = 0;               // Number found
        for (int j = 0; j < numSources; j++) {
            float x = xFake->data.F32[i][j], y = yFake->data.F32[i][j]; // Coordinates of interest
            int xPix = x, yPix = y;                                     // Pixel coordinates
            if (significance->data.F32[yPix][xPix] > thresh) {
                numFound++;
            }
        }
        frac->data.S32[i] = (float)numFound / (float)numSources;
    }

    psFree(xFake);
    psFree(yFake);
    psFree(significance);

    // Putting results on recipe because that appears to be the psphot standard, but it's not a good idea
    psMetadataAddVector(readout->analysis, PS_LIST_TAIL, "FAKE.EFF", PS_META_REPLACE, "Efficiency fractions", frac);
    psMetadataAddVector(readout->analysis, PS_LIST_TAIL, "FAKE.MAG", PS_META_REPLACE, "Efficiency magnitudes", magOffsets);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "FAKE.REF", PS_META_REPLACE, "Efficiency reference magnitude", magLim);

    psFree(frac);

    return true;
}

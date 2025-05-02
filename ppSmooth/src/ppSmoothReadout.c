#include "ppSmooth.h"

bool ppSmoothReadout(pmConfig *config, pmFPAview *view)
{
    bool status;

    // find the currently selected readout
    pmReadout *input = pmFPAfileThisReadout(config->files, view, "PPSMOOTH.INPUT");

    fprintf (stderr, "input: %d x %d\n", input->image->numCols, input->image->numRows);

    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, RECIPE_NAME);
    assert(status);

    psImageMaskType maskSat = pmConfigMaskGet("SAT", config); // Mask value for saturated pixels
    psMetadataAddImageMask (recipe, PS_LIST_TAIL, "MASK.SAT", PS_META_REPLACE, "user-defined mask", maskSat);

    psImageMaskType maskBad = pmConfigMaskGet("LOW", config); // Mask value for low pixels
    if (!maskBad) {
        maskBad = pmConfigMaskGet("BAD", config);
    }
    psMetadataAddImageMask (recipe, PS_LIST_TAIL, "MASK.BAD", PS_META_REPLACE, "user-defined mask", maskBad);

    // XXX set this based on the input image (see psphot)
    psImageMaskType maskVal = 0xffff;

    if (!input->mask) {
        pmReadoutGenerateMask(input, maskSat, maskBad);
    }

    if (!input->variance) {
        pmReadoutGenerateVariance(input, NULL, true);
    }

    float minGauss = 0.1;
    float nSigma = 3.0;
    float sigma;
    if (!(sigma = psMetadataLookupF32(&status,config->arguments,"SIGMA"))) {
      sigma = psMetadataLookupF32 (&status, recipe, "SIGMA");
    }

    bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve

    psTimerStart ("ppSmooth");

    // smooth the image in place, applying the mask as we go
    psImageSmoothMask_Threaded(input->image, input->image, input->mask, maskVal, sigma, nSigma, minGauss);
    psLogMsg("ppSmooth", PS_LOG_MINUTIA, "smooth image: %f sec\n", psTimerMark("ppSmooth"));

    // Smooth the variance in place, applying the mask as we go
    psImageSmoothMask_Threaded(input->variance, input->variance, input->mask, maskVal, sigma * M_SQRT1_2, nSigma, minGauss);
    psLogMsg("ppSmooth", PS_LOG_MINUTIA, "smooth variance: %f sec\n", psTimerMark("ppSmooth"));
    psImageConvolveSetThreads(oldThreads);

    // determine covariance matrix for this smoothing, replace existing kernel
    oldThreads = psImageCovarianceSetThreads(true);
    psKernel *kernel = psImageSmoothKernel(sigma, nSigma); // Kernel used for smoothing
    psKernel *covar = psImageCovarianceCalculate(kernel, input->covariance); // Covariance matrix
    psFree (input->covariance);
    input->covariance = covar;
    psFree(kernel);
    float factor = 1.0 / psImageCovarianceFactor(covar);
    psImageCovarianceSetThreads(oldThreads);

    // record the effective area and significance scaling factor
    float effArea = 8.0 * M_PI * PS_SQR(sigma);
    psMetadataAddF32(recipe, PS_LIST_TAIL, "EFFECTIVE_AREA", PS_META_REPLACE, "Effective Area", effArea);
    psMetadataAddF32(recipe, PS_LIST_TAIL, "SIGNIFICANCE_SCALE_FACTOR", PS_META_REPLACE, "Signicance scale factor", factor);

    return true;
}

// The variance is smoothed by the PSF^2, renomalized to maintain the input level of the
// variance.  We achieve this by smoothing with a Gaussian with sigma = SIGMA_SMTH/sqrt(2) with
// unity normalization.  Note that this process yields a smoothed image with correlated errors.
// The pixel-to-pixel variations in smooth_im will be decreased by a factor of
// 4*pi*SIGMA_SMTH^2, but for measurements based on apertures comparable to or larger than the
// smoothing kernel, the effective per-pixel variance is maintained.

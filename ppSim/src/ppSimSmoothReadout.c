#include "ppSim.h"

bool ppSimSmoothReadout(pmReadout *input, psMetadata *recipe)
{
    bool status;
    psTimerStart ("ppSmooth");

    float nSigma = psMetadataLookupF32(&status, recipe, "CONVOLVE.NSIGMA"); // SIGMA convolutions (pixels)
    if (!status) nSigma = 5.0;

    float sigma = psMetadataLookupF32(&status, recipe, "SEEING"); // Seeing SIGMA (pixels)

    char *modelName = psMetadataLookupStr(&status, recipe, "PSF.MODEL"); // Seeing SIGMA (pixels)
    if (!strcmp (modelName, "PS_MODEL_GAUSS")) {
      // bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve
      // smooth the image in place, applying the mask as we go
      psImageSmooth(input->image, sigma, nSigma);
      psLogMsg("ppSmooth", PS_LOG_MINUTIA, "smooth image: %f sec\n", psTimerMark("ppSmooth"));
      return true;
    }

    if (!strcmp (modelName, "PS_MODEL_PS1_V1")) {
      // bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading in psImageConvolve
      // smooth the image in place, applying the mask as we go
      psImageSmooth2dCacheData *smdata = psImageSmooth2dCacheAlloc(nSigma);
      psImageSmooth2dCacheKernel_PS1_V1 (smdata, sigma, 0.2);

      psImageSmooth2dCache_F32 (input->image, smdata);

      psFree (smdata);
      psLogMsg("ppSmooth", PS_LOG_MINUTIA, "smooth image: %f sec\n", psTimerMark("ppSmooth"));
      return true;
    }

    // psImageConvolveSetThreads(oldThreads);
    psLogMsg("ppSmooth", PS_LOG_MINUTIA, "failed to smooth image: %f sec\n", psTimerMark("ppSmooth"));
    return false;
}

// The variance is smoothed by the PSF^2, renomalized to maintain the input level of the
// variance.  We achieve this by smoothing with a Gaussian with sigma = SIGMA_SMTH/sqrt(2) with
// unity normalization.  Note that this process yields a smoothed image with correlated errors.
// The pixel-to-pixel variations in smooth_im will be decreased by a factor of
// 4*pi*SIGMA_SMTH^2, but for measurements based on apertures comparable to or larger than the
// smoothing kernel, the effective per-pixel variance is maintained.


// XXX this only allows for Gaussian PSFs.  To extend this, we need to convolve with an image
// of the PSF model

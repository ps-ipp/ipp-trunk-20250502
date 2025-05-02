# include "psphotInternal.h"

// this badly-named function performs photometry assuming (a) a supplied PSF, (b) background
// subtraction, (c) linear psf-model fits only, (d) a prior analysis has supplied the moments
// window parameters.  It is currently only being used by ppSub.

// NOTE: ppSub needs to perform extended source analysis for comets and trails.

bool psphotReadoutMinimal(pmConfig *config, const pmFPAview *view, const char *filerule) {

    // measure the total elapsed time in psphotReadout.  XXX the current threading plan
    // for psphot envisions threading within psphotReadout, not multiple threads calling
    // the same psphotReadout.  In the current plan, this dtime is the elapsed time used
    // jointly by the multiple threads, not the total time used by all threads.
    psTimerStart ("psphotReadout");

    pmModelClassSetLimits(PM_MODEL_LIMITS_LAX);

    // set the photcode for this image
    if (!psphotAddPhotcode(config, view, filerule)) {
        psError(PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // Generate the mask and weight images, including the user-defined analysis region of interest
    psphotSetMaskAndVariance (config, view, filerule);

    // only subtract background if needed?
    // activate this for a clean test with psphotMinimal. (add to recipe!)
    if (0) {
      // generate a background model (median, smoothed image)
      if (!psphotModelBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
      }
      if (!psphotSubtractBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
      }
    }

    // load the psf model, if suppled.  FWHM_X,FWHM_Y,etc are saved on readout->analysis
    if (!psphotLoadPSF (config, view, filerule)) {
      psError (PSPHOT_ERR_CONFIG, false, "missing psf model");
      return psphotReadoutCleanup (config, view, filerule);
    }

    // find the detections (by peak and/or footprint) in the image. (final pass)
    if (!psphotFindDetections(config, view, filerule, false)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failure in peak analysis");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // construct sources and measure basic stats (saved on detections->newSources)
    if (!psphotSourceStats (config, view, filerule, false)) { // pass 1
        psError(PSPHOT_ERR_UNKNOWN, false, "failure to generate sources");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // find blended neighbors of very saturated stars
    psphotDeblendSatstars (config, view, filerule);

    // mark blended peaks PS_SOURCE_BLEND
    if (!psphotBasicDeblend (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failed on deblend analysis");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // classify sources based on moments, brightness (use supplied psf shape parameters)
    if (!psphotRoughClass (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failed to find a valid PSF clump for image");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // merge the newly selected sources into the existing list
    psphotMergeSources (config, view, filerule);

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    if (!psphotGuessModels (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failure to Guess Model");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // linear PSF fit to source peaks
    psphotFitSourcesLinear (config, view, filerule, false, false);

    // measure the radial profiles to the sky
    psphotRadialProfileWings (config, view, filerule);

    // re-measure the kron mags with models subtracted and more appropriate windows
    psphotKronIterate(config, view, filerule, 1);

    // MEH -- sensless for SSdfiffs...
    psphotChipParams (config, view, filerule);

    // measure source size for the remaining sources
    psphotSourceSize (config, view, filerule, false);

    // NOTE: Petrosian and Isophotal mags are not relevant at this time
    // psphotExtendedSourceAnalysis (config, view, filerule);

    // in ppSub context, this is used to fit TRAILs (and maybe EXP for comets)
    psphotExtendedSourceFits (config, view, filerule);

    // calculate source magnitudes
    psphotMagnitudes(config, view, filerule);

    // XXX ensure this is measured if the analysis succeeds (even if quality is low)
    if (!psphotEfficiency(config, view, filerule)) {
        psErrorStackPrint(stderr, "Unable to determine detection efficiencies from fake sources");
        psErrorClear();
    }

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, filerule);

    // create the exported-metadata and free local data
    return psphotReadoutCleanupMinimal (config, view, filerule);
}

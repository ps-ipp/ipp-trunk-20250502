# include "psphotInternal.h"

bool psphotFullForceReadout(pmConfig *config, const pmFPAview *view, const char *filerule) {

    // measure the total elapsed time in psphotReadout.  
    psTimerStart ("psphotReadout");

    // allow objects to be fit with ugly models (central holes, extreme asymmetry, etc)
    pmModelClassSetLimits(PM_MODEL_LIMITS_LAX);

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    // set the photcode for this image
    if (!psphotAddPhotcode (config, view, filerule)) {
        psError (PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // optional break-point for processing
    char *breakPt = psMetadataLookupStr (NULL, recipe, "BREAK_POINT");
    PS_ASSERT_PTR_NON_NULL (breakPt, false);

    // Generate the mask and weight images, including the user-defined analysis region of interest
    psphotSetMaskAndVariance (config, view, filerule);
    if (!strcasecmp (breakPt, "NOTHING")) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    // generate a background model (median, smoothed image)
    if (!psphotModelBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!psphotSubtractBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!strcasecmp (breakPt, "BACKMDL")) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    if (!psphotLoadPSF (config, view, filerule)) {
    	// this only happens if we had a programming error in psphotLoadPSF
        psError (PSPHOT_ERR_UNKNOWN, false, "error loading psf model");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // include externally-supplied sources (loaded onto detections->newSources)
    psphotLoadExtSources (config, view, filerule);

#ifdef notmoved
    // XXX: moved down below
    // merge the newly selected sources into the existing list (detections->allSources)
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, filerule);
#endif

    // construct sources and measure moments and other basic stats (saved on detections->allSources)
    // all sources use the auto-scaled window appropriate to a PSF, except for the saturated
    // stars : these use a larger window (3x the basic window)
    if (!psphotFullForceSourceStats (config, view, filerule, true)) { // pass 1
        psError(PSPHOT_ERR_UNKNOWN, false, "failure to generate sources");
        return psphotReadoutCleanup (config, view, filerule);
    }
    
    // classify sources based on moments, brightness.  if a PSF model has been loaded, the PSF
    // clump defined for it is used not measured (detections->newSources)
    if (!psphotRoughClass (config, view, filerule)) { // pass 1
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to determine rough classifications");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // merge the newly selected sources into the existing list (detections->allSources)
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, filerule);

    // generate a psf model for any readouts which need one
    // psphotFullForcePSF (config, view, filerule);
    // XXX I'm not sure we need a different algorithm here or not : we are supplying the
    // sources; we should mark with a flag bit the ones we actually want to use as PSF
    // stars (this means we need to supply this info in the load).
    if (!psphotChoosePSF (config, view, filerule, false)) {
        // PSPHOT_ERR_DATA causes this program to exit gracefully
        psError (PSPHOT_ERR_DATA, false, "failed to construct psf model");
        return  psphotReadoutCleanup (config, view, filerule);
    }

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    // XXX this currently only generates a PSF model
    psphotGuessModels (config, view, filerule);

    // linear PSF fit to source peaks, subtract the models from the image (in PSF mask)
    // XXX this will fit non-PSF sources if they are supplied
    psphotFitSourcesLinear (config, view, filerule, false, false);

    // measure moments
    // XXX does this set the correct gaussian window (how to do multiple windows?)
    // we are calling this above...
    // psphotSourceStats (config, view, filerule, false);

    // measure kron fluxes
    psphotKronFlux (config, view, filerule);

    // Option to do the non-linear fitting for the brighter sources
    if (1) {
      // identify CRs and extended sources (only unmeasured sources are measured)
      psphotSourceSize (config, view, filerule, true); // pass 1 (detections->allSources)

      // non-linear PSF and EXT fit to brighter sources
      // replace model flux, adjust mask as needed, fit, subtract the models (full stamp)
      // XXX: can leave faulted job in done queue
      psphotBlendFit (config, view, filerule); // pass 1 (detections->allSources)

      // replace all sources
      psphotReplaceAllSources (config, view, filerule, false); // pass 1 (detections->allSources)

      // linear fit to include all sources (subtract again)
      // NOTE : apply to ALL sources (extended + psf)
      psphotFitSourcesLinear (config, view, filerule, true, true); // pass 2 (detections->allSources)
    }

    psphotChipParams (config, view, filerule);

    // measure petro fluxes
    psphotPetroFlux (config, view, filerule);

    // measure radial apertures
    psphotRadialApertures (config, view, filerule, 0);

    // measure galaxy shapes
    psphotGalaxyShape (config, view, filerule);

    // measure aperture photometry corrections
    if (!psphotApResid (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failed on psphotApResid");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // calculate source magnitudes (psf mag and ap mag)
    psphotMagnitudes(config, view, filerule);

    // calculate lensing parameters
    if (!psphotLensing(config, view, filerule)) {
	psErrorStackPrint(stderr, "Unable to do lensing parameters.");
        psErrorClear();
    }

    // replace background in residual image
    psphotSkyReplace (config, view, filerule);

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, filerule);

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, filerule);
}

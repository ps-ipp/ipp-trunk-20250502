# include "psphotInternal.h"

bool psphotForcedReadout(pmConfig *config, const pmFPAview *view, const char *filerule) {

    // measure the total elapsed time in psphotReadout.  XXX the current threading plan
    // for psphot envisions threading within psphotReadout, not multiple threads calling
    // the same psphotReadout.  In the current plan, this dtime is the elapsed time used
    // jointly by the multiple threads, not the total time used by all threads.
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

    // include externally-supplied sources
    psphotLoadExtSources (config, view, filerule);

    // merge the newly selected sources into the existing list
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, filerule);

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    psphotGuessModels (config, view, filerule);

    // linear PSF fit to source peaks, subtract the models from the image (in PSF mask)
    psphotFitSourcesLinear (config, view, filerule, false, false);

    // identify CRs and extended sources
    // XXX do I want to do this step?
    // XXX do I want to / need to calculate the moments?
    // psphotSourceSize (config, readout, sources, recipe, psf, 0);

    // calculate source magnitudes
    psphotMagnitudes(config, view, filerule);

    // XXX do I want to do this?
    // if (!psphotEfficiency(config, readout, view, psf, recipe, sources)) {
    //     psErrorStackPrint(stderr, "Unable to determine detection efficiencies from fake sources");
    //     psErrorClear();
    // }

    // replace background in residual image
    psphotSkyReplace (config, view, filerule);

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, filerule);

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, filerule);
}

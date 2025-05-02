# include "psphotInternal.h"

bool psphotMakePSFReadout(pmConfig *config, const pmFPAview *view, const char *filerule) {

    // measure the total elapsed time in psphotReadout.  XXX the current threading plan
    // for psphot envisions threading within psphotReadout, not multiple threads calling
    // the same psphotReadout.  In the current plan, this dtime is the elapsed time used
    // jointly by the multiple threads, not the total time used by all threads.
    psTimerStart ("psphotReadout");

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

    psphotLoadExtSources (config, view, filerule);

    // If sources have been supplied, then these should be used to measure the PSF include
    // externally-supplied sources; if not, we need to generate a set of possible PSF sources.
    // This function updates the SN entries for the loaded sources or generates a set of
    // detections from the image, if no external ones have been supplied.  Sources loaded from
    // a text file have no valid SN values, but psphotChoosePSF needs to select the top
    // PSF_MAX_NSTARS to generate the PSF.
    if (!psphotCheckExtSources (config, view, filerule)) {
	psLogMsg ("psphot", 3, "failure to select possible PSF sources (external or internal)");
	return psphotReadoutCleanup (config, view, filerule);
    }

    // Use bright stellar objects to measure PSF. If we do not have enough stars to generate
    // the PSF, build one from the SEEING guess and model class
    if (!psphotChoosePSF (config, view, filerule, true)) {
	psLogMsg ("psphot", 3, "failure to construct a psf model");
	return psphotReadoutCleanup (config, view, filerule);
    }

    // measure aperture photometry corrections
# if 0
    if (!psphotApResid (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failed on psphotApResid");
        return psphotReadoutCleanup (config, view, filerule);
    }
# endif

    return psphotReadoutCleanup (config, view, filerule);
}

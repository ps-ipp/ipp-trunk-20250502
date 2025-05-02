# include "psphotInternal.h"

// in this psphotReadout-variant, we are only trying to determine the PSF given an existing set
// of input source positions to use as initial PSF stars.
bool psphotReadoutFindPSF(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources) {

    psTimerStart ("psphotReadout");

    // set the photcode for the input
    if (!psphotAddPhotcode(config, view, filerule)) {
        psError(PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // Generate the mask and variance images, including the user-defined analysis region of interest
    psphotSetMaskAndVariance (config, view, filerule);

    // Note that in this implementation, we do NOT model the background and we do not
    // attempt to detect the sources in the image

    // include the externally-supplied sources (inSources)
    // (we assume a single set of input sources is supplied)
    if (!psphotDetectionsFromSources (config, view, filerule, inSources)) {
        psError(PSPHOT_ERR_ARGUMENTS, true, "Can't find PSF stars");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // construct detections->newSources and measure basic stats (moments, local sky)
    if (!psphotSourceStats(config, view, filerule, true)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "failure to generate sources");
	return false;
    }

    // peak flux is wrong : use the peak measured in the moments analysis:
    if (!psphotRepairLoadedSources(config, view, filerule)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "failure to repair sources");
	return false;
    }

    // classify sources based on moments, brightness (psf is not known)
    if (!psphotRoughClass (config, view, filerule)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to determine rough source class");
        return psphotReadoutCleanup (config, view, filerule);
    }

    if (!psphotImageQuality (config, view, filerule)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to measure image quality");
        return psphotReadoutCleanup (config, view, filerule);
    }

    if (!psphotChoosePSF(config, view, filerule, true)) {
        psError(PSPHOT_ERR_PSF, false, "Failed to construct a psf model");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // merge the newly selected sources into the existing list
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, filerule); 

# if 0
    // XXX if we want to determine the aperture residual correction here, we either
    // need to carry it out of the PSF determination analysis above, or save the model
    // fits from that analysis, or run the linear PSF fit for all objects currently in hand
    // construct an initial model for each object, set the radius to fitRadius, set circular fit mask
    psphotGuessModels (config, view, filerule);
# endif

# if 0
    // measure aperture photometry corrections
    if (!psphotApResid (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failed on psphotApResid");
        return psphotReadoutCleanup (config, view, filerule);
    }
# endif

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels(config, view, filerule);

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, filerule);
}

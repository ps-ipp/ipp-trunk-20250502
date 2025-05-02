# include "psphotInternal.h"

// in this psphotReadout-variant, we are only measuring the photometry for known sources, using
// a PSF generated for this observation from those sources
bool psphotReadoutKnownSources(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources) {

    psTimerStart ("psphotReadout");

    // set the photcode for this image
    if (!psphotAddPhotcode(config, view, filerule)) {
        psError(PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // Generate the mask and weight images, including the user-defined analysis region of interest
    psphotSetMaskAndVariance (config, view, filerule);

    // Note that in this implementation, we do NOT model the background and we do not
    // attempt to detect the sources in the image

    // include externally-supplied sources (supplied as PSPHOT.INPUT.CMF)
    if (!psphotDetectionsFromSources (config, view, filerule, inSources)) {
        psError(PSPHOT_ERR_ARGUMENTS, true, "Can't find PSF stars");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // construct sources and measure basic stats
    if (!psphotSourceStats (config, view, filerule, true)) {
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

    if (!psphotChoosePSF (config, view, filerule, true)) {
        psError(PSPHOT_ERR_PSF, false, "Failed to construct a psf model");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // merge the newly selected sources into the existing list
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, filerule); 

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    psphotGuessModels (config, view, filerule);

    // linear PSF fit to source peaks
    psphotFitSourcesLinear (config, view, filerule, false, false);

    // measure aperture photometry corrections
    if (!psphotApResid (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failed on psphotApResid");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // calculate source magnitudes
    psphotMagnitudes(config, view, filerule);

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, filerule);

    // set source->nFrames
    psphotSetNFrames (config, view, filerule);

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, filerule);
}

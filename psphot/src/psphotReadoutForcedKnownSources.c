# include "psphotInternal.h"

// in this psphotReadout-variant, we are only measuring the photometry for known source
// position, using a supplied PSF
bool psphotReadoutForcedKnownSources(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources) {

    psTimerStart ("psphotReadout");

    // remove cruft from the input analysis structure
    if (!psphotCleanInputs (config, view, filerule)) {
        psError (PSPHOT_ERR_PROG, false, "trouble setting up the inputs");
        return false;
    }

    // set the photcode for this image
    if (!psphotAddPhotcode(config, view, filerule)) {
        psError(PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // Generate the mask and weight images, including the user-defined analysis region of interest
    psphotSetMaskAndVariance (config, view, filerule);

    // Note that in this implementation, we do NOT model the background and we do not
    // attempt to detect the sources in the image

    if (!psphotAddKnownSources (config, view, filerule, inSources)) {
	psError(PSPHOT_ERR_UNKNOWN, false, "failure to load supplied sources");
	return false;
    }

    if (!psphotLoadPSF (config, view, filerule)) {
    	// this only happens if we had a programming error in psphotLoadPSF
        psError (PSPHOT_ERR_UNKNOWN, false, "error loading psf model");
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

    // calculate source magnitudes
    psphotMagnitudes(config, view, filerule);

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, filerule);

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, filerule);
}

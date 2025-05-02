# include "psphotInternal.h"

// XXX Should it be an error for any of these to not exist?

// This function is called by the stand-alone psphot program to set the mask values in the
// config file.  It sets the named mask values MASK.PSPHOT and MARK.PSPHOT in the PSPHOT
// recipe.  Functions or programs which call psphotReadout as a library function must set these
// named mask values in the PSPHOT recipe on their own.

bool psphotSetMaskBits (pmConfig *config) {

    psImageMaskType maskValue;
    psImageMaskType markValue;

    if (!pmConfigMaskSetBits (&maskValue, &markValue, config)) {
	psError (PS_ERR_UNKNOWN, true, "Unable to define the mask bit values");
	return false;
    }

    bool status = psphotSetMaskRecipe (config, maskValue, markValue);
    return status;
}

bool psphotSetMaskRecipe (pmConfig *config, psImageMaskType maskValue, psImageMaskType markValue) {

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    // set maskValue and markValue in the psphot recipe
    psMetadataAddImageMask (recipe, PS_LIST_TAIL, "MARK.PSPHOT", PS_META_REPLACE, "user-defined mask", markValue);
    psMetadataAddImageMask (recipe, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "user-defined mask", maskValue);

    return true;
}

// XXX should these be in config->analysis or somewhere else besides 'recipe'?

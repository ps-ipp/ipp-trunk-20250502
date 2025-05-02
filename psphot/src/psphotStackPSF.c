# include "psphotInternal.h"

// determine the 1st target PSF (either AUTO or defined by PSPHOT.STACK.TARGET.PSF.FWHM)
bool psphotStackPSF(const pmConfig *config, psphotStackOptions *options)  {

    bool mdok = false;

    int numCols = options->numCols;
    int numRows = options->numRows;
    psArray *psfs = options->psfs;
    psVector *inputMask = options->inputMask;

    // Get the recipe values
    psMetadata *psphotRecipe = psMetadataLookupMetadata(NULL, config->recipes, "PSPHOT"); // psphot recipe
    psAssert(psphotRecipe, "We've thrown an error on this before.");

    bool autoPSF = psMetadataLookupBool (&mdok, psphotRecipe, "PSPHOT.STACK.TARGET.PSF.AUTO");

    // Get the recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "PPSTACK"); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");
	
    char *psfModel = psMetadataLookupStr(NULL, recipe, "PSF.MODEL"); // Model for PSF

    options->targetSeeing = psVectorAllocEmpty(4, PS_TYPE_F32);

    if (autoPSF) {
	int psfInstances = psMetadataLookupS32(NULL, recipe, "PSF.INSTANCES"); // Number of instances for PSF
	float psfRadius = psMetadataLookupF32(NULL, recipe, "PSF.RADIUS"); // Radius for PSF
	int psfOrder = psMetadataLookupS32(NULL, recipe, "PSF.ORDER"); // Spatial order for PSF

	psString maskValStr = psMetadataLookupStr(&mdok, recipe, "MASK.VAL"); // Name of bits to mask going in
	if (!mdok || !maskValStr) {
	    psError(PSPHOT_ERR_CONFIG, false, "Unable to find MASK.VAL in recipe");
	    return false;
	}
	psImageMaskType maskVal = pmConfigMaskGet(maskValStr, config); // Bits to mask

	for (int i = 0; i < psfs->n; i++) {
	    if (inputMask->data.U8[i]) {
		psFree(psfs->data[i]);
		psfs->data[i] = NULL;
	    }
	}

	// Solve for the target PSF
	options->psf = pmPSFEnvelope(numCols, numRows, psfs, psfInstances, psfRadius, psfModel, psfOrder, psfOrder, maskVal);
	if (!options->psf) {
	    psError(PSPHOT_ERR_PSF, false, "Unable to determine output PSF.");
	    return false;
	}

        psMetadataAddPtr(config->arguments, PS_LIST_TAIL, "PSF.TARGET", PS_DATA_UNKNOWN, "Target PSF for stack", options->psf);
        float targetSeeing = pmPSFtoFWHM(options->psf, 0.5 * options->numCols, 0.5 * options->numRows); // FWHM for target
        psVectorAppend(options->targetSeeing, targetSeeing);
        psLogMsg("psphotStack", PS_LOG_INFO, "Target seeing FWHM (auto-scaled): %f\n", targetSeeing);
	return true;
    }

    // externally-defined PSF
    // XXX need to test for compatibility of target with inputs

    // is a single target FWHM specified, or a set of values?  set up the vector options->targetSeeing and the local 1st value
    float targetSeeing = psMetadataLookupF32 (&mdok, psphotRecipe, "PSPHOT.STACK.TARGET.PSF.FWHM");
    if (!mdok) {
	psVector *fwhmValues = psMetadataLookupVector(&mdok, psphotRecipe, "PSPHOT.STACK.TARGET.PSF.FWHM"); // Magnitude offsets
	psAssert (mdok, "missing psphot recipe value PSPHOT.STACK.TARGET.PSF.FWHM");
	for (int i = 0; i < fwhmValues->n; i++) {
	    psVectorAppend(options->targetSeeing, fwhmValues->data.F32[i]);
	}	    
	targetSeeing = fwhmValues->data.F32[0];
    } else {
        psVectorAppend(options->targetSeeing, targetSeeing);
    }

    // measured scale factors (fwhm = Sxx * 2.35 * scaleFactor / sqrt(2.0))
    // GAUSS  : 1.000
    // PGAUSS : 1.006
    // QGAUSS : 1.151
    // RGAUSS : 0.883
    // PS1_V1 : 1.134
	
    float scaleFactor = NAN;
    if (!strcmp(psfModel, "PS_MODEL_GAUSS")) {
	scaleFactor = 1.000;
    }
    if (!strcmp(psfModel, "PS_MODEL_PGAUSS")) {
	scaleFactor = 1.0006;
    }
    if (!strcmp(psfModel, "PS_MODEL_QGAUSS")) {
	scaleFactor = 1.151;
    }
    if (!strcmp(psfModel, "PS_MODEL_RGAUSS")) {
	scaleFactor = 0.883;
    }
    if (!strcmp(psfModel, "PS_MODEL_PS1_V1")) {
	scaleFactor = 1.134;
    }
    psAssert (isfinite(scaleFactor), "invalid model for PSF"); 

    float Sxx = sqrt(2.0)*targetSeeing / 2.35 / scaleFactor;

    // XXX probably should make the model type (and par 7) optional from recipe
    // psf = pmPSFBuildSimple(psfModel, Sxx, Sxx, 0.0, 1.0);
    options->psf = pmPSFBuildSimple(psfModel, Sxx, Sxx, 0.0, 0.2);
    if (!options->psf) {
	psError(PSPHOT_ERR_PSF, false, "Unable to build dummy PSF.");
	return false;
    }

    psMetadataAddPtr(config->arguments, PS_LIST_TAIL, "PSF.TARGET", PS_DATA_UNKNOWN, "Target PSF for stack", options->psf);
    psLogMsg("psphotStack", PS_LOG_INFO, "Target seeing FWHM (1 of %ld): %f\n", options->targetSeeing->n, targetSeeing);
    return true;
}

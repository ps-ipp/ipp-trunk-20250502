#include "ppStack.h"

//#define TESTING

pmPSF *ppStackPSF(const pmConfig *config, int numCols, int numRows,
                  const psArray *psfs, ppStackOptions *options)
{
    bool mdok = false;
    pmPSF *psf = NULL;

    const psVector *inputMask = options->inputMask;

    // Get the recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    bool autosize = psMetadataLookupBool(&mdok, recipe, "PSF.AUTOSIZE"); // Spatial order for PSF
    if (!mdok) {
	// older config files which lack PSF.AUTOSIZE used TRUE as the default
	autosize = true;
    }

    char *psfModel = psMetadataLookupStr(NULL, recipe, "PSF.MODEL"); // Model for PSF

    if (autosize) {

	int psfInstances = psMetadataLookupS32(NULL, recipe, "PSF.INSTANCES"); // Number of instances for PSF
	float psfRadius = psMetadataLookupF32(NULL, recipe, "PSF.RADIUS"); // Radius for PSF
	int psfOrder = psMetadataLookupS32(NULL, recipe, "PSF.ORDER"); // Spatial order for PSF

	psString maskValStr = psMetadataLookupStr(&mdok, recipe, "MASK.VAL"); // Name of bits to mask going in
	if (!mdok || !maskValStr) {
	    psError(PPSTACK_ERR_CONFIG, false, "Unable to find MASK.VAL in recipe");
	    return NULL;
	}
	psImageMaskType maskVal = pmConfigMaskGet(maskValStr, config); // Bits to mask

	for (int i = 0; i < psfs->n; i++) {
	    if (inputMask->data.U8[i]) {
		psFree(psfs->data[i]);
		psfs->data[i] = NULL;
	    }
	}

	// Solve for the target PSF
	psf = pmPSFEnvelope(numCols, numRows, psfs, psfInstances, psfRadius, psfModel, psfOrder, psfOrder, maskVal);
	if (!psf) {
	    psError(PPSTACK_ERR_PSF, false, "Unable to determine output PSF.");
	    return NULL;
	}
    } else {

	// Manually defined target PSF 
	float psfFWHM = psMetadataLookupF32(&mdok, recipe, "PSF.OUTPUT.FWHM"); // Radius for PSF
	if (!mdok) {
	    psfFWHM = 4.0;
	}

	//MEH -- preliminary add option -1 for choosing mean FWHM of inputs. Nsig<0 for scaling +Nsig by mean
	if (psfFWHM<0.0){
	    float NpsfFWHMsig = psMetadataLookupF32(&mdok, recipe, "PSF.OUTPUT.NFWHMSIG"); // Nsig to include
	    if (!mdok){
	    	NpsfFWHMsig = 0.0;
	    }
	    float clippedMean = options->clippedMean;
	    float clippedStdev = options->clippedStdev;
	    psfFWHM = clippedMean + NpsfFWHMsig*clippedStdev;
	    if (NpsfFWHMsig<0.0){
	    	psfFWHM = clippedMean + NpsfFWHMsig * -1.0*clippedStdev/clippedMean;
	    }
	}



	float Mxx = M_SQRT2 * psfFWHM / 2.35;

	psf = pmPSFBuildSimple(psfModel, Mxx, Mxx, 0.0, 0.0);
	if (!psf) {
	    psError(PPSTACK_ERR_PSF, false, "Unable to build dummy PSF.");
	    return NULL;
	}
    }

    return psf;
}

#include "ppSim.h"

bool ppSimAddOverscan (pmReadout *readout, pmConfig *config, psVector *biasCols, psVector *biasRows, psRandom *rng) {

    bool mdok;

    // skip this step if we did not generate a bias level
    if (biasCols == NULL) return true;
    if (biasRows == NULL) return true;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    float readnoise = psMetadataLookupF32(NULL, readout->parent->concepts, "CELL.READNOISE");// CCD read noise, e
    if (isnan(readnoise)) {
	psWarning("CELL.READNOISE is not set; reverting to recipe value READNOISE.");
	readnoise = psMetadataLookupF32(&mdok, recipe, "READNOISE");
	if (!mdok) {
	    psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find READNOISE in recipe.");
	    return NULL;
	}
    }

    // Add overscan
    // XXX put this in a wrapper
    for (int j = 0; j < biasCols->n; j++) {
	psImage *signal = psImageAlloc(biasCols->data.S32[j], biasRows->n, PS_TYPE_F32); // Overscan
	for (int y = 0; y < signal->numRows; y++) {
	    for (int x = 0; x < signal->numCols; x++) {
		signal->data.F32[y][x] = biasRows->data.F32[y];
	    }
	}
	psImage *variance = psImageAlloc(biasCols->data.S32[j], biasRows->n, PS_TYPE_F32); // Variance
	psImageInit(variance, PS_SQR(readnoise));

	ppSimAddNoise(signal, variance, readout->parent, config, rng);
	psListAdd(readout->bias, PS_LIST_TAIL, signal);

	psFree(variance);
	psFree(signal);     // Drop reference
    }
    return true;
}

# include "ppSim.h"

psVector *ppSimMakeBiassec (pmCell *cell, pmConfig *config) {

    bool mdok;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    bool bias = psMetadataLookupBool(&mdok, recipe, "BIAS"); // Generate a Bias?
    if (!bias) return NULL;

    psList *biassec = psMetadataLookupPtr(NULL, cell->concepts, "CELL.BIASSEC"); // Bias regions
    int numBias = psListLength(biassec); // Number of bias sections

    psVector *biasCols;
    if (numBias > 0) {
	biasCols = psVectorAlloc(numBias, PS_TYPE_S32);
	psListIterator *iter = psListIteratorAlloc(biassec, PS_LIST_HEAD, false); // Iterator
	psRegion *bias;         // Bias region, from iteration
	int i = 0;              // Counter
	while ((bias = psListGetAndIncrement(iter))) {
	    biasCols->data.S32[i++] = bias->x1 = bias->x0;
	}
    } else {
	biasCols = psVectorAlloc(1, PS_TYPE_S32);
	biasCols->data.S32[0] = psMetadataLookupS32(&mdok, recipe, "OVERSCAN.SIZE");
	if (!mdok) {
	    psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find OVERSCAN.SIZE in recipe.");
	    psFree(biasCols);
	    return NULL;
	}
	psRegion *newBias = psRegionAlloc(NAN, NAN, NAN, NAN); // New bias region, to be set later
	biassec = psListAlloc(newBias);
	psFree(newBias);
	psMetadataAdd(cell->concepts, PS_LIST_TAIL, "CELL.BIASSEC", PS_DATA_LIST | PS_META_REPLACE,
		      "Bias sections", biassec);
	psFree(biassec);
	numBias = 1;
    }

    return biasCols;
}

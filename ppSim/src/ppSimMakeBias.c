# include "ppSim.h"

psVector *ppSimMakeBias (bool *status, pmReadout *readout, pmConfig *config, const psRandom *rng) {

    bool mdok;

    if (status) *status = true;

    pmCell *cell = readout->parent;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    bool bias = psMetadataLookupBool(&mdok, recipe, "BIAS"); // Generate a Bias?
    if (!bias) return NULL;

    float biasLevel = psMetadataLookupF32(NULL, recipe, "BIAS.LEVEL"); // Bias level
    float biasRange = psMetadataLookupF32(NULL, recipe, "BIAS.RANGE"); // Bias range
    int biasOrder   = psMetadataLookupS32(NULL, recipe, "BIAS.ORDER"); // Bias order

    float readnoise = psMetadataLookupF32(NULL, cell->concepts, "CELL.READNOISE");// CCD read noise, e
    if (isnan(readnoise)) {
        psWarning("CELL.READNOISE is not set; reverting to recipe value READNOISE.");
        readnoise = psMetadataLookupF32(&mdok, recipe, "READNOISE");
        if (!mdok) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find READNOISE in recipe.");
            *status = false;
            return NULL;
        }
    }

    psImage *signal = readout->image;
    psImage *variance = readout->variance;

    int numRows = signal->numRows;
    int numCols = signal->numCols;

    // Polynomial for bias
    psPolynomial1D *biasPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, biasOrder);
    for (int j = 0; j < biasOrder + 1; j++) {
        biasPoly->coeff[j] = biasRange * psRandomGaussian(rng);
    }

    psVector *biasRows = psVectorAlloc(numRows, PS_TYPE_F32); // Bias value, per row
    int biasOffset = 0.5 * numRows * psRandomUniform(rng); // Offset to prevent common pattern

    for (int y = 0; y < numRows; y++) {
        // Adjust bias level for this row
        biasRows->data.F32[y] = psPolynomial1DEval(biasPoly, (float)(y + biasOffset) /
                                                  (float)numRows - 0.5) + biasLevel;

        for (int x = 0; x < numCols; x++) {

            // Bias level
            signal->data.F32[y][x] += biasRows->data.F32[y];
            variance->data.F32[y][x] += PS_SQR(readnoise);

        }
    }
    psFree(biasPoly);

    return biasRows;
}


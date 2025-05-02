# include "ppSim.h"

// Add noise to an image
bool ppSimAddNoise(psImage *signal, // Signal image, modified and returned
		   psImage *variance,
		   const pmCell *cell,
		   const pmConfig *config,		       
		   const psRandom *rng // Random number generator
    )
{
    assert(signal->type.type == PS_TYPE_F32);
    assert(signal->numCols == variance->numCols && signal->numRows == variance->numRows);

    bool mdok;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    // the recipe should set GAIN to NAN, and only modify to override the concept value
    float gain = psMetadataLookupF32(&mdok, recipe, "GAIN"); // CCD gain, e/ADU
    if (isnan(gain)) {
	gain = psMetadataLookupF32(&mdok, cell->concepts, "CELL.GAIN");
	if (!mdok) {
	    psWarning("CELL.GAIN is not set; assuming gain of 1.0.");
	    gain = 1.0;
	}
    }

    // Add the noise into the image
    for (int y = 0; y < signal->numRows; y++) {
        for (int x = 0; x < signal->numCols; x++) {
	    // XXX is psRandomGaussian doing this reasonally optimally?
	    // (generate a static array with the cumulative distribution, use the
	    // random number to select a bin from the histogram)
            signal->data.F32[y][x] += sqrtf(variance->data.F32[y][x]) * ppSimRandomGaussianNorm(rng);
            signal->data.F32[y][x] /= gain; // Converting to ADU
	    // XXX the variance probably should be scaled as well -- we only are OK since GAIN = 1
        }
    }

    // XXX why return this??
    return true;
}


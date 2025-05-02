# include "ppSim.h"

// Apply saturation limit to image
bool ppSimSaturate(pmReadout *readout, // Image to apply saturation
		   const pmConfig *config // configuration data
    )
{
    bool mdok;

    psImage *image = readout->image;
    pmCell *cell = readout->parent;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    float saturation = psMetadataLookupF32(NULL, cell->concepts, "CELL.SATURATION");
    if (isnan(saturation)) {
	psWarning("CELL.SATURATION is not set; reverting to recipe value SATURATION.");
	saturation = psMetadataLookupF32(&mdok, recipe, "SATURATION");
	if (!mdok) {
	    psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find SATURATION in recipe.");
	    return false;
	}
    }

    for (int y = 0; y < image->numRows; y++) {
        for (int x = 0; x < image->numCols; x++) {
            if (image->data.F32[y][x] > saturation) {
                image->data.F32[y][x] = saturation;
            }
        }
    }
    return true;
}


# include "ppStatsInternal.h"

psExit ppStatsChip(psMetadata *fpaResults, // Metadata holding the fpa results
                   pmChip *chip,     // Chip for which to get statistics
                   psFits *fits,     // FITS file handle
                   pmFPAview *view,  // View for analysis
                   ppStatsData *data,// The data
                   pmConfig *config // Configuration
    )
{
    assert(fpaResults);
    assert(chip);
    assert(view);
    assert(data);
    assert(config);

    const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip

    // Check to see if this is a chip of interest
    if (!p_ppStatsDoThis(data->chips, chipName)) {
        return PS_EXIT_SUCCESS;
    }
    if (!chip->file_exists && !chip->data_exists) {
        return PS_EXIT_SUCCESS;
    }

    // Extract Header and Concept values from the Chip level

    // find or generate a metadata to hold the results
    bool mdok;                          // Status of MD lookup
    psMetadata *chipResults = psMemIncrRefCounter(psMetadataLookupMetadata(&mdok, fpaResults, chipName));
    if (!mdok || !chipResults) {
        chipResults = psMetadataAlloc();
    }

    // Extract Header values
    if (psListLength(data->headers)) {
        // extract from existing headers
        if (chip->hdu) {
            if (fits && !pmChipReadHeader(chip, fits, config)) {
                psError (PS_ERR_IO, false, "trouble reading chip header\n");
                psFree(chipResults);
                return PS_EXIT_DATA_ERROR;
            }
            pmHDU *hdu = chip->hdu;     // HDU for headers
            p_ppStatsGetMetadata(chipResults, hdu->header, data->headers);
        }
        // extract from data->analysis output MD entries
        if (psListLength(data->analysis)) {
            p_ppStatsGetAnalysis (chipResults, data->headers, chip->analysis, data->analysis);
        }
    }

    // Extract Concept values
    if (psListLength(data->concepts) > 0) {
        if (fits && chip->hdu && !pmChipReadHeader(chip, fits, config)) {
            psError (PS_ERR_IO, false, "trouble reading chip header\n");
            psFree(chipResults);
            return PS_EXIT_DATA_ERROR;
        }
        pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_ALL, false, false, config);
        p_ppStatsGetMetadata(chipResults, chip->concepts, data->concepts);
    }

    // check if we can legitimately iterate to the cell level
    psArray *cells = chip->cells;       // Array of cells
    if (view->cell >= cells->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Desired cell view (%d) doesn't match "
                "number of cells (%ld)\n", view->cell, cells->n);
        return PS_EXIT_CONFIG_ERROR;
    }

    // Iterate over cells (if view->cell is set, skip all others)
    for (int i = 0; i < cells->n; i++) {
        if ((view->cell >= 0) && (i != view->cell)) continue;
        pmCell *cell = cells->data[i];  // Cell of interest

	// XXX for now, skip the video cells (cell->readouts->n > 1)
	if ((cell->readouts->n > 1) && !data->doFirstReadout3D){
	  psWarning ("Skipping Video Cell for ppStatsCell");
	  continue;
	}

        psExit result = ppStatsCell(chipResults, cell, fits, view, data, config);
        if (result != PS_EXIT_SUCCESS) {
            psError(PS_ERR_UNKNOWN, false, "trouble with cell stats for %d\n", i);
            if (fits) {
                pmChipFreeData(chip);
            }
            psFree(chipResults);
            return result;
        }
    }

    if (fits) {
        pmChipFreeData(chip);
    }

    p_ppStatsAddToHierarchy(chipResults, fpaResults, chipName, "Results for chip");

    psFree (chipResults);
    return PS_EXIT_SUCCESS;
}

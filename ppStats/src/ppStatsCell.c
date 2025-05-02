# include "ppStatsInternal.h"

psExit ppStatsCell(psMetadata *chipResults, // Metadata holding the chip results
                   pmCell *cell,        // Cell for which to get statistics
                   psFits *fits,        // FITS file handle
                   pmFPAview *view,  // View for analysis
                   ppStatsData *data,   // The data
                   pmConfig *config // Configuration
    )
{
    assert(chipResults);
    assert(cell);
    assert(data);
    assert(config);

    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell

    // Check to see if this is a cell of interest
    if (!p_ppStatsDoThis(data->cells, cellName)) {
        return PS_EXIT_SUCCESS;
    }

#if 0
    // select the header unit for this cell
    // XXX why is this needed for Cell but not Chip?
    pmHDU *hdu = pmHDUFromCell(cell); // HDU for cell
    if (!hdu || hdu->blankPHU) {
        if (fits) {
            // No HDU means there's no data in this cell
            return PS_EXIT_SUCCESS;
        } else {
            psError(PS_ERR_UNKNOWN, false, "Can't find HDU for cell\n");
            return PS_EXIT_CONFIG_ERROR;
        }
    }
#endif

    // Extract Header and Concept values from the Cell and Readout->analysis level

    // find or generate a metadata to hold the results
    bool mdok;                          // Status of MD lookup
    psMetadata *cellResults = psMemIncrRefCounter(psMetadataLookupMetadata(&mdok, chipResults, cellName));
    if (!mdok || !cellResults) {
        cellResults = psMetadataAlloc();
    }

    // Extract Header values
    if (psListLength(data->headers)) {
        // extract from existing headers
        if (cell->hdu) {
            if (fits && !pmCellReadHeader(cell, fits, config)) {
                psError (PS_ERR_IO, false, "trouble reading cell header\n");
                psFree(cellResults);
                return PS_EXIT_DATA_ERROR;
            }
            pmHDU *hdu = cell->hdu;     // HDU for headers
            p_ppStatsGetMetadata(cellResults, hdu->header, data->headers);
        }
        // extract from data->analysis output MD entries
        if (psListLength(data->analysis)) {
            p_ppStatsGetAnalysis (cellResults, data->headers, cell->analysis, data->analysis);
        }
    }

    // Extract Concept values
    if (psListLength(data->concepts) > 0) {
        if (fits && cell->hdu && !pmCellReadHeader(cell, fits, config)) {
            psError (PS_ERR_IO, false, "trouble reading cell header\n");
            psFree(cellResults);
            return PS_EXIT_DATA_ERROR;
        }
        pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_ALL, false, config);
        p_ppStatsGetMetadata(cellResults, cell->concepts, data->concepts);
    }

    // If we want to measure pixel statistics, we must read (or have) the pixel data
    if (data->doStats || psListLength(data->summary)) {
        // Read the image pixel data
        if (fits && !pmCellRead(cell, fits, config)) {
            psError (PS_ERR_IO, false, "trouble reading cell data\n");
            psFree(cellResults);
            return PS_EXIT_DATA_ERROR;
        }
    }

    // check if we can legitimately iterate to the readout level
    psArray *readouts = cell->readouts; // Array of component readouts
    if (view->readout >= readouts->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Desired readout view (%d) doesn't match "
                "number of readouts (%ld)\n", view->readout, readouts->n);
        return PS_EXIT_CONFIG_ERROR;
    }
    if ((view->readout == -1) && (readouts->n > 1)) {
        psWarning("Multiple readouts (%ld) present in cell %s\n", readouts->n, cellName);
        if (!data->doFirstReadout3D) {
            psWarning ("Skipping Video Cell for ppStatsCell");
            return PS_EXIT_SUCCESS;
        }
    }

    // Iterate over readouts
    for (int i = 0; i < readouts->n; i++) {
        if ((view->readout >= 0) && (i != view->readout)) continue;
        pmReadout *readout = readouts->data[i];  // Cell of interest
        psExit result = ppStatsReadout(cellResults, readout, i, data, config);
        if (result != PS_EXIT_SUCCESS) {
            psError(PS_ERR_UNKNOWN, false, "trouble with readout stats for %d\n", i);
            if (fits) {
                pmCellFreeData(cell);
            }
            psFree(cellResults);
            return result;
        }
    }

    // Add the cell results to the chip
    p_ppStatsAddToHierarchy(cellResults, chipResults, cellName, "Results for cell");
    psFree (cellResults);
    if (fits) {
        pmCellFreeData(cell);
    }
    return PS_EXIT_SUCCESS;
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPattern.h"

#define MASK_BAD 0xFFFF                 // Mask for bad pixels

bool ppVizPatternLoop(ppVizPatternData *data // Run-time data
    )
{
    pmConfig *config = data->config;                                        // Configuration data
    pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPVIZPATTERN.INPUT", 0); // File with PSF

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1))) {
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            psError(PS_ERR_UNKNOWN, false, "Error loading data from files.");
            return false;
        }

        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1))) {
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                psError(PS_ERR_UNKNOWN, false, "Error loading data from files.");
                return false;
            }

            pmReadout *readout;         // Readout from cell
            while ((readout = pmFPAviewNextReadout(view, input->fpa, 1))) {
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    psError(PS_ERR_UNKNOWN, false, "Error loading data from files.");
                    return false;
                }
                if (!readout->data_exists) {
                    continue;
                }

                int numCols = 0, numRows = 0; // Size of image

                // Get size of image from concepts
                if (numCols == 0 || numRows == 0) {
                    numCols = psMetadataLookupS32(NULL, cell->concepts, "CELL.XSIZE");
                    numRows = psMetadataLookupS32(NULL, cell->concepts, "CELL.YSIZE");
                }

                // Get size of image from TRIMSEC
                if (numCols == 0 || numRows == 0) {
                    psRegion *trimsec = psMetadataLookupPtr(NULL, cell->concepts, "CELL.TRIMSEC");
                    numCols = trimsec->x1 - trimsec->x0;
                    numRows = trimsec->y1 - trimsec->y0;
                }

                if (numCols <= 0 || numRows <= 0) {
                    psError(PS_ERR_UNKNOWN, true, "Image size isn't set: %dx%d.", numCols, numRows);
                    return false;
                }

                readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
                psImageInit(readout->image, 0.0);

                if (!pmPatternRowApply(readout, MASK_BAD) || !pmPatternCellApply(readout, MASK_BAD)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to apply pattern correction.");
                    return false;
                }

                // Remove bias sections to avoid warnings
                psMetadataItem *biassec = psMetadataLookup(readout->parent->concepts, "CELL.BIASSEC");
                if (psListLength(biassec->data.V)) {
                    psFree(biassec->data.V);
                    biassec->data.V = psListAlloc(NULL);
                }

                readout->data_exists = true;
                readout->parent->data_exists = true;
                readout->parent->parent->data_exists = true;

                pmHDU *hdu = pmHDUGetLowest(input->fpa, chip, cell); // HDU for readout
                if (!hdu) {
                    psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find HDU for data.");
                    return false;
                }
                if (!hdu->header) {
                    hdu->header = psMetadataAlloc();
                }
                ppVizPatternVersionHeader(hdu->header);


                // Readout
                if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                    psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
                    return false;
                }
            }
            // Cell
            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
                return false;
            }
        }
        // Chip
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
            return false;
        }
    }
    // FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
        return false;
    }

    return true;
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

#include "ppBackground.h"

bool ppBackgroundLoop(ppBackgroundData *data // Run-time data
    )
{
    pmConfig *config = data->config;                                        // Configuration data
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPBACKGROUND.IMAGE", 0);
    pmFPAfile *patternFile = pmFPAfileSelectSingle(config->files, "PPBACKGROUND.PATTERN", 0); // File with data
    pmFPAfile *bgFile = pmFPAfileSelectSingle(config->files, "PPBACKGROUND.BACKGROUND", 0);   // File with bg
    pmFPA *patternMosaic = pmFPAConstruct(file->camera, file->cameraName); // Mosaicked FPA for pattern

    psImageMaskType maskBad = pmConfigMaskGet("DETECTOR", config); // Mask value to set

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }
    pmConceptsCopyFPA(patternMosaic, file->fpa, false, false);
    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, file->fpa, 1))) {
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            psError(psErrorCodeLast(), false, "Error loading data from files.");
            return false;
        }

        if (chip->cells->n > 1) {
            psError(PPBACKGROUND_ERR_CONFIG, true,
                    "Input image and background model should be chip-mosaicked");
            return false;
        }

        if (data->patternName) {
            pmFPAfileActivate(config->files, false, NULL);
            pmFPAfileActivate(config->files, true, "PPBACKGROUND.PATTERN");

            pmCell *patternCell;               // Cell with pattern data
            while ((patternCell = pmFPAviewNextCell(view, patternFile->fpa, 1))) {
                if (!patternCell->process || !patternCell->file_exists) {
                    continue;
                }
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    psError(psErrorCodeLast(), false, "Error loading data from files.");
                    return false;
                }

                pmReadout *readout;         // Readout with pattern data
                while ((readout = pmFPAviewNextReadout(view, patternFile->fpa, 1))) {
                    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                        psError(psErrorCodeLast(), false, "Error loading data from files.");
                        return false;
                    }
                    if (!readout->data_exists) {
                        continue;
                    }

                    int numCols = 0, numRows = 0; // Size of image

                    // Get size of image from concepts
                    if (numCols == 0 || numRows == 0) {
                        numCols = psMetadataLookupS32(NULL, patternCell->concepts, "CELL.XSIZE");
                        numRows = psMetadataLookupS32(NULL, patternCell->concepts, "CELL.YSIZE");
                    }
                    // Get size of image from TRIMSEC
                    if (numCols == 0 || numRows == 0) {
                        psRegion *trimsec = psMetadataLookupPtr(NULL, patternCell->concepts, "CELL.TRIMSEC");
                        numCols = trimsec->x1 - trimsec->x0;
                        numRows = trimsec->y1 - trimsec->y0;
                    }
                    if (numCols <= 0 || numRows <= 0) {
                        psError(PS_ERR_UNKNOWN, true, "Image size isn't set: %dx%d.", numCols, numRows);
                        return false;
                    }

                    readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
                    psImageInit(readout->image, 0.0);
                    readout->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
                    psImageInit(readout->mask, 0);

                    if (!pmPatternRowApply(readout, maskBad) || !pmPatternCellApply(readout, maskBad)) {
                        psError(psErrorCodeLast(), false, "Unable to apply pattern correction.");
                        return false;
                    }

                    // Remove bias sections to avoid warnings
                    psMetadataItem *biassec = psMetadataLookup(patternCell->concepts, "CELL.BIASSEC");
                    if (psListLength(biassec->data.V)) {
                        psFree(biassec->data.V);
                        biassec->data.V = psListAlloc(NULL);
                    }

                    readout->data_exists = true;
                    readout->parent->data_exists = true;
                    readout->parent->parent->data_exists = true;

                    // Readout
                    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                        psError(psErrorCodeLast(), false, "Error saving data to files.");
                        return false;
                    }
                }
                // Cell
                if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                    psError(psErrorCodeLast(), false, "Error saving data to files.");
                    return false;
                }
            }

            if (data->patternName) {
                pmChip *patternChip = pmFPAviewThisChip(view, patternFile->fpa); // Chip for pattern
                pmChip *mosaic = pmFPAviewThisChip(view, patternMosaic); // Chip for mosaicked pattern
                if (!mosaic->hdu && !mosaic->parent->hdu) {
                    pmFPAAddSourceFromView(patternMosaic, view, file->format);
                }
                if (!pmChipMosaic(mosaic, patternChip, true, maskBad)) {
                    psError(psErrorCodeLast(), false, "Unable to mosaic pattern correction");
                    return false;
                }
                pmFPAfileActivate(config->files, true, NULL);
            //            pmFPAfileActivate(config->files, false, "PPBACKGROUND.PATTERN");
            }
        }

        pmChip *patternChip = patternFile ? pmFPAviewThisChip(view, patternMosaic) : NULL; // Chip with pattern
        pmChip *bgChip = bgFile ? pmFPAviewThisChip(view, bgFile->fpa) : NULL; // Chip with background model
        if (!ppBackgroundRestore(chip, bgChip, patternChip, view, config, maskBad)) {
            psError(psErrorCodeLast(), false, "Unable to replace background");
            return false;
        }
        pmChipFreeData(patternChip);

        pmHDU *hdu = pmHDUGetLowest(file->fpa, chip, NULL); // HDU for chip
        if (!hdu) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find HDU for data.");
            return false;
        }
        if (!hdu->header) {
            hdu->header = psMetadataAlloc();
        }
        ppBackgroundVersionHeader(hdu->header);

        // Chip
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "Error saving data to files.");
            return false;
        }
    }
    // FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(psErrorCodeLast(), false, "Error saving data to files.");
        return false;
    }

    psFree(patternMosaic);

    if (data->stats) {
        psImageMaskType maskVal = 0;    // Bits to mask
        if (!pmConfigMaskSetBits(&maskVal, NULL, config)) {
            psError(psErrorCodeLast(), false, "Unable to find bits to mask");
            return false;
        }
        ppStatsFPA(data->stats, file->fpa, view, maskVal, config);
    }

    psFree(view);

    return true;
}

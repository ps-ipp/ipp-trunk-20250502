/** @file ppMergeCamera.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppMerge.h"

/**
 * Define an output file, with its own FPA
 */
bool outputFile(pmConfig *config,       ///< Configuration
                const char *name,       ///< Name of output file
                pmFPAfileType type,     ///< Type of file
                psMetadata *format,     ///< Camera format
                pmFPAview *view         ///< View for PHU
    )
{
    assert(config);
    assert(name && strlen(name) > 0);
    assert(view);

    // Output image
    pmFPA *fpa = pmFPAConstruct(config->camera, config->cameraName); ///< FPA to contain the output
    if (!fpa) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to construct an FPA from camera configuration.");
        return false;
    }

    // Chip selection: turn on only the chips specified.
    bool status;
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
        pmFPASelectChip (fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(fpa, chipNum, false)) {
                psError(PS_ERR_IO, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                psFree (chips);
                return false;
            }
        }
    }
    psFree (chips);

    // Cell selection: turn on only the cells specified.  Note that this affects the same cell
    // for all chips
    char *cellLine = psMetadataLookupStr(&status, config->arguments, "CELL_SELECTIONS");
    psArray *cells = psStringSplitArray (cellLine, ",", false);
    if (cells->n > 0) {
        for (int i = 0; i < fpa->chips->n; i++) {
            pmChip *chip = fpa->chips->data[i];
            pmChipSelectCell (chip, -1, true); // deselect all cells
            for (int j = 0; j < cells->n; j++) {
                int cellNum = atoi(cells->data[j]);
                if (! pmChipSelectCell(chip, cellNum, false)) {
                    psError(PS_ERR_IO, false, "Cell number %d doesn't exist in camera.\n", cellNum);
                    psFree (cells);
                    return false;
                }
            }
        }
    }
    psFree (cells);

    pmFPAfile *output = pmFPAfileDefineOutput(config, fpa, name);
    psFree(fpa);                        // Drop reference
    if (!output) {
        psError(PS_ERR_IO, false, "Unable to generate output file from %s", name);
        return false;
    }
    if (output->type != type) {
        psError(PS_ERR_IO, true, "%s is not of type %s", name, pmFPAfileStringFromType(type));
        return false;
    }
    output->save = true;

    if (!pmFPAAddSourceFromView(fpa, view, format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate output FPA.");
        return false;
    }

    return true;
}


bool ppMergeCamera(pmConfig *config)
{
    bool haveMasks = false;             // Do we have masks?
    bool haveVariances = false;           // Do we have variance maps?

    ppMergeType type = psMetadataLookupS32(NULL, config->arguments, "TYPE"); ///< Type of frame
    psMetadata *inputs = psMetadataLookupMetadata(NULL, config->arguments, "INPUTS"); ///< The inputs info
    psMetadataIterator *iter = psMetadataIteratorAlloc(inputs, PS_LIST_HEAD, NULL); ///< Iterator
    psMetadataItem *item;               ///< Item from iteration
    int numFiles = 0;                   ///< Number of files
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_METADATA) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    "Component %s of the input metadata is not of type METADATA", item->name);
            psFree(iter);
            return false;
        }

        psMetadata *input = item->data.md; ///< The input metadata of interest

        psString image = psMetadataLookupStr(NULL, input, "IMAGE"); ///< Name of image
        if (!image || strlen(image) == 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Component %s lacks IMAGE of type STR", item->name);
            psFree(iter);
            return false;
        }

        bool mdok;
        psString mask = psMetadataLookupStr(&mdok, input, "MASK"); // Name of mask
        psString variance = psMetadataLookupStr(&mdok, input, "VARIANCE"); // Name of variance map

        // Add the image file
        psArray *imageFiles = psArrayAlloc(1); ///< Array of filenames for this FPA
        imageFiles->data[0] = psMemIncrRefCounter(image);
        psMetadataAddArray(config->arguments, PS_LIST_TAIL, "IMAGE.FILENAMES", PS_META_REPLACE,
                           "Filenames of image files", imageFiles);
        psFree(imageFiles);

        bool found = false;             // Found the file?
        pmFPAfile *imageFile = pmFPAfileDefineFromArgs(&found, config, "PPMERGE.INPUT", "IMAGE.FILENAMES");
        if (!imageFile || !found) {
            psError(PS_ERR_UNKNOWN, false, "Unable to define file from image %d (%s)", numFiles, image);
            return false;
        }
        if (imageFile->type != PM_FPA_FILE_IMAGE) {
            psError(PS_ERR_IO, true, "PPMERGE.INPUT is not of type IMAGE");
            return false;
        }

        // Optionally add the mask file
        if (mask && strlen(mask) > 0) {
            psArray *maskFiles = psArrayAlloc(1); ///< Array of filenames for this FPA
            maskFiles->data[0] = psMemIncrRefCounter(mask);
            psMetadataAddArray(config->arguments, PS_LIST_TAIL, "MASK.FILENAMES", PS_META_REPLACE,
                               "Filenames of mask files", maskFiles);
            psFree(maskFiles);

            bool status;
            pmFPAfile *maskFile = pmFPAfileBindFromArgs(&status, imageFile, config, "PPMERGE.INPUT.MASK",
                                                        "MASK.FILENAMES");
            if (!status) {
                psError(PS_ERR_UNKNOWN, false, "Unable to define file from mask %d (%s)", numFiles, mask);
                return false;
            }
            if (maskFile->type != PM_FPA_FILE_MASK) {
                psError(PS_ERR_IO, true, "PPMERGE.INPUT.MASK is not of type MASK");
                return false;
            }
            haveMasks = true;
        }

        // Optionally add the variance file
        if (variance && strlen(variance) > 0) {
            haveVariances = true;
            psArray *varianceFiles = psArrayAlloc(1); // Array of filenames for this FPA
            varianceFiles->data[0] = psMemIncrRefCounter(variance);
            psMetadataAddArray(config->arguments, PS_LIST_TAIL, "VARIANCE.FILENAMES", PS_META_REPLACE,
                               "Filenames of variance files", varianceFiles);
            psFree(varianceFiles);

            bool status;
            pmFPAfile *varianceFile = pmFPAfileBindFromArgs(&status, imageFile, config,
                                                            "PPMERGE.INPUT.VARIANCE", "VARIANCE.FILENAMES");
            if (!status) {
                psError(PS_ERR_UNKNOWN, false, "Unable to define file from variance %d (%s)",
                        numFiles, variance);
                return false;
            }
            if (varianceFile->type != PM_FPA_FILE_VARIANCE) {
                psError(PS_ERR_IO, true, "PPMERGE.INPUT.VARIANCE is not of type VARIANCE");
                return false;
            }
            haveVariances = true;
        }

        numFiles++;
    }
    psFree(iter);
    psMetadataRemoveKey(config->arguments, "IMAGE.FILENAMES");
    if (psMetadataLookup(config->arguments, "MASK.FILENAMES")) {
        psMetadataRemoveKey(config->arguments, "MASK.FILENAMES");
    }
    if (psMetadataLookup(config->arguments, "VARIANCE.FILENAMES")) {
        psMetadataRemoveKey(config->arguments, "VARIANCE.FILENAMES");
    }
    if (psMetadataLookup(config->arguments, "PSF.FILENAMES")) {
        psMetadataRemoveKey(config->arguments, "PSF.FILENAMES");
    }

    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INPUTS.NUM", 0, "Number of input files", numFiles);
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "INPUTS.MASKS", 0, "Got input masks?", haveMasks);
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "INPUTS.VARIANCES", 0,
                      "Got input variances?", haveVariances);


    // Check that all the inputs are consistent

// Check an FPA level to ensure the camera format and PHU are consistent across all input files
#define CHECK_LEVEL(HDU, CHIP, CELL) { \
    if (HDU) { \
        if (!phuView) { \
            phuView = pmFPAviewAlloc(0); \
            phuView->chip = CHIP; \
            phuView->cell = CELL; \
        } else if ((phuView->chip != (CHIP)) && (phuView->cell != (CELL))) { \
            psError(PS_ERR_UNKNOWN, true, "Differing PHU for input %d", i); \
            psFree(phuView); \
            return false; \
        } \
        if (!format) { \
            format = (HDU)->format; \
        } else if (format != (HDU)->format) { \
	  psError(PS_ERR_UNKNOWN, true, "Camera format %d doesn't match: %p vs %p on %s %s", \
		  i, format, (HDU)->format, (HDU)->extname, psMetadataLookupStr(NULL,(HDU)->header,"FILENAME")); \
            psFree(phuView); \
            return false; \
        } \
        continue; \
    } \
}
    // CZW: 2012-04-10 Version of above to allow different camera formats to be processed together.
/* #define CHECK_LEVEL(HDU, CHIP, CELL) { \ */
/*     if (HDU) { \ */
/*         if (!phuView) { \ */
/*             phuView = pmFPAviewAlloc(0); \ */
/*             phuView->chip = CHIP; \ */
/*             phuView->cell = CELL; \ */
/*         } else if ((phuView->chip != (CHIP)) && (phuView->cell != (CELL))) { \ */
/*             psError(PS_ERR_UNKNOWN, true, "Differing PHU for input %d", i); \ */
/*             psFree(phuView); \ */
/*             return false; \ */
/*         } \ */
/*         if (!format) { \ */
/*             format = (HDU)->format; \ */
/*         } else if (format != (HDU)->format) { \ */
/*         } \ */
/*         continue; \ */
/*     } \ */
/* } */

    psMetadata *format = NULL;          ///< Camera format
    pmFPAview *phuView = NULL;          ///< View to PHU
    for (int i = 0; i < numFiles; i++) {
        pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i); ///< File of interest
        pmFPA *fpa = input->fpa;        ///< FPA of interest
        CHECK_LEVEL(fpa->hdu, -1, -1);
        psArray *chips = fpa->chips;    ///< Array of chips
        for (int j = 0; j < chips->n; j++) {
            pmChip *chip = chips->data[j]; ///< Chip of interest
            CHECK_LEVEL(chip->hdu, j, -1);
            psArray *cells = chip->cells;   ///< Array of cells
            for (int k = 0; k < cells->n; k++) {
                pmCell *cell = cells->data[k]; ///< Cell of interest
                CHECK_LEVEL(cell->hdu, j, k);
            }
        }
    }
    if (!phuView || !format) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find PHU for input files.");
        psFree(phuView);
        return false;
    }

    // Cull chips and cells that don't have data
    // Otherwise the abundance of metadata in the concepts (esp. for GPC) can overload the memory
    for (int i = 0; i < numFiles; i++) {
        pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i); ///< File of interest
        pmFPA *fpa = input->fpa;        ///< FPA of interest
        psArray *chips = fpa->chips; ///< Array of chips in output
        for (int i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i]; ///< Chip of interest
            psArray *cells = chip->cells; ///< Array of cells
            int culled = 0;             ///< Number of culled cells
            for (int j = 0; j < cells->n; j++) {
                pmCell *cell = cells->data[j];
                pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); ///< HDU for cell
                if (!hdu || hdu->blankPHU) {
                    cell->data_exists = false;
                    cell->file_exists = false;
                    culled++;
                    if (cell->concepts) {
                        psFree(cell->concepts);
                        cell->concepts = NULL;
                    }
                }
            }
            if (culled == cells->n) {
                chip->data_exists = false;
                chip->file_exists = false;
                if (chip->concepts) {
                    psFree(chip->concepts);
                    chip->concepts = NULL;
                }
            }
        }
    }

    // Count the cells
    {
        int numCells = 0;               ///< Number of cells
        pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", 0); ///< Representative file
        pmFPA *fpa = input->fpa;        ///< FPA for file
        psArray *chips = fpa->chips; ///< Array of chips
        for (int i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i]; ///< Chip of interest
            psArray *cells = chip->cells; ///< Array of cells
            for (int j = 0; j < cells->n; j++) {
                pmCell *cell = cells->data[j]; ///< Cell of interest
                pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); ///< HDU that would have data
                if (hdu && !hdu->blankPHU) {
                    numCells++;
                }
            }
        }

        psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INPUTS.CELLS", 0, "Number of cells in input",
                         numCells);
    }

    psString outName = ppMergeOutputFile(config); ///< Name of output file

    pmFPAfileType fileType = PM_FPA_FILE_NONE; ///< Type of output file
    switch (type) {
      case PPMERGE_TYPE_BIAS:
        fileType = PM_FPA_FILE_IMAGE;
        break;
      case PPMERGE_TYPE_DARK:
        fileType = PM_FPA_FILE_DARK;
        break;
      case PPMERGE_TYPE_MASK:
        fileType = PM_FPA_FILE_MASK;
        break;
      case PPMERGE_TYPE_CTEMASK:
        fileType = PM_FPA_FILE_MASK;
        break;
      case PPMERGE_TYPE_SHUTTER:
        fileType = PM_FPA_FILE_IMAGE;
        break;
      case PPMERGE_TYPE_FLAT:
        fileType = PM_FPA_FILE_IMAGE;
        break;
      case PPMERGE_TYPE_NOISEMAP:
	fileType = PM_FPA_FILE_IMAGE;
	break;
      case PPMERGE_TYPE_FRINGE:
        fileType = PM_FPA_FILE_FRINGE;
        break;
      default:
        psAbort("Unknown frame type: %x", type);
    }

    if (!outputFile(config, outName, fileType, format, phuView)) {
        psFree(outName);
        psFree(phuView);
        return false;
    }
    psFree(outName);

    if (!outputFile(config, "PPMERGE.OUTPUT.SIGMA", PM_FPA_FILE_IMAGE, format, phuView)) {
        psFree(phuView);
        return false;
    }

    if (!outputFile(config, "PPMERGE.OUTPUT.COUNT", PM_FPA_FILE_IMAGE, format, phuView)) {
        psFree(phuView);
        return false;
    }

    psFree(phuView);

    return true;
}

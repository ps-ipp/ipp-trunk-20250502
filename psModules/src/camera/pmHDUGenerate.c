#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmHDUUtils.h"
#include "pmConcepts.h"
#include "pmConceptsStandard.h"
#include "pmHDUGenerate.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Add cells in a chip to a list
static bool addCellsFromChip(psList *list, // List of cells
                             const pmChip *chip // The chip from which to add cells
                            )
{
    assert(list);
    assert(chip);

    psArray *cells = chip->cells;       // Array of cells
    bool result = true;                 // Result of adding cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // A cell
        if (!cell->hdu) {               // Don't add cells that have their own HDU
            result |= psListAdd(list, PS_LIST_TAIL, cell);
        }
    }

    return result;
}

// Add cells in an FPA to a list
static bool addCellsFromFPA(psList *list, // List of cells
                            const pmFPA *fpa // The FPA from which to add cells
                           )
{
    assert(list);
    assert(fpa);

    psArray *chips = fpa->chips;        // Array of chips
    bool result = true;                 // Result of adding cells
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // A chip
        if (! chip->hdu) {              // Don't add chips that have their own HDU
            result |= addCellsFromChip(list, chip);
        }
    }

    return result;
}

// Get the maximum extent of the HDU from the trimsec and biassecs
static bool sizeHDU(int *xSize, int *ySize, // Size of HDU
                    psList *cells       // List of cells
                   )
{
    psListIterator *cellsIter = psListIteratorAlloc(cells, PS_LIST_HEAD, false); // Iterator for cells
    pmCell *cell = NULL;                // The cell from iteration
    bool mdok = true;                   // Status of MD lookup
    *xSize = 0;
    *ySize = 0;
    while ((cell = psListGetAndIncrement(cellsIter))) {
        psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
        if (mdok && trimsec && !psRegionIsNaN(*trimsec)) {
            *xSize = PS_MAX(trimsec->x1, *xSize);
            *ySize = PS_MAX(trimsec->y1, *ySize);
        } else {
            psFree(cellsIter);
            return false;
        }
        psList *biassecs = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.BIASSEC"); // Bias sections
        if (mdok && biassecs) {
            psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, false); // Iterator
            psRegion *biassec = NULL;   // The bias section
            while ((biassec = psListGetAndIncrement(biassecsIter))) {
                if (!psRegionIsNaN(*biassec)) {
                    *xSize = PS_MAX(biassec->x1, *xSize);
                    *ySize = PS_MAX(biassec->y1, *ySize);
                } else {
                    psFree(biassecsIter);
                    psFree(cellsIter);
                    return false;
                }
            }
            psFree(biassecsIter);
        }
    }
    psFree(cellsIter);

    return (*xSize != 0 && *ySize != 0);
}


static psRegion *sectionForImage(int *position, // Position on the output image, updated
                                 const psImage *image, // Image containing the sizes and offsets
                                 int readdir // Read direction, 1=rows, 2=cols
                                )
{
    psRegion *region = NULL;            // The region to return
    switch (readdir) {
    case 1:                           // Read direction is rows
        region = psRegionAlloc(*position, *position + image->numCols, image->row0,
                               image->row0 + image->numRows);
        *position += image->numCols;
        break;
    case 2:                           // Read direction is columns
        region = psRegionAlloc(image->col0, image->col0 + image->numCols, *position,
                               *position + image->numRows);
        *position += image->numRows;
        break;
    default:
        psAbort("Shouldn't ever get here!\n");
    }

    return region;
}

static bool doBiasSections(int *position, // Position on the output image, updated
                           int *readdir,// Read direction for cells
                           pmCell *cell // Cell
                          )
{
    psMetadataItem *biassecItem = psMetadataLookup(cell->concepts, "CELL.BIASSEC"); // Bias sections
    if (!biassecItem) {
        psWarning("CELL.BIASSEC has not been initialised in cell --- ignored.\n");
        return false;
    }
    psFree(biassecItem->data.V);        // Blow away the old list
    psList *biassecs = psListAlloc(NULL);
    biassecItem->data.V = biassecs;

    bool mdok = true;                   // Status of MD lookup
    int cellreaddir = psMetadataLookupS32(&mdok, cell->concepts, "CELL.READDIR"); // Read direction
    if (!mdok || (cellreaddir != 1 && cellreaddir != 2)) {
        // Probably unnecessary, but just in case....
        psWarning("CELL.READDIR is not set in cell --- ignored.\n");
        return false;
    }
    if (*readdir == 0) {
        *readdir = cellreaddir;
    } else if (*readdir != cellreaddir) {
        psWarning("CELL.READDIR does not match read direction for HDU --- ignored.\n");
        return false;
    }

    pmReadout *readout = cell->readouts->data[0]; // The first readout, as representative
    psList *biases = readout->bias; // The bias images from the readout

    psListIterator *biasIter = psListIteratorAlloc(biases, PS_LIST_HEAD, true); // Iterator for biases
    psImage *bias = NULL;       // Bias image from iteration
    while ((bias = psListGetAndIncrement(biasIter))) {
        // Construct a region
        psRegion *biassec = sectionForImage(position, bias, *readdir);
        psListAdd(biassecs, PS_LIST_TAIL, biassec);
        psFree(biassec);        // Drop reference
    }
    psFree(biasIter);

    return true;
}

// Attempt to read CELL.TRIMSEC and CELL.BIASSEC from the cell format if they are specified by VALUE
static bool readTrimBias(psList *cells // List of cells below the HDU
    )
{
    bool mdok;                          // Status of MD lookup
    psListIterator *cellsIter = psListIteratorAlloc(cells, PS_LIST_HEAD, false); // Iterator for cells
    pmCell *cell = NULL;                // Cell from iteration
    bool allFixed = true;               // We're able to fix all TRIMSEC and BIASSEC
    while ((cell = psListGetAndIncrement(cellsIter))) {
        psMetadataItem *trimsecItem = psMetadataLookup(cell->concepts, "CELL.TRIMSEC"); // Item with trimsec
        if (!trimsecItem || trimsecItem->type != PS_DATA_REGION) {
            psWarning("CELL.TRIMSEC has not been initialised in cell --- ignored.\n");
            return false;
        }
        psRegion *trimsec = trimsecItem->data.V; // Trim section
        if (!trimsec || psRegionIsNaN(*trimsec)) {
            const char *trimsecSource = psMetadataLookupStr(&mdok, cell->config, "CELL.TRIMSEC.SOURCE");
            if (strcmp(trimsecSource, "VALUE") == 0) {

                const char *trimsecStr = psMetadataLookupStr(&mdok, cell->config, "CELL.TRIMSEC");
                if (!trimsec) {
                    trimsec = trimsecItem->data.V = psRegionAlloc(NAN, NAN, NAN, NAN);
                }
                *trimsec = psRegionFromString(trimsecStr);
            } else {
                allFixed = false;
            }
        }

        psMetadataItem *biassecItem = psMetadataLookup(cell->concepts, "CELL.BIASSEC"); // Bias sections
        if (!biassecItem) {
            psWarning("CELL.BIASSEC has not been initialised in cell --- ignored.\n");
            return false;
        }
        psList *biassecs = biassecItem->data.V;
        if (!biassecs || biassecs->n != 0) {
            allFixed = false;
            continue;
        }

        const char *biassecSource = psMetadataLookupStr(&mdok, cell->config, "CELL.BIASSEC.SOURCE");
        if (biassecSource && strcmp(biassecSource, "VALUE") == 0) {
            const char *biassecStr = psMetadataLookupStr(&mdok, cell->config, "CELL.BIASSEC");
            psFree(biassecItem->data.V);
            biassecItem->data.V = p_pmConceptParseRegions(biassecStr);
        } else {
            allFixed = false;
        }
    }
    psFree(cellsIter);

    return allFixed;
}


// Generate CELL.TRIMSEC and CELL.BIASSEC for the cells
static bool generateTrimBias(psList *cells // List of cells below the HDU
                            )
{
    pmCell *cell = NULL;                // Cell from iteration
    int numCells = cells->n;            // Number of cells
    int cellNum = 0;                    // The cell number
    int position = 0;                   // Position on the image
    bool mdok = true;                   // Status of MD lookup
    int readdir = 0;                    // Read direction (1=rows, 2=cols)

    // First run through to do the LHS biases
    psListIterator *cellsIter = psListIteratorAlloc(cells, PS_LIST_HEAD, false); // Iterator for cells
    bool done = false;                  // Done with iteration (due to being halfway through)?
    while ((cell = psListGetAndIncrement(cellsIter)) && !done) {
        if (cellNum <= numCells/2 - 1) {
            doBiasSections(&position, &readdir, cell);
            cellNum++;
        } else {
            done = true;
        }
    }

    // Second run through to do the trim sections
    psListIteratorSet(cellsIter, PS_LIST_HEAD);
    while ((cell = psListGetAndIncrement(cellsIter))) {
        psMetadataItem *trimsecItem = psMetadataLookup(cell->concepts, "CELL.TRIMSEC"); // Item with trimsec
        if (!trimsecItem || trimsecItem->type != PS_DATA_REGION) {
            psWarning("CELL.TRIMSEC has not been initialised in cell --- ignored.\n");
            continue;
        }
        psRegion *trimsec = trimsecItem->data.V; // Trim section

        int cellreaddir = psMetadataLookupS32(&mdok, cell->concepts, "CELL.READDIR"); // Read direction
        if (!mdok || (cellreaddir != 1 && cellreaddir != 2)) {
            // Probably unnecessary, but just in case....
            psWarning("CELL.READDIR is not set in cell --- ignored.\n");
            continue;
        }
        if (readdir == 0 && mdok && cellreaddir != 0) {
            readdir = cellreaddir;
        } else if (readdir != cellreaddir) {
            psWarning("CELL.READDIR for cells within the HDU do not match!\n");
            cellreaddir = readdir;
        }

        pmReadout *readout = cell->readouts->data[0]; // The first readout, as representative
        // The proper image, used to get the size
        psImage *image = readout->image ? readout->image : (readout->mask ? readout->mask : readout->variance);
        if (!image) {
            continue;
        }
        if (readout->mask &&
                (readout->mask->numCols != image->numCols || readout->mask->numRows != image->numRows)) {
            psWarning("Image and mask have different sizes (%dx%d vs %dx%d)!\n",
                     image->numCols, image->numRows, readout->mask->numCols, readout->mask->numRows);
        }
        if (readout->variance &&
                (readout->variance->numCols != image->numCols || readout->variance->numRows != image->numRows)) {
            psWarning("Image and variance have different sizes (%dx%d vs %dx%d)!\n",
                     image->numCols, image->numRows, readout->variance->numCols, readout->variance->numRows);
        }
        // New reference
        trimsec = sectionForImage(&position, image, cellreaddir);
        psFree(trimsecItem->data.V);
        trimsecItem->data.V = trimsec;
    }

    // A final run through to do the RHS biases
    psListIteratorSet(cellsIter, cellNum);
    while ((cell = psListGetAndIncrement(cellsIter))) {
        doBiasSections(&position, &readdir, cell);
    }

    // Clean up
    psFree(cellsIter);

    return (position > 0);
}

// Check the type for a current image against a previous type
static psElemType checkTypes(psElemType previous, // Previously defined type, or 0
                             psElemType current // Current type
                            )
{
    if (previous == 0) {
        return current;
    }

    if (previous != current) {
        psWarning("Images within the HDU are of different types (%x vs %x) --- promoting\n",
                  previous, current);
        return PS_MAX(previous, current);
    }

    return previous;
}


// Paste the source image into the target, according to the provided region.  The source is then updated to
// reference the region within the target.
static psImage *pasteImage(psImage *target, // Target image, into which the paste is made
                           psImage *source,// Source image, from which the paste is made, and then changed
                           psRegion *region // Image section into which to paste
                          )
{
    if (source->numCols != region->x1 - region->x0 || source->numRows != region->y1 - region->y0) {
        psString regionString = psRegionToString(*region);
        psWarning("Image size (%dx%d) does not match region (%s).\n",
                 source->numCols, source->numRows, regionString);
        psFree(regionString);
    }
    psImageOverlaySection(target, source, region->x0, region->y0, "=");

    // Reference the HDU version, so that subsequent changes will touch the HDU
    return psImageSubset(target, *region);
}


// Generate the HDU, given a list of cells below that HDU.  This is the main engine function, that does all
// the work.
static bool generateHDU(pmHDU *hdu,     // HDU to generate
                        psList *cells   // List of cells
                       )
{
    // Check the number of readouts is consistent within the HDU
    int numReadouts = -1;               // Number of readouts
    psElemType imageType = 0;           // Type of readout images
    psElemType maskType = 0;            // Type of readout masks
    psElemType varianceType = 0;        // Type of readout variances
    {
        psListIterator *iter = psListIteratorAlloc(cells, PS_LIST_HEAD, false); // Iterator for cells
        pmCell *cell = NULL;                // The cell from iteration
        while ((cell = psListGetAndIncrement(iter)))
        {
            psArray *readouts = cell->readouts;
            if (numReadouts == -1) {
                numReadouts = readouts->n;
            } else if (readouts->n != numReadouts) {
                psError(PS_ERR_IO, true, "Number of readouts doesn't match: %ld vs %d\n", readouts->n,
                        numReadouts);
                return false;
            }
            for (int i = 0; i < numReadouts; i++) {
                pmReadout *readout = readouts->data[i]; // The readout
                if (!readout) {
                    continue;
                }

                if (!hdu->images && readout->image) {
                    imageType = checkTypes(imageType, readout->image->type.type);
                }
                if (!hdu->masks && readout->mask) {
                    maskType = checkTypes(maskType, readout->mask->type.type);
                }
                if (!hdu->variances && readout->variance) {
                    varianceType = checkTypes(varianceType, readout->variance->type.type);
                }
            }
        }
        psFree(iter);
    }
    if (numReadouts == 0 || (imageType == 0 && maskType == 0 && varianceType == 0)) {
        // Nothing from which to create an HDU
        // psError(PS_ERR_IO, true, "Nothing from which to create an HDU\n");
        psWarning("Nothing from which to create an HDU, must be empty\n");
        return true;
    }

    // Get the size of the HDU, either from existing trimsec and biassec, or generate these and try again
    int xSize = 0, ySize = 0;           // Size of HDU
    if (!sizeHDU(&xSize, &ySize, cells) &&
        !(readTrimBias(cells) && sizeHDU(&xSize, &ySize, cells)) &&
        !(generateTrimBias(cells) && sizeHDU(&xSize, &ySize, cells))) {
        psError(PS_ERR_IO, true, "Unable to determine size of HDU!\n");
        return false;
    }

    // Generate the HDU
    if (imageType) {
        hdu->images = psArrayAlloc(numReadouts);
        for (int i = 0; i < numReadouts; i++) {
            psImage *image = psImageAlloc(xSize, ySize, imageType);
            psImageInit(image, 0.0);
            hdu->images->data[i] = image;
        }
    }
    if (maskType) {
        hdu->masks = psArrayAlloc(numReadouts);
        for (int i = 0; i < numReadouts; i++) {
            psImage *mask = psImageAlloc(xSize, ySize, maskType);
            psImageInit(mask, 0);
            hdu->masks->data[i] = mask;
        }
    }
    if (varianceType) {
        hdu->variances = psArrayAlloc(numReadouts);
        for (int i = 0; i < numReadouts; i++) {
            psImage *variance = psImageAlloc(xSize, ySize, varianceType);
            psImageInit(variance, 0.0);
            hdu->variances->data[i] = variance;
        }
    }

    // Insert the pixels into the HDU
    {
        psListIterator *iter = psListIteratorAlloc(cells, PS_LIST_HEAD, false); // Iterator for cells
        pmCell *cell = NULL;           // The cell from iteration
        bool mdok = true;               // Result of MD lookup
        while ((cell = psListGetAndIncrement(iter)))
        {
            psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
            if (!mdok || !trimsec) {
                psAbort("Shouldn't ever get here --- CELL.TRIMSEC should have been set above.\n");
            }
            psList *biassecs = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.BIASSEC"); // Bias secionts
            if (!mdok || !biassecs) {
                psAbort("Shouldn't ever get here --- CELL.BIASSEC should have been set above.\n");
            }
            psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, false); // Iterator

            psArray *readouts = cell->readouts; // Array of readouts

            psArray *hduImages = hdu->images; // Array of images in the HDU
            psArray *hduMasks = hdu->masks; // Array of masks in the HDU
            psArray *hduVariances = hdu->variances; // Array of variances in the HDU
            for (int i = 0; i < readouts->n; i++) {
                pmReadout *readout = readouts->data[i]; // The readout of interest
                if (!readout) {
                    continue;
                }

                if (readout->image) {
                    psImage *new = pasteImage(hduImages->data[i], readout->image, trimsec);
                    psFree(readout->image); 
                    readout->image = new;
                }
                if (readout->mask) {
                    psImage *new = pasteImage(hduMasks->data[i], readout->mask, trimsec);
                    psFree(readout->mask);
                    readout->mask = new;
                }
                if (readout->variance) {
                    psImage *new = pasteImage(hduVariances->data[i], readout->variance, trimsec);
                    psFree(readout->variance);
                    readout->variance = new;
                }

                if (!biassecs || !readout->bias) {
                    // No need to worry about bias section
                    continue;
                }

                if (biassecs->n != readout->bias->n) {
                    psWarning("Number of bias sections (%ld) and number of biases (%ld) do not match.\n",
                              biassecs->n, readout->bias->n);
                }
                psListIterator *biasIter = psListIteratorAlloc(readout->bias, PS_LIST_HEAD, false); // Iteratr
                psImage *bias = NULL;   // Bias image, from iteration
                psListIteratorSet(biassecsIter, PS_LIST_HEAD);
                psRegion *biassec = NULL; // Bias region, from iteration
                psList *newBias = psListAlloc(NULL); // New list of bias images
                while ((bias = psListGetAndIncrement(biasIter)) &&
                        (biassec = psListGetAndIncrement(biassecsIter))) {
                    psImage *new = pasteImage(hduImages->data[i], bias, biassec);
                    psListAdd(newBias, PS_LIST_TAIL, new);
                    psFree(new);        // Drop reference
                }
                psFree(biasIter);

                // Add on the new list of bias images
                psFree(readout->bias);
                readout->bias = newBias;
            }
            psFree(biassecsIter);
        } // Iterating over cells within the HDU
        psFree(iter);
    }
    return true;
}

// Return the level that an extension applies to
static pmFPALevel extensionLevel(const pmHDU *hdu // HDU to check
                                )
{
    if (!hdu->format) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "HDU does not have a camera format.\n");
        return PM_FPA_LEVEL_NONE;
    }
    bool mdok = true;                   // Status of MD lookup
    psMetadata *file = psMetadataLookupMetadata(&mdok, hdu->format, "FILE"); // File info for camera format
    if (!mdok || !file) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Can't file FILE information for camera format "
                "configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }
    psString extensions = psMetadataLookupStr(&mdok, file, "EXTENSIONS"); // Where the HDUs are
    if (!mdok || !extensions || strlen(extensions) == 0) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Can't find EXTENSIONS in the FILE information of the camera "
                "format configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }
    if (strcasecmp(extensions, "CELL") == 0) {
        return PM_FPA_LEVEL_CELL;
    }
    if (strcasecmp(extensions, "CHIP") == 0) {
        return PM_FPA_LEVEL_CHIP;
    }
    if (strcasecmp(extensions, "FPA") == 0) {
        return PM_FPA_LEVEL_FPA;
    }
    if (strcasecmp(extensions, "NONE") == 0) {
        return PM_FPA_LEVEL_NONE;
    }
    // No idea what it is
    psError(PS_ERR_IO, true, "EXTENSIONS (%s) in FILE information is not FPA, CHIP, CELL or NONE.\n",
            extensions);
    return PM_FPA_LEVEL_NONE;
}

// Generate HDU for cells belonging to a chip --- just an iterator
static bool generateForCells(pmChip *chip // Chip for which to generate HDUs
                            )
{
    psArray *cells = chip->cells;       // Array of cells
    bool status = true;                 // Status of HDU generation
    for (long i = 0; i < cells->n; i++) {
        status |= pmHDUGenerateForCell(cells->data[i]);
    }
    return status;
}

// Generate HDU for chips belonging to an FPA --- just an iterator
static bool generateForChips(pmFPA *fpa // FPA for which to generate HDUs
                            )
{
    psArray *chips = fpa->chips;        // Array of chips
    bool status = true;                 // Status of HDU generation
    for (long i = 0; i < chips->n; i++) {
        status |= pmHDUGenerateForChip(chips->data[i]);
    }
    return status;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmHDUGenerateForCell(pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);

    // Get the HDU and a list of cells below it
    pmHDU *hdu = pmHDUFromCell(cell); // The HDU in the cell
    if (!hdu) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Can't find an HDU for cell.\n");
        return false;
    }
    if (hdu->images && hdu->masks && hdu->variances) {
        // It's already here!
        return true;
    }

    pmFPALevel extLevel = extensionLevel(hdu);
    switch (extLevel) {
    case PM_FPA_LEVEL_NONE:
    case PM_FPA_LEVEL_CELL: {
            psList *cells = psListAlloc(NULL); // List of cells below the HDU

            if (cell->hdu) {
                psListAdd(cells, PS_LIST_TAIL, cell);
            } else {
                pmChip *chip = cell->parent;    // The parent chip
                if (chip->hdu) {
                    addCellsFromChip(cells, chip);
                } else {
                    pmFPA *fpa = chip->parent;  // The parent FPA
                    if (fpa->hdu) {
                        addCellsFromFPA(cells, fpa);
                    }
                }
            }
            if (cells->n == 0) {
                // Nothing to do
		psFree (cells);
                return true;
            }
	    bool status = generateHDU(hdu, cells);
	    psFree (cells);
            return status;
        }
    case PM_FPA_LEVEL_CHIP:
    case PM_FPA_LEVEL_FPA:
        return pmHDUGenerateForChip(cell->parent);
    default:
        psAbort("Shouldn't ever get here: check your camera format configuration.\n");
    }
    return false;
}

bool pmHDUGenerateForChip(pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);

    // Get the HDU and a list of cells below it
    pmHDU *hdu = pmHDUFromChip(chip);   // The HDU in the chip
    if (!hdu) {
        // Nothing here; need to look further down
        return generateForCells(chip);
    }
    if (hdu->images && hdu->masks && hdu->variances) {
        // It's already here!
        return true;
    }

    pmFPALevel extLevel = extensionLevel(hdu);
    switch (extLevel) {
    case PM_FPA_LEVEL_CELL:
        // Work on lower levels
        return generateForCells(chip);
    case PM_FPA_LEVEL_NONE:
    case PM_FPA_LEVEL_CHIP: {
            // Work on this level
            psList *cells = psListAlloc(NULL);  // List of cells below the HDU
            if (chip->hdu) {
                addCellsFromChip(cells, chip);
            } else {
                pmFPA *fpa = chip->parent;  // The parent FPA
                if (fpa->hdu) {
                    addCellsFromFPA(cells, fpa);
                }
            }
            if (cells->n == 0) {
                // Nothing to do
		psFree (cells);
                return true;
            }

	    bool status = generateHDU(hdu, cells);
	    psFree (cells);
            return status;
        }
    case PM_FPA_LEVEL_FPA:
        // Work on higher levels
        return pmHDUGenerateForFPA(chip->parent);
    default:
        psAbort("Shouldn't ever get here: check your camera format configuration.\n");
    }
    return false;
}


bool pmHDUGenerateForFPA(pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    // Get the HDU and a list of cells below it
    pmHDU *hdu = pmHDUFromFPA(fpa);     // The HDU in the FPA
    if (!hdu) {
        // Nothing here; need to look further down
        return generateForChips(fpa);
    }
    if (hdu->images && hdu->masks && hdu->variances) {
        // It's already here!
        return true;
    }

    pmFPALevel extLevel = extensionLevel(hdu);
    switch (extLevel) {
    case PM_FPA_LEVEL_CELL:
    case PM_FPA_LEVEL_CHIP:
        // Work on lower levels
        return generateForChips(fpa);
    case PM_FPA_LEVEL_NONE:
    case PM_FPA_LEVEL_FPA: {
            // Work on this level
            psList *cells = psListAlloc(NULL); // List of cells below the HDU
            if (fpa->hdu) {
                addCellsFromFPA(cells, fpa);
            }
            if (cells->n == 0) {
                // Nothing to do
		psFree(cells);
                return true;
            }
            bool status = generateHDU(hdu, cells);
	    psFree(cells);
	    return status;
        }
    default:
        psAbort("Shouldn't ever get here: check your camera format configuration.\n");
    }
    return false;
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAUtils.h"
#include "pmHDUUtils.h"
#include "pmConceptsCopy.h"
#include "pmFPACopy.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static functions and macros
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Copy the value for a concept
#define COPY_CONCEPT(TARGET, SOURCE, NAME, TYPE) { \
    psMetadataItem *targetItem = psMetadataLookup(TARGET, NAME); \
    psMetadataItem *sourceItem = psMetadataLookup(SOURCE, NAME); \
    targetItem->data.TYPE = sourceItem->data.TYPE; }

// Find the blank (image-less) PHU, given a cell.
static pmHDU *findBlankPHU(const pmCell *cell // The cell for which to find the PHU
                          )
{
    assert(cell);

    if (cell->hdu && cell->hdu->blankPHU) {
        return cell->hdu;
    }
    pmChip *chip = cell->parent;        // The parent chip
    if (chip->hdu && chip->hdu->blankPHU) {
        return chip->hdu;
    }
    pmFPA *fpa = chip->parent;  // The parent FPA
    if (fpa->hdu && fpa->hdu->blankPHU) {
        return fpa->hdu;
    }

    return NULL;
}

// copy one of the psImage components of the readout
static void readoutCopyComponent(psImage **target, // Image to which to copy
                                 const psImage *source, // Image from which to copy
                                 psImageBinning *binning, // New binning
                                 bool xFlip, bool yFlip, // Flip in x or y?
                                 bool pixels // Copy the pixels?
                                 )
{
    if (!source) return;

    if (*target) {
        psFree(*target);
    }
    if (pixels) {
        *target = psImageFlip(NULL, source, xFlip, yFlip);
        return;
    }

    // I have the fine image size, I know the binning factor, determine the ruff image size
    binning->nXfine = source->numCols;
    binning->nYfine = source->numRows;
    psImageBinningSetRuffSize(binning, PS_IMAGE_BINNING_CENTER);
    *target = psImageAlloc(binning->nXruff, binning->nYruff, source->type.type);
    psImageInit (*target, 0.0);
    return;
}

// Update the output analysis metadata, adding stuff in the input
//
// This is probably very similar to psMetadataCopy, but we want to explicitly deal with arrays (especially
// since astronomical sources live in them)
static psMetadata *updateAnalysis(psMetadata *out, psMetadata *in)
{
    psAssert(in, "Require input");
    if (!out) {
        out = psMetadataAlloc();
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(in, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        psMetadataItem *original = psMetadataLookup(in, item->name); // Checking for MULTI
        psMetadataItem *extant = psMetadataLookup(out, item->name); // Existing item?
        if ((original && original->type == PS_DATA_METADATA_MULTI) ||
            (extant && extant->type == PS_DATA_METADATA_MULTI)) {
            psMetadataAddItem(out, item, PS_LIST_TAIL, PS_META_DUPLICATE_OK);
            continue;
        }

        switch (item->type) {
          case PS_DATA_ARRAY: {
              // Concatenate arrays if they already exist
              psMetadataItem *extant = psMetadataLookup(out, item->name); // Existing array?
              if (extant && extant->type == PS_DATA_ARRAY) {
                  psArray *new = item->data.V; // New array
                  psArray *old = extant->data.V; // Old array
                  long numNew = new->n, numOld = old->n; // Number of values in each
                  extant->data.V = old = psArrayRealloc(old, old->n + numNew);
                  for (long i = 0; i < numNew; i++) {
                      old->data[numOld + i] = psMemIncrRefCounter(new->data[i]);
                  }
                  old->n = numOld + numNew;
              } else {
                  psMetadataAddItem(out, item, PS_LIST_TAIL, PS_META_REPLACE);
              }
              break;
            default:
              // If in doubt, there's not much we can do except replace
              psMetadataAddItem(out, item, PS_LIST_TAIL, PS_META_REPLACE);
              break;
          }
        }
    }
    psFree(iter);

    return out;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static engine functions --- these do all the work.  Actually, cellCopy does all the work; the others
// merely iterate on the higher-level components.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Common engine for pmCellCopy and pmCellCopyStructure
// Does the actual splitting/splicing that's required to copy an FPA to a different representation.
static bool cellCopy(pmCell *target,     // The target cell
                     const pmCell *source, // The source cell, to be copied
                     bool pixels,        // Copy the pixels?
                     int xBin, int yBin  // (Relative) binning factors in x and y
                    )
{
    assert(target);
    assert(source);
    assert(xBin > 0 && yBin > 0);

    if (!source->data_exists) {
        // Copied everything that exists
        return true;
    }

    // XXX this is a programming / config error
    if (pixels && (xBin != 1 || yBin != 1)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to copy pixels if binning is set\n");
        return false;
    }

    // the binning structure carries the information on how to rebin the images if needed
    psImageBinning *binning = psImageBinningAlloc();
    binning->nXbin = xBin;
    binning->nYbin = yBin;

    psArray *sourceReadouts = source->readouts; // The source readouts
    int numReadouts = sourceReadouts->n; // Number of readouts copied

    // Need to check/change CELL.XPARITY and CELL.YPARITY
    bool mdokS = true;                   // Status of MD lookup
    bool mdokT = true;                   // Status of MD lookup
    bool xFlip = false;                 // Switch parity in x?
    bool yFlip = false;                 // Switch parity in y?

    // enforce the following conditions:
    // CELL.XPARITY is required for source
    // CELL.XPARITY must be +/- 1
    int xParitySource = psMetadataLookupS32(&mdokS, source->concepts, "CELL.XPARITY"); // Source parity
    int xParityTarget = psMetadataLookupS32(&mdokT, target->concepts, "CELL.XPARITY"); // Target x parity
    assert(mdokS && mdokT);

    if (xParityTarget == 0) {
        psMetadataItem *item = psMetadataLookup(target->concepts, "CELL.XPARITY"); // Item with parity
        xParityTarget = item->data.S32 = xParitySource;
    }

    psAssert (abs(xParitySource) == 1, "CELL.XPARITY not set for source");
    psAssert (abs(xParityTarget) == 1, "CELL.XPARITY not set for target");
    if (abs(xParitySource) != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.XPARITY is not set for source (%d)",
                xParitySource);
        psFree(binning);
        return false;
    } else {
        // Use the source parity
        COPY_CONCEPT(target->concepts, source->concepts, "CELL.XPARITY", S32);
        xParityTarget = xParitySource;
    }
    if (xParityTarget != xParitySource) {
        xFlip = true;
    }

    int yParityTarget = psMetadataLookupS32(&mdokT, target->concepts, "CELL.YPARITY"); // Target y parity
    int yParitySource = psMetadataLookupS32(&mdokS, source->concepts, "CELL.YPARITY"); // Source parity
    assert(mdokS && mdokT);

    if (yParityTarget == 0) {
        psMetadataItem *item = psMetadataLookup(target->concepts, "CELL.YPARITY"); // Item with parity
        yParityTarget = item->data.S32 = yParitySource;
    }

    psAssert (abs(yParitySource) == 1, "CELL.YPARITY not set for source");
    psAssert (abs(yParityTarget) == 1, "CELL.YPARITY not set for target");
    if (abs(yParitySource) != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.YPARITY is not set for source (%d)",
                yParitySource);
        psFree(binning);
        return false;
    } else {
        // Use the source parity
        COPY_CONCEPT(target->concepts, source->concepts, "CELL.YPARITY", S32);
        yParityTarget = yParitySource;
    }
    if (yParityTarget != yParitySource) {
        yFlip = true;
    }
    psTrace("psModules.camera", 3, "xFlip: %d; yFlip: %d\n", xFlip, yFlip);

    // Blow away extant readouts
    for (int i = 0; i < target->readouts->n; i++) {
        psFree(target->readouts->data[i]);
        target->readouts->data[i] = NULL;
    }
    target->readouts->n = 0;

    // Perform deep copy of the images.  I would prefer *not* to do a deep copy, in the interests of speed (we
    // still need to do another deep copy into the HDU for when we write out), but this is the only way I can
    // think of to provide security against copying a cell and then unknowingly changing the source when
    // manipulating the target.
    for (int i = 0; i < numReadouts; i++) {
        pmReadout *sourceReadout = sourceReadouts->data[i]; // The source readout
        pmReadout *targetReadout = pmReadoutAlloc(target); // The target readout; this adds it to the cell

        // Copy attributes
        // XXX is this correct under binning?
        targetReadout->col0 = sourceReadout->col0;
        targetReadout->row0 = sourceReadout->row0;
        targetReadout->process = sourceReadout->process;
        targetReadout->file_exists = sourceReadout->file_exists;
        targetReadout->data_exists = sourceReadout->data_exists;

        // Copy all three image components (image, mask, variance)
        readoutCopyComponent(&targetReadout->image, sourceReadout->image, binning, xFlip, yFlip, pixels);
        readoutCopyComponent(&targetReadout->mask, sourceReadout->mask, binning, xFlip, yFlip, pixels);
        readoutCopyComponent(&targetReadout->variance, sourceReadout->variance, binning, xFlip, yFlip,
                             pixels);
        // Copy covariance matrix: doesn't care about flips, etc.
        if (sourceReadout->covariance) {
            if (targetReadout->covariance) {
                psFree(targetReadout->covariance);
            }
            targetReadout->covariance = psKernelCopy(sourceReadout->covariance);
#if 0
            if (binning) {
                // XXX This isn't strictly correct, but we don't have a function that bins covariance matrices
                // with unequal binning factors.
                psKernel *covar = psImageCovarianceBin(PS_MAX(binning->nXbin, binning->nYbin),
                                                       targetReadout->covariance);
                psFree(targetReadout->covariance);
                targetReadout->covariance = covar;
            }
#endif
        }

        // Copy bias
        while (targetReadout->bias->n > 0) {
            psListRemove(targetReadout->bias, PS_LIST_HEAD);
        }

        // Iterate over the biases
        psListIterator *biasIter = psListIteratorAlloc(sourceReadout->bias, PS_LIST_HEAD, false);
        psImage *bias = NULL;           // Bias image from iteration
        while ((bias = psListGetAndIncrement(biasIter))) {
            psImage *biasCopy = NULL;          // Copy of the bias
            readoutCopyComponent (&biasCopy, bias, binning, xFlip, yFlip, pixels);
            psListAdd(targetReadout->bias, PS_LIST_TAIL, biasCopy);
            psFree(biasCopy);           // Drop reference
        }
        psFree(biasIter);

        // Copy the analysis metadata
        targetReadout->analysis = updateAnalysis(targetReadout->analysis, sourceReadout->analysis);

        targetReadout->data_exists = true;
        psFree(targetReadout);          // Drop reference
    }

    // Copy the remaining "concepts" over.  Don't copy the TRIMSEC or BIASSEC, since these will be created by
    // pmHDUGenerate if they don't already exist in the target.  Don't copy the XPARITY or YPARITY, since
    // we've used those to do the flips.  Don't copy the X0 and Y0 because they are updated below (and are
    // dependent upon the flips we've done above).
    psMetadataIterator *conceptsIter = psMetadataIteratorAlloc(source->concepts, PS_LIST_HEAD, NULL);
    psMetadataItem *conceptItem = NULL; // Item from iteration
    while ((conceptItem = psMetadataGetAndIncrement(conceptsIter))) {
        psString name = conceptItem->name; // Name of concept
        if (!strcmp(name, "CELL.TRIMSEC")) continue;
        if (!strcmp(name, "CELL.BIASSEC")) continue;
        if (!strcmp(name, "CELL.XPARITY")) continue;
        if (!strcmp(name, "CELL.YPARITY")) continue;
        if (!strcmp(name, "CELL.X0")) continue;
        if (!strcmp(name, "CELL.Y0")) continue;

        psMetadataItem *copy = psMetadataItemCopy(conceptItem); // Copy of the concept
        psMetadataAddItem(target->concepts, copy, PS_LIST_TAIL, PS_META_REPLACE);
        psFree(copy);               // Drop reference
    }
    psFree(conceptsIter);

    // Need to update CELL.TRIMSEC and CELL.BIASSEC if we changed the binning and they exist already.
    // XXX this code seems to be very similar to pmConceptsUpdate
    if ((binning->nXbin != 1) || (binning->nYbin != 1)) {
        bool mdok = false;
        psRegion *trimsec = psMetadataLookupPtr(&mdok, target->concepts, "CELL.TRIMSEC"); // The trim section
        if (mdok && trimsec && !psRegionIsNaN(*trimsec)) {
            *trimsec = psImageBinningSetRuffRegion(binning, *trimsec);
            // force integer pixels : truncate x0, roundup x1:
            trimsec->x0 = (int)trimsec->x0;
            if (trimsec->x1 > (int)trimsec->x1) {
                trimsec->x1 = (int)trimsec->x1 + 1;
            } else {
                trimsec->x1 = (int)trimsec->x1;
            }
            trimsec->y0 = (int)trimsec->y0;
            if (trimsec->y1 > (int)trimsec->y1) {
                trimsec->y1 = (int)trimsec->y1 + 1;
            } else {
                trimsec->y1 = (int)trimsec->y1;
            }
        }
        psList *biassecs = psMetadataLookupPtr(&mdok, target->concepts, "CELL.BIASSEC"); // The bias sections
        if (mdok && biassecs && biassecs->n > 0) {
            psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, true); // Iterator
            psRegion *biassec = NULL;   // Bias section, from iteration
            while ((biassec = psListGetAndIncrement(biassecsIter))) {
                if (!psRegionIsNaN(*biassec)) {
                    *biassec = psImageBinningSetRuffRegion(binning, *biassec);
                    // force integer pixels : truncate x0, roundup x1:
                    biassec->x0 = (int)biassec->x0;
                    if (biassec->x1 > (int)biassec->x1) {
                        biassec->x1 = (int)biassec->x1 + 1;
                    } else {
                        biassec->x1 = (int)biassec->x1;
                    }
                    biassec->y0 = (int)biassec->y0;
                    if (biassec->y1 > (int)biassec->y1) {
                        biassec->y1 = (int)biassec->y1 + 1;
                    } else {
                        biassec->y1 = (int)biassec->y1;
                    }
                }
            }
            psFree(biassecsIter);
        }
    }

    // Need to update CELL.X0 and CELL.Y0 if we flipped
    // XXX this section should probably use a common function consistent with psImageBinning
    if (xFlip) {
        int xZero = psMetadataLookupS32(NULL, source->concepts, "CELL.X0"); // CELL.X0 from source
        int xParity = psMetadataLookupS32(NULL, source->concepts, "CELL.XPARITY"); // Parity in x
        int sourceBin = psMetadataLookupS32(NULL, source->concepts, "CELL.XBIN"); // CELL.XBIN from source
        int xSize = psMetadataLookupS32(NULL, source->concepts, "CELL.XSIZE"); // CELL.XSIZE of source

        psAssert (abs(xParity) == 1, "CELL.XPARITY not set for source");

        if (sourceBin == 0) {
            // Don't know the binning; assume it is unity
            sourceBin = binning->nXbin;
        }

        // XXX make sure this is consistent with the psImageBinning
        psTrace("psModules.camera", 3, "CELL.X0: Before: %d After: %d\n", xZero, xZero + (xSize - 1) * xParity * sourceBin);
        psTrace("psModules.camera", 9, "(xParity: %d xBin: %d numCols: %d)\n", xParity, sourceBin, xSize);

        if (xParity == 0 || xSize == 0) {
            psWarning("New CELL.X0 may be incorrect due to missing concepts (CELL.XPARITY, CELL.XSIZE)");
        }

        xZero += (xSize - 1) * xParity * sourceBin; // Change the parity on the X0 position
        psMetadataItem *newItem = psMetadataLookup(target->concepts, "CELL.X0"); // CELL.X0 from target
        newItem->data.S32 = xZero;
    }
    if (yFlip) {
        int yZero = psMetadataLookupS32(NULL, source->concepts, "CELL.Y0"); // CELL.Y0 from source
        int yParity = psMetadataLookupS32(NULL, source->concepts, "CELL.YPARITY"); // Parity in y
        int sourceBin = psMetadataLookupS32(NULL, source->concepts, "CELL.YBIN"); // Binning in y
        int ySize = psMetadataLookupS32(NULL, source->concepts, "CELL.YSIZE"); // CELL.YSIZE of source

        if (sourceBin == 0) {
            // Don't know the binning; assume it is unity
            sourceBin = binning->nYbin;
        }

        psTrace("psModules.camera", 3, "CELL.Y0: Before: %d After: %d\n", yZero, yZero + (ySize - 1) * yParity * sourceBin);
        psTrace("psModules.camera", 9, "(yParity: %d yBin: %d numRows: %d)\n", yParity, sourceBin, ySize);

        if (yParity == 0 || ySize == 0) {
            psWarning("New CELL.Y0 may be incorrect due to missing concepts "
                      "(CELL.Y0, CELL.YPARITY, CELL.YBIN, CELL.YSIZE)");
        }

        yZero += (ySize - 1) * yParity * sourceBin; // Change the parity on the Y0 position
        psMetadataItem *newItem = psMetadataLookup(target->concepts, "CELL.Y0"); // CELL.Y0 from target
        newItem->data.S32 = yZero;
    }

    // Update the binning concepts
    // XXX this should probably be done with a common function using psImageBinning
    psMetadataItem *binItem = psMetadataLookup(target->concepts, "CELL.XBIN");
    binItem->data.S32 *= xBin;
    binItem = psMetadataLookup(target->concepts, "CELL.YBIN");
    binItem->data.S32 *= yBin;

    // Update the analysis metadata
    target->analysis = updateAnalysis(target->analysis, source->analysis);

    // Copy any headers
    pmHDU *targetHDU = pmHDUFromCell(target); // The target HDU
    if (targetHDU) {
        pmHDU *sourceHDU = pmHDUFromCell(source); // The source HDU
        if (sourceHDU && sourceHDU->header) {
            targetHDU->header = psMetadataCopy(targetHDU->header, sourceHDU->header);
        }
    }

    // Copy the PHU over as well, if required
    pmHDU *targetPHU = findBlankPHU(target); // The target PHU
    if (targetPHU && targetPHU != targetHDU) {
        // pmHDU *sourcePHU = pmHDUGetHighest(source->parent->parent, source->parent, source); // A source HDU
        pmHDU *sourcePHU = findBlankPHU(source); // The target PHU
        if (sourcePHU && sourcePHU->header) {
            targetPHU->header = psMetadataCopy(targetPHU->header, sourcePHU->header);
        }
    }

    psFree (binning);
    target->data_exists = true;
    target->parent->data_exists = true;
    return true;
}

// Common engine for pmChipCopy and pmChipCopyStructure
// Iterate on the components
static bool chipCopy(pmChip *target,          // The target chip
                     const pmChip *source, // The source chip, to be copied
                     bool pixels,             // Copy the pixels?
                     int xBin, int yBin       // (Relative) binning factors in x and y
                    )
{
    assert(target);
    assert(source);
    assert(xBin > 0);
    assert(yBin > 0);

    if (!source->data_exists) {
        // Copied everything that exists
        return true;
    }

    psArray *targetCells = target->cells; // The target cells
    psArray *sourceCells = source->cells; // The source cells
    if (targetCells->n != sourceCells->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Number of source cells (%ld) differs from the number of target cells (%ld)\n",
                sourceCells->n, targetCells->n);
        return false;
    }

    bool status = true;                 // Status of copy
    for (int i = 0; i < targetCells->n; i++) {
        pmCell *targetCell = targetCells->data[i]; // The target cell
        const char *cellName = psMetadataLookupStr(NULL, targetCell->concepts, "CELL.NAME"); // Name of cell
        int cellNum = pmChipFindCell(source, cellName); // Number of cell with that name
        if (cellNum >= 0) {
            pmCell *sourceCell = sourceCells->data[cellNum]; // The source cell
            status &= cellCopy(targetCell, sourceCell, pixels, xBin, yBin);
	    // update the attributes
	    targetCell->file_exists = sourceCell->file_exists;
	    targetCell->data_exists = sourceCell->data_exists;
	    targetCell->process     = sourceCell->process;
        }
    }

    // Update the analysis metadata
    target->analysis = updateAnalysis(target->analysis, source->analysis);

    // Update the concepts
    psMetadataItem *chipName = psMemIncrRefCounter(psMetadataLookup(target->concepts, "CHIP.NAME"));
    pmConceptsCopyChip(target, source, false);

    // update the attributes
    target->file_exists = source->file_exists;
    target->data_exists = source->data_exists;
    target->process     = source->process;

    psMetadataAddItem(target->concepts, chipName, PS_LIST_TAIL, PS_META_REPLACE);
    psFree(chipName);
    pmConceptsCopyFPA(target->parent, source->parent, false, false);

    // Update the astrometric parameters
    // free any previous versions
    psFree (target->toFPA);   target->toFPA   = psMemIncrRefCounter (source->toFPA);
    psFree (target->fromFPA); target->fromFPA = psMemIncrRefCounter (source->fromFPA);

    // Update the parent fpa astrometry parameters, or check that they match
    pmFPA *targetFPA = target->parent;
    pmFPA *sourceFPA = source->parent;

    // XXX should we require that both of these exist?
    if (targetFPA && sourceFPA) {
	psFree(targetFPA->toSky);   targetFPA->toSky = psMemIncrRefCounter (sourceFPA->toSky);
	psFree(targetFPA->toTPA);   targetFPA->toTPA = psMemIncrRefCounter (sourceFPA->toTPA);
	psFree(targetFPA->fromTPA); targetFPA->fromTPA = psMemIncrRefCounter (sourceFPA->fromTPA);
    }

    target->data_exists = true;
    return status;
}

// create a new pmChip with the data derived from the supplied chip
pmChip *pmChipDuplicate(pmFPA *fpa, const pmChip *sourceChip)
{
    assert(sourceChip);

    bool status;
    char *chipName = psMetadataLookupStr(&status, sourceChip->concepts, "CHIP.NAME");
    pmChip *targetChip = pmChipAlloc (NULL, chipName);
    targetChip->parent = fpa;

    psArray *sourceCells = sourceChip->cells; // The source cells

    for (int i = 0; i < sourceCells->n; i++) {
        pmCell *sourceCell = sourceCells->data[i]; // The sources cell
        const char *cellName = psMetadataLookupStr(NULL, sourceCell->concepts, "CELL.NAME"); // Name of cell
        // XXX are there other concepts I need to copy first?
        pmCell *targetCell = pmCellAlloc (targetChip, cellName);
        int xParityTarget = psMetadataLookupS32(&status, sourceCell->concepts, "CELL.XPARITY"); // Target x parity
        psAssert (abs(xParityTarget) == 1, "CELL.XPARITY not set for target");
        psMetadataAddS32 (targetCell->concepts, PS_LIST_TAIL, "CELL.XPARITY", PS_META_REPLACE, "", xParityTarget);
        int yParityTarget = psMetadataLookupS32(&status, sourceCell->concepts, "CELL.YPARITY"); // Target y parity
        psAssert (abs(xParityTarget) == 1, "CELL.YPARITY not set for target");
        psMetadataAddS32 (targetCell->concepts, PS_LIST_TAIL, "CELL.YPARITY", PS_META_REPLACE, "", yParityTarget);
        if (!cellCopy(targetCell, sourceCell, true, 1, 1)) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "failed to duplicate chip\n");
            return NULL;
        }
        // update the attributes
        targetCell->file_exists = sourceCell->file_exists;
        targetCell->data_exists = sourceCell->data_exists;
        targetCell->process     = sourceCell->process;
    }

    // Update the analysis metadata
    targetChip->analysis = updateAnalysis(targetChip->analysis, sourceChip->analysis);

    // Update the concepts
    pmConceptsCopyChip(targetChip, sourceChip, false);

    // update the attributes
    targetChip->file_exists = sourceChip->file_exists;
    targetChip->data_exists = sourceChip->data_exists;
    targetChip->process     = sourceChip->process;

    return targetChip;
}

// Common engine for pmFPACopy and pmFPACopyStructure.
// Iterate on the components
static bool fpaCopy(pmFPA *target,      // The target FPA
                    const pmFPA *source, // The source FPA, to be copied
                    bool pixels,        // Copy the pixels?
                    int xBin, int yBin  // (Relative) binning factors in x and y
                   )
{
    assert(target);
    assert(source);
    assert(xBin > 0);
    assert(yBin > 0);

    psArray *targetChips = target->chips; // The target chips
    psArray *sourceChips = source->chips; // The source chips
    if (targetChips->n != sourceChips->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Number of source chips (%ld) differs from the number of target chips (%ld)\n",
                sourceChips->n, targetChips->n);
        return false;
    }

    bool status = true;                 // Status of copy
    for (int i = 0; i < targetChips->n; i++) {
        pmChip *targetChip = targetChips->data[i]; // The target chip
        const char *chipName = psMetadataLookupStr(NULL, targetChip->concepts, "CHIP.NAME"); // Name of chip
        int chipNum = pmFPAFindChip(source, chipName); // Number of chip with that name
        if (chipNum >= 0) {
            pmChip *sourceChip = sourceChips->data[chipNum]; // The source chip
            status &= chipCopy(targetChip, sourceChip, pixels, xBin, yBin);
        }
    }

    // Update the analysis metadata
    target->analysis = updateAnalysis(target->analysis, source->analysis);

    // Update the concepts
    pmConceptsCopyFPA(target, source, false, false);

    return status;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmFPACopy(pmFPA *target, const pmFPA *source)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    if (target == source) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't copy FPA onto itself.");
        return false;
    }
    return fpaCopy(target, source, true, 1, 1);
}

bool pmChipCopy(pmChip *target, const pmChip *source)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    if (target == source) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't copy chip onto itself.");
        return false;
    }
    return chipCopy(target, source, true, 1, 1);
}

bool pmCellCopy(pmCell *target, const pmCell *source)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    if (target == source) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't copy cell onto itself.");
        return false;
    }
    return cellCopy(target, source, true, 1, 1);
}


bool pmFPACopyStructure(pmFPA *target, const pmFPA *source, int xBin, int yBin)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_INT_POSITIVE(xBin, false);
    PS_ASSERT_INT_POSITIVE(yBin, false);
    if (target == source) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't copy FPA onto itself.");
        return false;
    }
    return fpaCopy(target, source, false, xBin, yBin);
}

bool pmChipCopyStructure(pmChip *target, const pmChip *source, int xBin, int yBin)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_INT_POSITIVE(xBin, false);
    PS_ASSERT_INT_POSITIVE(yBin, false);
    if (target == source) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't copy chip onto itself.");
        return false;
    }
    return chipCopy(target, source, false, xBin, yBin);
}

bool pmCellCopyStructure(pmCell *target, const pmCell *source, int xBin, int yBin)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_INT_POSITIVE(xBin, false);
    PS_ASSERT_INT_POSITIVE(yBin, false);
    if (target == source) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't copy cell onto itself.");
        return false;
    }
    return cellCopy(target, source, false, xBin, yBin);
}

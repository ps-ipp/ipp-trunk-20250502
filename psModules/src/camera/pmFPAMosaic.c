#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAFlags.h"
#include "pmConceptsAverage.h"
#include "pmHDUUtils.h"
#include "pmConfig.h"
#include "pmAstrometryWCS.h"
#include "pmFPAExtent.h"

#include "pmFPAMosaic.h"


#define CELL_LIST_BUFFER 10             // Buffer size for cell lists

// #define BLANK_VALUE 0.0                 // Value for pixels that are blank in the mosaicked image (e.g., //

#define BLANK_VALUE NAN                 // Value for pixels that are blank in the mosaicked image (e.g., //
                                        // between cells).
                                        // XXX This should ultimately be set to NAN, but psphot doesn't like
                                        // that (masking needs to be more thorough). -- still true??

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static (private) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Do two regions overlap?
#define REGIONS_OVERLAP(region1, region2) \
((region1->x0 > region2->x0 && region1->x0 < region2->x1) || \
 (region1->x1 > region2->x0 && region1->x1 < region2->x1) || \
 (region1->y0 > region2->y0 && region1->y0 < region2->y1) || \
 (region1->y1 > region2->y0 && region1->y1 < region2->y1))

// Compare a value with a maximum and minimum
#define COMPARE(value,min,max) \
if ((value) < (min)) { \
    (min) = (value); \
} \
if ((value) > (max)) { \
    (max) = (value); \
}

// Update a concept to the assumed value
#define FIX_CONCEPT(SOURCE, NAME, TYPE, VALUE) \
psMetadataItem *item = psMetadataLookup(SOURCE, NAME); \
item->data.TYPE = VALUE;

// Get the bounds for an chip's pixels on the HDU
static bool chipBounds(psRegion *bounds, // The bounds for the chip
                       const pmChip *chip // The chip to examine for contiguity
                      )
{
    assert(chip);

    psArray *cells = chip->cells;       // The array of cells
    bool mdok = true;                   // Status of MD lookup
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
        if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
            psError(PS_ERR_UNKNOWN, true, "CELL.TRIMSEC hasn't been set for cell %d.\n", i);
            return false;
        }

        if (trimsec->x0 < bounds->x0) {
            bounds->x0 = trimsec->x0;
        }
        if (trimsec->x1 > bounds->x1) {
            bounds->x1 = trimsec->x1;
        }
        if (trimsec->y0 < bounds->y0) {
            bounds->y0 = trimsec->y0;
        }
        if (trimsec->y1 > bounds->y1) {
            bounds->y1 = trimsec->y1;
        }
    }

    return true;
}

// Make sure the TRIMSEC doesn't overlap with the established image bounds
static bool chipContiguousTrimsec(psRegion *bounds, // The bounds of the image, altered if primary==true
                                  const pmChip *chip // The chip to examine for contiguity
                                 )
{
    assert(bounds);
    assert(chip);

    psArray *cells = chip->cells;       // The array of cells
    bool mdok = true;                   // Status of MD lookup
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
        if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
            psError(PS_ERR_UNKNOWN, true, "CELL.TRIMSEC hasn't been set for cell %d.\n", i);
            return false;
        }

        if (REGIONS_OVERLAP(trimsec, bounds)) {
            return false;
        }
    }

    return true;
}

// Make sure the BIASSEC doesn't overlap with the established image bounds
static bool chipContiguousBiassec(psRegion *bounds, // The bounds of the image, altered if primary==true
                                  const pmChip *chip // The chip to examine for contiguity
                                 )
{
    assert(bounds);
    assert(chip);

    // Check that the biases don't get in the way
    psArray *cells = chip->cells;       // The array of cells
    bool mdok = true;                   // Status of MD lookup
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        psList *biassecs = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.BIASSEC"); // Bias sections
        if (!mdok || !biassecs) {
            psError(PS_ERR_UNKNOWN, true, "CELL.BIASSEC hasn't been set for cell %d.\n", i);
            return false;
        }
        if (biassecs->n == 0) {
            // No point allocating an iterator if there's nothing there to iterate on
            continue;
        }
        psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, false); // Iterator
        psRegion *biassec = NULL;       // Bias section from iteration
        while ((biassec = psListGetAndIncrement(biassecsIter))) {
            if (psRegionIsNaN(*biassec)) {
                continue;
            }
            if (REGIONS_OVERLAP(biassec, bounds)) {
                psFree(biassecsIter);
                return false;
            }
        }
        psFree(biassecsIter);
    }

    // If we've gotten this far, everything is fine.
    return true;
}

// Are the pixels for the FPA contiguous on the HDU?
// Work this out by examining all the CELL.TRIMSEC and CELL.BIASSEC regions for the component cells
static bool fpaContiguous(psRegion *bounds, // The bounds of the image, returned
                          const pmFPA *fpa // The FPA to examine for contiguity
                         )
{
    assert(bounds);
    assert(fpa);

    *bounds = psRegionSet(INFINITY, 0, INFINITY, 0);

    // Get the size of the pixels on the HDU
    psArray *chips = fpa->chips;        // The array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        if (!chipBounds(bounds, chip)) {
            return false;
        }
    }

    // Make sure the bias regions don't get in the way of the HDU
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        if (!chipContiguousBiassec(bounds, chip)) {
            return false;
        }
    }

    // If we got through it all, they must all be contiguous
    return true;
}



// Check a cell for niceness in the parity and binning
static bool niceCellParityBinning(int *xBin, int *yBin, // Binning for cell, to be returned
                                  const pmCell *cell // Cell to check for niceness
                                 )
{
    assert(xBin);
    assert(yBin);
    assert(cell);

    // A "nice" cell must have only a single readout
    if (cell->readouts->n != 1) {
        return false;
    }

    // A "nice" cell must have parity == 1
    bool mdok = true;                   // Status of MD lookup
    int xParity = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XPARITY"); // Parity in x
    if (!mdok || xParity == 0) {
        psError(PS_ERR_UNKNOWN, true, "CELL.XPARITY hasn't been set for cell.\n");
        return false;
    }
    if (xParity != 1) {
        return false;
    }
    int yParity = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YPARITY"); // Parity in y
    if (!mdok || yParity == 0) {
        psError(PS_ERR_UNKNOWN, true, "CELL.YPARITY hasn't been set for cell.\n");
        return false;
    }
    if (yParity != 1) {
        return false;
    }

    // A "nice" cell must have consistent binning
    int xBinCell = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XBIN"); // Binning in x
    if (!mdok || xBin <= 0) {
        psError(PS_ERR_UNKNOWN, true, "CELL.XBIN hasn't been set for cell.\n");
        return false;
    }
    int yBinCell = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YBIN"); // Binning in y
    if (!mdok || yBin <= 0) {
        psError(PS_ERR_UNKNOWN, true, "CELL.YBIN hasn't been set for cell.\n");
        return false;
    }
    if (*xBin == 0 || *yBin == 0) {
        *xBin = xBinCell;
        *yBin = yBinCell;
    } else if (xBinCell != *xBin || yBinCell != *yBin) {
        return false;
    }

    return true;
}


// Check a cell for niceness in the boundaries
static bool niceCellBounds(const pmCell *cell, // Cell to check for niceness
                           const psRegion *imageBounds // Bounds of the image on the HDU
                          )
{
    // A "nice" cell must have the (0,0) pixel at CELL.X0,CELL.Y0
    bool mdok = true;                   // Status of MD lookup
    int x0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.X0"); // Position of (0,0) on chip
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "CELL.X0 hasn't been set for cell.\n");
        return false;
    }
    int y0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.Y0"); // Position of (0,0) on chip
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "CELL.Y0 hasn't been set for cell.\n");
        return false;
    }
    pmReadout *readout = cell->readouts->data[0]; // A representative readout
    if (!readout) {
        return false;                   // Nothing here
    }
    if (x0 != readout->col0 + readout->image->col0 - (int)imageBounds->x0 ||
            y0 != readout->row0 + readout->image->row0 - (int)imageBounds->y0) {
        psTrace("psModules.camera", 5, "CELL.X0,Y0 don't match: %d,%d vs %d,%d\n", x0, y0,
                readout->col0 + readout->image->col0 - (int)imageBounds->x0,
                readout->row0 + readout->image->row0 - (int)imageBounds->y0);
        return false;
    }

    return true;
}


// Is the chip "nice"?  If so, return the region containing the chip pixels
static psRegion *niceChip(int *xBinChip, int *yBinChip, // Binning for chip, to be returned
                          const pmChip *chip // Chip to examine for "niceness".
                         )
{
    assert(xBinChip);
    assert(yBinChip);
    assert(chip);

    // Check that we've got the HDU in the chip or the FPA
    if ((!chip->hdu || !chip->hdu->images) && (!chip->parent->hdu || !chip->parent->hdu->images)) {
        return NULL;
    }

    // Check parity and binning for component cells
    *xBinChip = 0;
    *yBinChip = 0;
    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i]; // The cell of interest
        if (!niceCellParityBinning(xBinChip, yBinChip, cell)) {
            return NULL;
        }
    }

    // Now check that the pixels are all contiguous
    psRegion *imageBounds = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Bound of image on HDU
    if (!chipBounds(imageBounds, chip) || !chipContiguousBiassec(imageBounds, chip)) {
        psTrace("psModules.camera", 5, "Image isn't contiguous.\n");
        psFree(imageBounds);
        return NULL;
    }

    psString region = psRegionToString(*imageBounds);
    psTrace("psModules.camera", 7, "Image bounds: %s\n", region);
    psFree(region);

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i]; // The cell of interest
        if (!niceCellBounds(cell, imageBounds)) {
            psFree(imageBounds);
            return NULL;
        }
    }

    // Need to check all the other chips if the HDU is in the FPA
    pmFPA *fpa = chip->parent;          // The parent FPA
    if (fpa->hdu && fpa->hdu->images) {
        psArray *chips = fpa->chips;    // Array of chips
        for (int i = 0; i < chips->n; i++) {
            pmChip *testChip = chips->data[i]; // The chip of interest
            if (testChip == chip) {
                // Already done this one
                continue;
            }
            if (!chipContiguousTrimsec(imageBounds, testChip) ||
                    !chipContiguousBiassec(imageBounds, testChip)) {
                psTrace("psModules.camera", 5, "Image isn't contiguous.\n");
                psFree(imageBounds);
                return NULL;
            }
        }
    }

    return imageBounds;
}

// Is the FPA "nice"?  If so, return the region containing the FPA pixels
static psRegion *niceFPA(int *xBinFPA, int *yBinFPA, // Binning for FPA, to be returned
                         const pmFPA *fpa  // FPA to examine for "niceness".
                        )
{
    assert(xBinFPA);
    assert(yBinFPA);
    assert(fpa);

    // Check that we've got the HDU in the chip or the FPA
    if (!fpa->hdu || !fpa->hdu->images) {
        return NULL;
    }

    // Check parity and binning for component cells
    *xBinFPA = 0;
    *yBinFPA = 0;
    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i]; // The chip of interest
        for (int j = 0; i < chip->cells->n; i++) {
            pmCell *cell = chip->cells->data[j]; // The cell of interest
            if (!niceCellParityBinning(xBinFPA, yBinFPA, cell)) {
                return NULL;
            }
        }
    }

    // Now check that the pixels are all contiguous
    psRegion *imageBounds = psRegionAlloc(0, 0, 0, 0); // Bound of image on HDU
    if (!fpaContiguous(imageBounds, fpa)) {
        psTrace("psModules.camera", 5, "Image isn't contiguous.\n");
        psFree(imageBounds);
        return NULL;
    }

    psString region = psRegionToString(*imageBounds);
    psTrace("psModules.camera", 7, "Image bounds: %s\n", region);
    psFree(region);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i]; // The chip of interest
        for (int j = 0; i < chip->cells->n; i++) {
            pmCell *cell = chip->cells->data[j]; // The cell of interest
            if (!niceCellBounds(cell, imageBounds)) {
                psFree(imageBounds);
                return NULL;
            }
        }
    }

    return imageBounds;
}

// supporting macros used by imageMosaic()
// copy pixels without binning
#define COPY_WITH_PARITY_DIFFERENCE(TYPE) \
        case PS_TYPE_##TYPE: { \
                for (int y = 0; y < image->numRows; y++) { \
                    int yTarget =  yTargetBase + yParity * y; \
                    for (int x = 0; x < image->numCols; x++) { \
                        int xTarget = xTargetBase + xParity * x; \
                        mosaic->data.TYPE[yTarget][xTarget] = image->data.TYPE[y][x]; \
                    } \
                } \
            } \
            break;

// In case the original image is binned but the mosaic is not, we need to fill in the values in
// the mosaic.  this operation should be replaced with a call to one of the functions defined
// in psImageBinning
#define FILL_IN(TYPE) \
        case PS_TYPE_##TYPE: \
            for (int y = 0; y < image->numRows; y++) { \
                float yTargetBinBase = yTargetBase + yParity * yBinSource->data.S32[i] * y / yBinTarget; \
                for (int x = 0; x < image->numCols; x++) { \
                    float xTargetBinBase = xTargetBase + xParity * xBinSource->data.S32[i] * x / xBinTarget; \
                    for (int j = 0; j < yBinSource->data.S32[i]; j++) { \
                        int yTarget = (int)(yTargetBinBase + yParity * (float)j / (float)yBinTarget); \
                        for (int k = 0; k < xBinSource->data.S32[i]; k++) { \
                            int xTarget = (int)(xTargetBinBase + xParity * (float)k / (float)xBinTarget); \
                            mosaic->data.TYPE[yTarget][xTarget] = image->data.TYPE[y][x]; \
                        } \
                    } \
                } \
            } \
            break;

// Mosaic multiple images, with flips, binning and offsets
static psImage *imageMosaic(const psArray *source, // Images to splice in
                            const psVector *xFlip, const psVector *yFlip, // Need to flip x and y?
                            const psVector *xBinSource, // Binning in x of source images
                            const psVector *yBinSource, // Binning in y of source images
                            int xBinTarget, int yBinTarget, // Binning in x and y of target images
                            const psVector *x0, const psVector *y0, // Offsets for source images on target
                            double unexposed // Value for unexposed pixels
                           )
{
    assert(source);
    assert(xFlip && xFlip->type.type == PS_TYPE_U8);
    assert(yFlip && yFlip->type.type == PS_TYPE_U8);
    assert(xBinSource && xBinSource->type.type == PS_TYPE_S32);
    assert(yBinSource && yBinSource->type.type == PS_TYPE_S32);
    assert(x0 && x0->type.type == PS_TYPE_S32);
    assert(y0 && y0->type.type == PS_TYPE_S32);
    assert(xFlip->n == source->n);
    assert(yFlip->n == source->n);
    assert(xBinSource->n == source->n);
    assert(yBinSource->n == source->n);
    assert(x0->n == source->n);
    assert(y0->n == source->n);

    if (source->n == 0) {
        return NULL;
    }

    // Get the maximum extent of the mosaic image
    int xMin = +INT_MAX;
    int xMax = -INT_MAX;
    int yMin = +INT_MAX;
    int yMax = -INT_MAX;
    psElemType type = 0;
    int numImages = 0;                  // Number of images
    psTrace("psModules.camera", 3, "Mosaicking %ld cells.\n", source->n);
    for (int i = 0; i < source->n; i++) {
        psImage *image = source->data[i]; // The image of interest
        if (!image) {
            continue;
        }
        numImages++;

        // All input types must be the same
        if (type == 0) {
            type = image->type.type;
        }
        assert(type == image->type.type);

        // Size of cell in x and y
        int xParity = xFlip->data.U8[i] ? -1 : 1;
        int yParity = yFlip->data.U8[i] ? -1 : 1;
        psTrace("psModules.camera", 5, "Extent of cell %d: %d -> %d , %d -> %d\n", i, x0->data.S32[i],
                x0->data.S32[i] + xParity * xBinSource->data.S32[i] * image->numCols, y0->data.S32[i],
                y0->data.S32[i] + yParity * yBinSource->data.S32[i] * image->numRows);

        COMPARE(x0->data.S32[i], xMin, xMax);
        COMPARE(y0->data.S32[i], yMin, yMax);
        // Subtract the parity to get the inclusive limit (not exclusive)
        COMPARE(x0->data.S32[i] + xParity * xBinSource->data.S32[i] * image->numCols - xParity, xMin, xMax);
        COMPARE(y0->data.S32[i] + yParity * yBinSource->data.S32[i] * image->numRows - yParity, yMin, yMax);
    }
    if (numImages == 0) {
        return NULL;
    }

    // Set up the image
    // Since both upper and lower values are inclusive, we need to add one to the size
    float xSize = (float)(xMax - xMin + 1) / (float)xBinTarget;
    if (xSize - (int)xSize > 0) {
        xSize += 1;
    }
    float ySize = (float)(yMax - yMin + 1) / (float)yBinTarget;
    if (ySize - (int)ySize > 0) {
        ySize += 1;
    }

    psTrace("psModules.camera", 3, "Spliced image will be %dx%d\n", (int)xSize, (int)ySize);
    psImage *mosaic = psImageAlloc((int)xSize, (int)ySize, type); // The mosaic image
    psImageInit(mosaic, unexposed);

    // Next pass through the images to do the mosaicking
    // XXX this function uses summing for the output: is this the right choice?
    for (int i = 0; i < source->n; i++) {
        psImage *image = source->data[i]; // The image of interest
        if (!image) {
            continue;
        }
        int xParity = xFlip->data.U8[i] ? -1 : 1; // Parity difference, in x
        int yParity = yFlip->data.U8[i] ? -1 : 1; // Parity difference, in y
        int xTargetBase = (x0->data.S32[i] - xMin) / xBinTarget; // The base x position in the target frame
        int yTargetBase = (y0->data.S32[i] - yMin) / yBinTarget; // The base y position in the target frame

        // in the first case, we are just copy a section pixel-by-pixel
        if ((xBinSource->data.S32[i] == xBinTarget) &&
            (yBinSource->data.S32[i] == yBinTarget) &&
            (xFlip->data.U8[i] == 0) &&
            (yFlip->data.U8[i] == 0)) {
            // Let someone else do the hard work
            psImageOverlaySection(mosaic, image, xTargetBase, yTargetBase, "=");
            continue;
        }

        // in the second case, there's a difference with the parities, but we don't have to
        // worry about binning
        if (xBinSource->data.S32[i] == xBinTarget && yBinSource->data.S32[i] == yBinTarget) {
            switch (type) {
                COPY_WITH_PARITY_DIFFERENCE(U8);
                COPY_WITH_PARITY_DIFFERENCE(U16);
                COPY_WITH_PARITY_DIFFERENCE(U32);
                COPY_WITH_PARITY_DIFFERENCE(U64);
                COPY_WITH_PARITY_DIFFERENCE(S8);
                COPY_WITH_PARITY_DIFFERENCE(S16);
                COPY_WITH_PARITY_DIFFERENCE(S32);
                COPY_WITH_PARITY_DIFFERENCE(S64);
                COPY_WITH_PARITY_DIFFERENCE(F32);
                COPY_WITH_PARITY_DIFFERENCE(F64);
              default:
                psAbort("Should never get here.\n");
            }
            continue;
        }

        // In the third case, the images are flipped and have different binnnig.
        // We have to do all of the hard work ourselves
        switch (type) {
            FILL_IN(U8);
            FILL_IN(U16);
            FILL_IN(U32);
            FILL_IN(U64);
            FILL_IN(S8);
            FILL_IN(S16);
            FILL_IN(S32);
            FILL_IN(S64);
            FILL_IN(F32);
            FILL_IN(F64);
          default:
            psAbort("Should never get here.\n");
        }
    } // Iterating over images

    return mosaic;
}

// Add a cell and its various properties to the arrays
static bool addCell(psArray *images,    // Array of images
                    psArray *masks,     // Array of masks
                    psArray *variances,   // Array of variances
                    psVector *x0,       // Array of X0
                    psVector *y0,       // Array of Y0
                    psVector *xBin,     // Array of XBIN
                    psVector *yBin,     // Array of YBIN
                    psVector *xFlip,    // Array indicating whether x axis should be flipped
                    psVector *yFlip,    // Array indicating whether y axis should be flipped
                    const pmCell *cell, // Cell to add
                    int *xBinMin,       // The minimum x binning, returned
                    int *yBinMin,       // The minimum y binning, returned
                    bool chipStuff,      // Worry about chip stuff as well?
                    int x0Target, int y0Target, // Target x0 and y0 offsets
                    int xParityTarget, int yParityTarget // Target parities
                   )
{
    if (!cell) {
        return false;
    }

    if (cell->readouts->n > 1) {
        psWarning("Skipping video cell for mosaic.\n");
        return true;
    }

    // Expand the arrays and vectors to handle new data
    long index = images->n;               // The index to use
    if (images->n == images->nalloc) {
        images  = psArrayRealloc(images,  index + CELL_LIST_BUFFER);
        masks   = psArrayRealloc(masks,   index + CELL_LIST_BUFFER);
        variances = psArrayRealloc(variances, index + CELL_LIST_BUFFER);
        x0    = psVectorRealloc(x0,    index+ CELL_LIST_BUFFER);
        y0    = psVectorRealloc(y0,    index+ CELL_LIST_BUFFER);
        xBin  = psVectorRealloc(xBin,  index+ CELL_LIST_BUFFER);
        yBin  = psVectorRealloc(yBin,  index+ CELL_LIST_BUFFER);
        xFlip = psVectorRealloc(xFlip, index+ CELL_LIST_BUFFER);
        yFlip = psVectorRealloc(yFlip, index+ CELL_LIST_BUFFER);
    }

    images->n = index + 1;
    masks->n = index + 1;
    variances->n = index + 1;
    x0->n = index + 1;
    y0->n = index + 1;
    xBin->n = index + 1;
    yBin->n = index + 1;
    xFlip->n = index + 1;
    yFlip->n = index + 1;

    bool mdok = true;                   // Status of MD lookup
    bool good = true;                   // Is everything good?

    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip

    // Offset of the cell on the chip
    int x0Cell = psMetadataLookupS32(&mdok, cell->concepts, "CELL.X0");
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "CELL.X0 for cell %s,%s is not set.\n", chipName, cellName);
        good = false;
    }
    int y0Cell = psMetadataLookupS32(&mdok, cell->concepts, "CELL.Y0");
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "CELL.Y0 for cell %s,%s is not set.\n", chipName, cellName);
        good = false;
    }
    psTrace("psModules.camera", 5, "Cell %s,%s (%ld): x0=%d y0=%d\n",
            chipName, cellName, index, x0Cell, y0Cell);

    // Offset of the chip on the FPA
    int x0Chip = 0, y0Chip = 0;
    if (chipStuff) {
        pmChip *chip = cell->parent;    // The parent chip
        if (!chip) {
            psError(PS_ERR_UNKNOWN, true, "Cell has no parent chip --- can't find CHIP.X0 and CHIP.Y0\n");
            good = false;
        }
        x0Chip = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.X0");
        if (!mdok) {
            psError(PS_ERR_UNKNOWN, true, "CHIP.X0 for chip %s is not set.\n", chipName);
            good = false;
        }
        y0Chip = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.Y0");
        if (!mdok) {
            psError(PS_ERR_UNKNOWN, true, "CHIP.Y0 for chip %s is not set.\n", chipName);
            good = false;
        }
    }
    if (!good) {
	// XXX do something to address this?
    }

    // Binning
    xBin->data.S32[index] = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XBIN");
    if (!mdok || xBin->data.S32[index] == 0) {
        psError(PS_ERR_UNKNOWN, true, "CELL.XBIN for cell %s,%s is not set.\n", chipName, cellName);
        return false;
    } else if (xBin->data.S32[index] < *xBinMin) {
        *xBinMin = xBin->data.S32[index];
    }
    yBin->data.S32[index] = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YBIN");
    if (!mdok || yBin->data.S32[index] == 0) {
        psError(PS_ERR_UNKNOWN, true, "CELL.YBIN for cell %s,%s is not set.\n", chipName, cellName);
        return false;
    } else if (yBin->data.S32[index] < *yBinMin) {
        *yBinMin = yBin->data.S32[index];
    }

    // Do we need to flip?
    int xParityCell = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XPARITY");
    if (!mdok || (xParityCell != 1 && xParityCell != -1)) {
        psError(PS_ERR_UNKNOWN, true, "CELL.XPARITY for cell %s,%s is not set.\n", chipName, cellName);
        return false;
    }
    int yParityCell = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YPARITY");
    if (!mdok || (yParityCell != 1 && yParityCell != -1)) {
        psError(PS_ERR_UNKNOWN, true, "CELL.YPARITY for cell %s,%s is not set.\n", chipName, cellName);
        return false;
    }

    // Parity of the chip on the FPA
    int xParityChip = 1, yParityChip = 1;
    if (chipStuff) {
        pmChip *chip = cell->parent;    // The parent chip
        xParityChip = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.XPARITY");
        if (!mdok || (xParityChip != 1 && xParityChip != -1)) {
            psError(PS_ERR_UNKNOWN, true, "CHIP.XPARITY for chip %s is not set.\n", chipName);
            return false;
        }
        yParityChip = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.YPARITY");
        if (!mdok || (yParityChip != 1 && yParityChip != -1)) {
            psError(PS_ERR_UNKNOWN, true, "CHIP.YPARITY for chip %s is not set.\n", chipName);
            return false;
        }
    }

    // Set the flips on the basis of the parity
    // XXX if (level == CHIP) : only apply Cell parity
    // XXX if (level == FPA) : apply Chip & Cell parity
    if (xParityCell * xParityChip == xParityTarget) {
        xFlip->data.U8[index] = 0;
    } else {
        xFlip->data.U8[index] = 1;
    }
    if (yParityCell * yParityChip == yParityTarget) {
        yFlip->data.U8[index] = 0;
    } else {
        yFlip->data.U8[index] = 1;
    }

    x0->data.S32[index] = x0Chip + x0Cell - x0Target;
    y0->data.S32[index] = y0Chip + y0Cell - y0Target;

    // Add the readout to the array of images to be mosaicked
    psArray *readouts = cell->readouts; // The array of readouts
    pmReadout *readout = readouts->data[0]; // The only readout we'll bother with

    // The images to put into the mosaic
    images->data[index]  = psMemIncrRefCounter(readout->image);
    variances->data[index] = psMemIncrRefCounter(readout->variance);
    masks->data[index]   = psMemIncrRefCounter(readout->mask);

    psTrace("psModules.camera", 9, "Added cell (%p) %ld: %d,%d; %d,%d, %d,%d.\n", cell, index,
            x0->data.S32[index], y0->data.S32[index], xBin->data.S32[index], yBin->data.S32[index],
            xFlip->data.U8[index], yFlip->data.U8[index]);

    return true;
}


// Mosaic together the cells in a chip
static bool chipMosaic(psImage **mosaicImage, // The mosaic image, to be returned
                       psImage **mosaicMask, // The mosaic mask, to be returned
                       psImage **mosaicVariance, // The mosaic variance, to be returned
                       int *xBinChip, int *yBinChip, // The binning in x and y, to be returned
                       const pmChip *chip, // Chip to mosaic
                       const pmCell *targetCell, // Cell to which to mosaic
                       psImageMaskType blank // Mask value to give blank pixels
                      )
{
    assert(mosaicImage);
    assert(mosaicMask);
    assert(mosaicVariance);
    assert(xBinChip);
    assert(yBinChip);
    assert(chip);
    assert(targetCell);

    psArray *images = psArrayAlloc(0); // Array of images that will be mosaicked
    psArray *variances = psArrayAlloc(0); // Array of variance images to be mosaicked
    psArray *masks = psArrayAlloc(0); // Array of mask images to be mosaicked
    psVector *x0 = psVectorAlloc(0, PS_TYPE_S32); // Origin x coordinates
    psVector *y0 = psVectorAlloc(0, PS_TYPE_S32); // Origin y coordinates
    psVector *xBin = psVectorAlloc(0, PS_TYPE_S32); // Binning in x
    psVector *yBin = psVectorAlloc(0, PS_TYPE_S32); // Binning in y
    psVector *xFlip = psVectorAlloc(0, PS_TYPE_U8); // Flip in x?
    psVector *yFlip = psVectorAlloc(0, PS_TYPE_U8); // Flip in y?

    // Get the target characteristics
    bool mdok = true;                   // Status of MD lookup
    int x0Target = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.X0");
    if (!mdok) {
        psWarning("CELL.X0 is not set for the target cell; assuming 0.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.X0", S32, 0);
    }
    int y0Target = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.Y0");
    if (!mdok) {
        psWarning("CELL.Y0 is not set for the target cell; assuming 0.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.Y0", S32, 0);
    }
    int xParityTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.XPARITY");
    if (!mdok || (xParityTarget != -1 && xParityTarget != 1)) {
        psWarning("CELL.XPARITY is not set for the target cell; assuming 1.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.XPARITY", S32, 1);
        xParityTarget = 1;
    }
    int yParityTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.YPARITY");
    if (!mdok || (yParityTarget != -1 && yParityTarget != 1)) {
        psWarning("CELL.YPARITY is not set for the target cell; assuming 1.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.YPARITY", S32, 1);
        yParityTarget = 1;
    }

    // Binning for the mosaicked chip is the minimum binning allowed by the cells
    *xBinChip = INT_MAX;
    *yBinChip = INT_MAX;

    // Set up the required inputs
    bool allGood = true;                // Is everything good, well-behaved?
    psArray *cells = chip->cells;       // The array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // The cell of interest
        if (!cell || !cell->data_exists) {
            continue;
        }
        allGood &= addCell(images, masks, variances, x0, y0, xBin, yBin, xFlip, yFlip,
                           cell, xBinChip, yBinChip, false, x0Target, y0Target,
                           xParityTarget, yParityTarget);
    }

    // Check to see if the target has a smaller binning in mind
    int xBinTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.XBIN");
    if (!mdok || xBinTarget == 0) {
        // CELL.XBIN is not set for the target cell --- assume it's the same as the source
        FIX_CONCEPT(targetCell->concepts, "CELL.XBIN", S32, *xBinChip);
    } else {
        *xBinChip = xBinTarget;
    }
    int yBinTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.YBIN");
    if (!mdok || yBinTarget == 0) {
        // CELL.YBIN is not set for the target cell --- assume it's the same as the source
        FIX_CONCEPT(targetCell->concepts, "CELL.YBIN", S32, *yBinChip);
    } else {
        *yBinChip = yBinTarget;
    }

    // Mosaic the images together and we're done
    if (allGood) {
        *mosaicImage = imageMosaic(images, xFlip, yFlip, xBin, yBin, *xBinChip, *yBinChip, x0, y0, BLANK_VALUE);
        *mosaicVariance = imageMosaic(variances, xFlip, yFlip, xBin, yBin, *xBinChip, *yBinChip, x0, y0, BLANK_VALUE);
        *mosaicMask = imageMosaic(masks, xFlip, yFlip, xBin, yBin, *xBinChip, *yBinChip, x0, y0, blank);
    }

    // Clean up
    psFree(images);
    psFree(variances);
    psFree(masks);
    psFree(xFlip);
    psFree(yFlip);
    psFree(xBin);
    psFree(yBin);
    psFree(x0);
    psFree(y0);

    return allGood;
}

// Mosaic together the cells in a FPA
static bool fpaMosaic(psImage **mosaicImage, // The mosaic image, to be returned
                      psImage **mosaicMask, // The mosaic mask, to be returned
                      psImage **mosaicVariance, // The mosaic variance, to be returned
                      int *xBinFPA, int *yBinFPA, // The binning in x and y, to be returned
                      const pmFPA *fpa,  // FPA to mosaic
                      const pmChip *targetChip, // Chip to which to mosaic
                      const pmCell *targetCell, // Cell to which to mosaic
                      psImageMaskType blank  // Mask value to give blank pixels
                     )
{
    assert(mosaicImage);
    assert(mosaicMask);
    assert(mosaicVariance);
    assert(xBinFPA);
    assert(yBinFPA);
    assert(fpa);
    assert(targetChip);
    assert(targetCell);

    psArray *images = psArrayAlloc(0); // Array of images that will be mosaicked
    psArray *variances = psArrayAlloc(0); // Array of variance images to be mosaicked
    psArray *masks = psArrayAlloc(0); // Array of mask images to be mosaicked
    psVector *x0 = psVectorAlloc(0, PS_TYPE_S32); // Origin x coordinates
    psVector *y0 = psVectorAlloc(0, PS_TYPE_S32); // Origin y coordinates
    psVector *xBin = psVectorAlloc(0, PS_TYPE_S32); // Binning in x
    psVector *yBin = psVectorAlloc(0, PS_TYPE_S32); // Binning in y
    psVector *xFlip = psVectorAlloc(0, PS_TYPE_U8); // Flip in x?
    psVector *yFlip = psVectorAlloc(0, PS_TYPE_U8); // Flip in y?

    // Get the target characteristics
    bool mdok = true;                   // Status of MD lookup
    int x0Target = psMetadataLookupS32(&mdok, targetChip->concepts, "CHIP.X0");
    if (!mdok) {
        psWarning("CHIP.X0 is not set for the target chip; assuming 0.\n");
        FIX_CONCEPT(targetChip->concepts, "CHIP.X0", S32, 0);
    }
    int y0Target = psMetadataLookupS32(&mdok, targetChip->concepts, "CHIP.Y0");
    if (!mdok) {
        psWarning("CHIP.Y0 is not set for the target chip; assuming 0.\n");
        FIX_CONCEPT(targetChip->concepts, "CHIP.Y0", S32, 0);
    }
    x0Target += psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.X0");
    if (!mdok) {
        psWarning("CELL.X0 is not set for the target cell; assuming 0.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.X0", S32, 0);
    }
    y0Target += psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.Y0");
    if (!mdok) {
        psWarning("CELL.Y0 is not set for the target cell; assuming 0.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.Y0", S32, 0);
    }
    int xParityChipTarget = psMetadataLookupS32(&mdok, targetChip->concepts, "CHIP.XPARITY");
    if (!mdok || (xParityChipTarget != -1 && xParityChipTarget != 1)) {
        psWarning("CHIP.XPARITY is not set for the target chip; assuming 1.\n");
        FIX_CONCEPT(targetChip->concepts, "CHIP.XPARITY", S32, 1);
        xParityChipTarget = 1;
    }
    int yParityChipTarget = psMetadataLookupS32(&mdok, targetChip->concepts, "CHIP.YPARITY");
    if (!mdok || (yParityChipTarget != -1 && yParityChipTarget != 1)) {
        psWarning("CHIP.YPARITY is not set for the target chip; assuming 1.\n");
        FIX_CONCEPT(targetChip->concepts, "CHIP.YPARITY", S32, 1);
        yParityChipTarget = 1;
    }
    int xParityCellTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.XPARITY");
    if (!mdok || (xParityCellTarget != -1 && xParityCellTarget != 1)) {
        psWarning("CELL.XPARITY is not set for the target cell; assuming 1.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.XPARITY", S32, 1);
        xParityCellTarget = 1;
    }
    int yParityCellTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.YPARITY");
    if (!mdok || (yParityCellTarget != -1 && yParityCellTarget != 1)) {
        psWarning("CELL.YPARITY is not set for the target cell; assuming 1.\n");
        FIX_CONCEPT(targetCell->concepts, "CELL.YPARITY", S32, 1);
        yParityCellTarget = 1;
    }
    int xParityTarget = xParityChipTarget * xParityCellTarget;
    int yParityTarget = yParityChipTarget * yParityCellTarget;

    // Binning for the mosaicked chip is the minimum binning allowed by the cells
    *xBinFPA = INT_MAX;
    *yBinFPA = INT_MAX;

    // Set up the required inputs
    bool allGood = true;                // Is everything good, well-behaved?
    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // The chip of interest
        if (!chip || !chip->data_exists) {
            continue;
        }
        psArray *cells = chip->cells;   // The array of cells
        for (int j = 0; j < cells->n; j++) {
            pmCell *cell = cells->data[j];  // The cell of interest
            if (!cell || !cell->data_exists) {
                continue;
            }
            allGood |= addCell(images, masks, variances, x0, y0, xBin, yBin, xFlip, yFlip,
                               cell, xBinFPA, yBinFPA, true, x0Target, y0Target,
                               xParityTarget, yParityTarget);
        }
    }

    // Check to see if the target has a smaller binning in mind
    int xBinTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.XBIN");
    if (mdok && xBinTarget != 0) {
        *xBinFPA = xBinTarget;
    }
    int yBinTarget = psMetadataLookupS32(&mdok, targetCell->concepts, "CELL.YBIN");
    if (mdok && yBinTarget != 0) {
        *yBinFPA = yBinTarget;
    }

    // Mosaic the images together and we're done
    if (allGood) {
        *mosaicImage = imageMosaic(images, xFlip, yFlip, xBin, yBin, *xBinFPA, *yBinFPA, x0, y0, BLANK_VALUE);
        *mosaicVariance = imageMosaic(variances, xFlip, yFlip, xBin, yBin, *xBinFPA, *yBinFPA, x0, y0, BLANK_VALUE);
        *mosaicMask = imageMosaic(masks, xFlip, yFlip, xBin, yBin, *xBinFPA, *yBinFPA, x0, y0, blank);
    }

    // Clean up
    psFree(images);
    psFree(variances);
    psFree(masks);
    psFree(xFlip);
    psFree(yFlip);
    psFree(xBin);
    psFree(yBin);
    psFree(x0);
    psFree(y0);

    return allGood;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Mosaic all the cells in a chip together.
//
// It is desirable to do this without using psImageOverlay (or similar) if it can be at all avoided (because
// it's really really slow in that case).  There are therefore two cases:
//
// 1. The HDU is at the Chip or FPA level.  This is the fast case, and only works if the HDU is "nice", by
// which I mean:
//
//    - the CELL.TRIMSECs are contiguous on the HDU image
//    - the CELL.PARITYs are identically +1
//    - the CELL.XBIN and CELL.YBIN are all identical
//
// Then we can just use psImageSubset to get the "mosaicked" chip.
//
//
// 2. The HDU is at the cell level, or the above requirements are not met, in which case we mosaic the cells.
// This is the slow case.  We need to:
//
//    - Throw away the bias regions
//    - Convert all cells to common parity
//    - Mosaic the cells into an HDU image using CELL.X0 and CELL.Y0
//    - Update CELL.TRIMSECs
//
// Once the demands of case 1 have been met, or case 2 has been performed, then we can create a cell to hold
// the mosaic image.

bool pmChipMosaic(pmChip *target, const pmChip *source, bool deepCopy, psImageMaskType blank)
{
    // Target exists, and has only a single cell
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(target->cells, false);
    if (target->cells->n != 1) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Target chip for mosaicking must contain a single cell.\n");
        return false;
    }
    pmCell *targetCell = target->cells->data[0]; // The target cell
    PS_ASSERT_PTR_NON_NULL(targetCell, false);
    // Source exists
    PS_ASSERT_PTR_NON_NULL(source, false);


    psImage *mosaicImage   = NULL;      // The mosaic image
    psImage *mosaicMask    = NULL;      // The mosaic mask
    psImage *mosaicVariance = NULL;      // The mosaic variances

    // Find the HDU
    psRegion *chipRegion = NULL;        // Region on the HDU that corresponds to the chip
    int xBin = 0, yBin = 0;             // Binning for the chip mosaic
    if (!deepCopy && (chipRegion = niceChip(&xBin, &yBin, source))) {
        // Case 1 --- we need only cut out the region
        psTrace("psModules.camera", 1, "Case 1 mosaicking: simple cut-out.\n");
        pmHDU *hdu = source->hdu;       // The HDU that has the pixels
        if (!hdu || !hdu->images) {
            hdu = source->parent->hdu;
        }
        // force limits to land on chip
        psRegion bounds = psRegionForImage (hdu->images->data[0], *chipRegion);
        mosaicImage = psImageSubset(hdu->images->data[0], bounds);
        if (!mosaicImage) {
            psError(PS_ERR_UNKNOWN, false, "Unable to select image pixels.\n");
            return false;
        }
        if (hdu->masks) {
            mosaicMask = psImageSubset(hdu->masks->data[0], bounds);
            if (!mosaicMask) {
                psError(PS_ERR_UNKNOWN, false, "Unable to select mask pixels.\n");
                return false;
            }
        }
        if (hdu->variances) {
            mosaicVariance = psImageSubset(hdu->variances->data[0], bounds);
            if (!mosaicVariance) {
                psError(PS_ERR_UNKNOWN, false, "Unable to select variance pixels.\n");
                return false;
            }
        }
    } else {
        // Case 2 --- we need to mosaic by cut and paste
        psTrace("psModules.camera", 1, "Case 2 mosaicking: cut and paste.\n");
        if (!chipMosaic(&mosaicImage, &mosaicMask, &mosaicVariance, &xBin, &yBin, source, targetCell, blank)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to mosaic cells.\n");
            return false;
        }
        chipRegion = psRegionAlloc(0, 0, 0, 0); // We've cut and paste, so there's no valid trimsec
        *chipRegion = psRegionForImage (mosaicImage, *chipRegion);
    }
    psTrace("psModules.camera", 1, "xBin,yBin: %d,%d\n", xBin, yBin);


    // Set the concepts for the target cell
    psList *sourceCells = psArrayToList(source->cells); // List of cells
    pmConceptsAverageCells(targetCell, sourceCells, chipRegion, NULL, false);
    {
        psMetadataItem *item = psMetadataLookup(targetCell->concepts, "CELL.X0");
        item->data.S32 = 0;
        item = psMetadataLookup(targetCell->concepts, "CELL.Y0");
        item->data.S32 = 0;
        item = psMetadataLookup(targetCell->concepts, "CELL.XBIN");
        item->data.S32 = xBin;
        item = psMetadataLookup(targetCell->concepts, "CELL.YBIN");
        item->data.S32 = yBin;
    }
    psFree(sourceCells);
    psFree(chipRegion);

    // Copy the concepts
    target->concepts = psMetadataCopy(target->concepts, source->concepts); // Chip level
    target->parent->concepts = psMetadataCopy(target->parent->concepts, source->parent->concepts); // FPA lvl

    // Average the covariances
    psList *covariances = psListAlloc(NULL); // Input covariance matrices
    for (int i = 0; i < source->cells->n; i++) {
        pmCell *cell = source->cells->data[i]; // Cell of interest
        if (!cell || !cell->data_exists) {
            continue;
        }
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest
        if (!ro || !ro->covariance) {
            continue;
        }
        psListAdd(covariances, PS_LIST_TAIL, ro->covariance);
    }
    psKernel *mosaicCovariance = NULL;  // Covariance for mosaic
    if (psListLength(covariances) > 0) {
        psArray *covarArray = psListToArray(covariances); // Array with covariances
        mosaicCovariance = psImageCovarianceAverage(covarArray);
        psFree(covarArray);
    }
    psFree(covariances);

    // Now make a new readout to go in the target cell
    pmReadout *newReadout = pmReadoutAlloc(targetCell); // New readout
    newReadout->image  = mosaicImage;
    newReadout->mask   = mosaicMask;
    newReadout->variance = mosaicVariance;
    newReadout->covariance = mosaicCovariance;
    psFree(newReadout);                 // Drop reference

    // Data now exists in the targets
    pmChipSetDataStatus(target, true);
    pmCellSetDataStatus(targetCell, true);
    newReadout->data_exists = true;

    // Update the headers
    pmHDU *sourceHDU = pmHDUFromChip(source); // The HDU for the source
    pmHDU *targetHDU = pmHDUFromChip(target); // The HDU for the target
    targetHDU->header = psMetadataCopy(targetHDU->header, sourceHDU->header);
    pmHDU *targetPHU = pmHDUGetHighest(target->parent, target, NULL);
    pmHDU *sourcePHU = pmHDUGetHighest(source->parent, source, NULL);

    // Need to update NAXIS1, NAXIS2 in the target header, so that when we write a CMF, it has the correct
    // extent.  I'm not convinced that this is the best way to do this, but it should be, at worst, harmless,
    // since NAXIS[12] will get overwritten for an image with the proper dimensions.
    psRegion *naxis = pmChipExtent(target);
    psMetadataAddS32(targetHDU->header, PS_LIST_TAIL, "NAXIS1", PS_META_REPLACE, "Size in x",
                     naxis->x1 - naxis->x0);
    psMetadataAddS32(targetHDU->header, PS_LIST_TAIL, "NAXIS2", PS_META_REPLACE, "Size in y",
                     naxis->y1 - naxis->y0);
    psFree(naxis);


    if (!targetPHU) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find HDU after mosaicking.\n");
        return false;
    }
    if (!targetPHU->header) {
        // if we don't yet have a header, copy this one.
        // XXX do we need to create an empty one if the levels do not match??
        if (true) {
            targetPHU->header = psMetadataCopy(targetPHU->header, sourcePHU->header);
        } else {
            targetPHU->header = psMetadataAlloc();
        }
    }

    if (!pmConfigConformHeader(targetPHU->header, targetPHU->format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to conform header after mosaicking.\n");
        return false;
    }

    // If the cells contain the headers, we need to apply the WCS terms from (one of?) the cells
    int xParityCellTarget = psMetadataLookupS32(NULL, targetCell->concepts, "CELL.XPARITY");
    if (xParityCellTarget != -1 && xParityCellTarget != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.XPARITY is not set for target.");
        return false;
    }
    int xParityChipTarget = psMetadataLookupS32(NULL, target->concepts, "CHIP.XPARITY");
    if (xParityChipTarget != -1 && xParityChipTarget != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CHIP.XPARITY is not set for target.");
        return false;
    }
    int xParityTarget = xParityCellTarget * xParityChipTarget; // Target parity in x

    int yParityCellTarget = psMetadataLookupS32(NULL, targetCell->concepts, "CELL.YPARITY");
    if (yParityCellTarget != -1 && yParityCellTarget != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.YPARITY is not set for target.");
        return false;
    }
    int yParityChipTarget = psMetadataLookupS32(NULL, target->concepts, "CHIP.YPARITY");
    if (yParityChipTarget != -1 && yParityChipTarget != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CHIP.YPARITY is not set for target.");
        return false;
    }
    int yParityTarget = yParityCellTarget * yParityChipTarget; // Target parity in y

    for (int i = 0; i < source->cells->n; i++) {
        pmCell *cell = source->cells->data[i];
        if (!cell || !cell->hdu || !cell->hdu->header) {
            continue;
        }

        pmAstromWCS *wcs = pmAstromWCSfromHeader(cell->hdu->header); // WCS terms for this cell
        if (!wcs) {
            psTrace("psModules.camera", 1, "Unable to read cell WCS to generate chip WCS --- ignored.");
            continue;
        }

        int xBinCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN"); // Cell binning in x
        if (xBinCell == 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.XBIN is not set.");
            return false;
        }
        int xParitySource = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY") *
            psMetadataLookupS32(NULL, source->concepts, "CHIP.XPARITY"); // Source parity in x
        if (xParitySource != -1 && xParitySource != 1) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CHIP.XPARITY or CELL.XPARITY is not set for source.");
            return false;
        }
        bool xFlip = (xParitySource == xParityTarget ? false : true); // Flip the x sense of the WCS?
        int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0"); // Cell offset in x

        // Modify the wcs terms for the cell offset, binning, and parity
        float xBinRatio = (float)xBinCell / (float)xBin;
        if (xFlip) {
            wcs->crpix1 = x0Cell - wcs->crpix1 * xBinRatio;
            wcs->cdelt1 *= -1;
        } else {
            wcs->crpix1 = x0Cell + wcs->crpix1 * xBinRatio;
        }
        wcs->cdelt1 *= xBinRatio;

        int yBinCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN"); // Cell binning in y
        if (yBinCell == 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.YBIN is not set.");
            return false;
        }
        int yParitySource = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY") *
            psMetadataLookupS32(NULL, cell->parent->concepts, "CHIP.YPARITY"); // Source parity in y
        if (yParitySource != -1 && yParitySource != 1) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CHIP.YPARITY or CELL.YPARITY is not set for source.");
            return false;
        }
        bool yFlip = (yParitySource == yParityTarget ? false : true); // Flip the y sense of the WCS?
        int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0"); // Cell offset in y

        float yBinRatio = (float)yBinCell / (float)yBin;
        if (yFlip) {
            wcs->crpix2 = y0Cell - wcs->crpix2 * yBinRatio;
            wcs->cdelt2 *= -1;
        } else {
            wcs->crpix2 = y0Cell + wcs->crpix2 * yBinRatio;
        }
        wcs->cdelt2 *= yBinRatio;

        if (!pmAstromWCStoHeader(targetHDU->header, wcs)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to generate chip WCS from cell WCS.");
            psFree(wcs);
            return false;
        }
        psFree(wcs);

        // XXX rather than quitting at this point, we could save this wcs structure and compare
        // its values to the equivalent version from one of the other cells.
        break;
    }

    return true;
}


bool pmFPAMosaic(pmFPA *target, const pmFPA *source, bool deepCopy, psImageMaskType blank)
{
    // Target exists, and has only a single chip with single cell
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(target->chips, false);
    if (target->chips->n != 1) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Target FPA for mosaicking must contain a single chip.\n");
        return false;
    }
    pmChip *targetChip = target->chips->data[0]; // The target chip
    PS_ASSERT_PTR_NON_NULL(targetChip, false);
    PS_ASSERT_PTR_NON_NULL(targetChip->cells, false);
    if (target->chips->n != 1) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Target FPA for mosaicking must contain a single cell.\n");
        return false;
    }
    pmCell *targetCell = targetChip->cells->data[0]; // The target cell
    PS_ASSERT_PTR_NON_NULL(targetCell, false);
    // Source exists
    PS_ASSERT_PTR_NON_NULL(source, false);

    psImage *mosaicImage   = NULL;      // The mosaic image
    psImage *mosaicMask    = NULL;      // The mosaic mask
    psImage *mosaicVariances = NULL;      // The mosaic variances

    // Find the HDU
    psRegion *fpaRegion = NULL;         // Region on the HDU that corresponds to the FPA
    int xBin = 0, yBin = 0;             // Binning for the FPA mosaic
    if (!deepCopy && (fpaRegion = niceFPA(&xBin, &yBin, source))) {
        // Case 1 --- we need only cut out the region
        psTrace("psModules.camera", 1, "Case 1 mosaicking: simple cut-out.\n");
        pmHDU *hdu = source->hdu;         // The HDU that has the pixels
        mosaicImage = psImageSubset(hdu->images->data[0], *fpaRegion);
        if (hdu->masks) {
            mosaicMask = psImageSubset(hdu->masks->data[0], *fpaRegion);
        }
        if (hdu->variances) {
            mosaicVariances = psImageSubset(hdu->variances->data[0], *fpaRegion);
        }
    } else {
        // Case 2 --- we need to mosaic by cut and paste
        psTrace("psModules.camera", 1, "Case 2 mosaicking: cut and paste.\n");
        if (!fpaMosaic(&mosaicImage, &mosaicMask, &mosaicVariances, &xBin, &yBin, source,
                       targetChip, targetCell, blank)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to mosaic chips.\n");
            return false;
        }
        fpaRegion = psRegionAlloc(NAN, NAN, NAN, NAN); // We've cut and paste, so there's no valid trimsec
    }

    // Set the concepts for the target cell, and add the mosaic in
    // First we need a list of cells
    psList *sourceCells = psListAlloc(NULL); // List of source cells
    psArray *chips = source->chips;        // Array of chips
    pmChip *firstSourceChip = NULL;     // The first chip in the source FPA; for headers
    pmCell *firstSourceCell = NULL;     // The first cell in the source FPA; for headers
    for (long i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        if (!chip || !chip->data_exists) {
            continue;
        }
        psArray *cells = chip->cells;
        for (long j = 0; j < cells->n; j++) {
            pmCell *cell = cells->data[j]; // Cell of interest
            if (!cell || !cell->data_exists) {
                continue;
            }
            psListAdd(sourceCells, PS_LIST_TAIL, cell);

            // These are valid chip and cell to use for the header; grab the first such
            if (!firstSourceCell && !firstSourceChip) {
                firstSourceCell = cell;
                firstSourceChip = chip;
            }
        }
    }
    pmConceptsAverageCells(targetCell, sourceCells, fpaRegion, NULL, false);
    {
        psMetadataItem *item = psMetadataLookup(targetCell->concepts, "CELL.X0");
        item->data.S32 = 0;
        item = psMetadataLookup(targetCell->concepts, "CELL.Y0");
        item->data.S32 = 0;
        item = psMetadataLookup(targetCell->concepts, "CELL.XBIN");
        item->data.S32 = xBin;
        item = psMetadataLookup(targetCell->concepts, "CELL.YBIN");
        item->data.S32 = yBin;
    }
    psFree(sourceCells);
    psFree(fpaRegion);

    // Currently, there's nothing interesting in the chip concepts that needs to be updated.

    // Copy the concepts for the target FPA
    target->concepts = psMetadataCopy(target->concepts, source->concepts);

    // Average the covariances
    psList *covariances = psListAlloc(NULL); // Input covariance matrices
    for (int i = 0; i < covariances->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        if (!chip || !chip->data_exists) {
            continue;
        }
        psArray *cells = chip->cells;   // Cells in chip
        for (long j = 0; j < cells->n; j++) {
            pmCell *cell = cells->data[i]; // Cell of interest
            if (!cell || !cell->data_exists) {
                continue;
            }
            pmReadout *ro = cell->readouts->data[0]; // Readout of interest
            if (!ro || !ro->covariance) {
                continue;
            }
            psListAdd(covariances, PS_LIST_TAIL, ro->covariance);
        }
    }
    psKernel *mosaicCovariances = NULL; // Covariance for mosaic
    if (psListLength(covariances) > 0) {
        psArray *covarArray = psListToArray(covariances); // Array with covariances
        mosaicCovariances = psImageCovarianceAverage(covarArray);
        psFree(covarArray);
    }
    psFree(covariances);

    // Now make a new readout to go in the new cell
    pmReadout *newReadout = pmReadoutAlloc(targetCell); // New readout
    newReadout->image  = mosaicImage;
    newReadout->mask   = mosaicMask;
    newReadout->variance = mosaicVariances;
    newReadout->covariance = mosaicCovariances;
    psFree(newReadout);                 // Drop reference

    // Data now exists in the targets
    pmChipSetDataStatus(targetChip, true);
    pmCellSetDataStatus(targetCell, true);
    newReadout->data_exists = true;

    // Update the headers
    pmHDU *sourceHDU = pmHDUGetHighest(source, firstSourceChip, firstSourceCell); // The HDU for the source
    if (!sourceHDU) {
        psWarning("Unable to find HDU in source FPA; unable to copy headers.\n");
        return false;
    }
    pmHDU *targetHDU = pmHDUGetHighest(target, targetChip, targetCell); // The HDU for the target
    if (!targetHDU) {
        psWarning("Unable to find HDU in target FPA; unable to copy headers.\n");
        return false;
    }

    if (sourceHDU->header) {
        targetHDU->header = psMetadataCopy(targetHDU->header, sourceHDU->header);
    } else if (!targetHDU->header) {
        targetHDU->header = psMetadataAlloc();
    }

    if (!pmConfigConformHeader(targetHDU->header, targetHDU->format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to conform header after mosaicking.\n");
        return false;
    }

    return true;
}


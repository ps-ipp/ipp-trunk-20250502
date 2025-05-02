#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmReadoutStack.h"

// generate the specified image
// XXX should it be an error for the image to exist?
psImage *pmReadoutSetAnalysisImage(pmReadout *readout, // Readout containing image
                                   const char *name, // Name of image in analysis metadata
                                   int numCols, int numRows, // Expected size of image
                                   psElemType type, // Expected type of image
                                   double init // Initial value
    )
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    psImage *image = psImageAlloc(numCols, numRows, type);

    if (!psMetadataAddImage(readout->analysis, PS_LIST_TAIL, name, 0, "Analysis image from " __FILE__, image)) {
        psAbort ("analysis image already exists");
    }
    psImageInit(image, init);

    psFree (image); // we still have a view on readout->analysis
    return image;
}

// retrieve the specified image
// XXX not sure why this should call psMemIncrRefCounter
psImage *pmReadoutGetAnalysisImage(pmReadout *readout, // Readout containing image
                                   const char *name       // Name of image in analysis metadata
    )
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    bool mdok;                          // Status of MD lookup
    psImage *image = psMetadataLookupPtr(&mdok, readout->analysis, name);
    return image;
}

psImage *pmReadoutAnalysisImage(pmReadout *readout, // Readout containing image
                                const char *name, // Name of image in analysis metadata
                                int numCols, int numRows, // Expected size of image
                                psElemType type, // Expected type of image
                                double init // Initial value
    )
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    bool mdok;                          // Status of MD lookup
    psImage *image = psMetadataLookupPtr(&mdok, readout->analysis, name);
    if (!image) {
        image = psImageAlloc(numCols, numRows, type);
        psMetadataAddImage(readout->analysis, PS_LIST_TAIL, name, 0, "Analysis image from " __FILE__, image);
        psImageInit(image, init);
        return image;
    }
    if (image->numCols != numCols || image->numRows != numRows) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Analysis image %s has incorrect size (%dx%d vs %dx%d)",
                name, image->numCols, image->numRows, numCols, numRows);
        return NULL;
    }
    if (image->type.type != type) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Analysis image %s has incorrect type (%x vs %x)",
                name, image->type.type, type);
        return NULL;
    }
    return psMemIncrRefCounter(image);
}

// XXX for the moment, use col0, row0, numCols, numRows supplied from the outside
bool pmReadoutStackDefineOutput(pmReadout *readout, int col0, int row0, int numCols, int numRows, bool mask, bool variance, psImageMaskType blank)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    // XXX is this an error?
    if (readout->image) return false;
    readout->col0 = col0;
    readout->row0 = row0;

    // allocate the images
    readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImageInit(readout->image, NAN);

    if (mask) {
        // XXX is this an error?
        if (readout->mask) return false;
        readout->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
        psImageInit(readout->mask, blank);
    }

    if (variance) {
        // XXX is this an error?
        if (readout->variance) return false;
        readout->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        psImageInit(readout->variance, NAN);
    }

    return true;
}

bool pmReadoutStackSetOutputSize(int *col0, int *row0, int *numCols, int *numRows, const psArray *inputs)
{
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);
    PS_ASSERT_PTR_NON_NULL(col0, false);
    PS_ASSERT_PTR_NON_NULL(row0, false);
    PS_ASSERT_PTR_NON_NULL(numCols, false);
    PS_ASSERT_PTR_NON_NULL(numRows, false);

    // Step through each readout in the input image list to determine how big of an output
    // image is needed to combine these input images.

    int xMin = INT_MAX;
    int yMin = INT_MAX;
    int xMax = 0;
    int yMax = 0;
    int xSize = 0;
    int ySize = 0;           // The size of the output image

    bool valid = false;                 // Do we have a single valid input?
    for (long i = 0; i < inputs->n; i++) {
        pmReadout *readout = inputs->data[i]; // Readout of interest

        if (!readout) continue;

        // use the trimsec to define the max full range of the output pixels
        pmCell *cell = readout->parent; // The parent cell
        bool mdok = true;       // Status of MD lookup
        psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
        if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
            psWarning("CELL.TRIMSEC is not set for readout %ld --- ignored.\n", i);
        } else {
            xSize = PS_MAX(xSize, trimsec->x1 - trimsec->x0);
            ySize = PS_MAX(ySize, trimsec->y1 - trimsec->y0);
            xMin  = PS_MIN(xMin,  trimsec->x0);
            xMax  = PS_MAX(xMax,  trimsec->x1);
            yMin  = PS_MIN(yMin,  trimsec->y0);
            yMax  = PS_MAX(yMax,  trimsec->y1);
        }
        valid = true;
        psTrace("psModules.camera", 7, "Readout %ld: trimsec: %f,%f - %f,%f\n", i, trimsec->x0, trimsec->y0, trimsec->x1, trimsec->y1);
    }

    *col0 = xMin;
    *row0 = yMin;
    *numCols = xSize;
    *numRows = ySize;

    if (!valid) {
        psError(PS_ERR_UNKNOWN, false, "No valid input readouts.");
    }
    return valid;
}

bool pmReadoutUpdateSize(pmReadout *readout, int minCols, int minRows,
                         int numCols, int numRows, bool mask, bool variance,
                         psImageMaskType blank)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    if (readout->image) {
        readout->col0 = PS_MIN(minCols, readout->col0);
        readout->row0 = PS_MIN(minRows, readout->row0);
    } else {
        readout->col0 = minCols;
        readout->row0 = minRows;
    }

    // (reAllocate the images
    if (!readout->image) {
        readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        psImageInit(readout->image, NAN);
    }
    if (readout->image->numCols < numCols || readout->image->numRows < numRows) {
        // Generate the new output image by extending the current one, or making a whole new one
        psImage *newImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        psImageInit(newImage, NAN);
        psImageOverlaySection(newImage, readout->image, readout->col0, readout->row0, "=");
        psFree(readout->image);
        readout->image = newImage;
    }

    if (mask) {
        if (!readout->mask) {
            readout->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(readout->mask, blank);
        }
        if (readout->mask->numCols < numCols || readout->mask->numRows < numRows) {
            psImage *newMask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(newMask, blank);
            psImageOverlaySection(newMask, readout->mask, readout->col0, readout->row0, "=");
            psFree(readout->mask);
            readout->mask = newMask;
        }
    }

    if (variance) {
        if (!readout->variance) {
            readout->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
            psImageInit(readout->variance, NAN);
        }
        if (readout->variance->numCols < numCols || readout->variance->numRows < numRows) {
            psImage *newVariance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
            psImageInit(newVariance, NAN);
            psImageOverlaySection(newVariance, readout->variance, readout->col0, readout->row0, "=");
            psFree(readout->variance);
            readout->variance = newVariance;
        }
    }

    return true;
}

bool pmReadoutStackValidate(int *minInputColsPtr, int *maxInputColsPtr, int *minInputRowsPtr,
                            int *maxInputRowsPtr, int *numColsPtr, int *numRowsPtr,
                            const psArray *inputs)
{
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);
    PS_ASSERT_PTR_NON_NULL(minInputColsPtr, false);
    PS_ASSERT_PTR_NON_NULL(maxInputColsPtr, false);
    PS_ASSERT_PTR_NON_NULL(minInputRowsPtr, false);
    PS_ASSERT_PTR_NON_NULL(maxInputRowsPtr, false);
    PS_ASSERT_PTR_NON_NULL(numColsPtr, false);
    PS_ASSERT_PTR_NON_NULL(numRowsPtr, false);

    // Step through each readout in the input image list to determine how big of an output image is needed to
    // combine these input images.
    int maxInputCols = 0;               // The largest input column value
    int maxInputRows = 0;               // The largest input row value
    int minInputCols = INT_MAX;         // The smallest input column value
    int minInputRows = INT_MAX;         // The smallest input row value
    int xSize = 0, ySize = 0;           // The size of the output image

    int xMin = INT_MAX;
    int yMin = INT_MAX;
    int xMax = 0;
    int yMax = 0;

    bool valid = false;                 // Do we have a single valid input?
    for (long i = 0; i < inputs->n; i++) {
        pmReadout *readout = inputs->data[i]; // Readout of interest

        if (!readout) {
            continue;
        }
        if (!readout->process) {
            continue;
        }
        if (!readout->image) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Input readout %ld has NULL image.\n", i);
            return false;
        }

        // use the trimsec to define the max full range of the output pixels
        pmCell *cell = readout->parent; // The parent cell
        bool mdok = true;       // Status of MD lookup
        psRegion *trimsec = cell ? psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC") : NULL; // Trim section
        if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
            psWarning("CELL.TRIMSEC is not set for readout %ld --- attempting to use image size.\n", i);
            xSize = PS_MAX(xSize, readout->image->numCols);
            ySize = PS_MAX(ySize, readout->image->numRows);
            xMin = PS_MIN(xMin, 0);
            xMax = PS_MAX(xMax, readout->image->numCols - 1);
            yMin = PS_MIN(yMin, 0);
            yMax = PS_MAX(yMax, readout->image->numRows - 1);
        } else {
            xSize = PS_MAX(xSize, trimsec->x1 - trimsec->x0);
            ySize = PS_MAX(ySize, trimsec->y1 - trimsec->y0);
            xMin  = PS_MIN(xMin,  trimsec->x0);
            xMax  = PS_MAX(xMax,  trimsec->x1);
            yMin  = PS_MIN(yMin,  trimsec->y0);
            yMax  = PS_MAX(yMax,  trimsec->y1);
        }

        valid = true;

        // Range of pixels on output images
        minInputCols = PS_MAX(xMin, PS_MIN(minInputCols, readout->col0));
        maxInputCols = PS_MIN(xMax, PS_MAX(maxInputCols, readout->col0 + readout->image->numCols));
        minInputRows = PS_MAX(yMin, PS_MIN(minInputRows, readout->row0));
        maxInputRows = PS_MIN(yMax, PS_MAX(maxInputRows, readout->row0 + readout->image->numRows));

        psTrace("psModules.camera", 7, "Readout %ld: offset %d,%d; size %dx%d\n", i, readout->col0, readout->row0, readout->image->numCols, readout->image->numRows);
    }

    if (minInputColsPtr) {
        *minInputColsPtr = minInputCols;
    }
    if (maxInputColsPtr) {
        *maxInputColsPtr = maxInputCols;
    }
    if (minInputRowsPtr) {
        *minInputRowsPtr = minInputRows;
    }
    if (maxInputRowsPtr) {
        *maxInputRowsPtr = maxInputRows;
    }
    if (numColsPtr) {
        *numColsPtr = xSize;
    }
    if (numRowsPtr) {
        *numRowsPtr = ySize;
    }

    return valid;
}

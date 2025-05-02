#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmShifts.h"
#include "pmCellSquish.h"

// Comparing values to get ranges
#define COMPARE_SMALLER(TARGET, SOURCE) if ((SOURCE) < (TARGET)) (TARGET) = (SOURCE);
#define COMPARE_BIGGER(TARGET, SOURCE) if ((SOURCE) > (TARGET)) (TARGET) = (SOURCE);


bool pmCellSquish(pmCell *cell, psImageMaskType maskVal, bool useShifts)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_ARRAY_NON_NULL(cell->readouts, false);
    psArray *readouts = cell->readouts; // Array of readouts
    long numReadouts = readouts->n; // Number of readouts

    if (numReadouts <= 1) {
        // We squished everything there was to squish
        return true;
    }

    pmShifts *shifts = NULL;                   // Orthogonal transfer shifts
    if (useShifts) {
        bool mdok;                      // Status of MD lookup
        shifts = psMetadataLookupPtr(&mdok, cell->analysis, PM_SHIFTS_TABLE_NAME);
        if (!mdok || !shifts) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Squishing with shifts requested, but no shifts found.");
            return false;
        }
        if (shifts->num != numReadouts) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Number of shifts (%ld) does not match number of readouts (%ld).",
                    shifts->num, numReadouts);
            return false;
        }
    }

    // First pass to get the bounds, make sure everything is legit.
    int xMin = 0, xMax = 0, yMin = 0, yMax = 0; // Bounds of the squish
    bool valid = false;                 // Do we have a valid readout?
    int col0 = 0, row0 = 0, numCols = 0, numRows = 0;// Window parameters for the readouts
    int xShift = 0, yShift = 0;         // Shift due to orthogonal transfer, to be applied
    if (useShifts && shifts->xyRelative) {
        // Correct for final shift, to put in correct frame for the image that is read out
        xShift = - shifts->x->data.S32[shifts->num - 1];
        yShift = - shifts->y->data.S32[shifts->num - 1];
    }
    for (long i = 0; i < numReadouts; i++) {
        // Add in the shift
        if (useShifts) {
            if (shifts->xyRelative) {
                // Need to accumulate shift
                xShift += shifts->x->data.S32[i];
                yShift += shifts->y->data.S32[i];
            } else {
                // Correct for final shift, to put in correct frame for the image that is read out
                xShift = shifts->x->data.S32[i] - shifts->x->data.S32[shifts->num - 1];
                yShift = shifts->y->data.S32[i] - shifts->y->data.S32[shifts->num - 1];
            }
        }

        pmReadout *readout = readouts->data[i]; // Readout of interest
        if (!readout || !readout->image) {
            continue;
        }

        if (!valid) {
            valid = true;

            col0 = readout->col0;
            row0 = readout->row0;
            numCols = readout->image->numCols;
            numRows = readout->image->numRows;

            if (useShifts) {
                xMin = col0;
                xMax = col0 + numCols;
                yMin = row0;
                yMax = row0 + numRows;
            }
        } else {
            if (readout->col0 != col0 || readout->row0 != row0 ||
                readout->image->numCols != numCols || readout->image->numRows != numRows) {
                // Everything should have the same window because we've read it in from an image cube
                psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                        "Readout window [%d:%d,%d:%d] doesn't match canonical window [%d:%d,%d:%d]",
                        readout->col0, readout->col0 + readout->image->numCols,
                        readout->row0, readout->row0 + readout->image->numRows,
                        col0, col0 + numCols, row0, row0 + numRows);
                return false;
            }

            if (useShifts) {
                // If there is shifting, the actual window may change
                int xMinTest = readout->col0 + xShift; // Minimum x value
                int xMaxTest = readout->col0 + readout->image->numCols + xShift; // Maximum x value
                int yMinTest = readout->row0 + yShift; // Minimum y value
                int yMaxTest = readout->row0 + readout->image->numRows + yShift; // Maximum y value
                COMPARE_SMALLER(xMin, xMinTest);
                COMPARE_BIGGER(xMax, xMaxTest);
                COMPARE_SMALLER(yMin, yMinTest);
                COMPARE_BIGGER(yMax, yMaxTest);
            }
        }
    }

    if (useShifts) {
        // Size of combined image, after shifts applied
        numCols = xMax - xMin + 1;
        numRows = yMax - yMin + 1;
    }
    psImage *squishImage = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Squished image
    psImageInit(squishImage, 0.0);
    psImage *squishMask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK); // Squished mask
    psImageInit(squishMask, 0);

    // Second pass to do the squish
    xShift = yShift = 0;                // Reset the accumulated shifts
    if (useShifts && shifts->xyRelative) {
        // Correct for final shift, to put in correct frame for the image that is read out
        xShift = - shifts->x->data.S32[shifts->num - 1];
        yShift = - shifts->y->data.S32[shifts->num - 1];
    }
    for (long i = 0; i < numReadouts; i++) {
        if (useShifts) {
            if (shifts->xyRelative) {
                // Need to accumulate shift
                xShift += shifts->x->data.S32[i];
                yShift += shifts->y->data.S32[i];
            } else {
                // Correct for final shift, to put in correct frame for the image that is read out
                xShift = shifts->x->data.S32[i] - shifts->x->data.S32[shifts->num - 1];
                yShift = shifts->y->data.S32[i] - shifts->y->data.S32[shifts->num - 1];
            }
        }

        pmReadout *readout = readouts->data[i]; // Readout of interest
        if (!readout || !readout->image) {
            continue;
        }

        int xOffset = xMin - readout->col0; // Offset to squished readout in x
        int yOffset = yMin - readout->row0; // Offset to squished readout in y
        if (useShifts) {
            xOffset += xShift;
            yOffset += yShift;
        }

        psImage *image = readout->image; // The image of interest
        psImage *mask = readout->mask; // The mask of interest
        for (int y = 0; y < readout->image->numRows; y++) {
            int ySquish = y + yOffset; // Position on squished readout in y
            for (int x = 0; x < readout->image->numCols; x++) {
                int xSquish = x + xOffset; // Position on squished readout in x
                squishImage->data.F32[ySquish][xSquish] += image->data.F32[y][x];
                if (mask) {
                    squishMask->data.PS_TYPE_IMAGE_MASK_DATA[ySquish][xSquish] |= mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal;
                }
            }
        }
    }

    pmCellFreeReadouts(cell);
    pmReadout *squishRO = pmReadoutAlloc(cell); // New readout to hold squished image
    squishRO->image = squishImage;
    squishRO->mask = squishMask;
    squishRO->row0 = yMin;
    squishRO->col0 = xMin;
    psFree(squishRO);               // Drop reference

    return true;
}

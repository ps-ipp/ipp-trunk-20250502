#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionKernels.h"

#include "pmSubtractionMask.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Mark a pixel as blank in the image, mask and weight
static inline void markBlank(psImage *image, // Image to mark as blank
                             psImage *mask, // Mask to mark as blank (or NULL)
                             psImage *weight, // Weight map to mark as blank (or NULL)
                             int x, int y, // Coordinates to mark blank
                             psImageMaskType blank // Blank mask value
    )
{
    image->data.F32[y][x] = NAN;
    if (mask) {
        mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= blank;
    }
    if (weight) {
        weight->data.F32[y][x] = NAN;
    }
    return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

psImage *pmSubtractionMask(psRegion *bounds, const pmReadout *ro1, const pmReadout *ro2,
                           psImageMaskType maskVal, int size, int footprint, float badFrac,
                           pmSubtractionMode mode)
{
    int numCols = 0, numRows = 0;       // Size of the images
    if (ro1) {
        PM_ASSERT_READOUT_NON_NULL(ro1, NULL);
        PM_ASSERT_READOUT_IMAGE(ro1, NULL);
        PM_ASSERT_READOUT_MASK(ro1, NULL);
        numCols = ro1->image->numCols;
        numRows = ro1->image->numRows;
            }
    if (ro2) {
        PM_ASSERT_READOUT_NON_NULL(ro2, NULL);
        PM_ASSERT_READOUT_IMAGE(ro2, NULL);
        PM_ASSERT_READOUT_MASK(ro2, NULL);
        numCols = ro2->image->numCols;
        numRows = ro2->image->numRows;
    }
    if (ro1 && ro2) {
        PS_ASSERT_IMAGES_SIZE_EQUAL(ro1->image, ro2->image, NULL);
    }
    if (!ro1 && !ro2) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "No image provided.");
        return false;
    }
    psAssert(numCols > 0 && numRows > 0, "There should be an image provided");
    PS_ASSERT_INT_NONNEGATIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(footprint, NULL);
    if (isfinite(badFrac)) {
        PS_ASSERT_FLOAT_LARGER_THAN(badFrac, 0.0, NULL);
        PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(badFrac, 1.0, NULL);
    }

    // Dereference inputs for convenience
    psF32 **imageData1 = ro1 ? ro1->image->data.F32 : NULL;
    psF32 **imageData2 = ro2 ? ro2->image->data.F32 : NULL;
    psImageMaskType **maskData1 = ro1 ? ro1->mask->data.PS_TYPE_IMAGE_MASK_DATA : NULL;
    psImageMaskType **maskData2 = ro2 ? ro2->mask->data.PS_TYPE_IMAGE_MASK_DATA : NULL;

    // First, a pass through to determine the fraction of bad pixels
    if (bounds || (isfinite(badFrac) && badFrac != 1.0)) {
        int xMin = numCols, xMax = 0, yMin = numRows, yMax = 0; // Bounds of good pixels
        int numBad = 0;                 // Number of bad pixels
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (ro1 && ((maskData1[y][x] & maskVal) || !isfinite(imageData1[y][x]))) {
                    numBad++;
                    continue;
                }
                if (ro2 && ((maskData2[y][x] & maskVal) || !isfinite(imageData2[y][x]))) {
                    numBad++;
                    continue;
                }
                xMin = PS_MIN(xMin, x);
                xMax = PS_MAX(xMax, x);
                yMin = PS_MIN(yMin, y);
                yMax = PS_MAX(yMax, y);
            }
        }
        if (bounds) {
            bounds->x0 = xMin;
            bounds->x1 = xMax;
            bounds->y0 = yMin;
            bounds->y1 = yMax;
        }
        if (isfinite(badFrac) && badFrac != 1.0 && numBad > badFrac * numCols * numRows) {
            psError(PM_ERR_SMALL_AREA, true,
                    "Fraction of bad pixels (%d/%d=%f) exceeds limit (%f)\n",
                    numBad, numCols * numRows, (float)numBad/(float)(numCols * numRows), badFrac);
            return NULL;
        }
    }

    // Worried about the masks for bad pixels and bad stamps colliding, so make our own mask
    psImage *mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK); // The global mask
    psImageInit(mask, 0);
    psImageMaskType **maskData = mask->data.PS_TYPE_IMAGE_MASK_DATA; // Dereference for convenience

    // Block out a border around the edge of the image

    // Bottom stripe
    for (int y = 0; y < PS_MIN(size + footprint, numRows); y++) {
        for (int x = 0; x < numCols; x++) {
            maskData[y][x] |= PM_SUBTRACTION_MASK_BORDER;
        }
    }
    // Either side
    for (int y = PS_MIN(size + footprint, numRows); y < numRows - size - footprint; y++) {
        for (int x = 0; x < PS_MIN(size + footprint, numCols); x++) {
            maskData[y][x] |= PM_SUBTRACTION_MASK_BORDER;
        }
        for (int x = PS_MAX(numCols - size - footprint, 0); x < numCols; x++) {
            maskData[y][x] |= PM_SUBTRACTION_MASK_BORDER;
        }
    }
    // Top stripe
    for (int y = PS_MAX(numRows - size - footprint, 0); y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            maskData[y][x] |= PM_SUBTRACTION_MASK_BORDER;
        }
    }

    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (ro1 && maskData1[y][x] & maskVal) {
                maskData[y][x] |= PM_SUBTRACTION_MASK_BAD_1;
            }
            if (ro2 && maskData2[y][x] & maskVal) {
                maskData[y][x] |= PM_SUBTRACTION_MASK_BAD_2;
            }
        }
    }

    // We want to block out with the CONVOLVE mask anything that would be bad if we convolved with a bad
    // reference pixel (within 'size').  Then we want to block out with the REJ mask everything within a
    // footprint's distance of those (within 'footprint').

    bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading for psImageConvolve

    // Pixels that will be bad (or poor) if we convolve with a bad reference pixel
    if (ro1 && !psImageConvolveMask(mask, mask, PM_SUBTRACTION_MASK_BAD_1, PM_SUBTRACTION_MASK_CONVOLVE_1,
                                    -size, size, -size, size)) {
        psError(psErrorCodeLast(), false, "Unable to convolve bad pixels from mask 1.");
        psFree(mask);
        return NULL;
    }
    if (ro2 && !psImageConvolveMask(mask, mask, PM_SUBTRACTION_MASK_BAD_2, PM_SUBTRACTION_MASK_CONVOLVE_2,
                                    -size, size, -size, size)) {
        psError(psErrorCodeLast(), false, "Unable to convolve bad pixels from mask 2.");
        psFree(mask);
        return NULL;
    }

    // Pixels that should not be chosen as stamps
    psImageMaskType maskRej = PM_SUBTRACTION_MASK_BAD_1 | PM_SUBTRACTION_MASK_BAD_2 |
        PM_SUBTRACTION_MASK_BORDER;     // Mask value for rejection
    switch (mode) {
      case PM_SUBTRACTION_MODE_1:
        maskRej |= PM_SUBTRACTION_MASK_CONVOLVE_1;
        break;
      case PM_SUBTRACTION_MODE_2:
        maskRej |= PM_SUBTRACTION_MASK_CONVOLVE_2;
        break;
      case PM_SUBTRACTION_MODE_UNSURE:
      case PM_SUBTRACTION_MODE_SINGLE_AUTO:
      case PM_SUBTRACTION_MODE_DUAL:
        maskRej |= PM_SUBTRACTION_MASK_CONVOLVE_1 | PM_SUBTRACTION_MASK_CONVOLVE_2;
        break;
      default:
        psAbort("Unsupported subtraction mode: %x", mode);
    }
    if (ro1 && ro2 && !psImageConvolveMask(mask, mask, maskRej, PM_SUBTRACTION_MASK_REJ,
                                           -footprint, footprint, -footprint, footprint)) {
        psError(psErrorCodeLast(), false, "Unable to convolve bad pixels.");
        psFree(mask);
        return NULL;
    }

    psImageConvolveSetThreads(oldThreads);

    return mask;
}


bool pmSubtractionBorder(psImage *image, psImage *weight, psImage *mask,
                         int size, psImageMaskType blank)
{
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, false);
    if (mask) {
        PS_ASSERT_IMAGE_NON_NULL(mask, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, image, false);
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
    }
    if (weight) {
        PS_ASSERT_IMAGE_NON_NULL(weight, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(weight, image, false);
        PS_ASSERT_IMAGE_TYPE(weight, PS_TYPE_F32, false);
    }

    int numCols = image->numCols, numRows = image->numRows; // Image dimensions

    for (int y = size; y < numRows - size; y++) {
        for (int x = 0; x < size; x++) {
            markBlank(image, mask, weight, x, y, blank);
        }
        for (int x = numCols - size; x < numCols; x++) {
            markBlank(image, mask, weight, x, y, blank);
        }
    }
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < numCols; x++) {
            markBlank(image, mask, weight, x, y, blank);
        }
    }
    for (int y = numRows - size; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            markBlank(image, mask, weight, x, y, blank);
        }
    }

    return true;
}


bool pmSubtractionMaskApply(psImage *image, psImage *weight, const psImage *mask, pmSubtractionMode mode)
{
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, false);
    if (weight) {
        PS_ASSERT_IMAGE_NON_NULL(weight, false);
        PS_ASSERT_IMAGE_TYPE(weight, PS_TYPE_F32, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(weight, image, false);
    }
    PS_ASSERT_IMAGE_NON_NULL(mask, false);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(mask, image, false);

    bool maskVal = PM_SUBTRACTION_MASK_BORDER; // Value to mask
    switch (mode) {
      case PM_SUBTRACTION_MODE_1:
        maskVal |= PM_SUBTRACTION_MASK_CONVOLVE_1;
        break;
      case PM_SUBTRACTION_MODE_2:
        maskVal |= PM_SUBTRACTION_MASK_CONVOLVE_2;
        break;
      case PM_SUBTRACTION_MODE_DUAL:
        maskVal |= PM_SUBTRACTION_MASK_CONVOLVE_2 | PM_SUBTRACTION_MASK_CONVOLVE_2;
        break;
      case PM_SUBTRACTION_MODE_ERR:
      case PM_SUBTRACTION_MODE_UNSURE:
      case PM_SUBTRACTION_MODE_SINGLE_AUTO:
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unsuppored subtraction mode: %x", mode);
        return false;
    }

    int numCols = image->numCols, numRows = image->numRows; // Size of image
    psImageMaskType **maskData = mask->data.PS_TYPE_IMAGE_MASK_DATA; // Dereference mask

    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (maskData[y][x] & maskVal) {
                image->data.F32[y][x] = NAN;
                if (weight) {
                    weight->data.F32[y][x] = NAN;
                }
            }
        }
    }

    return true;
}

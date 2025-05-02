#ifndef PM_SUBTRACTION_MASK_H
#define PM_SUBTRACTION_MASK_H

#include <pslib.h>

/// Generate a mask for use in the subtraction process
psImage *pmSubtractionMask(
    psRegion *bounds,                   ///< Bounds of valid pixels (or NULL), returned
    const pmReadout *ro1,               ///< Readout 1
    const pmReadout *ro2,               ///< Readout 2
    psImageMaskType maskVal,            ///< Value to mask out
    int size,                           ///< Half-size of the kernel (pmSubtractionKernels.size)
    int footprint,                      ///< Half-size of the kernel footprint
    float badFrac,                      ///< Maximum fraction of bad input pixels to accept
    pmSubtractionMode mode              ///< Subtraction mode
    );

/// Mark the non-convolved part of the image as blank
bool pmSubtractionBorder(psImage *image,///< Image
                         psImage *weight, ///< Weight map (or NULL)
                         psImage *mask, ///< Mask (or NULL)
                         int size,      ///< Kernel half-size
                         psImageMaskType blank ///< Mask value for blank regions
    );

/// Apply the subtraction mask to an image and weight.
///
/// Unfortunately, image subtraction may result in a bi-modal image in masked areas, which can upset image
/// statistics (very important for quantising images so that a product can be written out!).  This function
/// sets masked areas to NAN in the image and weight.
bool pmSubtractionMaskApply(psImage *image, ///< Image to which to apply mask
                            psImage *weight, ///< Weight map to which to apply mask (or NULL)
                            const psImage *mask, ///< Subtraction mask
                            pmSubtractionMode mode ///< Subtraction mode
    );


#endif

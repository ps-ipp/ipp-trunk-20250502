/* @file  psImageInterpolate.h
 * @brief Functions for interpolating an image
 *
 * @author Robert DeSonia, MHPCC
 * @author Ross Harman, MHPCC
 * @author Joshua Hoblitt, University of Hawaii
 * @author Paul Price, Institute for Astronomy
 *
 * @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-04 02:58:25 $
 * Copyright 2004-2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_IMAGE_INTERPOLATE_H
#define PS_IMAGE_INTERPOLATE_H

#include <psType.h>
#include <psImage.h>
#include <psVector.h>
#include <psImageConvolve.h>

/// Enumeration of options in interpolation
//
// DON'T CHANGE THE ORDER OF THE BELOW without making corresponding changes to the source code, in particular
// the kernelSizes vector which has sizes for each of the interpolation modes (in order).
typedef enum {
    PS_INTERPOLATE_NONE = 0,            ///< No interpolate defined (error state)
    PS_INTERPOLATE_FLAT,                ///< Flat interpolation (nearest pixel)
    PS_INTERPOLATE_BILINEAR,            ///< Bilinear interpolation
    PS_INTERPOLATE_BIQUADRATIC,         ///< Biquadratic interpolation with 3x3 region
    PS_INTERPOLATE_GAUSS,               ///< Gaussian inteprolation with 3x3 region
    PS_INTERPOLATE_LANCZOS2,            ///< Sinc interpolation with 4x4 pixel kernel
    PS_INTERPOLATE_LANCZOS3,            ///< Sinc interpolation with 6x6 pixel kernel
    PS_INTERPOLATE_LANCZOS4,            ///< Sinc interpolation with 8x8 pixel kernel
    PS_INTERPOLATE_BILINEAR_SIMPLE,     ///< Simple manual bilinear interpolation
} psImageInterpolateMode;

/// Status of interpolation
typedef enum {
    PS_INTERPOLATE_STATUS_ERROR = 0,    ///< There was an error
    PS_INTERPOLATE_STATUS_OFF,          ///< The pixel fell completely off the image or in the border
    PS_INTERPOLATE_STATUS_BAD,          ///< The pixel is bad
    PS_INTERPOLATE_STATUS_POOR,         ///< The pixel is poor
    PS_INTERPOLATE_STATUS_GOOD,         ///< The pixel is good
} psImageInterpolateStatus;

/// Options for general interpolation.
///
/// We stuff in here all the constant values when doing interpolation, so that not all of it has to be pushed
/// onto the stack in the middle of a tight loop.  For this reason, even the image, mask and variance map are
/// included.
typedef struct {
    psImageInterpolateMode mode;        ///< Interpolation mode
    const psImage *image;               ///< Input image for interpolation
    const psImage *variance;            ///< Variance image for interpolation
    const psImage *mask;                ///< Mask image for interpolation
    psImageMaskType maskVal;            ///< Value to mask
    double badImage;                    ///< Image value if x,y location is not good
    double badVariance;                 ///< Variance value if x,y location is not good
    psImageMaskType badMask;            ///< Mask value to give bad pixels
    psImageMaskType poorMask;           ///< Mask value to give poor pixels
    float poorFrac;                     ///< Fraction of flux in bad pixels before output is marked bad
    bool shifting;                      ///< Shifting images? Don't interpolate if the shift is exact.
    int numKernels;                     ///< Number of pre-calculated kernels
    const psImage *kernel, *kernel2;    ///< 1D interpolation kernel and kernel^2 (row) for each spacing
    const psVector *sumKernel2;         ///< Sum of kernel^2 for each spacing
} psImageInterpolation;


/// Allocator
psImageInterpolation *psImageInterpolationAlloc(
    psImageInterpolateMode mode,        // Interpolation mode
    const psImage *image,               // Input image
    const psImage *variance,            // Variance image
    const psImage *mask,                // Mask image
    psImageMaskType maskVal,                 // Value to mask
    double badImage,                    // Value for image if bad
    double badVariance,                 // Value for variance if bad
    psImageMaskType badMask,                 // Mask value for bad pixels
    psImageMaskType poorMask,                // Mask value for poor pixels
    float poorFrac,                     // Fraction of flux for question
    int numKernels                      // Number of interpolation kernels to pre-calculate
    ) PS_ATTR_MALLOC;


/// Interpolate image pixel value given floating point coordinates.
psImageInterpolateStatus psImageInterpolate(
    double *imageValue,                 ///< Return value for image
    double *varianceValue,              ///< Return value for variance
    psImageMaskType *maskValue,              ///< Return value for mask
    float x, float y,                   ///< Location to which to interpolate
    const psImageInterpolation *options ///< Options
    );

// Return the appropriate interpolation mode given a char string name for that mode
psImageInterpolateMode psImageInterpolateModeFromString(const char *name // Mode name
    );

/// Return the variance factor for the appropriate position
///
/// psImageInterpolate sets the variance appropriate for extended regions (on the scale of the interpolation
/// kernel), but this is not appropriate for pixel-to-pixel statistics.  This function returns the conversion
/// factor.
float psImageInterpolateVarianceFactor(float x, float y, ///< Position of interest
                                       psImageInterpolateMode mode ///< Interpolation mode
    );

/// Generate the appropriate interpolation kernel
psKernel *psImageInterpolationKernel(float x, float y, ///< Position of interest
                                     psImageInterpolateMode mode ///< Interpolation mode
    );

#endif

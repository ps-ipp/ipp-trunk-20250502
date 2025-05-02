/* @file  psImageGeomManip.h
 *
 * @brief Contains basic image geometry manipulation operations, as
 *        specified in the PSLIB SDRS sections "Image Geometry Manipulations".
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.23 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:37 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#ifndef PS_IMAGE_GEOM_MANIP_H
#define PS_IMAGE_GEOM_MANIP_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psImage.h"
#include "psImageInterpolate.h"
#include "psCoord.h"
#include "psStats.h"
#include "psPixels.h"

/** Rebin image to new scale.
 *
 *  A new image is constructed in which the dimensions are reduced by a factor of
 *  1/scale.  The scale, always a positive number, is equal in each dimension and
 *  specified the number of pixels used to define a new pixel in the output image.
 *  The output image is generated from all input image pixels. This function is
 *  defined for psU8, psS8, psS16, psF32, psF64.
 *
 *  @return psImage    new image formed by rebinning input image.
 */
psImage* psImageRebin(
    psImage* out,                      ///< an psImage to recycle.  If NULL, a new image is created
    const psImage* in,                 ///< input image
    const psImage* mask,               ///< mask for input image.  If NULL, no masking is done.
    psImageMaskType maskVal,		///< the bits to check in mask.
    int scale,                         ///< the scale to rebin for each dimension
    const psStats* stats
    ///< the statistic to perform when rebinning.  Only one method should be set.
);

/** Resample image to new scale.
 *
 *  A new image is constructed in which the dimensions are increased by a
 *  factor of scale. The scale, always a positive number, is equal in each
 *  dimension. The output image is generated from all input image pixels.
 *  Each pixel in the output image is derived by interpolating between
 *  neighboring pixels using the specified interpolation method (mode).
 *
 *  @return psImage*    resampled image result
 */
psImage* psImageResample(
    psImage* out,                      ///< an psImage to recycle.  If NULL, a new image is created
    const psImage* in,                 ///< input image
    int scale,                       ///< resample scaling factor
    psImageInterpolateMode mode        ///< the interpolation mode used in resampling
);

/** Rotate the input image by given angle, specified in degrees.
 *
 *  The output image must contain all of the pixels from the input image in
 *  their new frame. Pixels in the output image which do not map to input
 *  pixels should be set to exposed. The center of rotation is always the
 *  center pixel of the image. The rotation is specified in the sense that a
 *  positive angle is an anti-clockwise rotation. This function must be
 *  defined for the following types: psU8, psU16, psS8, psS16, psF32, psF64.
 *
 *  @return psImage*     the rotated image result.
 */
psImage* psImageRotate(
    psImage* out,                      ///< an psImage to recycle.  If NULL, a new image is created
    const psImage* input,              ///< input image
    float angle,                       ///< the rotation angle in radians.
    double exposed,                    ///< the output image pixel values for non-imagery areas
    psImageInterpolateMode mode        ///< the interpolation mode used
);

/** Shift image by an arbitrary number of pixels (dx,dy) in either direction.
 *
 *  If the shift values are fractional, the output pixel values should
 *  interpolate between the input pixel values. The output image has the same
 *  dimensions as the input image. Pixels which fall off the edge of the
 *  output image are lost. Newly exposed pixels are set to the value given by
 *  exposed. This function must be defined for the following types: psU8,
 *  psU16, psS8, psS16, psF32, psF64.
 *
 *  @return psImage*     the shifted image result.
 */
psImage* psImageShift(
    psImage* out,                      ///< an psImage to recycle.  If NULL, a new image is created
    const psImage* input,              ///< input image
    float dx,                          ///< the shift in x direction.
    float dy,                          ///< the shift in y direction.
    double exposed,                    ///< the output image pixel values for non-imagery areas
    psImageInterpolateMode mode        ///< the interpolation mode to use
);

/// Apply a translation to an image
///
/// This function is very much like psImageShift, except that it applies the same shifts to the mask
bool psImageShiftMask(
    psImage **out,                      ///< Output shifted image
    psImage **outMask,                  ///< Output shifted mask, or NULL
    const psImage* in,                  ///< Input image
    const psImage *inMask,              ///< Input mask, or NULL
    psImageMaskType maskVal,		///< Value to mask
    float dx, float dy,                 ///< Shift to apply
    double exposed,                     ///< Value to give exposed pixels
    psImageMaskType blank,		///< Mask value for exposed pixels
    psImageInterpolateMode mode         ///< Interpolation mode
    );

/** Roll image by an integer number of pixels in either direction.
 *
 *  The output image is the same dimensions as the input image.  Edge pixels
 *  wrap to the other side (no values are lost).  This function is
 *  defined for psU8, psS8, psS16, psF32, psF64.
 *
 *  @return psImage* the rolled version of the input image.
 */
psImage* psImageRoll(
    psImage* out,                      ///< an psImage to recycle.  If NULL, a new image is created
    const psImage* input,              ///< input image
    int dx,                            ///< number of pixels to roll in the x-dimension
    int dy                             ///< number of pixels to roll in the y-dimension
);

/** Transform the input image according the supplied transformation.
 *
 *  Transform the input image according the supplied transformation. The size
 *  of the transformed image is defined by the supplied output image, if
 *  non-NULL, or the region otherwise (size region.x1 - region.x0 by region.y1
 *  region.y0, with out->x0 = region.x0 and out->y0 = region.y0). If the
 *  inputMask is non-NULL, those pixels in the inputMask matching inputMaskVal
 *  are to be ignored in the transformation. The inputMask must be of type
 *  psU8, and of the same size as the input, otherwise the function shall
 *  generate an error and return NULL. The transformation outToIn speciﬁes the
 *  coordinates in the input image of a pixel in the output image — note that
 *  this is the reverse of what might be naively expected, but it is what is
 *  required in order to use psImageInterpolate. If the pixels array is
 *  non-NULL, it shall consist of psPixelCoords, and only those pixels in the
 *  output image shall be transformed; otherwise, the entire image is
 *  generated. The interpolation is performed using the speciﬁed interpolation
 *  mode. Where a pixel in the output image does not correspond to a pixel in
 *  the input image (or all appropriate pixels in the input image are
 *  masked), the value shall be set to exposed, and the pixel added to the
 *  appropriate list of pixels (psPixels) in the array of blankPixels for
 *  return to the user. This function must be capable of handling the following
 *  types for the input (with corresponding types for the output): psF32, psF64.

 *
 *  @return psImage*    The transformed image.
 */
psImage* psImageTransform(
    psImage *output,                   ///< psImage to recycle, or NULL
    psPixels** blankPixels,            ///< list of pixels in output image not set, or NULL if no list is desired.
    const psImage *input,              ///< psImage to apply transform to
    const psImage *inputMask,          ///< if not NULL, mask of input psImage
    psImageMaskType inputMaskVal,	///< masking value for inputMask
    const psPlaneTransform *outToIn,   ///< the transform to apply
    psRegion region,                   ///< the size of the transformed image
    const psPixels* pixels,            /**< if not NULL, consists of psPixelCoords and specifies
                                                        * which pixels in output image shall be transformed;
                                                        * otherwise, entire image generated*/
    psImageInterpolateMode mode,       ///< the interpolation scheme to be used
    double exposedValue                ///< Exposed value to which non-corresponding pixels are set
);

// Flip the input image
psImage *psImageFlip(psImage *output,   // Output image, or NULL
                     const psImage *input, // Input image
                     bool xFlip,        // Flip x axis?
                     bool yFlip         // Flip y axis?
                    );

/// @}
#endif // #ifndef PS_IMAGE_GEOM_MANIP_H

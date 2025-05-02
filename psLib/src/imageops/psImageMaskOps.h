/* @file  psImageMaskOps.h
 *
 * @brief Contains basic image pixel manipulation operations, as
 *        specified in the PSLIB SDRS sections "Mask Operations"
 *
 * @author David Robbins, MHPCC
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:37 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_IMAGE_MASK_OPS_H
#define PS_IMAGE_MASK_OPS_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psImage.h"
#include "psCoord.h"
#include "psStats.h"
#include "psPixels.h"

/** Perform the mask opertion on all image pixels
 *
 *  The pixels are set by combining the existing pixel value and the given maskValue
 *  with a logical operation.  The allowed operations are =, AND, OR, and XOR.
 */
void psImageMaskPixels(psImage *image,
                       const char *op,
                       psImageMaskType maskValue);

/** Sets the bits inside the region, ignoring pixels outside.
 *
 *  The pixels are set by combining the existing pixel value and the given maskValue
 *  with a logical operation.  The allowed operations are =, AND, OR, and XOR.
 */
void psImageMaskRegion(
    psImage *image,                    ///< the image to set
    psRegion region,                   ///< the specified region
    const char *op,                    ///< the logical operation
    psImageMaskType maskValue		///< the specified bits
);

/** Sets the bits outside the region, ignoring pixels inside.
 *
 *  The pixels are set by combining the existing pixel value and the given maskValue
 *  with a logical operation.  The allowed operations are =, AND, OR, and XOR.
 */
void psImageKeepRegion(
    psImage *image,                    ///< the image to set
    psRegion region,                   ///< the specified region
    const char *op,                    ///< the logical operation
    psImageMaskType maskValue		///< the specified bits
);

/** Sets the bits inside the circle, ignoring the pixels outside.
 *
 *  The pixel values are set by combining the existing pixel value and the given maskValue
 *  with a logical operation.  The allowed operations are =, AND, OR, and XOR.
 */
void psImageMaskCircle(
    psImage *image,                    ///< the image to set
    double x,                          ///< the x coordinate of the circle's center
    double y,                          ///< the y coordinate of the circle's center
    double radius,                     ///< the radius of the specified circle
    const char *op,                    ///< the logical operation
    psImageMaskType maskValue		///< the specified bits
);

/** Sets the bits outside the circle, ignoring the pixels inside.
 *
 *  The pixel values are set by combining the existing pixel value and the given maskValue
 *  with a logical operation.  The allowed operations are =, AND, OR, and XOR.
 */
void psImageKeepCircle(
    psImage *image,                    ///< the image to set
    double x,                          ///< the x coordinate of the circle's center
    double y,                          ///< the y coordinate of the circle's center
    double radius,                     ///< the radius of the specified circle
    const char *op,                    ///< the logical operation
    psImageMaskType maskValue		///< the specified bits
);

/** Grows the specified values on the imput mask image, in, returning the result.
 *
 *  If out is NULL, then a new image of the same type and dimension as in shall
 *  be allocated and returned; otherwise out shall be modified.  If out is non-
 *  NULL and does not have the same size and type as in, the function shall
 *  generate an error and return NULL.  Pixels in the in image within growSize
 *  pixels (either horizontal or vertical) of a pixel which matches the maskVal
 *  shall have the corresponding pixel in the out image set to the growVal.
 *
 *  @return psImage*:
 */
psImage *psImageGrowMask(
    psImage *out,                      ///< the image to set and return
    const psImage *in,                 ///< the input to image
    psImageMaskType maskVal,		///< the specified mask value
    unsigned int growSize,             ///< the range of values from maskVal
    psImageMaskType growVal		///< the output value to set
);

/// @}
#endif // #ifndef PS_MASK_OPS_H

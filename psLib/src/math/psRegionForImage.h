/* @file  psRegionForImage.h
 * @brief regions definitions based on images
 *
 * $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * $Date: 2008-08-21 21:56:55 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_REGION_FOR_IMAGE_H
#define PS_REGION_FOR_IMAGE_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/** Sets an actual region based on image parameters.
 *
 *  An image region defined with negative upper limits may be rationalized for the bounds of a
 *  specific image with psRegionForImage.  The output of this function is a region with negative
 *  upper limits replaced by their corrected value appropriate to the given image.  In addition,
 *  the lower and upper limits are foced to lie within the bounds of the image.  If the lower-
 *  limit coordinates are lewss than the lower bound of the image, they are limited to the lower
 *  bound of the image.  Conversely, if the upper-limit coordinates are greater than the upper
 *  bound of the image, they are truncated to define only valid pixels.  If the lower-limit
 *  coordinates are greater than the upper bounds of the image, or the upper-limit coordinates
 *  are less than the lower bounds of the image, the coordinates should saturate on those limits.
 *
 *  @return psRegion:       A region with negative upper limits replaced by the corrected
 */
psRegion psRegionForImage(
    const psImage *image,               ///< the image for which the region is to be set
    psRegion in                         ///< the image region limits
);

/// @}
#endif

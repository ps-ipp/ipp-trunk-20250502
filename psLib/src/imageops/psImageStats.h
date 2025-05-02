/* @file psImageStats.h
 *
 * @brief Routines for calculating statistics on images.
 *
 * This file will hold the prototypes for procedures which calculate
 * statistic on images, histograms on images, and fit/evaluate Chebyshev
 * polynomials to images.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.32 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:37 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_IMAGE_STATS_H
#define PS_IMAGE_STATS_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psType.h"
#include "psVector.h"
#include "psImage.h"
#include "psStats.h"
#include "psHistogram.h"
#include "psPolynomial.h"
#include "psRegion.h"

/** This routine must determine the various statistics for the image.
 *
 *  Determine statistics for image (or subimage). The statistics to be
 *  determined are specified by stats. The mask allows pixels to be excluded
 *  if their corresponding mask pixel value matches the value of maskVal.
 *  This function must be defined for the following types: psS8, psU16, psF32,
 *  psF64.
 *
 *  @return bool   Successful operation?
 */
bool psImageStats(
    psStats* stats,                    ///< defines statistics to be calculated
    const psImage* in,                 ///< image (or subimage) to calculate stats
    const psImage* mask,               ///< mask data for image (NULL ok)
    psImageMaskType maskVal		///< mask value for mask
);

/** Construct a histogram from an image (or subimage).
 *
 *  The histogram to generate is specified by psHistogram hist (see section
 *  4.3.2 in SDRS). This function must be defined for the following types:
 *  psS8, psU16, psF32, psF64.
 *
 *  @return bool   Successful operation?
 */
bool psImageHistogram(
    psHistogram* out,                  ///< input histogram description & target
    const psImage* in,                 ///< Image data to be histogramed.
    const psImage* mask,               ///< mask data for image (NULL ok)
    psImageMaskType maskVal		///< mask Mask for mask
);

/** Fit a 2-D polynomial surface to an image.
 *
 *  The input structure coeffs contains the desired order and terms of
 *  interest. This function must be defined for the following types: psS8,
 *  psU16, psF32, psF64.
 *
 *  @return bool   Successful operation?
 *
 */
bool psImageFitPolynomial(
    psPolynomial2D* coeffs,            ///< coefficient structure carries in desired terms & target
    const psImage* input               ///< input image
);

/** Evaluate a 2-D polynomial surface for the image pixels.
 *
 *  Given the input polynomial coefficients, set the image pixel values on the
 *  basis of the polynomial function. This function must be defined for the
 *  following types: psS8, psU16, psF32, psF64.
 *
 *  @return psImage*    the resulting image
 */
psImage* psImageEvalPolynomial(
    psImage* input,                    ///< input image
    const psPolynomial2D* coeffs       ///< coefficient structure carries in desired terms
);

/** Returns the number of pixels in the image region which satisfy any of the mask bits.
 *
 *  An error (eg, invalid image, invalid region) results in a return value of -1.
 *  The vector must be U8.
 *
 *  @return long:       the number of pixels counted
 */
long psImageCountPixelMask(
    psImage *mask,                     ///< input image to count
    psRegion region,                   ///< input region of image
    psImageMaskType value		///< the mask value to satisfy
);

/// @}
#endif // #ifndef PS_IMAGE_STATS_H

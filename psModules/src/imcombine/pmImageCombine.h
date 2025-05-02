/* @file  pmImageCombine.h
 *
 * This file will perform image combination of several images of the
 * same field, produce a list of questionable pixels, then tag some
 * of those pixels as cosmic rays.
 *
 * @author Paul Price, IfA (original prototype)
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PM_IMAGE_COMBINE_H
#define PM_IMAGE_COMBINE_H

/// @addtogroup imcombine Image Combinations
/// @{

psImage *pmCombineImages(
    psImage *combine,                   ///< Combined image (output)
    psArray **questionablePixels,       ///< Array of rejection masks
    const psArray *images,              ///< Array of input images
    const psArray *errors,              ///< Array of input error images
    const psArray *masks,               ///< Array of input masks
    psU32 maskVal,                      ///< Mask value
    const psPixels *pixels,             ///< Pixels to combine
    psS32 numIter,                      ///< Number of rejection iterations
    psF32 sigmaClip                     ///< Number of standard deviations at which to reject
);

psArray *pmRejectPixels(
    const psArray *images,              ///< Array of input images
    const psArray *masks,               ///< Array of input image masks
    const psArray *errors,              ///< The pixels which were rejected in the combination
    const psArray *inToOut,             ///< Transformation from input to output system
    const psArray *outToIn,             ///< Transformation from output to input system
    psF32 rejThreshold,                 ///< Rejection threshold
    psF32 gradLimit                     ///< Gradient limit
);

/// @}
#endif

/** @file  psImageUnbin.h
 *
 *  @brief Functions to determine the image background
 *
 *  @author EAM, IfA
 *
 *  $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  $Date: 2009-01-27 06:39:37 $
 *  Copyright 2004-2005 IfA, University of Hawaii
 */

#ifndef PS_IMAGE_BACKGROUND_H
#define PS_IMAGE_BACKGROUND_H

/// @addtogroup ImageOps Image Operations
/// @{

#include <psStats.h>
#include <psImage.h>
#include <psVector.h>
#include <psType.h>
#include <psRandom.h>

void psImageBackgroundInit();

// Get the background for an image
bool psImageBackground(psStats *stats, // desired measurement and options
                       psVector **sample, // Vector of data used for analysis (buffer), or NULL
                       const psImage *image, // Image for which to get the background
                       const psImage *mask, // Mask image
                       psImageMaskType maskValue, // Mask pixels which this mask value
                       psRandom *rng // Random number generator (for pixel selection)
                      );

/// @}
#endif // #ifndef PS_IMAGE_BACKGROUND_H

/* the user may supply a psVector ** or NULL to sample.  if a vector is supplied,
   the user must free the resulting vector */

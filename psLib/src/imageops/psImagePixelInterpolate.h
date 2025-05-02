/** @file  psImagePixelInterpolate.c
 *
 *  @brief Functions for interpolating bad pixels in images
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_IMAGE_PIXEL_INTERPOLATE_H
#define PS_IMAGE_PIXEL_INTERPOLATE_H

/// @addtogroup ImageOps Image Operations
/// @{

// XXX make these all bit values?
typedef enum {
    PS_IMAGE_INTERPOLATE_GOOD   = 0x10,
    PS_IMAGE_INTERPOLATE_GOOD0  = 0x11,
    PS_IMAGE_INTERPOLATE_GOOD1  = 0x12,
    PS_IMAGE_INTERPOLATE_GOOD2  = 0x13,
    PS_IMAGE_INTERPOLATE_BAD    = 0x01,
    PS_IMAGE_INTERPOLATE_CENTER = 0x02,
    PS_IMAGE_INTERPOLATE_CORNER = 0x04,
    PS_IMAGE_INTERPOLATE_UR     = 0x04,
    PS_IMAGE_INTERPOLATE_UL     = 0x05,
    PS_IMAGE_INTERPOLATE_LR     = 0x06,
    PS_IMAGE_INTERPOLATE_LL     = 0x07,
} psImagePixelInterpolateType;

psImage *psImagePixelInterpolateState (int *nBad, int *nPoor, psImage *mask, psImageMaskType maskVal);
bool psImagePixelInterpolatePoor (psImage *image, psImage *state, psImage *mask, psImageMaskType maskVal);
bool psImagePixelInterpolateCenter (psImage *value, psImage *xCoord, psImage *yCoord, psImage *state, psImage *mask, psImageMaskType maskVal);

/// @}
#endif // #ifndef PS_IMAGE_MAP_H

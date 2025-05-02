/* @file  psImagePixelManip.h
 *
 * @brief Basic image pixel manipulation operations
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.17 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-14 00:39:50 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_IMAGE_PIXEL_MANIP_H
#define PS_IMAGE_PIXEL_MANIP_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psImage.h"
#include "psCoord.h"
#include "psStats.h"
#include "psPixels.h"

/** Clip image values outside of range to given values
 *
 *  All pixels with values less than min are set to the value vmin.  all pixels
 *  with values greater than max are set to the value vmax. This function is
 *  defined for psU8, psU16, psS8, psS16, psF32, psF64.
 *
 *  @return int     The number of clipped pixels
 */
int psImageClip(
    psImage* input,                    ///< the image to clip
    double min,                        ///< the minimum image value allowed
    double vmin,                       ///< the value pixels < min are set to
    double max,                        ///< the maximum image value allowed
    double vmax                        ///< the value pixels > max are set to
);

/** Clip NaN image pixels to given value.
 *
 *  Pixels with NaN, +Inf, or -Inf values are set to the specified value. This
 *  function is defined for psF32, psF64.
 *
 *  @return int     The number of clipped pixels
 */
int psImageClipNaN(
    psImage* input,                    ///< the image to clip
    float value                        ///< the value to set all NaN/Inf values to
);

/** Overlay subregion of image with another image
 *
 *  Replace the pixels in the image which correspond to the pixels in OVERLAY
 *  with values derived from the IMAGE and OVERLAY based on the given operator
 *  OP.  Valid operators are "=" (set image value to OVERLAY value), "+" (add
 *  OVERLAY value to image value), "-" (subtract OVERLAY from image), "*"
 *  (multiply OVERLAY times image), "/" (divide image by OVERLAY).  This
 *  function is defined for psU8, psS8, psS16, psF32, psF64.
 *
 *  @return int         0 if success, non-zero if failed.
 */
int psImageOverlaySection(
    psImage* image,                    ///< target image
    const psImage* overlay,            ///< the overlay image
    int x0,                            ///< the column to start overlay
    int y0,                            ///< the row to start overlay
    const char *op                     ///< the operation to perform for overlay
);

/// @}
#endif // #ifndef PS_IMAGE_PIXEL_MANIP_H

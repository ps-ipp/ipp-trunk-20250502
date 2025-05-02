/** @file  psImageStructManip.h
*
*  @brief basic image structure manipulation operations
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-11-08 01:10:15 $
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifndef PSIMAGE_STRUCT_MANIP_H
#define PSIMAGE_STRUCT_MANIP_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psImage.h"
#include "psRegion.h"
#include "psRegionForImage.h"

/** Create a subimage of the specified area.
 *
 *  Define a subimage of the specified area of the given image. This function must raise an
 *  error if the requested subset area lies outside of the parent image and return NULL. The
 *  argument image is the parent image, region.x0, region.y0 specify the starting pixel of the
 *  subraster, and region.x1,region.y1 specify the extent of the desired subraster. Note that
 *  the row and column of this upper right-hand corner NOT included in the region. In the event
 *  that x1 or y1 are negative, they shall be interpreted as being relative to the size of the
 *  parent image in that dimension. The entire subraster must be contained within the raster of
 *  the parent image. Note that the refCounter for the parent should be incremented.  This
 *  function must be defined for the following types: psU8, psU16, psS8, psS16, psF32, psF64.
 *
 *  @return psImage* : Pointer to psImage.
 *
 */
#ifdef DOXYGEN
psImage* psImageSubset(
    psImage* image,                    ///< Parent image.
    psRegion region                    ///< region of subimage
);
#else // ifdef DOXYGEN
psImage* p_psImageSubset(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psImage* image,                    ///< Parent image.
    psRegion region                    ///< region of subimage
) PS_ATTR_MALLOC;
#define psImageSubset(image, region) \
      p_psImageSubset(__FILE__, __LINE__, __func__, image, region)
#endif // ifdef DOXYGEN

/** Makes a copy of the image view on the parent:
 *  if this is a child, returns a child pointing at the same pixels
 *  if this is a parent, returns a child pointing at the full array
 */
psImage* psImageCopyView(psImage *output, psImage *input);

/** Makes a copy of a psImage
 *
 * @return psImage* Copy of the input psImage.  This may not be equal to the
 * output parameter
 *
 */
#ifdef DOXYGEN
psImage* psImageCopy(
    psImage* output,                   ///< if not NULL, a psImage that could be recycled.
    const psImage* input,              ///< the psImage to copy
    psElemType type                    ///< the desired datatype of the returned copy
);
#else // ifdef DOXYGEN
psImage* p_psImageCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psImage* output,                   ///< if not NULL, a psImage that could be recycled.
    const psImage* input,              ///< the psImage to copy
    psElemType type                    ///< the desired datatype of the returned copy
) PS_ATTR_MALLOC;
#define psImageCopy(output, input, type) \
      p_psImageCopy(__FILE__, __LINE__, __func__, output, input, type)
#endif // ifdef DOXYGEN

/** Trim an image
 *
 *  Trim the specified image in-place, which involves shuffling the pixels around in memory.
 *  The pixels in the region [col0:col1,row0:row1] shall consist the output image.  The column
 *  col1 and row row1 are NOT included in the range.  In the event that x1 or y1 are
 *  non-positive, they shall be interpreted as being relative to the size of the parent image
 *  in that dimension.
 *
 *  If the entire specified subimage is not contained within the parent image, an error results
 *  and the return value will be NULL.
 *
 *  N.B. If the input psImage is a child of another psImage, no pixel data will be trimmed,
 *  rather it equivalent to calling psImageSubset.  If the input psImage is, however, a parent
 *  psImage, any children will be obliterated, i.e., freed from memory.
 *
 *  @return psImage*  trimmed image result
 */
psImage* psImageTrim(
    psImage* image,                    ///< image to trim
    psRegion region                    ///< trim region
);

/// @}
#endif // #ifndef PSIMAGE_STRUCT_MANIP_H

/** @file  psPixels.h
 *
 *  @brief Contains psPixel related functions
 *
 *  @author Paul Price, IfA
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.31 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_PIXELS_H
#define PS_PIXELS_H

#include "psImage.h"
#include "psVector.h"
#include "psRegion.h"

/// @addtogroup DataContainer Data Containers
/// @{

/** Data structure for storing psPixel coordinates  */
typedef struct
{
    float x;                             ///< x coordinate
    float y;                             ///< y coordinate
}
psPixelCoord;

/** list of pixel coordinates
 *
 *  Usually an image mask is the best way to carry information about what
 *  pixels mean what. However, in the case where the number of pixels in which
 *  we are interested is limited, it is more efficient to simply carry a list
 *  of pixels. An example of this is in the image combination code, where we
 *  want to perform an operation on a relatively small fraction of pixels, and
 *  it is inefficient to go through an entire mask image checking each pixel.
 *
 */
typedef struct
{
    long n;                            ///< Number in use
    const long nalloc;                 ///< Number allocated
    psPixelCoord* data;                ///< The pixel coordinates
    psMutex lock;                       ///< Option lock for thread safety
}
psPixels;

#define PS_ASSERT_PIXELS_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->data || (NAME)->n < 0 || (NAME)->nalloc < 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: Pixels %s or one of its components is NULL.", \
            #NAME); \
    return RVAL; \
}

#define P_PSPIXELS_SET_NALLOC(pix,n) *(long*)&pix->nalloc = n


/** Allocates a new psPixels structure
 *
 *  @return psPixels*   new psPixels
 */
#ifdef DOXYGEN
psPixels* psPixelsAlloc(
    long nalloc                         ///< the size of the coordinate vectors
);
#else // ifdef DOXYGEN
psPixels* p_psPixelsAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc                         ///< the size of the coordinate vectors
) PS_ATTR_MALLOC;
#define psPixelsAlloc(nalloc) \
      p_psPixelsAlloc(__FILE__, __LINE__, __func__, nalloc)
#endif // ifdef DOXYGEN

/** Allocates a new empty psPixels structure
 *
 *  @return psPixels*   new psPixels
 */
#ifdef DOXYGEN
psPixels* psPixelsAllocEmpty(
    long nalloc                         ///< the size of the coordinate vectors
);
#else // ifdef DOXYGEN
psPixels* p_psPixelsAllocEmpty(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc                         ///< the size of the coordinate vectors
);
#define psPixelsAllocEmpty(nalloc) \
      p_psPixelsAllocEmpty(__FILE__, __LINE__, __func__, nalloc)
#endif // ifdef DOXYGEN


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psPixels structure, false otherwise.
 */
bool psMemCheckPixels(
    psPtr ptr                          ///< the pointer whose type to check
);


/** resizes a psPixels structure
 *
 *  @return psPixels*   resized psPixels
 */
#ifdef DOXYGEN
psPixels* psPixelsRealloc(
    psPixels* pixels,                  ///< psPixels to resize, or NULL to create new psPixels
    long nalloc                        ///< the size of the coordinate vectors
);
#else // ifdef DOXYGEN
psPixels* p_psPixelsRealloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psPixels* pixels,                  ///< psPixels to resize, or NULL to create new psPixels
    long nalloc                        ///< the size of the coordinate vectors
);
#define psPixelsRealloc(pixels, nalloc) \
      p_psPixelsRealloc(__FILE__, __LINE__, __func__, pixels, nalloc)
#endif // ifdef DOXYGEN


/** Add a pixel location to a psPixels
 *
 *  Grow the psPixels input by growth.  If growth is less that 1, 10 is used.  If a NULL
 *  psPixels is given, a new one is created.
 *
 *  @return psPixels*       psPixels with the value appended.
 */
psPixels* psPixelsAdd(
    psPixels* pixels,                  ///< psPixels to append new coordinate to.
    long growth,                       ///< Number of elements to grow the pixels list if necessary.
    float x,                           ///< x coordinate to append
    float y                            ///< y coordinate to append
);


/** Copies a psPixels object
 *
 *  Makes a deep copy of the data in a psPixels object.  Any data in the OUT
 *  parameter will be destroyed and OUT will be resized, if necessary.
 *
 *  @return psPixels*   a new psPixels that is a duplicate to IN
 */
#ifdef DOXYGEN
psPixels* psPixelsCopy(
    psPixels* out,                     ///< psPixels struct to recycle, or NULL
    const psPixels* pixels             ///< psPixels struct to copy
);
#else // ifdef DOXYGEN
psPixels* p_psPixelsCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psPixels* out,                     ///< psPixels struct to recycle, or NULL
    const psPixels* pixels             ///< psPixels struct to copy
);
#define psPixelsCopy(out, pixels) \
      p_psPixelsCopy(__FILE__, __LINE__, __func__, out, pixels)
#endif // ifdef DOXYGEN


/** Generate a psImage from a psPixels
 *
 *  psPixelsToMask shall return an image of type U8 with the pixels lying
 *  within the specified region set to the maskVal. The out image shall be
 *  modified if supplied, or allocated and returned if NULL. The size of the
 *  output image shall be region->x1 - region->x0 by region->y1 - region->y0,
 *  with out->x0 = region->x0 and out->y0 = region->y0. In the event that
 *  either of pixels or region are NULL, the function shall generate an
 *  error and return NULL.
 *
 *  @return psImage*    generated mask image
 */
psImage* psPixelsToMask(
    psImage* out,                      ///< psImage to recycle, or NULL
    const psPixels* pixels,            ///< list of pixels to use
    psRegion region,                   ///< region to define the output mask image
    psImageMaskType maskVal		///< the mask bit-values to act upon
);


/** Generate a psPixels from a mask psImage
 *
 *  psMaskToPixels shall return a psPixels consisting of the coordinates in
 *  the mask that match the maskVal. The out pixel list shall be modified if
 *  supplied, or allocated and returned if NULL. In hte event that mask is
 *  NULL, the function shall generate an error and return NULL.
 *
 *  @return psPixels*   generated psPixels pixel list
 */
psPixels* psPixelsFromMask(
    psPixels *out,                     ///< psPixels to recycle, or NULL
    const psImage *mask,               ///< the input mask psImage
    psImageMaskType maskVal		///< the mask bit-values to act upon
);


/** Concatenates two psPixels
 *
 *  psPixelsConcatenate shall concatenate pixels onto out. In the event that
 *  out is NULL, a new psPixels shall be allocated, and the contents of
 *  pixels simply copied in. If pixels is NULL, the function shall generate
 *  an error and return NULL.
 *
 *  @return psPixels         Concatenated psPixel list
 */
psPixels* psPixelsConcatenate(
    psPixels *out,                     ///< psPixels to recycle, or NULL
    const psPixels *pixels             ///< psPixels to append to OUT
);

/// Remove duplicates in a list of pixels
psPixels* psPixelsDuplicates(
    psPixels *out, ///< Output list with duplicates removed, or NULL
    const psPixels *pixels ///< Input list of pixels
    );

/** Prints a psPixels to specified destination.
 *
 *  @return bool:    True if successful.
*/
bool p_psPixelsPrint(
    FILE *fd,                          ///< destination file descriptor
    psPixels* pixels,                  ///< psPixels to print
    const char *name                   ///< printf-style format of header line
);


/** Sets the value of the the pixels array at the specified position to value.
 *
 *  A negative position means index from the end.
 *
 *  @return bool:       True if Successful, otherwise false.
*/
bool psPixelsSet(
    psPixels *pixels,                  ///< pixels to set
    long position,                     ///< position to set
    psPixelCoord value                 ///< pixels value to be set
);


/** Returns the value of the pixels array at the specified position.
 *
 *  A negative position means index from the end.
 *
 *  @return psPixelCoord:       The value of the pixels at the specified position.
*/
psPixelCoord psPixelsGet(
    const psPixels *pixels,            ///< input pixels from which to get
    long position                      ///< position to get
);


/** Get the number of elements in use from a specified psPixels. (pixels.n)
 *
 *  @return long:       The number of elements in use.
 */
long psPixelsLength(
    const psPixels *pixels             ///< input psPixels
);


/// @}
#endif // #ifndef PS_PIXELS_H

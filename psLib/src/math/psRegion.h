/* @file  psRegion.h
 * @brief image regions and related functions
 *
 * $Revision: 1.9 $ $Name: not supported by cvs2svn $
 * $Date: 2007-06-10 17:54:05 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_REGION_H
#define PS_REGION_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psString.h"

/** Basic image region structure.
 *
 * Struct for specifying a rectangular area in an image.
 *
 */
typedef struct
{
    float x0;                          ///< the first column of the region.
    float x1;                          ///< the last column of the region.
    float y0;                          ///< the first row of the region.
    float y1;                          ///< the last row of the region.
}
psRegion;

/** Create a pointer to a psRegion, with associated psMemBlock.
 *
 * @return psRegion* : a new psRegion.
 */
psRegion *psRegionAlloc(
    float x0,                          ///< the first column of the region.
    float x1,                          ///< the last column of the region + 1.
    float y0,                          ///< the first row of the region.
    float y1                           ///< the last row of the region + 1.
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psRegion structure, false otherwise.
 */
bool psMemCheckRegion(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Create a psRegion with the specified attributes.
 *
 *  @return psRegion :      a corresponding psRegion.
 */
psRegion psRegionSet(
    float x0,                          ///< the first column of the region.
    float x1,                          ///< the last column of the region + 1.
    float y0,                          ///< the first row of the region.
    float y1                           ///< the last row of the region + 1.
);

/** Create a psRegion with the attribute values given as a string.
 *
 *  Create a psRegion with the attribute values given as a string.  The format
 *  shall be of the standard IRAF form '[x0:x1,y0:y1]'
 *
 *  @return psRegion:       A new psRegion struct, or NULL is not successful.
 */
psRegion psRegionFromString(
    const char* region                 ///< image rectangular region in the form '[x0:x1,y0:y1]'
);

/** Create a psRegion from a string in IRAF form '[x0:x1,y0:y1]', returning range parities
 *
 *  Create a psRegion using the range defined by a string in the standard IRAF form
 *  '[x0:x1,y0:y1]'.  Unlike psRegionFromString, the ranges may have x0 > x1 or y0 > y1, in
 *  which case the xParity or yParity terms will be set to -1 (instead of the default +1).
 *
 *  @return psRegion:       A new psRegion struct, or NULL is not successful.
 */
psRegion psRegionAndParityFromString(
    int *xParity,		      ///< +1 if x0 <= x1, -1 otherwise
    int *yParity,		      ///< +1 if y0 <= y1, -1 otherwise
    const char* region		      ///< image rectangular region in the form '[x0:x1,y0:y1]'
);

/** Create a string of the standard IRAF form '[x0:x1,y0:y1]' from a psRegion.
 *
 *  @return psString:  A new string representing the psRegion as text, or NULL
 *                  is not successful.
 */
psString psRegionToString(
    const psRegion region              ///< the psRegion to convert to a string
);

/** Defines a region corresponding to the square with center at coordinate x,y
 *  and with coderadius.  The width of the square is 2radius + 1.
 *
 *  @return psRegion:       the newly defined psRegion.
 */
psRegion psRegionForSquare(
    double x,                          ///< x coordinate at square-center
    double y,                          ///< y coordinate at square-center
    double radius                      ///< radius of square
);

/** Test if any element of the region is NaN
 *
 * @return bool:        True if an element is NaN, otherwise false.
 */
bool psRegionIsNaN(
    psRegion region                    ///< Region to check
);

/// @}
#endif

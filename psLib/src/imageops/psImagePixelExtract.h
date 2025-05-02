/* @file  psImagePixelExtract.h
 *
 * @brief Basic image extraction operations
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:37 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PSIMAGE_PIXEL_EXTRACT_H
#define PSIMAGE_PIXEL_EXTRACT_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psImage.h"
#include "psVector.h"
#include "psStats.h"
#include "psPixels.h"

/* Cut direction flag.  Used with psImageCut function.
 */
typedef enum {
    PS_CUT_X_POS,                      ///< Cut in the x dimension from left to right
    PS_CUT_X_NEG,                      ///< Cut in the x dimension from rigth to left
    PS_CUT_Y_POS,                      ///< Cut in the y dimension from bottom up
    PS_CUT_Y_NEG                       ///< Cut in the y dimension from top down.
} psImageCutDirection;

/** Extracts a single complete row from the image and returns it to the
 *  provided vector, allocating it if it is NULL.
 *
 *  @return psVector*:      The row data extracted from psImage input
 */
psVector *psImageRow(
    psVector *out,                     ///< specified vector to return
    const psImage *input,              ///< input image
    int row                            ///< row number to extract
);

/** Extracts a single complete column from the image and returns it to the
 *  provided vector, allocating it if it is NULL.
 *
 *  @return psVector*:      The column data extracted from psImage input
 */
psVector *psImageCol(
    psVector *out,                     ///< specified vector to return
    const psImage *input,              ///< input image
    int column                         ///< column number to extract
);

/** Extract pixels from rectlinear region to a vector (array of floats).
 *
 *  The output vector contains either col1-col0 or row1-row0 elements, based
 *  on the value of the direction: e.g., if direction is PS_CUT_X_POS, there
 *  are col1-col0 elements. The region to be  sliced  is defined by the
 *  lower-left corner, (col0,row0), and the upper-right corner, (col1,row1).
 *  Note that the row and column of the  upper right-hand corner  are NOT
 *  included in the region. In the event that col1 or row1 are negative, they
 *  shall be interpreted as being relative to the size of the parent image in
 *  that dimension. The input region is collapsed in the direction perpendicular
 *  to that specified by direction, and each element of the output vectors is
 *  derived from the statistics of the pixels at that direction coordinate. The
 *  statistic used to derive the output vector value is specified by stats.
 *  If mask is non-NULL, pixels for which the corresponding mask pixel
 *  matches maskVal are excluded from operations. If coords is not NULL, the
 *  calculated coordinates along the slice are returned in this vector. Only
 *  one of the statistics choices may be specified, otherwise the function
 *  must return an error.
 *
 *  This function is defined for the following types: psS8, psU16, psF32, psF64.
 *
 * @return psVector    the resulting vector
 */
psVector* psImageSlice(
    psVector* out,                     ///< psVector to recycle, or NULL.
    psPixels* coords,
    ///< If not NULL, it is populated with the coordinate in the slice dimension
    ///< coorsponding to the output vector's value of the same position in the
    ///< vector.  This vector maybe resized and retyped as appropriate.
    const psImage* input,              ///< the input image in which to perform the slice
    const psImage* mask,               ///< the mask for the input image.
    psImageMaskType maskVal,		///< the mask value to apply to the mask
    psRegion region,                   ///< the slice region
    psImageCutDirection direction,     ///< the slice dimension and direction
    const psStats* stats               ///< the statistic to perform in slice operation
);

/** Extract pixels from an image along a line to a vector (array of floats).
 *
 *  The vector (xs,ys) - (xe,ye) forms the basis of the output vector. Pixels
 *  are considered in a rectangular region of width dw about this vector. The
 *  input region is collapsed in the perpendicular direction, and each element
 *  of the output vector represents pixel-sized boxes, where the value is
 *  derived from the statistics of the pixels interpolated along the
 *  perpendicular direction. The specific algorithm which must be used is
 *  described in the PSLib ADD (PSDC-430-006). The statistic used to derive
 *  the output vector value is specified by stats. Only one of the statistics
 *  choices may be specified, otherwise the function must return an error.
 *  This function must be defined for the following types: psS8, psU16, psF32,
 *  psF64.
 *
 *  @return psVector*    resulting vector
 */
psVector* psImageCut(
    psVector* out,                     ///< psVector to recycle, or NULL.
    psVector* cutCols,                 ///< if not NULL, the calculated column values along the slice (output)
    psVector* cutRows,                 ///< if not NULL, the calculated row values along the slice (output)
    const psImage* input,              ///< the input image in which to perform the cut
    const psImage* mask,               ///< the mask for the input image.
    psImageMaskType maskVal,		///< the mask value to apply to the mask
    psRegion region,                   ///< the start and end points to cut along
    unsigned int nSamples,             ///< the number of samples along the cut
    psImageInterpolateMode mode        ///< the interpolation method to use
);

/** Extract radial region data to a vector. A vector is constructed where each
 *  vector elements is derived from the statistics of the pixels which land
 *  within one of a sequence of radii. The radii are centered on the image
 *  pixel coordinate x,y, and are defined by the sequence of values in the
 *  vector radii. The specific algorithm which must be used is described in
 *  the PSLib ADD (PSDC-430-006). The statistic used to derive the output
 *  vector value is specified by stats. Only one of the statistics choices
 *  may be specified, otherwise the function must return an error. This
 *  function must be defined for the following types: psS8, psU16, psF32,
 *  psF64.
 *
 *  @return psVector    resulting vector
 */
psVector* psImageRadialCut(
    psVector* out,                     ///< psVector to recycle, or NULL.
    const psImage* input,              ///< the input image in which to perform the cut
    const psImage* mask,               ///< the mask for the input image.
    psImageMaskType maskVal,		///< the mask value to apply to the mask
    float x,                           ///< the column of the center of the cut circle
    float y,                           ///< the row of the center of the cut circle
    const psVector* radii,             ///< the radii of the cut circle
    const psStats* stats               ///< the statistic to perform in operation
);

/// @}
#endif // #ifndef PSIMAGE_PIXEL_EXTRACT_H

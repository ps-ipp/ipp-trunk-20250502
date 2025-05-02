/* @file psMathUtils.h
 * @brief Standard Mathematical Functions.
 *
 * This file contains standard math rotines.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-14 02:36:28 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_MATHUTILS_H
#define PS_MATHUTILS_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include "psVector.h"
#include "psScalar.h"
#include "psPolynomial.h"

typedef enum {
    PS_BINARY_DISECT_PASS,
    PS_BINARY_DISECT_OUTSIDE_RANGE,
    PS_BINARY_DISECT_INVALID_INPUT,
    PS_BINARY_DISECT_INVALID_TYPE,
} psVectorBinaryDisectResult;

/** Performs a binary disection on a monotonically non-decreasing vector.
 *  Searches through an array of data for a specified value.
 *
 *  @return psS32    corresponding index number of specified value
 */
psS32 psVectorBinaryDisect(
    psVectorBinaryDisectResult *status,
    const psVector *bins,               ///< Array of non-decreasing values
    const psScalar *x                   ///< Target value to find
);

/** Interpolates a series of data points for evaluation at a specific coordinate.  Uses a
 *  Lagrange interpolation method.
 *
 *  @return psScalar*    Lagrange interpolation value at given location
 */
psScalar *p_psVectorInterpolate(
    psScalar *out,                      ///< Output scalar, or NULL
    const psVector *domain,             ///< Domain (x coords) for interpolation
    const psVector *range,              ///< Range (y coords) for interpolation
    psS32 order,                        ///< Order of interpolation function
    const psScalar *x                   ///< Location at which to evaluate
);

bool p_psNormalizeVectorRange(psVector* myData,
                              psF64 outLow,
                              psF64 outHigh);

/// @}
#endif // #ifndef PS_MATHUTILS_H

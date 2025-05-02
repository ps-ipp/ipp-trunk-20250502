/* @file psSpline.h
 * @brief Standard Mathematical Functions.
 *
 * This file will hold the prototypes for procedures which allocate, free,
 * and evaluate splines.
 *
 * @author GLG (MHPCC)
 * reworked by EAM (IfA)
 *
 * @version $Revision: 1.62 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-08-09 01:40:07 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_SPLINE_H
#define PS_SPLINE_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>

#include "psVector.h"
#include "psScalar.h"
#include "psPolynomial.h"

/** One-Dimensional Spline
    This structure represents a 1D cubic spline.  Note the option for 
    equally-spaced knots allows a quick selection of the correct spline 
    segment.  The values (xMin, xMax, xDel) are stored for ease of access.
 */
typedef struct
{
    unsigned int n;    ///< The number of knots
    psF32 *xKnots; ///< x-coordinate of the knots
    psF32 *yKnots; ///< y-coordinate of the knots
    psF32 *d2yKnots; ///< 2nd derivative of y at the knots
    bool equalSpacing; // if knots are equally spaced, the seqment choice can be optimized
    psF32 xMin; // for equally-spaced knots, the value at the lower bound (xKnots[0])
    psF32 xMax; // for equally-spaced knots, the value at the upper bound (xKnots[n])
    psF32 xDel; // for equally-spaced knots, the spacing (xKnots[n] - xKnots[0])/n
}
psSpline1D;

/** Allocator for psSpline1D.
 *
 *  @return psSpline1D*    new 1-D spline struct
 */
psSpline1D *psSpline1DAlloc(void) PS_ATTR_MALLOC;

/** Create an empty 1D spline **/
psSpline1D *psSpline1DCreate(
    int nKnots);			///< number of knots

/** Evaluates 1-D spline polynomials at a specific coordinate.
 *
 *  @return float    result of spline polynomials evaluated at given location
 */
float psSpline1DEval(
    const psSpline1D *spline,          ///< spline pointer
    float x                            ///< location at which to evaluate
);

/** Evaluates 1-D spline polynomials at a specific coordinate.
 *
 *  @return float    result of spline polynomials evaluated at given location
 */
float psSpline1DEval_Segment(
    const psSpline1D *spline,          ///< spline pointer
    int n,			       // choice of segment
    float x                            ///< location at which to evaluate
);

/** Evaluates 1-D spline polynomials at a set of specific coordinates.
 *
 *  @return psVector*    results of spline polynomials evaluated at given locations
 */
psVector *psSpline1DEvalVector(
    const psSpline1D *spline,          ///< Coefficients of spline polynomials
    const psVector *x                  ///< locations at which to evaluate
);

/** Derive a one-dimensional spline fit.
 *
 *  Given a psSpline1D data structure and a set of x,y vectors, this routine
 *  generates the linear splines which satisfy those data points.
 *
 *  XXX EAM: add option to generate / select a subset of knots from the data
 *
 *  @return psSpline1D*:  the calculated one-dimensional splines
 */
psSpline1D *psSpline1DFitVector(
    const psVector* x,                 ///< Ordinates (or NULL to just use the indices)
    const psVector* y,                  ///< Coordinates.
    psF32 dyLower,			///< 1st derivative at lower bound
    psF32 dyUpper			///< 1st derivative at upper bound
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psSpline1D structure, false otherwise.
 */
bool psMemCheckSpline1D(
    psPtr ptr                          ///< the pointer whose type to check
);

// check for equal spacing and set internal boolean if true
bool psSpline1DisEqualSpacing (psSpline1D *spline);

// convert the cubic spline elements to a simply ordinary polynomial for segment n
psPolynomial1D *psSpline1DToPoly (psSpline1D *spline, int n);

/*****************************************************************************
    PS_SPLINE macros:
*****************************************************************************/
#define PS_ASSERT_SPLINE(NAME, RVAL) \
if (false == psMemCheckSpline1D(NAME)) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: argument %s is not a psSpline1D struct.\n",\
            #NAME); \
    return(RVAL); \
} \

#define PS_ASSERT_SPLINE_NON_NULL(NAME, RVAL) \
if ((NAME) == NULL) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: psSpline1D %s is NULL.", \
            #NAME); \
    return(RVAL); \
} \


/// @}
#endif // #ifndef PS_SPLINE_H

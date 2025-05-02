/* @file  psMinimizePolyFit.c
 * @brief basic minimization functions
 *
 * This file will contain function prototypes for various
 * 1-D polynomial fitting routines.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifndef PS_MINIMIZE_POLYFIT_H
#define PS_MINIMIZE_POLYFIT_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psVector.h"
#include "psMemory.h"
#include "psArray.h"
#include "psImage.h"
#include "psMatrix.h"
#include "psPolynomial.h"
#include "psSpline.h"
#include "psStats.h"
#include "psTrace.h"
#include "psError.h"
#include "psConstants.h"

/** Derive a polynomial fit.
 *
 *  psVectorFitPolynomial1d returns the polynomial that best fits the
 *  observations. The input parameters are a polynomial that specifies the
 *  fit order, myPoly, which will be altered and returned with the best-fit
 *  coefficients; and the observations, x, y and yErr. The independent
 *  variable list, x may be NULL, in which case the vector index is used.
 *  The dependent variable error, yErr may be null, in which case the solution
 *  is determined in the assumption that all data errors are equal. This
 *  function must be valid only for types psF32, psF64.
 *
 *  @return psPolynomial1D*    polynomial fit
 */

bool psVectorFitPolynomial1D(
    psPolynomial1D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x
);

bool psVectorFitPolynomial2D(
    psPolynomial2D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y
);

bool psVectorFitPolynomial3D(
    psPolynomial3D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z
);

bool psVectorFitPolynomial4D(
    psPolynomial4D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z,
    const psVector *t
);


bool psVectorClipFitPolynomial1D(
    psPolynomial1D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x
);

bool psVectorClipFitPolynomial2D(
    psPolynomial2D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y
);

bool psVectorClipFitPolynomial3D(
    psPolynomial3D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z
);

bool psVectorClipFitPolynomial4D(
    psPolynomial4D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z,
    const psVector *t
);

/// @}
#endif // #ifndef PS_MINIMIZE_POLYFIT_H

/** @file psSpline.c
    re-written by EAM 2023.01.23
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>

#include "psMemory.h"
#include "psVector.h"
#include "psScalar.h"
#include "psTrace.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psPolynomial.h"
#include "psSpline.h"
#include "psAssert.h"

#include "psMathUtils.h"

static void psSpline1DFree(psSpline1D *tmpSpline)
{
    if (tmpSpline == NULL) return;

    psFree(tmpSpline->xKnots);
    psFree(tmpSpline->yKnots);
    psFree(tmpSpline->d2yKnots);

    return;
}

/*
static void PS_PRINT_SPLINE2(psSpline1D *mySpline)
{
    printf("-------------- PS_PRINT_SPLINE2() --------------\n");
    printf("mySpline->n is %d\n", mySpline->n);
    if (!mySpline->xKnots) return;
    if (!mySpline->yKnots) return;
    if (!mySpline->d2yKnots) return;
    for (psS32 i = 0 ; i < mySpline->n ; i++) {
	printf("(x, y, d2y) : %f %f %f\n", mySpline->xKnots[i], mySpline->yKnots[i], mySpline->d2yKnots[i]);
    }
}
*/

/*****************************************************************************
CalculateSecondDerivs(): Given a set of x,y vectors corresponding to the spline knots, this
routine calculates the second derivatives of the interpolating cubic splines at those n points.

The boundary conditions may be:
* first derivative specified OR
* second derivatives = 0

One or the other condition may be true for each of the upper and lower bounds

NOTE: vectors must be F32

EAM 2023.01.19 : the comment above is wrong: the code below implements the 
splines with *only* the 1st derivatives at the end points set to 0.0.  the 
2nd derivatives are constrained by the equations for the splines.  it is not 
possible to specify both the endpoint 1st and 2nd derivaties: there would be 
too many constraints for the number of free parameters.

It is not clear that choosing to set the end point 1st derivatives to 0.0 is the best
option.  setting the 2nd derivatives to zero allow for linear extrapolation.

 *****************************************************************************/

void calculateSecondDerivs(
    const psSpline1D *mySpline,
    psF32 dyLower, // if not NAN, lower-bound 1st derivative is defined
    psF32 dyUpper  // if not NAN, lower-bound 1st derivative is defined
    )                  
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);

    psS32 n = mySpline->n; // n is the number of knots
    psF32 *u   = (psF32 *) psAlloc(n * sizeof(psF32));

    psF32 *X   = mySpline->xKnots;
    psF32 *Y   = mySpline->yKnots;
    psF32 *d2y = mySpline->d2yKnots;

    if (isfinite(dyLower)) {
      d2y[0] = -0.5;
      u[0]   = (3.0/(X[1]-X[0])) * ((Y[1]-Y[0])/(X[1]-X[0]) - dyLower);
    } else {
      d2y[0] = 0.0;
      u[0]   = 0.0;
    }

    for (psS32 i = 1; i < n - 1; i++) {
        psF32 dX = (X[i] - X[i-1]) / (X[i+1] - X[i-1]);
        psF32 dY = dX * d2y[i-1] + 2.0;
        d2y[i] = (dX - 1.0) / dY;
        u[i]   = ((Y[i+1] - Y[i])/(X[i+1] - X[i])) - ((Y[i] - Y[i-1])/(X[i] - X[i-1]));
        u[i]   = ((6.0 * u[i] / (X[i+1] - X[i-1])) - (dX * u[i-1])) / dY;

        psTrace("psLib.math", 6, "X[%d] is %f\n", i, X[i]);
        psTrace("psLib.math", 6, "Y[%d] is %f\n", i, Y[i]);
        psTrace("psLib.math", 6, "u[%d] is %f\n", i, u[i]);
    }

    if (isfinite(dyUpper)) {
      psF32 qn = 0.5;
      u[n-1] = (3.0/(X[n-1]-X[n-2])) * (dyUpper - (Y[n-1]-Y[n-2])/(X[n-1]-X[n-2]));
      d2y[n-1] = (u[n-1] - (qn * u[n-2])) / ((qn * d2y[n-2]) + 1.0);
    } else {
      d2y[n-1] = 0;
    }

    for (psS32 k = n-2; k >= 0; k--) {
	d2y[k] = d2y[k] * d2y[k+1] + u[k];
        psTrace("psLib.math", 6, "derivs2[%d] is %f\n", k, d2y[k]);
    }
    psFree(u);
    psTrace("psLib.math", 4, "---- %s() end ----\n", __func__);
    return;
}


/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

bool psMemCheckSpline1D(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) psSpline1DFree );
}

psSpline1D *psSpline1DAlloc(void)
{
    psSpline1D *tmpSpline = (psSpline1D *) psAlloc(sizeof(psSpline1D));

    tmpSpline->n = 0;
    tmpSpline->xMin = NAN;
    tmpSpline->xMax = NAN;
    tmpSpline->xDel = NAN;
    tmpSpline->equalSpacing = false;

    tmpSpline->xKnots   = NULL;
    tmpSpline->yKnots   = NULL;
    tmpSpline->d2yKnots = NULL;
    psMemSetDeallocator(tmpSpline, (psFreeFunc) psSpline1DFree);

    return(tmpSpline);
}

/** Create an empty 1D spline **/
psSpline1D *psSpline1DCreate(
    int nKnots)			///< number of knots
{
    psSpline1D *spline = psSpline1DAlloc();
    spline->n = nKnots; // number of knots

    spline->xKnots   = (psF32 *) psAlloc( spline->n * sizeof(psF32));
    spline->yKnots   = (psF32 *) psAlloc( spline->n * sizeof(psF32));
    spline->d2yKnots = (psF32 *) psAlloc( spline->n * sizeof(psF32));

    // x & y can both be F32 or F64. should knots be F64?
    for (psS32 i = 0 ; i < spline->n ; i++) {
	spline->xKnots[i]   = NAN;
	spline->yKnots[i]   = NAN;
	spline->d2yKnots[i] = NAN;
    }
    return(spline);
}

/*****************************************************************************
psSpline1DFitVector(): given a set of x,y vectors, this routine generates the
linear or cublic splines which satisfy those data points.

The formula for calculating the spline polynomials is derived from Numerical
Recipes in C.  The basic idea is that the polynomial is
 (1)     y = (A * y[0]) +
 (2)         (B * y[1]) +
 (3)         ((((A*A*A)-A) * mySpline->p_psDeriv2[0]) * H^2)/6.0 +
 (4)         ((((B*B*B)-B) * mySpline->p_psDeriv2[1]) * H^2)/6.0
Where:
 H = x[1]-x[0]
 A = (x[1]-x)/H
 B = (x-x[0])/H
The bulk of the code in this routine is the expansion of the above equation
into a polynomial in terms of x, and then saving the coefficients of the
powers of x in the spline polynomials.  This gets pretty complicated.

XXX: What types must be supported?
 *****************************************************************************/
psSpline1D *psSpline1DFitVector(
    const psVector* x,                  ///< Ordinates.
    const psVector* y,                  ///< Coordinates.
    psF32 dyLower,			///< 1st derivative at lower bound
    psF32 dyUpper)			///< 1st derivative at upper bound
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(y, NULL);
    PS_ASSERT_LONG_LARGER_THAN_OR_EQUAL(y->n, (long)2, NULL);

    psSpline1D *spline = psSpline1DCreate(y->n);

    // x & y can both be F32 or F64. should knots be F64?
    for (psS32 i = 0 ; i < spline->n ; i++) {
	spline->xKnots[i] = (x->type.type == PS_TYPE_F32) ? x->data.F32[i] : x->data.F64[i];
	spline->yKnots[i] = (y->type.type == PS_TYPE_F32) ? y->data.F32[i] : y->data.F64[i];
    }

    // Generate the second derivatives at each data point.
    calculateSecondDerivs(spline, dyLower, dyUpper);

    psTrace("psLib.math", 3, "---- %s() end ----\n", __func__);
    return(spline);
}

psPolynomial1D *psSpline1DToPoly (psSpline1D *spline, int n) {
    PS_ASSERT_INT_LESS_THAN(n, spline->n - 1, NULL);

    // convert the cubic spline coeffs to a polynomial. See above function comments and
    // Numerical Recipes in C.

    psF32 *xKnots   = spline->xKnots;
    psF32 *yKnots   = spline->yKnots;
    psF32 *d2yKnots = spline->d2yKnots;

    psF32 H = xKnots[n+1] - xKnots[n];
    if (fabs(H) <= FLT_EPSILON) {
        psError(PS_ERR_UNKNOWN, false, "x data points are not distinct (%d %d) (%f %f).\n",
                n, n+1, xKnots[n], xKnots[n+1]);
    }

    psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 3);

    // ******** Calculate 0-order term ********
    // From (1)
    myPoly->coeff[0] = yKnots[n] * xKnots[n+1]/H;
    // From (2)
    myPoly->coeff[0]-= (yKnots[n+1] * xKnots[n])/H;
    // From (3)
    psF32 tmp = (xKnots[n+1] * xKnots[n+1] * xKnots[n+1]) / (H * H * H);
    tmp-= xKnots[n+1] / H;
    tmp*= d2yKnots[n] * H * H / 6.0;
    myPoly->coeff[0]+= tmp;
    // From (4)
    tmp = -(xKnots[n] * xKnots[n] * xKnots[n]) / (H * H * H);
    tmp+= xKnots[n] / H;
    tmp*= d2yKnots[n+1] * H * H / 6.0;
    myPoly->coeff[0]+= tmp;

    //
    // ******** Calculate 1-order term ********
    //
    // From (1)
    myPoly->coeff[1] = -(yKnots[n]) / H;
    // From (2)
    myPoly->coeff[1]+= yKnots[n+1] / H;
    // From (3)
    tmp = -3.0 * xKnots[n+1] * xKnots[n+1] / (H * H * H);
    tmp+= (1.0 / H);
    tmp*= d2yKnots[n] * H * H / 6.0;
    myPoly->coeff[1]+= tmp;
    // From (4)
    tmp = 3.0 * xKnots[n] * xKnots[n] / (H * H * H);
    tmp-= 1.0 / H;
    tmp*= d2yKnots[n+1] * H * H / 6.0;
    myPoly->coeff[1]+= tmp;

    //
    // ******** Calculate 2-order term ********
    //
    // From (3)
    myPoly->coeff[2] = d2yKnots[n] * 3.0 * xKnots[n+1] / (6.0 * H);
    // From (4)
    myPoly->coeff[2]-= d2yKnots[n+1] * 3.0 * xKnots[n] / (6.0 * H);

    //
    // ******** Calculate 3-order term ********
    //
    // From (3)
    myPoly->coeff[3] = -d2yKnots[n] / (6.0 * H);
    // From (4)
    myPoly->coeff[3]+=  d2yKnots[n+1] / (6.0 * H);

    psTrace("psLib.math", 6, "(spline->spline[%u])->coeff[0] is %f\n", n, myPoly->coeff[0]);
    psTrace("psLib.math", 6, "(spline->spline[%u])->coeff[1] is %f\n", n, myPoly->coeff[1]);
    psTrace("psLib.math", 6, "(spline->spline[%u])->coeff[2] is %f\n", n, myPoly->coeff[2]);
    psTrace("psLib.math", 6, "(spline->spline[%u])->coeff[3] is %f\n", n, myPoly->coeff[3]);

    return myPoly;
}

// given an already-constructed spline, check/assert that the
// knot spacing is equal.  this allows some optimization
bool psSpline1DisEqualSpacing (psSpline1D *spline) {

    PS_ASSERT_PTR_NON_NULL(spline, false);
    PS_ASSERT_PTR_NON_NULL(spline->xKnots, false);
    PS_ASSERT_PTR_NON_NULL(spline->yKnots, false);
    PS_ASSERT_PTR_NON_NULL(spline->d2yKnots, false);
    
    // if the spline has equally-spaced xKnots, the values of the
    // xKnots can be predicted from the first, last, and delta values

    int n = spline->n;
    spline->xMax = spline->xKnots[n-1];
    spline->xMin = spline->xKnots[0];
    spline->xDel = (spline->xMax - spline->xMin) / (n - 1);
    
    // check that the xKnots actually follow this spacing:

    for (int i = 1; i < n - 1; i++) {
	float xValue = spline->xMin + i*spline->xDel;
	fprintf (stderr, "%d %f - %f = %f\n", i, spline->xKnots[i], xValue, spline->xKnots[i] - xValue);
    }

    spline->equalSpacing = true;
    return true;
}

// XXX EAM : changing implementation to use yKnot, d2yKnot instead of polynomials
float psSpline1DEval(
    const psSpline1D *spline,
    float x)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(spline, NAN);
    PS_ASSERT_INT_NONNEGATIVE(spline->n, NAN);
    PS_ASSERT_PTR_NON_NULL(spline->xKnots, NAN);

    psS32 n = spline->n;

    // XXX this should be linear extrapolation at the high or low ends
    if (x < spline->xKnots[0])   return psSpline1DEval_Segment(spline,   0, x);
    if (x > spline->xKnots[n-1]) return psSpline1DEval_Segment(spline, n-2, x);

    if (spline->equalSpacing) {
	int bin = (x - spline->xMin) / spline->xDel;
	bin = PS_MIN(PS_MAX(bin, 0), n - 2);
	return psSpline1DEval_Segment(spline, bin, x);
    }

    /* find correct element in array (x must be sorted) */
    int lo = 0;
    int hi = n-1;
    while (hi - lo > 1) {
	int i = 0.5*(hi+lo);
	if (spline->xKnots[i] > x) {
	    hi = i;
	} else {
	    lo = i;
	}
    }

    return psSpline1DEval_Segment(spline, lo, x);
}

// evaluate the spline at the given coordinate using the specified segment
// (YMMV if you use the wrong segment!)
float psSpline1DEval_Segment(
    const psSpline1D *spline,
    int n,
    float x)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(spline, NAN);
    PS_ASSERT_PTR_NON_NULL(spline->xKnots, NAN);
    PS_ASSERT_PTR_NON_NULL(spline->yKnots, NAN);
    PS_ASSERT_PTR_NON_NULL(spline->d2yKnots, NAN);
    PS_ASSERT_INT_NONNEGATIVE(spline->n, NAN);
    PS_ASSERT_INT_LESS_THAN(n, spline->n - 1, NAN);

    psF32 dX = spline->xKnots[n+1] - spline->xKnots[n];
    psF32 A  = (spline->xKnots[n+1] - x) / dX;
    psF32 B  = (x - spline->xKnots[n]) / dX;

    psF32 value = A*spline->yKnots[n] + B*spline->yKnots[n+1] + ((A*A*A - A)*spline->d2yKnots[n] + (B*B*B - B)*spline->d2yKnots[n+1])*(dX*dX) / 6.0;
    return value;
}

/*****************************************************************************
 returns a vector of the same type as the input (x)
 *****************************************************************************/
psVector *psSpline1DEvalVector(
    const psSpline1D *spline,
    const psVector *x)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(spline,           NULL);
    PS_ASSERT_PTR_NON_NULL(spline->xKnots,   NULL);
    PS_ASSERT_PTR_NON_NULL(spline->yKnots,   NULL);
    PS_ASSERT_PTR_NON_NULL(spline->d2yKnots, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spline->n,     NULL);

    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);

    psVector *tmpVector = psVectorAlloc(x->n, x->type.type);
    if (x->type.type == PS_TYPE_F32) {
        for (psS32 i=0;i<x->n;i++) {
            tmpVector->data.F32[i] = psSpline1DEval(spline, x->data.F32[i]);
        }
    } else {
        for (psS32 i=0;i<x->n;i++) {
            tmpVector->data.F64[i] = psSpline1DEval(spline, (psF32) x->data.F64[i]);
        }
    }
    
    psTrace("psLib.math", 3, "---- %s() end ----\n", __func__);
    return(tmpVector);
}

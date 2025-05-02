/** @file  psPolynomial.c
*
*  @brief Contains basic function allocation, deallocation, and evaluation
*         routines.
*
*  This file will hold the routiness for allocating, freeing, and evaluating
*  polynomials.  It also contains a Gaussian functions.
*
*  @version $Revision: 1.158 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-10-09 19:24:46 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*
*  XXX: Should the "coeffErr[]" be used as well?  Bug ???.  Ignore coeffErr
*
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*****************************************************************************/
/*  INCLUDE FILES                                                            */
/*****************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>

#include "psRandom.h"
#include "psMemory.h"
#include "psVector.h"
#include "psScalar.h"
#include "psTrace.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psPolynomial.h"
#include "psAbort.h"
#include "psAssert.h"


/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/
static void polynomial1DFree(psPolynomial1D* poly);
static void polynomial2DFree(psPolynomial2D* poly);
static void polynomial3DFree(psPolynomial3D* poly);
static void polynomial4DFree(psPolynomial4D* poly);

/*****************************************************************************/
/* GLOBAL VARIABLES                                                          */
/*****************************************************************************/

// None

/*****************************************************************************/
/* FILE STATIC VARIABLES                                                     */
/*****************************************************************************/

// None

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - LOCAL                                           */
/*****************************************************************************/

bool psMemCheckPolynomial1D(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)polynomial1DFree );
}

bool psMemCheckPolynomial2D(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)polynomial2DFree );
}

bool psMemCheckPolynomial3D(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)polynomial3DFree );
}

bool psMemCheckPolynomial4D(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)polynomial4DFree );
}

static void polynomial1DFree(psPolynomial1D* poly)
{
    psFree(poly->coeff);
    psFree(poly->coeffErr);
    psFree(poly->coeffMask);
}

static void polynomial2DFree(psPolynomial2D* poly)
{
    unsigned int x = 0;

    for (x = 0; x < (1 + poly->nX); x++) {
        psFree(poly->coeff[x]);
        psFree(poly->coeffErr[x]);
        psFree(poly->coeffMask[x]);
    }
    psFree(poly->coeff);
    psFree(poly->coeffErr);
    psFree(poly->coeffMask);
}

static void polynomial3DFree(psPolynomial3D* poly)
{
    unsigned int x = 0;
    unsigned int y = 0;

    for (x = 0; x < (1 + poly->nX); x++) {
        for (y = 0; y < (1 + poly->nY); y++) {
            psFree(poly->coeff[x][y]);
            psFree(poly->coeffErr[x][y]);
            psFree(poly->coeffMask[x][y]);
        }
        psFree(poly->coeff[x]);
        psFree(poly->coeffErr[x]);
        psFree(poly->coeffMask[x]);
    }

    psFree(poly->coeff);
    psFree(poly->coeffErr);
    psFree(poly->coeffMask);
}

static void polynomial4DFree(psPolynomial4D* poly)
{
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int z = 0;

    for (x = 0; x < (1 + poly->nX); x++) {
        for (y = 0; y < (1 + poly->nY); y++) {
            for (z = 0; z < (1 + poly->nZ); z++) {
                psFree(poly->coeff[x][y][z]);
                psFree(poly->coeffErr[x][y][z]);
                psFree(poly->coeffMask[x][y][z]);
            }
            psFree(poly->coeff[x][y]);
            psFree(poly->coeffErr[x][y]);
            psFree(poly->coeffMask[x][y]);
        }
        psFree(poly->coeff[x]);
        psFree(poly->coeffErr[x]);
        psFree(poly->coeffMask[x]);
    }

    psFree(poly->coeff);
    psFree(poly->coeffErr);
    psFree(poly->coeffMask);
}

/*****************************************************************************
p_psCreateChebyshevPolys(n): this routine takes as input the required order n,
and returns as output as a pointer to an array of n psPolynomial1D
structures, corresponding to the first n Chebyshev polynomials.
 
XXX: The output should be static since the Chebyshev polynomials might be
used frequently and the data structure created here does not contain the
outer coefficients of the Chebyshev polynomials.
 *****************************************************************************/
psPolynomial1D **p_psCreateChebyshevPolys(psS32 numPolys)
{
    PS_ASSERT_INT_LARGER_THAN_OR_EQUAL(numPolys, 1, NULL);

    psPolynomial1D **chebPolys = (psPolynomial1D **) psAlloc(numPolys * sizeof(psPolynomial1D *));
    for (psS32 i = 0; i < numPolys; i++) {
        chebPolys[i] = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, i);
    }

    // Create the Chebyshev polynomials.
    // Polynomial i has i-th order.
    chebPolys[0]->coeff[0] = 1.0;
    if (numPolys >= 2) {
        chebPolys[1]->coeff[1] = 1.0;

        for (psS32 i = 2; i < numPolys; i++) {
            for (psS32 j = 0; j < chebPolys[i - 1]->nX+1; j++) {
                chebPolys[i]->coeff[j + 1] = 2.0 * chebPolys[i - 1]->coeff[j];
            }
            for (psS32 j = 0; j < chebPolys[i - 2]->nX+1; j++) {
                chebPolys[i]->coeff[j] -= chebPolys[i - 2]->coeff[j];
            }
        }
    }

    if (psTraceGetLevel("psLib.math") >= 6) {
        for (psS32 j = 0; j < numPolys; j++) {
            PS_POLY_PRINT_1D(chebPolys[j]);
        }
    }
    return (chebPolys);
}


/** This function calculates the appropriate scaling factors needed to normalize the
 * input vector to the range -1 : +1.  These are stored on the polynomial in the given
 * direction.
 */
bool psChebyshevSetScale (psPolynomial2D* myPoly, const psVector *vec, int dir) {

    psAssert ((dir == 0) || (dir == 1), "invalid direction %d\n", dir);

    // find the min and max of the vector
    psF64 minValue = NAN;
    psF64 maxValue = NAN;

    for (int i = 0; i < vec->n; i++) {
	if (isnan(vec->data.F64[i])) continue;
	if (isnan(minValue)) { minValue = vec->data.F64[i]; }
	if (isnan(maxValue)) { maxValue = vec->data.F64[i]; }
	minValue = PS_MIN(minValue, vec->data.F64[i]);
	maxValue = PS_MAX(maxValue, vec->data.F64[i]);
    }
    if (minValue == maxValue) {
	psWarning ("insufficient data range to determine scale factors\n");
	return false;
    }

    myPoly->scale[dir] = 2.0 / (maxValue - minValue);
    myPoly->zero[dir]  = 1 - myPoly->scale[dir] * maxValue;
    return true;
}

/** This function generates a normalized vector in the range -1 : +1 based on the input
    vector using the scale factors stored in myPoly in the given direction.
 */
psVector *psChebyshevNormVector (const psPolynomial2D* myPoly, const psVector *vec, int dir) {

    psVector *norm = psVectorAlloc (vec->n, PS_TYPE_F64);

    if (vec->type.type == PS_TYPE_F64) {
	for (int i = 0; i < vec->n; i++) {
	    norm->data.F64[i] = vec->data.F64[i]*myPoly->scale[dir] + myPoly->zero[dir];
	}
	return norm;
    }
    if (vec->type.type == PS_TYPE_F32) {
	for (int i = 0; i < vec->n; i++) {
	    norm->data.F64[i] = vec->data.F32[i]*myPoly->scale[dir] + myPoly->zero[dir];
	}
	return norm;
    }

    psError(PS_ERR_UNKNOWN, true, "invalid type for chebyshev polynomial");
    return NULL;
}

# define CHEB_EVAL_0(OUT,IN) {OUT = 1.0;}
# define CHEB_EVAL_1(OUT,IN) {                       OUT = IN; }
# define CHEB_EVAL_2(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = 2.0*X2 - 1.0; }
# define CHEB_EVAL_3(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = IN*(4.0*X2 - 3.0); }
# define CHEB_EVAL_4(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = X2*(8.0*X2 - 8.0) + 1.0; }
# define CHEB_EVAL_5(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = IN *(X2*(16.0*X2 - 20.0) + 5.0); }
# define CHEB_EVAL_6(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = X2*(X2*(32.0*X2 - 48.0) + 18.0) - 1.0; }
# define CHEB_EVAL_7(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = IN *(X2*(X2*(64.0*X2 - 112.0) + 56.0) - 7.0); }
# define CHEB_EVAL_8(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = X2*(X2*(X2*(128.0*X2 - 256.0) + 160.0) - 32.0) + 1.0; }
# define CHEB_EVAL_9(OUT,IN) {psF64 X2 = PS_SQR(IN); OUT = IN *(X2*(X2*(X2*(256.0*X2 - 576.0) + 432.0) - 129.0) + 9.0); }

/** This function generates a vector containing the values of a Chebyshev polynomial of
    the given order evaluated at the coordinates given by the input vector, i.e., this
    function returns the vector T^n (x_i) where x_i is the input vector of values and n is
    the polynomial order.
 */
psVector *psChebyshevPolyVector (const psVector *vec, int order) {

    if (order > 9) {
	psWarning ("Chebyshev orders higher than 9 are not yet coded\n");
	return NULL;
    }

    psVector *out = psVectorAlloc (vec->n, PS_TYPE_F64);

    // easy but non-general implementation
    switch (order) {
      case 0:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_0(out->data.F64[i], vec->data.F64[i]); } break;
      case 1:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_1(out->data.F64[i], vec->data.F64[i]); } break;
      case 2:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_2(out->data.F64[i], vec->data.F64[i]); } break;
      case 3:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_3(out->data.F64[i], vec->data.F64[i]); } break;
      case 4:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_4(out->data.F64[i], vec->data.F64[i]); } break;
      case 5:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_5(out->data.F64[i], vec->data.F64[i]); } break;
      case 6:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_6(out->data.F64[i], vec->data.F64[i]); } break;
      case 7:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_7(out->data.F64[i], vec->data.F64[i]); } break;
      case 8:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_8(out->data.F64[i], vec->data.F64[i]); } break;
      case 9:
	for (int i = 0; i < vec->n; i++) { CHEB_EVAL_9(out->data.F64[i], vec->data.F64[i]); } break;
      default:
	psWarning ("Chebyshev orders higher than 9 are not yet coded\n");
	psFree (out);
	return NULL;
    }

    return out;
}

/*****************************************************************************
    Polynomial coefficients will be accessed in [w][x][y][z] fashion.
 *****************************************************************************/
static psF64 ordPolynomial1DEval(
    psF64 x,
    const psPolynomial1D* poly)
{
    unsigned int loop_x = 0;
    psF64 polySum = 0.0;
    psF64 xSum = 1.0;

    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    psTrace("psLib.math", 4, "Polynomial order is %u\n", poly->nX);
    for (loop_x = 0; loop_x < poly->nX+1; loop_x++) {
        psTrace("psLib.math", 4, "Polynomial coeff[%u] is %lf\n", loop_x, poly->coeff[loop_x]);
    }

    for (loop_x = 0; loop_x < poly->nX+1; loop_x++) {
        if (!(poly->coeffMask[loop_x] & PS_POLY_MASK_SET)) {
            psTrace("psLib.math", 8,
                    "polysum+= sum*coeff [%lf+= (%lf * %lf)\n", polySum, xSum, poly->coeff[loop_x]);
            polySum += xSum * poly->coeff[loop_x];
        }
        xSum *= x;
    }

    psTrace("psLib.math", 4, "---- %s() end ----\n", __func__);
    return(polySum);
}

static psF64 chebPolynomial1DEval(psF64 x, const psPolynomial1D* poly) {

    PS_ASSERT_INT_LARGER_THAN_OR_EQUAL(poly->nX, 0, NAN);

    psF64 xNorm = x*poly->scale[0] + poly->zero[0];

    psF64 polySum = 0.0;

    for (int ix = 0; ix <= poly->nX; ix++) {
        if (poly->coeffMask[ix] & PS_POLY_MASK_SET) continue;
	psF64 xCheb = NAN;
	switch (ix) {
	  case 0: CHEB_EVAL_0 (xCheb, xNorm); break;
	  case 1: CHEB_EVAL_1 (xCheb, xNorm); break;
	  case 2: CHEB_EVAL_2 (xCheb, xNorm); break;
	  case 3: CHEB_EVAL_3 (xCheb, xNorm); break;
	  case 4: CHEB_EVAL_4 (xCheb, xNorm); break;
	  case 5: CHEB_EVAL_5 (xCheb, xNorm); break;
	  case 6: CHEB_EVAL_6 (xCheb, xNorm); break;
	  case 7: CHEB_EVAL_7 (xCheb, xNorm); break;
	  case 8: CHEB_EVAL_8 (xCheb, xNorm); break;
	  case 9: CHEB_EVAL_9 (xCheb, xNorm); break;
	  default:
	    break;
	}
	polySum += poly->coeff[ix] * xCheb;
    }
    return polySum;
}

/*** version 1 is a general case and could be used for Norder > 9.  ***/
# ifdef CHEB_VERSION_1
void oldcode_1(void) {
    psVector *d;
    psF64 tmp = 0.0;

    unsigned int nTerms = 1 + poly->nX;
    unsigned int i;

    // Special case where the Chebyshev poly is constant.
    if (nTerms == 1) {
        if (!(poly->coeffMask[0] & PS_POLY_MASK_SET)) {
            tmp += poly->coeff[0];
        }
        return(tmp);
    }

    // Special case where the Chebyshev poly is linear.
    if (nTerms == 2) {
        if (!(poly->coeffMask[0] & PS_POLY_MASK_SET)) {
            tmp+= poly->coeff[0];
        }
        if (!(poly->coeffMask[1] & PS_POLY_MASK_SET)) {
            tmp+= poly->coeff[1] * x;
        }
        return(tmp);
    }

    // General case where the Chebyshev poly has 2 or more terms.
    d = psVectorAlloc(nTerms, PS_TYPE_F64);
    if (!(poly->coeffMask[nTerms-1] & PS_POLY_MASK_SET)) {
	d->data.F64[nTerms-1] = poly->coeff[nTerms-1];
    } else {
	d->data.F64[nTerms-1] = 0.0;
    }

    d->data.F64[nTerms-2] = (2.0 * x * d->data.F64[nTerms-1]);
    if (!(poly->coeffMask[nTerms-2] & PS_POLY_MASK_SET)) {
	d->data.F64[nTerms-2] += poly->coeff[nTerms-2];
    }

    for (i=nTerms-3;i>=1;i--) {
	d->data.F64[i] = (2.0 * x * d->data.F64[i+1]) - (d->data.F64[i+2]);
	if (!(poly->coeffMask[i] & PS_POLY_MASK_SET)) {
	    d->data.F64[i] += poly->coeff[i];
	}
    }

    tmp = (x * d->data.F64[1]) - (d->data.F64[2]);
    if (!(poly->coeffMask[0] & PS_POLY_MASK_SET)) {
	tmp += (0.5 * poly->coeff[0]);
    }
    psFree(d);
}
# endif

/*** version 0 should be removed when version 2 is ready ***/
# ifdef CHEB_VERSION_0
void oldcode_0(void) {
    // XXX: This is old code that does not use Clenshaw's formula.  Get rid of it.
    psPolynomial1D **chebPolys = p_psCreateChebyshevPolys(1 + poly->nX);

    tmp = 0.0;
    for (psS32 i=0;i<(1 + poly->nX);i++) {
	tmp+= (poly->coeff[i] * psPolynomial1DEval(chebPolys[i], x));
    }
    tmp-= (poly->coeff[0]/2.0);

    for (psS32 i=0;i<(1 + poly->nX);i++) {
	psFree(chebPolys[i]);
    }
    psFree(chebPolys);

    return(tmp);
}
# endif

static psF64 ordPolynomial2DEval(psF64 x,
                                 psF64 y,
                                 const psPolynomial2D* poly)
{
    PS_ASSERT_POLY_NON_NULL(poly, NAN);

    unsigned int loop_x = 0;
    unsigned int loop_y = 0;
    psF64 polySum = 0.0;
    psF64 xSum = 1.0;
    psF64 ySum = 1.0;

    for (loop_x = 0; loop_x < (1 + poly->nX); loop_x++) {
        ySum = xSum;
        for (loop_y = 0; loop_y < (1 + poly->nY); loop_y++) {
            if (!(poly->coeffMask[loop_x][loop_y] & PS_POLY_MASK_SET)) {
                polySum += ySum * poly->coeff[loop_x][loop_y];
            }
            ySum *= y;
        }
        xSum *= x;
    }

    return(polySum);
}

static psF64 chebPolynomial2DEval(psF64 x,
                                  psF64 y,
                                  const psPolynomial2D* poly)
{
    PS_ASSERT_POLY_NON_NULL(poly, NAN);

    psF64 xNorm = x*poly->scale[0] + poly->zero[0];
    psF64 yNorm = y*poly->scale[1] + poly->zero[1];

    psF64 polySum = 0.0;

    // XXX this could be quicker if we saved the N xvalues are re-used the resuls
    for (int ix = 0; ix <= poly->nX; ix++) {
	psF64 xCheb = NAN;
	switch (ix) {
	  case 0: CHEB_EVAL_0 (xCheb, xNorm); break;
	  case 1: CHEB_EVAL_1 (xCheb, xNorm); break;
	  case 2: CHEB_EVAL_2 (xCheb, xNorm); break;
	  case 3: CHEB_EVAL_3 (xCheb, xNorm); break;
	  case 4: CHEB_EVAL_4 (xCheb, xNorm); break;
	  case 5: CHEB_EVAL_5 (xCheb, xNorm); break;
	  case 6: CHEB_EVAL_6 (xCheb, xNorm); break;
	  case 7: CHEB_EVAL_7 (xCheb, xNorm); break;
	  case 8: CHEB_EVAL_8 (xCheb, xNorm); break;
	  case 9: CHEB_EVAL_9 (xCheb, xNorm); break;
	  default:
	    break;
	}
        for (int iy = 0; iy <= poly->nY; iy++) {
	    if (poly->coeffMask[ix][iy] & PS_POLY_MASK_SET) continue;
	    psF64 yCheb = NAN;
	    switch (iy) {
	      case 0: CHEB_EVAL_0 (yCheb, yNorm); break;
	      case 1: CHEB_EVAL_1 (yCheb, yNorm); break;
	      case 2: CHEB_EVAL_2 (yCheb, yNorm); break;
	      case 3: CHEB_EVAL_3 (yCheb, yNorm); break;
	      case 4: CHEB_EVAL_4 (yCheb, yNorm); break;
	      case 5: CHEB_EVAL_5 (yCheb, yNorm); break;
	      case 6: CHEB_EVAL_6 (yCheb, yNorm); break;
	      case 7: CHEB_EVAL_7 (yCheb, yNorm); break;
	      case 8: CHEB_EVAL_8 (yCheb, yNorm); break;
	      case 9: CHEB_EVAL_9 (yCheb, yNorm); break;
	      default:
		break;
	    }
	    polySum += poly->coeff[ix][iy] * xCheb * yCheb;
        }
    }
    return(polySum);
}

static psF64 ordPolynomial3DEval(psF64 x,
                                 psF64 y,
                                 psF64 z,
                                 const psPolynomial3D* poly)
{
    unsigned int loop_x = 0;
    unsigned int loop_y = 0;
    unsigned int loop_z = 0;
    psF64 polySum = 0.0;
    psF64 xSum = 1.0;
    psF64 ySum = 1.0;
    psF64 zSum = 1.0;

    for (loop_x = 0; loop_x < (1 + poly->nX); loop_x++) {
        ySum = xSum;
        for (loop_y = 0; loop_y < (1 + poly->nY); loop_y++) {
            zSum = ySum;
            for (loop_z = 0; loop_z < (1 + poly->nZ); loop_z++) {
                if (!(poly->coeffMask[loop_x][loop_y][loop_z] & PS_POLY_MASK_SET)) {
                    polySum += zSum * poly->coeff[loop_x][loop_y][loop_z];
                }
                zSum *= z;
            }
            ySum *= y;
        }
        xSum *= x;
    }

    return(polySum);
}

static psF64 chebPolynomial3DEval(psF64 x,
                                  psF64 y,
                                  psF64 z,
                                  const psPolynomial3D* poly)
{
    PS_ASSERT_DOUBLE_WITHIN_RANGE(x, -1.0, 1.0, 0.0);
    PS_ASSERT_DOUBLE_WITHIN_RANGE(y, -1.0, 1.0, 0.0);
    PS_ASSERT_DOUBLE_WITHIN_RANGE(z, -1.0, 1.0, 0.0);
    unsigned int loop_x = 0;
    unsigned int loop_y = 0;
    unsigned int loop_z = 0;
    unsigned int i = 0;
    psF64 polySum = 0.0;
    psPolynomial1D* *chebPolys = NULL;
    unsigned int maxChebyPoly = 0;

    // Determine how many Chebyshev polynomials
    // are needed, then create them.
    maxChebyPoly = poly->nX;
    if (poly->nY > maxChebyPoly) {
        maxChebyPoly = poly->nY;
    }
    if (poly->nZ > maxChebyPoly) {
        maxChebyPoly = poly->nZ;
    }
    chebPolys = p_psCreateChebyshevPolys(maxChebyPoly + 1);

    for (loop_x = 0; loop_x < (1 + poly->nX); loop_x++) {
        for (loop_y = 0; loop_y < (1 + poly->nY); loop_y++) {
            for (loop_z = 0; loop_z < (1 + poly->nZ); loop_z++) {
                if (!(poly->coeffMask[loop_x][loop_y][loop_z] & PS_POLY_MASK_SET)) {
                    polySum += poly->coeff[loop_x][loop_y][loop_z] *
                               psPolynomial1DEval(chebPolys[loop_x], x) *
                               psPolynomial1DEval(chebPolys[loop_y], y) *
                               psPolynomial1DEval(chebPolys[loop_z], z);
                }
            }
        }
    }

    for (i=0;i<maxChebyPoly+1;i++) {
        psFree(chebPolys[i]);
    }
    psFree(chebPolys);
    return(polySum);
}

static psF64 ordPolynomial4DEval(psF64 x,
                                 psF64 y,
                                 psF64 z,
                                 psF64 t,
                                 const psPolynomial4D* poly)
{
    unsigned int loop_x = 0;
    unsigned int loop_y = 0;
    unsigned int loop_z = 0;
    unsigned int loop_t = 0;
    psF64 polySum = 0.0;
    psF64 xSum = 1.0;
    psF64 ySum = 1.0;
    psF64 zSum = 1.0;
    psF64 tSum = 1.0;

    for (loop_x = 0; loop_x < (1 + poly->nX); loop_x++) {
        ySum = xSum;
        for (loop_y = 0; loop_y < (1 + poly->nY); loop_y++) {
            zSum = ySum;
            for (loop_z = 0; loop_z < (1 + poly->nZ); loop_z++) {
                tSum = zSum;
                for (loop_t = 0; loop_t < (1 + poly->nT); loop_t++) {
                    if (!(poly->coeffMask[loop_x][loop_y][loop_z][loop_t] & PS_POLY_MASK_SET)) {
                        polySum += tSum * poly->coeff[loop_x][loop_y][loop_z][loop_t];
                    }
                    tSum *= t;
                }
                zSum *= z;
            }
            ySum *= y;
        }
        xSum *= x;
    }

    return(polySum);
}

static psF64 chebPolynomial4DEval(psF64 x,
                                  psF64 y,
                                  psF64 z,
                                  psF64 t,
                                  const psPolynomial4D* poly)
{
    PS_ASSERT_DOUBLE_WITHIN_RANGE(x, -1.0, 1.0, 0.0);
    PS_ASSERT_DOUBLE_WITHIN_RANGE(y, -1.0, 1.0, 0.0);
    PS_ASSERT_DOUBLE_WITHIN_RANGE(z, -1.0, 1.0, 0.0);
    PS_ASSERT_DOUBLE_WITHIN_RANGE(t, -1.0, 1.0, 0.0);
    unsigned int loop_x = 0;
    unsigned int loop_y = 0;
    unsigned int loop_z = 0;
    unsigned int loop_t = 0;
    unsigned int i = 0;
    psF64 polySum = 0.0;
    psPolynomial1D* *chebPolys = NULL;
    unsigned int maxChebyPoly = 0;

    // Determine how many Chebyshev polynomials
    // are needed, then create them.
    maxChebyPoly = poly->nX;
    if (poly->nY > maxChebyPoly) {
        maxChebyPoly = poly->nY;
    }
    if (poly->nZ > maxChebyPoly) {
        maxChebyPoly = poly->nZ;
    }
    if (poly->nT > maxChebyPoly) {
        maxChebyPoly = poly->nT;
    }
    // Add 1 since p_psCreateChebyshevPolys() takes nTerms, not nOrder.
    chebPolys = p_psCreateChebyshevPolys(maxChebyPoly + 1);

    for (loop_x = 0; loop_x < (1 + poly->nX); loop_x++) {
        for (loop_y = 0; loop_y < (1 + poly->nY); loop_y++) {
            for (loop_z = 0; loop_z < (1 + poly->nZ); loop_z++) {
                for (loop_t = 0; loop_t < (1 + poly->nT); loop_t++) {
                    if (!(poly->coeffMask[loop_x][loop_y][loop_z][loop_t] & PS_POLY_MASK_SET)) {
                        polySum += poly->coeff[loop_x][loop_y][loop_z][loop_t] *
                                   psPolynomial1DEval(chebPolys[loop_x], x) *
                                   psPolynomial1DEval(chebPolys[loop_y], y) *
                                   psPolynomial1DEval(chebPolys[loop_z], z) *
                                   psPolynomial1DEval(chebPolys[loop_t], t);
                    }
                }
            }
        }
    }

    for (i=0;i<maxChebyPoly+1;i++) {
        psFree(chebPolys[i]);
    }
    psFree(chebPolys);
    return(polySum);
}

/*****************************************************************************/
/*  FUNCTION IMPLEMENTATION - PUBLIC                                         */
/*****************************************************************************/

/*****************************************************************************
    Evaluate a non-normalized Gaussian with the given mean and sigma at the
    given coordianate.  Note that this is not a Gaussian deviate.  The
    evaluated Gaussian is: \f[ exp(-\frac{(x-mean)^2}{2\sigma^2}) \f]
 *****************************************************************************/
float psGaussian(float x,
                 float mean,
                 float sigma,
                 bool normal)
{
    psF32 tmp = 1.0;

    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);

    if (normal == true) {
        tmp = 1.0 / sqrtf(2.0 * M_PI * (sigma * sigma));
    }

    psTrace("psLib.math", 4, "---- %s() end ----\n", __func__);
    return(tmp * exp(-((x - mean) * (x - mean)) / (2.0 * sigma * sigma)));
}

/*****************************************************************************
    This routine must allocate memory for the polynomial structures.
 
    XXX: How do we check for an appropriate value for n?
 *****************************************************************************/
psPolynomial1D* psPolynomial1DAlloc(
    psPolynomialType type,
    unsigned int nX)
{
    PS_ASSERT_POLY_VALID_TYPE(type, NULL);

    psU32 nOrder = nX;
    psPolynomial1D *newPoly = (psPolynomial1D* ) psAlloc(sizeof(psPolynomial1D));
    psMemSetDeallocator(newPoly, (psFreeFunc) polynomial1DFree);

    newPoly->type = type;
    newPoly->nX = nOrder;
    newPoly->coeff = psAlloc((nOrder + 1) * sizeof(psF64));
    newPoly->coeffErr = psAlloc((nOrder + 1) * sizeof(psF64));
    newPoly->coeffMask = (psMaskType *)psAlloc((nOrder + 1) * sizeof(psMaskType));
    for (psU32 i = 0; i < (nOrder + 1); i++) {
        newPoly->coeff[i] = 0.0;
        newPoly->coeffErr[i] = 0.0;
        newPoly->coeffMask[i] = PS_POLY_MASK_NONE;
    }

    // scale & zero are used for Chebyshev polynomials to define the relationship between
    // the independent variables and the normalized version with range -1 : +1.  These
    // must be determined for a specific data set.
    newPoly->scale[0] = NAN;
    newPoly->zero[0]  = NAN;

    return(newPoly);
}

psPolynomial2D* psPolynomial2DAlloc(
    psPolynomialType type,
    unsigned int nX,
    unsigned int nY)
{
    PS_ASSERT_POLY_VALID_TYPE(type, NULL);

    unsigned int x = 0;
    unsigned int y = 0;
    psPolynomial2D* newPoly = NULL;

    newPoly = (psPolynomial2D* ) psAlloc(sizeof(psPolynomial2D));
    psMemSetDeallocator(newPoly, (psFreeFunc) polynomial2DFree);

    newPoly->type = type;
    newPoly->nX = nX;
    newPoly->nY = nY;

    newPoly->coeff = psAlloc((1 + nX) * sizeof(psF64 *));
    newPoly->coeffErr = psAlloc((1 + nX) * sizeof(psF64 *));
    newPoly->coeffMask = (psMaskType **)psAlloc((1 + nX) * sizeof(psMaskType *));
    for (x = 0; x < (1 + nX); x++) {
        newPoly->coeff[x] = psAlloc((1 + nY) * sizeof(psF64));
        newPoly->coeffErr[x] = psAlloc((1 + nY) * sizeof(psF64));
        newPoly->coeffMask[x] = (psMaskType *)psAlloc((1 + nY) * sizeof(psMaskType));
    }
    for (x = 0; x < (1 + nX); x++) {
        for (y = 0; y < (1 + nY); y++) {
            newPoly->coeff[x][y] = 0.0;
            newPoly->coeffErr[x][y] = 0.0;
            newPoly->coeffMask[x][y] = PS_POLY_MASK_NONE;
        }
    }

    // scale & zero are used for Chebyshev polynomials to define the relationship between
    // the independent variables and the normalized version with range -1 : +1.  These
    // must be determined for a specific data set.
    for (int i = 0; i < 2; i++) {
      newPoly->scale[i] = NAN;
      newPoly->zero[i]  = NAN;
    }

    return(newPoly);
}

// XXX add 1D, 3D, 4D versions
bool psPolynomial2DRecycle(psPolynomial2D *poly,
                           psPolynomialType type,
                           unsigned int nX,
                           unsigned int nY)
{
    PS_ASSERT_INT_NONNEGATIVE(nX, NULL);
    PS_ASSERT_INT_NONNEGATIVE(nY, NULL);

    bool match = true;
    match &= (poly->type == type);
    match &= (poly->nX == type);
    match &= (poly->nY == type);

    if (!match) {
        for (int i = 0; i < poly->nX + 1; i++) {
            psFree (poly->coeff[i]);
            psFree (poly->coeffErr[i]);
            psFree (poly->coeffMask[i]);
        }
        psFree (poly->coeff);
        psFree (poly->coeffErr);
        psFree (poly->coeffMask);

        poly->type = type;
        poly->nX = nX;
        poly->nY = nY;

        poly->coeff = psAlloc((1 + nX) * sizeof(psF64 *));
        poly->coeffErr = psAlloc((1 + nX) * sizeof(psF64 *));
        poly->coeffMask = (psMaskType **)psAlloc((1 + nX) * sizeof(psMaskType *));
        for (int i = 0; i < (1 + nX); i++) {
            poly->coeff[i] = psAlloc((1 + nY) * sizeof(psF64));
            poly->coeffErr[i] = psAlloc((1 + nY) * sizeof(psF64));
            poly->coeffMask[i] = (psMaskType *)psAlloc((1 + nY) * sizeof(psMaskType));
        }
    }
    for (int i = 0; i < (1 + nX); i++) {
        for (int j = 0; j < (1 + nY); j++) {
            poly->coeff[i][j] = 0.0;
            poly->coeffErr[i][j] = 0.0;
            poly->coeffMask[i][j] = PS_POLY_MASK_NONE;
        }
    }
    return(true);
}

// XXX add 1D, 3D, 4D versions
psPolynomial2D *psPolynomial2DCopy(psPolynomial2D *out,
                                   psPolynomial2D *poly)
{
    if (out == NULL) {
        out = psPolynomial2DAlloc (poly->type, poly->nX, poly->nY);
    } else {
        psPolynomial2DRecycle (out, poly->type, poly->nX, poly->nY);
    }

    for (int i = 0; i < (1 + poly->nX); i++) {
        for (int j = 0; j < (1 + poly->nY); j++) {
            out->coeff[i][j] = poly->coeff[i][j];
            out->coeffErr[i][j] = poly->coeffErr[i][j];
            out->coeffMask[i][j] = poly->coeffMask[i][j];
        }
    }
    return(out);
}

psPolynomial3D* psPolynomial3DAlloc(
    psPolynomialType type,
    unsigned int nX,
    unsigned int nY,
    unsigned int nZ)
{
    PS_ASSERT_POLY_VALID_TYPE(type, NULL);

    //PS_ASSERT_INT_NONNEGATIVE(nX, NULL);
    //PS_ASSERT_INT_NONNEGATIVE(nY, NULL);
    //PS_ASSERT_INT_NONNEGATIVE(nZ, NULL);

    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int z = 0;
    psPolynomial3D* newPoly = NULL;

    newPoly = (psPolynomial3D* ) psAlloc(sizeof(psPolynomial3D));
    psMemSetDeallocator(newPoly, (psFreeFunc) polynomial3DFree);

    newPoly->type = type;
    newPoly->nX = nX;
    newPoly->nY = nY;
    newPoly->nZ = nZ;

    newPoly->coeff = psAlloc((nX + 1) * sizeof(psF64 **));
    newPoly->coeffErr = psAlloc((nX + 1) * sizeof(psF64 **));
    newPoly->coeffMask = (psMaskType ***)psAlloc((nX + 1) * sizeof(psMaskType **));
    for (x = 0; x < (1 + nX); x++) {
        newPoly->coeff[x] = psAlloc((nY + 1) * sizeof(psF64 *));
        newPoly->coeffErr[x] = psAlloc((nY + 1) * sizeof(psF64 *));
        newPoly->coeffMask[x] = (psMaskType **)psAlloc((nY + 1) * sizeof(psMaskType *));
        for (y = 0; y < (nY + 1); y++) {
            newPoly->coeff[x][y] = psAlloc((nZ + 1) * sizeof(psF64));
            newPoly->coeffErr[x][y] = psAlloc((nZ + 1) * sizeof(psF64));
            newPoly->coeffMask[x][y] = (psMaskType *)psAlloc((nZ + 1) * sizeof(psMaskType));
        }
    }
    for (x = 0; x < (nX + 1); x++) {
        for (y = 0; y < (nY + 1); y++) {
            for (z = 0; z < (nZ + 1); z++) {
                newPoly->coeff[x][y][z] = 0.0;
                newPoly->coeffErr[x][y][z] = 0.0;
                newPoly->coeffMask[x][y][z] = PS_POLY_MASK_NONE;
            }
        }
    }

    // scale & zero are used for Chebyshev polynomials to define the relationship between
    // the independent variables and the normalized version with range -1 : +1.  These
    // must be determined for a specific data set.
    for (int i = 0; i < 3; i++) {
      newPoly->scale[i] = NAN;
      newPoly->zero[i]  = NAN;
    }

    return(newPoly);
}

psPolynomial4D* psPolynomial4DAlloc(
    psPolynomialType type,
    unsigned int nX,
    unsigned int nY,
    unsigned int nZ,
    unsigned int nT)
{
    PS_ASSERT_POLY_VALID_TYPE(type, NULL);

    //PS_ASSERT_INT_NONNEGATIVE(nX, NULL);
    //PS_ASSERT_INT_NONNEGATIVE(nY, NULL);
    //PS_ASSERT_INT_NONNEGATIVE(nZ, NULL);
    //PS_ASSERT_INT_NONNEGATIVE(nT, NULL);

    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int z = 0;
    unsigned int t = 0;
    psPolynomial4D* newPoly = NULL;

    newPoly = (psPolynomial4D* ) psAlloc(sizeof(psPolynomial4D));
    psMemSetDeallocator(newPoly, (psFreeFunc) polynomial4DFree);

    newPoly->type = type;
    newPoly->nX = nX;
    newPoly->nY = nY;
    newPoly->nZ = nZ;
    newPoly->nT = nT;

    newPoly->coeff = psAlloc((nX + 1) * sizeof(psF64 ***));
    newPoly->coeffErr = psAlloc((nX + 1) * sizeof(psF64 ***));
    newPoly->coeffMask = (psMaskType ****)psAlloc((nX + 1) * sizeof(psMaskType ***));
    for (x = 0; x < (nX + 1); x++) {
        newPoly->coeff[x] = psAlloc((nY + 1) * sizeof(psF64 **));
        newPoly->coeffErr[x] = psAlloc((nY + 1) * sizeof(psF64 **));
        newPoly->coeffMask[x] = (psMaskType ***)psAlloc((nY + 1) * sizeof(psMaskType **));
        for (y = 0; y < (nY + 1); y++) {
            newPoly->coeff[x][y] = psAlloc((nZ + 1) * sizeof(psF64 *));
            newPoly->coeffErr[x][y] = psAlloc((nZ + 1) * sizeof(psF64 *));
            newPoly->coeffMask[x][y] = (psMaskType **)psAlloc((nZ + 1) * sizeof(psMaskType *));
            for (z = 0; z < (nZ + 1); z++) {
                newPoly->coeff[x][y][z] = psAlloc((nT + 1) * sizeof(psF64));
                newPoly->coeffErr[x][y][z] = psAlloc((nT + 1) * sizeof(psF64));
                newPoly->coeffMask[x][y][z] = (psMaskType *)psAlloc((nT + 1) * sizeof(psMaskType));
            }
        }
    }
    for (x = 0; x < (nX + 1); x++) {
        for (y = 0; y < (nY + 1); y++) {
            for (z = 0; z < (nZ + 1); z++) {
                for (t = 0; t < (nT + 1); t++) {
                    newPoly->coeff[x][y][z][t] = 0.0;
                    newPoly->coeffErr[x][y][z][t] = 0.0;
                    newPoly->coeffMask[x][y][z][t] = PS_POLY_MASK_NONE;
                }
            }
        }
    }

    // scale & zero are used for Chebyshev polynomials to define the relationship between
    // the independent variables and the normalized version with range -1 : +1.  These
    // must be determined for a specific data set.
    for (int i = 0; i < 4; i++) {
      newPoly->scale[i] = NAN;
      newPoly->zero[i]  = NAN;
    }

    return(newPoly);
}

/* note these functions accept unscaled values and apply the scaling saved on poly */
psF64 psPolynomial1DEval(const psPolynomial1D* poly,
                         psF64 x)
{
    PS_ASSERT_POLY_NON_NULL(poly, NAN);

    if (poly->type == PS_POLYNOMIAL_ORD) {
        return(ordPolynomial1DEval(x, poly));
    }
    if (poly->type == PS_POLYNOMIAL_CHEB) {
        return(chebPolynomial1DEval(x, poly));
    } 
    psError(PS_ERR_BAD_PARAMETER_TYPE, true,
	    _("Unknown polynomial type 0x%x found.  Evaluation failed."),
	    poly->type);

    return(NAN);
}

// this function must accept F32 and F64 input x vectors
// EAM XXX these functions seem inefficiently implemented with many nested function calls.
// they might benefit from unrolling.
psVector *psPolynomial1DEvalVector(const psPolynomial1D *poly,
                                   const psVector *x)
{
    PS_ASSERT_POLY_NON_NULL(poly, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);

    psVector *tmp;

    switch (x->type.type) {
    case PS_TYPE_F64:
        tmp = psVectorAlloc(x->n, PS_TYPE_F64);
        for (unsigned int i=0;i<x->n;i++) {
            tmp->data.F64[i] = psPolynomial1DEval(poly, x->data.F64[i]);
        }
        break;
    case PS_TYPE_F32:
        tmp = psVectorAlloc(x->n, PS_TYPE_F32);
        for (unsigned int i=0;i<x->n;i++) {
            tmp->data.F32[i] = psPolynomial1DEval(poly, x->data.F32[i]);
        }
        break;
    default:
        psError(PS_ERR_UNKNOWN, false, "invalid input data type.\n");
        return (NULL);
    }
    return(tmp);
}

psF64 psPolynomial2DEval(const psPolynomial2D* poly,
                         psF64 x,
                         psF64 y)
{
    PS_ASSERT_POLY_NON_NULL(poly, NAN);

    if (poly->type == PS_POLYNOMIAL_ORD) {
        return(ordPolynomial2DEval(x, y, poly));
    }
    if (poly->type == PS_POLYNOMIAL_CHEB) {
        return(chebPolynomial2DEval(x, y, poly));
    } 
    psError(PS_ERR_BAD_PARAMETER_TYPE, true,
	    _("Unknown polynomial type 0x%x found.  Evaluation failed."),
	    poly->type);
    return(NAN);
}

psVector *psPolynomial2DEvalChebVector(const psPolynomial2D *poly,
				       const psVector *x,
				       const psVector *y)
{

    if (!isfinite(poly->scale[0]) || !isfinite(poly->zero[0]) || !isfinite(poly->scale[1]) || !isfinite(poly->zero[1])) {
	// re-calculate if not already determined?  
	psError(PS_ERR_UNKNOWN, true, "normalization scales are not set for chebyshev polynomial");
	return (NULL);
    }

    // Number of polynomial terms
    int nXterm = 1 + poly->nX;      // Number of terms in x
    int nYterm = 1 + poly->nY;      // Number of terms in y
    if (nXterm > 9) {
	psError(PS_ERR_UNKNOWN, false, "failed 2D chebyshev fit: orders higher than 9 are not yet coded\n");
	return NULL;
    }
    if (nYterm > 9) {
	psError(PS_ERR_UNKNOWN, false, "failed 2D chebyshev fit: orders higher than 9 are not yet coded\n");
	return NULL;
    }

    // Generate normalized vectors for the range -1 : +1.  These functions cast to psF64
    psVector *xNorm = psChebyshevNormVector (poly, x, 0);
    psVector *yNorm = psChebyshevNormVector (poly, y, 1);
    
    // Generate the N cheb polynomials based on xNorm, yNorm
    psArray *xPolySet = psArrayAlloc (nXterm);
    for (int i = 0; i < nXterm; i++) {
	xPolySet->data[i] = psChebyshevPolyVector (xNorm, i);
    }
    psArray *yPolySet = psArrayAlloc (nYterm);
    for (int i = 0; i < nYterm; i++) {
	yPolySet->data[i] = psChebyshevPolyVector (yNorm, i);
    }

    psVector *out = psVectorAlloc (x->n, PS_TYPE_F64);

    psF64 *xData = xNorm->data.F64;
    psF64 *yData = yNorm->data.F64;
    psF64 *fData = out->data.F64;

    // loop over all elements of the data vector
    for (int i = 0; i < x->n; i++) {

	if (!finite(xData[i])) {fData[i] = NAN; continue; }
	if (!finite(yData[i])) {fData[i] = NAN; continue; }

	psF64 sum = 0.0;
	for (int jx = 0; jx < nXterm; jx++) {
	    psVector *jxCheb = xPolySet->data[jx];
	    for (int jy = 0; jy < nYterm; jy++) {
		psVector *jyCheb = yPolySet->data[jy];
		if (poly->coeffMask[jx][jy] & PS_POLY_MASK_SET) continue;
		sum += poly->coeff[jx][jy] * jxCheb->data.F64[i] * jyCheb->data.F64[i];
	    }
	}
	fData[i] = sum;
    }

    psFree (xPolySet);
    psFree (yPolySet);
    psFree (xNorm);
    psFree (yNorm);

    return out;
}

// this function must support input data types of F32 and F64
// all input vectors data types must match (all F32 or all F64)
psVector *psPolynomial2DEvalVector(const psPolynomial2D *poly,
                                   const psVector *x,
                                   const psVector *y)

{
    PS_ASSERT_POLY_NON_NULL(poly, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(y, NULL);

    unsigned int vecLen = x->n;

    // input vector types must match
    if (y->type.type != x->type.type) {
	psError(PS_ERR_UNKNOWN, true, "type mismatch in data vectors");
	return (NULL);
    }

    // Determine the length of the output vector to by the minimum of the x,y vectors
    // XXX shouldn't we require x & y to have the same length?  seems meaningless otherwise
    if (y->n != vecLen) {
	psError(PS_ERR_UNKNOWN, true, "length mismatch in data vectors");
	return (NULL);
    }

    if (poly->type == PS_POLYNOMIAL_CHEB) {
	psVector *out = psPolynomial2DEvalChebVector (poly, x, y);
	return out;
    }

    switch (x->type.type) {
      case PS_TYPE_F32: {
	  // Create output vector to return
	  psVector *out = psVectorAlloc(vecLen, PS_TYPE_F32);

	  // Evaluate the polynomial at the specified points
	  for (unsigned int i = 0; i < vecLen; i++) {
	      out->data.F32[i] = psPolynomial2DEval(poly,x->data.F32[i],y->data.F32[i]);
	  }
	  return out;
      }
      case PS_TYPE_F64: {
	  // Create output vector to return
	  psVector *out = psVectorAlloc(vecLen, PS_TYPE_F64);

	  // Evaluate the polynomial at the specified points
	  for (unsigned int i = 0; i < vecLen; i++) {
	      out->data.F64[i] = psPolynomial2DEval(poly,x->data.F64[i],y->data.F64[i]);
	  }
	  return out;
      }
      default:
        psError(PS_ERR_UNKNOWN, false, "invalid input data type.\n");
        return (NULL);
    }
    psAbort ("impossible");
    return NULL;
}

psF64 psPolynomial3DEval(const psPolynomial3D* poly,
                         psF64 x,
                         psF64 y,
                         psF64 z)
{
    PS_ASSERT_POLY_NON_NULL(poly, NAN);

    if (poly->type == PS_POLYNOMIAL_ORD) {
        return(ordPolynomial3DEval(x, y, z, poly));
    } else if (poly->type == PS_POLYNOMIAL_CHEB) {
        return(chebPolynomial3DEval(x, y, z, poly));
    } else {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Unknown polynomial type 0x%x found.  Evaluation failed."),
                poly->type);
    }
    return(NAN);
}

// XXX: The output of this routine is always psF64 while 1D and 2D are
// dependent on the input vectors.
psVector *psPolynomial3DEvalVector(
    const psPolynomial3D *poly,
    const psVector *x,
    const psVector *y,
    const psVector *z)
{
    PS_ASSERT_POLY_NON_NULL(poly, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(y, NULL);
    PS_ASSERT_VECTOR_NON_NULL(z, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(z, NULL);

    psVector *tmp;
    unsigned int vecLen=x->n;

    // Determine the length of output vector from min of the input vectors
    if (y->n < vecLen) {
        vecLen = y->n;
    }
    if (z->n < vecLen) {
        vecLen = z->n;
    }

    // Allocate output vector
    tmp = psVectorAlloc(vecLen, PS_TYPE_F64);

    // Evaluate polynomial
    // XXX: Consult with IfA: is this how they want to handle multiple data types?
    if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F32)
            && (z->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F32[i],
                                                  y->data.F32[i], z->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F32[i],
                                                  y->data.F32[i], z->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F32[i],
                                                  y->data.F64[i], z->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F32[i],
                                                  y->data.F64[i], z->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F64[i],
                                                  y->data.F32[i], z->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F64[i],
                                                  y->data.F32[i], z->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F64[i],
                                                  y->data.F64[i], z->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial3DEval(poly, x->data.F64[i],
                                                  y->data.F64[i], z->data.F64[i]);
        }
    }


    // Return output vector
    return(tmp);
}

psF64 psPolynomial4DEval(
    const psPolynomial4D* poly,
    psF64 x,
    psF64 y,
    psF64 z,
    psF64 t)
{
    PS_ASSERT_POLY_NON_NULL(poly, NAN);

    if (poly->type == PS_POLYNOMIAL_ORD) {
        return(ordPolynomial4DEval(x,y,z,t, poly));
    } else if (poly->type == PS_POLYNOMIAL_CHEB) {
        return(chebPolynomial4DEval(x,y,z,t, poly));
    } else {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Unknown polynomial type 0x%x found.  Evaluation failed."),
                poly->type);
    }
    return(NAN);
}

psVector *psPolynomial4DEvalVector(
    const psPolynomial4D *poly,
    const psVector *x,
    const psVector *y,
    const psVector *z,
    const psVector *t)
{
    PS_ASSERT_POLY_NON_NULL(poly, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(y, NULL);
    PS_ASSERT_VECTOR_NON_NULL(z, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(z, NULL);
    PS_ASSERT_VECTOR_NON_NULL(t, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(t, NULL);

    psVector *tmp;
    unsigned int vecLen=x->n;

    // Determine output vector size from min of input vectors
    if (z->n < vecLen) {
        vecLen = z->n;
    }
    if (y->n < vecLen) {
        vecLen = y->n;
    }
    if (t->n < vecLen) {
        vecLen = t->n;
    }

    // Allocoutput vector
    tmp = psVectorAlloc(vecLen, PS_TYPE_F64);

    // Evaluate polynomial
    // XXX: Consult with IfA: is this how they want to handle multiple data types?
    if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F32)
            && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F32[i], z->data.F32[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F32[i], z->data.F32[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F32[i], z->data.F64[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F32[i], z->data.F64[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F64[i], z->data.F32[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F64[i], z->data.F32[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F64[i], z->data.F64[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F32) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F32[i],
                                                  y->data.F64[i], z->data.F64[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F32[i], z->data.F32[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F32[i], z->data.F32[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F32[i], z->data.F64[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F32)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F32[i], z->data.F64[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F64[i], z->data.F32[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F32) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F64[i], z->data.F32[i], t->data.F64[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F32)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F64[i], z->data.F64[i], t->data.F32[i]);
        }
    } else if ((x->type.type == PS_TYPE_F64) && (y->type.type == PS_TYPE_F64)
               && (z->type.type == PS_TYPE_F64) && (t->type.type == PS_TYPE_F64)) {
        for (unsigned int i = 0; i < vecLen; i++) {
            tmp->data.F64[i] = psPolynomial4DEval(poly, x->data.F64[i],
                                                  y->data.F64[i], z->data.F64[i],
                                                  t->data.F64[i]);
        }
    }


    // Evaluate polynomial
    for (unsigned int i = 0; i < vecLen; i++) {
        tmp->data.F64[i] = psPolynomial4DEval(poly,
                                              x->data.F64[i],
                                              y->data.F64[i],
                                              z->data.F64[i],
                                              t->data.F64[i]);
    }

    // Return output vector
    return(tmp);
}


/** @file  psMinimize.c
 *  \brief basic minimization functions
 *  @ingroup Math
 *
 *  This file will contain functions to minimize an arbitrary function at
 *  a data point, fit an arbitrary function to a set of data points, and
 *  fit a 1-D polynomial to a set of data points.
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.34 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 *  XXX: psMatrixLUSolution() does not return error codes when the results are NANs.
 *
 *  XXX: For clip-fit functions, what should we do if the mask is NULL?
 *
 *  XXX: the sums are built for 2*(order + 1) elements, but it should be 2*order + 1
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "psMinimizePolyFit.h"
#include "psAbort.h"
#include "psAssert.h"
#include "psMinimizeLMM.h"  // For Gauss-Jordan routines
#include "psStats.h"
#include "psImage.h"
#include "psImageStructManip.h"
#include "psBinaryOp.h"
#include "psLogMsg.h"
#include "psMathUtils.h"
#include "psBinomialCoeff.h"
/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/
#define CZW 0

# define USE_GAUSS_JORDAN 1
# define USE_ROBUST_STATS_FOR_CLIPPING 1

#define PS_VECTOR_GEN_CHEBY_INDEX(VEC, SIZE, TYPE) \
VEC = psVectorAlloc(SIZE, TYPE); \
if (TYPE == PS_TYPE_F64) { \
    for (psS32 i = 0 ; i < SIZE ; i++) { \
        VEC->data.F64[i] = ((2.0 / ((psF64) (SIZE - 1))) * ((psF64) i)) - 1.0; \
    }\
} else if (TYPE == PS_TYPE_F32){ \
    for (psS32 i = 0 ; i < SIZE ; i++) { \
        VEC->data.F32[i] = ((2.0 / ((psF32) (SIZE - 1))) * ((psF32) i)) - 1.0; \
    }\
}\

// free a local temporary F64 vector (TEMP) which is a copy of a non-F64 vector (ORIG)
# define PS_FREE_TEMP_F64_VECTOR(ORIG, TEMP) \
if ((ORIG != NULL) && (ORIG->type.type != PS_TYPE_F64)) { psFree(TEMP); }

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* GLOBAL VARIABLES                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* FILE STATIC VARIABLES                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - LOCAL                                           */
/*****************************************************************************/

/******************************************************************************
BuildSums1D(sums, x, polyOrder, sums): this routine calculates the powers of
input parameter "x" between 0 and input parameter nTerms*2.  The result is
returned as a psVector sums.
*****************************************************************************/
static psVector *BuildSums1D(
    psVector* sums,
    psF64 x,
    psS32 nTerm)
{
    psS32 nSum = 0;
    psF64 xSum = 0.0;

    //
    // XXX: Why do we multiply by 2 here?  It's better to do it outside and
    // have the definition of this function remain sensible.
    //
    nSum = 2*nTerm;
    if (sums == NULL) {
        sums = psVectorAlloc(nSum, PS_TYPE_F64);
    } else if (nSum > sums->n) {
        sums = psVectorRealloc(sums, nSum);
        sums->n = nSum;
    }

    xSum = 1.0;
    for (psS32 i = 0; i < nSum; i++) {
        sums->data.F64[i] = xSum;
        xSum *= x;
    }
    return (sums);
}

/******************************************************************************
BuildSums2D(sums, x, y, nXterm, nYterm): this routine calculates the powers of
input parameter "x" and "y" between 0 and input parameter nXterms*2 and
nYterm*2.  The result is returned as a psImage sums.
 *****************************************************************************/
static psImage *BuildSums2D(
    psImage *sums,
    psF64 x,
    psF64 y,
    psS32 nXterm,
    psS32 nYterm)
{
    psS32 nXsum = 0;
    psS32 nYsum = 0;
    psF64 xSum = 1.0;
    psF64 ySum = 1.0;

    // note that we are using the X and Y elements of the image reversed to be
    // consistent with the other BuildSumsND functions in terms of the definition
    // of the sums array
    nXsum = 2*nXterm;
    nYsum = 2*nYterm;
    if (sums == NULL) {
        sums = psImageAlloc(nYsum, nXsum, PS_TYPE_F64);
    }
    if ((nYsum != sums->numCols) || (nXsum != sums->numRows)) {
        psFree (sums);
        sums = psImageAlloc(nYsum, nXsum, PS_TYPE_F64);
    }

    xSum = 1.0;
    for (psS32 i = 0; i < nXsum; i++) {
        ySum = xSum;
        for (psS32 j = 0; j < nYsum; j++) {
            sums->data.F64[i][j] = ySum;
            ySum *= y;
        }
        xSum *= x;
    }

    return (sums);
}

/******************************************************************************
BuildSums3D(sums, x, y, z, nXterm, nYterm, nZterm): this routine calculates
the powers of input parameter "x", "y", and "z" between 0 and input parameter
nXterms*2, nYterm*2, and nZterm*2.  The result is returned as a 3-D array sums.
 *****************************************************************************/
static psF64 ***BuildSums3D(
    psF64 ***sums,
    psF64 x,
    psF64 y,
    psF64 z,
    psS32 nXterm,
    psS32 nYterm,
    psS32 nZterm)
{
    psS32 nXsum = 0;
    psS32 nYsum = 0;
    psS32 nZsum = 0;
    psF64 xSum = 1.0;
    psF64 ySum = 1.0;
    psF64 zSum = 1.0;

    nXsum = 2*nXterm;
    nYsum = 2*nYterm;
    nZsum = 2*nZterm;
    if (sums == NULL) {
        sums = (psF64 ***) psAlloc (nXsum*sizeof(psF64));
        for (psS32 i = 0; i < nXsum; i++) {
            sums[i] = (psF64 **) psAlloc (nYsum*sizeof(psF64));
            for (psS32 j = 0; j < nYsum; j++) {
                sums[i][j] = (psF64 *) psAlloc (nZsum*sizeof(psF64));
            }
        }
    }
    // careful with this function: there is no size checking and realloc for reuse

    if (1) {
        zSum = 1.0;
        for (psS32 k = 0; k < nZsum; k++) {
            ySum = zSum;
            for (psS32 j = 0; j < nYsum; j++) {
                xSum = ySum;
                for (psS32 i = 0; i < nXsum; i++) {
                    sums[i][j][k] = xSum;
                    xSum *= x;
                }
                ySum *= y;
            }
            zSum *= z;
        }
    } else {
        xSum = 1.0;
        for (psS32 i = 0; i < nXsum; i++) {
            ySum = xSum;
            for (psS32 j = 0; j < nYsum; j++) {
                zSum = ySum;
                for (psS32 k = 0; k < nZsum; k++) {
                    sums[i][j][k] = zSum;
                    zSum *= z;
                }
                ySum *= y;
            }
            xSum *= x;
        }
    }

    return (sums);
}

/******************************************************************************
    BuildSums4D(sums, x, y, z, t, nXterm, nYterm, nZterm, nTterm). equiv to
    BuildSums2D(). The result is returned as a psF64 ****
*****************************************************************************/
static psF64 ****BuildSums4D(
    psF64 ****sums,
    psF64 x,
    psF64 y,
    psF64 z,
    psF64 t,
    psS32 nXterm,
    psS32 nYterm,
    psS32 nZterm,
    psS32 nTterm)
{
    psS32 nXsum = 0;
    psS32 nYsum = 0;
    psS32 nZsum = 0;
    psS32 nTsum = 0;
    psF64 xSum = 1.0;
    psF64 ySum = 1.0;
    psF64 zSum = 1.0;
    psF64 tSum = 1.0;

    nXsum = 2*nXterm;
    nYsum = 2*nYterm;
    nZsum = 2*nZterm;
    nTsum = 2*nTterm;
    if (sums == NULL) {
        sums = (psF64 ****) psAlloc (nXsum*sizeof(psF64));
        for (psS32 i = 0; i < nXsum; i++) {
            sums[i] = (psF64 ***) psAlloc (nYsum*sizeof(psF64));
            for (psS32 j = 0; j < nYsum; j++) {
                sums[i][j] = (psF64 **) psAlloc (nZsum*sizeof(psF64));
                for (psS32 k = 0; k < nZsum; k++) {
                    sums[i][j][k] = (psF64 *) psAlloc (nTsum*sizeof(psF64));
                }
            }
        }
    }
    // careful with this function: there is no size checking and realloc for reuse

    tSum = 1.0;
    for (psS32 m = 0; m < nTsum; m++) {
        zSum = tSum;
        for (psS32 k = 0; k < nZsum; k++) {
            ySum = zSum;
            for (psS32 j = 0; j < nYsum; j++) {
                xSum = ySum;
                for (psS32 i = 0; i < nXsum; i++) {
                    sums[i][j][k][m] = xSum;
                    xSum *= x;
                }
                ySum *= y;
            }
            zSum *= z;
        }
        tSum *= t;
    }
    return (sums);
}

/******************************************************************************
 ******************************************************************************
 Analytical 1-D fitting routines.
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
 ******************************************************************************
 1-D Vector Poly Fitting Code.
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
vectorFitPolynomial1DCheb():  This routine will fit a Chebyshev
polynomial of degree myPoly->nX to the data points (x, y) and return the
coefficients of that polynomial.
*****************************************************************************/
static bool vectorFitPolynomial1DCheb(
    psPolynomial1D* myPoly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector* y,
    const psVector* yErr,
    const psVector* x)
{
    PS_ASSERT_POLY_NON_NULL(myPoly, NULL);
    PS_ASSERT_INT_LARGER_THAN_OR_EQUAL(myPoly->nX, 0, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F64, NULL);
    if (yErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, yErr, NULL);
        PS_ASSERT_VECTOR_TYPE(yErr, PS_TYPE_F64, NULL);
    }
    if (x != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, x, NULL);
        PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F64, NULL);
    }
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, mask, NULL);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, NULL);
    }

    int numTerms = myPoly->nX + 1;      // Number of polynomial terms
    int numData = x->n;                 // Number of data elements

    psImage *A = psImageAlloc(numTerms, numTerms, PS_TYPE_F64); // Least-squares matrix
    psVector *B = psVectorAlloc(numTerms, PS_TYPE_F64); // Least-squares vector
    psImageInit(A, 0.0);
    psVectorInit(B, 0.0);

    psPolynomial1D **chebPolys = p_psCreateChebyshevPolys(numTerms); // The chebyshev polynomials
    psImage *sums = psImageAlloc(numData, numTerms, PS_TYPE_F64);
    for (int i = 0; i < numTerms; i++) {
        if (myPoly->coeffMask[i] & PS_POLY_MASK_BOTH) {
            continue;
        }
        psPolynomial1D *cheb = chebPolys[i];
        psVector *sum = psPolynomial1DEvalVector(cheb, x);
        memcpy(sums->data.F64[i], sum->data.F64, numData*sizeof(psF64));
        psFree(sum);
        psFree(cheb);
    }
    psFree(chebPolys);

    // Dereference pointers, for speed in the loop
    psF64 **matrix = A->data.F64;       // Least-squares matrix
    psF64 *vector = B->data.F64;        // Least-squares vector
    psVectorMaskType *dataMask = NULL;              // Mask for data
    if (mask) {
        dataMask = mask->data.PS_TYPE_VECTOR_MASK_DATA;
    }
    psMaskType *coeffMask = myPoly->coeffMask;      // Mask for polynomial terms
    psF64 *yData = y->data.F64;         // Coordinate data
    psF64 *yErrData = NULL;             // Errors in the coordinate
    if (yErr) {
        yErrData = yErr->data.F64;
    }
    psF64 **sumsData = sums->data.F64;  // Sums

    for (int k = 0; k < numData; k++) {
        if (dataMask && dataMask[k]) {
            continue;
        }

        double wt;
        if (!yErr) {
            wt = 1.0;
        } else {
            // this filters fErr == 0 values
            wt = (yErrData[k] == 0) ? 0.0 : 1.0 / PS_SQR(yErrData[k]);
        }

        for (int i = 0; i < numTerms; i++) {
            if (coeffMask[i] & PS_POLY_MASK_BOTH) {
                matrix[i][i] = 1.0;
                continue;
            }
            vector[i] += yData[k] * sumsData[i][k] * wt;
            matrix[i][i] += sumsData[i][k] * sumsData[i][k] * wt; // The diagonal entry
            for (int j = i + 1; j < numTerms; j++) { // The upper diagonal only: we will use symmetry
                if (coeffMask[j] & PS_POLY_MASK_BOTH) {
                    continue;
                }
                double value = sumsData[i][k] * sumsData[j][k] * wt; // The value to add to the matrix
                matrix[i][j] += value;
                matrix[j][i] += value;  // Taking advantage of the symmetry
            }
        }
    }
    psFree(sums);

    if (psTraceGetLevel("psLib.math") >= 6) {
        PS_IMAGE_PRINT_F64(A);
        PS_VECTOR_PRINT_F64(B);
    }

    bool status = false;
    if (USE_GAUSS_JORDAN) {
        status = psMatrixGJSolve(A, B);
    } else {
        status = psMatrixLUSolve(A, B);
    }
    if (!status) {
	psError(PS_ERR_UNKNOWN, false, "Could not solve linear equations.\n");
	goto escape;
    } 

    // the first nTerm entries in B correspond directly to the desired
    // polynomial coefficients.  this is only true for the 1D case
    for (psS32 k = 0; k < numTerms; k++) {
	myPoly->coeff[k] = B->data.F64[k];
	myPoly->coeffErr[k] = sqrt(A->data.F64[k][k]);
    }
    // The constant needs to be multiplied by 2, because it's half the a_0.
    myPoly->coeff[0] *= 2.0;
    myPoly->coeffErr[0] *= 2.0;

    psFree(A);
    psFree(B);
    return true;

escape:
    psFree(A);
    psFree(B);
    return false;
}

/******************************************************************************
VectorFitPolynomial1DOrd(myPoly, *mask, maskValue, *y, *yErr, *x): This is a
private routine which will fit a 1-D polynomial to a set of (x, f) pairs.  The
x and fErr vectors may be NULL.  All non-NULL vectors must be of type
PS_TYPE_F64.
 
XXX EAM : since this is a private function, can we drop the ASSERTS?
XXX EAM : can we drop the LUD version? it does not calculate coeffErr values!!
*****************************************************************************/
static bool VectorFitPolynomial1DOrd(
    psPolynomial1D* myPoly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x)
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(myPoly, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F64, false);
    if (mask) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (x) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
        PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F64, false);
    }
    if (fErr) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, PS_TYPE_F64, false);
    }

    if (psTraceGetLevel("psLib.math") >= 6) {
        psTrace("psLib.math", 6, "VectorFitPolynomial1D()\n");
        for (psS32 i = 0; i < f->n; i++) {
            psTrace("psLib.math", 6, "(x, f, fErr) is (");
            if (x != NULL) {
                psTrace("psLib.math", 6, "%f, %f, ", x->data.F64[i], f->data.F64[i]);
            } else {
                psTrace("psLib.math", 6, "%f, %f, ", (psF64) i, f->data.F64[i]);
            }
            if (fErr != NULL) {
                psTrace("psLib.math", 6, "%f)\n", fErr->data.F64[i]);
            } else {
                psTrace("psLib.math", 6, "NULL)\n");
            }
        }
    }

    int nTerm = myPoly->nX + 1;         // Number of terms in the equation
    int nData = f->n;                   // Number of data points
    psImage *A = psImageAlloc(nTerm, nTerm, PS_TYPE_F64); // Least-squares matrix
    psVector *B = psVectorAlloc(nTerm, PS_TYPE_F64); // Least-squares vector

    // Initialize data structures.
    if (!psImageInit(A, 0.0) || !psVectorInit(B, 0.0)) {
        psError(PS_ERR_UNKNOWN, false, "Could initialize data structures A, B.  Returning NULL.\n");
        psFree(A);
        psFree(B);
        psTrace("psLib.math", 4, "---- %s() End ----\n", __func__);
        return false;
    }

    // Build the least squares matrix and vector

    // Dereference some pointers for speed in the loop
    psVectorMaskType *dataMask = NULL;              // Dereferenced version of mask for data points
    if (mask) {
        dataMask = mask->data.PS_TYPE_VECTOR_MASK_DATA;
    }
    psMaskType *coeffMask = myPoly->coeffMask;      // Dereferenced version of mask for polynomial terms
    psF64 *ordinates = NULL;            // Dereferenced version of ordinate data
    if (x) {
        ordinates = x->data.F64;
    }
    psF64 *coordinates = f->data.F64;   // Dereferenced version of coordinate data
    psF64 *coordErr = NULL;             // Dereferenced version of coordinate errors
    if (fErr) {
        coordErr = fErr->data.F64;
    }
    psF64 *vector = B->data.F64;        // Dereferenced version of least-squares vector
    psF64 **matrix = A->data.F64;       // Dereferenced version of least-squares matrix

    psVector* xSums = NULL;             // Contains 1, x, x^2, x^3, etc, for ease of calculation
    for (int k = 0; k < nData; k++) {
        if (dataMask && dataMask[k] & maskValue) {
            continue;
        }
        if (ordinates) {
            xSums = BuildSums1D(xSums, ordinates[k], nTerm);
        } else {
            xSums = BuildSums1D(xSums, (psF64)k, nTerm);
        }
        psF64 *sums = xSums->data.F64;  // Dereferenced version of sums

        double wt;
        if (!fErr) {
            wt = 1.0;
        } else {
            // this filters fErr == 0 values
            wt = (coordErr[k] == 0) ? 0.0 : 1.0 / PS_SQR(coordErr[k]);
        }

        for (int i = 0; i < nTerm; i++) {
            if (coeffMask[i] & PS_POLY_MASK_SET) {
                matrix[i][i] = 1.0;
                continue;
            }
            vector[i] += coordinates[k] * sums[i] * wt;
            matrix[i][i] += sums[2 * i] * wt; // The diagonal entry
            for (int j = i + 1; j < nTerm; j++) { // The upper diagonal only: we will use symmetry
                if (coeffMask[j] & PS_POLY_MASK_SET) {
                    continue;
                }
                double value = sums[i + j] * wt; // The value to add to the matrix
                matrix[i][j] += value;
                matrix[j][i] += value;  // Taking advantage of the symmetry
            }
        }
    }
    psFree(xSums);

    // elements which are masked for fitting need to be subtracted from the vector
    for (int i = 0; i < nTerm; i++) {
	if (coeffMask[i] & PS_POLY_MASK_BOTH) {
	    continue;
	}
	for (int j = 0; j < nTerm; j++) { // The upper diagonal only: we will use symmetry
	    if (coeffMask[j] & PS_POLY_MASK_SET) {
		continue;
	    }
	    if (!(coeffMask[j] & PS_POLY_MASK_FIT)) {
		continue;
	    }
	    vector[i] -= matrix[i][j]*myPoly->coeff[j];
	}
    }
    
    // set the un-fitted and un-set elements to 0 or 1 for pivots
    for (int i = 0; i < nTerm; i++) {
	if (coeffMask[i] & PS_POLY_MASK_BOTH) {
	    for (int j = 0; j < nTerm; j++) { // The upper diagonal only: we will use symmetry
		matrix[i][j] = 0.0;
		matrix[j][i] = 0.0;
	    }
	    matrix[i][i] = 1.0;
	    continue;
	}
    }

    if (psTraceGetLevel("psLib.math") >= 4) {
        printf("Least-squares vector:\n");
        for (int i = 0; i < nTerm; i++) {
            printf("%f ", B->data.F64[i]);
        }
        printf("\n");
        printf("Least-squares matrix:\n");
        for (int i = 0; i < nTerm; i++) {
            for (int j = 0; j < nTerm; j++) {
                printf("%f ", A->data.F64[i][j]);
            }
            printf("\n");
        }
    }

    bool status = false;
    if (USE_GAUSS_JORDAN) {

#if (CZW)
      	printf("CZW: about to do GJ: %d\n",status);
	for (psS32 k = 0; k < nTerm; k++) {
	  printf("CZW: %d %f \t",k,B->data.F64[k]);
	  for (psS32 kk = 0; kk < nTerm; kk++) {
	    printf(" %f ",(A->data.F64[k][kk]));
	  }
	  printf("\n");
	}
#endif
        status = psMatrixGJSolve(A, B);
#if (CZW)
	printf("CZW: just did GJ: %d\n",status);
	for (psS32 k = 0; k < nTerm; k++) {
	  printf("CZW: %d %f \t",k,B->data.F64[k]);
	  for (psS32 kk = 0; kk < nTerm; kk++) {
	    printf(" %f ",(A->data.F64[k][kk]));
	  }
	  printf("\n");
	}
#endif
    } else {
        status = psMatrixLUSolve(A, B);
    }
    if (!status) {
	psError(PS_ERR_UNKNOWN, false, "Could not solve linear equations.\n");
	goto escape;
    } 

    // the first nTerm entries in B correspond directly to the desired
    // polynomial coefficients.  this is only true for the 1D case
    for (psS32 k = 0; k < nTerm; k++) {
	if (coeffMask[k] & PS_POLY_MASK_FIT) continue;
	myPoly->coeff[k] = B->data.F64[k];
	myPoly->coeffErr[k] = sqrt(A->data.F64[k][k]);
    }

    psFree(A);
    psFree(B);
    return true;

escape:
    psFree(A);
    psFree(B);
    return false;
}

/******************************************************************************
psVectorFitPolynomial1D():  This routine fits a polynomial of arbitrary degree
(specified in poly) to the data points (x, y) and return that polynomial.
Types F32 and F64 are supported, however, type F32 is done via vector
conversion only.
 *****************************************************************************/
bool psVectorFitPolynomial1D(
    psPolynomial1D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x)
{
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_INT_NONNEGATIVE(poly->nX, false);

    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_NON_EMPTY(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE_F32_OR_F64(fErr, false);
    }
    if (x != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
        PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, false);
    }

    // Convert input vectors to F64 if necessary.
    psVector *f64 = (f->type.type == PS_TYPE_F64) ? (psVector *) f : psVectorCopy (NULL, f, PS_TYPE_F64);
    psVector *x64 = NULL;
    if (x != NULL) {
        x64 = (x->type.type == PS_TYPE_F64) ? (psVector *) x : psVectorCopy (NULL, x, PS_TYPE_F64);
    }
    psVector *fErr64 = NULL;
    if (fErr != NULL) {
        fErr64 = (fErr->type.type == PS_TYPE_F64) ? (psVector *) fErr : psVectorCopy (NULL, fErr, PS_TYPE_F64);
    }

    bool result = true;

    // Define values that may be used by PS_POLYNOMIAL_ORD form.
    // these scaling values put the dynamic range of the data into some vaguely sensible location
    bool scale = false;
    bool median_zero = false;
    double median = 0.0;
    double sigma = 1.0;

    switch (poly->type) {
    case PS_POLYNOMIAL_ORD:
      if ((f64->n < 10000)&&(poly->nX > 1)) {
	scale = true;

	// generate a subset of the unmasked values:
	psVector *tmp = psVectorAllocEmpty (x64->n, PS_TYPE_F64);
	for (int itmp = 0; itmp < x64->n; itmp++) {
	  if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[itmp] && maskValue)) continue;
	  psVectorAppend (tmp, x64->data.F64[itmp]);
	}
	psVector *sorted = psVectorSort(NULL,tmp);
	median = sorted->data.F64[sorted->n / 2];
	// CZW: I'm not bothering to scale this because it doesn't really matter.
	sigma  = (sorted->data.F64[3 * sorted->n / 4] - sorted->data.F64[sorted->n / 4]); 
	psFree(sorted);
	psFree(tmp);

	if ((!isfinite(median))||
	    (!isfinite(sigma))) {
	  median = 0.0;
	  sigma = 1.0;
	}
	if (fabs(median) < 1e-10) {
	  // CZW 2014-10-21: This median is small and close to zero.  This can cause issues with the
	  // scaling,
	  median_zero = true;
	  if ((sigma == 1.0)||(fabs(sigma) <= 1e-10)) {
	    // Don't bother scaling if sigma is unity (it's already scaled) or if the sigma calculation has gone wrong.
	    scale = false;
	  }
	}

	// I can't see a way to not clobber x if it's already F64, so make a copy.x
	psVector *z64 = psVectorCopy(NULL,x64,PS_TYPE_F64);
	psBinaryOp(z64,z64,"-",psScalarAlloc(median,PS_TYPE_F64));
	psBinaryOp(z64,z64,"/",psScalarAlloc(sigma,PS_TYPE_F64));

#if (CZW)
	printf("poly1d: Scale parameters: %f %f\n",median,sigma);
#endif

	result = VectorFitPolynomial1DOrd(poly, mask, maskValue, f64, fErr64, z64);
	psFree(z64); // Done with this.
      }
      else {
	result = VectorFitPolynomial1DOrd(poly, mask, maskValue, f64, fErr64, x64);
      }
        
      if (!result) {
	psError(PS_ERR_UNKNOWN, false, "Could not fit polynomial.  Returning NULL.\n");
      }
      
      if (scale) {
	// Undo scaling in the polynomial values.
	psF64 *Zcoeff = psAlloc((1 + poly->nX) * sizeof(psF64));
	psF64 *ZcoeffErr = psAlloc((1 + poly->nX) * sizeof(psF64));
	
	for (psS32 i = 0; i <= poly->nX; i++) {
	  Zcoeff[i] = poly->coeff[i];
	  ZcoeffErr[i] = poly->coeffErr[i];
#if (CZW)
	  printf("poly1d: fit parameters: %d %f %f\n",
		 i,poly->coeff[i],poly->coeffErr[i]);
#endif
	}

	for (psS32 i = 0; i <= poly->nX; i++) {
	  poly->coeff[i] = 0.0;
	  poly->coeffErr[i] = 0.0;
	  if (median_zero) {  // If the median is zero, the obtained solution just needs to be scaled by the sigma values.
	    poly->coeff[i]    = Zcoeff[i] * pow(1.0 / sigma,i);
	    poly->coeffErr[i] = ZcoeffErr[i] * pow(1.0 / sigma,i);
	  }
	  else { // Otherwise, do the correct transformations by expanding the (x-m)/s terms.
	    for (psS32 j = 0; j <= poly->nX; j++) {
#if (CZW)
	      printf("        %d %d %f %f %f %f => %f\n",
		     i,j,Zcoeff[j],
		     pow(1.0 / sigma,j) * pow(-1,j - i),
		     pow(median,j - i),
		     1.0 * psBinomialCoeff(j,i),
		     Zcoeff[j] * pow(1.0 / sigma,j) * pow(-1,j  -i) * pow(median,j - i) * 1.0 * psBinomialCoeff(j,i)
		     );
#endif
	      poly->coeff[i] += Zcoeff[j] * pow(1.0 / sigma,j) * pow(-1,j - i) * pow(median,j - i) * psBinomialCoeff(j,i);
	      poly->coeffErr[i] += pow(ZcoeffErr[j] * pow(1.0 / sigma,j) * pow(-1,j - i) * pow(median,j - 1) * psBinomialCoeff(j,i),2);
	    }
	    poly->coeffErr[i] = sqrt(poly->coeffErr[i]);
	  }
#if (CZW)
	  printf("poly1d: unscaled parameters: %d %f %f\n",
		 i,poly->coeff[i], poly->coeffErr[i]);
#endif
	}
	psFree(Zcoeff);
	psFree(ZcoeffErr);

      } // End scaling block.
	
        break;
    case PS_POLYNOMIAL_CHEB:
        if (mask != NULL) {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: ignoring mask and maskValue with Chebyshev polynomials.\n");
        }
        if (fErr != NULL) {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: ignoring error vector with Chebyshev polynomials.\n");
        }
        if (x == NULL) {
            // If x==NULL, create an x64 vector with x values set to (-1:1).
            PS_VECTOR_GEN_CHEBY_INDEX(x64, f64->n, PS_TYPE_F64);
        }

        result = vectorFitPolynomial1DCheb(poly, mask, maskValue, f64, fErr64, x64);
        if (!result) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit polynomial.  Returning NULL.\n");
        }

        if (x == NULL) {
            psFree(x64);
        }
        break;
    default:
        psError(PS_ERR_UNKNOWN, true, "Incorrect polynomial type (%d).  Returning NULL.\n", poly->type);
        result = false;
        break;
    }

    // Free psVectors that were created for NULL arguments.
    PS_FREE_TEMP_F64_VECTOR (f, f64);
    PS_FREE_TEMP_F64_VECTOR (x, x64);
    PS_FREE_TEMP_F64_VECTOR (fErr, fErr64);

    return result;
}

// This function accepts F32 and F64 input vectors.
bool psVectorClipFitPolynomial1D(
    psPolynomial1D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *xIn)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_PTR_NON_NULL(stats, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);
    PS_ASSERT_VECTOR_NON_NULL(mask, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(mask, f, false);
    PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);

    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(fErr, f, false);
        PS_ASSERT_VECTOR_TYPE(fErr, f->type.type, false);
    }

    // Internal pointers for possibly NULL vectors.
    psVector *x = NULL;
    if (xIn != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(xIn, f, false);
        PS_ASSERT_VECTOR_TYPE(xIn, f->type.type, false);
        x = (psVector *) xIn;
    } else {
        if (poly->type == PS_POLYNOMIAL_ORD) {
            x = psVectorCreate(NULL, 0, f->n, 1, f->type.type);
        } else if (poly->type == PS_POLYNOMIAL_CHEB) {
            if (f->type.type == PS_TYPE_F32) {
                PS_VECTOR_GEN_CHEBY_INDEX(x, f->n, PS_TYPE_F32);
            } else if (f->type.type == PS_TYPE_F64) {
                PS_VECTOR_GEN_CHEBY_INDEX(x, f->n, PS_TYPE_F64);
            }
        } else {
            psError(PS_ERR_UNKNOWN, true, "Error, bad poly type.\n");
            return false;
        }
    }

    // the user supplies one of various stats option pairs,
    // determine the desired mean and stdev STATS options:
    // XXX enforce consistency?
    // XXX psStatsGetValue() probably has inverted precedence
    psStatsOptions meanOption = stats->options & (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN | PS_STAT_ROBUST_MEDIAN | PS_STAT_CLIPPED_MEAN | PS_STAT_FITTED_MEAN | PS_STAT_FITTED_MEAN);
    psStatsOptions stdevOption = stats->options & (PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_STDEV | PS_STAT_CLIPPED_STDEV | PS_STAT_FITTED_STDEV | PS_STAT_FITTED_STDEV);
    if (!meanOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
        return false;
    }
    if (!stdevOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
        return false;
    }

    // clipping range defined by min and max and/or clipSigma
    psF32 minClipSigma;
    psF32 maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = fabs(stats->max);
    } else {
        maxClipSigma = fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = fabs(stats->min);
    } else {
        minClipSigma = fabs(stats->clipSigma);
    }
    psVector *resid = psVectorAlloc(f->n, PS_TYPE_F64);

    psTrace("psLib.math", 4, "stats->clipIter is %d\n", stats->clipIter);
    psTrace("psLib.math", 4, "(minClipSigma, maxClipSigma) is (%.2f, %.2f)\n", minClipSigma, maxClipSigma);

    //
    for (psS32 N = 0; N < stats->clipIter; N++) {
        psTrace("psLib.math", 6, "Loop iteration %d.  Calling psVectorFitPolynomial1D()\n", N);
        psS32 Nkeep = 0;
        if (psTraceGetLevel("psLib.math") >= 6) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    psTrace("psLib.math", 6,  "mask[%d] is %d\n", i, mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
                }
            }
        }
        if (!psVectorFitPolynomial1D(poly, mask, maskValue, f, fErr, x)) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit polynomial.  Returning false.\n");
            if (xIn == NULL) {
                psFree(x);
            }
	    psFree(resid);
	    
            return false;
        }

        psVector *fit = psPolynomial1DEvalVector(poly, x);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Could not call psPolynomial3DEvalVector().  Returning false.\n");
            if (xIn == NULL) {
                psFree(x);
            }
            psFree(resid);
            return false;
        }
        for (psS32 i = 0 ; i < f->n ; i++) {
            if (f->type.type == PS_TYPE_F64) {
                resid->data.F64[i] = f->data.F64[i] - fit->data.F64[i];
            } else {
                resid->data.F64[i] = (psF64) (f->data.F32[i] - fit->data.F32[i]);
            }
        }
        if (psTraceGetLevel("psLib.math") >= 6) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    if (!((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue))) {
                        psTrace("psLib.math", 6,  "(f, fit)[%d] is (%f, %f).  resid is (%f)\n",
                                i, f->data.F32[i], fit->data.F32[i], resid->data.F64[i]);
                    }
                }
            }
        }

        if (!psVectorStats(stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "Could not compute statistics on the resid vector.  Returning false.\n");
            psFree(resid);
            psFree(fit);
            return false;
        }

        double meanValue = psStatsGetValue (stats, meanOption);
        double stdevValue = psStatsGetValue (stats, stdevOption);

        psTrace("psLib.math", 5, "Mean is %f\n", meanValue);
        psTrace("psLib.math", 5, "Stdev is %f\n", stdevValue);
        psF32 minClipValue = -minClipSigma*stdevValue;
        psF32 maxClipValue = +maxClipSigma*stdevValue;

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (psS32 i = 0; i < resid->n; i++) {
            if ((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) {
                continue;
            }

            if ((resid->data.F64[i] - meanValue > maxClipValue) || (resid->data.F64[i] - meanValue < minClipValue)) {
                if (f->type.type == PS_TYPE_F64) {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F64[i], i, resid->data.F64[i]);
                } else {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F32[i], i, resid->data.F64[i]);
                }

                if (mask != NULL) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                }
                continue;
            }
            Nkeep++;
        }

        //
        // We should probably exit this loop if no new elements were masked
        // since the polynomial fit won't change.
        //
        psTrace("psLib.math", 6, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree(fit);
    }

    // Free psVectors that were created for NULL arguments.
    if (xIn == NULL) {
        psFree(x);
    }
    // Free other local temporary variables
    psFree(resid);

    psTrace("psLib.math", 3, "---- %s() end ----\n", __func__);
    return true;
}


/******************************************************************************
 ******************************************************************************
 2-D Vector Code.
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
VectorFitPolynomial2DOrd(myPoly, *mask, maskValue, *f, *fErr, *x, *y): This is
a private routine which will fit a 2-D polynomial to a set of (x, y)-(f)
pairs.  All non-NULL vectors must be of type PS_TYPE_F64.
 
 *****************************************************************************/
static bool VectorFitPolynomial2DOrd(
    psPolynomial2D* myPoly,
    const psVector* mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y)
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(myPoly, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nX, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nY, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F64, false);
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, PS_TYPE_F64, false);
    }
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }

    // Number of polynomial terms
    int nXterm = 1 + myPoly->nX;      // Number of terms in x
    int nYterm = 1 + myPoly->nY;      // Number of terms in y
    int nTerm = nXterm * nYterm;            // Total number of terms

    psImage *A = psImageAlloc(nTerm, nTerm, PS_TYPE_F64); // Least-squares matrix
    psVector *B = psVectorAlloc(nTerm, PS_TYPE_F64); // Least-squares vector

    // Initialize data structures.
    if (!psImageInit(A, 0.0) || !psVectorInit(B, 0.0)) {
        psError(PS_ERR_UNKNOWN, false, "Could initialize data structures A, B.  Returning NULL.\n");
        psFree(A);
        psFree(B);
        psTrace("psLib.math", 6, "---- %s() End ----\n", __func__);
        return false;
    }

    // Dereference stuff, to make the loop go faster
    psF64 **matrix = A->data.F64;       // Dereference the least-squares matrix
    psF64 *vector = B->data.F64;        // Dereference the least-squares vector
    psMaskType **coeffMask = myPoly->coeffMask;     // Dereference mask for polynomial terms
    psVectorMaskType *dataMask = NULL;              // Dereference mask for data
    if (mask) {
        dataMask = mask->data.PS_TYPE_VECTOR_MASK_DATA;
    }
    psF64 *xData = x->data.F64;         // Dereference x
    psF64 *yData = y->data.F64;         // Dereference y
    psF64 *fData = f->data.F64;         // Dereference f
    psF64 *fErrData = NULL;             // Dereference fErr
    if (fErr) {
        fErrData = fErr->data.F64;
    }

    // Build the least-squares matrix and vector
    psImage *xySums = NULL;               // The sums: 1, x, x^2, ... x^(2n+1), y, xy, x^2y, ... x^(2n+1)
    for (int k = 0; k < x->n; k++) {
        if (dataMask && dataMask[k] & maskValue) {
            continue;
        }
        xySums = BuildSums2D(xySums, xData[k], yData[k], nXterm, nYterm);
        psF64 **sums = xySums->data.F64;// Dereference sums

        double wt;                      // Weight
        if (!fErrData) {
            wt = 1.0;
        } else {
            // this filters fErr == 0 values
            wt = (fErrData[k] == 0.0) ? 0.0 : 1.0 / PS_SQR(fErrData[k]);
        }

        // Iterating over the matrix
        for (int i = 0; i < nTerm; i++) {
            int l = i / nYterm;         // x index
            int m = i % nYterm;         // y index
            if (coeffMask[l][m] & PS_POLY_MASK_SET) {
                matrix[i][i] = 1.0;
                continue;
            }
            vector[i] += fData[k] * sums[l][m] * wt;
            matrix[i][i] += sums[2*l][2*m] * wt; // The diagonal entry
            for (int j = i + 1; j < nTerm; j++) { // Doing the upper diagonal only: we will use symmetry
                int p = j / nYterm;     // x index
                int q = j % nYterm;     // y index
                if (coeffMask[p][q] & PS_POLY_MASK_SET) {
                    continue;
                }
                double value = sums[l+p][m+q] * wt; // Value to add in
                matrix[i][j] += value;
                matrix[j][i] += value;  // Taking advantage of the symmetry
            }
        }
    }
    psFree(xySums);

    // elements which are masked for fitting need to be subtracted from the vector
    for (int i = 0; i < nTerm; i++) {
	int ix = i / nYterm;         // x index
	int iy = i % nYterm;         // y index
	if (coeffMask[ix][iy] & PS_POLY_MASK_BOTH) {
	    continue;
	}
	for (int j = 0; j < nTerm; j++) { // The upper diagonal only: we will use symmetry
	    int jx = j / nYterm;         // x index
	    int jy = j % nYterm;         // y index
	    if (coeffMask[jx][jy] & PS_POLY_MASK_SET) {
		continue;
	    }
	    if (!(coeffMask[jx][jy] & PS_POLY_MASK_FIT)) {
		continue;
	    }
	    vector[i] -= matrix[i][j]*myPoly->coeff[jx][jy];
	}
    }
    
    // set the un-fitted and un-set elements to 0 or 1 for pivots
    for (int i = 0; i < nTerm; i++) {
	int ix = i / nYterm;         // x index
	int iy = i % nYterm;         // y index
	if (coeffMask[ix][iy] & PS_POLY_MASK_BOTH) {
	    for (int j = 0; j < nTerm; j++) { // The upper diagonal only: we will use symmetry
		matrix[i][j] = 0.0;
		matrix[j][i] = 0.0;
	    }
	    matrix[i][i] = 1.0;
	    continue;
	}
    }

    if (psTraceGetLevel("psLib.math") >= 4) {
        printf("Least-squares vector:\n");
        for (int i = 0; i < nTerm; i++) {
            printf("%f ", B->data.F64[i]);
        }
        printf("\n");
        printf("Least-squares matrix:\n");
        for (int i = 0; i < nTerm; i++) {
            for (int j = 0; j < nTerm; j++) {
                printf("%f ", A->data.F64[i][j]);
            }
            printf("\n");
        }
    }

    bool status = false;
    if (USE_GAUSS_JORDAN) {
        status = psMatrixGJSolve(A, B);
    } else {
        status = psMatrixLUSolve(A, B);
    }
    if (!status) {
	psError(PS_ERR_UNKNOWN, false, "Could not solve linear equations.\n");
	goto escape;
    } 

    // select the appropriate solution entries (retain the incoming values if masked on the fit)
    for (int i = 0; i < nTerm; i++) {
        int ix = i / nYterm;         // x index
        int iy = i % nYterm;         // y index
	if (coeffMask[ix][iy] & PS_POLY_MASK_FIT) continue;
	myPoly->coeff[ix][iy] = B->data.F64[i];
	myPoly->coeffErr[ix][iy] = sqrt(A->data.F64[i][i]);
    }
    psFree(A);
    psFree(B);
    return true;

escape:
    psFree (A);
    psFree (B);
    return false;
}

/******************************************************************************
VectorFitPolynomial2DCheb(myPoly, *mask, maskValue, *f, *fErr, *x, *y): This is
a private routine which will fit a 2-D polynomial to a set of (x, y)-(f)
pairs.  All non-NULL vectors must be of type PS_TYPE_F64.
 
 *****************************************************************************/
static bool VectorFitPolynomial2DCheb(
    psPolynomial2D* myPoly,
    const psVector *f,
    const psVector *x,
    const psVector *y)
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(myPoly, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nX, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nY, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F64, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);

    // Number of polynomial terms
    int nXterm = 1 + myPoly->nX;      // Number of terms in x
    int nYterm = 1 + myPoly->nY;      // Number of terms in y
    int nTerm = nXterm * nYterm;      // Total number of terms
    if (nXterm > 9) {
	psError(PS_ERR_UNKNOWN, false, "failed 2D chebyshev fit: orders higher than 9 are not yet coded\n");
	return false;
    }
    if (nYterm > 9) {
	psError(PS_ERR_UNKNOWN, false, "failed 2D chebyshev fit: orders higher than 9 are not yet coded\n");
	return false;
    }

    // determine scale factors
    if (!psChebyshevSetScale (myPoly, x, 0)) { psError(PS_ERR_UNKNOWN, false, "failed 2D chebyshev fit.\n"); return false; }
    if (!psChebyshevSetScale (myPoly, y, 1)) { psError(PS_ERR_UNKNOWN, false, "failed 2D chebyshev fit.\n"); return false; }

    // generate normalized vectors
    psVector *xNorm = psChebyshevNormVector (myPoly, x, 0);
    psVector *yNorm = psChebyshevNormVector (myPoly, y, 1);

    // generate the N cheb polynomials based on xNorm, yNorm
    psArray *xPolySet = psArrayAlloc (nXterm);
    for (int i = 0; i < nXterm; i++) {
	xPolySet->data[i] = psChebyshevPolyVector (xNorm, i);
    }
    psArray *yPolySet = psArrayAlloc (nYterm);
    for (int i = 0; i < nYterm; i++) {
	yPolySet->data[i] = psChebyshevPolyVector (yNorm, i);
    }

    psF64 *fData = f->data.F64;         // Dereference f

    psImage *A = psImageAlloc(nTerm, nTerm, PS_TYPE_F64); // Least-squares matrix
    psVector *B = psVectorAlloc(nTerm, PS_TYPE_F64); // Least-squares vector

    // Initialize data structures (should not be able to fail)
    psAssert (psImageInit(A, 0.0), "Could initialize data structures A");
    psAssert (psVectorInit(B, 0.0), "Could initialize data structures B");

    // Dereference stuff, to make the loop go faster
    psF64 **matrix = A->data.F64;       // Dereference the least-squares matrix
    psF64 *vector = B->data.F64;        // Dereference the least-squares vector

    // loop over all elements of the data vector
    for (int k = 0; k < x->n; k++) {

	if (!finite(fData[k])) continue;
    
	// XXX can we only calculate the upper diagonal?
	int nelem = 0;
	for (int jx = 0; jx < nXterm; jx++) {
	    psVector *jxCheb = xPolySet->data[jx];
	    for (int jy = 0; jy < nYterm; jy++) {
		psVector *jyCheb = yPolySet->data[jy];
		psF64 chebValue = jxCheb->data.F64[k] * jyCheb->data.F64[k];
		
		vector[nelem] += fData[k] * chebValue;

		int melem = 0;
		for (int kx = 0; kx < nXterm; kx++) {
		    psVector *kxCheb = xPolySet->data[kx];
		    for (int ky = 0; ky < nYterm; ky++) {
			psVector *kyCheb = yPolySet->data[ky];
			matrix[nelem][melem] += chebValue * kxCheb->data.F64[k]*kyCheb->data.F64[k];
			melem++;
		    }
		}
		nelem++;
	    }
	}
    }

    if (psTraceGetLevel("psLib.math") >= 4) {
        printf("Least-squares vector:\n");
        for (int i = 0; i < nTerm; i++) {
            printf("%f ", B->data.F64[i]);
        }
        printf("\n");
        printf("Least-squares matrix:\n");
        for (int i = 0; i < nTerm; i++) {
            for (int j = 0; j < nTerm; j++) {
                printf("%f ", A->data.F64[i][j]);
            }
            printf("\n");
        }
    }

    bool status = false;
    if (USE_GAUSS_JORDAN) {
        status = psMatrixGJSolve(A, B);
    } else {
        status = psMatrixLUSolve(A, B);
    }
    if (!status) {
	psError(PS_ERR_UNKNOWN, false, "Could not solve linear equations.\n");
	goto escape;
    } 

    // unroll the result:
    int nelem = 0;
    for (int jx = 0; jx < nXterm; jx++) {
	for (int jy = 0; jy < nYterm; jy++) {
	    myPoly->coeff[jx][jy]    = B->data.F64[nelem];
	    myPoly->coeffErr[jx][jy] = sqrt(A->data.F64[nelem][nelem]);
	    nelem ++;
	}
    }
    psFree(A);
    psFree(B);

    psFree (xNorm);
    psFree (yNorm);
    psFree (xPolySet);
    psFree (yPolySet);

    return true;

escape:
    psFree (A);
    psFree (B);
    return false;
}

/******************************************************************************
psVectorFitPolynomial2D():  This routine fits a 2D polynomial of arbitrary
degree (specified in poly) to the data points (x, y)-(f) and returns that
polynomial.  Types F32 and F64 are supported, however, type F32 is done via
vector conversion only.
 *****************************************************************************/
bool psVectorFitPolynomial2D(
    psPolynomial2D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y)
{
    PS_ASSERT_POLY_NON_NULL(poly, false);
    // PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, false);

    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE_F32_OR_F64(fErr, false);
    }

    // Convert input vectors to F64 if necessary.
    psVector *f64 = (f->type.type == PS_TYPE_F64) ? (psVector *) f : psVectorCopy(NULL, f, PS_TYPE_F64);
    psVector *x64 = (x->type.type == PS_TYPE_F64) ? (psVector *) x : psVectorCopy(NULL, x, PS_TYPE_F64);
    psVector *y64 = (y->type.type == PS_TYPE_F64) ? (psVector *) y : psVectorCopy(NULL, y, PS_TYPE_F64);

    psVector *fErr64 = NULL;
    if (fErr != NULL) {
        fErr64 = (fErr->type.type == PS_TYPE_F64) ? (psVector *) fErr : psVectorCopy(NULL, fErr, PS_TYPE_F64);
    }

    bool result = true;

    switch (poly->type) {
    case PS_POLYNOMIAL_ORD:
        result = VectorFitPolynomial2DOrd(poly, mask, maskValue, f64, fErr64, x64, y64);
        if (!result) {
            psError(PS_ERR_UNKNOWN, true, "Could not fit polynomial.  Returning NULL.\n");
        }
        break;
    case PS_POLYNOMIAL_CHEB:
      if (mask != NULL) {
	  psLogMsg(__func__, PS_LOG_WARN, "WARNING: ignoring mask and maskValue with Chebyshev polynomials.\n");
      }
      if (fErr != NULL) {
	  psLogMsg(__func__, PS_LOG_WARN, "WARNING: ignoring error values for Chebyshev polynomials.\n");
      }
      result = VectorFitPolynomial2DCheb(poly, f64, x64, y64);
      if (!result) {
	  psError(PS_ERR_UNKNOWN, true, "Could not fit polynomial.  Returning NULL.\n");
      }
      break;
    default:
        psError(PS_ERR_UNKNOWN, true, "Incorrect polynomial type.  Returning NULL.\n");
        result = false;
        break;
    }

    // Free psVectors that were created for NULL arguments.
    PS_FREE_TEMP_F64_VECTOR (f, f64);
    PS_FREE_TEMP_F64_VECTOR (x, x64);
    PS_FREE_TEMP_F64_VECTOR (y, y64);
    PS_FREE_TEMP_F64_VECTOR (fErr, fErr64);

    return result;
}

bool psVectorClipFitPolynomial2D(
    psPolynomial2D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, false);
    PS_ASSERT_PTR_NON_NULL(stats, false);
    PS_ASSERT_VECTOR_NON_NULL(mask, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);

    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_TYPE(x, f->type.type, false);

    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_TYPE(y, f->type.type, false);

    PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
    PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);

    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, f->type.type, false);
    }

    // the user supplies one of various stats option pairs,
    // determine the desired mean and stdev STATS options:
    // XXX enforce consistency?
    // XXX psStatsGetValue() probably has inverted precedence
    psStatsOptions meanOption = stats->options & (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN | PS_STAT_ROBUST_MEDIAN | PS_STAT_CLIPPED_MEAN | PS_STAT_FITTED_MEAN | PS_STAT_FITTED_MEAN);
    psStatsOptions stdevOption = stats->options & (PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_STDEV | PS_STAT_CLIPPED_STDEV | PS_STAT_FITTED_STDEV | PS_STAT_FITTED_STDEV);
    if (!meanOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
        return false;
    }
    if (!stdevOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
        return false;
    }

    // clipping range defined by min and max and/or clipSigma
    psF32 minClipSigma;
    psF32 maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = fabs(stats->max);
    } else {
        maxClipSigma = fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = fabs(stats->min);
    } else {
        minClipSigma = fabs(stats->clipSigma);
    }
    psVector *resid = psVectorAlloc(f->n, PS_TYPE_F64);

    psTrace("psLib.math", 4, "stats->clipIter is %d\n", stats->clipIter);
    psTrace("psLib.math", 4, "(minClipSigma, maxClipSigma) is (%.2f, %.2f)\n", minClipSigma, maxClipSigma);

    for (psS32 N = 0; N < stats->clipIter; N++) {
        psTrace("psLib.math", 6, "Loop iteration %d.  Calling psVectorFitPolynomial1D()\n", N);
        psS32 Nkeep = 0;
        if (psTraceGetLevel("psLib.math") >= 7) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    psTrace("psLib.math", 7,  "mask[%d] is %d\n", i, mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
                }
            }
        }

        if (!psVectorFitPolynomial2D(poly, mask, maskValue, f, fErr, x, y)) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit a polynomial to the data.  Returning false.\n");
            psFree(resid);
            return false;
        }

        psVector *fit = psPolynomial2DEvalVector(poly, x, y);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Could not call psPolynomial3DEvalVector().  Returning NULL.\n");
            psFree(resid);
            return false;
        }

        for (psS32 i = 0 ; i < f->n ; i++) {
            if (f->type.type == PS_TYPE_F64) {
                resid->data.F64[i] = f->data.F64[i] - fit->data.F64[i];
            } else {
                resid->data.F64[i] = (psF64) (f->data.F32[i] - fit->data.F32[i]);
            }
        }

        if (psTraceGetLevel("psLib.math") >= 7) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    if (!((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue))) {
                        psTrace("psLib.math", 7,  "point %d at %f %f : value, fit : %f  %f resid: %f\n",
                                i, x->data.F32[i], y->data.F32[i], f->data.F32[i], fit->data.F32[i], resid->data.F64[i]);
                    }
                }
            }
        }

        if (!psVectorStats(stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "Could not compute statistics on the resid vector.  Returning NULL.\n");
            psFree(resid);
            psFree(fit);
            return false;
        }

        double meanValue = psStatsGetValue (stats, meanOption);
        double stdevValue = psStatsGetValue (stats, stdevOption);

        psTrace("psLib.math", 5, "Mean is %f\n", meanValue);
        psTrace("psLib.math", 5, "Stdev is %f\n", stdevValue);
        psF32 minClipValue = -minClipSigma*stdevValue;
        psF32 maxClipValue = +maxClipSigma*stdevValue;

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (psS32 i = 0; i < resid->n; i++) {
            if ((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) {
                continue;
            }

            if ((resid->data.F64[i] - meanValue > maxClipValue) || (resid->data.F64[i] - meanValue < minClipValue)) {
                if (fit->type.type == PS_TYPE_F64) {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F64[i], i, resid->data.F64[i]);
                } else {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F32[i], i, resid->data.F64[i]);
                }

                if (mask != NULL) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                }
                continue;
            }
            Nkeep++;
        }
        psTrace("psLib.math", 4, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree(fit);
    }
    // Free local temporary variables
    psFree(resid);

    psTrace("psLib.math", 3, "---- %s() end ----\n", __func__);
    return true;
}


/******************************************************************************
 ******************************************************************************
 3-D Vector Code.
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
VectorFitPolynomial3DOrd(myPoly, *mask, maskValue, *f, *fErr, *x, *y, *z):
This is a private routine which will fit a 3-D polynomial to a set of (x,
y, z)-(f) pairs.  All non-NULL vectors must be of type PS_TYPE_F64.
 
 *****************************************************************************/
static bool VectorFitPolynomial3DOrd(
    psPolynomial3D* myPoly,
    const psVector* mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z)
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(myPoly, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nX, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nY, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nZ, false);

    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F64, false);
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, PS_TYPE_F64, false);
    }
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_NON_NULL(z, false);
    PS_ASSERT_VECTOR_TYPE(z, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, false);
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }

    int nXterm = 1 + myPoly->nX;        // Number of x terms
    int nYterm = 1 + myPoly->nY;        // Number of y terms
    int nZterm = 1 + myPoly->nZ;        // Number of z terms
    int nTerm = nXterm * nYterm * nZterm; // Total number of terms
    int nData = x->n;                   // Number of data points
    psImage    *A = psImageAlloc(nTerm, nTerm, PS_TYPE_F64); // Least-squares matrix
    psVector   *B = psVectorAlloc(nTerm, PS_TYPE_F64); // Least-squares vector

    // Initialize data structures.
    if (!psImageInit(A, 0.0) || !psVectorInit(B, 0.0)) {
        psError(PS_ERR_UNKNOWN, false, "Could initialize data structures A, B.  Returning NULL.\n");
        psFree(A);
        psFree(B);
        psTrace("psLib.math", 4, "---- %s() End ----\n", __func__);
        return false;
    }

    // Dereference points for speed in the loop
    psF64 **matrix = A->data.F64;       // Least-squares matrix
    psF64 *vector = B->data.F64;        // Least-squares vector
    psF64 *xData = x->data.F64;         // x
    psF64 *yData = y->data.F64;         // y
    psF64 *zData = z->data.F64;         // z
    psF64 *fData = f->data.F64;         // f
    psF64 *fErrData = NULL;             // Error in f
    if (fErr) {
        fErrData = fErr->data.F64;
    }
    psVectorMaskType *dataMask = NULL;              // Mask for data
    if (mask) {
        dataMask = mask->data.PS_TYPE_VECTOR_MASK_DATA;
    }
    psMaskType ***coeffMask = myPoly->coeffMask;    // Mask for polynomial terms
    int nYZterm = nYterm * nZterm;      // Multiplication of the numbers, to calculate the index

    // Build the B and A data structs.
    psF64 ***Sums = NULL;         // Sums look like: 1, x, x^2, ... x^(2n+1), y, xy, x^2y, ... x^(2n+1)*y, ...
    for (int k = 0; k < nData; k++) {
        if (dataMask && dataMask[k] & maskValue) {
            continue;
        }

        Sums = BuildSums3D(Sums, xData[k], yData[k], zData[k], nXterm, nYterm, nZterm);

        double wt;
        if (fErr == NULL) {
            wt = 1.0;
        } else {
            // this filters fErr == 0 values
            wt = (fErr->data.F64[k] == 0.0) ? 0.0 : 1.0 / PS_SQR(fErrData[k]);
        }

        for (int i = 0; i < nTerm; i++) {
            int ix = i / nYZterm; // x index
            int iy = (i % nYZterm) / nZterm; // y index
            int iz = (i % nYZterm) % nZterm; // z index
            if (coeffMask[ix][iy][iz] & PS_POLY_MASK_BOTH) {
                matrix[i][i] = 1.0;
                continue;
            }

            vector[i] += fData[k] * Sums[ix][iy][iz] * wt;
            matrix[i][i] += Sums[2*ix][2*iy][2*iz] * wt;
            for (int j = i + 1; j < nTerm; j++) {
                int jx = j / (nYZterm); // x index
                int jy = (j % nYZterm) / nZterm; // y index
                int jz = (j % nYZterm) % nZterm; // z index
                if (coeffMask[jx][jy][jz] & PS_POLY_MASK_BOTH) {
                    continue;
                }
                double value = Sums[ix+jx][iy+jy][iz+jz] * wt;
                matrix[i][j] += value;
                matrix[j][i] += value;
            }
        }
    }

    // Free the sums
    for (psS32 ix = 0; ix < 2*nXterm; ix++) {
        for (psS32 iy = 0; iy < 2*nYterm; iy++) {
            psFree(Sums[ix][iy]);
        }
        psFree(Sums[ix]);
    }
    psFree(Sums);


    bool status = false;
    if (USE_GAUSS_JORDAN) {
        status = psMatrixGJSolve(A, B);
    } else {
        status = psMatrixLUSolve(A, B);
    }
    if (!status) {
	psError(PS_ERR_UNKNOWN, false, "Could not solve linear equations.\n");
	goto escape;
    } 

    // select the appropriate solution entries
    for (int i = 0; i < nTerm; i++) {
	int ix = i / nYZterm; // x index
	int iy = (i % nYZterm) / nZterm; // y index
	int iz = (i % nYZterm) % nZterm; // z index
	if (coeffMask[ix][iy][iz] & PS_POLY_MASK_FIT) continue;
	myPoly->coeff[ix][iy][iz] = B->data.F64[i];
	myPoly->coeffErr[ix][iy][iz] = sqrt(A->data.F64[i][i]);
    }
    psFree(A);
    psFree(B);
    return true;

escape:
    psFree(A);
    psFree(B);
    return false;
}

/******************************************************************************
psVectorFitPolynomial3D():  This routine fits a 3D polynomial of arbitrary
degree (specified in poly) to the data points (x, y, z)-(f) and returns that
polynomial.  Types F32 and F64 are supported, however, type F32 is done via
vector conversion only.
 *****************************************************************************/
bool psVectorFitPolynomial3D(
    psPolynomial3D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z)
{
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, false);

    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_NON_NULL(z, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, false);
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE_F32_OR_F64(fErr, false);
    }

    // Convert input vectors to F64 if necessary.
    psVector *f64 = (f->type.type == PS_TYPE_F64) ? (psVector *) f : psVectorCopy(NULL, f, PS_TYPE_F64);
    psVector *x64 = (x->type.type == PS_TYPE_F64) ? (psVector *) x : psVectorCopy(NULL, x, PS_TYPE_F64);
    psVector *y64 = (y->type.type == PS_TYPE_F64) ? (psVector *) y : psVectorCopy(NULL, y, PS_TYPE_F64);
    psVector *z64 = (z->type.type == PS_TYPE_F64) ? (psVector *) z : psVectorCopy(NULL, z, PS_TYPE_F64);

    psVector *fErr64 = NULL;
    if (fErr != NULL) {
        fErr64 = (fErr->type.type == PS_TYPE_F64) ? (psVector *) fErr : psVectorCopy(NULL, fErr, PS_TYPE_F64);
    }

    bool result = true;

    switch (poly->type) {
    case PS_POLYNOMIAL_ORD:
        result = VectorFitPolynomial3DOrd(poly, mask, maskValue, f64, fErr64, x64, y64, z64);
        if (!result) {
            psError(PS_ERR_UNKNOWN, true, "Could not fit polynomial.  Returning NULL.\n");
        }
        break;
    case PS_POLYNOMIAL_CHEB:
        if (mask != NULL) {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: ignoring mask and maskValue with Chebyshev polynomials.\n");
        }
        psError(PS_ERR_UNKNOWN, true, "3-D Chebyshev polynomial vector fitting has not been implemented.  Returning NULL.\n");
        result = false;
        break;
    default:
        psError(PS_ERR_UNKNOWN, true, "Incorrect polynomial type.  Returning NULL.\n");
        result = false;
        break;
    }

    // Free psVectors that were created for NULL arguments.
    PS_FREE_TEMP_F64_VECTOR (f, f64);
    PS_FREE_TEMP_F64_VECTOR (x, x64);
    PS_FREE_TEMP_F64_VECTOR (y, y64);
    PS_FREE_TEMP_F64_VECTOR (z, z64);
    PS_FREE_TEMP_F64_VECTOR (fErr, fErr64);

    return result;
}

bool psVectorClipFitPolynomial3D(
    psPolynomial3D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, false);
    PS_ASSERT_PTR_NON_NULL(stats, false);
    PS_ASSERT_VECTOR_NON_NULL(mask, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);

    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_TYPE(x, f->type.type, false);

    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_TYPE(y, f->type.type, false);

    PS_ASSERT_VECTOR_NON_NULL(z, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, false);
    PS_ASSERT_VECTOR_TYPE(z, f->type.type, false);

    PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
    PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);

    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, f->type.type, false);
    }

    // the user supplies one of various stats option pairs,
    // determine the desired mean and stdev STATS options:
    // XXX enforce consistency?
    // XXX psStatsGetValue() probably has inverted precedence
    psStatsOptions meanOption = stats->options & (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN | PS_STAT_ROBUST_MEDIAN | PS_STAT_CLIPPED_MEAN | PS_STAT_FITTED_MEAN | PS_STAT_FITTED_MEAN);
    psStatsOptions stdevOption = stats->options & (PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_STDEV | PS_STAT_CLIPPED_STDEV | PS_STAT_FITTED_STDEV | PS_STAT_FITTED_STDEV);
    if (!meanOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
        return false;
    }
    if (!stdevOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
        return false;
    }

    // clipping range defined by min and max and/or clipSigma
    psF32 minClipSigma;
    psF32 maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = fabs(stats->max);
    } else {
        maxClipSigma = fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = fabs(stats->min);
    } else {
        minClipSigma = fabs(stats->clipSigma);
    }
    psVector *resid = psVectorAlloc(f->n, PS_TYPE_F64);

    psTrace("psLib.math", 4, "stats->clipIter is %d\n", stats->clipIter);
    psTrace("psLib.math", 4, "(minClipSigma, maxClipSigma) is (%.2f, %.2f)\n", minClipSigma, maxClipSigma);

    for (psS32 N = 0; N < stats->clipIter; N++) {
        psTrace("psLib.math", 6, "Loop iteration %d.  Calling psVectorFitPolynomial1D()\n", N);
        psS32 Nkeep = 0;
        if (psTraceGetLevel("psLib.math") >= 6) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    psTrace("psLib.math", 6,  "mask[%d] is %d\n", i, mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
                }
            }
        }

        if (!psVectorFitPolynomial3D(poly, mask, maskValue, f, fErr, x, y, z)) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit a polynomial to the data.  Returning NULL.\n");
            psFree(resid);
            return false;
        }
        psVector *fit = psPolynomial3DEvalVector(poly, x, y, z);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Could not call psPolynomial3DEvalVector().  Returning NULL.\n");
            psFree(resid);
            return false;
        }
        for (psS32 i = 0 ; i < f->n ; i++) {
            if (f->type.type == PS_TYPE_F64) {
                resid->data.F64[i] = f->data.F64[i] - fit->data.F64[i];
            } else {
                resid->data.F64[i] = ((psF64) f->data.F32[i]) - fit->data.F64[i];
            }
        }

        if (psTraceGetLevel("psLib.math") >= 6) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    if (!((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue))) {
                        psTrace("psLib.math", 6,  "(f, fit)[%d] is (%f, %f).  resid is (%f)\n",
                                i, f->data.F32[i], fit->data.F32[i], resid->data.F64[i]);
                    }
                }
            }
        }

        if (!psVectorStats(stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "Could not compute statistics on the resid vector.  Returning NULL.\n");
            psFree(resid);
            psFree(fit);
            return false;
        }

        double meanValue = psStatsGetValue (stats, meanOption);
        double stdevValue = psStatsGetValue (stats, stdevOption);

        psTrace("psLib.math", 5, "Mean is %f\n", meanValue);
        psTrace("psLib.math", 5, "Stdev is %f\n", stdevValue);
        psF32 minClipValue = -minClipSigma*stdevValue;
        psF32 maxClipValue = +maxClipSigma*stdevValue;

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (psS32 i = 0; i < resid->n; i++) {
            if ((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) {
                continue;
            }

            if ((resid->data.F64[i] - meanValue > maxClipValue) || (resid->data.F64[i] - meanValue < minClipValue))  {
                if (f->type.type == PS_TYPE_F64) {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F64[i], i, resid->data.F64[i]);
                } else {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F32[i], i, resid->data.F64[i]);
                }

                if (mask != NULL) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                }
                continue;
            }
            Nkeep++;
        }
        psTrace("psLib.math", 6, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree(fit);
    }
    // Free local temporary variables
    psFree(resid);

    psTrace("psLib.math", 3, "---- %s() end ----\n", __func__);
    return true;
}

/******************************************************************************
 ******************************************************************************
 4-D Vector Code.
 ******************************************************************************
 *****************************************************************************/
/******************************************************************************
VectorFitPolynomial4DOrd(myPoly, *mask, maskValue, *f, *fErr, *x, *y, *z, *t):
This is a private routine which will fit a 4-D polynomial to a set of (x,
y, z, t)-(f) pairs.  All non-NULL vectors must be of type PS_TYPE_F64.
 
 *****************************************************************************/
static bool VectorFitPolynomial4DOrd(
    psPolynomial4D* myPoly,
    const psVector* mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z,
    const psVector *t)
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(myPoly, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nX, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nY, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nZ, false);
    PS_ASSERT_INT_NONNEGATIVE(myPoly->nT, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F64, false);
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, PS_TYPE_F64, false);
    }
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_NON_NULL(z, false);
    PS_ASSERT_VECTOR_TYPE(z, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, false);
    PS_ASSERT_VECTOR_NON_NULL(t, false);
    PS_ASSERT_VECTOR_TYPE(t, PS_TYPE_F64, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, t, false);
    if (mask) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }


    int nXterm = 1 + myPoly->nX;        // Number of x terms
    int nYterm = 1 + myPoly->nY;        // Number of y terms
    int nZterm = 1 + myPoly->nZ;        // Number of z terms
    int nTterm = 1 + myPoly->nT;        // Number of t terms
    int nTerm = nXterm * nYterm * nZterm * nTterm; // Total number of terms
    int nData = x->n;                   // Number of data points
    psImage    *A = psImageAlloc(nTerm, nTerm, PS_TYPE_F64); // Least-squares matrix
    psVector   *B = psVectorAlloc(nTerm, PS_TYPE_F64); // Least-squares vector

    // Initialize data structures.
    if (!psImageInit(A, 0.0) || !psVectorInit(B, 0.0)) {
        psError(PS_ERR_UNKNOWN, false, "Could initialize data structures A, B.  Returning NULL.\n");
        psFree(A);
        psFree(B);
        psTrace("psLib.math", 4, "---- %s() End ----\n", __func__);
        return false;
    }

    // Dereference points for speed in the loop
    psF64 **matrix = A->data.F64;       // Least-squares matrix
    psF64 *vector = B->data.F64;        // Least-squares vector
    psF64 *xData = x->data.F64;         // x
    psF64 *yData = y->data.F64;         // y
    psF64 *zData = z->data.F64;         // z
    psF64 *tData = t->data.F64;         // t
    psF64 *fData = f->data.F64;         // f
    psF64 *fErrData = NULL;             // Error in f
    if (fErr) {
        fErrData = fErr->data.F64;
    }
    psVectorMaskType *dataMask = NULL;              // Mask for data
    if (mask) {
        dataMask = mask->data.PS_TYPE_VECTOR_MASK_DATA;
    }
    psMaskType ****coeffMask = myPoly->coeffMask;    // Mask for polynomial terms
    int nYZTterm = nYterm * nZterm * nTterm; // Multiplication of the numbers, for calculating the index
    int nZTterm = nZterm * nTterm;      // Multiplication of the numbers, for calculating the index

    // Build the B and A data structs.
    psF64 ****Sums = NULL;        // Sums look like: 1, x, x^2, ... x^(2n+1), y, xy, x^2y, ... x^(2n+1)*y, ...
    for (int k = 0; k < nData; k++) {
        if (dataMask && dataMask[k] & maskValue) {
            continue;
        }

        Sums = BuildSums4D(Sums, xData[k], yData[k], zData[k], tData[k], nXterm, nYterm, nZterm, nTterm);

        double wt;
        if (fErr == NULL) {
            wt = 1.0;
        } else {
            // this filters fErr == 0 values
            wt = (fErr->data.F64[k] == 0.0) ? 0.0 : 1.0 / PS_SQR(fErrData[k]);
        }

        for (int i = 0; i < nTerm; i++) {
            int ix = i / (nYZTterm); // x index
            int iy = (i % (nYZTterm)) / (nZTterm); // y index
            int iz = ((i % (nYZTterm)) % (nZTterm)) / nTterm; // z index
            int it = ((i % (nYZTterm)) % (nZTterm)) % nTterm; // t index
            if (coeffMask[ix][iy][iz][it] & PS_POLY_MASK_BOTH) {
                matrix[i][i] = 1.0;
                continue;
            }

            vector[i] += fData[k] * Sums[ix][iy][iz][it] * wt;
            matrix[i][i] += Sums[2*ix][2*iy][2*iz][2*it] * wt;
            for (int j = i + 1; j < nTerm; j++) {
                int jx = j / nYZTterm; // x index
                int jy = (j % nYZTterm) / nZTterm; // y index
                int jz = ((j % nYZTterm) % nZTterm) / nTterm; // z index
                int jt = ((j % nYZTterm) % nZTterm) % nTterm; // t index
                if (coeffMask[jx][jy][jz][jt] & PS_POLY_MASK_BOTH) {
                    continue;
                }
                double value = Sums[ix+jx][iy+jy][iz+jz][it+jt] * wt;
                matrix[i][j] += value;
                matrix[j][i] += value;
            }
        }
    }

    // Free the sums
    if (Sums == NULL) {
        assert (nData == 0);
    } else {
        for (int ix = 0; ix < 2*nXterm; ix++) {
            for (int iy = 0; iy < 2*nYterm; iy++) {
                for (int iz = 0; iz < 2*nZterm; iz++) {
                    psFree(Sums[ix][iy][iz]);
                }
                psFree(Sums[ix][iy]);
            }
            psFree(Sums[ix]);
        }
        psFree(Sums);
    }

    bool status = false;
    if (USE_GAUSS_JORDAN) {
        status = psMatrixGJSolve(A, B);
    } else {
        status = psMatrixLUSolve(A, B);
    }
    if (!status) {
	psError(PS_ERR_UNKNOWN, false, "Could not solve linear equations.\n");
	goto escape;
    } 

    // select the appropriate solution entries
    for (int i = 0; i < nTerm; i++) {
	int ix = i / nYZTterm; // x index
	int iy = (i % nYZTterm) / nZTterm; // y index
	int iz = ((i % nYZTterm) % nZTterm) / nTterm; // z index
	int it = ((i % nYZTterm) % nZTterm) % nTterm; // t index
	if (coeffMask[ix][iy][iz][it] & PS_POLY_MASK_FIT) continue;
	myPoly->coeff[ix][iy][iz][it] = B->data.F64[i];
	myPoly->coeffErr[ix][iy][iz][it] = sqrt(A->data.F64[i][i]);
    }
    psFree(A);
    psFree(B);
    return true;

escape:
    psFree(A);
    psFree(B);
    return false;
}

/******************************************************************************
psVectorFitPolynomial4D():  This routine fits a 4D polynomial of arbitrary
degree (specified in poly) to the data points (x, y, z, t)-(f) and returns
that polynomial.  Types F32 and F64 are supported, however, type F32 is done
via vector conversion only.
 *****************************************************************************/
bool psVectorFitPolynomial4D(
    psPolynomial4D *poly,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z,
    const psVector *t)
{
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, false);

    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_NON_NULL(z, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, false);
    PS_ASSERT_VECTOR_NON_NULL(t, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, t, false);
    if (mask) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE_F32_OR_F64(fErr, false);
    }

    // Convert input vectors to F64 if necessary.
    psVector *f64 = (f->type.type == PS_TYPE_F64) ? (psVector *) f : psVectorCopy(NULL, f, PS_TYPE_F64);
    psVector *x64 = (x->type.type == PS_TYPE_F64) ? (psVector *) x : psVectorCopy(NULL, x, PS_TYPE_F64);
    psVector *y64 = (y->type.type == PS_TYPE_F64) ? (psVector *) y : psVectorCopy(NULL, y, PS_TYPE_F64);
    psVector *z64 = (z->type.type == PS_TYPE_F64) ? (psVector *) z : psVectorCopy(NULL, z, PS_TYPE_F64);
    psVector *t64 = (t->type.type == PS_TYPE_F64) ? (psVector *) t : psVectorCopy(NULL, t, PS_TYPE_F64);

    psVector *fErr64 = NULL;
    if (fErr != NULL) {
        fErr64 = (fErr->type.type == PS_TYPE_F64) ? (psVector *) fErr : psVectorCopy(NULL, fErr, PS_TYPE_F64);
    }

    bool result = true;

    switch (poly->type) {
    case PS_POLYNOMIAL_ORD:
        result = VectorFitPolynomial4DOrd(poly, mask, maskValue, f64, fErr64, x64, y64, z64, t64);
        if (!result) {
            psError(PS_ERR_UNKNOWN, true, "Could not fit polynomial.  Returning NULL.\n");
        }
        break;
    case PS_POLYNOMIAL_CHEB:
        if (mask != NULL) {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: ignoring mask and maskValue with Chebyshev polynomials.\n");
        }
        psError(PS_ERR_UNKNOWN, true, "4-D Chebyshev polynomial vector fitting has not been implemented.  Returning NULL.\n");
        result = false;
        break;
    default:
        psError(PS_ERR_UNKNOWN, true, "Incorrect polynomial type.  Returning NULL.\n");
        result = false;
        break;
    }

    // Free psVectors that were created for NULL arguments.
    PS_FREE_TEMP_F64_VECTOR (f, f64);
    PS_FREE_TEMP_F64_VECTOR (x, x64);
    PS_FREE_TEMP_F64_VECTOR (y, y64);
    PS_FREE_TEMP_F64_VECTOR (z, z64);
    PS_FREE_TEMP_F64_VECTOR (t, t64);
    PS_FREE_TEMP_F64_VECTOR (fErr, fErr64);

    return result;
}


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
    const psVector *t)
{
    psTrace("psLib.math", 3, "---- %s() begin ----\n", __func__);
    PS_ASSERT_POLY_NON_NULL(poly, false);
    PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, false);
    PS_ASSERT_PTR_NON_NULL(stats, false);
    PS_ASSERT_VECTOR_NON_NULL(mask, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, false);

    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, false);
    PS_ASSERT_VECTOR_TYPE(x, f->type.type, false);

    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, false);
    PS_ASSERT_VECTOR_TYPE(y, f->type.type, false);

    PS_ASSERT_VECTOR_NON_NULL(z, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, false);
    PS_ASSERT_VECTOR_TYPE(z, f->type.type, false);

    PS_ASSERT_VECTOR_NON_NULL(t, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, t, false);
    PS_ASSERT_VECTOR_TYPE(t, f->type.type, false);

    PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, false);
    PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);

    if (fErr != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, fErr, false);
        PS_ASSERT_VECTOR_TYPE(fErr, f->type.type, false);
    }

    // the user supplies one of various stats option pairs,
    // determine the desired mean and stdev STATS options:
    // XXX enforce consistency?
    // XXX psStatsGetValue() probably has inverted precedence
    psStatsOptions meanOption = stats->options & (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN | PS_STAT_ROBUST_MEDIAN | PS_STAT_CLIPPED_MEAN | PS_STAT_FITTED_MEAN | PS_STAT_FITTED_MEAN);
    psStatsOptions stdevOption = stats->options & (PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_STDEV | PS_STAT_CLIPPED_STDEV | PS_STAT_FITTED_STDEV | PS_STAT_FITTED_STDEV);
    if (!meanOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
        return false;
    }
    if (!stdevOption) {
        psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
        return false;
    }

    // clipping range defined by min and max and/or clipSigma
    psF32 minClipSigma;
    psF32 maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = fabs(stats->max);
    } else {
        maxClipSigma = fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = fabs(stats->min);
    } else {
        minClipSigma = fabs(stats->clipSigma);
    }
    psVector *resid = psVectorAlloc(f->n, PS_TYPE_F64);

    psTrace("psLib.math", 4, "stats->clipIter is %d\n", stats->clipIter);
    psTrace("psLib.math", 4, "(minClipSigma, maxClipSigma) is (%.2f, %.2f)\n", minClipSigma, maxClipSigma);

    for (psS32 N = 0; N < stats->clipIter; N++) {
        psTrace("psLib.math", 6, "Loop iteration %d.  Calling psVectorFitPolynomial4D()\n", N);
        psS32 Nkeep = 0;
        if (psTraceGetLevel("psLib.math") >= 6) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    psTrace("psLib.math", 6,  "mask[%d] is %d\n", i, mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
                }
            }
        }

        if (!psVectorFitPolynomial4D (poly, mask, maskValue, f, fErr, x, y, z, t)) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit a polynomial to the data.  Returning NULL.\n");
            psFree(resid);
            return false;
        }

        psVector *fit = psPolynomial4DEvalVector (poly, x, y, z, t);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Could not call psPolynomial4DEvalVector().  Returning NULL.\n");
            psFree(resid);
            return false;
        }
        for (psS32 i = 0 ; i < f->n ; i++) {
            if (f->type.type == PS_TYPE_F64) {
                resid->data.F64[i] = f->data.F64[i] - fit->data.F64[i];
            } else {
                resid->data.F64[i] = ((psF64) f->data.F32[i]) - fit->data.F64[i];
            }
        }

        if (psTraceGetLevel("psLib.math") >= 6) {
            if (mask != NULL) {
                for (psS32 i = 0 ; i < mask->n ; i++) {
                    if (!((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue))) {
                        psTrace("psLib.math", 6,  "(f, fit)[%d] is (%f, %f).  resid is (%f)\n",
                                i, f->data.F32[i], fit->data.F32[i], resid->data.F64[i]);
                    }
                }
            }
        }

        if (!psVectorStats(stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "Could not compute statistics on the resid vector.  Returning NULL.\n");
            psFree(resid);
            psFree(fit);
            return false;
        }

        double meanValue = psStatsGetValue (stats, meanOption);
        double stdevValue = psStatsGetValue (stats, stdevOption);

        psTrace("psLib.math", 5, "Mean is %f\n", meanValue);
        psTrace("psLib.math", 5, "Stdev is %f\n", stdevValue);
        psF32 minClipValue = -minClipSigma*stdevValue;
        psF32 maxClipValue = +maxClipSigma*stdevValue;

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (psS32 i = 0; i < resid->n; i++) {
            if ((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) {
                continue;
            }

            if ((resid->data.F64[i] - meanValue > maxClipValue) || (resid->data.F64[i] - meanValue < minClipValue)) {
                if (f->type.type == PS_TYPE_F64) {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F64[i], i, resid->data.F64[i]);
                } else {
                    psTrace("psLib.math", 6, "Masking element %d (%f).  resid->data.F64[%d] is %f\n",
                            i, fit->data.F32[i], i, resid->data.F64[i]);
                }

                if (mask != NULL) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                }
                continue;
            }
            Nkeep++;
        }
        psTrace("psLib.math", 6, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree (fit);
    }
    // Free local temporary variables
    psFree (resid);

    psTrace("psLib.math", 3, "---- %s() end ----\n", __func__);
    return true;
}

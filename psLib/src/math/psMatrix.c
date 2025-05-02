/** @file  psMatrix.c
 *
 *  @brief Provides functions for linear algebra operations on psImages and psVectors.
 *
 *  Functions are provided to:
 *      Transpose a psImage
 *      Compute LUD
 *      Solve LUD
 *      Matrix inversion
 *      Calculate determinant
 *      Matrix addition
 *      Matrix subtraction
 *      Matrix multiplication
 *      Calculate Eigenvectors
 *      Convert matrix to vector
 *      Convert vector to matrix
 *
 *  These functions treat psImages as if they were matrices, therefore there is no psMatrix.
 *
 *  @author Ross Harman, MHPCC
 *  @author Robert DeSonia, MHPCC
 *  @author Andy Becker, University of Washington (SVD).
 *
 *  @version $Revision: 1.58 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-02 20:48:12 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

// XXX Future optimisation: use GSL type appropriate to the psLib type, and use memcpy instead of copying each
// value individually.


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/
#include <string.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_eigen.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psError.h"
#include "psImage.h"
#include "psVector.h"
#include "psMatrix.h"
#include "psAssert.h"

#include "psTrace.h"

/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/

/** Preprocessor macro to generate error for image dimensionality not set to PS_DIMEN_IMAGE */
#define PS_CHECK_DIMEN_AND_TYPE(NAME, PS_DIMEN, CLEANUP)                                             \
if (NAME->type.dimen != PS_DIMEN) {                                                                 \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true,                                                        \
            "Invalid operation. %s has incorrect dimensionality %d.", #NAME, PS_DIMEN);             \
    CLEANUP;                                                                                  \
} else if(NAME->type.type!=PS_TYPE_F64 && NAME->type.type!=PS_TYPE_F32) {                           \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true,                                                        \
            "Invalid operation. %s not PS_TYPE_F64.", #NAME);                                       \
    CLEANUP;                                                                                  \
}

/** Preprocessor macro to check that input is not equal to output */
#define PS_CHECK_POINTERS(NAME1, NAME2, CLEANUP)                                                     \
if (NAME1 == NAME2) {                                                                               \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true,                                                       \
            "Invalid operation: Pointer to %s is same as %s.", #NAME1, #NAME2);                     \
    CLEANUP;                                                                                  \
}

/** Preprocessor macro to check that an image is square */
#define PS_CHECK_SQUARE(NAME, CLEANUP)                                                               \
if (NAME->numCols != NAME->numRows) {                                                               \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid operation: %s not square array.", #NAME);     \
    CLEANUP;                                                                                  \
}

/** Preprocessor macro to initalize a GSL matrix. */
#define PS_GSL_MATRIX_INITIALIZE(LHS_NAME, RHS_NAME)                                                \
LHS_NAME.size1 = numRows;                                                                           \
LHS_NAME.size2 = numCols;                                                                           \
LHS_NAME.tda   = numCols;                                                                           \
LHS_NAME.data  = RHS_NAME;

////////////////////////////////////////////////////////////////////////////////
// Conversion functions
////////////////////////////////////////////////////////////////////////////////

// gsl_vector holds *doubles*, so we can directly copy F64, but need to convert F32

/** Static function to copy psF32 or psF64 vector data to a GSL vector */
static void vectorPStoGSL(gsl_vector *out, const psVector *in)
{
    psAssert(out->size == in->n, "Sizes don't match!");

    long n = in->n;                     // Size of input
    switch (in->type.type) {
      case PS_TYPE_F32:
        for (long i = 0; i < n; i++) {
            out->data[i] = in->data.F32[i];
        }
        break;
      case PS_TYPE_F64:
        memcpy(out->data, in->data.F64, n * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        break;
      default:
        psAbort("Unsupported vector type: %x\n", in->type.type);
    }
    return;
}

/** Static function to copy GSL vector data to a psF32 or psF64 vector */
static void vectorGSLtoPS(psVector *out, const gsl_vector *in)
{
    psAssert(in->size == out->n, "Sizes don't match!");

    long n = out->n;                    // Size of output
    switch (out->type.type) {
      case PS_TYPE_F32:
        for (long i = 0; i < n; i++) {
            out->data.F32[i] = in->data[i];
        }
        break;
      case PS_TYPE_F64:
        memcpy(out->data.F64, in->data, n * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        break;
      default:
        psAbort("Unsupported vector type: %x\n", out->type.type);
    }
    return;
}


/** Static function to copy psF32 or psF64 image data to a GSL matrix */
static void matrixPStoGSL(gsl_matrix *out, const psImage *in)
{
    psAssert(out->size1 == in->numRows && out->size2 == in->numCols, "Sizes don't match!");

    int numCols = in->numCols, numRows = in->numRows; // Size of matrix
    switch (in->type.type) {
      case PS_TYPE_F32:
        for (int y = 0, i = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++, i++) {
                out->data[i] = in->data.F32[y][x];
            }
        }
        break;
      case PS_TYPE_F64:
        if (in->parent|| out->tda != out->size1) {
            for (int y = 0, i = 0; y < numRows; y++, i += numCols) {
                memcpy(&out->data[i], in->data.F64[y], numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
            }
        } else {
            memcpy(out->data, in->p_rawDataBuffer, numCols * numRows * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        }
        break;
      default:
        psAbort("Unsupported vector type: %x\n", in->type.type);
    }
    return;
}

/** Static function to copy GSL matrix data to a psF32 or psF64 image */
static void matrixGSLtoPS(psImage *out, const gsl_matrix *in)
{
    psAssert(in->size1 == out->numRows && in->size2 == out->numCols, "Sizes don't match!");

    int numCols = out->numCols, numRows = out->numRows; // Size of matrix
    switch (out->type.type) {
      case PS_TYPE_F32:
        for (int y = 0, i = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++, i++) {
                out->data.F32[y][x] = in->data[i];
            }
        }
        break;
      case PS_TYPE_F64:
        if (out->parent || in->tda != in->size1) {
            for (int y = 0, i = 0; y < numRows; y++, i += numCols) {
                memcpy(out->data.F64[y], &in->data[i], numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
            }
        } else {
            memcpy(out->p_rawDataBuffer, in->data, numCols * numRows * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        }
        break;
      default:
        psAbort("Unsupported vector type: %x\n", out->type.type);
    }
    return;
}


/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

psImage* psMatrixLUDecomposition(psImage* out,
                     psVector** perm,
                     const psImage* in)
{
    psS32 signum = 0;
    psS32 numRows = 0;
    psS32 numCols = 0;
    gsl_matrix *lu = NULL;
    gsl_permutation permGSL;


    #define psMatrixLUDecomposition_EXIT {psFree(out); return NULL;}

    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in, psMatrixLUDecomposition_EXIT);
    PS_CHECK_POINTERS(in, out, psMatrixLUDecomposition_EXIT);
    PS_CHECK_DIMEN_AND_TYPE(in, PS_DIMEN_IMAGE, psMatrixLUDecomposition_EXIT);
    PS_ASSERT_GENERAL_PTR_NON_NULL(perm, psMatrixLUDecomposition_EXIT);

    out = psImageRecycle(out, in->numCols, in->numRows, in->type.type);

    PS_CHECK_SQUARE(in, psMatrixLUDecomposition_EXIT); // gsl_linalg_LU_decomp would fail on non-square input.
    PS_CHECK_SQUARE(out, psMatrixLUDecomposition_EXIT);

    // Initialize data
    numRows = in->numRows;
    numCols = in->numCols;

    // Initialize GSL data
    permGSL.size = numCols;
    if (sizeof(size_t) == 4) {
        *perm = psVectorRecycle(*perm, numCols, PS_TYPE_S32);
    } else if (sizeof(size_t) == 8) {
        *perm = psVectorRecycle(*perm, numCols, PS_TYPE_S64);
    } else {
        psError(PS_ERR_UNKNOWN, true,
                "Failed to allocate the permutation vector; "
                "could not determine the cooresponding data type.");
        psMatrixLUDecomposition_EXIT;
    }

    (*perm)->n = numCols;
    permGSL.data = (psPtr)((*perm)->data.U8);
    lu = gsl_matrix_alloc(numRows, numCols);

    // Copy psImage data into GSL matrix data
    matrixPStoGSL(lu, in);

    // Calculate LU decomposition
    gsl_linalg_LU_decomp(lu, &permGSL, &signum); // N.B., uses Gaussian Elimination with partial pivoting.

    // Copy GSL matrix data to psImage data
    matrixGSLtoPS(out, lu);

    // Free GSL data
    gsl_matrix_free(lu);

    return out;
}

psVector* psMatrixLUSolution(psVector* out,
                          const psImage* LU,
                          const psVector* RHS,
                          const psVector* perm)
{
    psS32 numRows = 0;
    psS32 numCols = 0;
    gsl_matrix *lu;
    gsl_permutation permGSL;
    gsl_vector *b = NULL;
    gsl_vector *x = NULL;

    #define LUSOLVE_CLEANUP {psFree(out); return NULL;}

    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(LU, LUSOLVE_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(LU, PS_DIMEN_IMAGE, LUSOLVE_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(LU, LUSOLVE_CLEANUP);
    PS_ASSERT_GENERAL_VECTOR_NON_NULL(RHS, LUSOLVE_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(RHS, PS_DIMEN_VECTOR, LUSOLVE_CLEANUP);
    PS_ASSERT_GENERAL_VECTOR_NON_NULL(perm, LUSOLVE_CLEANUP);

    out = psVectorRecycle(out, LU->numRows, LU->type.type);

    PS_CHECK_POINTERS(out, RHS, LUSOLVE_CLEANUP);
    PS_CHECK_POINTERS(RHS, perm, LUSOLVE_CLEANUP);
    PS_CHECK_POINTERS(out, perm, LUSOLVE_CLEANUP);

    // Initialize data
    numRows = LU->numRows;
    numCols = LU->numCols;

    // Initialize GSL data
    lu = gsl_matrix_alloc(numRows, numCols);
    matrixPStoGSL(lu, LU);
    b = gsl_vector_alloc(RHS->n);
    vectorPStoGSL(b, RHS);
    x = gsl_vector_alloc(RHS->n);

    out->n = numCols;
    permGSL.size = perm->n;
    permGSL.data = (psPtr)(perm->data.U8);

    // Solve for {x} in equation: {b} = [A]{x}
    gsl_linalg_LU_solve(lu, &permGSL, b, x);

    // Copy GSL vector data to psVector data
    vectorGSLtoPS(out, x);

    // Free GSL data
    gsl_vector_free(b);
    gsl_vector_free(x);
    gsl_matrix_free(lu);

    return out;
}

psImage *psMatrixLUInvert(psImage *out,
                          const psImage* LU,
                          const psVector* perm)
{
    psS32 numRows = 0;
    psS32 numCols = 0;
    gsl_matrix *lu, *inverse;
    gsl_permutation permGSL;

    #define LUSOLVE_CLEANUP {psFree(out); return NULL;}

    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(LU, LUSOLVE_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(LU, PS_DIMEN_IMAGE, LUSOLVE_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(LU, LUSOLVE_CLEANUP);
    PS_ASSERT_GENERAL_VECTOR_NON_NULL(perm, LUSOLVE_CLEANUP);

    out = psImageRecycle(out, LU->numCols, LU->numRows, LU->type.type);

    // Initialize data
    numRows = LU->numRows;
    numCols = LU->numCols;

    // Initialize GSL data
    lu = gsl_matrix_alloc(numRows, numCols);
    matrixPStoGSL(lu, LU);

    permGSL.size = perm->n;
    permGSL.data = (psPtr)(perm->data.U8);

    inverse = gsl_matrix_alloc(numRows, numCols);

    // Solve for {x} in equation: {b} = [A]{x}
    gsl_linalg_LU_invert(lu, &permGSL, inverse);

    // Copy GSL vector data to psVector data
    matrixGSLtoPS(out, inverse);

    // Free GSL data
    gsl_matrix_free(lu);
    gsl_matrix_free(inverse);

    return out;
}

// This is the LU Decomposition version of the matrix equation solver.  It solves the equation
// Ax = B, where A is a square matrix (NxN) and B is a vector of length N.  This solver only
// yields the solution for x, which is returned to the vector B.  This now DOES calculate the
// inverse of A.  A and B may be F32 or F64  XXX can they differ?
bool psMatrixLUSolve(psImage *a,
                     psVector *b
                    )
{
    PS_ASSERT_IMAGE_NON_NULL(a, false);
    PS_ASSERT_VECTOR_NON_NULL(b, false);
    PS_ASSERT_IMAGE_TYPE_F32_OR_F64(a, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(b, false);
    PS_ASSERT_INT_EQUAL(a->numCols, a->numRows, false);
    PS_ASSERT_VECTOR_SIZE(b, (long int)a->numCols, false);

    // Check for non-finite entries in matrix
    switch (a->type.type) {
      case PS_TYPE_F32: {
          psF32 **values = a->data.F32; /* Dereference */
          int numCols = a->numCols, numRows = a->numRows; /* Size of matrix */
          for (int i = 0; i < numRows; i++) {
              for (int j = 0; j < numCols; j++) {
                  if (!isfinite(values[i][j])) {
                      // psError(PS_ERR_BAD_PARAMETER_VALUE, 3,
                      // "Input matrix contains non-finite elements: matrix[%d][%d] is %.2f\n",
                      // i, j, values[i][j]);
                      return false;
                  }
              }
          }
          break;
      }
      case PS_TYPE_F64: {
          psF64 **values = a->data.F64; /* Dereference */
          int numCols = a->numCols, numRows = a->numRows; /* Size of matrix */
          for (int i = 0; i < numRows; i++) {
              for (int j = 0; j < numCols; j++) {
                  if (!isfinite(values[i][j])) {
                      // psError(PS_ERR_BAD_PARAMETER_VALUE, 3,
                      // "Input matrix contains non-finite elements: matrix[%d][%d] is %.2f\n",
                      // i, j, values[i][j]);
                      return false;
                  }
              }
          }
          break;
      }
        // MATRIX_CHECK_NONFINITE_CASE(F32, a);
        // MATRIX_CHECK_NONFINITE_CASE(F64, a);
      default:
        psAbort("Should never get here.");
    }

    // Decompose the matrix and solve
    psVector *perm = NULL;              // Permutation vector
    psImage *lu = psMatrixLUDecomposition(NULL, &perm, a); // LU decomposed matrix
    if (!lu) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate LU decomposed matrix");
        psFree(perm);
        return false;
    }
    psVector *ans = psMatrixLUSolution(NULL, lu, b, perm); // Answer

    // invert the matrix : check here for an ill-conditioned matrix?
    psMatrixLUInvert (a, lu, perm);

    psFree(lu);
    psFree(perm);
    if (!ans) {
        psError(PS_ERR_UNKNOWN, false, "Unable to solve matrix equation.");
        return false;
    }

    memcpy(b->data.U8, ans->data.U8, b->n * PSELEMTYPE_SIZEOF(ans->type.type));
    psFree(ans);

    return true;
}

// This is the Gauss-Jordan elimination version of the matrix equation solver.  It solves the
// equation Ax = B, where A is a square matrix (NxN) and B is a vector of length N.  This
// solver calculates both the solution for x, which is returned to the vector B and the inverse
// of A (returned in A).  A and B may be F32 or F64, but must match.

// Gauss-Jordan elimination using full pivots based on William Kahan's BASIC example and Press
// et al's description.  Substantially reworked for psLib style: major modifications to conform
// to C indexing, use a boolean to track the completed pivot rows and catch the singular matrix
// early on.  Also, much cleaner control loops than the Press implementation.

// (based on version by William Kahan -- see Ohana/src/libohana/doc/kahan-gji.pdf)

# define MAX_RANGE 1.0e7
// MAX_RANGE is used to test for ill-conditioned input matrices.  For an ill-conditioned
// matrix, one or more of the pivots trends towards zero, and growth goes to infinity.  Rather
// than allow this to go to the numerical precision, I am raising an error if |growth| > MAX_RANGE
bool psMatrixGJSolve(psImage *a,
                     psVector *b
    )
{
    PS_ASSERT_IMAGE_NON_NULL(a, false);
    PS_ASSERT_VECTOR_NON_NULL(b, false);
    PS_ASSERT_IMAGE_TYPE_F32_OR_F64(a, false);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(b, false);
    psAssert (a->type.type == b->type.type, "types must match for now");
    PS_ASSERT_INT_EQUAL(a->numCols, a->numRows, false);
    PS_ASSERT_VECTOR_SIZE(b, (long int)a->numCols, false);

    // Check for non-finite entries in matrix
    switch (a->type.type) {
      case PS_TYPE_F32: {
          psF32 **values = a->data.F32; /* Dereference */
          int numCols = a->numCols, numRows = a->numRows; /* Size of matrix */
          for (int i = 0; i < numRows; i++) {
              for (int j = 0; j < numCols; j++) {
                  if (!isfinite(values[i][j])) {
                      return false;
                  }
              }
          }
          break;
      }
      case PS_TYPE_F64: {
          psF64 **values = a->data.F64; /* Dereference */
          int numCols = a->numCols, numRows = a->numRows; /* Size of matrix */
          for (int i = 0; i < numRows; i++) {
              for (int j = 0; j < numCols; j++) {
                  if (!isfinite(values[i][j])) {
                      return false;
                  }
              }
          }
          break;
      }
      default:
        psAbort("Should never get here.");
    }

    // Following the algorithm laid out by Press et al., we loop along the matrix diagonal, but
    // we do not operate on the diagonal elements in order.  Instead, we are looking for the
    // current max element and operating on that diagonal element.  This is effectively column
    // pivoting.  Row pivoting is perfomed explicitly.

    int nSquare = a->numCols;

    psVector *colIndexV = psVectorAlloc(nSquare, PS_TYPE_S32);
    psVector *rowIndexV = psVectorAlloc(nSquare, PS_TYPE_S32);
    psVector *pivotV    = psVectorAlloc(nSquare, PS_TYPE_S32);
    psVectorInit (pivotV, 0.0);

    if (a->type.type == PS_TYPE_F32) {
        psF32 **A = a->data.F32;
        psF32  *B = b->data.F32;
        int *colIndex = colIndexV->data.S32;
        int *rowIndex = rowIndexV->data.S32;
        int *pivot    = pivotV->data.S32;
        psF32 growth = 1.0;

        for (int diag = 0; diag < nSquare; diag++) {

            psF32 maxval = 0.0;
            int maxrow = 0;
            int maxcol = 0;

            // search for the next pivot
            for (int row = 0; row < nSquare; row++) {
                if (!isfinite(A[row][diag])) goto escape;

                // if we have already operated on this row (pivot[row] is true), skip it
                if (pivot[row]) continue;

                // if we have not yet operated on this row (pivot[row] is false), look for pivot for this row
                for (int col = 0; col < nSquare; col++) {
                    if (pivot[col]) continue;
                    if (fabs (A[row][col]) < maxval) continue;
                    maxval = fabs (A[row][col]);
                    maxrow = row;
                    maxcol = col;
                }
            }

            // if pivot[maxcol] is set, we have already done this row: this implies a singular matrix
            if (pivot[maxcol]) goto escape;
            pivot[maxcol] = 1;

            // if the selected pivot is off the diagonal, do a row swap
            if (maxrow != maxcol) {
                for (int col = 0; col < nSquare; col++) PS_SWAP (A[maxrow][col], A[maxcol][col]);
                PS_SWAP (B[maxrow], B[maxcol]);
            }
            rowIndex[diag] = maxrow;
            colIndex[diag] = maxcol;
            if (A[maxcol][maxcol] == 0.0) goto escape;
            // Kahan replaces the 0.0 pivot with epsilon*(largest element in column) + underFlow.
            // Here we are going to raise an error if the dynamic range is too large.

            /* rescale by pivot reciprocal */
            psF32 tmpval = 1.0 / A[maxcol][maxcol];
            A[maxcol][maxcol] = 1.0;
            for (int col = 0; col < nSquare; col++) A[maxcol][col] *= tmpval;
            B[maxcol] *= tmpval;

            // check for ill-conditioned matrix: measure the pivot growth and trigger on over/under flow
            growth *= tmpval;
            psTrace ("psLib.math", 4, "growth : %e\n", growth);
            if (fabs(growth) > MAX_RANGE) goto escape;

            /* adjust the elements above the pivot */
            for (int row = 0; row < nSquare; row++) {
                if (row == maxcol) continue;
                tmpval = A[row][maxcol];
                A[row][maxcol] = 0.0;
                for (int col = 0; col < nSquare; col++) A[row][col] -= A[maxcol][col]*tmpval;
                B[row] -= B[maxcol]*tmpval;
            }
        }

        // swap back the inverse matrix based on the row swaps above
        for (int col = nSquare - 1; col >= 0; col--) {
            if (rowIndex[col] != colIndex[col]) {
                for (int row = 0; row < nSquare; row++) PS_SWAP (A[row][rowIndex[col]], A[row][colIndex[col]]);
            }
        }
    } else {
        psF64 **A = a->data.F64;
        psF64  *B = b->data.F64;
        int *colIndex = colIndexV->data.S32;
        int *rowIndex = rowIndexV->data.S32;
        int *pivot    = pivotV->data.S32;
        psF64 growth = 1.0;

        for (int diag = 0; diag < nSquare; diag++) {

            psF64 maxval = 0.0;
            int maxrow = 0;
            int maxcol = 0;

            // search for the next pivot
            for (int row = 0; row < nSquare; row++) {
                if (!isfinite(A[row][diag])) goto escape;

                // if we have already operated on this row (pivot[row] is true), skip it
                if (pivot[row]) continue;

                // if we have not yet operated on this row (pivot[row] is false), look for pivot for this row
                for (int col = 0; col < nSquare; col++) {
                    if (pivot[col]) continue;
                    if (fabs (A[row][col]) < maxval) continue;
                    maxval = fabs (A[row][col]);
                    maxrow = row;
                    maxcol = col;
                }
            }

            // if pivot[maxcol] is set, we have already done this row: this implies a singular matrix
            if (pivot[maxcol]) goto escape;
            pivot[maxcol] = 1;

            // if the selected pivot is off the diagonal, do a row swap
            if (maxrow != maxcol) {
                for (int col = 0; col < nSquare; col++) PS_SWAP (A[maxrow][col], A[maxcol][col]);
                PS_SWAP (B[maxrow], B[maxcol]);
            }
            rowIndex[diag] = maxrow;
            colIndex[diag] = maxcol;
            if (A[maxcol][maxcol] == 0.0) goto escape;
            // Kahan replaces the 0.0 pivot with epsilon*(largest element in column) + underFlow.
            // Here we are going to raise an error if the dynamic range is too large.

            /* rescale by pivot reciprocal */
            psF64 tmpval = 1.0 / A[maxcol][maxcol];
            A[maxcol][maxcol] = 1.0;
            for (int col = 0; col < nSquare; col++) A[maxcol][col] *= tmpval;
            B[maxcol] *= tmpval;

            // check for ill-conditioned matrix: measure the pivot growth and trigger on over/under flow
            growth *= tmpval;
            psTrace ("psLib.math", 4, "growth : %e\n", growth);
            if (fabs(growth) > MAX_RANGE) goto escape;

            /* adjust the elements above the pivot */
            for (int row = 0; row < nSquare; row++) {
                if (row == maxcol) continue;
                tmpval = A[row][maxcol];
                A[row][maxcol] = 0.0;
                for (int col = 0; col < nSquare; col++) A[row][col] -= A[maxcol][col]*tmpval;
                B[row] -= B[maxcol]*tmpval;
            }
        }

        // swap back the inverse matrix based on the row swaps above
        for (int col = nSquare - 1; col >= 0; col--) {
            if (rowIndex[col] != colIndex[col]) {
                for (int row = 0; row < nSquare; row++) PS_SWAP (A[row][rowIndex[col]], A[row][colIndex[col]]);
            }
        }
    }

    psFree (pivotV);
    psFree (rowIndexV);
    psFree (colIndexV);
    return true;

escape:
    psFree (pivotV);
    psFree (rowIndexV);
    psFree (colIndexV);
    return false;
}

psImage* psMatrixInvert(psImage* out,
                        const psImage* in,
                        float *determinant)
{
    psS32 signum = 0;
    psS32 numRows = 0;
    psS32 numCols = 0;
    gsl_matrix *inv = NULL;
    gsl_matrix *lu = NULL;
    gsl_permutation *perm = NULL;

    #define INVERT_CLEANUP { psFree(out); return NULL; }
    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in, INVERT_CLEANUP);
    PS_CHECK_POINTERS(in, out, INVERT_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(in, PS_DIMEN_IMAGE, INVERT_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(in, INVERT_CLEANUP);

    out = psImageRecycle(out, in->numCols, in->numRows, in->type.type);

    PS_CHECK_SQUARE(in, INVERT_CLEANUP);
    PS_CHECK_SQUARE(out, INVERT_CLEANUP);

    // Initialize data
    numRows = in->numRows;
    numCols = in->numCols;

    // Initialize GSL data
    perm = gsl_permutation_alloc(numRows);
    lu = gsl_matrix_alloc(numRows, numCols);
    inv = gsl_matrix_alloc(numRows, numCols);
    matrixPStoGSL(lu, in);

    // Invert data and calculate determinant
    gsl_linalg_LU_decomp(lu, perm, &signum);
    gsl_linalg_LU_invert(lu, perm, inv);
    if (determinant) {
      // XXX this is getting the wrong value: is it the wrong calculation?
      // it disagrees with the results of
      // det = (psF32)gsl_linalg_LU_det(lu, signum);
      // used in psMatrixDeterminatn
      // *determinant = (float)gsl_linalg_LU_lndet(lu);
      *determinant = (psF32)gsl_linalg_LU_det(lu, signum);
    }

    // Copy GSL matrix data to psImage data
    matrixGSLtoPS(out, inv);

    // Free GSL structs
    gsl_permutation_free(perm);
    gsl_matrix_free(lu);
    gsl_matrix_free(inv);

    return out;
}

float psMatrixDeterminant(const psImage* in)
{
    psS32 signum = 0;
    psS32 numRows = 0;
    psS32 numCols = 0;
    psF32 det = 0;
    gsl_matrix *lu = NULL;
    gsl_permutation *perm = NULL;

    #define DETERMINANT_EXIT { return 0; }
    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in, DETERMINANT_EXIT);
    PS_CHECK_DIMEN_AND_TYPE(in, PS_DIMEN_IMAGE, DETERMINANT_EXIT);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(in, DETERMINANT_EXIT);
    PS_CHECK_SQUARE(in, DETERMINANT_EXIT);

    // Initialize data
    numRows = in->numRows;
    numCols = in->numCols;

    // Allocate GSL structs
    perm = gsl_permutation_alloc(numRows);
    lu = gsl_matrix_alloc(numRows, numCols);
    matrixPStoGSL(lu, in);

    // Calculate determinant
    gsl_linalg_LU_decomp(lu, perm, &signum);
    det = (psF32)gsl_linalg_LU_lndet(lu);

    // Free GSL structs
    gsl_permutation_free(perm);
    gsl_matrix_free(lu);

    return det;
}

psImage* psMatrixMultiply(psImage* out,
                          const psImage* in1,
                          const psImage* in2)
{
    #define MULTIPLY_CLEANUP { psFree(out); return NULL; }

    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in1, MULTIPLY_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in2, MULTIPLY_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(in1, MULTIPLY_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(in2, MULTIPLY_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(in1, PS_DIMEN_IMAGE, MULTIPLY_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(in2, PS_DIMEN_IMAGE, MULTIPLY_CLEANUP);
    PS_CHECK_POINTERS(in1, out, MULTIPLY_CLEANUP);
    PS_CHECK_POINTERS(in1, in2, MULTIPLY_CLEANUP);

    int rows1 = in1->numRows, cols1 = in1->numCols; // Size of input 1
    int rows2 = in2->numRows, cols2 = in2->numCols; // Size of input 2
    if (cols1 != rows2) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Incompatible dimensions for matrix multiplication: %dx%d * %dx%d (row x col)",
                rows1, cols1, rows2, cols2);
        MULTIPLY_CLEANUP;
    }
    int common = cols1;                 // Common dimension

    if (in1->type.type != in2->type.type) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid operation: data types of in1 and in2 must match.");
        MULTIPLY_CLEANUP;
    }

    psElemType type = in1->type.type;   // Data type
    int outRows = rows1, outCols = cols2; // Size of output
    out = psImageRecycle(out, outCols, outRows, type);

#define MATRIX_MULTIPLY_CASE(TYPE) \
  case PS_TYPE_##TYPE: \
    for (int i = 0; i < outRows; i++) { \
        for (int j = 0; j < outCols; j++) { \
            ps##TYPE value = 0.0; \
            for (int k = 0; k < common; k++) { \
                value += in1->data.TYPE[i][k] * in2->data.TYPE[k][j]; \
            } \
            out->data.TYPE[i][j] = value; \
        } \
    } \
    break;

    switch (type) {
        MATRIX_MULTIPLY_CASE(F32);
        MATRIX_MULTIPLY_CASE(F64);
      default:
        psAbort("Should never get here.  Unsupported type: %x", type);
    }

    return out;
}

psImage* psMatrixTranspose(psImage* out, const psImage* in)
{
    // Error checks
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(in, NULL);
    PS_CHECK_DIMEN_AND_TYPE(in, PS_DIMEN_IMAGE, return NULL);
    PS_ASSERT(in != out, NULL);

    int numCols = in->numRows, numRows = in->numCols; // Size of transposed image
    psElemType type = in->type.type;    // Data type

    out = psImageRecycle(out, numCols, numRows, type);

#define TRANSPOSE_CASE(TYPE) \
  case PS_TYPE_##TYPE: { \
      for (int i = 0; i < numRows; i++) { \
          for (int j = 0; j < numCols; j++) { \
              out->data.TYPE[i][j] = in->data.TYPE[j][i]; \
          } \
      } \
      break; \
  }

    switch (type) {
        TRANSPOSE_CASE(F32);
        TRANSPOSE_CASE(F64);
      default:
        psAbort("Unsupported type: %x", type);
    }

    return out;
}

psImage* psMatrixEigenvectors(psImage* out,
                              const psImage* in)
{
    psS32 numRows = 0;
    psS32 numCols = 0;
    gsl_vector *eVals = NULL;
    gsl_eigen_symmv_workspace *w = NULL;
    gsl_matrix *outGSL = NULL;
    gsl_matrix *inGSL = NULL;

    #define EIGENVECTORS_CLEANUP { psFree(out); return NULL; }
    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in, EIGENVECTORS_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(in, PS_DIMEN_IMAGE, EIGENVECTORS_CLEANUP);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(in, EIGENVECTORS_CLEANUP);
    PS_CHECK_POINTERS(in, out, EIGENVECTORS_CLEANUP);

    out = psImageRecycle(out, in->numCols, in->numRows, in->type.type);

    // Initialize data
    numRows = in->numRows;
    numCols = in->numCols;

    inGSL = gsl_matrix_alloc(numRows, numCols);
    matrixPStoGSL(inGSL, in);
    outGSL = gsl_matrix_alloc(numRows, numCols);

    // Allocate GSL structs
    eVals = gsl_vector_alloc(numRows);
    w = gsl_eigen_symmv_alloc(numRows);

    // Non-square matrices not allowed
    PS_CHECK_SQUARE(in, EIGENVECTORS_CLEANUP);
    PS_CHECK_SQUARE(out, EIGENVECTORS_CLEANUP);

    // Calculate Eigenvalues and Eigenvectors...Eigenvalues not currently used
    gsl_eigen_symmv(inGSL, eVals, outGSL, w);

    // Copy GSL matrix data to psImage data
    matrixGSLtoPS(out, outGSL);

    // Free GSL structs
    gsl_matrix_free(inGSL);
    gsl_matrix_free(outGSL);
    gsl_eigen_symmv_free(w);
    gsl_vector_free(eVals);

    return out;
}

psVector* psMatrixToVector(psVector* outVector,
                           const psImage* inImage)
{
    psS32 size = 0;

    #define psMatrixToVector_EXIT {psFree(outVector); return NULL;}

    // Error checks
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(inImage, psMatrixToVector_EXIT);
    PS_CHECK_DIMEN_AND_TYPE(inImage, PS_DIMEN_IMAGE, psMatrixToVector_EXIT);
    PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(inImage, psMatrixToVector_EXIT);

    if (inImage->numRows == 1) {
        // Create transposed row vector
        outVector = psVectorRecycle(outVector, inImage->numCols, inImage->type.type);
        outVector->type.dimen = PS_DIMEN_TRANSV;
    } else if (inImage->numCols == 1) {
        // Create non-transposed column vector
        outVector = psVectorRecycle(outVector, inImage->numRows, inImage->type.type);
    } else {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Image does not have dim with 1 col or 1 row: (%d x %d).",
                inImage->numRows, inImage->numCols);
        psMatrixToVector_EXIT;
    }

    // More checks
    if (outVector->type.dimen == PS_DIMEN_VECTOR) {
        PS_CHECK_DIMEN_AND_TYPE(outVector, PS_DIMEN_VECTOR, psMatrixToVector_EXIT);

        if (outVector->n == 0) {
            outVector->n = inImage->numRows;
        }

        if (outVector->n != inImage->numRows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image and vector sizes differ: (%d vs %ld).",
                    inImage->numRows, outVector->n);
            psMatrixToVector_EXIT;
        }

        size = PSELEMTYPE_SIZEOF(inImage->type.type) * inImage->numRows;

    } else if (outVector->type.dimen == PS_DIMEN_TRANSV) {
        PS_CHECK_DIMEN_AND_TYPE(outVector, PS_DIMEN_TRANSV, psMatrixToVector_EXIT);

        if (outVector->n == 0) {
            outVector->n = inImage->numCols;
        }

        if (outVector->n != inImage->numCols) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image and vector sizes differ: (%d vs %ld).",
                    inImage->numCols, outVector->n);
            psMatrixToVector_EXIT;
        }

        size = PSELEMTYPE_SIZEOF(inImage->type.type) * inImage->numCols;
    }

    memcpy(outVector->data.U8, inImage->data.U8[0], size);

    return outVector;
}

psImage* psVectorToMatrix(psImage* outImage,
                          const psVector* inVector)
{
    psS32 size = 0;

    #define VECTORTOMATRIX_CLEANUP {psFree(outImage); return NULL; }
    // Error checks
    PS_ASSERT_GENERAL_VECTOR_NON_NULL(inVector, VECTORTOMATRIX_CLEANUP);

    if (inVector->type.dimen == PS_DIMEN_VECTOR) {
        PS_CHECK_DIMEN_AND_TYPE(inVector, PS_DIMEN_VECTOR, VECTORTOMATRIX_CLEANUP);
        PS_ASSERT_GENERAL_VECTOR_NON_EMPTY(inVector, VECTORTOMATRIX_CLEANUP);

        outImage = psImageRecycle(outImage, 1, inVector->n, inVector->type.type);

        // More checks for PS_DIMEN_VECTOR
        if (outImage->numCols > 1) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image has more than 1 column: numCols = %d.",
                    outImage->numCols);
            VECTORTOMATRIX_CLEANUP;
        } else if (outImage->numRows != inVector->n) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image and vector sizes differ: (%d vs %ld).",
                    outImage->numRows, inVector->n);
            VECTORTOMATRIX_CLEANUP;
        }

        size = PSELEMTYPE_SIZEOF(outImage->type.type) * outImage->numRows;

    } else if (inVector->type.dimen == PS_DIMEN_TRANSV) {
        PS_CHECK_DIMEN_AND_TYPE(inVector, PS_DIMEN_TRANSV, VECTORTOMATRIX_CLEANUP);
        PS_ASSERT_GENERAL_VECTOR_NON_EMPTY(inVector, VECTORTOMATRIX_CLEANUP);
        outImage = psImageRecycle(outImage, inVector->n, 1, inVector->type.type);
        // More checks for PS_DIMEN_TRANSV
        if (outImage->numRows > 1) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image has more than 1 row: numRows = %d.",
                    outImage->numRows);
            VECTORTOMATRIX_CLEANUP;
        } else if (outImage->numCols != inVector->n) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image and vector sizes differ: (%d vs %ld).",
                    outImage->numCols, inVector->n);
            VECTORTOMATRIX_CLEANUP;
        }

        size = PSELEMTYPE_SIZEOF(outImage->type.type) * outImage->numCols;
    }

    PS_ASSERT_GENERAL_IMAGE_NON_NULL(outImage, VECTORTOMATRIX_CLEANUP);
    PS_CHECK_DIMEN_AND_TYPE(outImage, PS_DIMEN_IMAGE, VECTORTOMATRIX_CLEANUP);

    memcpy(outImage->data.U8[0], inVector->data.U8, size);

    return outImage;
}

psVector *psMatrixSolveSVD(psVector *out, const psImage *matrix, const psVector *vector, float thresh)
{
    #define psMatrixSolveSVD_EXIT {psFree(out); return NULL; }
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(matrix, psMatrixSolveSVD_EXIT);
    PS_CHECK_DIMEN_AND_TYPE(matrix, PS_DIMEN_IMAGE, psMatrixSolveSVD_EXIT);
    PS_ASSERT_GENERAL_VECTOR_NON_NULL(vector, psMatrixSolveSVD_EXIT);
    PS_CHECK_DIMEN_AND_TYPE(vector, PS_DIMEN_VECTOR, psMatrixSolveSVD_EXIT);

    int numCols = matrix->numCols, numRows = matrix->numRows; // Size of matrix

    // Decompose matrix: A = U S V^T
    gsl_matrix *A = gsl_matrix_alloc(numRows, numCols); // Input matrix in GSL-speak; becomes matrix U
    gsl_matrix *V = gsl_matrix_alloc(numCols, numCols); // Untransposed matrix V
    gsl_vector *S = gsl_vector_alloc(numCols);          // Singular values
    gsl_vector *work = gsl_vector_alloc(numCols);       // Work space for GSL

    matrixPStoGSL(A, matrix);

    int gslStatus = 0;                  // Status of GSL
    if ((gslStatus = gsl_linalg_SV_decomp(A, V, S, work))) {
        const char *err = gsl_strerror(gslStatus);
        psError(PS_ERR_UNKNOWN, true, "Unable to decompose matrix: %s", err);
        gsl_matrix_free(A);
        gsl_matrix_free(V);
        gsl_vector_free(S);
        gsl_vector_free(work);
        return NULL;
    }
    gsl_vector_free(work);

    if (isfinite(thresh) && thresh > 0.0) {
        // Trim the singular values
        double total = 0.0;             // Total of singular values
        for (int i = 0; i < numCols; i++) {
            total += gsl_vector_get(S, i);
        }
        thresh *= total;
        for (int i = 0; i < numCols; i++) {
            double value = gsl_vector_get(S, i); // Singular value
            if (value < thresh) {
                psTrace("psLib.math", 5, "Trimming singular value %d: %lg", i, value);
                gsl_vector_set(S, i, 0.0);
#if 0
                for (int j = 0; j < numCols; j++) {
                    // Being thorough; probably unnecessary
                    gsl_matrix_set(V, j, i, 0.0);
                    gsl_matrix_set(A, j, i, 0.0);
                }
#endif
            } else {
                psTrace("psLib.math", 5, "Singular value %d: %lg", i, value);
            }
        }
    }

    // Solve system (or minimise least-squares if overconstrained): Ax = b
    gsl_vector *b = gsl_vector_alloc(numCols); // Vector b
    gsl_vector *x = gsl_vector_alloc(numCols); // Solution

    vectorPStoGSL(b, vector);

    if ((gslStatus = gsl_linalg_SV_solve(A, V, S, b, x))) {
        const char *err = gsl_strerror(gslStatus);
        psError(PS_ERR_UNKNOWN, true, "Unable to solve matrix equation: %s", err);
        gsl_matrix_free(A);
        gsl_matrix_free(V);
        gsl_vector_free(S);
        gsl_vector_free(b);
        gsl_vector_free(x);
        return NULL;
    }

    gsl_matrix_free(A);
    gsl_matrix_free(V);
    gsl_vector_free(S);
    gsl_vector_free(b);

    out = psVectorRecycle(out, numCols, PS_TYPE_F64);

    vectorGSLtoPS(out, x);
    gsl_vector_free(x);

    return out;
}

// This code supplied by Andy Becker (becker@astro.washington.edu)
psImage *psMatrixSVD_old(psImage* evec, psVector* eval, const psImage* in)
{
    #define psMatrixSVD_EXIT {psFree(evec); psFree(eval); return NULL;}

    // Error checks  Missing one for eval
    PS_ASSERT_GENERAL_IMAGE_NON_NULL(in, psMatrixSVD_EXIT);
    PS_CHECK_POINTERS(in, evec, psMatrixSVD_EXIT);
    PS_CHECK_DIMEN_AND_TYPE(in, PS_DIMEN_IMAGE, psMatrixSVD_EXIT);

    // evec ends up : numCols x numCols
    evec = psImageRecycle(evec,  in->numCols, in->numCols, in->type.type);
    eval = psVectorRecycle(eval, in->numCols, in->type.type);

    // Initialize data
    int numRows = in->numRows;
    int numCols = in->numCols;

    gsl_matrix *A = gsl_matrix_alloc(numRows, numCols);
    gsl_matrix *V = gsl_matrix_alloc(numCols, numCols);
    gsl_vector *S = gsl_vector_alloc(numCols);
    gsl_vector *work = gsl_vector_alloc(numCols);

    // Copy psImage data into GSL matrix data
    matrixPStoGSL(A, in);

    // Calculate SVD decomposition
    gsl_linalg_SV_decomp(A, V, S, work);

    // Copy GSL matrix data to psImage data
    matrixGSLtoPS(evec, V);
    vectorGSLtoPS(eval, S);

    // Take the square root of eval
    for (int i = 0; i < eval->n; i++) {
        eval->data.F64[i] = sqrt(eval->data.F64[i]);
        /* make sure that these things are sorted! */
        if (i > 0) {
            psAssert(eval->data.F64[i] <= eval->data.F64[i-1], "impossible");
        }
    }

    // Free GSL data
    gsl_matrix_free(A);
    gsl_matrix_free(V);
    gsl_vector_free(S);
    gsl_vector_free(work);

    return evec;
}

// this is basically a wrapper for the gsl function: gsl_linalg_SV_decomp() SVD decomposes
// matrix A based on the following equation: A = U w V^T .  This function (as usual for SVD
// implementations) returns V not V^T.  U and V are returned to images; w is returned to a
// vector representing the diagonal of w.  The input image A is not modified.  U, V, and w may
// be supplied as NULL or may be allocated; their lengths are set here to match the
// dimensionality of A.  XXX there is no error handling for the gsl functions (anywhere in
// psMatrix.c)
bool psMatrixSVD(psImage **U, psVector **w, psImage **V, const psImage *A)
{
    // Error checks  Missing one for eval
    PS_ASSERT_PTR_NON_NULL(U, false);
    PS_ASSERT_PTR_NON_NULL(w, false);
    PS_ASSERT_PTR_NON_NULL(V, false);
    PS_ASSERT_PTR_NON_NULL(A, false);

    // A is provided with size Nx,Ny = numCols,numRows
    // U has size Nx,Ny
    // V has size Nx,Nx
    // w has size Nx

    // Initialize data
    int numRows = A->numRows;
    int numCols = A->numCols;

    *U = psImageRecycle(*U,  numCols, numRows, A->type.type);
    *V = psImageRecycle(*V,  numCols, numCols, A->type.type);
    *w = psVectorRecycle(*w, numCols, A->type.type);

    gsl_matrix *Agsl = gsl_matrix_alloc(numRows, numCols);
    gsl_matrix *Vgsl = gsl_matrix_alloc(numCols, numCols);
    gsl_vector *Sgsl = gsl_vector_alloc(numCols);
    gsl_vector *work = gsl_vector_alloc(numCols);

    // Copy psImage data into GSL matrix data
    matrixPStoGSL(Agsl, A);

    // Calculate SVD decomposition
    gsl_linalg_SV_decomp(Agsl, Vgsl, Sgsl, work);

    // Copy GSL matrix data to psImage data
    matrixGSLtoPS(*V, Vgsl);
    matrixGSLtoPS(*U, Agsl);  // gsl_linalg_SV_decomp replaces A with U
    vectorGSLtoPS(*w, Sgsl);

    // Free GSL data
    gsl_matrix_free(Agsl);
    gsl_matrix_free(Vgsl);
    gsl_vector_free(Sgsl);
    gsl_vector_free(work);

    return true;
}


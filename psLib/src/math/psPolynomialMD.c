#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "psMemory.h"
#include "psVector.h"
#include "psImage.h"
#include "psPolynomial.h"
#include "psArray.h"
#include "psMatrix.h"
#include "psError.h"
#include "psAbort.h"
#include "psAssert.h"
#include "psTrace.h"

#include "psPolynomialMD.h"


//#define DEBUG

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Destructor
static void polynomialMDFree(psPolynomialMD *poly // Polynomial
    )
{
    if (!poly) return;
    psFree(poly->orders);
    psFree(poly->coeff);

    psFree(poly->fitMatrix);
    psFree(poly->fitVector);
    psFree(poly->tmpMatrix);
    psFree(poly->tmpVector);

    psFree(poly->fitBuffer);

    psFree(poly->ownMask);
    psFree(poly->deviations);

    psFree(poly->LUperm);
    psFree(poly->LU);
    
    return;
}

// Generate a least-squares matrix and vector
// Note: only generates the diagonal and lower triangle of the matrix
static void polynomialMDLeastSquares(const psPolynomialMD *poly, // Polynomial
                                     psImage *matrix, // Least-squares matrix to fill out
                                     psVector *vector, // Least-squares vector to fill out
                                     const psVector *coords, // Coordinates
                                     float value, // Value
                                     float error, // Error
                                     psVector *buffer // Buffer for evaluations
    )
{
    int numTerms = vector->n;           // Number of terms
    psAssert(matrix->numCols == numTerms && matrix->numRows == numTerms, "impossible");
    psAssert(buffer && buffer->n == numTerms && buffer->type.type == PS_TYPE_F64, "impossible");

#ifdef DEBUG
    psImageInit(matrix, NAN);
    psVectorInit(vector, NAN);
#endif

    buffer->data.F64[0] = 1.0;
    for (int i = 0, index = 1; i < poly->dim; i++) {
        int order = poly->orders->data.U8[i]; // Order of polynomial
        float coord = coords->data.F32[i]; // Coordinate of interest
        double value = coord;           // Value of polynomial stages
        buffer->data.F64[index++] = value;
        for (int j = 2; j <= order; j++, index++) {
            value *= coord;
            buffer->data.F64[index] = value;
        }
    }

    double invSigma2 = (error == 0.0) ? 1.0 : 1.0 / PS_SQR(error); // 1/sigma^2
    for (int i = 0; i < numTerms; i++) {
        for (int j = 0; j < i; j++) {
            matrix->data.F64[i][j] = buffer->data.F64[i] * buffer->data.F64[j] * invSigma2;
        }
        matrix->data.F64[i][i] = PS_SQR(buffer->data.F64[i]) * invSigma2;
        vector->data.F64[i] = value * buffer->data.F64[i] * invSigma2;
    }

    return;
}

// Accumulate the lower triangle of the matrix
static void polynomialMDAccumulate(psImage *targetMatrix, // Final least-squares matrix
                                   psVector *targetVector, // Final least-squares vector
                                   const psImage *sourceMatrix, // Input least-squares matrix
                                   const psVector *sourceVector // Input least-squares vector
    )
{
    int numTerms = targetVector->n;     // Number of terms in polynomial

    for (int j = 0; j < numTerms; j++) {
        for (int k = 0; k < j; k++) {
            targetMatrix->data.F64[j][k] += sourceMatrix->data.F64[j][k];
        }
        targetMatrix->data.F64[j][j] += sourceMatrix->data.F64[j][j];
        targetVector->data.F64[j] += sourceVector->data.F64[j];
    }

    return;
}

// Fill in the upper triangle of the matrix
static void polynomialMDFill(psImage *matrix // Final least-squares matrix
    )
{
    int numTerms = matrix->numCols;     // Number of terms in polynomial

    for (int j = 0; j < numTerms; j++) {
        for (int k = j + 1; k < numTerms; k++) {
            matrix->data.F64[j][k] = matrix->data.F64[k][j];
        }
    }

    return;
}

// Calculate the standard deviation of the fit
static void polynomialMDStdev(psPolynomialMD *poly, // Polynomial for which to measure stdev
                              psVector *deviations, // Deviations, or NULL
                              const psArray *coords, // Array of coordinates
                              const psVector *values, // Measured values
                              const psVector *mask, // Mask for values
			      psVectorMaskType maskVal 
                              )
{
    psAssert(poly, "impossible");
    psAssert(coords, "impossible");
    psAssert(values, "impossible");

    double rms = 0.0;                   // Root mean square deviation
    int numValues = values->n;          // Number of values
    int numGood = numValues;            // Number of good values
    for (int i = 0; i < numValues; i++) {
        if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal)) {
            numGood--;
            continue;
        }
        float diff = values->data.F32[i] - psPolynomialMDEval(poly, coords->data[i]);
        if (deviations) {
            deviations->data.F32[i] = diff;
        }
        rms += PS_SQR(diff);
    }
    poly->stdevFit = sqrt(rms / (double)numGood);

    return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

psPolynomialMD *psPolynomialMDAlloc(const psVector *orders)
{
    psPolynomialMD *poly = psAlloc(sizeof(psPolynomialMD));
    psMemSetDeallocator(poly, (psFreeFunc)polynomialMDFree);

    int dim = orders->n;                // Number of dimensions

        #define CHECK_ORDER_CASE_SIGNED(TYPE) \
          case PS_TYPE_##TYPE: { \
              for (int i = 0; i < dim; i++) { \
                  if (orders->data.TYPE[i] <= 0) { \
                      psAbort("Order %d is non-positive.", i); \
                  } \
              } \
              break; \
          }

    switch (orders->type.type) {
        CHECK_ORDER_CASE_SIGNED(S8);
        CHECK_ORDER_CASE_SIGNED(S16);
        CHECK_ORDER_CASE_SIGNED(S32);
        CHECK_ORDER_CASE_SIGNED(S64);
      case PS_TYPE_F32:
      case PS_TYPE_F64:
        psAbort("Floating-point order vector is not supported.");
      case PS_TYPE_U8:
      case PS_TYPE_U16:
      case PS_TYPE_U32:
      case PS_TYPE_U64:
        // These can pass unchecked
        break;
      default:
        psAbort("Unknown type: %x", orders->type.type);
    }

    poly->dim = dim;
    poly->orders = psVectorCopy(NULL, orders, PS_TYPE_U8);
    poly->numFit = 0;
    poly->stdevFit = NAN;

    int numTerms = 1;                   // Number of terms in polynomial
    for (int i = 0; i < dim; i++) {
        numTerms += poly->orders->data.U8[i];
    }
    poly->coeff = psVectorAlloc(numTerms, PS_TYPE_F64);

    // internal temporary variables so we can use this in a tight, threaded loop
    poly->fitMatrix = NULL;
    poly->fitVector = NULL;
    poly->tmpMatrix = NULL;
    poly->tmpVector = NULL;

    poly->fitBuffer = NULL;

    poly->ownMask = NULL;
    poly->deviations = NULL;

    poly->LUperm = NULL;
    poly->LU = NULL;

    return poly;
}

double psPolynomialMDEval(const psPolynomialMD *poly, ///< Polynomial
                          const psVector *coords ///< Coordinates
    )
{
    PS_ASSERT_POLYNOMIALMD_NON_NULL(poly, NAN);
    PS_ASSERT_VECTOR_NON_NULL(coords, NAN);
    PS_ASSERT_VECTOR_SIZE(coords, (long)poly->dim, NAN);
    PS_ASSERT_VECTOR_TYPE(coords, PS_TYPE_F32, NAN);

    psVector *orders = poly->orders; // Orders for each polynomial
    psF64 *coeff = poly->coeff->data.F64; // Coefficients

    double sum = coeff[0];              // Sum of all polynomials
    for (int i = 0, index = 1; i < poly->dim; i++) {
        int order = orders->data.U8[i]; // Order of polynomial
        float coord = coords->data.F32[i]; // Coordinate of interest
        float value = coord; // Value of the polynomial stage
        sum += coeff[index++] * value;
        for (int j = 2; j <= order; j++, index++) {
            value *= coord;
            sum += coeff[index] * value;
        }
    }

    return sum;
}

bool psPolynomialMDFit(psPolynomialMD *poly, const psVector *values, const psVector *errors,
                       const psVector *mask, psVectorMaskType maskVal, const psArray *coordsArray)
{
    PS_ASSERT_POLYNOMIALMD_NON_NULL(poly, false);
    PS_ASSERT_VECTOR_NON_NULL(values, false);
    PS_ASSERT_VECTOR_TYPE(values, PS_TYPE_F32, false);
    if (errors) {
        PS_ASSERT_VECTOR_NON_NULL(errors, false);
        PS_ASSERT_VECTOR_TYPE(errors, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, errors, false);
    }
    if (maskVal == 0) {
        mask = NULL;
    }
    if (mask) {
        PS_ASSERT_VECTOR_NON_NULL(mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, mask, false);
    }
    PS_ASSERT_ARRAY_NON_NULL(coordsArray, false);
    PS_ASSERT_ARRAY_SIZE(coordsArray, values->n, false);

    int numValues = values->n;          // Number of values
    int numTerms = poly->coeff->n;      // Number of terms

    // we recycle matrices and vectors carried by poly to avoid alloc / threading hits
    poly->fitMatrix = psImageRecycle(poly->fitMatrix, numTerms, numTerms, PS_TYPE_F64); // Least-squares matrix
    poly->fitVector = psVectorRecycle(poly->fitVector, numTerms, PS_TYPE_F64); // Least-squares vector

    psImageInit(poly->fitMatrix, 0.0);
    psVectorInit(poly->fitVector, 0.0);

    poly->tmpMatrix = psImageRecycle (poly->tmpMatrix, numTerms, numTerms, PS_TYPE_F64); // Least-squares matrix for each term
    poly->tmpVector = psVectorRecycle(poly->tmpVector, numTerms, PS_TYPE_F64); // Least-squares vector for each term
    poly->fitBuffer = psVectorRecycle(poly->fitBuffer, numTerms, PS_TYPE_F64); // Buffer for evaluations of polynomial stages

    for (int i = 0; i < numValues; i++) {
        psVector *coords = coordsArray->data[i];
        PS_ASSERT_VECTOR_NON_NULL(coords, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(coords, poly->orders, false);
        PS_ASSERT_VECTOR_TYPE(coords, PS_TYPE_F32, false);

        if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal)) {
            continue;
        }

	float err = errors ? errors->data.F32[i] : 0.0;
        polynomialMDLeastSquares(poly, poly->tmpMatrix, poly->tmpVector, coords, values->data.F32[i], err, poly->fitBuffer);
        polynomialMDAccumulate(poly->fitMatrix, poly->fitVector, poly->tmpMatrix, poly->tmpVector);
    }
    polynomialMDFill(poly->fitMatrix);

    poly->LU = psMatrixLUDecomposition(poly->LU, &poly->LUperm, poly->fitMatrix); // LU-decomposed matrix
    if (!poly->LU) {
        psError(PS_ERR_UNKNOWN, false, "Unable to LU-Decompose least-squares matrix.");
        return false;
    }

    poly->coeff = psMatrixLUSolution(poly->coeff, poly->LU, poly->fitVector, poly->LUperm);
    if (!poly->coeff) {
        psError(PS_ERR_UNKNOWN, false, "Unable to solve least-squares equation.");
        return false;
    }

    polynomialMDStdev(poly, poly->deviations, coordsArray, values, mask, maskVal);

    return true;
}

bool psPolynomialMDClipFit(psPolynomialMD *poly, const psVector *values, const psVector *errors,
                           const psVector *mask, psVectorMaskType maskVal, const psArray *coordsArray,
                           int numIter, float rej) 
{

    PS_ASSERT_POLYNOMIALMD_NON_NULL(poly, false);
    PS_ASSERT_VECTOR_NON_NULL(values, false);
    PS_ASSERT_VECTOR_TYPE(values, PS_TYPE_F32, false);
    if (errors) {
        PS_ASSERT_VECTOR_NON_NULL(errors, false);
        PS_ASSERT_VECTOR_TYPE(errors, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, errors, false);
    }
    if (maskVal == 0) {
        mask = NULL;
    }
    if (mask) {
        PS_ASSERT_VECTOR_NON_NULL(mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, mask, false);
    }
    PS_ASSERT_ARRAY_NON_NULL(coordsArray, false);
    PS_ASSERT_ARRAY_SIZE(coordsArray, (long)values->n, false);
    PS_ASSERT_INT_NONNEGATIVE(numIter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);

    int numValues = values->n;          // Number of values
    int numTerms = poly->coeff->n;      // Number of terms

    int numClipped = INT_MAX;           // Number of values clipped int an interation
    int numGood = numValues;            // Number of good values

    // copy the input mask to a local temporary mask 
    poly->ownMask = psVectorRecycle(poly->ownMask, numValues, PS_TYPE_VECTOR_MASK); // Our own mask for input values
    psVectorInit(poly->ownMask, 0);
    for (int i = 0; mask && (i < numValues); i++) {
	if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal) {
	    poly->ownMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
	    numGood--;
	}
    }

    // make sure we have storage for the residuals
    poly->deviations = psVectorRecycle (poly->deviations, numValues, PS_TYPE_F32);

    for (int iter = 0; (iter < numIter) && (numClipped > 0) && (numGood > numTerms); iter++) {
        numClipped = 0;

	// XXX raise error?
        if (!psPolynomialMDFit(poly, values, errors, poly->ownMask, maskVal, coordsArray)) {
            return false;
        }

        psTrace("psLib.math", 7, "RMS from %d points is %lf\n", numGood, poly->stdevFit);

        // Reject
        float limit = rej * poly->stdevFit; // Rejection limit
        for (int i = 0; i < numValues; i++) {

# if (0)
	    fprintf (stderr, "coords: ");
	    psVector *coords = coordsArray->data[i];
	    for (int j = 0; j < coords->n; j++) {
		fprintf (stderr, "%f ", coords->data.F32[j]);
	    }
# endif
	    
	    psTrace("psLib.math", 10, "point %d (%f,%f: %f > %f)\n",
		    i, values->data.F32[i], values->data.F32[i] - poly->deviations->data.F32[i], poly->deviations->data.F32[i], limit);

            if (poly->ownMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;

            if (fabs(poly->deviations->data.F32[i]) > limit) {
                psTrace("psLib.math", 9, "Rejected point %d (%f,%f: %f > %f)\n",
                        i, values->data.F32[i], values->data.F32[i] + poly->deviations->data.F32[i], 
                        poly->deviations->data.F32[i], limit);
                poly->ownMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
                numClipped++;
                numGood--;
            }
        }
        psTrace("psLib.math", 7, "Rejected %d points (%d remaining)\n", numClipped, numGood);
    }

    if (numClipped > 0) {
        // Need to do a final re-evaluation of the fit
        if (!psPolynomialMDFit(poly, values, errors, poly->ownMask, maskVal, coordsArray)) {
            return false;
        }
    }

    return true;
}

// XXX EAM : This function was trying to save calculation time at the expense of a LOT of allocs.
# if (0)
// Accumulate and solve the least-squares equation
static bool polynomialMDClipFit(psPolynomialMD *poly, // Polynomial
                                psImage *matrix, // Least-squares matrix
                                psVector *vector, // Least-squares vector
                                psImage **lu, // LU-decomposed matrix
                                psVector **perm, // Permutations vector
                                const psArray *matrices, // Individual least-squares matrices
                                const psArray *vectors, // Individual least-squares vectors
                                const psVector *mask, // Mask for values
    )
{
    int numValues = vectors->n;         // Number of values

    // Accumulate the least-squares matrix and vector
    psImageInit(matrix, 0.0);
    psVectorInit(vector, 0.0);
    for (int i = 0; i < numValues; i++) {
        if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            continue;
        }

        psImage *newMatrix = matrices->data[i]; // Matrix to accumulate
        psVector *newVector = vectors->data[i]; // Vector to accumulate

        polynomialMDAccumulate(matrix, vector, newMatrix, newVector);
    }
    polynomialMDFill(matrix);

    // Solve least-squares equation
    *lu = psMatrixLUDecomposition(*lu, perm, matrix);
    if (!*lu) {
        psError(PS_ERR_UNKNOWN, false, "Unable to LU-Decompose least-squares matrix.");
        return false;
    }
    poly->coeff = psMatrixLUSolution(poly->coeff, *lu, vector, *perm);
    if (!poly->coeff) {
        psError(PS_ERR_UNKNOWN, false, "Unable to solve least-squares equation.");
        return false;
    }
    return true;
}


bool psPolynomialMDClipFit(psPolynomialMD *poly, const psVector *values, const psVector *errors,
                           const psVector *mask, psVectorMaskType maskVal, const psArray *coordsArray,
                           int numIter, float rej)
{
    PS_ASSERT_POLYNOMIALMD_NON_NULL(poly, false);
    PS_ASSERT_VECTOR_NON_NULL(values, false);
    PS_ASSERT_VECTOR_TYPE(values, PS_TYPE_F32, false);
    if (errors) {
        PS_ASSERT_VECTOR_NON_NULL(errors, false);
        PS_ASSERT_VECTOR_TYPE(errors, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, errors, false);
    }
    if (maskVal == 0) {
        mask = NULL;
    }
    if (mask) {
        PS_ASSERT_VECTOR_NON_NULL(mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, mask, false);
    }
    PS_ASSERT_ARRAY_NON_NULL(coordsArray, false);
    PS_ASSERT_ARRAY_SIZE(coordsArray, (long)values->n, false);
    PS_ASSERT_INT_NONNEGATIVE(numIter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);

    int numValues = values->n;          // Number of values
    int numTerms = poly->coeff->n;      // Number of terms

    psArray *matrices = psArrayAlloc(numValues); // Least-squares matrix for each input value
    psArray *vectors = psArrayAlloc(numValues); // Least-squares vector for each input value

    // Generate least-squares matrices and vectors
    psVector *buffer = psVectorAlloc(numTerms, PS_TYPE_F64); // Buffer for evaluations of polynomial stages
    for (int i = 0; i < numValues; i++) {
        psVector *coords = coordsArray->data[i];
        PS_ASSERT_VECTOR_NON_NULL(coords, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(coords, poly->orders, false);
        PS_ASSERT_VECTOR_TYPE(coords, PS_TYPE_F32, false);

        if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal)) {
            continue;
        }

        matrices->data[i] = psImageAlloc(numTerms, numTerms, PS_TYPE_F64);
        vectors->data[i] = psVectorAlloc(numTerms, PS_TYPE_F64);

        polynomialMDLeastSquares(poly, matrices->data[i], vectors->data[i], coords, values->data.F32[i],
                                 errors ? errors->data.F32[i] : 0.0, buffer);
    }
    psFree(buffer);

    // Iterate over the solution
    int numGood = numValues;            // Number of good values
    psImage *matrix = psImageAlloc(numTerms, numTerms, PS_TYPE_F64); // Least-squares matrix
    psVector *vector = psVectorAlloc(numTerms, PS_TYPE_F64); // Least-squares vector
    psVector *ownMask = psVectorAlloc(numValues, PS_TYPE_VECTOR_MASK); // Our own mask for input values
    psVectorInit(ownMask, 0);
    if (mask) {
        for (int i = 0; i < numValues; i++) {
            if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal) {
                ownMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
                numGood--;
            }
        }
    }
    psImage *lu = NULL;                 // LU-decomposed matrix
    psVector *perm = NULL;              // Permutation vector
    psVector *deviations = psVectorAlloc(numValues, PS_TYPE_F32); // Deviations from fit
    int numClipped = INT_MAX;           // Number of values clipped int an interation
    for (int iter = 0; iter < numIter && numClipped > 0 && numGood > numTerms; iter++) {
        numClipped = 0;

        if (!polynomialMDClipFit(poly, matrix, vector, &lu, &perm, matrices, vectors, ownMask)) {
            psFree(matrix);
            psFree(vector);
            psFree(lu);
            psFree(perm);
            psFree(matrices);
            psFree(vectors);
            psFree(ownMask);
            psFree(deviations);
            return false;
        }

        polynomialMDStdev(poly, deviations, coordsArray, values, ownMask);
        psTrace("psLib.math", 7, "RMS from %d points is %lf\n", numGood, poly->stdevFit);

        // Reject
        float limit = rej * poly->stdevFit; // Rejection limit
        for (int i = 0; i < numValues; i++) {
            if (ownMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                continue;
            }
            if (fabs(deviations->data.F32[i]) > limit) {
                psTrace("psLib.math", 9, "Rejected point %d (%f,%f: %f > %f)\n",
                        i, values->data.F32[i], psPolynomialMDEval(poly, coordsArray->data[i]),
                        deviations->data.F32[i], limit);
                ownMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
                numClipped++;
                numGood--;
            }
        }
        psTrace("psLib.math", 7, "Rejected %d points (%d remaining)\n", numClipped, numGood);
    }

    if (numClipped > 0) {
        // Need to do a final re-evaluation of the fit
        if (!polynomialMDClipFit(poly, matrix, vector, &lu, &perm, matrices, vectors, ownMask)) {
            psFree(matrix);
            psFree(vector);
            psFree(lu);
            psFree(perm);
            psFree(matrices);
            psFree(vectors);
            psFree(ownMask);
            psFree(deviations);
            return false;
        }
        polynomialMDStdev(poly, NULL, coordsArray, values, ownMask);
    }

    psFree(matrix);
    psFree(vector);
    psFree(lu);
    psFree(perm);
    psFree(matrices);
    psFree(vectors);
    psFree(ownMask);
    psFree(deviations);

    return true;
}
# endif

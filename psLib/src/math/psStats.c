/** @file  psStats.c
 *  \brief basic statistical operations
 *  @ingroup Stat
 *
 *  This file will hold the definitions of the histogram and stats data
 *  structures.  It also contains prototypes for procedures which operate
 *  on those data structures.
 *
 *  @author GLG (MHPCC), EAM (IfA)
 *
 * XXX: Must do
 * nSubsample points
 * use ->min and ->max (PS_STAT_USE_RANGE)
 *
 *  @version $Revision: 1.231 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2006 IfA, University of Hawaii
 */


// Note: choice of return values in many places is quite grey.  For example, calculating the sample standard
// deviation: here it's an error to get an array of size zero, but it's not an error to get an array of size
// unity, even though the standard deviation is not defined in that case (NAN).

// reworking the return values and failure conditions: it should only be an error if the
// inputs imply a programming error: eg, NULL data vectors, non-sensical input
// parameters.  If the statistic cannot be calculated (0 length vector, 0 range, no
// unmasked data values), the statistic should be reported as NAN, but an error should not
// be raised.  (TBD: do we need to have a unique field in psStats or a return parameter
// that can be checked for an invalid result?)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <strings.h>

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/
#include "psMemory.h"
#include "psAbort.h"
#include "psVector.h"
#include "psTrace.h"
#include "psLogMsg.h"
#include "psError.h"
#include "psHistogram.h"
#include "psStats.h"
#include "psMinimizeLMM.h"
#include "psMinimizePolyFit.h"
#include "psPolynomial.h"
#include "psAssert.h"
#include "psMathUtils.h"
#include "psList.h"
#include "psString.h"



/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/
#define PS_GAUSS_WIDTH 3                // The width of the Gaussian smoothing.
#define PS_CLIPPED_NUM_ITER_LB 1 // This corresponds to N in the ADD.
#define PS_CLIPPED_NUM_ITER_UB 10
#define PS_CLIPPED_SIGMA_LB 1.0
#define PS_CLIPPED_SIGMA_UB 10.0
#define PS_POLY_MEDIAN_MAX_ITERATIONS 30

#define TRACE "psLib.math"

#define MASK_MARK 0x80   // XXX : can we change this? bit to use internally to mark data as bad
#define PS_ROBUST_MAX_ITERATIONS 20     // Maximum number of iterations for robust statistics

#define PS_BIN_MIDPOINT(HISTOGRAM, BIN_NUM) \
(0.5 * (HISTOGRAM->bounds->data.F32[(BIN_NUM)] + HISTOGRAM->bounds->data.F32[(BIN_NUM)+1]))

// set the bin closest to the corresponding value.  if USE_END is +/- 1,
// out-of-range saturates on lower/upper bin REGARDLESS of actual value
#define PS_BIN_FOR_VALUE(RESULT, VECTOR, VALUE, USE_END) { \
        psVectorBinaryDisectResult result; \
        psScalar tmpScalar; \
        tmpScalar.type.type = PS_TYPE_F32; \
        tmpScalar.data.F32 = (VALUE); \
        RESULT = psVectorBinaryDisect (&result, VECTOR, &tmpScalar); \
        switch (result) { \
          case PS_BINARY_DISECT_PASS: \
            break; \
          case PS_BINARY_DISECT_OUTSIDE_RANGE: \
            psTrace(TRACE, 6, "selected bin outside range"); \
            if (USE_END == -1) { RESULT = 0; } \
            if (USE_END == +1) { RESULT = VECTOR->n - 1; } \
            break; \
          case PS_BINARY_DISECT_INVALID_INPUT: \
          case PS_BINARY_DISECT_INVALID_TYPE: \
            psAbort ("programming error"); \
            break; \
        } }

# define PS_BIN_INTERPOLATE(RESULT, VECTOR, BOUNDS, BIN, VALUE) { \
        float dX, dY, Xo, Yo, Xt; \
        if (BIN == BOUNDS->n - 1) { \
            dX = 0.5*(BOUNDS->data.F32[BIN+1] - BOUNDS->data.F32[BIN-1]); \
            dY = VECTOR->data.F32[BIN] - VECTOR->data.F32[BIN-1]; \
            Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]); \
            Yo = VECTOR->data.F32[BIN]; \
        } else { \
            dX = 0.5*(BOUNDS->data.F32[BIN+2] - BOUNDS->data.F32[BIN]); \
            dY = VECTOR->data.F32[BIN+1] - VECTOR->data.F32[BIN]; \
            Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]); \
            Yo = VECTOR->data.F32[BIN]; \
        } \
        if (dY != 0) { \
            Xt = (VALUE - Yo)*dX/dY + Xo; \
        } else { \
            Xt = Xo; \
        } \
        Xt = PS_MIN (BOUNDS->data.F32[BIN+1], PS_MAX(BOUNDS->data.F32[BIN], Xt)); \
        psTrace(TRACE, 6, "(Xo, Yo, dX, dY, Xt, Yt) is (%.2f %.2f %.2f %.2f %.2f %.2f)\n", \
                Xo, Yo, dX, dY, Xt, VALUE); \
        RESULT = Xt; }

# define COUNT_WARNING(LIMIT, INTERVAL, ...) { \
        static int nCalls = 1; \
        if (nCalls < LIMIT) { \
            psWarning(__VA_ARGS__); \
        } \
        if (!(nCalls % INTERVAL)) { \
            psWarning(__VA_ARGS__); \
            psWarning("(warning raised %d times)", nCalls); \
        } \
        nCalls ++; \
}

// Debug information
#define CZW 0

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

// None

/*****************************************************************************/
/* GLOBAL VARIABLES                                                          */
/*****************************************************************************/

// None

/*****************************************************************************/
/* FILE STATIC VARIABLES                                                     */
/*****************************************************************************/

// None

/******************************************************************************
MISC PRIVATE STATISTICAL FUNCTIONS

NOTE: it is assumed that any call to these statistical functions will have
been preceded by a call to the psVectorStats() function.  Various sanity tests
will only be performed in psVectorStats().

For many of these private stats routines, it is possible that there are no acceptable elements
in the input vector (if no elements lie within range, or there are no unmasked elements, or the
input vector is NULL).  In such cases, we set the desired value to NAN and return an error.
The calling function may clear the error.
*****************************************************************************/

// static psF32 fitQuadraticSearchForYThenReturnBin(const psVector *xVec, psVector *yVec, psS32 binNum, psF32 yVal);
static psF32 fitLinearSearchForYThenReturnBin(const psVector *xVec, psVector *yVec, psS32 binNum, psF32 yVal);

/******************************************************************************
vectorSampleMean(myVector, maskVector, maskVal, stats): calculates the
mean of the input vector.  If there was a problem with the mean calculation,
this routine sets stats->sampleMean to NAN.

using the method below, with a single loop for various options costs only a small amount and is
much easier to debug. running 10000 tests of 1000 point vectors for the two methods gives:

                     separate    single          w/errors
(mask: 0, range: 0): 0.067 sec  0.073 sec (10%)  0.072 sec
(mask: 1, range: 0): 0.098 sec  0.102 sec ( 4%)  0.119 sec
(mask: 0, range: 0): 0.136 sec  0.162 sec (20%)  0.170 sec
(mask: 1, range: 0): 0.177 sec  0.181 sec ( 2%)  0.198 sec

(effect of errors not tested)

To optmize this, use a macro and ifdef in or out the three states (errors, mask, range)
*****************************************************************************/
    static bool vectorSampleMean(const psVector* myVector,
                                 const psVector* errors,
                                 const psVector* maskVector,
                                 psVectorMaskType maskVal,
                                 psStats* stats)
{
    long count = 0;                     // Number of points contributing to this mean
    psF32 mean = 0.0;                   // The mean
    psF32 weight;

    psF32 *data = myVector->data.F32;   // Dereference
    int numData = myVector->n;          // Number of data points

    psVectorMaskType *maskData = (maskVector == NULL) ? NULL : maskVector->data.PS_TYPE_VECTOR_MASK_DATA;
    bool useRange = stats->options & PS_STAT_USE_RANGE;

    psF32 sumWeights = 0.0;  // The sum of the weights
    psF32 *errorsData = (errors == NULL) ? NULL : errors->data.F32;

    for (long i = 0; i < numData; i++) {
        // Check if the data is with the specified range
        if (!isfinite(data[i]))
            continue;
        if (useRange && (data[i] < stats->min))
            continue;
        if (useRange && (data[i] > stats->max))
            continue;
        if (maskData && (maskData[i] & maskVal))
            continue;
        if (errors) {
            weight = (errorsData[i] == 0) ? 0.0 : PS_SQR(errorsData[i]);
            mean += data[i] * weight;
            sumWeights += weight;
        } else {
            mean += data[i];
        }
        count++;

    }
    if (errors) {
        mean = (count > 0) ? mean / sumWeights : NAN;
    } else {
        mean = (count > 0) ? mean / count : NAN;
    }
    stats->sampleMean = mean;

    if (!isnan(mean)) {
        stats->results |= PS_STAT_SAMPLE_MEAN;
    }
    return true;
}

/******************************************************************************
vectorMinMax(myVector, maskVector, maskVal, stats): calculates the minimum and maximum of the input vector.
If there was a problem with the calculation, this routine sets stats->max and stats->min to NAN.  Return the
number of valid values in the vector (those not masked or outside the specified range).
XXX : using the method below, with a single loop for various options
      costs only a small amount and is much easier to debug. running 10000 tests
      of 1000 point vectors for the two methods gives:
                     separate    single
(mask: 0, range: 0):  0.101 sec  0.149 sec
(mask: 1, range: 0):  0.125 sec  0.160 sec
(mask: 0, range: 0):  0.179 sec  0.208 sec
(mask: 1, range: 0):  0.200 sec  0.244 sec
*****************************************************************************/
    static long vectorMinMax(const psVector* myVector,
                             const psVector* maskVector,
                             psVectorMaskType maskVal,
                             psStats* stats
        )
{
    psF32 max = -PS_MAX_F32;            // The calculated maximum
    psF32 min = PS_MAX_F32;             // The calculated minimum
    psF32 *vector = myVector->data.F32; // Dereference the vector

    int num = myVector->n;              // Number of values
    int numValid = 0;                   // Number of valid values

    psVectorMaskType *maskData = (maskVector == NULL) ? NULL : maskVector->data.PS_TYPE_VECTOR_MASK_DATA;
    bool useRange = stats->options & PS_STAT_USE_RANGE;

    for (long i = 0; i < num; i++) {
        // Check if the data is with the specified range
        if (!isfinite(vector[i]))
            continue;
        if (useRange && (vector[i] < stats->min))
            continue;
        if (useRange && (vector[i] > stats->max))
            continue;
        if (maskData && (maskData[i] & maskVal))
            continue;

        numValid++;
        max = PS_MAX (vector[i], max);
        min = PS_MIN (vector[i], min);
    }

    // XXX save numValid in psStats?
    if (numValid == 0) {
        stats->max = NAN;
        stats->min = NAN;
    } else {
        stats->max = max;
        stats->min = min;
        stats->results |= PS_STAT_MIN;
        stats->results |= PS_STAT_MAX;
    }
    return numValid;
}

/******************************************************************************
vectorSampleMedian(myVector, maskVector, maskVal, stats): calculates the median
and quartiles of the input vector.  Returns true on success (including if there
were no valid input vector elements). Expects F32 vector for input.
*****************************************************************************/
static bool vectorSampleMedian(const psVector* inVector,
                               const psVector* maskVector,
                               psVectorMaskType maskVal,
                               psStats* stats)
{
    bool useRange = stats->options & PS_STAT_USE_RANGE;
    psVectorMaskType *maskData = (maskVector == NULL) ? NULL : maskVector->data.PS_TYPE_VECTOR_MASK_DATA; // Dereference the vector
    psF32 *input = inVector->data.F32; // Dereference the vector

    // use the temporary vector for the sorted output
    stats->tmpData = psVectorRecycle (stats->tmpData, inVector->n, PS_TYPE_F32);
    psVector *outVector = stats->tmpData;
    psF32 *output = outVector->data.F32; // Dereference the vector

    if (maskData) psAssert (maskVector->n == inVector->n, "oops");

    long count = 0;                     // Number of valid entries

    // Store all non-masked data points within the min/max range
    // into the temporary vectors.
    for (long i = 0; i < inVector->n; i++) {
	psAssert (count >= 0, "oops");
	psAssert (count < outVector->n, "oops");
	psAssert (i >= 0, "oops");
	psAssert (i < inVector->n, "oops");

        if (!isfinite(input[i])) continue;
        if (useRange && (input[i] < stats->min)) continue;
        if (useRange && (input[i] > stats->max)) continue;
        if (maskData && (maskData[i] & maskVal)) continue;

        output[count] = input[i];
        count++;
    }
    outVector->n = count;

    if (count == 0) {
        COUNT_WARNING(10, 100, "No valid data in input vector.\n");
        stats->sampleUQ = NAN;
        stats->sampleLQ = NAN;
        stats->sampleMedian = NAN;
        return true;
    }

    // Sort the temporary vector.
    if (!psVectorSort(outVector, outVector)) { // Sort in-place (since it's a copy, it's OK)
        // an error in psVectorSort is a serious error:
	// NULL input vector, psVectorCopy failure, invalid vector type
        psError(PS_ERR_UNEXPECTED_NULL, false, _("Failed to sort input data."));
        stats->sampleUQ = NAN;
        stats->sampleLQ = NAN;
        stats->sampleMedian = NAN;
        return false;
    }

    // Calculate the median.  Use the average if the number of samples if even.
    int midPt = (count/2);
    psAssert (midPt >=           0, "oops");
    psAssert (midPt < outVector->n, "oops");
    if (count % 2 == 0) {
	psAssert ((midPt - 1) >=           0, "oops");
	psAssert ((midPt - 1) < outVector->n, "oops");
        stats->sampleMedian = 0.5 * (output[midPt - 1] + output[midPt]);
    } else {
        stats->sampleMedian = output[midPt];
    }

    int Qmin = (int)(0.25*count);
    int Qmax = (int)(0.75*count);
    psAssert (Qmin >=           0, "oops");
    psAssert (Qmin < outVector->n, "oops");
    psAssert (Qmax >=           0, "oops");
    psAssert (Qmax < outVector->n, "oops");

    // Calculate the quartile points exactly.
    stats->sampleUQ = output[Qmax];
    stats->sampleLQ = output[Qmin];
     
    stats->results |= PS_STAT_SAMPLE_MEDIAN;
    stats->results |= PS_STAT_SAMPLE_QUARTILE;

    return true;
}

/******************************************************************************
vectorSampleStdev(myVector, maskVector, maskVal, stats): calculates the
stdev of the input vector.
Inputs
    myVector
    maskVector
    maskVal
    stats
Returns
    NULL

using the method below, with a single loop for various options costs only a small amount and is
much easier to debug.
*****************************************************************************/

static bool vectorSampleStdev(const psVector* myVector,
                              const psVector* errors,
                              const psVector* maskVector,
                              psVectorMaskType maskVal,
                              psStats* stats)
{
    // This procedure requires the mean.  If it has not been already
    // calculated, then call vectorSampleMean()
    if (!(stats->results & PS_STAT_SAMPLE_MEAN)) {
        vectorSampleMean(myVector, errors, maskVector, maskVal, stats);
    }

    // If the mean is NAN, then generate a warning and set the stdev to NAN.
    if (isnan(stats->sampleMean)) {
        COUNT_WARNING(10, 100, "WARNING: vectorSampleStdev(): sample mean is NAN. Setting stats->sampleStdev = NAN.");
        stats->sampleStdev = NAN;
        return true;
    }

    psF32 *data = myVector->data.F32;   // Dereference
    psVectorMaskType *maskData = (maskVector == NULL) ? NULL : maskVector->data.PS_TYPE_VECTOR_MASK_DATA;
    bool useRange = stats->options & PS_STAT_USE_RANGE;
    psF32 *errorsData = (errors == NULL) ? NULL : errors->data.F32;

    // Accumulate the sums
    double mean = stats->sampleMean;    // The mean
    double sumSquares = 0.0;            // Sum of the squares
    double sumDiffs = 0.0;              // Sum of the differences
    double errorDivisor = 0.0;          // Division for errors
    long count = 0;                     // Number of data points being used
    for (long i = 0; i < myVector->n; i++) {
        // Check if the data is with the specified range
        if (!isfinite(data[i]))
            continue;
        if (useRange && (data[i] < stats->min)) {
            continue;
        }
        if (useRange && (data[i] > stats->max)) {
            continue;
        }
        if (maskData && (maskData[i] & maskVal)) {
            continue;
        }

        double diff = data[i] - mean;
        sumSquares += PS_SQR(diff);
        sumDiffs += diff;
        count++;
        if (errors) {
            errorDivisor += 1.0 / PS_SQR(errorsData[i]);
        }
    }

    if (count == 0) {
        // This is an ambiguous case: error or not?
        // It's not an empty array (that's been asserted on previously), but everything's been masked out.
        // Assume that the user knows what he's doing when he masks out everything --> no error.
        stats->sampleStdev = NAN;
        COUNT_WARNING(10, 100, "WARNING: vectorSampleStdev(): no valid psVector elements (%ld). Setting stats->sampleStdev = NAN.\n", count);
        return true;
    }
    if (count == 1) {
        stats->sampleStdev = 0.0;
        COUNT_WARNING(10, 100, "WARNING: vectorSampleStdev(): only one valid psVector elements (%ld). Setting stats->sampleStdev = 0.0.\n", count);
        return true;
    }

    if (errors) {
        stats->sampleStdev = (1.0 / sqrtf(errorDivisor));
    } else {
        stats->sampleStdev = sqrt((sumSquares - (sumDiffs * sumDiffs / (float)count)) / (float)(count - 1));
    }
    stats->results |= PS_STAT_SAMPLE_STDEV;

    return true;
}

static bool vectorSampleMoments(const psVector* myVector,
                                const psVector* maskVector,
                                psVectorMaskType maskVal,
                                psStats* stats)
{
    // This procedure requires the mean and standard deviation
    if (!(stats->results & PS_STAT_SAMPLE_MEAN)) {
        vectorSampleMean(myVector, NULL, maskVector, maskVal, stats);
    }
    if (isnan(stats->sampleMean)) {
        COUNT_WARNING(10, 100, "WARNING: vectorSampleMoments(): sample mean is NAN.\n");
        goto SAMPLE_MOMENTS_BAD;
    }
    if (!(stats->results & PS_STAT_SAMPLE_STDEV)) {
        vectorSampleStdev(myVector, NULL, maskVector, maskVal, stats);
    }
    if (isnan(stats->sampleStdev) || stats->sampleStdev == 0.0) {
        COUNT_WARNING(10, 100, "WARNING: vectorSampleMoments(): sample stdev is NAN or 0.\n");
        goto SAMPLE_MOMENTS_BAD;
    }

    psF32 *data = myVector->data.F32;   // Dereference
    psVectorMaskType *maskData = (maskVector == NULL) ? NULL : maskVector->data.PS_TYPE_VECTOR_MASK_DATA;
    bool useRange = stats->options & PS_STAT_USE_RANGE;

    // Accumulate the sums
    double mean = stats->sampleMean;    // The mean
    double sum3 = 0.0;                  // Sum of the cubes of the differences
    double sum4 = 0.0;                  // Sum of the fourth powers of the differences
    long count = 0;                     // Number of data points being used
    for (long i = 0; i < myVector->n; i++) {
        // Check if the data is with the specified range
        if (!isfinite(data[i]))
            continue;
        if (useRange && (data[i] < stats->min)) {
            continue;
        }
        if (useRange && (data[i] > stats->max)) {
            continue;
        }
        if (maskData && (maskData[i] & maskVal)) {
            continue;
        }

        double diff = data[i] - mean;   // Difference from the mean
        double temp;                    // Temporary variable for accumulating

        sum3 += temp = PS_SQR(diff);
        sum4 += temp * diff;

        count++;
    }

    psAssert(count > 1, "impossible");                  // It should be, because we have a mean and standard deviation

    double stdev = stats->sampleStdev;  // Standard deviation
    double variance = PS_SQR(stdev);    // Variance

    // Formula for skewness and kurtosis from Numerical Recipes in C, p 613.
    // Note that we are defining the kurtosis relative to a normal distribution (hence the -3.0).
    stats->sampleSkewness = sum3 / (count * variance * stdev);
    stats->sampleKurtosis = sum4 / (count * PS_SQR(variance)) - 3.0;
    stats->results |= PS_STAT_SAMPLE_SKEWNESS | PS_STAT_SAMPLE_KURTOSIS;

    return true;

 SAMPLE_MOMENTS_BAD:
    // stats->sampleStdev has already been set
    stats->sampleSkewness = NAN;
    stats->sampleKurtosis = NAN;
    return true;
}

/******************************************************************************
vectorClippedStats(myVector, errors, maskInput, maskValInput, stats): calculates the
clipped stats (mean or stdev) of the input vector.

Inputs
    myVector
    maskInput
    maskValInput
    stats
Returns
    true for success; false otherwise
*****************************************************************************/
static bool vectorClippedStats(const psVector* myVector,
                               const psVector* errors,
                               psVector* maskInput,
                               psVectorMaskType maskValInput,
                               psStats* stats
    )
{
    // Ensure that stats->clipIter is within the proper range.
    PS_ASSERT_INT_WITHIN_RANGE(stats->clipIter,
                               PS_CLIPPED_NUM_ITER_LB,
                               PS_CLIPPED_NUM_ITER_UB, -1);

    // Ensure that stats->clipSigma is within the proper range.
    PS_ASSERT_FLOAT_WITHIN_RANGE(stats->clipSigma,
                                 PS_CLIPPED_SIGMA_LB,
                                 PS_CLIPPED_SIGMA_UB, -1);

    // unless we succeed, these will have NAN values
    stats->clippedMean = NAN;
    stats->clippedStdev = NAN;
    stats->clippedNvalues = 0;

    // We copy the mask vector, to preserve the original
    psVectorMaskType maskVal = MASK_MARK | maskValInput;

    // use the temporary vector for local temporary mask
    stats->tmpMask = psVectorRecycle (stats->tmpMask, myVector->n, PS_TYPE_VECTOR_MASK);
    psVector *tmpMask = stats->tmpMask;
    psVectorInit(tmpMask, 0);
    if (maskInput) {
        for (long i = 0; i < myVector->n; i++) {
            if (maskInput->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValInput) {
                tmpMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = maskVal;
            }
        }
    }

    // 1. Compute the sample median, which we save for output
    vectorSampleMedian(myVector, tmpMask, maskVal, stats);
    if (isnan(stats->sampleMedian)) {
        COUNT_WARNING(10, 100, "Call to vectorSampleMedian returned NAN\n");
        return true;
    }
    psTrace(TRACE, 6, "The initial sample median is %f\n", stats->sampleMedian);

    // 2. Compute the sample standard deviation, which we save for output
    vectorSampleStdev(myVector, errors, tmpMask, maskVal, stats);
    if (isnan(stats->sampleStdev)) {
        COUNT_WARNING(10, 100, "Call to vectorSampleStdev returned NAN\n");
        return true;
    }
    psTrace(TRACE, 6, "The initial sample stdev is %f\n", stats->sampleStdev);

    // 3. Use the sample median as the first estimator of the mean X.
    psF32 clippedMean = stats->sampleMedian;

    // 4. Use the sample stdev as the first estimator of the mean stdev.
    psF32 clippedStdev = stats->sampleStdev;

    // 5. Repeat N (stats->clipIter) times:
    long numClipped = 0;                // Number of values clipped
    bool clipped = true;                // Have we clipped anything in this iteration
    for (int iter = 0; iter < stats->clipIter && clipped; iter++) {
        clipped = false;
        psTrace(TRACE, 6, "------------ Iteration %d ------------\n", iter);
        // a) Exclude all values x_i for which |x_i - x| > K * stdev
        if (errors) {
            // XXXX if we convert errors to variance, this should square the other terms (A*A faster than
            // sqrt(A))
            for (long j = 0; j < myVector->n; j++) {
                if (!tmpMask->data.PS_TYPE_VECTOR_MASK_DATA[j] &&
                    fabsf(myVector->data.F32[j] - clippedMean) > stats->clipSigma * errors->data.F32[j]) {
                    tmpMask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 0xff;
                    psTrace(TRACE, 10, "Clipped %ld: %f +/- %f\n", j,
                            myVector->data.F32[j], errors->data.F32[j]);
                    numClipped++;
                    clipped = true;
                }
            }
        } else {
            for (long j = 0; j < myVector->n; j++) {
                if (!tmpMask->data.PS_TYPE_VECTOR_MASK_DATA[j] &&
                    fabsf(myVector->data.F32[j] - clippedMean) > (stats->clipSigma * clippedStdev)) {
                    tmpMask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 0xff;
                    psTrace(TRACE, 10, "Clipped %ld: %f\n", j, myVector->data.F32[j]);
                    numClipped++;
                    clipped = true;
                }
            }
        }

        // b) compute new mean and stdev
        // Allocate a psStats structure for calculating the mean and stdev.
        // XXX Can we just use this psStats structure to calculate the SAMPLE MEAN and STDEV?
        // psStats *statsTmp = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        // vectorSampleMean(myVector, errors, tmpMask, maskVal, statsTmp);
        // vectorSampleStdev(myVector, errors, tmpMask, maskVal, statsTmp);
        vectorSampleMean(myVector, errors, tmpMask, maskVal, stats);
        vectorSampleStdev(myVector, errors, tmpMask, maskVal, stats);
        psTrace(TRACE, 6, "The new sample mean is %f\n", stats->sampleMean);
        psTrace(TRACE, 6, "The new sample stdev is %f\n", stats->sampleStdev);

        // If the new mean and stdev are NAN, we must exit the loop.
        // Otherwise, use the new results and continue.
        if (isnan(stats->sampleMean) || isnan(stats->sampleStdev)) {
            iter = stats->clipIter;
            COUNT_WARNING(10, 100, "vectorSampleMean() or vectorSampleStdev() returned a NAN.\n");
            clippedMean = NAN;
            clippedStdev = NAN;
            return true;
        } else {
            clippedMean = stats->sampleMean;
            clippedStdev = stats->sampleStdev;
        }
        // psFree(statsTmp);
    }

    // Number of values used in calculation is the total number of data values, minus those we clipped
    stats->clippedNvalues = myVector->n - numClipped;

    // 7. The last calcuated value of x is the clipped mean.
    // 8. The last calcuated value of stdev is the clipped stdev.
    // we always return both stats even if only one was requested
    stats->clippedMean = clippedMean;
    stats->clippedStdev = clippedStdev;

    stats->results |= PS_STAT_CLIPPED_MEAN;
    stats->results |= PS_STAT_CLIPPED_STDEV;

    psTrace(TRACE, 6, "The final clipped mean is %f\n", clippedMean);
    psTrace(TRACE, 6, "The final clipped stdev is %f\n", clippedStdev);

    return true;
}

/******************************************************************************
vectorRobustStats(myVector, maskInput, maskValInput, stats): This is the new
version of the robust stats routine.

XXX: MUST DO: If the errors in the input values are known, then the same
approach is used, except that the histograms become probability density
functions (PDFs). In this case, the input values are spread out, so that they
do not simply contribute a single unit to the histogram, but rather contribute
a fraction of a value, equivalent to the weight. In the interests of speed, a
boxcar PDF may be used to represent each input value (as opposed to a
Gaussian), where the boxcar width is equal to 2p2 ln 2 times the error and
each input value contributes constant area. Then the robust median and
standard deviation are estimated in the same manner as above.

XXX: Check for errors in psLib routines that we call.

XXX: Review and ensure that all memory is free'ed at premature exits.
*****************************************************************************/
#define INITIAL_NUM_BINS 1000.0
static bool vectorRobustStats(const psVector* myVector,
                              const psVector* errors,
                              psVector* maskInput,
                              psVectorMaskType maskValInput,
                              psStats* stats)
{
    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(myVector);
    }

    // we need to generate an internal mask and maskVal which ensures a bit is set
    // and tested even if there is no supplied mask (and/or the maskVal is 0)
    // XXX this would be better if we had globally defined mask values
    psVectorMaskType maskVal = MASK_MARK | maskValInput;
    psVector *mask = psVectorAlloc(myVector->n, PS_TYPE_VECTOR_MASK); // The actual mask we will use
    psVectorInit(mask, 0);
    if (maskInput) {
        for (long i = 0; i < myVector->n; i++) {
            if (maskInput->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValInput) {
                mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = maskVal;
            }
        }
    }

    // statsMinMax is only applied to a subset of the data points
    psStats *statsMinMax = psStatsAlloc(PS_STAT_MIN | PS_STAT_MAX); // Statistics for min and max
    psHistogram *histogram = NULL;      // Histogram of the data
    psHistogram *cumulative = NULL;     // Cumulative histogram of the data
    float min = NAN, max = NAN;         // Mimimum and maximum values
    float sigma = NAN;                  // The robust standard deviation
    long totalDataPoints = 0;           // Total number of (unmasked) data points

    float binSize = 0.0;            // Size of bins for histogram
    long binLo, binHi;
    long binL2, binH2;
    long binL4, binH4;
    long binMedian;

    // Iterate to get the best bin size; an iteration limit is enforced at the bottom of the loop.
    for (int iterate = 1; iterate > 0; iterate++) {
        psTrace(TRACE, 6, "-------------------- Iterating on Bin size.  Iteration number %d --------------------\n", iterate);

        if (iterate >= PS_ROBUST_MAX_ITERATIONS) {
          // This occurs when a large number of the values are identical --- a bin size cannot be found
          // that will spread out the distribution.  Therefore, set what we can, and fall over
          // gracefully.
          COUNT_WARNING(10, 100, "Maximum number of iterations (%d) exceeded.", PS_ROBUST_MAX_ITERATIONS);
          goto escape;
        }

        // Get the minimum and maximum values
        int numValid = vectorMinMax(myVector, mask, maskVal, statsMinMax); // Number of valid values
        min = statsMinMax->min;
        max = statsMinMax->max;
        if (numValid == 0 || isnan(min) || isnan(max)) {
            // Data range calculation failed
            COUNT_WARNING(10, 100, "Failed to calculate the min/max of the input vector.\n");
            goto escape;
        }
        if (!isfinite(max - min)) {
            COUNT_WARNING(10, 100, "Range of of the input vector is too large: %lf.\n", (double)max - (double) min);
            goto escape;
        }
        psTrace(TRACE, 6, "Data min/max is (%.2f, %.2f)\n", min, max);

        // If all data points have the same value, then we set the appropriate members of stats and return.
        if (fabs(max - min) <= FLT_EPSILON) {
            stats->robustMedian = min;
            stats->robustStdev = 0.0;
            stats->robustUQ = min;
            stats->robustLQ = min;
            stats->robustN50 = numValid;
            // XXX this is sort of an invalid / out-of-bounds result: to set or not to set these bits:
            stats->results |= PS_STAT_ROBUST_MEDIAN;
            stats->results |= PS_STAT_ROBUST_STDEV;
            stats->results |= PS_STAT_ROBUST_QUARTILE;
            COUNT_WARNING(10, 100, "All data points have the same value: %f.\n", min);
            psFree(mask);
            psFree(statsMinMax);
            return true;
        }

        if ((iterate == 1) && (stats->options & PS_STAT_USE_BINSIZE)) {
            // Set initial bin size to the specified value.
            binSize = stats->binsize;
            psTrace(TRACE, 6, "Setting initial robust bin size to %.2f\n", binSize);
        } else {
            // Determine the bin size of the robust histogram, using the pre-defined number of bins
	    binSize = (max - min) / INITIAL_NUM_BINS;
        }
        psTrace(TRACE, 6, "Initial robust bin size is %.2f\n", binSize);

        // ADD step 0: Construct the histogram with the specified bin size.  NOTE: we can
        // not specify the bin size precisely since the argument to psHistogramAlloc() is
        // the number of bins, not the binSize.  If we get here, we know that binSize !=
        // 0.0.  We can also have a floating-point round-off error such that the last bin
        // of the histogram does not correspond exactly with the value of 'max'.  Let's be
        // a bit generous and extend the histogram by two bins in either direction
        long numBins = 4 + (max - min) / binSize; // Number of bins
        psTrace(TRACE, 6, "Numbins is %ld\n", numBins);
        psTrace(TRACE, 6, "Creating a robust histogram from data range (%.2f - %.2f)\n", min, max);

        // We are sometimes causing psHistogramAlloc to assert.
        // Assert here so we can get more information about what is going wrong.
        psAssert(numBins > 0, "Invalid numBins: %ld max: %f min: %f binSize: %f", numBins, max, min, binSize);
        
        // Generate the histogram
        histogram = psHistogramAlloc(min - 2.0*binSize, max + 2.0*binSize, numBins);
        // XXXXX we need to consider this step if errors -> variance
        if (!psVectorHistogram(histogram, myVector, errors, mask, maskVal)) {
            // if psVectorHistogram returns false, we have a programming error
            psError(PS_ERR_UNKNOWN, false, "Unable to generate histogram for robust statistics.\n");
            psFree(histogram);
            psFree(cumulative);
            psFree(statsMinMax);
            psFree(mask);
            return false;
        }
        if (psTraceGetLevel("psLib.math") >= 8) {
            PS_VECTOR_PRINT_F32(histogram->bounds);
            PS_VECTOR_PRINT_F32(histogram->nums);
        }

        // perversity check: if most of the values land in a single bin, then we probably
        // have a perverse case (eg, small number of points at extremely large / small
        // values; nearly bi-modal distribution).  if so, keep only points within 5? 10?
        // bins of that excess bin:
        int nMaxBin = histogram->nums->data.F32[0];
        int iMaxBin = 0;
        for (long i = 1; i < histogram->nums->n; i++) {
            if (histogram->nums->data.F32[i] > nMaxBin) {
                nMaxBin = histogram->nums->data.F32[i];
                iMaxBin = i;
            }
        }
        if (nMaxBin > numValid / 2) {
            float minKeep = histogram->bounds->data.F32[iMaxBin] - 10*binSize;
            float maxKeep = histogram->bounds->data.F32[iMaxBin + 1] + 10*binSize;
            int nInvalid = 0;
            for (long i = 0; i < myVector->n; i++) {
                // skip the already-masked values
                if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal) continue;
                bool invalid = false;
                invalid |= (myVector->data.F32[i] < minKeep);
                invalid |= (myVector->data.F32[i] > maxKeep);
                invalid |= (!isfinite(myVector->data.F32[i]));
                if (!invalid) continue;
                mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = maskVal;
                nInvalid ++;
            }

            if (nInvalid) {
              psTrace(TRACE, 6, "data is concentrated in a single bin (%d = %f - %f), masking %d extreme outliers and retrying\n", 
		      iMaxBin, histogram->bounds->data.F32[iMaxBin], histogram->bounds->data.F32[iMaxBin+1], nInvalid);
              psFree(histogram);
              psFree(cumulative);
              histogram = NULL;
              cumulative = NULL;
              continue;
            }
            // if we did not mask anything, give up.
        }

        // We were causing psHistogramAlloc to assert. 
        // Assert here so we can get more information about what is going wrong.
        psAssert(numBins > 0, "Invalid numBins %ld max: %f min: %f binSize: %f", numBins, max, min, binSize);

        // ADD step 1: Convert the specific histogram to a cumulative histogram
        // The cumulative histogram data points correspond to the UPPER bound value (N < Bin[i+1])
        cumulative = psHistogramAlloc(min, max, numBins);
        cumulative->nums->data.F32[0] = histogram->nums->data.F32[0];
        cumulative->bounds->data.F32[0] = histogram->bounds->data.F32[1];

	// Correctly fill the cumulative distribution with monotonically increasing values (skip zero valued bins).
	long Nc = 1;  // track the current bin of cumulative
	// the boundaries for the current cumulative bin are from upper end of the last valid histogram bin to the 
	// upper end of the current histogram bin
	for (long i = 1; i < histogram->nums->n - 1; i++) {
	    if (histogram->nums->data.F32[i] == 0.0) continue;
	    cumulative->nums->data.F32[Nc] = cumulative->nums->data.F32[Nc - 1] + histogram->nums->data.F32[i];
	    cumulative->bounds->data.F32[Nc] = histogram->bounds->data.F32[i+1];
	    Nc ++;
	}
	long Nlast = Nc - 1;  // last valid cumulative bin 
	for (long i = Nc; i < histogram->nums->n; i++) { // Ensure the unused entries are filled.
	    cumulative->nums->data.F32[i] = cumulative->nums->data.F32[Nlast];
	    cumulative->bounds->data.F32[i] = cumulative->bounds->data.F32[i-1] + 1.0;
	}
	
        if (psTraceGetLevel("psLib.math") >= 8) {
            PS_VECTOR_PRINT_F32(cumulative->bounds);
            PS_VECTOR_PRINT_F32(cumulative->nums);
        }

        // ADD step 2: Find the bin which contains the 50% data point.
        totalDataPoints = cumulative->nums->data.F32[numBins - 1];
        psTrace(TRACE, 6, "Total data points is %ld\n", totalDataPoints);

        // find bin which is the lower bound of median (value[binMedian] < median < value[binMedian+1]
        PS_BIN_FOR_VALUE(binMedian, cumulative->nums, totalDataPoints/2.0, 0);

        psTrace(TRACE, 6, "The median bin is %ld (%.2f to %.2f)\n", binMedian, cumulative->bounds->data.F32[binMedian], cumulative->bounds->data.F32[binMedian+1]);

        // ADD step 3: Interpolate to the exact 50% position in bin units
        // stats->robustMedian = fitQuadraticSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binMedian, totalDataPoints/2.0);
	// float robustBin = fitQuadraticSearchForYThenReturnXusingValues(cumulative->bounds, cumulative->nums, binMedian, totalDataPoints/2.0);
        // fprintf (stderr, "robustBin : %f vs %f\n", robustBin, stats->robustMedian);
	// There's no reason to do a quadratic fit near the 50% bin, as it's approximately linear there.
	// Instead, do a 5-point linear fit.
        stats->robustMedian = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binMedian, totalDataPoints/2.0);

        // convert bin to bin value: this is the robust histogram median.
        if (isnan(stats->robustMedian)) {
            COUNT_WARNING(10, 100, "Failed to fit a quadratic and calculate the 50-percent position.\n");
            goto escape;
        }
        psTrace(TRACE, 6, "Current robust median is %f\n", stats->robustMedian);

        // ADD step 4: Find the bins which contains the 15.8655% (-1 sigma) and 84.1345% (+1 sigma) data
        // points also find the 30.8538% (-0.5 sigma) and 69.1462% (+0.5 sigma) points
        PS_BIN_FOR_VALUE(binLo, cumulative->nums, totalDataPoints * 0.158655f, 0);
        PS_BIN_FOR_VALUE(binHi, cumulative->nums, totalDataPoints * 0.841345f, 0);
        PS_BIN_FOR_VALUE(binL2, cumulative->nums, totalDataPoints * 0.308538f, 0);
        PS_BIN_FOR_VALUE(binH2, cumulative->nums, totalDataPoints * 0.691462f, 0);
	PS_BIN_FOR_VALUE(binL4, cumulative->nums, totalDataPoints * 0.022750f, 0);
	PS_BIN_FOR_VALUE(binH4, cumulative->nums, totalDataPoints * 0.977250f, 0);
	
	
        psTrace(TRACE, 6, "The 15.8655%% and 84.1345%% data point bins are (%ld, %ld).\n",
                binLo, binHi);
        psTrace(TRACE, 6, "binLo midpoint is %f\n", PS_BIN_MIDPOINT(cumulative, binLo));
        psTrace(TRACE, 6, "binHi midpoint is %f\n", PS_BIN_MIDPOINT(cumulative, binHi));
        psTrace(TRACE, 6, "binL2 midpoint is %f\n", PS_BIN_MIDPOINT(cumulative, binL2));
        psTrace(TRACE, 6, "binH2 midpoint is %f\n", PS_BIN_MIDPOINT(cumulative, binH2));

        if ((binLo < 0) || (binHi < 0)) {
            COUNT_WARNING(10, 100, "Failed to calculate the 15.8655%% and 84.1345%% data points.\n");
            goto escape;
        }
    
        // ADD step 4b: Interpolate Sigma (linearly) to find these two positions exactly: these are the 1sigma
        // positions.
        psTrace(TRACE, 6, "binLo is %ld.  Nums at that bin and the next are (%.2f, %.2f)\n",
                binLo, cumulative->nums->data.F32[binLo], cumulative->nums->data.F32[binLo+1]);
        psTrace(TRACE, 6, "binHi is %ld.  Nums at that bin and the next are (%.2f, %.2f)\n",
                binHi, cumulative->nums->data.F32[binHi], cumulative->nums->data.F32[binHi+1]);

        // find the +0.5 and -0.5 sigma points with linear interpolation.  binLo and binHi are the bins
        // containing the value of interest.  constrain the answer to be within the current bin.
        // (extrapolation should not be needed and will result in errors)
        float binLoF32, binHiF32, binL2F32, binH2F32, binL4F32, binH4F32;
#if (0)
        PS_BIN_INTERPOLATE (binLoF32, cumulative->nums, cumulative->bounds, binLo,
                            totalDataPoints * 0.158655f);
        PS_BIN_INTERPOLATE (binHiF32, cumulative->nums, cumulative->bounds, binHi,
                            totalDataPoints * 0.841345f);
        PS_BIN_INTERPOLATE (binL2F32, cumulative->nums, cumulative->bounds, binL2,
                            totalDataPoints * 0.308538f);
        PS_BIN_INTERPOLATE (binH2F32, cumulative->nums, cumulative->bounds, binH2,
                            totalDataPoints * 0.691462f);
        PS_BIN_INTERPOLATE (binL4F32, cumulative->nums, cumulative->bounds, binL4,
                            totalDataPoints * 0.022750f);
        PS_BIN_INTERPOLATE (binH4F32, cumulative->nums, cumulative->bounds, binH4,
                            totalDataPoints * 0.977250f);
#else
        binLoF32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binLo, totalDataPoints * 0.158655);
	binHiF32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binHi, totalDataPoints * 0.841345);	       
        binL2F32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binL2, totalDataPoints * 0.308538);
	binH2F32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binH2, totalDataPoints * 0.691462);	       
        binL4F32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binL4, totalDataPoints * 0.022750);
	binH4F32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binH4, totalDataPoints * 0.977250);
#endif	
        // report +/- 1 sigma points
        psTrace(TRACE, 5,
                "The exact 15.8655 and 84.1345 percent data point positions are: (%f, %f)\n",
                binLoF32, binHiF32);
        psTrace(TRACE, 5,
                "The exact 30.8538 and 69.1462 percent data point positions are: (%f, %f)\n",
                binL2F32, binH2F32);
        psTrace(TRACE, 5,
                "The exact 02.2275 and 97.7250 percent data point positions are: (%f, %f)\n",
                binL4F32, binH4F32);

	// If some of the fits failed, attempt to fix this
	if (!isfinite(binLoF32) && isfinite(binHiF32)) { binLoF32 = -1.0 * binHiF32; }
	if (!isfinite(binHiF32) && isfinite(binLoF32)) { binHiF32 = -1.0 * binLoF32; }
	if (!isfinite(binL2F32) && isfinite(binH2F32)) { binL2F32 = -1.0 * binH2F32; }
	if (!isfinite(binH2F32) && isfinite(binL2F32)) { binH2F32 = -1.0 * binL2F32; }
	if (!isfinite(binL4F32) && isfinite(binH4F32)) { binL4F32 = -1.0 * binH4F32; }
	if (!isfinite(binH4F32) && isfinite(binL4F32)) { binH4F32 = -1.0 * binL4F32; }
	
        // ADD step 5: Determine SIGMA as the distance between binL2 and binH2 (+/- 0.5 sigma)


        float sigma1 = (binH2F32 - binL2F32);
        float sigma2 = (binHiF32 - binLoF32) / 2.0;
        float sigma4 = (binH4F32 - binL4F32) / 4.0;

	// Fix again?
	if (!isfinite(sigma1) && isfinite(sigma2) && isfinite(sigma4)) { sigma1 = (sigma2 + sigma4) / 2.0; }
	if (!isfinite(sigma2) && isfinite(sigma1) && isfinite(sigma4)) { sigma2 = (sigma1 + sigma4) / 2.0; }
	if (!isfinite(sigma4) && isfinite(sigma2) && isfinite(sigma1)) { sigma4 = (sigma2 + sigma1) / 2.0; }
	
        // take the smallest of the three: if we have a clump with wide outliers, sigma2 and
        // sigma4 will be biased high; if we have a bi-modal distribution, sigma1 and sigma2
        // will be biased high.
	//        sigma = PS_MIN (sigma1, PS_MIN (sigma2, sigma4));
	// CZW: Instead, take the median.  Taking the MIN forces a bias on unbiased data.
	//      It seems like occasionally getting the wrong answer on a complex distribution
	//      is more acceptable than always getting the wrong answer for simple ones.

	
	sigma = PS_MAX( PS_MIN(sigma1,sigma2),
			PS_MIN( PS_MAX(sigma1,sigma2),
				sigma4));

        psTrace(TRACE, 6, "The 1x sigma is %f.\n", sigma1);
        psTrace(TRACE, 6, "The 2x sigma is %f.\n", sigma2);
        psTrace(TRACE, 6, "The 4x sigma is %f.\n", sigma4);

        psTrace(TRACE, 6, "The current sigma is %f.\n", sigma);
	//        stats->robustStdev = sigma;
	stats->robustStdev = sigma;

#if (CZW && 0) 
	// Skewness check: Find least biased sample for each pair.
	sigma1 = 2.0 * PS_MIN(binH2F32 - stats->robustMedian,
			      stats->robustMedian - binL2F32);
	sigma2 = 1.0 * PS_MIN(binHiF32 - stats->robustMedian,
			      stats->robustMedian - binLoF32);
	sigma4 = 0.5 * PS_MIN(binH4F32 - stats->robustMedian,
			      stats->robustMedian - binL4F32);
	// Kurtosis check: Take median sample as the solution.
	stats->robustStdev = PS_MAX( PS_MIN(sigma1,sigma2),
				     PS_MIN( PS_MAX(sigma1,sigma2),
					     sigma4));
#endif

	
#if (CZW)
	//	printf("CZW: bad sigma?: %f %f  %f %f  %f %f  %f %f %f  %f\n",
	//	       binH2F32,binL2F32,binHiF32,binLoF32,binH4F32,binL4F32,
	//	       sigma1,sigma2,sigma4,sigma);
	
	printf("CZW Robust (%d): median %f sigma %f delta: %f \n\t %f %f %f %f %f %f %f \n\t %f %f %f %f %f %f %f\n",
	       iterate,
	       stats->robustMedian,stats->robustStdev,
	       fabs(cumulative->bounds->data.F32[binMedian] - cumulative->bounds->data.F32[binMedian + 1]),
	       
	       cumulative->bounds->data.F32[binMedian-3],cumulative->bounds->data.F32[binMedian-2],
	       cumulative->bounds->data.F32[binMedian-1],
	       cumulative->bounds->data.F32[binMedian],
	       cumulative->bounds->data.F32[binMedian+1],
	       cumulative->bounds->data.F32[binMedian+2],cumulative->bounds->data.F32[binMedian+3],
	       
	       cumulative->nums->data.F32[binMedian-3],cumulative->nums->data.F32[binMedian-2],
	       cumulative->nums->data.F32[binMedian-1],
	       cumulative->nums->data.F32[binMedian],
	       cumulative->nums->data.F32[binMedian+1],
	       cumulative->nums->data.F32[binMedian+2],cumulative->nums->data.F32[binMedian+3]);
	//	PS_VECTOR_PRINT_F32(histogram->bounds);
	//	PS_VECTOR_PRINT_F32(histogram->nums);
	//	PS_VECTOR_PRINT_F32(cumulative->bounds);
	//	PS_VECTOR_PRINT_F32(cumulative->nums);
#endif

        // ADD step 6: If the measured SIGMA is less than 2 times the bin size, exclude points which are more
        // than 25 bins from the median, recalculate the bin size, and perform the algorithm again.
        if (sigma < (3.0 * binSize)) {
            psTrace(TRACE, 6, "*************: Do another iteration (%f %f).\n", sigma, binSize);

	    // these limits are supposed to be 25 x the raw bin size, NOT 25 of the cumulative histogram bins
	    psF32 medianLo = stats->robustMedian - 25*binSize;
	    psF32 medianHi = stats->robustMedian + 25*binSize;

            // long maskLo = PS_MAX(0, (binMedian - 25)); // Low index for masking region
            // long maskHi = PS_MIN(cumulative->bounds->n - 1, (binMedian + 25)); // High index for masking
            // psF32 medianLo = cumulative->bounds->data.F32[maskLo]; // Value at low index
            // psF32 medianHi = cumulative->bounds->data.F32[maskHi]; // Value at high index
            psTrace(TRACE, 6, "Masking data more than 25 bins from the median\n");
            // psTrace(TRACE, 6, "The median is at bin number %ld.  We mask bins outside the bin range (%ld:%ld)\n", binMedian, maskLo, maskHi);
            psTrace(TRACE, 6, "Masking data outside (%f %f)\n", medianLo, medianHi);
	    int Nmasked = 0;
            for (long i = 0 ; i < myVector->n ; i++) {
                if ((myVector->data.F32[i] < medianLo) || (myVector->data.F32[i] > medianHi)) {
                    if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & MASK_MARK) continue;
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= MASK_MARK;
                    psTrace(TRACE, 6, "Masking element %ld is %f\n", i, myVector->data.F32[i]);
		    Nmasked ++;
                }
            }

	    if (Nmasked == 0) {
		// no significant change to the sigma & binsize -- we are done here 
		iterate = -1;
		continue;
	    }

            // Free the histograms; they will be recreated on the next iteration, with new bounds
            psFree(histogram);
            histogram = NULL;

            psFree(cumulative);
            cumulative = NULL;

            if (iterate >= PS_ROBUST_MAX_ITERATIONS) {
                // This occurs when a large number of the values are identical --- a bin size cannot be found
                // that will spread out the distribution.  Therefore, set what we can, and fall over
                // gracefully.
                // stats->robustMedian has already been set
                stats->robustStdev = sigma;
                stats->robustUQ = stats->robustMedian;
                stats->robustLQ = stats->robustMedian;
                stats->robustN50 = numValid;
                stats->results |= PS_STAT_ROBUST_MEDIAN;
                stats->results |= PS_STAT_ROBUST_STDEV;
                stats->results |= PS_STAT_ROBUST_QUARTILE;
                COUNT_WARNING(10, 100, "Maximum number of iterations (%d) exceeded.", PS_ROBUST_MAX_ITERATIONS);
                psFree(mask);
                psFree(statsMinMax);
                return true;
            }
        } else {
            // We've got the bin size correct now
            psTrace(TRACE, 6, "*************: No more iteration.  sigma is %f\n", sigma);
            iterate = -1;
        }
    }
    
    // XXX test lines while studying algorithm errors
    // fprintf (stderr, "robust stats test %7.1f +/- %7.1f : %4ld %4ld %4ld %4ld %4ld  : %f %f %f\n",
    // stats->robustMedian, stats->robustStdev,
    // binLo, binL2, binMedian, binH2, binHi, binSize, max, min);

    // ADD step 7: Find the bins which contains the 25% and 75% data points.
    long binLo25, binHi25;
    PS_BIN_FOR_VALUE (binLo25, cumulative->nums, totalDataPoints * 0.25f, 0);
    PS_BIN_FOR_VALUE (binHi25, cumulative->nums, totalDataPoints * 0.75f, 0);
    psTrace(TRACE, 6, "The 25-percent and 75-percent data point bins are (%ld, %ld).\n", binLo25, binHi25);

    // ADD step 8: Interpolate to find these two positions exactly: these are the upper and lower quartile
    // positions.
    psF32 binLo25F32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binLo25, totalDataPoints * 0.25f);
    psF32 binHi25F32 = fitLinearSearchForYThenReturnBin(cumulative->bounds, cumulative->nums, binHi25, totalDataPoints * 0.75f);
    if (isnan(binLo25F32) || isnan(binHi25F32)) {
        COUNT_WARNING(10, 100, "could not determine the robustUQ or LQ: fitLinearSearchForYThenReturnBin() returned a NAN.\n");
        goto escape;
    }

    stats->robustLQ = binLo25F32;
    stats->robustUQ = binHi25F32;
    psTrace(TRACE, 6, "The 25 and 75 percent data point exact positions are (%f, %f).\n",
            binLo25F32, binHi25F32);
    long N50 = 0;
    for (long i = 0 ; i < myVector->n ; i++) {
        if (!mask->data.PS_TYPE_VECTOR_MASK_DATA[i] &&
            (binLo25F32 <= myVector->data.F32[i]) && (binHi25F32 >= myVector->data.F32[i])) {
            N50++;
        }
    }
    stats->robustN50 = N50;
    psTrace(TRACE, 6, "The robustN50 is %ld.\n", N50);
    psTrace(TRACE, 6, "The robust median and stdev are %f, %f\n", stats->robustMedian, stats->robustStdev);

    // Clean up
    psFree(histogram);
    psFree(cumulative);
    psFree(statsMinMax);
    psFree(mask);

    stats->results |= PS_STAT_ROBUST_MEDIAN;
    stats->results |= PS_STAT_ROBUST_STDEV;
    stats->results |= PS_STAT_ROBUST_QUARTILE;

    return true;

escape:
    stats->robustMedian = NAN;
    stats->robustStdev = NAN;
    stats->robustUQ = NAN;
    stats->robustLQ = NAN;
    stats->robustN50 = 0;
    stats->results |= PS_STAT_ROBUST_MEDIAN;
    stats->results |= PS_STAT_ROBUST_STDEV;
    stats->results |= PS_STAT_ROBUST_QUARTILE;

    psFree(histogram);
    psFree(cumulative);
    psFree(statsMinMax);
    psFree(mask);

    return true;
}

/********************
 * perform an asymmetric fit to the population.  In development, this was called
 * "vectorFittedStats_v4" all versions of fitted stats now resolve to this function (only v4
 * has really been used) vectorFittedStats requires guess for fittedMean and fittedStdev
 * robustN50 should also be set gaussian fit is performed using 1D polynomial to ln(y) this
 * version follows the upper portion of the distribution until it passes 0.5*peak
 ********************/
static bool vectorFittedStats (const psVector* myVector,
                                  const psVector* errors,
                                  psVector* mask,
                                  psVectorMaskType maskVal,
                                  psStats* stats)
{

    // This procedure requires the mean.  If it has not been already
    // calculated, then call vectorSampleMean()
    if (!(stats->results & PS_STAT_ROBUST_MEDIAN)) {
        if (!vectorRobustStats(myVector, errors, mask, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, "failure to measure robust stats\n");
            return false;
        }
    }

    // If the mean is NAN, then generate a warning and set the stdev to NAN.
    if (isnan(stats->robustMedian)) {
	stats->fittedMean = NAN;
	stats->fittedStdev = NAN;
	stats->results |= PS_STAT_FITTED_MEAN;
	stats->results |= PS_STAT_FITTED_STDEV;
        return true;
    }

    if (stats->robustStdev <= FLT_EPSILON) {
	stats->fittedMean = stats->robustMedian;
	stats->fittedStdev = stats->robustStdev;
	stats->results |= PS_STAT_FITTED_MEAN;
	stats->results |= PS_STAT_FITTED_STDEV;
        return true;
    }
    if (myVector->n < 1) { printf("There are no elements in this vector.\n"); abort(); }
    float guessStdev = stats->robustStdev;  // pass the guess sigma
    float guessMean = stats->robustMedian;  // pass the guess mean

    psTrace(TRACE, 6, "The ** starting ** guess mean  is %f.\n", guessMean);
    psTrace(TRACE, 6, "The ** starting ** guess stdev is %f.\n", guessStdev);

    bool done = false;
    for (int iteration = 0; !done && (iteration < 2); iteration ++) {
        psStats *statsMinMax = psStatsAlloc(PS_STAT_MIN | PS_STAT_MAX); // Statistics for min and max

        psF32 binSize = 1;
        if (stats->options & PS_STAT_USE_BINSIZE) {
            // Set initial bin size to the specified value.
            binSize = stats->binsize;
            psTrace(TRACE, 6, "Setting initial robust bin size to %.2f\n", binSize);
        } else {
            // construct a histogram with (sigma/2 < binsize < sigma)
            // set roughly so that the lowest bins have about 2 cnts
            // Nsmallest ~ N50 / (4*dN))
	  //            psF32 dN = PS_MAX (1, PS_MIN (4, stats->robustN50 / 8));

	  // CZW 2013-11-20: We know that the histogram is going to be basically Gaussian.
	  // Furthermore, we only use the inner +/- 2 sigma parts.  Therefore, define the
	  // binsize such that the bin at 2 sigma contains ~50 points (S/N ~ 7).  robustN50
	  // contains half the total points, so 2 * robustN50 / 50 is the fraction of all
	  // points in the 2 sigma bin.  Dance the erf() relations around, and it looks like
	  // there's a factor of about 1/20 to include.  Keep the PS_MAX to ensure we never bin
	  // wider than 1 sigma when the number of points is small.
	  psF32 dN = PS_MAX(1, (stats->robustN50 / 500.0));
	  binSize = guessStdev / dN;
        }

        // Determine the min/max of the vector (which prior outliers masked out)
        int numValid = vectorMinMax(myVector, mask, maskVal, statsMinMax); // Number of values
        float min = statsMinMax->min;
        float max = statsMinMax->max;
        if (numValid == 0 || isnan(min) || isnan(max)) {
            COUNT_WARNING(10, 100, "Failed to calculate the min/max of the input vector.\n");
            psFree(statsMinMax);
            goto escape;
        }

        // If all data points have the same value, then we set the appropriate members of stats and return.
        if (fabs(max - min) <= FLT_EPSILON) {
            COUNT_WARNING(10, 100, "All data points have the same value: %f.\n", min);
            stats->fittedMean = min;
            stats->fittedStdev = 0.0;
            stats->results |= PS_STAT_FITTED_MEAN;
            stats->results |= PS_STAT_FITTED_STDEV;
            return true;
        }

        // Calculate the number of bins.
        // XXX can we calculate the binMin, binMax **before** building this histogram?
        // if the range is too absurd, adjust numBins & binSize
	// We no longer want to reset the binSize here, as it can cause odd things.  Better to select
	// a number of bins, and then set the min/max values to put those bins sanely around the mean.
	//        long numBins = PS_MAX (50, PS_MIN (10000, (max - min) / binSize));
	//        binSize = (max - min) / (float) numBins;
        psTrace(TRACE, 6, "The new min/max values are (%f, %f).\n", min, max);
        psTrace(TRACE, 6, "The new bin size is %f.\n", binSize);
	//        psTrace(TRACE, 6, "The numBins is %ld\n", numBins);


#define FITTED_CLIPPING_NUM 5.0
	if (min < guessMean - FITTED_CLIPPING_NUM * guessStdev) {
	  min = guessMean - FITTED_CLIPPING_NUM * guessStdev;
	}
	if (max > guessMean + FITTED_CLIPPING_NUM * guessStdev) {
	  max = guessMean + FITTED_CLIPPING_NUM * guessStdev;
	}
        long numBins = PS_MAX (50, PS_MIN (10000, (max - min) / binSize));
	if (CZW) { printf("I've clipped: %f %f => %f %f ; %f %ld\n",guessMean,guessStdev,min,max,binSize,stats->robustN50); }
        psHistogram *histogram = psHistogramAlloc(min, max, numBins); // A new histogram (without outliers)
        if (!psVectorHistogram(histogram, myVector, errors, mask, maskVal)) {
            COUNT_WARNING(10, 100, "Unable to generate histogram for fitted statistics v4.\n");
            psFree(histogram);
            psFree(statsMinMax);
            goto escape;
        }
        if (psTraceGetLevel("psLib.math") >= 8) {
            PS_VECTOR_PRINT_F32(histogram->nums);
        }

        // now fit a Gaussian to the upper and lower halves about the peak independently

        // set the full-range upper and lower limits
        psF32 maxFitSigma = 2.0;
        if (isfinite(stats->clipSigma)) {
            maxFitSigma = fabs(stats->clipSigma);
        }
        if (isfinite(stats->max)) {
            maxFitSigma = fabs(stats->max);
        }

        psF32 minFitSigma = 2.0;
        if (isfinite(stats->clipSigma)) {
            minFitSigma = fabs(stats->clipSigma);
        }
        if (isfinite(stats->min)) {
            minFitSigma = fabs(stats->min);
        }

        // select the min and max bins, saturating on the lower and upper end-points
        long binMin, binMax;
        PS_BIN_FOR_VALUE (binMin, histogram->bounds, guessMean - minFitSigma*guessStdev, 0);
        PS_BIN_FOR_VALUE (binMax, histogram->bounds, guessMean + maxFitSigma*guessStdev, 0);

        if (binMin == binMax) {
            COUNT_WARNING(10, 100, "Failed to calculate the min/max of the input vector.\n");
            psFree(statsMinMax);
            psFree(histogram);
            goto escape;
        }

        // search for mode (peak of histogram within range mean-2sigma - mean+2sigma
        long  binPeak = binMin;
        float valPeak = histogram->nums->data.F32[binPeak];
        for (int i = binMin; i < binMax; i++) {
            if (histogram->nums->data.F32[i] > valPeak) {
                binPeak = i;
                valPeak = histogram->nums->data.F32[binPeak];
            }
            psTrace (TRACE, 6, "(%f = %.0f) ", histogram->bounds->data.F32[i], histogram->nums->data.F32[i]);
	    if (CZW) { printf("CENTERED_HISTOGRAM: %f %f\n",
			      PS_BIN_MIDPOINT(histogram,i),
			      histogram->nums->data.F32[i]); }
        }
        psTrace (TRACE, 6, "\n");

	if (CZW) { printf("Bin selection done: %ld %f %f %ld %f %f %ld %f %f\n",
			  binMin,PS_BIN_MIDPOINT(histogram,binMin),histogram->nums->data.F32[binMin],
			  binMax,PS_BIN_MIDPOINT(histogram,binMax),histogram->nums->data.F32[binMax],
			  binPeak,PS_BIN_MIDPOINT(histogram,binPeak),histogram->nums->data.F32[binPeak]);
	}
	
        // assume a reasonably well-defined gaussian-like population; run from peak out until val < 0.25*peak
        psTrace(TRACE, 6, "The clipped numBins is %ld\n", binMax - binMin);
        psTrace(TRACE, 6, "The clipped min is %f (%ld)\n", PS_BIN_MIDPOINT(histogram, binMin), binMin);
        psTrace(TRACE, 6, "The clipped max is %f (%ld)\n", PS_BIN_MIDPOINT(histogram, binMax - 1), binMax - 1);
        psTrace(TRACE, 6, "The clipped peak is %f (%ld)\n", PS_BIN_MIDPOINT(histogram, binPeak), binPeak);
        psTrace(TRACE, 6, "The clipped peak value is %f\n", histogram->nums->data.F32[binPeak]);

	
	float lowfitMean = NAN;
	float lowfitStdev = NAN;
        {
            // fit the lower half of the distribution
            // run down until we drop below 0.25*valPeak
            // run up until we drop below 0.50*valPeak
            long binS = binMin;
            long binE = PS_MIN (binPeak + 3, binMax);
            for (int i = binPeak - 3; i >= binMin; i--) {
                if (histogram->nums->data.F32[i] < 0.25*valPeak) {
                    binS = i;
                    break;
                }
            }
            for (int i = binPeak + 3; i < binMax; i++) {
                if (histogram->nums->data.F32[i] < 0.50*valPeak) {
                    binE = i;
                    break;
                }
            }
            psTrace(TRACE, 6, "Lower bound for lower half: %f (%ld)\n",
                    PS_BIN_MIDPOINT(histogram, binS), binS);
            psTrace(TRACE, 6, "Upper bound for lower half: %f (%ld)\n",
                    PS_BIN_MIDPOINT(histogram, binE), binE);

            psVector *y = psVectorAllocEmpty(binE - binS, PS_TYPE_F32); // Vector of coordinates
            psVector *x = psVectorAllocEmpty(binE - binS, PS_TYPE_F32); // Vector of ordinates
            long j = 0;
            for (long i = binS; i < binE; i++) {
                if (histogram->nums->data.F32[i] <= 0.0)
                    continue;
                x->data.F32[j] = PS_BIN_MIDPOINT(histogram, i);
                // note this is the natural log: expected distribution is A exp(-(x-xo)^2/2sigma^2)
                y->data.F32[j] = log(histogram->nums->data.F32[i]);
                j++;
            }
            y->n = x->n = j;
	    
            // fit 2nd order polynomial to ln(y) = -(x-xo)^2/2sigma^2
            // XXX this fit may fail with an error for an ill-conditioned matrix (bad data)
            // we probably should be able to handle the data errors gracefully
            psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
            bool status = psVectorFitPolynomial1D (poly, NULL, 0, y, NULL, x);
#if (CZW && 1)
	    printf("CZW: LowfitPoly: %f %f %f\n",poly->coeff[0],poly->coeff[1],poly->coeff[2]);
	    for (long i = 0; i < x->n; i++) {
	      printf("CZW: Lowfit: %d %ld %f %f %f\n",
		     status,i,x->data.F32[i],y->data.F32[i],
		     poly->coeff[0] + poly->coeff[1] * x->data.F32[i] +
		     poly->coeff[2] * pow(x->data.F32[i],2));
	    }
#endif
            psFree(x);
            psFree(y);

            if (!status) {
                psErrorClear();
                COUNT_WARNING(10, 100, "Failed to fit a gaussian to the robust histogram.\n");
                psFree(poly);
                psFree(histogram);
                psFree(statsMinMax);
                goto escape;
            }

            if (poly->coeff[2] >= 0.0) {
                COUNT_WARNING(10, 100, "Failed parabolic fit: %f + %f x + %f x^2\n", poly->coeff[0], poly->coeff[1], poly->coeff[2]);

                psFree(poly);
                psFree(histogram);
                psFree(statsMinMax);

                // sometimes, the guessStdev is much too large.  in this case, the entire real population
                // tends to be found in a single bin.  make one attempt to recover by dropping the guessStdev
                // down by a jump and trying again
                if (iteration == 0) {
                    guessStdev = 0.25*guessStdev;
                    psTrace(TRACE, 6, "*** retry, new stdev is %f.\n", guessStdev);
                    continue;
                }

                COUNT_WARNING(10, 100, "fit did not converge\n");
                goto escape;
            }

            // calculate lower mean & stdev from parabolic fit -- use this as the result
            lowfitStdev = sqrt(-0.5/poly->coeff[2]);
            lowfitMean  = poly->coeff[1]*PS_SQR(lowfitStdev);

            psTrace(TRACE, 6, "Parabolic Lower fit results: %f + %f x + %f x^2\n", poly->coeff[0], poly->coeff[1], poly->coeff[2]);
            psTrace(TRACE, 6, "The lower mean  is %f.\n", lowfitMean);
            psTrace(TRACE, 6, "The lower stdev is %f.\n", lowfitStdev);

            psFree(poly);
        }

	float fullfitMean  = NAN;
	float fullfitStdev = NAN;
	float minValueSym  = NAN;
	float maxValueSym  = NAN;

	// try the full fit as well:
	{
            // fit a symmetric distribution
            // run up until we drop below 0.15*valPeak
            // run up until we drop below 0.15*valPeak
            long binS = binMin;
            long binE = binMax;
            for (int i = binPeak - 3; i >= binMin; i--) {
                if (histogram->nums->data.F32[i] < 0.15*valPeak) {
                    binS = i;
                    break;
                }
            }
            for (int i = binPeak + 3; i < binMax; i++) {
                if (histogram->nums->data.F32[i] < 0.15*valPeak) {
                    binE = i;
                    break;
                }
            }

            psTrace(TRACE, 6, "Lower bound for symmetric range: %f (%ld)\n",
                    PS_BIN_MIDPOINT(histogram, binS), binS);
            psTrace(TRACE, 6, "Upper bound for symmetric range: %f (%ld)\n",
                    PS_BIN_MIDPOINT(histogram, binE), binE);

            psVector *y = psVectorAllocEmpty(binE - binS, PS_TYPE_F32); // Vector of coordinates
            psVector *x = psVectorAllocEmpty(binE - binS, PS_TYPE_F32); // Vector of ordinates
            long j = 0;
            for (long i = binS; i < binE; i++) {
                if (histogram->nums->data.F32[i] <= 0.0)
                    continue;
                x->data.F32[j] = PS_BIN_MIDPOINT(histogram, i);
                // note this is the natural log: expected distribution is A exp(-(x-xo)^2/2sigma^2)
                y->data.F32[j] = log(histogram->nums->data.F32[i]);
                j++;
            }
            y->n = x->n = j;

            // fit 2nd order polynomial to ln(y) = -(x-xo)^2/2sigma^2
            psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
            bool status = psVectorFitPolynomial1D (poly, NULL, 0, y, NULL, x);
#if (CZW && 1)
	    printf("CZW: FullfitPoly: %f %f %f\n",poly->coeff[0],poly->coeff[1],poly->coeff[2]);
	    for (long i = 0; i < x->n; i++) {
	      printf("CZW: Fullfit: %d %ld %f %f %f\n",
		     status,i,x->data.F32[i],y->data.F32[i],
		     poly->coeff[0] + poly->coeff[1] * x->data.F32[i] +
		     poly->coeff[2] * pow(x->data.F32[i],2));
	    }
#endif
            psFree(x);
            psFree(y);

            if (!status) {
                psErrorClear();
                COUNT_WARNING(10, 100, "Failed to fit a gaussian to the robust histogram.\n");
                psFree(poly);
                psFree(histogram);
                psFree(statsMinMax);
                goto escape;
            }

            // calculate upper mean & stdev from parabolic fit -- ignore this value
            fullfitStdev = sqrt(-0.5/poly->coeff[2]);
            fullfitMean = poly->coeff[1]*PS_SQR(fullfitStdev);

#ifndef PS_NO_TRACE
            psTrace(TRACE, 6, "Parabolic Symmetric fit results: %f + %f x + %f x^2\n", poly->coeff[0], poly->coeff[1], poly->coeff[2]);
            psTrace(TRACE, 6, "The symmetric mean  is %f.\n", fullfitMean);
            psTrace(TRACE, 6, "The symmetric stdev is %f.\n", fullfitStdev);
#endif

            // if we converge on a solution outside the range binMin - binMax, use a more conservative range
            minValueSym = PS_BIN_MIDPOINT(histogram, binS);
            maxValueSym = PS_BIN_MIDPOINT(histogram, binE - 1);

            // saturate on min or max value
            if (fullfitMean < minValueSym) {
                fullfitMean = minValueSym;
                psTrace(TRACE, 6, "The symmetric mean is out of bounds, saturating to %f.\n", guessMean);
            }

            // saturate on min or max value
            if (fullfitMean > maxValueSym) {
                fullfitMean = maxValueSym;
                psTrace(TRACE, 6, "The symmetric mean is out of bounds, saturating to %f.\n", guessMean);
            }

	    
            psFree (poly);
        }

	// we now have the fullfit and the lowfit mean and stdev values
	// accept the fullfit unless minValueSym < lowfitMean < fullfitMean

	if (isfinite(lowfitMean) && isfinite(lowfitStdev) && (lowfitMean < fullfitMean) && (lowfitMean > minValueSym)) {
	    guessMean  = lowfitMean;
	    guessStdev = lowfitStdev;
	} else {
	    guessMean  = fullfitMean;
	    guessStdev = fullfitStdev;
	}

	if (!isfinite(guessMean) || !isfinite(guessStdev)) {
	    guessMean  = stats->robustMedian;
	    guessStdev = stats->robustStdev;
	}

	if (guessStdev > 0.75*stats->robustStdev) {
	    done = true;
	}

	
#if (CZW && 1)
	printf("CZW IN FITTED: iter   %d %f \n"
	       "               low    %f %f \n"
	       "               full   %f %f \n"
	       "               robust %f %f \n"
	       "               final  %f %f\n",
	       iteration,minValueSym,
	       lowfitMean,lowfitStdev,
	       fullfitMean,fullfitStdev,
	       stats->robustMedian,stats->robustStdev,
	       guessMean,guessStdev);
#endif

        // Clean up after fitting
        psFree (histogram);
        psFree (statsMinMax);
    }

    // The fitted mean is the Gaussian mean.
    stats->fittedMean = guessMean;
    psTrace(TRACE, 6, "The fitted mean is %f.\n", stats->fittedMean);

    // The fitted standard deviation
    stats->fittedStdev = guessStdev;
    psTrace(TRACE, 6, "The fitted stdev is %f.\n", stats->fittedStdev);

    stats->results |= PS_STAT_FITTED_MEAN;
    stats->results |= PS_STAT_FITTED_STDEV;

    return true;

escape:
    stats->fittedMean = NAN;
    stats->fittedStdev = NAN;
    stats->results |= PS_STAT_FITTED_MEAN;
    stats->results |= PS_STAT_FITTED_STDEV;

    return true;
}


/******************************************************************************
vectorSmoothHistGaussian(): This routine smoothes the data in the input
robustHistogram with a Gaussian of width sigma.  It returns a psVector of the
smoothed data.

XXX this function is unused
*****************************************************************************/
psVector *vectorSmoothHistGaussian(psHistogram *histogram,
                                   psF32 sigma)
{
    psTrace(TRACE, 4, "---- %s() begin ----\n", __func__);
    psTrace(TRACE, 5, "(histogram->nums->n, sigma) is (%d, %.2f\n", (int) histogram->nums->n, sigma);
    PS_ASSERT_PTR_NON_NULL(histogram, NULL);
    PS_ASSERT_PTR_NON_NULL(histogram->bounds, NULL);
    PS_ASSERT_PTR_NON_NULL(histogram->nums, NULL);
    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(histogram->nums);
    }

    long numBins = histogram->nums->n;  // Number of histogram bins
    psVector *smooth = psVectorAlloc(numBins, PS_TYPE_F32); // Smoothed version of histogram bins
    const psVector *bounds = histogram->bounds; // The bounds for the histogram bins

    if (!histogram->uniform) {
        //
        // We get here if the histogram is non-uniform.  This code is not tested.
        // However, it is also not used anywhere, yet.
        //
        psWarning("WARNING: vectorSmoothHistGaussian() on non-uniform "
                  "histograms has not been tested or used.\n");

        for (long i = 0; i < numBins; i++) {
            // Determine the midpoint of bin i.
            float iMid = PS_BIN_MIDPOINT(histogram, i);

            //
            // We determine the bin numbers (jMin:jMax) corresponding to a
            // range of data values surrounding iMid.  The range is of size:
            // 2*PS_GAUSS_WIDTH*sigma
            long jMin, jMax;
            psF32 x = iMid - (PS_GAUSS_WIDTH * sigma);
            for (jMin = i; jMin > 0 && bounds->data.F32[jMin] > x; jMin--)
                ;
            x = iMid + (PS_GAUSS_WIDTH * sigma);
            for (jMax = i; jMax < bounds->n - 1 && bounds->data.F32[jMax + 1] > x; jMax++)
                ;

            //
            // Loop from jMin to jMax, computing the gaussian of data i.
            //
            smooth->data.F32[i] = 0.0;
            for (long j = jMin ; j <= jMax ; j++) {
                float jMid = PS_BIN_MIDPOINT(histogram, j);
                smooth->data.F32[i] +=
                    histogram->nums->data.F32[j] * psGaussian(jMid, iMid, sigma, true);
            }
        }
    } else {
        //
        // We get here if the histogram is uniform.
        //
        for (long i = 0; i < numBins; i++) {
            psF32 binSize = bounds->data.F32[1] - bounds->data.F32[0];
            psS32 gaussWidth = ((PS_GAUSS_WIDTH * sigma) / binSize);

            //
            // XXX: The following is wrong.  However, in practice, the sigma was too
            // large compared to the binSize.  This meant that the smoothing was done
            // over 500 bins in the robust stats algorithm.  This mean that the smoothing
            // took way too long.
            //
#if 0

            gaussWidth = 10;
#endif
            //
            // We determine the bin numbers (jMin:jMax) corresponding to a
            // range of data values surrounding iMid.  The range is of size:
            // 2*PS_GAUSS_WIDTH*sigma
            //
            psS32 jMin = PS_MAX(i - gaussWidth, 0);
            psS32 jMax = PS_MIN(i + gaussWidth, bounds->n - 1);

            //
            // Loop from jMin to jMax, computing the gaussian of data i.
            //
            smooth->data.F32[i] = 0.0;
            float iMid = PS_BIN_MIDPOINT(histogram, i);
            for (long j = jMin ; j <= jMax ; j++) {
                float jMid = PS_BIN_MIDPOINT(histogram, j);
                smooth->data.F32[i] +=
                    histogram->nums->data.F32[j] * psGaussian(jMid, iMid, sigma, true);
            }
        }
    }

    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(smooth);
    }
    psTrace(TRACE, 4, "---- %s() end ----\n", __func__);
    return(smooth);
}

/*****************************************************************************/

/* FUNCTION IMPLEMENTATION - PUBLIC                                          */

/*****************************************************************************/

// free function
static void statsFree(psStats *stats)
{
    if (!stats) return;
    psFree (stats->tmpData);
    psFree (stats->tmpMask);
    return;
}

/******************************************************************************
    psStatsAlloc(): This routine must create a new psStats data structure.
*****************************************************************************/
psStats* p_psStatsAlloc(const char *file, unsigned int lineno, const char *func, psStatsOptions options)
{
    psStats *stats = p_psAlloc(file, lineno, func, sizeof(psStats));
    psMemSetDeallocator(stats, (psFreeFunc)statsFree);

    // set initial, default values
    psStatsInit (stats);

    // these values are can be set as desired by the user.  they are not affected by
    // psStatsInit
    stats->clipSigma = 3.0;
    stats->clipIter = 3;
    stats->nSubsample = 100000;
    stats->options = options;
    stats->tmpData = NULL;
    stats->tmpMask = NULL;

    return stats;
}

// reset the values which are output, and which may be used from one psStats stage to the next
void psStatsInit(psStats *stats)
{
    stats->sampleMean = NAN;
    stats->sampleMedian = NAN;
    stats->sampleStdev = NAN;
    stats->sampleUQ = NAN;
    stats->sampleLQ = NAN;
    stats->sampleSkewness = NAN;
    stats->sampleKurtosis = NAN;
    stats->robustMedian = NAN;
    stats->robustStdev = NAN;
    stats->robustUQ = NAN;
    stats->robustLQ = NAN;
    stats->robustN50 = -1;
    stats->fittedMean = NAN;
    stats->fittedStdev = NAN;
    stats->fittedNfit = -1;
    stats->clippedMean = NAN;
    stats->clippedStdev = NAN;
    stats->clippedNvalues = -1;     // XXX: This is never used
    stats->min = NAN;
    stats->max = NAN;
    stats->binsize = NAN;
    return;
}


bool psMemCheckStats(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)statsFree );
}

/******************************************************************************
psVectorStats(in, mask, maskVal, stats): this is the public API
function which calls the above private stats functions based on what bits
were set in stats->options.

Inputs
    in
    mask
    maskVal
    stats
Returns
    The stats structure.

XXX: Should we free stats if the asserts fail? NO; we don't own it (RHL).
*****************************************************************************/
bool psVectorStats(psStats* stats,
                   const psVector* in,
                   const psVector* errors,
                   const psVector* mask,
                   psVectorMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(stats, false);
    PS_ASSERT_VECTOR_NON_NULL(in, false);
    PS_ASSERT_VECTOR_NON_EMPTY(in, false);
    if (mask) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(mask, in, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (errors) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(errors, in, false);
        PS_ASSERT_VECTOR_TYPE(errors, in->type.type, false);
    }

    // Convert types, as necessary
    psVector *inF32 = NULL;             // Input vector of values, F32 version
    if (in->type.type == PS_TYPE_F32) {
        inF32 = psMemIncrRefCounter((psPtr)in);
    } else {
        inF32 = psVectorCopy(NULL, in, PS_TYPE_F32);
    }
    psVector *errorsF32 = NULL;         // Input vector of errors, F32 version
    if (errors) {
        if (errors->type.type == PS_TYPE_F32) {
            errorsF32 = psMemIncrRefCounter((psPtr)errors);
        } else {
            errorsF32 = psVectorCopy(NULL, errors, PS_TYPE_F32);
        }
    }
    psVector *maskVector = NULL;            // Input mask vector, U8 version
    if (mask) {
        if (mask->type.type == PS_TYPE_VECTOR_MASK) {
            maskVector = psMemIncrRefCounter((psPtr)mask);
        } else {
            maskVector = psVectorCopy(NULL, mask, PS_TYPE_VECTOR_MASK);
        }
    }

    if ((stats->options & PS_STAT_USE_RANGE) && (stats->min >= stats->max)) {
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(stats->max, stats->min, false);
    }

    if ((stats->options & PS_STAT_USE_BINSIZE) && (stats->min >= stats->max)) {
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(stats->binsize, 0.0, false);
    }

    // init the value of stats->results: this is used internally to check if
    // prior functions have been called
    stats->results = PS_STAT_NONE;
    bool status = true;

    // ************************************************************************
    if (stats->options & PS_STAT_SAMPLE_MEAN) {
	// NOTE: vectorSampleMean cannot return 'false'
        if (!vectorSampleMean(inF32, errorsF32, maskVector, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, "Failed to calculate vector sample mean");
            status &= false;
        }
    }

    // ************************************************************************
    if (stats->options & (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_QUARTILE)) {
	// NOTE: vectorSampleMedian only returns 'false' for very bad cases:
	// NULL input vector, psVectorCopy failure, invalid vector type (likely programming errors)
	if (!vectorSampleMedian(inF32, maskVector, maskVal, stats)) {
	    psError(PS_ERR_UNKNOWN, false, "Failed to calculate sample median");
	    status &= false;
	}
    }

    // ************************************************************************
    if (stats->options & PS_STAT_SAMPLE_STDEV) {
	// NOTE: vectorSampleStdev cannot return 'false'
        if (!vectorSampleStdev(inF32, errorsF32, maskVector, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, "Failed to calculate sample stdev");
            status &= false;
        }
    }

    if (stats->options & (PS_STAT_SAMPLE_SKEWNESS | PS_STAT_SAMPLE_KURTOSIS)) {
	// NOTE: vectorSampleMoments cannot return 'false'
        if (!vectorSampleMoments(inF32, maskVector, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, "Failed to calculate sample moments");
            status &= false;
        }
    }

    // ************************************************************************
    if (stats->options & (PS_STAT_MAX | PS_STAT_MIN)) {
	// NOTE: vectorMinMax returns 0 if there are no valid values, 
	// but this is not an error condition.  stats.min,max are set to NAN.
	// vectorMinMax cannot raise an error
        vectorMinMax(inF32, maskVector, maskVal, stats);
    }

    // ************************************************************************
    if (stats->options & (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_ROBUST_QUARTILE)) {
        if (!vectorRobustStats(inF32, errorsF32, maskVector, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, _("Failed to calculate robust statistics"));
            status &= false;
        }
    }

    // ************************************************************************
    if (stats->options & (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV)) {
        if (!vectorFittedStats(inF32, errorsF32, maskVector, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, _("Failed to calculate fitted statistics"));
            status &= false;
        }
    }

    // ************************************************************************
    if ((stats->options & PS_STAT_CLIPPED_MEAN) || (stats->options & PS_STAT_CLIPPED_STDEV)) {
        if (!vectorClippedStats(inF32, errorsF32, maskVector, maskVal, stats)) {
            psError(PS_ERR_UNKNOWN, false, "Failed to calculate clipped statistics\n");
            status &= false;
        }
    }

    psFree(inF32);
    psFree(errorsF32);
    psFree(maskVector);
    return status;
}

psStatsOptions psStatsOptionFromString(const char *string)
{
    PS_ASSERT_STRING_NON_EMPTY(string, PS_STAT_NONE);

#define READ_STAT(NAME, SYMBOL) \
    if (strcasecmp(string, NAME) == 0) { \
        return SYMBOL; \
    }

    READ_STAT("MEAN",       PS_STAT_SAMPLE_MEAN);
    READ_STAT("STDEV",      PS_STAT_SAMPLE_STDEV);
    READ_STAT("SKEWNESS",   PS_STAT_SAMPLE_SKEWNESS);
    READ_STAT("KURTOSIS",   PS_STAT_SAMPLE_KURTOSIS);
    READ_STAT("MEDIAN",     PS_STAT_SAMPLE_MEDIAN);
    READ_STAT("QUARTILE",   PS_STAT_SAMPLE_QUARTILE);
    READ_STAT("SAMPLE_MEAN",     PS_STAT_SAMPLE_MEAN);
    READ_STAT("SAMPLE_STDEV",    PS_STAT_SAMPLE_STDEV);
    READ_STAT("SAMPLE_MEDIAN",   PS_STAT_SAMPLE_MEDIAN);
    READ_STAT("SAMPLE_QUARTILE", PS_STAT_SAMPLE_QUARTILE);
    READ_STAT("SAMPLE_SKEWNESS", PS_STAT_SAMPLE_SKEWNESS);
    READ_STAT("SAMPLE_KURTOSIS", PS_STAT_SAMPLE_KURTOSIS);
    READ_STAT("ROBUST",          PS_STAT_ROBUST_MEDIAN);
    READ_STAT("ROBUST_MEDIAN",   PS_STAT_ROBUST_MEDIAN);
    READ_STAT("ROBUST_STDEV",    PS_STAT_ROBUST_STDEV);
    READ_STAT("ROBUST_QUARTILE", PS_STAT_ROBUST_QUARTILE);
    READ_STAT("FITTED",          PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_MEAN",     PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_STDEV",    PS_STAT_FITTED_STDEV);
    READ_STAT("FITTED_V2",       PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_MEAN_V2",  PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_STDEV_V2", PS_STAT_FITTED_STDEV);
    READ_STAT("FITTED_V3",       PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_MEAN_V3",  PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_STDEV_V3", PS_STAT_FITTED_STDEV);
    READ_STAT("FITTED_V4",       PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_MEAN_V4",  PS_STAT_FITTED_MEAN);
    READ_STAT("FITTED_STDEV_V4", PS_STAT_FITTED_STDEV);
    READ_STAT("CLIPPED",         PS_STAT_CLIPPED_MEAN);
    READ_STAT("CLIPPED_MEAN",    PS_STAT_CLIPPED_MEAN);
    READ_STAT("CLIPPED_STDEV",   PS_STAT_CLIPPED_STDEV);

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to parse statistic: %s\n", string);
    return PS_STAT_NONE;
}

psString psStatsOptionToString(psStatsOptions option)
{
    psString string = NULL;             // String to return

#define WRITE_STAT(NAME, SYMBOL) \
    if (option & SYMBOL) { \
        psStringAppend(&string, "%s ", NAME); \
    }

    // Same list as above (for psStatsFromString), but with repeat symbols removed
    WRITE_STAT("SAMPLE_MEAN",     PS_STAT_SAMPLE_MEAN);
    WRITE_STAT("SAMPLE_STDEV",    PS_STAT_SAMPLE_STDEV);
    WRITE_STAT("SAMPLE_MEDIAN",   PS_STAT_SAMPLE_MEDIAN);
    WRITE_STAT("SAMPLE_QUARTILE", PS_STAT_SAMPLE_QUARTILE);
    WRITE_STAT("SAMPLE_SKEWNESS", PS_STAT_SAMPLE_SKEWNESS);
    WRITE_STAT("SAMPLE_KURTOSIS", PS_STAT_SAMPLE_KURTOSIS);
    WRITE_STAT("ROBUST_MEDIAN",   PS_STAT_ROBUST_MEDIAN);
    WRITE_STAT("ROBUST_STDEV",    PS_STAT_ROBUST_STDEV);
    WRITE_STAT("ROBUST_QUARTILE", PS_STAT_ROBUST_QUARTILE);
    WRITE_STAT("FITTED_MEAN",     PS_STAT_FITTED_MEAN);
    WRITE_STAT("FITTED_STDEV",    PS_STAT_FITTED_STDEV);
    WRITE_STAT("CLIPPED_MEAN",    PS_STAT_CLIPPED_MEAN);
    WRITE_STAT("CLIPPED_STDEV",   PS_STAT_CLIPPED_STDEV);

    return string;
}

psStats *psStatsFromString(const char *string)
{
    psList *subStrings = psStringSplit(string, " ,;", false); // List of sub-strings
    if (!subStrings || psListLength(subStrings) == 0) {
        // Nothing here
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "No string to parse for statistics: %s\n", string);
        psFree(subStrings);
        return NULL;
    }
    psStats *stats = psStatsAlloc(0);   // Generate empty stats structure
    psListIterator *iterator = psListIteratorAlloc(subStrings, PS_LIST_HEAD, false); // Iterator
    psString statString;                // Statistic string, from iteration
    while ((statString = psListGetAndIncrement(iterator))) {
        psStatsOptions option = psStatsOptionFromString(statString);
        if (option == 0) {
            psWarning("Unable to interpret statistic option: %s --- ignored.\n", statString);
            continue;
        }
        stats->options |= option;
    }
    psFree(iterator);
    psFree(subStrings);
    return stats;
}

psString psStatsToString(const psStats *stats)
{
    return psStatsOptionToString(stats->options);
}

psStatsOptions psStatsSingleOption(psStatsOptions option)
{
    switch (option & ~(PS_STAT_USE_RANGE | PS_STAT_USE_BINSIZE)) {
      case PS_STAT_SAMPLE_MEAN:
      case PS_STAT_SAMPLE_MEDIAN:
      case PS_STAT_SAMPLE_STDEV:
      case PS_STAT_SAMPLE_QUARTILE:
      case PS_STAT_SAMPLE_SKEWNESS:
      case PS_STAT_SAMPLE_KURTOSIS:
      case PS_STAT_ROBUST_MEDIAN:
      case PS_STAT_ROBUST_STDEV:
      case PS_STAT_ROBUST_QUARTILE:
      case PS_STAT_FITTED_MEAN:
      case PS_STAT_FITTED_STDEV:
      case PS_STAT_CLIPPED_MEAN:
      case PS_STAT_CLIPPED_STDEV:
      case PS_STAT_MAX:
      case PS_STAT_MIN:
        return option & ~(PS_STAT_USE_RANGE | PS_STAT_USE_BINSIZE);
      default:
        return 0;
    }
    psAbort("Should never get here.\n");
    return 0;
}

psStatsOptions psStatsMeanOption(psStatsOptions options)
{
    return options & (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN | PS_STAT_ROBUST_MEDIAN |
                      PS_STAT_CLIPPED_MEAN | PS_STAT_FITTED_MEAN);
}

psStatsOptions psStatsStdevOption(psStatsOptions options)
{
    return options & (PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_STDEV | PS_STAT_CLIPPED_STDEV |
                      PS_STAT_FITTED_STDEV);
}


double psStatsGetValue(const psStats *stats, psStatsOptions option)
{
    // We could call psStatsSingle to check, but it would be a waste since we effectively do it anyway
    switch (option & ~(PS_STAT_USE_RANGE | PS_STAT_USE_BINSIZE)) {
      case PS_STAT_SAMPLE_MEAN:
        return stats->sampleMean;
      case PS_STAT_SAMPLE_MEDIAN:
        return stats->sampleMedian;
      case PS_STAT_SAMPLE_STDEV:
        return stats->sampleStdev;
      case PS_STAT_SAMPLE_SKEWNESS:
        return stats->sampleSkewness;
      case PS_STAT_SAMPLE_KURTOSIS:
        return stats->sampleKurtosis;
      case PS_STAT_ROBUST_MEDIAN:
        return stats->robustMedian;
      case PS_STAT_ROBUST_STDEV:
        return stats->robustStdev;
      case PS_STAT_FITTED_MEAN:
        return stats->fittedMean;
      case PS_STAT_FITTED_STDEV:
        return stats->fittedStdev;
      case PS_STAT_CLIPPED_MEAN:
        return stats->clippedMean;
      case PS_STAT_CLIPPED_STDEV:
        return stats->clippedStdev;
      case PS_STAT_MAX:
        return stats->max;
      case PS_STAT_MIN:
        return stats->min;
      case PS_STAT_SAMPLE_QUARTILE:
      case PS_STAT_ROBUST_QUARTILE:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot return a single quartile value; "
                "get them yourself.\n");
        return NAN;
      default:
        return NAN;
    }
    psAbort("Should never get here.\n");
    return NAN;
}

// other private functions used above

# if (0)
static psF32 QuadraticInverse(psF32 a,
                              psF32 b,
                              psF32 c,
                              psF32 y,
                              psF32 xLo,
                              psF32 xHi
    )
{
    psF64 tmp = sqrt((y - c)/a + (b*b)/(4.0 * a * a));

    psF64 x1 = -b/(2.0*a) + tmp;
    psF64 x2 = -b/(2.0*a) - tmp;

    if (xLo <= x1 && x1 <= xHi) {
        return x1;
    }
    if (xLo <= x2 && x2 <= xHi) {
        return x2;
    }
    return 0.5 * (xLo + xHi);
}

static psF32 LinearInverse(psF32 a,
			   psF32 b,
			   psF32 y,
			   psF32 xLo,
			   psF32 xHi
    )
{
    psF64 x = (y - b) / a;

    if (xLo <= x && x <= xHi) {
        return x;
    }
    return 0.5 * (xLo + xHi);
}
# endif

# if (0)
/******************************************************************************
fitQuadraticSearchForYThenReturnX(*xVec, *yVec, binNum, yVal): A general
routine which fits a quadratic to three points and returns the x-value
corresponding to the input y-value.  This routine takes psVectors of x/y pairs
as input, and fits a quadratic to the 3 points surrounding element binNum in
the vectors (the midpoint between element i and i+1 is used for x[i]).  It
then determines for what value x does that quadratic f(x) = yVal (the input
parameter).

*****************************************************************************/
static psF32 fitQuadraticSearchForYThenReturnX(const psVector *xVec,
                                               psVector *yVec,
                                               psS32 binNum,
                                               psF32 yVal
    )
{
    psTrace(TRACE, 4, "---- %s() begin ----\n", __func__);
    psTrace(TRACE, 5, "binNum, yVal is (%d, %f)\n", binNum, yVal);
    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(xVec);
        PS_VECTOR_PRINT_F32(yVec);
    }

    PS_ASSERT_VECTOR_NON_NULL(xVec, NAN);
    PS_ASSERT_VECTOR_NON_NULL(yVec, NAN);
    PS_ASSERT_VECTOR_TYPE(xVec, PS_TYPE_F32, NAN);
    PS_ASSERT_VECTOR_TYPE(yVec, PS_TYPE_F32, NAN);
    //    PS_ASSERT_VECTORS_SIZE_EQUAL(xVec, yVec, NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(xVec->n - 1), NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(yVec->n - 1), NAN);

    psVector *x = psVectorAlloc(3, PS_TYPE_F64);
    psVector *y = psVectorAlloc(3, PS_TYPE_F64);
    psF32 tmpFloat = 0.0f;

    if ((binNum >= 1) && (binNum < (yVec->n - 2)) && (binNum < (xVec->n - 2))) {
        // The general case.  We have all three points.
        x->data.F64[0] = (psF64) (0.5 * (xVec->data.F32[binNum - 1] + xVec->data.F32[binNum]));
        x->data.F64[1] = (psF64) (0.5 * (xVec->data.F32[binNum] + xVec->data.F32[binNum+1]));
        x->data.F64[2] = (psF64) (0.5 * (xVec->data.F32[binNum+1] + xVec->data.F32[binNum+2]));
        y->data.F64[0] = yVec->data.F32[binNum - 1];
        y->data.F64[1] = yVec->data.F32[binNum];
        y->data.F64[2] = yVec->data.F32[binNum + 1];
        psTrace(TRACE, 6, "x vec (orig) is (%f %f %f %f)\n", xVec->data.F32[binNum - 1],
                xVec->data.F32[binNum], xVec->data.F32[binNum+1], xVec->data.F32[binNum+2]);
        psTrace(TRACE, 6, "x data is (%f %f %f)\n", x->data.F64[0], x->data.F64[1], x->data.F64[2]);
        psTrace(TRACE, 6, "y data is (%f %f %f)\n", y->data.F64[0], y->data.F64[1], y->data.F64[2]);

        //
        // Ensure that the y value lies within range of the y values.
        //
        if (! (((y->data.F64[0] <= yVal) && (yVal <= y->data.F64[2])) ||
               ((y->data.F64[2] <= yVal) && (yVal <= y->data.F64[0]))) ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified yVal, %g, is not within y-range, %g to %g."),
                    (psF64)yVal, y->data.F64[0], y->data.F64[2]);
        }

        //
        // Ensure that the y values are monotonic.
        //
        if (((y->data.F64[0] < y->data.F64[1]) && !(y->data.F64[1] <= y->data.F64[2])) ||
            ((y->data.F64[0] > y->data.F64[1]) && !(y->data.F64[1] >= y->data.F64[2]))) {
            psError(PS_ERR_UNKNOWN, true,
                    "This routine must be called with monotonically increasing or decreasing data points.\n");
            psFree(x);
            psFree(y);
            psTrace(TRACE, 5, "---- %s() end ----\n", __func__);
            return NAN;
        }

        // Determine the coefficients of the polynomial.
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        if (!psVectorFitPolynomial1D(myPoly, NULL, 0, y, NULL, x)) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    _("Failed to fit a 1-dimensional polynomial to the three specified data points.  "
                      "Returning NAN."));
            psFree(x);
            psFree(y);
            psTrace(TRACE, 5, "---- %s(NAN) end ----\n", __func__);
            return NAN;
        }
        psTrace(TRACE, 6, "myPoly->coeff[0] is %f\n", myPoly->coeff[0]);
        psTrace(TRACE, 6, "myPoly->coeff[1] is %f\n", myPoly->coeff[1]);
        psTrace(TRACE, 6, "myPoly->coeff[2] is %f\n", myPoly->coeff[2]);
        psTrace(TRACE, 6, "Fitted y vec is (%f %f %f)\n",
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[0]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[1]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[2]));

        psTrace(TRACE, 6, "We fit the polynomial, now find x such that f(x) equals %f\n", yVal);
        tmpFloat = QuadraticInverse(myPoly->coeff[2], myPoly->coeff[1], myPoly->coeff[0], yVal,
                                    x->data.F64[0], x->data.F64[2]);
        psFree(myPoly);

        if (isnan(tmpFloat)) {
            psError(PS_ERR_UNEXPECTED_NULL,
                    false, _("Failed to determine the median of the fitted polynomial.  Returning NAN."));
            psFree(x);
            psFree(y);
            psTrace(TRACE, 5, "---- %s(NAN) end ----\n", __func__);
            return(NAN);
        }
    } else {
        // These are special cases where the bin is at the beginning or end of the vector.
        if (binNum == 0) {
            // We have two points only at the beginning of the vectors x and y.
            tmpFloat = 0.5 * (xVec->data.F32[binNum] +
                              xVec->data.F32[binNum + 1]);
        } else if (binNum == (xVec->n - 1)) {
            // The special case where we have two points only at the end of
            // the vectors x and y.
            // XXX: Is this right?
            tmpFloat = xVec->data.F32[binNum];
        } else if (binNum == (xVec->n - 2)) {
            // XXX: Is this right?
            tmpFloat = 0.5 * (xVec->data.F32[binNum] + xVec->data.F32[binNum + 1]);
        }
    }

    psTrace(TRACE, 6, "FIT: return %f\n", tmpFloat);
    psFree(x);
    psFree(y);

    psTrace(TRACE, 5, "---- %s(%f) end ----\n", __func__, tmpFloat);
    return tmpFloat;
}

/******************************************************************************
fitQuadraticSearchForYThenReturnXusingValues(*xVec, *yVec, binNum, yVal): A general routine
which fits a quadratic to three points and returns the x-value corresponding to the input
y-value.  This routine takes psVectors of x/y pairs as input, and fits a quadratic to the 3
points surrounding element binNum in the vectors.  This version uses the values of x[i] for the
x coordinates (not the midpoints).  This is appropriate for a cumulative histogram.  It then
determines for what value x does that quadratic f(x) = yVal (the input parameter).

*****************************************************************************/
static psF32 fitQuadraticSearchForYThenReturnXusingValues(const psVector *xVec,
                                                          psVector *yVec,
                                                          psS32 binNum,
                                                          psF32 yVal
    )
{
    psTrace(TRACE, 4, "---- %s() begin ----\n", __func__);
    psTrace(TRACE, 5, "binNum, yVal is (%d, %f)\n", binNum, yVal);
    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(xVec);
        PS_VECTOR_PRINT_F32(yVec);
    }

    PS_ASSERT_VECTOR_NON_NULL(xVec, NAN);
    PS_ASSERT_VECTOR_NON_NULL(yVec, NAN);
    PS_ASSERT_VECTOR_TYPE(xVec, PS_TYPE_F32, NAN);
    PS_ASSERT_VECTOR_TYPE(yVec, PS_TYPE_F32, NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(xVec->n - 1), NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(yVec->n - 1), NAN);

    psVector *x = psVectorAlloc(3, PS_TYPE_F64);
    psVector *y = psVectorAlloc(3, PS_TYPE_F64);
    psF32 tmpFloat = 0.0f;

    if ((binNum >= 1) && (binNum < (yVec->n - 2)) && (binNum < (xVec->n - 2))) {
        // The general case.  We have all three points.
        x->data.F64[0] = xVec->data.F32[binNum - 1];
        x->data.F64[1] = xVec->data.F32[binNum];
        x->data.F64[2] = xVec->data.F32[binNum+1];
        y->data.F64[0] = yVec->data.F32[binNum - 1];
        y->data.F64[1] = yVec->data.F32[binNum];
        y->data.F64[2] = yVec->data.F32[binNum + 1];
        psTrace(TRACE, 6, "x vec (orig) is (%f %f %f %f)\n", xVec->data.F32[binNum - 1],
                xVec->data.F32[binNum], xVec->data.F32[binNum+1], xVec->data.F32[binNum+2]);
        psTrace(TRACE, 6, "x data is (%f %f %f)\n", x->data.F64[0], x->data.F64[1], x->data.F64[2]);
        psTrace(TRACE, 6, "y data is (%f %f %f)\n", y->data.F64[0], y->data.F64[1], y->data.F64[2]);

        //
        // Ensure that the y value lies within range of the y values.
        //
        if (! (((y->data.F64[0] <= yVal) && (yVal <= y->data.F64[2])) ||
               ((y->data.F64[2] <= yVal) && (yVal <= y->data.F64[0]))) ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified yVal, %g, is not within y-range, %g to %g."),
                    (psF64)yVal, y->data.F64[0], y->data.F64[2]);
        }

        //
        // Ensure that the y values are monotonic.
        //
        if (((y->data.F64[0] < y->data.F64[1]) && !(y->data.F64[1] <= y->data.F64[2])) ||
            ((y->data.F64[0] > y->data.F64[1]) && !(y->data.F64[1] >= y->data.F64[2]))) {
            psError(PS_ERR_UNKNOWN, true,
                    "This routine must be called with monotonically increasing or decreasing data points.\n");
            psFree(x);
            psFree(y);
            psTrace(TRACE, 5, "---- %s() end ----\n", __func__);
            return NAN;
        }

        // Determine the coefficients of the polynomial.
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        if (!psVectorFitPolynomial1D(myPoly, NULL, 0, y, NULL, x)) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    _("Failed to fit a 1-dimensional polynomial to the three specified data points.  "
                      "Returning NAN."));
            psFree(x);
            psFree(y);
            psTrace(TRACE, 5, "---- %s(NAN) end ----\n", __func__);
            return NAN;
        }
        psTrace(TRACE, 6, "myPoly->coeff[0] is %f\n", myPoly->coeff[0]);
        psTrace(TRACE, 6, "myPoly->coeff[1] is %f\n", myPoly->coeff[1]);
        psTrace(TRACE, 6, "myPoly->coeff[2] is %f\n", myPoly->coeff[2]);
        psTrace(TRACE, 6, "Fitted y vec is (%f %f %f)\n",
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[0]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[1]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[2]));

        psTrace(TRACE, 6, "We fit the polynomial, now find x such that f(x) equals %f\n", yVal);
        tmpFloat = QuadraticInverse(myPoly->coeff[2], myPoly->coeff[1], myPoly->coeff[0], yVal,
                                    x->data.F64[0], x->data.F64[2]);
        psFree(myPoly);

        if (isnan(tmpFloat)) {
            psError(PS_ERR_UNEXPECTED_NULL,
                    false, _("Failed to determine the median of the fitted polynomial.  Returning NAN."));
            psFree(x);
            psFree(y);
            psTrace(TRACE, 5, "---- %s(NAN) end ----\n", __func__);
            return(NAN);
        }
    } else {
        // These are special cases where the bin is at the beginning or end of the vector.
        if (binNum == 0) {
            // We have two points only at the beginning of the vectors x and y.
            // XXX this does not seem to be doing the linear interpolation / extrapolation
            tmpFloat = 0.5 * (xVec->data.F32[binNum] +
                              xVec->data.F32[binNum + 1]);
        } else if (binNum == (xVec->n - 1)) {
            // The special case where we have two points only at the end of
            // the vectors x and y.
            // XXX: Is this right?
            tmpFloat = xVec->data.F32[binNum];
        } else if (binNum == (xVec->n - 2)) {
            // XXX: Is this right?
            tmpFloat = 0.5 * (xVec->data.F32[binNum] + xVec->data.F32[binNum + 1]);
        }
    }

    psTrace(TRACE, 6, "FIT: return %f\n", tmpFloat);
    psFree(x);
    psFree(y);

    psTrace(TRACE, 5, "---- %s(%f) end ----\n", __func__, tmpFloat);
    return tmpFloat;
}

/******************************************************************************
fitQuadraticSearchForYThenReturnXusingValues(*xVec, *yVec, binNum, yVal): A general routine
which fits a quadratic to three points and returns the x bin value corresponding to the input
y-value.  This routine takes psVectors of x/y pairs as input, and fits a quadratic to the 3
points surrounding element binNum in the vectors.  This version uses the values of x[i] for the
x coordinates (not the midpoints).  This is appropriate for a cumulative histogram.  It then
determines for what value x does that quadratic f(x) = yVal (the input parameter).

XXX this function is used a fair amount in an inner loop: the polynomial fitting and evaluation
could easily be done with statically allocated doubles, skipping the psLib versions of
polynomial fitting, etc.

*****************************************************************************/
static psF32 fitQuadraticSearchForYThenReturnBin(const psVector *xVec,
                                                 psVector *yVec,
                                                 psS32 binNum,
                                                 psF32 yVal
    )
{
    psTrace(TRACE, 5, "binNum, yVal is (%d, %f)\n", binNum, yVal);
    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(xVec);
        PS_VECTOR_PRINT_F32(yVec);
    }

    PS_ASSERT_VECTOR_NON_NULL(xVec, NAN);
    PS_ASSERT_VECTOR_NON_NULL(yVec, NAN);
    PS_ASSERT_VECTOR_TYPE(xVec, PS_TYPE_F32, NAN);
    PS_ASSERT_VECTOR_TYPE(yVec, PS_TYPE_F32, NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(xVec->n - 1), NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(yVec->n - 1), NAN);

    //    psVector *x = psVectorAlloc(3, PS_TYPE_F64);
    //    psVector *y = psVectorAlloc(3, PS_TYPE_F64);
    psVector *x = psVectorAlloc(5, PS_TYPE_F64);
    psVector *y = psVectorAlloc(5, PS_TYPE_F64);
    psF32 tmpFloat = 0.0f;

    //    if ((binNum >= 1) && (binNum <= (yVec->n - 2)) && (binNum <= (xVec->n - 2))) {
    if ((binNum >= 2) && (binNum <= (yVec->n - 3)) && (binNum <= (xVec->n - 3))) {
        // The general case.  We have all three points.
      //        x->data.F64[0] = binNum - 1;
      //        x->data.F64[1] = binNum;
      //        x->data.F64[2] = binNum + 1;
      x->data.F64[0] = xVec->data.F32[binNum - 2];
      x->data.F64[1] = xVec->data.F32[binNum - 1];
      x->data.F64[2] = xVec->data.F32[binNum + 0];
      x->data.F64[3] = xVec->data.F32[binNum + 1];
      x->data.F64[4] = xVec->data.F32[binNum + 2];
        y->data.F64[0] = yVec->data.F32[binNum - 2];
        y->data.F64[1] = yVec->data.F32[binNum - 1];
        y->data.F64[2] = yVec->data.F32[binNum + 0];
	y->data.F64[3] = yVec->data.F32[binNum + 1];
	y->data.F64[4] = yVec->data.F32[binNum + 2];
        psTrace(TRACE, 6, "x vec (orig) is (%f %f %f %f)\n", xVec->data.F32[binNum - 1], xVec->data.F32[binNum], xVec->data.F32[binNum+1], xVec->data.F32[binNum+2]);
        psTrace(TRACE, 6, "x data is (%f %f %f)\n", x->data.F64[0], x->data.F64[1], x->data.F64[2]);
        psTrace(TRACE, 6, "y data is (%f %f %f)\n", y->data.F64[0], y->data.F64[1], y->data.F64[2]);


        // Ensure that the y value lies within range of the y values.
        if (! (((y->data.F64[0] <= yVal) && (yVal <= y->data.F64[4])) ||
               ((y->data.F64[4] <= yVal) && (yVal <= y->data.F64[0]))) ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified yVal, %g, is not within y-range, %g to %g."),
                    (psF64)yVal, y->data.F64[0], y->data.F64[2]);
            return NAN;
        }

        // Ensure that the y values are monotonic.
        if (((y->data.F64[0] < y->data.F64[1]) && !(y->data.F64[1] <= y->data.F64[2])) ||
            ((y->data.F64[0] > y->data.F64[1]) && !(y->data.F64[1] >= y->data.F64[2]))) {
            psError(PS_ERR_UNKNOWN, true,
                    "This routine must be called with monotonically increasing or decreasing data points.\n");
            psFree(x);
            psFree(y);
            return NAN;
        }

        // Determine the coefficients of the polynomial.
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
        if (!psVectorFitPolynomial1D(myPoly, NULL, 0, y, NULL, x)) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    _("Failed to fit a 1-dimensional polynomial to the three specified data points.  "
                      "Returning NAN."));
            psFree(x);
            psFree(y);
            return NAN;
        }

        psTrace(TRACE, 6, "myPoly->coeff[0] is %f\n", myPoly->coeff[0]);
        psTrace(TRACE, 6, "myPoly->coeff[1] is %f\n", myPoly->coeff[1]);
        psTrace(TRACE, 6, "myPoly->coeff[2] is %f\n", myPoly->coeff[2]);
        psTrace(TRACE, 6, "Fitted y vec is (%f %f %f)\n",
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[0]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[1]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[2]));

        psTrace(TRACE, 6, "We fit the polynomial, now find x such that f(x) equals %f\n", yVal);
        float binValue = QuadraticInverse(myPoly->coeff[2], myPoly->coeff[1], myPoly->coeff[0], yVal, x->data.F64[0], x->data.F64[4]);
        psFree(myPoly);

        if (isnan(binValue)) {
            psError(PS_ERR_UNEXPECTED_NULL,
                    false, _("Failed to determine the median of the fitted polynomial.  Returning NAN."));
            psFree(x);
            psFree(y);
            return(NAN);
        }
	
        // I believe that mathematically the fitted bin position must be between binNum - 1 and binNum + 1
	//	assert (binValue >= binNum - 1);
	//	assert (binValue <= binNum + 1);

	//	int fitBin = binValue;
	//        float dX = xVec->data.F32[fitBin+1] - xVec->data.F32[fitBin];
	//        float dY = binValue - fitBin;
	//        tmpFloat = xVec->data.F32[fitBin] + dY * dX;
	tmpFloat = binValue;
	
    } else {
        // These are special cases where the bin is at the beginning or end of the vector.
        if (binNum == 0) {
            // We have two points only at the beginning of the vectors x and y.
            // X = (dX/dY)(Y - Yo) + Xo
            float dX = xVec->data.F32[1] - xVec->data.F32[0];
            float dY = yVec->data.F32[1] - yVec->data.F32[0];
            if (dY == 0.0) {
                tmpFloat = xVec->data.F32[0];
            } else {
                tmpFloat = (yVal - yVec->data.F32[0]) * (dX / dY) + xVec->data.F32[0];
            }
        } else if (binNum == (xVec->n - 1)) {
            // We have two points only at the end of the vectors x and y.
            // X = (dX/dY)(Y - Yo) + Xo
            float dX = xVec->data.F32[binNum] - xVec->data.F32[binNum-1];
            float dY = yVec->data.F32[binNum] - yVec->data.F32[binNum-1];
            if (dY == 0.0) {
                tmpFloat = xVec->data.F32[binNum-1];
            } else {
                tmpFloat = (yVal - yVec->data.F32[binNum-1]) * (dX / dY) + xVec->data.F32[binNum-1];
            }
        }
    }

    psTrace(TRACE, 6, "FIT: return %f\n", tmpFloat);
    psFree(x);
    psFree(y);

    return tmpFloat;
}
# endif


/******************************************************************************
fitQuadraticSearchForYThenReturnXusingValues(*xVec, *yVec, binNum, yVal): A general routine
which fits a quadratic to three points and returns the x bin value corresponding to the input
y-value.  This routine takes psVectors of x/y pairs as input, and fits a quadratic to the 3
points surrounding element binNum in the vectors.  This version uses the values of x[i] for the
x coordinates (not the midpoints).  This is appropriate for a cumulative histogram.  It then
determines for what value x does that quadratic f(x) = yVal (the input parameter).

XXX this function is used a fair amount in an inner loop: the polynomial fitting and evaluation
could easily be done with statically allocated doubles, skipping the psLib versions of
polynomial fitting, etc.

*****************************************************************************/
static psF32 fitLinearSearchForYThenReturnBin(const psVector *xVec,
					      psVector *yVec,
					      psS32 binNum,
					      psF32 yVal
    )
{

# if (1)
# define HALF_SIZE 2
  double Sx = 0.0;

  double Sy = 0.0;
  double Sxx = 0.0;
  double Sxy = 0.0;
  double deltaY = 0.0;
  int N = 0;

  for (int u = binNum - HALF_SIZE; u <= binNum + HALF_SIZE; u++) {
    if ((u >= 0)&&(u < yVec->n)) {
      if (u+1 < xVec->n) {
	Sx += yVec->data.F32[u];
	Sxx += PS_SQR(yVec->data.F32[u]);

	deltaY = xVec->data.F32[u];
	//deltaY = 0.5 * (xVec->data.F32[u] + xVec->data.F32[u+1]);
	Sy += deltaY;
	Sxy += yVec->data.F32[u] * deltaY;
	N += 1;
      }
    }
  }
  double Det = N * Sxx - Sx * Sx;
  if (Det == 0.0) return NAN;
  if (N == 0) return NAN;

  double C0 = (Sy*Sxx - Sx*Sxy) / Det;
  double C1 = (Sxy*N - Sx*Sy) / Det;
  
  double value = C0 + yVal*C1;
  return value;
  
  
# else
    psTrace(TRACE, 5, "binNum, yVal is (%d, %f)\n", binNum, yVal);
    if (psTraceGetLevel("psLib.math") >= 8) {
        PS_VECTOR_PRINT_F32(xVec);
        PS_VECTOR_PRINT_F32(yVec);
    }

    PS_ASSERT_VECTOR_NON_NULL(xVec, NAN);
    PS_ASSERT_VECTOR_NON_NULL(yVec, NAN);
    PS_ASSERT_VECTOR_TYPE(xVec, PS_TYPE_F32, NAN);
    PS_ASSERT_VECTOR_TYPE(yVec, PS_TYPE_F32, NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(xVec->n - 1), NAN);
    PS_ASSERT_INT_WITHIN_RANGE(binNum, 0, (int)(yVec->n - 1), NAN);

    //    psVector *x = psVectorAlloc(3, PS_TYPE_F64);
    //    psVector *y = psVectorAlloc(3, PS_TYPE_F64);
    psVector *x = psVectorAlloc(5, PS_TYPE_F64);
    psVector *y = psVectorAlloc(5, PS_TYPE_F64);
    psF32 tmpFloat = 0.0f;

    if ((binNum >= 2) && (binNum <= (yVec->n - 3)) && (binNum <= (xVec->n - 3))) {
	x->data.F64[0] = xVec->data.F32[binNum - 2];
	x->data.F64[1] = xVec->data.F32[binNum - 1];
	x->data.F64[2] = xVec->data.F32[binNum + 0];
	x->data.F64[3] = xVec->data.F32[binNum + 1];
	x->data.F64[4] = xVec->data.F32[binNum + 2];

	y->data.F64[0] = yVec->data.F32[binNum - 2];
	y->data.F64[1] = yVec->data.F32[binNum - 1];
	y->data.F64[2] = yVec->data.F32[binNum + 0];
	y->data.F64[3] = yVec->data.F32[binNum + 1];
	y->data.F64[4] = yVec->data.F32[binNum + 2];
	psTrace(TRACE, 6, "x vec (orig) is (%f %f %f %f)\n", xVec->data.F32[binNum - 1], xVec->data.F32[binNum], xVec->data.F32[binNum+1], xVec->data.F32[binNum+2]);
	psTrace(TRACE, 6, "x data is (%f %f %f)\n", x->data.F64[0], x->data.F64[1], x->data.F64[2]);
	psTrace(TRACE, 6, "y data is (%f %f %f)\n", y->data.F64[0], y->data.F64[1], y->data.F64[2]);

	// Ensure that the y value lies within range of the y values.
	if (! (((y->data.F64[0] <= yVal) && (yVal <= y->data.F64[4])) ||
	       ((y->data.F64[4] <= yVal) && (yVal <= y->data.F64[0]))) ) {
	    psError(PS_ERR_BAD_PARAMETER_VALUE, true,
		    _("Specified yVal, %g, is not within y-range, %g to %g."),
                    (psF64)yVal, y->data.F64[0], y->data.F64[2]);
            return NAN;
        }

        // Ensure that the y values are monotonic.
        if (((y->data.F64[0] < y->data.F64[1]) && !(y->data.F64[1] <= y->data.F64[2])) ||
            ((y->data.F64[0] > y->data.F64[1]) && !(y->data.F64[1] >= y->data.F64[2]))) {
            psError(PS_ERR_UNKNOWN, true,
                    "This routine must be called with monotonically increasing or decreasing data points.\n");
            psFree(x);
            psFree(y);
            return NAN;
        }

        // Determine the coefficients of the polynomial.
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
        if (!psVectorFitPolynomial1D(myPoly, NULL, 0, y, NULL, x)) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    _("Failed to fit a 1-dimensional polynomial to the three specified data points.  "
                      "Returning NAN."));
            psFree(x);
            psFree(y);
            return NAN;
        }

        psTrace(TRACE, 6, "myPoly->coeff[0] is %f\n", myPoly->coeff[0]);
        psTrace(TRACE, 6, "myPoly->coeff[1] is %f\n", myPoly->coeff[1]);
        psTrace(TRACE, 6, "Fitted y vec is (%f %f)\n",
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[0]),
                (psF32) psPolynomial1DEval(myPoly, (psF64) x->data.F64[1]));

        psTrace(TRACE, 6, "We fit the polynomial, now find x such that f(x) equals %f\n", yVal);
        float binValue = LinearInverse(myPoly->coeff[1], myPoly->coeff[0], yVal, x->data.F64[0], x->data.F64[4]);
        psFree(myPoly);

        if (isnan(binValue)) {
            psError(PS_ERR_UNEXPECTED_NULL,
                    false, _("Failed to determine the median of the fitted polynomial.  Returning NAN."));
            psFree(x);
            psFree(y);
            return(NAN);
        }
	
        // I believe that mathematically the fitted bin position must be between binNum - 1 and binNum + 1
	//	assert (binValue >= binNum - 1);
	//	assert (binValue <= binNum + 1);

	//	int fitBin = binValue;
	//        float dX = xVec->data.F32[fitBin+1] - xVec->data.F32[fitBin];
	//        float dY = binValue - fitBin;
	//        tmpFloat = xVec->data.F32[fitBin] + dY * dX;
	tmpFloat = binValue;
		
	
    } else {
        // These are special cases where the bin is at the beginning or end of the vector.
        if (binNum == 0) {
            // We have two points only at the beginning of the vectors x and y.
            // X = (dX/dY)(Y - Yo) + Xo
            float dX = xVec->data.F32[1] - xVec->data.F32[0];
            float dY = yVec->data.F32[1] - yVec->data.F32[0];
            if (dY == 0.0) {
                tmpFloat = xVec->data.F32[0];
            } else {
                tmpFloat = (yVal - yVec->data.F32[0]) * (dX / dY) + xVec->data.F32[0];
            }
        } else if (binNum == (xVec->n - 1)) {
            // We have two points only at the end of the vectors x and y.
            // X = (dX/dY)(Y - Yo) + Xo
            float dX = xVec->data.F32[binNum] - xVec->data.F32[binNum-1];
            float dY = yVec->data.F32[binNum] - yVec->data.F32[binNum-1];
            if (dY == 0.0) {
                tmpFloat = xVec->data.F32[binNum-1];
            } else {
                tmpFloat = (yVal - yVec->data.F32[binNum-1]) * (dX / dY) + xVec->data.F32[binNum-1];
            }
        }
    }

    psTrace(TRACE, 6, "FIT: return %f\n", tmpFloat);
    psFree(x);
    psFree(y);

    return tmpFloat;
# endif
}

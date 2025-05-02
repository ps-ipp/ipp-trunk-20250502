/** @file  psHistogram.h
 *  \brief basic histogram functions
 *  @ingroup Math
 *
 *  @author GLG (MHPCC), EAM (IfA)
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2006 IfA, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>
#include <limits.h>
#include <strings.h>

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/
#include "psMemory.h"
#include "psAbort.h"
//#include "psImage.h"
#include "psVector.h"
#include "psTrace.h"
#include "psLogMsg.h"
#include "psError.h"
#include "psStats.h"
#include "psHistogram.h"
#include "psAssert.h"
#include "psMathUtils.h"
//#include "psList.h"
//#include "psString.h"

static void histogramFree(psHistogram* myHist);

/******************************************************************************
psHistogramAlloc(lower, upper, n): allocate a uniform histogram structure
with the specifed upper and lower limits, and the specifed number of bins.
This routine will also set the bounds for each of the bins.

Input:
    lower
    upper
    n
Returns:
    The histogram structure
 *****************************************************************************/
psHistogram* psHistogramAlloc(float lower, float upper, int n)
{
    psTrace("psLib.math", 3, "---- %s() begin  ----\n", __func__);
    psTrace("psLib.math", 5, "(lower, upper, n) is (%f, %f, %d)\n", lower, upper, n);
    psAssert(n > 0, "Number of bins must be positive not %d", n);
    psAssert(upper >= lower, "Bounds must be sensical");

    // Allocate memory for the new histogram structure.  If there are N bins, then there are N+1 bounds to
    // those bins.
    psHistogram *newHist = (psHistogram* ) psAlloc(sizeof(psHistogram)); // The new histogram structure
    psMemSetDeallocator(newHist, (psFreeFunc) histogramFree);
    psVector* newBounds = psVectorAlloc(n + 1, PS_TYPE_F32);
    newHist->bounds = newBounds;

    // Calculate the bounds for each bin.
    psF32 binSize = (upper - lower) / (psF32)n; // The histogram bin size
    // XXX: Is the following necessary? It prevents the max data point from being in a non-existant bin.
    binSize += FLT_EPSILON;
    for (long i = 0; i < n + 1; i++) {
        newBounds->data.F32[i] = lower + (binSize * (psF32)i);
    }

    // Allocate the bins, and initialize them to zero.
    newHist->nums = psVectorAlloc(n, PS_TYPE_F32);
    psVectorInit(newHist->nums, 0.0);

    // Initialize the other members.
    newHist->minNum = 0;
    newHist->maxNum = 0;
    newHist->uniform = true;

    psTrace("psLib.math", 3, "---- %s() end  ----\n", __func__);
    return newHist;
}

/******************************************************************************
psHistogramAllocGeneric(bounds): allocate a non-uniform histogram structure
with the specifed bounds.

Input:
    bounds
Returns:
    The histogram structure
 *****************************************************************************/
psHistogram* psHistogramAllocGeneric(const psVector* bounds)
{
    psTrace("psLib.math", 3, "---- %s() begin  ----\n", __func__);
    PS_ASSERT_VECTOR_NON_NULL(bounds, NULL);
    PS_ASSERT_VECTOR_TYPE(bounds, PS_TYPE_F32, NULL);
    PS_ASSERT_LONG_LARGER_THAN_OR_EQUAL(bounds->n, (long)2, NULL);

    // Allocate memory for the new histogram structure.
    psHistogram *newHist = (psHistogram* ) psAlloc(sizeof(psHistogram)); // The new histogram structure
    psMemSetDeallocator(newHist, (psFreeFunc) histogramFree);
    psVector* newBounds = psVectorCopy(NULL, bounds, PS_TYPE_F32);
    newHist->bounds = newBounds;

    // Allocate the bins, and initialize them to zero.  If there are N bounds,
    // then there are N-1 bins.
    newHist->nums = psVectorAlloc((bounds->n) - 1, PS_TYPE_F32);
    psVectorInit(newHist->nums, 0.0);

    // Initialize the other members.
    newHist->minNum = 0;
    newHist->maxNum = 0;
    newHist->uniform = false;

    psTrace("psLib.math", 3, "---- %s() end  ----\n", __func__);
    return (newHist);
}

static void histogramFree(psHistogram* myHist)
{
    psFree(myHist->bounds);
    psFree(myHist->nums);
}


bool psMemCheckHistogram(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)histogramFree );
}

/*****************************************************************************
UpdateHistogramBins(binNum, out, data, error): This routine is to be used when
updating the histogram in the presence of errors in the input data.  We treat
the data point as a boxcar PDF and update a range of points surrounding the
histogram bin which contains the point.  The width of that boxcar is defined
as 2.35 * error.

XXX: Must test this.
 *****************************************************************************/
static bool UpdateHistogramBins(long binNum, // Bin number of the data point
                                psHistogram* out, // The histogram to be updated
                                psF32 data, // The data point value
                                psF32 error // The error in the data point
                               )
{
    psTrace("psLib.math", 3, "---- %s() begin  ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(out, false);
    PS_ASSERT_PTR_NON_NULL(out->bounds, false);
    PS_ASSERT_PTR_NON_NULL(out->nums, false);
    PS_ASSERT_LONG_WITHIN_RANGE(binNum, (long)0, (long)((out->nums->n)-1), false);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(error, 0.0, false);
    PS_ASSERT_FLOAT_WITHIN_RANGE(data, out->bounds->data.F32[0],
                                 out->bounds->data.F32[(out->bounds->n)-1], false);

    psF32 boxcarWidth = 2.35 * error;   // Width of the boxcar
    psF32 boxcarCenter = (out->bounds->data.F32[binNum] +
                          out->bounds->data.F32[binNum+1]) / 2.0; // Centre of the boxcar
    psF32 boxcarLeft = boxcarCenter - (boxcarWidth / 2.0); // Left endpoint of the boxcar for the PDF
    psF32 boxcarRight = boxcarCenter + (boxcarWidth / 2.0); // Right endpoint of the boxcar for the PDF
    psS32 boxcarLeftBinNum = 0;         // Bin number for left endpoint
    psS32 boxcarRightBinNum = 0;        // Bin number for right endpoint

    // Determine the left endpoint of the boxcar for the PDF.
    for (long bin = binNum; bin >= 0; bin--) {
        if (out->nums->data.F32[bin] <= boxcarLeft) {
            boxcarLeftBinNum = bin;
            break;
        }
    }

    // Determine the right endpoint of the boxcar for the PDF.
    for (long bin = binNum; bin < out->nums->n; bin++) {
        if (out->nums->data.F32[bin] >= boxcarRight) {
            boxcarRightBinNum = bin;
            break;
        }
    }

    // If the boxcar fits entirely inside this bin, then simply add 1.0 to the
    // bin and return.
    if (boxcarLeftBinNum == boxcarRightBinNum) {
        out->nums->data.F32[binNum] += 1.0;
        psTrace("psLib.math", 3, "---- %s(true) end  ----\n", __func__);
        return true;
    }

    // If we get here, multiple bins must be updated.  We handle the left-most endpoint, and right-most
    // endpoints differently.
    out->nums->data.F32[boxcarLeftBinNum] +=
        (out->bounds->data.F32[boxcarLeftBinNum+1] - boxcarLeft) / boxcarWidth;

    // Loop through the center bins, if any.
    for (long bin = boxcarLeftBinNum + 1; bin < (boxcarRightBinNum - 1); bin++) {
        out->nums->data.F32[bin] +=
            (out->bounds->data.F32[bin+1] - out->bounds->data.F32[bin]) / boxcarWidth;
    }

    // Handle the right endpoint differently.
    out->nums->data.F32[boxcarRightBinNum]+=
        (boxcarRight - out->bounds->data.F32[boxcarRightBinNum]) / boxcarWidth;

    psTrace("psLib.math", 3, "---- %s(true) end  ----\n", __func__);
    return true;
}

/*****************************************************************************
psVectorHistogram(out, in, errors, mask, maskVal): this procedure takes as
input a preallocated and initialized histogram structure.  It fills the bins
in that histogram structure in accordance with the input data "in" and the,
possibly NULL, mask vector.

Inputs:
    out
    in
    mask
    maskVal
Returns:
    The histogram structure "out".
 *****************************************************************************/
bool psVectorHistogram(psHistogram* out,
                               const psVector* values,
                               const psVector* errors,
                               const psVector* mask,
                               psVectorMaskType maskVal)
{
    psTrace("psLib.math", 3, "---- %s() begin  ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(out, false);
    PS_ASSERT_VECTOR_NON_NULL(out->bounds, false);
    PS_ASSERT_VECTOR_TYPE(out->bounds, PS_TYPE_F32, false);
    PS_ASSERT_INT_NONNEGATIVE(out->bounds->n, false);
    PS_ASSERT_VECTOR_NON_NULL(out->nums, false);
    PS_ASSERT_VECTOR_TYPE(out->nums, PS_TYPE_F32, false);
    PS_ASSERT_INT_NONNEGATIVE(out->nums->n, false);
    PS_ASSERT_VECTOR_NON_NULL(values, out);
    if (mask) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
    }
    if (errors) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, errors, false);
        PS_ASSERT_VECTOR_TYPE(errors, values->type.type, false);
    }

    long binNum = 0;                    // A temporary bin number
    long numBins = out->nums->n;        // The total number of bins
    psScalar tmpScalar;
    tmpScalar.type.type = PS_TYPE_F32;

    // Convert input and errors vectors to F32 if necessary.
    psVector* inF32 = NULL;             // F32 version of input vector
    if (values->type.type == PS_TYPE_F32) {
        inF32 = psMemIncrRefCounter((psPtr)values);
    } else {
        inF32 = psVectorCopy(NULL, values, PS_TYPE_F32);
    }
    psVector* errorsF32 = NULL;         // F32 version of errors vector
    if (errors) {
        if (errors->type.type == PS_TYPE_F32) {
            errorsF32 = psMemIncrRefCounter((psPtr)errors);
        } else {
            errorsF32 = psVectorCopy(NULL, errors, PS_TYPE_F32);
        }
    }

    for (long i = 0; i < inF32->n; i++) {
        // Check if this pixel is masked, and if so, skip it.
        if (!mask || (mask && (!(mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskVal)))) {
            if (inF32->data.F32[i] < out->bounds->data.F32[0]) {
                // If this pixel is below minimum value, count it, then skip.
                out->minNum++;
            } else if (inF32->data.F32[i] > out->bounds->data.F32[numBins]) {
                // If this pixel is above maximum value, count it, then skip.
                out->maxNum++;
            } else {
                // If this is a uniform histogram, determining the correct bin
                // number is almost trivial.  start with a guess from the uniform scale.
                if (out->uniform == true) {
                    double binSize = (out->bounds->data.F32[out->nums->n] - out->bounds->data.F32[0]) / (float) out->nums->n; // Histogram bin size
                    binNum = (inF32->data.F32[i] - out->bounds->data.F32[0]) / binSize;
                    binNum = PS_MAX (binNum, 0);
                    binNum = PS_MIN (binNum, numBins - 1);

                    // value is in bin 'i' if bound[i] <= value < bound[i]

                    // we may slightly overshoot or undershoot.  creep up or down on the true bin
                    if (inF32->data.F32[i] < out->bounds->data.F32[binNum]) {
                        psTrace("psLib.math", 6, "missed target bin, adjusting: %f vs %f to %f\n", inF32->data.F32[i], out->bounds->data.F32[binNum], out->bounds->data.F32[binNum+1]);
                        while ((inF32->data.F32[i] < out->bounds->data.F32[binNum]) && (binNum > 0)) {
                            binNum --;
                        }

                    }
                    if (inF32->data.F32[i] >= out->bounds->data.F32[binNum+1]) {
                        psTrace("psLib.math", 6, "missed target bin, adjusting: %f vs %f to %f\n", inF32->data.F32[i], out->bounds->data.F32[binNum], out->bounds->data.F32[binNum+1]);
                        while ((inF32->data.F32[i] >= out->bounds->data.F32[binNum+1]) && (binNum < numBins - 1)) {
                            binNum ++;
                        }
                    }

                    if (errorsF32) {
                        if (!UpdateHistogramBins(binNum, out, inF32->data.F32[i], errorsF32->data.F32[i])) {
                            psLogMsg(__func__, PS_LOG_WARN, "WARNING: Failed to update the histogram "
                                     "bins with the errors vector.\n");
                        }
                    } else {
                        // This if-statement really shouldn't be necessary.
                        // However, due to numerical lack of precision, we
                        // occasionally produce a binNum outside the range.
                        if (binNum >= out->nums->n) {
                            binNum = out->nums->n - 1;
                        }
                        (out->nums->data.F32[binNum])+= 1.0;
                    }

                } else {
                    // If this is a non-uniform histogram, determining the
                    // correct bin number requires a bit more work.
                    tmpScalar.data.F32 = inF32->data.F32[i];
                    psVectorBinaryDisectResult result;
                    binNum = psVectorBinaryDisect(&result, out->bounds, &tmpScalar);
                    if (result != PS_BINARY_DISECT_PASS) {
                        continue;
                    }
                    if (errorsF32 != NULL) {
                        if (!UpdateHistogramBins(binNum, out, inF32->data.F32[i], errors->data.F32[i])) {
                            psLogMsg(__func__, PS_LOG_WARN, "WARNING: Failed to update the histogram "
                                     "bins with the errors vector.\n");
                        }
                    } else {
                        out->nums->data.F32[binNum] += 1.0;
                    }
                }
            }
        }
    }

    psFree(inF32);
    psFree(errorsF32);

    psTrace("psLib.math", 3, "---- %s() end  ----\n", __func__);
    return true;
}


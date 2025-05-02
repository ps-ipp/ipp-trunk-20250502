#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <math.h>

#include "psMemory.h"
#include "psError.h"
#include "psErrorCodes.h"
#include "psAssert.h"
#include "psTrace.h"
#include "psVector.h"
#include "psStats.h"
#include "psClip.h"

// No-op; purpose of function is merely to identify the type
static void clipParamsFree(psClipParams *params)
{
    return;
}

psClipParams *psClipParamsAlloc(psStatsOptions meanStat, psStatsOptions stdevStat,
                                psVectorMaskType masked, psVectorMaskType clipped)
{
    psClipParams *params = psAlloc(sizeof(psClipParams)); // Clip parameters
    psMemSetDeallocator(params, (psFreeFunc)clipParamsFree);

    params->meanStat = meanStat;
    params->stdevStat = stdevStat;
    params->fracHigh = 0.0;
    params->fracLow = 0.0;
    params->numKeep = 0;
    params->iter = 1;
    params->rej = 0.0;
    params->masked = masked;
    params->clipped = clipped;

    params->mean = NAN;
    params->stdev = NAN;

    return params;
}


long psClipMinMax(const psClipParams *params, const psVector *values, psVector *mask)
{
    PS_ASSERT_VECTOR_NON_NULL(values, -1);
    PS_ASSERT_VECTOR_NON_NULL(mask, -1);
    PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, -1);
    PS_ASSERT_PTR(params, -1);
    PS_ASSERT_VECTORS_SIZE_EQUAL(values, mask, -1);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(params->fracLow, 0.0, -1);
    PS_ASSERT_FLOAT_LESS_THAN(params->fracLow, 1.0, -1);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(params->fracHigh, 0.0, -1);
    PS_ASSERT_FLOAT_LESS_THAN(params->fracHigh, 1.0, -1);
    PS_ASSERT_INT_NONZERO(params->clipped, -1);

    if (params->fracLow == 0.0 && params->fracHigh == 0.0) {
        // No min-max rejection desired
        return 0;
    }

    psVectorMaskType masked = params->masked; // Indicates masked values
    psVectorMaskType clipped = params->clipped; // Indicates clipped values
    masked |= clipped;                  // Make sure we're also masking clipped values
    psVectorMaskType *maskData = mask->data.PS_TYPE_VECTOR_MASK_DATA; // Dereference mask
    long totalMasked = 0;               // Total number of pixels masked

    // Apply fracLow,fracHigh if there are enough values

    // Run through to get number of operational values
    long numValid = 0;                  // Number of valid values
    for (long i = 0; i < mask->n; i++) {
        if (!(maskData[i] & masked)) {
            numValid++;
        }
    }
    psTrace("psLib.math", 2, "%ld valid values.\n", numValid);

    // XXX: Not sure if sorting provides the fastest implementation.  It might be quicker to do a linear pass
    // through the data, pulling out the highest M and lowest N values,

    float keepFrac = 1.0 - params->fracLow - params->fracHigh; // Fraction of values to keep
    if (numValid * keepFrac >= params->numKeep) {
        psTrace("psLib.math", 1, "Applying min/max clipping.\n");
        psVector *index = psVectorSortIndex(NULL, values);
        int numLow = numValid * params->fracLow; // Number of low values to clip
        int numHigh = numValid * params->fracHigh; // Number of high values to clip
        // Low values
        psS32 *indexData = index->data.S32; // Dereference index
        long numMasked = 0;             // Number masked
        for (long i = 0; i < index->n && numMasked < numLow; i++) {
            // Don't count the ones that are already masked
            if (! (maskData[indexData[i]] & masked)) {
                maskData[indexData[i]] |= clipped;
                numMasked++;
            }
        }
        totalMasked += numMasked;
        numMasked = 0;
        // High values
        for (long i = values->n - 1;  i >= 0 && numMasked < numHigh; i--) {
            // Don't count the ones that are already masked
            if (! (maskData[indexData[i]] & masked)) {
                maskData[indexData[i]] |= clipped;
                numMasked++;
            }
        }
        totalMasked += numMasked;
        psFree(index);
    }

    return totalMasked;
}


long psClipReject(psClipParams *params, const psVector *values, psVector *mask, const psVector *errors)
{
    PS_ASSERT_VECTOR_NON_NULL(values, -1);
    PS_ASSERT_VECTOR_NON_NULL(mask, -1);
    PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, -1);
    PS_ASSERT_PTR(params, -1);
    PS_ASSERT_VECTORS_SIZE_EQUAL(values, mask, -1);
    if (errors) {
        PS_ASSERT_VECTOR_NON_NULL(errors, -1);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, errors, -1);
        PS_ASSERT_VECTOR_TYPE_F32_OR_F64(values, -1);
        PS_ASSERT_VECTOR_TYPE_EQUAL(values, errors, -1);
    }
    PS_ASSERT_INT_NONZERO(params->meanStat, -1);
    PS_ASSERT_INT_NONZERO(params->stdevStat, -1);
    if (!isfinite(params->rej) || params->rej < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Rejection limit (%f) is not valid.\n", params->rej);
        return -1;
    }

    if (params->rej == 0.0) {
        // No clipping desired
        return 0;
    }

    psVectorMaskType masked = params->masked; // Indicates masked values
    psVectorMaskType clipped = params->clipped; // Indicates clipped values
    masked |= clipped;                  // Make sure we're also masking clipped values

    // Get statistics
    psStats *stats = psStatsAlloc(params->meanStat | params->stdevStat);
    if (!psVectorStats(stats, values, errors, mask, masked)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to perform statistics on values.\n");
        psFree(stats);
        return -1;
    }
    params->mean = psStatsGetValue(stats, params->meanStat);
    params->stdev = psStatsGetValue(stats, params->stdevStat);

    #define REJECT_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
        ps##TYPE *valuesData = values->data.TYPE; /* Dereference for speed */ \
        psVectorMaskType *maskData = mask->data.PS_TYPE_VECTOR_MASK_DATA; /* Dereference mask for speed */ \
        if (errors) { \
            ps##TYPE *errorsData = errors->data.TYPE; \
            for (int j = 0; j < values->n; j++) { \
                if (!(maskData[j] & masked) && \
                        fabs(valuesData[j] - params->mean) > params->rej * errorsData[j]) { \
                    maskData[j] |= clipped; \
                    totalMasked++; \
                } \
            } \
        } else { \
            ps##TYPE limit = params->rej * params->stdev; /* Limit on deviation */ \
            for (int j = 0; j < values->n; j++) { \
                if (!(maskData[j] & masked) && fabs(valuesData[j] - params->mean) > limit) { \
                    maskData[j] |= clipped; \
                    totalMasked++; \
                } \
            } \
        } \
    }

    long totalMasked = 0;               // Total number of pixels masked
    switch (values->type.type) {
        REJECT_CASE(S8);
        REJECT_CASE(S16);
        REJECT_CASE(S32);
        REJECT_CASE(S64);
        REJECT_CASE(F32);
        REJECT_CASE(F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unsupported vector type: %x\n", values->type.type);
        psFree(stats);
        return -1;
    }

    psFree(stats);
    return totalMasked;
}

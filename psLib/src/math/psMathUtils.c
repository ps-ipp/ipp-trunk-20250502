/** @file psMathUtils.c
 *
 *  This file contains standard math routines.
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 23:34:03 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
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
#include "psMemory.h"
#include "psVector.h"
#include "psScalar.h"
#include "psTrace.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psPolynomial.h"
#include "psMathUtils.h"
#include "psAssert.h"
#include "psConstants.h"
#include "psAbort.h"

/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/

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

/*****************************************************************************
This is a macro covering the various types for the below function.
 *****************************************************************************/
#define VECTOR_BINARY_DISECT_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
    ps##TYPE *bounds = bins->data.TYPE; \
    ps##TYPE value = x->data.TYPE; \
    long min; \
    long max; \
    long mid; \
    psTrace("psLib.math", 5, "---- () begin ----\n"); \
    psTrace("psLib.math", 4, "Determining the bin for: %f\n", (float) value); \
    if (value < bounds[0]) { \
        psTrace("psLib.math", 3, \
                 "psVectorBinaryDisect : ordinate %f is outside vector range (%f - %f).", \
                 (double)value, (double)bounds[0], (double)bounds[numBins-1]); \
        *status = PS_BINARY_DISECT_OUTSIDE_RANGE; \
        return (0); \
    } \
    if (value > bounds[numBins-1]) { \
        psTrace("psLib.math", 3, \
                 "psVectorBinaryDisect : ordinate %f is outside vector range (%f - %f).", \
                 (double)value, (double)bounds[0], (double)bounds[numBins-1]); \
        *status = PS_BINARY_DISECT_OUTSIDE_RANGE; \
        return (numBins-1); \
    } \
    \
    min = 0; \
    max = numBins-2; \
    mid = ((max+1)-min)/2; \
    while (min != max) { \
        \
        if (value == bounds[mid]) { \
            psTrace("psLib.math", 4, "found %ld\n", mid); \
            psTrace("psLib.math", 5, "---- %s(%ld) end (1) ----\n", __func__, mid); \
            return(mid); \
        } else if (value < bounds[mid]) { \
            max = mid-1; \
        } else { \
            min = mid; \
        } \
        mid = ((max+1)+min)/2; \
    } \
    psTrace("psLib.math", 5, "---- %s(%ld) end (2) ----\n", __func__, min); \
    return(min); \
}

/*****************************************************************************
psVectorBinDisect(): This function takes as input an array of data as well as a single value for that data.
The input vector values are assumed to be non-decreasing (v[i-1] <= v[i] for all i).  This routine does a
binary disection of the vector and returns "i" such that (v[i] <= x < v[i+1).  If x lies outside the range of
v[], then this routine prints a warning message and returns (-2 or -1).
  *****************************************************************************/
psS32 psVectorBinaryDisect(
    psVectorBinaryDisectResult *status,
    const psVector *bins,
    const psScalar *x)
{
    assert (status);
    PS_ASSERT_GENERAL_VECTOR_NON_NULL (bins, *status = PS_BINARY_DISECT_INVALID_INPUT; return 0);
    PS_ASSERT_GENERAL_VECTOR_NON_EMPTY(bins, *status = PS_BINARY_DISECT_INVALID_INPUT; return 0);
    PS_ASSERT_GENERAL_PTR_NON_NULL(x, *status = PS_BINARY_DISECT_INVALID_INPUT; return 0);
    PS_ASSERT_GENERAL_PTR_TYPE_EQUAL(x, bins, *status = PS_BINARY_DISECT_INVALID_INPUT; return 0);
    long numBins = bins->n;             // Number of bins

    *status = PS_BINARY_DISECT_PASS;

    switch (x->type.type) {
        VECTOR_BINARY_DISECT_CASE(S8);
        VECTOR_BINARY_DISECT_CASE(S16);
        VECTOR_BINARY_DISECT_CASE(S32);
        VECTOR_BINARY_DISECT_CASE(S64);
        VECTOR_BINARY_DISECT_CASE(U8);
        VECTOR_BINARY_DISECT_CASE(U16);
        VECTOR_BINARY_DISECT_CASE(U32);
        VECTOR_BINARY_DISECT_CASE(U64);
	VECTOR_BINARY_DISECT_CASE(F32);
        VECTOR_BINARY_DISECT_CASE(F64);
    default: {
            char* strType;
            PS_TYPE_NAME(strType, x->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "psLib type %s is not supported.", strType);
	    *status = PS_BINARY_DISECT_INVALID_TYPE;
            return 0;
        }
    }
    psAbort ("programming error");
    return (0);
}


/*************************************************************************************************************
Helper macro for p_psVectorInterpolate: handles each of the types
*************************************************************************************************************/
#define VECTOR_INTERPOLATE_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
    psTrace("psLib.math", 4, "---- %s() begin %u-order.) (%d data points) ----\n", __func__, order, order+1); \
    if (x->data.TYPE < domain->data.TYPE[0]) { \
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: x is outside the domain of input data.\n"); \
        out->data.TYPE = range->data.TYPE[0]; \
        return(out); \
    } \
    if (x->data.TYPE > domain->data.TYPE[domain->n-1]) { \
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: x is outside the domain of input data.\n"); \
        out->data.TYPE = range->data.TYPE[domain->n-1]; \
        return(out); \
    } \
    psVector *p = psVectorCopy(NULL, range, PS_TYPE_##TYPE); \
    psVectorBinaryDisectResult result; \
    psS32 binNum = psVectorBinaryDisect(&result, domain, x); \
    \
    psS32 numIntPoints = order+1; \
    psS32 origin; \
    if (0 == numIntPoints%2) { \
        origin = binNum - ((numIntPoints/2) - 1); \
    } else { \
        origin = binNum - (numIntPoints/2); \
        if ((x->data.TYPE-domain->data.TYPE[binNum]) > (domain->data.TYPE[binNum+1]-x->data.TYPE)) { \
            /* x is closer to binNum+1. */\
            origin = 1 + (binNum - (numIntPoints/2)); \
        } \
    } \
    origin = PS_MAX(origin, 0); \
    origin = PS_MIN(origin, (domain->n - numIntPoints)); \
    \
    /* From NR, during each iteration of the m loop, we are computing the p_{i ... i+m} terms. */ \
    for (psU32 m = 1 ; m < numIntPoints ; m++) { \
        for (psU32 i = origin ; i < (numIntPoints+origin-m) ; i++) { \
            p->data.TYPE[i] = (((x->data.TYPE - domain->data.TYPE[i+m]) * p->data.TYPE[i]) + \
                               ((domain->data.TYPE[i] - x->data.TYPE) * p->data.TYPE[i+1])) / \
                              (domain->data.TYPE[i] - domain->data.TYPE[i+m]); \
        } \
    } \
    out->data.TYPE = p->data.TYPE[origin]; \
    psFree(p); \
    psTrace("psLib.math", 4, "---- %s(....) end ----\n", __func__); \
    return(out); \
}

/*****************************************************************************
p_psVectorInterpolate(): This routine will take as input psVectors domain and
range, and the x value, assumed to lie with the domain vector.  It produces
as output the LaGrange interpolated value of a polynomial of the specified
order around the point x.
 
XXX: This stuff does not currently work with a mask.
 *****************************************************************************/
psScalar *p_psVectorInterpolate(
    psScalar *out,
    const psVector *domain,
    const psVector *range,
    psS32 order,
    const psScalar *x)
{
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);
    PS_ASSERT_VECTOR_NON_NULL(domain, NULL);
    PS_ASSERT_VECTOR_NON_NULL(range, NULL);
    PS_ASSERT_PTR_NON_NULL(x, NULL);
    PS_ASSERT_INT_NONNEGATIVE(order, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(domain, range, NULL);
    PS_ASSERT_PTR_TYPE_EQUAL(domain, range, NULL);
    PS_ASSERT_PTR_TYPE_EQUAL(domain, x, NULL);
    if (!out) {
        out = psScalarAlloc(0, x->type.type);
    } else {
        PS_ASSERT_PTR_TYPE_EQUAL(domain, out, NULL);
    }

    switch (x->type.type) {
        VECTOR_INTERPOLATE_CASE(U8);
        VECTOR_INTERPOLATE_CASE(U16);
        VECTOR_INTERPOLATE_CASE(U32);
        VECTOR_INTERPOLATE_CASE(U64);
        VECTOR_INTERPOLATE_CASE(S8);
        VECTOR_INTERPOLATE_CASE(S16);
        VECTOR_INTERPOLATE_CASE(S32);
        VECTOR_INTERPOLATE_CASE(S64);
        VECTOR_INTERPOLATE_CASE(F32);
        VECTOR_INTERPOLATE_CASE(F64);
    default: {
            char* strType;
            PS_TYPE_NAME(strType, x->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "psLib type %s is not supported.", strType);
            return NULL;
        }
    }

    return NULL;
}

/*****************************************************************************
Helper macro for p_psNormalizeVectorRange: handles each of the types
 *****************************************************************************/
#define NORMALIZE_VECTOR_RANGE_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
    ps##TYPE low = outLow; \
    ps##TYPE high = outHigh; \
    ps##TYPE min = (ps##TYPE)PS_MAX_##TYPE; \
    ps##TYPE max = (ps##TYPE)-PS_MAX_##TYPE; \
    \
    for (long i = 0; i < myData->n; i++) { \
        if (myData->data.TYPE[i] < min) { \
            min = myData->data.TYPE[i]; \
        } \
        if (myData->data.TYPE[i] > max) { \
            max = myData->data.TYPE[i]; \
        } \
    } \
    \
    /* Ensure that max!=min before we divide by (max-min) */ \
    if (max != min) { \
        for (long i = 0; i < myData->n; i++) { \
            myData->data.TYPE[i] = (low + (myData->data.TYPE[i] - min) * \
                                    (high - low) / (max - min)); \
        } \
    } else { \
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: (max==min).  Setting all elements to min.\n"); \
        for (long i = 0; i < myData->n; i++) \
        { \
            \
            myData->data.TYPE[i] = low; \
            \
        } \
    } \
    break; \
} \

/*************************************************************************************************************
p_psNormalizeVectorRange(myData, low, high): this function normalises the vector (myData) to the range
low:
high.
*************************************************************************************************************/
bool p_psNormalizeVectorRange(psVector* myData,
                              psF64 outLow,
                              psF64 outHigh)
{
    PS_ASSERT_VECTOR_NON_NULL(myData, false);
    psTrace("psLib.math", 4, "---- %s() begin ----\n", __func__);

    switch (myData->type.type) {
        NORMALIZE_VECTOR_RANGE_CASE(U8);
        NORMALIZE_VECTOR_RANGE_CASE(U16);
        NORMALIZE_VECTOR_RANGE_CASE(U32);
        NORMALIZE_VECTOR_RANGE_CASE(U64);
        NORMALIZE_VECTOR_RANGE_CASE(S8);
        NORMALIZE_VECTOR_RANGE_CASE(S16);
        NORMALIZE_VECTOR_RANGE_CASE(S32);
        NORMALIZE_VECTOR_RANGE_CASE(S64);
        NORMALIZE_VECTOR_RANGE_CASE(F32);
        NORMALIZE_VECTOR_RANGE_CASE(F64);
    default: {
            char* strType;
            PS_TYPE_NAME(strType, myData->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "psLib type %s is not supported.", strType);
            break;
        }
    }
    psTrace("psLib.math", 4, "---- %s() end ----\n", __func__);
    return true;
}



/** @file  tst_psStats01.c
*
*  @brief Contains tests for psVectorStats with max calculations.
*
*  We extensively test the code with data type PS_TYPE_F32.  If these pass, we
*  do a much simpler test with data types PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_F64.
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.4 $  $Name: not supported by cvs2svn $
*  @date $Date: 2007-03-27 22:52:03 $
*
* Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define N 15

static psF32 samplesF32[N] = { 1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
                               11.01, -12.02, 13.03, 14.04, -15.05 };
static psS8  samplesS8[N]  = {1, 2, -3, 4, 5, -6, 7, 8, -9, 10, 11, -12, 13, 14, -15};
static psU16 samplesU16[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static psF64 samplesF64[N] = { 1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
                               11.01, -12.02, 13.03, 14.04, -15.05 };

static psF64 expectedMaxNoMaskF32                = 14.04;
static psF64 expectedMaxNoMaskS8                 = 14.00;
static psF64 expectedMaxNoMaskU16                = 15.00;
static psF64 expectedMaxNoMaskF64                = 14.04;

static psF64 expectedMaxWithMaskF32              = 13.03;
static psF64 expectedMaxRangeNoMaskF32           = 10.00;
static psF64 expectedMaxRangeWithMaskF32         = 13.03;

psS32 main(psS32 argc, char* argv[] )
{
    psMemId idGlobal = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(32);

    psStats* myStats = psStatsAlloc(PS_STAT_MAX);
    psVector* myVector =  psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    psVector* maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;
    for (psS32 i = 0; i < N; i++) {
        myVector->data.F32[i] = samplesF32[i];
        if (i < 13) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    // Call psVectorStats() with no vector mask
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxNoMaskF32, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxWithMaskF32, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with data range with no mask
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_MAX | PS_STAT_USE_RANGE;
        myStats->max = 10.2;
        myStats->min = 0.0;
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxRangeNoMaskF32, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxRangeNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with data range and mask
    {
        psMemId id = psMemGetId();
        myStats->max = 14.0;
        myStats->min = 0.0;
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxRangeWithMaskF32, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxRangeWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with data range with no valid data
    {
        psMemId id = psMemGetId();
        myStats->max = 100.00;
        myStats->min = 90.00;
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(!result, "psVectorStats failed as expected");
        ok(isnan(myStats->max), "psVectorStats() returned NAN");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");

    // Test StatsMax: S8
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_MAX);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_S8);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.S8[i] = samplesS8[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxNoMaskS8, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxNoMaskS8);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test StatsMax: U16
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_MAX);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_U16);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.U16[i] = samplesU16[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxNoMaskU16, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxNoMaskU16);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test StatsMax: F64
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_MAX);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_F64);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.F64[i] = samplesF64[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->max), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->max, expectedMaxNoMaskF64, 1e-4,
                     "The max was %f, should be %f", myStats->max, expectedMaxNoMaskF64);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


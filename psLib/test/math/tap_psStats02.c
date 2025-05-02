/** @file  tst_psStats02.c
*
*  @brief Contains tests for psVectorStats with min calculations
*
*  We extensively test the code with data type PS_TYPE_F32.  If these pass, we
*  do a much simpler test with data types PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_F64.
*
*  If the psStats,c code every changes such that vectors of different type
*  are handled by different routines, then these tests must be extended.
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

static psF64 expectedMinNoMaskF32                = -15.05;
static psF64 expectedMinNoMaskS8                 = -15.00;
static psF64 expectedMinNoMaskU16                = 1.00;
static psF64 expectedMinNoMaskF64                = -15.05;

static psF64 expectedMinWithMaskF32              = -12.02;
static psF64 expectedMinRangeNoMaskF32           =   1.10;
static psF64 expectedMinRangeWithMaskF32         = -12.02;

psS32 main(psS32 argc, char* argv[] )
{
    psMemId idGlobal = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(32);

    psStats* myStats = psStatsAlloc(PS_STAT_MIN);
    psVector* myVector = psVectorAlloc(N, PS_TYPE_F32);
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

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinNoMaskF32, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask.                       */
    /*************************************************************************/
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinWithMaskF32, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with data range with no mask
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_MIN | PS_STAT_USE_RANGE;
        myStats->max = 10.2;
        myStats->min = 0.0;
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinRangeNoMaskF32, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinRangeNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with data range and mask
    {
        psMemId id = psMemGetId();
        myStats->max = 10.1;
        myStats->min = -15.00;
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinRangeWithMaskF32, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinRangeWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with data range with no valid data
    {
        psMemId id = psMemGetId();
        myStats->max = 100.00;
        myStats->min = 90.00;
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(!result, "psVectorStats failed as expected");
        ok(isnan(myStats->min), "psVectorStats() returned NAN");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");


    // Test StatsMin: S8
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_MIN);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_S8);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.S8[i] = samplesS8[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinNoMaskS8, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinNoMaskS8);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test StatsMin: U16
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
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinNoMaskU16, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinNoMaskU16);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test StatsMin: F64
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_MIN);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_F64);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.F64[i] = samplesF64[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->min), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->min, expectedMinNoMaskF64, 1e-4,
                     "The min was %f, should be %f", myStats->min, expectedMinNoMaskF64);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


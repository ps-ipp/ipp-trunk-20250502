/*****************************************************************************
    This routine must ensure that PS_STAT_SAMPLE_QUARTILE is correctly computed
    by the procedure psArrayStats().
 
    XXX: Must add tests for various data types, other than psF32.  Copy code
    from tst_psStats00.c-tst_psStats02.c.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define N1 25
#define N (8 * N1) // Don't change this (N must be a multiple of 8)
float realLQNoMask   = N/4.0;
float realUQNoMask   = 3.0 * (N/4.0);
float realLQWithMask   = N/8.0;
float realUQWithMask   = 3.0 * (N/8.0);

psS32 main()
{
    psMemId idGlobal = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(18);

    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_QUARTILE);
    psVector *myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    psVector *maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;
    for (int i=0;i<N;i++) {
        myVector->data.F32[i] = (float) i;
        if (i < (N/2)) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }


    // Call psVectorStats() with no vector mask; test sampleLQ
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleLQ), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleLQ, realLQNoMask, 1e-4,
                     "The sampleLQ was %f, should be %f", myStats->sampleLQ, realLQNoMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with no vector mask; test sampleUQ
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleUQ), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleUQ, realUQNoMask, 1e-4,
                     "The sampleUQ was %f, should be %f", myStats->sampleUQ, realUQNoMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask; test sampleLQ
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleLQ), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleLQ, realLQWithMask, 1e-4,
                     "The sampleLQ was %f, should be %f", myStats->sampleLQ,
                     realLQWithMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask; test sampleUQ
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleUQ), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleUQ, realUQWithMask, 1e-4,
                     "The sampleUQ was %f, should be %f", myStats->sampleUQ,
                     realUQWithMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");
}

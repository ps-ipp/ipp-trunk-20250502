/*****************************************************************************
    This routine must ensure that PS_STAT_SAMPLE_MEDIAN is correctly computed
    by the procedure psVectorStats().
 
    XXX: Must add tests for various data types, other than psF32.  Copy code
    from tst_psStats00.c-tst_psStats02.c.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define N1 1029   // This should be an odd number.
#define N ((4 * N1) + 1)
float realMedianWithMask = (float) (N-3)/4;
float realMedianNoMask = (float) (N-1)/2;

psS32 main()
{
    psMemId idGlobal = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(9);

    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    psVector *myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    psVector *maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;
    // Set the appropriate values for the vector data.
    for (int i=0;i<N;i++) {
        myVector->data.F32[i] = (float) i;
        if (i < (N/2)) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    // Call psVectorStats() with no vector mask
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleMedian), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMedian, realMedianNoMask, 1e-4,
                     "The sample median was %f, should be %f", myStats->sampleMedian, realMedianNoMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleMedian), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMedian, realMedianWithMask, 1e-4,
                     "The sample median was %f, should be %f", myStats->sampleMedian, realMedianWithMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");
}

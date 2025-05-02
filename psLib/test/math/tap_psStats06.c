/*****************************************************************************
    This routine must ensure that PS_STAT_SAMPLE_STDEV is correctly computed
    by the procedure psArrayStats().
 
    XXX: Must add tests for various data types, other than psF32.  Copy code
    from tst_psStats00.c-tst_psStats02.c.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
//#include "float.h"
//#include <math.h>
#define N 15
float realStdevNoMask = 4.472136;
float realStdevWithMask = 2.160247;

psS32 main()
{
    psMemId idGlobal = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(9);

    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_STDEV);
    psVector *myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    psVector *maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;
    psS32 count           = 0;
    for (int i=0;i<N;i++) {
        myVector->data.F32[i] = (float) i;
        if (i < (N/2)) {
            maskVector->data.U8[i] = 0;
            count++;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    // Call psVectorStats() with no vector mask
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleStdev), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleStdev, realStdevNoMask, 1e-4,
                     "The mean was %f, should be %f", myStats->sampleStdev, realStdevNoMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats succeeded");
        ok(!isnan(myStats->sampleStdev), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleStdev, realStdevWithMask, 1e-4,
                     "The mean was %f, should be %f", myStats->sampleStdev, realStdevWithMask);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");
}

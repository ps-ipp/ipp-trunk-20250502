/*****************************************************************************
    This routine must ensure that PS_STAT_SAMPLE_MEDIAN is correctly computed
    by the procedure psVectorStats().
 
    XXX: Must add tests for various data types, other than psF32.  Copy code
    from tst_psStats00.c-tst_psStats02.c.
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#define N1 1029   // This should be an odd number.
#define N ((4 * N1) + 1)

psS32 main()
{
    psLogSetFormat("HLNM");
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("p_psVectorSampleMean", 0);
    psTraceSetLevel("p_psVectorCheckNonEmpty", 0);
    psTraceSetLevel("p_psVectorNValues", 0);
    psTraceSetLevel("p_psVectorSampleMedian", 0);
    psTraceSetLevel("psStatsAlloc", 0);
    psTraceSetLevel("p_psConvertToF32", 0);
    psTraceSetLevel("psVectorStats", 0);

    psStats *myStats    = NULL;
    psS32 testStatus      = true;
    psS32 globalTestStatus = true;
    psS32 i               = 0;
    psVector *myVector  = NULL;
    psVector *maskVector= NULL;
    float median        = 1e99;
    float realMedianWithMask = (float) (N-3)/4;
    float realMedianNoMask = (float) (N-1)/2;
    psS32 currentId       = psMemGetId();
    psS32 memLeaks        = 0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;

    // Set the appropriate values for the vector data.
    for (i=0;i<N;i++) {
        myVector->data.F32[i] = (float) i;
    }

    // Set the mask vector and calculate the expected median.
    for (i=0;i<N;i++) {
        if (i < (N/2)) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_MEDIAN: no vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    median = myStats->sampleMedian;

    printf("Called psVectorStats() on a vector with no elements masked.\n");
    printf("The expected median was %f.  The calculated median was %f.\n",
           realMedianNoMask, median);
    if (median == realMedianNoMask) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }

    printFooter(stdout,
                "psStats functions",
                "PS_STAT_SAMPLE_MEDIAN: no vector mask",
                testStatus);

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask.                       */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_MEDIAN: with vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    median = myStats->sampleMedian;
    printf("Called psVectorStats() on a vector with last N/2 elements masked.\n");
    printf("The expected median was %f.  The calculated median was %f.\n",
           realMedianWithMask, median);
    if (median == realMedianWithMask) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }

    printFooter(stdout,
                "psStats functions",
                "PS_STAT_SAMPLE_MEDIAN: with vector mask",
                testStatus);

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "psStats(): deallocating memory");

    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    printFooter(stdout,
                "psStats functions",
                "psStats(): deallocating memory",
                testStatus);

    return (!globalTestStatus);
}

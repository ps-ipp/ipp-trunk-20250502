/*****************************************************************************
    This routine must ensure that PS_STAT_SAMPLE_QUARTILE is correctly computed
    by the procedure psArrayStats().
 
    XXX: Must add tests for various data types, other than psF32.  Copy code
    from tst_psStats00.c-tst_psStats02.c.
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#include "psTest.h"
#include "float.h"
#include <math.h>

#define N1 25  //
#define N (8 * N1) // Don't change this (N must be a multiple of 8)

psS32 main()
{
    psLogSetFormat("HLNM");
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("p_psVectorMax", 0);
    psTraceSetLevel("p_psVectorMin", 0);
    psTraceSetLevel("p_psVectorCheckNonEmpty", 0);
    psTraceSetLevel("p_psVectorNValues", 0);
    psTraceSetLevel("p_psVectorSampleQuartiles", 0);
    psTraceSetLevel("psStatsAlloc", 0);
    psTraceSetLevel("p_psConvertToF32", 0);
    psTraceSetLevel("psVectorStats", 0);


    psStats *myStats    = NULL;
    psS32 testStatus      = true;
    psS32 globalTestStatus = true;
    psS32 i               = 0;
    psVector *myVector  = NULL;
    psVector *maskVector= NULL;
    // NOTE: These values were calculated by running the function on the data.
    // A: They must be changed if we adjust the number of data points.
    // B: We don't really know that they are correct.
    float realLQNoMask   = N/4.0;
    float realUQNoMask   = 3.0 * (N/4.0);
    float realLQWithMask   = N/8.0;
    float realUQWithMask   = 3.0 * (N/8.0);
    psS32 count           = 0;
    psS32 currentId       = psMemGetId();
    psS32 memLeaks        = 0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_SAMPLE_QUARTILE);
    myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;

    // Set the appropriate values for the vector data.
    for (i=0;i<N;i++) {
        myVector->data.F32[i] = (float) i;
    }

    // Set the mask vector and calculate the expected maximum.
    for (i=0;i<N;i++) {
        if (i < (N/2)) {
            maskVector->data.U8[i] = 0;
            count++;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_LQ: no vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);

    printf("Called psVectorStats() on a vector with no elements masked.\n");
    printf("The expected sampleLQ was %f; the calculated sampleLQ was %f\n",
           realLQNoMask, myStats->sampleLQ);
    if (fabs(realLQNoMask - myStats->sampleLQ) <= 2.0 * FLT_EPSILON) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }
    printFooter(stdout,
                "psVector functions",
                "PS_STAT_SAMPLE_LQ: no vector mask",
                testStatus);



    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_UQ: no vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);

    printf("Called psVectorStats() on a vector with no elements masked.\n");
    printf("The expected sampleUQ was %f; the calculated sampleUQ was %f\n",
           realUQNoMask, myStats->sampleUQ);
    if (fabs(realUQNoMask - myStats->sampleUQ) <= 2.0 * FLT_EPSILON) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }
    printFooter(stdout,
                "psVector functions",
                "PS_STAT_SAMPLE_UQ: no vector mask",
                testStatus);

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask.                       */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_LQ: with vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);

    printf("Called psVectorStats() on a vector with elements masked.\n");
    printf("The expected sampleLQ was %f; the calculated sampleLQ was %f\n",
           realLQWithMask, myStats->sampleLQ);
    if (fabs(realLQWithMask - myStats->sampleLQ) <= 2.0 * FLT_EPSILON) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }
    printFooter(stdout,
                "psVector functions",
                "PS_STAT_SAMPLE_LQ: with vector mask",
                testStatus);



    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_UQ: with vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);

    printf("Called psVectorStats() on a vector with elements masked.\n");
    printf("The expected sampleUQ was %f; the calculated sampleUQ was %f\n",
           realUQWithMask, myStats->sampleUQ);
    if (fabs(realUQWithMask - myStats->sampleUQ) <= 2.0 * FLT_EPSILON) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }
    printFooter(stdout,
                "psVector functions",
                "PS_STAT_SAMPLE_UQ: with vector mask",
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
                "psVector functions",
                "psStats(): deallocating memory",
                testStatus);

    return (!globalTestStatus);
}

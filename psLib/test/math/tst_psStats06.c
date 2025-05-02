/*****************************************************************************
    This routine must ensure that PS_STAT_SAMPLE_STDEV is correctly computed
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

#define N 15

psS32 main()
{
    psLogSetFormat("HLNM");
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("p_psVectorSampleMean", 0);
    psTraceSetLevel("p_psVectorMax", 0);
    psTraceSetLevel("p_psVectorMin", 0);
    psTraceSetLevel("p_psVectorCheckNonEmpty", 0);
    psTraceSetLevel("p_psVectorNValues", 0);
    psTraceSetLevel("p_psVectorSampleStdevOLD", 0);
    psTraceSetLevel("p_psVectorSampleStdev", 0);
    psTraceSetLevel("psStatsAlloc", 0);
    psTraceSetLevel("p_psConvertToF32", 0);
    psTraceSetLevel("psVectorStats", 0);

    psStats *myStats    = NULL;
    psS32 testStatus      = true;
    psS32 globalTestStatus = true;
    psS32 i               = 0;
    psVector *myVector  = NULL;
    psVector *maskVector= NULL;
    float stdev         = 0.0;
    // NOTE: These values were calculated by running the function on the data.
    // A: They must be changed if we adjust the number of data points.
    // B: We don't really know that they are correct.
    float realStdevNoMask   = 4.472136;
    float realStdevWithMask = 2.160247;
    psS32 count           = 0;
    psS32 currentId       = psMemGetId();
    psS32 memLeaks        = 0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_SAMPLE_STDEV);
    myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;

    stdev = 0.0;
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
                            "PS_STAT_SAMPLE_STDEV: no vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    stdev = myStats->sampleStdev;

    printf("Called psVectorStats() on a vector with no elements masked.\n");
    printf("The expected stdev was %f; the calculated stdev was %f\n",
           realStdevNoMask, stdev);
    if (fabs(stdev - realStdevNoMask) <= 2.0 * FLT_EPSILON) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }
    printFooter(stdout,
                "psVector functions",
                "PS_STAT_SAMPLE_STDEV: no vector mask",
                testStatus);

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask.                       */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "PS_STAT_SAMPLE_STDEV: with vector mask");

    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    stdev = myStats->sampleStdev;
    printf("Called psVectorStats() on a vector with last N/2 elements masked.\n");
    printf("The expected stdev was %f; the calculated stdev was %f\n",
           realStdevWithMask, stdev);
    if (fabs(stdev - realStdevWithMask) <= 2.0 * FLT_EPSILON) {
        testStatus = true;
    } else {
        testStatus = false;
        globalTestStatus = false;
    }

    printFooter(stdout,
                "psVector functions",
                "PS_STAT_SAMPLE_STDEV: with vector mask",
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

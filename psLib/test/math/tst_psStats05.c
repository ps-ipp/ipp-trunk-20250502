/*****************************************************************************
    This routine must ensure that the psStats structure is correctly
    allocated and deallocated by the procedure psStatsAlloc().
 
    XXX: This should be test 00.
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#define MISC_FLOAT_NUMBER 345.0

psS32 main()
{
    psLogSetFormat("HLNM");
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("psStatsAlloc", 0);

    psStats *myStats    = NULL;
    psS32 testStatus      = true;
    psS32 currentId       = psMemGetId();
    psS32 memLeaks        = 0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "Allocate the psStats structure.");

    myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    myStats->sampleMean = MISC_FLOAT_NUMBER;
    myStats->sampleMedian = MISC_FLOAT_NUMBER;
    myStats->sampleStdev = MISC_FLOAT_NUMBER;
    myStats->sampleUQ = MISC_FLOAT_NUMBER;
    myStats->sampleLQ = MISC_FLOAT_NUMBER;
    myStats->robustMedian = MISC_FLOAT_NUMBER;
    myStats->robustStdev = MISC_FLOAT_NUMBER;
    myStats->robustUQ = MISC_FLOAT_NUMBER;
    myStats->robustLQ = MISC_FLOAT_NUMBER;
    myStats->robustN50 = MISC_FLOAT_NUMBER;
    myStats->fittedMean = MISC_FLOAT_NUMBER;
    myStats->fittedStdev = MISC_FLOAT_NUMBER;
    myStats->fittedNfit = MISC_FLOAT_NUMBER;
    myStats->clippedMean = MISC_FLOAT_NUMBER;
    myStats->clippedStdev = MISC_FLOAT_NUMBER;
    myStats->clippedNvalues = MISC_FLOAT_NUMBER;
    myStats->clipSigma = MISC_FLOAT_NUMBER;
    myStats->clipIter = MISC_FLOAT_NUMBER;
    myStats->min = MISC_FLOAT_NUMBER;
    myStats->max = MISC_FLOAT_NUMBER;
    myStats->binsize = MISC_FLOAT_NUMBER;
    myStats->nSubsample = MISC_FLOAT_NUMBER;
    myStats->options = 0x0;

    psMemCheckCorruption(1);

    printFooter(stdout,
                "psStats functions",
                "Allocate the psStats structure.",
                testStatus);

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "Deallocate the psStats structure.");
    psFree(myStats);

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    psMemCheckCorruption(1);

    printFooter(stdout,
                "psStats functions",
                "Deallocate the psStats structure.",
                testStatus);

    return (!testStatus);
}

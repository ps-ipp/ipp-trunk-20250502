/*****************************************************************************
    This routine must ensure that the psHistogram structure is correctly
    allocated and deallocated by the procedure psHistogramAlloc().
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#include "psMemory.h"
#define LOWER 20.0
#define UPPER 30.0

psS32 main()
{
    psLogSetFormat("HLNM");
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("p_psVectorBinDisect", 0);
    psTraceSetLevel("psHistogramAlloc", 0);
    psTraceSetLevel("psHistogramAllocGeneric", 0);
    psTraceSetLevel("UpdateHistogramBins", 0);
    psTraceSetLevel("psVectorHistogram", 0);
    psHistogram *myHist = NULL;
    psS32 testStatus      = true;
    psS32 memLeaks        = 0;
    psS32 i               = 0;
    psS32 nb              = 0;
    psS32 numBins         = 0;
    psS32 currentId       = 0;

    currentId       = psMemGetId();
    for (nb=0;nb<4;nb++) {
        if (nb == 0)
            numBins = 1;
        if (nb == 1)
            numBins = 2;
        if (nb == 2)
            numBins = 10;
        if (nb == 3)
            numBins = 20;
        /*********************************************************************/
        /*  Allocate and initialize data structures                          */
        /*********************************************************************/
        printPositiveTestHeader(stdout,
                                "psStats functions",
                                "Allocate the psHistogram structure.");

        myHist = psHistogramAlloc(LOWER, UPPER, numBins);

        if (myHist->nums->n != numBins) {
            printf("ERROR: myHist->nums->n is wrong size (%ld)\n", myHist->nums->n);
            testStatus = false;
        }

        if (myHist->bounds->n != numBins+1) {
            printf("ERROR: myHist->bounds->n is wrong size (%ld)\n", myHist->bounds->n);
            testStatus = false;
        }

        for (i=0;i<numBins;i++) {
            if (myHist->nums->data.F32[i] != 0.0) {
                printf("ERROR: myHist->nums->data.U32[%d] not initialized to 0.\n", i);
                testStatus = false;
            }
            myHist->nums->data.F32[i] = 0.0;
        }

        if (myHist->minNum != 0) {
            printf("ERROR: myHist->minNum is %d\n", myHist->minNum);
            testStatus = false;
        }

        if (myHist->maxNum != 0) {
            printf("myHist->maxNum is %d\n", myHist->maxNum);
            testStatus = false;
        }

        if (myHist->uniform != true) {
            printf("ERROR: myHist->uniform is %d\n", myHist->uniform);
            testStatus = false;
        }

        for (i=0;i<numBins;i++) {
            printf("Bin number %d bounds: (%6.3f - %6.3f)\n", i,
                   myHist->bounds->data.F32[i],
                   myHist->bounds->data.F32[i+1]);
        }

        psMemCheckCorruption(1);
        psFree(myHist);
        psMemCheckCorruption(1);

        printFooter(stdout,
                    "psStats functions",
                    "Allocate the psHistogram structure.",
                    testStatus);
    }


    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "Allocate the psHistogram structure. (UPPER<LOWER)");

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message.");
    myHist = psHistogramAlloc(UPPER, LOWER, numBins);
    if (myHist != NULL) {
        printf("ERROR: myHist != NULL\n");
    }
    printFooter(stdout,
                "psStats functions",
                "Allocate the psHistogram structure. (UPPER<LOWER)",
                testStatus);

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/
    printPositiveTestHeader(stdout,
                            "psStats functions",
                            "Deallocate the psHistogram structure.");

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    psMemCheckCorruption(1);

    printFooter(stdout,
                "psStats functions",
                "Deallocate the psHistogram structure.",
                testStatus);

    return (!testStatus);
}

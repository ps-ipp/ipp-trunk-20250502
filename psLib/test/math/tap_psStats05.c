/*****************************************************************************
    This routine must ensure that the psStats structure is correctly
    allocated and deallocated by the procedure psStatsAlloc().
 
    XXX: This should be test 00.  It should be merged into other tests
         since it's so trivial.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define MISC_FLOAT_NUMBER 342.0

psS32 main()
{
    psLogSetFormat("HLNM");
    plan_tests(1);

    {
        psMemId id = psMemGetId();
        psStats *myStats =psStatsAlloc(PS_STAT_SAMPLE_MEAN);
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
        psFree(myStats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

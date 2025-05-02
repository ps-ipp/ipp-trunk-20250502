/*****************************************************************************
    This routine must ensure that PS_STAT_CLIPPED_MEAN and
    PS_STAT_CLIPPED_STDEV is calculate correctly by the procedure
    psVectorStats().
 
    XXX: The capability is here to test a wide variety of input parameters.
    We must do this, later.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_DATA 1000
#define VERBOSE 0
#define EXTRA_VERBOSE 0
#define PERCENT_OUTLIERS 2
#define ERROR_TOLERANCE .10
#define ERRORS 1.0
#define SEED 1995

#define TST_IN_NULL  0x00000001
#define TST_IN_F32  0x00000002
#define TST_IN_F64  0x00000004
#define TST_IN_S8  0x00000008
#define TST_IN_U16  0x00000010
#define TST_IN_S32  0x00000020
#define TST_ERRORS_NULL  0x00000040
#define TST_ERRORS_F32  0x00000080
#define TST_ERRORS_F64  0x00000100
#define TST_ERRORS_S8  0x00000200
#define TST_ERRORS_U16  0x00000400
#define TST_ERRORS_S32  0x00000800
#define TST_MASK_NULL  0x00001000
#define TST_MASK_U8  0x00002000
#define TST_MASK_S32  0x00004000


psBool genericClippedStatsTest(
    unsigned int flags,
    psS32 numData,
    psU32 maskValue,
    psBool expectedRC)
{
    psS32 currentId = psMemGetId();
    psBool testStatus = true;
    psS32 memLeaks = 0;
    psVector *in = NULL;
    psVector *errors = NULL;
    psVector *mask = NULL;
    srand(SEED);

    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
    psVector *truth = psVectorAlloc(numData, PS_TYPE_F64);
    for (long i = 0; i < numData; i++) {
        truth->data.F64[i] = psRandomGaussian(rng);
    }
    psFree(rng);

    if (expectedRC == true) {
        if (VERBOSE)
            printf("This test should not generate any errors.\n");
    }
    if (expectedRC == false) {
        if (VERBOSE)
            printf("This test should generate an error message, and return NULL.\n");
    }

    if (flags & TST_IN_NULL) {
        if (VERBOSE)
            printf("        using a NULL in vector\n");
    }

    if (flags & TST_IN_F32) {
        if (VERBOSE)
            printf("        using a psF32 in vector\n");
        in = psVectorCopy(in, truth, PS_TYPE_F32);
    }

    if (flags & TST_IN_F64) {
        if (VERBOSE)
            printf("        using a psF64 in vector\n");
        in = psVectorCopy(in, truth, PS_TYPE_F64);
    }

    if (flags & TST_IN_S8) {
        if (VERBOSE)
            printf("        using a psS8 in vector\n");
        in = psVectorCopy(in, truth, PS_TYPE_S8);
    }

    if (flags & TST_IN_U16) {
        if (VERBOSE)
            printf("        using a psU16 in vector\n");
        in = psVectorCopy(in, truth, PS_TYPE_U16);
    }

    if (flags & TST_IN_S32) {
        if (VERBOSE)
            printf("        using a psS32 in vector\n");
        in = psVectorCopy(in, truth, PS_TYPE_S32);
    }

    if (flags & TST_ERRORS_NULL) {
        if (VERBOSE)
            printf("        using a NULL errors vector\n");
    }

    if (flags & TST_ERRORS_F32) {
        if (VERBOSE)
            printf("        using a psF32 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_F32);
        for (psS32 i=0;i<numData;i++) {
            errors->data.F32[i] = ERRORS;
        }
    }

    if (flags & TST_ERRORS_F64) {
        if (VERBOSE)
            printf("        using a psF64 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS32 i=0;i<numData;i++) {
            errors->data.F64[i] = ERRORS;
        }
    }

    if (flags & TST_ERRORS_S8) {
        if (VERBOSE)
            printf("        using a psS8 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_S8);
        for (psS32 i=0;i<numData;i++) {
            errors->data.S8[i] = (psS8) ERRORS;
        }
    }

    if (flags & TST_ERRORS_U16) {
        if (VERBOSE)
            printf("        using a psU16 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_U16);
        for (psS32 i=0;i<numData;i++) {
            errors->data.U16[i] = (psU16) ERRORS;
        }
    }

    if (flags & TST_ERRORS_S32) {
        if (VERBOSE)
            printf("        using a psS32 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            errors->data.S32[i] = (psS32) ERRORS;
        }
    }


    if (flags & TST_MASK_NULL) {
        if (VERBOSE)
            printf("        using a NULL mask vector\n");
    }

    if (flags & TST_MASK_U8) {
        if (VERBOSE)
            printf("        using a psU8 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_U8);
        for (psS32 i=0;i<numData;i++) {
            mask->data.U8[i] = (psU8) 0;
        }
    }

    if (flags & TST_MASK_S32) {
        if (VERBOSE)
            printf("        using a psS32 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            mask->data.S32[i] = (psS32) 0;
        }
    }


    //
    // We add a few outliers to the input data.
    //
    psVector *outliers = psVectorAlloc(numData, PS_TYPE_U8);
    psVectorInit(outliers, 0);
    long numOutliers = 0;
    if (flags & TST_IN_F32) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.F32[i] = 100.0;
                outliers->data.U8[i] = 1;
                numOutliers++;
            }
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Data %d: (%f)\n", i, in->data.F32[i]);
            }
        }
    }
    if (flags & TST_IN_F64) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.F64[i] = 100.0;
                outliers->data.U8[i] = 1;
                numOutliers++;
            }
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Data %d: (%f)\n", i, in->data.F64[i]);
            }
        }
    }
    if (flags & TST_IN_S8) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.S8[i] = 100;
                outliers->data.U8[i] = 1;
                numOutliers++;
            }
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Data %d: (%d)\n", i, in->data.S8[i]);
            }
        }
    }
    if (flags & TST_IN_U16) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.U16[i] = 100;
                outliers->data.U8[i] = 1;
                numOutliers++;
            }
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Data %d: (%d)\n", i, in->data.U16[i]);
            }
        }
    }
    if (flags & TST_IN_S32) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.S32[i] = 100;
                outliers->data.U8[i] = 1;
                numOutliers++;
            }
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Data %d: (%d)\n", i, in->data.S32[i]);
            }
        }
    }

    if (VERBOSE)
        printf("%ld outliers.\n", numOutliers);

    //
    // We calculate the sample mean and stdev without and outliers in the data.
    // We will use this later in determining if the clipped stats are correct.
    //
    psF32 sampleMean=0.0;
    psF32 sampleStdev=0.0;
    if (expectedRC == true) {
        psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        bool rc = psVectorStats(myStats, in, errors, outliers, 1);
        if (rc == false) {
            diag("TEST ERROR: the psVectorStats() function returned NULL (a).\n");
            testStatus = false;
        } else {
            sampleMean = myStats->sampleMean;
            sampleStdev = myStats->sampleStdev;
        }
        psFree(myStats);
    }
    psFree(outliers);

    //
    // We call psVectorStats() and calculate the clipped stats.
    //
    psStats *myStats = psStatsAlloc(PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
    myStats->clipSigma = 5.0;
    myStats->clipIter = 2;
    bool rc = psVectorStats(myStats, in, errors, mask, maskValue);
    if (rc == false) {
        if (expectedRC == true) {
            diag("TEST ERROR: the psVectorStats() function returned NULL (b).\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            diag("TEST ERROR: the psVectorStats() function returned non-NULL.\n");
            testStatus = false;
        }

        if (VERBOSE)
            printf("Used %ld data points after clipping %ld.\n", myStats->clippedNvalues,
                   in->n - myStats->clippedNvalues);

        if (fabs(myStats->clippedMean - sampleMean) > (ERROR_TOLERANCE * sampleMean)) {
            diag("TEST ERROR: the clipped mean was %f, should have been %f\n", myStats->clippedMean, sampleMean);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the clipped mean was %f, should have been %f\n", myStats->clippedMean, sampleMean);
        }

        if (fabs(myStats->clippedStdev - sampleStdev) > (ERROR_TOLERANCE * sampleStdev)) {
            diag("TEST ERROR: the clipped stdev was %f, should have been %f\n", myStats->clippedStdev, sampleStdev);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the clipped stdev was %f, should have been %f\n", myStats->clippedStdev, sampleStdev);
        }

    }

    psFree(myStats);
    psFree(truth);
    psFree(in);
    psFree(errors);
    psFree(mask);
    psMemCheckCorruption(stderr, false);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    return(testStatus);
}

#define TRACE_LEVEL 0
psS32 main()
{
    psMemId id = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(6);

    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorSampleMean", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorMax", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorMin", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorCheckNonEmpty", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorNValues", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorClippedStats", TRACE_LEVEL);
    psTraceSetLevel("p_psNormalizeVectorRange", TRACE_LEVEL);
    psTraceSetLevel("psStatsAlloc", TRACE_LEVEL);
    psTraceSetLevel("p_psConvertToF32", TRACE_LEVEL);
    psTraceSetLevel("psVectorStats", TRACE_LEVEL);

    ok(genericClippedStatsTest(TST_IN_NULL | TST_ERRORS_NULL | TST_MASK_NULL, NUM_DATA, 1, false), "GenericClippedStatsTest(): NULL data");
    ok(genericClippedStatsTest(TST_IN_F32 | TST_ERRORS_NULL | TST_MASK_NULL, NUM_DATA, 1, true), "GenericClippedStatsTest(): F32 data");
    ok(genericClippedStatsTest(TST_IN_F64 | TST_ERRORS_NULL | TST_MASK_NULL, NUM_DATA, 1, true), "GenericClippedStatsTest(): F64 data");
    ok(genericClippedStatsTest(TST_IN_F32 | TST_ERRORS_F32 | TST_MASK_NULL, NUM_DATA, 1, true), "GenericClippedStatsTest(): non-NULL error vector");
    ok(genericClippedStatsTest(TST_IN_F32 | TST_ERRORS_NULL | TST_MASK_U8, NUM_DATA, 1, true), "GenericClippedStatsTest(): non-NULL mask vector");
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}

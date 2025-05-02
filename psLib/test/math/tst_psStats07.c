/*****************************************************************************
   This routine must ensure that PS_STAT_ROBUST_QUARTILE is correctly computed
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

#define NUM_DATA 1000
#define MEAN 32.0
#define STDEV 2.0
#define PERCENT_OUTLIERS 1
#define OUTLIER_MAGNITUDE (NUM_DATA * 10)
#define ERROR_TOLERANCE .10
#define ERRORS 1000.0
#define SEED 1995
#define VERBOSE 0

#define TST_IN_NULL             0x00000001
#define TST_IN_F32              0x00000002
#define TST_IN_F64              0x00000004
#define TST_IN_S8               0x00000008
#define TST_IN_U16              0x00000010
#define TST_IN_S32              0x00000020
#define TST_ERRORS_NULL         0x00000040
#define TST_ERRORS_F32          0x00000080
#define TST_ERRORS_F64          0x00000100
#define TST_ERRORS_S8           0x00000200
#define TST_ERRORS_U16          0x00000400
#define TST_ERRORS_S32          0x00000800
#define TST_MASK_NULL           0x00001000
#define TST_MASK_U8             0x00002000
#define TST_MASK_S32            0x00004000

psBool genericRobustStatsTest(
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
    printPositiveTestHeader(stdout, "psMathUtils functions", "psVectorStats Robust Stats Routine");

    if (expectedRC == true) {
        printf("This test should not generate any errors.\n");
    }
    if (expectedRC == false) {
        printf("This test should generate an error message, and return NULL.\n");
    }
    psVector *gaussVector = p_psGaussianDev(MEAN, STDEV, numData);


    if (flags & TST_IN_NULL) {
        printf("        using a NULL in vector\n");
    }

    if (flags & TST_IN_F32) {
        printf("        using a psF32 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_F32);
        for (psS32 i=0;i<numData;i++) {
            in->data.F32[i] = gaussVector->data.F32[i];
            in->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%.1f)\n", i, in->data.F32[i]);
            }
        }
    }

    if (flags & TST_IN_F64) {
        printf("        using a psF64 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS32 i=0;i<numData;i++) {
            in->data.F64[i] = (psF64) gaussVector->data.F32[i];
            in->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%.1f)\n", i, in->data.F64[i]);
            }
        }
    }

    if (flags & TST_IN_S8) {
        printf("        using a psS8 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_S8);
        for (psS32 i=0;i<numData;i++) {
            in->data.S8[i] = (psS8) gaussVector->data.F32[i];
            in->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.S8[i]);
            }
        }
    }

    if (flags & TST_IN_U16) {
        printf("        using a psU16 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_U16);
        for (psS32 i=0;i<numData;i++) {
            in->data.U16[i] = (psU16) gaussVector->data.F32[i];
            in->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.U16[i]);
            }
        }
    }

    if (flags & TST_IN_S32) {
        printf("        using a psS32 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            in->data.S32[i] = (psS32) gaussVector->data.F32[i];
            in->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.S32[i]);
            }
        }
    }
    psFree(gaussVector);

    //    in->n = in->nalloc;
    if (flags & TST_ERRORS_NULL) {
        printf("        using a NULL errors vector\n");
    }

    if (flags & TST_ERRORS_F32) {
        printf("        using a psF32 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_F32);
        for (psS32 i=0;i<numData;i++) {
            errors->data.F32[i] = ERRORS;
            errors->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%.1f)\n", i, errors->data.F32[i]);
            }
        }
    }

    if (flags & TST_ERRORS_F64) {
        printf("        using a psF64 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS32 i=0;i<numData;i++) {
            errors->data.F64[i] = ERRORS;
            errors->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%.1f)\n", i, errors->data.F64[i]);
            }
        }
    }

    if (flags & TST_ERRORS_S8) {
        printf("        using a psS8 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_S8);
        for (psS32 i=0;i<numData;i++) {
            errors->data.S8[i] = (psS8) ERRORS;
            errors->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%d)\n", i, errors->data.S8[i]);
            }
        }
    }

    if (flags & TST_ERRORS_U16) {
        printf("        using a psU16 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_U16);
        for (psS32 i=0;i<numData;i++) {
            errors->data.U16[i] = (psU16) ERRORS;
            errors->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%d)\n", i, errors->data.U16[i]);
            }
        }
    }

    if (flags & TST_ERRORS_S32) {
        printf("        using a psS32 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            errors->data.S32[i] = (psS32) ERRORS;
            errors->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%d)\n", i, errors->data.S32[i]);
            }
        }
    }


    if (flags & TST_MASK_NULL) {
        printf("        using a NULL mask vector\n");
    }

    if (flags & TST_MASK_U8) {
        printf("        using a psU8 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_U8);
        for (psS32 i=0;i<numData;i++) {
            mask->data.U8[i] = (psU8) 0;
            mask->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original mask data %d: (%d)\n", i, mask->data.U8[i]);
            }
        }
    }

    if (flags & TST_MASK_S32) {
        printf("        using a psS32 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            mask->data.S32[i] = (psS32) 0;
            mask->n++;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original mask data %d: (%d)\n", i, mask->data.S32[i]);
            }
        }
    }


    //
    // We calculate the sample mean and stdev without and outliers in the data.
    // We will use this later in determining if the clipped stats are correct.
    //
    psF32 sampleMean;
    psF32 sampleStdev;
    psF32 sampleMedian;
    psF32 sampleLQ;
    psF32 sampleUQ;
    if (expectedRC == true) {
        psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_QUARTILE);
        psStats *rc = psVectorStats(myStats, in, NULL, NULL, maskValue);
        if (rc == NULL) {
            printf("TEST ERROR: the psVectorStats() function returned NULL.\n");
            testStatus = false;
        } else {
            sampleMean = myStats->sampleMean;
            sampleStdev = myStats->sampleStdev;
            sampleMedian = myStats->sampleMedian;
            sampleLQ = myStats->sampleLQ;
            sampleUQ = myStats->sampleUQ;
        }
        psFree(myStats);
    }

    //
    // We add a few outliers to the input data.
    //
    if (flags & TST_IN_F32) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.F32[i] = (psF32) OUTLIER_MAGNITUDE;
            }
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%.1f)\n", i, in->data.F32[i]);
            }
        }
    }
    if (flags & TST_IN_F64) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.F64[i] = (psF64) OUTLIER_MAGNITUDE;
            }
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%.1f)\n", i, in->data.F64[i]);
            }
        }
    }
    if (flags & TST_IN_S8) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.S8[i] = (psS8) OUTLIER_MAGNITUDE;
            }
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.S8[i]);
            }
        }
    }
    if (flags & TST_IN_U16) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.U16[i] = (psU16) OUTLIER_MAGNITUDE;
            }
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.U16[i]);
            }
        }
    }
    if (flags & TST_IN_S32) {
        for (psS32 i=0;i<numData;i++) {
            if (PERCENT_OUTLIERS > (random() % 100)) {
                in->data.S32[i] = (psS32) OUTLIER_MAGNITUDE;
            }
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.S32[i]);
            }
        }
    }

    //
    // We call psVectorStats() and calculate the clipped stats.
    //
    psStats *myStats = psStatsAlloc(
                           PS_STAT_ROBUST_MEDIAN |
                           PS_STAT_ROBUST_STDEV |
                           PS_STAT_ROBUST_QUARTILE |
                           PS_STAT_FITTED_MEAN |
                           PS_STAT_FITTED_STDEV);
    psStats *rc = psVectorStats(myStats, in, errors, mask, maskValue);

    if (rc == NULL) {
        if (expectedRC == true) {
            printf("TEST ERROR: the psVectorStats() function returned NULL.\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            printf("TEST ERROR: the psVectorStats() function returned non-NULL.\n");
            testStatus = false;
        }

        //
        // Fitted Mean
        //
        if (fabs(myStats->fittedMean - sampleMean) > (ERROR_TOLERANCE * sampleMean)) {
            printf("TEST ERROR: the fitted mean was %.2f, should have been %.2f\n", myStats->fittedMean, sampleMean);
            testStatus = false;
        } else if (VERBOSE) {
            printf("GOOD: the fitted mean was %.2f, should have been %.2f\n", myStats->fittedMean, sampleMean);
        }

        //
        // Fitted Stdev
        //
        if (fabs(myStats->fittedStdev - sampleStdev) > (ERROR_TOLERANCE * sampleStdev)) {
            printf("TEST ERROR: the fitted stdev was %.2f, should have been %.2f\n", myStats->fittedStdev, sampleStdev);
            testStatus = false;
        } else if (VERBOSE) {
            printf("GOOD: the fitted stdev was %.2f, should have been %.2f\n", myStats->fittedStdev, sampleStdev);
        }

        //
        // Robust Stdev
        //
        if (fabs(myStats->robustStdev - sampleStdev) > (ERROR_TOLERANCE * sampleStdev)) {
            printf("TEST ERROR: the robust stdev was %.2f, should have been %.2f\n", myStats->robustStdev, sampleStdev);
            testStatus = false;
        } else if (VERBOSE) {
            printf("GOOD: the robust stdev was %.2f, should have been %.2f\n", myStats->robustStdev, sampleStdev);
        }

        //
        // Robust Median
        //
        if (fabs(myStats->robustMedian - sampleMedian) > (ERROR_TOLERANCE * sampleMedian)) {
            printf("TEST ERROR: the robust median was %.2f, should have been %.2f\n", myStats->robustMedian, sampleMedian);
            testStatus = false;
        } else if (VERBOSE) {
            printf("GOOD: the robust median was %.2f, should have been %.2f\n", myStats->robustMedian, sampleMedian);
        }

        //
        // Robust LQ
        //
        if (fabs(myStats->robustLQ - sampleLQ) > (ERROR_TOLERANCE * sampleLQ)) {
            printf("TEST ERROR: the robust LQ was %.2f, should have been %.2f\n", myStats->robustLQ, sampleLQ);
            testStatus = false;
        } else if (VERBOSE) {
            printf("GOOD: the robust LQ was %.2f, should have been %.2f\n", myStats->robustLQ, sampleLQ);
        }

        //
        // Robust UQ
        //
        if (fabs(myStats->robustUQ - sampleUQ) > (ERROR_TOLERANCE * sampleUQ)) {
            printf("TEST ERROR: the robust UQ was %.2f, should have been %.2f\n", myStats->robustUQ, sampleUQ);
            testStatus = false;
        } else if (VERBOSE) {
            printf("GOOD: the robust UQ was %.2f, should have been %.2f\n", myStats->robustUQ, sampleUQ);
        }
    }

    psFree(myStats);
    psFree(in);
    psFree(errors);
    psFree(mask);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    return(testStatus);
}


psS32 main()
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psBool testStatus = true;
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    #define TRACE_LEVEL 0

    psTraceSetLevel(".", TRACE_LEVEL);
    psTraceSetLevel("psGaussian", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorMax", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorMin", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorCheckNonEmpty", TRACE_LEVEL);
    psTraceSetLevel("p_psNormalizeVectorRange", TRACE_LEVEL);
    psTraceSetLevel("p_ps1DPolyMedian", TRACE_LEVEL);
    psTraceSetLevel("fitQuadraticSearchForYThenReturnX", TRACE_LEVEL);
    psTraceSetLevel("PsVectorDup", TRACE_LEVEL);
    psTraceSetLevel("psMinimizeLMChi2Gauss1D", TRACE_LEVEL);
    psTraceSetLevel("LinInterpolate", TRACE_LEVEL);
    psTraceSetLevel("p_psVectorRobustStats", TRACE_LEVEL);
    psTraceSetLevel("psStatsAlloc", TRACE_LEVEL);
    psTraceSetLevel("psHistogramAlloc", TRACE_LEVEL);
    psTraceSetLevel("psHistogramAllocGeneric", TRACE_LEVEL);
    psTraceSetLevel("UpdateHistogramBins", TRACE_LEVEL);
    psTraceSetLevel("psVectorHistogram", TRACE_LEVEL);
    psTraceSetLevel("p_psConvertToF32", TRACE_LEVEL);
    psTraceSetLevel("psVectorStats", TRACE_LEVEL);

    testStatus &= genericRobustStatsTest(TST_IN_F32 | TST_ERRORS_NULL | TST_MASK_NULL, NUM_DATA, 1, true);
    testStatus &= genericRobustStatsTest(TST_IN_F32 | TST_ERRORS_NULL | TST_MASK_U8, NUM_DATA, 1, true);

    if (testStatus) {
        printf("TEST PASSED.\n");
    } else {
        printf("TEST FAILED.\n");
    }
}


/*****************************************************************************
   This routine must ensure that PS_STAT_ROBUST_QUARTILE is correctly computed
   by the procedure psArrayStats().
 
   XXX: Must add tests for various data types, other than psF32.  Copy code
   from tst_psStats00.c-tst_psStats02.c.

   XXX: This basically works.  Must add bad paramater tests, and test other
   data types, different vector sizes, etc.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_DATA 1000
#define MEAN 32.0
#define STDEV 2.0
#define PERCENT_OUTLIERS 1
#define OUTLIER_MAGNITUDE (NUM_DATA * 10)
#define ERROR_TOLERANCE .10
#define ERRORS 1000.0
#define SEED 1995
#define VERBOSE 0
#define EXTRA_VERBOSE 0

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

/*****************************************************************************
    p_psGaussianDev()
 This private routine (formerly a psLib API routine) creates a psVector of the
 specified size and type F32 and fills it with a random Gaussian distribution
 of numbers with the specified mean and sigma.
 
XXX: It's possible to have a different seed everytime.  However, for now,
for testability, we use a common seed.
 *****************************************************************************/
#define PS_XXX_GAUSSIAN_SEED 1995
psVector* p_psGaussianDev(psF32 mean,
                          psF32 sigma,
                          unsigned int Npts)
{
    PS_ASSERT_INT_NONNEGATIVE(Npts, NULL);

    //    psRandom *r = psRandomAllocSpecific(PS_RANDOM_TAUS, p_psRandomGetSystemSeed());
    psRandom *r = psRandomAllocSpecific(PS_RANDOM_TAUS, PS_XXX_GAUSSIAN_SEED);
    psVector* gauss = psVectorAlloc(Npts, PS_TYPE_F32);
    for (unsigned int i = 0; i < Npts; i++) {
        gauss->data.F32[i] = mean + p_psRandomGaussian(r, sigma);
        gauss->n++;
    }
    psFree(r);

    return(gauss);
}

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

    if (expectedRC == true) {
        if (VERBOSE)
            printf("This test should not generate any errors.\n");
    }
    if (expectedRC == false) {
        if (VERBOSE)
            printf("This test should generate an error message, and return NULL.\n");
    }
    psVector *gaussVector = p_psGaussianDev(MEAN, STDEV, numData);


    if (flags & TST_IN_NULL) {
        if (VERBOSE)
            printf("        using a NULL in vector\n");
    }

    if (flags & TST_IN_F32) {
        if (VERBOSE)
            printf("        using a psF32 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_F32);
        for (psS32 i=0;i<numData;i++) {
            in->data.F32[i] = gaussVector->data.F32[i];
//            in->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%.1f)\n", i, in->data.F32[i]);
            }
        }
    }

    if (flags & TST_IN_F64) {
        if (VERBOSE)
            printf("        using a psF64 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS32 i=0;i<numData;i++) {
            in->data.F64[i] = (psF64) gaussVector->data.F32[i];
//            in->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%.1f)\n", i, in->data.F64[i]);
            }
        }
    }

    if (flags & TST_IN_S8) {
        if (VERBOSE)
            printf("        using a psS8 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_S8);
        for (psS32 i=0;i<numData;i++) {
            in->data.S8[i] = (psS8) gaussVector->data.F32[i];
//            in->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.S8[i]);
            }
        }
    }

    if (flags & TST_IN_U16) {
        if (VERBOSE)
            printf("        using a psU16 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_U16);
        for (psS32 i=0;i<numData;i++) {
            in->data.U16[i] = (psU16) gaussVector->data.F32[i];
//            in->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.U16[i]);
            }
        }
    }

    if (flags & TST_IN_S32) {
        if (VERBOSE)
            printf("        using a psS32 in vector\n");
        in = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            in->data.S32[i] = (psS32) gaussVector->data.F32[i];
//            in->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original in data %d: (%d)\n", i, in->data.S32[i]);
            }
        }
    }
    psFree(gaussVector);

    //    in->n = in->nalloc;
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
//            errors->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%.1f)\n", i, errors->data.F32[i]);
            }
        }
    }

    if (flags & TST_ERRORS_F64) {
        if (VERBOSE)
            printf("        using a psF64 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS32 i=0;i<numData;i++) {
            errors->data.F64[i] = ERRORS;
//            errors->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%.1f)\n", i, errors->data.F64[i]);
            }
        }
    }

    if (flags & TST_ERRORS_S8) {
        if (VERBOSE)
            printf("        using a psS8 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_S8);
        for (psS32 i=0;i<numData;i++) {
            errors->data.S8[i] = (psS8) ERRORS;
//            errors->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%d)\n", i, errors->data.S8[i]);
            }
        }
    }

    if (flags & TST_ERRORS_U16) {
        if (VERBOSE)
            printf("        using a psU16 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_U16);
        for (psS32 i=0;i<numData;i++) {
            errors->data.U16[i] = (psU16) ERRORS;
//            errors->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%d)\n", i, errors->data.U16[i]);
            }
        }
    }

    if (flags & TST_ERRORS_S32) {
        if (VERBOSE)
            printf("        using a psS32 errors vector\n");
        errors = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            errors->data.S32[i] = (psS32) ERRORS;
//            errors->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original errors data %d: (%d)\n", i, errors->data.S32[i]);
            }
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
//            mask->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original mask data %d: (%d)\n", i, mask->data.U8[i]);
            }
        }
    }

    if (flags & TST_MASK_S32) {
        if (VERBOSE)
            printf("        using a psS32 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            mask->data.S32[i] = (psS32) 0;
//            mask->n++;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original mask data %d: (%d)\n", i, mask->data.S32[i]);
            }
        }
    }


    //
    // We calculate the sample mean and stdev without and outliers in the data.
    // We will use this later in determining if the clipped stats are correct.
    //
    psF32 sampleMean=0.0;
    psF32 sampleStdev=0.0;
    psF32 sampleMedian=0.0;
    psF32 sampleLQ=0.0;
    psF32 sampleUQ=0.0;
    if (expectedRC == true) {
        psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_QUARTILE);
        bool rc = psVectorStats(myStats, in, NULL, NULL, maskValue);
        if (rc == false) {
            diag("TEST ERROR: the psVectorStats() function returned NULL.\n");
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

        if (EXTRA_VERBOSE) {
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

        if (EXTRA_VERBOSE) {
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

        if (EXTRA_VERBOSE) {
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

        if (EXTRA_VERBOSE) {
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

        if (EXTRA_VERBOSE) {
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
    bool rc = psVectorStats(myStats, in, errors, mask, maskValue);

    if (rc == false) {
        if (expectedRC == true) {
            diag("TEST ERROR: the psVectorStats() function returned NULL.\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            diag("TEST ERROR: the psVectorStats() function returned non-NULL.\n");
            testStatus = false;
        }

        //
        // Fitted Mean
        //
        if (fabs(myStats->fittedMean - sampleMean) > (ERROR_TOLERANCE * sampleMean)) {
            diag("TEST ERROR: the fitted mean was %.2f, should have been %.2f\n", myStats->fittedMean, sampleMean);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the fitted mean was %.2f, should have been %.2f\n", myStats->fittedMean, sampleMean);
        }

        //
        // Fitted Stdev
        //
        if (fabs(myStats->fittedStdev - sampleStdev) > (ERROR_TOLERANCE * sampleStdev)) {
            diag("TEST ERROR: the fitted stdev was %.2f, should have been %.2f\n", myStats->fittedStdev, sampleStdev);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the fitted stdev was %.2f, should have been %.2f\n", myStats->fittedStdev, sampleStdev);
        }

        //
        // Robust Stdev
        //
        if (fabs(myStats->robustStdev - sampleStdev) > (ERROR_TOLERANCE * sampleStdev)) {
            diag("TEST ERROR: the robust stdev was %.2f, should have been %.2f\n", myStats->robustStdev, sampleStdev);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the robust stdev was %.2f, should have been %.2f\n", myStats->robustStdev, sampleStdev);
        }

        //
        // Robust Median
        //
        if (fabs(myStats->robustMedian - sampleMedian) > (ERROR_TOLERANCE * sampleMedian)) {
            diag("TEST ERROR: the robust median was %.2f, should have been %.2f\n", myStats->robustMedian, sampleMedian);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the robust median was %.2f, should have been %.2f\n", myStats->robustMedian, sampleMedian);
        }

        //
        // Robust LQ
        //
        if (fabs(myStats->robustLQ - sampleLQ) > (ERROR_TOLERANCE * sampleLQ)) {
            diag("TEST ERROR: the robust LQ was %.2f, should have been %.2f\n", myStats->robustLQ, sampleLQ);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the robust LQ was %.2f, should have been %.2f\n", myStats->robustLQ, sampleLQ);
        }

        //
        // Robust UQ
        //
        if (fabs(myStats->robustUQ - sampleUQ) > (ERROR_TOLERANCE * sampleUQ)) {
            diag("TEST ERROR: the robust UQ was %.2f, should have been %.2f\n", myStats->robustUQ, sampleUQ);
            testStatus = false;
        } else if (EXTRA_VERBOSE) {
            printf("GOOD: the robust UQ was %.2f, should have been %.2f\n", myStats->robustUQ, sampleUQ);
        }
    }

    psFree(myStats);
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


psS32 main()
{
    psMemId id = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(4);

    ok(genericRobustStatsTest(TST_IN_F32 | TST_ERRORS_NULL | TST_MASK_NULL,
       NUM_DATA, 1, true), "RobustStatsTest() F32 data, NULL errors, NULL mask");
    ok(genericRobustStatsTest(TST_IN_F32 | TST_ERRORS_NULL | TST_MASK_U8, 
       NUM_DATA, 1, true), "RobustStatsTest() F32 data, non-NULL errors and mask vector");
    ok(genericRobustStatsTest(TST_IN_F32 | TST_ERRORS_F32 | TST_MASK_U8,
        NUM_DATA, 1, true), "RobustStatsTest() F32 data, non-NULL errors and mask vector");

    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}


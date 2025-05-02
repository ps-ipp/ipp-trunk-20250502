/** @file  tst_psStats00.c
 *
 *  @brief Contains tests for psVectorStats with sample mean calculations
 *
 
 *  We extensively test the code with data type PS_TYPE_F32.  If these pass, we do a much
 *  simpler test with data type PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_F64.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-05-08 06:21:16 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
 */

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define N 15

static psF32 samplesF32[N] =
    {
        1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
        11.01, -12.02, 13.03, 14.04, -15.05
    };
static psS8  samplesS8[N]  =
    {
        1, 2, -3, 4, 5, -6, 7, 8, -9, 10, 11, -12, 13, 14, -15
    };
static psU16 samplesU16[N] =
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };
static psF64 samplesF64[N] =
    {
        1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
        11.01, -12.02, 13.03, 14.04, -15.05
    };
static psF32 errorsF32[N] =
    {
        -0.10,  0.11, -0.12,  0.13, -0.14,  0.15, -0.16,  0.17,
        -0.18,  0.19, -0.20,  0.21, -0.22,  0.23, -0.24
    };

static psF64 expectedMeanNoMaskF32              =  2.060667;
static psF64 expectedMeanWithMaskF32            =  2.123846;
static psF64 expectedMeanNoMaskS8               =  2.000000;
static psF64 expectedMeanNoMaskU16              =  8.000000;
static psF64 expectedMeanNoMaskF64              =  2.060667;
static psF64 expectedMeanRangeNoMaskF32         =  0.137500;
static psF64 expectedMeanRangeWithMaskF32       = -0.366667;
static psF64 expectedWeightMeanNoMaskF32        =  2.020035;
static psF64 expectedWeightMeanWithMaskF32      =  2.036018;
static psF64 expectedWeightMeanNoMaskRangeF32   = -0.650684;
static psF64 expectedWeightMeanWithMaskRangeF32 = -1.046423;

#include <unistd.h>
psS32 main(psS32 argc, char* argv[] )
{
    psMemId idGlobal = psMemGetId();
    psLogSetFormat("HLNM");
    plan_tests(60);

    // Allocate data vectors for F32 tests
    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psVector *myVector = psVectorAlloc(N, PS_TYPE_F32);
    psVector *myErrors = psVectorAlloc(N, PS_TYPE_F32);
    psVector *maskVector = psVectorAlloc(N, PS_TYPE_U8);
    // Set the appropriate values for the vector data.
    for (long i = 0; i < N; i++) {
        myVector->data.F32[i] =  samplesF32[i];
        myErrors->data.F32[i] =  errorsF32[i];
        if (i > 1) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    // Call psVectorStats() with no vector mask.
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded (F32: no mask vector, no error vector)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanNoMaskF32, 1e-4,
                     "The mean was %f, should be %f", myStats->sampleMean, expectedMeanNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke psVectorStats with no vector mask and error vector
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, myErrors, NULL, 0);
        ok(result, "psVectorStats suceeded (F32: no mask vector, with error vector)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedWeightMeanNoMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedWeightMeanNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke psVectorStats with no vector mask and data range
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE;
        myStats->min = -10.0;
        myStats->max =   8.0;
        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded (F32, no mask, no errors, with data range)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanRangeNoMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanRangeNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Invoke psVectorStats with no vector mask, errors and data range
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE;
        bool result = psVectorStats(myStats, myVector, myErrors, NULL, 0);
        ok(result, "psVectorStats suceeded (F32, no mask, with errors and data range)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedWeightMeanNoMaskRangeF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedWeightMeanNoMaskRangeF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask=1
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_SAMPLE_MEAN;
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats suceeded (F32, with mask, no errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanWithMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke psVectorStats with vector mask and error vector
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, myErrors, maskVector, 1);
        ok(result, "psVectorStats suceeded (F32, with mask and errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedWeightMeanWithMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedWeightMeanWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke psVectorStats with vector mask and data range
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE;
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result, "psVectorStats suceeded (F32, with mask and data range)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanRangeWithMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanRangeWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke psVectorStats with vector mask, errors, and data range
    {
        psMemId id = psMemGetId();
        bool result = psVectorStats(myStats, myVector, myErrors, maskVector, 1);
        ok(result, "psVectorStats suceeded (F32, withmask, errors, and data range)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedWeightMeanWithMaskRangeF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedWeightMeanWithMaskRangeF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask=2.
    {
        psMemId id = psMemGetId();
        myStats->options = PS_STAT_SAMPLE_MEAN;
        for (psS32 i = 0; i < N; i++) {
            if (maskVector->data.U8[i] == 1) {
                maskVector->data.U8[i] = 2;
            }
        }
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 2);
        ok(result, "psVectorStats suceeded (F32, with mask = 2, no errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanWithMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanWithMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with vector mask=3.
    {
        psMemId id = psMemGetId();
        for (psS32 i = 0; i < N; i++)
        {
            if (maskVector->data.U8[i] == 2) {
                maskVector->data.U8[i] = 3;
            }
        }
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 4);
        ok(result, "psVectorStats suceeded (F32, with mask = 3, no errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanNoMaskF32, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanNoMaskF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Mask all values and verify return is FALSE
    {
        psMemId id = psMemGetId();
        for(psS32 i = 0; i < N; i++)
        {
            maskVector->data.U8[i] = 1;
        }
        bool result = psVectorStats(myStats, myVector, NULL, maskVector, 1);
        ok(result == true, "psVectorStats() returned TRUE (All values masked)");
        ok(isnan(myStats->sampleMean), "psVectorStats() returned NAN");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call psVectorStats() with NULL inputs.
    {
        psMemId id = psMemGetId();
        ok(!psVectorStats(myStats, NULL, NULL, NULL, 0), "psVectorStats() returned false with NULL inputs");
        ok(!psVectorStats(NULL, myVector, NULL, NULL, 0), "psVectorStats() returned false with NULL inputs");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(myStats);
    psFree(myVector);
    psFree(myErrors);
    psFree(maskVector);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");


    // Test SampleMean: S8
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_S8);
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.S8[i] =  samplesS8[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded (S8, no mask, no errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanNoMaskS8, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanNoMaskS8);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test SampleMean: U16
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_U16);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.U16[i] =  samplesU16[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded (U16, no mask, no errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanNoMaskU16, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanNoMaskU16);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test SampleMean: F64
    {
        psMemId id = psMemGetId();
        psStats* myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psVector* myVector = psVectorAlloc(N, PS_TYPE_F64);
        myVector->n = N;
        for (psS32 i = 0; i < N; i++)
        {
            myVector->data.F64[i] =  samplesF64[i];
        }

        bool result = psVectorStats(myStats, myVector, NULL, NULL, 0);
        ok(result, "psVectorStats suceeded (F64, no mask, no errors)");
        ok(!isnan(myStats->sampleMean), "psVectorStats() returned non-NAN");
        is_float_tol(myStats->sampleMean, expectedMeanNoMaskF64, 1e-4, "The mean was %f, should be %f", myStats->sampleMean, expectedMeanNoMaskF64);
        psFree(myStats);
        psFree(myVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

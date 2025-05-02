/*****************************************************************************
This routine must ensure that the various random number generator functions
work properly.
 
    ensure that psRandom structs are properly allocated by psRandomAllocSpecific().
    ensure that psRandomUniform() produces a sequence of numbers with
        proper mean and stdev.
    ensure that psRandomGaussian() produces a sequence of numbers with
        proper mean and stdev.
    ensure that psRandomPoisson() produces a sequence of numbers with
        proper mean and stdev.
    ensure that psRandomReset() properly seeds the random number
        generator for psRandomUniform().
    ensure that psRandomReset() properly seeds the random number
        generator for psRandomGaussian().
    ensure that psRandomReset() properly seeds the random number
        generator for psRandomPoisson().
 
XXX: I removed a test that generated error output.  How should we test that?
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#define NUM_DATA 10000
#define SEED 54321
#define SEED2 345
#define UNIFORM_MEAN 0.5
#define UNIFORM_STDEV 0.3
#define GAUSSIAN_MEAN 0.0
#define GAUSSIAN_STDEV 1.0
#define POISSON_MEAN 15.0
#define POISSON_STDEV (POISSON_MEAN / 4)
#define ERROR_TOLERANCE 0.2
#define VERBOSE 0
# define is_float_tol_per(VALUE,EXPECT,TOL,COMMENT, ...)\
ok((fabs((VALUE)-(EXPECT)) < (TOL)), COMMENT, ## __VA_ARGS__);

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(34);

    // ensure that psRandom structs are properly allocated by psRandomAllocSpecific()
    {
        psMemId id = psMemGetId();
        // Valid type allocation
        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 1, "Skipping tests because psRandomAllocSpecific() failed");
        ok(myRNG->type == PS_RANDOM_TAUS, "psRandomAllocSpecific() set type properly");
        psFree(myRNG);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Unallowed type allocation
    {
        psMemId id = psMemGetId();
        psRandom *myRNG = psRandomAllocSpecific(100,SEED);
        ok(myRNG == NULL, "psRandomAllocSpecific() refused to generate psRandom with unallowed type");
        psFree(myRNG);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Negative seed value
    {
        psMemId id = psMemGetId();
        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS,-5);
        ok(myRNG != NULL, "psRandomAllocSpecific() allows negative seed");
        psFree(myRNG);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testRandomUniform(void)
    {
        // testRandomUniform()
        psMemId id = psMemGetId();
        psVector *rans = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans->n = rans->nalloc;
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 2, "Skipping tests because psRandomAllocSpecific() failed");

        // Initialize vector data with random number
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans->data.F64[i] = psRandomUniform(myRNG);
        }

        // Perform vector stats on random data (mean, stdev)
        psVectorStats(stats, rans, NULL, NULL, 0);
        stats->options = PS_STAT_SAMPLE_STDEV;
        psVectorStats(stats, rans, NULL, NULL, 0);

        // Verify mean and stdev
        is_float_tol_per(stats->sampleMean, UNIFORM_MEAN, ERROR_TOLERANCE, "Mean is within expected range");
        is_float_tol_per(stats->sampleStdev, UNIFORM_STDEV, ERROR_TOLERANCE, "StDev is within expected range");

        skip_end();

        psFree(myRNG);
        psFree(rans);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Ensure psRandomUniform() returns 0 for NULL psRandom struct
    {
        psMemId id = psMemGetId();
        ok(psRandomUniform(NULL) == 0, "Ensure psRandomUniform() returns 0 for NULL psRandom struct");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testRandomGaussian(void)
    {
        diag("testRandomGaussian()");
        psMemId id = psMemGetId();
        psVector *rans = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans->n = rans->nalloc;
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 2, "Skipping tests because psRandomAllocSpecific() failed");

        // Initialize vector with random data
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans->data.F64[i] = psRandomGaussian(myRNG);
        }

        // Perform vector stats on data (mean, stdev)
        psVectorStats(stats, rans, NULL, NULL, 0);
        stats->options = PS_STAT_SAMPLE_STDEV;
        psVectorStats(stats, rans, NULL, NULL, 0);

        // Verify mean and stdev
        is_float_tol_per(stats->sampleMean, GAUSSIAN_MEAN, ERROR_TOLERANCE, "Mean is within expected range");
        is_float_tol_per(stats->sampleStdev, GAUSSIAN_STDEV, ERROR_TOLERANCE, "StDev is within expected range");

        skip_end();

        psFree(myRNG);
        psFree(rans);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Ensure psRandomGaussian() returns 0 for NULL psRandom struct
    {
        psMemId id = psMemGetId();
        ok(psRandomGaussian(NULL) == 0, "Ensure psRandomGaussian() returns 0 for NULL psRandom struct");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testRandomPoisson(void)
    {
        diag("testRandomPoisson()");
        psMemId id = psMemGetId();
        psVector *rans = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans->n = rans->nalloc;
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 2, "Skipping tests because psRandomAllocSpecific() failed");

        // Initialize vector with random data
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans->data.F64[i] = psRandomPoisson(myRNG, POISSON_MEAN);
        }

        // Perform vector stats on random data (mean, stdev)
        psVectorStats(stats, rans, NULL, NULL, 0);
        stats->options = PS_STAT_SAMPLE_STDEV;
        psVectorStats(stats, rans, NULL, NULL, 0);
        // Verify mean and stdev
        is_float_tol_per(stats->sampleMean, POISSON_MEAN, ERROR_TOLERANCE, "Mean is within expected range");
        is_float_tol_per(stats->sampleStdev, POISSON_STDEV, ERROR_TOLERANCE, "StDev is within expected range");

        skip_end();

        psFree(myRNG);
        psFree(rans);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Ensure psRandomPoisson() returns 0 for NULL psRandom struct
    {
        psMemId id = psMemGetId();
        ok(psRandomPoisson(NULL, POISSON_MEAN) == 0, "Ensure psRandomPoisson() returns 0 for NULL psRandom struct");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testRandomResetUniform(void)
    {
        diag("testRandomUniform(): ensure the seed resets properly");
        psMemId id = psMemGetId();
        psVector *rans00 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans00->n = rans00->nalloc;
        psVector *rans01 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans01->n = rans01->nalloc;
        psVector *rans02 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans02->n = rans02->nalloc;

        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 1, "Skipping tests because psRandomAllocSpecific() failed");


        // Random reset
        psRandomReset(myRNG, SEED);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans00->data.F64[i] = psRandomUniform(myRNG);
        }
        psRandomReset(myRNG, SEED2);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans01->data.F64[i] = psRandomUniform(myRNG);
        }
        psRandomReset(myRNG, SEED);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans02->data.F64[i] = psRandomUniform(myRNG);
        }

        // Verify reset to original seed produces same results
        psBool errorFlag = false;
        for (psS32 i = 0 ; i < NUM_DATA ; i++)
        {
            if (rans00->data.F64[i] != rans02->data.F64[i]) {
                if (VERBOSE) {
                    psError(PS_ERR_UNKNOWN,true,"psRandomUniform did not produce the same results with the same seed");
                }
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psRandomUniform() produced the same results with the same seed");
        skip_end();
        psFree(myRNG);
        psFree(rans00);
        psFree(rans01);
        psFree(rans02);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // XXX: How to test the proper generation of error messages?
    if (0) {
        psRandom *myRNG1 = NULL;
        myRNG1 = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        //    psLogSetDestination("dest:stderr");
        psLogSetDestination(0);
        psRandomReset(myRNG1,0);
        //    psLogSetDestination("dest:stderr");
        psLogSetDestination(2);
        psFree(myRNG1);

        // Reset a NULL psRandom variable, should generate an error message
        psRandomReset(NULL,SEED);
    }

    // testRandomResetGaussian(void)
    {
        // testRandomGaussian(): ensure the seed resets properly
        psMemId id = psMemGetId();
        psVector *rans00 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans00->n = rans00->nalloc;
        psVector *rans01 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans01->n = rans01->nalloc;
        psVector *rans02 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans02->n = rans02->nalloc;

        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 1, "Skipping tests because psRandomAllocSpecific() failed");


        // Initialize random data in vectors
        psRandomReset(myRNG, SEED);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans00->data.F64[i] = psRandomGaussian(myRNG);
        }
        psRandomReset(myRNG, SEED2);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans01->data.F64[i] = psRandomGaussian(myRNG);
        }
        psRandomReset(myRNG, SEED);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans02->data.F64[i] = psRandomGaussian(myRNG);
        }

        // Verify data from original seed produces same data after reset
        psBool errorFlag = false;
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            if (rans00->data.F64[i] != rans02->data.F64[i]) {
                if (VERBOSE) {
                    psError(PS_ERR_UNKNOWN,true,"psRandomGaussian did not produce the same results with the same seed");
                }
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psRandomGaussian() produced the same results with the same seed");
        skip_end();
        psFree(myRNG);
        psFree(rans00);
        psFree(rans01);
        psFree(rans02);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testRandomResetPoisson(void)
    {
        diag("testRandomPoisson(): ensure the seed resets properly");
        psMemId id = psMemGetId();
        psVector *rans00 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans00->n = rans00->nalloc;
        psVector *rans01 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans01->n = rans01->nalloc;
        psVector *rans02 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
        rans02->n = rans02->nalloc;

        psRandom *myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        ok(myRNG != NULL, "psRandom struct was allocated properly");
        skip_start(myRNG == NULL, 1, "Skipping tests because psRandomAllocSpecific() failed");

        // Initialize vectors with random data
        psRandomReset(myRNG, SEED);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans00->data.F64[i] = psRandomPoisson(myRNG, POISSON_MEAN);
        }
        psRandomReset(myRNG, SEED2);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans01->data.F64[i] = psRandomPoisson(myRNG, POISSON_MEAN);
        }
        psRandomReset(myRNG, SEED);
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            rans02->data.F64[i] = psRandomPoisson(myRNG, POISSON_MEAN);
        }

        // Verify the original seed produces same data after reset
        psBool errorFlag = false;
        for (psS32 i = 0 ; i < NUM_DATA ; i++) {
            if (rans00->data.F64[i] != rans02->data.F64[i]) {
                if (VERBOSE) {
                    psError(PS_ERR_UNKNOWN,true,"psRandomPoisson did not produce the same results with the same seed");
                }
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psRandomPoisson() produced the same results with the same seed");
        skip_end();
        psFree(myRNG);
        psFree(rans00);
        psFree(rans01);
        psFree(rans02);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

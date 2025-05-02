/*****************************************************************************
This routine must ensure that the various random number generator functions
work properly.
 
    t00(): ensure that psRandom structs are properly allocated by psRandomAllocSpecific().
    t01(): ensure that psRandomUniform() produces a sequence of numbers with
    proper mean and stdev.
    t02(): ensure that psRandomGaussian() produces a sequence of numbers with
    proper mean and stdev.
    t03(): ensure that psRandomPoisson() produces a sequence of numbers with
    proper mean and stdev.
    t04(): ensure that psRandomReset() properly seeds the random number
    generator for psRandomUniform().
    t05(): ensure that psRandomReset() properly seeds the random number
    generator for psRandomGaussian().
    t06(): ensure that psRandomReset() properly seeds the random number
    generator for psRandomPoisson().
 *****************************************************************************/
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include "pslib_strict.h"
#include "psTest.h"

#define NUM_DATA 10000
#define SEED 54321
#define SEED2 345
#define UNIFORM_MEAN 0.5
#define UNIFORM_STDEV 0.3
#define GAUSSIAN_MEAN 0.0
#define GAUSSIAN_STDEV 1.0
#define POISSON_MEAN 15.0
#define POISSON_STDEV (POISSON_MEAN / 4)
#define ERROR_TOLERANCE 0.1
psS32 testStatus = true;

static psS32 testRandomAlloc(void);
static psS32 testRandomUniform(void);
static psS32 testRandomGaussian(void);
static psS32 testRandomPoisson(void);
static psS32 testRandomResetUniform(void);
static psS32 testRandomResetGaussian(void);
static psS32 testRandomResetPoisson(void);

testDescription tests[] = {
                              {testRandomAlloc,000,"psRandomAllocSpecific",0,false},
                              {testRandomUniform,000,"psRandomUniform",0,false},
                              {testRandomGaussian,000,"psRandomGaussian",0,false},
                              {testRandomPoisson,000,"psRandomPoisson",0,false},
                              {testRandomResetUniform,000,"psRandomReset",0,false},
                              {testRandomResetGaussian,000,"psRandomReset",0,false},
                              {testRandomResetPoisson,000,"psRandomReset",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    if(!runTestSuite(stderr,"psRandom",tests,argc,argv)) {
        return 1;
    }

    return 0;
}

psS32 testRandomAlloc(void)
{
    psRandom *myRNG = NULL;

    // Valid type allocation
    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }
    if (myRNG->type != PS_RANDOM_TAUS) {
        psError(PS_ERR_UNKNOWN,true,"Type %d not as expected %d",myRNG->type,PS_RANDOM_TAUS);
        return 2;
    }
    psFree(myRNG);

    // Valid type allocation with seed equal to zero
    int fd1 = creat("seed_msglog1.txt", 0666);
    //    psLogSetDestination("file:seed_msglog1.txt");
    psLogSetDestination(fd1);
    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
    //    psLogSetDestination("dest:stderr");
    psLogSetDestination(2);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 3;
    }
    if (myRNG->type != PS_RANDOM_TAUS) {
        psError(PS_ERR_UNKNOWN,true,"Type %d not as expected %d",myRNG->type,PS_RANDOM_TAUS);
        return 4;
    }
    psFree(myRNG);

    // Invalid type allocation
    psLogMsg(__func__,PS_LOG_INFO,"Invalid type, should generate error message");
    myRNG = psRandomAllocSpecific(100,SEED);
    if (myRNG != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NULL for invalid type");
        return 5;
    }

    // Negative seed value
    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS,-5);
    if(myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return allocated psRandom");
        return 6;
    }
    close(fd1);
    psFree(myRNG);

    return 0;
}

psS32 testRandomUniform(void)
{
    psRandom *myRNG = NULL;
    psVector *rans = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans->n = rans->nalloc;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);

    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }

    // Initialize vector data with random number
    for (psS32 i = 0 ; i < NUM_DATA ; i++) {
        rans->data.F64[i] = psRandomUniform(myRNG);
    }
    // Perform vector stats on random data (mean, stdev)
    stats = psVectorStats(stats, rans, NULL, NULL, 0);
    stats->options = PS_STAT_SAMPLE_STDEV;
    stats = psVectorStats(stats, rans, NULL, NULL, 0);
    // Verify mean and stdev
    if ((fabs(stats->sampleMean - UNIFORM_MEAN) / UNIFORM_MEAN) > ERROR_TOLERANCE) {
        psError(PS_ERR_UNKNOWN,true,"psRandomUniform mean is %.2f, should be %.2f",
                stats->sampleMean, UNIFORM_MEAN);
        return 2;
    }
    if ((fabs(stats->sampleStdev - UNIFORM_STDEV) / UNIFORM_STDEV) > ERROR_TOLERANCE) {
        psError(PS_ERR_UNKNOWN,true,"psRandomUniform stdev is %.2f, should be %.2f",
                stats->sampleStdev, UNIFORM_STDEV);
        return 3;
    }

    psFree(myRNG);
    psFree(rans);
    psFree(stats);

    psLogMsg(__func__,PS_LOG_INFO,"NULL psRandom variable, should generate error message");
    if(psRandomUniform(NULL) != 0) {
        psError(PS_ERR_UNKNOWN,true,"Did not return zero for null psRandom");
        return 4;
    }

    return 0;
}

psS32 testRandomGaussian(void)
{
    psRandom *myRNG = NULL;
    psVector *rans = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans->n = rans->nalloc;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);

    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }

    // Initialize vector with random data
    for (psS32 i = 0 ; i < NUM_DATA ; i++) {
        rans->data.F64[i] = psRandomGaussian(myRNG);
    }

    // Perform vector stats on data (mean, stdev)
    stats = psVectorStats(stats, rans, NULL, NULL, 0);
    stats->options = PS_STAT_SAMPLE_STDEV;
    stats = psVectorStats(stats, rans, NULL, NULL, 0);

    // Verify statistics
    if ((fabs(stats->sampleMean - GAUSSIAN_MEAN) / 1.0) > ERROR_TOLERANCE) {
        psError(PS_ERR_UNKNOWN,true,"psRandomGaussian mean is %.2f, should be %.2f",
                stats->sampleMean, GAUSSIAN_MEAN);
        return 2;
    }
    if ((fabs(stats->sampleStdev - GAUSSIAN_STDEV) / GAUSSIAN_STDEV) > ERROR_TOLERANCE) {
        psError(PS_ERR_UNKNOWN,true,"psRandomGaussian stdev is %.2f, should be %.2f",
                stats->sampleStdev, GAUSSIAN_STDEV);
        return 3;
    }

    psFree(myRNG);
    psFree(rans);
    psFree(stats);

    psLogMsg(__func__,PS_LOG_INFO,"NULL psRandom variable, should generate error message");
    if(psRandomGaussian(NULL) != 0) {
        psError(PS_ERR_UNKNOWN,true,"Did not return zero for null psRandom");
        return 4;
    }

    return 0;
}

psS32 testRandomPoisson(void)
{
    psRandom *myRNG = NULL;
    psVector *rans = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans->n = rans->nalloc;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);

    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }

    // Initialize vector with random data
    for (psS32 i = 0 ; i < NUM_DATA ; i++) {
        rans->data.F64[i] = psRandomPoisson(myRNG, POISSON_MEAN);
    }

    // Perform statistics on data
    stats = psVectorStats(stats, rans, NULL, NULL, 0);
    stats->options = PS_STAT_SAMPLE_STDEV;
    stats = psVectorStats(stats, rans, NULL, NULL, 0);
    if ((fabs(stats->sampleMean - POISSON_MEAN) / POISSON_MEAN) > ERROR_TOLERANCE) {
        psError(PS_ERR_UNKNOWN,true,"psRandomPoisson mean is %.2f, should be %.2f",
                stats->sampleMean, POISSON_MEAN);
        return 2;
    }
    if ((fabs(stats->sampleStdev - POISSON_STDEV) / POISSON_STDEV) > ERROR_TOLERANCE) {
        psError(PS_ERR_UNKNOWN,true,"psRandomPoisson stdev is %.2f, should be %.2f",
                stats->sampleStdev, POISSON_STDEV);
        return 3;
    }

    psFree(myRNG);
    psFree(rans);
    psFree(stats);

    psLogMsg(__func__,PS_LOG_INFO,"NULL psRandom variable, should generate error message");
    if(psRandomPoisson(NULL, POISSON_MEAN) != 0) {
        psError(PS_ERR_UNKNOWN,true,"Did not return zero for null psRandom");
        return 4;
    }

    return 0;
}

psS32 testRandomResetUniform(void)
{
    psRandom *myRNG = NULL;
    psVector *rans00 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans00->n = rans00->nalloc;
    psVector *rans01 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans01->n = rans01->nalloc;
    psVector *rans02 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans02->n = rans02->nalloc;

    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }

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
    for (psS32 i = 0 ; i < NUM_DATA ; i++) {
        if (rans00->data.F64[i] != rans02->data.F64[i]) {
            psError(PS_ERR_UNKNOWN,true,"psRandomUniform did not produce the same results with the same seed");
            return i+1;
        }
    }

    psFree(myRNG);
    psFree(rans00);
    psFree(rans01);
    psFree(rans02);

    psRandom *myRNG1 = NULL;
    myRNG1 = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    //    psLogSetDestination("dest:stderr");
    psLogSetDestination(0);
    psRandomReset(myRNG1,0);
    //    psLogSetDestination("dest:stderr");
    psLogSetDestination(2);
    psFree(myRNG1);

    psLogMsg(__func__,PS_LOG_INFO,"Reset a NULL psRandom variable, should generate an error message");
    psRandomReset(NULL,SEED);

    return 0;
}

psS32 testRandomResetGaussian(void)
{
    psRandom *myRNG = NULL;
    psVector *rans00 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans00->n = rans00->nalloc;
    psVector *rans01 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans01->n = rans01->nalloc;
    psVector *rans02 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans02->n = rans02->nalloc;

    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }

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
    for (psS32 i = 0 ; i < NUM_DATA ; i++) {
        if (rans00->data.F64[i] != rans02->data.F64[i]) {
            psError(PS_ERR_UNKNOWN,true,"psRandomGaussian did not produce the same results with the same seed");
            return 2;
        }
    }

    psFree(myRNG);
    psFree(rans00);
    psFree(rans01);
    psFree(rans02);

    return 0;
}

psS32 testRandomResetPoisson(void)
{
    psRandom *myRNG = NULL;
    psVector *rans00 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans00->n = rans00->nalloc;
    psVector *rans01 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans01->n = rans01->nalloc;
    psVector *rans02 = psVectorAlloc(NUM_DATA, PS_TYPE_F64);
    rans02->n = rans02->nalloc;

    myRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
    if (myRNG == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Could not allocate psRandom structure");
        return 1;
    }

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
    for (psS32 i = 0 ; i < NUM_DATA ; i++) {
        if (rans00->data.F64[i] != rans02->data.F64[i]) {
            psError(PS_ERR_UNKNOWN,true,"psRandomPoisson did not produce the same results with the same seed");
            return 2;
        }
    }

    psFree(myRNG);
    psFree(rans00);
    psFree(rans01);
    psFree(rans02);

    return 0;
}


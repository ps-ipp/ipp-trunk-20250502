/** @file  tst_psStats00.c
*
*  @brief Contains tests for psVectorStats with sample mean calculations
*
*  We extensively test the code with data type PS_TYPE_F32.  If these pass, we
*  do a much simpler test with data type PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_F64.
*
*  If the psStats,c code ever changes such that vectors of different type
*  are handled by different routines, then these tests must be extended.
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.8 $  $Name: not supported by cvs2svn $
*  @date $Date: 2006-07-28 00:44:05 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*/

#include "pslib_strict.h"
#include "psTest.h"

#define ERROR_TOL  0.0001
#define N 15
#define VERBOSE 1
static psS32 testStatsSampleMeanF32(void);
static psS32 testStatsSampleMeanS8(void);
static psS32 testStatsSampleMeanU16(void);
static psS32 testStatsSampleMeanF64(void);

testDescription tests[] = {
                              {testStatsSampleMeanF32, 512, "psVectorStats",0,false},
                              {testStatsSampleMeanS8, 512, "psVectorStats",0,false},
                              {testStatsSampleMeanU16, 512, "psVectorStats",0,false},
                              {testStatsSampleMeanF64, 512, "psVectorStats",0,false},
                              {NULL}
                          };

static psF32 samplesF32[N] = { 1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
                               11.01, -12.02, 13.03, 14.04, -15.05 };
static psS8  samplesS8[N]  = {1, 2, -3, 4, 5, -6, 7, 8, -9, 10, 11, -12, 13, 14, -15};
static psU16 samplesU16[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static psF64 samplesF64[N] = { 1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
                               11.01, -12.02, 13.03, 14.04, -15.05 };
static psF32 errorsF32[N] = { -0.10,  0.11, -0.12,  0.13, -0.14,  0.15, -0.16,  0.17,
                              -0.18,  0.19, -0.20,  0.21, -0.22,  0.23, -0.24 };

static psF64 expectedMeanNoMaskF32              =  2.060667;
static psF64 expectedMeanWithMaskF32            =  2.123846;
static psF64 expectedMeanNoMaskS8               =  2.000000;
static psF64 expectedMeanNoMaskU16              =  8.000000;
static psF64 expectedMeanNoMaskF64              =  2.060667;
static psF64 expectedMeanRangeNoMaskF32         =  0.137500;
static psF64 expectedMeanRangeWithMaskF32       = -0.366667;
static psF64 expectedWeightMeanNoMaskF32        =  1.807210;
static psF64 expectedWeightMeanWithMaskF32      =  1.890217;
static psF64 expectedWeightMeanNoMaskRangeF32   =  0.640952;
static psF64 expectedWeightMeanWithMaskRangeF32 =  0.046574;

psF64 rtc(void);
psF64 sT, eT;
psF64 diff;

#include <unistd.h>
psS32 main(psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("p_psVectorSampleMean", 10);
    psTraceSetLevel("p_psVectorMax", 0);
    psTraceSetLevel("p_psVectorMin", 0);
    psTraceSetLevel("p_psVectorNValues", 0);
    psTraceSetLevel("p_psNormalizeVectorRange", 0);
    psTraceSetLevel("psStatsAlloc", 0);
    psTraceSetLevel("p_psConvertToF32", 0);
    psTraceSetLevel("psVectorStats", 0);

    return ( ! runTestSuite(stderr, "psVectorStats",tests,argc,argv) );
}

psS32 testStatsSampleMeanF32(void)
{
    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psVector *myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    psVector *myErrors = psVectorAlloc(N, PS_TYPE_F32);
    myErrors->n = N;
    psVector *maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;

    psF64 mean = 0.0;
    // Set the appropriate values for the vector data.
    for (long i = 0; i < N; i++) {
        myVector->data.F32[i] =  samplesF32[i];
        myErrors->data.F32[i] =  errorsF32[i];
    }

    // Set the mask vector and calculate the expected maximum.
    for (psS32 i = 0; i < N; i++) {

        if (i > 1) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanNoMaskF32);
    }
    if (isnan(myStats->sampleMean) || (fabs(mean - expectedMeanNoMaskF32) > ERROR_TOL)) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedMeanNoMaskF32);
        return 1;
    }

    // Invoke psVectorStats with no vector mask and error vector
    myStats = psVectorStats(myStats, myVector, myErrors, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedWeightMeanNoMaskF32);
    }
    if (isnan(myStats->sampleMean) || (fabs(mean - expectedWeightMeanNoMaskF32) > ERROR_TOL)) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedWeightMeanNoMaskF32);
        return 10;
    }

    // Invoke psVectorStats with no vector mask and data range
    myStats->min = -10.0;
    myStats->max =   8.0;
    myStats->options = PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE;
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanRangeNoMaskF32);
    }
    if ( fabs(mean - expectedMeanRangeNoMaskF32) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Return value %f not as expected %f",
                mean, expectedMeanRangeNoMaskF32);
        return 2;
    }

    // Invoke psVectorStats with no vector mask, errors and data range
    myStats = psVectorStats(myStats, myVector, myErrors, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedWeightMeanNoMaskRangeF32);
    }
    if ( fabs(mean - expectedWeightMeanNoMaskRangeF32) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Return value %f not as expected %f",
                mean, expectedWeightMeanNoMaskRangeF32);
        return 20;
    }
    myStats->options = PS_STAT_SAMPLE_MEAN;

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask=1.                             */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    mean = myStats->sampleMean;
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanWithMaskF32);
    }
    if ( fabs(mean - expectedMeanWithMaskF32) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedMeanWithMaskF32);
        return 3;
    }

    // Invoke psVectorStats with vector mask and error vector
    myStats = psVectorStats(myStats, myVector, myErrors, maskVector, 1);
    mean = myStats->sampleMean;
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedWeightMeanWithMaskF32);
    }
    if ( fabs(mean - expectedWeightMeanWithMaskF32) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedWeightMeanWithMaskF32);
        return 30;
    }

    // Invoke psVectorStats with vector mask and data range
    myStats->options = PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE;
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanRangeWithMaskF32);
    }
    if ( fabs(mean - expectedMeanRangeWithMaskF32) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Return value %f not as expected %f",
                mean, expectedMeanRangeWithMaskF32);
        return 4;
    }

    // Invoke psVectorStats with vector mask, errors, and data range
    myStats = psVectorStats(myStats, myVector, myErrors, maskVector, 1);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedWeightMeanWithMaskRangeF32);
    }
    if ( fabs(mean - expectedWeightMeanWithMaskRangeF32) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Return value %f not as expected %f",
                mean, expectedWeightMeanWithMaskRangeF32);
        return 40;
    }
    myStats->options = PS_STAT_SAMPLE_MEAN;

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask=2.                             */
    /*************************************************************************/
    // Set the mask vector and calculate the expected maximum.
    // Set the mask vector.
    for (psS32 i = 0; i < N; i++) {
        if (maskVector->data.U8[i] == 1) {
            maskVector->data.U8[i] = 2;
        }
    }
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 2);
    mean = myStats->sampleMean;
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanWithMaskF32);
    }
    if (fabs(mean - expectedMeanWithMaskF32) > ERROR_TOL )  {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean,expectedMeanWithMaskF32);
        return 5;
    }

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask=3.                             */
    /*************************************************************************/
    // Set the mask vector and calculate the expected maximum.
    // Set the mask vector.
    for (psS32 i = 0; i < N; i++) {
        if (maskVector->data.U8[i] == 2) {
            maskVector->data.U8[i] = 3;
        }
    }
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 4);
    mean = myStats->sampleMean;
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanNoMaskF32);
    }
    if (fabs(mean - expectedMeanNoMaskF32) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Return value %f not as expected %f",
                mean,expectedMeanNoMaskF32);
        return 6;
    }

    // Mask all values and verify return is NAN
    for(psS32 i = 0; i < N; i++) {
        maskVector->data.U8[i] = 1;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate warning message");
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    mean = myStats->sampleMean;
    if( !isnan(mean) ) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NAN with all values masked");
        return 7;
    }

    /*************************************************************************/
    /*  Call psVectorStats() with NULL inputs.                               */
    /*************************************************************************/
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message.");
    if( psVectorStats(myStats, NULL, NULL, NULL, 0) != NULL ) {
        psError(PS_ERR_UNKNOWN,true,"psVectorStats did not return NULL");
        return 8;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message.");
    psStats *myStats2 = psVectorStats(NULL, myVector, NULL, NULL, 0);
    if ( myStats2 != NULL ) {
        psError(PS_ERR_UNKNOWN,true,"psVectorStats did not return NULL");
        return 9;
    }

    /*************************************************************************/
    /*  Deallocate data structures                                           */
    /*************************************************************************/
    psFree(myStats);
    psFree(myVector);
    psFree(myErrors);
    psFree(maskVector);
    psFree(myStats2);

    return 0;
}

psS32 testStatsSampleMeanS8(void)
{
    psStats*  myStats  = NULL;
    psVector* myVector = NULL;
    psF64     mean     = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    myVector = psVectorAlloc(N, PS_TYPE_S8);
    myVector->n = N;

    mean = 0.0;
    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.S8[i] =  samplesS8[i];
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanNoMaskS8);
    }
    if ( fabs(mean - expectedMeanNoMaskS8) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedMeanNoMaskS8);
        return 1;
    }

    /*************************************************************************/
    /*  Deallocate data structures                                           */
    /*************************************************************************/
    psFree(myStats);
    psFree(myVector);

    return 0;
}

psS32 testStatsSampleMeanU16(void)
{
    psStats*  myStats  = NULL;
    psVector* myVector = NULL;
    psF64     mean     = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    myVector = psVectorAlloc(N, PS_TYPE_U16);
    myVector->n = N;

    mean = 0.0;
    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.U16[i] =  samplesU16[i];
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanNoMaskU16);
    }
    if ( fabs(mean - expectedMeanNoMaskU16) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedMeanNoMaskU16);
        return 1;
    }

    /*************************************************************************/
    /*  Deallocate data structures                                           */
    /*************************************************************************/
    psFree(myStats);
    psFree(myVector);

    return 0;
}

psS32 testStatsSampleMeanF64(void)
{
    psStats*  myStats  = NULL;
    psVector* myVector = NULL;
    psF64     mean     = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    myVector = psVectorAlloc(N, PS_TYPE_F64);
    myVector->n = N;

    mean = 0.0;
    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.F64[i] =  samplesF64[i];
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    mean = myStats->sampleMean;
    // Verify return value is as expected
    if (VERBOSE) {
        printf("psVectorStats() returned %.2f: expected was %.2f\n", mean, expectedMeanNoMaskF64);
    }
    if ( fabs(mean - expectedMeanNoMaskF64) > ERROR_TOL ) {
        psError(PS_ERR_UNKNOWN,true,"Returned value %f not as expected %f",
                mean, expectedMeanNoMaskF64);
        return 1;
    }

    /*************************************************************************/
    /*  Deallocate data structures                                           */
    /*************************************************************************/
    psFree(myStats);
    psFree(myVector);

    return 0;
}


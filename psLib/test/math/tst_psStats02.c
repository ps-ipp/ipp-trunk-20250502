/** @file  tst_psStats02.c
*
*  @brief Contains tests for psVectorStats with min calculations
*
*  We extensively test the code with data type PS_TYPE_F32.  If these pass, we
*  do a much simpler test with data types PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_F64.
*
*  If the psStats,c code every changes such that vectors of different type
*  are handled by different routines, then these tests must be extended.
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.4 $  $Name: not supported by cvs2svn $
*  @date $Date: 2006-07-28 00:44:05 $
*
* Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*/

#include "pslib_strict.h"
#include "psTest.h"

#define N 15
#define ERROR_TOL  0.0001

static psS32 testStatsMinF32(void);
static psS32 testStatsMinS8(void);
static psS32 testStatsMinU16(void);
static psS32 testStatsMinF64(void);

testDescription tests[] = {
                              {testStatsMinF32, 518, "psVectorStats", 0, false},
                              {testStatsMinS8, 518, "psVectorStats", 0, false},
                              {testStatsMinU16, 518, "psVectorStats", 0, false},
                              {testStatsMinF64, 518, "psVectorStats", 0, false},
                              {NULL}
                          };

static psF32 samplesF32[N] = { 1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
                               11.01, -12.02, 13.03, 14.04, -15.05 };
static psS8  samplesS8[N]  = {1, 2, -3, 4, 5, -6, 7, 8, -9, 10, 11, -12, 13, 14, -15};
static psU16 samplesU16[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static psF64 samplesF64[N] = { 1.1, 2.2, -3.3, 4.4, 5.5, -6.6, 7.7, 8.8, -9.9, 10.0,
                               11.01, -12.02, 13.03, 14.04, -15.05 };

static psF64 expectedMinNoMaskF32                = -15.05;
static psF64 expectedMinNoMaskS8                 = -15.00;
static psF64 expectedMinNoMaskU16                = 1.00;
static psF64 expectedMinNoMaskF64                = -15.05;

static psF64 expectedMinWithMaskF32              = -12.02;
static psF64 expectedMinRangeNoMaskF32           =   1.10;
static psF64 expectedMinRangeWithMaskF32         = -12.02;

psS32 main(psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    //
    // We list pertinent psStats.c functions here for debugging ease.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("p_psVectorMax", 0);
    psTraceSetLevel("p_psVectorMin", 0);
    psTraceSetLevel("p_psVectorCheckNonEmpty", 0);
    psTraceSetLevel("p_psVectorNValues", 0);
    psTraceSetLevel("p_psNormalizeVectorRange", 0);
    psTraceSetLevel("psStatsAlloc", 0);
    psTraceSetLevel("p_psConvertToF32", 0);
    psTraceSetLevel("psVectorStats", 0);

    return ( ! runTestSuite(stderr, "psVectorStats", tests, argc, argv) );
}

psS32 testStatsMinF32(void)
{
    psStats*  myStats    = NULL;
    psVector* myVector   = NULL;
    psVector* maskVector = NULL;
    psF64     min        = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_MIN);
    myVector = psVectorAlloc(N, PS_TYPE_F32);
    myVector->n = N;
    maskVector = psVectorAlloc(N, PS_TYPE_U8);
    maskVector->n = N;

    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.F32[i] = samplesF32[i];
    }

    // Set the mask vector and calculate the expected maximum.
    for (psS32 i = 0; i < N; i++) {
        if (i < 13) {
            maskVector->data.U8[i] = 0;
        } else {
            maskVector->data.U8[i] = 1;
        }
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    min = myStats->min;

    if (fabs(min - expectedMinNoMaskF32) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min no mask return value %lf not as expected %lf",
                min, expectedMinNoMaskF32);
        return 1;
    }

    /*************************************************************************/
    /*  Call psVectorStats() with vector mask.                       */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    min = myStats->min;
    if (fabs(min - expectedMinWithMaskF32) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min with mask return value %lf not as expected %lf",
                min, expectedMinWithMaskF32);
        return 2;
    }

    // Invoke function with data range with no mask
    myStats->options = PS_STAT_MIN | PS_STAT_USE_RANGE;
    myStats->max = 10.1;
    myStats->min = 0.0;
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    min = myStats->min;

    if(fabs(min - expectedMinRangeNoMaskF32) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min with range no mask %lf not as expected %lf",
                min, expectedMinRangeNoMaskF32);
        return 3;
    }

    // Invoke function with data range and mask
    myStats->max = 10.1;
    myStats->min = -15.00;
    myStats = psVectorStats(myStats, myVector, NULL, maskVector, 1);
    min = myStats->min;
    if(fabs(min - expectedMinRangeWithMaskF32) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min with range with mask %lf not as expected %lf",
                min, expectedMinRangeWithMaskF32);
        return 3;
    }

    // Invoke function with data range with no valid data
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    myStats->max = 100.00;
    myStats->min = 90.00;
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    min = myStats->min;

    if(!isnan(min)) {
        psError(PS_ERR_UNKNOWN,true,"Min with range with no valid elemenets did not return NAN");
        return 4;
    }

    psFree(myStats);
    psFree(myVector);
    psFree(maskVector);

    return 0;
}

psS32 testStatsMinS8(void)
{
    psStats*  myStats    = NULL;
    psVector* myVector   = NULL;
    psF64     min        = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_MIN);
    myVector = psVectorAlloc(N, PS_TYPE_S8);
    myVector->n = N;

    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.S8[i] = samplesS8[i];
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    min = myStats->min;

    if (fabs(min - expectedMinNoMaskS8) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min no mask return value %lf not as expected %lf",
                min, expectedMinNoMaskS8);
        return 1;
    }

    psFree(myStats);
    psFree(myVector);

    return 0;
}

psS32 testStatsMinU16(void)
{
    psStats*  myStats    = NULL;
    psVector* myVector   = NULL;
    psF64     min        = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_MAX);
    myVector = psVectorAlloc(N, PS_TYPE_U16);
    myVector->n = N;

    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.U16[i] = samplesU16[i];
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    min = myStats->min;

    if (fabs(min - expectedMinNoMaskU16) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min no mask return value %lf not as expected %lf",
                min, expectedMinNoMaskU16);
        return 1;
    }

    psFree(myStats);
    psFree(myVector);

    return 0;
}

psS32 testStatsMinF64(void)
{
    psStats*  myStats    = NULL;
    psVector* myVector   = NULL;
    psF64     min        = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    myStats = psStatsAlloc(PS_STAT_MIN);
    myVector = psVectorAlloc(N, PS_TYPE_F64);
    myVector->n = N;

    // Set the appropriate values for the vector data.
    for (psS32 i = 0; i < N; i++) {
        myVector->data.F64[i] = samplesF64[i];
    }

    /*************************************************************************/
    /*  Call psVectorStats() with no vector mask.                    */
    /*************************************************************************/
    myStats = psVectorStats(myStats, myVector, NULL, NULL, 0);
    min = myStats->min;

    if (fabs(min - expectedMinNoMaskF64) > ERROR_TOL) {
        psError(PS_ERR_UNKNOWN,true,"Min no mask return value %lf not as expected %lf",
                min, expectedMinNoMaskF64);
        return 1;
    }

    psFree(myStats);
    psFree(myVector);

    return 0;
}


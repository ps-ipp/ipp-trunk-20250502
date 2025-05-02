#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
        pmSourceLocalSky(): needs more thorough testing with acceptable input params.
        pmSourceLocalSkyVariance(): needs more thorough testing with acceptable input params.
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (8)
#define TEST_NUM_COLS           (16)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(26);


    // ----------------------------------------------------------------------
    // pmSourceLocalSky() tests
    // Call pmSourceLocalSky() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->pixels->numRows ; i++) {
            for (int j = 0 ; j < src->pixels->numCols ; j++) {
                src->pixels->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSky(NULL, PS_STAT_SAMPLE_MEAN, 10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSky() returned FALSE with NULL pmSource input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSky() with NULL pmSource->pixels input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSky(src, PS_STAT_SAMPLE_MEAN, 10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSky() returned FALSE with NULL pmSource->pixels input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSky() with NULL pmSource->peak input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->pixels->numRows ; i++) {
            for (int j = 0 ; j < src->pixels->numCols ; j++) {
                src->pixels->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        bool rc = pmSourceLocalSky(src, PS_STAT_SAMPLE_MEAN, 10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSky() returned FALSE with NULL pmSource->peak input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSky() with NULL pmSource->maskObj input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->pixels->numRows ; i++) {
            for (int j = 0 ; j < src->pixels->numCols ; j++) {
                src->pixels->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSky(src, PS_STAT_SAMPLE_MEAN, -10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSky() returned FALSE with NULL pmSource->maskObj input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSky() with negative input radius
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->pixels->numRows ; i++) {
            for (int j = 0 ; j < src->pixels->numCols ; j++) {
                src->pixels->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSky(src, PS_STAT_SAMPLE_MEAN, -10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSky() returned FALSE with negative input radius");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    
    // Call pmSourceLocalSky() with acceptable input parameters
    // XX: Future Improvements:
    //     Test more PS_STATS types
    //     Test more psRegion values (region bigger than image, 0 region, etc.)
    //     Test mask values
    //
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        psF32 mean = 0.0;
        for (int i = 0 ; i < src->pixels->numRows ; i++) {
            for (int j = 0 ; j < src->pixels->numCols ; j++) {
                src->pixels->data.F32[i][j] = (float) (i + j);
                mean+= (float) (i + j);
                src->maskObj->data.U8[i][j] = 0;
	    }
	}
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSky(src, PS_STAT_SAMPLE_MEAN, 10.0, 0, 0);
        ok(rc == true, "pmSourceLocalSky() returned TRUE with acceptable input parameters");
        psF32 actualMean =  mean / (int) (TEST_NUM_ROWS * TEST_NUM_COLS);
        psF32 testMean = src->moments->Sky;
        ok(TEST_FLOATS_EQUAL(actualMean, testMean), "pmSourceLocalSky() calculated the mean correctly");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceLocalSkyVariance() tests
    // Call pmSourceLocalSkyVariance() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->variance->numRows ; i++) {
            for (int j = 0 ; j < src->variance->numCols ; j++) {
                src->variance->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSkyVariance(NULL, PS_STAT_SAMPLE_MEAN, 10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSkyVariance() returned FALSE with NULL pmSource input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSkyVariance() with NULL pmSource->variance input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSkyVariance(src, PS_STAT_SAMPLE_MEAN, 10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSkyVariance() returned FALSE with NULL pmSource->variance input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSkyVariance() with NULL pmSource->maskObj input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->variance->numRows ; i++) {
            for (int j = 0 ; j < src->variance->numCols ; j++) {
                src->variance->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSkyVariance(src, PS_STAT_SAMPLE_MEAN, -10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSkyVariance() returned FALSE with NULL pmSource->maskObj input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSkyVariance() with NULL pmSource->peak input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->variance->numRows ; i++) {
            for (int j = 0 ; j < src->variance->numCols ; j++) {
                src->variance->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        bool rc = pmSourceLocalSkyVariance(src, PS_STAT_SAMPLE_MEAN, 10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSkyVariance() returned FALSE with NULL pmSource->peak input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSkyVariance() with negative input radius
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i =0 ; i < src->variance->numRows ; i++) {
            for (int j = 0 ; j < src->variance->numCols ; j++) {
                src->variance->data.F32[i][j] = (float) (i + j);
	    }
	}
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSkyVariance(src, PS_STAT_SAMPLE_MEAN, -10.0, 1, 2);
        ok(rc == false, "pmSourceLocalSkyVariance() returned FALSE with negative input radius");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceLocalSkyVariance() with acceptable input parameters
    // XX: Future Improvements:
    //     Test more PS_STATS types
    //     Test more psRegion values (region bigger than image, 0 region, etc.)
    //     Test mask values
    //
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        psF32 mean = 0.0;
        for (int i = 0 ; i < src->variance->numRows ; i++) {
            for (int j = 0 ; j < src->variance->numCols ; j++) {
                src->variance->data.F32[i][j] = (float) (i + j);
                mean+= (float) (i + j);
                src->maskObj->data.U8[i][j] = 0;
	    }
	}
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        bool rc = pmSourceLocalSkyVariance(src, PS_STAT_SAMPLE_MEAN, 10.0, 0, 0);
        ok(rc == true, "pmSourceLocalSkyVariance() returned TRUE with acceptable input parameters");
        psF32 actualMean =  mean / (int) (TEST_NUM_ROWS * TEST_NUM_COLS);
        psF32 testMean = src->moments->dSky;
        ok(TEST_FLOATS_EQUAL(actualMean, testMean), "pmSourceLocalSky() calculated the mean correctly");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    XXX: All functions only tested with unallowable input parameters.
    I tried to use acceptable data, but could not get the source code to work the
    way I thought it should have.
*/
#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (10)
#define TEST_NUM_COLS           (16)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         10
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_SOURCES		100
int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(11);


    // ----------------------------------------------------------------------
    // pmSourceContour() tests
    // psArray *pmSourceContour (psImage *image, int xc, int yc, float threshold)
    // Call pmSourceContour() with NULL psImage input parameter
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psArray *array = pmSourceContour(NULL, 1, 2, 3.0);
        ok(array == NULL, "pmSourceContour() returned NULL with NULL psImage input parameter");
        psFree(img);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceContour() with unallowed row/column numbers
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psArray *array = pmSourceContour(img, -1, 2, 3.0);
        ok(array == NULL, "pmSourceContour() returned NULL with column = -1");
        array = pmSourceContour(img, TEST_NUM_COLS, 2, 3.0);
        ok(array == NULL, "pmSourceContour() returned NULL with column >= numCols");
        array = pmSourceContour(img, 1, -1, 3.0);
        ok(array == NULL, "pmSourceContour() returned NULL with row = -1");
        array = pmSourceContour(img, 1, TEST_NUM_ROWS, 3.0);
        ok(array == NULL, "pmSourceContour() returned NULL with row >= numRows");

        psFree(img);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceContour() with acceptable input parameters
    // XXX: These tests currently fail
    if (0) {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < img->numRows ; i++) {
            for (int j = 0 ; j < img->numCols ; j++) {
                img->data.F32[i][j] = (float) ((TEST_NUM_ROWS + TEST_NUM_COLS) - (abs(i - (TEST_NUM_ROWS/2)) + abs(j - (TEST_NUM_COLS/2))));
	    }
	}
        if (1) {
            for (int i = 0 ; i < img->numRows ; i++) {
                for (int j = 0 ; j < img->numCols ; j++) {
                    printf("(%.0f)", img->data.F32[i][j]);
    	    }
                printf("\n");
	    }
	}

        psArray *array = pmSourceContour(img, TEST_NUM_COLS/2, TEST_NUM_ROWS/2, 22.0);
        ok(array != NULL, "pmSourceContour() returned non-NULL with acceptable input parameters");
        for (int i = 0 ; i < array->n ; i++) {
            psVector *vec = (psVector *) array->data[i];
            printf("Point %d: (%.2f %.2f)\n", i, vec->data.F32[0], vec->data.F32[1]);
	}

        psFree(array);
        psFree(img);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceContour_Crude() tests
    // psArray *pmSourceContour_Crude_Crude(pmSource *source, psImage *image, psF32 level)
    // Call pmSourceContour_Crude() with NULL psSource input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->moments = pmMomentsAlloc();
        src->modelEXT = pmModelAlloc(1);
        psImage *img = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < img->numRows ; i++) {
            for (int j = 0 ; j < img->numCols ; j++) {
                if ((i >= TEST_NUM_ROWS/4) && (i < 3*TEST_NUM_ROWS/4) &&
                    (j >= TEST_NUM_COLS/4) && (j < 3*TEST_NUM_COLS/4)) {
                    img->data.F32[i][j] = 5.0;
		} else {
                    img->data.F32[i][j] = 0.0;
		}
	    }
	}
        psArray *array = pmSourceContour_Crude(NULL, img, 3.0);
        ok(array == NULL, "pmSourceContour_Crude() returned NULL with NULL pmSource input parameter");
        psFree(img);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceContour_Crude() with NULL psImage input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->moments = pmMomentsAlloc();
        src->modelEXT = pmModelAlloc(1);
        psImage *img = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < img->numRows ; i++) {
            for (int j = 0 ; j < img->numCols ; j++) {
                if ((i >= TEST_NUM_ROWS/4) && (i < 3*TEST_NUM_ROWS/4) &&
                    (j >= TEST_NUM_COLS/4) && (j < 3*TEST_NUM_COLS/4)) {
                    img->data.F32[i][j] = 5.0;
		} else {
                    img->data.F32[i][j] = 0.0;
		}
	    }
	}
        psArray *array = pmSourceContour_Crude(src, NULL, 3.0);
        ok(array == NULL, "pmSourceContour_Crude() returned NULL with NULL pmImage input parameter");
        psFree(img);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceContour_Crude() with acceptable input parameters
    // XXX: Must correct this
    if (0) {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->moments = pmMomentsAlloc();
        src->modelEXT = pmModelAlloc(1);
        psImage *img = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);

        if (0) {
            for (int i = 0 ; i < img->numRows ; i++) {
                for (int j = 0 ; j < img->numCols ; j++) {
                    img->data.F32[i][j] = (float) ((TEST_NUM_ROWS + TEST_NUM_COLS) - (abs(i - (TEST_NUM_ROWS/2)) + abs(j - (TEST_NUM_COLS/2))));
		}
	    }
	}
        printf("Calling pmSourceContour_Crude()\n");
        psArray *array = pmSourceContour_Crude(src, img, 22.0);
        printf("Called pmSourceContour_Crude()\n");
        ok(array != NULL, "pmSourceContour_Crude() returned non-NULL with NULL pmImage input parameter");
        psFree(img);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

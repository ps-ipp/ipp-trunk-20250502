/** @file tst_pmSubtractSky.c
 *
 *  @brief Contains the tests for pmSubtractSky.c:
 *
 * test00: This code will test the pmSubtractSky routine.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-05-25 22:02:23 $
 *
 *  XXX: I added the CELL.TRIMSEC region code but there are not tests for it.
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include "psTest.h"
#include "pslib.h"
#include "pmSubtractSky.h"
#define NUM_ROWS 512
#define NUM_COLS 512
#define POLY_X_ORDER 3
#define POLY_Y_ORDER 3
#define ERROR_TOLERANCE 1.0
#define OBJECT_INTENSITY 2000.0
static int test00(void);
static int test01(void);
testDescription tests[] = {
                              {test00, 000, "pmSubtractSky", 0, false},
                              {test01, 000, "pmSubtractSky: warning, error messages", 0, false},
                              {NULL}
                          };

float func(int i, int j)
{
    return((float) (i + j));
}

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
    //    test00();
}

/******************************************************************************
 *****************************************************************************/
int doSubtractSkySimple(int numCols, int numRows, int binFactor)
{
    int i;
    int j;
    int testStatus = 0;
    psImage *tmpImageF32 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    //    pmReadout *myReadout = pmReadoutAlloc(numCols, numRows, tmpImageF32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    myReadout->image = tmpImageF32;
    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psPolynomial2D *myPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, POLY_X_ORDER, POLY_Y_ORDER);

    printPositiveTestHeader(stdout, "pmSubtractSky", "doSubtractSkySimple");
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = func(i, j);
        }
    }

    myReadout = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL,
                              binFactor, myStats, 10.0);
    if (myReadout == NULL) {
        printf("TEST ERROR: pmSubtractSky() returned NULL.\n");
        testStatus = 1;
    } else {
        for (i=0;i<numRows;i++) {
            for (j=0;j<numCols;j++) {
                if (ERROR_TOLERANCE < fabs(myReadout->image->data.F32[i][j])) {
                    printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j, myReadout->image->data.F32[i][j]);
                    testStatus = 1;
                }
            }
        }
    }
    psFree(myReadout);
    psFree(myStats);
    psFree(myPoly);
    printFooter(stdout, "pmSubtractSky", "doSubtractSkySimple", true);
    return(testStatus);
}

/******************************************************************************
 *****************************************************************************/
int doSubtractSkyWithObjects(int numCols, int numRows, int binFactor)
{
    int i;
    int j;
    int testStatus = 0;
    psImage *tmpImageF32 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    //    pmReadout *myReadout = pmReadoutAlloc(numCols, numRows, tmpImageF32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    myReadout->image = tmpImageF32;
    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psPolynomial2D *myPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, POLY_X_ORDER, POLY_Y_ORDER);
    psImage *trueImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psF32 errorTolerance = ERROR_TOLERANCE * ((psF32) binFactor);

    printPositiveTestHeader(stdout, "pmSubtractSky", "doSubtractSkyWithObjects");
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = func(i, j);
        }
    }
    // We insert a few bright spots in the image.
    myReadout->image->data.F32[NUM_ROWS/4][NUM_COLS/4]+= OBJECT_INTENSITY;
    myReadout->image->data.F32[NUM_ROWS/4][3*NUM_COLS/4]+= OBJECT_INTENSITY;
    myReadout->image->data.F32[3*NUM_ROWS/4][NUM_COLS/4]+= OBJECT_INTENSITY;
    myReadout->image->data.F32[3*NUM_ROWS/4][3*NUM_COLS/4]+= OBJECT_INTENSITY;
    PS_IMAGE_SET_F32(trueImage, 0.0);
    trueImage->data.F32[NUM_ROWS/4][NUM_COLS/4]+= OBJECT_INTENSITY;
    trueImage->data.F32[NUM_ROWS/4][3*NUM_COLS/4]+= OBJECT_INTENSITY;
    trueImage->data.F32[3*NUM_ROWS/4][NUM_COLS/4]+= OBJECT_INTENSITY;
    trueImage->data.F32[3*NUM_ROWS/4][3*NUM_COLS/4]+= OBJECT_INTENSITY;

    myReadout = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL,
                              binFactor, myStats, 2.0);
    if (myReadout == NULL) {
        printf("TEST ERROR: pmSubtractSky() returned NULL.\n");
        testStatus = 1;
    } else {
        for (i=0;i<numRows;i++) {
            for (j=0;j<numCols;j++) {
                if (errorTolerance < fabs(myReadout->image->data.F32[i][j] - trueImage->data.F32[i][j])) {
                    printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j,
                           myReadout->image->data.F32[i][j], trueImage->data.F32[i][j]);
                    testStatus = 1;
                }
            }
        }
    }
    psFree(myReadout);
    psFree(myStats);
    psFree(myPoly);
    psFree(trueImage);
    printFooter(stdout, "pmSubtractSky", "doSubtractSkyWithObjects", true);
    return(testStatus);
}


int test00( void )
{
    int testStatus = 0;

    psTraceSetLevel(".", 0);

    // Bin Factor == 1
    printf("doSubtractSkySimple(1, 1, 1)\n");
    testStatus |= doSubtractSkySimple(1, 1, 1);
    printf("doSubtractSkySimple(NUM_COLS, 1, 1)\n");
    testStatus |= doSubtractSkySimple(NUM_COLS, 1, 1);

    printf("doSubtractSkySimple(1, NUM_ROWS, 1)\n");
    testStatus |= doSubtractSkySimple(1, NUM_ROWS, 1);
    printf("doSubtractSkySimple(NUM_COLS, NUM_ROWS, 1)\n");
    testStatus |= doSubtractSkySimple(NUM_COLS, NUM_ROWS, 1);

    // Bin Factor == 2
    printf("doSubtractSkySimple(1, 1, 2)\n");
    testStatus |= doSubtractSkySimple(1, 1, 2);
    printf("doSubtractSkySimple(NUM_COLS, 1, 2)\n");
    testStatus |= doSubtractSkySimple(NUM_COLS, 1, 2);
    printf("doSubtractSkySimple(1, NUM_ROWS, 2)\n");
    testStatus |= doSubtractSkySimple(1, NUM_ROWS, 2);
    printf("doSubtractSkySimple(NUM_COLS, NUM_ROWS, 2)\n");
    testStatus |= doSubtractSkySimple(NUM_COLS, NUM_ROWS, 2);

    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 1)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 1);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 2)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 2);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 4)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 8);

    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 8)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 8);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 16)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 16);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 32)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 32);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 64)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 64);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 128)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 128);
    printf("doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 256)\n");
    testStatus |= doSubtractSkyWithObjects(NUM_COLS, NUM_ROWS, 256);

    return(testStatus);
}

#define NUM_ROWS_SMALL 16
#define NUM_COLS_SMALL 16
int test01( void )
{
    int testStatus = 0;
    psS32 i;
    psS32 j;
    psImage *tmpImageF32 = psImageAlloc(NUM_COLS_SMALL, NUM_ROWS_SMALL, PS_TYPE_F32);
    psImage *tmpImageF64 = psImageAlloc(NUM_COLS_SMALL, NUM_ROWS_SMALL, PS_TYPE_F64);
    //    pmReadout *myReadout = pmReadoutAlloc(NUM_COLS_SMALL, NUM_ROWS_SMALL, tmpImageF32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    myReadout->image = tmpImageF32;
    pmReadout *rc = NULL;
    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psPolynomial2D *myPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, POLY_X_ORDER, POLY_Y_ORDER);

    printPositiveTestHeader(stdout, "pmSubtractSky", "Testing bad input parameter conditions.");
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            myReadout->image->data.F32[i][j] = func(i, j);
        }
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with NULL pmReadout.  Should error.\n\n");
    rc = pmSubtractSky(NULL, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, myStats, 2.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmSubtractSky() returned a non-NULL pmReadout\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with NULL pmReadout->image.  Should error.\n\n");
    myReadout->image = NULL;
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, myStats, 2.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmSubtractSky() returned a non-NULL pmReadout\n");
        testStatus = false;
    }
    myReadout->image = tmpImageF32;

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with PS_TYPE_F64 pmReadout->image.  Should error.\n\n");
    myReadout->image = tmpImageF64;
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, myStats, 2.0);
    if (rc != NULL) {
        printf("TEST ERROR: pmSubtractSky() returned a non-NULL pmReadout\n");
        testStatus = false;
    }
    myReadout->image = tmpImageF32;

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with NULL fitSpec.  Should return image, no ERROR, no WARNING.\n\n");
    rc = pmSubtractSky(myReadout, NULL, PM_FIT_POLYNOMIAL, 1, myStats, 2.0);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractSky() returned something other than pmReadout\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with PM_FIT_NONE fit.  Should return image, no ERROR, no WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_NONE, 1, myStats, 2.0);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractSky() returned something other than pmReadout\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with PM_FIT_SPLINE fit.  Should return image, no ERROR, no WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_SPLINE, 1, myStats, 2.0);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractSky() returned something other than pmReadout\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with NULL myStats.  Should fit entire image, generate WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, NULL, 2.0);
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            if (ERROR_TOLERANCE < fabs(rc->image->data.F32[i][j])) {
                printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j,
                       rc->image->data.F32[i][j]);
                testStatus = false;
            }
        }
    }

    printf("----------------------------------------------------------------\n");
    psU64 oldOptions = myStats->options;
    myStats->options = 0;
    printf("Calling pmSubtractSky() with no myStats->options specified.  Should fit entire image, generate WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, myStats, 2.0);
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            if (ERROR_TOLERANCE < fabs(rc->image->data.F32[i][j])) {
                printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j,
                       rc->image->data.F32[i][j]);
                testStatus = false;
            }
        }
    }
    myStats->options = oldOptions;

    printf("----------------------------------------------------------------\n");
    oldOptions = myStats->options;
    myStats->options = PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN;
    printf("Calling pmSubtractSky() with multiple myStats->options specified.  Should fit entire image, generate WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, myStats, 2.0);
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            if (ERROR_TOLERANCE < fabs(rc->image->data.F32[i][j])) {
                printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j,
                       rc->image->data.F32[i][j]);
                testStatus = false;
            }
        }
    }
    myStats->options = oldOptions;

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with 0 binFactor.  Should fit entire image, generate WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 0, myStats, 2.0);
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            if (ERROR_TOLERANCE < fabs(rc->image->data.F32[i][j])) {
                printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j,
                       rc->image->data.F32[i][j]);
                testStatus = false;
            }
        }
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with -1 binFactor.  Should fit entire image, generate WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, -1, myStats, 2.0);
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            if (ERROR_TOLERANCE < fabs(rc->image->data.F32[i][j])) {
                printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j,
                       rc->image->data.F32[i][j]);
                testStatus = false;
            }
        }
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with -1.0 clipSD.  Should fit entire image, generate WARNING.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL, 1, myStats, -1.0);
    for (i=0;i<NUM_ROWS_SMALL;i++) {
        for (j=0;j<NUM_COLS_SMALL;j++) {
            if (ERROR_TOLERANCE < fabs(rc->image->data.F32[i][j])) {
                printf("TEST ERROR: image[%d][%d] is %f, should be 0.0\n", i, j,
                       rc->image->data.F32[i][j]);
                testStatus = false;
            }
        }
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmSubtractSky() with bogus psFit.  Should generate Error.\n\n");
    rc = pmSubtractSky(myReadout, (void *) myPoly, 54321, 1, myStats, -1.0);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractSky() returned something other than pmReadout\n");
        testStatus = false;
    }


    //    myReadout = pmSubtractSky(myReadout, (void *) myPoly, PM_FIT_POLYNOMIAL,
    //                              1, myStats, 2.0);


    printf("----------------------------------------------------------------\n");
    psFree(myReadout);
    psFree(myStats);
    psFree(myPoly);
    psFree(tmpImageF64);
    printFooter(stdout, "pmSubtractSky", "Testing bad input parameter conditions.", true);
    return(testStatus);
}

/** @file tst_pmSubtractBias.c
 *
 *  @brief Contains the tests for pmSubtractBias.c:
 *
 * test00a: This code will subtract full bias frames from the input image.
 * XXX: Must test:
 *  Various image offsets.
 *  Various image size combinations.
 *  Various data types for the bias and input images.
 *  Ensure code works when CELL.TRIMSEC is not set.
 * test00b: This code will subtract full dark frames from the input image.
 * XXX: Must test:
 *  Various image offsets.
 *  Various image size combinations.
 *  Various data types for the bias and input images.
 *  Code properly determines CELL.DARKTIME from cell metadata.
 *  Ensure code works when CELL.DARKTIME is not set.
 *  Ensure code works when CELL.TRIMSEC is not set.
 *  test03: Calculate a row overscan vector and subtract it from each
 *  row in the input image.
 * test05:
 *
 *  @author GLG, MHPCC
 *
 *  XXX: Memory leaks are not being detected.
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-05-25 22:02:23 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

#include "psTest.h"
#include "pslib.h"
#include "pmSubtractBias.h"
static int test00a(void);
static int test00b(void);
//static int test01(void);
//static int test02(void);
//static int test03(void);
//static int test04(void);
static int test05(void);
testDescription tests[] = {
                              {test00a, 000, "doSubtractBiasFullFrame", 0, true},
                              {test00b, 000, "doSubtractDarkFullFrame", 0, true},
                              //                              {test01, 000, "pmSubtractBias", 0, true},
                              //                              {test02, 000, "pmSubtractBias", 0, true},
                              //                              {test03, 000, "pmSubtractBias", 0, true},
                              //                              {test04, 000, "pmSubtractBias", 0, true},
                              {test05, 000, "pmSubtractBias", 0, false},
                              {NULL}
                          };

psS32 currentId = 0;
psS32 memLeaks = 0;             // XXX: remove

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");

    psTraceSetLevel(".", 0);
    psTraceSetLevel("spline1DFree", 0);
    psTraceSetLevel("calculateSecondDerivs", 0);
    psTraceSetLevel("vectorBinDisectF32", 0);
    psTraceSetLevel("vectorBinDisectF64", 0);
    psTraceSetLevel("p_psVectorBinDisect", 0);
    psTraceSetLevel("psSpline1DAlloc", 0);
    psTraceSetLevel("psVectorFitSpline1D", 0);
    psTraceSetLevel("psSpline1DEval", 0);
    psTraceSetLevel("psSpline1DEvalVector", 0);

    psS32 currentId = psMemGetId(); // XXX: remove
    psS32 memLeaks = 0;             // XXX: remove
    if (0) {
        PRINT_MEMLEAKS(0);
    }

    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}

#define NUM_ROWS 8
#define NUM_COLS 8
#define MAX_HEADER_MSG_LENGTH 1000
#define POLYNOMIAL_FIT_ORDER 2
#define NUM_OVERSCANS 2
/******************************************************************************
doSubtractBiasFullFrame(): a sample pmReadout as well as a bias image are
created and the bias image is subtracted from the pmReadout.
 *****************************************************************************/
int doSubtractBiasFullFrame(int numCols, int numRows)
{
    int i;
    int j;
    float actual;
    float expect;
    int testStatus = 0;
    psImage *tmpImage1 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tmpImage2 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    pmReadout *myBias = pmReadoutAlloc(NULL);
    myReadout->image = tmpImage1;
    myBias->image = tmpImage2;

    char *HeaderMessageStr = (char *) psAlloc(MAX_HEADER_MSG_LENGTH);
    sprintf(HeaderMessageStr, "doSubtractBiasFullFrame(%d, %d)", numRows, numCols);
    printPositiveTestHeader(stdout, "pmSubtractBias", HeaderMessageStr);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
            myBias->image->data.F32[i][j] = 1.0;
        }
    }

    myReadout = pmSubtractBias(myReadout, NULL, PM_FIT_NONE, false,
                               NULL, 0, myBias, NULL);

    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            expect = ((float) (i + j)) - 1.0;
            actual = myReadout->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = 1;
            }
        }
    }


    psFree(myReadout);
    psFree(myBias);
    printFooter(stdout, "pmSubtractBias", HeaderMessageStr, true);
    psFree(HeaderMessageStr);
    return(testStatus);
}


int test00a( void )
{
    int testStatus = 0;

    testStatus |= doSubtractBiasFullFrame(1, 1);
    testStatus |= doSubtractBiasFullFrame(NUM_COLS, 1);
    testStatus |= doSubtractBiasFullFrame(1, NUM_ROWS);
    testStatus |= doSubtractBiasFullFrame(NUM_COLS, NUM_ROWS);
    return(testStatus);
}


/******************************************************************************
doSubtractDarkFullFrame(): a sample pmReadout as well as a dark image are
created and the dark image is subtracted from the pmReadout.
 *****************************************************************************/
int doSubtractDarkFullFrame(int numCols, int numRows)
{
    int i;
    int j;
    float actual;
    float expect;
    int testStatus = 0;
    psImage *tmpImage1 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tmpImage2 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    pmReadout *myDark = pmReadoutAlloc(NULL);
    myReadout->image = tmpImage1;
    myDark->image = tmpImage2;

    char *HeaderMessageStr = (char *) psAlloc(MAX_HEADER_MSG_LENGTH);
    sprintf(HeaderMessageStr, "doSubtractDarkFullFrame(%d, %d)", numRows, numCols);
    printPositiveTestHeader(stdout, "pmSubtractBias", HeaderMessageStr);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
            myDark->image->data.F32[i][j] = 1.0;
        }
    }

    myReadout = pmSubtractBias(myReadout, NULL, PM_FIT_NONE, false,
                               NULL, 0, NULL, myDark);

    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            expect = ((float) (i + j)) - 1.0;
            actual = myReadout->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = 1;
            }
        }
    }


    psFree(myReadout);
    psFree(myDark);
    printFooter(stdout, "pmSubtractBias", HeaderMessageStr, true);
    psFree(HeaderMessageStr);
    return(testStatus);
}


int test00b( void )
{
    int testStatus = 0;

    testStatus |= doSubtractDarkFullFrame(1, 1);
    testStatus |= doSubtractDarkFullFrame(NUM_COLS, 1);
    testStatus |= doSubtractDarkFullFrame(1, NUM_ROWS);
    testStatus |= doSubtractDarkFullFrame(NUM_COLS, NUM_ROWS);
    return(testStatus);
}


/*
int doSubtractOverscansTestInputCases(int numCols, int numRows)
{
    int i;
    int j;
    int testStatus = 0;
    psImage *tmpImage1 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tmpImage2 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tmpImage3 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tmpImage4 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tmpImage2Short = psImageAlloc(numCols-1, numRows-1, PS_TYPE_F32);
    psImage *tmpImage3Short = psImageAlloc(numCols-1, numRows-1, PS_TYPE_F32);
    psImage *tmpImage4Short = psImageAlloc(numCols-1, numRows-1, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    myReadout->image = tmpImage1;
    pmReadout *rc = NULL;
    psList *list;
    psList *listShort;
    psStats *stat = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psImage *tmpImage5 = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    pmReadout *myBias = pmReadoutAlloc(NULL);
    myBias->image = tmpImage5;
    printPositiveTestHeader(stdout, "pmSubtractBias", "Testing input parameter error conditions");
 
    psImage *tmpImage5ShortRows = psImageAlloc(numCols, numRows-1, PS_TYPE_F32);
    pmReadout *myBiasShortRows = pmReadoutAlloc(NULL);
    myBiasShortRows->image = tmpImage5ShortRows;
    psImage *tmpImage5ShortCols = psImageAlloc(numCols-1, numRows, PS_TYPE_F32);
    pmReadout *myBiasShortCols = pmReadoutAlloc(NULL);
    myBiasShortCols->image = tmpImage5ShortCols;
 
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
            tmpImage2->data.F32[i][j] = 3.0;
            tmpImage3->data.F32[i][j] = 4.0;
            tmpImage4->data.F32[i][j] = 5.0;
            myBias->image->data.F32[i][j] = 1.0;
        }
    }
    list = psListAlloc(tmpImage2);
    psListAdd(list, PS_LIST_HEAD, tmpImage3);
    psListAdd(list, PS_LIST_HEAD, tmpImage4);
 
    for (i=0;i<numRows-1;i++) {
        for (j=0;j<numCols-1;j++) {
            tmpImage2Short->data.F32[i][j] = 3.0;
            tmpImage3Short->data.F32[i][j] = 4.0;
            tmpImage4Short->data.F32[i][j] = 5.0;
        }
    }
    listShort = psListAlloc(tmpImage2Short);
    psListAdd(listShort, PS_LIST_HEAD, tmpImage3Short);
    psListAdd(listShort, PS_LIST_HEAD, tmpImage4Short);
    for (i=0;i<numRows-1;i++) {
        for (j=0;j<numCols;j++) {
            myBiasShortRows->image->data.F32[i][j] = 1.0;
        }
    }
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols-1;j++) {
            myBiasShortCols->image->data.F32[i][j] = 1.0;
        }
    }
 
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with NULL overscan list and PM_OVERSCAN_ALL.  Should generate error.\n");
    rc = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_ALL, stat, 0, PM_FIT_NONE, NULL);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with NULL overscan list and PM_OVERSCAN_ROWS.  Should generate error.\n");
    rc = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_ROWS, stat, 0, PM_FIT_NONE, NULL);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with NULL overscan list and PM_OVERSCAN_COLUMNS.  Should generate error.\n");
    rc = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_COLUMNS, stat, 0, PM_FIT_NONE, NULL);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with non-NULL overscan list and PM_OVERSCAN_NONE.  Should generate warning.\n");
    rc = pmSubtractBias(myReadout, NULL, list, PM_OVERSCAN_NONE, stat, 0, PM_FIT_NONE, myBias);
 
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            psF32 expect = ((float) (i + j)) - 1.0;
            psF32 actual = rc->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = 1;
            }
 
            // Restore myReadout for next test.
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }
 
    // XXX: This does not seem to be a requirement.
    if (0) {
        printf("------------------------------------------------------------------\n");
        printf("Calling pmSubtractBias() with NULL overscan list and PM_OVERSCAN_NONE.  Should generate warning.\n");
        rc = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_NONE, stat,
                            0, PM_FIT_NONE, myBias);
 
        for (i=0;i<numRows;i++) {
            for (j=0;j<numCols;j++) {
                psF32 expect = ((float) (i + j)) - 1.0;
                psF32 actual = rc->image->data.F32[i][j];
                if (FLT_EPSILON < fabs(expect - actual)) {
                    printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                    testStatus = 1;
                }
 
                // Restore myReadout for next test.
                myReadout->image->data.F32[i][j] = (float) (i + j);
            }
        }
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with PM_OVERSCAN_NONE and PM_FIT_POLYNOMIAL.  Should generate Warning.\n");
    rc = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_NONE, stat, 0, PM_FIT_POLYNOMIAL, myBias);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            psF32 expect = ((float) (i + j)) - 1.0;
            psF32 actual = rc->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = 1;
            }
 
            // Restore myReadout for next test.
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }
 
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with PM_OVERSCAN_ALL and PM_FIT_SPLINE.  Should generate Warning.\n");
    rc = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_NONE, stat, 0, PM_FIT_SPLINE, myBias);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            psF32 expect = ((float) (i + j)) - 1.0;
            psF32 actual = rc->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = 1;
            }
 
            // Restore myReadout for next test.
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }
 
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with multiple stats->options.  Should generate Warning.\n");
    stat->options|= PS_STAT_SAMPLE_MEDIAN;
    myReadout = pmSubtractBias(myReadout, NULL, list, PM_OVERSCAN_ALL, stat,
                               0, PM_FIT_NONE, NULL);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            psF32 expect = ((float) (i + j)) - 12.0;
            psF32 actual = rc->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = 1;
            }
        }
    }
    stat->options = PS_STAT_SAMPLE_MEAN;
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() undersize overscans (PM_OVERSCAN_ROWS).  Should generate Warning.\n");
    rc = pmSubtractBias(myReadout, NULL, listShort, PM_OVERSCAN_ROWS, stat,
                        0, PM_FIT_NONE, NULL);
    if (0) {
        for (i=0;i<numRows;i++) {
            for (j=0;j<numCols;j++) {
                psF32 expect = ((float) (i + j)) - 12.0;
                psF32 actual = rc->image->data.F32[i][j];
                if (FLT_EPSILON < fabs(expect - actual)) {
                    printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                    testStatus = 1;
                }
            }
        }
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() undersize overscans (PM_OVERSCAN_COLUMNS).  Should generate Warning.\n");
    rc = pmSubtractBias(myReadout, NULL, listShort, PM_OVERSCAN_COLUMNS, stat,
                        0, PM_FIT_NONE, NULL);
    if (0) {
        for (i=0;i<numRows;i++) {
            for (j=0;j<numCols;j++) {
                psF32 expect = ((float) (i + j)) - 12.0;
                psF32 actual = rc->image->data.F32[i][j];
                if (FLT_EPSILON < fabs(expect - actual)) {
                    printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                    testStatus = 1;
                }
            }
        }
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() undersize bias image (short rows).  Should generate Error.\n");
    myReadout = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_NONE, NULL,
                               0, PM_FIT_NONE, myBiasShortRows);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() undersize bias image (short columns).  Should generate Error.\n");
    myReadout = pmSubtractBias(myReadout, NULL, NULL, PM_OVERSCAN_NONE, NULL,
                               0, PM_FIT_NONE, myBiasShortCols);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with bogus PM_FIT.  Should generate Error.\n");
    myReadout = pmSubtractBias(myReadout, NULL, list, PM_OVERSCAN_ROWS, stat,
                               0, 54321, NULL);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
    printf("------------------------------------------------------------------\n");
    printf("Calling pmSubtractBias() with bogus overScanAxis.  Should generate Error.\n");
    myReadout = pmSubtractBias(myReadout, NULL, list, 54321, stat,
                               0, PM_FIT_NONE, NULL);
    if (rc != myReadout) {
        printf("TEST ERROR: pmSubtractBias() did not return input pmReadout.\n");
        testStatus = false;
        psFree(rc);
    }
 
    if (0) {
        for (i=0;i<numRows;i++) {
            for (j=0;j<numCols;j++) {
                psF32 expect = ((float) (i + j)) - 12.0;
                psF32 actual = rc->image->data.F32[i][j];
                if (FLT_EPSILON < fabs(expect - actual)) {
                    printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                    testStatus = 1;
                }
            }
        }
    }
 
    printf("------------------------------------------------------------------\n");
    psFree(myReadout);
    psFree(tmpImage2);
    psFree(tmpImage3);
    psFree(tmpImage4);
    psFree(tmpImage2Short);
    psFree(tmpImage3Short);
    psFree(tmpImage4Short);
    psFree(myBias);
    psFree(myBiasShortRows);
    psFree(myBiasShortCols);
    psFree(stat);
    psFree(list);
    psFree(listShort);
 
    printFooter(stdout, "pmSubtractBias", "Testing input parameter error conditions", true);
    return(testStatus);
}
 
int test04( void )
{
    int testStatus = 0;
 
    testStatus |= doSubtractOverscansTestInputCases(NUM_COLS, NUM_ROWS);
    return(testStatus);
}
*/

void PS_POLY1D_PRINT(
    psPolynomial1D *poly)
{
    printf("-------------- PS_POLY1D_PRINT() --------------\n");
    printf("poly->nX is %d\n", poly->nX);
    for (psS32 i = 0 ; i < (1 + poly->nX) ; i++) {
        printf("poly->coeff[%d] is %f\n", i, poly->coeff[i]);
    }
}

void PS_PRINT_SPLINE2(psSpline1D *mySpline)
{
    printf("-------------- PS_PRINT_SPLINE2() --------------\n");
    if (mySpline != NULL) {
        printf("mySpline->n is %d\n", mySpline->n);
        for (psS32 i = 0 ; i < mySpline->n ; i++) {
            if (mySpline->spline[i] != NULL) {
                PS_POLY1D_PRINT(mySpline->spline[i]);
            }
        }
        if (mySpline->knots != NULL) {
            PS_VECTOR_PRINT_F32(mySpline->knots);
        }
    } else {
        printf("NULL\n");
    }
    printf("-------------- PS_PRINT_SPLINE2() DONE --------------\n");
}






/******************************************************************************
doSubtractOverscansGeneric(): This is a general version of the
bias subtraction tests which allows the various parameters to be specified
as arguments.
 *****************************************************************************/
int doSubtractOverscansGeneric(
    int imageNumCols,
    int imageNumRows,
    int overscanNumCols,
    int overscanNumRows,
    int numOverscans,
    pmOverscanAxis overscanaxis,
    pmFit fit,
    psS32 nBin)
{
    int i;
    int j;
    float actual;
    float expect;
    int testStatus = 0;

    printPositiveTestHeader(stdout, "pmSubtractBias", "PUT COMMENT HERE");
    printf("---- doSubtractOverscansGeneric() ----\n");
    printf("    Image size: %d by %d\n", imageNumRows, imageNumCols);
    printf("    Overscan size: %d by %d\n", overscanNumRows, overscanNumCols);
    printf("    Total Overscans: %d\n", numOverscans);
    printf("    Binning factor: %d\n", nBin);
    if (overscanaxis == PM_OVERSCAN_ROWS)
        printf("    Overscan axis: PM_OVERSCAN_ROWS\n");
    if (overscanaxis == PM_OVERSCAN_COLUMNS)
        printf("    Overscan axis: PM_OVERSCAN_COLUMNS\n");
    if (overscanaxis == PM_OVERSCAN_ALL)
        printf("    Overscan axis: PM_OVERSCAN_ALL\n");
    if (overscanaxis == PM_OVERSCAN_NONE)
        printf("    Overscan axis: PM_OVERSCAN_NONE\n");
    if (fit == PM_FIT_NONE)
        printf("    Fit type: PM_FIT_NONE\n");
    if (fit == PM_FIT_POLYNOMIAL)
        printf("    Fit type: PM_FIT_POLYNOMIAL\n");
    if (fit == PM_FIT_SPLINE)
        printf("    Fit type: PM_FIT_SPLINE\n");

    //
    // Create and initialize input image, FPA hierarchy.
    //
    const psMetadata *camera = psMetadataAlloc();
    pmFPA* fpa = pmFPAAlloc(camera);

    if (fpa == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc returned a NULL.\n");
        return 1;
    }

    pmChip *chip = pmChipAlloc(fpa, "ChipName");
    if (chip == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmChipAlloc returned a NULL.\n");
        return 2;
    }

    pmCell *cell = pmCellAlloc(chip, (psMetadata *) camera, "CellName");
    if (cell == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmCellAlloc returned a NULL.\n");
        return 3;
    }

    pmReadout *myReadout = pmReadoutAlloc(cell);
    myReadout->image = psImageAlloc(imageNumCols, imageNumRows, PS_TYPE_F32);
    for (i=0;i<myReadout->image->numRows;i++) {
        for (j=0;j<myReadout->image->numCols;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }

    //
    // Set overscan axis in the metadata.
    //
    psBool rc = false;
    if (overscanaxis == PM_OVERSCAN_ROWS) {
        rc = psMetadataAddS32(myReadout->parent->concepts, PS_LIST_HEAD, "CELL.READDIR", 0, NULL, 1);
    } else if (overscanaxis == PM_OVERSCAN_COLUMNS) {
        rc = psMetadataAddS32(myReadout->parent->concepts, PS_LIST_HEAD, "CELL.READDIR", 0, NULL, 2);
    } else if (overscanaxis == PM_OVERSCAN_ALL) {
        rc = psMetadataAddS32(myReadout->parent->concepts, PS_LIST_HEAD, "CELL.READDIR", 0, NULL, 3);
    } else if (overscanaxis == PM_OVERSCAN_NONE) {
        rc = psMetadataAddS32(myReadout->parent->concepts, PS_LIST_HEAD, "CELL.READDIR", 0, NULL, 0);
    }
    if (rc == false) {
        printf("TEST ERROR: Could not set CELL.READDIR metadata.\n");
        testStatus = 1;
    }

    psStats *stat = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, POLYNOMIAL_FIT_ORDER);
    psSpline1D *mySpline = NULL;


    if (0) {
        if (overscanNumRows <= 0) {
            overscanNumRows = 1;
        }
        if (overscanNumCols <= 0) {
            overscanNumCols = 1;
        }
    }
    psF32 oAverage = 0.0;
    myReadout->bias = NULL;
    for (psS32 i = 0 ; i < numOverscans ; i++) {
        psImage *tmpImage = psImageAlloc(overscanNumCols, overscanNumRows, PS_TYPE_F32);
        psF32 oValue = (float) (i + 3);
        PS_IMAGE_SET_F32(tmpImage, oValue);
        oAverage += oValue;
        if (myReadout->bias == NULL) {
            myReadout->bias = psListAlloc(tmpImage);
        } else {
            psListAdd(myReadout->bias, PS_LIST_HEAD, tmpImage);
        }
    }
    oAverage/= (psF32) numOverscans;
    if (0) {
        if (fit == PM_FIT_NONE) {
            myReadout = pmSubtractBias(myReadout, NULL, PM_FIT_NONE, overscanaxis,
                                       stat, nBin, NULL, NULL);
        } else if (fit == PM_FIT_POLYNOMIAL) {
            myReadout = pmSubtractBias(myReadout, myPoly, PM_FIT_POLYNOMIAL, overscanaxis,
                                       stat, nBin, NULL, NULL);
        } else if (fit == PM_FIT_SPLINE) {
            //        mySpline = psSpline1DAlloc();
            myReadout = pmSubtractBias(myReadout, mySpline, PM_FIT_SPLINE, overscanaxis,
                                       stat, nBin, NULL, NULL);
        }
        if (myReadout == NULL ) {
            printf("TEST ERROR: pmSubtractBias() returned NULL.\n");
            testStatus = 1;
        } else {
            for (i=0;i<imageNumRows;i++) {
                for (j=0;j<imageNumCols;j++) {
                    expect = ((float) (i + j)) - oAverage;
                    actual = myReadout->image->data.F32[i][j];
                    if (FLT_EPSILON < fabs(expect - actual)) {
                        printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                        testStatus = 1;
                    } else {
                        //printf("GOOD: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                    }
                }
            }
        }
    }

    // HEY
    psFree(fpa);
    psFree(stat);
    psFree(myPoly);
    psFree(mySpline);

    printFooter(stdout, "pmSubtractBias", "Column Overscans", true);
    return(testStatus);
}


































/******************************************************************************
test05a() The following combinations are tested here:
 Overscan images are same size, no fit, bin factor is 1.
 Overscan images are same size, no fit, bin factor is 2.
 *****************************************************************************/
int test05a(
    psS32 imageNumCols,
    psS32 imageNumRows,
    psS32 overscanNumCols,
    psS32 overscanNumRows)
{
    int testStatus = 0;

    // imageNumCols, imageNumRows, overscanNumCols, overscanNumRows,
    // overscanaxis, fit, nBin

    //
    // Overscan images are same size, no fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_NONE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_NONE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_NONE, 1);

    //
    // Overscan images are same size, no fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_NONE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_NONE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_NONE, 2);

    return(testStatus);
}


/******************************************************************************
test05b() The following combinations are tested here:
 Overscan images are too small, spline fit, bin factor is 1.
 Overscan images are too small, spline fit, bin factor is 2.
 Overscan images are same size, spline fit, bin factor is 1.
 Overscan images are same size, spline fit, bin factor is 2.
 Overscan images are too big,   spline fit, bin factor is 1.
 Overscan images are too big,   spline fit, bin factor is 2.
 A single overscan image of the same size, spline fit, bin factor is 1.
 
 Overscan images are too small, polynomial fit, bin factor is 1.
 Overscan images are too small, polynomial fit, bin factor is 2.
 Overscan images are same size, polynomial fit, bin factor is 1.
 Overscan images are same size, polynomial fit, bin factor is 2.
 Overscan images are too big,   polynomial fit, bin factor is 1.
 Overscan images are too big,   polynomial fit, bin factor is 2.
 A single overscan image of the same size, polynomial fit, bin factor is 1.
 
XXX: Must add M-by-N image size tests.
 *****************************************************************************/
int test05b(
    psS32 imageNumCols,
    psS32 imageNumRows,
    psS32 overscanNumCols,
    psS32 overscanNumRows)
{
    int testStatus = 0;

    //
    // Overscan images are too small, spline fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 1);

    //
    // Overscan images are too small, spline fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 2);


    //
    // Overscan images are same size, spline fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 1);

    //
    // Overscan images are same size, spline fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 2);

    //
    // Overscan images are too big, spline fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 1);

    //
    // Overscan images are too big, spline fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2 , NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 2);


    //
    // A single overscan image of the same size, spline fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, 1,
                  PM_OVERSCAN_ALL,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, 1,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_SPLINE, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, 1,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_SPLINE, 1);


    //
    // Overscan images are too small, polynomial fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 1);

    //
    // Overscan images are too small, polynomial fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols-2, overscanNumRows-2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 2);


    //
    // Overscan images are same size, polynomial fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 1);

    //
    // Overscan images are same size, polynomial fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 2);

    //
    // Overscan images are too big, polynomial fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 1);

    //
    // Overscan images are too big, polynomial fit, bin factor is 2.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 2);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols+2, overscanNumRows+2, NUM_OVERSCANS,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 2);

    //
    // A single overscan image of the same size, polynomial fit, bin factor is 1.
    //
    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, 1,
                  PM_OVERSCAN_ALL,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, 1,
                  PM_OVERSCAN_COLUMNS,
                  PM_FIT_POLYNOMIAL, 1);

    testStatus |= doSubtractOverscansGeneric(imageNumCols, imageNumRows,
                  overscanNumCols, overscanNumRows, 1,
                  PM_OVERSCAN_ROWS,
                  PM_FIT_POLYNOMIAL, 1);

    return(testStatus);
}



#define LOW_COLS 3
#define LOW_ROWS 3

/******************************************************************************
test05(): See test05a() and test05b().
 
We run the tests in test05b() starting with all possible combinations of
sizes.
 
XXX: Must add M-by-N image size tests.
 *****************************************************************************/
int test05()
{
    int testStatus = 0;
    //    testStatus = test05a(NUM_COLS, NUM_ROWS, NUM_COLS, NUM_ROWS);

    //    testStatus|= test05b(LOW_COLS, LOW_ROWS, LOW_COLS, LOW_ROWS);
    //    testStatus|= test05b(LOW_COLS, LOW_ROWS, LOW_COLS, NUM_ROWS);
    //    testStatus|= test05b(LOW_COLS, LOW_ROWS, NUM_COLS, LOW_ROWS);
    //    testStatus|= test05b(LOW_COLS, LOW_ROWS, NUM_COLS, NUM_ROWS);

    //    testStatus|= test05b(LOW_COLS, NUM_ROWS, LOW_COLS, LOW_ROWS);
    //    testStatus|= test05b(LOW_COLS, NUM_ROWS, LOW_COLS, NUM_ROWS);
    //    testStatus|= test05b(LOW_COLS, NUM_ROWS, NUM_COLS, LOW_ROWS);
    //    testStatus|= test05b(LOW_COLS, NUM_ROWS, NUM_COLS, NUM_ROWS);

    //    testStatus|= test05b(NUM_COLS, LOW_ROWS, LOW_COLS, LOW_ROWS);
    //    testStatus|= test05b(NUM_COLS, LOW_ROWS, LOW_COLS, NUM_ROWS);
    //    testStatus|= test05b(NUM_COLS, LOW_ROWS, NUM_COLS, LOW_ROWS);
    //    testStatus|= test05b(NUM_COLS, LOW_ROWS, NUM_COLS, NUM_ROWS);

    //    testStatus|= test05b(NUM_COLS, NUM_ROWS, LOW_COLS, LOW_ROWS);
    //    testStatus|= test05b(NUM_COLS, NUM_ROWS, LOW_COLS, NUM_ROWS);
    //    testStatus|= test05b(NUM_COLS, NUM_ROWS, NUM_COLS, LOW_ROWS);
    testStatus|= test05b(NUM_COLS, NUM_ROWS, NUM_COLS, NUM_ROWS);


    return(testStatus);
}


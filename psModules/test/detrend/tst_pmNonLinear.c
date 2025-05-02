/* @file tst_pmNonLinear.c
 *
 *  @brief Contains the tests for pmNonLinear.c:
 *
 * test00: This code will create a simple polynomial, and call
 * pmNonLinearityPolynomial() for a variety of image sizes [(1, 1), (1,
 * N), (N, 1), (N, N)].  
 *
 * test01: This code will create simple table lookup vectors, and call
 * pmNonLinearityPolynomial() for a variety of image sizes [(1, 1), (1,
 * N), (N, 1), (N, N)].  
 *
 * test02, test03: This code tests the functions with various unallowable
 * input parameters (NULLS) and incorrect vector sizes.
 *
 *  @author GLG, MHPCC
 *
 *  XXX: Add tests in which the lookup file has incorrect number of entries,
 *  and where the data is outside the pmReadout range.
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-01-26 21:10:51 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

#include "psTest.h"
#include "pslib.h"
#include "pmNonLinear.h"
static int test00(void);
static int test01(void);
static int test02(void);
static int test03(void);
testDescription tests[] = {
                              {test00, 000, "pmNonLinearityPolynomial", true, false},
                              {test01, 000, "pmNonLinearityLookup", true, false},
                              {test02, 000, "pmNonLinearityPolynomial(): error/warning conditions", true, false},
                              {test03, 000, "pmNonLinearityLookup(): error/warning conditions", true, false},
                              {NULL}
                          };

#define NUM_ROWS 8
#define NUM_COLS 8
#define LOOKUP_FILENAME ".tmp_tst_pmNonLinearLookupFile"
int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    //
    // We generate a lookup file for future tests.  We should probably remove
    // it when we're done.
    //
    FILE *fp = fopen(LOOKUP_FILENAME, "w");
    ;
    for (psS32 i=0;i<PS_MAX(NUM_COLS, NUM_ROWS)*3;i++) {
        fprintf(fp, "%f %f\n", (float) i, (float) (2 * i));
    }
    fclose(fp);

    //    system("rm LOOKUP_FILENAME");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}

int doNonLinearityPolynomialTest(int numCols, int numRows)
{
    int i;
    int j;
    float actual;
    float expect;
    int testStatus = true;
    psImage *myImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    myReadout->image = myImage;
    psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    myPoly->coeff[1] = 1.0;

    printPositiveTestHeader(stdout, "pmNonLinear", "doNonLinearityPolynomialTest");

    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }

    myReadout = pmNonLinearityPolynomial(myReadout, myPoly);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            expect = psPolynomial1DEval(myPoly, (float) (i + j));
            actual = myReadout->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = false;
            }
        }
    }


    psFree(myReadout);
    psFree(myPoly);
    printFooter(stdout, "pmNonLinear", "doNonLinearityPolynomialTest", true);
    return(testStatus);
}

int test00( void )
{
    int testStatus = 0;

    testStatus |= doNonLinearityPolynomialTest(1, 1);
    testStatus |= doNonLinearityPolynomialTest(NUM_COLS, 1);
    testStatus |= doNonLinearityPolynomialTest(1, NUM_ROWS);
    testStatus |= doNonLinearityPolynomialTest(NUM_COLS, NUM_ROWS);

    return(testStatus);
}

int doNonLinearityLookupTest(int numCols, int numRows)
{
    int i;
    int j;
    float actual;
    float expect;
    int testStatus = true;
    psImage *myImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    myReadout->image = myImage;

    printPositiveTestHeader(stdout, "pmNonLinear", "doNonLinearityLookupTest");
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }

    myReadout = pmNonLinearityLookup(myReadout, LOOKUP_FILENAME);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            expect = (float) (2 * (i + j));
            actual = myReadout->image->data.F32[i][j];
            if (FLT_EPSILON < fabs(expect - actual)) {
                printf("TEST ERROR: image[%d][%d] is %f, should be %f\n", i, j, actual, expect);
                testStatus = false;
            }
        }
    }

    psFree(myReadout);
    printFooter(stdout, "pmNonLinear", "doNonLinearityLookupTest", true);
    return(testStatus);
}

int test01( void )
{
    int testStatus = 0;

    testStatus |= doNonLinearityLookupTest(1, 1);
    testStatus |= doNonLinearityLookupTest(NUM_COLS, 1);
    testStatus |= doNonLinearityLookupTest(1, NUM_ROWS);
    testStatus |= doNonLinearityLookupTest (NUM_COLS, NUM_ROWS);

    return(testStatus);
}

int test02( void )
{
    int i;
    int j;
    int testStatus = true;
    psImage *myImage = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    pmReadout *rc = NULL;
    myReadout->image = myImage;
    psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 2);
    myPoly->coeff[1] = 1.0;

    printPositiveTestHeader(stdout, "pmNonLinear", "Testing bad input parameter conditions.");
    for (i=0;i<NUM_ROWS;i++) {
        for (j=0;j<NUM_COLS;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }

    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityPolynomial() with NULL input readout.  Should generate error, return NULL.\n");
    rc = pmNonLinearityPolynomial(NULL, myPoly);
    if (rc != NULL) {
        printf("TEST ERROR: pmNonLinearityPolynomial() returned a non-NULL pmReadout\n");
        testStatus = false;
    }

    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityPolynomial() with NULL input readout->image.  Should generate error, return NULL.\n");
    psImage *tmpImage = myReadout->image;
    myReadout->image = NULL;
    rc = pmNonLinearityPolynomial(myReadout, myPoly);
    if (rc != NULL) {
        printf("TEST ERROR: pmNonLinearityPolynomial() returned a non-NULL pmReadout\n");
        testStatus = false;
    }
    myReadout->image = tmpImage;

    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityPolynomial() with NULL polynomial.  Should generate error, return NULL.\n");
    rc = pmNonLinearityPolynomial(myReadout, NULL);
    if (rc != NULL) {
        printf("TEST ERROR: pmNonLinearityPolynomial() returned a non-NULL pmReadout\n");
        testStatus = false;
    }

    psFree(myReadout);
    psFree(myPoly);
    return(testStatus);
}


int test03Init(pmReadout *myReadout)
{
    for (psS32 i=0;i<NUM_ROWS;i++) {
        for (psS32 j=0;j<NUM_COLS;j++) {
            myReadout->image->data.F32[i][j] = (float) (i + j);
        }
    }
    return(0);
}

int test03()
{
    int testStatus = true;
    psImage *myImage = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
    pmReadout *myReadout = pmReadoutAlloc(NULL);
    pmReadout *rc = NULL;
    myReadout->image = myImage;

    test03Init(myReadout);
    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityLookup() with NULL input pmReadout.  Should generate error, return NULL.\n");
    rc = pmNonLinearityLookup(NULL, LOOKUP_FILENAME);
    if (rc != NULL) {
        printf("TEST ERROR: pmNonLinearityPolynomial() returned a non-NULL pmReadout\n");
        testStatus = false;
    }

    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityLookup() with NULL input pmReadout->image.  Should generate error, return NULL.\n");
    psImage *tmpImage = myReadout->image;
    myReadout->image = NULL;
    rc = pmNonLinearityLookup(myReadout, LOOKUP_FILENAME);
    if (rc != NULL) {
        printf("TEST ERROR: pmNonLinearityPolynomial() returned a non-NULL pmReadout\n");
        testStatus = false;
    }
    myReadout->image = tmpImage;

    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityLookup() with non-existent lookup file.\n");
    rc = pmNonLinearityLookup(myReadout, "I_DONT_EXIST");
    if (rc == NULL) {
        printf("TEST ERROR: pmNonLinearityPolynomial() returned a NULL pmReadout\n");
        testStatus = false;
    }


    printf("------------------------------------------------------------\n");
    printf("Calling pmNonLinearityLookup() with one pixels outside inFlux range.  Should generate warnings.\n");

    psFree(myReadout);

    printFooter(stdout, "pmNonLinear", "Testing bad input parameter conditions.", true);
    return(testStatus);
}

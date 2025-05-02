/** @file tst_pmObjects.c
 *
 *  @brief Contains the tests for pmObjects.c:
 *
 * test00: This code will test the pmObjects routines.
 *
 *  @author GLG, MHPCC
 *
 * XXX: Must test
 *       pmSourceRoughClass
 *  many others...
 *
 *
 * XXX: Must test output results for many other functions.
 *
 * XXX: There are many cases where row/col can be switched in the code.
 * We must test that here by using non-square images.  All tests
 * in this file should be run with non-square images.
 *
 * XXX: Memory leaks are not being caught.  If I allocated a psVector in these functions
 * and never deallocate, no error is generated.
 *
 * XXX: Much of this file is commented out due to the API changes in rel 7.
 *
 *
Fully Tested:
    pmPeakAlloc()
    pmMomentsAlloc()
Weakly Tested:
    pmSourceMoments()
    most of psObjects.c is not tested
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-24 00:11:59 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include "psTest.h"
#include "pslib.h"
#include "pmObjects.h"
#include "pmModelGroup.h"

#define NUM_ROWS 10
#define NUM_COLS 10
#define ERROR_TOLERANCE 1.0
#define ERROR_TOL 0.001
static int test00(void);
static int test01(void);
static int test02(void);
static int test03(void);
static int test04(void);
static int test05(void);
//static int test06(void);
//static int test07(void);
//static int test08(void);
//static int test09(void);
//static int test15(void);
//static int test16(void);
//static int test20(void);
testDescription tests[] = {
                              {test00, 000, "pmObjects: structure allocators and deallocators", true, false},
                              {test01, 001, "pmObjects: psFindVectorPeaks()", true, false},
                              {test02, 001, "pmObjects: psFindImagePeaks()", true, false},
                              {test03, 001, "pmObjects: pmCullPeaks()", true, false},
                              {test04, 001, "pmObjects: pmSourceLocalSky()", true, false},
                              {test05, 001, "pmObjects: pmSourceMoments()", true, false},
                              //                              {test06, 001, "pmObjects: pmSourceSetPixelsCircle()", true, false},
                              //                              {test07, 001, "pmObjects: pmMin()", true, false},
                              //                              {test08, 001, "pmObjects: pmSourceModelGuess()", true, false},
                              //{test09, 001, "pmObjects: pmSourceContour()", true, false},
                              //{test15, 001, "pmObjects: pmSourceAddModel()", true, false},
                              //{test16, 001, "pmObjects: pmSourceSubModel()", true, false},
                              //{test20, 001, "pmObjects: pmSourceSubModel()", true, false},
                              {NULL}
                          };

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    //
    // We include the function names here in psTraceSetLevel() commands for
    // debugging convenience.  There is no guarantee that this list of functions
    // is complete.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("pmPeakAlloc", 0);
    psTraceSetLevel("pmMomentsAlloc", 0);
    psTraceSetLevel("modelFree", 0);
    psTraceSetLevel("pmModelAlloc", 0);
    psTraceSetLevel("sourceFree", 0);
    psTraceSetLevel("pmSourceAlloc", 0);
    psTraceSetLevel("pmFindVectorPeaks", 0);
    psTraceSetLevel("getRowVectorFromImage", 0);
    psTraceSetLevel("myListAddPeak", 0);
    psTraceSetLevel("pmFindImagePeaks", 0);
    psTraceSetLevel("isItInThisRegion", 0);
    psTraceSetLevel("pmCullPeaks", 0);
    psTraceSetLevel("pmPeaksSubset", 0);
    psTraceSetLevel("pmSourceLocalSky", 0);
    psTraceSetLevel("checkRadius2", 0);
    psTraceSetLevel("pmSourceMoments", 0);
    psTraceSetLevel("pmComparePeakAscend", 0);
    psTraceSetLevel("pmComparePeakDescend", 0);
    psTraceSetLevel("pmSourcePSFClump", 0);
    psTraceSetLevel("pmSourceRoughClass", 0);
    psTraceSetLevel("pmSourceDefinePixels", 0);
    psTraceSetLevel("pmSourceModelGuess", 0);
    psTraceSetLevel("pmModelEval", 0);
    psTraceSetLevel("findValue", 0);
    psTraceSetLevel("pmSourceContour", 0);
    psTraceSetLevel("pmSourceFitModel_v5", 0);
    psTraceSetLevel("pmSourceFitModel", 0);
    psTraceSetLevel("p_pmSourceAddOrSubModel", 0);
    psTraceSetLevel("pmSourceAddModel", 0);
    psTraceSetLevel("pmSourceSubModel", 0);

    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}

/******************************************************************************
test00(): Test the various allocators and deallocators.
 *****************************************************************************/
int test00( void )
{
    bool testStatus = true;
    psTraceSetLevel(".", 0);

    printf("Testing pmPeakAlloc()...\n");
    pmPeak *tmpPeak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
    if (tmpPeak == NULL) {
        printf("TEST ERROR: pmPeakAlloc() returned a NULL pmPeak\n");
        testStatus = false;
    } else {
        if (tmpPeak->x != 1) {
            printf("TEST ERROR: pmPeakAlloc() improperly set pmPeak->x\n");
            testStatus = false;
        }
        if (tmpPeak->y != 2) {
            printf("TEST ERROR: pmPeakAlloc() improperly set pmPeak->y\n");
            testStatus = false;
        }
        if (tmpPeak->counts != 3.0) {
            printf("TEST ERROR: pmPeakAlloc() improperly set pmPeak->counts\n");
            testStatus = false;
        }
        if (tmpPeak->class != PM_PEAK_LONE) {
            printf("TEST ERROR: pmPeakAlloc() improperly set pmPeak->class\n");
            testStatus = false;
        }
    }
    psFree(tmpPeak);

    printf("Testing pmMomentsAlloc()...\n");
    pmMoments *tmpMoments = pmMomentsAlloc();
    if (tmpMoments == NULL) {
        printf("TEST ERROR: pmMomentsAlloc() returned a NULL pmMoments\n");
        testStatus = false;
    } else {
        if ((fabs(tmpMoments->x-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->y-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->Sx-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->Sy-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->Sxy-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->Sum-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->Peak-0.0) > ERROR_TOL) ||
                (fabs(tmpMoments->Sky-0.0) > ERROR_TOL) ||
                (tmpMoments->nPixels != 0)) {
            printf("TEST ERROR: pmMomentsAlloc() did not properly initialize the pmMoments structure.\n");
            printf("    tmpMoments->x is %f\n", tmpMoments->x);
            printf("    tmpMoments->y is %f\n", tmpMoments->y);
            printf("    tmpMoments->Sx is %f\n", tmpMoments->Sx);
            printf("    tmpMoments->Sy is %f\n", tmpMoments->Sy);
            printf("    tmpMoments->Sxy is %f\n", tmpMoments->Sxy);
            printf("    tmpMoments->Sum is %f\n", tmpMoments->Sum);
            printf("    tmpMoments->Peak is %f\n", tmpMoments->Peak);
            printf("    tmpMoments->Sky is %f\n", tmpMoments->Sky);
            printf("    tmp    Moments->nPixels is %d\n", tmpMoments->nPixels);
            testStatus = false;
        }
    }
    psFree(tmpMoments);


    //
    // Loop through each type of model
    //
    psS32 i = 0;
    while (0 != pmModelClassParameterCount(i)) {
        printf("Testing pmModelAlloc(%s)...\n", pmModelClassGetName(0));
        pmModel *tmpModel = pmModelAlloc(i);
        if (tmpModel == NULL) {
            printf("TEST ERROR: pmModelAlloc(%s) returned a NULL pmModel\n", pmModelClassGetName(0));
            testStatus = false;
        } else {

            /* XXX: Should we test that the members were set correctly?
                        if ((tmpModel->params->n != 7) || (tmpModel->dparams->n != 7)) {
                            printf("TEST ERROR: pmModelAlloc(PS_MODEL_GAUSS) allocated an incorrect number of params (%ld, %ld)\n",
                                   tmpModel->params->n, tmpModel->dparams->n);
                            testStatus = false;
                        } else {
                            for (psS32 i = 0 ; i < 7 ; i++) {
                                if ((tmpModel->params->data.F32[i] != 0.0) ||
                                        (tmpModel->dparams->data.F32[i] != 0.0)) {
                                    printf("TEST ERROR: pmModelAlloc(PS_MODEL_GAUSS) did not ininitialize the params/dparams array to 0.0.\n");
                                    testStatus = false;
                                }
                            }
                        }
            */
        }
        psFree(tmpModel);
        i++;
    }


    pmSource *tmpSource = pmSourceAlloc();
    if (tmpSource == NULL) {
        printf("TEST ERROR: pmSourceAlloc() returned a NULL pmSource\n");
        testStatus = false;
    }
    psFree(tmpSource);

    return(testStatus);
}

/******************************************************************************
test01(): we first test pmFindVectorPeaks() with a variety of bad input
parameters.  Then we test it with a simple vector both 1- and multi-elements.
 *****************************************************************************/
#define TST01_VECTOR_LENGTH 10
bool test_pmFindVectorPeaks(int n)
{
    bool testStatus = true;
    psVector *inData = psVectorAlloc(n, PS_TYPE_F32);
    inData->n = inData->nalloc;
    psVector *outData = NULL;

    printf("-------------- Calling test_pmFindVectorPeaks on an %d size vector. --------------\n", n);
    //
    // Test first pixel peak.
    //
    printf("Test pmFindVectorPeaks() with a first-element peak.\n");
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (n-i);
    }
    inData->data.F32[0] = (float) n;

    outData= pmFindVectorPeaks(inData, 0.0);
    if (outData == NULL) {
        printf("TEST ERROR: pmFindVectorPeaks returned a NULL psVector.\n");
        testStatus = false;
    } else {
        if (outData->n != 1) {
            printf("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        if (outData->data.U32[0] != 0) {
            printf("TEST ERROR: Did not find peak at element 0.\n");
            testStatus = false;
        }
        psFree(outData);
    }

    //
    // Test first pixel peak, large threshold
    //
    printf("Test pmFindVectorPeaks() with a first-element peak, large threshold.\n");
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (n-i);
    }
    inData->data.F32[0] = (float) n;

    outData= pmFindVectorPeaks(inData, (float) (n*n));
    if (outData == NULL) {
        printf("TEST ERROR: pmFindVectorPeaks returned a NULL psVector.\n");
        testStatus = false;
    } else {

        if (outData->n != 0) {
            printf("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        psFree(outData);

        // Skip remaining tests if the input vector has length 1.
        if (n == 1) {
            psFree(inData);
            return(testStatus);
        }
    }

    //
    // Test last pixel peak.
    //
    printf("Test pmFindVectorPeaks() with a last-element peak.\n");
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (i);
    }
    inData->data.F32[n-1] = (float) n;

    outData= pmFindVectorPeaks(inData, 0.0);
    if (outData == NULL) {
        printf("TEST ERROR: pmFindVectorPeaks returned a NULL psVector.\n");
        testStatus = false;
    } else {

        if (outData->n != 1) {
            printf("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        if (outData->data.U32[0] != n-1) {
            printf("TEST ERROR: Did not find peak at element %d.\n", n-1);
            testStatus = false;
        }
        psFree(outData);
    }

    //
    // Test last pixel peak, large threshold.
    //
    printf("Test pmFindVectorPeaks() with a last-element peak, large threshold.\n");
    for (psS32 i = 0 ; i < n ; i++) {
        inData->data.F32[i] = (float) (i);
    }
    inData->data.F32[n-1] = (float) n;

    outData= pmFindVectorPeaks(inData, (float) (n*n));
    if (outData == NULL) {
        printf("TEST ERROR: pmFindVectorPeaks returned a NULL psVector.\n");
        testStatus = false;
    } else {

        if (outData->n != 0) {
            printf("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        psFree(outData);
    }

    //
    // Test interior peaks.
    // Set all even number elements to be peaks.
    //
    printf("Test pmFindVectorPeaks() with all even-numbered elements peak.\n");
    for (psS32 i = 0 ; i < n ; i++) {
        if (0 == i%2) {
            inData->data.F32[i] = (float) (2 * i);
        } else {
            inData->data.F32[i] = (float) (i);
        }
    }
    inData->data.F32[0] = (float) n;


    outData= pmFindVectorPeaks(inData, 0.0);
    if (outData == NULL) {
        printf("TEST ERROR: pmFindVectorPeaks returned a NULL psVector.\n");
        testStatus = false;
    } else {

        if (outData->n != n/2) {
            printf("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }

        for (psS32 i = 0 ; i < outData->n ; i++) {
            if (outData->data.U32[i] != (2 * i)) {
                printf("TEST ERROR: the %d-th peak is element number %d\n", i, outData->data.U32[i]);
                testStatus = false;
            }
        }
        psFree(outData);
    }

    //
    // Test interior peaks, with threshold = n*n.
    // Should generate an empty output psVector.
    //
    printf("Test pmFindVectorPeaks() with all even-numbered elements peak, large threshold.\n");
    outData= pmFindVectorPeaks(inData, (float) (n*n));
    if (outData == NULL) {
        printf("TEST ERROR: pmFindVectorPeaks returned a NULL psVector.\n");
        testStatus = false;
    } else {

        if (outData->n != 0) {
            printf("TEST ERROR: outData->n is %ld\n", outData->n);
            testStatus = false;
        }
        psFree(outData);
    }

    psFree(inData);
    return(testStatus);
}

int test01( void )
{
    bool testStatus = true;
    psVector *tmpVec = NULL;
    psVector *tmpVecF64 = psVectorAlloc(TST01_VECTOR_LENGTH, PS_TYPE_F64);
    psVector *tmpVecEmpty = psVectorAlloc(0, PS_TYPE_F32);
    tmpVecF64->n = tmpVecF64->nalloc;
    tmpVecEmpty->n = tmpVecEmpty->nalloc;

    psTraceSetLevel(".", 0);

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmFindVectorPeaks with NULL psVector.  Should generate error and return NULL.\n");
    tmpVec = pmFindVectorPeaks(NULL, 0.0);
    if (tmpVec != NULL) {
        printf("TEST ERROR: pmFindVectorPeaks() returned a non-NULL psVector.\n");
        testStatus = false;
        psFree(tmpVec);
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmFindVectorPeaks with empty psVector.  Should generate error and return NULL.\n");
    tmpVec = pmFindVectorPeaks(tmpVecEmpty, 0.0);
    if (tmpVec != NULL) {
        printf("TEST ERROR: pmFindVectorPeaks() returned a non-NULL psVector.\n");
        testStatus = false;
        psFree(tmpVec);
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmFindVectorPeaks with PS_TYPE_F64 psVector.  Should generate error and return NULL.\n");
    tmpVec = pmFindVectorPeaks(tmpVecF64, 0.0);
    if (tmpVec != NULL) {
        printf("TEST ERROR: pmFindVectorPeaks() returned a non-NULL psVector.\n");
        testStatus = false;
        psFree(tmpVec);
    }
    testStatus&= test_pmFindVectorPeaks(1);
    testStatus&= test_pmFindVectorPeaks(TST01_VECTOR_LENGTH);

    psFree(tmpVecF64);
    psFree(tmpVecEmpty);
    return(testStatus);
}

/******************************************************************************
test02():
// XXX: Must test flat peaks.
// XXX: test 1-by-n and n-by-1 images.
 *****************************************************************************/
#define TST02_NUM_ROWS 5
#define TST02_NUM_COLS 5
bool test_pmFindImagePeaks(int numRows, int numCols)
{
    printf("-------------- Calling test_pmFindImagePeaks on an %d-by-%d image. --------------\n", numRows, numCols);
    //    if ((numRows < 4) || (numCols < 4)) {
    //        printf("WARNING: Don't call this test with a smaller than 4-by-4 image.\n");
    //        return(true);
    //    }
    bool testStatus = true;
    psImage *inData = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psArray *outData = NULL;

    //
    // Initialize test image.
    //
    for (psS32 i = 0 ; i < numRows ; i++) {
        for (psS32 j = 0 ; j < numCols ; j++) {
            inData->data.F32[i][j] = PS_SQR(i - numRows/2) + PS_SQR(j-numCols/2);
        }
    }
    //
    // Set corner and center pixels as peaks.
    //
    inData->data.F32[0][0] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[0][numCols-1] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[numRows-1][0] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[numRows-1][numCols-1] = PS_SQR(numRows) + PS_SQR(numCols);
    inData->data.F32[numRows/2][numCols/2] = PS_SQR(numRows) + PS_SQR(numCols);

    //
    // Print image.
    //
    for (psS32 i = 0 ; i < numRows ; i++) {
        for (psS32 j = 0 ; j < numCols ; j++) {
            printf("(%.1f) ", inData->data.F32[i][j]);
        }
        printf("\n");
    }

    //
    // Call pmFindImagePeaks() with a threshold of 0.0.
    //
    outData = pmFindImagePeaks(inData, 0.0);

    if (outData == NULL) {
        printf("TEST ERROR: pmFindImagePeaks returned a NULL psList.\n");
        testStatus = false;
    } else {
        psS32 expectedNumPeaks;
        if ((numRows == 1) && (numCols == 1)) {
            expectedNumPeaks = 1;
        } else if ((numRows == 1) || (numCols == 1)) {
            expectedNumPeaks = 3;
        } else {
            expectedNumPeaks = 5;
        }
        if (outData->n != expectedNumPeaks) {
            printf("TEST ERROR: pmFindImagePeaks found %ld peaks (should be %d)\n", outData->n, expectedNumPeaks);
            testStatus = false;
        }

        // HEY: verify
        for (psS32 i = 0 ; i < outData->n ; i++) {
            pmPeak *tmpPeak = (pmPeak *) outData->data[i];
            if (((tmpPeak->x == 0) && (tmpPeak->y == 0)) ||
                    ((tmpPeak->x == 0) && (tmpPeak->y == numRows-1)) ||
                    ((tmpPeak->x == numCols-1) && (tmpPeak->y == 0)) ||
                    ((tmpPeak->x == numCols-1) && (tmpPeak->y == numRows-1))) {
                if (!((tmpPeak->class & PM_PEAK_LONE) || (tmpPeak->class & PM_PEAK_EDGE))) {
                    printf("TEST ERROR: (0) peak at (%d, %d) (%f) ->class set improperly (0x%x).",
                           tmpPeak->y, tmpPeak->x, tmpPeak->counts, tmpPeak->class);
                    printf(" should be (0x%x or 0x%x).\n", PM_PEAK_LONE, PM_PEAK_EDGE);
                    testStatus = false;
                }
            } else if ((tmpPeak->x == numCols/2) && (tmpPeak->y == numRows/2)) {
                if (tmpPeak->class != PM_PEAK_LONE) {
                    printf("TEST ERROR: (1) peak at (%d, %d) (%f) ->class set improperly (0x%x).\n",
                           tmpPeak->y, tmpPeak->x, tmpPeak->counts, tmpPeak->class);
                    printf(" should be (0x%x).\n", PM_PEAK_LONE);
                    testStatus = false;
                }
            } else {
                printf("TEST ERROR: Peak at (%d, %d) (%f)\n", tmpPeak->y, tmpPeak->x, tmpPeak->counts);
                testStatus = false;
            }
        }
    }

    psFree(inData);
    psFree(outData);
    return(testStatus);
}

int test02( void )
{
    bool testStatus = true;
    psArray *tmpArray = NULL;
    psImage *tmpImageF64 = psImageAlloc(TST02_NUM_ROWS, TST02_NUM_COLS, PS_TYPE_F64);
    psImage *tmpImageEmpty = psImageAlloc(0, 0, PS_TYPE_F32);

    psTraceSetLevel(".", 0);

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmFindImagePeaks with NULL psImage.  Should generate error and return NULL.\n");
    tmpArray = pmFindImagePeaks(NULL, 0.0);
    if (tmpArray != NULL) {
        printf("TEST ERROR: pmFindImagePeaks() returned a non-NULL psImage.\n");
        testStatus = false;
        psFree(tmpArray);
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmFindImagePeaks with empty psImage.  Should generate error and return NULL.\n");
    tmpArray = pmFindImagePeaks(tmpImageEmpty, 0.0);
    if (tmpArray != NULL) {
        printf("TEST ERROR: pmFindImagePeaks() returned a non-NULL psImage.\n");
        testStatus = false;
        psFree(tmpArray);
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmFindImagePeaks with PS_TYPE_F64 psImage.  Should generate error and return NULL.\n");
    tmpArray = pmFindImagePeaks(tmpImageF64, 0.0);
    if (tmpArray != NULL) {
        printf("TEST ERROR: pmFindImagePeaks() returned a non-NULL psImage.\n");
        testStatus = false;
        psFree(tmpArray);
    }
    printf("----------------------------------------------------------------------------------\n");
    //    testStatus&= test_pmFindImagePeaks(1, 1);
    //    testStatus&= test_pmFindImagePeaks(2, 5);
    //    testStatus&= test_pmFindImagePeaks(5, 2);
    // HEY: add code for small images
    //    testStatus&= test_pmFindImagePeaks(1, 1);
    //    testStatus&= test_pmFindImagePeaks(1, 8);
    //    testStatus&= test_pmFindImagePeaks(8, 1);
    testStatus&= test_pmFindImagePeaks(TST02_NUM_ROWS,   TST02_NUM_COLS);
    testStatus&= test_pmFindImagePeaks(2*TST02_NUM_ROWS, TST02_NUM_COLS);
    testStatus&= test_pmFindImagePeaks(TST02_NUM_ROWS,   2*TST02_NUM_COLS);


    psFree(tmpImageF64);
    psFree(tmpImageEmpty);
    return(testStatus);
}

/******************************************************************************
test03(): We first test pmCullPeaks() with various NULL and unallowable input
parameters.  Then we generate a list of peaks and test that pmCullPeaks()
removes them correctly.
 *****************************************************************************/
int test03( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
    psArray *outData = NULL;

    /* XXX: Modify for new pmCullPeaks()
            printf("----------------------------------------------------------------------------------\n");
            printf("Calling pmCullPeaks with NULL psList.  Should generate error and return NULL.\n");
            outData = pmCullPeaks(NULL, 0.0, NULL);
            if (outData != NULL) {
                printf("TEST ERROR: pmCulPeaks() returned a non-NULL psList.\n");
                testStatus = false;
        }
    */

    //
    // Set peaks in input image.  All even-column and even-row pixels are
    // set non-zero, all other pixels are set to zero.
    //
    psS32 numPeaksOrig = 0;
    for (psS32 i = 0 ; i < NUM_ROWS ; i++) {
        for (psS32 j = 0 ; j < NUM_COLS ; j++) {
            if ((0 == i%2) && (0 == j%2)) {
                imgData->data.F32[i][j] = (float) (i + 10);
                numPeaksOrig++;
            } else {
                imgData->data.F32[i][j] = 0.0;
            }
        }
    }
    for (psS32 i = 0 ; i < NUM_ROWS ; i++) {
        for (psS32 j = 0 ; j < NUM_COLS ; j++) {
            printf("(%.1f) ", imgData->data.F32[i][j]);
        }
        printf("\n");
    }
    printf("Set %d peaks\n", numPeaksOrig);

    //
    // Call pmCullPeaks() with HUGE maxValue and NULL psRegion.  Should not
    // remove any peaks.
    //
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmCullPeaks with large maxValue and NULL psRegion.\n");
    outData = pmFindImagePeaks(imgData, 0.0);
    /* XXX: Modify for new pmCullPeaks
        outData = pmCullPeaks(outData, PS_MAX_F32, NULL);

        if (outData == NULL) {
            printf("TEST ERROR: pmCullPeaks() returned a non-NULL psList.\n");
            testStatus = false;
            return(testStatus);
        }
        if (outData->n != numPeaksOrig) {
            printf("TEST ERROR (0): pmCullPeaks incorrectly removed peaks\n");
            printf("The pmCullPeaks() output had %d peaks, should have had %d peaks.\n", outData->n, numPeaksOrig);
            testStatus = false;
        }
    */
    psFree(outData);

    //
    // Call pmCullPeaks() with TINY maxValue and NULL psRegion.  Should
    // remove all peaks.
    //
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmCullPeaks with tiny maxValue and NULL psRegion.\n");
    outData = pmFindImagePeaks(imgData, 0.0);
    printf("pmFindImagePeaks found %ld peaks\n", outData->n);
    /* XXX: Modify for new pmCullPeaks
        outData = pmCullPeaks(outData, 0.0, NULL);

        if (outData == NULL) {
            printf("TEST ERROR: pmCullPeaks() returned a non-NULL psList.\n");
            testStatus = false;
            return(testStatus);
        }
        if (outData->n != 0) {
            printf("TEST ERROR (1): pmCullPeaks incorrectly removed peaks\n");
            printf("The pmCullPeaks() output had %d peaks, should have had %d peaks.\n", outData->n, 0);
            testStatus = false;
        }
        psFree(outData);
    */

    //
    // Call pmCullPeaks() with HUGE maxValue and disjoint psRegion.  Should
    // not remove any peaks.
    //
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmCullPeaks with large maxValue and disjoint psRegion.\n");
    outData = pmFindImagePeaks(imgData, 0.0);
    printf("pmFindImagePeaks found %ld peaks\n", outData->n);
    psRegion tmpRegion = psRegionSet(10000.0, 20000.0, 10000.0, 20000.0);

    /* XXX: Modify for new pmCullPeaks
        outData = pmCullPeaks(outData, PS_MAX_F32, tmpRegion);

        if (outData == NULL) {
            printf("TEST ERROR: pmCullPeaks() returned a non-NULL psList.\n");
            testStatus = false;
            return(testStatus);
        }
        if (outData->n != numPeaksOrig) {
            printf("TEST ERROR (2): pmCullPeaks incorrectly removed peaks\n");
            printf("The pmCullPeaks() output had %d peaks, should have had %d peaks.\n", outData->n, numPeaksOrig);
            testStatus = false;
        }
    */
    psFree(outData);

    //
    // Call pmCullPeaks() with HUGE maxValue and non-disjoint psRegion.  Should
    // remove all peaks.
    //
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmCullPeaks with large maxValue and non-disjoint psRegion.\n");
    outData = pmFindImagePeaks(imgData, 0.0);
    printf("pmFindImagePeaks found %ld peaks\n", outData->n);
    tmpRegion = psRegionSet(-PS_MAX_F32, PS_MAX_F32, -PS_MAX_F32, PS_MAX_F32);
    /* XXX: Modify for new pmCullPeaks
        outData = pmCullPeaks(outData, PS_MAX_F32, tmpRegion);

        if (outData == NULL) {
            printf("TEST ERROR: pmCullPeaks() returned a non-NULL psList.\n");
            testStatus = false;
            return(testStatus);
        }
        if (outData->n != 0) {
            printf("TEST ERROR (3): pmCullPeaks incorrectly removed peaks\n");
            printf("The pmCullPeaks() output had %d peaks, should have had %d peaks.\n", outData->n, 0);
            testStatus = false;
        }
    */
    psFree(outData);

    printf("----------------------------------------------------------------------------------\n");
    psFree(imgData);
    return(testStatus);
}



































































#define TST04_NUM_ROWS 100
#define TST04_NUM_COLS 100
#define TST04_SKY 20.0
#define TST04_INNER_RADIUS 3
#define TST04_OUTER_RADIUS 5
/******************************************************************************
test04(): We first test pmSourceLocalSky() with various NULL and unallowable
input parameters.
 
XXX: Should we produce tests with boundary numbers for the inner/outer radius?
 
XXX: Call this with varying sizes for the image.
 *****************************************************************************/
int test04( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(TST04_NUM_COLS, TST04_NUM_ROWS, PS_TYPE_F32);
    psImageInit(imgData, TST04_SKY);
    psImage *imgMask = psImageAlloc(TST04_NUM_COLS, TST04_NUM_ROWS, PS_TYPE_U8);
    psImageInit(imgMask, 0);
    //    psImage *imgMaskS8 = psImageAlloc(TST04_NUM_COLS, TST04_NUM_ROWS, PS_TYPE_S8);
    //    psImageInit(imgMaskS8, 0);
    psImage *imgDataF64 = psImageAlloc(TST04_NUM_COLS, TST04_NUM_ROWS, PS_TYPE_F64);
    psImageInit(imgDataF64, 0.0);
    pmPeak *tmpPeak = pmPeakAlloc((psF32) (TST04_NUM_ROWS / 2),
                                  (psF32) (TST04_NUM_COLS / 2),
                                  200.0,
                                  PM_PEAK_LONE);
    pmSource *tmpSource = pmSourceAlloc();
    tmpSource->pixels = imgData;
    tmpSource->mask = imgMask;
    tmpSource->peak = tmpPeak;

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceLocalSky with NULL tmpSource.  Should generate error and return FALSE.\n");
    bool rc = pmSourceLocalSky(NULL, PS_STAT_SAMPLE_MEAN, 10.0);
    if (rc != false) {
        printf("TEST ERROR: pmSourceLocalSky() returned a non-FALSE pmSource.\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceLocalSky with Radius<0.0.  Should generate error and return FALSE.\n");
    rc = pmSourceLocalSky(tmpSource, PS_STAT_SAMPLE_MEAN, -10.0);
    if (rc != false) {
        printf("TEST ERROR: pmSourceLocalSky() returned a non-FALSE pmSource.\n");
        testStatus = false;
    }

    //
    // XXX: The following code should be a separate function, and we should call it
    // with a variety of image sizes, peaks.
    //
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceLocalSky with valid data.\n");
    tmpPeak->x = (psF32) (TST04_NUM_ROWS / 2);
    tmpPeak->y = (psF32) (TST04_NUM_COLS / 2);
    rc = pmSourceLocalSky(tmpSource, PS_STAT_SAMPLE_MEAN, 10.0);

    if (rc == false) {
        printf("TEST ERROR: pmSourceLocalSky() returned a FALSE pmSource.\n");
        testStatus = false;
    } else {
        if (tmpSource->peak == NULL) {
            printf("TEST ERROR: pmSourceLocalSky() returned a NULL pmSource->peak.\n");
            testStatus = false;
        } else {
            if (tmpSource->peak->x != tmpPeak->x) {
                printf("TEST ERROR: pmSourceLocalSky() pmSource->peak->x was %d, should have been %d.\n",
                       tmpSource->peak->x, tmpPeak->x);
                testStatus = false;
            }

            if (tmpSource->peak->y != tmpPeak->y) {
                printf("TEST ERROR: pmSourceLocalSky() pmSource->peak->y was %d, should have been %d.\n",
                       tmpSource->peak->y, tmpPeak->y);
                testStatus = false;
            }

            if (tmpSource->peak->counts != tmpPeak->counts) {
                printf("TEST ERROR: pmSourceLocalSky() pmSource->peak->counts was %f, should have been %f.\n",
                       tmpSource->peak->counts, tmpPeak->counts);
                testStatus = false;
            }

            if (tmpSource->peak->class != tmpPeak->class) {
                printf("TEST ERROR: pmSourceLocalSky() pmSource->peak->class was %d, should have been %d.\n",
                       tmpSource->peak->class, tmpPeak->class);
                testStatus = false;
            }
        }

        if (tmpSource->moments == NULL) {
            printf("TEST ERROR: pmSourceLocalSky() returned a NULL pmSource->moments.\n");
            testStatus = false;
        } else {
            if (tmpSource->moments->Sky != TST04_SKY) {
                printf("TEST ERROR: pmSourceLocalSky() pmSource->moments->Sky was %f, should have been %f.\n", tmpSource->moments->Sky, TST04_SKY);
                testStatus = false;
            }
        }
    }

    printf("----------------------------------------------------------------------------------\n");
    psFree(tmpSource);
    //    psFree(imgData);
    //    psFree(imgDataF64);
    //    psFree(imgMask);
    //    psFree(imgMaskS8);
    return(testStatus);
}

#define TST05_NUM_ROWS 100
#define TST05_NUM_COLS 100
#define TST05_SKY 20.0
#define TST05_INNER_RADIUS 3
#define TST05_OUTER_RADIUS 5
/******************************************************************************
test05(): We first test pmSourceMoments() with various NULL and unallowable
input parameters.
 
XXX: Should we produce tests with boundary numbers for the inner/outer radius?
 
XXX: Call this with varying sizes for the image.
 
XXX: The actual values of the moments are not tested.
 *****************************************************************************/
int test05( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(TST04_NUM_COLS, TST04_NUM_ROWS, PS_TYPE_F32);
    psImageInit(imgData, TST04_SKY);
    psImage *imgMask = psImageAlloc(TST04_NUM_COLS, TST04_NUM_ROWS, PS_TYPE_U8);
    psImageInit(imgMask, 0);
    pmPeak *tmpPeak = pmPeakAlloc((psF32) (TST04_NUM_ROWS / 2),
                                  (psF32) (TST04_NUM_COLS / 2),
                                  200.0,
                                  PM_PEAK_LONE);
    pmSource *tmpSource = pmSourceAlloc();
    tmpSource->pixels = imgData;
    tmpSource->mask = imgMask;
    tmpSource->peak = tmpPeak;
    psBool rc = pmSourceLocalSky(tmpSource, PS_STAT_SAMPLE_MEAN, 10.0);

    if (rc == false) {
        printf("TEST ERROR: pmSourceLocalSky() returned a FALSE pmSource.\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceMoments with NULL pmSource.  Should generate error and return FALSE.\n");
    rc = pmSourceMoments(NULL, 10.0);
    if (rc != false) {
        printf("TEST ERROR: pmSourceMoments() returned TRUE.\n");
        testStatus = false;
    }
    // XXX: test with pmSource->peaks NULL
    // XXX: test with pmSource->pixels NULL
    // XXX: test with pmSource->mask NULL

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceMoments with radius < 0.0.  Should generate error and return FALSE.\n");
    rc = pmSourceMoments(tmpSource, -10.0);
    if (rc != false) {
        printf("TEST ERROR: pmSourceMoments() returned TRUE.\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------------\n");
    psFree(tmpSource);
    return(testStatus);

}

#define TST09_NUM_ROWS 70
#define TST09_NUM_COLS 70
#define TST09_SKY 5.0
#define TST09_INNER_RADIUS 3
#define TST09_OUTER_RADIUS 10
#define LEVEL (TST09_SKY + 10.0)
/******************************************************************************
test09(): We first test pmSourceContour() with various NULL and unallowable
input parameters.
 
XXX: We don't verify the numbers.
 *****************************************************************************/
int test09( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(TST09_NUM_COLS, TST09_NUM_ROWS, PS_TYPE_F32);
    psImageInit(imgData, TST09_SKY);
    psImage *imgMask = psImageAlloc(TST09_NUM_COLS, TST09_NUM_ROWS, PS_TYPE_U8);
    psImageInit(imgMask, 0);
    pmPeak *tmpPeak = pmPeakAlloc((psF32) (TST09_NUM_ROWS / 2),
                                  (psF32) (TST09_NUM_COLS / 2),
                                  200.0,
                                  PM_PEAK_LONE);
    pmSource *tmpSource = pmSourceAlloc();
    tmpSource->pixels = imgData;
    tmpSource->mask = imgMask;
    tmpSource->peak = tmpPeak;
    psBool rc = pmSourceLocalSky(tmpSource, PS_STAT_SAMPLE_MEAN, 10.0);
    if (rc == false) {
        printf("TEST ERROR: pmSourceLocalSky() returned a FALSE pmSource.\n");
        testStatus = false;
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceContour with NULL pmSource .  Should generate error, return FALSE.\n");
    rc = pmSourceContour(NULL, imgData, LEVEL, PS_CONTOUR_CRUDE);
    if (rc != false) {
        printf("TEST ERROR: pmSourceContour() returned TRUE.\n");
        testStatus = false;
        psFree(rc);
    }

    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceContour with NULL psImage .  Should generate error, return FALSE.\n");
    rc = pmSourceContour(tmpSource, NULL, LEVEL, PS_CONTOUR_CRUDE);
    if (rc != FALSE) {
        printf("TEST ERROR: pmSourceContour() returned TRUE.\n");
        testStatus = false;
        psFree(rc);
    }

    //
    // XXX: pmSourceContour() has a problem with contour tops/bottoms.
    // Must correct this.
    //
    if (1) {
        printf("----------------------------------------------------------------------------------\n");
        printf("Calling pmSourceContour with acceptable data.\n");
        printf("NOTE: must figure out the parameters for this test to be meaningful.\n");
        tmpSource->modelPSF->params->data.F32[0] = TST09_SKY;
        tmpSource->modelPSF->params->data.F32[1] = 15.0;
        tmpSource->modelPSF->params->data.F32[2] = (psF32) (TST09_NUM_ROWS / 2);
        tmpSource->modelPSF->params->data.F32[3] = (psF32) (TST09_NUM_COLS / 2);
        tmpSource->modelPSF->params->data.F32[4] = 2.0;
        tmpSource->modelPSF->params->data.F32[5] = 2.0;
        tmpSource->modelPSF->params->data.F32[6] = 2.0;
        rc = pmSourceContour(tmpSource, imgData, LEVEL, PS_CONTOUR_CRUDE);
        if (rc == false) {
            printf("TEST ERROR: pmSourceContour() returned FALSE.\n");
            testStatus = false;
        } else {
            psFree(rc);
        }
    }

    psFree(tmpSource);
    return(testStatus);
}

#define TST15_NUM_ROWS 100
#define TST15_NUM_COLS 100
#define TST15_SKY 10.0
#define TST15_INNER_RADIUS 3
#define TST15_OUTER_RADIUS 5
/******************************************************************************
test15(): We first test pmSourceAddModel() with various NULL and unallowable
input parameters.
 
XXX: We don't verify the numbers.
 *****************************************************************************/
/*
int test15( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(TST15_NUM_COLS, TST15_NUM_ROWS, PS_TYPE_F32);
    psImageInit(imgData, TST15_SKY);
    psImage *imgMask = psImageAlloc(TST15_NUM_COLS, TST15_NUM_ROWS, PS_TYPE_U8);
    psImageInit(imgMask, 0);
    pmPeak *tmpPeak = pmPeakAlloc((psF32) (TST15_NUM_ROWS / 2),
                                  (psF32) (TST15_NUM_COLS / 2),
                                  200.0,
                                  PM_PEAK_LONE);
    pmSource *tmpSource = pmSourceAlloc();
    tmpSource->pixels = imgData;
    tmpSource->mask = imgMask;
    tmpSource->peak = tmpPeak;
    psBool rc = pmSourceLocalSky(tmpSource, PS_STAT_SAMPLE_MEAN, 10.0);
    if (rc == false) {
        printf("TEST ERROR: pmSourceLocalSky() returned a FALSE pmSource.\n");
        testStatus = false;
    }
 
 
    tmpSource->modelPSF = pmModelAlloc(PS_MODEL_GAUSS);
    tmpSource->modelPSF->params->data.F32[0] = 5.0;
    tmpSource->modelPSF->params->data.F32[1] = 70.0;
    tmpSource->modelPSF->params->data.F32[2] = (psF32) (TST15_NUM_ROWS / 2);
    tmpSource->modelPSF->params->data.F32[3] = (psF32) (TST15_NUM_COLS / 2);
    tmpSource->modelPSF->params->data.F32[4] = 1.0;
    tmpSource->modelPSF->params->data.F32[5] = 1.0;
    tmpSource->modelPSF->params->data.F32[6] = 2.0;
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceAddModel with NULL psImage.  Should generate error, return FALSE.\n");
    rc = pmSourceAddModel(NULL, tmpSource, true);
    if (rc == true) {
        printf("TEST ERROR: pmSourceAddModel() returned TRUE.\n");
        testStatus = false;
    }
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceAddModel with NULL psSrc.  Should generate error, return FALSE.\n");
    rc = pmSourceAddModel(imgData, NULL, true);
    if (rc == true) {
        printf("TEST ERROR: pmSourceAddModel() returned TRUE.\n");
        testStatus = false;
    }
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceAddModel with acceptable data.\n");
    rc = pmSourceAddModel(imgData, tmpSource, true);
    if (rc != true) {
        printf("TEST ERROR: pmSourceAddModel() returned FALSE.\n");
        testStatus = false;
    }
 
    psFree(tmpSource);
    psFree(imgData);
    return(testStatus);
}
*/

#define TST16_NUM_ROWS 100
#define TST16_NUM_COLS 100
#define TST16_SKY 10.0
#define TST16_INNER_RADIUS 3
#define TST16_OUTER_RADIUS 5
/******************************************************************************
test16(): We first test pmSourceSubModel() with various NULL and unallowable
input parameters.
 
XXX: We don't verify the numbers.
 *****************************************************************************/
/*
int test16( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(TST16_NUM_COLS, TST16_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < imgData->numRows; i++) {
        for (psS32 j = 0 ; j < imgData->numCols; j++) {
            imgData->data.F32[i][j] = TST16_SKY;
        }
    }
    pmSource *tmpSource = NULL;
    psBool rc = false;
 
    pmPeak *tmpPeak = pmPeakAlloc((psF32) (TST16_NUM_ROWS / 2),
                                  (psF32) (TST16_NUM_COLS / 2),
                                  200.0,
                                  PM_PEAK_LONE);
 
    printf("Calling pmSourceLocalSky with valid data.\n");
    tmpPeak->x = (psF32) (TST16_NUM_ROWS / 2);
    tmpPeak->y = (psF32) (TST16_NUM_COLS / 2);
    tmpSource = pmSourceLocalSky(imgData,
                                 tmpPeak,
                                 PS_STAT_SAMPLE_MEAN,
                                 (psF32) TST16_INNER_RADIUS,
                                 (psF32) TST16_OUTER_RADIUS);
 
    if (tmpSource == NULL) {
        printf("TEST ERROR: pmSourceLocalSky() returned a NULL pmSource.\n");
        testStatus = false;
    }
 
    tmpSource->modelPSF = pmModelAlloc(PS_MODEL_GAUSS);
    tmpSource->modelPSF->params->data.F32[0] = 5.0;
    tmpSource->modelPSF->params->data.F32[1] = 70.0;
    tmpSource->modelPSF->params->data.F32[2] = (psF32) (TST16_NUM_ROWS / 2);
    tmpSource->modelPSF->params->data.F32[3] = (psF32) (TST16_NUM_COLS / 2);
    tmpSource->modelPSF->params->data.F32[4] = 1.0;
    tmpSource->modelPSF->params->data.F32[5] = 1.0;
    tmpSource->modelPSF->params->data.F32[6] = 2.0;
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceSubModel with NULL psImage.  Should generate error, return FALSE.\n");
    rc = pmSourceSubModel(NULL, tmpSource, true);
    if (rc == true) {
        printf("TEST ERROR: pmSourceSubModel() returned TRUE.\n");
        testStatus = false;
    }
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceSubModel with NULL psSrc.  Should generate error, return FALSE.\n");
    rc = pmSourceSubModel(imgData, NULL, true);
    if (rc == true) {
        printf("TEST ERROR: pmSourceSubModel() returned TRUE.\n");
        testStatus = false;
    }
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceSubModel with acceptable data.\n");
    rc = pmSourceSubModel(imgData, tmpSource, true);
    if (rc != true) {
        printf("TEST ERROR: pmSourceSubModel() returned FALSE.\n");
        testStatus = false;
    }
 
    psFree(tmpSource);
    psFree(imgData);
    return(testStatus);
}
*/

#define TST20_NUM_ROWS 100
#define TST20_NUM_COLS 100
#define TST20_SKY 10.0
#define TST20_INNER_RADIUS 3
#define TST20_OUTER_RADIUS 5
/******************************************************************************
test20(): We first test pmSourceSubModel() with various NULL and unallowable
input parameters.
 
XXX: We don't verify the numbers.
 *****************************************************************************/
/*
int test20( void )
{
    bool testStatus = true;
    psImage *imgData = psImageAlloc(TST20_NUM_COLS, TST20_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < imgData->numRows; i++) {
        for (psS32 j = 0 ; j < imgData->numCols; j++) {
            imgData->data.F32[i][j] = TST20_SKY;
        }
    }
    pmSource *tmpSource = NULL;
    psBool rc = false;
 
    pmPeak *tmpPeak = pmPeakAlloc((psF32) (TST20_NUM_ROWS / 2),
                                  (psF32) (TST20_NUM_COLS / 2),
                                  200.0,
                                  PM_PEAK_LONE);
 
    printf("Calling pmSourceLocalSky with valid data.\n");
    tmpPeak->x = (psF32) (TST20_NUM_ROWS / 2);
    tmpPeak->y = (psF32) (TST20_NUM_COLS / 2);
    tmpSource = pmSourceLocalSky(imgData,
                                 tmpPeak,
                                 PS_STAT_SAMPLE_MEAN,
                                 (psF32) TST20_INNER_RADIUS,
                                 (psF32) TST20_OUTER_RADIUS);
 
    if (tmpSource == NULL) {
        printf("TEST ERROR: pmSourceLocalSky() returned a NULL pmSource.\n");
        testStatus = false;
    }
 
    tmpSource->modelPSF = pmModelAlloc(PS_MODEL_GAUSS);
 
 
    tmpSource->modelPSF->params->data.F32[0] = 5.0;
    tmpSource->modelPSF->params->data.F32[1] = 70.0;
    tmpSource->modelPSF->params->data.F32[2] = (psF32) (TST20_NUM_ROWS / 2);
    tmpSource->modelPSF->params->data.F32[3] = (psF32) (TST20_NUM_COLS / 2);
    tmpSource->modelPSF->params->data.F32[4] = 1.0;
    tmpSource->modelPSF->params->data.F32[5] = 1.0;
    tmpSource->modelPSF->params->data.F32[6] = 2.0;
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceFitModel with NULL psImage.  Should generate error, return FALSE.\n");
    rc = pmSourceFitModel(tmpSource, NULL);
    if (rc == true) {
        printf("TEST ERROR: pmSourceFitModel() returned TRUE.\n");
        testStatus = false;
    }
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceFitModel with NULL pmSource.  Should generate error, return FALSE.\n");
    rc = pmSourceFitModel(NULL, imgData);
    if (rc == true) {
        printf("TEST ERROR: pmSourceFitModel() returned TRUE.\n");
        testStatus = false;
    }
 
    printf("----------------------------------------------------------------------------------\n");
    printf("Calling pmSourceFitModel with acceptable data.\n");
    rc = pmSourceFitModel(tmpSource, imgData);
    printf("pmSourceFitModel returned %d\n", rc);
 
    // XXX: Memory leaks are not being tested
    psVector *junk = psVectorAlloc(10, PS_TYPE_F32);
    junk->data.F32[0] = 0.0;
 
    psFree(tmpSource);
    psFree(imgData);
    return(testStatus);
}
*/


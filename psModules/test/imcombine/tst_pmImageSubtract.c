/** @file tst_pmImageSubtract.c
 *
 *  @brief Contains the tests for pmImageSubtract.c:
 *
 *  test00: This code will test the various functions in pmObjects.c
 *
 *  @author GLG, MHPCC
 *
 *  XXX: Most test simply ensure that the functions can be called with allowable
 *  data.  More work need to be done to verify the results.
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-05-25 22:02:46 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include "psTest.h"
#include "pslib.h"
#include "pmImageSubtract.h"
#define ERROR_TOLERANCE 1.0
static int test00(void);
static int test01(void);
static int test02(void);
static int test03(void);
testDescription tests[] = {
                              {test00, 000, "pmSubtractionKernelsAllocPOIS()", true, false},
                              {test01, 000, "pmSubtractionKernelsAllocISIS()", true, false},
                              {test02, 000, "pmSubtractionFindStamps()", true, false},
                              {test03, 000, "pmSubtractionCalculateEquation()", true, false},
                              {NULL}
                          };

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}


/*******************************************************************************
NOTE: This function returns FALSE if there were no errors.
 
XXX: Call with various unallowable input parameters.
 
XXX: Untested: we don't loop through the (u, v, xOrder, yOrder) psVectors and
ensure that each value is set correctly.
 ******************************************************************************/
psBool testPOISAlloc(psS32 size,
                     psS32 SpatialOrder)
{
    printf("Testing pmSubtractionKernelsAllocPOIS(%d, %d)\n", size, SpatialOrder);

    bool testStatus = false;
    psS32 nBasisFunctions = (2 * size + 1) * (2 * size + 1) * (SpatialOrder + 1) * (SpatialOrder + 2) / 2;

    psSubtractionKernels *kernels = pmSubtractionKernelsAllocPOIS(size, SpatialOrder);
    if (kernels == NULL) {
        printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() returned a NULL psSubtractionKernels.\n");
        testStatus = true;
    } else {
        if (kernels->type != PM_SUBTRACTION_KERNEL_POIS) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated the wrong kernels->type.\n");
            testStatus = true;
        }

        if ((kernels->u == NULL) ||
                (kernels->u->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a NULL ->u member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a incorrect length ->u member.\n");
            testStatus = true;
        }

        if ((kernels->v == NULL) ||
                (kernels->v->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a NULL ->v member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a incorrect length ->v member.\n");
            testStatus = true;
        }

        if (kernels->sigma != NULL) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a non-NULL ->sigma member.\n");
            testStatus = true;
        }

        if ((kernels->xOrder == NULL) ||
                (kernels->xOrder->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a NULL ->xOrder member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a incorrect length ->xOrder member.\n");
            testStatus = true;
        }

        if ((kernels->yOrder == NULL) ||
                (kernels->yOrder->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a NULL ->yOrder member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a incorrect length ->yOrder member.\n");
            testStatus = true;
        }

        if (kernels->subIndex != 0) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a non-zero ->subIndex member (%d).\n", kernels->subIndex);
            testStatus = true;
        }

        psS32 i = kernels->subIndex;
        if ((kernels->u->data.F32[i] != 0) ||
                (kernels->v->data.F32[i] != 0)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS(): the ->subIndex member points to a kernel with (%f, %f) (u, v) basis function.\n",
                   kernels->u->data.F32[i], kernels->v->data.F32[i]);
            testStatus = true;
        }

        if (kernels->preCalc != NULL) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated a non-NULL ->preCalc member.\n");
            testStatus = true;
        }

        if (kernels->size != size) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated an incorrect ->size member (%d).\n", kernels->size);
            testStatus = true;
        }

        if (kernels->spatialOrder != SpatialOrder) {
            printf("TEST ERROR: pmSubtractionKernelsAllocPOIS() generated an incorrect ->spatialOrder member (%d).\n", kernels->spatialOrder);
            testStatus = true;
        }
    }
    psFree(kernels);
    return(testStatus);
}

/*******************************************************************************
NOTE: This function returns TRUE if there were no errors.
 ******************************************************************************/
int test00( void )
{
    bool testStatus = false;

    testStatus|= testPOISAlloc(1, 1);
    testStatus|= testPOISAlloc(2, 3);
    testStatus|= testPOISAlloc(3, 4);

    return(!testStatus);
}

/*******************************************************************************
NOTE: This function returns FALSE if there were no errors.
 
XXX: Call with various unallowable input parameters.
 
XXX: Untested: we don't loop through the (u, v, xOrder, yOrder) psVectors and
ensure that each value is set correctly.  We don't ensure that he preCalc
psImages are set correctly.
 ******************************************************************************/
psBool testISISAlloc(psS32 sigmaLength,
                     psS32 orderLength,
                     psS32 size,
                     psS32 SpatialOrder)
{
    printf("Testing pmSubtractionKernelsAllocISIS(%d, %d, %d, %d)\n",
           sigmaLength, orderLength, size, SpatialOrder);

    psVector *sigmas = psVectorAlloc(sigmaLength, PS_TYPE_F32);
    sigmas->n = sigmas->nalloc;
    for (psS32 i = 0 ; i < sigmas->n ; i++) {
        sigmas->data.F32[i] = 1.0 + (psF32) i;
    }
    psVector *orders = psVectorAlloc(orderLength, PS_TYPE_S32);
    orders->n = orders->nalloc;
    for (psS32 i = 0 ; i < orders->n ; i++) {
        orders->data.S32[i] = i + 2;
    }

    bool testStatus = false;
    psS32 numSigmas = sigmas->n;
    psS32 nBasisFunctions = 0;
    for (psS32 s = 0 ; s < numSigmas ; s++) {
        for (psS32 o = 0 ; o < orders->n ; o++) {
            nBasisFunctions+= ((orders->data.S32[o] + 1) * (orders->data.S32[o] + 2) / 2);
        }
    }
    nBasisFunctions*= ((SpatialOrder + 1) * (SpatialOrder + 2) / 2);

    psSubtractionKernels *kernels = pmSubtractionKernelsAllocISIS(sigmas, orders, size, SpatialOrder);
    if (kernels == NULL) {
        printf("TEST ERROR: pmSubtractionKernelsAllocISIS() returned a NULL psSubtractionKernels.\n");
        testStatus = true;
    } else {
        if (kernels->type != PM_SUBTRACTION_KERNEL_ISIS) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated the wrong kernels->type.\n");
            testStatus = true;
        }

        if ((kernels->u == NULL) ||
                (kernels->u->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a NULL ->u member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a incorrect length ->u member.\n");
            testStatus = true;
        }

        if ((kernels->v == NULL) ||
                (kernels->v->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a NULL ->v member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a incorrect length ->v member.\n");
            testStatus = true;
        }

        if ((kernels->sigma == NULL) ||
                (kernels->sigma->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a NULL ->sigma member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a incorrect length ->sigma member.\n");
            testStatus = true;
        }

        if ((kernels->xOrder == NULL) ||
                (kernels->xOrder->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a NULL ->xOrder member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a incorrect length ->xOrder member.\n");
            testStatus = true;
        }

        if ((kernels->yOrder == NULL) ||
                (kernels->yOrder->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a NULL ->yOrder member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a incorrect length ->yOrder member.\n");
            testStatus = true;
        }

        if (kernels->subIndex != 0) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a non-zero ->subIndex member (%d).\n", kernels->subIndex);
            testStatus = true;
        }

        //
        // Ensure that kernels->subIndex points to the correct kernel.
        //
        psS32 i = kernels->subIndex;
        if ((kernels->u->data.F32[i] != 0.0) ||
                (kernels->v->data.F32[i] != 0.0) ||
                (kernels->xOrder->data.F32[i] != 0.0) ||
                (kernels->yOrder->data.F32[i] != 0.0) ||
                (kernels->sigma->data.F32[i] != sigmas->data.F32[0])) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS(): the ->subIndex member points to the wrong kernel.\n");
            printf("TEST ERROR: (u, v, xOrder, yOrder, sigma) is (%f, %f, %f, %f, %f).\n",
                   kernels->u->data.F32[i], kernels->v->data.F32[i],
                   kernels->xOrder->data.F32[i], kernels->yOrder->data.F32[i],
                   kernels->sigma->data.F32[i]);
            testStatus = true;
        }

        //
        // Ensure that the preCalc images are allocated correctly.
        //
        if ((kernels->preCalc == NULL) ||
                (kernels->preCalc->n != nBasisFunctions)) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a NULL ->preCalc member or\n");
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated a incorrect length ->preCalc member.\n");
            testStatus = true;
        } else {
            for (psS32 k = 0 ; k < kernels->u->n ; k++) {
                psImage *kerImg = (psImage *) kernels->preCalc->data[k];
                if (kerImg == NULL) {
                    printf("TEST ERROR: the %d-th kernel preCalc image is NULL.\n", k);
                    testStatus = true;
                } else {
                    if (kerImg->type.type != PS_TYPE_F32) {
                        printf("TEST ERROR: preCalc image %d had ioncorrect type.\n", k);
                        testStatus = true;
                    }
                    if ((kerImg->numRows != (1 + (2 * size))) ||
                            (kerImg->numCols != (1 + (2 * size)))) {
                        printf("TEST ERROR: preCalc image %d had incorrect size (%d, %d).\n", k,
                               kerImg->numRows, kerImg->numCols);
                        testStatus = true;
                    }
                }
            }
        }

        if (kernels->size != size) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated an incorrect ->size member (%d).\n", kernels->size);
            testStatus = true;
        }

        if (kernels->spatialOrder != SpatialOrder) {
            printf("TEST ERROR: pmSubtractionKernelsAllocISIS() generated an incorrect ->spatialOrder member (%d).\n", kernels->spatialOrder);
            testStatus = true;
        }
    }
    psFree(sigmas);

    psFree(kernels->u);
    psFree(kernels->v);
    psFree(kernels->sigma);
    psFree(kernels->xOrder);
    psFree(kernels->yOrder);
    psFree(kernels->preCalc);
    psFree(kernels);
    psFree(orders);
    return(testStatus);
}

/*******************************************************************************
NOTE: This function returns TRUE if there were no errors.
 ******************************************************************************/
int test01( void )
{
    bool testStatus = false;

    /*
        testStatus|= testISISAlloc(1, 1, 1, 1);
        testStatus|= testISISAlloc(2, 2, 2, 2);
        testStatus|= testISISAlloc(2, 3, 4, 5);
        testStatus|= testISISAlloc(3, 4, 5, 6);
    */

    return(!testStatus);
}


/*******************************************************************************
NOTE: This function returns FALSE if there were no errors.
 
XXX: Can we test to ensure that no stamps overlap?
 
XXX: Test stamp alloc/dealloc functions.
 ******************************************************************************/
#define TST02_THRESHOLD 3.0
#define TST02_MASK_VAL 1
psBool testFindStamps(psS32 numCols,
                      psS32 numRows,
                      psS32 xNum,
                      psS32 yNum,
                      psS32 border)
{
    printf("Testing pmSubtractionFindStamps(%d, %d, %d, %d, %d)\n",
           numCols, numRows, xNum, yNum, border);
    bool testStatus = false;

    // Create a test image and set a single pixel in the center of each stamp.
    psImage *tstImg = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    int numStamps = 0;
    PS_IMAGE_SET_F32(tstImg, 0.0);
    for (psS32 j = 0; j < yNum; j++) {
        for (psS32 i = 0; i < xNum; i++) {
            psS32 yMin = border + j * (numRows - 2.0 * border) / yNum;
            psS32 yMax = PS_MIN(numRows-1, (border + (j + 1) * (numRows - 2.0 * border) / yNum) - 1);
            psS32 xMin = border + i * (numCols - 2.0 * border) / xNum;
            psS32 xMax = PS_MIN(numCols-1, (border + (i + 1) * (numCols - 2.0 * border) / xNum) - 1);

            tstImg->data.F32[(yMax+yMin)/2][(xMax+xMin)/2] = TST02_THRESHOLD + (psF32) (i + j);
            numStamps++;
        }
    }
    psImage *tmpMask= psImageAlloc(numCols, numRows, PS_TYPE_U8);
    PS_IMAGE_SET_U8(tstImg, 0);

    //-------------------------------------------------------------------------
    printf("Calling with a NULL psImage.  Should generate error, return NULL.\n");
    psArray *stamps = pmSubtractionFindStamps(NULL, NULL, tmpMask, TST02_MASK_VAL,
                      TST02_THRESHOLD, xNum, yNum,
                      border);
    if (stamps != NULL) {
        printf("TEST ERROR: pmSubtractionFindStamps returned a non-NULL psArray.\n");
        psFree(stamps);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a non-positive xNum.  Should generate error, return NULL.\n");
    stamps = pmSubtractionFindStamps(NULL, tstImg, tmpMask, TST02_MASK_VAL,
                                     TST02_THRESHOLD, 0, yNum,
                                     border);
    if (stamps != NULL) {
        printf("TEST ERROR: pmSubtractionFindStamps returned a non-NULL psArray.\n");
        psFree(stamps);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a non-positive yNum.  Should generate error, return NULL.\n");
    stamps = pmSubtractionFindStamps(NULL, tstImg, tmpMask, TST02_MASK_VAL,
                                     TST02_THRESHOLD, xNum, 0,
                                     border);
    if (stamps != NULL) {
        printf("TEST ERROR: pmSubtractionFindStamps returned a non-NULL psArray.\n");
        psFree(stamps);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a non-positive border.  Should generate error, return NULL.\n");
    stamps = pmSubtractionFindStamps(NULL, tstImg, tmpMask, TST02_MASK_VAL,
                                     TST02_THRESHOLD, xNum, yNum,
                                     0);
    if (stamps != NULL) {
        printf("TEST ERROR: pmSubtractionFindStamps returned a non-NULL psArray.\n");
        psFree(stamps);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with acceptable input parameters, non-NULL mask.\n");
    stamps = pmSubtractionFindStamps(NULL, tstImg, tmpMask, TST02_MASK_VAL,
                                     TST02_THRESHOLD, xNum, yNum,
                                     border);
    if (stamps == NULL) {
        printf("TEST ERROR: pmSubtractionFindStamps returned a non-NULL psArray.\n");
        testStatus = true;
    } else {
        if (stamps->n != numStamps) {
            printf("TEST ERROR: %ld stamps were found, %d were expected.\n",
                   stamps->n, numStamps);
            testStatus = true;
        }
        for (psS32 s = 0 ; s < stamps->n ; s++) {
            pmStamp *stamp = (pmStamp *) stamps->data[s];
            psS32 row = stamp->y;
            psS32 col = stamp->x;
            // printf("Stamp %d at (%d, %d) has a value of %f\n", s, row, col, tstImg->data.F32[row][col]);
            if (tstImg->data.F32[row][col] < TST02_THRESHOLD) {
                if (stamp->status != PM_STAMP_NONE) {
                    printf("TEST ERROR: stamp %d had peak value %f (below theshold) and the status was not set to PM_STAMP_NONE.\n",
                           s, tstImg->data.F32[row][col]);
                    testStatus = true;
                }
            } else {
                if (stamp->status != PM_STAMP_RECALC) {
                    printf("TEST ERROR: stamp %d had peak value %f (above theshold) and the status was not set to PM_STAMP_RECALC.\n",
                           s, tstImg->data.F32[row][col]);
                    testStatus = true;
                }
            }
        }
    }
    psFree(stamps);

    //-------------------------------------------------------------------------
    printf("Calling with acceptable input parameters, NULL mask.\n");
    stamps = pmSubtractionFindStamps(NULL, tstImg, NULL, TST02_MASK_VAL,
                                     TST02_THRESHOLD, xNum, yNum,
                                     border);
    if (stamps == NULL) {
        printf("TEST ERROR: pmSubtractionFindStamps returned a non-NULL psArray.\n");
        testStatus = true;
    } else {
        if (stamps->n != numStamps) {
            printf("TEST ERROR: %ld stamps were found, %d were expected.\n",
                   stamps->n, numStamps);
            testStatus = true;
        }
        for (psS32 s = 0 ; s < stamps->n ; s++) {
            pmStamp *stamp = (pmStamp *) stamps->data[s];
            psS32 row = stamp->y;
            psS32 col = stamp->x;
            //printf("Stamp %d at (%d, %d) has a value of %f\n", s, row, col, tstImg->data.F32[row][col]);
            if (tstImg->data.F32[row][col] < TST02_THRESHOLD) {
                if (stamp->status != PM_STAMP_NONE) {
                    printf("TEST ERROR: stamp %d had peak value %f (below theshold) and the status was not set to PM_STAMP_NONE.\n",
                           s, tstImg->data.F32[row][col]);
                    testStatus = true;
                }
            } else {
                if (stamp->status != PM_STAMP_RECALC) {
                    printf("TEST ERROR: stamp %d had peak value %f (above theshold) and the status was not set to PM_STAMP_RECALC.\n",
                           s, tstImg->data.F32[row][col]);
                    testStatus = true;
                }
            }
        }
    }

    psFree(tstImg);
    psFree(tmpMask);
    psFree(stamps);

    return(testStatus);
}



/*******************************************************************************
NOTE: This function returns TRUE if there were no errors.
 ******************************************************************************/
int test02( void )
{
    bool testStatus = false;

    testStatus|= testFindStamps(100, 100, 2, 2, 2);
    testStatus|= testFindStamps(100, 100, 10, 10, 2);

    return(!testStatus);
}


psF32 genRanFloat(psF32 low,
                  psF32 high)
{
    psF32 ran1 = (((psF32) (random() % 10000)) / 10000.0);
    return(low + (ran1 * (high - low)));
}

//
// XXX: POIS kernels are producing NANs if the image size is 20 or less.
//
// XXX: ISIS kernels are producing NANS if the TST03_ORDER_LENGTH is 2 or larger.
//
#define TST03_THRESHOLD  3.0
#define TST03_MASK_VAL  1
#define TST03_KERNEL_SIZE 2
#define TST03_SPATIAL_ORDER 2
#define TST03_ORDER_LENGTH 1
#define TST03_SIGMA_LENGTH 1
#define TST03_PSF_MAX  10.0
#define TST03_BG 0.0
#define TST03_IMAGE_SIZE 25
#define TST03_NUM_COLS  TST03_IMAGE_SIZE
#define TST03_NUM_ROWS  TST03_IMAGE_SIZE
#define TST03_NUM_STAMPS 2
#define TST03_NUM_STAMPS_COLS TST03_NUM_STAMPS
#define TST03_NUM_STAMPS_ROWS TST03_NUM_STAMPS
#define TST03_BORDER  TST03_KERNEL_SIZE
//#define TST03_FOOTPRINT (((TST03_IMAGE_SIZE - (2 * TST03_BORDER)) / TST03_NUM_STAMPS) - TST03_KERNEL_SIZE)
#define TST03_FOOTPRINT  4
#define TST03_PSF_WIDTH  (TST03_FOOTPRINT/2 - 1)

/*******************************************************************************
This routine generates an object in the center of the stamp defined by the
(xMin, xMax) and (yMin, yMax) boundary.
 ******************************************************************************/
psBool genObject(psImage *tstImg,
                 psImage *refImg,
                 psS32 xMin,
                 psS32 xMax,
                 psS32 yMin,
                 psS32 yMax)
{
    if (((1 + 2 * TST03_PSF_WIDTH) > (xMax - xMin)) ||
            ((1 + 2 * TST03_PSF_WIDTH) > (yMax - yMin))) {
        printf("INCORRECT TEST CONFIGURATION: TST03_PSF_WIDTH is too big.\n");
        printf("TST03_PSF_WIDTH is %d: (xMin - xMax) is %d\n", TST03_PSF_WIDTH, (xMax - xMin));
        return(FALSE);
    }
    //
    // This code basically creates a peak at the center of the stamp with
    // a height of TST03_PSF_MAX
    //
    psS32 xCenter = (xMax + xMin) / 2;
    psS32 yCenter = (yMax + yMin) / 2;
    psF32 subImageWidth = 1.0 + sqrtf(PS_SQR(((psF32) ((yMax-yMin)/2))) + PS_SQR(((psF32) ((xMax-xMin)/2))));
    for (psS32 y = yMin ; y <= yMax ; y++) {
        for (psS32 x = xMin ; x <= xMax ; x++) {
            psF32 dist = sqrtf(PS_SQR((psF32) (y - yCenter)) + PS_SQR((psF32) (x - xCenter)));
            psF32 pixel = TST03_PSF_MAX * PS_SQR(((psF32) (subImageWidth - dist)) / ((psF32) subImageWidth));
            if (pixel < 0.0) {
                pixel = 0.0;
            }
            refImg->data.F32[y][x] = pixel + TST03_BG;
            tstImg->data.F32[y][x] = pixel + TST03_BG;
            // Add some noise for the test image.
            tstImg->data.F32[y][x]+= genRanFloat(0.0, 1.0);
        }
    }

    return(TRUE);
}

/*******************************************************************************
NOTE: This function returns FALSE if there were no errors.
 
XXX: We should use a larger variety of input parameter configurations.
 
I test the following functions here (since they linearly rely on a set of data
structures that the previoues ones generate):
    pmSubtractionCalculateEquation()
    pmSubtractionSolveEquation()
    pmSubtractionRejectStamps()
    pmSubtractionKernelImage()
 ******************************************************************************/
psBool testSubCalcEqu(psS32 numCols,
                      psS32 numRows,
                      psS32 xNum,
                      psS32 yNum,
                      psS32 border,
                      pmSubtractionKernelsType KernelType)
{
    printf("Testing pmSubtractionCalculateEquation(): \n");
    printf("    image size is (%d, %d)\n", numRows, numCols);
    printf("    num stamps is (%d, %d).  Border is %d\n", yNum, xNum, border);
    if (KernelType == PM_SUBTRACTION_KERNEL_POIS) {
        printf("   kernel type is PM_SUBTRACTION_KERNEL_POIS.\n");
    } else if (KernelType == PM_SUBTRACTION_KERNEL_ISIS) {
        printf("   kernel type is PM_SUBTRACTION_KERNEL_ISIS.\n");
    }
    bool testStatus = false;

    // Create a test image and set a single pixel in the center of each stamp.
    psImage *tstImg = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *refImg = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *maskImg = psImageAlloc(numCols, numRows, PS_TYPE_U8);
    PS_IMAGE_SET_F32(tstImg, 0.0);
    PS_IMAGE_SET_F32(maskImg, 0);
    for (psS32 j = 0; j < yNum; j++) {
        for (psS32 i = 0; i < xNum; i++) {
            psS32 yMin = border + j * (numRows - 2.0 * border) / yNum;
            psS32 yMax = PS_MIN(numRows-1, (border + (j + 1) * (numRows - 2.0 * border) / yNum) - 1);
            psS32 xMin = border + i * (numCols - 2.0 * border) / xNum;
            psS32 xMax = PS_MIN(numCols-1, (border + (i + 1) * (numCols - 2.0 * border) / xNum) - 1);

            genObject(tstImg, refImg, xMin, xMax, yMin, yMax);
        }
    }
    printf("Generating stamps...\n");
    psArray *stamps = pmSubtractionFindStamps(NULL, tstImg, NULL, TST03_MASK_VAL,
                      TST03_THRESHOLD, xNum, yNum,
                      border);

    //
    // PsVectors sigmas and orders are for ISIS kernels only.
    //
    psVector *sigmas = psVectorAlloc(TST03_SIGMA_LENGTH, PS_TYPE_F32);
    sigmas->n = sigmas->nalloc;
    for (psS32 i = 0 ; i < sigmas->n ; i++) {
        sigmas->data.F32[i] = 1.0 + (psF32) i;
    }
    psVector *orders = psVectorAlloc(TST03_ORDER_LENGTH, PS_TYPE_S32);
    orders->n = orders->nalloc;
    for (psS32 i = 0 ; i < orders->n ; i++) {
        orders->data.S32[i] = i + 2;
    }

    //
    // Create the subtraction kernels
    //
    printf("Generating kernel basis functions...\n");
    psSubtractionKernels *myKernels = NULL;
    if (KernelType == PM_SUBTRACTION_KERNEL_POIS) {
        myKernels = pmSubtractionKernelsAllocPOIS(TST03_KERNEL_SIZE, TST03_SPATIAL_ORDER);
    } else if (KernelType == PM_SUBTRACTION_KERNEL_ISIS) {
        myKernels = pmSubtractionKernelsAllocISIS(sigmas, orders, TST03_KERNEL_SIZE, TST03_SPATIAL_ORDER);
    }

    if ((stamps == NULL) ||
            (myKernels == NULL)) {
        printf("TEST ERROR: stamps or myKernels is NULL.\n");
        testStatus = true;
    } else {
        //-------------------------------------------------------------------------
        printf("Calling with a NULL psArray stamps.  Should generate error, return FALSE.\n");
        psBool rc = pmSubtractionCalculateEquation(NULL, refImg, tstImg, myKernels, TST03_FOOTPRINT);
        if (rc == TRUE) {
            printf("TEST ERROR: pmSubtractionCalculateEquation() returned TRUE.\n");
            testStatus = true;
        }

        //-------------------------------------------------------------------------
        printf("Calling with a NULL reference images.  Should generate error, return FALSE.\n");
        rc = pmSubtractionCalculateEquation(stamps, NULL, tstImg, myKernels, TST03_FOOTPRINT);
        if (rc == TRUE) {
            printf("TEST ERROR: pmSubtractionCalculateEquation() returned TRUE.\n");
            testStatus = true;
        }

        //-------------------------------------------------------------------------
        printf("Calling with a NULL input images.  Should generate error, return FALSE.\n");
        rc = pmSubtractionCalculateEquation(stamps, refImg, NULL, myKernels, TST03_FOOTPRINT);
        if (rc == TRUE) {
            printf("TEST ERROR: pmSubtractionCalculateEquation() returned TRUE.\n");
            testStatus = true;
        }

        //-------------------------------------------------------------------------
        printf("Calling with a NULL kernel basis functions.  Should generate error, return FALSE.\n");
        rc = pmSubtractionCalculateEquation(stamps, refImg, tstImg, NULL, TST03_FOOTPRINT);
        if (rc == TRUE) {
            printf("TEST ERROR: pmSubtractionCalculateEquation() returned TRUE.\n");
            testStatus = true;
        }

        //-------------------------------------------------------------------------
        printf("Calling with acceptable input parameters.  Should return TRUE.\n");

        rc = pmSubtractionCalculateEquation(stamps, refImg, tstImg, myKernels, TST03_FOOTPRINT);
        if (rc != TRUE) {
            printf("TEST ERROR: pmSubtractionCalculateEquation() returned FALSE.\n");
            testStatus = true;
        } else {

            if (0) {
                for (psS32 s = 0 ; s < stamps->n ; s++) {
                    printf("********************************* Stamp %d *********************************\n", s);
                    pmStamp *stamp = (pmStamp *) stamps->data[s];
                    if (stamp->vector != NULL) {
                        PS_VECTOR_PRINT_F64(stamp->vector);
                    }
                    if (stamp->matrix != NULL) {
                        printf("Stamp matrix size is (%d, %d)\n", stamp->matrix->numRows, stamp->matrix->numCols);
                        PS_IMAGE_PRINT_F64(stamp->matrix);
                    }
                }
            }


            //-------------------------------------------------------------------------
            printf("Calling pmSubtractionSolveEquation() with a NULL stamp argument.  Should generate error, return FALSE.\n");
            psVector *solution = pmSubtractionSolveEquation(NULL, NULL);
            if (solution != NULL) {
                printf("TEST ERROR: pmSubtractionSolveEquation() returned non-NULL.\n");
                testStatus = true;
            }

            //-------------------------------------------------------------------------
            printf("Calling pmSubtractionSolveEquation() with acceptable input parameters.  Should return non-NULL.\n");
            solution = pmSubtractionSolveEquation(NULL, stamps);
            if (solution == NULL) {
                printf("TEST ERROR: pmSubtractionSolveEquation() returned NULL.\n");
                testStatus = true;
            } else {
                printf("The solution vector is:\n");
                for (psS32 i = 0 ; i < solution->n ; i++) {
                    printf("(%.2f) ", solution->data.F64[i]);
                }
                printf("\n");

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionRejectStamps() with acceptable input parameters.  Should return TRUE.\n");
                rc = pmSubtractionRejectStamps(stamps, maskImg, 0xff, TST03_FOOTPRINT, 1.0, refImg,
                                               tstImg, solution, myKernels);
                if (rc != TRUE) {
                    printf("TEST ERROR: pmSubtractionRejectStamps() returned FALSE.\n");
                    testStatus = true;
                } else {}

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() with NULL solution.  Should generate error, return NULL.\n");
                psImage *kernelImg = pmSubtractionKernelImage(NULL, NULL, myKernels, 0.0, 0.0);
                if (kernelImg != NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned non-NULL.\n");
                    testStatus = true;
                }
                free(kernelImg);

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() with NULL kernels.  Should generate error, return NULL.\n");
                kernelImg = pmSubtractionKernelImage(NULL, solution, NULL, 0.0, 0.0);
                if (kernelImg != NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned non-NULL.\n");
                    testStatus = true;
                }
                free(kernelImg);

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() unallowable x value.  Should generate error, return NULL.\n");
                kernelImg = pmSubtractionKernelImage(NULL, solution, myKernels, -2.0, 0.0);
                if (kernelImg != NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned non-NULL.\n");
                    testStatus = true;
                }
                free(kernelImg);

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() unallowable x value.  Should generate error, return NULL.\n");
                kernelImg = pmSubtractionKernelImage(NULL, solution, myKernels, 2.0, 0.0);
                if (kernelImg != NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned non-NULL.\n");
                    testStatus = true;
                }
                free(kernelImg);

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() unallowable y value.  Should generate error, return NULL.\n");
                kernelImg = pmSubtractionKernelImage(NULL, solution, myKernels, 0.0, -2.0);
                if (kernelImg != NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned non-NULL.\n");
                    testStatus = true;
                }
                free(kernelImg);

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() unallowable y value.  Should generate error, return NULL.\n");
                kernelImg = pmSubtractionKernelImage(NULL, solution, myKernels, 0.0, 2.0);
                if (kernelImg != NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned non-NULL.\n");
                    testStatus = true;
                }
                free(kernelImg);

                //-------------------------------------------------------------------------
                printf("Calling pmSubtractionKernelImage() with acceptable input parameters.  Should return a psImage.\n");
                kernelImg = pmSubtractionKernelImage(NULL, solution, myKernels, 0.5, 0.5);
                if (kernelImg == NULL) {
                    printf("TEST ERROR: pmSubtractionKernelImage() returned NULL.\n");
                    testStatus = true;
                } else {
                    for (psS32 row = 0 ; row < kernelImg->numRows; row++) {
                        for (psS32 col = 0 ; col < kernelImg->numCols; col++) {
                            printf("%f ", kernelImg->data.F32[row][col]);
                        }
                        printf("\n");
                    }
                }
                free(kernelImg);

                psFree(solution);
            }
        }
    }
    psFree(tstImg);
    psFree(refImg);
    psFree(stamps);
    psFree(myKernels);
    psFree(orders);
    psFree(sigmas);

    return(testStatus);
}

/*******************************************************************************
NOTE: This function returns TRUE if there were no errors.
 ******************************************************************************/
int test03( void )
{
    bool testStatus = false;


    srand(1995);
    if (1)
        testStatus|= testSubCalcEqu(TST03_NUM_COLS,
                                    TST03_NUM_ROWS,
                                    TST03_NUM_STAMPS_COLS,
                                    TST03_NUM_STAMPS_ROWS,
                                    TST03_BORDER,
                                    PM_SUBTRACTION_KERNEL_POIS);


    srand(1995);
    if (1)
        testStatus|= testSubCalcEqu(TST03_NUM_COLS,
                                    TST03_NUM_ROWS,
                                    TST03_NUM_STAMPS_COLS,
                                    TST03_NUM_STAMPS_ROWS,
                                    TST03_BORDER,
                                    PM_SUBTRACTION_KERNEL_ISIS);



    return(!testStatus);
}

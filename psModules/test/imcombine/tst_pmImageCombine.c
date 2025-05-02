/** @file tst_pmImageCombine.c
 *
 *  @brief Contains the tests for pmImageCombine.c:
 *
 *  test00: This code will test the various functions in the pmImageCombine.c file.
 *
 *  @author GLG, MHPCC
 *
 *  XXX: Must verify the results internally.  Don't use stdout file.
 *  XXX: Must test masks with pmRejectPixels()
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-03-04 01:01:34 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include "psTest.h"
#include "pslib.h"
#include "pmImageCombine.h"
static int test00(void);
testDescription tests[] = {
                              {test00, 000, "pmCombineImages()", true, false},
                              {NULL}
                          };

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}

psF32 genRanFloat(psF32 low,
                  psF32 high)
{
    psF32 ran1 = (((psF32) (random() % 10000)) / 10000.0);
    return(low + (ran1 * (high - low)));
}

psF32 genRanInt(psS32 low,
                psS32 high)
{
    psF32 ran1 = (((psF32) (random() % 10000)) / 10000.0);
    return((psS32) (low + (ran1 * (high - low))));
}

#define TST00_EXPANSION_FACTOR_X 1.0
#define TST00_EXPANSION_FACTOR_Y 1.0
#define TST00_OFFSET_X 0.0
#define TST00_OFFSET_Y 0.0
#define TST00_NUM_PIXELS 10
#define TST00_MASK_VALUE 1
#define TST00_NUM_ITERATIONS 4
#define TST00_SIGMA_CLIP 1.0
#define TST00_REJECTION_THRESHOLD 0.01
#define TST00_GRADIENT_LIMIT 10.0
/*******************************************************************************
NOTE: This function returns FALSE if there were no errors.
 ******************************************************************************/
psBool testCombineImages(psS32 numRows,
                         psS32 numCols,
                         psS32 numImages)
{
    printf("Testing pmCombineImages(%d, %d, %d)\n", numRows, numCols, numImages);
    bool testStatus = false;

    psArray *images = psArrayAlloc(numImages);
    psArray *errors = psArrayAlloc(numImages);
    psArray *masks = psArrayAlloc(numImages);
    images->n = images->nalloc;
    errors->n = errors->nalloc;
    masks->n = masks->nalloc;
    for (psS32 i = 0 ; i < numImages ; i++) {
        images->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_F32);
        errors->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_F32);
        masks->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_U8);
        psImage *image = (psImage *) images->data[i];
        psImage *error = (psImage *) errors->data[i];
        psImage *mask = (psImage *) masks->data[i];
        PS_IMAGE_SET_F32(image, 0.0);
        PS_IMAGE_SET_F32(error, 1.0);
        PS_IMAGE_SET_U8(mask, 0);

        for (psS32 row = 0 ; row < numRows ; row++) {
            for (psS32 col = 0 ; col < numCols ; col++) {
                // Scale row/col to [-1.0:1.0]
                psF32 rowScaled = ((psF32) (row - (numRows/2))) / ((psF32) (numRows/2));
                psF32 colScaled = ((psF32) (col - (numCols/2))) / ((psF32) (numCols/2));
                image->data.F32[row][col] = PS_SQR((2.0 - rowScaled) + (2.0 - colScaled)) + genRanFloat(0.0, 0.5);
            }
        }
    }

    //
    // Same as above except the numImages is wrong
    //
    psArray *imagesLong = psArrayAlloc(numImages+1);
    psArray *errorsLong = psArrayAlloc(numImages+1);
    psArray *masksLong = psArrayAlloc(numImages+1);
    imagesLong->n = imagesLong->nalloc;
    errorsLong->n = errorsLong->nalloc;
    masksLong->n = masksLong->nalloc;
    for (psS32 i = 0 ; i < numImages+1 ; i++) {
        imagesLong->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_F32);
        errorsLong->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_F32);
        masksLong->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_U8);
        psImage *image = (psImage *) imagesLong->data[i];
        psImage *error = (psImage *) errorsLong->data[i];
        psImage *mask = (psImage *) masksLong->data[i];
        PS_IMAGE_SET_F32(image, 0.0);
        PS_IMAGE_SET_F32(error, 1.0);
        PS_IMAGE_SET_U8(mask, 0);

        for (psS32 row = 0 ; row < numRows ; row++) {
            for (psS32 col = 0 ; col < numCols ; col++) {
                // Scale row/col to [-1.0:1.0]
                psF32 rowScaled = ((psF32) (row - (numRows/2))) / ((psF32) (numRows/2));
                psF32 colScaled = ((psF32) (col - (numCols/2))) / ((psF32) (numCols/2));
                image->data.F32[row][col] = PS_SQR((2.0 - rowScaled) + (2.0 - colScaled)) + genRanFloat(0.0, 0.5);
            }
        }
    }

    //
    // Same as above except the type is wrong
    //
    psArray *imagesBadType = psArrayAlloc(numImages);
    psArray *errorsBadType = psArrayAlloc(numImages);
    psArray *masksBadType = psArrayAlloc(numImages);
    imagesBadType->n = imagesBadType->nalloc;
    errorsBadType->n = errorsBadType->nalloc;
    masksBadType->n = masksBadType->nalloc;
    for (psS32 i = 0 ; i < numImages ; i++) {
        imagesBadType->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_F64);
        errorsBadType->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_F64);
        masksBadType->data[i] = (psPtr *) psImageAlloc(numCols, numRows, PS_TYPE_S8);
        psImage *image = (psImage *) imagesBadType->data[i];
        psImage *error = (psImage *) errorsBadType->data[i];
        psImage *mask = (psImage *) masksBadType    ->data[i];
        PS_IMAGE_SET_F32(image, 0.0);
        PS_IMAGE_SET_F32(error, 1.0);
        PS_IMAGE_SET_U8(mask, 0);

        for (psS32 row = 0 ; row < numRows ; row++) {
            for (psS32 col = 0 ; col < numCols ; col++) {
                // Scale row/col to [-1.0:1.0]
                psF32 rowScaled = ((psF32) (row - (numRows/2))) / ((psF32) (numRows/2));
                psF32 colScaled = ((psF32) (col - (numCols/2))) / ((psF32) (numCols/2));
                image->data.F32[row][col] = PS_SQR((2.0 - rowScaled) + (2.0 - colScaled)) + genRanFloat(0.0, 0.5);
            }
        }
    }

    psPixels *pixels = psPixelsAlloc(TST00_NUM_PIXELS);
    pixels->n = pixels->nalloc;
    for (psS32 p = 0 ; p < TST00_NUM_PIXELS ; p++) {
        psS32 col =  genRanInt(0, numCols);
        psS32 row =  genRanInt(0, numRows);
        pixels->data[p].x = (psF32) col;
        pixels->data[p].y = (psF32) row;
        psS32 im = genRanInt(0, numImages-1);
        psImage *image = (psImage *) images->data[im];
        image->data.F32[row][col] += 100.0;
        printf("Generating a bad pixel in image (%d) at (%d, %d)\n", im, row, col);
    }

    psArray *questionablePixels = NULL;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);

    //-------------------------------------------------------------------------
    printf("Calling with a NULL images.  Should generate error, return NULL.\n");
    psImage *outImg = pmCombineImages(NULL, &questionablePixels, NULL, errors,
                                      masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                                      TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a long images.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, imagesLong, errors,
                             masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a bad type images.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, imagesBadType, errors,
                             masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a long errors.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, images, errorsLong,
                             masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a bad type errors.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, images, errorsBadType,
                             masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a long masks.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, images, errors,
                             masksLong, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a bad type masks.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, images, errors,
                             masksBadType, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with a NULL stats.  Should generate error, return NULL.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, images, errors,
                             masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, NULL);
    if (outImg != NULL) {
        printf("TEST ERROR: pmCombineImages() returned a non-NULL psImage.\n");
        psFree(outImg);
        testStatus = true;
    }

    //-------------------------------------------------------------------------
    printf("Calling with acceptable data.  Should generate a psImage.\n");
    outImg = pmCombineImages(NULL, &questionablePixels, images, errors,
                             masks, TST00_MASK_VALUE, pixels, TST00_NUM_ITERATIONS,
                             TST00_SIGMA_CLIP, stats);
    if (outImg == NULL) {
        printf("TEST ERROR: pmCombineImages() returned a NULL psImage.\n");
        testStatus = true;
    }
    if (0) {
        for (psS32 p = 0 ; p < TST00_NUM_PIXELS ; p++) {
            psS32 col = (psS32) (pixels->data[p]).x;
            psS32 row = (psS32) (pixels->data[p]).y;
            printf("------------------------------------------\n");
            printf("Pixel (%d, %d) in combined image is %f\n", row, col, outImg->data.F32[row][col]);
            for (psS32 i = 0 ; i < numImages ; i++) {
                psImage *image = (psImage *) images->data[i];
                printf("Pixel (after combine) (%d, %d) in image (%d) is %f\n", row, col, i, image->data.F32[row][col]);
            }
        }
    }

    if (questionablePixels->n != numImages) {
        printf("TEST ERROR: pmCombineImages(): questionablePixels->n was %ld, should have been %d\n",
               questionablePixels->n, numImages);
        testStatus = true;
    } else {
        // XXX: We should internally verify this with the pixels list, not merely use the stdout.
        for (psS32 i = 0 ; i < questionablePixels->n ; i++) {
            psPixels *myPixels = (psPixels *) questionablePixels->data[i];
            for (psS32 p = 0 ; p < myPixels->n ; p++) {
                printf("Image %d, questionable pixel %d is (%f %f)\n",
                       i, p, myPixels->data[p].y, myPixels->data[p].x);
            }
        }
    }

    psArray *expandTransforms = psArrayAlloc(numImages);
    psArray *contractTransforms = psArrayAlloc(numImages);
    expandTransforms->n = expandTransforms->nalloc;
    contractTransforms->n = contractTransforms->nalloc;
    for (psS32 im = 0 ; im < numImages ; im++) {
        psPlaneTransform *ptExpand = psPlaneTransformAlloc(2, 2);
        ptExpand->x->coeff[0][0] = TST00_OFFSET_X;
        ptExpand->x->coeff[1][0] = TST00_EXPANSION_FACTOR_X;
        ptExpand->y->coeff[0][0] = TST00_OFFSET_Y;
        ptExpand->y->coeff[0][1] = TST00_EXPANSION_FACTOR_Y;
        expandTransforms->data[im] = (psPtr *) ptExpand;

        psPlaneTransform *ptContract = psPlaneTransformAlloc(2, 2);
        ptContract->x->coeff[0][0] = -TST00_OFFSET_X;
        ptContract->x->coeff[1][0] = 1.0 / TST00_EXPANSION_FACTOR_X;
        ptContract->y->coeff[0][0] = -TST00_OFFSET_Y;
        ptContract->y->coeff[0][1] = 1.0 / TST00_EXPANSION_FACTOR_Y;
        contractTransforms->data[im] = (psPtr *) ptContract;
    }

    //-------------------------------------------------------------------------
    //
    // XXX: psRejectPixels() has known bugs.  Specifically, in the psImageTransform() call.
    // We exclude this from our tests.
    //
    printf("\n\n\nCalling pmRejectPixels() with acceptable data.  Should generate a psArray.\n");
    psArray *pixelRejects = pmRejectPixels(images, NULL, questionablePixels, expandTransforms,
                                           contractTransforms, TST00_REJECTION_THRESHOLD,
                                           TST00_GRADIENT_LIMIT);
    if (pixelRejects == NULL) {
        printf("TEST ERROR: pmRejectPixels() returned a NULL psArray.\n");
        testStatus = true;
    } else {
        // XXX: We should internally verify this with the pixels list, not merely use the stdout.
        for (psS32 i = 0 ; i < pixelRejects->n ; i++) {
            psPixels *myPixels = (psPixels *) pixelRejects->data[i];
            printf("tst_pmImageCombine.c: Image %d had %ld rejects.\n", i, myPixels->n);

            for (psS32 p = 0 ; p < myPixels->n ; p++) {
                printf("Image %d, rejected pixel %d is (%f %f)\n", i, p,
                       myPixels->data[p].y, myPixels->data[p].x);
            }
        }
        psFree(pixelRejects);
    }

    psFree(images);
    psFree(errors);
    psFree(masks);
    psFree(imagesLong);
    psFree(errorsLong);
    psFree(masksLong);
    psFree(imagesBadType);
    psFree(errorsBadType);
    psFree(masksBadType);
    psFree(pixels);
    psFree(stats);

    return(testStatus);
}

/*******************************************************************************
NOTE: This function returns TRUE if there were no errors.
 ******************************************************************************/
int test00( void )
{
    bool testStatus = false;

    testStatus|= testCombineImages(10, 10, 5);

    return(!testStatus);
}

/** @file  pmImageCombine.c
 *
 *  This file will perform image combination of several images of the
 *  same field, produce a list of questionable pixels, then tag some
 *  of those pixels as cosmic rays.
 *
 *  @author Paul Price, IfA (original prototype)
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  XXX: pmRejectPixels() has a known bug with the pmImageTransform() call.
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

// XXX this is somewhat messy and unclear on the masking.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <config.h>
#include <stdio.h>
#include <math.h>
#include "pslib.h"

#define PIXEL_LIST_BUFFER 100           // Size of the pixel list buffer

// Data structure for use as a buffer in combining pixels
typedef struct
{
    psVector *pixels;                   // Pixel values
    psVector *masks;                    // Pixel masks
    psVector *errors;                   // Pixel errors
    psStats *stats;                     // Statistics to use with combination
}
combineBuffer;

void combineBufferFree(combineBuffer *buffer)
{
    psFree(buffer->pixels);
    psFree(buffer->masks);
    psFree(buffer->errors);
    psFree(buffer->stats);
}

combineBuffer *combineBufferAlloc(long numImages // Number of images that will be combined
                                 )
{
    combineBuffer *buffer = psAlloc(sizeof(combineBuffer));
    psMemSetDeallocator(buffer, (psFreeFunc)combineBufferFree);

    buffer->pixels = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->masks = psVectorAlloc(numImages, PS_TYPE_VECTOR_MASK);
    buffer->errors = psVectorAlloc(numImages, PS_TYPE_F32);
    buffer->stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);

    return buffer;
}


static bool combinePixels(psImage *combine, // Combined image, for output
                          psArray *questionablePixels, // Array of rejection masks
                          int x, int y, // Position in the images
                          const psArray *images, // Array of input images
                          const psArray *errors, // Array of input error images
                          const psArray *masks, // Array of input masks
                          psImageMaskType maskVal, // Mask value
                          psS32 numIter, // Number of rejection iterations
                          psF32 sigmaClip, // Number of standard deviations at which to reject
                          combineBuffer *buffer // Buffer for combination; to avoid multiple allocations
                         )
{
    assert(combine);
    assert(x >= 0 && x < combine->numCols);
    assert(y >= 0 && y < combine->numRows);
    assert(images);
    int numImages = images->n;          // Number of images to combine
    if (masks) {
        assert(masks->n == numImages);
    }
    if (errors) {
        assert(errors->n == numImages);
    }
    assert(numIter >= 0);
    assert(sigmaClip > 0);
    assert(!questionablePixels || (questionablePixels && questionablePixels->n == numImages));

    if (buffer) {
        psMemIncrRefCounter(buffer);
    } else {
        buffer = combineBufferAlloc(numImages);
    }

    psVector *pixelData = buffer->pixels; // Values for the pixel of interest
    psVector *pixelErrors = buffer->errors; // Errors for the pixel of interest
    psVector *pixelMasks = buffer->masks; // Masks for the pixel of interest
    psStats *stats = buffer->stats;     // Statistics for combination

    //
    // Loop through each image, extract the pixel/mask/error data into psVectors.
    //
    if (!masks) {
        psVectorInit(pixelMasks, 0);
    }
    if (!errors) {
        pixelErrors = NULL;
    }
    for (int i = 0; i < numImages; i++) {
        // Set the pixel data
        psImage *image = images->data[i]; // Image of interest
        pixelData->data.F32[i] = image->data.F32[y][x];
        // Set the pixel mask data, if necessary
        if (masks) {
            psImage *mask = masks->data[i]; // Mask of interest
            pixelMasks->data.PS_TYPE_VECTOR_MASK_DATA[i] = (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal);
        }
        // Set the pixel error data, if necessary
        if (errors) {
            psImage *error = errors->data[i]; // Error image of interest
            pixelErrors->data.F32[i] = error->data.F32[y][x];
        }
    }

    //
    // Iterate on the pixels, rejecting outliers
    //
    for (int iter = 0; iter < numIter; iter++) {
        // Combine all the pixels, using the specified stat.
        if (!psVectorStats(stats, pixelData, pixelErrors, pixelMasks, 0xff)) {
            psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
            return false;
        }
        if (isnan(stats->sampleMean)) {
            combine->data.F32[y][x] = NAN;
            psFree(buffer);
            return false;
        }
        float combinedPixel = stats->sampleMean; // Value of the combination

        if (iter == 0) {
            // Save the value produced with no rejection, since it may be useful later
            // (if the rejection turns out to be unnecessary)
            combine->data.F32[y][x] = combinedPixel;
        }

        //
        // Reject all pixels that lie more that sigmaClip standard deviations from
        // the combined pixel value.
        //
        int numRejects = 0;     // Number of rejections
        float stdev = stats->sampleStdev;
        for (int i = 0; i < numImages; i++) {
            if (!(pixelMasks->data.PS_TYPE_VECTOR_MASK_DATA[i] & 0xff) &&
                    fabs(pixelData->data.F32[i] - combinedPixel) > sigmaClip * stdev) {
                // Reject pixel as questionable
                numRejects++;
                pixelMasks->data.PS_TYPE_IMAGE_MASK_DATA[i] = 0xff;
                if (questionablePixels) {
                    // Mark the pixel as questionable
                    psPixels *qp = questionablePixels->data[i]; // Questionable pixels for this image
                    int qpNum = qp->n; // Number of QPs in the image of interest
                    if (qpNum >= qp->nalloc) {
                        // Grow dynamically, if required
                        qp = psPixelsRealloc(qp, qp->nalloc + PIXEL_LIST_BUFFER);
                        questionablePixels->data[i] = qp;
                    }
                    qp->data[qpNum].x = x;
                    qp->data[qpNum].y = y;
                    qp->n++;
                }
            }
        }

        //
        // If the number of rejected pixels is zero, then there's no point continuing the loop.
        //
        if (numRejects == 0) {
            break;
        }

        //
        // XXX: Is it possible to have all pixels rejected?  If so, we should exit the loop.
        //
    }

    psFree(buffer);
    return true;
}

psImage *pmCombineImages(
    psImage *combine,                   ///< Combined image (output)
    psArray **questionablePixels,       ///< Array of rejection masks
    const psArray *images,              ///< Array of input images
    const psArray *errors,              ///< Array of input error images
    const psArray *masks,               ///< Array of input masks
    psImageMaskType maskVal,                      ///< Mask value
    const psPixels *pixels,             ///< Pixels to combine
    psS32 numIter,                      ///< Number of rejection iterations
    psF32 sigmaClip                     ///< Number of standard deviations at which to reject
)
{
    psTrace("psModules.imcombine", 3, "Calling pmCombineImages(%ld)\n", images->n);

    PS_ASSERT_ARRAY_NON_NULL(images, NULL);
    PS_ASSERT_INT_POSITIVE(images->n, NULL);
    long numImages = images->n;          // Number of images
    int numCols = ((psImage*)images->data[0])->numCols; // Size in x
    int numRows = ((psImage*)images->data[0])->numRows; // Size in y

    if (combine) {
        PS_ASSERT_IMAGE_NON_NULL(combine, NULL);
        PS_ASSERT_IMAGE_SIZE(combine, numCols, numRows, NULL);
    }
    if (questionablePixels && !*questionablePixels) {
        PS_ASSERT_ARRAY_NON_NULL(*questionablePixels, NULL);
        PS_ASSERT_ARRAY_SIZE(*questionablePixels, numImages, 0);
    }
    for (int i = 1; i < images->n; i++) {
        psImage *image = images->data[i]; // Image of interest
        PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, NULL);
        PS_ASSERT_IMAGE_SIZE(image, numCols, numRows, NULL);
    }
    if (errors) {
        PS_ASSERT_ARRAYS_SIZE_EQUAL(images, errors, NULL);
        for (int i = 0; i < images->n; i++) {
            psImage *error = errors->data[i];
            PS_ASSERT_IMAGE_SIZE(error, numCols, numRows, NULL);
            PS_ASSERT_IMAGE_TYPE(error, PS_TYPE_F32, NULL);
        }
    }
    if (masks) {
        PS_ASSERT_ARRAYS_SIZE_EQUAL(images, masks, NULL);
        for (int i = 0; i < images->n; i++) {
            psImage *mask  = masks->data[i];
            PS_ASSERT_IMAGE_SIZE(mask, numCols, numRows, NULL);
            PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
        }
    }
    PS_ASSERT_INT_POSITIVE(numIter, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(sigmaClip, 0.0, NULL);

    // Allocate and initialize the combined image, if necessary.
    if (!combine) {
        combine = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        if (pixels) {
            // Set everything we're not working on to NAN
            psImageInit(combine, NAN);
        }
    }

    //
    // Allocate the questionablePixels psArray, if necesssary, then create a psPixels
    // struct for each image.
    //
    if (questionablePixels) {
        if (*questionablePixels == NULL) {
            *questionablePixels = psArrayAlloc(numImages);
        } else if ((*questionablePixels)->n != numImages) {
            *questionablePixels = psArrayRealloc(*questionablePixels, numImages);
        }
        for (int i = 0; i < numImages; i++) {
            psFree((*questionablePixels)->data[i]);
            (*questionablePixels)->data[i] = psPixelsAlloc(PIXEL_LIST_BUFFER);
        }
    }

    combineBuffer *buffer = combineBufferAlloc(numImages); // Buffer for combination

    if (pixels) {
        // Only those specified pixels should be combined.

        for (int p = 0; p < pixels->n; p++) {
            int x = pixels->data[p].x; // Column of interest
            int y = pixels->data[p].y; // Row of interest

            if (!combinePixels(combine, questionablePixels ? *questionablePixels : NULL, x, y,
                               images, errors, masks, maskVal, numIter, sigmaClip, buffer)) {
                // Bad pixel --- no big deal
                psErrorClear();
            }
        }
    } else {
        //
        // We get here if there is a NULL list of pixels to combine.
        // Therefore, we combine all pixels in all images.
        //

        //
        // Loop over all pixels in all images, set the appropriate data, mask,
        // error vectors, call psVectorStats(), and set the result in the
        // combine image.
        //
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (!combinePixels(combine, questionablePixels ? *questionablePixels : NULL, x, y,
                                   images, errors, masks, maskVal, numIter, sigmaClip, buffer)) {
                    // Bad pixel --- no big deal
                    psErrorClear();
                }
            }
        }
    }

    psFree(buffer);

    psTrace("psModules.imcombine", 3, "Exiting pmCombineImages(%ld)\n", images->n);
    return combine;
}


/******************************************************************************
XXX: Directly from Paul Price
 *****************************************************************************/
static psF32 CalcGradient(
    psImage *image,
    psImage *imageMask,
    psS32 x,
    psS32 y
)
{
    psTrace("psModules.imcombine", 4, "Calling CalcGradient(%d, %d)\n", x, y);
    int num = 0;
    psVector *pixels = psVectorAlloc(8, PS_TYPE_F32); // Array of pixels
    psVector *mask = psVectorAlloc(8, PS_TYPE_VECTOR_MASK); // Corresponding mask

    // Get limits
    int xMin = PS_MAX(x - 1, 0);
    int xMax = PS_MIN(x + 1, image->numCols - 1);
    int yMin = PS_MAX(y - 1, 0);
    int yMax = PS_MIN(y + 1, image->numRows - 1);
    if (imageMask != NULL) {
        for (int j = yMin; j <= yMax; j++) {
            for (int i = xMin; i <= xMax; i++) {
                if ((i != x) && (j != y) && (0 == imageMask->data.PS_TYPE_IMAGE_MASK_DATA[j][i])) {
                    pixels->data.F32[num] = image->data.F32[j][i];
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[num] = 0;
                    num++;
                } else {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[num] = 1;
                }
            }
        }
    } else {
        //
        // This code is simply the previous loop without the imageMask.
        // XXX: Consider restructuring this.
        //
        for (int j = yMin; j <= yMax; j++) {
            for (int i = xMin; i <= xMax; i++) {
                if ((i != x) && (j != y)) {
                    pixels->data.F32[num] = image->data.F32[j][i];
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[num] = 0;
                    num++;
                } else {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[num] = 1;
                }
            }
        }
    }

    pixels->n = num;
    mask->n = num;

    // Get the median
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    if (!psVectorStats(stats, pixels, NULL, mask, 1)) {
        psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
    }
    float median = stats->sampleMedian;
    psFree(stats);
    psFree(pixels);
    psFree(mask);

    psTrace("psModules.imcombine", 4, "Exiting CalcGradient(%d, %d)\n", x, y);
    return(median / image->data.F32[y][x]);
}

/******************************************************************************
DetermineRegion(image, myOutToIn): for a psImage and a psPlaneTransform to that
image, this routine determines the size of the input image which maps to that
image, and returns the result in a psRegion struct.

XXX: Basically, this routine is only guaranteed to work if the transform is
linear.

XXX: Shouldn't this functionality be part of psImageTransform()?
 *****************************************************************************/
static psRegion DetermineRegion(psImage *image,
                                psPlaneTransform *myOutToIn)
{
    psTrace("psModules.imcombine", 4, "Calling DetermineRegion()\n");
    psRegion myRegion;
    myRegion.x0 = PS_MAX_F32;
    myRegion.x1 = PS_MIN_F32;
    myRegion.y0 = PS_MAX_F32;
    myRegion.y1 = PS_MIN_F32;
    psPlane in;
    psPlane out;

    in.x = 0.0;
    in.y = 0.0;

    psPlaneTransformApply(&out, myOutToIn, &in);
    if (out.x < myRegion.x0) {
        myRegion.x0 = out.x;
    }
    if (out.x > myRegion.x1) {
        myRegion.x1 = out.x;
    }
    if (out.y < myRegion.y0) {
        myRegion.y0 = out.y;
    }
    if (out.y > myRegion.y1) {
        myRegion.y1 = out.y;
    }

    in.x = (psF32) (image->numCols);
    in.y = 0.0;
    psPlaneTransformApply(&out, myOutToIn, &in);
    if (out.x < myRegion.x0) {
        myRegion.x0 = out.x;
    }
    if (out.x > myRegion.x1) {
        myRegion.x1 = out.x;
    }
    if (out.y < myRegion.y0) {
        myRegion.y0 = out.y;
    }
    if (out.y > myRegion.y1) {
        myRegion.y1 = out.y;
    }

    in.x = (psF32) (image->numCols);
    ;
    in.y = 0.0;
    psPlaneTransformApply(&out, myOutToIn, &in);
    if (out.x < myRegion.x0) {
        myRegion.x0 = out.x;
    }
    if (out.x > myRegion.x1) {
        myRegion.x1 = out.x;
    }
    if (out.y < myRegion.y0) {
        myRegion.y0 = out.y;
    }
    if (out.y > myRegion.y1) {
        myRegion.y1 = out.y;
    }

    in.x = (psF32) (image->numCols);
    in.y = (psF32) (image->numRows);
    psPlaneTransformApply(&out, myOutToIn, &in);
    if (out.x < myRegion.x0) {
        myRegion.x0 = out.x;
    }
    if (out.x > myRegion.x1) {
        myRegion.x1 = out.x;
    }
    if (out.y < myRegion.y0) {
        myRegion.y0 = out.y;
    }
    if (out.y > myRegion.y1) {
        myRegion.y1 = out.y;
    }

    psTrace("psModules.imcombine", 4, "Exiting DetermineRegion()\n");
    return(myRegion);
}

/******************************************************************************
XXX: Don't we have a psLib function for this?
 *****************************************************************************/
static psImage *ImageConvertF32(psImage *image)
{
    psTrace("psModules.imcombine", 4, "Calling ImageConvertF32()\n");
    psImage *imgF32 = psImageAlloc(image->numCols, image->numRows, PS_TYPE_F32);

    for (psS32 i = 0 ; i < image->numRows ; i++) {
        for (psS32 j = 0 ; j < image->numCols ; j++) {
            imgF32->data.F32[i][j] = (psF32) image->data.PS_TYPE_IMAGE_MASK_DATA[i][j];
        }
    }

    psTrace("psModules.imcombine", 4, "Exiting ImageConvertF32()\n");
    return(imgF32);
}


//
// The following macros define how big the initial pixel list will be, and
// how much it should be incremented when realloc'ed.
//
#define PS_REJECT_PIXEL_INITIAL_PIXEL_LIST_LENGTH 100
#define PS_REJECT_PIXEL_INITIAL_PIXEL_LIST_LENGTH_INC 100
/******************************************************************************
pmRejectPixels(images, errors, inToOut, outToIn, rejThreshold,
gradLimit)

XXX: Optimization: we don't need to transform the entire mask image.
XXX: The inToOut and outToIn transforms are confusing.  Verify that what
     I think they mean syncs with PWP.
 *****************************************************************************/
psArray *pmRejectPixels(
    const psArray *images,              ///< Array of input images
    const psArray *masks,               ///< Array of input image masks
    const psArray *errors,              ///< The pixels which were rejected in the combination
    const psArray *inToOut,             ///< Transformation from input to output system
    const psArray *outToIn,             ///< Transformation from output to input system
    psF32 rejThreshold,                 ///< Rejection threshold
    psF32 gradLimit                     ///< Gradient limit
)
{
    psTrace("psModules.imcombine", 3, "Calling pmRejectPixels()\n");
    PS_ASSERT_PTR_NON_NULL(images, NULL);
    for (psS32 im = 0 ; im < images->n ; im++) {
        psImage *tmpImage = (psImage *) images->data[im];
        PS_ASSERT_IMAGE_NON_NULL(tmpImage, NULL);
        PS_ASSERT_IMAGE_NON_EMPTY(tmpImage, NULL);
        PS_ASSERT_IMAGE_TYPE(tmpImage, PS_TYPE_F32, NULL);
        if (masks != NULL) {
            PS_ASSERT_INT_EQUAL(images->n, masks->n, NULL);
            psImage *tmpMask = (psImage *) masks->data[im];
            PS_ASSERT_IMAGE_NON_NULL(tmpMask, NULL);
            PS_ASSERT_IMAGE_NON_EMPTY(tmpMask, NULL);
            PS_ASSERT_IMAGE_TYPE(tmpMask, PS_TYPE_F32, NULL); // XXX really F32??
            PS_ASSERT_IMAGES_SIZE_EQUAL(tmpImage, tmpMask, NULL);
        }
        PS_ASSERT_IMAGES_SIZE_EQUAL(((psImage *) images->data[0]), tmpImage, NULL);
    }
    PS_ASSERT_PTR_NON_NULL(errors, NULL);
    PS_ASSERT_PTR_NON_NULL(inToOut, NULL);
    PS_ASSERT_PTR_NON_NULL(outToIn, NULL);
    // Ensure that the psArray parameters have an element for each image.
    psS32 numImages = images->n;
    PS_ASSERT_INT_EQUAL(numImages, errors->n, NULL);
    PS_ASSERT_INT_EQUAL(numImages, inToOut->n, NULL);
    PS_ASSERT_INT_EQUAL(numImages, outToIn->n, NULL);

    //
    // Create the psArray of psPixelLists, one for each image, for rejected pixels.
    //
    psArray *rejects = psArrayAlloc(numImages);
    for (psS32 im = 0 ; im < numImages ; im++) {
        rejects->data[im] = (psPtr *) psPixelsAlloc(PS_REJECT_PIXEL_INITIAL_PIXEL_LIST_LENGTH);
        ((psPixels *)(rejects->data[im]))->n = ((psPixels *)(rejects->data[im]))->nalloc;
        psPixels *pixels = (psPixels *) rejects->data[im];
        pixels->n = 0;
    }
    //
    // rPtr is used to maintain a count of the questionable pixels for each image.
    //
    psVector *rPtr = psVectorAlloc(numImages, PS_TYPE_S32);
    psVectorInit(rPtr, 0);

    psS32 numCols = ((psImage *) images->data[0])->numCols;
    psS32 numRows = ((psImage *) images->data[0])->numRows;
    psRegion myRegion = psRegionSet(0, numCols-1, 0, numRows-1);
    psU32 maskVal = 1;  // XXX: Is this appropriate?

    psPlane *inCoords = psAlloc(sizeof(psPlane));
    psPlane *outCoords = psAlloc(sizeof(psPlane));

    for (psS32 im = 0 ; im < numImages ; im++) {
        //
        // Extract data from psArrays.
        //
        psPixels *pixelList = (psPixels *) errors->data[im];

        psImage *currImage = (psImage *) images->data[im];
        myRegion.x0 = 0;
        myRegion.x1 = currImage->numCols;
        myRegion.y0 = 0;
        myRegion.y1 = currImage->numRows;
        psPlaneTransform *myInToOut = (psPlaneTransform *) inToOut->data[im];
        psPlaneTransform *myOutToIn = (psPlaneTransform *) outToIn->data[im];

        //
        // Create a psImageMaskType mask image from the list of cosmic pixels.
        //
        psImage *maskImage = NULL;
        maskImage = psPixelsToMask(maskImage, pixelList, myRegion, maskVal);
        psImage *maskImageF32 = ImageConvertF32(maskImage);

        //
        // Transform that mask image into detector coordinate space
        //
        psRegion myRegionXForm = DetermineRegion(maskImageF32, myOutToIn);
        psImage *transformedImage = psImageTransform(NULL, NULL, maskImageF32, NULL,
                                    0, myOutToIn, myRegionXForm, NULL,
                                    PS_INTERPOLATE_BILINEAR, 0);

        //
        // Loop over all cosmic pixels.  Transform their coords to detector space.
        // If the value of the transformed mask is larger than rejThreshold, then
        // this might be a cosmic ray pixel.  We then calculate the mean gradient
        // in other images.
        //

        psImageInterpolateOptions *interp = psImageInterpolateOptionsAlloc(PS_INTERPOLATE_BILINEAR,
                                                                           transformedImage, NULL, NULL,
                                                                           0, 0.0, 0.0, 0, 0, 0.0);

        for (psS32 p = 0 ; p < pixelList->n ; p++) {
            inCoords->x = 0.5 + (psF32) (pixelList->data[p]).x;
            inCoords->y = 0.5 + (psF32) (pixelList->data[p]).y;
            psPlaneTransformApply(outCoords, myInToOut, inCoords);
            double maskVal;
            if (!psImageInterpolate(&maskVal, NULL, NULL, outCoords->x, outCoords->y, interp)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image.");
                psFree(interp);
                psFree(maskImage);
                psFree(maskImageF32);
                psFree(transformedImage);
                psFree(inCoords);
                psFree(outCoords);
                psFree(rejects);
                return NULL;
            }
            if (maskVal > rejThreshold) {

                // This is a possible cosmic array pixel.  We must calculate the gradient
                // at this location in all input images.
                psF32 meanGrads = 0.0;
                psS32 numGrads = 0;
                //
                // Loop through all other images, calculate their mean gradient.
                //
                for (psS32 otherImg = 0 ; otherImg < numImages ; otherImg++) {
                    if (im != otherImg) {
                        // Map the outCoords to inCoords that for otherImg space.
                        psImage *tmpMask = NULL;
                        if (masks != NULL) {
                            tmpMask = masks->data[otherImg];
                        }
                        psPlaneTransformApply(inCoords,
                                              (psPlaneTransform * )outToIn->data[otherImg],
                                              outCoords);
                        psS32 xPix = (int)(inCoords->x - 0.5);
                        psS32 yPix = (int)(inCoords->y - 0.5);
                        if ((xPix >= 0) && (xPix <= ((psImage*)(images->data[otherImg]))->numCols - 1) &&
                                (yPix >= 0) && (yPix <= ((psImage*)(images->data[otherImg]))->numRows - 1)) {
                            meanGrads += CalcGradient(images->data[otherImg], tmpMask, xPix, yPix);
                            numGrads++;
                        }
                    }
                }
                if (numGrads > 0) {
                    meanGrads /= (psF32) numGrads;
                } else {
                    // XXX: my idea.  Verify with PWP:
                    meanGrads = 1.0 + gradLimit;
                }

                // XXX: The SDRS and the prototype code differ significantly here:
                // if (CalcGradient(inputs->data[i], pixelList->data.x, pixelList->data.y) < (gradLimit * meanGrads)) {
                if (meanGrads < gradLimit) {
                    //
                    // Add this to the list of questionable pixels.  We must ensure that the
                    // pixelList is large enough; if not, we realloc()
                    //
                    psS32 ptr = rPtr->data.S32[im];
                    psPixels *pixelListPtr = (psPixels *) rejects->data[im];
                    if (ptr >= pixelListPtr->nalloc) {
                        rejects->data[im] = (psPtr *) psPixelsRealloc(((psPixels *) rejects->data[im]),
                                            ((((psPixels *) rejects->data[im])->nalloc) + PS_REJECT_PIXEL_INITIAL_PIXEL_LIST_LENGTH_INC));
                        // XXX: Can the realloc() fail?  Must we check for NULL?
                    }

                    ((psPixels *) rejects->data[im])->data[ptr].x = (pixelList->data[p]).x;
                    ((psPixels *) rejects->data[im])->data[ptr].y = (pixelList->data[p]).y;
                    (rPtr->data.S32[im])++;
                    // XXX: this pixel ->n increment is wierd
                    (((psPixels *) rejects->data[im])->n)++;
                }
            }
        }

        psFree(interp);
        psFree(maskImage);
        psFree(maskImageF32);
        psFree(transformedImage);
    }

    psFree(inCoords);
    psFree(outCoords);
    psTrace("psModules.imcombine", 3, "Exiting pmRejectPixels()\n");
    return(rejects);
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionThreads.h"
#include "pmSubtractionKernels.h"

#include "pmStackReject.h"

#define PIXEL_LIST_BUFFER 100           // Number of pixels to add to list at a time

//#define TESTING                         // Testing output

// Mask values
typedef enum {
    PM_STACK_MASK_BAD      = 0x01,      // Bad pixel
    PM_STACK_MASK_CONVOLVE = 0x02,      // Touching a bad pixel
    PM_STACK_MASK_ALL      = 0xff,      // All mask bits
} pmStackMask;

static bool threaded = false;           // Running threaded?


// Grow the rejection mask
static inline bool stackRejectGrow(psImage *target,   // Target mask image (product)
                                   psImage *source, // Source mask image (to be grown)
                                   const pmSubtractionKernels *kernels, // Subtraction kernels
                                   int numCols, int numRows, // Size of image
                                   int xMin, int xMax, int yMin, int yMax, // Bounds of convolution
                                   float poorFrac       // Fraction for "poor"
    )
{
    int size = kernels->size;           // Half-size of convolution kernel
    int x = PS_MIN(xMin + size + 1, kernels->xMax); // x coordinate of interest
    int y = PS_MIN(yMin + size + 1, kernels->yMax); // y coordinate of interest

    psImage *polyValues = p_pmSubtractionPolynomialFromCoords(NULL, kernels, x, y); // Polynomial
    int box = p_pmSubtractionBadRadius(NULL, kernels, polyValues, false, poorFrac); // Radius of bad box
    psTrace("psModules.imcombine", 10, "Growing by %d", box);
    psFree(polyValues);

    if (box > 0) {
        // Convolve a subimage, then stick it in the target
        psImage *mask = psImageSubset(source, psRegionSet(xMin - box, xMax + box,
                                                          yMin - box, yMax + box)); // Mask to convolve
        psImage *convolved = psImageConvolveMask(NULL, mask, PM_STACK_MASK_BAD, PM_STACK_MASK_CONVOLVE,
                                                 -box, box, -box, box); // Convolved mask
        psFree(mask);

        int numBytes = (xMax - xMin) * PSELEMTYPE_SIZEOF(PS_TYPE_IMAGE_MASK); // Number of bytes to copy
        psAssert(convolved->numCols - 2 * box == xMax - xMin, "Bad number of columns");
        psAssert(convolved->numRows - 2 * box == yMax - yMin, "Bad number of rows");

        for (int yTarget = yMin, ySource = box; yTarget < yMax; yTarget++, ySource++) {
            memcpy(&target->data.PS_TYPE_IMAGE_MASK_DATA[yTarget][xMin],
                   &convolved->data.PS_TYPE_IMAGE_MASK_DATA[ySource][box], numBytes);
        }
        psFree(convolved);
    } else {
        // Just copy over
        int numBytes = (xMax - xMin) * PSELEMTYPE_SIZEOF(PS_TYPE_IMAGE_MASK); // Number of bytes to copy
        for (int yTarget = yMin; yTarget < yMax; yTarget++) {
            memcpy(&target->data.PS_TYPE_IMAGE_MASK_DATA[yTarget][xMin],
                   &source->data.PS_TYPE_IMAGE_MASK_DATA[yTarget][xMin], numBytes);
        }
    }

    return true;
}

// Thread entry for stackRejectGrow
static bool stackRejectGrowThread(psThreadJob *job // Job to execute
    )
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Job arguments
    psImage *target = args->data[0];    // Target mask image
    psImage *source = args->data[1];    // Source mask image
    const pmSubtractionKernels *kernels = args->data[2]; // Subtraction kernels
    int numCols = PS_SCALAR_VALUE(args->data[3], S32); // Number of columns
    int numRows = PS_SCALAR_VALUE(args->data[4], S32); // Number of rows
    int xMin = PS_SCALAR_VALUE(args->data[5], S32); // Minimum x value
    int xMax = PS_SCALAR_VALUE(args->data[6], S32); // Maximum x value
    int yMin = PS_SCALAR_VALUE(args->data[7], S32); // Minimum y value
    int yMax = PS_SCALAR_VALUE(args->data[8], S32); // Maximum y value
    float poorFrac = PS_SCALAR_VALUE(args->data[9], F32); // Fraction for "poor"

    return stackRejectGrow(target, source, kernels, numCols, numRows, xMin, xMax, yMin, yMax, poorFrac);
}

bool pmStackRejectThreadsInit(void)
{
    if (threaded) {
        psAbort("Already running threaded.");
    }
    threaded = true;

    if (!pmSubtractionThreaded()) {
        pmSubtractionThreadsInit();
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PSMODULES_STACK_REJECT_GROW", 10);
        task->function = &stackRejectGrowThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    return true;
}


psPixels *pmStackReject(const psPixels *in, int numCols, int numRows, float threshold, int stride,
                        const psArray *subRegions, const psArray *subKernels)
{
    PS_ASSERT_PIXELS_NON_NULL(in, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(threshold, 0.0, NULL);
    PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(threshold, 1.0, NULL);

    if (!subRegions || !subKernels) {
      psTrace("psModules.imcombine",2,"Do not have the necessary kernels and regions, returning input pixels.");
      psPixels *out = psPixelsCopy(NULL,in);
      return out;
    }
    
    PS_ASSERT_ARRAY_NON_NULL(subRegions, NULL);
    PS_ASSERT_ARRAY_NON_NULL(subKernels, NULL);
    PS_ASSERT_ARRAYS_SIZE_EQUAL(subRegions, subKernels, NULL);

    // Trivial case
    if (in->n == 0) {
        return psPixelsAllocEmpty(0);
    }

    // Check consistency of kernels
    int numRegions = subRegions->n;     // Number of regions
    int size = 0;                       // Size of kernel
    for (int i = 0; i < numRegions; i++) {
        pmSubtractionKernels *kernels = subKernels->data[i]; // Kernel of interest
        if (size == 0) {
            size = kernels->size;
        } else if (kernels->size != size) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Kernel sizes are not identical: %d vs %d",
                    size, kernels->size);
            return NULL;
        }
    }

    psImage *mask = psPixelsToMask(NULL, in, psRegionSet(0, numCols - 1, 0, numRows - 1), 1); // Mask
    psImage *image = psImageCopy(NULL, mask, PS_TYPE_F32); // Floating-point version, so we can convolve
    psFree(mask);

    // Convolve the image with the kernel --- we're basically applying a matched filter and then thresholding
    pmReadout *convRO = pmReadoutAlloc(NULL); // Readout with convolved image
    pmReadout *inRO = pmReadoutAlloc(NULL); // Readout with input image
    inRO->image = image;
    convRO->image = psImageAlloc(image->numCols, image->numRows, PS_TYPE_F32);
    for (int i = 0; i < numRegions; i++) {
        psRegion *region = subRegions->data[i]; // Region of interest
        pmSubtractionKernels *kernels = subKernels->data[i]; // Kernel of interest
        if (!pmSubtractionConvolve(NULL, convRO, NULL, inRO, NULL, stride, 0, 0, 1.0, 0.0, 0.0,
                                   region, kernels, false, true)) {
            psError(psErrorCodeLast(), false, "Unable to convolve mask image in region %d.", i);
            psFree(convRO);
            psFree(inRO);
            return NULL;
        }

        // Need to adjust the thresholding level for the normalisation of the kernel --- the application of
        // the kernel may scale the unit level that we've inserted.

        // Image of the kernel at the centre of the region
        psImage *kernel = pmSubtractionKernelImage(kernels, 0.5, 0.5, false);
        if (!kernel) {
            psError(psErrorCodeLast(), false, "Unable to generate kernel image.");
            psFree(convRO);
            psFree(inRO);
            return NULL;
        }
        float sum = 0.0;
        for (int y = 0; y < kernel->numRows; y++) {
            for (int x = 0; x < kernel->numCols; x++) {
                sum += kernel->data.F32[y][x];
            }
        }
        psFree(kernel);

        // Range for normalisation
        int xMin = PS_MAX(0, region->x0), xMax = PS_MIN(numCols - 1, region->x1);
        int yMin = PS_MAX(0, region->y0), yMax = PS_MIN(numRows - 1, region->y1);
        psTrace("psModules.imcombine", 2, "Normalising convolved mask image by %f over %d:%d,%d:%d\n",
                sum, xMin, xMax, yMin, yMax);
        sum = 1.0 / sum;
        for (int y = yMin; y <= yMax; y++) {
            for (int x = xMin; x <= xMax; x++) {
                convRO->image->data.F32[y][x] *= sum;
            }
        }
    }
    psFree(inRO);
    psImage *convolved = psMemIncrRefCounter(convRO->image);
    psFree(convRO);

#ifdef TESTING
    {
        static int seqNum = 0;          // Sequence number
        psString name = NULL;           // Name of image
        psStringAppend(&name, "inspect_conv_%02d.fits", seqNum);
        seqNum++;
        psFits *fits = psFitsOpen(name, "w"); // FITS file pointer
        psFree(name);
        psFitsWriteImage(fits, NULL, convolved, 0, NULL);
        psFitsClose(fits);
    }
#endif

    // Threshold the convolved image
    psPixels *bad = psPixelsAllocEmpty(PIXEL_LIST_BUFFER); // List of pixels that should be masked
    for (int y = size; y < convolved->numRows - size; y++) {
        for (int x = size; x < convolved->numCols - size; x++) {
            if (convolved->data.F32[y][x] > threshold) {
                bad = psPixelsAdd(bad, bad->nalloc, x, y);
            }
        }
    }
    psTrace("psModules.imcombine", 7, "Found %ld bad pixels", bad->n);
    psFree(convolved);

    return bad;
}


psPixels *pmStackRejectGrow(const psPixels *in, int numCols, int numRows, float poorFrac,
                            const psArray *subRegions, const psArray *subKernels)
{
    PS_ASSERT_PIXELS_NON_NULL(in, NULL);
    PS_ASSERT_ARRAY_NON_NULL(subRegions, NULL);
    PS_ASSERT_ARRAY_NON_NULL(subKernels, NULL);
    PS_ASSERT_ARRAYS_SIZE_EQUAL(subRegions, subKernels, NULL);

    psImage *source = psPixelsToMask(NULL, in, psRegionSet(0, numCols - 1, 0, numRows - 1),
                                     PM_STACK_MASK_BAD); // Mask image to grow

#ifdef TESTING
    {
        static int seqNum = 0;          // Sequence number
        psString name = NULL;           // Name of image
        psStringAppend(&name, "reject_orig_%02d.fits", seqNum);
        seqNum++;
        psFits *fits = psFitsOpen(name, "w"); // FITS file pointer
        psFree(name);
        psFitsWriteImage(fits, NULL, source, 0, NULL);
        psFitsClose(fits);
    }
#endif

    // Need to set psImageConvolveMask threading OFF because that would generate threads on top of threads
    bool oldThreads = psImageConvolveSetThreads(false); // Old value of threading for psImageColvolve

    psImage *target = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK); // Grown image
    psImageInit(target, 0);
    for (int i = 0; i < subRegions->n; i++) {
        psRegion *region = subRegions->data[i]; // Subtraction region
        pmSubtractionKernels *kernels = subKernels->data[i]; // Subtraction kernel

        int size = kernels->size;           // Half-size of kernel
        int fullSize = 2 * size + 1;        // Full size of kernel

        // Get region for convolution: [xMin:xMax,yMin:yMax]
        int xMin = PS_MAX(region->x0, size), xMax = PS_MIN(region->x1, numCols - size);
        int yMin = PS_MAX(region->y0, size), yMax = PS_MIN(region->y1, numRows - size);

        for (int j = yMin; j < yMax; j += fullSize) {
            int ySubMax = PS_MIN(j + fullSize, yMax); // Range for subregion of interest
            for (int i = xMin; i < xMax; i += fullSize) {
                int xSubMax = PS_MIN(i + fullSize, xMax); // Range for subregion of interest

                if (threaded) {
                    psThreadJob *job = psThreadJobAlloc("PSMODULES_STACK_REJECT_GROW"); // Job to execute
                    psArray *args = job->args; // Job arguments
                    psArrayAdd(args, 1, target);
                    psArrayAdd(args, 1, source);
                    psArrayAdd(args, 1, kernels);
                    PS_ARRAY_ADD_SCALAR(args, numCols, PS_TYPE_S32);
                    PS_ARRAY_ADD_SCALAR(args, numRows, PS_TYPE_S32);
                    PS_ARRAY_ADD_SCALAR(args, i, PS_TYPE_S32);
                    PS_ARRAY_ADD_SCALAR(args, xSubMax, PS_TYPE_S32);
                    PS_ARRAY_ADD_SCALAR(args, j, PS_TYPE_S32);
                    PS_ARRAY_ADD_SCALAR(args, ySubMax, PS_TYPE_S32);
                    PS_ARRAY_ADD_SCALAR(args, poorFrac, PS_TYPE_F32);
                    if (!psThreadJobAddPending(job)) {
                        psFree(source);
                        psFree(target);
                        return NULL;
                    }
                } else if (!stackRejectGrow(target, source, kernels, numCols, numRows,
                                            i, xSubMax, j, ySubMax, poorFrac)) {
                    psError(psErrorCodeLast(), false, "Unable to grow bad pixels.");
                    psFree(source);
                    psFree(target);
                    return NULL;
                }
            }
        }
    }

    if (!psThreadPoolWait(false, true)) {
        psError(psErrorCodeLast(), false, "Unable to grow bad pixels.");
        psFree(source);
        psFree(target);
        return NULL;
    }

    // Harvest the jobs
    if (threaded) {
        psThreadJob *job;                   // Job to destroy
        while ((job = psThreadJobGetDone())) {
            psAssert(strcmp(job->type, "PSMODULES_STACK_REJECT_GROW") == 0,
                     "Job has incorrect type: %s", job->type);
            psFree(job);
        }

    }

    psImageConvolveSetThreads(oldThreads);

#ifdef TESTING
    {
        static int seqNum = 0;          // Sequence number
        psString name = NULL;           // Name of image
        psStringAppend(&name, "reject_grow_%02d.fits", seqNum);
        seqNum++;
        psFits *fits = psFitsOpen(name, "w"); // FITS file pointer
        psFree(name);
        psFitsWriteImage(fits, NULL, target, 0, NULL);
        psFitsClose(fits);
    }
#endif

    psFree(source);
    psPixels *bad = psPixelsFromMask(NULL, target, PM_STACK_MASK_ALL); // All bad pixels
    psFree(target);
    psTrace("psModules.imcombine", 7, "Total %ld bad pixels", bad->n);

    return bad;
}

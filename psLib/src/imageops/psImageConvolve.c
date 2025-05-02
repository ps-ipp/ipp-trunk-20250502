/// @file  psImageConvolve.c
///
/// @brief Contains FFT transform related functions for psImage.
///
/// @author Robert DeSonia, MHPCC
/// @author Paul Price, IfA
/// @author Eugene Magnier, IfA
///
/// @version $Revision: 1.84 $ $Name: not supported by cvs2svn $
/// @date $Date: 2009-02-05 23:56:14 $
///
/// Copyright 2004-2007 Institute for Astronomy, University of Hawaii
///

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include "psAbort.h"
#include "psMemory.h"
#include "psLogMsg.h"
#include "psError.h"
#include "psAssert.h"
#include "psScalar.h"
#include "psBinaryOp.h"
#include "psImageFFT.h"
#include "psImageStructManip.h"
#include "psImagePixelManip.h"
#include "psTrace.h"
#include "psThread.h"

#include "psImageConvolve.h"

#define MIN_GAUSS_FRAC 0.25             // Minimum Gaussian fraction to accept when smoothing

static bool threaded = false;           // Run image convolution threaded?
static pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;

static void kernelFree(psKernel *kernel)
{
    if (kernel) {
        psFree(kernel->image);
        psFree(kernel->p_kernelRows);
    }
    return;
}

// Set up indirections, so we can refer to kernel->kernel[-1][-3] for the (-1,-1) element, instead of
// kernel->image[kernel->yMax - kernel->yMin + 1][kernel->yMax - kernel->yMin + 3] (yuk!).
static void kernelRedirects(psKernel *kernel, // Kernel for which to set up the redirects
                            int numRows // Number of rows
    )
{
    int xMin = kernel->xMin, yMin = kernel->yMin; // Minimum values

    kernel->p_kernelRows = psAlloc(sizeof(float*)*numRows);
    for (int i = 0; i < numRows; i++) {
        kernel->p_kernelRows[i] = kernel->image->data.PS_TYPE_KERNEL_DATA[i] - xMin;
    }
    kernel->kernel = kernel->p_kernelRows - yMin;
    return;
}

psKernel *p_psKernelAlloc(const char *file,
			  unsigned int lineno,
			  const char *func,
			  int xMin, int xMax, int yMin, int yMax)
{
    // Check the inputs to make sure max > min; if not, switch.
    // following is explicitly spelled out in the SDRS as a requirement
    if (yMin > yMax) {
        psWarning("Specified yMin, %d, was greater than yMax, %d.  Values swapped.",
                 yMin, yMax);

        int temp = yMin;
        yMin = yMax;
        yMax = temp;
    }
    if (xMin > xMax) {
        psWarning("Specified xMin, %d, was greater than xMax, %d.  Values swapped.",
                 xMin, xMax);

        int temp = xMin;
        xMin = xMax;
        xMax = temp;
    }

    int numRows = yMax - yMin + 1;      // Number of rows for kernel image
    int numCols = xMax - xMin + 1;      // Number of columns for kernel image

    psKernel *kernel = p_psAlloc(file, lineno, func, sizeof(psKernel)); // The kernel, to be returned
    psMemSetDeallocator(kernel,(psFreeFunc)kernelFree);

    kernel->xMin = xMin;
    kernel->xMax = xMax;
    kernel->yMin = yMin;
    kernel->yMax = yMax;
    kernel->image = psImageAlloc(numCols, numRows, PS_TYPE_KERNEL);
    psImageInit(kernel->image, 0.0);

    kernelRedirects(kernel, numRows);

    return kernel;
}

psKernel *psKernelAllocFromImage(psImage *image, int x0, int y0)
{
    psKernel *kernel = psAlloc(sizeof(psKernel)); // The kernel, to be returned
    psMemSetDeallocator(kernel,(psFreeFunc)kernelFree);

    int numCols = image->numCols, numRows = image->numRows; // Size of image

    kernel->xMin = - x0;
    kernel->xMax = numCols - 1 - x0;
    kernel->yMin = - y0;
    kernel->yMax = numRows - 1 - y0;
    kernel->image = psMemIncrRefCounter(image);

    kernelRedirects(kernel, numRows);

    return kernel;
}

psKernel *psKernelCopy(const psKernel *in)
{
    PS_ASSERT_KERNEL_NON_NULL(in, NULL);

    psKernel *out = psAlloc(sizeof(psKernel)); // The copied kernel, to be returned
    psMemSetDeallocator(out,(psFreeFunc)kernelFree);

    out->image = psImageCopy(NULL, in->image, PS_TYPE_KERNEL);
    out->xMin = in->xMin;
    out->xMax = in->xMax;
    out->yMin = in->yMin;
    out->yMax = in->yMax;

    kernelRedirects(out, out->image->numRows);

    return out;
}

bool psMemCheckKernel(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)kernelFree );
}


psKernel *psKernelGenerate(const psVector *tShifts, const psVector *xShifts, const psVector *yShifts,
                           float totalTime, bool xyRelative)
{
    PS_ASSERT_VECTOR_NON_NULL(tShifts, NULL);
    PS_ASSERT_VECTOR_NON_NULL(xShifts, NULL);
    PS_ASSERT_VECTOR_NON_NULL(yShifts, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(tShifts, xShifts, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(tShifts, yShifts, NULL);
    PS_ASSERT_VECTOR_TYPE(tShifts, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(xShifts, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTOR_TYPE(yShifts, PS_TYPE_S32, NULL);

    if (isnan(totalTime)) {
        // It's more expensive to check for NAN than 0.0
        totalTime = 0.0;
    }

    // If there are no shifts, the kernel is just a 1 at 0,0
    long num = tShifts->n;              // Number of shifts
    if (num == 0) {
        psKernel *kernel = psKernelAlloc(0,0,0,0);
        kernel->kernel[0][0] = 1;
        return kernel;
    }

    // Get dimensions and scaling
    int xMin, xMax, yMin, yMax;         // Range of values for kernel
    int xLast, yLast;                   // Last location, for relative shifts
    float tSum = tShifts->data.F32[0];   // Sum of the times
    xLast = xMin = xMax = xShifts->data.S32[0];
    yLast = yMin = yMax = yShifts->data.S32[0];
    int x0, y0;                         // Final location; everything is relative to this
    x0 = xShifts->data.S32[num - 1];
    y0 = yShifts->data.S32[num - 1];
    for (long i = 1; i < num; i++) {
        int x = xShifts->data.S32[i] - x0; // x position in kernel
        int y = yShifts->data.S32[i] - y0; // y position in kernel
        if (xyRelative) {
            x += xLast;
            y += yLast;
            xLast = x;
            yLast = y;
        }
        if (x < xMin) {
            xMin = x;
        }
        if (x > xMax) {
            xMax = x;
        }
        if (y < yMin) {
            yMin = y;
        }
        if (y > yMax) {
            yMax = y;
        }

        if (totalTime <= 0) {
            tSum += tShifts->data.F32[i];
        }
    }

    psTrace("psLib.imageops", 5, "Kernel range: %d:%d,%d:%d\n", xMin, xMax, yMin, yMax);

    if (totalTime > 0) {
        // Then the total time is simply the final value
        // NB: We assume the counter starts at zero!
        tSum = totalTime;
    }

    // One more pass through to set the kernel
    psKernel *kernel = psKernelAlloc(xMin, xMax, yMin, yMax); // The kernel
    xLast = xShifts->data.S32[0];
    yLast = yShifts->data.S32[0];
    float tLast = 0.0;                  // Last value for t
    for (int i = 0; i < num; i++) {
        int x = xShifts->data.S32[i] - x0; // x position in kernel
        int y = yShifts->data.S32[i] - y0; // y position in kernel
        if (xyRelative) {
            x += xLast;
            y += yLast;
            xLast = x;
            yLast = y;
        }
        float t = tShifts->data.F32[i];
        if (totalTime > 0) {
            t -= tLast;
            tLast = tShifts->data.F32[i];
        }

        kernel->kernel[y][x] += t;
    }

    // Normalise the kernel by the total time (kernel sum should be unity)
    psBinaryOp(kernel->image, kernel->image, "*", psScalarAlloc(1.0 / tSum, PS_TYPE_F32));

    return kernel;
}

bool psKernelTruncate(psKernel *kernel, float frac)
{
    PS_ASSERT_KERNEL_NON_NULL(kernel, false);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(frac, 0.0, false);
    PS_ASSERT_FLOAT_LESS_THAN(frac, 1.0, false);

    if (frac == 0.0) {
        // Nothing to do
        return true;
    }

    int xMin = kernel->xMin, xMax = kernel->xMax, yMin = kernel->yMin, yMax = kernel->yMax; // Bounds
    int maxSize = PS_MAX(PS_MAX(PS_MAX(-xMin, xMax), -yMin), yMax); // Maximum size

    // Determine the threshold
    // Summing absolute values because large negative deviations have power as well
    double sumKernel = 0.0;             // Sum of the kernel
    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            sumKernel += fabsf(kernel->kernel[y][x]);
        }
    }

    float threshold = sumKernel * (1.0 - frac); // Threshold for truncation

    int truncateRadius = maxSize;	// Truncation radius
    bool truncate = false;

    // Find truncation size
    for (int radius = 1; !truncate && (radius < maxSize); radius++) {
        int uMin = PS_MAX(-radius, xMin);
        int uMax = PS_MIN(radius, xMax);
        int vMin = PS_MAX(-radius, yMin);
        int vMax = PS_MIN(radius, yMax);
        int r2 = PS_SQR(radius);
        double sum = 0.0;
        for (int v = vMin; v <= vMax; v++) {
            int v2 = PS_SQR(v);
            for (int u = uMin; u <= uMax; u++) {
                int u2 = PS_SQR(u);
                if (u2 + v2 <= r2) {
                    sum += fabsf(kernel->kernel[v][u]);
                }
            }
        }
	// This is the truncation radius
        if (sum > threshold) {
	    truncate = true;
            truncateRadius = radius;
        }
    }

    // Do nothing if no truncation is possible
    if (!truncate) {
        return true;
    }

    // Truncate the kernel
    {
        int uMin = PS_MAX(-truncateRadius, xMin);
        int uMax = PS_MIN(truncateRadius, xMax);
        int vMin = PS_MAX(-truncateRadius, yMin);
        int vMax = PS_MIN(truncateRadius, yMax);
        int r2 = PS_SQR(truncateRadius);
        for (int v = vMin; v <= vMax; v++) {
            int v2 = PS_SQR(v);
            for (int u = uMin; u <= uMax; u++) {
                int u2 = PS_SQR(u);
                if (u2 + v2 > r2) {
                    kernel->kernel[v][u] = 0.0;
                }
            }
        }
    }
    kernel->xMin = PS_MAX(-truncateRadius, kernel->xMin);
    kernel->xMax = PS_MIN(truncateRadius, kernel->xMax);
    kernel->yMin = PS_MAX(-truncateRadius, kernel->yMin);
    kernel->yMax = PS_MIN(truncateRadius, kernel->yMax);

    return true;
    }



psImage *psImageConvolveDirect(psImage *out, const psImage *in, const psKernel *kernel)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    if (in->type.type != PS_TYPE_S32 && in->type.type != PS_TYPE_F32 && in->type.type != PS_TYPE_F64) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Unallowable operation: psImage %s is not of type S32 or F{32,64}.", "in");
        return(NULL);
    }
    PS_ASSERT_PTR_NON_NULL(kernel, NULL);
    PS_ASSERT_PTR_NON_NULL(kernel->kernel, NULL);

    // Pull out kernel parameters, for convenience
    int xMin = kernel->xMin;
    int xMax = kernel->xMax;
    int yMin = kernel->yMin;
    int yMax = kernel->yMax;
    float **kernelData = kernel->kernel;

    int numRows = in->numRows;          // Number of rows
    int numCols = in->numCols;          // Number of columns

#if 1

    // This is the usual way of doing the convolution, but it wastes time if there's a large number of zero
    // pixels in the kernel.
#define SPATIAL_CONVOLVE_CASE(TYPE) \
    case PS_TYPE_##TYPE: { \
        ps##TYPE **inData = in->data.TYPE; /* Dereference input data */ \
        out = psImageRecycle(out, numCols, numRows, PS_TYPE_##TYPE); \
        for (int row = 0; row < numRows; row++) { \
            ps##TYPE *outRow = out->data.TYPE[row]; \
            int kRowMin = PS_MAX(yMin, row + 1 - numRows); \
            int kRowMax = PS_MIN(yMax, row); \
            for (int col = 0; col < numCols; col++) { \
                int kColMin = PS_MAX(xMin, col + 1 - numCols); \
                int kColMax = PS_MIN(xMax, col); \
                ps##TYPE pixel = 0.0; \
                for (int kRow = kRowMin; kRow <= kRowMax; kRow++) { \
                    for (int kCol = kColMin; kCol <= kColMax; kCol++) { \
                        pixel += kernelData[kRow][kCol] * inData[row - kRow][col - kCol]; \
                    } \
                } \
                outRow[col] = pixel; \
            } \
        } \
    } \
    break;

#else

    // This turns the convolution inside-out, allowing us to skip kernel pixels that have no contribution.
#define SPATIAL_CONVOLVE_CASE(TYPE) \
    case PS_TYPE_##TYPE: { \
        ps##TYPE **inData = in->data.TYPE; /* Dereference input data */ \
        out = psImageRecycle(out, numCols, numRows, PS_TYPE_##TYPE); \
        psImageInit(out, 0.0); \
        for (int ky = yMin; ky <= yMax; ky++) { \
            for (int kx = xMin; kx <= xMax; kx++) { \
                float kValue = kernelData[ky][kx]; /* Kernel value */ \
                if (kValue == 0.0) { \
                    continue; \
                } \
                for (int y = PS_MAX(ky, 0); y < PS_MIN(numRows, numRows + ky); y++) { \
                    for (int x = PS_MAX(kx, 0); x < PS_MIN(numCols, numCols + kx); x++) { \
                        out->data.TYPE[y][x] += kValue * inData[y - ky][x - kx]; \
                    } \
                } \
            } \
        } \
    } \
    break;

#endif

    switch (in->type.type) {
        SPATIAL_CONVOLVE_CASE(S32);
        SPATIAL_CONVOLVE_CASE(F32);
        SPATIAL_CONVOLVE_CASE(F64);
      default:
        psAbort("Should never get here: bad type that was asserted on previously.");
    }

    return out;
}

psImage *psImageConvolveMaskDirect(psImage *out, const psImage *mask, psImageMaskType maskVal,
                                   psImageMaskType setVal, int xMin, int xMax, int yMin, int yMax)
{
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
    if (out == mask && ((maskVal & setVal) || !setVal)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't convolve mask in-place if values to set contains values to convolve.");
        return NULL;
    }

    if (yMin > yMax) {
        psWarning("Specified yMin, %d, was greater than yMax, %d.  Values swapped.",
                 yMin, yMax);

        int temp = yMin;
        yMin = yMax;
        yMax = temp;
    }
    if (xMin > xMax) {
        psWarning("Specified xMin, %d, was greater than xMax, %d.  Values swapped.",
                 xMin, xMax);

        int temp = xMin;
        xMin = xMax;
        xMax = temp;
    }

    int numRows = mask->numRows;        // Number of rows
    int numCols = mask->numCols;        // Number of columns

    if (!out) {
        // Propagate the non-masked values
        out = (psImage*)psBinaryOp(NULL, (const psPtr)mask, "&", psScalarAlloc(~maskVal, PS_TYPE_IMAGE_MASK));
    }

    // Dereference mask images
    psImageMaskType **maskData = mask->data.PS_TYPE_IMAGE_MASK_DATA;
    psImageMaskType **outData = out->data.PS_TYPE_IMAGE_MASK_DATA;

    if (setVal) {
        // Grow any pixels matching maskVal, setting setVal
        for (int row = 0; row < numRows; row++) {
            for (int col = 0; col < numCols; col++) {
                if (maskData[row][col] & maskVal) {
                    for (int kRow = PS_MAX(yMin, -row); kRow <= PS_MIN(yMax, numRows - row - 1); kRow++) {
                        for (int kCol = PS_MAX(xMin, -col); kCol <= PS_MIN(xMax, numCols - col - 1); kCol++) {
                            outData[row + kRow][col + kCol] |= setVal;
                        }
                    }
                }
            }
        }
    } else {
        // Each pixel receives any maskVal bits set within the convolution window
        for (int row = 0; row < numRows; row++) {
            for (int col = 0; col < numCols; col++) {
                psImageMaskType pixel = outData[row][col]; // Pixel value to set
                if (pixel & maskVal) {
                    // Already done this one
                    continue;
                }
                for (int kRow = PS_MAX(yMin, -row); kRow <= PS_MIN(yMax, numRows - row - 1); kRow++) {
                    for (int kCol = PS_MAX(xMin, -col); kCol <= PS_MIN(xMax, numCols - col - 1); kCol++) {
                        pixel |= maskData[row][col] & maskVal;
                    }
                }
                outData[row][col] = pixel;
            }
        }
    }

    return out;
}


psImage *psImageConvolveMaskFFT(psImage *out, const psImage *mask, psImageMaskType maskVal,
                                psImageMaskType setVal, int xMin, int xMax, int yMin, int yMax, float thresh)
{
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(thresh, 0.0, NULL);
    PS_ASSERT_FLOAT_LESS_THAN(thresh, 1.0, NULL);
    if (out == mask && (maskVal & setVal)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't convolve mask in-place if values to set contains values to convolve.");
        return NULL;
    }

    if (yMin > yMax) {
        psWarning("Specified yMin, %d, was greater than yMax, %d.  Values swapped.",
                 yMin, yMax);

        int temp = yMin;
        yMin = yMax;
        yMax = temp;
    }
    if (xMin > xMax) {
        psWarning("Specified xMin, %d, was greater than xMax, %d.  Values swapped.",
                 xMin, xMax);

        int temp = xMin;
        xMin = xMax;
        xMax = temp;
    }

    int numRows = mask->numRows, numCols = mask->numCols; // Size of image

    psImage *onoff = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Pixels on or off
    psImageInit(onoff, 0);
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                onoff->data.F32[y][x] = 1.0;
            }
        }
    }

    psKernel *kernel = psKernelAlloc(xMin, xMax, yMin, yMax);
    psImageInit(kernel->image, 1.0);
    psImage *convolved = psImageConvolveFFT(NULL, onoff, NULL, 0, kernel);
    psFree(onoff);
    psFree(kernel);
    if (!convolved) {
        psError(PS_ERR_UNKNOWN, false, "Unable to convolve mask.");
        return NULL;
    }

    if (!setVal) {
        setVal = maskVal;
    }

    if (!out) {
        out = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    }
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            out->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = (convolved->data.F32[y][x] >= thresh) ?
                (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] | setVal) : mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x];
        }
    }

    psFree(convolved);

    return out;
}


// Generate normalised Gaussian vector
#define IMAGE_SMOOTH_GAUSS(TARGET, SIZE, SIGMA, TYPE) \
    TARGET = psVectorAlloc(2 * SIZE + 1, PS_TYPE_##TYPE); /* Gaussian */ \
    double sum = 0.0; /* Sum of Gaussian, for normalisation */ \
    double factor = -0.5/PS_SQR(SIGMA); /* Multiplier for exponential */ \
    for (int i = -SIZE, j = 0; i <= SIZE; i++, j++) { \
        sum += TARGET->data.TYPE[j] = exp(factor * PS_SQR(i)); \
    } \
    for (int i = 0; i < 2 * SIZE + 1; i++) { \
        TARGET->data.TYPE[i] /= sum; \
    }

psKernel *psImageSmoothKernel(float sigma, float nSigma)
{
    PS_ASSERT_FLOAT_LARGER_THAN(sigma, 0.0, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(nSigma, 0.0, NULL);

    int size = sigma * nSigma + 0.5; // Half-size of kernel
    psKernel *kernel = psKernelAlloc(-size, size, -size, size); // Kernel to return

    psVector *gaussNorm = NULL;
    IMAGE_SMOOTH_GAUSS(gaussNorm, size, sigma, F32);
    psF32 *gauss = &gaussNorm->data.F32[size]; // Convenient kernel-like indexing for Gaussian
    for (int y = -size; y <= size; y++) {
        for (int x = -size; x <= size; x++) {
            kernel->kernel[y][x] = gauss[y] * gauss[x];
        }
    }
    psFree(gaussNorm);

    return kernel;
}


bool psImageSmooth(psImage *image, double  sigma, double  Nsigma)
{
    PS_ASSERT_IMAGE_NON_NULL(image, false);

    if (sigma < 0.25) {
        psTrace("psLib.imageops", 5, "sigma: %f < 0.25 skipping smoothing", sigma);
        return true;
    }

    // relevant terms
    int Nrange = sigma*Nsigma + 0.5;    // Number of pixels either side for convolution kernel
    if (Nrange < 1) {
        // using zero will cause an arithmetic exception in the loop
        psTrace("psLib.imageops", 5, "Nrange %d for smoothing too small skipping smoothing", Nrange);
        return true;
    }
    int Nx = image->numCols;            // Number of columns
    int Ny = image->numRows;            // Number of rows

#define IMAGESMOOTH_CASE(TYPE) \
  case PS_TYPE_##TYPE: { \
        /* generate normalized gaussian */ \
        psVector *gaussnorm = NULL; \
        IMAGE_SMOOTH_GAUSS(gaussnorm, Nrange, sigma, TYPE) \
        ps##TYPE *gauss = &gaussnorm->data.TYPE[Nrange]; \
        \
        /* Smooth in X direction */ \
        { \
            psVector *calculation = psVectorAlloc(Nx, PS_TYPE_##TYPE); \
            for (int j = 0; j < Ny; j++) { \
                ps##TYPE *vi = image->data.TYPE[j]; \
                ps##TYPE *vo = calculation->data.TYPE; \
                int xMax = PS_MIN(Nrange, Nx); \
                /* Smooth first Nrange pixels, with renorm */ \
                for (int i = 0; i < xMax; i++, vi++, vo++) { \
		    int convRange = PS_MIN(Nrange + 1, Nx - i);  \
                    ps##TYPE *vr = vi - i; \
                    ps##TYPE *vg = gauss - i; \
                    double g = 0.0; \
                    double s = 0.0; \
                    for (int n = -i; n < convRange; n++, vr++, vg++) { \
                        s += *vg * *vr; \
                        g += *vg; \
                    } \
                    *vo = s / g; \
                } \
                /* If that's all the pixels we have, then we're done already */ \
                if (Nx > Nrange) { \
                    /* Smooth middle pixels */ \
                    for (int i = Nrange; i < Nx - Nrange; i++, vi++, vo++) { \
                        ps##TYPE *vr = vi - Nrange; \
                        ps##TYPE *vg = gauss - Nrange; \
                        double s = 0; \
                        for (int n = -Nrange; n < Nrange + 1; n++, vr++, vg++) { \
                            s += *vg * *vr; \
                        } \
                        *vo = s; \
                    } \
                    /* Smooth last Nrange pixels, with renorm */ \
		    /* if Nx < 2*Nrange, this pass starts at i == Nrange */ \
		    int xMin = PS_MAX(Nx - Nrange, Nrange); \
                    for (int i = xMin; i < Nx; i++, vi++, vo++) { \
                        ps##TYPE *vr = vi - Nrange; \
                        ps##TYPE *vg = gauss - Nrange; \
                        double g = 0.0; \
                        double s = 0.0; \
                        for (int n = -Nrange; n < Nx - i; n++, vr++, vg++) { \
                            s += *vg * *vr; \
                            g += *vg; \
                        } \
                        *vo = s / g; \
                    } \
                } \
                memcpy(image->data.TYPE[j], calculation->data.TYPE, Nx*sizeof(ps##TYPE)); \
            } \
            psFree(calculation); \
        } \
        \
        /* Smooth in Y direction */ \
        psArray *rows = psArrayAlloc(Nrange); \
        /* Smooth the first Nrange pixels, with renorm */ \
        int yMax = PS_MIN(Nrange, Ny); \
        for (int j = 0; j < yMax; j++) { \
	    int convRange = PS_MIN(Nrange + 1, Ny - j);		       \
            psVector *calculation = psVectorAlloc(Nx, PS_TYPE_##TYPE); \
            /* Zero the output row */ \
            memset(calculation->data.TYPE, 0, Nx*sizeof(ps##TYPE)); \
            double sum = 0.0; \
            for (int n = -j; n < convRange; n++) { sum += gauss[n]; } \
            for (int n = -j; n < convRange; n++) { \
                ps##TYPE *vi = image->data.TYPE[j+n]; \
                ps##TYPE *vo = calculation->data.TYPE; \
                double g = gauss[n] / sum; \
                for (int i = 0; i < Nx; i++, vi++, vo++) { \
                    *vo += *vi * g; \
                } \
            } \
            /* Save output rows on temp array of rows */ \
            rows->data[j] = calculation; \
        } \
        if (Ny < Nrange) { \
            /* Need to save the first bit, then we're done */ \
            for (int j = 0; j < Ny; j++) {  \
                psVector *save = rows->data[j]; \
                memcpy(image->data.TYPE[j], save->data.TYPE, Nx*sizeof(ps##TYPE)); \
            } \
        } else { \
            /* Smooth middle pixels */ \
            psVector *calculation = psVectorAlloc(Nx, PS_TYPE_##TYPE); \
            for (int j = Nrange; j < Ny - Nrange; j++) { \
                memset(calculation->data.TYPE, 0, Nx*sizeof(ps##TYPE)); \
                for (int n = -Nrange; n < Nrange + 1; n++) { \
                    ps##TYPE *vi = image->data.TYPE[j+n]; \
                    ps##TYPE *vo = calculation->data.TYPE; \
                    double g = gauss[n]; \
                    for (int i = 0; i < Nx; i++, vi++, vo++) { \
                        *vo += *vi * g; \
                    } \
                } \
                /* Write the output row */ \
                int Nr = j % Nrange; \
                psVector *save = rows->data[Nr]; \
                memcpy(image->data.TYPE[j-Nrange], save->data.TYPE, Nx*sizeof(ps##TYPE)); \
                /* Juggle the pointers, so that next run we use the one we just wrote */ \
                rows->data[Nr] = calculation; \
                calculation = save; \
            } \
            /* Smooth last Nrange pixels, with renorm */		\
	    /* if Ny < 2*Nrange, this pass starts at j == Nrange */ \
	    int yMin = PS_MAX(Ny - Nrange, Nrange); \
            for (int j = yMin; j < Ny; j++) { \
                /* save the Nrange-offset output row, then zero */ \
                memset(calculation->data.TYPE, 0, Nx*sizeof(ps##TYPE)); \
                double sum = 0.0; \
                for (int n = -Nrange; n < Ny - j; n++) { sum += gauss[n]; } \
                for (int n = -Nrange; n < Ny - j; n++) { \
                    ps##TYPE *vi = image->data.TYPE[j+n]; \
                    ps##TYPE *vo = calculation->data.TYPE; \
                    double g = gauss[n] / sum; \
                    for (int i = 0; i < Nx; i++, vi++, vo++) { \
                        *vo += *vi * g; \
                    } \
                } \
                /* Write the output row */ \
                int Nr = j % Nrange; \
                psVector *save = rows->data[Nr]; \
                memcpy(image->data.TYPE[j-Nrange], save->data.TYPE, Nx*sizeof(ps##TYPE)); \
                /* Juggle the pointers, so that next run we use the one we just wrote */ \
                rows->data[Nr] = calculation; \
                calculation = save; \
            } \
            psFree(calculation); \
            /* Write the remaining rows */ \
            for (int j = Ny; j < Ny + Nrange; j++) {  \
                int Nr = j % Nrange; \
                psVector *save = rows->data[Nr]; \
                memcpy(image->data.TYPE[j-Nrange], save->data.TYPE, Nx*sizeof(ps##TYPE)); \
            } \
        } \
        psFree(rows); \
        psFree(gaussnorm); \
        break; \
    }

    switch (image->type.type) {
        IMAGESMOOTH_CASE(F32);
        IMAGESMOOTH_CASE(F64);
    default: {
            char *typeStr;
            PS_TYPE_NAME(typeStr,image->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
            return false;
        }
    }
    return true;
}

void psImageSmoothCacheDataFree (psImageSmoothCacheData *smdata) {
    psFree (smdata->resultX);
    psFree (smdata->resultY);
    psFree (smdata->kernel);
}

// allocate the psImageSmoothCache data structure, but do not define the kernel
psImageSmoothCacheData *psImageSmoothCacheAlloc (psImage *image, double sigma, double Nsigma) {

    psImageSmoothCacheData *smdata = psAlloc(sizeof(psImageSmoothCacheData));
    psMemSetDeallocator(smdata, (psFreeFunc) psImageSmoothCacheDataFree);

    smdata->kernel = NULL;

    if (!image) {
	// relevant terms
	smdata->Nrange = sigma*Nsigma + 0.5;    // Number of pixels either side for convolution kernel
	smdata->Nx = 0;
	smdata->Ny = 0;
	smdata->resultX = NULL;
	smdata->resultY = NULL;
	return smdata;
    }

    // relevant terms
    smdata->Nrange = sigma*Nsigma + 0.5;    // Number of pixels either side for convolution kernel
    smdata->Nx = image->numCols;            // Number of columns
    smdata->Ny = image->numRows;            // Number of rows

    // XXX drop this : we now require a call to a kernel-creation function (like psImageSmoothCacheKernel_Gauss)
    // IMAGE_SMOOTH_GAUSS(smdata->kernel, smdata->Nrange, sigma, F32);
       
    // use a temp running buffer for X and Y directions.
    smdata->resultX = psAlloc(smdata->Nx * sizeof(psF32));
    memset (smdata->resultX, 0, smdata->Nx*sizeof(psF32));

    smdata->resultY = psAlloc(smdata->Ny * sizeof(psF32));
    memset (smdata->resultY, 0, smdata->Ny*sizeof(psF32));

    return smdata;
}

// generate a Gaussian smoothing kernel for supplied sigma.  sigma here does not need to match
// that used to allocate the structure, but it is recommended
bool psImageSmoothCacheKernel_Gauss (psImageSmoothCacheData *smdata, float sigma) {
    // check for NULL structure elements?
    psFree (smdata->kernel);
    IMAGE_SMOOTH_GAUSS(smdata->kernel, smdata->Nrange, sigma, F32);
    return true;
}

// we can use the same DATA structure on multiple images of the same size
bool psImageSmoothCache_F32(psImage *image, psImageSmoothCacheData *smdata)
{
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_VECTOR_NON_NULL(smdata->kernel, false);
    // assert on data type

    // relevant terms
    int Nrange = smdata->Nrange;    // Number of pixels either side for convolution kernel
    int Nx = smdata->Nx;            // Number of columns
    int Ny = smdata->Ny;            // Number of rows

    psF32 *gauss = &smdata->kernel->data.F32[Nrange];
    psF32 *resultX = smdata->resultX;
    psF32 *resultY = smdata->resultY;
       
    /* Smooth in X direction */
    {
	for (int j = 0; j < Ny; j++) {
	    psF32 *vi = image->data.F32[j];
	    int xMax = PS_MIN(Nrange, Nx);
	    /* Smooth first Nrange pixels, with renorm */
	    for (int i = 0; i < xMax; i++, vi++) {
		int convRange = PS_MIN(Nrange + 1, Nx - i);
		psF32 *vr = vi - i;
		psF32 *vg = gauss - i;
		double g = 0.0;
		double s = 0.0;
		for (int n = -i; n < convRange; n++, vr++, vg++) {
		    s += *vg * *vr;
		    g += *vg;
		}
		resultX[i] = s / g;
	    }
	    /* If that's all the pixels we have, then we're done already */
	    if (Nx > Nrange) {
		/* Smooth middle pixels; if Nx < 2*Nrange, this pass is skipped */
		for (int i = Nrange; i < Nx - Nrange; i++, vi++) {
		    psF32 *vr = vi - Nrange;
		    psF32 *vg = gauss - Nrange;
		    double s = 0;
		    for (int n = -Nrange; n < Nrange + 1; n++, vr++, vg++) {
			s += *vg * *vr;
		    }
		    resultX[i] = s;
		}
		/* Smooth last Nrange pixels, with renorm */
		// if Nx < 2*Nrange, this pass starts at i == Nrange
		int xMin = PS_MAX(Nx - Nrange, Nrange);
		for (int i = xMin; i < Nx; i++, vi++) {
		    psF32 *vr = vi - Nrange;
		    psF32 *vg = gauss - Nrange;
		    double g = 0.0;
		    double s = 0.0;
		    for (int n = -Nrange; n < Nx - i; n++, vr++, vg++) {
			s += *vg * *vr;
			g += *vg;
		    }
		    resultX[i] = s / g;
		}
	    }
	    memcpy(image->data.F32[j], resultX, Nx*sizeof(psF32));
	}
    }
       
    // this section probably hits the cache poorly for large images, but is probably OK for small ones
    /* Smooth in Y direction */
    {
	for (int i = 0; i < Nx; i++) {
	    int yMax = PS_MIN(Nrange, Ny);
	    /* Smooth first Nrange pixels, with renorm */
	    for (int j = 0; j < yMax; j++) {
		int convRange = PS_MIN(Nrange + 1, Ny - j);
		psF32 *vg = gauss - j;
		double g = 0.0;
		double s = 0.0;
		for (int n = -j; n < convRange; n++, vg++) {
		    psF32 vr = image->data.F32[j+n][i];
		    s += *vg * vr;
		    g += *vg;
		}
		resultY[j] = s / g;
	    }
	    /* If that's all the pixels we have, then we're done already */
	    if (Ny > Nrange) {
		/* Smooth middle pixels */
		for (int j = Nrange; j < Ny - Nrange; j++) {
		    psF32 *vg = gauss - Nrange;
		    double s = 0;
		    for (int n = -Nrange; n < Nrange + 1; n++, vg++) {
			psF32 vr = image->data.F32[j+n][i]; 
			s += *vg * vr;
		    }
		    resultY[j] = s;
		}
		/* Smooth last Nrange pixels, with renorm */
		// if Ny < 2*Nrange, this pass starts at j == Nrange
		int yMin = PS_MAX(Ny - Nrange, Nrange);
		for (int j = yMin; j < Ny; j++) {
		    psF32 *vg = gauss - Nrange;
		    double g = 0.0;
		    double s = 0.0;
		    for (int n = -Nrange; n < Ny - j; n++, vg++) {
			psF32 vr = image->data.F32[j+n][i];
			s += *vg * vr;
			g += *vg;
		    }
		    resultY[j] = s / g;
		}
	    }
	    // loop here 
	    for (int j = 0; j < Ny; j++) {
		image->data.F32[j][i] = resultY[j];
	    }
	}
    }
    return true;
}

static bool imageSmoothMaskPixels(psVector *out, const psImage *image, const psImage *mask,
                                  psImageMaskType maskVal, const psVector *x, const psVector *y,
                                  const psVector *gaussNorm, float minGauss, int size, int start, int stop)
{
    const psF32 *gauss = &gaussNorm->data.F32[size]; // Gaussian convolution kernel
    int numCols = image->numCols, numRows = image->numRows; // Size of image
    int xLast = numCols - 1, yLast = numRows - 1; // Last index
    for (int i = start; i < stop; i++) {
        int xPix = x->data.S32[i], yPix = y->data.S32[i]; // Pixel coordinates for smoothing

        int yMin = PS_MAX(yPix - size, 0);
        int yMax = PS_MIN(yPix + size, yLast);
        int xMin = PS_MAX(xPix - size, 0);
        int xMax = PS_MIN(xPix + size, xLast);

        const float *yGauss = &gauss[yMin - yPix];

        double ySumIG = 0.0, ySumG = 0.0;
        for (int v = yMin; v <= yMax; v++, yGauss++) {
            const float *xGauss = &gauss[xMin - xPix];
            double xSumIG = 0.0, xSumG = 0.0;
            const psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[v][xMin];
            const psF32 *imageData = &image->data.F32[v][xMin];
            for (int u = xMin; u <= xMax; u++, xGauss++, imageData++, maskData++) {
                if (*maskData & maskVal) {
                    continue;
                }
                xSumIG += *imageData * *xGauss;
                xSumG += *xGauss;
            }
            if (xSumG > minGauss) {
                ySumIG += xSumIG * *yGauss;
                ySumG += xSumG * *yGauss;
            }
        }

        out->data.F32[i] = ySumG > minGauss ? ySumIG / ySumG : NAN;
    }

    return true;
}

static bool psImageSmoothMaskPixelsThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert(job->args, "programming error: no job arguments");
    psAssert(job->args->n == 11, "programming error: wrong number of job arguments");

    psVector *out = job->args->data[0]; // Output vector
    const psImage *image  = job->args->data[1]; // Input image
    const psImage *mask   = job->args->data[2]; // Input mask
    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[3], PS_TYPE_IMAGE_MASK_DATA);
    const psVector *x = job->args->data[4];
    const psVector *y = job->args->data[5];
    const psVector *gaussNorm = job->args->data[6];
    float minGauss = PS_SCALAR_VALUE(job->args->data[7], F32);
    int size = PS_SCALAR_VALUE(job->args->data[8], S32);
    int start = PS_SCALAR_VALUE(job->args->data[9], S32);
    int stop = PS_SCALAR_VALUE(job->args->data[10], S32);
    return imageSmoothMaskPixels(out, image, mask, maskVal, x, y, gaussNorm,
                                 minGauss, size, start, stop);
}


psVector *psImageSmoothMaskPixels(const psImage *image, const psImage *mask, psImageMaskType maskVal,
                                  const psVector *x, const psVector *y,
                                  float sigma, float numSigma, float minGauss)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
    PS_ASSERT_IMAGES_SIZE_EQUAL(image, mask, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, NULL);

    int size = sigma * numSigma + 0.5;  // Half-size of kernel

    int num = x->n;                     // Number of pixels to smooth
    psVector *out = psVectorAlloc(num, PS_TYPE_F32); // Output results

    // Generate normalized gaussian
    psVector *gaussNorm = NULL;
    IMAGE_SMOOTH_GAUSS(gaussNorm, size, sigma, F32);

    // Columns
    if (threaded) {
        int numThreads = psThreadPoolSize(); // Number of threads
        int delta = (numThreads) ? num / numThreads + 1 : num; // Block of cols to do at once
        for (int start = 0; start < num; start += delta) {
            int stop = PS_MIN(start + delta, num);  // End of block

            psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_SMOOTHMASK_PIXELS");
            psArrayAdd(job->args, 0, out);
            psArrayAdd(job->args, 0, (psImage*)image);
            psArrayAdd(job->args, 0, (psImage*)mask);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal, PS_TYPE_IMAGE_MASK);
            psArrayAdd(job->args, 0, (psVector*)x);
            psArrayAdd(job->args, 0, (psVector*)y);
            psArrayAdd(job->args, 0, gaussNorm);
            PS_ARRAY_ADD_SCALAR(job->args, minGauss, PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, size, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, start, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, stop, PS_TYPE_S32);
            if (!psThreadJobAddPending(job)) {
                psFree(gaussNorm);
                psFree(out);
                return NULL;
            }
        }
        if (!psThreadPoolWait(true, true)) {
            psError(PS_ERR_UNKNOWN, false, "Error waiting for threads.");
            psFree(gaussNorm);
            psFree(out);
            return NULL;
        }
    } else if (!imageSmoothMaskPixels(out, image, mask, maskVal, x, y,
                                      gaussNorm, minGauss, size, 0, num)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to smooth pixels.");
        psFree(gaussNorm);
        psFree(out);
        return NULL;
    }

    psFree(gaussNorm);
    return out;
}


// Smooth an image with masked pixels
// The calculation and calcMask images are *deliberately* backwards (row,col instead of col,row or
// [col][row] instead of [row][col]) to optimise the iteration over rows; such instances are marked "BW"
#define IMAGESMOOTH_MASK_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
    psImage *calculation = psImageAlloc(numRows, numCols, PS_TYPE_##TYPE); /* Calculation image; BW */ \
    psImage *calcMask = psImageAlloc(numRows, numCols, PS_TYPE_IMAGE_MASK); /* Mask for calculation image; BW */ \
    \
    /** Smooth in X direction **/ \
    for (int j = 0; j < numRows; j++) { \
        for (int i = 0; i < numCols; i++) { \
            int xMin = PS_MAX(i - size, 0); \
            int xMax = PS_MIN(i + size, xLast); \
            const psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[j][xMin]; \
            const ps##TYPE *imageData = &image->data.TYPE[j][xMin]; \
            int uMin = - PS_MIN(i, size); /* Minimum kernel index */ \
            const psF32 *gaussData = &gauss[uMin]; \
            double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */ \
            for (int x = xMin; x <= xMax; x++, maskData++, imageData++, gaussData++) { \
                if (*maskData & maskVal) { \
                    continue; \
                } \
                sumIG += *imageData * *gaussData; \
                sumG += *gaussData; \
            } \
            if (sumG > minGauss) { \
                /* BW */ \
                calculation->data.TYPE[i][j] = sumIG / sumG; \
                calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] = 0; \
            } else { \
                /* BW */ \
                calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] = 0xFF; \
            } \
        } \
    } \
    \
    output = psImageRecycle(output, numCols, numRows, PS_TYPE_##TYPE); \
    \
    /** Smooth in Y direction  **/ \
    for (int i = 0; i < numCols; i++) { \
        for (int j = 0; j < numRows; j++) { \
            int yMin = PS_MAX(j - size, 0); \
            int yMax = PS_MIN(j + size, yLast); \
            const psImageMaskType *maskData = &calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][yMin]; /* BW */ \
            const ps##TYPE *imageData = &calculation->data.TYPE[i][yMin]; /* BW */ \
            int vMin = - PS_MIN(j, size); /* Minimum kernel index */ \
            const psF32 *gaussData = &gauss[vMin]; \
            double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */ \
            for (int y = yMin; y <= yMax; y++, maskData++, imageData++, gaussData++) { \
                if (*maskData) { \
                    continue; \
                } \
                sumIG += *imageData * *gaussData; \
                sumG += *gaussData; \
            } \
            output->data.TYPE[j][i] = (sumG > minGauss) ? sumIG / sumG : NAN; \
        } \
    } \
    \
    psFree(calculation); \
    psFree(calcMask); \
    psFree(gaussNorm); \
    \
    return output; \
}

psImage *psImageSmoothMask(psImage *output, const psImage *image, const psImage *mask,
                           psImageMaskType maskVal, float sigma, float numSigma, float minGauss)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
    PS_ASSERT_IMAGES_SIZE_EQUAL(image, mask, NULL);

    int numCols = image->numCols, numRows = image->numRows; // Size of image
    int xLast = numCols - 1, yLast = numRows - 1; // Last index
    int size = sigma * numSigma + 0.5;  // Half-size of kernel

    // Generate normalized gaussian
    psVector *gaussNorm = NULL;
    IMAGE_SMOOTH_GAUSS(gaussNorm, size, sigma, F32);
    const psF32 *gauss = &gaussNorm->data.F32[size]; // Gaussian convolution kernel

    switch (image->type.type) {
      case PS_TYPE_F32: {
          psImage *calculation = psImageAlloc(numRows, numCols, PS_TYPE_F32); /* Calculation image; BW */
          psImage *calcMask = psImageAlloc(numRows, numCols, PS_TYPE_IMAGE_MASK); /* Mask for calculation image; BW */

          /** Smooth in X direction **/
          for (int j = 0; j < numRows; j++) {
              for (int i = 0; i < numCols; i++) {
                  int xMin = PS_MAX(i - size, 0);
                  int xMax = PS_MIN(i + size, xLast);
                  const psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[j][xMin];
                  const psF32 *imageData = &image->data.F32[j][xMin];
                  int uMin = - PS_MIN(i, size); /* Minimum kernel index */
                  const psF32 *gaussData = &gauss[uMin];
                  double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */
                  for (int x = xMin; x <= xMax; x++, maskData++, imageData++, gaussData++) {
                      if (*maskData & maskVal) {
                          continue;
                      }
                      // if ((i == 1000) && (j > 10) && (j < 15)) {
                      //          fprintf (stderr, "%d %d  %d  %f  %f   %f %f\n", i, j, x, *imageData, *gaussData, sumIG, sumG);
                      // }
                      sumIG += *imageData * *gaussData;
                      sumG += *gaussData;
                  }
                  if (sumG > minGauss) {
                      /* BW */
                      calculation->data.F32[i][j] = sumIG / sumG;
                      calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] = 0;
                  } else {
                      /* BW */
                      calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] = 0xFF;
                  }
              }
          }

          output = psImageRecycle(output, numCols, numRows, PS_TYPE_F32);

          /** Smooth in Y direction  **/
          for (int i = 0; i < numCols; i++) {
              for (int j = 0; j < numRows; j++) {
                  int yMin = PS_MAX(j - size, 0);
                  int yMax = PS_MIN(j + size, yLast);
                  const psImageMaskType *maskData = &calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][yMin]; /* BW */
                  const psF32 *imageData = &calculation->data.F32[i][yMin]; /* BW */
                  int vMin = - PS_MIN(j, size); /* Minimum kernel index */
                  const psF32 *gaussData = &gauss[vMin];
                  double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */
                  for (int y = yMin; y <= yMax; y++, maskData++, imageData++, gaussData++) {
                      if (*maskData) {
                          continue;
                      }
                      sumIG += *imageData * *gaussData;
                      sumG += *gaussData;
                  }
                  output->data.F32[j][i] = (sumG > minGauss) ? sumIG / sumG : NAN;
              }
          }

          psFree(calculation);
          psFree(calcMask);
          psFree(gaussNorm);

          return output;
      }
        IMAGESMOOTH_MASK_CASE(F64);
      default:
        psFree(gaussNorm);
        char *typeStr;
        PS_TYPE_NAME(typeStr,image->type.type);
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified psImage type, %s, is not supported."),
                typeStr);
        return false;
    }

    psAbort("Should never reach here.");
    return false;
}

/*********************** smooth with mask, threaded version ***********************/

bool psImageSmoothMask_ScanRows_F32(psImage *calculation, psImage *calcMask, const psImage *image,
                                    const psImage *mask, psImageMaskType maskVal, psVector *gaussNorm,
                                    float minGauss, int size, int rowStart, int rowStop)
{
    const psF32 *gauss = &gaussNorm->data.F32[size]; // Gaussian convolution kernel

    int numCols = image->numCols;
    int xLast = numCols - 1; // Last index

    for (int j = rowStart; j < rowStop; j++) {
        for (int i = 0; i < numCols; i++) {
            int xMin = PS_MAX(i - size, 0);
            int xMax = PS_MIN(i + size, xLast);
            const psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[j][xMin];
            const psF32 *imageData = &image->data.F32[j][xMin];
            int uMin = - PS_MIN(i, size); /* Minimum kernel index */
            const psF32 *gaussData = &gauss[uMin];
            double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */
            for (int x = xMin; x <= xMax; x++, maskData++, imageData++, gaussData++) {
                if (*maskData & maskVal) {
                    continue;
                }
                sumIG += *imageData * *gaussData;
                sumG += *gaussData;
            }
            if (sumG > minGauss) {
                /* BW */
                calculation->data.F32[i][j] = sumIG / sumG;
                calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] = 0;
            } else {
                /* BW */
                calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] = 0xFF;
            }
        }
    }
    return true;
}

bool psImageSmoothMask_ScanCols_F32(psImage *output, psImage *calculation, psImage *calcMask,
                                    psImageMaskType maskVal, psVector *gaussNorm, float minGauss,
                                    int size, int colStart, int colStop)
{
    const psF32 *gauss = &gaussNorm->data.F32[size]; // Gaussian convolution kernel

    int numRows = output->numRows;
    int yLast = numRows - 1; // Last index

    for (int i = colStart; i < colStop; i++) {
        for (int j = 0; j < numRows; j++) {
            int yMin = PS_MAX(j - size, 0);
            int yMax = PS_MIN(j + size, yLast);
            const psImageMaskType *maskData = &calcMask->data.PS_TYPE_IMAGE_MASK_DATA[i][yMin]; /* BW */
            const psF32 *imageData = &calculation->data.F32[i][yMin]; /* BW */
            int vMin = - PS_MIN(j, size); /* Minimum kernel index */
            const psF32 *gaussData = &gauss[vMin];
            double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */
            for (int y = yMin; y <= yMax; y++, maskData++, imageData++, gaussData++) {
                if (*maskData) {
                    continue;
                }
                sumIG += *imageData * *gaussData;
                sumG += *gaussData;
            }
            output->data.F32[j][i] = (sumG > minGauss) ? sumIG / sumG : NAN;
        }
    }
    return true;
}

bool psImageSmoothMask_ScanRows_F32_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert (job->args, "programming error: job args not alloced?");
    psAssert (job->args->n == 10, "programming error: job Nargs mismatch");

    psImage *calculation  = job->args->data[0]; // calculation image
    psImage *calcMask     = job->args->data[1]; // calculation mask
    const psImage *image  = job->args->data[2]; // input image
    const psImage *mask   = job->args->data[3]; // input mask

    psImageMaskType maskVal    = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);
    psVector *gaussNorm   = job->args->data[5]; // gauss kernel
    float minGauss        = PS_SCALAR_VALUE(job->args->data[6],F32);
    int size              = PS_SCALAR_VALUE(job->args->data[7],S32);
    int rowStart          = PS_SCALAR_VALUE(job->args->data[8],S32);
    int rowStop           = PS_SCALAR_VALUE(job->args->data[9],S32);
    return psImageSmoothMask_ScanRows_F32(calculation, calcMask, image, mask, maskVal,
                                          gaussNorm, minGauss, size,
                                          rowStart, rowStop);
}

bool psImageSmoothMask_ScanCols_F32_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert (job->args, "programming error: job args not alloced?");
    psAssert (job->args->n == 9, "programming error: job Nargs mismatch");

    psImage *output       = job->args->data[0]; // input image
    psImage *calculation  = job->args->data[1]; // calculation image
    psImage *calcMask     = job->args->data[2]; // calculation mask
    psImageMaskType maskVal    = PS_SCALAR_VALUE(job->args->data[3],PS_TYPE_IMAGE_MASK_DATA);

    psVector *gaussNorm   = job->args->data[4]; // gauss kernel
    float minGauss        = PS_SCALAR_VALUE(job->args->data[5],F32);
    int size              = PS_SCALAR_VALUE(job->args->data[6],S32);
    int colStart          = PS_SCALAR_VALUE(job->args->data[7],S32);
    int colStop           = PS_SCALAR_VALUE(job->args->data[8],S32);
    return psImageSmoothMask_ScanCols_F32(output, calculation, calcMask, maskVal,
                                          gaussNorm, minGauss, size,
                                          colStart, colStop);
}

psImage *psImageSmoothMask_Threaded(psImage *output, const psImage *image, const psImage *mask,
                                    psImageMaskType maskVal, float sigma, float numSigma, float minGauss)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
    PS_ASSERT_IMAGES_SIZE_EQUAL(image, mask, NULL);

    int numCols = image->numCols, numRows = image->numRows; // Size of image

    int nThreads = psThreadPoolSize();
    int scanCols = nThreads ? (numCols / 4 / nThreads) : numCols;
    int scanRows = nThreads ? (numRows / 4 / nThreads) : numRows;
    int size = sigma * numSigma + 0.5;  // Half-size of kernel

    // Generate normalized gaussian
    psVector *gaussNorm = NULL;
    IMAGE_SMOOTH_GAUSS(gaussNorm, size, sigma, F32);

    switch (image->type.type) {
        // Smooth an image with masked pixels
        // The calculation and calcMask images are *deliberately* backwards (row,col instead of col,row or
        // [col][row] instead of [row][col]) to optimise the iteration over rows; such instances are marked "BW"

      case PS_TYPE_F32: {
          psImage *calculation = psImageAlloc(numRows, numCols, PS_TYPE_F32); /* Calculation image; BW */
          psImage *calcMask = psImageAlloc(numRows, numCols, PS_TYPE_IMAGE_MASK); /* Mask for calculation image; BW */

          if (threaded) {
              /** Smooth in X direction **/
              for (int rowStart = 0; rowStart < numRows; rowStart+=scanRows) {
                  int rowStop = PS_MIN (rowStart + scanRows, numRows);

                  // allocate a job, construct the arguments for this job
                  psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_SMOOTHMASK_SCANROWS");
                  psArrayAdd(job->args, 0, calculation);
                  psArrayAdd(job->args, 0, calcMask);
                  psArrayAdd(job->args, 0, (psImage *) image); // cast away const
                  psArrayAdd(job->args, 0, (psImage *) mask); // cast away const
                  PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
                  psArrayAdd(job->args, 0, gaussNorm);
                  PS_ARRAY_ADD_SCALAR(job->args, minGauss, PS_TYPE_F32);
                  PS_ARRAY_ADD_SCALAR(job->args, size,     PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, rowStart, PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, rowStop,  PS_TYPE_S32);
                  // -> psImageSmoothMask_ScanRows_F32 (calculation, calcMask, image, mask, maskVal, gauss, minGauss, size, rowStart, rowStop);

                  // if threading is not active, we simply run the job and return
                  if (!psThreadJobAddPending(job)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                      psFree(calculation);
                      psFree(calcMask);
                      psFree(gaussNorm);
                      return false;
                  }
              }
              // wait here for the threaded jobs to finish (NOP if threading is not active)
              if (!psThreadPoolWait(true, true)) {
                  psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                  psFree(calculation);
                  psFree(calcMask);
                  psFree(gaussNorm);
                  return false;
              }
          } else if (!psImageSmoothMask_ScanRows_F32(calculation, calcMask, image, mask, maskVal,
                                                     gaussNorm, minGauss, size, 0, numRows)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
              psFree(calculation);
              psFree(calcMask);
              psFree(gaussNorm);
              return false;
          }

          output = psImageRecycle(output, numCols, numRows, PS_TYPE_F32);

          /** Smooth in Y direction  **/
          if (threaded) {
              for (int colStart = 0; colStart < numCols; colStart+=scanCols) {
                  int colStop = PS_MIN (colStart + scanCols, numCols);

                  // allocate a job, construct the arguments for this job
                  psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_SMOOTHMASK_SCANCOLS");
                  psArrayAdd(job->args, 0, output);
                  psArrayAdd(job->args, 0, calculation);
                  psArrayAdd(job->args, 0, calcMask);
                  PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
                  psArrayAdd(job->args, 0, gaussNorm);
                  PS_ARRAY_ADD_SCALAR(job->args, minGauss, PS_TYPE_F32);
                  PS_ARRAY_ADD_SCALAR(job->args, size,     PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, colStart, PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, colStop,  PS_TYPE_S32);
                  // -> psImageSmoothMask_ScanCols_F32 (output, calculation, calcMask, maskVal, gauss, minGauss, size, colStart, colStop);

                  // if threading is not active, we simply run the job and return
                  if (!psThreadJobAddPending(job)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                      psFree(calculation);
                      psFree(calcMask);
                      psFree(gaussNorm);
                      return false;
                  }
              }

              // wait here for the threaded jobs to finish (NOP if threading is not active)
              if (!psThreadPoolWait(true, true)) {
                  psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                  psFree(calculation);
                  psFree(calcMask);
                  psFree(gaussNorm);
                  return false;
              }
          } else if (!psImageSmoothMask_ScanCols_F32(output, calculation, calcMask, maskVal,
                                                     gaussNorm, minGauss, size, 0, numCols)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
              psFree(calculation);
              psFree(calcMask);
              psFree(gaussNorm);
              return false;
          }

          psFree(calculation);
          psFree(calcMask);
          psFree(gaussNorm);

          return output;
      }
      default:
        psFree(gaussNorm);
        char *typeStr;
        PS_TYPE_NAME(typeStr,image->type.type);
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified psImage type, %s, is not supported."),
                typeStr);
        return false;
    }

    psAbort("Should never reach here.");
    return false;
}


/*********************** smooth no-mask ***********************/

bool psImageSmoothNoMask_ScanRows_F32(psImage *calculation, const psImage *image,
				      psVector *gaussNorm, float minGauss, 
				      int size, int rowStart, int rowStop)
{
    const psF32 *gauss = &gaussNorm->data.F32[size]; // Gaussian convolution kernel

    int numCols = image->numCols;
    int xLast = numCols - 1; // Last index

    for (int j = rowStart; j < rowStop; j++) {
        for (int i = 0; i < numCols; i++) {
            int xMin = PS_MAX(i - size, 0);
            int xMax = PS_MIN(i + size, xLast);
            const psF32 *imageData = &image->data.F32[j][xMin];
            int uMin = - PS_MIN(i, size); /* Minimum kernel index */
            const psF32 *gaussData = &gauss[uMin];
            double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */
            for (int x = xMin; x <= xMax; x++, imageData++, gaussData++) {
                sumIG += *imageData * *gaussData;
                sumG += *gaussData;
            }
            if (sumG > minGauss) {
                /* BW */
                calculation->data.F32[i][j] = sumIG / sumG;
            } 
        }
    }
    return true;
}

bool psImageSmoothNoMask_ScanCols_F32(psImage *output, psImage *calculation, 
				      psVector *gaussNorm, float minGauss,
				      int size, int colStart, int colStop)
{
    const psF32 *gauss = &gaussNorm->data.F32[size]; // Gaussian convolution kernel

    int numRows = output->numRows;
    int yLast = numRows - 1; // Last index

    for (int i = colStart; i < colStop; i++) {
        for (int j = 0; j < numRows; j++) {
            int yMin = PS_MAX(j - size, 0);
            int yMax = PS_MIN(j + size, yLast);
            const psF32 *imageData = &calculation->data.F32[i][yMin]; /* BW */
            int vMin = - PS_MIN(j, size); /* Minimum kernel index */
            const psF32 *gaussData = &gauss[vMin];
            double sumIG = 0.0, sumG = 0.0; /* Sums for convolution */
            for (int y = yMin; y <= yMax; y++, imageData++, gaussData++) {
                sumIG += *imageData * *gaussData;
                sumG += *gaussData;
            }
            output->data.F32[j][i] = (sumG > minGauss) ? sumIG / sumG : NAN;
        }
    }
    return true;
}

bool psImageSmoothNoMask_ScanRows_F32_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert (job->args, "programming error: job args not alloced?");
    psAssert (job->args->n == 7, "programming error: job Nargs mismatch");

    psImage *calculation  = job->args->data[0]; // calculation image
    const psImage *image  = job->args->data[1]; // input image

    psVector *gaussNorm   = job->args->data[2]; // gauss kernel
    float minGauss        = PS_SCALAR_VALUE(job->args->data[3],F32);
    int size              = PS_SCALAR_VALUE(job->args->data[4],S32);
    int rowStart          = PS_SCALAR_VALUE(job->args->data[5],S32);
    int rowStop           = PS_SCALAR_VALUE(job->args->data[6],S32);
    return psImageSmoothNoMask_ScanRows_F32(calculation, image,
                                          gaussNorm, minGauss, size,
                                          rowStart, rowStop);
}

bool psImageSmoothNoMask_ScanCols_F32_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert (job->args, "programming error: job args not alloced?");
    psAssert (job->args->n == 7, "programming error: job Nargs mismatch");

    psImage *output       = job->args->data[0]; // input image
    psImage *calculation  = job->args->data[1]; // calculation image

    psVector *gaussNorm   = job->args->data[2]; // gauss kernel
    float minGauss        = PS_SCALAR_VALUE(job->args->data[3],F32);
    int size              = PS_SCALAR_VALUE(job->args->data[4],S32);
    int colStart          = PS_SCALAR_VALUE(job->args->data[5],S32);
    int colStop           = PS_SCALAR_VALUE(job->args->data[6],S32);
    return psImageSmoothNoMask_ScanCols_F32(output, calculation,
                                          gaussNorm, minGauss, size,
                                          colStart, colStop);
}

psImage *psImageSmoothNoMask_Threaded(psImage *output, const psImage *image, float sigma, float numSigma, float minGauss)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);

    int numCols = image->numCols, numRows = image->numRows; // Size of image

    int nThreads = psThreadPoolSize();
    int scanCols = nThreads ? (numCols / 4 / nThreads) : numCols;
    int scanRows = nThreads ? (numRows / 4 / nThreads) : numRows;
    int size = sigma * numSigma + 0.5;  // Half-size of kernel

    // Generate normalized gaussian
    psVector *gaussNorm = NULL;
    IMAGE_SMOOTH_GAUSS(gaussNorm, size, sigma, F32);

    switch (image->type.type) {
        // Smooth an image with masked pixels
        // The calculation and calcMask images are *deliberately* backwards (row,col instead of col,row or
        // [col][row] instead of [row][col]) to optimise the iteration over rows; such instances are marked "BW"

      case PS_TYPE_F32: {
          psImage *calculation = psImageAlloc(numRows, numCols, PS_TYPE_F32); /* Calculation image; BW */

          if (threaded) {
              /** Smooth in X direction **/
              for (int rowStart = 0; rowStart < numRows; rowStart+=scanRows) {
                  int rowStop = PS_MIN (rowStart + scanRows, numRows);

                  // allocate a job, construct the arguments for this job
                  psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_SMOOTH_NOMASK_SCANROWS");
                  psArrayAdd(job->args, 0, calculation);
                  psArrayAdd(job->args, 0, (psImage *) image); // cast away const
                  psArrayAdd(job->args, 0, gaussNorm);
                  PS_ARRAY_ADD_SCALAR(job->args, minGauss, PS_TYPE_F32);
                  PS_ARRAY_ADD_SCALAR(job->args, size,     PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, rowStart, PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, rowStop,  PS_TYPE_S32);
                  // -> psImageSmoothNoMask_ScanRows_F32 (calculation, image, gauss, minGauss, size, rowStart, rowStop);

                  // if threading is not active, we simply run the job and return
                  if (!psThreadJobAddPending(job)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                      psFree(calculation);
                      psFree(gaussNorm);
                      return false;
                  }
              }
              // wait here for the threaded jobs to finish (NOP if threading is not active)
              if (!psThreadPoolWait(true, true)) {
                  psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                  psFree(calculation);
                  psFree(gaussNorm);
                  return false;
              }
          } else if (!psImageSmoothNoMask_ScanRows_F32(calculation, image, gaussNorm, minGauss, size, 0, numRows)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
              psFree(calculation);
              psFree(gaussNorm);
              return false;
          }

          output = psImageRecycle(output, numCols, numRows, PS_TYPE_F32);

          /** Smooth in Y direction  **/
          if (threaded) {
              for (int colStart = 0; colStart < numCols; colStart+=scanCols) {
                  int colStop = PS_MIN (colStart + scanCols, numCols);

                  // allocate a job, construct the arguments for this job
                  psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_SMOOTH_NOMASK_SCANCOLS");
                  psArrayAdd(job->args, 0, output);
                  psArrayAdd(job->args, 0, calculation);
                  psArrayAdd(job->args, 0, gaussNorm);
                  PS_ARRAY_ADD_SCALAR(job->args, minGauss, PS_TYPE_F32);
                  PS_ARRAY_ADD_SCALAR(job->args, size,     PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, colStart, PS_TYPE_S32);
                  PS_ARRAY_ADD_SCALAR(job->args, colStop,  PS_TYPE_S32);
                  // -> psImageSmoothMask_ScanCols_F32 (output, calculation, gauss, minGauss, size, colStart, colStop);

                  // if threading is not active, we simply run the job and return
                  if (!psThreadJobAddPending(job)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                      psFree(calculation);
                      psFree(gaussNorm);
                      return false;
                  }
              }

              // wait here for the threaded jobs to finish (NOP if threading is not active)
              if (!psThreadPoolWait(true, true)) {
                  psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
                  psFree(calculation);
                  psFree(gaussNorm);
                  return false;
              }
          } else if (!psImageSmoothNoMask_ScanCols_F32(output, calculation, gaussNorm, minGauss, size, 0, numCols)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to smooth image");
              psFree(calculation);
              psFree(gaussNorm);
              return false;
          }

          psFree(calculation);
          psFree(gaussNorm);

          return output;
      }
      default:
        psFree(gaussNorm);
        char *typeStr;
        PS_TYPE_NAME(typeStr,image->type.type);
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified psImage type, %s, is not supported."),
                typeStr);
        return false;
    }

    psAbort("Should never reach here.");
    return false;
}

/****************************************************************************************/

bool psImageSmoothMaskF32(psImage *image, psImage *mask, psImageMaskType maskVal,
                          double  sigma, double  Nsigma)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);

    // relevant terms
    int Nrange = sigma*Nsigma + 0.5;    // Number of pixels either side for convolution kernel
    int Nx = image->numCols;            // Number of columns
    int Ny = image->numRows;            // Number of rows

    /* generate normalized gaussian */
    psVector *gaussnorm = NULL;
    IMAGE_SMOOTH_GAUSS(gaussnorm, Nrange, sigma, F32);
    psF32 *gauss = &gaussnorm->data.F32[Nrange];

    /** Smooth in X direction **/
    psVector *calculation = psVectorAlloc(Nx, PS_TYPE_F32);
    for (int j = 0; j < Ny; j++) {
        psImageMaskType  *vm = mask->data.PS_TYPE_IMAGE_MASK_DATA[j];
        psF32 *vi = image->data.F32[j];
        psF32 *vo = calculation->data.F32;
        // loop over all pixels in the row
        for (int i = 0; i < Nx; i++, vi++, vo++, vm++) {
            int offset = PS_MIN (i, Nrange);
            psImageMaskType  *sm = vm - offset;
            psF32 *si = vi - offset;
            psF32 *sg = gauss - offset;
            double g = 0.0;
            double s = 0.0;
            // loop over all valid pixels in the smoothing kernel
            int xMin = PS_MAX (i - Nrange, 0);
            int xMax = PS_MIN (i + Nrange + 1, Nx);
            for (int n = xMin; n < xMax; n++, sm++, si++, sg++) {
                if (*sm & maskVal)
                    continue;
                s += *sg * *si;
                g += *sg;
            }
            *vo = (g > MIN_GAUSS_FRAC) ? s / g : NAN;
        }
        memcpy(image->data.F32[j], calculation->data.F32, Nx*sizeof(psF32));
    }
    psFree(calculation);

    /** Smooth in Y direction  **/
    // allocate and save Nrange extra row vectors for storage. these will be
    // written over the output rows only after we are Nrange rows beyond them
    int Nsave = Nrange + 1;
    psArray *rows = psArrayAlloc(Nsave);
    for (int i = 0; i < Nsave; i++) {
        rows->data[i] = psVectorAlloc(Nx, PS_TYPE_F32);
    }

    psVector *outsum = psVectorAlloc(Nx, PS_TYPE_F32);
    for (int j = 0; j < Ny; j++) {
        psVector *output = rows->data[j % Nsave];
        memset (output->data.F32, 0, Nx*sizeof(psF32));
        memset (outsum->data.F32, 0, Nx*sizeof(psF32));
        int yMin = PS_MAX (j - Nrange, 0);
        int yMax = PS_MIN (j + Nrange + 1, Ny);
        for (int n = yMin; n < yMax; n++) {
            psImageMaskType  *vm = mask->data.PS_TYPE_IMAGE_MASK_DATA[n];
            psF32 *vi = image->data.F32[n];
            psF32 *vo = output->data.F32;
            psF32 *vs = outsum->data.F32;
            double g = gauss[n - j];
            for (int i = 0; i < Nx; i++, vi++, vo++, vm++, vs++) {
                if (*vm & maskVal) continue;
                if (!isfinite(*vi)) continue;
                *vo += *vi * g;
                *vs += g;
            }
        }
        // renormalize the row
        psF32 *vo = output->data.F32;
        psF32 *vs = outsum->data.F32;
        for (int i = 0; i < Nx; i++, vo++, vs++) {
            *vo = (*vs > MIN_GAUSS_FRAC) ? *vo / *vs : NAN;
        }

        // Write the output row
        if (j - Nrange >= 0) {
            int Nout = (j - Nrange) % Nsave;
            psVector *save = rows->data[Nout];
            memcpy(image->data.F32[j-Nrange], save->data.F32, Nx*sizeof(psF32));
        }
    }

    // Write the remaining output rows
    for (int j = PS_MAX(0, Ny - Nrange); j < Ny; j++) {
        psVector *save = rows->data[j % Nsave];
        memcpy(image->data.F32[j], save->data.F32, Nx*sizeof(psF32));
    }
    psFree(rows);
    psFree(outsum);
    psFree(gaussnorm);
    return true;
}


// Convolve mask columns
static bool imageConvolveMaskColumns(psImage *target, // Output, convolved image
                                     const psImage *input, // Input image
                                     int start, int stop, // Range of rows
                                     psImageMaskType maskVal, // Value to mask; NOTE subtle difference!
                                     int xMin, int xMax // Range in x for kernel
                                     )
{
    // Dereference mask images
    psImageMaskType **inputData = input->data.PS_TYPE_IMAGE_MASK_DATA;
    psImageMaskType **targetData = target->data.PS_TYPE_IMAGE_MASK_DATA;

    int numCols = input->numCols;       // Number of columns

    for (int y = start; y < stop; y++) {
        int min = 0, max = 0;           // Minimum and maximum points to mask
        bool masking = false;           // Currently masking?
        for (int x = 0; x < numCols; x++) {
            if (inputData[y][x] & maskVal) {
                if (!masking) {
                    masking = true;
                    min = x + xMin;
                    max = x + xMax;
                } else {
                    max++;
                }
            } else if (masking) {
                // Do the masking
                masking = false;
                min = PS_MAX(0, min);
                max = PS_MIN(numCols - 1, max);
                memset(&targetData[y][min], 0xff, (max - min + 1) * PSELEMTYPE_SIZEOF(PS_TYPE_IMAGE_MASK));
            }
        }
        if (masking) {
            // Mask from the minimum to the end of the row
            min = PS_MAX(0, min);
            memset(&targetData[y][min], 0xff, (numCols - min) * PSELEMTYPE_SIZEOF(PS_TYPE_IMAGE_MASK));
        }
    }
    return true;
}

static bool imageConvolveMaskRows(psImage *target, // Output, convolved image
                                     const psImage *input, // Input image
                                     int start, int stop, // Range of rows
                                     psImageMaskType setVal, // Value to set; NOTE subtle difference!
                                     int yMin, int yMax // Range in y for kernel
                                     )
{
    // Dereference mask images
    psImageMaskType **inputData = input->data.PS_TYPE_IMAGE_MASK_DATA;
    psImageMaskType **targetData = target->data.PS_TYPE_IMAGE_MASK_DATA;

    int numRows = input->numRows;       // Number of rows

    for (int x = start; x < stop; x++) {
        int min = 0, max = 0;           // Minimum and maximum points to mask
        bool masking = false;           // Currently masking?
        for (int y = 0; y < numRows; y++) {
            if (inputData[y][x]) {
                if (!masking) {
                    masking = true;
                    min = y + yMin;
                    max = y + yMax;
                } else {
                    max++;
                }
            } else if (masking) {
                // Do the masking
                masking = false;
                min = PS_MAX(0, min);
                max = PS_MIN(numRows - 1, max);
                for (int i = min; i <= max; i++) {
                    targetData[i][x] |= setVal;
                }
            }
        }
        if (masking) {
            // Mask from the minimum to the end of the column
            for (int i = PS_MAX(0, min); i < numRows; i++) {
                targetData[i][x] |= setVal;
            }
        }
    }
    return true;
}

static bool imageConvolveMaskThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Arguments for job

    psImage *target = args->data[0];       // Output mask image
    const psImage *input = args->data[1];// Input mask image
    int start = PS_SCALAR_VALUE(args->data[2], S32); // Row/col to start at
    int stop = PS_SCALAR_VALUE(args->data[3], S32); // Row/col to stop at
    psImageMaskType maskVal = PS_SCALAR_VALUE(args->data[4], PS_TYPE_IMAGE_MASK_DATA); // Value to mask/set
    int kernelMin = PS_SCALAR_VALUE(args->data[5], S32); // Minimum range for kernel
    int kernelMax = PS_SCALAR_VALUE(args->data[6], S32); // Maximum range for kernel
    bool row = PS_SCALAR_VALUE(args->data[7], U8); // Do row (true) or column (false)?

    return row ? imageConvolveMaskRows(target, input, start, stop, maskVal, kernelMin, kernelMax) :
        imageConvolveMaskColumns(target, input, start, stop, maskVal, kernelMin, kernelMax);
}

psImage *psImageConvolveMask(psImage *out, const psImage *mask, psImageMaskType maskVal,
                             psImageMaskType setVal, int xMin, int xMax, int yMin, int yMax)
{
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
    if (out) {
        PS_ASSERT_IMAGE_NON_NULL(out, NULL);
        PS_ASSERT_IMAGE_TYPE(out, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGES_SIZE_EQUAL(out, mask, NULL);
        if (out == mask && ((maskVal & setVal) || !setVal)) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Can't convolve mask in-place if values to set contains values to convolve.");
            return NULL;
        }
    }

    if (yMin > yMax) {
        psWarning("Specified yMin, %d, was greater than yMax, %d.  Values swapped.",
                 yMin, yMax);

        int temp = yMin;
        yMin = yMax;
        yMax = temp;
    }
    if (xMin > xMax) {
        psWarning("Specified xMin, %d, was greater than xMax, %d.  Values swapped.",
                 xMin, xMax);

        int temp = xMin;
        xMin = xMax;
        xMax = temp;
    }

    int numRows = mask->numRows;        // Number of rows
    int numCols = mask->numCols;        // Number of columns

    // Propagate the non-masked values
    out = (psImage*)psBinaryOp(out, (const psPtr)mask, "&", psScalarAlloc(~setVal, PS_TYPE_IMAGE_MASK));

    if (!setVal) {
        setVal = maskVal;
    }

    psImage *conv = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK); // Temporary convolved image
    psImageInit(conv, 0);

    // Since we're just masking everything inside a square, it's separable

    // Rows
    if (threaded) {
        int numThreads = psThreadPoolSize(); // Number of threads
        int deltaRows = (numThreads) ? numRows / numThreads + 1 : numRows; // Block of rows to do at once
        for (int start = 0; start < numRows; start += deltaRows) {
            int stop = PS_MIN(start + deltaRows, numRows);  // end of row block

            psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_CONVOLVE_MASK");
            psArrayAdd(job->args, 0, conv);
            psArrayAdd(job->args, 0, (psImage*)mask); // Casting away const to put on arguments
            PS_ARRAY_ADD_SCALAR(job->args, start, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, stop, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, xMin, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, xMax, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, 0x00, PS_TYPE_U8); // specify rows
            if (!psThreadJobAddPending(job)) {
                psFree(conv);
                psFree(out);
                return NULL;
            }
        }
        if (!psThreadPoolWait(true, true)) {
            psError(PS_ERR_UNKNOWN, false, "Error waiting for threads.");
            psFree(conv);
            psFree(out);
            return NULL;
        }
    } else if (!imageConvolveMaskColumns(conv, mask, 0, numRows, maskVal, xMin, xMax)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to convolve mask columns.");
        psFree(conv);
        psFree(out);
        return NULL;
    }

    // Columns
    if (threaded) {
        int numThreads = psThreadPoolSize(); // Number of threads
        int deltaCols = (numThreads) ? numCols / numThreads + 1 : numCols; // Block of cols to do at once
        for (int start = 0; start < numCols; start += deltaCols) {
            int stop = PS_MIN(start + deltaCols, numCols);  // end of col block

            psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_CONVOLVE_MASK");
            psArrayAdd(job->args, 0, out);
            psArrayAdd(job->args, 0, conv);
            PS_ARRAY_ADD_SCALAR(job->args, start, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, stop, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, setVal, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, yMin, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, yMax, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, 0xff, PS_TYPE_U8); // specify cols
            if (!psThreadJobAddPending(job)) {
                psFree(conv);
                psFree(out);
                return NULL;
            }
        }
        if (!psThreadPoolWait(true, true)) {
            psError(PS_ERR_UNKNOWN, false, "Error waiting for threads.");
            psFree(conv);
            psFree(out);
            return NULL;
        }
    } else if (!imageConvolveMaskRows(out, conv, 0, numCols, setVal, yMin, yMax)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to convolve mask columns.");
        psFree(conv);
        psFree(out);
        return NULL;
    }


#if 0
    for (int y = 0; y < numRows; y++) {
        int min = 0, max = 0;           // Minimum and maximum points to mask
        bool masking = false;           // Currently masking?
        for (int x = 0; x < numCols; x++) {
            if (maskData[y][x] & maskVal) {
                if (!masking) {
                    masking = true;
                    min = x + xMin;
                    max = x + xMax;
                } else {
                    max++;
                }
            } else if (masking) {
                // Do the masking
                masking = false;
                min = PS_MAX(0, min);
                max = PS_MIN(numCols - 1, max);
                memset(&convData[y][min], 0xff, (max - min + 1) * PSELEMTYPE_SIZEOF(PS_TYPE_IMAGE_MASK));
            }
        }
        if (masking) {
            // Mask from the minimum to the end of the row
            min = PS_MAX(0, min);
            memset(&convData[y][min], 0xff, (numCols - min) * PSELEMTYPE_SIZEOF(PS_TYPE_IMAGE_MASK));
        }
    }
    for (int x = 0; x < numCols; x++) {
        int min = 0, max = 0;           // Minimum and maximum points to mask
        bool masking = false;           // Currently masking?
        for (int y = 0; y < numRows; y++) {
            if (convData[y][x]) {
                if (!masking) {
                    masking = true;
                    min = y + yMin;
                    max = y + yMax;
                } else {
                    max++;
                }
            } else if (masking) {
                // Do the masking
                masking = false;
                min = PS_MAX(0, min);
                max = PS_MIN(numRows - 1, max);
                for (int i = min; i <= max; i++) {
                    outData[i][x] |= setVal;
                }
            }
        }
        if (masking) {
            // Mask from the minimum to the end of the column
            for (int i = PS_MAX(0, min); i < numRows; i++) {
                outData[i][x] |= setVal;
            }
        }
    }
#endif

    psFree(conv);

    return out;
}

// XXX for now, either thread all or none
// have to call this before calling psImageSmoothMask_Threaded
bool psImageConvolveSetThreads(bool set)
{
    pthread_mutex_lock(&threadMutex);
    bool old = threaded;                // Old value
    if (set && !threaded) {
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_CONVOLVE_MASK", 8);
            task->function = &imageConvolveMaskThread;
            psThreadTaskAdd(task);
            psFree(task);
        }
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_SMOOTHMASK_SCANROWS", 10);
            task->function = &psImageSmoothMask_ScanRows_F32_Threaded;
            psThreadTaskAdd(task);
            psFree(task);
        }
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_SMOOTHMASK_SCANCOLS", 9);
            task->function = &psImageSmoothMask_ScanCols_F32_Threaded;
            psThreadTaskAdd(task);
            psFree(task);
        }
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_SMOOTH_NOMASK_SCANROWS", 7);
            task->function = &psImageSmoothNoMask_ScanRows_F32_Threaded;
            psThreadTaskAdd(task);
            psFree(task);
        }
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_SMOOTH_NOMASK_SCANCOLS", 7);
            task->function = &psImageSmoothNoMask_ScanCols_F32_Threaded;
            psThreadTaskAdd(task);
            psFree(task);
        }
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_SMOOTHMASK_PIXELS", 9);
            task->function = &psImageSmoothMaskPixelsThread;
            psThreadTaskAdd(task);
            psFree(task);
        }
    } else if (!set && threaded) {
        psThreadTaskRemove("PSLIB_IMAGE_CONVOLVE_MASK");
        psThreadTaskRemove("PSLIB_IMAGE_SMOOTHMASK_SCANROWS");
        psThreadTaskRemove("PSLIB_IMAGE_SMOOTHMASK_SCANCOLS");
        psThreadTaskRemove("PSLIB_IMAGE_SMOOTH_NOMASK_SCANROWS");
        psThreadTaskRemove("PSLIB_IMAGE_SMOOTH_NOMASK_SCANCOLS");
        psThreadTaskRemove("PSLIB_IMAGE_SMOOTHMASK_PIXELS");
    }
    threaded = set;
    pthread_mutex_unlock(&threadMutex);
    return old;
}

bool psImageConvolveGetThreads(void)
{
    return threaded;
}

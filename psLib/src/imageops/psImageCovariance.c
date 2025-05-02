#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psConstants.h"
#include "psImageStructManip.h"
#include "psImagePixelManip.h"
#include "psImageConvolve.h"
#include "psTrace.h"
#include "psBinaryOp.h"
#include "psScalar.h"
#include "psThread.h"
#include "psImageInterpolate.h"

#include "psImageCovariance.h"

static bool threaded = false;           // Run threaded?

psKernel *psImageCovarianceNone(void)
{
    psKernel *covar = psKernelAlloc(0, 0, 0, 0); // Covariance pseudo-matrix
    covar->kernel[0][0] = 1.0;
    return covar;
}

/** 

 * changes from 28666
 ** add scale to imageCovarianceCalculate (& multiply at return)
 ** add scale to imageCovarianceCalculateThread

 ** accumulate scale (sum kernel^2) in psImageCovarianceCalculate and pass to imageCovarianceCalculate
 ** accumulate scale (sum kernel^2) in psImageCovarianceCalculateFactor and pass to imageCovarianceCalculate

 **/

/// Calculation of covariance matrix element when convolving
static float imageCovarianceCalculate(const psKernel *covar, // Original covariance matrix
                                      const psKernel *kernel, // Convolution kernel
                                      int x, int y,           // Coordinates in output covariance matrix
                                      float scale             // Scale to apply
                                      )
{
    psAssert(covar, "Require covariance matrix");
    psAssert(kernel, "Require kernel");

    // Need to go:
    // (x,y) --kernel--> (u,v) --covar--> (p,q) --kernel--> (0,0)
    // All coordinates (x,y), (u,v), and (p,q) are "absolute", meaning that they are in x,y space, which is
    // the space of the output covariance matrix.
    //
    // So, the ranges in the coordinates are:
    // (x,y): -outSize:outSize
    // (u,v): (x,y)-kernelSize:(x,y)+kernelSize  AND  -kernelSize-inSize:kernelSize+inSize
    // (p,q): -kernelSize:kernelSize  AND  (u,v)-inSize:(u,v)+inSize
    // Here outSize is the size of the output covariance matrix, kernelSize is the size of the convolution
    // kernel, and inSize is the size of the input covariance matrix.
    // Since (u,v) and (p,q) have two ways of specifying the range (one from the target coordinate and one
    // from the source coordinate), we take the smallest possible (because everything else is zero outside).

    // Range for v
    int vMin = PS_MAX(kernel->yMin + covar->yMin, y + kernel->yMin);
    int vMax = PS_MIN(kernel->yMax + covar->yMax, y + kernel->yMax);
    // Range for u
    int uMin = PS_MAX(kernel->xMin + covar->xMin, x + kernel->xMin);
    int uMax = PS_MIN(kernel->xMax + covar->xMax, x + kernel->xMax);

    double sum = 0.0;           // Sum for value of covariance matrix at (x,y)
    for (int v = vMin; v <= vMax; v++) {
        // Range for q
        int qMin = PS_MAX(v + covar->yMin, kernel->yMin);
        int qMax = PS_MIN(v + covar->yMax, kernel->yMax);
        for (int u = uMin; u <= uMax; u++) {
            // Range for p
            int pMin = PS_MAX(u + covar->xMin, kernel->xMin);
            int pMax = PS_MIN(u + covar->xMax, kernel->xMax);

            double xyuvValue = kernel->kernel[v-y][u-x]; // Value for (x,y) --> (u,v)

            double uvpqValue = 0.0; // Value for (u,v) --> (p,q) --> (0,0)
            for (int q = qMin; q <= qMax; q++) {
                for (int p = pMin; p <= pMax; p++) {
                    uvpqValue += (double)covar->kernel[q-v][p-u] * (double)kernel->kernel[q][p];
                }
            }
            sum += xyuvValue * uvpqValue;
        }
    }

    return scale * sum;
}

/// Thread entry point for calculation of covariance matrix element when convolving
static bool imageCovarianceCalculateThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert(job->args, "No job arguments");
    psAssert(job->args->n == 6, "Wrong number of job arguments: %ld", job->args->n);

    psKernel *out = job->args->data[0]; // Output covariance matrix
    const psKernel *covar = job->args->data[1]; // Input covariance matrix
    const psKernel *kernel = job->args->data[2]; // Convolution kernel
    int x = PS_SCALAR_VALUE(job->args->data[3], S32); // x coordinate in output covariance matrix
    int y = PS_SCALAR_VALUE(job->args->data[4], S32); // y coordinate in output covariance matrix
    float scale = PS_SCALAR_VALUE(job->args->data[5], F32); // Scaling to apply

    out->kernel[y][x] = imageCovarianceCalculate(covar, kernel, x, y, scale);

    return true;
}



psKernel *psImageCovarianceCalculate(const psKernel *kernel, const psKernel *covariance)
{
    PS_ASSERT_KERNEL_NON_NULL(kernel, NULL);

    // See http://en.wikipedia.org/wiki/Error_propagation
    //
    // If
    //     f_k = sum_i A_ik x_i
    // is a set of functions, then the covariance matrix for f is given by:
    //     M^f_ij = sum_k sum_l A_ik M^x_kl A_lj
    // where M^x is the covariance matrix for x.
    // Note that the errors in f are correlated (covariance) even if the errors in x are not.

    psKernel *covar;                    // Covariance matrix to use
    if (covariance) {
        covar = psMemIncrRefCounter((psKernel*)covariance); // Casting away const
    } else {
        covar = psImageCovarianceNone();
    }

    // Check for non-finite elements
    double sumKernel = 0.0, sumKernel2 = 0.0; // Sum of the kernel
    for (int y = kernel->yMin; y <= kernel->yMax; y++) {
        for (int x = kernel->xMin; x <= kernel->xMax; x++) {
            if (!isfinite(kernel->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "Non-finite covariance matrix element in kernel at %d,%d", x, y);
                psFree(covar);
                return NULL;
            }
            sumKernel += kernel->kernel[y][x];
            sumKernel2 += PS_SQR(kernel->kernel[y][x]);
        }
    }
    for (int y = covar->yMin; y <= covar->yMax; y++) {
        for (int x = covar->xMin; x <= covar->xMax; x++) {
            if (!isfinite(covar->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "Non-finite covariance matrix element in covariance matrix at %d,%d", x, y);
                psFree(covar);
                return NULL;
            }
        }
    }

    // The above (double) sum for the covariance matrix means that, for each point in the output covariance
    // matrix, we need to work out all combinations of getting to the central point via a kernel, input
    // covariance matrix and another kernel.  This means that the resultant covariance matrix has twice the
    // size of the kernel plus the size of the input covariance matrix.
    int xMin = kernel->xMin - kernel->xMax + covar->xMin, xMax = kernel->xMax - kernel->xMin + covar->xMax;
    int yMin = kernel->yMin - kernel->yMax + covar->yMin, yMax = kernel->yMax - kernel->yMin + covar->yMax;
    psKernel *out = psKernelAlloc(xMin, xMax, yMin, yMax); // Covariance matrix for output
    float scale = 1.0 / sumKernel2;          // Scaling to apply

    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            if (threaded) {
                psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_COVARIANCE_CALCULATE");
                psArrayAdd(job->args, 0, out);
                psArrayAdd(job->args, 0, covar);
                psArrayAdd(job->args, 0, (psKernel*)kernel); // Casting away const
                PS_ARRAY_ADD_SCALAR(job->args, x, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(job->args, y, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(job->args, scale, PS_TYPE_F32);
                if (!psThreadJobAddPending(job)) {
                    psFree(covar);
                    return NULL;
                }
            } else {
                out->kernel[y][x] = imageCovarianceCalculate(covar, kernel, x, y, scale);
            }
        }
    }

    if (threaded && !psThreadPoolWait(true, true)) {
        psError(PS_ERR_UNKNOWN, false, "Error waiting for threads.");
        return false;
    }

    psFree(covar);

    return out;
}

float psImageCovarianceCalculateFactor(const psKernel *kernel, const psKernel *covariance)
{
    psKernel *covar;                    // Covariance matrix to use
    if (covariance) {
        covar = psMemIncrRefCounter((psKernel*)covariance); // Casting away const
    } else {
        covar = psImageCovarianceNone();
    }

    // Check for non-finite elements
    double sumKernel2 = 0.0; // Sum of the squared kernel
    for (int y = kernel->yMin; y <= kernel->yMax; y++) {
        for (int x = kernel->xMin; x <= kernel->xMax; x++) {
            if (!isfinite(kernel->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "Non-finite covariance matrix element in kernel at %d,%d", x, y);
                psFree(covar);
                return NAN;
            }
            sumKernel2 += PS_SQR(kernel->kernel[y][x]);
        }
    }
    for (int y = covar->yMin; y <= covar->yMax; y++) {
        for (int x = covar->xMin; x <= covar->xMax; x++) {
            if (!isfinite(covar->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "Non-finite covariance matrix element in covariance matrix at %d,%d", x, y);
                psFree(covar);
                return NAN;
            }
        }
    }

    float scale = 1.0 / sumKernel2;     // Scale to apply
    float factor = imageCovarianceCalculate(covar, kernel, 0, 0, scale); // Covariance factor
    psFree(covar);
    return factor;
}

// Calculation of covariance matrix element when binning
static float imageCovarianceBin(const psKernel *covar, // Original covariance matrix
                                int bin,               // Binning factor
                                float binVal,          // Convolution kernel value for binning
                                int x, int y           // Coordinates in output covariance matrix
    )
{
    psAssert(covar, "Require covariance matrix");
    psAssert(bin > 0 && binVal > 0, "Require binning: %d %f", bin, binVal);

    int binMin = -(bin - 1) / 2, binMax = bin / 2; // Range of "kernel"

    // Range for v
    int vMin = PS_MAX(binMin + covar->yMin, bin * y + binMin);
    int vMax = PS_MIN(binMax + covar->yMax, bin * y + binMax);
    // Range for u
    int uMin = PS_MAX(binMin + covar->xMin, bin * x + binMin);
    int uMax = PS_MIN(binMax + covar->xMax, bin * x + binMax);

    double sum = 0.0;           // Sum for value of covariance matrix at (x,y)
    for (int v = vMin; v <= vMax; v++) {
        // Range for q
        int qMin = PS_MAX(v + covar->yMin, binMin);
        int qMax = PS_MIN(v + covar->yMax, binMax);
        for (int u = uMin; u <= uMax; u++) {
            // Range for p
            int pMin = PS_MAX(u + covar->xMin, binMin);
            int pMax = PS_MIN(u + covar->xMax, binMax);

            double xyuvValue = binVal; // Value for (x,y) --> (u,v)

            double uvpqValue = 0.0; // Value for (u,v) --> (p,q) --> (0,0)
            for (int q = qMin; q <= qMax; q++) {
                for (int p = pMin; p <= pMax; p++) {
                    uvpqValue += (double)covar->kernel[q-v][p-u] * (double)binVal;
                }
            }
            sum += xyuvValue * uvpqValue;
        }
    }

    return sum;
}

/// Thread entry point for calculation of covariance matrix element when binning
static bool imageCovarianceBinThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    psAssert(job->args, "No job arguments");
    psAssert(job->args->n == 6, "Wrong number of job arguments: %ld", job->args->n);

    psKernel *out = job->args->data[0]; // Output covariance matrix
    const psKernel *covar = job->args->data[1]; // Input covariance matrix
    int bin = PS_SCALAR_VALUE(job->args->data[2], S32); // Binning factor
    float binVal = PS_SCALAR_VALUE(job->args->data[3], F32); // Convolution kernel value for binning
    int x = PS_SCALAR_VALUE(job->args->data[4], S32); // x coordinate in output covariance matrix
    int y = PS_SCALAR_VALUE(job->args->data[5], S32); // y coordinate in output covariance matrix

    out->kernel[y][x] = imageCovarianceBin(covar, bin, binVal, x, y);

    return true;
}


psKernel *psImageCovarianceBin(int bin, const psKernel *covariance, bool average)
{
    psKernel *covar;                    // Covariance matrix to use
    if (covariance) {
        covar = psMemIncrRefCounter((psKernel*)covariance); // Casting away const
    } else {
        covar = psImageCovarianceNone();
    }

    // Check for non-finite elements
    for (int y = covar->yMin; y <= covar->yMax; y++) {
        for (int x = covar->xMin; x <= covar->xMax; x++) {
            if (!isfinite(covar->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "Non-finite covariance matrix element in covariance matrix at %d,%d", x, y);
                psFree(covar);
                return NULL;
            }
        }
    }

    // The following calculation is the same as for psImageCovarianceCalculate(), except that the "convolution
    // kernel" has a constant value, and the output covariance kernel is binned down, so we unbin it in the
    // ranges for the inputs.

    // Values for "convolution kernel"
    int binMin = -(bin - 1) / 2, binMax = bin / 2; // Range of "kernel"
    int binDiff = binMax - binMin;      // Difference between the max and min
    float binVal = average ? 1.0 / PS_SQR(bin) : 1.0; // Value of "kernel" pixels

    // Size of the output covariance matrix
    int xMin = (covar->xMin - binDiff) / bin + 0.5, xMax = (covar->xMax + binDiff) / bin + 0.5;
    int yMin = (covar->yMin - binDiff) / bin + 0.5, yMax = (covar->yMax + binDiff) / bin + 0.5;
    psKernel *out = psKernelAlloc(xMin, xMax, yMin, yMax); // Covariance matrix for output

    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            if (threaded) {
                psThreadJob *job = psThreadJobAlloc("PSLIB_IMAGE_COVARIANCE_BIN");
                psArrayAdd(job->args, 0, out);
                psArrayAdd(job->args, 0, covar);
                PS_ARRAY_ADD_SCALAR(job->args, bin, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(job->args, binVal, PS_TYPE_F32);
                PS_ARRAY_ADD_SCALAR(job->args, x, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(job->args, y, PS_TYPE_S32);
                if (!psThreadJobAddPending(job)) {
                    psFree(covar);
                    return NULL;
                }
            } else {
                out->kernel[y][x] = imageCovarianceBin(covar, bin, binVal, x, y);
            }
        }
    }
    if (threaded && !psThreadPoolWait(true, true)) {
        psError(PS_ERR_UNKNOWN, false, "Error waiting for threads.");
        return false;
    }
    psFree(covar);

    return out;
}

float psImageCovarianceFactor(const psKernel *covariance)
{
    return covariance ? covariance->kernel[0][0] : 1.0;
}

float psImageCovarianceFactorForAperture(const psKernel *covar, float radius)
{
    if (!covar) return 1.0;

    float Sum = 0.0;

    for (int y = covar->yMin; y <= covar->yMax; y++) {
        if (y < -radius) continue;
        if (y > +radius) continue;
        for (int x = covar->xMin; x <= covar->xMax; x++) {
            if (x < -radius) continue;
            if (x > +radius) continue;

            if (hypot(x, y) > radius) continue;

            psAssert (isfinite(covar->kernel[y][x]), "invalid NAN in covariance matrix");
            Sum += covar->kernel[y][x];
        }
    }

    return Sum;
}

float psImageCovarianceSum(const psKernel *covar)
{
    PS_ASSERT_KERNEL_NON_NULL(covar, NAN);

    int xMin = covar->xMin, xMax = covar->xMax, yMin = covar->yMin, yMax = covar->yMax; // Range for covariance
    double sum = 0.0;                                                                   // Sum of covariance
    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            sum += covar->kernel[y][x];
        }
    }

    return sum;
}


psKernel *psImageCovarianceAverage(const psArray *array)
{
    PS_ASSERT_ARRAY_NON_NULL(array, NULL);
    PS_ASSERT_ARRAY_NON_EMPTY(array, NULL);

    psVector *weights = psVectorAlloc(array->n, PS_TYPE_F32); // Weights to apply
    psVectorInit(weights, 1.0);
    psKernel *out = psImageCovarianceAverageWeighted(array, weights);
    psFree(weights);
    return out;
}

psKernel *psImageCovarianceAverageWeighted(const psArray *array, const psVector *weights)
{
    PS_ASSERT_ARRAY_NON_NULL(array, NULL);
    PS_ASSERT_ARRAY_NON_EMPTY(array, NULL);
    if (!weights) {
        return psImageCovarianceAverage(array);
    }
    PS_ASSERT_VECTOR_TYPE(weights, PS_TYPE_F32, NULL);

    int xMin = INT_MAX, xMax = INT_MIN, yMin = INT_MAX, yMax = INT_MIN; // Range for covariance
    double sumWeights = 0.0;            // Sum of weights
    for (int i = 0; i < array->n; i++) {
        psKernel *covar = array->data[i]; // Covariance matrix
        if (!covar) {
            continue;
        }
        xMin = PS_MIN(xMin, covar->xMin);
        xMax = PS_MAX(xMax, covar->xMax);
        yMin = PS_MIN(yMin, covar->yMin);
        yMax = PS_MAX(yMax, covar->yMax);
        sumWeights += weights->data.F32[i];
    }
    if (sumWeights == 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "No covariance matrices supplied for summation");
        return NULL;
    }

    psKernel *sum = psKernelAlloc(xMin, xMax, yMin, yMax); // Summed covariance
    psImageInit(sum->image, 0.0);
    for (int i = 0; i < array->n; i++) {
        psKernel *covar = array->data[i]; // Covariance matrix
        if (!covar) {
            continue;
        }
        for (int y = covar->yMin; y <= covar->yMax; y++) {
            for (int x = covar->xMin; x <= covar->xMax; x++) {
                if (!isfinite(covar->kernel[y][x])) {
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                            "Non-finite covariance matrix element at %d,%d for input %d",
                            x, y, i);
                    psFree(sum);
                    return NULL;
                }
                sum->kernel[y][x] += weights->data.F32[i] * covar->kernel[y][x];
            }
        }
    }
    psBinaryOp(sum->image, sum->image, "/", psScalarAlloc((float)sumWeights, PS_TYPE_F32));

    return sum;
}


psKernel *psImageCovarianceTruncate(const psKernel *covar, float frac)
{
    PS_ASSERT_KERNEL_NON_NULL(covar, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(frac, 0, 1, NULL);

    int xMin = covar->xMin, xMax = covar->xMax, yMin = covar->yMin, yMax = covar->yMax; // Range
    int maxRadius = PS_MAX(PS_MAX(PS_MAX(xMax, yMax), -xMin), -yMin); // Maximum radius of covariance matrix

    double sum = 0.0;                   // Sum of covariance
    psVector *radiusSum = psVectorAlloc(maxRadius + 1, PS_TYPE_F64); // Totals within (square) radius
    psVectorInit(radiusSum, 0.0);
    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            int radius = PS_MAX(abs(x), abs(y)); // Squarish radius
            psAssert(radius <= maxRadius, "Radius doesn't fit");
            if (!isfinite(covar->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Non-finite covariance matrix element at %d,%d",
                        x, y);
                return NULL;
            }
            radiusSum->data.F64[radius] += fabsf(covar->kernel[y][x]);
            sum += fabsf(covar->kernel[y][x]);
        }
    }

    // Determine radius for truncation
    double threshold = (1.0 - frac) * sum; // Threshold for truncation
    double enclosed = 0.0;              // Enclosed value
    int radius;                         // Radius at which to truncate
    for (radius = 0; radius <= maxRadius && enclosed < threshold; radius++) {
        enclosed += radiusSum->data.F64[radius];
    }
    psFree(radiusSum);

    if (radius >= maxRadius) {
        radius = maxRadius;
    }

    // Generate truncated version
    int xMinNew = PS_MAX(xMin, -radius), xMaxNew = PS_MIN(xMax, radius); // New range in x
    int yMinNew = PS_MAX(yMin, -radius), yMaxNew = PS_MIN(yMax, radius); // New range in y
    psKernel *trunc = psKernelAlloc(xMinNew, xMaxNew, yMinNew, yMaxNew); // Truncated covariance matrix
    int numBytes = (xMaxNew - xMinNew + 1) * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes to copy
    for (int y = yMinNew; y <= yMaxNew; y++) {
        memcpy(&trunc->kernel[y][xMinNew], &covar->kernel[y][xMinNew], numBytes);
    }

    return trunc;
}


bool psImageCovarianceTransfer(psImage *variance, psKernel *covar)
{
    PS_ASSERT_IMAGE_NON_NULL(variance, NULL);
    if (!covar) {
        // Transferred all we could!
        return true;
    }
    PS_ASSERT_KERNEL_NON_NULL(covar, NULL);

    float factor = psImageCovarianceFactor(covar); // Factor to transfer
    if (factor == 1.0) {
        // No work required
        return true;
    }

    psBinaryOp(variance, variance, "*", psScalarAlloc(factor, PS_TYPE_F32));
    psBinaryOp(covar->image, covar->image, "/", psScalarAlloc(factor, PS_TYPE_F32));

    return true;
}


psKernel *psImageCovarianceScale(const psKernel *in, float scale)
{
    // Trivial cases
    if (!in) {
        psKernel *out = psKernelAlloc(0, 0, 0, 0); // Output covariance
        out->kernel[0][0] = 1.0;
        return out;
    }
    PS_ASSERT_KERNEL_NON_NULL(in, NULL);
    if (scale == 1.0) {
        psImage *copy = psImageCopy(NULL, in->image, PS_TYPE_F32); // Copy of input covariance
        psKernel *out = psKernelAllocFromImage(copy, -in->xMin, -in->yMin); // Output covariance
        psFree(copy);
        return out;
    }

    int xMinIn = in->xMin, xMaxIn = in->xMax, yMinIn = in->yMin, yMaxIn = in->yMax; // Input size
    int xMinOut = (float)xMinIn / scale - 0.5, xMaxOut = (float)xMaxIn / scale + 0.5;     // Output size in x
    int yMinOut = (float)yMinIn / scale - 0.5, yMaxOut = (float)yMaxIn / scale + 0.5;     // Output size in y

    // Over-fill the covariance matrix so we're not troubled by edge effects
    psKernel *overfill = psKernelAlloc(xMinIn - 1, xMaxIn + 1, yMinIn - 1, yMaxIn + 1); // Overfilled covar
    psImageInit(overfill->image, 0.0);
    int numOverlay = (xMaxIn - xMinIn + 1) * (yMaxIn - yMinIn + 1); // Number of pixels to overlay
    if (psImageOverlaySection(overfill->image, in->image, 1, 1, "=") != numOverlay) {
        psError(psErrorCodeLast(), false, "Unable to overfill covariance matrix.");
        psFree(overfill);
        return NULL;
    }

    psImageInterpolation *interp = psImageInterpolationAlloc(PS_INTERPOLATE_BILINEAR, overfill->image,
                                                             NULL, NULL, 0, NAN, NAN, 0xFF, 0xFF,
                                                             0.0, 0); // Interpolation
    psFree(overfill);

    // In transforming the positions, we get +0.5 to account for the centre of the pixels being at 0.5
    // and +1 to account for the overfill.

    psKernel *out = psKernelAlloc(xMinOut, xMaxOut, yMinOut, yMaxOut); // Output covariance
    double outSum = 0.0;                                               // Sum of covariance
    for (int y = yMinOut; y <= yMaxOut; y++) {
        float yIn = y * scale + 0.5 - yMinIn + 1; // Position on input image (not the kernel)
        for (int x = xMinOut; x <= xMaxOut; x++) {
            float xIn = x * scale + 0.5 - xMinIn + 1; // Position on input (not the kernel)

            double value;                                     // Value on output
            if (!psImageInterpolate(&value, NULL, NULL, xIn, yIn, interp)) {
                psError(psErrorCodeLast(), false, "Unable to interpolate kernel.");
                return false;
            }
            outSum += out->kernel[y][x] = value;
        }
    }
    psFree(interp);

    // Problem: the interpolation has introduced power into the covariance matrix that shouldn't be there.  We
    // can't scale the sum of the matrix because that would throw off the central value and affect the noise
    // calculation for this image.  But we can't scale just by the central value because that would throw off
    // the noise calculation for convolutions of this image.  We choose to scale by the sum of the non-central
    // elements.  This is almost having the best of both worlds.

    double inSum = 0.0;                 // Sum of covariance
    for (int y = yMinIn; y <= yMaxIn; y++) {
        for (int x = xMinIn; x <= xMaxIn; x++) {
            inSum += in->kernel[y][x];
        }
    }

    float norm = (inSum - in->kernel[0][0]) / (outSum - out->kernel[0][0]) / PS_SQR(scale); // Renormalisation
    for (int y = yMinOut; y <= yMaxOut; y++) {
        for (int x = xMinOut; x <= xMaxOut; x++) {
            if (x != 0 && y != 0) {
                out->kernel[y][x] *= norm;
            }
        }
    }

    return out;
}


bool psImageCovarianceSetThreads(bool set)
{
    bool old = threaded;                // Old value
    if (set && !threaded) {
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_COVARIANCE_CALCULATE", 6);
            task->function = &imageCovarianceCalculateThread;
            psThreadTaskAdd(task);
            psFree(task);
        }
        {
            psThreadTask *task = psThreadTaskAlloc("PSLIB_IMAGE_COVARIANCE_BIN", 6);
            task->function = &imageCovarianceBinThread;
            psThreadTaskAdd(task);
            psFree(task);
        }
    } else if (!set && threaded) {
        psThreadTaskRemove("PSLIB_IMAGE_COVARIANCE_CALCULATE");
        psThreadTaskRemove("PSLIB_IMAGE_COVARIANCE_BIN");
    }
    threaded = set;
    return old;
}

bool psImageCovarianceGetThreads(void)
{
    return threaded;
}

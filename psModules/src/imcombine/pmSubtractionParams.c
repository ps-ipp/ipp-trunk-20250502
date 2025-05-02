#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionParams.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// XXX this was moved to pmSubtraction.c in r15443 -- delete 
// Convolve the reference stamp by the kernel
static psKernel *convolveStamp(const pmSubtractionStamp *stamp, // Stamp to be convolved
                               const psKernel *kernel, // Kernel by which to convolve
                               int footprint // Size of area to be convolved
    )
{
    psKernel *convolution = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Result
    psKernel *reference = stamp->reference; // Reference stamp, to be convolved

    // Range of kernel
    int uMin = kernel->xMin;
    int uMax = kernel->xMax;
    int vMin = kernel->yMin;
    int vMax = kernel->yMax;

    for (int y = -footprint; y <= footprint; y++) {
        psF32 *conv = &convolution->kernel[y][-footprint]; // Dereference convolution
        for (int x = -footprint; x <= footprint; x++, conv++) {
            *conv = 0.0;

            int xStart = x + uMin;      // Start index for convolution
            for (int v = vMin; v <= vMax; v++) {
                psF32 *ref = &reference->kernel[y + v][xStart]; // Dereference reference image
                psF32 *krnl = &kernel->kernel[v][uMin]; // Dereference kernel in x
                for (int u = uMin; u <= uMax; u++, ref++, krnl++) {
                    *conv += *ref * *krnl;
                }
            }
        }
    }

    return convolution;
}
#endif

/// Select the appropriate convolution, given the kernel basis function and subtraction mode
static inline psKernel *selectConvolution(const pmSubtractionStamp *stamp, // Stamp
                                          int kernelIndex, // Index for kernel component
                                          pmSubtractionMode mode // Mode of subtraction
    )
{
    switch (mode) {
      case PM_SUBTRACTION_MODE_1:
        return stamp->convolutions1->data[kernelIndex];
      case PM_SUBTRACTION_MODE_2:
        return stamp->convolutions2->data[kernelIndex];
      default:
        psAbort("Unsupported subtraction mode: %x", mode);
    }
    return NULL;                        // Unreached
}

// Accumulate cross-term sums for a stamp
static void accumulateCross(double *sumI, // Sum of I(x)/sigma(x)^2
                            double *sumII, // Sum of I(x)^2/sigma(x)^2
                            double *sumIC, // Sum of I(x)conv(x)/sigma(x)^2
                            const pmSubtractionStamp *stamp, // Stamp
                            const psKernel *target, // Target stamp
                            int kernelIndex, // Index for kernel component
                            int footprint, // Size of region of interest
                            pmSubtractionMode mode // Mode of subtraction
    )
{
    psKernel *weight = stamp->weight;   // Weight image
    psKernel *convolution = selectConvolution(stamp, kernelIndex, mode); // Convolution of interest

    for (int y = -footprint; y <= footprint; y++) {
        psF32 *in = &target->kernel[y][-footprint]; // Dereference input
        psF32 *wt = &weight->kernel[y][-footprint]; // Dereference weight
        psF32 *conv = &convolution->kernel[y][-footprint]; // Dereference convolution
        for (int x = -footprint; x <= footprint; x++, in++, wt++, conv++) {
            double temp = *in * *wt; // Temporary product
            *sumI += temp;
            *sumII += *in * temp;
            *sumIC += *conv * temp;
        }
    }
    return;
}

// Accumulate convolution sums for a stamp
static void accumulateConvolutions(double *sumC, // Sum of conv(x)/sigma(x)^2
                                   double *sumCC, // Sum of conv(x)^2/sigma(x)^2
                                   const pmSubtractionStamp *stamp, // Stamp with input and weight
                                   int kernelIndex, // Index for kernel component
                                   int footprint, // Size of region of interest
                                   pmSubtractionMode mode // Mode of subtraction
    )
{
    psKernel *weight = stamp->weight;   // Weight image
    psKernel *convolution = selectConvolution(stamp, kernelIndex, mode); // Convolution of interest

    for (int y = -footprint; y <= footprint; y++) {
        psF32 *wt = &weight->kernel[y][-footprint]; // Dereference weight
        psF32 *conv = &convolution->kernel[y][-footprint]; // Dereference convolution
        for (int x = -footprint; x <= footprint; x++, wt++, conv++) {
            double convNoise = *conv * *wt; // Temporary product
            *sumC += convNoise;
            *sumCC += *conv * convNoise;
        }
    }
    return;
}

static double accumulateChi2(const psKernel *target, // Target stamp
                             pmSubtractionStamp *stamp, // Stamp with weight
                             int kernelIndex, // Index for kernel component
                             double coeff, // Coefficient of convolution
                             double bg,  // Background term
                             int footprint, // Size of region of interest
                             pmSubtractionMode mode // Mode of subtraction
    )
{
    double chi2 = 0.0;
    psKernel *weight = stamp->weight;   // Weight image
    psKernel *convolution = selectConvolution(stamp, kernelIndex, mode); // Convolution of interest

    for (int y = -footprint; y <= footprint; y++) {
        psF32 *in = &target->kernel[y][-footprint]; // Dereference input
        psF32 *wt = &weight->kernel[y][-footprint]; // Dereference weight
        psF32 *conv = &convolution->kernel[y][-footprint]; // Dereference convolution
        for (int x = -footprint; x <= footprint; x++, in++, wt++, conv++) {
            chi2 += PS_SQR(*in - bg - coeff * *conv) * *wt;
        }
    }

    return chi2;
}

// Return the initial value of chi^2
static double initialChi2(const psKernel *target, // Target stamp
                          const pmSubtractionStamp *stamp, // Stamp
                          int footprint, // Size of convolution
                          pmSubtractionMode mode // Mode of subtraction
    )
{
    psKernel *weight = stamp->weight;   // Weight image
    psKernel *source;                   // Source stamp
    switch (mode) {
      case PM_SUBTRACTION_MODE_1:
        source = stamp->image1;
        break;
      case PM_SUBTRACTION_MODE_2:
        source = stamp->image2;
        break;
      default:
        psAbort("Unsupported subtraction mode: %x", mode);
    }

    double chi2 = 0.0;                  // Chi^2
    for (int y = -footprint; y <= footprint; y++) {
        psF32 *in = &target->kernel[y][-footprint]; // Dereference input
        psF32 *wt = &weight->kernel[y][-footprint]; // Dereference weight
        psF32 *ref = &source->kernel[y][-footprint]; // Derference reference
        for (int x = -footprint; x <= footprint; x++, in++, wt++, ref++) {
            float diff = *in - *ref;    // Temporary value
            chi2 += PS_SQR(diff) * *wt;
        }
    }

    return chi2;
}

// Subtract a convolution from the input
static void subtractConvolution(psKernel *target, // Target stamp
                                const pmSubtractionStamp *stamp, // Stamp
                                int kernelIndex, // Index for kernel component
                                float coeff, // Coefficient of subtraction
                                float bg, // Background term
                                int footprint, // Size of region of interest
                                pmSubtractionMode mode // Mode of subtraction
    )
{
    psKernel *convolution = selectConvolution(stamp, kernelIndex, mode); // Convolution of interest
    for (int y = -footprint; y <= footprint; y++) {
        psF32 *in = &target->kernel[y][-footprint]; // Dereference input
        psF32 *conv = &convolution->kernel[y][-footprint]; // Dereference convolution
        for (int x = -footprint; x <= footprint; x++, in++, conv++) {
            *in -= *conv * coeff + bg;
        }
    }

    return;
}


pmSubtractionKernels *pmSubtractionKernelsOptimumISIS(pmSubtractionKernelsType type, int size, int inner,
                                                      int spatialOrder, const psVector *fwhms, int maxOrder,
                                                      const pmSubtractionStampList *stamps, int footprint,
                                                      float tolerance, float penalty, psRegion bounds,
                                                      pmSubtractionMode mode)
{
    if (type != PM_SUBTRACTION_KERNEL_ISIS && type != PM_SUBTRACTION_KERNEL_GUNK) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Invalid kernel type: %x\n", type);
        return NULL;
    }
    PS_ASSERT_INT_NONNEGATIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(inner, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);
    PS_ASSERT_VECTOR_NON_NULL(fwhms, NULL);
    PS_ASSERT_VECTOR_TYPE(fwhms, PS_TYPE_F32, NULL);
    PS_ASSERT_INT_NONNEGATIVE(maxOrder, NULL);
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, NULL);
    PS_ASSERT_INT_NONNEGATIVE(footprint, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(tolerance, 0.0, NULL);

    // Generate the kernels to test
    int numGaussians = fwhms->n;       // Number of Gaussians
    int numKernels = numGaussians * (maxOrder + 1) * (maxOrder + 2) / 2; // Number of kernel components
    psString params = NULL;             // Parameter, for description
    for (int i = 0; i < numGaussians; i++) {
        psStringAppend(&params, "%.2f,", fwhms->data.F32[i]);
    }
    params[strlen(params) - 1] = '\0';

    psVector *orders = psVectorAlloc(numGaussians, PS_TYPE_S32); // Polynomial orders
    psVectorInit(orders, maxOrder);
    pmSubtractionKernels *kernels = p_pmSubtractionKernelsRawISIS(size, spatialOrder, fwhms, orders,
                                                                  penalty, bounds, mode); // Kernels
    psFree(orders);
    psFree(kernels->description);
    kernels->description = NULL;
    psStringAppend(&kernels->description, "OptISIS(%d,(%s),%d)", size, params, spatialOrder);
    psFree(params);

    // Need to save the stamp inputs --- we're changing the values!
    int numStamps = stamps->num;        // Number of stamps
    psArray *targets = psArrayAlloc(numStamps); // Deep copies of the targets
    psVector *badStamps = psVectorAlloc(numStamps, PS_TYPE_U8); // Mark the bad stamps
    psVectorInit(badStamps, 0);
    for (int i = 0; i < numStamps; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (stamp->status != PM_SUBTRACTION_STAMP_CALCULATE && stamp->status != PM_SUBTRACTION_STAMP_USED) {
            badStamps->data.U8[i] = 0xff;
            continue;
        }
        psKernel *target;               // Target image of interest
        switch (mode) {
          case PM_SUBTRACTION_MODE_1:
            target = stamp->image2;
            break;
          case PM_SUBTRACTION_MODE_2:
            target = stamp->image1;
            break;
          default:
            psAbort("Unsupported subtraction mode: %x", mode);
        }
        psImage *copy = psImageCopy(NULL, target->image, PS_TYPE_F32); // Copy of the image
        targets->data[i] = psKernelAllocFromImage(copy, size + footprint, size + footprint);
        psFree(copy);                   // Drop reference
    }

    // Generate the convolutions, accumulate sums, and measure initial chi^2
    double sum1 = 0.0;                  // sum of 1/sigma(x,y)^2
    psVector *sumC = psVectorAlloc(numKernels, PS_TYPE_F64); // sum of R(x)*k(u)/sigma(x)^2
    psVector *sumCC = psVectorAlloc(numKernels, PS_TYPE_F64); // sum of [R(x)*k(u)]^2/sigma(x)^2
    psVectorInit(sumC, 0.0);
    psVectorInit(sumCC, 0.0);
    double lastChi2 = 0.0;              // Chi^2 from last iteration
    int numPixels = 0;                  // Number of pixels contributing to chi^2
    for (int i = 0; i < numStamps; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (badStamps->data.U8[i]) {
            continue;
        }
        if (!pmSubtractionConvolveStamp(stamp, kernels, footprint)) {
            psError(psErrorCodeLast(), false, "Unable to convolve stamp %d.", i);
            psFree(targets);
            psFree(kernels);
            psFree(badStamps);
            return NULL;
        }

        // This sum is invariant to the kernel
        psKernel *weight = stamp->weight; // Weight image
        for (int v = -footprint; v <= footprint; v++) {
            psF32 *wt = &weight->kernel[v][-footprint]; // Dereference weight
            for (int u = -footprint; u <= footprint; u++, wt++) {
                sum1 += 1.0 * *wt;
            }
        }
        if (!isfinite(sum1)) {
            psError(PM_ERR_DATA, true,
                    "Sum of 1/sigma^2 is non-finite for stamp %d (%d,%d)\n",
                    i, (int)stamp->x, (int)stamp->y);
            psFree(targets);
            psFree(kernels);
            psFree(badStamps);
            return NULL;
        }

        for (int j = 0; j < numKernels; j++) {
            accumulateConvolutions(&sumC->data.F64[j], &sumCC->data.F64[j], stamp, j, footprint, mode);
        }

        lastChi2 += initialChi2(targets->data[i], stamp, footprint, mode);
        numPixels += PS_SQR(2 * footprint + 1);
    }
    lastChi2 /= numPixels;

    // Rank the kernel components
    psVector *ranking = psVectorAlloc(numKernels, PS_TYPE_S32); // Ranking of the kernel components
    psVectorInit(ranking, -1);
    int cutIndex = -1;                  // Index at which to cut off kernels
    for (int iter = 0; iter < numKernels; iter++) {
        int bestIndex = -1;             // Index of best kernel component
        double bestChi2 = INFINITY;     // Value of best chi^2
        double bestCoeff = 0;           // Value of best coefficient
        double bestBG = 0;              // Value of best background

        for (int i = 0; i < numKernels; i++) {
            if (ranking->data.S32[i] >= 0) {
                continue;
            }

            double sumI = 0.0;          // sum of I(x)/sigma(x)^2
            double sumII = 0.0;         // sum of I(x)^2/sigma(x)^2
            double sumIC = 0.0;         // sum of I(x)C(x)/sigma(x)^2

            for (int j = 0; j < numStamps; j++) {
                if (badStamps->data.U8[j]) {
                    continue;
                }
                pmSubtractionStamp *stamp = stamps->stamps->data[j]; // Stamp of interest
                accumulateCross(&sumI, &sumII, &sumIC, stamp, targets->data[j], i, footprint, mode);
            }

            double invDet = 1.0 / (sum1 * sumCC->data.F64[i] - PS_SQR(sumC->data.F64[i])); // Determinant^-1
            double coeff = invDet * (sum1 * sumIC - sumC->data.F64[i] * sumI); // Coefficient for kernel
            double bg = invDet * (sumCC->data.F64[i] * sumI - sumC->data.F64[i] * sumIC); // Background

            double chi2 = 0.0;          // Chi^2
            for (int j = 0; j < numStamps; j++) {
                if (badStamps->data.U8[j]) {
                    continue;
                }
                pmSubtractionStamp *stamp = stamps->stamps->data[j]; // Stamp of interest
                chi2 += accumulateChi2(targets->data[j], stamp, i, coeff, bg, footprint, mode);
            }

            if (chi2 < bestChi2) {
                bestIndex = i;
                bestCoeff = coeff;
                bestChi2 = chi2;
                bestBG = bg;
            }

            psTrace("psModules.imcombine", 8, "%d: %lf %lf %lf %lf %lf %lf\n", i, sum1, sumI, sumII, sumIC,
                    sumC->data.F64[i], sumCC->data.F64[i]);
            psTrace("psModules.imcombine", 6, "%d: %lf %lf %lf\n", i, coeff, bg, chi2);
        }
        bestChi2 /= numPixels;

        if (bestIndex == -1) {
            psError(PM_ERR_DATA, true, "Unable to find best kernel component in round %d.", iter);
            psFree(targets);
            psFree(sumC);
            psFree(sumCC);
            psFree(ranking);
            psFree(kernels);
            psFree(badStamps);
            return NULL;
        }

        // And the winner is....
        ranking->data.S32[bestIndex] = iter;
        // Remove its contribution, and don't include it in the future.
        for (int j = 0; j < numStamps; j++) {
            if (badStamps->data.U8[j]) {
                continue;
            }
            pmSubtractionStamp *stamp = stamps->stamps->data[j]; // Stamp of interest
            subtractConvolution(targets->data[j], stamp, bestIndex, bestCoeff, bestBG, footprint, mode);
        }

        double diff = lastChi2 - bestChi2; // Difference in chi^2 between iterations

        psTrace("psModules.imcombine", 3, "The winner of round %d is %d (%f,%d,%d): %lf (%lf) --> %lf\n",
                iter, bestIndex, kernels->widths->data.F32[bestIndex], kernels->u->data.S32[bestIndex],
                kernels->v->data.S32[bestIndex], bestCoeff, diff, bestChi2);

        if (fabsf(diff) < tolerance) {
            cutIndex = iter;
            break;
        }

        lastChi2 = bestChi2;
    }
    psFree(targets);
    psFree(sumC);
    psFree(sumCC);

    if (cutIndex < 0) {
        psError(PM_ERR_DATA, true, "Unable to converge to tolerance %g\n", tolerance);
        psFree(ranking);
        psFree(kernels);
        psFree(badStamps);
        return NULL;
    }

    int newSize = cutIndex + 1;         // Size of new kernel basis set
    psTrace("psModules.imcombine", 2, "Accepting %d kernels.\n", newSize);
    psVector *uNew = psVectorAlloc(newSize, PS_TYPE_S32);
    psVector *vNew = psVectorAlloc(newSize, PS_TYPE_S32);
    psVector *widthsNew = psVectorAlloc(newSize, PS_TYPE_F32);
    psArray *preCalcNew = psArrayAlloc(newSize);
    psArray *convNew = psArrayAlloc(numStamps);
    for (int i = 0; i < numStamps; i++) {
        if (badStamps->data.U8[i]) {
            continue;
        }
        convNew->data[i] = psArrayAlloc(newSize);
    }

    for (int i = 0; i < numKernels; i++) {
        int rank = ranking->data.S32[i]; // This kernel component's ranking
        if (rank >= 0 && rank < newSize) {
            uNew->data.S32[rank] = kernels->u->data.S32[i];
            vNew->data.S32[rank] = kernels->v->data.S32[i];
            widthsNew->data.F32[rank] = kernels->widths->data.F32[i];
            preCalcNew->data[rank] = psMemIncrRefCounter(kernels->preCalc->data[i]);

            for (int j = 0; j < numStamps; j++) {
                if (badStamps->data.U8[j]) {
                    continue;
                }
                pmSubtractionStamp *stamp = stamps->stamps->data[j]; // Stamp of interest
                psArray *convolutions = convNew->data[j]; // Convolutions for this stamp
                convolutions->data[rank] = psMemIncrRefCounter(selectConvolution(stamp, i, mode));
            }
        }
    }
    psFree(kernels->u);
    psFree(kernels->v);
    psFree(kernels->widths);
    psFree(kernels->preCalc);
    kernels->u = uNew;
    kernels->v = vNew;
    kernels->widths = widthsNew;
    kernels->preCalc = preCalcNew;
    kernels->num = newSize;

    for (int i = 0; i < numStamps; i++) {
        if (badStamps->data.U8[i]) {
            continue;
        }
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        psFree(stamp->convolutions1);
        psFree(stamp->convolutions2);
        switch (mode) {
          case PM_SUBTRACTION_MODE_1:
            stamp->convolutions1 = convNew->data[i];
            stamp->convolutions2 = NULL;
            break;
          case PM_SUBTRACTION_MODE_2:
            stamp->convolutions1 = NULL;
            stamp->convolutions2 = convNew->data[i];
            break;
          default:
            psAbort("Unsupported subtraction mode: %x", mode);
        }
    }

    psFree(badStamps);
    psFree(ranking);

    // Maintain photometric scaling
    if (type == PM_SUBTRACTION_KERNEL_ISIS) {

        // XXX in r26035, this code was just wrong.  we had:

        // psKernel *subtract = kernels->preCalc->data[0]

        // but, kernels->preCalc was an array of psArray, not an array of kernels.  It is now
        // an array of pmSubtractionKernelPreCalc.

        pmSubtractionKernelPreCalc *subtract = kernels->preCalc->data[0]; // Kernel to subtract from the rest

        for (int i = 1; i < newSize; i++) {
            if (kernels->u->data.S32[i] % 2 == 0 && kernels->v->data.S32[i] % 2 == 0) {
                pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[i]; // Kernel of interest
                psBinaryOp(preCalc->kernel->image, preCalc->kernel->image, "-", subtract->kernel->image);
            }
        }
    } else if (type == PM_SUBTRACTION_KERNEL_GUNK) {
        psStringPrepend(&kernels->description, "GUNK=");
        psStringAppend(&kernels->description, "+POIS(%d,%d)", inner, spatialOrder);

        for (int i = 0; i < newSize; i++) {
            if (kernels->u->data.S32[i] % 2 == 0 && kernels->v->data.S32[i] % 2 == 0) {
                pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[i]; // Kernel of interest
                preCalc->kernel->kernel[0][0] -= 1.0;
            }
        }

        if (!p_pmSubtractionKernelsAddGrid(kernels, numGaussians, inner)) {
            psAbort("Should never get here.");
        }

        kernels->type = PM_SUBTRACTION_KERNEL_GUNK;
    }

    return kernels;
}

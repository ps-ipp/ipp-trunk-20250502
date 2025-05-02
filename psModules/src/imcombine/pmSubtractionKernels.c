#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>

#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionHermitian.h"
#include "pmSubtractionDeconvolve.h"
#include "pmSubtractionVisual.h"

#define RINGS_BUFFER 10                 // Buffer size for RINGS data

// Free function for pmSubtractionKernels
static void subtractionKernelsFree(pmSubtractionKernels *kernels)
{
    psFree(kernels->description);
    psFree(kernels->fwhms);
    psFree(kernels->orders);
    psFree(kernels->u);
    psFree(kernels->v);
    psFree(kernels->widths);
    psFree(kernels->uStop);
    psFree(kernels->vStop);
    psFree(kernels->preCalc);
    psFree(kernels->penalties1);
    psFree(kernels->penalties2);
    psFree(kernels->solution1);
    psFree(kernels->solution2);
    psFree(kernels->solution1err);
    psFree(kernels->solution2err);
    psFree(kernels->sampleStamps);
}

// Free function for pmSubtractionPreCalcKernel
static void pmSubtractionKernelPreCalcFree(pmSubtractionKernelPreCalc *kernel)
{
    psFree(kernel->xKernel);
    psFree(kernel->yKernel);
    psFree(kernel->kernel);

    psFree(kernel->uCoords);
    psFree(kernel->vCoords);
    psFree(kernel->poly);
}

// Raise an integer to an integer power
static inline long power(int value,     // Value
                         int exp        // Exponent
    )
{
    if (exp == 0) {
        return 1.0;
    }
    long result = value;               // Result to return
    for (int i = 2; i <= exp; i++) {
        result *= value;
    }
    return result;
}

// Generate 1D convolution kernel for SIMPLE
psVector *pmSubtractionKernelSIMPLE(float sigma, // Gaussian width
				    int order,   // Unused polynomial order
				    int size     // Kernel half-size
				    )
{
  int fullSize = 2 * size + 1;    // Full size of the kernel
  psVector *kernel = psVectorAlloc(fullSize, PS_TYPE_F32); // Kernel to return
  float expNorm = -0.5 / PS_SQR(sigma); // Normalization for exponential
  float norm    = 1.0 / sqrtf(2.0 * M_PI * sigma * sigma); // Correct Normalization for Gaussian
  if (sigma < 0.1) {
    kernel->data.F32[size] = 1.0;
    return(kernel);
  }
  for (int i = 0, x = -size; x <= size; i++,x++) {
    kernel->data.F32[i] = norm * expf(expNorm * PS_SQR(x));
  }
  return(kernel);
}


// Generate 1D convolution kernel for ISIS
psVector *pmSubtractionKernelISIS(float sigma, // Gaussian width
                                       int order, // Polynomial order
                                       int size // Kernel half-size
    )
{
    int fullSize = 2 * size + 1;        // Full size of kernel
    psVector *kernel = psVectorAlloc(fullSize, PS_TYPE_F32); // Kernel to return

    float expNorm = -0.5 / PS_SQR(sigma); // Normalisation for exponential
    float norm = 1.0 / (M_2_PI * sqrtf(sigma)); // Normalisation for Gaussian
    for (int i = 0, x = -size; x <= size; i++, x++) {
        kernel->data.F32[i] = norm * power(x, order) * expf(expNorm * PS_SQR(x));
    }

    return kernel;
}

// Generate 1D convolution kernel for HERM (normalized for 2D)
psVector *pmSubtractionKernelHERM(float sigma, // Gaussian width
                                       int order, // Polynomial order
                                       int size // Kernel half-size
    )
{
    int fullSize = 2 * size + 1;        // Full size of kernel
    psVector *kernel = psVectorAlloc(fullSize, PS_TYPE_F32); // Kernel to return

    // for now, we are only allowing equal orders and sigmas in X and Y
    float nf = exp(lgamma(order + 1));
    float norm = 1.0 / sqrt(nf*sigma*sqrt(M_2_PI));

    for (int i = 0, x = -size; x <= size; i++, x++) {
        float xf = x / sigma;
        float z = -0.25*xf*xf;
        kernel->data.F32[i] = norm * p_pmSubtractionHermitianPolynomial(xf, order) * exp(z);
    }

    return kernel;
}

// Generate 1D convolution kernel for HERM (normalized for 2D)
psKernel *pmSubtractionKernelHERM_RADIAL(float sigma, // Gaussian width
                                         int order, // Polynomial order
                                         int size // Kernel half-size
    )
{
    psKernel *kernel = psKernelAlloc(-size, size, -size, size); // 2D Kernel

    // for now, we are only allowing equal orders and sigmas in X and Y
    float nf = exp(lgamma(order + 1));
    float norm = 1.0 / sqrt(nf*sigma*sqrt(M_2_PI));

    // generate 2D radial hermitian
    for (int v = -size; v <= size; v++) {
        for (int u = -size; u <= size; u++) {
            float r = hypot(u, v) / sigma;
            float z = -0.25*r*r;
            kernel->kernel[v][u] = norm * p_pmSubtractionHermitianPolynomial(r, order) * exp(z);
        }
    }

    return kernel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semi-public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool p_pmSubtractionKernelsAddGrid(pmSubtractionKernels *kernels, int start, int size)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PS_ASSERT_INT_NONNEGATIVE(start, false);
    PS_ASSERT_INT_NONNEGATIVE(size, false);

    int numNew = PS_SQR(2 * size + 1) - 1;  // Number of new kernel parameters to add

    // Ensure the sizes match
    kernels->widths = psVectorRealloc(kernels->widths, start + numNew);
    kernels->u = psVectorRealloc(kernels->u, start + numNew);
    kernels->v = psVectorRealloc(kernels->v, start + numNew);
    kernels->preCalc = psArrayRealloc(kernels->preCalc, start + numNew);

    kernels->penalties1 = psVectorRealloc(kernels->penalties1, start + numNew);
    kernels->penalties2 = psVectorRealloc(kernels->penalties2, start + numNew);

    kernels->inner = start;
    kernels->num += numNew;

    // Generate a set of kernels for each (u,v)
    for (int v = - size, index = start; v <= size; v++) {
        for (int u = - size; u <= size; u++, index++) {
            if (v == 0 && u == 0) {
                // Skip normalisation component: added explicitly
                index--;
                continue;
            }
            kernels->widths->data.F32[index] = NAN;
            kernels->u->data.S32[index] = u;
            kernels->v->data.S32[index] = v;
            kernels->preCalc->data[index] = NULL;

	    // XXX this needs to be changed to use the *convolved* second moment
            kernels->penalties1->data.F32[index] = kernels->penalty * PS_SQR(PS_SQR(u) + PS_SQR(v));
            psAssert (isfinite(kernels->penalties1->data.F32[index]), "invalid penalty");

            kernels->penalties2->data.F32[index] = kernels->penalty * PS_SQR(PS_SQR(u) + PS_SQR(v));
            psAssert (isfinite(kernels->penalties2->data.F32[index]), "invalid penalty");

            psTrace("psModules.imcombine", 7, "Kernel %d: %d %d\n", index, u, v);
        }
    }

    kernels->widths->n = start + numNew;
    kernels->u->n = start + numNew;
    kernels->v->n = start + numNew;
    kernels->preCalc->n = start + numNew;

    kernels->penalties1->n = start + numNew;
    kernels->penalties2->n = start + numNew;

    return true;
}

# if (CENTRAL_DELTA)
// This version of the code uses the central pixel to force zero net flux, Alard actually uses
// kernel(0) for this purpose (for even order, kernel[i] = kernel'[i] - kernel[0])
static bool pmSubtractionKernelPreCalcNormalize(pmSubtractionKernels *kernels, pmSubtractionKernelPreCalc *preCalc,
						int index, int uOrder, int vOrder, float fwhm,
						bool AlardLuptonStyle, bool forceZeroNull)
{
    // we have 4 cases here:
    // 1) for odd functions, normalize the kernel by the maximum swing / Npix
    // 2) for even functions, normalize the kernel to unity
    // 3) for alard-lupton style normalization, subtract 1 from the 0,0 pixel for all even functions
    // 4) for deconvolved hermitians, subtract 1 from the 0,0 pixel for the 0,0 function(s)

    // Calculate moments
    double sum = 0.0, sum2 = 0.0;           // Sum of kernel component
    float min = INFINITY, max = -INFINITY;  // Minimum and maximum kernel value

    for (int v = preCalc->kernel->yMin; v <= preCalc->kernel->yMax; v++) {
        for (int u = preCalc->kernel->xMin; u <= preCalc->kernel->xMax; u++) {
            double value = preCalc->kernel->kernel[v][u];
            double value2 = PS_SQR(value);
            sum += value;
            sum2 += value2;
            min = PS_MIN(value, min);
            max = PS_MAX(value, max);
        }
    }

#if 0
    fprintf(stderr, "%d raw: %lf, null: %f, min: %lf, max: %lf\n", index, sum, preCalc->kernel->kernel[0][0], min, max);
#endif

    bool zeroNull = false;              // Zero out using the null position?
    float scale2D = NAN;                // Scaling for 2-D kernels

    if (AlardLuptonStyle) {
        if (uOrder % 2 == 0 && vOrder % 2 == 0) {
            // Even functions: normalise to unit sum and subtract null pixel so that sum is zero
	    // Re-normalize 
            // scale2D  = 1.0 / fabs(sum);
            scale2D  = 1.0 / sqrt(sum2) / PS_SQR(fwhm);
            zeroNull = true;
        } else {
            // Odd functions: choose normalisation so that parameters have about the same strength as for even
            // functions, no subtraction of null pixel because the sum is already (near) zero
            scale2D = 1.0 / sqrt(sum2) / PS_SQR(fwhm);
            zeroNull = false;
        }
    }

    if (!AlardLuptonStyle && (uOrder == 0 && vOrder == 0)) {
        zeroNull = true;
    }
    if (forceZeroNull) {
        // Force rescaling and subtraction of null pixel even though the order doesn't indicate it's even
        scale2D = 1.0 / fabs(sum) / PS_SQR(fwhm);
        zeroNull = true;
    }
    if (!forceZeroNull && ((uOrder % 2) || (vOrder % 2))) {
        // Odd function
        scale2D = 1.0 / sqrt(sum2) / PS_SQR(fwhm);
    }

    float scale1D = sqrtf(scale2D);     // Scaling for 1-D kernels
    if (preCalc->xKernel) {
        psBinaryOp(preCalc->xKernel, preCalc->xKernel, "*", psScalarAlloc(scale1D, PS_TYPE_F32));
    }
    if (preCalc->yKernel) {
        psBinaryOp(preCalc->yKernel, preCalc->yKernel, "*", psScalarAlloc(scale1D, PS_TYPE_F32));
    }

    psBinaryOp(preCalc->kernel->image, preCalc->kernel->image, "*", psScalarAlloc(scale2D, PS_TYPE_F32));

    if (zeroNull) {
        // preCalc->kernel->kernel[0][0] -= 1.0;
        preCalc->kernel->kernel[0][0] -= sum * scale2D;
    }

#if 0
    {
        double Sum = 0.0;   // Sum of kernel component
        double Sum2 = 0.0;   // Sum of kernel component
        float min = INFINITY, max = -INFINITY;  // Minimum and maximum kernel value
	for (int v = preCalc->kernel->yMin; v <= preCalc->kernel->yMax; v++) {
	    for (int u = preCalc->kernel->xMin; u <= preCalc->kernel->xMax; u++) {
		double value = preCalc->kernel->kernel[v][u];
                Sum += value;
		Sum2 += PS_SQR(value);
                min = PS_MIN(preCalc->kernel->kernel[v][u], min);
                max = PS_MAX(preCalc->kernel->kernel[v][u], max);
            }
        }
        fprintf(stderr, "%d sum: %lf, sum2: %lf, null: %f, min: %lf, max: %lf, scale: %f\n", index, Sum, Sum2, preCalc->kernel->kernel[0][0], min, max, scale2D);
    }
#endif

    kernels->widths->data.F32[index] = fwhm;
    kernels->u->data.S32[index] = uOrder;
    kernels->v->data.S32[index] = vOrder;
    if (kernels->preCalc->data[index]) {
        psFree(kernels->preCalc->data[index]);
    }
    kernels->preCalc->data[index] = preCalc;
    psTrace("psModules.imcombine", 7, "Kernel %d: %f %d %d\n", index, fwhm, uOrder, vOrder);

    return true;
}

# else /* CENTRAL_DELTA */

static bool zeroIsNormal = false;

// this code uses kernel(0) to force zero flux, and is invalid for other kinds of normalizations
static bool pmSubtractionKernelPreCalcNormalize(pmSubtractionKernels *kernels, pmSubtractionKernelPreCalc *preCalc,
						int index, int uOrder, int vOrder, float fwhm,
						pmSubtractionKernelPreCalc *zeroKernel)
{
    // 1) for odd functions: no renormalization 
    // 2) for even functions, normalize the kernel to unity
    // 3) for even functions & index > 0, subtract kernel(0)

    // Calculate moments
    double sum = 0.0, sum2 = 0.0;           // Sum of kernel component
    float min = INFINITY, max = -INFINITY;  // Minimum and maximum kernel value

    for (int v = preCalc->kernel->yMin; v <= preCalc->kernel->yMax; v++) {
        for (int u = preCalc->kernel->xMin; u <= preCalc->kernel->xMax; u++) {
            double value = preCalc->kernel->kernel[v][u];
            double value2 = PS_SQR(value);
            sum += value;
            sum2 += value2;
            min = PS_MIN(value, min);
            max = PS_MAX(value, max);
        }
    }

#if 0
    fprintf(stderr, "%d raw: %lf, null: %f, min: %lf, max: %lf\n", index, sum, preCalc->kernel->kernel[0][0], min, max);
#endif

    float scale2D = 1.0;		// Scaling for 2-D kernels
    float scale1D = 1.0;		// Scaling for 1-D kernels

    if (uOrder % 2 == 0 && vOrder % 2 == 0) {

	scale2D = 1.0 / sum;		// Scaling for 2-D kernels
	scale1D = sqrtf(scale2D);		// Scaling for 1-D kernels

	for (int v = preCalc->kernel->yMin; v <= preCalc->kernel->yMax; v++) {
	    for (int u = preCalc->kernel->xMin; u <= preCalc->kernel->xMax; u++) {
		preCalc->kernel->kernel[v][u] *= scale2D;
	    }
	}
# if (ZERO_KERNEL_ZERO_FLUX)
	int firstZeroIndex = 0;
# else
	int firstZeroIndex = 1;
# endif
	if (index < firstZeroIndex) {
	    zeroIsNormal = true;
	} else {
	    psAssert(zeroIsNormal, "failed to normalize zero kernel first");
	    psAssert(zeroKernel, "failed to supply zero kernel");
	    for (int v = preCalc->kernel->yMin; v <= preCalc->kernel->yMax; v++) {
		for (int u = preCalc->kernel->xMin; u <= preCalc->kernel->xMax; u++) {
		    preCalc->kernel->kernel[v][u] -= zeroKernel->kernel->kernel[v][u];
		}
	    }
	}

	// XXX why do we bother renormlizing the 1D kernels?  I don't think we use them again...
	if (preCalc->xKernel) {
	    psBinaryOp(preCalc->xKernel, preCalc->xKernel, "*", psScalarAlloc(scale1D, PS_TYPE_F32));
	}
	if (preCalc->yKernel) {
	    psBinaryOp(preCalc->yKernel, preCalc->yKernel, "*", psScalarAlloc(scale1D, PS_TYPE_F32));
	}
    }

#if 0
    {
        double Sum = 0.0;   // Sum of kernel component
        double Sum2 = 0.0;   // Sum of kernel component
        float min = INFINITY, max = -INFINITY;  // Minimum and maximum kernel value
	for (int v = preCalc->kernel->yMin; v <= preCalc->kernel->yMax; v++) {
	    for (int u = preCalc->kernel->xMin; u <= preCalc->kernel->xMax; u++) {
		double value = preCalc->kernel->kernel[v][u];
                Sum += value;
		Sum2 += PS_SQR(value);
                min = PS_MIN(preCalc->kernel->kernel[v][u], min);
                max = PS_MAX(preCalc->kernel->kernel[v][u], max);
            }
        }
        fprintf(stderr, "%d sum: %lf, sum2: %lf, null: %f, min: %lf, max: %lf, scale: %f\n", index, Sum, Sum2, preCalc->kernel->kernel[0][0], min, max, scale2D);
    }
#endif

    if (kernels) {
	kernels->widths->data.F32[index] = fwhm;
	kernels->u->data.S32[index] = uOrder;
	kernels->v->data.S32[index] = vOrder;
	if (kernels->preCalc->data[index]) {
	    psFree(kernels->preCalc->data[index]);
	}
	kernels->preCalc->data[index] = preCalc;
    }
    psTrace("psModules.imcombine", 7, "Kernel %d: %f %d %d\n", index, fwhm, uOrder, vOrder);

    return true;
}
# endif /* Central Delta */

pmSubtractionKernels *p_pmSubtractionKernelsRawISIS(int size, int spatialOrder,
                                                    const psVector *fwhmsIN, const psVector *ordersIN,
                                                    float penalty, psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_VECTOR_NON_NULL(fwhmsIN, NULL);
    PS_ASSERT_VECTOR_TYPE(fwhmsIN, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(ordersIN, NULL);
    PS_ASSERT_VECTOR_TYPE(ordersIN, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(fwhmsIN, ordersIN, NULL);
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);

    // check the requested fwhm values: any values <= 0.0 should be dropped
    psVector *fwhms  = psVectorAllocEmpty (fwhmsIN->n, PS_TYPE_F32);
    psVector *orders = psVectorAllocEmpty (ordersIN->n, PS_TYPE_S32);
    for (int i = 0; i < fwhmsIN->n; i++) {
        if (fwhmsIN->data.F32[i] <= FLT_EPSILON) continue;
        psVectorAppend(fwhms, fwhmsIN->data.F32[i]);
        psVectorAppend(orders, ordersIN->data.S32[i]);
    }

    int numGaussians = fwhms->n;       // Number of Gaussians

    int num = 0;                        // Number of basis functions
    for (int i = 0; i < numGaussians; i++) {
        int gaussOrder = orders->data.S32[i]; // Polynomial order to apply to Gaussian
        num += (gaussOrder + 1) * (gaussOrder + 2) / 2;
    }

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_ISIS, size, fwhms, orders, spatialOrder, penalty, bounds, mode); // Kernels
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

# if (!CENTRAL_DELTA && !ZERO_KERNEL_ZERO_FLUX)
    // in this case, subtract the 0th kernel from everyone else
    float zeroFWHM = fwhms->data.F32[0];
    float zeroSigma = zeroFWHM / (2.0 * sqrtf(2.0 * logf(2.0))); // Gaussian sigma
    pmSubtractionKernelPreCalc *zeroKernel = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_ISIS, 0, 0, size, zeroSigma); // structure to hold precalculated values
    pmSubtractionKernelPreCalcNormalize (NULL, zeroKernel, -1, 0, 0, zeroFWHM, NULL);
# endif

    // Set the kernel parameters
    for (int i = 0, index = 0; i < numGaussians; i++) {
        float sigma = fwhms->data.F32[i] / (2.0 * sqrtf(2.0 * logf(2.0))); // Gaussian sigma

# if (!CENTRAL_DELTA && ZERO_KERNEL_ZERO_FLUX)
	// in this case, subtract the a 1/2 width Gaussian from each series
	float zeroFWHM = 0.50 * fwhms->data.F32[0];
	float zeroSigma = zeroFWHM / (2.0 * sqrtf(2.0 * logf(2.0))); // Gaussian sigma
	pmSubtractionKernelPreCalc *zeroKernel = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_ISIS, 0, 0, size, zeroSigma); // structure to hold precalculated values
	pmSubtractionKernelPreCalcNormalize (NULL, zeroKernel, -1, 0, 0, zeroFWHM, NULL);
# endif
        // Iterate over (u,v) order
        for (int uOrder = 0; uOrder <= orders->data.S32[i]; uOrder++) {
            for (int vOrder = 0; vOrder <= orders->data.S32[i] - uOrder; vOrder++, index++) {

                pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_ISIS, uOrder, vOrder, size, sigma); // structure to hold precalculated values
# if (CENTRAL_DELTA)
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], false, false);
# else
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], zeroKernel);
# endif
            }
        }
# if (!CENTRAL_DELTA && ZERO_KERNEL_ZERO_FLUX)
	psFree(zeroKernel);
# endif
    }

# if (!CENTRAL_DELTA && !ZERO_KERNEL_ZERO_FLUX)
    psFree(zeroKernel);
# endif
    psFree(orders);
    psFree(fwhms);

    return kernels;
}

pmSubtractionKernels *pmSubtractionKernelsISIS_RADIAL(int size, int spatialOrder,
                                                      const psVector *fwhmsIN, const psVector *ordersIN,
                                                      float penalty, psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_VECTOR_NON_NULL(fwhmsIN, NULL);
    PS_ASSERT_VECTOR_TYPE(fwhmsIN, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(ordersIN, NULL);
    PS_ASSERT_VECTOR_TYPE(ordersIN, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(fwhmsIN, ordersIN, NULL);
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);

    // check the requested fwhm values: any values <= 0.0 should be dropped
    psVector *fwhms  = psVectorAllocEmpty (fwhmsIN->n, PS_TYPE_F32);
    psVector *orders = psVectorAllocEmpty (ordersIN->n, PS_TYPE_S32);
    for (int i = 0; i < fwhmsIN->n; i++) {
        if (fwhmsIN->data.F32[i] <= FLT_EPSILON) continue;
        psVectorAppend(fwhms, fwhmsIN->data.F32[i]);
        psVectorAppend(orders, ordersIN->data.S32[i]);
    }

    int numGaussians = fwhms->n;       // Number of Gaussians

    int num = 0;                        // Number of basis functions
    for (int i = 0; i < numGaussians; i++) {
        int gaussOrder = orders->data.S32[i]; // Polynomial order to apply to Gaussian
        num += (gaussOrder + 1) * (gaussOrder + 2) / 2;
        num += (11 - gaussOrder - 1);   // include all higher order radial terms
    }

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_ISIS_RADIAL, size, fwhms, orders, spatialOrder, penalty, bounds, mode); // Kernels
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    // Set the kernel parameters
    for (int i = 0, index = 0; i < numGaussians; i++) {
        float sigma = fwhms->data.F32[i] / (2.0 * sqrtf(2.0 * logf(2.0))); // Gaussian sigma
        // Iterate over (u,v) order
        for (int uOrder = 0; uOrder <= orders->data.S32[i]; uOrder++) {
            for (int vOrder = 0; vOrder <= orders->data.S32[i] - uOrder; vOrder++, index++) {
                pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_ISIS, uOrder, vOrder, size, sigma); // structure to hold precalculated values
# if (CENTRAL_DELTA)
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], true, false);
# else
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], NULL);
# endif
            }
        }
        for (int order = orders->data.S32[i] + 1; order < 11; order ++, index ++) {
            // XXX modify size for hermitians to account for sqrt(2) in Hermitian definition (relative to ISIS Gaussian)
            pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_ISIS_RADIAL, order, order, size, sigma / sqrt(2.0)); // structure to hold precalculated values
# if (CENTRAL_DELTA)
            pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, order, order, fwhms->data.F32[i], true, true);
# else
            pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, order, order, fwhms->data.F32[i], NULL);
# endif
        }
    }
    return kernels;
}

pmSubtractionKernels *pmSubtractionKernelsHERM(int size, int spatialOrder,
                                               const psVector *fwhmsIN, const psVector *ordersIN,
                                               float penalty, psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_VECTOR_NON_NULL(fwhmsIN, NULL);
    PS_ASSERT_VECTOR_TYPE(fwhmsIN, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(ordersIN, NULL);
    PS_ASSERT_VECTOR_TYPE(ordersIN, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(fwhmsIN, ordersIN, NULL);
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);

    // check the requested fwhm values: any values <= 0.0 should be dropped
    psVector *fwhms  = psVectorAllocEmpty (fwhmsIN->n, PS_TYPE_F32);
    psVector *orders = psVectorAllocEmpty (ordersIN->n, PS_TYPE_S32);
    for (int i = 0; i < fwhmsIN->n; i++) {
        if (fwhmsIN->data.F32[i] <= FLT_EPSILON) continue;
        psVectorAppend(fwhms, fwhmsIN->data.F32[i]);
        psVectorAppend(orders, ordersIN->data.S32[i]);
    }

    int numGaussians = fwhms->n;       // Number of Gaussians

    int num = 0;                        // Number of basis functions
    for (int i = 0; i < numGaussians; i++) {
        int gaussOrder = orders->data.S32[i]; // Polynomial order to apply to Gaussian
        num += (gaussOrder + 1) * (gaussOrder + 2) / 2;
    }

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_HERM, size, fwhms, orders, spatialOrder, penalty, bounds, mode); // Kernels
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    // Set the kernel parameters
    for (int i = 0, index = 0; i < numGaussians; i++) {
        float sigma = fwhms->data.F32[i] / (2.0 * sqrtf(2.0 * logf(2.0))); // Gaussian sigma
        // Iterate over (u,v) order
        for (int uOrder = 0; uOrder <= orders->data.S32[i]; uOrder++) {
            for (int vOrder = 0; vOrder <= orders->data.S32[i] - uOrder; vOrder++, index++) {
                pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_HERM, uOrder, vOrder, size, sigma); // structure to hold precalculated values
# if (CENTRAL_DELTA)
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], true, false);
# else
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], NULL);
# endif
            }
        }
    }

    return kernels;
}

pmSubtractionKernels *pmSubtractionKernelsDECONV_HERM(int size, int spatialOrder,
                                                      const psVector *fwhmsIN, const psVector *ordersIN,
                                                      float penalty, psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_VECTOR_NON_NULL(fwhmsIN, NULL);
    PS_ASSERT_VECTOR_TYPE(fwhmsIN, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(ordersIN, NULL);
    PS_ASSERT_VECTOR_TYPE(ordersIN, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(fwhmsIN, ordersIN, NULL);
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);

    // check the requested fwhm values: any values <= 0.0 should be dropped
    psVector *fwhms  = psVectorAllocEmpty (fwhmsIN->n, PS_TYPE_F32);
    psVector *orders = psVectorAllocEmpty (ordersIN->n, PS_TYPE_S32);
    for (int i = 0; i < fwhmsIN->n; i++) {
        if (fwhmsIN->data.F32[i] <= FLT_EPSILON) continue;
        psVectorAppend(fwhms, fwhmsIN->data.F32[i]);
        psVectorAppend(orders, ordersIN->data.S32[i]);
    }

    int numGaussians = fwhms->n;       // Number of Gaussians

    int num = 0;                        // Number of basis functions
    for (int i = 0; i < numGaussians; i++) {
        int gaussOrder = orders->data.S32[i]; // Polynomial order to apply to Gaussian
        num += PS_SQR(gaussOrder + 1);
    }

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_DECONV_HERM, size, fwhms, orders, spatialOrder, penalty, bounds, mode); // Kernels
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    // XXXXX hard-wired reference sigma for now of 1.7 pix (== 4.0 pix fwhm == 1.0 arcsec in simtest)
    // generate the Gaussian deconvolution kernel
    # define DECONV_SIGMA 1.6
    psKernel *kernelGauss = pmSubtractionDeconvolveGauss (size, DECONV_SIGMA);

# if 1
    psArray *deconKernels = psArrayAllocEmpty(100);
# endif

    // Set the kernel parameters
    for (int i = 0, index = 0; i < numGaussians; i++) {
        float sigma = fwhms->data.F32[i] / (2.0 * sqrtf(2.0 * logf(2.0))); // Gaussian sigma
        // Iterate over (u,v) order
        for (int uOrder = 0; uOrder <= orders->data.S32[i]; uOrder++) {
            for (int vOrder = 0; vOrder <= orders->data.S32[i]; vOrder++, index++) {

                pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc(PM_SUBTRACTION_KERNEL_HERM, uOrder, vOrder, size, sigma); // structure to hold precalculated values

                // save the generated 2D kernel as the target, deconvolve it by Gaussian, replacing the generated 2D kernel
                psKernel *kernelTarget = preCalc->kernel;
                preCalc->kernel = pmSubtractionDeconvolveKernel(kernelTarget, kernelGauss); // Kernel

                // XXX do we use Alard-Lupton normalization (last param true) or not?
# if (CENTRAL_DELTA)
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], true, false);
# else		
                pmSubtractionKernelPreCalcNormalize (kernels, preCalc, index, uOrder, vOrder, fwhms->data.F32[i], NULL);
# endif
                // XXXX test demo that deconvolved kernel is valid
# if 1
                psImage *kernelConv = psImageConvolveFFT(NULL, preCalc->kernel->image, NULL, 0, kernelGauss);
                psArrayAdd (deconKernels, 100, kernelConv);
                psFree (kernelConv);

                if (!uOrder && !vOrder){
                    pmSubtractionVisualShowSubtraction (kernelTarget->image, preCalc->kernel->image, kernelConv);
                }
# endif
            }
        }
    }

# if 1
    psImage *dot = psImageAlloc(deconKernels->n, deconKernels->n, PS_TYPE_F32);
    for (int i = 0; i < deconKernels->n; i++) {
        for (int j = 0; j <= i; j++) {
            psImage *t1 = deconKernels->data[i];
            psImage *t2 = deconKernels->data[j];

            double sum = 0.0;
            for (int iy = 0; iy < t1->numRows; iy++) {
                for (int ix = 0; ix < t1->numCols; ix++) {
                    sum += t1->data.F32[iy][ix] * t2->data.F32[iy][ix];
                }
            }
            dot->data.F32[j][i] = sum;
            dot->data.F32[i][j] = sum;
        }
    }
    pmSubtractionVisualShowSubtraction (dot, NULL, NULL);
    psFree (dot);
    psFree (deconKernels);
# endif

    return kernels;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

pmSubtractionKernels *pmSubtractionKernelsAlloc(int numBasisFunctions, pmSubtractionKernelsType type,
                                                int size, psVector *fwhms, psVector *orders, int spatialOrder, float penalty, psRegion bounds,
                                                pmSubtractionMode mode)
{
    pmSubtractionKernels *kernels = psAlloc(sizeof(pmSubtractionKernels)); // Kernels, to return
    psMemSetDeallocator(kernels, (psFreeFunc)subtractionKernelsFree);

    kernels->type = type;
    kernels->description = NULL;
    kernels->num = numBasisFunctions;
    kernels->fwhms = psMemIncrRefCounter(fwhms);
    kernels->orders = psMemIncrRefCounter(orders);
    kernels->u = psVectorAlloc(numBasisFunctions, PS_TYPE_S32);
    kernels->v = psVectorAlloc(numBasisFunctions, PS_TYPE_S32);
    kernels->widths = psVectorAlloc(numBasisFunctions, PS_TYPE_F32);
    kernels->uStop = NULL;
    kernels->vStop = NULL;
    kernels->xMin = bounds.x0;
    kernels->xMax = bounds.x1;
    kernels->yMin = bounds.y0;
    kernels->yMax = bounds.y1;
    kernels->preCalc = psArrayAlloc(numBasisFunctions);
    kernels->penalty = penalty;
    kernels->penalties1 = psVectorAlloc(numBasisFunctions, PS_TYPE_F32);
    psVectorInit(kernels->penalties1, NAN);
    kernels->penalties2 = psVectorAlloc(numBasisFunctions, PS_TYPE_F32);
    psVectorInit(kernels->penalties2, NAN);
    kernels->havePenalties = false;
    kernels->size = size;
    kernels->inner = 0;
    kernels->binning = 0;
    kernels->ringsOrder = 0;
    kernels->spatialOrder = spatialOrder;
    kernels->bgOrder = 0;
    kernels->mode = mode;
    kernels->solution1 = NULL;
    kernels->solution2 = NULL;
    kernels->solution1err = NULL;
    kernels->solution2err = NULL;
    kernels->mean = NAN;
    kernels->rms = NAN;
    kernels->numStamps = 0;
    kernels->sampleStamps = NULL;

    kernels->fResSigmaMean  = NAN;
    kernels->fResSigmaStdev = NAN;
    kernels->fResOuterMean  = NAN;
    kernels->fResOuterStdev = NAN;
    kernels->fResTotalMean  = NAN;
    kernels->fResTotalStdev = NAN;

    return kernels;
}

pmSubtractionKernelPreCalc *pmSubtractionKernelPreCalcAlloc(pmSubtractionKernelsType type, int uOrder, int vOrder, int size, float sigma) {

    pmSubtractionKernelPreCalc *preCalc = psAlloc(sizeof(pmSubtractionKernelPreCalc)); // Kernels, to return
    psMemSetDeallocator(preCalc, (psFreeFunc)pmSubtractionKernelPreCalcFree);

    // 1D kernel realizations:
    switch (type) {
      case PM_SUBTRACTION_KERNEL_ISIS:
        preCalc->xKernel = pmSubtractionKernelISIS(sigma, uOrder, size);
        preCalc->yKernel = pmSubtractionKernelISIS(sigma, vOrder, size);
        preCalc->uCoords = NULL;
        preCalc->vCoords = NULL;
        preCalc->poly    = NULL;
        break;
      case PM_SUBTRACTION_KERNEL_HERM:
        preCalc->xKernel = pmSubtractionKernelHERM(sigma, uOrder, size);
        preCalc->yKernel = pmSubtractionKernelHERM(sigma, vOrder, size);
        preCalc->uCoords = NULL;
        preCalc->vCoords = NULL;
        preCalc->poly    = NULL;
        break;
    case PM_SUBTRACTION_KERNEL_SIMPLE:
      preCalc->xKernel = pmSubtractionKernelSIMPLE(sigma,uOrder,size);
      preCalc->yKernel = pmSubtractionKernelSIMPLE(sigma,vOrder,size);
      preCalc->uCoords = NULL;
      preCalc->vCoords = NULL;
      preCalc->poly    = NULL;
      break;
      case PM_SUBTRACTION_KERNEL_RINGS:
        // the RINGS kernel uses the uCoords, vCoords, and poly elements of the structure
        // we allocate these vectors here, but leave the kernel generation to the main function
        preCalc->xKernel = NULL;
        preCalc->yKernel = NULL;
        preCalc->kernel  = NULL;
        preCalc->uCoords = psVectorAllocEmpty(size, PS_TYPE_S32); // u coords
        preCalc->vCoords = psVectorAllocEmpty(size, PS_TYPE_S32); // v coords
        preCalc->poly    = psVectorAllocEmpty(size, PS_TYPE_F32); // Polynomial
        return preCalc;
      case PM_SUBTRACTION_KERNEL_ISIS_RADIAL:
        preCalc->kernel  = pmSubtractionKernelHERM_RADIAL(sigma, uOrder, size);
        preCalc->xKernel = NULL;
        preCalc->yKernel = NULL;
        preCalc->uCoords = NULL;
        preCalc->vCoords = NULL;
        preCalc->poly    = NULL;
        return preCalc;
      default:
        psAbort("programming error: invalid type for PreCalc kernel");
    }

    preCalc->kernel = psKernelAlloc(-size, size, -size, size); // 2D Kernel

    // generate 2D kernel from 1D realizations
    for (int v = -size, y = 0; v <= size; v++, y++) {
        for (int u = -size, x = 0; u <= size; u++, x++) {
            preCalc->kernel->kernel[v][u] = preCalc->xKernel->data.F32[x] * preCalc->yKernel->data.F32[y]; // Value of kernel
        }
    }

    return preCalc;
}

pmSubtractionKernels *pmSubtractionKernelsPOIS(int size, int spatialOrder, float penalty, psRegion bounds,
                                               pmSubtractionMode mode)
{
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);

    int num = PS_SQR(2 * size + 1) - 1; // Number of basis functions

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(0, PM_SUBTRACTION_KERNEL_POIS, size, NULL, NULL, spatialOrder, penalty, bounds, mode); // Kernels
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    if (!p_pmSubtractionKernelsAddGrid(kernels, 0, size)) {
        psAbort("Should never get here.");
    }

    return kernels;
}


pmSubtractionKernels *pmSubtractionKernelsISIS(int size, int spatialOrder,
                                               const psVector *fwhms, const psVector *orders,
                                               float penalty, psRegion bounds, pmSubtractionMode mode)
{
    pmSubtractionKernels *kernels = p_pmSubtractionKernelsRawISIS(size, spatialOrder, fwhms, orders,
                                                                  penalty, bounds, mode); // Kernels
    if (!kernels) {
        return NULL;
    }

    return kernels;
}

pmSubtractionKernels *pmSubtractionKernelsSPAM(int size, int spatialOrder, int inner, int binning,
                                               float penalty, psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);
    PS_ASSERT_INT_NONNEGATIVE(inner, NULL);
    PS_ASSERT_INT_LARGER_THAN(size, inner, NULL);
    PS_ASSERT_INT_POSITIVE(binning, NULL);

    // The outer region should be divisible by the "binning"; otherwise allocate remainder to the inner region
    int numOuter = (size - inner) / binning; // Number of summed pixels in the outer region
    int numInner = inner + (size - inner) % binning; // Number of pixels in the inner region
    assert(numOuter * binning + numInner == size);
    int numTotal = numOuter + numInner; // Total number of summed pixels

    psTrace("psModules.imcombine", 3, "Inner: %d Outer: %d\n", numInner, numOuter);

    int num = PS_SQR(2 * numTotal + 1) - 1; // Number of basis functions

    psTrace("psModules.imcombine", 3, "Number of basis functions: %d\n", num);

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_SPAM, size, NULL, NULL, spatialOrder, penalty, bounds, mode); // Kernels
    kernels->inner = inner;
    kernels->binning = binning;
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    kernels->uStop = psVectorAlloc(num, PS_TYPE_S32);
    kernels->vStop = psVectorAlloc(num, PS_TYPE_S32);

    psVector *locations = psVectorAlloc(2 * numTotal + 1, PS_TYPE_S32); // Locations for each kernel element
    psVector *widths = psVectorAlloc(2 * numTotal + 1, PS_TYPE_S32); // Widths for each kernel element
    locations->data.S32[numTotal] = 0;
    widths->data.S32[numTotal] = 0;
    for (int i = 1; i <= numInner; i++) {
        locations->data.S32[numTotal + i] = i;
        widths->data.S32[numTotal + i] = 0;
        locations->data.S32[numTotal - i] = - i;
        widths->data.S32[numTotal - i] = 0;
    }
    for (int i = numInner + 1; i <= numTotal; i++) {
        locations->data.S32[numTotal + i] = locations->data.S32[numTotal + i - 1] +
            widths->data.S32[numTotal + i - 1] + 1;
        widths->data.S32[numTotal + i] = binning - 1;
        locations->data.S32[numTotal - i] = locations->data.S32[numTotal - i + 1] - binning;
        widths->data.S32[numTotal - i] = binning - 1;
    }

    if (psTraceGetLevel("psModules.imcombine") >= 10) {
        for (int i = 0; i < 2 * numTotal + 1; i++) {
            psTrace("psModules.imcombine", 10, "%d: %d -> %d\n", i, locations->data.S32[i],
                    locations->data.S32[i] + widths->data.S32[i]);
        }
    }

    // Set the kernel parameters
    for (int i = - numTotal, index = 0; i <= numTotal; i++) {
        int u = locations->data.S32[numTotal + i]; // Location of pixel
        int uStop = u + widths->data.S32[numTotal + i]; // Width of pixel

        for (int j = - numTotal; j <= numTotal; j++, index++) {
            if (i == 0 && j == 0) {
                // Skip normalisation component: added explicitly
                index--;
                continue;
            }
            int v = locations->data.S32[numTotal + j]; // Location of pixel
            int vStop = v + widths->data.S32[numTotal + j]; // Width of pixel

            kernels->u->data.S32[index] = u;
            kernels->v->data.S32[index] = v;
            kernels->uStop->data.S32[index] = uStop;
            kernels->vStop->data.S32[index] = vStop;

            psTrace("psModules.imcombine", 7, "Kernel %d: %d %d %d %d\n", index,
                    u, uStop, v, vStop);
        }
    }

    psFree(locations);
    psFree(widths);

    psWarning("Kernel penalty for dual-convolution is not configured for SPAM kernels.");
    psVectorInit(kernels->penalties1, 0.0);
    psVectorInit(kernels->penalties2, 0.0);

    return kernels;
}


pmSubtractionKernels *pmSubtractionKernelsFRIES(int size, int spatialOrder, int inner, float penalty,
                                                psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);
    PS_ASSERT_INT_NONNEGATIVE(inner, NULL);
    PS_ASSERT_INT_LARGER_THAN(size, inner, NULL);

    int fibNum = 0;                     // Number of Fibonacci values
    int fibLast = 1, fibTotal = 2;      // Fibonacci sequence
    while (fibTotal < size - inner) {
        int temp = fibTotal;
        fibTotal += fibLast;
        fibLast = temp;
        fibNum++;
    }

    int numInner = inner;               // Number of pixels in the inner region
    int numOuter = fibNum;              // Number of summed pixels in the outer region
    int numTotal = numOuter + numInner; // Total number of summed pixels

    psTrace("psModules.imcombine", 3, "Inner: %d Outer: %d\n", numInner, numOuter);

    int num = PS_SQR(2 * numTotal + 1) - 1; // Number of basis functions

    psTrace("psModules.imcombine", 3, "Number of basis functions: %d\n", num);

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_FRIES, size, NULL, NULL, spatialOrder, penalty, bounds, mode); // Kernels
    kernels->inner = inner;
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    kernels->uStop = psVectorAlloc(num, PS_TYPE_S32);
    kernels->vStop = psVectorAlloc(num, PS_TYPE_S32);

    psVector *start = psVectorAlloc(2 * numTotal + 1, PS_TYPE_S32);
    psVector *stop = psVectorAlloc(2 * numTotal + 1, PS_TYPE_S32);
    start->data.S32[numTotal] = 0;
    stop->data.S32[numTotal] = 0;
    for (int i = 1; i <= numInner; i++) {
        start->data.S32[numTotal + i] = i;
        stop->data.S32[numTotal + i] = i;
        start->data.S32[numTotal - i] = -i;
        stop->data.S32[numTotal - i] = -i;
    }
    for (int i = numInner + 1, fibLast = 1, fib = 2, temp; i <= numTotal;
         i++, fib = (temp = fib) + fibLast, fibLast = temp) {
        start->data.S32[numTotal + i] = stop->data.S32[numTotal + i - 1] + 1;
        stop->data.S32[numTotal + i] = PS_MIN(start->data.S32[numTotal + i] + fib - 1, size);
        start->data.S32[numTotal - i] = - stop->data.S32[numTotal + i];
        stop->data.S32[numTotal - i] = - start->data.S32[numTotal + i];
    }

    if (psTraceGetLevel("psModules.imcombine") >= 10) {
        for (int i = 0; i < 2 * numTotal + 1; i++) {
            psTrace("psModules.imcombine", 10, "%d: %d -> %d\n", i, start->data.S32[i], stop->data.S32[i]);
        }
    }

    // Set the kernel parameters
    for (int i = - numTotal, index = 0; i <= numTotal; i++) {
        int u = start->data.S32[numTotal + i]; // Location of pixel
        int uStop = stop->data.S32[numTotal + i]; // Width of pixel
        for (int j = - numTotal; j <= numTotal; j++, index++) {
            if (i == 0 && j == 0) {
                // Skip normalisation component: added explicitly
                index--;
                continue;
            }
            int v = start->data.S32[numTotal + j]; // Location of pixel
            int vStop = stop->data.S32[numTotal + j]; // Width of pixel

            kernels->u->data.S32[index] = u;
            kernels->v->data.S32[index] = v;
            kernels->uStop->data.S32[index] = uStop;
            kernels->vStop->data.S32[index] = vStop;

            psTrace("psModules.imcombine", 7, "Kernel %d: %d %d %d %d\n", index,
                    u, uStop, v, vStop);
        }
    }

    psFree(start);
    psFree(stop);

    psWarning("Kernel penalty for dual-convolution is not configured for FRIES kernels.");
    psVectorInit(kernels->penalties1, 0.0);
    psVectorInit(kernels->penalties2, 0.0);

    return kernels;
}

// Grid United with Normal Kernel [description: GUNK=ISIS(...)+POIS(...)]
pmSubtractionKernels *pmSubtractionKernelsGUNK(int size, int spatialOrder, const psVector *fwhms,
                                               const psVector *orders, int inner, float penalty,
                                               psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);
    PS_ASSERT_VECTOR_NON_NULL(fwhms, NULL);
    PS_ASSERT_VECTOR_TYPE(fwhms, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(orders, NULL);
    PS_ASSERT_VECTOR_TYPE(orders, PS_TYPE_S32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(fwhms, orders, NULL);
    PS_ASSERT_INT_NONNEGATIVE(inner, NULL);
    PS_ASSERT_INT_LESS_THAN(inner, size, NULL);

    pmSubtractionKernels *kernels = p_pmSubtractionKernelsRawISIS(size, spatialOrder, fwhms, orders, penalty, bounds, mode); // Kernels
    kernels->type = PM_SUBTRACTION_KERNEL_GUNK;
    kernels->inner = inner;
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, (int) kernels->num);

    int numISIS = kernels->num;         // Number of ISIS kernels

    if (!p_pmSubtractionKernelsAddGrid(kernels, numISIS, inner)) {
        psAbort("Should never get here.");
    }

    return kernels;
}

// RINGS --- just what it says
pmSubtractionKernels *pmSubtractionKernelsRINGS(int size, int spatialOrder, int inner, int ringsOrder,
                                                float penalty, psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, NULL);
    PS_ASSERT_INT_NONNEGATIVE(inner, NULL);
    PS_ASSERT_INT_LESS_THAN_OR_EQUAL(inner, size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(ringsOrder, NULL);

    int fibNum = 0;                     // Number of Fibonacci values
    {
        int fibIndex = 1, fibIndexMinus1 = 0; // Fibonnacci parameters
        int radius = inner;
        while (radius + fibIndex < size) {
            radius++;
            int fibNew = fibIndex + fibIndexMinus1;
            fibIndexMinus1 = fibIndex;
            fibIndex = fibNew;
            radius += fibIndex;
            fibNum++;
        }
    }

    int numInner = inner - 1;           // Number of pixels in the inner region
    int numOuter = fibNum;              // Number of summed pixels in the outer region

    int numRings = numOuter + numInner; // Number of rings (not including the central pixel)
    int numPoly = PM_SUBTRACTION_POLYTERMS(ringsOrder); // Number of polynomial variants of each ring

    int num = numRings * numPoly; // Total number of basis functions

    pmSubtractionKernels *kernels = pmSubtractionKernelsAlloc(num, PM_SUBTRACTION_KERNEL_RINGS, size, NULL, NULL, spatialOrder, penalty, bounds, mode); // Kernels
    kernels->inner = inner;
    kernels->ringsOrder = ringsOrder;
    pmSubtractionKernelsMakeDescription(kernels);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "kernel: %s --> %d elements", kernels->description, num);

    // Set the Gaussian kernel parameters
    int fibIndex = 1, fibIndexMinus1 = 0; // Fibonnacci parameters
    int radiusLast = 1;                 // Last radius
    for (int i = 1, index = 0; i < numRings + 1; i++) {
        float lower2;                   // Lower limit of radius^2
        float upper2;                   // Upper limit of radius^2
        if (i <= inner) {
            // A ring every pixel width
            float radius = i;
            lower2 = PS_SQR(radius - 0.5);
            upper2 = PS_SQR(radius + 0.5);
            radiusLast = i;
        } else {
            // Rings Fibonacci distributed (2, 3, 5...)
            int fibNew = fibIndex + fibIndexMinus1;
            fibIndexMinus1 = fibIndex;
            fibIndex = fibNew;

            float radiusLower = radiusLast + 1;
            radiusLast = radiusLower + fibIndex;
            float radiusUpper = radiusLast;

            lower2 = PS_SQR(radiusLower - 0.5);
            upper2 = PS_SQR(radiusUpper + 0.5);
        }

        psTrace("psModules.imcombine", 8, "Radius limits: %f --> %f\n", sqrtf(lower2), sqrtf(upper2));

        // Iterate over (u,v) order
        for (int uOrder = 0; uOrder <= (i == 0 ? 0 : ringsOrder); uOrder++) {
            for (int vOrder = 0; vOrder <= (i == 0 ? 0 : ringsOrder - uOrder); vOrder++, index++) {

                pmSubtractionKernelPreCalc *preCalc = pmSubtractionKernelPreCalcAlloc (PM_SUBTRACTION_KERNEL_RINGS, 0, 0, RINGS_BUFFER, 0.0);
                double moment = 0.0;    // Moment, for penalty

                if (i == 0) {
                    // Central pixel is easy
                    preCalc->uCoords->data.S32[0] = 0;
                    preCalc->vCoords->data.S32[0] = 0;
                    preCalc->poly->data.F32[0] = 1.0;
                    preCalc->uCoords->n = 1;
                    preCalc->vCoords->n = 1;
                    preCalc->poly->n = 1;
                    radiusLast = 0;
                    moment = 0.0;
                } else {
                    int j = 0;          // Index for data
                    double norm = 0.0;  // Normalisation
                    for (int v = -size; v <= size; v++) {
                        int v2 = PS_SQR(v);   // Square of v
                        float vPoly = powf(v/(float)size, vOrder); // Value of v^vOrder

                        for (int u = -size; u <= size; u++) {
                            int u2 = PS_SQR(u); // Square of u
                            int distance2 = u2 + v2; // Distance from the centre
                            if (distance2 > lower2 && distance2 < upper2) {
                                float uPoly = powf(u/(float)size, uOrder); // Value of u^uOrder

                                float polyVal = uPoly * vPoly; // Value of polynomial
                                if (polyVal != 0) { // No point adding it otherwise
                                    preCalc->uCoords->data.S32[j] = u;
                                    preCalc->vCoords->data.S32[j] = v;
                                    preCalc->poly->data.F32[j] = polyVal;
                                    norm += polyVal;
                                    moment += PS_SQR(polyVal) * PS_SQR(PS_SQR(u) + PS_SQR(v));

                                    psVectorExtend(preCalc->uCoords, RINGS_BUFFER, 1);
                                    psVectorExtend(preCalc->vCoords, RINGS_BUFFER, 1);
                                    psVectorExtend(preCalc->poly, RINGS_BUFFER, 1);
                                    psTrace("psModules.imcombine", 9, "u = %d, v = %d, poly = %f\n",
                                            u, v, preCalc->poly->data.F32[j]);
                                    j++;
                                }
                            }
                        }
                    }
                    // Normalise kernel component to unit sum
                    if (uOrder % 2 == 0 && vOrder % 2 == 0) {
                        psBinaryOp(preCalc->poly, preCalc->poly, "*", psScalarAlloc(1.0 / norm, PS_TYPE_F32));
                        // Add subtraction of 0,0 component to preserve photometric scaling
                        preCalc->uCoords->data.S32[j] = 0;
                        preCalc->vCoords->data.S32[j] = 0;
                        preCalc->poly->data.F32[j] = -1.0;
                        psVectorExtend(preCalc->uCoords, RINGS_BUFFER, 1);
                        psVectorExtend(preCalc->vCoords, RINGS_BUFFER, 1);
                        psVectorExtend(preCalc->poly, RINGS_BUFFER, 1);
                    } else {
                        norm = powf(size, uOrder) * powf(size, vOrder);
                        psBinaryOp(preCalc->poly, preCalc->poly, "*", psScalarAlloc(1.0 / norm, PS_TYPE_F32));
                    }
                    moment /= PS_SQR(norm);
                }

                psTrace("psModules.imcombine", 8, "%ld pixels in kernel\n", preCalc->uCoords->n);

                kernels->preCalc->data[index] = preCalc;
                kernels->u->data.S32[index] = uOrder;
                kernels->v->data.S32[index] = vOrder;

		// XXX convert to use the convolved 2nd moment
                kernels->penalties1->data.F32[index] = kernels->penalty * fabsf(moment);
                if (!isfinite(kernels->penalties1->data.F32[index])) {
                    psAbort ("invalid penalty");
                }
                kernels->penalties2->data.F32[index] = kernels->penalty * fabsf(moment);
                if (!isfinite(kernels->penalties2->data.F32[index])) {
                    psAbort ("invalid penalty");
                }

                psTrace("psModules.imcombine", 7, "Kernel %d: %d %d %d\n", index,
                        i, uOrder, vOrder);
            }
        }
    }

    return kernels;
}

pmSubtractionKernels *pmSubtractionKernelsGenerate(pmSubtractionKernelsType type, int size, int spatialOrder,
                                                   const psVector *fwhms, const psVector *orders, int inner,
                                                   int binning, int ringsOrder, float penalty, psRegion bounds,
                                                   pmSubtractionMode mode)
{
    switch (type) {
      case PM_SUBTRACTION_KERNEL_POIS:
        return pmSubtractionKernelsPOIS(size, spatialOrder, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_ISIS:
        return pmSubtractionKernelsISIS(size, spatialOrder, fwhms, orders, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_ISIS_RADIAL:
        return pmSubtractionKernelsISIS_RADIAL(size, spatialOrder, fwhms, orders, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_HERM:
        return pmSubtractionKernelsHERM(size, spatialOrder, fwhms, orders, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_DECONV_HERM:
        return pmSubtractionKernelsDECONV_HERM(size, spatialOrder, fwhms, orders, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_SPAM:
        return pmSubtractionKernelsSPAM(size, spatialOrder, inner, binning, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_FRIES:
        return pmSubtractionKernelsFRIES(size, spatialOrder, inner, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_GUNK:
        return pmSubtractionKernelsGUNK(size, spatialOrder, fwhms, orders, inner, penalty, bounds, mode);
      case PM_SUBTRACTION_KERNEL_RINGS:
        return pmSubtractionKernelsRINGS(size, spatialOrder, inner, ringsOrder, penalty, bounds, mode);
    case PM_SUBTRACTION_KERNEL_SIMPLE:
      return pmSubtractionKernelsISIS(size,spatialOrder,fwhms,orders,penalty,bounds,mode);
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unknown kernel type: %x", type);
        return NULL;
    }
}


// Intermediate string parsing functions required because of different APIs for strtol and strtof
static inline int parseStringInt(const char *string)
{
    return strtol(string, NULL, 10);
}
static inline float parseStringFloat(const char *string)
{
    return strtof(string, NULL);
}


// Parse a string of a number, up to some delimiter, and advance past the delimiter
#define PARSE_STRING_NUMBER(TARGET, STRING, DELIM, PARSEFUNC) { \
    char *start = STRING;               /* Start of string */ \
    char *end = strchr(STRING, DELIM);  /* End of string */ \
    if (!end) { \
        psAbort("End of string encountered"); \
    } \
    int stringSize = end - STRING;      /* Size of string with value, NOT including \0 */ \
    char value[stringSize + 1];         /* String to parse */ \
    strncpy(value, start, stringSize); \
    value[stringSize] = '\0'; \
    TARGET = PARSEFUNC(value); \
    STRING += stringSize + 1;           /* Advance past delimiter */ \
}


pmSubtractionKernels *pmSubtractionKernelsFromDescription(const char *description, int bgOrder,
                                                          psRegion bounds, pmSubtractionMode mode)
{
    PS_ASSERT_STRING_NON_EMPTY(description, NULL);

    if (bgOrder != 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Background order %d is not yet supported.", bgOrder);
        return false;
    }

    pmSubtractionKernelsType type = PM_SUBTRACTION_KERNEL_NONE; // Type of kernel
    int size = 0;                       // Half-size of kernel
    int spatialOrder = 0;               // Order of spatial variations
    const psVector *fwhms = NULL;       // FWHM of Gaussians
    const psVector *orders = NULL;      // Polynomial order for each FWHM
    int inner = 0;                      // Size of inner region
    int binning = 0;                    // Binning to use
    int ringsOrder = 0;                 // Polynomial order for rings
    float penalty = 0.0;                // Penalty for wideness

    // currently known descriptions:
    // ISIS(...), ISIS_RADIAL(...), HERM(...), DECONV_HERM(...), POIS(...), SPAM(...),
    // FRIES(...), GUNK=ISIS(...)+POIS(...), RINGS(...),
    // the descriptive name is the set of characters before the (

    type = pmSubtractionKernelsTypeFromString (description);
    char *ptr = strchr(description, '(') + 1;
    psAssert (ptr, "description is missing kernel parameters");

    switch (type) {
      case PM_SUBTRACTION_KERNEL_ISIS:
      case PM_SUBTRACTION_KERNEL_ISIS_RADIAL:
      case PM_SUBTRACTION_KERNEL_HERM:
      case PM_SUBTRACTION_KERNEL_DECONV_HERM:
    case PM_SUBTRACTION_KERNEL_SIMPLE:
        PARSE_STRING_NUMBER(size, ptr, ',', parseStringInt);

        // Count the number of Gaussians
        int numGauss = 0;
        for (char *string = ptr; string; string = strchr(string + 1, '(')) {
            numGauss++;
        }

        fwhms = psVectorAlloc(numGauss, PS_TYPE_F32);
        orders = psVectorAlloc(numGauss, PS_TYPE_S32);

        for (int i = 0; i < numGauss; i++) {
            ptr++;                                                               // Eat the '('
            PARSE_STRING_NUMBER(fwhms->data.F32[i], ptr, ',', parseStringFloat); // Eat "1.234,"
            PARSE_STRING_NUMBER(orders->data.S32[i], ptr, ')', parseStringInt);  // Eat "3)"
        }

        ptr++;                      // Eat ','
        PARSE_STRING_NUMBER(spatialOrder, ptr, ',', parseStringInt);
        penalty = parseStringFloat(ptr);
        break;
      case PM_SUBTRACTION_KERNEL_RINGS:
        PARSE_STRING_NUMBER(size, ptr, ',', parseStringInt);
        PARSE_STRING_NUMBER(inner, ptr, ',', parseStringInt);
        PARSE_STRING_NUMBER(ringsOrder, ptr, ',', parseStringInt);
        PARSE_STRING_NUMBER(spatialOrder, ptr, ',', parseStringInt);
        PARSE_STRING_NUMBER(penalty, ptr, ')', parseStringInt);
        break;
      default:
        psAbort("Deciphering kernels other than ISIS, HERM, DECONV_HERM or RINGS is not currently supported.");
    }

    pmSubtractionKernels *outKernel = pmSubtractionKernelsGenerate(type, size, spatialOrder,
								  fwhms, orders, inner, binning,
								  ringsOrder, penalty, bounds, mode);
    psFree (fwhms);
    psFree (orders);

    return outKernel;
}


// the input string can either be just the name or the description string.  Currently known
// descriptions: ISIS(...), ISIS_RADIAL(...), HERM(...), DECONV_HERM(...), POIS(...),
// SPAM(...), FRIES(...), GUNK=ISIS(...)+POIS(...), RINGS(...),
pmSubtractionKernelsType pmSubtractionKernelsTypeFromString(const char *type)
{
    // for a bare name (ISIS, HERM), use the full string length.
    // otherwise, use the length up to the first '('
    int nameLength = strlen(type);
    char *ptr = strchr(type, '(');
    if (ptr) {
        nameLength = ptr - type;
    }

    if (strncasecmp(type, "POIS", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_POIS;
    }
    if (strncasecmp(type, "ISIS", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_ISIS;
    }
    if (strncasecmp(type, "ISIS_RADIAL", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_ISIS_RADIAL;
    }
    if (strncasecmp(type, "HERM", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_HERM;
    }
    if (strncasecmp(type, "DECONV_HERM", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_DECONV_HERM;
    }
    if (strncasecmp(type, "SPAM", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_SPAM;
    }
    if (strncasecmp(type, "FRIES", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_FRIES;
    }
    if (strncasecmp(type, "GUNK", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_GUNK;
    }
    // note that GUNK has a somewhat different description
    if (strncasecmp(type, "GUNK=ISIS", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_GUNK;
    }
    if (strncasecmp(type, "RINGS", nameLength) == 0) {
        return PM_SUBTRACTION_KERNEL_RINGS;
    }
    if (strncasecmp(type, "SIMPLE", nameLength) == 0) {
      return PM_SUBTRACTION_KERNEL_SIMPLE;
    }
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised kernel type: %s", type);
    return PM_SUBTRACTION_KERNEL_NONE;
}

bool pmSubtractionKernelsMakeDescription(pmSubtractionKernels *kernels) {

    // free if it exists
    psFree (kernels->description);

    // generate the description parameter string
    psString params = NULL;
    if (kernels->fwhms) {
	for (int i = 0; i < kernels->fwhms->n; i++) {
	    psStringAppend(&params, "(%.1f,%d)", kernels->fwhms->data.F32[i], kernels->orders->data.S32[i]);
	}
    }

    switch (kernels->type) {
      case PM_SUBTRACTION_KERNEL_ISIS:
	psStringAppend (&kernels->description, "ISIS(%d,%s,%d,%.2e)", kernels->size, params, kernels->spatialOrder, kernels->penalty);
	break;

      case PM_SUBTRACTION_KERNEL_ISIS_RADIAL:
	psStringAppend(&kernels->description, "ISIS_RADIAL(%d,%s,%d,%.2e)", kernels->size, params, kernels->spatialOrder, kernels->penalty);
	break;

      case PM_SUBTRACTION_KERNEL_HERM:
	psStringAppend(&kernels->description, "HERM(%d,%s,%d,%.2e)", kernels->size, params, kernels->spatialOrder, kernels->penalty);
	break;

      case PM_SUBTRACTION_KERNEL_DECONV_HERM:
	psStringAppend(&kernels->description, "DECONV_HERM(%d,%s,%d,%.2e)", kernels->size, params, kernels->spatialOrder, kernels->penalty);
	break;

      case PM_SUBTRACTION_KERNEL_POIS:
	psStringAppend(&kernels->description, "POIS(%d,%d,%.2e)", kernels->size, kernels->spatialOrder, kernels->penalty);
	break;

      case PM_SUBTRACTION_KERNEL_SPAM:
	psStringAppend(&kernels->description, "SPAM(%d,%d,%d,%d,%.2e)", kernels->size, kernels->inner, kernels->binning, kernels->spatialOrder, kernels->penalty);
	break;

      case PM_SUBTRACTION_KERNEL_FRIES:
	psStringAppend(&kernels->description, "FRIES(%d,%d,%d,%.2e)", kernels->size, kernels->inner, kernels->spatialOrder, kernels->penalty);
	break;

	// Grid United with Normal Kernel [description: GUNK=ISIS(...)+POIS(...)]
      case PM_SUBTRACTION_KERNEL_GUNK:
	psStringAppend(&kernels->description, "GUNK=ISIS(%d,%s,%d,%.2e)", kernels->size, params, kernels->spatialOrder, kernels->penalty);
	psStringAppend(&kernels->description, "+POIS(%d,%d)", kernels->inner, kernels->spatialOrder);
	break;
	
      case PM_SUBTRACTION_KERNEL_RINGS:
	psStringAppend(&kernels->description, "RINGS(%d,%d,%d,%d,%.2e)", kernels->size, kernels->inner, kernels->ringsOrder, kernels->spatialOrder, kernels->penalty);
	break;

    case PM_SUBTRACTION_KERNEL_SIMPLE:
      psStringAppend(&kernels->description, "SIMPLE(%d,%s)",kernels->size,params);
      break;
      default:
        psAbort("unknown kernel");
    }
    psFree (params);
    return true;
}

pmSubtractionKernels *pmSubtractionKernelsCopy(const pmSubtractionKernels *in)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(in, NULL);

    pmSubtractionKernels *out = psAlloc(sizeof(pmSubtractionKernels)); // Kernels, to return
    psMemSetDeallocator(out, (psFreeFunc)subtractionKernelsFree);

    out->type = in->type;
    out->description = psMemIncrRefCounter(in->description);
    out->num = in->num;
    out->u = psMemIncrRefCounter(in->u);
    out->v = psMemIncrRefCounter(in->v);
    out->widths = psMemIncrRefCounter(in->widths);
    out->preCalc = psMemIncrRefCounter(in->preCalc);
    out->penalty = in->penalty;
    out->penalties1 = psMemIncrRefCounter(in->penalties1);
    out->penalties2 = psMemIncrRefCounter(in->penalties2);
    out->uStop = psMemIncrRefCounter(in->uStop);
    out->vStop = psMemIncrRefCounter(in->vStop);
    out->size = in->size;
    out->inner = in->inner;
    out->spatialOrder = in->spatialOrder;
    out->bgOrder = in->bgOrder;
    out->mode = in->mode;
    out->xMin = in->xMin;
    out->xMax = in->xMax;
    out->yMin = in->yMin;
    out->yMax = in->yMax;
    out->solution1 = in->solution1 ? psVectorCopy(NULL, in->solution1, PS_TYPE_F64) : NULL;
    out->solution2 = in->solution2 ? psVectorCopy(NULL, in->solution2, PS_TYPE_F64) : NULL;
    out->solution1err = in->solution1err ? psVectorCopy(NULL, in->solution1err, PS_TYPE_F64) : NULL;
    out->solution2err = in->solution2err ? psVectorCopy(NULL, in->solution2err, PS_TYPE_F64) : NULL;
    out->sampleStamps = psMemIncrRefCounter(in->sampleStamps);

    return out;
}

#define KERNEL_MOSAIC 2                 // Half-number of kernel instances in the mosaic image
psImage *pmSubtractionKernelsImageMosaic(pmSubtractionKernels *kernels) {

    psTrace("psModules.imcombine", 2, "Generating diagnostic image..\n");

    // Generate image with convolution kernels
    int size = kernels->size;       // Half-size of kernel
    int fullSize = 2 * size + 1 + 1; // Full size of kernel
    int imageSize = (2 * KERNEL_MOSAIC + 1) * fullSize;
    psImage *convKernels = psImageAlloc((kernels->mode == PM_SUBTRACTION_MODE_DUAL ? 2 : 1) *
					imageSize - 1 +
					(kernels->mode == PM_SUBTRACTION_MODE_DUAL ? 4 : 0),
					imageSize - 1, PS_TYPE_F32);
    psImageInit(convKernels, NAN);
    for (int j = -KERNEL_MOSAIC; j <= KERNEL_MOSAIC; j++) {
	for (int i = -KERNEL_MOSAIC; i <= KERNEL_MOSAIC; i++) {
	    psImage *kernel = pmSubtractionKernelImage(kernels, (float)i / (float)KERNEL_MOSAIC,
						       (float)j / (float)KERNEL_MOSAIC,
						       false); // Image of the kernel
	    if (!kernel) {
		psError(psErrorCodeLast(), false, "Unable to generate kernel image.");
		psFree(convKernels);
		return NULL;
	    }

	    if (psImageOverlaySection(convKernels, kernel, (i + KERNEL_MOSAIC) * fullSize,
				      (j + KERNEL_MOSAIC) * fullSize, "=") == 0) {
		psError(psErrorCodeLast(), false, "Unable to overlay kernel image.");
		psFree(kernel);
		psFree(convKernels);
		return NULL;
	    }
	    psFree(kernel);

	    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
		kernel = pmSubtractionKernelImage(kernels, (float)i / (float)KERNEL_MOSAIC,
						  (float)j / (float)KERNEL_MOSAIC,
						  true); // Image of the kernel
		if (!kernel) {
		    psError(psErrorCodeLast(), false, "Unable to generate kernel image.");
		    psFree(convKernels);
		    return NULL;
		}

		if (psImageOverlaySection(convKernels, kernel,
					  (2 * KERNEL_MOSAIC + 1 + i + KERNEL_MOSAIC) * fullSize + 4,
					  (j + KERNEL_MOSAIC) * fullSize, "=") == 0) {
		    psError(psErrorCodeLast(), false, "Unable to overlay kernel image.");
		    psFree(kernel);
		    psFree(convKernels);
		    return NULL;
		}
		psFree(kernel);
	    }
	}
    }
    return convKernels;
}

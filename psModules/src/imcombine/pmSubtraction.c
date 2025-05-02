/** @file pmSubtraction.c
 *
 *  @author Paul Price, IfA
 *  @author GLG, MHPCC
 *
 *  Copyright 2004-2007 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmHDU.h"                      // Required for pmFPA.h
#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionEquation.h"
#include "pmSubtractionVisual.h"
#include "pmSubtractionThreads.h"

bool psFitsWriteImageSimple (char *filename, psImage *image, psMetadata *header);

#include "pmSubtraction.h"

# define FFT_WINDOW 0
//#define TESTING

#define PIXEL_LIST_BUFFER 100           // Number of entries to add to pixel list at a time
#define MIN_SAMPLE_STATS    7           // Minimum number to use sample statistics; otherwise use quartiles
#define USE_KERNEL_ERR                  // Use kernel error image?
#define NUM_COVAR_POS 5                 // Number of positions for covariance calculation
//MEH -- this is causing diffim fault 5 -- seems not as robust
#define USE_LOGFIT_REJECT

// XXX we need to pass these fwhm values elsewhere.  These should go on one of the structure, but 
// things are too confusing to do that now.  just save them here.
static float FWHM1 = NAN;
static float FWHM2 = NAN;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Generate the kernel to apply to the variance from the normal kernel
static psKernel *varianceKernel(psKernel *out, // Output kernel
                                const psKernel *normalKernel // Normal kernel
                                )
{
    // Kernel range
    int xMin = normalKernel->xMin, xMax = normalKernel->xMax;
    int yMin = normalKernel->yMin, yMax = normalKernel->yMax;

    if (!out) {
        out = psKernelAlloc(xMin, xMax, yMin, yMax);
    }

    // Take the square of the normal kernel
    for (int v = yMin; v <= yMax; v++) {
        for (int u = xMin; u <= xMax; u++) {
            out->kernel[v][u] = PS_SQR(normalKernel->kernel[v][u]);
        }
    }
    return out;
}

// Contribute to an image of the solved kernel component using the preCalculated image
static void solvedKernelPreCalc(psKernel *kernel, // Kernel, updated
                                const pmSubtractionKernels *kernels, // Kernel basis functions
                                float value,                         // Normalisation value for basis function
                                int index                  // Index of basis function of interest
    )
{
    int size = kernels->size;           // Kernel half-size
    pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[index]; // Precalculated values
#if 0
    // Iterating over the kernel
    for (int y = 0, v = -size; v <= size; y++, v++) {
        float yValue = value * preCalc->yKernel->data.F32[y];
        for (int x = 0, u = -size; u <= size; x++, u++) {
            kernel->kernel[v][u] +=  yValue * preCalc->xKernel->data.F32[x];
        }
    }
    // Photometric scaling for even kernels only
    if (kernels->u->data.S32[i] % 2 == 0 && kernels->v->data.S32[i] % 2 == 0) {
        kernel->kernel[0][0] -= value;
    }
#else
    for (int v = -size; v <= size; v++) {
        for (int u = -size; u <= size; u++) {
            kernel->kernel[v][u] +=  value * preCalc->kernel->kernel[v][u];
        }
    }
#endif

    return;
}

// Generate an image of the solved kernel
static psKernel *solvedKernel(psKernel *kernel, // Kernel, to return
                              const pmSubtractionKernels *kernels, // Kernel basis functions
                              const psImage *polyValues, // Spatial polynomial values
                              bool normalise,            // Add normalisation?
                              bool wantDual // Want the dual (second) kernel?
                              )
{
    assert(kernels);
    assert(polyValues);

    int numKernels = kernels->num;      // Number of kernel basis functions
    int size = kernels->size;           // Kernel half-size
    if (!kernel) {
        kernel = psKernelAlloc(-size, size, -size, size);
    }
    psImageInit(kernel->image, 0.0);

    for (int i = 0; i < numKernels; i++) {
        double value = p_pmSubtractionSolutionCoeff(kernels, polyValues, i, wantDual); // Polynomial value
        if (wantDual) {
            // The model is built with the dual convolution terms added, so to produce zero residual the
            // equation results in negative coefficients which we must undo.
            value *= -1.0;
        }

        switch (kernels->type) {
          case PM_SUBTRACTION_KERNEL_POIS: {
              int u = kernels->u->data.S32[i]; // Offset in x
              int v = kernels->v->data.S32[i]; // Offset in y
              kernel->kernel[v][u] += value;
              kernel->kernel[0][0] -= value;
              break;
          }
          /* SPAM and FRIES use the same method */
          case PM_SUBTRACTION_KERNEL_SPAM:
          case PM_SUBTRACTION_KERNEL_FRIES: {
              int uStart = kernels->u->data.S32[i];
              int uStop = kernels->uStop->data.S32[i];
              int vStart = kernels->v->data.S32[i];
              int vStop = kernels->vStop->data.S32[i];

              // Normalising sum of kernel component to unity
              float norm = 1.0 / (float)((uStop - uStart + 1) * (vStop - vStart + 1));

              for (int v = vStart; v <= vStop; v++) {
                  for (int u = uStart; u <= uStop; u++) {
                      kernel->kernel[v][u] += norm * value;
                      kernel->kernel[0][0] -= value;
                  }
              }
              break;
          }
          case PM_SUBTRACTION_KERNEL_GUNK: {
              if (i < kernels->inner) {
                  solvedKernelPreCalc(kernel, kernels, value, i);
              } else {
                  // Using delta function
                  int u = kernels->u->data.S32[i]; // Offset in x
                  int v = kernels->v->data.S32[i]; // Offset in y
                  kernel->kernel[v][u] += value;
                  kernel->kernel[0][0] -= value;
              }
              break;
          }
	case PM_SUBTRACTION_KERNEL_SIMPLE: {
              solvedKernelPreCalc(kernel, kernels, 1.0, i);
              break;
	}	  
	  
          case PM_SUBTRACTION_KERNEL_ISIS:
          case PM_SUBTRACTION_KERNEL_ISIS_RADIAL:
          case PM_SUBTRACTION_KERNEL_HERM:
          case PM_SUBTRACTION_KERNEL_DECONV_HERM: {
              solvedKernelPreCalc(kernel, kernels, value, i);
              break;
          }
          case PM_SUBTRACTION_KERNEL_RINGS: {
              pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[i]; // Precalculated kernels
              int num = preCalc->uCoords->n;     // Number of pixels

              for (int j = 0; j < num; j++) {
                  int u = preCalc->uCoords->data.S32[j];
                  int v = preCalc->vCoords->data.S32[j]; // Kernel coordinates
                  kernel->kernel[v][u] += preCalc->poly->data.F32[j] * value;
              }
              // Photometric scaling is built into the kernel --- no subtraction!
              break;
          }
          default:
            psAbort("Should never get here.");
        }
    }

    /* we have three possible implementations for handling flux conservation and photometric scaling.
       
       1) the original implementation of Alard-Lupton subtracted the 0th kernel gaussian from
          the rest of the kernels.  the non-zero integral of the 0th kernel allows for some
          flux re-scaling if needed.

       2) the original ppSub implementation subtracted a delta function from the kernels.  this
          seems to make the solution sensitive to noise terms.

       3) accept the measured flux normalization and make all kernels have zero integral, but
          use a gaussian to zero the flux.  

	  *** the operation below is only needed if we use option (2) ***
    */

    if (normalise) {
        // Put in the normalisation component
        kernel->kernel[0][0] += (wantDual ? 1.0 : p_pmSubtractionSolutionNorm(kernels));
    }
    return kernel;
}

// Subtract the (0,0) element to preserve photometric scaling
static void convolveSub(psKernel *convolved, // Convolved image
                        const psKernel *image, // Image being convolved
                        int footprint  // Size of region of interest
                        )
{
    // Can't use psBinaryOp because the images are of different size
    for (int y = -footprint; y <= footprint; y++) {
        for (int x = -footprint; x <= footprint; x++) {
            convolved->kernel[y][x] -= image->kernel[y][x];
        }
    }
    return;
}

// Generate the convolution given some offset
static psKernel *convolveOffset(const psKernel *image, // Image to convolve (a kernel for convenience)
                                int u, int v, // Offset to apply
                                int footprint // Size of region of interest
                                )
{
    psKernel *convolved = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Convolved image
    for (int y = -footprint; y <= footprint; y++) {
        for (int x = -footprint; x <= footprint; x++) {
            convolved->kernel[y][x] = image->kernel[y - v][x - u];
        }
    }
    return convolved;
}


// Convolve an image using FFT
static void convolveFFT(psImage *target,// Place the result in here
                        psImage *image, // Image to convolve
                        psImage *mask, // Mask image
                        psImageMaskType maskVal, // Value to mask
                        const psKernel *kernel, // Kernel by which to convolve
                        psRegion region,// Region of interest
                        float background, // Background to add
                        int size        // Size of (square) kernel
                        )
{
    psRegion border = psRegionSet(region.x0 - size, region.x1 + size,
                                  region.y0 - size, region.y1 + size); // Add a border

    psImage *subImage = image ? psImageSubset(image, border) : NULL; // Subimage to convolve
    psImage *subMask = mask ? psImageSubset(mask, border) : NULL; // Subimage mask

# if (FFT_WINDOW)
    psImage *convolved = psImageConvolveFFTwithWindow(NULL, subImage, subMask, maskVal, kernel); // Convolution
# else
    psImage *convolved = psImageConvolveFFT(NULL, subImage, subMask, maskVal, kernel); // Convolution
# endif

    psFree(subImage);
    psFree(subMask);

    // Now, we have to stick it in where it belongs
    int xMin = region.x0, xMax = region.x1, yMin = region.y0, yMax = region.y1; // Bounds of region
    if (background != 0.0) {
        for (int yTarget = yMin, ySource = size; yTarget < yMax; yTarget++, ySource++) {
            for (int xTarget = xMin, xSource = size; xTarget < xMax; xTarget++, xSource++) {
                target->data.F32[yTarget][xTarget] = convolved->data.F32[ySource][xSource] + background;
            }
        }
    } else {
        int numBytes = (xMax - xMin) * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes to copy
        for (int yTarget = yMin, ySource = size; yTarget < yMax; yTarget++, ySource++) {
            memcpy(&target->data.F32[yTarget][xMin], &convolved->data.F32[ySource][size], numBytes);
        }
    }
    psFree(convolved);

    return;
}


// Convolve an image using FFT
static void convolveVarianceFFT(psImage *target,// Place the result in here
                                psImage *variance, // Variance map to convolve
                                psImage *kernelErr, // Kernel error image
                                psImage *mask, // Mask image
                                psImageMaskType maskVal, // Value to mask
                                const psKernel *kernel, // Kernel by which to convolve
                                psRegion region,// Region of interest
                                int size        // Size of (square) kernel
                                )
{
    psRegion border = psRegionSet(region.x0 - size, region.x1 + size,
                                  region.y0 - size, region.y1 + size); // Add a border

    psImage *subVariance = variance ? psImageSubset(variance, border) : NULL; // Variance map
    psImage *subKE = kernelErr ? psImageSubset(kernelErr, border) : NULL; // Kernel error image
    psImage *subMask = mask ? psImageSubset(mask, border) : NULL; // Mask

    // XXX Can trim this a little by combining the convolution: only have to take the FFT of the kernel once
# if (FFT_WINDOW)
    psImage *convVariance = psImageConvolveFFTwithWindow(NULL, subVariance, subMask, maskVal, kernel); // Convolved variance
    psImage *convKE = subKE ? psImageConvolveFFTwithWindow(NULL, subKE, subMask, maskVal, kernel) : NULL; // Conv KE
# else
    psImage *convVariance = psImageConvolveFFT(NULL, subVariance, subMask, maskVal, kernel); // Convolved variance
    psImage *convKE = subKE ? psImageConvolveFFT(NULL, subKE, subMask, maskVal, kernel) : NULL; // Conv KE
# endif

    psFree(subVariance);
    psFree(subKE);
    psFree(subMask);

    // Now, we have to stick it in where it belongs
    int xMin = region.x0, xMax = region.x1, yMin = region.y0, yMax = region.y1; // Bounds of region
    if (convKE) {
        for (int yTarget = yMin, ySource = size; yTarget < yMax; yTarget++, ySource++) {
            for (int xTarget = xMin, xSource = size; xTarget < xMax; xTarget++, xSource++) {
                target->data.F32[yTarget][xTarget] = convVariance->data.F32[ySource][xSource] +
                    convKE->data.F32[ySource][xSource];
            }
        }
    } else {
        int numBytes = (xMax - xMin) * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes to copy
        for (int yTarget = yMin, ySource = size; yTarget < yMax; yTarget++, ySource++) {
            memcpy(&target->data.F32[yTarget][xMin], &convVariance->data.F32[ySource][size], numBytes);
        }
    }

    psFree(convVariance);
    psFree(convKE);

    return;
}


// Convolve an image directly
static void convolveDirect(psImage *target, // Put the result here
                           const psImage *image, // Image to convolve
                           const psKernel *kernel, // Kernel by which to convolve
                           psRegion region,// Region of interest
                           float background, // Background to add
                           int size        // Size of (square) kernel
                           )
{
    for (int y = region.y0; y < region.y1; y++) {
        for (int x = region.x0; x < region.x1; x++) {
            target->data.F32[y][x] = background;
            for (int v = -size; v <= size; v++) {
                for (int u = -size; u <= size; u++) {
                    target->data.F32[y][x] += kernel->kernel[v][u] * image->data.F32[y - v][x - u];
                }
            }
        }
    }
    return;
}

// Convolve a region of an image
static inline void convolveRegion(psImage *convImage, // Convolved image (output)
                                  psImage *convVariance, // Convolved variance map (output), or NULL
                                  psImage *convMask, // Convolve mask (output), or NULL
                                  psKernel **kernelImage, // Convolution kernel for the image
                                  psKernel **kernelVariance, // Convolution kernel for the variance map, or NULL
                                  psImage *image, // Image to convolve
                                  psImage *variance, // Variance map to convolve, or NULL
                                  const psKernel *covar,               // Covariance, or NULL
                                  psImage *kernelErr, // Kernel error image, or NULL
                                  psImage *subMask, // Subtraction mask
                                  const pmSubtractionKernels *kernels, // Kernels
                                  const psImage *polyValues, // Polynomial values
                                  float background, // Background value to apply
                                  psRegion region, // Region to convolve
                                  psImageMaskType maskBad, // Value to give bad pixels
                                  psImageMaskType maskPoor, // Value to give poor pixels
                                  float poorFrac, // Fraction for "poor"
                                  bool useFFT,  // Use FFT to convolve?
                                  bool wantDual // Want the dual convolution?
    )
{
    *kernelImage = solvedKernel(*kernelImage, kernels, polyValues, true, wantDual);
    if (variance || subMask) {
        *kernelVariance = varianceKernel(*kernelVariance, *kernelImage);
    }

    psImageMaskType subBad;                  // Bad pixels in subtraction mask
    psImageMaskType subConvBad;              // Bad pixels in subtraction mask when convolving
    psImageMaskType subConvPoor;             // Poor pixels in subtraction mask when convolving
    if (kernels->mode == PM_SUBTRACTION_MODE_1 || (kernels->mode == PM_SUBTRACTION_MODE_DUAL && !wantDual)) {
        subBad = PM_SUBTRACTION_MASK_BAD_1;
        subConvBad = PM_SUBTRACTION_MASK_CONVOLVE_BAD_1;
        subConvPoor = PM_SUBTRACTION_MASK_CONVOLVE_1;
    } else {
        subBad = PM_SUBTRACTION_MASK_BAD_2;
        subConvBad = PM_SUBTRACTION_MASK_CONVOLVE_BAD_2;
        subConvPoor = PM_SUBTRACTION_MASK_CONVOLVE_2;
    }

    // Convolve the image and variance
    if (useFFT) {
        // Use Fast Fourier Transform to do the convolution
        // This provides a big speed-up for large kernels
        convolveFFT(convImage, image, subMask, subBad, *kernelImage, region, background, kernels->size);
        if (variance) {
            convolveVarianceFFT(convVariance, variance, kernelErr, subMask, subBad, *kernelVariance,
                                region, kernels->size);
        }
    } else {
        // XXX Direct convolution doesn't account for bad pixels yet
        convolveDirect(convImage, image, *kernelImage, region, background, kernels->size);
        if (variance) {
            convolveDirect(convVariance, variance, *kernelVariance, region, 0.0, kernels->size);
        }
    }

    if (variance && covar) {
        // Apply covariance factor to variance map, to allow for spatial variation
        float factor = psImageCovarianceCalculateFactor(*kernelImage, covar); // Factor to apply
        for (int y = region.y0; y < region.y1; y++) {
            for (int x = region.x0; x < region.x1; x++) {
                convVariance->data.F32[y][x] *= factor;
            }
        }
    }

    // Convolve the mask for bad/poor pixels
    if (subMask && convMask) {
        int box = p_pmSubtractionBadRadius(*kernelImage, kernels, polyValues,
                                           wantDual, poorFrac); // Size of bad box
        psAssert(box >= 0, "Bad radius must be >= 0");

        int colMin = region.x0, colMax = region.x1, rowMin = region.y0, rowMax = region.y1; // Bounds
        psImage *convolved = NULL; // Convolved subtraction mask
        if (box > 0) {
            psRegion maskRegion = psRegionSet(colMin - box, colMax + box,
                                              rowMin - box, rowMax + box); // Region to convolve
            psImage *image = subMask ? psImageSubset(subMask, maskRegion) : NULL; // Mask to convolve
            convolved = psImageConvolveMask(NULL, image, subBad, subConvBad, -box, box, -box, box);
            psFree(image);
        } else {
            convolved = psImageSubset(subMask, region);
        }

        psAssert(convolved->numCols - 2 * box == colMax - colMin, "Bad number of columns");
        psAssert(convolved->numRows - 2 * box == rowMax - rowMin, "Bad number of rows");

        for (int yTarget = rowMin, ySource = box; yTarget < rowMax; yTarget++, ySource++) {
            // Dereference images
            psImageMaskType *target = &convMask->data.PS_TYPE_IMAGE_MASK_DATA[yTarget][colMin]; // Target values
            psImageMaskType *source = &convolved->data.PS_TYPE_IMAGE_MASK_DATA[ySource][box]; // Source values
            for (int xTarget = colMin; xTarget < colMax; xTarget++, target++, source++) {
                if (*source & subConvBad) {
                    *target |= maskBad;
                } else if (*source & subConvPoor) {
                    *target &= ~maskBad;
                    *target |= maskPoor;
                } else {
                    *target &= ~maskBad & ~maskPoor;
                }
            }
        }

        psFree(convolved);
    }

    return;
}

#ifdef USE_KERNEL_ERR
// Generate an image that can be used to track systematic errors in the kernel
static psImage *subtractionKernelErrImage(const psImage *image, // Image from which to make kernel error image
                                          float kernelError // Relative systematic error in kernel
    )
{
    if (!isfinite(kernelError) || kernelError == 0.0) {
        return NULL;
    }

    int numCols = image->numCols, numRows = image->numRows; // Size of image
    psImage *kernelErr = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Kernel error image

    float kernelError2 = PS_SQR(kernelError); // Square of the kernel error
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            kernelErr->data.F32[y][x] = PS_SQR(image->data.F32[y][x]) * kernelError2;
        }
    }

    return kernelErr;
}
#endif

// Convolve a stamp using a pre-calculated kernel basis function
static psKernel *convolveStampPreCalc(const psKernel *image, // Image to convolve
                                      const pmSubtractionKernels *kernels, // Kernel basis functions
                                      int index,                            // Index of basis function of interest
                                      int footprint                         // Half-size of stamp
    )
{
    pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[index]; // Precalculated data
#if 0
    // Convolving using separable convolution
    int size = kernels->size;     // Size of kernel

    // Convolve in x
    // Need to convolve a bit more than the footprint, for the y convolution
    int yMin = -size - footprint, yMax = size + footprint; // Range for y
    psKernel *temp = psKernelAlloc(yMin, yMax,
                                   -footprint, footprint); // Temporary convolution; NOTE: wrong way!
    for (int y = yMin; y <= yMax; y++) {
        for (int x = -footprint; x <= footprint; x++) {
            float value = 0.0;    // Value of convolved pixel
            int uMin = x - size, uMax = x + size; // Range for u
            psF32 *xKernelData = &preCalc->xKernel->data.F32[xKernel->n - 1]; // Kernel values
            psF32 *imageData = &image->kernel[y][uMin]; // Image values
            for (int u = uMin; u <= uMax; u++, xKernelData--, imageData++) {
                value += *xKernelData * *imageData;
            }
            temp->kernel[x][y] = value; // NOTE: putting in wrong way!
        }
    }

    // Convolve in y
    psKernel *convolved = psKernelAlloc(-footprint, footprint, -footprint, footprint);// Convolved image
    for (int x = -footprint; x <= footprint; x++) {
        for (int y = -footprint; y <= footprint; y++) {
            float value = 0.0;    // Value of convolved pixel
            int vMin = y - size, vMax = y + size; // Range for v
            psF32 *yKernelData = &preCalc->yKernel->data.F32[yKernel->n - 1]; // Kernel values
            psF32 *imageData = &temp->kernel[x][vMin]; // Image values; NOTE: wrong way!
            for (int v = vMin; v <= vMax; v++, yKernelData--, imageData++) {
                value += *yKernelData * *imageData;
            }
            convolved->kernel[y][x] = value;
        }
    }
    psFree(temp);

    // Photometric scaling for even kernels only
    if (kernels->u->data.S32[index] % 2 == 0 && kernels->v->data.S32[index] % 2 == 0) {
        convolveSub(convolved, image, footprint);
    }
    return convolved;
#else
    // Convolving using precalculated kernel
    return p_pmSubtractionConvolveStampPrecalc(image, preCalc->kernel);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semi-public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

psKernel *p_pmSubtractionConvolveStampPrecalc(const psKernel *image, const psKernel *kernel)
{
    PS_ASSERT_KERNEL_NON_NULL(image, NULL);
    PS_ASSERT_KERNEL_NON_NULL(kernel, NULL);

# if (FFT_WINDOW)
    psImage *conv = psImageConvolveFFTwithWindow(NULL, image->image, NULL, 0, kernel); // Convolved image
# else
    psImage *conv = psImageConvolveFFT(NULL, image->image, NULL, 0, kernel); // Convolved image
# endif

    // note: do not attempt to renormalize kernels here: cannot have different stars with
    // different kernel ratios

    int x0 = - image->xMin, y0 = - image->yMin; // Position of centre of convolved image
    psKernel *convolved = psKernelAllocFromImage(conv, x0, y0); // Kernel version

    // pmSubtractionVisualShowSubtraction(image->image, kernel->image, conv);

    psFree(conv);
    return convolved;
}

int p_pmSubtractionBadRadius(psKernel *preKernel, const pmSubtractionKernels *kernels,
                             const psImage *polyValues, bool wantDual, float poorFrac)
{
    psKernel *kernel;                   // Kernel to use
    if (!preKernel) {
        kernel = solvedKernel(NULL, kernels, polyValues, true, wantDual);
    } else {
        kernel = psMemIncrRefCounter(preKernel);
    }
    PS_ASSERT_IMAGE_NON_NULL(polyValues, -1);

    int xMin = kernel->xMin, xMax = kernel->xMax, yMin = kernel->yMin, yMax = kernel->yMax; // Bounds

    // Determine the threshold between bad and poor
    double sumKernel2 = 0.0;            // Sum of the kernel-squared
    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            sumKernel2 += PS_SQR(kernel->kernel[y][x]);
        }
    }
    float threshold = sumKernel2 * poorFrac; // Threshold between poor and bad

    // Get bounds of threshold region
    // Start with the entire kernel, and keep reducing the size of the box until it sum goes above threshold
    int box = kernels->size;            // Size of box with bad pixels
    for (double sumBox = 0.0; sumBox < threshold && box > 0; box--) {
        for (int x = -box; x <= box; x++) {
            sumBox += PS_SQR(kernel->kernel[-box][x]) + PS_SQR(kernel->kernel[box][x]);
        }
        for (int y = -box + 1; y <= box - 1; y++) {
            // Note: not doing corners
            sumBox += PS_SQR(kernel->kernel[y][-box]) + PS_SQR(kernel->kernel[y][box]);
        }
    }

    psFree(kernel);

    return box;
}

void p_pmSubtractionPolynomialNormCoords(float *xOut, float *yOut, float xIn, float yIn,
                                         int xMin, int xMax, int yMin, int yMax)
{
    float xNormSize = xMax - xMin, yNormSize = yMax - yMin; // Size to use for normalisation
    *xOut = 2.0 * (float)(xIn - xMin - xNormSize/2.0) / xNormSize;
    *yOut = 2.0 * (float)(yIn - yMin - yNormSize/2.0) / yNormSize;
    return;
}

psImage *p_pmSubtractionPolynomialFromCoords(psImage *output, const pmSubtractionKernels *kernels,
                                             int x, int y)
{
    assert(kernels);

    float xNorm, yNorm;                 // Normalised coordinates
    p_pmSubtractionPolynomialNormCoords(&xNorm, &yNorm, x, y,
                                        kernels->xMin, kernels->xMax, kernels->yMin, kernels->yMax);
    return p_pmSubtractionPolynomial(output, kernels->spatialOrder, xNorm, yNorm);
}

psImage *p_pmSubtractionPolynomial(psImage *output, int spatialOrder, float x, float y)
{
    assert(spatialOrder >= 0);
    assert(x >= -1 && x <= 1);
    assert(y >= -1 && y <= 1);

    output = psImageRecycle(output, spatialOrder + 1, spatialOrder + 1, PS_TYPE_F64);
    output->data.F64[0][0] = 1.0;

    double value = 1.0;
    for (int i = 1; i <= spatialOrder; i++) {
        value *= x;
        output->data.F64[0][i] = value;
    }

    value = 1.0;
    for (int j = 1; j <= spatialOrder; j++) {
        value *= y;
        output->data.F64[j][0] = value;
    }

    for (int j = 1; j <= spatialOrder; j++) {
        for (int i = 1; i <= spatialOrder - j; i++) {
            output->data.F64[j][i] = output->data.F64[j][0] * output->data.F64[0][i];
        }
    }

    return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Convolve a stamp by a single kernel basis function
static psKernel *convolveStampSingle(const pmSubtractionKernels *kernels, // Kernel basis functions
                                     int index, // Kernel basis function index
                                     const psKernel *image, // Image to convolve (a kernel for convenience)
                                     int footprint // Size of region of interest
    )
{
    switch (kernels->type) {
      case PM_SUBTRACTION_KERNEL_POIS: {
          int u = kernels->u->data.S32[index]; // Offset in x
          int v = kernels->v->data.S32[index]; // Offset in y
          psKernel *convolved = convolveOffset(image, u, v, footprint); // Convolved image
          convolveSub(convolved, image, footprint);
          return convolved;
      }
        // Method for SPAM and FRIES is the same
      case PM_SUBTRACTION_KERNEL_SPAM:
      case PM_SUBTRACTION_KERNEL_FRIES: {
          psKernel *convolved = psKernelAlloc(-footprint, footprint,
                                              -footprint, footprint); // Convolved image
          int uStart = kernels->u->data.S32[index];
          int uStop = kernels->uStop->data.S32[index];
          int vStart = kernels->v->data.S32[index];
          int vStop = kernels->vStop->data.S32[index];
          float norm = 1.0 / (uStop - uStart + 1) * (vStop - vStart + 1); // Normalisation
          for (int y = -footprint; y <= footprint; y++) {
              for (int x = -footprint; x <= footprint; x++) {
                  double sum = 0.0;
                  for (int v = vStart; v <= vStop; v++) {
                      for (int u = uStart; u <= uStop; u++) {
                          sum += image->kernel[y - v][x - u];
                      }
                  }
                  convolved->kernel[y][x] = norm * sum;
              }
          }
          convolveSub(convolved, image, footprint);
          return convolved;
      }
      case PM_SUBTRACTION_KERNEL_GUNK: {
          if (index < kernels->inner) {
              // Photometric scaling is already built in to the precalculated kernel
              return convolveStampPreCalc(image, kernels, index, footprint);
          }
          // Using delta function
          int u = kernels->u->data.S32[index]; // Offset in x
          int v = kernels->v->data.S32[index]; // Offset in y
          psKernel *convolved = convolveOffset(image, u, v, footprint); // Convolved image
          convolveSub(convolved, image, footprint);
          return convolved;
      }
      case PM_SUBTRACTION_KERNEL_ISIS:
      case PM_SUBTRACTION_KERNEL_ISIS_RADIAL:
      case PM_SUBTRACTION_KERNEL_HERM:
      case PM_SUBTRACTION_KERNEL_DECONV_HERM: {
            return convolveStampPreCalc(image, kernels, index, footprint);
        }
      case PM_SUBTRACTION_KERNEL_RINGS: {
          psKernel *convolved = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Convolved image
          pmSubtractionKernelPreCalc *preCalc = kernels->preCalc->data[index]; // Precalculated data

          int num = preCalc->uCoords->n;         // Number of pixels
          psS32 *uData = preCalc->uCoords->data.S32; // Dereference v coordinate
          psS32 *vData = preCalc->vCoords->data.S32; // Dereference u coordinate
          psF32 *polyData = preCalc->poly->data.F32; // Dereference polynomial values
          psF32 **imageData = image->kernel;  // Dereference image
          psF32 **convData = convolved->kernel; // Dereference convolved image
          for (int y = -footprint; y <= footprint; y++) {
              for (int x = -footprint; x <= footprint; x++) {
                  double sum = 0.0;             // Accumulated sum from convolution
                  for (int j = 0; j < num; j++) {
                      int u = uData[j], v = vData[j]; // Kernel coordinates
                      sum += imageData[y - v][x - u] * polyData[j];
                  }
                  convData[y][x] = sum;
                  // Photometric scaling is built into the kernel --- no subtraction!
              }
          }
          return convolved;
      }
      default:
        psAbort("Should never get here.");
    }
    return NULL;
}

// Convolve the stamp by each of the kernel basis functions
static psArray *convolveStamp(psArray *convolutions, // The convolutions
                              const psKernel *image, // Image to convolve
                              const pmSubtractionKernels *kernels, // Kernel basis functions
                              int footprint // Stamp half-size
    )
{
    assert(image);
    assert(kernels);
    assert(footprint >= 0);

    if (convolutions) {
        return convolutions;
    }

    int numKernels = kernels->num;      // Number of kernels
    convolutions = psArrayAlloc(numKernels);

    for (int i = 0; i < numKernels; i++) {
        convolutions->data[i] = convolveStampSingle(kernels, i, image, footprint);
    }

    return convolutions;
}


bool pmSubtractionConvolveStampThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmSubtractionStamp *stamp = job->args->data[0]; // List of stamps
    pmSubtractionKernels *kernels = job->args->data[1]; // Kernels
    int footprint = PS_SCALAR_VALUE(job->args->data[2], S32); // Stamp index -- MEH - it is?

    return pmSubtractionConvolveStamp(stamp, kernels, footprint);
}

bool pmSubtractionConvolveStamp (pmSubtractionStamp *stamp, pmSubtractionKernels *kernels, int footprint)
{
    PS_ASSERT_PTR_NON_NULL(stamp, false);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PS_ASSERT_INT_NONNEGATIVE(footprint, false);

    if (stamp->status != PM_SUBTRACTION_STAMP_CALCULATE) {
        psError(PM_ERR_PROG, true, "Stamp not marked for calculation.");
        return false;
    }

    switch (kernels->mode) {
      case PM_SUBTRACTION_MODE_1:
        stamp->convolutions1 = convolveStamp(stamp->convolutions1, stamp->image1, kernels, footprint);
        break;
      case PM_SUBTRACTION_MODE_2:
        stamp->convolutions2 = convolveStamp(stamp->convolutions2, stamp->image2, kernels, footprint);
        break;
      case PM_SUBTRACTION_MODE_UNSURE:
      case PM_SUBTRACTION_MODE_DUAL:
        stamp->convolutions1 = convolveStamp(stamp->convolutions1, stamp->image1, kernels, footprint);
        stamp->convolutions2 = convolveStamp(stamp->convolutions2, stamp->image2, kernels, footprint);
	if (!pmSubtractionKernelPenaltiesStamp(stamp, kernels)) {
	    psAbort("failure in penalties");
	}
        break;
      default:
        psAbort("Unsupported subtraction mode: %x", kernels->mode);
    }

#ifdef TESTING
    //MEH - index conflict or changed in past?
    for (int j = 0; j < kernels->num; j++) {
        if (stamp->convolutions1) {
            psString convName = NULL;
            //psStringAppend(&convName, "conv1_%03d_%03d.fits", index, j);
            psStringAppend(&convName, "conv1_xxx_%03d.fits", j);
	    psFits *fits = psFitsOpen(convName, "w");
            psFree(convName);
            psKernel *conv = stamp->convolutions1->data[j];
            psFitsWriteImage(fits, NULL, conv->image, 0, NULL);
            psFitsClose(fits);
        }

        if (stamp->convolutions2) {
            psString convName = NULL;
            //psStringAppend(&convName, "conv2_%03d_%03d.fits", index, j);
            psStringAppend(&convName, "conv2_xxx_%03d.fits", j);
	    psFits *fits = psFitsOpen(convName, "w");
            psFree(convName);
            psKernel *conv = stamp->convolutions2->data[j];
            psFitsWriteImage(fits, NULL, conv->image, 0, NULL);
            psFitsClose(fits);
        }
    }
#endif

    return true;
}

bool pmSubtractionConvolveStamps(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels) 
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);

    psTimerStart("pmSubtractionConvolveStamps");

    int footprint = stamps->footprint;  // Half-size of stamps

    // We iterate over each stamp and generate the convolution if needed.  We do NOT need the
    // convolution if (a) it has already been calculated or (b) the stamp is not available for
    // use (available = USED or CALCULATE)
    
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest

        bool keep = false;
	keep |= (stamp->status == PM_SUBTRACTION_STAMP_USED);
	keep |= (stamp->status == PM_SUBTRACTION_STAMP_CALCULATE);
	if (!keep) continue;

	bool haveConvolutions = false;
	if (kernels->mode == PM_SUBTRACTION_MODE_1) {
	    haveConvolutions = (stamp->convolutions1 != NULL);
	}
	if (kernels->mode == PM_SUBTRACTION_MODE_2) {
	    haveConvolutions = (stamp->convolutions2 != NULL);
	}
	if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
	    haveConvolutions = (stamp->convolutions1 != NULL) && (stamp->convolutions2 != NULL);
	}
        if (haveConvolutions) {
            continue;
        }

        if (pmSubtractionThreaded()) {
            psThreadJob *job = psThreadJobAlloc("PSMODULES_SUBTRACTION_CONVOLVE_STAMP");
            psArrayAdd(job->args, 1, stamp);
            psArrayAdd(job->args, 1, kernels);
            PS_ARRAY_ADD_SCALAR(job->args, footprint, PS_TYPE_S32);
            if (!psThreadJobAddPending(job)) {
                return false;
            }
        } else {
            pmSubtractionConvolveStamp(stamp, kernels, footprint);
        }
    }
    if (!psThreadPoolWait(true, true)) {
        psError(psErrorCodeLast(), false, "Error waiting for threads.");
        return false;
    }
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Convolve stamps: %f sec", psTimerClear("pmSubtractionConvolveStamps"));
    return true;
}

int pmSubtractionRejectStamps(pmSubtractionKernels *kernels, pmSubtractionStampList *stamps,
                              pmSubtractionQuality *match, psImage *subMask, float sigmaRej)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, -1);
    PS_ASSERT_IMAGE_NON_EMPTY(subMask, -1);
    PS_ASSERT_IMAGE_TYPE(subMask, PS_TYPE_IMAGE_MASK, -1);

    // Comment from PAP (r18287): I used to measure the rms deviation about zero, and use that as the
    // sigma against which to clip, but the distribution is actually something like a chi^2 or
    // Student's t, both of which become Gaussian-like with large N.  Therefore, let's just
    // treat this as a Gaussian distribution.

    // Comment from EAM (r29777): The residual distribution is only chisq-like if the model is
    // a good fit to the data.  In the (likely) case that there is a systematic difference
    // between the model and the data, the squared-residual distribution grows quadratically
    // with increasing flux: the systematic residual flux is a constant factor times the source
    // flux; the squared-residual is then of the form (k0 + k1*flux)^2, where k0 comes from the
    // Gaussian distributed residual and k1*flux is the systematic residual error.

    // By rejecting sources with the largest squared-residuals, the rejection biases against
    // the brighter sources; in severe cases, this pushes the measurement to the weakest
    // sources with the most noise.  To account for this, let's fit a 2nd order polynomial to
    // the distribution of flux vs squared-residual, subtract that fit, and reject sources
    // which are significantly deviant from that distribution.

    // if we only have 3 or fewer stars, we have to accept them all
    if (match->nGood < 3) {
	kernels->mean = NAN;
	kernels->rms = NAN;
	kernels->numStamps = match->nGood;
	
	psLogMsg("psModules.imcombine", PS_LOG_INFO, "only %d stars, keeping them all (good luck!)",  kernels->numStamps);
	return 0;
    }

    kernels->mean = NAN;
    kernels->rms = NAN;
    kernels->numStamps = -1;

    psTrace("psModules.imcombine", 1, "Number of good stamps: %d\n", match->nGood);

    // the chisq & flux vectors are calculated by pmSubtractionCalculateChisqAndMoments

    // use 3hi/3lo sigma clipping on the chisq fit
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
    stats->clipSigma = 5.0;
    stats->clipIter = 2;
    psPolynomial1D *model = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 2);

#ifdef USE_LOGFIT_REJECT
    // CZW: Since flux and chisq can range over many orders of magnitude, use a log-log fit to calculate the
    // RMS scatter.  This should be more robust against outliers that can impact the fit quality.
    psVector *logchisq = psVectorAlloc(match->chisq->n,PS_TYPE_F32);
    psVector *logflux  = psVectorAlloc(match->fluxes->n,PS_TYPE_F32);
    for (int qq = 0; qq < match->chisq->n; qq++) {
      if ((match->chisq->data.F32[qq] > 0)&&
	  (match->fluxes->data.F32[qq] > 0)) {
	logchisq->data.F32[qq] = log10(match->chisq->data.F32[qq]);
	logflux->data.F32[qq]  = log10(match->fluxes->data.F32[qq]);
      }
      else {
	// Ignore negative values.  Maybe we could do an offset or something, but not today.
	match->stampMask->data.PS_TYPE_VECTOR_MASK_DATA[qq] |= PM_SUBTRACTION_STAMP_REJECTED;
      }	
      //      if (!(match->stampMask->data.PS_TYPE_VECTOR_MASK_DATA[qq] & 0xff)) {
      //	psLogMsg("psModules.imcombine", PS_LOG_INFO, "RRRRRR: %d %g %g %g %g\n",
      //		 qq,match->chisq->data.F32[qq],match->fluxes->data.F32[qq],
      //		 0.0,0.0);
      // }

    }

    if (0) { 
      FILE *f = fopen ("vector.dat", "w");
      psAssert (f, "should not fail to open test file");
      
      for (int qq = 0; qq < match->chisq->n; qq++) {
	fprintf (f, "%f %f : %f %f : %d\n", 
		 match->chisq->data.F32[qq], match->fluxes->data.F32[qq], 
		 logchisq->data.F32[qq], logflux->data.F32[qq], 
		 match->stampMask->data.PS_TYPE_VECTOR_MASK_DATA[qq]);
      }
      fclose (f);
    }


    bool result = psVectorClipFitPolynomial1D(model, stats, match->stampMask, 0xff, logchisq, NULL, logflux);
    if (!result) {
	psError(PM_ERR_DATA, false, "Unable to measure statistics for deviations.");
        psFree(model);
        psFree(stats);
	return -1;
    }

    // However, since we've done a log-log fit, we can't rely on the statistics in the stats object
    // To accurately represent the distribution.  I've tried to massage them (as dlog10(X) = abs(1/(X log(10))) dX),
    // but ended up deciding to just manually calculate the residual, and then pass that to a robust stats object.
    psStats *residStats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    psVector *residData = psVectorAlloc(match->chisq->n,PS_TYPE_F32);
    int counter = 0;
    for (int qq = 0; qq < match->chisq->n; qq++) {
      if (!(match->stampMask->data.PS_TYPE_VECTOR_MASK_DATA[qq] & 0xff)) {
	double vv = pow(10,
			model->coeff[0] +
			model->coeff[1] * log10(match->fluxes->data.F32[qq]) +
			model->coeff[2] * pow(log10(match->fluxes->data.F32[qq]),2));
	residData->data.F32[qq] = match->chisq->data.F32[qq] - vv;
	counter++;
	//			psLogMsg("psModules.imcombine", PS_LOG_INFO, "SSSSS: %d %g %g %g %g\n",
	//			 qq,match->chisq->data.F32[qq],match->fluxes->data.F32[qq],
	//			 vv,residData->data.F32[qq]);
      }
    }
    psVectorStats(residStats,residData,NULL,match->stampMask,0xff);
    if (isnan(residStats->robustMedian)) {
	psError(PM_ERR_DATA, false, "Unable to measure statistics for deviations.");
        psFree(model);
        psFree(stats);
	psFree(logchisq);
	psFree(logflux);
	psFree(residStats);
	psFree(residData);

        return -1;
    }

    kernels->mean = residStats->robustMedian;
    kernels->rms =  residStats->robustStdev;
    kernels->numStamps = counter;

    psLogMsg ("pmPSFtry", 4, "chisq vs flux resid: %f +/- %f\n", residStats->robustMedian,residStats->robustStdev);
    psFree(logchisq);
    psFree(logflux);
    psFree(residStats);
    psFree(residData);

#else
    // CZW: Otherwise, use the original linear fit code.
    bool result = psVectorClipFitPolynomial1D(model, stats, match->stampMask, 0xff, match->chisq, NULL, match->fluxes);
    if (!result) {
	psError(PM_ERR_DATA, false, "Unable to measure statistics for deviations.");
        psFree(model);
        psFree(stats);
	return -1;
    }
    if (isnan(stats->sampleMean)) {
	psError(PM_ERR_DATA, false, "Unable to measure statistics for deviations.");
        psFree(model);
        psFree(stats);
        return -1;
    }
    kernels->mean = stats->sampleMean;
    kernels->rms =  stats->sampleStdev;
    kernels->numStamps = stats->clippedNvalues;
    psLogMsg ("pmPSFtry", 4, "chisq vs flux resid: %f +/- %f\n", stats->sampleMean, stats->sampleStdev);
#endif

    psLogMsg ("pmPSFtry", 4, "chisq vs flux model: %e + %e flux + %e flux^2\n", model->coeff[0], model->coeff[1], model->coeff[2]);
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Mean deviation from %d stamps: %lf +/- %lf",  kernels->numStamps, kernels->mean, kernels->rms);

    psString ds9name = NULL;            // Filename for ds9 region file
    static int ds9num = 0;              // File number for ds9 region file
    psStringAppend(&ds9name, "stamps_reject_%d.ds9", ds9num);
    FILE *ds9 = pmSubtractionStampsFile(stamps, ds9name, "rejected stamps");
    psFree(ds9name);
    ds9num++;

    int footprint = stamps->footprint;  // Half-size of stamp region of interest
    int numRejected = 0;                // Number of stamps rejected
    int numGood = 0;                    // Number of good stamps
    psString log = NULL;                // Log message

    // save DS9 region files for the stamps and mark for rejection and replacement
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
	if (stamp->status  != PM_SUBTRACTION_STAMP_USED) { continue; }
        if (match->stampMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            // Should we reject stars with low deviation?  Well, if this is really a Gaussian-like
            // distribution and they're low, then we have the right to ask why.  Isn't it suspicious that
            // they're anomalously low, compared to the rest of the population which (we hope) is indicative
            // of normality?  Besides, the standard deviation is going to be blown up by stars that didn't
            // subtract well, in which case very few (if any) stars will be legitimately rejected for being
            // low.
	    psTrace("psModules.imcombine", 3, "Rejecting stamp %d (%d,%d)\n", i,
		    (int)(stamp->x - 0.5), (int)(stamp->y - 0.5));
	    psStringAppend(&log, "Stamp %d (%d,%d): %f : %f : %f\n", 
			   i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5),
			   match->chisq->data.F32[i], match->fluxes->data.F32[i], match->chisq->data.F32[i] - psPolynomial1DEval(model, match->fluxes->data.F32[i])); 
	    numRejected++;
	    for (int y = stamp->y - footprint; y <= stamp->y + footprint; y++) {
		for (int x = stamp->x - footprint; x <= stamp->x + footprint; x++) {
		    subMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= PM_SUBTRACTION_MASK_REJ;
		}
	    }
	    pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "red");

	    // Set stamp for replacement
	    stamp->x = 0;
	    stamp->y = 0;
	    stamp->xNorm = NAN;
	    stamp->yNorm = NAN;
	    stamp->status = PM_SUBTRACTION_STAMP_REJECTED;
	    // Recalculate convolutions
	    psFree(stamp->convolutions1);
	    psFree(stamp->convolutions2);
	    stamp->convolutions1 = stamp->convolutions2 = NULL;
	    psFree(stamp->image1);
	    psFree(stamp->image2);
	    psFree(stamp->weight);
	    stamp->image1 = stamp->image2 = stamp->weight = NULL;
	    psFree(stamp->matrix);
	    stamp->matrix = NULL;
	    psFree(stamp->vector);
	    stamp->vector = NULL;
	} else {
	    numGood++;
	    pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "green");
        }
    }

    if (numRejected == 0) {
        psStringAppend(&log, "<none>\n");
    }
    psLogMsg("psModules.imcombine", PS_LOG_DETAIL, "%s", log);
    psFree(log);

    if (ds9) {
        fclose(ds9);
    }

    psFree(model);
    psFree(stats);

    if (numRejected > 0) {
        psLogMsg("psModules.imcombine", PS_LOG_INFO, "%d good stamps; %d rejected.\n", numGood, numRejected);
    } else {
        psLogMsg("psModules.imcombine", PS_LOG_INFO, "%d good stamps; 0 rejected.\n", numGood);
    }

    return numRejected;
}

psKernel *pmSubtractionKernel(const pmSubtractionKernels *kernels, float x, float y, bool wantDual)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(x, -1.0, 1.0, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(y, -1.0, 1.0, NULL);

    psImage *polyValues = p_pmSubtractionPolynomial(NULL, kernels->spatialOrder, x, y); // Solved polynomial
    psKernel *kernel = solvedKernel(NULL, kernels, polyValues, true, wantDual); // The appropriate kernel
    psFree(polyValues);

    return kernel;
}

// generate an image of the convolution kernel realized at the given coordinate
// if 'wantDual' is set, solution2 is supplied
psImage *pmSubtractionKernelImage(const pmSubtractionKernels *kernels, float x, float y, bool wantDual)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(x, -1.0, 1.0, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(y, -1.0, 1.0, NULL);

    psKernel *kernel = pmSubtractionKernel(kernels, x, y, wantDual); // Convolution kernel
    psImage *image = psMemIncrRefCounter(kernel->image); // Image of the kernel
    psFree(kernel);

    return image;
}


float pmSubtractionVarianceFactor(const pmSubtractionKernels *kernels, float x, float y, bool wantDual)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NAN);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NAN);
    PS_ASSERT_FLOAT_WITHIN_RANGE(x, -1.0, 1.0, NAN);
    PS_ASSERT_FLOAT_WITHIN_RANGE(y, -1.0, 1.0, NAN);

    // Precalulate polynomial values
    psImage *polyValues = p_pmSubtractionPolynomial(NULL, kernels->spatialOrder, x, y);

    psKernel *kernel = solvedKernel(NULL, kernels, polyValues, true, wantDual); // The appropriate kernel
    psFree(polyValues);

    double sumKernel2 = 0.0;            // Sum of the kernel squared
    double sumKernel = 0.0;             // Sum of the kernel
    for (int y = kernel->yMin; y <= kernel->yMax; y++) {
        for (int x = kernel->xMin; x <= kernel->xMax; x++) {
            sumKernel += kernel->kernel[y][x];
            sumKernel2 += PS_SQR(kernel->kernel[y][x]);
        }
    }

    psFree(kernel);

    return sumKernel2 / PS_SQR(sumKernel);
}

#if 1
psArray *pmSubtractionKernelSolutions(const pmSubtractionKernels *kernels, float x, float y, bool wantDual)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(x, -1.0, 1.0, NULL);
    PS_ASSERT_FLOAT_WITHIN_RANGE(y, -1.0, 1.0, NULL);

    psVector *solution = wantDual ? kernels->solution2 : kernels->solution1; // Solution of interest
    psVector *backup = psVectorCopy(NULL, solution, PS_TYPE_F64);  // Backup version

    int num = kernels->num;             // Number of kernel basis functions

    psImage *polyValues = p_pmSubtractionPolynomial(NULL, kernels->spatialOrder, x, y); // Solved polynomial
    psArray *images = psArrayAlloc(num + 1); // Images of each kernel to return

    // The whole kernel
    {
        psKernel *kernel = solvedKernel(NULL, kernels, polyValues, true, wantDual); // The appropriate kernel
        images->data[0] = psMemIncrRefCounter(kernel->image);
        psFree(kernel);
    }

    // The parts
    psVectorInit(solution, 0.0);
    for (int i = 0; i < num; i++) {
        solution->data.F64[i] = backup->data.F64[i];
        psKernel *kernel = solvedKernel(NULL, kernels, polyValues, false, wantDual); // The appropriate kernel
#if 0
        int size = kernels->size;
        double sum = 0.0;
        for (int v = -size; v <= size; v++) {
            for (int u = -size; u <= size; u++) {
                sum += kernel->kernel[v][u];
            }
        }
        fprintf(stderr, "Kernel %d: %lf\n", i, sum);
#endif
        images->data[i + 1] = psMemIncrRefCounter(kernel->image);
        psFree(kernel);
        solution->data.F64[i] = 0.0;
    }
    psFree(polyValues);
    psVectorCopy(solution, backup, PS_TYPE_F64);
    psFree(backup);

    return images;
}
#endif


// XXX Put kernelImage, kernelVariance and polyValues on thread-dependent data
static bool subtractionConvolvePatch(int numCols, int numRows, // Size of image
                                     int x0, int y0, // Offsets for image
                                     pmReadout *out1, pmReadout *out2, // Output readouts
                                     psImage *convMask, // Output convolved mask
                                     const pmReadout *ro1, const pmReadout *ro2, // Input readouts
                                     psImage *kernelErr1, psImage *kernelErr2, // Kernel error images
                                     psImage *subMask, // Input subtraction mask
                                     psImageMaskType maskBad, // Mask value to give bad pixels
                                     psImageMaskType maskPoor, // Mask value to give poor pixels
                                     float poorFrac, // Fraction for "poor"
                                     const psRegion *region, // Patch to convolve
                                     const pmSubtractionKernels *kernels, // Kernels
                                     bool doBG, // Add in background when convolving?
                                     bool useFFT // Use FFT to do the convolution?
    )
{
    int size = kernels->size;           // Half-size of kernel
    int xMin = region->x0, xMax = region->x1, yMin = region->y0, yMax = region->y1; // Bounds of patch

    psKernel *kernelImage = NULL;       // Kernel for the images
    psKernel *kernelVariance = NULL;      // Kernel for the variance maps

    // Only generate polynomial values every kernel footprint, since we have already assumed
    // (with the stamps) that it does not vary rapidly on this scale.
    psImage *polyValues = p_pmSubtractionPolynomialFromCoords(NULL, kernels, xMin + x0 + size + 1,
                                                              yMin + y0 + size + 1);        // Polynomial
    float background = doBG ? p_pmSubtractionSolutionBackground(kernels, polyValues) : 0.0; // Background term

    if (kernels->mode == PM_SUBTRACTION_MODE_1 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        convolveRegion(out1->image, out1->variance, out1->mask, &kernelImage, &kernelVariance,
                       ro1->image, ro1->variance, ro1->covariance, kernelErr1, subMask, kernels,
                       polyValues, background, *region, maskBad, maskPoor, poorFrac, useFFT, false);
    }
    if (kernels->mode == PM_SUBTRACTION_MODE_2 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        convolveRegion(out2->image, out2->variance, out2->mask, &kernelImage, &kernelVariance,
                       ro2->image, ro2->variance, ro2->covariance, kernelErr2, subMask, kernels,
                       polyValues, background, *region, maskBad, maskPoor, poorFrac, useFFT,
                       kernels->mode == PM_SUBTRACTION_MODE_DUAL);
    }

    psFree(kernelImage);
    psFree(kernelVariance);
    psFree(polyValues);

    if ((kernels->mode == PM_SUBTRACTION_MODE_1 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) && ro1->mask) {
        psImageMaskType **target = out1->mask->data.PS_TYPE_IMAGE_MASK_DATA; // Target mask
        psImageMaskType **source = ro1->mask->data.PS_TYPE_IMAGE_MASK_DATA; // Source mask

        for (int y = yMin; y < yMax; y++) {
            for (int x = xMin; x < xMax; x++) {
                target[y][x] |= source[y][x];
            }
        }
    }
    if ((kernels->mode == PM_SUBTRACTION_MODE_2 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) && ro2->mask) {
        psImageMaskType **target = out2->mask->data.PS_TYPE_IMAGE_MASK_DATA; // Target mask
        psImageMaskType **source = ro2->mask->data.PS_TYPE_IMAGE_MASK_DATA; // Source mask

        for (int y = yMin; y < yMax; y++) {
            for (int x = xMin; x < xMax; x++) {
                target[y][x] |= source[y][x];
            }
        }
    }

    return true;
}

bool pmSubtractionConvolveThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Arguments
    int numCols = PS_SCALAR_VALUE(args->data[0], S32); // Number of columns
    int numRows = PS_SCALAR_VALUE(args->data[1], S32); // Number of rows
    int x0 = PS_SCALAR_VALUE(args->data[2], S32); // Offset in x
    int y0 = PS_SCALAR_VALUE(args->data[3], S32); // Offset in x
    pmReadout *out1 = args->data[4];    // Output readout 1
    pmReadout *out2 = args->data[5];    // Output readout 2
    psImage *convMask = args->data[6];  // Output convolved mask
    const pmReadout *ro1 = args->data[7]; // Input readout 1
    const pmReadout *ro2 = args->data[8]; // Input readout 2
    psImage *kernelErr1 = args->data[9]; // Kernel error image 1
    psImage *kernelErr2 = args->data[10]; // Kernel error image 2
    psImage *subMask = args->data[11]; // Subtraction mask
    psImageMaskType maskBad = PS_SCALAR_VALUE(args->data[12], PS_TYPE_IMAGE_MASK_DATA); // Output mask value for bad pixels
    psImageMaskType maskPoor = PS_SCALAR_VALUE(args->data[13], PS_TYPE_IMAGE_MASK_DATA); // Output mask value for poor pixels
    float poorFrac = PS_SCALAR_VALUE(args->data[14], F32); // Fraction for "poor"
    const psRegion *region = args->data[15]; // Region to convolve
    const pmSubtractionKernels *kernels = args->data[16]; // Kernels
    bool doBG = PS_SCALAR_VALUE(args->data[17], U8); // Do background subtraction?
    bool useFFT = PS_SCALAR_VALUE(args->data[18], U8); // Use FFT for convolution?

    return subtractionConvolvePatch(numCols, numRows, x0, y0, out1, out2, convMask, ro1, ro2, kernelErr1,
                                    kernelErr2, subMask, maskBad, maskPoor, poorFrac, region, kernels,
                                    doBG, useFFT);
}

bool pmSubtractionConvolve(pmReadout *out1, pmReadout *out2, const pmReadout *ro1, const pmReadout *ro2,
                           psImage *subMask, int stride, psImageMaskType maskBad, psImageMaskType maskPoor,
                           float poorFrac, float kernelError, float covarFrac, const psRegion *region,
                           const pmSubtractionKernels *kernels, bool doBG, bool useFFT)
{
    int numCols = 0, numRows = 0;       // Image dimensions
    int x0 = 0, y0 = 0;                 // Image offset
    if (kernels->mode == PM_SUBTRACTION_MODE_1 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        PM_ASSERT_READOUT_NON_NULL(out1, false);
        PM_ASSERT_READOUT_NON_NULL(ro1, false);
        PM_ASSERT_READOUT_IMAGE(ro1, false);
        PM_ASSERT_READOUT_IMAGE(out1, false);
        numCols = ro1->image->numCols;
        numRows = ro1->image->numRows;
        x0 = ro1->col0;
        y0 = ro1->row0;
    }
    if (kernels->mode == PM_SUBTRACTION_MODE_2 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        PM_ASSERT_READOUT_NON_NULL(out2, false);
        PM_ASSERT_READOUT_NON_NULL(ro2, false);
        PM_ASSERT_READOUT_IMAGE(ro2, false);
        PM_ASSERT_READOUT_IMAGE(out2, false);
        if (numCols == 0 && numRows == 0) {
            numCols = ro2->image->numCols;
            numRows = ro2->image->numRows;
            x0 = ro2->col0;
            y0 = ro2->row0;
        }
    }
    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        PS_ASSERT_IMAGES_SIZE_EQUAL(ro1->image, ro2->image, false);
    }
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, false);
    if (subMask) {
        PS_ASSERT_IMAGE_NON_NULL(subMask, false);
        PS_ASSERT_IMAGE_TYPE(subMask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGE_SIZE(subMask, numCols, numRows, false);
    }
    PS_ASSERT_INT_NONNEGATIVE(stride, false);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(poorFrac, 0.0, false);
    PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(poorFrac, 1.0, false);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(kernelError, 0.0, false);
    PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(kernelError, 1.0, false);
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(covarFrac, 0.0, false);
    PS_ASSERT_FLOAT_LESS_THAN(covarFrac, 1.0, false);
    if (region && psRegionIsNaN(*region)) {
        psString string = psRegionToString(*region);
        psError(PM_ERR_PROG, true, "Input region (%s) contains NAN values", string);
        psFree(string);
        return false;
    }

    psTimerStart("pmSubtractionConvolve");

    bool threaded = pmSubtractionThreaded(); // Running threaded?

    // XXX This is no longer used 
    psImage *convMask = NULL;           // Convolved mask image (common to inputs 1 and 2)
    if (subMask) {
        if (kernels->mode == PM_SUBTRACTION_MODE_1 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
            convMask = out1->mask;
        }
        if (kernels->mode == PM_SUBTRACTION_MODE_2 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
            if (!convMask) {
                convMask = out2->mask;
            }
        }
    }

    psImage *kernelErr1 = NULL, *kernelErr2 = NULL; // Kernel error images
#ifdef USE_KERNEL_ERR
    if (kernels->mode == PM_SUBTRACTION_MODE_1 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        kernelErr1 = subtractionKernelErrImage(ro1->image, kernelError);
    }
    if (kernels->mode == PM_SUBTRACTION_MODE_2 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        kernelErr2 = subtractionKernelErrImage(ro2->image, kernelError);
    }
#endif

    int size = kernels->size;           // Half-size of kernel

    // Get region for convolution: [xMin:xMax,yMin:yMax]
    int xMin = kernels->xMin + size, xMax = kernels->xMax - size;
    int yMin = kernels->yMin + size, yMax = kernels->yMax - size;
    if (region) {
        xMin = PS_MAX(region->x0, xMin);
        xMax = PS_MIN(region->x1, xMax);
        yMin = PS_MAX(region->y0, yMin);
        yMax = PS_MIN(region->y1, yMax);
    }

#if 0
    // XXX Use thread-specific data to store these
    psImage *polyValues = NULL;         // Pre-calculated polynomial values
    psKernel *kernelImage = NULL;       // Kernel for the images
    psKernel *kernelVariance = NULL;      // Kernel for the variance maps
#endif

    // Need to turn off threads at the psLib level --- otherwise, we end up with threads on top of threads,
    // and everything is executing psThreadPoolWait, waiting for some other mythical thread to complete the
    // thread's work.
    bool oldThreads = psImageConvolveSetThreads(false); // Old value of threading for psImageConvolve

    if (stride == 0) {
        // Use the full size of the kernel
        stride = 2 * size + 1;
    }

    for (int j = yMin; j < yMax; j += stride) {
        int ySubMax = PS_MIN(j + stride, yMax); // Range for subregion of interest
        for (int i = xMin; i < xMax; i += stride) {
            int xSubMax = PS_MIN(i + stride, xMax); // Range for subregion of interest

            psRegion *subRegion = psRegionAlloc(i, xSubMax, j, ySubMax); // Bounds of subtraction
	    // for a TEST, do not run threaded for testing
            // if (false && threaded) {
	    if (threaded) {
                psThreadJob *job = psThreadJobAlloc("PSMODULES_SUBTRACTION_CONVOLVE");
                psArray *args = job->args;
                PS_ARRAY_ADD_SCALAR(args, numCols, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(args, numRows, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(args, x0, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(args, y0, PS_TYPE_S32);
                psArrayAdd(args, 1, out1);
                psArrayAdd(args, 1, out2);
                psArrayAdd(args, 1, convMask);
                psArrayAdd(args, 1, (pmReadout*)ro1); // Casting away const
                psArrayAdd(args, 1, (pmReadout*)ro2); // Casting away const
                psArrayAdd(args, 1, kernelErr1);
                psArrayAdd(args, 1, kernelErr2);
                psArrayAdd(args, 1, subMask);
                PS_ARRAY_ADD_SCALAR(args, maskBad, PS_TYPE_IMAGE_MASK);
                PS_ARRAY_ADD_SCALAR(args, maskPoor, PS_TYPE_IMAGE_MASK);
                PS_ARRAY_ADD_SCALAR(args, poorFrac, PS_TYPE_F32);
                psArrayAdd(args, 1, subRegion);
                psArrayAdd(args, 1, (pmSubtractionKernels*)kernels); // Casting away const
                PS_ARRAY_ADD_SCALAR(args, doBG, PS_TYPE_U8);
                PS_ARRAY_ADD_SCALAR(args, useFFT, PS_TYPE_U8);

                if (!psThreadJobAddPending(job)) {
                    return false;
                }
            } else {
                subtractionConvolvePatch(numCols, numRows, x0, y0, out1, out2, convMask, ro1, ro2,
                                         kernelErr1, kernelErr2, subMask, maskBad, maskPoor, poorFrac,
                                         subRegion, kernels, doBG, useFFT);
            }
            psFree(subRegion);
        }
    }

    if (!psThreadPoolWait(false, true)) {
        psError(psErrorCodeLast(), false, "Error waiting for threads.");
        return false;
    }

    // We don't rely on psThreadPoolWait to harvest the jobs because the job contains a reference to the
    // subMask, which is being changed on a thread, and psThreadPoolWait doesn't know that it needs to be
    // locked before freeing.  After psThreadPoolWait, however, the jobs are completed, the threads are idle,
    // and so there's no need to lock the subMask when we're blowing away the jobs.
    if (threaded) {
        psThreadJob *job;               // Completed job
        while ((job = psThreadJobGetDone())) {
            psAssert(strcmp(job->type, "PSMODULES_SUBTRACTION_CONVOLVE") == 0,
                     "Job has incorrect type: %s", job->type);
            psFree(job);
        }
    }
    psImageConvolveSetThreads(oldThreads);

    psFree(kernelErr1);
    psFree(kernelErr2);

    static int nOut1 = 0;
    static int nOut2 = 0;

    // Calculate covariances
    // This can be fairly involved, so we only do it for a small number of instances
    float position[NUM_COVAR_POS] = { -1.0, -0.5, 0.0, +0.5, +1.0 }; // Positions for covariance calculations
    // Enable threads for covariance calculation, since we're not threading on top of it
    oldThreads = psImageCovarianceSetThreads(true);
    if (kernels->mode == PM_SUBTRACTION_MODE_1 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        psArray *covars = psArrayAlloc(PS_SQR(NUM_COVAR_POS)); // Covariances
        for (int y = 0, i = 0; y < NUM_COVAR_POS; y++) {
            for (int x = 0; x < NUM_COVAR_POS; x++, i++) {
                psKernel *kernel = pmSubtractionKernel(kernels, position[x], position[y],
                                                       false); // Convolution kernel
                psKernelTruncate(kernel, covarFrac);
                covars->data[i] = psImageCovarianceCalculate(kernel, ro1->covariance);
		if (0) {
		    char name[128];
		    snprintf (name, 128, "covar.sample1.%03d.fits", nOut1);
		    psKernel *cov = covars->data[i];
		    psFitsWriteImageSimple (name, cov->image, NULL);

		    snprintf (name, 128, "incovar.sample1.%03d.fits", nOut1);
		    psFitsWriteImageSimple (name, ro1->covariance->image, NULL);

		    snprintf (name, 128, "conv.sample1.%03d.fits", nOut1);
		    psFitsWriteImageSimple (name, kernel->image, NULL);

		    fprintf (stderr, "incov: %d,%d; kern: %d,%d, outcov: %d,%d\n", 
			     ro1->covariance->image->numCols, ro1->covariance->image->numRows, 
			     kernel->image->numCols, kernel->image->numRows,
			     cov->image->numCols, cov->image->numRows);

		    nOut1 ++;
		}
                psFree(kernel);
            }
        }
	psFree(out1->covariance);
        out1->covariance = psImageCovarianceAverage(covars);
        psFree(covars);
        if (!out1->covariance) {
            psError(PM_ERR_UNKNOWN, false, "psImageCovarianceAverage returned NULL for out1.");
            return false;
        }
        // Remove covariance factor from covariance, since we've put it in the variance map already
        float factor = psImageCovarianceFactor(out1->covariance);
        psBinaryOp(out1->covariance->image, out1->covariance->image, "/", psScalarAlloc(factor, PS_TYPE_F32));
    }
    if (kernels->mode == PM_SUBTRACTION_MODE_2 || kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        psArray *covars = psArrayAlloc(PS_SQR(NUM_COVAR_POS)); // Covariances
        for (int y = 0, i = 0; y < NUM_COVAR_POS; y++) {
            for (int x = 0; x < NUM_COVAR_POS; x++, i++) {
                psKernel *kernel = pmSubtractionKernel(kernels, position[x], position[y],
                                                       kernels->mode == PM_SUBTRACTION_MODE_DUAL); // Convolution kernel
                psKernelTruncate(kernel, covarFrac);
                covars->data[i] = psImageCovarianceCalculate(kernel, ro2->covariance);
		if (0) {
		    char name[128];
		    snprintf (name, 128, "covar.sample2.%03d.fits", nOut2);
		    psKernel *cov = covars->data[i];
		    psFitsWriteImageSimple (name, cov->image, NULL);

		    snprintf (name, 128, "incovar.sample2.%03d.fits", nOut2);
		    psFitsWriteImageSimple (name, ro2->covariance->image, NULL);

		    snprintf (name, 128, "conv.sample2.%03d.fits", nOut2);
		    psFitsWriteImageSimple (name, kernel->image, NULL);

		    fprintf (stderr, "incov: %d,%d; kern: %d,%d, outcov: %d,%d\n", 
			     ro2->covariance->image->numCols, ro2->covariance->image->numRows, 
			     kernel->image->numCols, kernel->image->numRows,
			     cov->image->numCols, cov->image->numRows);

		    nOut2 ++;
		}
                psFree(kernel);
            }
        }
	psFree(out2->covariance);
        out2->covariance = psImageCovarianceAverage(covars);
        psFree(covars);
        if (!out2->covariance) {
            psError(PM_ERR_UNKNOWN, false, "psImageCovarianceAverage returned NULL for out2.");
            return false;
        }
        // Remove covariance factor from covariance, since we've put it in the variance map already
        float factor = psImageCovarianceFactor(out2->covariance);
        psBinaryOp(out2->covariance->image, out2->covariance->image, "/", psScalarAlloc(factor, PS_TYPE_F32));
    }
    psImageCovarianceSetThreads(oldThreads);

    // Copy anything that wasn't convolved (they may have been allocated though, so free them)
    switch (kernels->mode) {
      case PM_SUBTRACTION_MODE_1:
        if (out2) {
	    psFree(out2->image);
	    psFree(out2->variance);
	    psFree(out2->mask);
	    psFree(out2->covariance);
            out2->image = psMemIncrRefCounter(ro2->image);
            out2->variance = psMemIncrRefCounter(ro2->variance);
            out2->mask = psMemIncrRefCounter(ro2->mask);
            out2->covariance = psMemIncrRefCounter(ro2->covariance);
        }
        break;
      case PM_SUBTRACTION_MODE_2:
        if (out1) {
	    psFree(out1->image);
	    psFree(out1->variance);
	    psFree(out1->mask);
	    psFree(out1->covariance);
            out1->image = psMemIncrRefCounter(ro1->image);
            out1->variance = psMemIncrRefCounter(ro1->variance);
            out1->mask = psMemIncrRefCounter(ro1->mask);
            out1->covariance = psMemIncrRefCounter(ro1->covariance);
        }
        break;
      case PM_SUBTRACTION_MODE_DUAL:
        break;
      default:
        psAbort("Should never get here.");
    }

    // Data exists on the outputs now
    if (out1) {
        out1->data_exists = true;
        if (out1->parent) {
            out1->parent->data_exists = out1->parent->parent->data_exists = true;
        }
    }
    if (out2) {
        out2->data_exists = true;
        if (out2->parent) {
            out2->parent->data_exists = out2->parent->parent->data_exists = true;
        }
    }

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Convolve image: %f sec",
             psTimerClear("pmSubtractionConvolve"));

    return true;
}

bool pmSubtractionGetFWHMs(float *fwhm1, float *fwhm2) {

  *fwhm1 = FWHM1;
  *fwhm2 = FWHM2;
  return true;
}

bool pmSubtractionSetFWHMs(float fwhm1, float fwhm2) {

  FWHM1 = fwhm1;
  FWHM2 = fwhm2;
  return true;
}

static void pmSubtractionQualityFree(pmSubtractionQuality *quality) {

    psFree (quality->fluxes);
    psFree (quality->chisq);
    psFree (quality->moments);
    psFree (quality->stampMask);
}    

pmSubtractionQuality *pmSubtractionQualityAlloc() {

    pmSubtractionQuality *quality = psAlloc(sizeof(pmSubtractionQuality)); // Stamp list to return
    psMemSetDeallocator(quality, (psFreeFunc)pmSubtractionQualityFree);

    quality->fluxes = NULL;
    quality->chisq = NULL;
    quality->moments = NULL;
    quality->stampMask = NULL;

    quality->score = NAN;
    quality->mode = PM_SUBTRACTION_MODE_ERR;
    quality->spatialOrder = -1;
    quality->nGood = 0;
    
    return quality;
}

/** @file  psImageInterpolate.c
 *
 *  @brief Contains functions for interpolating an image
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Ross Harman, MHPCC
 *  @author Paul Price, IfA
 *
 *  @version $Revision: 1.35 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-04 21:43:11 $
 *
 *  Copyright 2004-2007 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psError.h"
#include "psAssert.h"
#include "psString.h"
#include "psImage.h"
#include "psImageConvolve.h"

# define IS_BILIN_SEPARABLE 0

#include "psImageInterpolate.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interpolation kernels
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Kernel sizes as a function of interpolation mode: probably faster than a 'switch'
static int kernelSizes[] = { 0,    // NONE
                             0,    // FLAT
                             2,    // BILINEAR
                             3,    // BIQUADRATIC
                             3,    // GAUSS
                             4,    // LANCZOS2
                             6,    // LANCZOS3
                             8,    // LANCZOS4
                             2     // BILINEAR_SIMPLE
};

// code below uses statically allocated kernel arrays that need to know the 
// maximum possible size of a kernel.  16 is >> 8 above
# define MAX_KERNEL_SIZE 16
# define USE_MAX_KERNEL 0

# if (IS_BILIN_SEPARABLE)
/// Generate a linear interpolation kernel
static inline void interpolationKernelBilinear(psF32 *kernel, // Kernel vector to populate
                                               float frac // Fraction of pixel
                                               )
{
    kernel[0] = 1.0 - frac;
    kernel[1] = frac;
}
# else
/// Generate a linear interpolation kernel
static inline void interpolationKernelBilinear(
    psF32 kernel[kernelSizes[PS_INTERPOLATE_BILINEAR]][kernelSizes[PS_INTERPOLATE_BILINEAR]], // Kernel
    float xFrac, float yFrac // Fraction of pixel
    )
{
    // (xF*yF, (1-xF)(1-yF) = 1 - xF - yF + xFyF 

    kernel[0][0] = 1.0 - xFrac - yFrac + xFrac*yFrac;
    kernel[1][0] = yFrac - xFrac*yFrac;
    kernel[0][1] = xFrac - xFrac*yFrac;
    kernel[1][1] = xFrac*yFrac;
}
# endif

#if 0
/// Generate a biquadratic interpolation kernel
/// This reduces Gene's original made-up 2D kernel to 1D
static inline void interpolationKernelBiquadratic(psF32 *kernel, // Kernel vector to populate
                                                  float frac // Fraction of pixel
    )
{
    float x = frac / 6.0;
    float xx = frac * x;
    kernel[0] = -x + xx;
    kernel[1] = 1.0/3.0 - 2.0 * xx;
    kernel[2] = x + xx;
}
#else
/// Generate a biquadratic interpolation kernel
static inline void interpolationKernelBiquadratic(
    psF32 kernel[kernelSizes[PS_INTERPOLATE_BIQUADRATIC]][kernelSizes[PS_INTERPOLATE_BIQUADRATIC]], // Kernel
    float xFrac, float yFrac // Fraction of pixel
    )
{
    double xxFrac = xFrac * xFrac / 6.0;
    double yyFrac = yFrac * yFrac / 6.0;
    double xyFrac = 0.25 * xFrac * yFrac;
    xFrac /= 6.0;
    yFrac /= 6.0;
    kernel[0][0] = - 1.0/9.0 - xFrac - yFrac + xxFrac + yyFrac + xyFrac;
    kernel[0][1] = 2.0/9.0 - yFrac - 2.0 * xxFrac + yyFrac;
    kernel[0][2] = - 1.0/9.0 + xFrac - yFrac + xxFrac + yyFrac - xyFrac;
    kernel[1][0] = 2.0/9.0 - xFrac + xxFrac - 2.0 * yyFrac;
    kernel[1][1] = 5.0/9.0 - 2.0 * xxFrac - 2.0 * yyFrac;
    kernel[1][2] = 2.0/9.0 + xFrac + xxFrac - 2.0 * yyFrac;
    kernel[2][0] = - 1.0/9.0 - xFrac + yFrac + xxFrac + yyFrac - xyFrac;
    kernel[2][1] = 2.0/9.0 + yFrac - 2.0 * xxFrac + yyFrac;
    kernel[2][2] = - 1.0/9.0 + xFrac + yFrac + xxFrac + yyFrac + xyFrac;
}
#endif


#if 0
/// Generate a bicubic interpolation kernel
/// "cubic Hermite spline" has a=-1/2 for:
/// W(x) = 1 when x == 0
///      = (a+2)|x|^3 - (a+3)|x|^2 + 1 when 0 < |x| < 1
///      = a|x|^3 - 5a|x|^2 + 8a|x| - 4a when 1 < |x| < 2
///      = 0 otherwise
static inline void interpolationKernelBicubic(psF32 *kernel, // Kernel vector to populate
                                              float frac // Fraction of pixel
    )
{
    float frac2 = PS_SQR(frac);
    float frac3 = frac2 * frac;
    kernel[0] = 0.5 * frac + frac2 - 0.5 * frac3; // x = -(1 + frac)
    kernel[1] = 1.5 * frac3 - 2.5 * frac2 + 1.0; // x = -frac
    kernel[2] = 0.5 * frac + 2.0 * frac2 - 1.5 * frac3; // x = 1 - frac
    kernel[3] = -0.5 * frac2 - frac3;   // x = 2 - frac
}
#endif

// Generate Lanczos interpolation kernel
static inline void interpolationKernelLanczos(psF32 *kernel, // Kernel vector to populate
                                              int size,    // Size of kernel
                                              float frac   // Fraction of pixel
                                              )
{
    if (fabs(frac) < FLT_EPSILON) {
        // No real shift
        for (int i = 0; i < (size - 1) / 2; i++) {
            kernel[i] = 0.0;
        }
        kernel[(size - 1) / 2] = 1.0;
        for (int i = (size - 1) / 2 + 1; i < size; i++) {
            kernel[i] = 0.0;
        }
        return;
    }

    float norm1 = size / 2.0 / PS_SQR(M_PI); // Normalisation for laczos
    float norm2 = M_PI;                 // Normalisation for sinc function 1
    float norm3 = M_PI * 2.0 / (float)size; // Normalisation for sinc function 2
    float pos = - (size - 1)/2 - frac;  // Position of interest
    for (int i = 0; i < size; i++, pos += 1.0) {
        if (fabs(pos) < FLT_EPSILON) {
            kernel[i] = 1.0;
        } else {
            kernel[i] = norm1 * sinf(pos * norm2) * sinf(pos * norm3) / PS_SQR(pos);
        }
    }
    return;
}

// Generate Gauss interpolation kernel
static inline void interpolationKernelGauss(psF32 *kernel, // Kernel vector to populate
                                          int size, // Size of kernel
                                          float sigma, // Width of kernel
                                          float frac // Fraction of pixel
                                          )
{
    for (int i = 0, pos = - (size - 1) / 2; i < size; i++, pos++) {
        kernel[i] = expf(-0.5 / PS_SQR(sigma) * PS_SQR(pos - frac));
    }
    // XXX Normalise?
    return;
}


// Generate 1D interpolation kernel
static inline void interpolationKernel(psF32 *kernel, // Kernel vector to populate
                                       float frac, // Fraction of pixel
                                       psImageInterpolateMode mode // Mode of interpolation
                                       )
{
    psAssert(frac >= 0.0 && frac <= 1.0, "Pixel fraction is %f", frac);
    switch (mode) {
      case PS_INTERPOLATE_FLAT:
        // Nothing to pre-compute
        break;
      case PS_INTERPOLATE_BILINEAR_SIMPLE:
        // Nothing to pre-compute
	kernel[0] = 1.0 - frac;
	kernel[1] = frac;
        break;
# if (IS_BILIN_SEPARABLE)
      case PS_INTERPOLATE_BILINEAR:
        interpolationKernelBilinear(kernel, frac);
        break;
# endif
      case PS_INTERPOLATE_GAUSS:
        interpolationKernelGauss(kernel, kernelSizes[mode], 0.5, frac);
        break;
      case PS_INTERPOLATE_LANCZOS2:
      case PS_INTERPOLATE_LANCZOS3:
      case PS_INTERPOLATE_LANCZOS4:
        interpolationKernelLanczos(kernel, kernelSizes[mode], frac);
        break;
# if (!IS_BILIN_SEPARABLE)
      case PS_INTERPOLATE_BILINEAR:
# endif
      case PS_INTERPOLATE_BIQUADRATIC:  // 2D kernel
      default:
        psAbort("Unsupported interpolation mode: %x", mode);
    }

    return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void imageInterpolationFree(psImageInterpolation *interp)
{
    // Casting away const
    psFree(interp->image);
    psFree(interp->mask);
    psFree(interp->variance);
    psFree(interp->kernel);
    psFree(interp->kernel2);
    psFree(interp->sumKernel2);
}

psImageInterpolation *psImageInterpolationAlloc(psImageInterpolateMode mode,
                                                const psImage *image, const psImage *variance,
                                                const psImage *mask, psImageMaskType maskVal,
                                                double badImage, double badVariance,
                                                psImageMaskType badMask, psImageMaskType poorMask,
                                                float poorFrac, int numKernels)
{
    psImageInterpolation *interp = psAlloc(sizeof(psImageInterpolation)); // Options, to return
    psMemSetDeallocator(interp, (psFreeFunc)imageInterpolationFree);

    interp->mode = mode;
    // Casting away const to add to interp
    interp->image = psMemIncrRefCounter((psImage*)image);
    interp->variance = psMemIncrRefCounter((psImage*)variance);
    interp->mask = psMemIncrRefCounter((psImage*)mask);
    interp->maskVal = maskVal;
    interp->badImage = badImage;
    interp->badVariance = badVariance;
    interp->badMask = badMask;
    interp->poorMask = poorMask;
    interp->poorFrac = poorFrac;
    interp->shifting = true;

    // Pre-calculate interpolation kernels
    psImage *kernel = NULL;             // Kernel
    psImage *kernel2 = NULL;            // Kernel^2
    psVector *sumKernel2 = NULL;        // Sum of kernel^2

    switch (mode) {
# if (IS_BILIN_SEPARABLE)
      case PS_INTERPOLATE_BILINEAR:
# endif
      case PS_INTERPOLATE_GAUSS:
      case PS_INTERPOLATE_LANCZOS2:
      case PS_INTERPOLATE_LANCZOS3:
      case PS_INTERPOLATE_LANCZOS4:
        if (numKernels > 0) {
            int size = kernelSizes[mode];      // Size of kernel

            kernel = psImageAlloc(size, numKernels, PS_TYPE_F32); // Kernel
            kernel2 = psImageAlloc(size, numKernels, PS_TYPE_F32); // Kernel^2
            sumKernel2 = psVectorAlloc(numKernels, PS_TYPE_F32); // Sum of kernel^2

            for (int i = 0; i < numKernels; i++) {
                float frac = i / (float)numKernels;   // Fraction of shift
                interpolationKernel(kernel->data.F32[i], frac, mode);

                float sum = 0.0;               // Sum of kernel^2
                for (int j = 0; j < size; j++) {
                    sum += kernel2->data.F32[i][j] = PS_SQR(kernel->data.F32[i][j]);
                }
                sumKernel2->data.F32[i] = sum;
            }
        }
        break;
# if (!IS_BILIN_SEPARABLE)
      case PS_INTERPOLATE_BILINEAR:
# endif
      case PS_INTERPOLATE_BIQUADRATIC:
      case PS_INTERPOLATE_BILINEAR_SIMPLE:
        // 2D kernel, would cost too much memory to pre-calculate
        break;

      default:
        psAbort("Unsupported interpolation mode: %x", mode);
    }

    interp->kernel = kernel;
    interp->kernel2 = kernel2;
    interp->sumKernel2 = sumKernel2;
    interp->numKernels = numKernels;

    return interp;
}


// Interpolation engine for flat mode (nearest pixel)
static inline psImageInterpolateStatus interpolateFlat(double *imageValue, double *varianceValue,
                                                       psImageMaskType *maskValue, float x, float y,
                                                       const psImageInterpolation *interp)
{
    // Parameters have been checked by psImageInterpolate()

    const psImage *image = interp->image; // Image of interest
    int xInt = round(x - 0.5 + FLT_EPSILON); // Pixel closest to point of interest in x
    int yInt = round(y - 0.5 + FLT_EPSILON); // Pixel closest to point of interest in y
    int xLast = image->numCols - 1;     // Last pixel in x
    int yLast = image->numRows - 1;     // Last pixel in y

    if (xInt < 0 || xInt > xLast || yInt < 0 || yInt > yLast) {
        // At least one pixel of the interpolation kernel is off the image
        if (imageValue) {
            *imageValue = interp->badImage;
        }
        if (varianceValue) {
            *varianceValue = interp->badVariance;
        }
        if (maskValue) {
            *maskValue = interp->badMask;
        }

        return PS_INTERPOLATE_STATUS_OFF;
    } else {

        // Image and variance 'interpolation' according to image type
        #define FLAT_CASE(TYPE) \
          case PS_TYPE_##TYPE: { \
            if (imageValue) { \
                *imageValue = interp->image->data.TYPE[yInt][xInt]; \
            } \
            if (varianceValue && interp->variance) { \
                *varianceValue = interp->variance->data.TYPE[yInt][xInt]; \
            } \
            break; \
        }

        switch (interp->image->type.type) {
            FLAT_CASE(U8);
            FLAT_CASE(U16);
            FLAT_CASE(U32);
            FLAT_CASE(U64);
            FLAT_CASE(S8);
            FLAT_CASE(S16);
            FLAT_CASE(S32);
            FLAT_CASE(S64);
            FLAT_CASE(F32);
            FLAT_CASE(F64);
          default:
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unrecognised type for image: %x",
                    interp->image->type.type);
            return PS_INTERPOLATE_STATUS_ERROR;
        }

        if (maskValue) {
            if (interp->mask) {
                *maskValue = interp->mask->data.PS_TYPE_IMAGE_MASK_DATA[yInt][xInt];
            } else {
                *maskValue = 0;
            }
        }
    }
    return PS_INTERPOLATE_STATUS_GOOD;
}


// Can't interpolate: kernel is off the image
#define INTERPOLATE_OFF() { \
        *imageValue = interp->badImage; \
        if (varianceValue) { \
            *varianceValue = interp->badVariance; \
        } \
        if (maskValue) { \
            *maskValue = interp->badMask; \
        } \
        return PS_INTERPOLATE_STATUS_OFF; \
    }

// Set up the kernel parameters; defines some useful values
#define INTERPOLATE_SETUP(X, Y) \
    /* Central pixel is the pixel below the point of interest */ \
    int xCentral = floor((X) - 0.5), yCentral = floor((Y) - 0.5); /* Central pixel */ \
    float xFrac = (X) - xCentral - 0.5, yFrac = (Y) - yCentral - 0.5; /* Fraction of pixel */ \
    bool xExact = fabsf(xFrac) < FLT_EPSILON, yExact = fabsf(yFrac) < FLT_EPSILON; /* Are shifts exact? */ \

// Set up the kernel parameters; defines some useful values
// EAM : I'm unsure of the right answer here
#define INTERPOLATE_SETUP_EAM(X, Y)				 \
    int xCentral = floor((X)), yCentral = floor((Y)); /* Central pixel */ \
    float xFrac = (X) - xCentral, yFrac = (Y) - yCentral; /* Fraction of pixel */ \
    bool xExact = fabsf(xFrac) < FLT_EPSILON, yExact = fabsf(yFrac) < FLT_EPSILON; /* Are shifts exact? */ \

// Determine the range of the kernel; defines some useful values
#define INTERPOLATE_RANGE() \
    int xLast = image->numCols - 1, yLast = image->numRows - 1; /* Last pixels of image */ \
    /* Proper range of kernel on image; not all may be defined */ \
    int xStart = xCentral - (size - 1) / 2, xStop = xCentral + size / 2; \
    int yStart = yCentral - (size - 1) / 2, yStop = yCentral + size / 2; \
    if (xStart > xLast || xStop < 0 || yStart > yLast || yStop < 0) { \
        /* Interpolation kernel is entirely off the image */ \
        INTERPOLATE_OFF(); /* Returns */ \
    } \
    int xMin, xMax, yMin, yMax;         /* Minimum and maximum valid pixels on image */ \
    int iMin, iMax, jMin, jMax;         /* Minimum and maximum valid pixels on kernel */ \
    bool offImage = false;              /* At least one pixel of the kernel is off the image */ \
    /* XXX Haven't got single dimension exact shifts implemented properly yet. */ \
    xExact = yExact = false; \
    if (xExact) { \
        if (xStart < 0 || xStart > xLast) { \
            INTERPOLATE_OFF(); /* Returns */ \
        } \
        iMin = (size - 1) / 2; \
        iMax = iMin + 1; \
    } else { \
        if (xStart < 0) { \
            xMin = 0; \
            iMin = -xStart; \
            offImage = true; \
        } else { \
            xMin = xStart; \
            iMin = 0; \
        } \
        if (xStop > xLast) { \
            xMax = xLast; \
            iMax = xLast - xStart + 1; \
            offImage = true; \
        } else { \
            xMax = xStop; \
            iMax = size; \
        } \
    } \
    if (yExact) { \
        if (yStart < 0 || yStart > yLast) { \
            INTERPOLATE_OFF(); /* Returns */ \
        } \
        jMin = (size - 1) / 2; \
        jMax = jMin + 1; \
    } else { \
        if (yStart < 0) { \
            yMin = 0; \
            jMin = -yStart; \
            offImage = true; \
        } else { \
            yMin = yStart; \
            jMin = 0; \
        } \
        if (yStop > yLast) { \
            yMax = yLast; \
            jMax = yLast - yStart + 1; \
            offImage = true; \
        } else { \
            yMax = yStop; \
            jMax = size; \
        } \
    }

#define INTERPOLATE_CHECK() \
    if (xMin < 0) { /* XXX warn or error? */ } \
    if (yMin < 0) { /* XXX warn or error? */ } \
    if (xMax >= image->numCols) { /* XXX warn or error? */ } \
    if (yMax >= image->numRows) { /* XXX warn or error? */ } \

// Determine the result of the interpolation after all the math has been done
static psImageInterpolateStatus interpolateResult(const psImageInterpolation *interp,
                                                  double *imageValue, double *varianceValue,
                                                  psImageMaskType *maskValue,
                                                  double sumImage, double sumVariance, double sumBad,
                                                  double sumKernel, double sumKernel2,
                                                  bool wantVariance, bool haveMask)
{
    *imageValue = sumKernel > 0 ? sumImage / sumKernel : interp->badImage;
    if (wantVariance) {
        if (sumBad > 0) {
            sumVariance *= sumKernel2 / (sumKernel2 - sumBad);
        }
        *varianceValue = sumVariance / PS_SQR(sumKernel);
    }
    if (sumKernel == 0.0) {
        // No kernel contributions at all
        if (haveMask && maskValue) {
            *maskValue |= interp->badMask;
        }
        return PS_INTERPOLATE_STATUS_BAD;
    }
    if (sumBad == 0) {
        // Completely good pixel
        return PS_INTERPOLATE_STATUS_GOOD;
    }
    if (sumBad < PS_SQR(interp->poorFrac) * sumKernel2) {
        // Some pixels masked: poor pixel
        if (haveMask && maskValue) {
            *maskValue |= interp->poorMask;
        }
        return PS_INTERPOLATE_STATUS_POOR;
    }
    // Many pixels (or a few important pixels) masked: bad pixel
    if (haveMask && maskValue) {
        *maskValue |= interp->badMask;
    }
    return PS_INTERPOLATE_STATUS_BAD;
}

// Interpolation engine for separable interpolation kernels
static psImageInterpolateStatus interpolateSeparable(double *imageValue, double *varianceValue,
                                                     psImageMaskType *maskValue, float x, float y,
                                                     const psImageInterpolation *interp)
{
    // Parameters have been checked by psImageInterpolate()

    psImageInterpolateMode mode = interp->mode; // Mode of interpolation
    const psImage *image = interp->image; // Image of interest
    const psImage *mask = interp->mask; // Image mask
    const psImage *variance = interp->variance; // Image variance
    psImageMaskType maskVal = interp->maskVal; // Value to mask
    bool wantVariance = variance && varianceValue; // Does the user want the variance value?
    bool haveMask = mask && maskVal; // Does the user want the variance value?

    // Kernel basics
    int size = kernelSizes[mode];       // Size of kernel
    INTERPOLATE_SETUP(x, y);
    if (xExact && yExact) {
        return interpolateFlat(imageValue, varianceValue, maskValue, x, y, interp);
    }
    INTERPOLATE_RANGE();
    INTERPOLATE_CHECK();

    // Get the appropriate kernels
    psVector *xKernelNew = NULL, *yKernelNew = NULL;  // New kernels if not pre-calculated
    psVector *xKernel2New = NULL, *yKernel2New = NULL;// New kernel^2 if not pre-calculated
    psF32 *xKernel, *yKernel;           // Kernel values
    psF32 *xKernel2, *yKernel2;         // Kernel^2 values
    float xSumKernel2, ySumKernel2;     // Sum of kernel^2
    int numKernels = interp->numKernels; // Number of pre-calculated kernels
    if (numKernels > 0) {
        const psImage *kernel = interp->kernel; // Pre-calculated kernels
        const psImage *kernel2 = interp->kernel2; // Pre-calculated kernel^2
        const psVector *sumKernel2 = interp->sumKernel2; // Pre-calculated kernel^2
        int xIndex = xFrac * numKernels + 0.5; // Index of closest pre-calculated kernel
        if (xIndex == numKernels) {
            xIndex = 0;
        }
        psAssert(xIndex >=0 && xIndex < numKernels, "xIndex is %d", xIndex);
        xKernel = kernel->data.F32[xIndex];
        xKernel2 = kernel2->data.F32[xIndex];
        xSumKernel2 = sumKernel2->data.F32[xIndex];
        int yIndex = yFrac * numKernels + 0.5; // Index of closest pre-calculated kernel
        if (yIndex == numKernels) {
            yIndex = 0;
        }
        psAssert(yIndex >=0 && yIndex < numKernels, "yIndex is %d", yIndex);
        yKernel = kernel->data.F32[yIndex];
        yKernel2 = kernel2->data.F32[yIndex];
        ySumKernel2 = sumKernel2->data.F32[yIndex];
    } else {
        xKernelNew = psVectorAlloc(size, PS_TYPE_F32);
        xKernel = xKernelNew->data.F32;
        interpolationKernel(xKernel, xFrac, mode);
        yKernelNew = psVectorAlloc(size, PS_TYPE_F32);
        yKernel = yKernelNew->data.F32;
        interpolationKernel(yKernel, yFrac, mode);

        if (haveMask || wantVariance || offImage) {
            xKernel2New = psVectorAlloc(size, PS_TYPE_F32);
            xKernel2 = xKernel2New->data.F32;
            interpolationKernel(xKernel2, xFrac, mode);
            yKernel2New = psVectorAlloc(size, PS_TYPE_F32);
            yKernel2 = yKernel2New->data.F32;
            interpolationKernel(yKernel2, yFrac, mode);
            xSumKernel2 = 0.0;
            ySumKernel2 = 0.0;
            for (int i = 0; i < size; i++) {
                xSumKernel2 += xKernel2[i] = PS_SQR(xKernel2[i]);
                ySumKernel2 += yKernel2[i] = PS_SQR(yKernel2[i]);
            }
        } else {
            // Required for compilation when optimising
            xSumKernel2 = ySumKernel2 = 0.0;
            xKernel2 = yKernel2 = NULL;
        }
    }

    float sumKernel = 0.0;              // Sum of the kernel
    double sumBad = 0.0;                // Sum of bad kernel-squared contributions
    double sumImage = 0.0;              // Sum of image multiplied by kernel
    double sumVariance = 0.0;           // Sum of variance multiplied by kernel-squared
    float sumKernel2 = xSumKernel2 * ySumKernel2; // Sum of kernel^2

    // Measure kernel contribution from outside image

    if (offImage) {
        // Bottom rows
        for (int j = 0; j < jMin; j++) {
            sumBad += yKernel2[j] * xSumKernel2;
        }
        // Two sides
        for (int j = jMin; j < jMax; j++) {
            float xSumBad = 0.0;
            for (int i = 0; i < iMin; i++) {
                xSumBad += xKernel2[i];
            }
            for (int i = iMax; i < size; i++) {
                xSumBad += xKernel2[i];
            }
            sumBad += ySumKernel2 * xSumBad;
        }
        // Top rows
        for (int j = jMax; j < size; j++) {
            sumBad += yKernel2[j] * xSumKernel2;
        }
    }

    yKernel += jMin;
    yKernel2 += jMin;
    xKernel += iMin;
    xKernel2 += iMin;

#define INTERPOLATE_SEPARATE_CASE(TYPE) \
  case PS_TYPE_##TYPE: { \
      if (wantVariance) { \
          if (haveMask) { \
              /* Variance and mask */ \
              const psF32 *yKernelData = yKernel; \
              const psF32 *yKernel2Data = yKernel2; \
              for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++, yKernelData++, yKernel2Data++) { \
                  double xSumImage = 0.0; /* Sum of image multiplied by kernel in x */ \
                  double xSumVariance = 0.0; /* Sum of image multiplied by kernel-squared in x */ \
                  float xSumKernel = 0.0; /* Sum of kernel in x */ \
                  float xSumBad = 0.0; /* Sum of bad kernel-squared in x */ \
                  /* Dereferenced versions of inputs */ \
                  const ps##TYPE *imageData = &image->data.TYPE[yPix][xMin]; \
                  const ps##TYPE *varianceData = &variance->data.TYPE[yPix][xMin]; \
                  const psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[yPix][xMin]; \
                  const psF32 *xKernelData = xKernel; \
                  const psF32 *xKernel2Data = xKernel2; \
                  for (int i = iMin; i < iMax; i++, imageData++, varianceData++, maskData++, \
                                               xKernelData++, xKernel2Data++) { \
                      float kernelValue = *xKernelData; /* Value of kernel in x */ \
                      float kernelValue2 = *xKernel2Data; /* Square of kernel in x */ \
                      if (*maskData & maskVal) { \
                          xSumBad += kernelValue2; \
                      } else { \
                          xSumImage += kernelValue * *imageData; \
                          xSumVariance += kernelValue2 * *varianceData; \
                          xSumKernel += kernelValue; \
                      } \
                  } \
                  float kernelValue = *yKernelData; /* Value of kernel in y */ \
                  float kernelValue2 = *yKernel2Data; /* Value of kernel-squared in y */ \
                  sumImage += kernelValue * xSumImage; \
                  sumVariance += kernelValue2 * xSumVariance; \
                  sumBad += kernelValue2 * xSumBad; \
                  sumKernel += kernelValue * xSumKernel; \
              } \
          } else { \
              /* Variance, no mask */ \
              const psF32 *yKernelData = yKernel; \
              const psF32 *yKernel2Data = yKernel2; \
              for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++, yKernelData++, yKernel2Data++) { \
                  double xSumImage = 0.0; /* Sum of image multiplied by kernel in x */ \
                  double xSumVariance = 0.0; /* Sum of image multiplied by kernel-squared in x */ \
                  float xSumKernel = 0.0; /* Sum of kernel in x */ \
                  /* Dereferenced versions of inputs */ \
                  const ps##TYPE *imageData = &image->data.TYPE[yPix][xMin]; \
                  const ps##TYPE *varianceData = &variance->data.TYPE[yPix][xMin]; \
                  const psF32 *xKernelData = xKernel; \
                  const psF32 *xKernel2Data = xKernel2; \
                  for (int i = iMin; i < iMax; i++, imageData++, varianceData++, \
                                               xKernelData++, xKernel2Data++) { \
                      float kernelValue = *xKernelData; /* Value of kernel */ \
                      float kernelValue2 = *xKernel2Data; /* Square of kernel */ \
                      xSumImage += kernelValue * *imageData; \
                      xSumVariance += kernelValue2 * *varianceData; \
                      xSumKernel += kernelValue; \
                  } \
                  float kernelValue = *yKernelData; /* Value of kernel in y */ \
                  float kernelValue2 = *yKernel2Data; /* Value of kernel-squared in y */ \
                  sumImage += kernelValue * xSumImage; \
                  sumVariance += kernelValue2 * xSumVariance; \
                  sumKernel += kernelValue * xSumKernel; \
              } \
          } \
      } else if (haveMask) { \
          /* Mask, no variance */ \
          const psF32 *yKernelData = yKernel; \
          const psF32 *yKernel2Data = yKernel2; \
          for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++, yKernelData++, yKernel2Data++) { \
              double xSumImage = 0.0; /* Sum of image multiplied by kernel in x */ \
              float xSumKernel = 0.0; /* Sum of kernel in x */ \
              float xSumBad = 0.0; /* Sum of bad kernel-squared in x */ \
              /* Dereferenced versions of inputs */ \
              const ps##TYPE *imageData = &image->data.TYPE[yPix][xMin]; \
              const psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[yPix][xMin]; \
              const psF32 *xKernelData = xKernel; \
              const psF32 *xKernel2Data = xKernel2; \
              for (int i = iMin; i < iMax; i++, imageData++, maskData++, xKernelData++, xKernel2Data++) { \
                  float kernelValue = *xKernelData; /* Value of kernel */ \
                  float kernelValue2 = *xKernel2Data; /* Value of kernel-squared */ \
                  if (*maskData & maskVal) { \
                      xSumBad += kernelValue2; \
                  } else { \
                      xSumImage += kernelValue * *imageData; \
                      xSumKernel += kernelValue; \
                  } \
              } \
              float kernelValue = *yKernelData; /* Value of kernel in y */ \
              float kernelValue2 = *yKernel2Data; /* Value of kernel-squared in y */ \
              sumImage += kernelValue * xSumImage; \
              sumBad += kernelValue2 * xSumBad; \
              sumKernel += kernelValue * xSumKernel; \
          } \
      } else {\
          /* Neither variance nor mask */ \
          const psF32 *yKernelData = yKernel; \
          for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++, yKernelData++) { \
              double xSumImage = 0.0; /* Sum of image multiplied by kernel in x */ \
              float xSumKernel = 0.0; /* Sum of kernel in x */ \
              /* Dereferenced versions of inputs */ \
              const ps##TYPE *imageData = &image->data.TYPE[yPix][xMin]; \
              const psF32 *xKernelData = xKernel; \
              for (int i = iMin; i < iMax; i++, imageData++, xKernelData++) { \
                  float kernelValue = *xKernelData; /* Value of kernel */ \
                  xSumImage += kernelValue * *imageData; \
                  xSumKernel += kernelValue; \
              } \
              float kernelValue = *yKernelData; /* Value of kernel in y */ \
              sumImage += kernelValue * xSumImage; \
              sumKernel += kernelValue * xSumKernel; \
          } \
      } \
    } \
    break;


    switch (image->type.type) {
        INTERPOLATE_SEPARATE_CASE(U8);
        INTERPOLATE_SEPARATE_CASE(U16);
        INTERPOLATE_SEPARATE_CASE(U32);
        INTERPOLATE_SEPARATE_CASE(U64);
        INTERPOLATE_SEPARATE_CASE(S8);
        INTERPOLATE_SEPARATE_CASE(S16);
        INTERPOLATE_SEPARATE_CASE(S32);
        INTERPOLATE_SEPARATE_CASE(S64);
        INTERPOLATE_SEPARATE_CASE(F32);
        INTERPOLATE_SEPARATE_CASE(F64);
      default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unrecognised type for image: %x",
                image->type.type);
        return PS_INTERPOLATE_STATUS_ERROR;
    }

    psFree(xKernelNew);
    psFree(yKernelNew);
    psFree(xKernel2New);
    psFree(yKernel2New);

    return interpolateResult(interp, imageValue, varianceValue, maskValue, sumImage, sumVariance, sumBad,
                             sumKernel, sumKernel2, wantVariance, haveMask);
}

// Interpolation engine for (non-separable) interpolation kernels
static psImageInterpolateStatus interpolateKernel(double *imageValue, double *varianceValue,
                                                  psImageMaskType *maskValue, float x, float y,
                                                  const psImageInterpolation *interp)
{
    // Parameters have been checked by psImageInterpolate()

    psImageInterpolateMode mode = interp->mode; // Mode of interpolation
    const psImage *image = interp->image; // Image of interest
    const psImage *mask = interp->mask; // Image mask
    const psImage *variance = interp->variance; // Image variance
    psImageMaskType maskVal = interp->maskVal; // Value to mask
    bool wantVariance = variance && varianceValue; // Does the user want the variance value?
    bool haveMask = mask && maskVal; // Does the user want the variance value?

    // Kernel basics
    int size = kernelSizes[mode];       // Size of kernel
    INTERPOLATE_SETUP(x, y);
    if (xExact && yExact) {
        return interpolateFlat(imageValue, varianceValue, maskValue, x, y, interp);
    }
    INTERPOLATE_RANGE();
    INTERPOLATE_CHECK();

    // Get the appropriate kernels
# if (USE_MAX_KERNEL)
    psAssert (size <= MAX_KERNEL_SIZE, "oops");
    psF32 kernel[MAX_KERNEL_SIZE][MAX_KERNEL_SIZE];
    for (int ix = 0; ix < MAX_KERNEL_SIZE; ix++) {
      for (int iy = 0; iy < MAX_KERNEL_SIZE; iy++) {
	kernel[ix][iy] = 1;
      }
    }
# else
    psF32 kernel[size][size];
    for (int ix = 0; ix < size; ix++) {
      for (int iy = 0; iy < size; iy++) {
	kernel[ix][iy] = 1;
      }
    }
# endif

# if (IS_BILIN_SEPARABLE)
    psAssert(mode == PS_INTERPOLATE_BIQUADRATIC, "Mode is %x", mode);
    interpolationKernelBiquadratic(kernel, xFrac, yFrac);
# else
    psAssert((mode == PS_INTERPOLATE_BIQUADRATIC) || (mode == PS_INTERPOLATE_BILINEAR), "Mode is %x", mode);
    if (mode == PS_INTERPOLATE_BIQUADRATIC) {
	interpolationKernelBiquadratic(kernel, xFrac, yFrac);
    } else {
	interpolationKernelBilinear(kernel, xFrac, yFrac);
    }
# endif

# if (0)
    for (int i = 0; i < size; i++) {
	for (int j = 0; j < size; j++) {
	    fprintf (stderr, "%f ", kernel[i][j]);
	}
	fprintf (stderr, "\n");
    }
# endif

    float sumKernel = 0.0;              // Sum of the kernel
    double sumBad = 0.0;                // Sum of bad kernel-squared contributions
    double sumImage = 0.0;              // Sum of image multiplied by kernel
    double sumVariance = 0.0;           // Sum of variance multiplied by kernel-squared
    float sumKernel2 = 0.0;             // Sum of kernel^2

    // Add contributions in an area outside the image
#define INTERPOLATE_KERNEL_ADD_OFFIMAGE() { \
        sumBad += PS_SQR(kernel[j][i]); \
    }

    // Measure kernel contribution from outside image
    if (offImage) {
        if (!yExact) {
            // Bottom rows
            for (int j = 0; j < jMin; j++) {
                for (int i = 0; i < size; i++) {
                    INTERPOLATE_KERNEL_ADD_OFFIMAGE();
                }
            }
        }
        if (!xExact) {
            // Two sides
            for (int j = jMin; j < jMax; j++) {
                for (int i = 0; i < iMin; i++) {
                    INTERPOLATE_KERNEL_ADD_OFFIMAGE();
                }
                for (int i = iMax; i < size; i++) {
                    INTERPOLATE_KERNEL_ADD_OFFIMAGE();
                }
            }
        }
        if (!yExact) {
            // Top rows
            for (int j = jMax; j < size; j++) {
                for (int i = 0; i < size; i++) {
                    INTERPOLATE_KERNEL_ADD_OFFIMAGE();
                }
            }
        }
    }

#define INTERPOLATE_KERNEL_CASE(TYPE) \
  case PS_TYPE_##TYPE: { \
      if (wantVariance) { \
          if (haveMask) { \
              /* Variance and mask */ \
              for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++) { \
                  for (int i = iMin, xPix = xMin; i < iMax; i++, xPix++) { \
                      float kernelValue = kernel[j][i]; /* Value of kernel */ \
                      float kernelValue2 = PS_SQR(kernelValue); /* Square of kernel */ \
                      if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yPix][xPix] & maskVal) { \
                          sumBad += kernelValue2; \
                      } else { \
                          sumImage += kernelValue * image->data.TYPE[yPix][xPix]; \
                          sumVariance += kernelValue2 * variance->data.TYPE[yPix][xPix]; \
                          sumKernel += kernelValue; \
                          sumKernel2 += kernelValue2; \
                      } \
                  } \
              } \
              \
          } else { \
              /* Variance, no mask */ \
              for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++) { \
                  for (int i = iMin, xPix = xMin; i < iMax; i++, xPix++) { \
                      float kernelValue = kernel[j][i]; /* Value of kernel */ \
                      float kernelValue2 = PS_SQR(kernelValue); /* Square of kernel */ \
                      sumImage += kernelValue * image->data.TYPE[yPix][xPix]; \
                      sumVariance += kernelValue2 * variance->data.TYPE[yPix][xPix]; \
                      sumKernel += kernelValue; \
                      sumKernel2 += kernelValue2; \
                  } \
              } \
          } \
      } else if (haveMask) { \
          /* Mask, no variance */ \
          for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++) { \
              for (int i = iMin, xPix = xMin; i < iMax; i++, xPix++) { \
                  float kernelValue = kernel[j][i]; /* Value of kernel */ \
                  float kernelValue2 = PS_SQR(kernelValue); /* Square of kernel */ \
                  if (mask->data.PS_TYPE_IMAGE_MASK_DATA[yPix][xPix] & maskVal) { \
                      sumBad += kernelValue2; \
                  } else { \
                      sumImage += kernelValue * image->data.TYPE[yPix][xPix]; \
                      sumKernel += kernelValue; \
                      sumKernel2 += kernelValue2; \
                  } \
              } \
          } \
      } else { \
          /* Neither variance nor mask */ \
          for (int j = jMin, yPix = yMin; j < jMax; j++, yPix++) { \
              for (int i = iMin, xPix = xMin; i < iMax; i++, xPix++) { \
                  float kernelValue = kernel[j][i]; /* Value of kernel */ \
                  sumImage += kernelValue * image->data.TYPE[yPix][xPix]; \
                  sumKernel += kernelValue; \
              } \
          } \
      } \
    } \
    break;

    switch (image->type.type) {
        INTERPOLATE_KERNEL_CASE(U8);
        INTERPOLATE_KERNEL_CASE(U16);
        INTERPOLATE_KERNEL_CASE(U32);
        INTERPOLATE_KERNEL_CASE(U64);
        INTERPOLATE_KERNEL_CASE(S8);
        INTERPOLATE_KERNEL_CASE(S16);
        INTERPOLATE_KERNEL_CASE(S32);
        INTERPOLATE_KERNEL_CASE(S64);
        INTERPOLATE_KERNEL_CASE(F32);
        INTERPOLATE_KERNEL_CASE(F64);
      default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unrecognised type for image: %x",
                image->type.type);
        return PS_INTERPOLATE_STATUS_ERROR;
    }

    return interpolateResult(interp, imageValue, varianceValue, maskValue, sumImage, sumVariance, sumBad,
                             sumKernel, sumKernel2, wantVariance, haveMask);
}


psImageInterpolateStatus interpolateJustWork(double *imageValue, double *varianceValue,
					     psImageMaskType *maskValue, float x, float y,
					     const psImageInterpolation *interp) {
  float u1,u2;
  int xl,xh;
  int yl,yh;
  const psImage *image = interp->image; // Image to interpolate

  xl = floor(x);
  if (xl < 0) { xl = 0; }
  if (xl >= image->numCols) {xl = image->numCols - 1; }
  xh = xl + 1;
  if (xh >= image->numCols) {xh = image->numCols - 1; }

  yl = floor(y);
  if (yl < 0) { yl = 0; }
  if (yl >= image->numRows) {yl = image->numRows - 1; }
  yh = yl + 1;
  if (yh >= image->numRows) {yh = image->numRows - 1; }

  if (imageValue && image) {
    if (image->data.F32[yl][xl] == image->data.F32[yl][xh]) {
      u1 = image->data.F32[yl][xh];
    }
    else {
      u1 = (xh - x) * image->data.F32[yl][xl] + (x - xl) * image->data.F32[yl][xh];
    }
    if (image->data.F32[yh][xl] == image->data.F32[yh][xh]) {
      u2 = image->data.F32[yh][xh];
    }
    else {
      u2 = (xh - x) * image->data.F32[yh][xl] + (x - xl) * image->data.F32[yh][xh];
    }
    if (u1 == u2) {
      *imageValue = u1;
    }
    else {
      *imageValue = (yh - y) * u1 + (y - yl) * u2;
    }
    if ((floor(x) >= image->numCols)||
	(floor(y) >= image->numRows)) {
      *imageValue = NAN;
    }
  }
#if (0)
  if (*imageValue == 0.0) {
    fprintf(stderr,"IJK: Zero!: %g %g [%d %d %d %d] %g %g (%g %g %g %g)\n",
	    x,y,xl,xh,yl,yh,u1,u2,
	    image->data.F32[yl][xl],image->data.F32[yl][xh],image->data.F32[yh][xl],image->data.F32[yh][xh]
	    );
  }
#endif
  return PS_INTERPOLATE_STATUS_GOOD;
}
  

psImageInterpolateStatus psImageInterpolate(double *imageValue, double *varianceValue,
                                            psImageMaskType *maskValue, float x, float y,
                                            const psImageInterpolation *interp)
{
    PS_ASSERT_PTR_NON_NULL(imageValue, PS_INTERPOLATE_STATUS_ERROR);
    PS_ASSERT_PTR_NON_NULL(interp, PS_INTERPOLATE_STATUS_ERROR);

    const psImage *image = interp->image; // Image to interpolate
    const psImage *mask = interp->mask; // Mask to interpolate
    const psImage *variance = interp->variance; // Variance to interpolate

    PS_ASSERT_IMAGE_NON_NULL(image, PS_INTERPOLATE_STATUS_ERROR);
    if (varianceValue && variance) {
        PS_ASSERT_IMAGE_NON_NULL(variance, PS_INTERPOLATE_STATUS_ERROR);
        PS_ASSERT_IMAGE_TYPE(variance, image->type.type, PS_INTERPOLATE_STATUS_ERROR);
        psAssert(image->numCols == variance->numCols && image->numRows == variance->numRows,
                 "Image and variance sizes");
    }
    if (maskValue && mask) {
        PS_ASSERT_IMAGE_NON_NULL(mask, PS_INTERPOLATE_STATUS_ERROR);
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, PS_INTERPOLATE_STATUS_ERROR);
        psAssert(image->numCols == mask->numCols && image->numRows == mask->numRows, "Image and mask sizes");
    }

    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(interp->poorFrac, 0.0, PS_INTERPOLATE_STATUS_ERROR);

    switch (interp->mode) {
      case PS_INTERPOLATE_FLAT:
        return interpolateFlat(imageValue, varianceValue, maskValue, x, y, interp);
      case PS_INTERPOLATE_BIQUADRATIC:
        return interpolateKernel(imageValue, varianceValue, maskValue, x, y, interp);
      case PS_INTERPOLATE_BILINEAR_SIMPLE:
	return interpolateJustWork(imageValue, varianceValue, maskValue, x, y, interp);
      case PS_INTERPOLATE_BILINEAR:
# if (!IS_BILIN_SEPARABLE)
	return interpolateKernel(imageValue, varianceValue, maskValue, x, y, interp);
# endif
      case PS_INTERPOLATE_GAUSS:
      case PS_INTERPOLATE_LANCZOS2:
      case PS_INTERPOLATE_LANCZOS3:
      case PS_INTERPOLATE_LANCZOS4:
        return interpolateSeparable(imageValue, varianceValue, maskValue, x, y, interp);
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified interpolation method (%d) is not supported."),
                interp->mode);
        return PS_INTERPOLATE_STATUS_ERROR;
    }

    psAbort("Should never reach here.");
    return PS_INTERPOLATE_STATUS_ERROR;
}


static float varianceFactorKernel(float x, float y, psImageInterpolateMode mode)
{
    int size = kernelSizes[mode];       // Size of kernel

    // Kernel basics
    INTERPOLATE_SETUP(x, y);
    xExact = yExact = false;
    if (xExact && yExact) { /* possible alternative */ }

# if (USE_MAX_KERNEL)
    psAssert (size <= MAX_KERNEL_SIZE, "oops");
    psF32 xKernel[MAX_KERNEL_SIZE], yKernel[MAX_KERNEL_SIZE]; // Interpolation kernels
    for (int ix = 0; ix < MAX_KERNEL_SIZE; ix++) { xKernel[ix] = 1; yKernel[ix] = 1; }
# else
    // init these arrays
    psF32 xKernel[size], yKernel[size]; // Interpolation kernels
    for (int ix = 0; ix < size; ix++) { xKernel[ix] = 1; yKernel[ix] = 1; }
# endif

    interpolationKernel(xKernel, xFrac, mode);
    interpolationKernel(yKernel, yFrac, mode);

    float xSumKernel = 0.0, ySumKernel = 0.0; // Sum of kernel
    float xSumKernel2 = 0.0, ySumKernel2 = 0.0; // Sum of kernel^2
    for (int i = 0; i < size; i++) {
        xSumKernel += xKernel[i];
        xSumKernel2 += PS_SQR(xKernel[i]);
        ySumKernel += yKernel[i];
        ySumKernel2 += PS_SQR(yKernel[i]);
    }

    return (xSumKernel2 * ySumKernel2) / PS_SQR(xSumKernel * ySumKernel);
}

float psImageInterpolateVarianceFactor(float x, float y, psImageInterpolateMode mode)
{
    switch (mode) {
      case PS_INTERPOLATE_FLAT:
      case PS_INTERPOLATE_BILINEAR_SIMPLE:
        // No smearing by design
        return 1.0;
      case PS_INTERPOLATE_BILINEAR:
      case PS_INTERPOLATE_BIQUADRATIC:
      case PS_INTERPOLATE_GAUSS:
      case PS_INTERPOLATE_LANCZOS2:
      case PS_INTERPOLATE_LANCZOS3:
      case PS_INTERPOLATE_LANCZOS4:
        return varianceFactorKernel(x, y, mode);
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified interpolation method (%d) is not supported."),
                mode);
        return NAN;
    }

    psAbort("Should never reach here.");
    return NAN;
}


psKernel *psImageInterpolationKernel(float x, float y, psImageInterpolateMode mode)
{
    int size = kernelSizes[mode];       // Size of kernel

    // Kernel basics
    INTERPOLATE_SETUP(x, y);
    xExact = yExact = false;
    if (xExact && yExact) { /* possible alternative */ }

# if (USE_MAX_KERNEL)
    psAssert (size <= MAX_KERNEL_SIZE, "oops");
    psF32 xKernel[MAX_KERNEL_SIZE], yKernel[MAX_KERNEL_SIZE]; // Interpolation kernels
    for (int ix = 0; ix < MAX_KERNEL_SIZE; ix++) { xKernel[ix] = 1; yKernel[ix] = 1; }
# else
    psF32 xKernel[size], yKernel[size]; // Interpolation kernels
    for (int ix = 0; ix < size; ix++) { xKernel[ix] = 1; yKernel[ix] = 1; }
# endif

    interpolationKernel(xKernel, xFrac, mode);
    interpolationKernel(yKernel, yFrac, mode);

    // Need to normalise the kernel, since that's what the interpolation does
    double xSum = 0.0, ySum = 0.0;
    for (int i = 0; i < size; i++) {
        xSum += xKernel[i];
        ySum += yKernel[i];
    }
    for (int i = 0; i < size; i++) {
        xKernel[i] /= xSum;
        yKernel[i] /= ySum;
    }

    int min = -size/2, max = (size - 1) / 2; // Range for kernel
    psKernel *kernel = psKernelAlloc(min, max, min, max); // Kernel to return

    fprintf (stderr, "kernel: %d x %d == %d\n", kernel->image->numRows, kernel->image->numCols, size);
    psAssert (size == kernel->image->numRows, "oops");
    psAssert (size == kernel->image->numCols, "oops");

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            kernel->image->data.F32[y][x] = yKernel[y] * xKernel[x];
        }
    }

    return kernel;
}


psImageInterpolateMode psImageInterpolateModeFromString(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, PS_INTERPOLATE_NONE);

    if (!strcasecmp(name, "FLAT"))     return PS_INTERPOLATE_FLAT;
    if (!strcasecmp(name, "BILINEAR")) return PS_INTERPOLATE_BILINEAR;
    if (!strcasecmp(name, "SIMPLEBILINEAR")) return PS_INTERPOLATE_BILINEAR_SIMPLE;
    if (!strcasecmp(name, "BIQUADRATIC")) return PS_INTERPOLATE_BIQUADRATIC;
    if (!strcasecmp(name, "GAUSS"))    return PS_INTERPOLATE_GAUSS;
    if (!strcasecmp(name, "LANCZOS2")) return PS_INTERPOLATE_LANCZOS2;
    if (!strcasecmp(name, "LANCZOS3")) return PS_INTERPOLATE_LANCZOS3;
    if (!strcasecmp(name, "LANCZOS4")) return PS_INTERPOLATE_LANCZOS4;

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Unknown interpolation type %s"), name);
    return PS_INTERPOLATE_NONE;
}

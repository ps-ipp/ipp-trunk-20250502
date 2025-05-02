/* @file  psImageConvolve.h
 *
 * @brief image convolution functionality
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.41 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-05 22:36:19 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_IMAGE_CONVOLVE_H
#define PS_IMAGE_CONVOLVE_H

/// @addtogroup ImageOps Image Operations
/// @{

#include "psImage.h"
#include "psVector.h"
#include "psType.h"
#include "psError.h"
#include "psAssert.h"

#define PS_TYPE_KERNEL PS_TYPE_F32     ///< the data member to use for kernel image */
#define PS_TYPE_KERNEL_DATA F32        ///< the data member to use for kernel image */
#define PS_TYPE_KERNEL_NAME "psF32"    ///< the data type for kernel as a string */

/// a structure to contain data related to image smoothing with a pre-cached 1D gauss kernel
typedef struct {
    int Nx;
    int Ny;
    int Nrange;
    psF32 *resultX;
    psF32 *resultY;
    psVector *kernel;
} psImageSmoothCacheData;

/// a structure to contain data related to image smoothing with a pre-cached 1D gauss kernel
typedef struct {
    float Nsigma;
    int Ns;				// number of pixel radii
    float *radflux;			// conv kernel in special positions
} psImageSmooth2dCacheData;

/// A convolution kernel
typedef struct {
    psImage *image;                    ///< Kernel data, in the form of an image
    int xMin;                          ///< Most negative x index
    int yMin;                          ///< Most negative y index
    int xMax;                          ///< Most positive x index
    int yMax;                          ///< Most positive y index
    float **kernel;                    ///< Pointer to the kernel data
    float **p_kernelRows;              ///< Pointer to the rows of the kernel data; not intended for user use.
} psKernel;

#define PS_ASSERT_KERNEL_NON_NULL(KERNEL, RETURNVALUE) \
    if ((KERNEL) == NULL || (KERNEL)->kernel == NULL) { \
        psError(PS_ERR_BAD_PARAMETER_NULL, true, \
                "Unallowable operation: psKernel %s or its data is NULL.", \
                #KERNEL); \
        return RETURNVALUE; \
    } \
    PS_ASSERT_IMAGE_NON_NULL((KERNEL)->image, RETURNVALUE);

#define PS_ASSERT_KERNELS_SIZE_EQUAL(KERNEL1, KERNEL2, RETURNVALUE) \
    if ((KERNEL1)->xMin != (KERNEL2)->xMin || \
        (KERNEL1)->xMax != (KERNEL2)->xMax || \
        (KERNEL1)->yMin != (KERNEL2)->yMin || \
        (KERNEL1)->yMax != (KERNEL2)->yMax) { \
        psError(PS_ERR_BAD_PARAMETER_NULL, true, \
                "Unallowable operation: Kernels %s and %s are not the same size.", \
                #KERNEL1, #KERNEL2); \
        return RETURNVALUE; \
    } \
    PS_ASSERT_IMAGES_SIZE_EQUAL((KERNEL1)->image, (KERNEL2)->image, RETURNVALUE);

/// Allocates a convolution kernel of the given range
///
/// In order to perform a convolution, we need to define the convolution
/// kernel. We need a more general object than a psImage so that we can
/// incorporate the offset from the (0, 0) pixel to the (0, 0) value of the
/// kernel. It might be convenient to allow both positive and negative
/// indices to convey the positive and negative shifts. One might consider
/// setting the x0 and y0 members of a psImage to the appropriate offsets,
/// but this is not the purpose of these members, and doing so may affect the
/// behavior of other psImage operations.
///
/// This construction allows the kernel member to use negative indices, while
/// preserving the location of psMemBlocks relative to allocated memory.
///
/// The maximum extent of the kernel shifts shall be defined by the xMin,
/// xMax, yMin and yMax members. Note that xMin and yMin, under normal
/// circumstances, should be negative numbers. That is,
/// myKernel->kernel[-3][-2] may be defined if yMin and xMin are equal to or
/// more negative than -3 and -2, respectively.
///
/// In the event that one of the minimum values is greater than the
/// corresponding maximum value, the function shall generate a warning, and
/// the offending values shall be exchanged.
///
/// @return psKernel*          A new kernel object
///
#ifdef DOXYGEN
psKernel *psKernelAlloc(
    int xMin,                          ///< Most negative x index
    int xMax,                          ///< Most positive x index
    int yMin,                          ///< Most negative y index
    int yMax                           ///< Most positive y index
);
#else // ifdef DOXYGEN
psKernel *p_psKernelAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    int xMin,                          ///< Most negative x index
    int xMax,                          ///< Most positive x index
    int yMin,                          ///< Most negative y index
    int yMax                           ///< Most positive y index
) PS_ATTR_MALLOC;
#define psKernelAlloc(xMin, xMax, yMin, yMax)				\
    p_psKernelAlloc(__FILE__, __LINE__, __func__, (xMin), (xMax), (yMin), (yMax))
#endif // ifdef DOXYGEN

/// Allocate a convolution kernel from a provided image
psKernel *psKernelAllocFromImage(psImage *image, ///< Image from which to define kernel
                                 int x0, int y0 ///< Coordinates of kernel centre
    );

/// Copy a kernel
///
/// Performs a deep copy of the input kernel
psKernel *psKernelCopy(
    const psKernel *in                  ///< Kernel to be copied
    );

/// Checks the type of a particular pointer.
///
/// Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
///
/// @return bool:       True if the pointer matches a psKernel structure, false otherwise.
///
bool psMemCheckKernel(
    psPtr ptr                          ///< the pointer whose type to check
);


/// Generates a kernel given a list of shift values
///
/// Given a list of values (e.g., shifts made in the course of OT guiding),
/// psKernelGenerate shall return the appropriate kernel.  The vectors xShifts
/// and yShifts, which are a list of shifts relative to some starting point,
/// will be supplied by the user. The elements of the vectors should be of an
/// integer type; otherwise the values shall be truncated to integers. The
/// output kernel shall be normalized such that the sum over the kernel is
/// unity.
///
/// If the vectors are not of the same number of elements, then the function
/// shall generate a warning shall be generated, following which, the longer
/// vector trimmed to the length of the shorter, and the function shall continue.
///
/// @return psKernel*    new Kernel object
///
psKernel *psKernelGenerate(
    const psVector *tShifts,            ///< list of time shifts (F32)
    const psVector *xShifts,            ///< list of x-axis shifts (S32)
    const psVector *yShifts,            ///< list of y-axis shifts (S32)
    float totalTime,                    ///< Total time (relative times if negative or zero)
    bool xyRelative                     ///< Are x,y positions relative (shifts) or absolute?
);

/// Truncate a kernel
///
/// Truncates the outer parts of the kernel where the contribution is below the nominated fraction of the
/// total kernel.
bool psKernelTruncate(
    psKernel *in,                       ///< Kernel to be truncated
    float frac                          ///< Fraction for truncation threshold
    );


/// Convolve an image with a kernel, using a direct convolution
///
/// This is appropriate for small kernels, where there is no time saving to use FFT method.
///
/// @return psImage*  resulting image
///
psImage *psImageConvolveDirect(
    psImage *out,                       ///< Output image, or NULL
    const psImage *in,                  ///< Image to convolve
    const psKernel *kernel              ///< kernel to colvolve with
);

/// Convolve a mask image with a kernel
///
/// Returns a mask, grown by the supplied convolution bounds.  Only those pixels specified by the maskVal are
/// grown, being ORed with setVal; the rest are simply propagated.  If setVal is zero, uses maskVal; note that
/// the mode of growing individual bits in maskVal is NOT supported because this algorithm does not enable it.
psImage *psImageConvolveMask(psImage *out, ///< Output image, or NULL
                             const psImage *mask, ///< Mask to convolve
                             psImageMaskType maskVal, ///< Mask value to convolve
                             psImageMaskType setVal, ///< Mask value to set; 0 to propagate maskVal
                             int xMin, int xMax, int yMin, int yMax ///< Convolution bounds
    );

/// Convolve a mask image with a kernel, using direct convolution
///
/// Returns a mask, grown by the supplied convolution bounds.  Only those pixels specified by the maskVal are
/// grown, being ORed with setVal; the rest are simply propagated.  If setVal is zero, then individual bits
/// matching maskVal are grown.
psImage *psImageConvolveMaskDirect(psImage *out, ///< Output image, or NULL
                                   const psImage *mask, ///< Mask to convolve
                                   psImageMaskType maskVal, ///< Mask value to convolve
                                   psImageMaskType setVal, ///< Mask value to set; 0 to propagate maskVal
                                   int xMin, int xMax, int yMin, int yMax ///< Convolution bounds
    );

/// Convolve a mask image with a kernel, using the FFT
///
/// Returns a mask, grown by the supplied convolution bounds.  Only those pixels specified by the maskVal are
/// grown, being ORed with setVal; the rest are simply propagated.  If setVal is zero, uses maskVal; note that
/// the mode of growing individual bits in maskVal is NOT supported because this algorithm does not enable it.
/// Uses psImageConvolveFFT to convolve those pixels which are masked, and then thresholds at the specified
/// level.
psImage *psImageConvolveMaskFFT(psImage *out, ///< Output image, or NULL
                                const psImage *mask, ///< Mask to convolve
                                psImageMaskType maskVal, ///< Mask value to convolve
                                psImageMaskType setVal, ///< Mask value to set; 0 to use maskVal
                                int xMin, int xMax, int yMin, int yMax, ///< Convolution bounds
                                float thresh ///< Threshold (0..1) for convolved floating-point image
    );

/// Smooths an image by parts using 1D Gaussian independently in x and y.
///
/// Applies a circularly symmetric Gaussian smoothing first in x and then in y
/// directions with just a vector.  This process is 2N faster than 2D convolutions (in general).
///
/// @return bool        TRUE if successful, otherwise FALSE
///
bool psImageSmooth(
    psImage *image,                    ///< the image to be smoothed
    double  sigma,                     ///< the width of the smoothing kernel in pixels
    double  Nsigma                     ///< the size of the smoothing box in sigmas
);

/// Return the kernel used for smoothing
psKernel *psImageSmoothKernel(
    float sigma,                        ///< Width of the smoothing kernel, pixels
    float nSigma                        ///< Size of the smoothing box, sigma
    );

/// Smooth an imageby parts using 1D Gaussian independently in x and y, allowing for
/// MASKED PIXELS
///
/// Applies a circularly symmetric Gaussian smoothing first in x and then in y
/// directions with just a vector.  This process is 2N faster than 2D convolutions (in general).
psImage *psImageSmoothMask(
    psImage *output,                    ///< Output image, or NULL
    const psImage *image,               ///< Input image (F32 or F64)
    const psImage *mask,                ///< Mask image
    psImageMaskType maskVal,            ///< Mask value
    float sigma,                        ///< Width of the smoothing kernel (pixels)
    float numSigma,                     ///< Size of the smoothing box (sigma)
    float minGauss                      ///< Minimum fraction of Gaussian to accept
    );

/// Smooth particular pixels on an image, allowing for MASKED PIXELS
///
/// Applies a circularly symmetric Gaussian smoothing first in x and then in y
/// directions with just a vector.  This process is 2N faster than 2D convolutions (in general).
psVector *psImageSmoothMaskPixels(
    const psImage *image,               ///< Input image (F32)
    const psImage *mask,                ///< Mask image
    psImageMaskType maskVal,            ///< Value to mask
    const psVector *x,                  ///< x coordinates
    const psVector *y,                  ///< y coordinates
    float sigma,                        ///< Width of the smoothing kernel (pixels)
    float numSigma,                     ///< Size of the smoothing box (sigma)
    float minGauss                      ///< Minimum fraction of Gaussian to accept
    );

/// Smooth an image by parts using 1D Gaussian independently in x and y, allowing for
/// MASKED PIXELS : THREADED VERSION
///
/// Applies a circularly symmetric Gaussian smoothing first in x and then in y
/// directions with just a vector.  This process is 2N faster than 2D convolutions (in general).
psImage *psImageSmoothMask_Threaded(psImage *output,
                                    const psImage *image,
                                    const psImage *mask,
                                    psImageMaskType maskVal,
                                    float sigma,
                                    float numSigma,
                                    float minGauss);

/// Smooth an image by parts using 1D Gaussian independently in x and y, allowing for
/// MASKED PIXELS : THREADED VERSION
///
/// Applies a circularly symmetric Gaussian smoothing first in x and then in y
/// directions with just a vector.  This process is 2N faster than 2D convolutions (in general).
psImage *psImageSmoothNoMask_Threaded(psImage *output,
                                    const psImage *image,
                                    float sigma,
                                    float numSigma,
                                    float minGauss);

/// Smooth an image by parts IN-SITU using 1D Gaussian independently in x and y, allowing
/// for MASKED PIXELS
bool psImageSmoothMaskF32(
    psImage *image,                    ///< the image to be smoothed
    psImage *mask,                     ///< optional mask
    psImageMaskType maskVal,            ///< masked bits
    double  sigma,                     ///< the width of the smoothing kernel in pixels
    double  Nsigma                     ///< the size of the smoothing box in sigmas
);

psImageSmoothCacheData *psImageSmoothCacheAlloc (psImage *image, double sigma, double Nsigma);
bool psImageSmoothCache_F32(psImage *image, psImageSmoothCacheData *smdata);
bool psImageSmoothCacheKernel_Gauss (psImageSmoothCacheData *smdata, float sigma);

/// Control threading for image convolution functions
///
/// Returns old threading status
bool psImageConvolveSetThreads(bool threaded ///< Run image convolution threaded?
    );

/// Return whether image convolution functions are threaded
bool psImageConvolveGetThreads(void);

psImageSmooth2dCacheData *psImageSmooth2dCacheAlloc (float Nsigma);
bool psImageSmooth2dCacheKernel_PS1_V1 (psImageSmooth2dCacheData *smdata, float sigma, float kappa);
bool psImageSmooth2dCacheKernel_Gauss (psImageSmooth2dCacheData *smdata, float sigma);
bool psImageSmooth2dCache_F32(psImage *image, psImageSmooth2dCacheData *smdata);

/// @}
#endif // #ifndef PS_IMAGE_CONVOLVE_H

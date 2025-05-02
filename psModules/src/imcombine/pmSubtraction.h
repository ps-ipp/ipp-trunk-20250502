/* @file pmSubtraction.h
 *
 * PSF-matched image subtraction, based on the Alard & Lupton (1998) and Alard (2000) methods.
 *
 * @author Paul Price, IfA
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.36 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:25 $
 * Copyright 2004-207 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_SUBTRACTION_H
#define PM_SUBTRACTION_H

// #include <pslib.h>
// #include <pmHDU.h>
// #include <pmFPA.h>
// #include <pmSubtractionKernels.h>
// #include <pmSubtractionStamps.h>

// if we use the original ppSub implementation, we subtract a central delta-function for all
// kernels to for a zero integral.  in the Alard-Lupton style zeroing, we can either subtract a
// Gaussian from all kernels (ZERO_KERNEL_ZERO_FLUX), or we can subtract it from all but the
// first kernel.
# define CENTRAL_DELTA 0
# define ZERO_KERNEL_ZERO_FLUX 1

/// @addtogroup imcombine Image Combinations
/// @{

/// Number of terms in a polynomial
#define PM_SUBTRACTION_POLYTERMS(ORDER) (((ORDER) + 1) * ((ORDER) + 2) / 2)

/// Set the indices for the normalisation and background terms
#define PM_SUBTRACTION_INDICES(NORM,BG,KERNELS) { \
    int numSpatial = PM_SUBTRACTION_POLYTERMS((KERNELS)->spatialOrder); /* Number of spatial terms */ \
    NORM = (KERNELS)->num * numSpatial; \
    BG = NORM + 1; \
}

/// Return the index for the start of the normalisation terms
#define PM_SUBTRACTION_INDEX_NORM(KERNELS) \
    ((KERNELS)->num * PM_SUBTRACTION_POLYTERMS((KERNELS)->spatialOrder))

/// Return the index for the start of the background terms
#define PM_SUBTRACTION_INDEX_BG(KERNELS) \
    (((KERNELS)->num * PM_SUBTRACTION_POLYTERMS((KERNELS)->spatialOrder)) + 1)


/// Convolve the reference stamp with the kernel components
bool pmSubtractionConvolveStamp(pmSubtractionStamp *stamp, ///< Stamp to convolve
                                pmSubtractionKernels *kernels, ///< Kernel parameters
                                int footprint ///< Half-size of region over which to calculate equation
    );

bool pmSubtractionConvolveStamps(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels);

bool pmSubtractionConvolveStampThread(psThreadJob *job);

/// Reject stamps
int pmSubtractionRejectStamps(pmSubtractionKernels *kernels, ///< Kernel parameters to update
                              pmSubtractionStampList *stamps, ///< Stamps
                              pmSubtractionQuality *match, ///< data on the subtraction quality
                              psImage *subMask, ///< Subtraction mask
                              float sigmaRej ///< Number of RMS deviations above zero at which to reject
    );

/// Generate the convolution kernel
psKernel *pmSubtractionKernel(const pmSubtractionKernels *kernels, ///< Kernel parameters
                              float x, float y, ///< Normalised position [-1,1] for which to generate image
                              bool wantDual ///< Calculate for the dual kernel?
    );

/// Generate an image of the convolution kernel
psImage *pmSubtractionKernelImage(const pmSubtractionKernels *kernels, ///< Kernel parameters
                                  float x, float y,///< Normalised position [-1,1] for which to generate image
                                  bool wantDual ///< Calculate for the dual kernel?
                                  );

/// Return the variance factor for a kernel
///
/// The variance factor allows conversion from the large-scale variance (which is what is calculated by
/// pmSubtractionConvolve) and the small-scale (pixel-to-pixel) variance.
float pmSubtractionVarianceFactor(const pmSubtractionKernels *kernels, ///< Kernel parameters
                                  float x, float y, ///< Normalised position [-1,1]
                                  bool wantDual ///< Calculate for the dual kernel?
    );

/// Generate images of the convolution kernel elements
psArray *pmSubtractionKernelSolutions(const pmSubtractionKernels *kernels, ///< Kernel parameters
                                      float x, float y, ///< Normalised position [-1,1] for images
                                      bool wantDual ///< Calculate for the dual kernel?
    );


/// Execute a thread job to convolve a patch of the image
bool pmSubtractionConvolveThread(psThreadJob *job ///< Job to execute
    );

/// Convolve image in preparation for subtraction
bool pmSubtractionConvolve(pmReadout *out1, ///< Output image 1
                           pmReadout *out2, ///< Output image 2 (DUAL mode only)
                           const pmReadout *ro1, // Input image 1
                           const pmReadout *ro2, // Input image 2
                           psImage *subMask, ///< Subtraction mask (or NULL)
                           int stride,  ///< Size of convolution patches
                           psImageMaskType maskBad, ///< Mask value to give bad pixels
                           psImageMaskType maskPoor, ///< Mask value to give poor pixels
                           float poorFrac, ///< Fraction for "poor"
                           float kernelError, ///< Relative systematic error in kernel
                           float covarFrac,  ///< Truncation fraction for kernel before covariance calculation
                           const psRegion *region, ///< Region to convolve (or NULL)
                           const pmSubtractionKernels *kernels, ///< Kernel parameters
                           bool doBG,   ///< Apply background term?
                           bool useFFT  ///< Use Fast Fourier Transform for the convolution?
    );

/// Generate the convolution of an image, given a precalculated kernel
///
/// The 'image' is a kernel for convenience --- intended to be a stamp
psKernel *p_pmSubtractionConvolveStampPrecalc(const psKernel *image, ///< Image to convolve
                                              const psKernel *kernel ///< Kernel by which to convolve
    );

/// Return normalised coordinates
void p_pmSubtractionPolynomialNormCoords(
    float *xOut, float *yOut,           ///< Normalised coordinates, returned
    float xIn, float yIn,               ///< Input coordinates
    int xMin, int xMax, int yMin, int yMax ///< Bounds of validity
    );

/// Given (normalised) coordinates (x,y), generate a matrix where the elements (i,j) are x^i * y^j
psImage *p_pmSubtractionPolynomial(psImage *output, ///< Output matrix, or NULL
                                   int spatialOrder, ///< Maximum spatial polynomial order
                                   float x, float y ///< Normalised position of interest, [-1,1]
    );

/// Given pixel coordinates (x,y), generate a matrix where the elements (i,j) are x^i * y^j
///
/// Same as p_pmSubtractionPolynomial except that the normalisation is applied
psImage *p_pmSubtractionPolynomialFromCoords(psImage *output, ///< Output matrix, or NULL
                                             const pmSubtractionKernels *kernels, ///< Kernel parameters
                                             int x, int y ///< Position of interest
    );

/// Return the radius from the centre of the convolution kernel that distinguishes "bad" and "poor" pixels
int p_pmSubtractionBadRadius(psKernel *preKernel, ///< Pre-calculated convolution kernel
                             const pmSubtractionKernels *kernels, ///< Kernel parameters
                             const psImage *polyValues, ///< Polynomial values
                             bool wantDual, ///< Calculate for the dual kernel?
                             float poorFrac ///< Fraction for "poor"
    );

bool pmSubtractionGetFWHMs(float *fwhm1, float *fwhm2);
bool pmSubtractionSetFWHMs(float fwhm1, float fwhm2);

pmSubtractionQuality *pmSubtractionQualityAlloc();

/// @}
#endif

#ifndef PM_SUBTRACTION_MATCH_H
#define PM_SUBTRACTION_MATCH_H

#include <pslib.h>

#include <pmHDU.h>
#include <pmFPA.h>
#include <pmSubtractionKernels.h>
#include <pmSubtractionStamps.h>
#include <pmSubtraction.h>

/// Match two images
bool pmSubtractionMatch(pmReadout *conv1, ///< Output convolved data for image 1
                        pmReadout *conv2, ///< Output convolved data for image 2
                        const pmReadout *ro1, ///< Image 1
                        const pmReadout *ro2, ///< Image 2
                        // Stamp parameters
                        int footprint,  ///< Stamp half-size
                        int stride,     ///< Size for convolution patches
                        float regionSize, ///< Typical size of iso-kernel regions
                        float stampSpacing, ///< Typical spacing between stamps
                        float threshold, ///< Threshold for stamps
                        const psArray *sources, ///< Sources for stamps
                        const char *stampsName, ///< Filename for stamps
                        // Kernel parameters
                        pmSubtractionKernelsType type, ///< Kernel type
                        int size,       ///< Kernel half-size
                        int order,      ///< Spatial polynomial order
                        psVector *widths, ///< ISIS Gaussian widths
                        const psVector *orders, ///< ISIS Polynomial orders
                        int inner,      ///< Inner radius for various kernel types
                        int ringsOrder, ///< RINGS polynomial order
                        int binning,    ///< SPAM kernel binning
                        float penalty,  ///< Penalty for wideness
                        bool optimum,   ///< Search for optimum ISIS kernel?
                        const psVector *optFWHMs, ///< FWHMs for optimum search
                        int optOrder,   ///< Maximum order for optimum search
                        float optThreshold, ///< Threshold for optimum search (0..1)
                        // Operational parameters
                        int iter,       ///< Rejection iterations
                        float rej,      ///< Rejection threshold
                        float normFrac, ///< Fraction of flux in window for normalisation window
                        float sysError, ///< Relative systematic error in images
                        float skyError, ///< Relative systematic error in images
                        float kernelError, ///< Relative systematic error in kernel
                        float covarFrac,   ///< Fraction for kernel truncation before covariance calculation
                        psImageMaskType maskVal, ///< Value to mask for input
                        psImageMaskType maskBad, ///< Mask for output bad pixels
                        psImageMaskType maskPoor, ///< Mask for output poor pixels
                        float poorFrac, ///< Fraction for "poor"
                        float badFrac,   ///< Maximum fraction of bad input pixels to accept
                        pmSubtractionMode mode ///< Mode of subtraction; may be modified
    );

/// Match two images using precalculated kernel
bool pmSubtractionMatchPrecalc(pmReadout *conv1, ///< Output convolved data for image 1
                               pmReadout *conv2, ///< Output convolved data for image 2
                               const pmReadout *ro1, ///< Image 1
                               const pmReadout *ro2, ///< Image 2
                               psMetadata *analysis, ///< Analysis metadata with pre-calculated kernel, region
                               int stride, ///< Size for convolution patches
                               float kernelError, ///< Relative systematic error in kernel
                               float covarFrac,   ///< Fraction for kernel truncation before covariance calc.
                               psImageMaskType maskVal, ///< Value to mask for input
                               psImageMaskType maskBad, ///< Mask for output bad pixels
                               psImageMaskType maskPoor, ///< Mask for output poor pixels
                               float poorFrac, ///< Fraction for "poor"
                               float badFrac ///< Maximum fraction of bad input pixels to accept
    );

/// Execute a thread job to measure the PSF width ratios
bool pmSubtractionOrderThread(psThreadJob *job ///< Job to execute
    );

/// Measure the PSF width ratio for a single stamp
bool pmSubtractionOrderStamp(psVector *ratios, ///< PSF width ratios
                             psVector *mask, ///< Mask for PSF width ratios
                             const pmSubtractionStampList *stamps, ///< List of stamps
                             const psArray *models, ///< Pre-calculated gaussian models
                             const psVector *modelSums, ///< Pre-calculated gaussian model sums
                             int index, ///< Index of stamp
                             float bg1, ///< Background for image 1
                             float bg2  ///< Background for image 2
    );

/// Determine which image to convolve
pmSubtractionMode pmSubtractionOrder(pmSubtractionStampList *stamps, ///< Stamps that have been extracted
                                     float bg1, float bg2 ///< Background for each image
    );

/// Determine best subtraction mode to use
///
/// Subtractions are attempted each way, and the mode with the lower residual is taken to be the best
pmSubtractionMode pmSubtractionBestMode(
    pmSubtractionStampList **stamps,    ///< Stamps to use for solution
    pmSubtractionKernels **kernels,     ///< Kernels to use for solution
    const psImage *subMask,             ///< Subtraction mask
    float rej                           ///< Rejection threshold for stamps
    );


/// Scale subtraction parameters according to the FWHMs of the inputs
// bool pmSubtractionParamsScale(
//     int *kernelSize,                    ///< Half-size of the kernel
//     int *stampSize,                     ///< Half-size of the stamp (footprint)
//     psVector *widths,                   ///< ISIS widths
//     float scaleRef,                     ///< Reference width for scaling
//     float scaleMin,                     ///< Minimum scaling ratio, or NAN
//     float scaleMax                      ///< Maximum scaling ratio, or NAN
//     );

bool pmSubtractionParamsScale(int *kernelSize, int *stampSize, psVector *widths, float fwhm1, float fwhm2);

bool pmSubtractionParamScaleOptions(bool scale, float scaleRef, float scaleMin, float scaleMax);

bool pmSubtractionMatchAttempt(
    pmSubtractionQuality **bestMatch,
    pmSubtractionKernels *kernels, 
    pmSubtractionStampList *stamps, 
    pmSubtractionMode mode, 
    int spatialOrder, 
    bool final
    );

#endif

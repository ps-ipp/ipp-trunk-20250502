/* @file psImageCovariance.h
 *
 * @brief Calculations involving image covariance
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-13 21:47:42 $
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_IMAGE_COVARIANCE_H
#define PS_IMAGE_COVARIANCE_H

/// @addtogroup ImageOps Image Operations
/// @{

#include <psImageConvolve.h>

// We don't carry the entire covariance matrix for an image (the size goes as N^2, for N pixels, which makes
// storage difficult; and if that's not enough, the time to do the calculation is definitely impractical).
// Since there are (generally) lots of zeros in the covariance matrix, and the same basic pattern repeats (for
// background pixels), we can just carry that pattern ("covariance pseudo-matrix").  We carry this in a
// psKernel, since the values are the covariance between the pixel of consideration (at 0,0 in the kernel) and
// neighbouring pixels.  Note that this may not be strictly correct near sources, but this is the best we can
// do (and much better than most currently do).

/// Allocate a covariance pseudo-matrix with no covariance
psKernel *psImageCovarianceNone(void);

/// Calculate the covariance pseudo-matrix for a convolution kernel
psKernel *psImageCovarianceCalculate(
    const psKernel *kernel,             ///< Convolution kernel
    const psKernel *covariance          ///< Current covariance pseudo-matrix
    );

/// Calculate the covariance pseudo-matrix for binning
psKernel *psImageCovarianceBin(
    int bin,                            ///< Binning factor
    const psKernel *covariance,         ///< Current covariance pseudo-matrix
    bool average                        ///< Averaging pixels when binning?
    );

/// Return the pixel-to-pixel covariance factor
float psImageCovarianceFactor(
    const psKernel *covariance          ///< Covariance pseudo-matrix
    );

/// Return the pixel-to-pixel covariance factor following calculation
///
/// This doesn't require calculation of the entire covariance matrix, so is much faster.
float psImageCovarianceCalculateFactor(
    const psKernel *kernel,             ///< Convolution kernel
    const psKernel *covariance          ///< Current covariance pseudo-matrix
    );


/// Return the covariance factor for an aperture of a given radius
float psImageCovarianceFactorForAperture(const psKernel *covar, float radius);

/// Return the sum of the covariance pseudo-matrix
float psImageCovarianceSum(
    const psKernel *covariance          ///< Covariance pseudo-matrix
    );

/// Average multiple covariance pseudo-matrices
psKernel *psImageCovarianceAverage(
    const psArray *array                ///< Array of covariance pseudo-matrices
    );

/// Weighted average of multiple covariance pseudo-matrices
psKernel *psImageCovarianceAverageWeighted(
    const psArray *array,               ///< Array of covariance pseudo-matrices
    const psVector *weights             ///< Weights for each (F32)
    );

/// Truncate covariance pseudo-matrix
///
/// The covariance pseudo-matrix is truncated by removing the outer regions that contribute less than the
/// nominated fraction of the total.
psKernel *psImageCovarianceTruncate(
    const psKernel *covar,              ///< Covariance pseudo-matrix
    float frac                          ///< Fraction of covariance to truncate
    );

/// Transfer covariance factor from covariance pseudo-matrix to variance map
bool psImageCovarianceTransfer(
    psImage *variance,                  ///< Variance map to which to transfer
    psKernel *covar                     ///< Covariance pseudo-matrix from which to transfer
    );


/// Rescale a covariance matrix following a change in plate scale
///
/// The covariance matrix is stretched or shrunk to match the new plate scale.
psKernel *psImageCovarianceScale(
    const psKernel *in,                 ///< Input covariance pseudo-matrix
    float scale                         ///< Scale factor (output plate scale relative to input plate scale)
    );

/// Control threading for image covariance functions
///
/// Returns old threading status
bool psImageCovarianceSetThreads(bool threaded ///< Run image covariance functions threaded?
    );

/// Return whether image covariance functions are threaded
bool psImageCovarianceGetThreads(void);

/// @}
#endif // #ifndef PS_IMAGE_COVARIANCE_H

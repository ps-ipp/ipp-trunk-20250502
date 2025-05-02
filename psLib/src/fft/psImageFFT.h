/// @file  psImageFFT.h
/// @brief Contains FFT transform related functions for psImage
///
/// @author Paul Price, IfA
/// @author Robert DeSonia, MHPCC
///
/// @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
/// @date $Date: 2009-01-27 06:39:37 $
/// Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
///

#ifndef PS_IMAGE_FFT_H
#define PS_IMAGE_FFT_H

#include "psImage.h"
#include "psImageConvolve.h"

typedef struct {
    void *fft;
    int xMin;                          ///< Most negative x index
    int yMin;                          ///< Most negative y index
    int xMax;                          ///< Most positive x index
    int yMax;                          ///< Most positive y index
    int paddedCols;
    int paddedRows;
} psKernelFFT;

/// @addtogroup MathOps Mathematical Operations
/// @{

/// Forward FFT of an image
///
/// Applies a forward FFT (exponent -1), with the result returned in both real and imaginary parts.  The FFT
/// is not normalised (a forward followed by a reverse is the original scaled by the total number of pixels).
/// The FFT takes advantage of the fact that the input is purely real; hence the output number of columns is
/// numCols/2 + 1 (with division rounding down).  Only implemented for F32 input.
bool psImageForwardFFT(psImage **real,  ///< Real part of FFT
                       psImage **imag,  ///< Imaginary part of FFT
                       const psImage *in///< Input image (F32)
    );

/// Backward FFT of an image
///
/// Applies a backward FFT (exponent +1) from the real and imaginary parts with the (purely) real result
/// returned.  The FFT is not normalised (a forward followed by a reverse is the original scaled by the total
/// number of pixels).  The FFT takes advantage of the fact that the output will be purely real; hence the
/// input number of columns is numCols/2 + 1 (with division rounding down); to manage the redundancy (is the
/// original number of columns even or odd?), we need the original number of columns (the number of columns of
/// the image that was input to psImageForwardFFT) to be provided.  Only implemented for F32 input.
bool psImageBackwardFFT(psImage **out,///< Output image
                        const psImage *real, ///< Real input (F32)
                        const psImage *imag, ///< Imaginary input (F32)
                        int origCols    ///< Original number of columns
    );


/// Power spectrum of an image
///
/// Generates the power spectrum of an image.  Only implemented for F32 input.
psImage* psImagePowerSpectrum(psImage *out, const psImage *in);

/// Multiply complex images
///
/// The input images are the real and imaginary parts of each of two images.  The real and imaginary parts
/// of the output image are returned.  Only implemented for F32 input.
bool psImageComplexMultiply(psImage **outReal, ///< Real part of output
                            psImage **outImag, ///< Imaginary part of output
                            const psImage *in1Real, ///< Real part of input 1
                            const psImage *in1Imag, ///< Imaginary part of input 1
                            const psImage *in2Real, ///< Real part of input 2
                            const psImage *in2Imag ///< Imaginary part of input 2
    );

/// Convolve an image with a kernel, using the FFT
///
/// This is appropriate for larger kernels, where the direct convolution is slow.  The input image and kernel
/// are suitably padded to avoid wrap-around effects.
psImage *psImageConvolveFFT(
    psImage *out,                       ///< Output image, or NULL
    const psImage *in,                  ///< Image to convolve
    const psImage *mask,                ///< Corresponding mask
    psImageMaskType maskVal,		///< Value to mask
    const psKernel *kernel              ///< kernel to convolve with
    );

/// Convolve an image with a kernel, using the FFT, applying a window function
///
/// This is appropriate for larger kernels, where the direct convolution is slow.  The input image and kernel
/// are suitably padded to avoid wrap-around effects.
psImage *psImageConvolveFFTwithWindow(
    psImage *out,                       ///< Output image, or NULL
    const psImage *in,                  ///< Image to convolve
    const psImage *mask,                ///< Corresponding mask
    psImageMaskType maskVal,		///< Value to mask
    const psKernel *kernel              ///< kernel to convolve with
    );

/// Allocate an the psKernelFFT structure
psKernelFFT *psKernelFFTAlloc(
    const psKernel *input		///< kernel to convolve with
);

/// Generate an FFT'ed kernel suitable for convolution with the given image
psKernelFFT *psImageConvolveKernelInit(
    const psImage *in,			///< representative image to convolve
    const psKernel *kernel		///< kernel to convolve with
    );

/// Convolve the given image with the pre-FFT'ed kernel
psImage *psImageConvolveKernel(
    psImage *out,			///< Output image, or NULL
    const psImage *in,			///< Image to convolve
    const psImage *mask,		///< Corresponding mask
    psImageMaskType maskVal,		///< Value to mask
    const psKernelFFT *kernel		///< kernel to convolve with
    );

/// @}
#endif // #ifndef PS_IMAGE_FFT_H

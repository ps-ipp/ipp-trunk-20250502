/// @file  psImageFFT.c
///
/// @brief Contains FFT transform related functions for psImage.
///
/// @author Paul Price, IfA
/// @author Robert DeSonia, MHPCC
///
/// @version $Revision: 1.29 $ $Name: not supported by cvs2svn $
/// @date $Date: 2009-01-27 06:39:37 $
///
/// Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
///

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>
#include <limits.h>
#include <pthread.h>

#include "psAbort.h"
#include "psAssert.h"
#include "psError.h"
#include "psMemory.h"
#include "psLogMsg.h"
#include "psConstants.h"
#include "psImageStructManip.h"
#include "psImageConvolve.h"
#include "psThread.h"
#include "psFFT.h"
#include "psImageFFT.h"

#define FFT_CONVOLVE_BINARY_SIZE 1               // Scale up to next binary size

// Lock FFTW access
#define FFTW_LOCK \
if (threaded) { \
    psFFTLock(); \
}
// Unlock FFTW access
#define FFTW_UNLOCK \
if (threaded) { \
    psFFTUnlock(); \
}

#define FFTW_PLAN_RIGOR FFTW_ESTIMATE   // How rigorous the FFTW planning is

static psBool fftwWisdomImported = false; // Has the FFTW wisdom been imported yet?

bool psImageForwardFFT(psImage **real, psImage **imag, const psImage *in)
{
    PS_ASSERT_IMAGE_NON_NULL(in, false);
    PS_ASSERT_IMAGE_TYPE(in, PS_TYPE_F32, false);
    PS_ASSERT_PTR_NON_NULL(real, false);
    PS_ASSERT_PTR_NON_NULL(imag, false);

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    // Make sure the system-level wisdom information is imported.
    FFTW_LOCK;
    if (!fftwWisdomImported) {
        fftwf_import_system_wisdom();
        fftwWisdomImported = true;
    }
    FFTW_UNLOCK;

    int numCols = in->numCols;          // Number of columns
    int numRows = in->numRows;          // Number of rows

    psF32 *input;                       // Input data, for FFTW
    if (!in->parent) {
        // No parent --- can just use the data buffer
        input = psMemIncrRefCounter(in->p_rawDataBuffer);
    } else {
	// Need to copy the data
	input = psAlloc(numCols * numRows * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        for (int y = 0; y < numRows; y++) {
            memcpy(&input[y * numRows], in->data.F32[y], numCols);
        }
    }

    // Do the FFT

    FFTW_LOCK;
    fftwf_complex *out = fftwf_malloc((numCols/2 + 1) * numRows * sizeof(fftwf_complex)); // Output data
    fftwf_plan plan = fftwf_plan_dft_r2c_2d(numRows, numCols, input, out, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(plan);

    FFTW_LOCK;
    fftwf_destroy_plan(plan);
    FFTW_UNLOCK;

    psFree(input);

    // Pull the real and imaginary parts out
    numCols = numCols/2 + 1;            // x dimension is halved by FFTW
    *real = psImageRecycle(*real, numCols, numRows, PS_TYPE_F32);
    *imag = psImageRecycle(*imag, numCols, numRows, PS_TYPE_F32);
    for (int y = 0, index = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; index++, x++) {
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
            // C99 complex support
            (*real)->data.F32[y][x] = creal(out[index]);
            (*imag)->data.F32[y][x] = cimag(out[index]);
#else
            // FFTW's backup complex support
            (*real)->data.F32[y][x] = out[index][0];
            (*imag)->data.F32[y][x] = out[index][1];
#endif
        }
    }

    FFTW_LOCK;
    fftwf_free(out);
    FFTW_UNLOCK;

    return true;
}

bool psImageBackwardFFT(psImage **out, const psImage *real, const psImage *imag, int origCols)
{
    PS_ASSERT_IMAGE_NON_NULL(real, false);
    PS_ASSERT_IMAGE_TYPE(real, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_NON_NULL(imag, false);
    PS_ASSERT_IMAGE_TYPE(imag, PS_TYPE_F32, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(real, imag, false);
    PS_ASSERT_PTR_NON_NULL(out, false);

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    int numCols = real->numCols;        // Number of columns
    int numRows = real->numRows;        // Number of rows

    // Because of the way FFT r2c and c2r work, need the number of columns in the target to be:
    // 2 * numCols = 2*(numCols/2 + 1) = origCols % 2 ? origCols + 1 : origCols + 2
    if (2 * numCols != ((origCols % 2) ? origCols + 1 : origCols + 2)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Number of columns in FFT-ed images (%d) should be origCols (%d) / 2 + 1 = %d",
                numCols, origCols, origCols/2 + 1);
        return false;
    }

    if (*out && (*out)->parent) {
        // It has a parent, so we can't write directly into the buffer.
        // It had better be the correct size and type, because we don't want to resize a child image
        PS_ASSERT_IMAGE_SIZE(*out, origCols, numRows, false);
        PS_ASSERT_IMAGE_TYPE(*out, PS_TYPE_F32, false);
    }

    // Make sure the system-level wisdom information is imported.
    FFTW_LOCK;
    if (!fftwWisdomImported) {
        fftwf_import_system_wisdom();
        fftwWisdomImported = true;
    }
    FFTW_UNLOCK;

    // Stuff the real and imaginary parts in
    psF32 *target = psAlloc(2 * numCols * numRows * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Target for FFTW
    FFTW_LOCK;
    fftwf_complex *in = fftwf_malloc(numCols * numRows * sizeof(fftwf_complex)); // Input data
    FFTW_UNLOCK;

    for (int y = 0, index = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; index++, x++) {
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
            // C99 complex support
            in[index] = real->data.F32[y][x] + imag->data.F32[y][x] * I;
#else
            // FFTW's backup complex support
            in[index][0] = real->data.F32[y][x];
            in[index][1] = imag->data.F32[y][x];
#endif
        }
    }

    // Do the FFT
    FFTW_LOCK;
    fftwf_plan plan = fftwf_plan_dft_c2r_2d(numRows, origCols, in, target, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(plan);

    FFTW_LOCK;
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    FFTW_UNLOCK;

    // Copy the target pixels into the output
    if (!(*out) || !(*out)->parent) {
        *out = psImageRecycle(*out, origCols, numRows, PS_TYPE_F32);
        memcpy((*out)->p_rawDataBuffer, target, numRows * origCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    } else {
        for (int y = 0, index = 0; y < numRows; y++, index += origCols) {
            memcpy(&(*out)->data.F32[y][0], &target[index], origCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
    }

    psFree(target);

    return true;
}

psImage* psImagePowerSpectrum(psImage *out, const psImage *in)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_TYPE(in, PS_TYPE_F32, NULL);

    psImage *real = NULL;              // Real component of FFT
    psImage *imag = NULL;              // Imaginary component of FFT

    if (!psImageForwardFFT(&real, &imag, in)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to perform forward FFT.");
        return NULL;
    }

    int numCols = real->numCols;        // Number of columns
    int numRows = real->numRows;        // Number of rows

    float norm = 1.0 / PS_SQR(in->numCols) / PS_SQR(in->numRows);
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            // Power spectrum is the square of the complex modulus
            real->data.F32[y][x] = norm * (PS_SQR(real->data.F32[y][x]) + PS_SQR(imag->data.F32[y][x]));
        }
    }
    psFree(imag);

    return real;
}


bool psImageComplexMultiply(psImage **outReal, psImage **outImag,
                            const psImage *in1Real, const psImage *in1Imag,
                            const psImage *in2Real, const psImage *in2Imag)
{
    PS_ASSERT_IMAGE_NON_NULL(in1Real, false);
    PS_ASSERT_IMAGE_NON_NULL(in1Imag, false);
    PS_ASSERT_IMAGE_NON_NULL(in2Real, false);
    PS_ASSERT_IMAGE_NON_NULL(in2Imag, false);
    PS_ASSERT_IMAGE_TYPE(in1Real, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_TYPE(in1Imag, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_TYPE(in2Real, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_TYPE(in2Imag, PS_TYPE_F32, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(in1Imag, in1Real, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(in2Real, in1Real, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(in2Imag, in1Real, false);
    PS_ASSERT_PTR_NON_NULL(outReal, false);
    PS_ASSERT_PTR_NON_NULL(outImag, false);
    if (*outReal) {
        PS_ASSERT_IMAGE_NON_NULL(*outReal, false);
        PS_ASSERT_IMAGE_NON_NULL(*outImag, false);
    } else if (*outImag) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "If the output real part is provided, the output imaginary part must also be provided.");
        return false;
    }

    int numRows = in1Real->numRows;     // Number of rows
    int numCols = in1Real->numCols;     // Number of columns

    // Need to worry if the outputs are children
    psImage *targetReal, *targetImag;   // Target real and imaginary parts

    if ((*outReal)->parent) {
        // It had better be the correct size and type, because we don't want to resize a child image
        PS_ASSERT_IMAGE_TYPE(*outReal, PS_TYPE_F32, false);
        PS_ASSERT_IMAGE_SIZE(*outReal, numCols, numRows, false);
        targetReal = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    } else {
        *outReal = psImageRecycle(*outReal, numCols, numRows, PS_TYPE_F32);
        targetReal = psMemIncrRefCounter(*outReal);
    }
    if ((*outImag)->parent) {
        // It had better be the correct size and type, because we don't want to resize a child image
        if ((*outReal)->numCols != numCols || (*outReal)->numRows != numRows ||
            (*outImag)->type.type != PS_TYPE_F32) {
            // Plug potential memory leak --- need to free targetReal if there's a problem
            psFree(targetReal);
        }
        PS_ASSERT_IMAGE_TYPE(*outReal, PS_TYPE_F32, false);
        PS_ASSERT_IMAGE_SIZE(*outReal, numCols, numRows, false);
        targetImag = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    } else {
        *outImag = psImageRecycle(*outImag, numCols, numRows, PS_TYPE_F32);
        targetImag = psMemIncrRefCounter(*outImag);
    }

    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            // (a + bi) * (c + di) = (ac - bd) + (bc + ad)i
            float real = in1Real->data.F32[y][x] * in2Real->data.F32[y][x] -
                in1Imag->data.F32[y][x] * in2Imag->data.F32[y][x];
            float imag = in1Imag->data.F32[y][x] * in2Real->data.F32[y][x] +
                in1Real->data.F32[y][x] * in2Imag->data.F32[y][x];
            targetReal->data.F32[y][x] = real;
            targetImag->data.F32[y][x] = imag;
        }
    }

    if ((*outReal)->parent) {
        *outReal = psImageCopy(*outReal, targetReal, PS_TYPE_F32);
    }
    if ((*outImag)->parent) {
        *outImag = psImageCopy(*outImag, targetImag, PS_TYPE_F32);
    }

    psFree(targetReal);
    psFree(targetImag);

    return true;
}


psImage *psImageConvolveFFT(psImage *out, const psImage *in, const psImage *mask, psImageMaskType maskVal,
                            const psKernel *kernel)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_TYPE(in, PS_TYPE_F32, NULL);
    PS_ASSERT_KERNEL_NON_NULL(kernel, NULL);
    if (mask) {
        PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, in, NULL);
    }

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    int numCols = in->numCols, numRows = in->numRows; // Size of image
    int xMin = kernel->xMin, xMax = kernel->xMax, yMin = kernel->yMin, yMax = kernel->yMax; // Kernel sizes

    // Need to pad the input image to protect from wrap-around effects
    if (xMax - xMin > numCols || yMax - yMin > numRows) {
        // Cannot pad the image if the kernel is larger.
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Kernel cannot extend further than input image size (%dx%d vs %dx%d)."),
                xMax, yMax, numCols, numRows);
        return NULL;
    }

    int paddedCols = numCols + PS_MAX(-xMin, xMax); // Number of columns in padded image
    int paddedRows = numRows + PS_MAX(-yMin, yMax); // Number of rows in padded image

#if CONVOLVE_FFT_BINARY_SIZE
    // Make the size an integer power of two
    {
        int twoCols, twoRows;           // Size that is a factor of two
        for (twoCols = 1; twoCols <= paddedCols && twoCols < INT_MAX - 1; twoCols <<= 1); // No action
        for (twoRows = 1; twoRows <= paddedRows && twoRows < INT_MAX - 1; twoRows <<= 1); // No action
        if (paddedCols > twoCols || paddedRows > twoRows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Unable to scale size (%dx%d) up to factor of two",
                    paddedCols, paddedRows);
            return NULL;
        }
        paddedCols = twoCols;
        paddedRows = twoRows;
    }
#endif

    int numPadded = paddedCols * paddedRows; // Number of pixels in padded image

    // Create data array containing the padded image and padded kernel
    FFTW_LOCK;
    psF32 *data = fftwf_malloc(2 * numPadded * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Data for FFTW
    FFTW_UNLOCK;
    psF32 *dataPtr = data;              // Pointer into FFTW data
    psF32 **imageData = in->data.F32;   // Pointer into image data

    // Image part of data array
    size_t goodBytes = numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes per image row
    size_t padBytes = (paddedCols - numCols) * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes to pad
    for (int y = 0; y < numRows; y++, dataPtr += paddedCols, imageData++) {
        memcpy(dataPtr, *imageData, goodBytes);
        memset(dataPtr + numCols, 0, padBytes);
    }
    memset(dataPtr, 0, (paddedRows - numRows) * paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));

#if 0
    {
        // Use this for inspecting the result of copying the image
        psImage *test = psImageAlloc(paddedCols, paddedRows, PS_TYPE_F32);
        psFree(test->p_rawDataBuffer);
        test->p_rawDataBuffer = data;
        test->data.V[0] = test->p_rawDataBuffer;
        for (int y = 1; y < paddedRows; y++) {
            test->data.V[y] = (psPtr)((int8_t *)test->data.V[y - 1] +
                                      paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
        // View image here
        test->p_rawDataBuffer = NULL;
        psFree(test);
    }
#endif

    // Kernel part of data array
    dataPtr = data + numPadded;         // Reset to kernel image location
    float norm = 1.0 / (float)(paddedRows * paddedCols); // Normalisation to correct for FFT
    // We could generate the padded kernel image using memcpy, but by going pixel by pixel we can apply the
    // normalisation that corrects for the FFT renormalisation.  By applying it to the kernel here, we save
    // applying it to the entire output image.
    int xNegMin = PS_MIN(-1, xMin), xNegMax = PS_MIN(-1, xMax); // Min and max for x when negative
    int xPosMin = PS_MAX(0, xMin), xPosMax = PS_MAX(0, xMax); // Min and max for x when positive
    int yNegMin = PS_MIN(-1, yMin), yNegMax = PS_MIN(-1, yMax); // Min and max for x when negative
    int yPosMin = PS_MAX(0, yMin), yPosMax = PS_MAX(0, yMax); // Min and max for x when positive
    int blankCols = xNegMin + paddedCols - xPosMax - 1; // Number of columns between kernel extrema
    int blankRows = (yNegMin + paddedRows - yPosMax - 1) * paddedCols; // Rows between kernel extrema
    size_t blankColBytes = blankCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes in blankCols
    for (int y = yPosMin; y <= yPosMax; y++) {
        // y is positive
        for (int x = xPosMin; x <= xPosMax; x++, dataPtr++) {
            // x is positive
            *dataPtr = kernel->kernel[y][x] * norm;
        }
        // Columns between kernel extrema
        memset(dataPtr, 0, blankColBytes);
        dataPtr += blankCols;
        for (int x = xNegMin; x <= xNegMax; x++, dataPtr++) {
            // x is negative
            *dataPtr = kernel->kernel[y][x] * norm;
        }
    }
    // Rows between kernel extrema
    memset(dataPtr, 0, blankRows * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    dataPtr += blankRows;
    for (int y = yNegMin; y <= yNegMax; y++) {
        // y is negative
        for (int x = xPosMin; x <= xPosMax; x++, dataPtr++) {
            // x is positive
            *dataPtr = kernel->kernel[y][x] * norm;
        }
        // Columns between kernel extrema
        memset(dataPtr, 0, blankColBytes);
        dataPtr += blankCols;
        for (int x = xNegMin; x <= xNegMax; x++, dataPtr++) {
            // x is negative
            *dataPtr = kernel->kernel[y][x] * norm;
        }
    }

#if 0
    {
        // Use this for inspecting the result of copying the kernel
        psImage *test = psImageAlloc(paddedCols, paddedRows, PS_TYPE_F32);
        psFree(test->p_rawDataBuffer);
        test->p_rawDataBuffer = &data[numPadded];
        test->data.V[0] = test->p_rawDataBuffer;
        for (int y = 1; y < paddedRows; y++) {
            test->data.V[y] = (psPtr)((int8_t *)test->data.V[y - 1] +
                                      paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
        // View image here
        test->p_rawDataBuffer = NULL;
        psFree(test);
    }
#endif

    // Mask bad pixels (which may be NANs), lest they infect everything
    if (mask && maskVal) {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                    data[x + paddedCols * y] = 0;
                }
            }
        }
    }

    // Do the forward FFT
    // Note that the FFT images have different size from the input
    FFTW_LOCK;
    fftwf_complex *fft = fftwf_malloc(2 * (paddedCols/2 + 1) * paddedRows * sizeof(fftwf_complex)); // FFT
    FFTW_UNLOCK;
    int size[] = { paddedRows, paddedCols }; // Size of transforms
    int fftCols = paddedCols/2 + 1, fftRows = paddedRows; // Size of FFT images
    int fftPixels = fftCols * fftRows;  // Number of pixels in FFT image

    FFTW_LOCK;
    fftwf_plan forward = fftwf_plan_many_dft_r2c(2, size, 2, data, NULL, 1, paddedCols * paddedRows,
                                                 fft, NULL, 1, fftPixels, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(forward);

    FFTW_LOCK;
    fftwf_destroy_plan(forward);
    FFTW_UNLOCK;

#if 0
    {
        // Use this for inspecting the result of the fft XXX : K & I are backwards...
        psImage *testKr = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        psImage *testKi = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        psImage *testIr = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        psImage *testIi = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        for (int iy = 0; iy < fftRows; iy++) {
	    for (int ix = 0; ix < fftCols; ix++) {
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
		testKr->data.F32[iy][ix] = creal(fft[ix + iy*fftCols]);
		testKi->data.F32[iy][ix] = cimag(fft[ix + iy*fftCols]);
		testIr->data.F32[iy][ix] = creal(fft[ix + iy*fftCols + fftPixels]);
		testIi->data.F32[iy][ix] = cimag(fft[ix + iy*fftCols + fftPixels]);
#else
		testKr->data.F32[iy][ix] = fft[ix + iy*fftCols][0];
		testKi->data.F32[iy][ix] = fft[ix + iy*fftCols][1];
		testIr->data.F32[iy][ix] = fft[ix + iy*fftCols + fftPixels][0];
		testIi->data.F32[iy][ix] = fft[ix + iy*fftCols + fftPixels][1];
#endif
	    }
        }
	fprintf (stderr, "generated test images\n");
	fprintf (stderr, "please inspect\n");
        psFree(testKr);
        psFree(testKi);
        psFree(testIr);
        psFree(testIi);
    }
#endif

    // Multiply the two transforms
    for (int i = 0, j = fftPixels; i < fftPixels; i++, j++) {
        // (a + bi) * (c + di) = (ac - bd) + (bc + ad)i
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
        // C99 complex support
        fft[i] *= fft[j];
#else
        // FFTW's backup complex support
        float imageReal = fft[i][0], imageImag = fft[i][1];
        float kernelReal = fft[j][0], kernelImag = fft[j][1];
        fft[i][0] = imageReal * kernelReal - imageImag * kernelImag;
        fft[i][1] = imageImag * kernelReal + imageReal * kernelImag;
#endif
    }

    // Do the backward FFT
    FFTW_LOCK;
    fftwf_plan backward = fftwf_plan_dft_c2r_2d(paddedRows, paddedCols, fft, data, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(backward);

    FFTW_LOCK;
    fftwf_destroy_plan(backward);
    fftwf_free(fft);
    FFTW_UNLOCK;

    // Copy into the target, without the padding
    out = psImageRecycle(out, numCols, numRows, PS_TYPE_F32);
    psF32 **outData = out->data.F32;    // Pointer into output
    dataPtr = data;                     // Reset to start
    for (int y = 0; y < numRows; y++, outData++, dataPtr += paddedCols) {
        memcpy(*outData, dataPtr, goodBytes);
    }

    FFTW_LOCK;
    fftwf_free(data);
    FFTW_UNLOCK;

    return out;
}

psImage *psImageConvolveFFTwithWindow(psImage *out, const psImage *in, const psImage *mask, psImageMaskType maskVal, const psKernel *kernel)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_TYPE(in, PS_TYPE_F32, NULL);
    PS_ASSERT_KERNEL_NON_NULL(kernel, NULL);
    if (mask) {
        PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, in, NULL);
    }

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    int numCols = in->numCols, numRows = in->numRows; // Size of image
    int xMin = kernel->xMin, xMax = kernel->xMax, yMin = kernel->yMin, yMax = kernel->yMax; // Kernel sizes

    // Need to pad the input image to protect from wrap-around effects
    if (xMax - xMin > numCols || yMax - yMin > numRows) {
        // Cannot pad the image if the kernel is larger.
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Kernel cannot extend further than input image size (%dx%d vs %dx%d)."),
                xMax, yMax, numCols, numRows);
        return NULL;
    }

    int paddedCols = numCols + PS_MAX(-xMin, xMax); // Number of columns in padded image
    int paddedRows = numRows + PS_MAX(-yMin, yMax); // Number of rows in padded image

#if CONVOLVE_FFT_BINARY_SIZE
    // Make the size an integer power of two
    {
        int twoCols, twoRows;           // Size that is a factor of two
        for (twoCols = 1; twoCols <= paddedCols && twoCols < INT_MAX - 1; twoCols <<= 1); // No action
        for (twoRows = 1; twoRows <= paddedRows && twoRows < INT_MAX - 1; twoRows <<= 1); // No action
        if (paddedCols > twoCols || paddedRows > twoRows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Unable to scale size (%dx%d) up to factor of two",
                    paddedCols, paddedRows);
            return NULL;
        }
        paddedCols = twoCols;
        paddedRows = twoRows;
    }
#endif

    int numPadded = paddedCols * paddedRows; // Number of pixels in padded image

    // Create data array containing the padded image and padded kernel
    FFTW_LOCK;
    psF32 *data = fftwf_malloc(2 * numPadded * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Data for FFTW
    FFTW_UNLOCK;
    psF32 *dataPtr = data;              // Pointer into FFTW data
    psF32 **imageData = in->data.F32;   // Pointer into image data

    // Image part of data array
    size_t goodBytes = numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes per image row
    size_t padBytes = (paddedCols - numCols) * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes to pad
    for (int y = 0; y < numRows; y++, dataPtr += paddedCols, imageData++) {
        memcpy(dataPtr, *imageData, goodBytes);
        memset(dataPtr + numCols, 0, padBytes);
    }
    memset(dataPtr, 0, (paddedRows - numRows) * paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));

#if 0
    {
        // Use this for inspecting the result of copying the image
        psImage *test = psImageAlloc(paddedCols, paddedRows, PS_TYPE_F32);
        psFree(test->p_rawDataBuffer);
        test->p_rawDataBuffer = data;
        test->data.V[0] = test->p_rawDataBuffer;
        for (int y = 1; y < paddedRows; y++) {
            test->data.V[y] = (psPtr)((int8_t *)test->data.V[y - 1] +
                                      paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
        // View image here
        test->p_rawDataBuffer = NULL;
        psFree(test);
    }
#endif

    // Kernel part of data array
    dataPtr = data + numPadded;         // Reset to kernel image location
    float norm = 1.0 / (float)(paddedRows * paddedCols); // Normalisation to correct for FFT
    // We could generate the padded kernel image using memcpy, but by going pixel by pixel we can apply the
    // normalisation that corrects for the FFT renormalisation.  By applying it to the kernel here, we save
    // applying it to the entire output image.
    int xNegMin = PS_MIN(-1, xMin), xNegMax = PS_MIN(-1, xMax); // Min and max for x when negative
    int xPosMin = PS_MAX(0, xMin), xPosMax = PS_MAX(0, xMax); // Min and max for x when positive
    int yNegMin = PS_MIN(-1, yMin), yNegMax = PS_MIN(-1, yMax); // Min and max for x when negative
    int yPosMin = PS_MAX(0, yMin), yPosMax = PS_MAX(0, yMax); // Min and max for x when positive
    int blankCols = xNegMin + paddedCols - xPosMax - 1; // Number of columns between kernel extrema
    int blankRows = (yNegMin + paddedRows - yPosMax - 1) * paddedCols; // Rows between kernel extrema
    size_t blankColBytes = blankCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes in blankCols
    for (int y = yPosMin; y <= yPosMax; y++) {
        // y is positive
        for (int x = xPosMin; x <= xPosMax; x++, dataPtr++) {
            // x is positive
            *dataPtr = kernel->kernel[y][x] * norm;
        }
        // Columns between kernel extrema
        memset(dataPtr, 0, blankColBytes);
        dataPtr += blankCols;
        for (int x = xNegMin; x <= xNegMax; x++, dataPtr++) {
            // x is negative
            *dataPtr = kernel->kernel[y][x] * norm;
        }
    }
    // Rows between kernel extrema
    memset(dataPtr, 0, blankRows * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    dataPtr += blankRows;
    for (int y = yNegMin; y <= yNegMax; y++) {
        // y is negative
        for (int x = xPosMin; x <= xPosMax; x++, dataPtr++) {
            // x is positive
            *dataPtr = kernel->kernel[y][x] * norm;
        }
        // Columns between kernel extrema
        memset(dataPtr, 0, blankColBytes);
        dataPtr += blankCols;
        for (int x = xNegMin; x <= xNegMax; x++, dataPtr++) {
            // x is negative
            *dataPtr = kernel->kernel[y][x] * norm;
        }
    }

#if 0
    {
        // Use this for inspecting the result of copying the kernel
        psImage *test = psImageAlloc(paddedCols, paddedRows, PS_TYPE_F32);
        psFree(test->p_rawDataBuffer);
        test->p_rawDataBuffer = &data[numPadded];
        test->data.V[0] = test->p_rawDataBuffer;
        for (int y = 1; y < paddedRows; y++) {
            test->data.V[y] = (psPtr)((int8_t *)test->data.V[y - 1] +
                                      paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
        // View image here
        test->p_rawDataBuffer = NULL;
        psFree(test);
    }
#endif

    // Mask bad pixels (which may be NANs), lest they infect everything
    if (mask && maskVal) {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                    data[x + paddedCols * y] = 0;
                }
            }
        }
    }

    // Do the forward FFT
    // Note that the FFT images have different size from the input
    FFTW_LOCK;
    fftwf_complex *fft = fftwf_malloc(2 * (paddedCols/2 + 1) * paddedRows * sizeof(fftwf_complex)); // FFT
    FFTW_UNLOCK;
    int size[] = { paddedRows, paddedCols }; // Size of transforms
    int fftCols = paddedCols/2 + 1, fftRows = paddedRows; // Size of FFT images
    int fftPixels = fftCols * fftRows;  // Number of pixels in FFT image

    FFTW_LOCK;
    fftwf_plan forward = fftwf_plan_many_dft_r2c(2, size, 2, data, NULL, 1, paddedCols * paddedRows,
                                                 fft, NULL, 1, fftPixels, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(forward);

    FFTW_LOCK;
    fftwf_destroy_plan(forward);
    FFTW_UNLOCK;

    // apply a window function:
    psVector *xWindow = psVectorAlloc(fftCols, PS_TYPE_F32);
    psVector *yWindow = psVectorAlloc(fftRows, PS_TYPE_F32);

    // not sure what window function to use.  trying a simple Gaussian.
    // In the x-direction, the window goes from ~1 @ x = 0 to something low at x = fftCols
    // In the y direction, the window goes to a small value at y = fftRows / 2, and back up
    for (int i = 0; i < fftCols; i++) {
	float xNorm = 2.0 * i / (float) fftCols; // sigma = 0.5
	xWindow->data.F32[i] = exp(-0.5*xNorm*xNorm);
    }
    for (int i = 0; i < fftRows / 2 + 1; i++) {
	float xNorm = 2.0 * i / (float) (fftRows / 2.0); // sigma = 0.5
	yWindow->data.F32[i] = exp(-0.5*xNorm*xNorm);
	yWindow->data.F32[fftRows - 1 - i] = yWindow->data.F32[i];
    }

    for (int iy = 0; iy < fftRows; iy++) {
	float yValue = yWindow->data.F32[iy];
	for (int ix = 0; ix < fftCols; ix++) {
	    float xValue = xWindow->data.F32[ix];
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
	    fft[ix + iy*fftCols + fftPixels] *= xValue * yValue; // kernel element (real + I*imaginary)
#else
	    fft[ix + iy*fftCols + fftPixels][0] *= xValue * yValue;
	    fft[ix + iy*fftCols + fftPixels][1] *= xValue * yValue;
#endif
	}
    }

#if 0
    {
        // Use this for inspecting the result of the fft XXX : K & I are backwards...
        psImage *testKr = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        psImage *testKi = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        psImage *testIr = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        psImage *testIi = psImageAlloc(fftCols, fftRows, PS_TYPE_F32);
        for (int iy = 0; iy < fftRows; iy++) {
	    for (int ix = 0; ix < fftCols; ix++) {
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
		testIr->data.F32[iy][ix] = creal(fft[ix + iy*fftCols]);
		testIi->data.F32[iy][ix] = cimag(fft[ix + iy*fftCols]);
		testKr->data.F32[iy][ix] = creal(fft[ix + iy*fftCols + fftPixels]);
		testKi->data.F32[iy][ix] = cimag(fft[ix + iy*fftCols + fftPixels]);
#else
		testIr->data.F32[iy][ix] = fft[ix + iy*fftCols][0];
		testIi->data.F32[iy][ix] = fft[ix + iy*fftCols][1];
		testKr->data.F32[iy][ix] = fft[ix + iy*fftCols + fftPixels][0];
		testKi->data.F32[iy][ix] = fft[ix + iy*fftCols + fftPixels][1];
#endif
	    }
        }
	fprintf (stderr, "generated test images\n");
	fprintf (stderr, "please inspect\n");
        psFree(testKr);
        psFree(testKi);
        psFree(testIr);
        psFree(testIi);
    }
#endif

    // Multiply the two transforms
    for (int i = 0, j = fftPixels; i < fftPixels; i++, j++) {
        // (a + bi) * (c + di) = (ac - bd) + (bc + ad)i
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
        // C99 complex support
        fft[i] *= fft[j];
#else
        // FFTW's backup complex support
        float imageReal = fft[i][0], imageImag = fft[i][1];
        float kernelReal = fft[j][0], kernelImag = fft[j][1];
        fft[i][0] = imageReal * kernelReal - imageImag * kernelImag;
        fft[i][1] = imageImag * kernelReal + imageReal * kernelImag;
#endif
    }

    // Do the backward FFT
    FFTW_LOCK;
    fftwf_plan backward = fftwf_plan_dft_c2r_2d(paddedRows, paddedCols, fft, data, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(backward);

    FFTW_LOCK;
    fftwf_destroy_plan(backward);
    fftwf_free(fft);
    FFTW_UNLOCK;

    // Copy into the target, without the padding
    out = psImageRecycle(out, numCols, numRows, PS_TYPE_F32);
    psF32 **outData = out->data.F32;    // Pointer into output
    dataPtr = data;                     // Reset to start
    for (int y = 0; y < numRows; y++, outData++, dataPtr += paddedCols) {
        memcpy(*outData, dataPtr, goodBytes);
    }

    FFTW_LOCK;
    fftwf_free(data);
    FFTW_UNLOCK;

    return out;
}

void psKernelFFTFree(psKernelFFT *kernel) {

    if (!kernel->fft) return;
    
    bool threaded = psThreadPoolSize(); // Are we running threaded?

    FFTW_LOCK;
    fftwf_free(kernel->fft);
    FFTW_UNLOCK;

    return;
}

psKernelFFT *psKernelFFTAlloc(const psKernel *input) {

    psKernelFFT *kernel = psAlloc(sizeof(psKernelFFT));
    psMemSetDeallocator(kernel, (psFreeFunc)psKernelFFTFree);

    kernel->fft = NULL;

    kernel->xMin = input->xMin;
    kernel->xMax = input->xMax;
    kernel->yMin = input->yMin;
    kernel->yMax = input->yMax;

    kernel->paddedCols = 0;
    kernel->paddedRows = 0;

    return kernel;
}

// generate the FFTed kernel image appropriate to be used for convolution of the given input image
psKernelFFT *psImageConvolveKernelInit(const psImage *in, const psKernel *kernel)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_TYPE(in, PS_TYPE_F32, NULL);
    PS_ASSERT_KERNEL_NON_NULL(kernel, NULL);

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    int numCols = in->numCols, numRows = in->numRows; // Size of image
    int xMin = kernel->xMin, xMax = kernel->xMax, yMin = kernel->yMin, yMax = kernel->yMax; // Kernel sizes

    // Need to pad the kernel (and later the input image) to protect from wrap-around effects
    if (xMax - xMin > numCols || yMax - yMin > numRows) {
        // Cannot pad the image if the kernel is larger.
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Kernel cannot extend further than input image size (%dx%d vs %dx%d)."),
                xMax, yMax, numCols, numRows);
        return NULL;
    }

    int paddedCols = numCols + PS_MAX(-xMin, xMax); // Number of columns in padded image
    int paddedRows = numRows + PS_MAX(-yMin, yMax); // Number of rows in padded image

#if CONVOLVE_FFT_BINARY_SIZE
    // Make the size an integer power of two
    {
        int twoCols, twoRows;           // Size that is a factor of two
        for (twoCols = 1; twoCols <= paddedCols && twoCols < INT_MAX - 1; twoCols <<= 1); // No action
        for (twoRows = 1; twoRows <= paddedRows && twoRows < INT_MAX - 1; twoRows <<= 1); // No action
        if (paddedCols > twoCols || paddedRows > twoRows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Unable to scale size (%dx%d) up to factor of two",
                    paddedCols, paddedRows);
            return NULL;
        }
        paddedCols = twoCols;
        paddedRows = twoRows;
    }
#endif

    int numPadded = paddedCols * paddedRows; // Number of pixels in padded image

    // Create data array containing the padded kernel
    FFTW_LOCK;
    psF32 *data = fftwf_malloc(numPadded * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Data for FFTW
    FFTW_UNLOCK;

    // Generate the padded kernel image.  We could generate the padded kernel image using
    // memcpy, but by going pixel by pixel we can apply the normalisation that corrects for the
    // FFT renormalisation.  By applying it to the kernel here, we save applying it to the
    // output image(s).

    // copy kernel into data array
    psF32 *dataPtr = data;              // Pointer into FFTW data
    float norm = 1.0 / (float)(paddedRows * paddedCols); // Normalisation to correct for FFT

    int xNegMin = PS_MIN(-1, xMin), xNegMax = PS_MIN(-1, xMax); // Min and max for x when negative
    int xPosMin = PS_MAX(0, xMin), xPosMax = PS_MAX(0, xMax); // Min and max for x when positive
    int yNegMin = PS_MIN(-1, yMin), yNegMax = PS_MIN(-1, yMax); // Min and max for x when negative
    int yPosMin = PS_MAX(0, yMin), yPosMax = PS_MAX(0, yMax); // Min and max for x when positive
    int blankCols = xNegMin + paddedCols - xPosMax - 1; // Number of columns between kernel extrema
    int blankRows = (yNegMin + paddedRows - yPosMax - 1) * paddedCols; // Rows between kernel extrema
    size_t blankColBytes = blankCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes in blankCols
    for (int y = yPosMin; y <= yPosMax; y++) {
        // y is positive
        for (int x = xPosMin; x <= xPosMax; x++, dataPtr++) {
            // x is positive
            *dataPtr = kernel->kernel[y][x] * norm;
        }
        // Columns between kernel extrema
        memset(dataPtr, 0, blankColBytes);
        dataPtr += blankCols;
        for (int x = xNegMin; x <= xNegMax; x++, dataPtr++) {
            // x is negative
            *dataPtr = kernel->kernel[y][x] * norm;
        }
    }
    // Rows between kernel extrema
    memset(dataPtr, 0, blankRows * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    dataPtr += blankRows;
    for (int y = yNegMin; y <= yNegMax; y++) {
        // y is negative
        for (int x = xPosMin; x <= xPosMax; x++, dataPtr++) {
            // x is positive
            *dataPtr = kernel->kernel[y][x] * norm;
        }
        // Columns between kernel extrema
        memset(dataPtr, 0, blankColBytes);
        dataPtr += blankCols;
        for (int x = xNegMin; x <= xNegMax; x++, dataPtr++) {
            // x is negative
            *dataPtr = kernel->kernel[y][x] * norm;
        }
    }

#if 0
    {
        // Use this for inspecting the result of copying the kernel
        psImage *test = psImageAlloc(paddedCols, paddedRows, PS_TYPE_F32);
        psFree(test->p_rawDataBuffer);
        test->p_rawDataBuffer = data;
        test->data.V[0] = test->p_rawDataBuffer;
        for (int y = 1; y < paddedRows; y++) {
            test->data.V[y] = (psPtr)((int8_t *)test->data.V[y - 1] + paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
        // View image here or write to disk
        test->p_rawDataBuffer = NULL;
        psFree(test);
    }
#endif

    // Do the forward FFT
    // Note that the FFT images have different size from the input
    FFTW_LOCK;
    fftwf_complex *fft = fftwf_malloc((paddedCols/2 + 1) * paddedRows * sizeof(fftwf_complex)); // FFT
    FFTW_UNLOCK;

    FFTW_LOCK;
    fftwf_plan plan = fftwf_plan_dft_r2c_2d(paddedRows, paddedCols, data, fft, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(plan);

    FFTW_LOCK;
    fftwf_destroy_plan(plan);
    fftwf_free(data);
    FFTW_UNLOCK;

    psKernelFFT *output = psKernelFFTAlloc(kernel);
    output->fft = fft;
    output->paddedCols = paddedCols;
    output->paddedRows = paddedRows;

    return output;
}

psImage *psImageConvolveKernel(psImage *out, const psImage *in, const psImage *mask, psImageMaskType maskVal, const psKernelFFT *kernel)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);
    PS_ASSERT_IMAGE_TYPE(in, PS_TYPE_F32, NULL);
    PS_ASSERT_PTR_NON_NULL(kernel, NULL);
    PS_ASSERT_PTR_NON_NULL(kernel->fft, NULL);
    if (mask) {
        PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, in, NULL);
    }

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    int numCols = in->numCols, numRows = in->numRows; // Size of image
    int xMin = kernel->xMin, xMax = kernel->xMax, yMin = kernel->yMin, yMax = kernel->yMax; // Kernel sizes

    // Need to pad the input image to protect from wrap-around effects
    if (xMax - xMin > numCols || yMax - yMin > numRows) {
        // Cannot pad the image if the kernel is larger.
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Kernel cannot extend further than input image size (%dx%d vs %dx%d)."), xMax, yMax, numCols, numRows);
        return NULL;
    }

    int paddedCols = numCols + PS_MAX(-xMin, xMax); // Number of columns in padded image
    int paddedRows = numRows + PS_MAX(-yMin, yMax); // Number of rows in padded image

#if CONVOLVE_FFT_BINARY_SIZE
    // Make the size an integer power of two
    {
        int twoCols, twoRows;           // Size that is a factor of two
        for (twoCols = 1; twoCols <= paddedCols && twoCols < INT_MAX - 1; twoCols <<= 1); // No action
        for (twoRows = 1; twoRows <= paddedRows && twoRows < INT_MAX - 1; twoRows <<= 1); // No action
        if (paddedCols > twoCols || paddedRows > twoRows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Unable to scale size (%dx%d) up to factor of two",
                    paddedCols, paddedRows);
            return NULL;
        }
        paddedCols = twoCols;
        paddedRows = twoRows;
    }
#endif

    if (paddedCols != kernel->paddedCols) {
	psAbort(_("Image is inconsistent with allocated kernel"));
    }	
    if (paddedRows != kernel->paddedRows) {
        psAbort(_("Image is inconsistent with allocated kernel"));
    }	

    int numPadded = paddedCols * paddedRows; // Number of pixels in padded image

    // Create data array containing the padded image
    FFTW_LOCK;
    psF32 *data = fftwf_malloc(numPadded * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Data for FFTW
    FFTW_UNLOCK;
    psF32 *dataPtr = data;              // Pointer into FFTW data
    psF32 **imageData = in->data.F32;   // Pointer into image data

    // Copy image into data array
    size_t goodBytes = numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes per image row
    size_t padBytes = (paddedCols - numCols) * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes to pad
    for (int y = 0; y < numRows; y++, dataPtr += paddedCols, imageData++) {
        memcpy(dataPtr, *imageData, goodBytes);
        memset(dataPtr + numCols, 0, padBytes);
    }
    memset(dataPtr, 0, (paddedRows - numRows) * paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));

#if 0
    {
        // Use this for inspecting the result of copying the image
        psImage *test = psImageAlloc(paddedCols, paddedRows, PS_TYPE_F32);
        psFree(test->p_rawDataBuffer);
        test->p_rawDataBuffer = data;
        test->data.V[0] = test->p_rawDataBuffer;
        for (int y = 1; y < paddedRows; y++) {
            test->data.V[y] = (psPtr)((int8_t *)test->data.V[y - 1] +
                                      paddedCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
        }
        // View image here
        test->p_rawDataBuffer = NULL;
        psFree(test);
    }
#endif

    // Mask bad pixels (which may be NANs), lest they infect everything
    if (mask && maskVal) {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                    data[x + paddedCols * y] = 0;
                }
            }
        }
    }

    // Do the forward FFT
    // Note that the FFT images have different size from the input
    FFTW_LOCK;
    fftwf_complex *fft = fftwf_malloc((paddedCols/2 + 1) * paddedRows * sizeof(fftwf_complex)); // FFT
    FFTW_UNLOCK;

    int fftCols = paddedCols/2 + 1, fftRows = paddedRows; // Size of FFT images
    int fftPixels = fftCols * fftRows;  // Number of pixels in FFT image

    FFTW_LOCK;
    fftwf_plan forward = fftwf_plan_dft_r2c_2d(paddedRows, paddedCols, data, fft, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(forward);

    FFTW_LOCK;
    fftwf_destroy_plan(forward);
    FFTW_UNLOCK;

    fftwf_complex *kernelFFT = kernel->fft;

    // Multiply the two transforms
    for (int i = 0; i < fftPixels; i++) {
        // (a + bi) * (c + di) = (ac - bd) + (bc + ad)i
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
        // C99 complex support
        fft[i] *= kernelFFT[i];
#else
        // FFTW's backup complex support
        float imageReal = fft[i][0], imageImag = fft[i][1];
        float kernelReal = kernelFFT[i][0], kernelImag = kernelFFT[i][1];
        fft[i][0] = imageReal * kernelReal - imageImag * kernelImag;
        fft[i][1] = imageImag * kernelReal + imageReal * kernelImag;
#endif
    }

    // Do the backward FFT
    FFTW_LOCK;
    fftwf_plan backward = fftwf_plan_dft_c2r_2d(paddedRows, paddedCols, fft, data, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(backward);

    FFTW_LOCK;
    fftwf_destroy_plan(backward);
    fftwf_free(fft);
    FFTW_UNLOCK;

    // Copy into the target, without the padding
    out = psImageRecycle(out, numCols, numRows, PS_TYPE_F32);
    psF32 **outData = out->data.F32;    // Pointer into output
    dataPtr = data;                     // Reset to start
    for (int y = 0; y < numRows; y++, outData++, dataPtr += paddedCols) {
        memcpy(*outData, dataPtr, goodBytes);
    }

    FFTW_LOCK;
    fftwf_free(data);
    FFTW_UNLOCK;

    return out;
}

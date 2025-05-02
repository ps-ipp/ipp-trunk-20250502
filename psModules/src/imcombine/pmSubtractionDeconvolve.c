#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fftw3.h>
#include <pslib.h>

#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionDeconvolve.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionVisual.h"

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

psKernel *pmSubtractionDeconvolveGauss (int size, float sigma) {

    psKernel *kernel = psKernelAlloc (-size, size, -size, size);

    // build the gaussian from 2 1-D Gaussians
    psVector *vector = pmSubtractionKernelISIS(sigma, 0, size);

    // generate 2D kernel, calculate moments
    for (int v = -size, y = 0; v <= size; v++, y++) {
	for (int u = -size, x = 0; u <= size; u++, x++) {
	    double value = vector->data.F32[x] * vector->data.F32[y]; // Value of kernel
	    kernel->kernel[v][u] = value;
	}
    }

    psFree (vector);
    return kernel;
}

// deconvolve kernelTarget by kernelConv to get the kernel which, when convolved
// by kernelConv results in kernelTarget...
// XXX using complex to complex, explicitly setting the imaginary part to zero
psKernel *pmSubtractionDeconvolveKernel (psKernel *kernelTarg, psKernel *kernelConv) {

    PS_ASSERT_KERNEL_NON_NULL(kernelTarg, NULL);
    PS_ASSERT_KERNEL_NON_NULL(kernelConv, NULL);

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    // Size of image
    int numCols = kernelConv->image->numCols;
    int numRows = kernelConv->image->numRows;

    // kernel sizes
    int xMin = kernelConv->xMin;
    int xMax = kernelConv->xMax;
    int yMin = kernelConv->yMin;
    int yMax = kernelConv->yMax;
    if (xMin != kernelTarg->xMin) goto escape;
    if (xMax != kernelTarg->xMax) goto escape;
    if (yMin != kernelTarg->yMin) goto escape;
    if (yMax != kernelTarg->yMax) goto escape;

    int numPixels = numCols * numRows; // Number of pixels in padded image

    // operation is: Kt = FFT(kernelTarg), Kc = FFT(kernelConv)
    // Kd = (Kt * Kc) / (Kc * Kc^*)

    // Create data array containing the image and kernel
    FFTW_LOCK;
    // psF32 *dataTarg = fftwf_malloc(numPixels * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Data for FFTW
    // psF32 *dataConv = fftwf_malloc(numPixels * PSELEMTYPE_SIZEOF(PS_TYPE_F32)); // Data for FFTW
    fftwf_complex *dataTarg = fftwf_malloc(numPixels * sizeof(fftwf_complex)); // Data for FFTW
    fftwf_complex *dataConv = fftwf_malloc(numPixels * sizeof(fftwf_complex)); // Data for FFTW
    FFTW_UNLOCK;

    // size_t numBytes = numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32); // Number of bytes per image row

    // copy data from kernelTarg image to dataTarg array
    for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	    dataTarg[x + y*numCols][0] = kernelTarg->image->data.F32[y][x];
	    dataTarg[x + y*numCols][1] = 0.0;
	}
    }
    
    // kernel must be copied to corners of image (0,0 pixel is center of kernel)
    // copy data from kernelConv image to dataConv array
    int oy = 0;
    for (int iy = 0; iy <= yMax; iy++, oy++) {
	int ox = 0;
	for (int ix = 0; ix <= xMax; ix++, ox++) {
	    dataConv[ox + oy*numCols][0] = kernelConv->kernel[iy][ix];
	    dataConv[ox + oy*numCols][1] = 0.0;
	}
	for (int ix = xMin; ix <= -1; ix++, ox++) {
	    dataConv[ox + oy*numCols][0] = kernelConv->kernel[iy][ix];
	    dataConv[ox + oy*numCols][1] = 0.0;
	}
    }
    for (int iy = yMin; iy <= -1; iy++, oy++) {
	int ox = 0;
	for (int ix = 0; ix <= xMax; ix++, ox++) {
	    dataConv[ox + oy*numCols][0] = kernelConv->kernel[iy][ix];
	    dataConv[ox + oy*numCols][1] = 0.0;
	}
	for (int ix = xMin; ix <= -1; ix++, ox++) {
	    dataConv[ox + oy*numCols][0] = kernelConv->kernel[iy][ix];
	    dataConv[ox + oy*numCols][1] = 0.0;
	}
    }

    // Do the forward FFTs
    // Note that the FFT images have different size from the input
    FFTW_LOCK;
    fftwf_complex *fftTarg = fftwf_malloc(numCols * numRows * sizeof(fftwf_complex)); // FFT
    fftwf_complex *fftConv = fftwf_malloc(numCols * numRows * sizeof(fftwf_complex)); // FFT
    FFTW_UNLOCK;

    FFTW_LOCK;
    fftwf_plan forwardTarg = fftwf_plan_dft_2d(numRows, numCols, dataTarg, fftTarg, FFTW_FORWARD, FFTW_PLAN_RIGOR);
    fftwf_plan forwardConv = fftwf_plan_dft_2d(numRows, numCols, dataConv, fftConv, FFTW_FORWARD, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(forwardTarg);
    fftwf_execute(forwardConv);

    FFTW_LOCK;
    fftwf_destroy_plan(forwardTarg);
    fftwf_destroy_plan(forwardConv);
    FFTW_UNLOCK;

    // Combine the two transforms 
    // Targ = Tr + iTi, Conv = Cr + iCi
    // Deco = Dr + iDi
    // (Dr + i Di) = (Tr + iTi) / (Cr + iCi)
    // (Dr + i Di) = (Tr + iTi) * (Cr - iCi) / (Cr^2 - Ci^2)

    // but anywhere Cr^2 - Ci^2 < 1e-7 of the max, mask it

    // the X dimension is halved by FFTW
    // int numColsOut = numCols / 2 + 1;

    // generate Det = Cr^2 - Ci^2
    float maxValue = 0.0;
    psImage *det = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tR  = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *tI  = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *cR  = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *cI  = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (int iy = 0; iy < numRows; iy++) {
	for (int ix = 0; ix < numCols; ix++) {
	    float convReal = fftConv[ix + iy*numCols][0];
	    float convImag = fftConv[ix + iy*numCols][1];
	    det->data.F32[iy][ix] = convReal*convReal - convImag*convImag;
	    maxValue = PS_MAX(maxValue, fabs(det->data.F32[iy][ix]));

	    tR->data.F32[iy][ix] = fftTarg[ix + iy*numCols][0];
	    tI->data.F32[iy][ix] = fftTarg[ix + iy*numCols][1];
	    cR->data.F32[iy][ix] = fftConv[ix + iy*numCols][0];
	    cI->data.F32[iy][ix] = fftConv[ix + iy*numCols][1];
	}
    }

    // pmSubtractionVisualShowSubtraction (det, tR, tI);
    // pmSubtractionVisualShowSubtraction (det, cR, cI);

# if 1
# define TOL 1e-7
    float limit = TOL*maxValue;
    // generate Deco = targ * conv^* / (Cr^2 - Ci^2)
    for (int iy = 0; iy < numRows; iy++) {
	for (int ix = 0; ix < numCols; ix++) {
	    float targReal = fftTarg[ix + iy*numCols][0];
	    float targImag = fftTarg[ix + iy*numCols][1];
	    float convReal = fftConv[ix + iy*numCols][0];
	    float convImag = fftConv[ix + iy*numCols][1];
	    if (fabs(det->data.F32[iy][ix]) < limit) {
		fftTarg[ix + iy*numCols][0] = 0.0;
		fftTarg[ix + iy*numCols][1] = 0.0;
	    } else {
		fftTarg[ix + iy*numCols][0] = (targReal*convReal + targImag*convImag) / det->data.F32[iy][ix];
		fftTarg[ix + iy*numCols][1] = (targImag*convReal - targReal*convImag) / det->data.F32[iy][ix];
		// fftTarg[ix + iy*numCols][0] = (targReal*convReal + targImag*convImag);
		// fftTarg[ix + iy*numCols][1] = (targImag*convReal - targReal*convImag);
	    }
	}
    }
# else
    for (int iy = 0; iy < numRows; iy++) {
	for (int ix = 0; ix < numCols; ix++) {
	    float targReal = fftTarg[ix + iy*numCols][0];
	    float targImag = fftTarg[ix + iy*numCols][1];
	    float convReal = fftConv[ix + iy*numCols][0];
	    float convImag = fftConv[ix + iy*numCols][1];
	    fftTarg[ix + iy*numCols][0] = targReal*convReal - targImag*convImag;
	    fftTarg[ix + iy*numCols][1] = targImag*convReal + targReal*convImag;
	}
    }
# endif

    for (int iy = 0; iy < numRows; iy++) {
	for (int ix = 0; ix < numCols; ix++) {
	    tR->data.F32[iy][ix] = fftTarg[ix + iy*numCols][0];
	    tI->data.F32[iy][ix] = fftTarg[ix + iy*numCols][1];
	}
    }
    // pmSubtractionVisualShowSubtraction (det, tR, tI);

    // Do the backward FFT
    FFTW_LOCK;
    fftwf_plan backward = fftwf_plan_dft_2d(numRows, numCols, fftTarg, dataTarg, FFTW_BACKWARD, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(backward);

    FFTW_LOCK;
    fftwf_destroy_plan(backward);
    fftwf_free(fftTarg);
    fftwf_free(fftConv);
    FFTW_UNLOCK;

    psKernel *output = psKernelAlloc (kernelTarg->xMin, kernelTarg->xMax, kernelTarg->yMin, kernelTarg->yMax);
    for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	    output->image->data.F32[y][x] = dataTarg[x + y*numCols][0];
	}
    }

    FFTW_LOCK;
    fftwf_free(dataTarg);
    fftwf_free(dataConv);
    FFTW_UNLOCK;

    return output;

 escape:
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "mismatch between kernel and image");
    return NULL;

}

bool pmSubtractionDeconvolutionTest (int order) {

    float sigma = 1.0;
    int size = 31;

    // generate a Hermite polynomial 
    psVector *xKernel = pmSubtractionKernelHERM(sigma, order, size); // x Kernel
    psVector *yKernel = pmSubtractionKernelHERM(sigma, order, size); // y Kernel
    psKernel *kernelTarget = psKernelAlloc(-size, size, -size, size);	// Kernel

    // generate 2D kernel, calculate moments
    for (int v = -size, y = 0; v <= size; v++, y++) {
	for (int u = -size, x = 0; u <= size; u++, x++) {
	    double value = xKernel->data.F32[x] * yKernel->data.F32[y]; // Value of kernel
	    kernelTarget->kernel[v][u] = value;
	}
    }

    // Gaussian convolution kernel
    psKernel *kernelGauss = pmSubtractionDeconvolveGauss (size, 3.0);

    // deconvolve the target by the gaussian:
    psKernel *kernel = pmSubtractionDeconvolveKernel(kernelTarget, kernelGauss); // Kernel

    // re-convolve the kernel
    psImage *kernelConv = psImageConvolveFFT(NULL, kernel->image, NULL, 0, kernelGauss);
    pmSubtractionVisualShowSubtraction (kernelTarget->image, kernel->image, kernelConv);

    return true;

}

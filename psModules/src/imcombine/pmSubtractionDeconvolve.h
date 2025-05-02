#ifndef PM_SUBTRACTION_DECONVOLVE_H
#define PM_SUBTRACTION_DECONVOLVE_H

/* these function support deconvolution operations used to generate deconvolved kernels.  These
   are kernels which, when convolved with the image sources, will yield a nearly orthonormal
   basis set.  The analysis starts with an orthonormal basis set (eg, Hermitian functions) and
   deconvolves those basis functions with a Gaussian approximating the Gaussian of the image of
   interest */

psKernel *pmSubtractionDeconvolveGauss (int size, float sigma);
psKernel *pmSubtractionDeconvolveKernel (psKernel *kernelTarg, psKernel *kernelConv);

bool pmSubtractionDeconvolutionTest (int order);

# endif

#ifndef PM_SUBTRACTION_EQUATION_H
#define PM_SUBTRACTION_EQUATION_H

#include "pmSubtractionStamps.h"
#include "pmSubtractionKernels.h"
#include "pmSubtraction.h"

/// Execute a thread job to calculate the least-squares equation for a stamp
bool pmSubtractionCalculateEquationThread(psThreadJob *job ///< Job to execute
    );

/// Calculate the least-squares equation to match the image quality for a single stamp
bool pmSubtractionCalculateEquationStamp(pmSubtractionStampList *stamps, ///< Stamps
                                         pmSubtractionKernels *kernels, ///< Kernel parameters
                                         int index ///< Index of stamp
    );

/// Calculate the least-squares equation to match the image quality
bool pmSubtractionCalculateEquation(pmSubtractionStampList *stamps, ///< Stamps
                                    pmSubtractionKernels *kernels ///< Kernel parameters
    );

/// Solve the least-squares equation to match the image quality
bool pmSubtractionSolveEquation(pmSubtractionKernels *kernels, ///< Kernel parameters
                                const pmSubtractionStampList *stamps ///< Stamps
    );

/// Calculate deviations
psVector *pmSubtractionCalculateDeviations(pmSubtractionStampList *stamps, ///< Stamps
                                           pmSubtractionKernels *kernels ///< Kernel parameters
    );

/// Calculate the value of a polynomial, specified by coefficients and polynomial values
double p_pmSubtractionCalculatePolynomial(const psVector *coeff, ///< Coefficients
                                          const psImage *polyValues, ///< Polynomial values
                                          int order, ///< Order of polynomials
                                          int index, ///< Index at which to begin
                                          int step ///< Step between subsequent indices
    );

/// Return the specified coefficient in the solution
double p_pmSubtractionSolutionCoeff(const pmSubtractionKernels *kernels, ///< Kernel parameters
                                    const psImage *polyValues, ///< Polynomial values
                                    int index, ///< Coefficient index to calculate
                                    bool wantDual ///< Calculate the coefficient for the dual solution?
    );

/// Return the normalisation in the solution
double p_pmSubtractionSolutionNorm(const pmSubtractionKernels *kernels ///< Kernel parameters
    );

/// Return the background (difference) in the solution
double p_pmSubtractionSolutionBackground(const pmSubtractionKernels *kernels, ///< Kernel parameters
                                         const psImage *polyValues ///< Polynomial values
    );

bool pmSubtractionCalculateNormalization(
  pmSubtractionStampList *stamps,
  const pmSubtractionMode mode);

bool pmSubtractionCalculateNormalizationStamp(
    pmSubtractionStamp *stamp,		// stamp on which to save normalization)
    const psKernel *input,		// Input image (target)
    const psKernel *reference,		// Reference image (convolution source)
    int footprint,			// (Half-)Size of stamp
    int normWindow1,			// Window (half-)size for normalisation measurement
    int normWindow2			// Window (half-)size for normalisation measurement
  );

bool pmSubtractionCalculateMoments(
    pmSubtractionKernels *kernels, // Kernels
    pmSubtractionStampList *stamps);

bool pmSubtractionCalculateMomentsStamp(
    pmSubtractionKernels *kernels, // Kernels
    pmSubtractionStamp *stamp,		// stamp on which to save normalization)
    int footprint,			// (Half-)Size of stamp
    int normWindow1,			// Window (half-)size for normalisation measurement
    int normWindow2			// Window (half-)size for normalisation measurement
    );

bool pmSubtractionCalculateMomentsKernel(double *Mxx, double *Myy, psKernel *image, int footprint, int window);

bool pmSubtractionChisqStats(psVector *fluxesVector, psVector *chisqDVector, psVector *chisqRVector, psVector *momentVector, psVector *stampMask, psKernel *convolved1, psKernel *convolved2, psKernel *difference, psKernel *residual, psKernel *weight, psKernel *window);

bool pmSubtractionCalculateChisqAndMoments(pmSubtractionQuality **bestMatch, pmSubtractionStampList *stamps, pmSubtractionKernels *kernels);
#endif

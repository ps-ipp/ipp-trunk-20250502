#ifndef PM_SUBTRACTION_PARAMS_H
#define PM_SUBTRACTION_PARAMS_H

#include <pslib.h>
#include <pmSubtractionKernels.h>
#include <pmSubtractionStamps.h>

/// Generate a set of optimum kernels for ISIS (or GUNK)
pmSubtractionKernels *pmSubtractionKernelsOptimumISIS(pmSubtractionKernelsType type, ///< Kernel type
                                                      int size, ///< Half-size of kernel
                                                      int inner, ///< Inner radius for GUNK
                                                      int spatialOrder, ///< Spatial polynomial order
                                                      const psVector *fwhms, ///< Gaussian FWHMs to try
                                                      int maxOrder, ///< Maximum polynomial order
                                                      const pmSubtractionStampList *stamps, ///< Stamps
                                                      int footprint, ///< Convolution footprint for stamps
                                                      float tolerance, ///< Maximum difference in chi^2
                                                      float penalty, ///< Penalty for wideness
                                                      psRegion bounds,       ///< Bounds of validity
                                                      pmSubtractionMode mode ///< Mode for subtraction
    );

#endif

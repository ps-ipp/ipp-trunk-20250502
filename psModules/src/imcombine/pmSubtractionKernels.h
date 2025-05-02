#ifndef PM_SUBTRACTION_KERNELS_H
#define PM_SUBTRACTION_KERNELS_H

// #include <string.h>
// #include <pslib.h>

// Assertion to check pmSubtractionKernels
#define PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(KERNELS, RETURNVALUE) { \
    PS_ASSERT_PTR_NON_NULL(KERNELS, RETURNVALUE); \
    PS_ASSERT_STRING_NON_EMPTY((KERNELS)->description, RETURNVALUE); \
    PS_ASSERT_INT_NONNEGATIVE((KERNELS)->num, RETURNVALUE); \
    PS_ASSERT_VECTOR_NON_NULL((KERNELS)->u, RETURNVALUE); \
    PS_ASSERT_VECTOR_NON_NULL((KERNELS)->v, RETURNVALUE); \
    PS_ASSERT_VECTOR_TYPE((KERNELS)->u, PS_TYPE_S32, RETURNVALUE); \
    PS_ASSERT_VECTOR_TYPE((KERNELS)->v, PS_TYPE_S32, RETURNVALUE); \
    PS_ASSERT_VECTOR_SIZE((KERNELS)->u, (KERNELS)->num, RETURNVALUE); \
    PS_ASSERT_VECTOR_SIZE((KERNELS)->v, (KERNELS)->num, RETURNVALUE); \
    if ((KERNELS)->type == PM_SUBTRACTION_KERNEL_ISIS) { \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->widths, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->widths, PS_TYPE_F32, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->widths, (KERNELS)->num, RETURNVALUE); \
    } \
    if ((KERNELS)->type == PM_SUBTRACTION_KERNEL_ISIS_RADIAL) { \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->widths, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->widths, PS_TYPE_F32, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->widths, (KERNELS)->num, RETURNVALUE); \
    } \
    if ((KERNELS)->type == PM_SUBTRACTION_KERNEL_SIMPLE) { \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->widths, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->widths, PS_TYPE_F32, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->widths, (KERNELS)->num, RETURNVALUE); \
    } \
    if ((KERNELS)->type == PM_SUBTRACTION_KERNEL_HERM) { \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->widths, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->widths, PS_TYPE_F32, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->widths, (KERNELS)->num, RETURNVALUE); \
    } \
    if ((KERNELS)->type == PM_SUBTRACTION_KERNEL_DECONV_HERM) { \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->widths, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->widths, PS_TYPE_F32, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->widths, (KERNELS)->num, RETURNVALUE); \
    } \
    if ((KERNELS)->uStop || (KERNELS)->vStop) { \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->uStop, RETURNVALUE); \
        PS_ASSERT_VECTOR_NON_NULL((KERNELS)->vStop, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->uStop, PS_TYPE_S32, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->vStop, PS_TYPE_S32, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->uStop, (KERNELS)->num, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->vStop, (KERNELS)->num, RETURNVALUE); \
    } \
    if ((KERNELS)->preCalc) { \
        PS_ASSERT_ARRAY_NON_NULL((KERNELS)->preCalc, RETURNVALUE); \
        PS_ASSERT_ARRAY_SIZE((KERNELS)->preCalc, (KERNELS)->num, RETURNVALUE); \
    } \
    PS_ASSERT_INT_NONNEGATIVE((KERNELS)->size, RETURNVALUE); \
    PS_ASSERT_INT_NONNEGATIVE((KERNELS)->inner, RETURNVALUE); \
    PS_ASSERT_INT_NONNEGATIVE((KERNELS)->spatialOrder, RETURNVALUE); \
    PS_ASSERT_INT_NONNEGATIVE((KERNELS)->bgOrder, RETURNVALUE); \
}

// Assertion to check that the solution is attached
#define PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(KERNELS, RETURNVALUE) { \
    PS_ASSERT_VECTOR_NON_NULL((KERNELS)->solution1, RETURNVALUE); \
    PS_ASSERT_VECTOR_TYPE((KERNELS)->solution1, PS_TYPE_F64, RETURNVALUE); \
    PS_ASSERT_VECTOR_SIZE((KERNELS)->solution1, \
                          (KERNELS)->num * PM_SUBTRACTION_POLYTERMS((KERNELS)->spatialOrder) + 1 + \
                              PM_SUBTRACTION_POLYTERMS((KERNELS)->bgOrder), \
                          RETURNVALUE); \
    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) { \
        PS_ASSERT_VECTOR_NON_NULL(kernels->solution2, RETURNVALUE); \
        PS_ASSERT_VECTOR_TYPE((KERNELS)->solution2, PS_TYPE_F64, RETURNVALUE); \
        PS_ASSERT_VECTOR_SIZE((KERNELS)->solution2, \
                              (KERNELS)->num * PM_SUBTRACTION_POLYTERMS((KERNELS)->spatialOrder), \
                               RETURNVALUE); \
    } \
}

// Generate 1D convolution kernel for SIMPLE
psVector *pmSubtractionKernelSIMPLE(float sigma, // Gaussian width
				    int order,   // Unused polynomial order
				    int size     // Kernel half-size
				    );

// Generate 1D convolution kernel for ISIS
psVector *pmSubtractionKernelISIS(float sigma, // Gaussian width
                                       int order, // Polynomial order
                                       int size // Kernel half-size
    );

// Generate 1D convolution kernel for HERM (normalized for 2D)
psVector *pmSubtractionKernelHERM(float sigma, // Gaussian width
                                       int order, // Polynomial order
                                       int size // Kernel half-size
    );

/// Generate a delta-function grid for subtraction kernels (like the POIS kernel)
bool p_pmSubtractionKernelsAddGrid(pmSubtractionKernels *kernels, ///< The subtraction kernels to append to
                                   int start, ///< Index at which to start appending
                                   int size ///< Half-size of the grid
    );

/// General allocator for pmSubtractionKernels
///
/// Unlike the functions for the specific kernel type, this function does not set up the basis functions, but
/// merely allocates space for their storage.
pmSubtractionKernels *pmSubtractionKernelsAlloc(int numBasisFunctions, ///< Number of basis functions
                                                pmSubtractionKernelsType type, ///< Kernel type
                                                int size, ///< Half-size of kernel
						psVector *fwhms, ///< requested kernel basis function
						psVector *orders,
                                                int spatialOrder, ///< Order of spatial variations
                                                float penalty, ///< Penalty for wideness
                                                psRegion bounds,       ///< Bounds for validity
                                                pmSubtractionMode mode ///< Mode for subtraction
    );

/// Allocator for pre-calculated kernel data structure
pmSubtractionKernelPreCalc *pmSubtractionKernelPreCalcAlloc(
    pmSubtractionKernelsType type, ///< type of kernel to allocate (not all can be pre-calculated)
    int uOrder,                    ///< order in x-direction
    int vOrder,                    ///< order in x-direction
    int size,                      ///< Half-size of the kernel
    float sigma                    ///< sigma of gaussian kernel
    );


/// Generate POIS kernels
pmSubtractionKernels *pmSubtractionKernelsPOIS(int size, ///< Half-size of the kernel (in both dims)
                                               int spatialOrder, ///< Order of spatial variations
                                               float penalty, ///< Penalty for wideness
                                               psRegion bounds,       ///< Bounds for validity
                                               pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate ISIS kernels without the flux scaling built in
pmSubtractionKernels *p_pmSubtractionKernelsRawISIS(int size, ///< Half-size of the kernel
                                                    int spatialOrder, ///< Order of spatial variations
                                                    const psVector *fwhms, ///< Gaussian FWHMs
                                                    const psVector *orders, ///< Polynomial order of gaussians
                                                    float penalty, ///< Penalty for wideness
                                                    psRegion bounds,       ///< Bounds for validity
                                                    pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate ISIS kernels
pmSubtractionKernels *pmSubtractionKernelsISIS(int size, ///< Half-size of the kernel
                                               int spatialOrder, ///< Order of spatial variations
                                               const psVector *fwhms, ///< Gaussian FWHMs
                                               const psVector *orders, ///< Polynomial order of gaussians
                                               float penalty, ///< Penalty for wideness
                                               psRegion bounds,       ///< Bounds for validity
                                               pmSubtractionMode mode ///< Mode for subtraction
                                               );

/// Generate ISIS + RADIAL_HERM kernels
pmSubtractionKernels *pmSubtractionKernelsISIS_RADIAL(int size, ///< Half-size of the kernel
                                                      int spatialOrder, ///< Order of spatial variations
                                                      const psVector *fwhms, ///< Gaussian FWHMs
                                                      const psVector *orders, ///< Polynomial order of gaussians
                                                      float penalty, ///< Penalty for wideness
                                                      psRegion bounds,       ///< Bounds for validity
                                                      pmSubtractionMode mode ///< Mode for subtraction
                                               );

/// Generate HERM kernels
pmSubtractionKernels *pmSubtractionKernelsHERM(int size, ///< Half-size of the kernel
                                               int spatialOrder, ///< Order of spatial variations
                                               const psVector *fwhms, ///< Gaussian FWHMs
                                               const psVector *orders, ///< order of hermitian polynomials
                                               float penalty, ///< Penalty for wideness
                                               psRegion bounds,       ///< Bounds for validity
                                               pmSubtractionMode mode ///< Mode for subtraction
                                               );

/// Generate DECONV_HERM kernels
pmSubtractionKernels *pmSubtractionKernelsDECONV_HERM(int size, ///< Half-size of the kernel
                                                      int spatialOrder, ///< Order of spatial variations
                                                      const psVector *fwhms, ///< Gaussian FWHMs
                                                      const psVector *orders, ///< order of hermitian polynomials
                                                      float penalty, ///< Penalty for wideness
                                                      psRegion bounds,       ///< Bounds for validity
                                                      pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate SPAM kernels
pmSubtractionKernels *pmSubtractionKernelsSPAM(int size, ///< Half-size of the kernel
                                               int spatialOrder, ///< Order of spatial variations
                                               int inner, ///< Inner radius to preserve unbinned
                                               int binning, ///< Kernel binning factor
                                               float penalty, ///< Penalty for wideness
                                               psRegion bounds,       ///< Bounds for validity
                                               pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate FRIES kernels
pmSubtractionKernels *pmSubtractionKernelsFRIES(int size, ///< Half-size of the kernel
                                                int spatialOrder, ///< Order of spatial variations
                                                int inner, ///< Inner radius to preserve unbinned
                                                float penalty, ///< Penalty for wideness
                                                psRegion bounds,       ///< Bounds for validity
                                                pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate GUNK kernels
pmSubtractionKernels *pmSubtractionKernelsGUNK(int size, ///< Half-size of the kernel
                                               int spatialOrder, ///< Order of spatial variations
                                               const psVector *fwhms, ///< Gaussian FWHMs
                                               const psVector *orders, ///< Polynomial order of gaussians
                                               int inner, ///< Inner radius containing grid of delta functions
                                               float penalty, ///< Penalty for wideness
                                               psRegion bounds,       ///< Bounds for validity
                                               pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate RINGS kernels
pmSubtractionKernels *pmSubtractionKernelsRINGS(int size, ///< Half-size of the kernel
                                                int spatialOrder, ///< Order of spatial variations
                                                int inner, ///< Inner radius to preserve unbinned
                                                int ringsOrder, ///< Polynomial order
                                                float penalty, ///< Penalty for wideness
                                                psRegion bounds,       ///< Bounds for validity
                                                pmSubtractionMode mode ///< Mode for subtraction
    );


/// Generate a kernel of a specified type
pmSubtractionKernels *pmSubtractionKernelsGenerate(pmSubtractionKernelsType type, ///< Kernel type
                                                   int size, ///< Half-size of the kernel
                                                   int spatialOrder, ///< Order of spatial variations
                                                   const psVector *fwhms, ///< Gaussian FWHMs
                                                   const psVector *orders, ///< Polynomial order of gaussians
                                                   int inner, ///< Inner radius to preserve unbinned
                                                   int binning, ///< Kernel binning factor
                                                   int ringsOrder, ///< Polynomial order for RINGS
                                                   float penalty, ///< Penalty for wideness
                                                   psRegion bounds,       ///< Bounds for validity
                                                   pmSubtractionMode mode ///< Mode for subtraction
    );

/// Generate a kernel using the description
pmSubtractionKernels *pmSubtractionKernelsFromDescription(
    const char *description,            ///< Description of kernel
    int bgOrder,                        ///< Polynomial order for background fitting
    psRegion bounds,                    ///< Bounds for validity
    pmSubtractionMode mode              ///< Mode for subtraction
    );

/// Return the appropriate type from a string
pmSubtractionKernelsType pmSubtractionKernelsTypeFromString(const char *string // String name for kernel type
    );

bool pmSubtractionKernelsMakeDescription(pmSubtractionKernels *kernels);


/// Copy kernels
///
/// A deep copy is performed on the solution only; the other components are merely pointers.
pmSubtractionKernels *pmSubtractionKernelsCopy(
    const pmSubtractionKernels *in      // Kernels to copy
    );

psImage *pmSubtractionKernelsImageMosaic(pmSubtractionKernels *kernels);

#endif

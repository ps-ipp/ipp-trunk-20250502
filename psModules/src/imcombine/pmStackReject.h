#ifndef PM_STACK_REJECT_H
#define PM_STACK_REJECT_H

#include <pslib.h>
#include <pmSubtractionKernels.h>

/// Given a list of pixels from the convolved image, find the corresponding (smaller subset of) pixels in the
/// original image, and then convolve those to get the list of all pixels which should be rejected
///
/// We apply a matched filter to the corresponding mask image, and threshold to find the original pixels
psPixels *pmStackReject(const psPixels *in, ///< List of pixels in the convolved image
                        int numCols, int numRows, ///< Size of image of interest
                        float threshold, ///< Threshold on convolved image, 0..1
                        int stride,     ///< Size of convolution patches
                        const psArray *regions, ///< Array of image regions for image
                        const psArray *kernels ///< Array of kernel parameters for each region
    );

/// Given a list of pixels from the convolved image, we grow them by convolution to get the list of all pixels
/// which should be rejected.
psPixels *pmStackRejectGrow(const psPixels *in, ///< List of pixels in the convolved image
                            int numCols, int numRows, ///< Size of image of interest
                            float poorFrac, ///< Fraction for "poor"
                            const psArray *regions, ///< Array of image regions for image
                            const psArray *kernels ///< Array of kernel parameters for each region
    );

/// Initialise threads for stack rejection
bool pmStackRejectThreadsInit(void);

#endif

#ifndef PM_SUBTRACTION_STAMPS_H
#define PM_SUBTRACTION_STAMPS_H

/// Allocate a list of stamps
pmSubtractionStampList *pmSubtractionStampListAlloc(
    int numCols, // Number of columns in image
    int numRows, // Number of rows in image
    const psRegion *region, // Region for stamps, or NULL
    int footprint, // Half-size of stamps
    float spacing, // Rough average spacing between stamps
    float normFrac, // Fraction of flux in window for normalisation window
    float sysErr,  // Relative systematic error or NAN
    float skyErr  // Relative systematic error or NAN
    );

/// Assertion for stamp list to be valid
#define PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(LIST, RETURNVALUE) { \
    PS_ASSERT_PTR_NON_NULL(LIST, RETURNVALUE); \
    PS_ASSERT_ARRAY_NON_NULL((LIST)->stamps, RETURNVALUE); \
    PS_ASSERT_ARRAY_NON_NULL((LIST)->regions, RETURNVALUE); \
    PS_ASSERT_INT_POSITIVE((LIST)->num, RETURNVALUE); \
    PS_ASSERT_ARRAY_SIZE((LIST)->stamps, (LIST)->num, RETURNVALUE); \
    PS_ASSERT_ARRAY_SIZE((LIST)->regions, (LIST)->num, RETURNVALUE); \
    PS_ASSERT_INT_NONNEGATIVE((LIST)->footprint, RETURNVALUE); \
    if ((LIST)->x || (LIST)->y || (LIST)->flux) { \
        PS_ASSERT_ARRAY_NON_NULL((LIST)->x, RETURNVALUE); \
        PS_ASSERT_ARRAY_NON_NULL((LIST)->y, RETURNVALUE); \
        PS_ASSERT_ARRAY_NON_NULL((LIST)->flux, RETURNVALUE); \
        PS_ASSERT_ARRAY_SIZE((LIST)->x, (LIST)->num, RETURNVALUE); \
        PS_ASSERT_ARRAY_SIZE((LIST)->y, (LIST)->num, RETURNVALUE); \
        PS_ASSERT_ARRAY_SIZE((LIST)->flux, (LIST)->num, RETURNVALUE); \
    } \
}

/// Copy a list of stamps
///
/// A deep copy is performed of the stamp list and the component stamps
pmSubtractionStampList *pmSubtractionStampListCopy(
    const pmSubtractionStampList *in    // Stamp list to copy
    );

/// Allocate a stamp
pmSubtractionStamp *pmSubtractionStampAlloc(void);

// find and extract the stamps
bool pmSubtractionStampsSelect(pmSubtractionStampList **stamps, // Stamps to read
			       const pmReadout *ro1, // Readout 1
			       const pmReadout *ro2, // Readout 2
			       const psImage *subMask, // Mask for subtraction, or NULL
			       psImage *variance,  // Variance map
			       const psRegion *region, // Region of interest
			       float thresh1,  // Threshold for stamp finding on readout 1
			       float thresh2,  // Threshold for stamp finding on readout 2
			       float stampSpacing, // Spacing between stamps
			       float normFrac,     // Fraction of flux in window for normalisation window
			       float sysError,     // Relative systematic error in images
			       float skyError,     // Relative systematic error in images
			       int size,         // Kernel half-size
			       int footprint,     // Convolution footprint for stamps
			       pmSubtractionMode mode // Mode for subtraction
    );


/// Find stamps on an image
pmSubtractionStampList *pmSubtractionStampsFind(
    pmSubtractionStampList *stamps, ///< Output stamps, or NULL
    const psImage *image1, ///< Image for which to find stamps
    const psImage *image2, ///< Image for which to find stamps
    const psImage *mask, ///< Mask, or NULL
    const psRegion *region, ///< Region to search, or NULL
    float thresh1, ///< Threshold for stamps in image 1
    float thresh2, ///< Threshold for stamps in image 2
    int size, ///< Kernel half-size
    int footprint, ///< Half-size for stamps
    float spacing, ///< Rough spacing for stamps
    float normFrac, // Fraction of flux in window for normalisation window
    float sysErr,  ///< Relative systematic error in images
    float skyErr,  ///< Relative systematic error in images
    pmSubtractionMode mode ///< Mode for subtraction
    );

/// Set stamps based on a list of x,y
pmSubtractionStampList *pmSubtractionStampsSet(
    const psVector *x, ///< x coordinates for each stamp
    const psVector *y, ///< y coordinates for each stamp
    const psImage *image, ///< Image for flux of stamp
    const psImage *mask, ///< Mask, or NULL
    const psRegion *region, ///< Region to search, or NULL
    int size, ///< Kernel half-size
    int footprint, ///< Half-size for stamps
    float spacing, ///< Rough spacing for stamps
    float normFrac, // Fraction of flux in window for normalisation window
    float sysErr,  ///< Systematic error in images
    float skyErr,  ///< Systematic error in images
    pmSubtractionMode mode ///< Mode for subtraction
    );

/// Set stamps based on a list of sources
pmSubtractionStampList *pmSubtractionStampsSetFromSources(
    const psArray *sources,             ///< Sources for each stamp
    const psImage *image,               ///< Image for flux of stamp
    const psImage *subMask,             ///< Mask, or NULL
    const psRegion *region,             ///< Region to search, or NULL
    int size,                           ///< Kernel half-size
    int footprint,                      ///< Half-size for stamps
    float spacing,                      ///< Rough spacing for stamps
    float normFrac, // Fraction of flux in window for normalisation window
    float sysErr,                       ///< Systematic error in images
    float skyErr,                       ///< Systematic error in images
    pmSubtractionMode mode              ///< Mode for subtraction
    );

/// Set stamps based on values in a file
pmSubtractionStampList *pmSubtractionStampsSetFromFile(
    const char *filename,               ///< Filename of file containing x,y (or x,y,flux) on each line
    const psImage *image,               ///< Image for flux of stamp
    const psImage *subMask,             ///< Mask, or NULL
    const psRegion *region,             ///< Region to search, or NULL
    int size,                           ///< Kernel half-size
    int footprint,                      ///< Half-size for stamps
    float spacing,                      ///< Rough spacing for stamps
    float normFrac, // Fraction of flux in window for normalisation window
    float sysErr,                       ///< Systematic error in images
    float skyErr,                       ///< Systematic error in images
    pmSubtractionMode mode              ///< Mode for subtraction
    );

/// Calculate the window and normalisation window from the stamps
bool pmSubtractionStampsGetWindow(
    bool *tryAgain, 			///< re-try with new stamp size?
    pmSubtractionStampList *stamps,     ///< List of stamps
    int kernelSize                      ///< Half-size of kernel
    );

/// Extract stamps from the images
bool pmSubtractionStampsExtract(pmSubtractionStampList *stamps, ///< Stamps
                                psImage *image1, ///< Reference image
                                psImage *image2, ///< Input image (or NULL)
                                psImage *variance, ///< Variance map
                                int kernelSize, ///< Kernel half-size
                                psRegion bounds ///< Bounds of validity
    );


/// Turn on/off generation of ds9 region files
///
/// Intended for debugging
void pmSubtractionRegions(bool state    ///< Generate ds9 region files?
    );

/// Open a file for ds9 regions
///
/// Intended for debugging
FILE *pmSubtractionStampsFile(const pmSubtractionStampList *stamps, ///< List of stamps, for outlines
                              const char *filename, ///< Filename to write
                              const char *description ///< Description of file
    );

/// Print a stamp position to ds9 region file
///
/// Intended for debugging
void pmSubtractionStampPrint(FILE *ds9, ///< ds9 region file
                             float x, float y, ///< Position of stamp
                             float size,///< Size of circle
                             const char *color ///< Colour
    );


bool pmSubtractionStampsResetStatus (pmSubtractionStampList *stamps);


bool pmSubtractionKernelPenaltiesStamp(pmSubtractionStamp *stamp, pmSubtractionKernels *kernels);
bool pmSubtractionKernelPenalties(pmSubtractionStamp *stamp, pmSubtractionKernels *kernels, int index);
float pmSubtractionKernelPenaltySingle(psKernel *kernel, bool zeroNull);

#endif

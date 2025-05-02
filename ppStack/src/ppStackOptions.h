#ifndef PPSTACK_OPTIONS_H
#define PPSTACK_OPTIONS_H

/// Options for stacking process
typedef struct {
    // Setup
    bool convolve;                      // Convolve images?
    bool matchZPs;                      // Adjust relative fluxes based on transparency analysis?
    bool photometry;                    // Perform photometry?
    bool doBackground;                  // Do background model combination?
    psMetadata *stats;                  // Statistics for output
    FILE *statsFile;                    // File to which to write statistics
    psArray *origImages, *origMasks, *origVariances; // Filenames of the original images
    psArray *convImages, *convMasks, *convVariances; // Filenames for the temporary convolved images
    psArray *origCovars;                // Original covariances matrices
    int num;                            // Number of inputs
    int quality;                        // Bad data quality flag

    bool clipPercent;                   // use percentile range to clip?

    // Prepare
    pmPSF *psf;                         // Target PSF
    psVector *inputSeeing;              // Input seeing FWHMs
    float targetSeeing;                 // Target seeing FWHM
    psVector *exposures;                // Exposure times
    float sumExposure;                  // Sum of exposure times
    float zp;                           // Zero point for output
    float zpErr;			// Zero point error for output
    float airmass;			// airmass for output image
    float airmassSlope;			// airmass slope used for zp analysis
    psVector *inputMask;                // Mask for inputs
    psArray *sourceLists;               // Individual lists of sources for matching
    psVector *norm;                     // Normalisation for each image
    psVector *zpInput;			// reported zero point of input exposure
    psVector *expTimeInput;		// exposure time of input exposure
    psVector *airmassInput;		// airmass of input exposure
    psArray *sources;                   // Matched sources
    float clippedMean;                  // clipped mean of input fwhm
    float clippedStdev;                 // clipped stdev of input fwhm 
    psVector *bscaleOffset;             // hack offset of bzero using 0.5*bscale if option, 0.0 if not
    // Convolve
    psArray *cells;                     // Cells for convolved images --- a handle for reading again
    psArray *kernels;                   // PSF-matching kernels --- required in the stacking
    psArray *regions;                   // PSF-matching regions --- required in the stacking
    int numCols, numRows;               // Size of image
    psVector *matchChi2;                // chi^2 for stamps from matching
    psVector *weightings;               // Combination weightings for images (1/noise^2)
    psArray *convCovars;                // Convolved covariance matrices
    // Combine initial
    pmReadout *outRO;                   // Output readout
    pmReadout *expRO;                   // Exposure readout
    psArray *inspect;                   // Array of arrays of pixels to inspect
    // Rejection
    psArray *rejected;                  // Rejected pixels
    // Background
    pmReadout *bkgRO;                   // Output background readout
    psArray   *bkgImages;               // Input background images
} ppStackOptions;

/// Allocator
ppStackOptions *ppStackOptionsAlloc(void);


#endif

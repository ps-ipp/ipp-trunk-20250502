#ifndef PP_IMAGE_H
#define PP_IMAGE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>  // for strcasecmp
#include <unistd.h>   // for unlink
#include "pslib.h"
#include "psmodules.h"
#include "psphot.h"
#include "psastro.h"
#include "ppStats.h"
#include "ppImageErrorCodes.h"

#define RECIPE_NAME "PPIMAGE"           // Name of the recipe to use
#define TIMER_TOTAL   "PPIMAGE.TOTAL"   // Name of timer for total time
#define TIMER_DETREND "PPIMAGE.DETREND" // Name of timer for detrend time
#define TIMER_PHOT    "PPIMAGE.PHOT"    // Name of timer for photometry time

// Options for ppImage processing
typedef struct {
    // actions which ppImage should perform
    bool doMaskBuild;                   // Build internal mask
    bool doVarianceBuild;               // Build internal variance map
    bool doApplyBurntool;               // apply burntool correction
    bool doApplyPixelZero;              // put to zero pixels underneath mask
    bool doMaskBurntool;                // mask potential burntool trails
    bool doMaskSat;                     // mask saturated pixels
    bool doMaskLow;                     // mask low pixels
    bool doMask;                        // Mask bad pixels
    bool doAuxMask;                     // apply auxillary mask
    bool doNonLin;                      // Non-linearity correction
    bool doNewNonLin;                   // Non-linearity correction
    bool doOverscan;                    // Overscan subtraction
    bool doNoiseMap;                    // Bias subtraction
    bool doBias;                        // Bias subtraction
    bool doDark;                        // Dark subtraction
    bool doRemnance;                    // Remnance masking
    bool doShutter;                     // Shutter correction
    bool doFlat;                        // Flat-field normalisation
    bool doPatternRow;                  // Row pattern correction
    bool doPatternCell;                 // Cell pattern correction
    bool doPatternContinuity;           // Cell continuity correction
    bool doBackgroundContinuity;        // Do mosaic continuity correction
    bool doPatternDeadCells;            // match dead cell background patterns
    bool doFringe;                      // Fringe subtraction
    bool doPhotom;                      // Source identification and photometry
    bool doBG;                          // Background subtraction
    bool doAstromChip;                  // per-chip Astrometry
    bool doAstromMosaic;                // full-mosaic Astrometry
    bool doStats;                       // call ppStats on the image
    bool checkCTE;                      // measure pixel-based variance
    bool checkNoise;                    // measure cell-level variance
    bool squashNANs;                    // measure cell-level variance
    bool applyParity;                   // Apply Cell parities
    bool doMaskStats;                   // Calculate mask statistics
  
    bool doCrosstalkMeasure;            // measure crosstalk signal
    bool doCrosstalkCorrect;            // apply crosstalk correction
    bool addNoise;                      // Add noise to degrade MD image to 3pi

    bool hasVideo;                      // Determine if this OTA has a video cell
    bool useVideoDark;                  // Should we use a video dark if we can?
    bool useVideoMask;                  // Should we use a video mask if we can?
  
    // output files requested
    bool BaseFITS;
    bool BaseMaskFITS;
    bool BaseVarianceFITS;

    bool ChipFITS;
    bool ChipMaskFITS;
    bool ChipVarianceFITS;

    bool FPA1FITS;
    bool FPA2FITS;
    bool Bin1FITS;
    bool Bin1JPEG;
    bool Bin2FITS;
    bool Bin2JPEG;

    // make values for abstract concepts of masking
    psImageMaskType maskValue;          // apply this bit-mask to choose masked bits
    psImageMaskType markValue;          // apply this bit-mask to choose masked bits
    psImageMaskType satMask;            // Mask value to give saturated pixels
    psImageMaskType lowMask;            // Mask value to give bad pixels
    psImageMaskType flatMask;           // Mask value to give bad flat pixels
    psImageMaskType darkMask;           // Mask value to give bad dark pixels
    psImageMaskType blankMask;          // Mask value to give blank pixels
    psImageMaskType burntoolMask;       // Suspect pixels that fall where a burntool trail is expected.
    // non-linear correction parameters
    psDataType nonLinearType;
    psMetadataItem *nonLinearData;
    void *nonLinearSource;

    // options for the analysis
    pmOverscanOptions *overscan;        // Overscan options
    int burntoolTrails;
    // binning parameters
    int xBin1;                          // x-binning, scale 1
    int yBin1;                          // y-binning, scale 1
    int xBin2;                          // x-binning, scale 2
    int yBin2;                          // y-binning, scale 2

    // parameters used by the fringe analysis
    float fringeRej;                    // Fringe rejection limit
    int fringeIter;                     // Fringe iterations
    float fringeKeep;                   // Fringe keep fraction

    // Pattern correction
    int patternRowOrder;                   // Polynomial order
    int patternRowIter;                    // Clipping iterations
    float patternRowRej;                   // Clipping threshold
    float patternRowThresh;                // Ignore threshold
    psStatsOptions patternRowMean;         // Statistic for mean
    psStatsOptions patternRowStdev;        // Statistic for stdev
    psStatsOptions patternCellBG;          // statistic for background
    psStatsOptions patternCellMean;        // Statistic for mean

    int patternContinuityEdgeWidth;        // Size of box to use for edge matching.
  
    int remnanceSize;                   // Size for remnance detection
    float remnanceThresh;               // Threshold for remnance detection

    char *normClass;                    // class to use for per-class normalization

    psU16 maskstat_static;
    psU16 maskstat_dynamic;
    psU16 maskstat_magic;
    psU16 maskstat_advisory;

    psString auxVideoMask;                // auxillary video mask file
  
} ppImageOptions;

// Cells to be used in the detrend
typedef struct {
    pmCell *input;                      // The input cell, to be operated upon
    pmCell *mask;                       // The bad pixel mask
    pmCell *bias;                       // The bias correction
    pmCell *dark;                       // The dark correction
    pmCell *flat;                       // The flat-field correction
} ppImageDetrend;

ppImageOptions *ppImageOptionsAlloc(void);

// Determine the processing options
ppImageOptions *ppImageOptionsParse(pmConfig *config);

// Get the configuration
pmConfig *ppImageArguments(int argc, char **argv);

// Determine what type of camera, and initialise
ppImageOptions *ppImageParseCamera(pmConfig *config);
bool ppImageSetMaskBits (pmConfig *config, ppImageOptions *options);
bool ppImageMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
  );

// apply the cell flips to the input data before analysis
bool ppImageParityFlip (pmConfig *config, const ppImageOptions *options, const pmFPAview *view, bool native);

// Loop over the input
bool ppImageLoop(pmConfig *config, ppImageOptions *options);

// free memory, check for leaks
void ppImageCleanup (pmConfig *config, ppImageOptions *options);

// perform the detrend analysis on the current readout
bool ppImageDetrendReadout (pmConfig *config, ppImageOptions *options, pmFPAview *view);

bool ppImageDetrendBias(pmReadout *inputReadout, pmReadout *bias, pmReadout *dark, ppImageOptions *options);

bool ppImageDetrendNewNonLinear(pmReadout *input, pmFPAview *linearity, pmConfig *config);

bool ppImageDetrendNonLinear(pmReadout *input, pmFPAview *linearity, pmConfig *config);
bool ppImageDetrendNonLinearLookup(pmReadout *input, psMetadataItem *dataItem);
bool ppImageDetrendNonLinearPolynomial(pmReadout *input, psMetadataItem *dataItem);

bool ppImageDetrendCell(ppImageDetrend *detrend, ppImageOptions *options, pmConfig *config);
bool ppImageDetrendBias(pmReadout *inputReadout, pmReadout *bias, pmReadout *dark, ppImageOptions *options);
pmReadout* ppImageDetrendSelectFirst(pmCell *cell, char *name, bool doThis);

bool ppImageDetrendFree(pmConfig *config, pmFPAview *view);
bool ppImageFringeFree(pmConfig *config, pmFPAview *view);

bool ppImageCheckCTE(pmConfig *config, ppImageOptions *options, pmFPAview *view);

bool ppImageCheckNoise(pmConfig *config, ppImageOptions *options, pmFPAview *view);

bool ppImageSquashNANs(pmConfig *config, ppImageOptions *options, pmFPAview *view);

bool ppImageBurntoolMask(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmReadout *readout);
bool ppImageBurntoolMaskFromTable(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmReadout *readout);

bool ppImageBurntoolApply(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmReadout *readout);

bool ppImageDetrendPatternApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options);
bool ppImageDetrendPatternApplyCell (pmConfig *config, pmFPA *fpa, pmChip *chip, pmCell *cell, pmFPAview *view, ppImageOptions *options);

// Do background continuity step
bool ppImageMosaicBackground(pmConfig *config, const ppImageOptions *options);

// Record which detrend file was used for the detrending
bool ppImageDetrendRecord(
    pmCell *cell,                       // Cell of interest
    const pmConfig *config,             // Configuration
    const ppImageOptions *options,      // Processing options
    const pmFPAview *view               // View to cell
    );

bool ppImageRebinChip (pmConfig *config, pmFPAview *view, ppImageOptions *options, char *outName);

bool ppImagePhotom(psMetadata *stats, pmConfig *config, pmFPAview *view);
bool ppImageAstrom(pmConfig *config, psMetadata *stats);
bool ppImageAddstar(pmConfig *config);

// Subtract background from the chip-mosaicked image
bool ppImageSubtractBackground(
    pmConfig *config,                   // Configuration
    const pmFPAview *view,              // View to chip of interest
    const ppImageOptions *options       // Processing options
    );

bool ppImageMosaicChip (pmConfig *config, const ppImageOptions *options, const pmFPAview *view,
                        const char *outFile, const char *inFile);
bool ppImageMosaicFPA (pmConfig *config, const ppImageOptions *options,
                       const char *outFile, const char *inFile);

bool ppImageSetMaskBits (pmConfig *config, ppImageOptions *options);

void ppImageFileCheck (pmConfig *config);

// functions used by ppFocus
pmConfig *ppFocusArguments(int argc, char **argv);
ppImageOptions *ppFocusParseCamera (pmConfig *config, int entry);
bool ppFocusGetFWHM (pmConfig *config, psVector *focus, psVector *fwhm);
bool ppFocusFitFWHM (pmConfig *config, psVector *focus, psVector *fwhm);

void ppFocusDropCamera (pmConfig *config);

bool ppImageDefineFile (pmConfig *config, pmFPA *input, char *filerule, char *argname, pmFPAfileType fileType, pmDetrendType detrendType);

// write stats to output file
bool ppImageStatsOutput(pmConfig *config, // Configuration
                        psMetadata *stats, // Statistics output
                        const ppImageOptions *options // Options
    );


// measure the crosstalk signal
bool ppImageMeasureCrosstalk(pmConfig *config, ppImageOptions *options, pmFPAview *view);

// correct the crosstalk signal
bool ppImageCorrectCrosstalk(pmConfig *config, ppImageOptions *options, pmFPAview *view);

// Measure fringes
bool ppImageDetrendFringeMeasure(pmReadout *readout, // Readout to measure
                                 pmCell *fringe, // Fringe cell (each readout is a different component)
                                 const bool isResidual,
                                 const ppImageOptions *options // Options
    );

// Solve the fringe system
bool ppImageDetrendFringeSolve(pmChip *scienceChip, // Chip with science
                               const pmChip *refChip, // Chip with reference fringes
                               const bool isResidual,
                               const ppImageOptions *options // Options
    );

// Generate fringe frame
bool ppImageDetrendFringeGenerate(pmCell *science, // Science cell
                                  pmCell *fringes, // Fringe cell, one readout per fringe component
                                  const ppImageOptions *options // Options
    );


bool ppImageDetrendFringeApply (pmConfig *config, // config
                                pmChip *chip, // science chip
                                const pmFPAview *inputView, // current view
                                const ppImageOptions *options // options
    );

/// Return short version information
psString ppImageVersion(void);

/// Return software source
psString ppImageSource(void);

/// Return long version information
psString ppImageVersionLong(void);

/// Populate the header with version information for all dependencies
bool ppImageVersionHeader(psMetadata *metadata ///< Header to populate
    );

/// Print version information
void ppImageVersionPrint(void);


// calculate stats, including MD5
bool ppImagePixelStats(pmConfig *config,// Configuration
                       psMetadata *stats, // Statistics output
                       const ppImageOptions *options, // Options
                       const pmFPAview *inputView // View to data
    );

// Calculate Mask statistics
bool ppImageMaskStats(pmConfig *config, pmFPAview *view, psMetadata *stats);

// calculate stats from headers and concepts
bool ppImageMetadataStats(pmConfig *config, // Configuration
                          psMetadata *stats, // Statistics output
                          const ppImageOptions *options // Options
    );

void ppImageFileCheck(pmConfig *config);

/// Dump memory summary to text file
void ppImageMemoryDump(const char *description);


//Functions needed to degrade MD exposures to 3pi exposures

bool ppImageAddNoise(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmFPA *fpa) ;
double ppImageRandomGaussian (const psRandom *rnd, double mean, double sigma);
double ppImageRandomGaussianNorm (const psRandom *rnd);
void ppImageRandomGaussianFree(void);

bool ppImageAuxiliaryMask(pmConfig *config, const pmFPAview *view, const ppImageOptions *options, psMetadata *stats);

bool ppImageSetThreads (void);

#endif

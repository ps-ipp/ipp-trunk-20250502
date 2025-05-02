#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>
#include <psphot.h>

#ifndef PPSTACK_H
#define PPSTACK_H

#define PPSTACK_RECIPE "PPSTACK"        // Name of the recipe
#define PPSTACK_INSPECT_PIXELS "PPSTACK.PIXELS" // Name of rejected pixels metadata items

#include <pslib.h>
#include <psmodules.h>

// Mask values for inputs
typedef enum {
    PPSTACK_MASK_NONE   = 0x00,         // Nothing wrong
    PPSTACK_MASK_CAL    = 0x01,         // Photometric calibration failed
    PPSTACK_MASK_PSF    = 0x02,         // PSF measurement failed
    PPSTACK_MASK_MATCH  = 0x04,         // PSF-matching failed
    PPSTACK_MASK_CHI2   = 0x08,         // Chi^2 too deviant
    PPSTACK_MASK_REJECT = 0x10,         // Rejection failed
    PPSTACK_MASK_BAD    = 0x20,         // Bad image (too many pixels rejected)
    PPSTACK_MASK_ALL    = 0xff          // All errors
} ppStackMask;

// List of files
typedef enum {
    PPSTACK_FILES_NONE,                 // NOP list
    PPSTACK_FILES_PREPARE,              // Files for preparation
    PPSTACK_FILES_TARGET,               // Files for target generation
    PPSTACK_FILES_CONVOLVE,             // Files for convolution
    PPSTACK_FILES_STACK,                // Stack files
    PPSTACK_FILES_UNCONV,               // Unconvolved stack files
    PPSTACK_FILES_PHOT,                 // Files for photometry
    PPSTACK_FILES_BKG,                  // Files for bkg
    PPSTACK_FILES_MEDIAN_IN,                // Files for median only stacks.
    PPSTACK_FILES_MEDIAN_OUT                // Files for median only stacks.
} ppStackFileList;

#include "ppStackOptions.h"
#include "ppStackThread.h"
#include "ppStackLoop.h"
#include "ppStackErrorCodes.h"

// Setup command-line arguments
bool ppStackArgumentsSetup(int argc, char *argv[], // Command-line arguments
                           pmConfig *config  // Configuration
    );

// Parse command-line arguments
bool ppStackArgumentsParse(pmConfig *config  // Configuration
    );

// Parse cameras
bool ppStackCamera(pmConfig *config     // Configuration
    );

// Determine target PSF for input images
pmPSF *ppStackPSF(const pmConfig *config, // Configuration
                  int numCols, int numRows, // Size of image
                  const psArray *psfs,  // List of input PSFs
                  ppStackOptions *options // full options including Mask for inputs
    );

// Re-do photometry on input sources
//
// Photometry for the sources is replaced by what is measured
bool ppStackInputPhotometry(const pmReadout *ro, // Readout
                            const psArray *sources, // Sources to photometer
                            const pmConfig *config // Configuration
    );

// Perform stacking on a readout
//
// Returns two arrays: pixels to inspect for each input image, and pixels to reject for each input image.
psArray *ppStackReadoutInitial(const pmConfig *config,   // Configuration
                               pmReadout *outRO,   // Output readout
                               const psArray *readouts, // Input readouts
                               const psVector *mask, // Mask for input readouts
                               const psVector *weightings, // Weighting factors for each image
                               const psVector *exposures,  // Exposure time for each image
                               const psVector *addVariance // Additional variance for rejection
    );

// Thread entry point for ppStackReadoutInitial
bool ppStackReadoutInitialThread(psThreadJob *job // Job to process
    );

// Concatenate inspection lists for each input image
bool ppStackInspect(psThreadJob *job    // Job to process
    );

// Perform stacking on a readout
bool ppStackReadoutFinal(const pmConfig *config,   // Configuration
                         pmReadout *outRO,   // Output readout
                         pmReadout *expRO,   // Exposure readout
                         const psArray *readouts, // Input readouts
                         const psVector *mask, // Mask for input readouts
                         const psArray *rejected, // Array with pixels rejected in each image
                         const psVector *weightings, // Weighting factors for each image
                         const psVector *exposures,  // Exposure times for each image
                         const psVector *addVariance, // Additional variance for rejection
                         bool safety,                 // Enable safety switch?
                         const psVector *norm,         // Normalisations to apply
                         const psVector *bscaleApplyOffset  // hack for offset based on bscale
    );

// Thread entry point for ppStackReadoutFinal
bool ppStackReadoutFinalThread(psThreadJob *job // Job to process
    );

// Thread entry point for ppStackReadoutPercent
bool ppStackReadoutPercentThread(psThreadJob *job);

bool ppStackReadoutPercent(const pmConfig *config,
			   pmReadout *outRO,
			   pmReadout *expRO,
			   const psArray *readouts,
			   const psVector *mask,
			   const psVector *weightings,
			   const psVector *exposures,
			   const psVector *addVariance);

// Perform median stacking for background
bool ppStackCombineBackground(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config);
bool ppStackCombineMedian(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config);
bool ppStackLinearScale(psArray *inputs, pmConfig *config);

// Return software version
psString ppStackVersion(void);

/// Return software source
psString ppStackSource(void);

// Return long description of software version
psString ppStackVersionLong(void);

// Supplement header with software version
bool ppStackVersionHeader(psMetadata *header // Header to supplement
    );

/// Print version information
void ppStackVersionPrint(void);

/// Generate target PSF image
psImage *ppStackTarget(ppStackOptions *options, // Options for stacking
                       pmConfig *config         // Configuration
    );

/// Convolve image to match specified seeing
bool ppStackMatch(pmReadout *readout,   // Readout to be convolved; replaced with output
                  const psImage *target,   // Target PSF image
                  ppStackOptions *options, // Options for stacking
                  int index,            // Index of image to match
                  const pmConfig *config // Configuration
    );


/// Calculate transparency differences between images
///
/// Corrects the source PSF photometry to a common system.  Return the sum of the exposure times.
bool ppStackSourcesTransparency(ppStackOptions *options, // Stacking options
                                const pmFPAview *view, // View to readout
                                const pmConfig *config // Configuration
    );

/// Dump memory debugging information
void ppStackMemDump(
    const char *name                    ///< Stage name, for inclusion in the output file name
    );

/// Activate/deactivate a list of files
void ppStackFileActivation(
    pmConfig *config,                   // Configuration
    ppStackFileList list,               // Files to turn on/off
    bool state                          // Activation state
    );

// Activate/deactivate a single element for a list
void ppStackFileActivationSingle(
    pmConfig *config,                   // Configuration
    ppStackFileList list,               // Files to turn on/off
    bool state,                         // Activation state
    int num                             // Number of file in sequence
    );

/// Iterate down the hierarchy, loading files
///
/// We can get away with this simplistic treatment of the FPA hierarchy because we're working on skycells.
pmFPAview *ppStackFilesIterateDown(
    pmConfig *config                    // Configuration
    );

/// Iterate up the hierarchy, writing files
///
/// We can get away with this simplistic treatment of the FPA hierarchy because we're working on skycells.
bool ppStackFilesIterateUp(
    pmConfig *config                    // Configuration
    );

/// Write an image to a FITS file
bool ppStackWriteImage(
    const char *name,                   // Name of image
    psMetadata *header,                 // Header
    const psImage *image,               // Image
    pmConfig *config                    // Configuration
    );

bool ppStackWriteVariance(const char *name, // Name of image
			  psMetadata *header, // Header
			  const psImage *variance, // Variance
			  const psImage *covariance, // Variance
			  pmConfig *config // Configuration
    );

/// Return an appropriate exit code based on the error code
psExit ppStackExitCode(psExit exitValue);

bool ppStackCleanup(pmConfig *config, ppStackOptions *options) PS_ATTR_NORETURN;

#endif

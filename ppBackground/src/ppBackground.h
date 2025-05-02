#ifndef PP_VIZPSF_H
#define PP_VIZPSF_H

#include <pslib.h>
#include <psmodules.h>

#include "ppBackgroundErrorCodes.h"

#define PPBACKGROUND_RECIPE "PPBACKGROUND"      // Recipe name

// Data for processing
typedef struct {
    psString patternName;               // Filename of pattern
    psString backgroundName;            // Filename of background
    psString imageName;                 // Filenames of input image
    psString maskName;                  // Filenames of input mask
    psString varianceName;              // Filenames of input variance
    psString auxMaskName;               // Filename of auxillary mask
    psString outRoot;                   // Output root name
    psMetadata *stats;                  // Statistics for output
    FILE *statsFile;                    // Output statistics file
    pmConfig *config;                   // Configuration
} ppBackgroundData;

/// Initialise data for processing
ppBackgroundData *ppBackgroundDataInit(int *argc, char *argv[] // Command-line arguments
    );

/// Parse command-line arguments
bool ppBackgroundArguments(ppBackgroundData *data, // Data for processing
                        int argc, char *argv[] // Command-line arguments
    );

/// Parse camera configurations
bool ppBackgroundCamera(ppBackgroundData *data // Data for processing
    );

/// Loop over input data, processing
bool ppBackgroundLoop(ppBackgroundData *data // Data for processing
    );

/// Determine the binning from the recipe if available.
psImageBinning *ppBackgroundBinningByRecipe(const psImage *image, // Image for which to generate a bg model
					    const pmConfig *config, // Configuration
					    psString recipe_name,
					    psString Xbin_name,
					    psString Ybin_name
					    );


/// Restore the background to an image
bool ppBackgroundRestore(
    pmChip *chip,                       // Chip to correct
    const pmChip *background,           // Chip with background model
    const pmChip *pattern,              // Chip with pattern
    const pmFPAview *view,              // View to data
    pmConfig *config,                   // Configuration
    psImageMaskType maskBad             // value to use for bad pixels
    );

/// Determine exit code
psExit ppBackgroundExitCode(
    psExit exitValue                    // Current exit code
    );

/// Add version information to header
bool ppBackgroundVersionHeader(
    psMetadata *header                  // Header to supplement
    );

/// Print version information to stdout
void ppBackgroundVersionPrint(void);

#endif

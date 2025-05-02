#ifndef DVO_MAKE_CORR_H
#define DVO_MAKE_CORR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "pslib.h"
#include "psmodules.h"

#define RECIPE_NAME "DVOCORR"		// Name of the recipe to use
#define TIMERNAME "dvoMakeCorr"		// Name of timer

// Options for dvoMakeCorr
typedef struct {
    int test;
} dvoMakeCorrOptions;

dvoMakeCorrOptions *dvoMakeCorrOptionsAlloc(void);

dvoMakeCorrOptions *dvoMakeCorrOptionsParse(pmConfig *config);

// Get the configuration
pmConfig *dvoMakeCorrArguments(int argc, char **argv);

// Determine what type of camera, and initialise
dvoMakeCorrOptions *dvoMakeCorrParseCamera(pmConfig *config);

// Loop over the input
bool dvoMakeCorrLoop(pmConfig *config, dvoMakeCorrOptions *options);

// convert low-res image to hi-res image
bool dvoMakeCorrUnbin (pmConfig *config, pmFPAview *view, char *outName, psImage *inImage, pmChip *inChip, char *refName);

// free memory, check for leaks
void dvoMakeCorrCleanup (pmConfig *config, dvoMakeCorrOptions *options);

/// Return short version information
psString dvoMakeCorrVersion(void);

/// Return long version information
psString dvoMakeCorrVersionLong(void);

/// Update the metadata with version information for all dependencies
void dvoMakeCorrVersionMetadata(psMetadata *metadata ///< Metadata to update with version information
    );

#endif

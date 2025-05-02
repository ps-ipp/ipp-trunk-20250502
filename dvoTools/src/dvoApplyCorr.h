#ifndef DVO_MAKE_CORR_H
#define DVO_MAKE_CORR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include "pslib.h"
#include "psmodules.h"

#define RECIPE_NAME "DVOCORR"		// Name of the recipe to use
#define TIMERNAME "dvoApplyCorr"		// Name of timer

// Options for dvoApplyCorr
typedef struct {
    int test;
} dvoApplyCorrOptions;

dvoApplyCorrOptions *dvoApplyCorrOptionsAlloc(void);

dvoApplyCorrOptions *dvoApplyCorrOptionsParse(pmConfig *config);

// Get the configuration
pmConfig *dvoApplyCorrArguments(int argc, char **argv);

// Determine what type of camera, and initialise
dvoApplyCorrOptions *dvoApplyCorrParseCamera(pmConfig *config);

// Loop over the input
bool dvoApplyCorrLoop(pmConfig *config, dvoApplyCorrOptions *options);

// apply the correction factor pixel-by-pixel
// XXX : old value bool dvoApplyCorrReadout (pmConfig *config, pmFPAview *view, char *inName, char *corrName);
bool dvoApplyCorrReadout (pmCell *inCell, pmChip *corrChip);

// free memory, check for leaks
void dvoApplyCorrCleanup (pmConfig *config, dvoApplyCorrOptions *options);

/// Return short version information
psString dvoApplyCorrVersion(void);

/// Return long version information
psString dvoApplyCorrVersionLong(void);

/// Update the metadata with version information for all dependencies
void dvoApplyCorrVersionMetadata(psMetadata *metadata ///< Metadata to update with version information
    );

#endif

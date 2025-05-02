#ifndef PP_SMOOTH_H
#define PP_SMOOTH_H

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

#define RECIPE_NAME   "PPSMOOTH"         // Name of the recipe to use
#define TIMER_TOTAL   "PPSMOOTH.TOTAL"   // Name of timer for total time
#define TIMER_DETREND "PPSMOOTH.DETREND" // Name of timer for detrend time
#define TIMER_PHOT    "PPSMOOTH.PHOT"    // Name of timer for photometry time

// Get the configuration
pmConfig *ppSmoothArguments(int argc, char **argv);

// Determine what type of camera, and initialise
bool ppSmoothParseCamera(pmConfig *config);

// Loop over the input
bool ppSmoothLoop(pmConfig *config);

// free memory, check for leaks
void ppSmoothCleanup (pmConfig *config);

// perform the detrend analysis on the current readout
bool ppSmoothReadout (pmConfig *config, pmFPAview *view);

bool ppSmoothDefineFile (pmConfig *config, pmFPA *input, char *filerule, char *argname, pmFPAfileType fileType, pmDetrendType detrendType);

/// Return short version information
psString ppSmoothVersion(void);

/// Return software source
psString ppSmoothSource(void);

/// Return long version information
psString ppSmoothVersionLong(void);

/// Populate the header with version information for all dependencies
bool ppSmoothVersionHeader(psMetadata *metadata ///< Header to populate
    );

// update header without multiple updates
bool ppSmoothVersionUpdateHeader (pmFPA *fpa, pmChip *chip, pmCell *cell);

/// Print version information
void ppSmoothVersionPrint(void);

#endif

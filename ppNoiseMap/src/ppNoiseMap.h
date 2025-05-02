#ifndef PP_NOISE_MAP_H
#define PP_NOISE_MAP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>  // for strcasecmp
#include <unistd.h>   // for unlink
#include "pslib.h"
#include "psmodules.h"

#define RECIPE_NAME   "PPNOISEMAP"	 // Name of the recipe to use
#define TIMER_TOTAL   "PPNOISEMAP.TOTAL" // Name of timer for total time

/// Get the configuration
pmConfig *ppNoiseMapArguments(int argc, char **argv);

/// Determine what type of camera, and initialise
bool ppNoiseMapParseCamera(pmConfig *config);

/// Loop over the input
bool ppNoiseMapLoop(pmConfig *config);

/// free memory, check for leaks
void ppNoiseMapCleanup (pmConfig *config);

/// define the input files
bool ppNoiseMapDefineFile (pmConfig *config, pmFPA *input, char *filerule, char *argname, pmFPAfileType fileType);

/// Return short version information
psString ppNoiseMapVersion(void);

/// Return software source
psString ppNoiseMapSource(void);

/// Return long version information
psString ppNoiseMapVersionLong(void);

/// Populate the header with version information for all dependencies
bool ppNoiseMapVersionHeader(psMetadata *header);

/// Print version information
void ppNoiseMapVersionPrint(void);

/// perform the noise measurement
bool ppNoiseMapReadout (pmConfig *config, pmFPAview *view);

/// measure the noise for the readout
bool ppNoiseMapStats(pmReadout *out, const pmReadout *in, psImageMaskType maskVal, int xBin, int yBin);
#endif

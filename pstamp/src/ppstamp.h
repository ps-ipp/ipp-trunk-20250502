#ifndef PP_STAMP_H
#define PP_STAMP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pstampint.h"
#include "ppstampOptions.h"

#define TIMERNAME "ppstamp"             // Name of timer

// Determine the processing options
bool *ppstampOptionsParse(pmConfig *config);

// Get the configuration
pmConfig *ppstampArguments(int argc, char **argv, ppstampOptions **);

// Determine what type of camera, and initialise
bool ppstampParseCamera(pmConfig *config, ppstampOptions *options);

int ppstampMakeStamp(pmConfig *config, ppstampOptions *);
pmFPAfile * ppstampBuildMosaic(pmConfig *config, pmFPAfile *input, pmFPAview *view);

// free memory, check for leaks
void ppstampCleanup (pmConfig *config, ppstampOptions *options);

/// Return short version information
psString ppstampVersion(void);

/// Return long version information
psString ppstampVersionLong(void);

/// Update the metadata with version information for all dependencies
void ppstampVersionMetadata(psMetadata *metadata, ppstampOptions *options);

psRegion *ppstampCellRegion(const pmCell *cell);
psRegion *ppstampChipRegion(const pmChip *chip);

extern bool ppstampMegacamWorkaround;
#endif

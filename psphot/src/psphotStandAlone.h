# ifdef HAVE_CONFIG_H
# include <config.h>
# endif

#ifndef PSPHOT_STAND_ALONE_H
#define PSPHOT_STAND_ALONE_H

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include "psphot.h"
#include "psphotStatsFile.h"

// Top level functions
pmConfig       *psphotArguments (int argc, char **argv);
bool            psphotParseCamera (pmConfig *config);
bool            psphotMosaicChip(pmConfig *config, const pmFPAview *view, char *outFile, char *inFile);
void            psphotCleanup (pmConfig *config);
psExit          psphotGetExitStatus (void);

#endif

#ifndef PP_VIZPSF_H
#define PP_VIZPSF_H

#include <pslib.h>
#include <psmodules.h>

#define PPVIZPATTERN_RECIPE "PPVIZPATTERN"      // Recipe name

// Data for processing
typedef struct {
    psString patternName;               // Filename with pattern
    psString outRoot;                   // Output root name
    pmConfig *config;                   // Configuration
} ppVizPatternData;

/// Initialise data for processing
ppVizPatternData *ppVizPatternDataInit(int *argc, char *argv[] // Command-line arguments
    );

/// Parse command-line arguments
bool ppVizPatternArguments(ppVizPatternData *data, // Data for processing
                        int argc, char *argv[] // Command-line arguments
    );

/// Parse camera configurations
bool ppVizPatternCamera(ppVizPatternData *data // Data for processing
    );

/// Loop over input data, processing
bool ppVizPatternLoop(ppVizPatternData *data // Data for processing
    );

/// Add version information to header
bool ppVizPatternVersionHeader(
    psMetadata *header                  // Header to supplement
    );

/// Print version information to stdout
void ppVizPatternVersionPrint(void);

#endif

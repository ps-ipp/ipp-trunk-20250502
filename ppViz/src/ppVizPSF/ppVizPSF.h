#ifndef PP_VIZPSF_H
#define PP_VIZPSF_H

#include <pslib.h>
#include <psmodules.h>

#define PPVIZPSF_RECIPE "PPVIZPSF"      // Recipe name

// Data for processing
typedef struct {
    psString psfName;                   // Filename with PSF
    psString sourcesName;               // Filename with sources
    int fakeNum;                        // Number of fake sources
    float fakeMag;                      // Magnitude of fake sources
    psString outRoot;                   // Output root name
    float minFlux;                      // Minimum flux for sources
    bool useResiduals;			// include PSF residuals in output?
    int size;                           // Size of PSF image
    float x, y;                         // Position of fake source
    psArray *input;                     // Input positions and magnitudes
    pmConfig *config;                   // Configuration
} ppVizPSFData;

/// Initialise data for processing
ppVizPSFData *ppVizPSFDataInit(int *argc, char *argv[] // Command-line arguments
    );

/// Parse command-line arguments
bool ppVizPSFArguments(ppVizPSFData *data, // Data for processing
                        int argc, char *argv[] // Command-line arguments
    );

/// Parse camera configurations
bool ppVizPSFCamera(ppVizPSFData *data // Data for processing
    );

/// Loop over input data, processing
bool ppVizPSFLoop(ppVizPSFData *data // Data for processing
    );

/// Add version information to header
bool ppVizPSFVersionHeader(
    psMetadata *header                  // Header to supplement
    );

/// Print version information to stdout
void ppVizPSFVersionPrint(void);

#endif

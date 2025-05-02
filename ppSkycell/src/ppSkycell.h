#ifndef PP_SKYCELL_H
#define PP_SKYCELL_H

#include <pslib.h>
#include <psmodules.h>

#define PPSKYCELL_RECIPE "PPSKYCELL"    // Recipe name

// Data for processing
typedef struct {
    psString imagesName;                // Filename with images
    psString wcsrefName;                // Filename with WCS references
    psString outRoot;                   // Output root name
    int numInputs;                      // Number of inputs
    psImageMaskType maskVal;            // Value to mask
    int bin1, bin2;                     // Binning factors
    pmConfig *config;                   // Configuration
    bool doFits;                        // hold whether to do fits as well.
    int  exptimeOrder;                  // Order of exptime scaling.
} ppSkycellData;

/// Initialise data for processing
ppSkycellData *ppSkycellDataInit(int *argc, char *argv[] // Command-line arguments
    );

/// Parse command-line arguments
bool ppSkycellArguments(ppSkycellData *data, // Data for processing
                        int argc, char *argv[] // Command-line arguments
    );

/// Parse camera configurations
bool ppSkycellCamera(ppSkycellData *data // Data for processing
    );

/// Loop over input data, processing
bool ppSkycellLoop(ppSkycellData *data // Data for processing
    );


#endif

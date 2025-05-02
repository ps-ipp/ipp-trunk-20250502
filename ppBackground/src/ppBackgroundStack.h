#ifndef PP_BACKGROUNDSTACK_H
#define PP_BACKGROUNDSTACK_H

#include <pslib.h>
#include <psmodules.h>

#include "ppBackgroundErrorCodes.h"

#define PPBACKGROUND_RECIPE "PPBACKGROUND_STACK"      // Recipe name

// Data for processing
typedef struct {
  psMetadata *contents;   // Metadata containing information about the data arrays
  psArray *smfs;          // List of pointers to individual smf headers
  
  // data->models->name->XY__->{image/ra/dec/calibrated/offset/scale}
  // data->models->counter->XY__->{image/ra/dec/calibrated/offset/scale}
  psMetadata *models;     // Metadata containing the information about the model data
  
  psMetadata *OTA_solutions; // List of pointers to the OTA-specific solution images
  bool fit_OTAS;          // Calculate OTA solutions based on these inputs.
  psString OTApath;       // Location for pre-solved OTA solutions.

  psImageMap *modelMap;
  psS32 model_iteration;
  // These are the full extent of the input data
  psF32 ra_min;
  psF32 ra_max;
  psF32 dec_min;
  psF32 dec_max;

  // Because the binning code can't handle subsections sanely, these hold the working subsection.
  psF32 x_min;
  psF32 x_max;
  psF32 y_min;
  psF32 y_max;

  psArray *stack_data;
  psArray *stacks;        // List of stacks to be corrected.
  psString outRoot;       // Output root name
  pmConfig *config;       // Configuration
} ppBackgroundStackData;

/// Initialise data for processing
ppBackgroundStackData *ppBackgroundStackDataInit(int *argc, char *argv[] // Command-line arguments
    );

/// Parse command-line arguments
bool ppBackgroundStackArguments(ppBackgroundStackData *data, // Data for processing
				int argc, char *argv[] // Command-line arguments
    );

/// Parse camera configurations
bool ppBackgroundStackCamera(ppBackgroundStackData *data // Data for processing
    );

/// Loop over input data, processing
bool ppBackgroundStackLoop(ppBackgroundStackData *data // Data for processing
    );

bool ppBackgroundStackModelFitOTASolution(ppBackgroundStackData *data);
bool ppBackgroundStackDataModelFit(ppBackgroundStackData *data);
bool ppBackgroundStackCalibApply(ppBackgroundStackData *data);
bool ppBackgroundStackModelFit(ppBackgroundStackData *data);


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

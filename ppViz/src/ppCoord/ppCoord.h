#ifndef PP_VIZPSF_H
#define PP_VIZPSF_H

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>


#define PPVIZPSF_RECIPE "PPVIZPSF"      // Recipe name

// Data for processing
typedef struct {
    psString astromName;                // Filename with astrometry
    psString rawName;                   // Filename with raw image (or NULL)
    psString pixelsName;                // Filename with pixel coordinates
    psString chipName;                  // Name of chip of interest
    psString radecName;                 // Filename with sky coordinates
    psString streaksName;               // Filename with streaks (sky coordinates)
    psString clustersName;              // Filename with clusters (sky coordinates)
    pmConfig *config;                   // Configuration
    bool radians;                       // RA,Dec are in radians?
    bool all;                           // Only all coordinates?
    psString ds9name;                   // Name of ds9 region file
    FILE *ds9;                          // ds9 output file handle
    float ds9radius;                    // Radius of ds9 regions
    psString ds9color;                  // Color of ds9 regions
} ppCoordData;

/// Initialise data for processing
ppCoordData *ppCoordDataInit(int *argc, char *argv[] // Command-line arguments
    );

/// Parse command-line arguments
bool ppCoordArguments(ppCoordData *data, // Data for processing
                        int argc, char *argv[] // Command-line arguments
    );

/// Parse camera configurations
bool ppCoordCamera(ppCoordData *data // Data for processing
    );

/// Loop over input data, processing
bool ppCoordLoop(ppCoordData *data // Data for processing
    );

/// Add version information to header
bool ppCoordVersionHeader(
    psMetadata *header                  // Header to supplement
    );

/// Print version information to stdout
void ppCoordVersionPrint(void);

#endif

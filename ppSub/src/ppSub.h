/** @file ppSub.h
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PP_SUB_H
#define PP_SUB_H

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSubErrorCodes.h"

/// @addtogroup ppSub
/// @{

#define PPSUB_RECIPE "PPSUB"            /// Name of the recipe to use
#define WCS_TOLERANCE 0.001             // Tolerance for WCS

// Output files, for activation/deactivation
typedef enum {
    PPSUB_FILES_INPUT    = 0x01,        // Input files
    PPSUB_FILES_CONV     = 0x02,        // Convolved files (output)
    PPSUB_FILES_SUB      = 0x04,        // Subtracted files (output)
    PPSUB_FILES_INV      = 0x08,        // Inverse subtracted files (output)
    PPSUB_FILES_PSF      = 0x10,        // PSF files (output)
    PPSUB_FILES_PHOT_SUB = 0x20,        // Subtraction photometry files (output)
    PPSUB_FILES_PHOT_INV = 0x40,        // Inverse subtraction photometry files (output)
    PPSUB_FILES_PHOT     = 0x80,        // General photometry files (internal)
    PPSUB_FILES_ALL      = 0xFF,        // All files
} ppSubFiles;

/// Data for processing
typedef struct {
    pmConfig *config;                   // Configuration
    psErrorCode quality;                // Quality code; 0 for no problem
    bool photometry;                    // Perform photometry?
    bool inverse;                       // Output inverse subtraction as well?
    bool forcedPhot1;                   // perform forced photometry?
    bool forcedPhot2;                   // perform forced photometry?
    bool saveInConv;                    // Save convolved input?
    bool saveRefConv;                   // Save convolved reference?
    psString stamps;                    // Stamps file
    pmPSF *psf;                         // Point Spread Function
    psString statsName;                 // Name of statistics file
    FILE *statsFile;                    // Statistics file
    psMetadata *stats;                  // Statistics
} ppSubData;

/// Constructor
ppSubData *ppSubDataAlloc(pmConfig *config ///< Configuration
    );

/// Setup the arguments parsing
bool ppSubArguments(int argc, char *argv[], ///< Command-line arguments
                    ppSubData *data ///< Processing data
    );

/// Parse the camera input
bool ppSubCamera(ppSubData *data        ///< Processing data
    );

/// Loop over the FPA hierarchy
bool ppSubLoop(ppSubData *data          ///< Processing data
    );

/// Generate (if needed) and set or update the masks for input and reference images
bool ppSubSetMasks(pmConfig *config     ///< Configuration
    );

// Calculate mask statistics
bool ppSubMaskStats(pmConfig *config, pmFPAview *view, psMetadata *stats);

/// Generate the PSF-matching kernel and convolve the images as needed.  Most of this function involves
/// looking up the parameters in the recipe and supplying them to the function pmSubtractionMatch()
bool ppSubMatchPSFs(ppSubData *data     ///< Processing data
    );

/// Threshold low pixels in image
bool ppSubLowThreshold(ppSubData *data  ///< Processing data
    );

/// Generate the output readout and pass the kernel info to the header
bool ppSubDefineOutput(const char *name,///< Name of output to define
                       pmConfig *config ///< Configuration
    );

/// Photometry stage 1: measure the PSF from the minuend image
bool ppSubMakePSF(ppSubData *data       ///< Processing data
    );

/// Perform the actual image subtraction, update output concepts
bool ppSubReadoutSubtract(pmConfig *config ///< Configuration
    );


/// Photometry stage 2: find and measure sources on the subtracted image
bool ppSubReadoutPhotometry(const char *name, ///< Name of file to photometer
                            ppSubData *data ///< Processing data
    );

bool ppSubInputDetections (bool *foundDetections, const char *sourcesName, const char *imageName, ppSubData *data);
bool ppSubReadoutForcedPhot(const char *outputName, const char *targetName, const char *sourceName, ppSubData *data);
bool psphotCopyResults (bool *foundDetections, pmFPAfile *target, pmFPAfile *source, pmFPAview *view);

/// Higher-order background subtraction
bool ppSubBackground(pmConfig *config   ///< Configuration
    );

/// Perform Variance correction (rescale within a modest range)
bool ppSubVarianceRescale(pmConfig *config,   ///< Configuration
                          ppSubData *data     ///< Processing data
    );

/// Put the program version information into a header
bool ppSubVersionHeader(psMetadata *header ///< Header to populate
    );

/// Print version information
void ppSubVersionPrint(void);

/// write the version info to a string
psString ppSubVersionLong(void);

/// Mark the data quality as bad and prepare to suspend processing
void ppSubDataQuality(ppSubData *data,  ///< Processing data
                      psErrorCode error,///< Error code
                      ppSubFiles files  ///< Files to deactivate
    );


/// Activate or deactivate files
void ppSubFilesActivate(pmConfig *config, // Configuration
                        ppSubFiles files, // File to activate/deactivate
                        bool state      // Activation state
    );

/// Generate a view suitable for a readout
///
/// Assumes we're working with skycells
pmFPAview *ppSubViewReadout(void);

/// Iterate down the FPA hierarchy, opening files
bool ppSubFilesIterateDown(pmConfig *config, // Configuration
                           ppSubFiles files // Files to open
    );

/// Iterate up the FPA hierarchy, closing files
bool ppSubFilesIterateUp(pmConfig *config, // Configuration
                         ppSubFiles files // Files to open
    );

/// Collect statistics
bool ppSubReadoutStats(ppSubData *data  // Processing data
    );

/// Generate JPEG images
bool ppSubReadoutJpeg(pmConfig *config  // Configuration
    );

/// Generate JPEG images
bool ppSubResidualSampleJpeg(pmConfig *config);

/// Generate inverse subtraction
bool ppSubReadoutInverse(pmConfig *config // Configuration
    );


// Copy every instance of a single keyword from one metadata to another
bool psMetadataCopySingle(psMetadata *target, psMetadata *source, const char *name);

bool ppSubCopyPSF (pmFPAfile *output, pmFPAfile *input, pmFPAview *view);

/// Return appropriate exit code
psExit ppSubExitCode(psExit exitValue   // Current exit value
    );

bool ppSubFlagNeighbors(pmConfig *config, pmFPAview *view, psArray *sources, bool matchRef);
bool ppSubMatchSources (psArray *objects, psArray *sources, float RADIUS, float MIN_SN);
bool ppSubSetSourceImageIDs (psArray *sources, int imageID);

void ppSubSetThreads (void);

// ppSubMaskSetInMetadata examines named mask values and set the bits for maskValue and
// markValue.  Ensures that the below-named mask values are set, and calculates the mask value
// to catch all of the mask values marked as 'bad'.  Supplies the fallback name if the primary
// name is not found, or the default values if the fallback name is not found.
bool ppSubMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
  );

// lookup an image mask value by name from a psMetadata, without requiring the entry to
// be of type psImageMaskType, but verifying that it will fit in psImageMaskType
psImageMaskType psMetadataLookupImageMaskFromGeneric (bool *status, const psMetadata *md, const char *name);

///@}
#endif

/** @file ppMerge.h
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PP_MERGE_H
#define PP_MERGE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

/// @addtogroup ppArith
/// @{

#define TIMERNAME "ppMerge"             ///< Name for timer
#define PPMERGE_RECIPE "PPMERGE"        ///< Recipe name
#define THREADED 1                      ///< Compile with threads?

/**
 * Type of frame to merge
 */
typedef enum {
    PPMERGE_TYPE_UNKNOWN,               ///< Unknown type
    PPMERGE_TYPE_BIAS,                  ///< Bias frame
    PPMERGE_TYPE_DARK,                  ///< (Multi-)Dark frame
    PPMERGE_TYPE_MASK,                  ///< Mask frame
    PPMERGE_TYPE_CTEMASK,		///< CTE Mask based on flat variance
    PPMERGE_TYPE_SHUTTER,               ///< Shutter frame
    PPMERGE_TYPE_FLAT,                  ///< Flat-field frame (dome or sky)
    PPMERGE_TYPE_FRINGE,                ///< Fringe frame
    PPMERGE_TYPE_NOISEMAP,              ///< Noise map
} ppMergeType;

/**
 * Files, for activation
 */
typedef enum {
    PPMERGE_FILES_ALL,                  ///< All files
    PPMERGE_FILES_INPUT,                ///< Input files
    PPMERGE_FILES_OUTPUT                ///< Output files
} ppMergeFiles;

/**
 * \brief Group of files to read
 *
 * Each file contributes a readout, into which is read a chunk from that file
 */
typedef struct {
    psArray *readouts;                  ///< Input readouts
    bool read;                          ///< Has the scan been read?
    bool busy;                          ///< Is the scan being processed?
    int firstScan;                      ///< First row of the chunk to be read for this group
    int lastScan;                       ///< Last row of the chunk to be read for this group
} ppMergeFileGroup;

/**
 * Parse command-line arguments and recipe
 */
bool ppMergeArguments(int argc, char *argv[], ///< Command-line arguments
                      pmConfig *config  ///< Configuration
    );

/**
 * Set up camera files
 */
bool ppMergeCamera(pmConfig *config     ///< Configuration
    );

/**
 * Measure scale and zero-points
 */
bool ppMergeScaleZero(pmConfig *config  ///< Configuration
    );

/**
 * Main loop to do the merging
 */
bool ppMergeLoop(pmConfig *config       ///< Configuration
    );

/**
 * Main loop for masks
 */
bool ppMergeMask(pmConfig *config       ///< Configuration
    );

/**
 * Read nominated input file
 */
bool ppMergeFileReadInput(pmConfig *config, ///< Configuration
                          pmReadout *readout, ///< Readout into which to read
                          int num,      ///< Number of file in sequence
                          int rows      ///< Number of rows to read at once
    );

/**
 * Open nominated input file
 */
bool ppMergeFileOpenInput(pmConfig *config, ///< Configuration
                          const pmFPAview *view, ///< View to open
                          int num       ///< Number of file in sequence
    );

bool ppMergeFileFreeInput(pmConfig *config, const pmFPAview *view, int num);

/**
 * Set the data level for files specified by name; return an array of the files
 */
psArray *ppMergeFileDataLevel(const pmConfig *config, ///< Configuration
                              const char *name ///< Name of files
    );

/**
 * Activate/deactivate a list of files
 */
bool ppMergeFileActivate(const pmConfig *config, ///< Configuration
                         ppMergeFiles files, ///< Files to turn on/off
                         bool state     ///< Activation state
    );

/**
 * Activate/deactivate a single element for a list; return array of files
 */
psArray *ppMergeFileActivateSingle(const pmConfig *config, ///< Configuration
                                   ppMergeFiles files, ///< Files to turn on/off
                                   bool state,   ///< Activation state
                                   int num ///< Number of file in sequence
    );

/**
 * Return name of output pmFPAfile
 */
psString ppMergeOutputFile(const pmConfig *config ///< Configuration
    );


/**
 * Allocator for group of files
 */
ppMergeFileGroup *ppMergeFileGroupAlloc(void);

/**
 * Read chunk into the first available file group
 */
ppMergeFileGroup *ppMergeReadChunk(bool *status, ///< Status of read
                                   psArray *fileGroups, ///< All groups
                                   pmConfig *config, ///< Configuration
                                   int numChunk ///< Chunk number (only for interest)
    );

/**
 * Set up thread handling
 */
bool ppMergeSetThreads(void);


/// Return software version
psString ppMergeVersion(void);

/// Return software source
psString ppMergeSource(void);

/// Return detailed software version information
psString ppMergeVersionLong(void);

/// Populate a FITS header with version information
bool ppMergeVersionHeader(
    psMetadata *header                  ///< Header to populate
    );

/// Print version information
void ppMergeVersionPrint(void);


///@}
#endif

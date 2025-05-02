/* @file  pmFPAview.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.35 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 *
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_FILE_H
#define PM_FPA_FILE_H

#include <pslib.h>

#include <pmFPALevel.h>
#include <pmFPA.h>
#include <pmFPAview.h>
#include <pmDetrendDB.h>

/// @addtogroup Camera Camera Layout
/// @{

typedef enum {
    PM_FPA_BEFORE,
    PM_FPA_AFTER,
} pmFPAfilePlace;

typedef enum {
    PM_FPA_FILE_NONE,
    PM_FPA_FILE_SX,
    PM_FPA_FILE_OBJ,
    PM_FPA_FILE_CMP,
    PM_FPA_FILE_CMF,
    PM_FPA_FILE_CFF,
    PM_FPA_FILE_WCS,
    PM_FPA_FILE_RAW,
    PM_FPA_FILE_IMAGE,
    PM_FPA_FILE_MASK,
    PM_FPA_FILE_VARIANCE,
    PM_FPA_FILE_FRINGE,
    PM_FPA_FILE_DARK,
    PM_FPA_FILE_PSF,
    PM_FPA_FILE_JPEG,
    PM_FPA_FILE_KAPA,
    PM_FPA_FILE_HEADER,
    PM_FPA_FILE_ASTROM_MODEL,
    PM_FPA_FILE_ASTROM_REFSTARS,
    PM_FPA_FILE_KH_CORRECT,
    PM_FPA_FILE_SUBKERNEL,
    PM_FPA_FILE_SRCTEXT,
    PM_FPA_FILE_PATTERN,
    PM_FPA_FILE_PATTERN_ROW_AMP,
    PM_FPA_FILE_PATTERN_DEAD_CELLS,
    PM_FPA_FILE_LINEARITY,
    PM_FPA_FILE_NEWNONLIN,
    PM_FPA_FILE_EXPNUM,
} pmFPAfileType;

typedef enum {
    PM_FPA_MODE_NONE,
    PM_FPA_MODE_READ,
    PM_FPA_MODE_WRITE,
    PM_FPA_MODE_INTERNAL,
    PM_FPA_MODE_REFERENCE,
} pmFPAfileMode;

typedef enum {
    PM_FPA_STATE_OPEN     = 0x01,
    PM_FPA_STATE_CLOSED   = 0x02,
    PM_FPA_STATE_INACTIVE = 0x04,
} pmFPAfileState;

typedef struct {
    pmFPAfileMode mode;                 // is this file read, written, or only used internally?
    pmFPAfileType type;                 // what type of data is read from / written to disk?
    pmFPAfileState state;               // have we opened the file, etc?

    pmFPALevel fileLevel;               // what level in the FPA hierarchy represents a unique file?
    pmFPALevel dataLevel;               // at what level do we read/write the data segment? (request by user)
    pmFPALevel freeLevel;               // at what level do we free the data segment? (set by program)
    pmFPALevel mosaicLevel;             // at what level is the mosaic?

    pmFPA *fpa;                         // for I/O files, we carry a pointer to the complete fpa
    psFits *fits;                       // for I/O files of fits type (IMAGE, CMP, CMF) we carry a file handle
    psFitsCompression *compression;     // Compression for FITS images
    psFitsOptions *options;             // FITS I/O options

    bool wrote_phu;                     // have we written a PHU for this file?
    psMetadata *header;                 // pointer (view) to the current hdu header

    pmReadout *readout;                 // for internal files, we only carry a single readout

    psMetadata *names;                  // filenames supplied by the cmdline or detdb are saved here

    char *filerule;                     // rule for constructing a filename when needed
    char *filesrc;                      // rule to find file in pmFPAfile->names list

    char *name;                         // the name of the rule (useful for debugging / tracing)
    char *filename;                     // the current name of an active file
    char *origname;                     // the original name (before mangling) of an active file
    char *extname;                      // the current name of an active file extension

    pmDetrendSelectResults *detrend;    // Detrend information, from pmDetrendSelect

    bool save;                          // Should the file be saved?

    // the following elements are used for WRITE-mode IMAGE-type pmFPAfiles to inform
    // the creation of a new image based on an existing image
    pmFPA *src;                         // if an output FPA, inherit from this FPA
    int xBin;                           // desired binning in x direction
    int yBin;                           // desired binning in y direction

    psMetadata *camera;                 // Camera configuration
    psString cameraName;                // Name of the camera
    psMetadata *format;                 // Camera format
    psString formatName;                // name of the camera format

    int fileIndex;			// Index of file
    psS64 fileID;		        // internal sequence number

    psS64 imageId, sourceId;            // Image and source identifiers
} pmFPAfile;

// allocate an empty pmFPAfile structure
pmFPAfile *pmFPAfileAlloc(void);

// select the readout from the named pmFPAfile; if the named file does not exist,
pmReadout *pmFPAfileThisReadout (psMetadata *files, const pmFPAview *view, const char *name);

// select the cell from the named pmFPAfile; if the named file does not exist,
pmCell *pmFPAfileThisCell (psMetadata *files, const pmFPAview *view, const char *name);

// select the chip from the named pmFPAfile; if the named file does not exist,
pmChip *pmFPAfileThisChip (psMetadata *files, const pmFPAview *view, const char *name);

// add the specified filename info (value) to the files of the given mode using the given reference name
bool pmFPAfileAddFileNames (psMetadata *files, char *name, char *value, int mode);

// convert the rule to a name based on the current view
psString pmFPANameFromRule(const char *rule, const pmFPA *fpa, const pmFPAview *view);

// convert the rule to a name based on the current view
psString pmFPAfileNameFromRule(const char *rule, const pmFPAfile *file, const pmFPAview *view);

bool pmFPAfileCopyView (pmFPA *out, pmFPA *in, const pmFPAview *view);

bool pmFPAfileCopyStructureView (pmFPA *out, const pmFPA *in, int xBin, int yBin, const pmFPAview *view);

// Return the file type enum from a string
pmFPAfileType pmFPAfileTypeFromString(const char *type);

// Return the file type as a string
const char *pmFPAfileStringFromType(pmFPAfileType type);

/// Select files with the same name from the list of files
///
/// Returns all files if name is NULL.
psArray *pmFPAfileSelect(psMetadata *files, ///< All files
                         const char *name ///< Name of file(s) to return, or NULL for all
    );

/// Select a specific instance of a file from the list of files
///
/// Returns the num-th instance of all files if name is NULL.
pmFPAfile *pmFPAfileSelectSingle(psMetadata *files, ///< All files
                                 const char *name, ///< Name of file
                                 int num ///< Instance number of specific instance
    );

/// Set strict checking when freeing file
///
/// If true (default), freeing a pmFPAfile involves asserting that the FITS filehandle has been closed.
bool pmFPAfileFreeSetStrict(bool new    // New state
    );


/// @}
# endif

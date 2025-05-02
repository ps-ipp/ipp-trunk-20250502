/* @file  psFits.h
 * @brief Contains Fits I/O routines
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.37 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-12 22:54:53 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_FITS_H
#define PS_FITS_H

/// @addtogroup FileIO Input/Output
/// @{

#include <fitsio.h>
#include "psType.h"
#include "psArray.h"
#include "psVector.h"
#include "psMetadata.h"
#include "psImage.h"
#include "psFitsFloat.h"

#include "psErrorCodes.h"

/// FITS HDU type.
typedef enum {
    PS_FITS_TYPE_NONE = -1,            ///< Unknown HDU type
    PS_FITS_TYPE_IMAGE = IMAGE_HDU,    ///< Image HDU type
    PS_FITS_TYPE_BINARY_TABLE = BINARY_TBL, ///< Binary table HDU type
    PS_FITS_TYPE_ASCII_TABLE = ASCII_TBL,   ///< ASCII table HDU type
    PS_FITS_TYPE_ANY = ANY_HDU         ///< Any HDU type
} psFitsType;

/// FITS compression type
typedef enum {
    PS_FITS_COMPRESS_NONE = 0,          ///< No compression
    PS_FITS_COMPRESS_GZIP,              ///< GZIP compression (of the pixels only)
    PS_FITS_COMPRESS_RICE,              ///< RICE compression (of the pixels only)
    PS_FITS_COMPRESS_HCOMPRESS,         ///< HCOMPRESS compression (of the pixels only)
    PS_FITS_COMPRESS_PLIO               ///< PLIO compression (of the pixels only; appropriate for masks)
} psFitsCompressionType;

/// FITS scaling method: how to set BSCALE and BZERO
typedef enum {
    PS_FITS_SCALE_NONE,                 ///< No auto-scaling to be applied (BSCALE = 1, BZERO = 0)
    PS_FITS_SCALE_RANGE,                ///< Auto-scale to preserve dynamic range
    PS_FITS_SCALE_STDEV_POSITIVE,       ///< Auto-scale to sample stdev, place mean at lower limit
    PS_FITS_SCALE_STDEV_NEGATIVE,       ///< Auto-scale to sample stdev, place mean at upper limit
    PS_FITS_SCALE_STDEV_BOTH,           ///< Auto-scale to sample stdev, place mean at middle
    PS_FITS_SCALE_MANUAL,                ///< Manual scaling (use specified BSCALE and BZERO)
    PS_FITS_SCALE_LOG_RANGE,            ///< Take logarithm, Auto-scale to preserve dynamic range
    PS_FITS_SCALE_LOG_STDEV_POSITIVE,   ///< Take logarithm, Auto-scale to sample stdev, place mean at lower limit
    PS_FITS_SCALE_LOG_STDEV_NEGATIVE,   ///< Take logarithm, Auto-scale to sample stdev, place mean at upper limit
    PS_FITS_SCALE_LOG_STDEV_BOTH,       ///< Take logarithm, Auto-scale to sample stdev, place mean at middle
    PS_FITS_SCALE_LOG_MANUAL,           ///< Manual scaling (use specified BSCALE, BZERO, and BOFFSET)
    PS_FITS_SCALE_ASINH_RANGE,          ///< Do asinh scaling, auto-scale to preserve dynamic range
    PS_FITS_SCALE_ASINH_STDEV_POSITIVE, ///< Do asinh scaling, auto-scale to sample stdev, place mean at lower limit
    PS_FITS_SCALE_ASINH_STDEV_NEGATIVE, ///< Do asinh scaling, auto-scale to sample stdev, place mean at upper limit
    PS_FITS_SCALE_ASINH_STDEV_BOTH,     ///< Do asinh scaling, auto-scale to sample stdev, place mean at middle
    PS_FITS_SCALE_ASINH_MANUAL          ///< Manual scaling after doing asinh scaling.(use specified BSCALE, BZERO, BOFFSET, BSOFTEN)
} psFitsScaling;

/// Options for FITS I/O
typedef struct {
    char *extword;                      ///< user-specified word to name extensions (NULL implies EXTNAME)
    struct {
        bool compression;               ///< Compression convention: handling of compressed images
        bool psBitpix;                  ///< Custom floating-point image
    } conventions;                      ///< Conventions to honour
    // The following options are particular to writing images; they needn't be set for anything else.
    psFitsFloat floatType;              ///< Desired custom floating-point for output images
    int bitpix;                         ///< Desired BITPIX for output images; 0 to use as provided
    psFitsScaling scaling;              ///< Scaling scheme to use when quantising floating-point values
    bool fuzz;                          ///< Fuzz the values when quantising floating-point values?
    double bscale, bzero;               ///< Manually specified BSCALE and BZERO (for SCALE_MANUAL)
    double boffset;                     ///< Manually specified BOFFSET (for SCALE_MANUAL)
    double bsoften;                     ///< Manuallly specified BSOFTEN (for SCALE_MANUAL)
    double mean, stdev;                 ///< Mean and standard deviation of image
    int stdevBits;                      ///< Number of bits to sample a standard deviation (for SCALE_STDEV_*)
    float stdevNum;                     ///< Number of standard deviations to pad off the edge
} psFitsOptions;


/// FITS file
typedef struct {
    fitsfile* fd;                       ///< the CFITSIO fits files handle.
    bool writable;                      ///< Is the file writable?
    psFitsOptions *options;             ///< Options for FITS I/O, or NULL
} psFits;


/** FITS compression settings. */
typedef struct {
    psFitsCompressionType type;         ///< type of compression
    psVector *tilesize;                 ///< vector defining compression tile size
    int noisebits;                      ///< noise bits
    int scale;                          ///< hcompress scale
    int smooth;                         ///< hcompress smothing
} psFitsCompression;

/** Opens a FITS file and allocates the associated psFits object.
 *
 *  @return psFits*    new psFits object for the FITS files specified or
 *                     NULL if the open of the FITS file failed
 */
psFits* psFitsOpen(
    const char* filename,              ///< the FITS file name
    const char* mode
    /**< File open mode. Could be one of the following:
     *       'r' (read only),
     *       'r+' (read & write),
     *       'rw' (same as 'r+'), or
     *       'w' (create new file for writing)
     */
);


/// Generate an error including the cfitsio error string
#ifdef DOXYGEN
psErrorCode psFitsError(
    int status,                         ///< cfitsio status value
    bool new,                           ///< new error?
    const char *errorMsg,               ///< printf-style format of header line
    ...                                 ///< any parameters required in format
    );
#else // ifdef DOXYGEN
psErrorCode p_psFitsError(
    const char* filename,               ///< file name
    unsigned int lineno,                ///< line number in file
    const char* func,                   ///< function name
    int status,                         ///< cfitsio status value
    bool new,                           ///< new error?
    const char *errorMsg,               ///< printf-style format of header line
    ...                                 ///< any parameters required in format
    ) PS_ATTR_FORMAT(printf, 6, 7);
#ifndef SWIG
#define psFitsError(status,new,...) \
      p_psFitsError(__FILE__,__LINE__,__func__,status,new,__VA_ARGS__)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN

/// dump the full CFITSIO error stack
#ifdef DOXYGEN
bool psFitsDumpErrors(
    psErrorCode code,                   ///< psLib code to use
    const char *message,                ///< printf-style format of header line
    ...                                 ///< any parameters required in format
    );
#else // ifdef DOXYGEN
bool p_psFitsDumpErrors(
    const char* filename,               ///< file name
    unsigned int lineno,                ///< line number in file
    const char* func,                   ///< function name
    psErrorCode code,                   ///< psLib code to use
    const char *message,                ///< printf-style format of header line
    ...                                 ///< any parameters required in format
    ) PS_ATTR_FORMAT(printf, 5, 6);
#ifndef SWIG
#define psFitsDumpErrors(CODE,...) \
        p_psFitsDumpErrors(__FILE__,__LINE__,__func__,CODE,__VA_ARGS__)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN

/** Creates a new FITS options struct
 *
 *  @return psFitsOptions or NULL on failure
 */
psFitsCompression* psFitsCompressionAlloc(
    psFitsCompressionType type,         ///< type of compression
    psVector *tilesize,                 ///< vector defining compression tile size
    int noisebits,                      ///< noise bits
    int scale,                          ///< hcompress scale
    int smooth                          ///< hcompress smothing
);

/// Return the FITS compression type specified by a string
psFitsCompressionType psFitsCompressionTypeFromString(
    const char *string                  ///< String with compression type
    );

/** Closes a FITS file.
 *
 *  @return bool      TRUE if FITS file was successfully closed, otherwise FALSE
 */
bool psFitsClose(
    psFits* fits                       ///< psFits object to close
);

/// Allocator for options
psFitsOptions *psFitsOptionsAlloc(void);

/** Enables/configures FITS compression.
 *
 * Note that HCOMPRESS compression is not presently supported.
 *
 *  @return bool      TRUE if successfully configured, otherwise FALSE
 */
bool psFitsSetCompression(
    psFits* fits,                       ///< psFits object for which to set compression
    psFitsCompressionType type,         ///< type of compression
    psVector *tilesize,                 ///< vector defining compression tile size
    int noisebits,                      ///< noise bits
    int scale,                          ///< hcompress scale
    int smooth                          ///< hcompress smothing
);

/// Get the compression options for a file handle
psFitsCompression *psFitsCompressionGet(
    psFits* fits                        ///< psFits object for which to get compression
);

/// Get the compression type for a file handle
psFitsCompressionType psFitsCompressionGetType(
    psFits* fits                        ///< psFits object for which to get compression type
    );

/** Sets FITS write options
 *
 *  @return bool      TRUE if successfully configured, otherwise FALSE
 */
bool psFitsCompressionApply(
    psFits* fits,                       ///< psFits object for which to set compression
    psFitsCompression *compress         ///< options object
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psFits structure, false
 *                      otherwise.
 */
bool psMemCheckFits(
    psPtr ptr                          ///< the pointer whose type to check
);

// set the user-defined extension name;
bool psFitsSetExtnameWord (psFits *fits, const char *extword);

// move to the first HDU where extword == extname.  this is equivalent to fits_movnam_hdu() for
// a user-defined word in place of EXTNAME
bool p_psFitsMoveExtName_UserKey(const psFits *fits,
                                 const char *extname,
                                 const char *extword);

/** Moves the FITS HDU to the specified extension name.
 *
 *  @return bool        TRUE if the extension name was found and move was
 *                      successful, otherwise FALSE
 */
bool psFitsMoveExtName(
    const psFits* fits,                ///< the psFits object to move
    const char* extname                ///< the extension name
);

/** Moves the FITS HDU to the specified extension name without generating errors.
 *
 *  @return bool        TRUE if the extension name was found and move was
 *                      successful, otherwise FALSE
 */
bool psFitsMoveExtNameClean(
    const psFits* fits,                ///< the psFits object to move
    const char* extname                ///< the extension name
);

/** Moves the FITS HDU to the specified extension number
 *
 *  @return bool        TRUE if the extension number was found and move was
 *                      successful, otherwise FALSE
 */
bool psFitsMoveExtNum(
    const psFits* fits,                ///< the psFits object to move
    int extnum,                        ///< the extension number to move to (zero is primary HDU)
    bool relative                      ///< if true, extnum is a relative number to the current position
);

/** Moves the FITS HDU to the end of the file
 *
 *  @return bool        TRUE if the move was successful, otherwise FALSE
*/
bool psFitsMoveLast(
    psFits* fits                       ///< the psFits object to move
);

/** Get the current extension number, where 0 is the primary HDU.
 *
 *  @return int        Current HDU number of the psFits file or < 0 if an error
 *                     occurred.
 */
int psFitsGetExtNum(
    const psFits* fits                 ///< the psFits object
);

/** Get the current extension name.
 *
 *  @return int        Current HDU name of the psFits file or NULL if an
 *                     error occurred.
 */
psString psFitsGetExtName(
    const psFits* fits                 ///< the psFits object
);

/** Set the current extension's name
 *
 *  @return bool       TRUE if the extension was successfully set, otherwise FALSE.
 */
bool psFitsSetExtName(
    psFits* fits,                      ///< the psFits object
    const char* name                   ///< the extension name
);

/** Get the total number of HDUs in the FITS file.
 *
 *  @return int        The total number of HDUs in the FITS file or < 0 if an
 *                     error occurred.
 */
int psFitsGetSize(
    const psFits* fits                 ///< the psFits object
);

/** Remove the an HDU as specified by number
 *
 *  @return bool        TRUE if the specified HDU was removed, otherwise FALSE
 */
bool psFitsDeleteExtNum(
    psFits* fits,                      ///< the psFits object
    int extnum,                        ///< the extension number to delete (zero is primary HDU)
    bool relative                      ///< if true, extnum is a relative number to the current position
);

/** Remove the an HDU as specified by extension name
 *
 *  @return bool        TRUE if the specified HDU was removed, otherwise FALSE
 */
bool psFitsDeleteExtName(
    psFits* fits,                      ///< the psFits object
    const char* extname                ///< the extension name to delete
);

/** Get the extension type of the current HDU.
 *
 *  @return psFitsType The type of the current HDU.  If PS_FITS_TYPE_UNKNOWN,
 *                     the type could not be determined.
 */
psFitsType psFitsGetExtType(
    const psFits* fits                 ///< the psFits object
);

/** Delete all extensions after the current position
 *
 *  @return bool        TRUE if the operation was successful, otherwise FALSE
 */
bool psFitsTruncate(
    psFits* fits                       ///< the psFits object
);

// Return the psLib type, given a cfitsio data type
psDataType p_psFitsTypeFromCfitsio(int datatype // cfitsio data type
                                  );

// Return the cfitsio data type, given a psLib type
bool p_psFitsTypeToCfitsio(psDataType type, // psLib data type
                           int* bitPix, // The corresponding BITPIX (returned)
                           double* bZero, // The corresponding BZERO (returned)
                           int* dataType// The corresponding cfitsio data type (returned)
                          );

#define PS_ASSERT_FITS_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->fd) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Error: FITS file pointer %s is NULL", #NAME); \
    return RVAL; \
}

#define PS_ASSERT_FITS_WRITABLE(NAME, RVAL) \
if (!(NAME)->writable) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Error: FITS file %s not open for writing.", #NAME); \
    return RVAL; \
}

/// @}
#endif // #ifndef PS_FITS_H

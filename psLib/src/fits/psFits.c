/** @file  psFits.c
 *
 *  @brief Contains Fits I/O routines
 *
 *  @ingroup FileIO
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.85 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-10 20:58:17 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include "psFits.h"
#include "psFitsHeader.h"
#include "psError.h"
#include "psAssert.h"
#include "psImageStructManip.h"
#include "psMemory.h"
#include "psString.h"
#include "psLogMsg.h"
#include "psTrace.h"
#include "psVector.h"
#include "psAbort.h"
#include "psFitsFloat.h"

#define MAX_STRING_LENGTH 256           // Maximum length string for FITS routines

#define FITS_OPEN_RETRIES 16            // Number of retries to attempt when opening a FITS file
#define FITS_OPEN_RETRY_WAIT_MIN 200000 // Wait between retries (usec) first time
#define FITS_OPEN_RETRY_WAIT_MAX 5000000 // double wait time up to 5 sec

static char *defaultExtword = "EXTNAME";


bool p_psFitsDumpErrors (const char* filename, unsigned int lineno, const char* func,
                         psErrorCode code, const char *message, ...) {

    char fitsErr[MAX_STRING_LENGTH];

    va_list ap;
    va_start(ap, message);
    p_psErrorV(filename, lineno, func, PS_ERR_BAD_FITS, true, message, ap);
    va_end(ap);

    while (fits_read_errmsg(fitsErr)) {
        p_psError(filename, lineno, func, PS_ERR_BAD_FITS, false, "CFITSIO error: %s", fitsErr);
    }

    return true;
}

psErrorCode p_psFitsError(const char* filename, unsigned int lineno, const char* func,
                          int status, bool new, const char *errorMsg, ...)
{
    char fitsErr[MAX_STRING_LENGTH];

    if (status == 0) {
        return PS_ERR_NONE;
    }

    va_list ap;                         // Variable arguments
    va_start(ap, errorMsg);
    psErrorV(PS_ERR_IO, new, errorMsg, ap);
    va_end(ap);

    while (fits_read_errmsg(fitsErr)) {
        psError(PS_ERR_IO, false, "[CFITSIO error: %s]", fitsErr);
    }
    return PS_ERR_IO;
}

static bool isHDUEmpty(const psFits* fits)
{
    /* check for keys - no keys means this is really an empty HDU */
    int keysexist = -1;
    int morekeys;
    int status = 0;

    fits_get_hdrspace(fits->fd, &keysexist, &morekeys, &status);

    // if no keys exist and not primary HDU, this really is an empty HDU
    if (keysexist == 0) {
        return true;
    }

    return false;

}

static bool fitsClose(psFits* fits)
{
    int status = 0;

    if (fits != NULL) {
        if (fits_close_file(fits->fd, &status)) {
            psFitsDumpErrors (PS_ERR_IO, "Error while closing psFits object");
            return false;
        }
        fits->fd = NULL;
    }
    return true;
}

static void fitsFree(psFits* fits)
{
    if (!fits) return;
    if (fits->fd) {
        fitsClose(fits);
    }
    psFree(fits->options);
}

bool psFitsClose(psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);

    bool status = fitsClose(fits);
    psFree(fits);

    return status;
}

psFits* psFitsOpen(const char* name, const char* mode)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    // Check that the directory exists: for NFS-mounted file systems, the directory may
    // take some time to appear

    int dirstat = 0;                    // Status of directory access
    psString tmpname = psStringCopy(name); // Copy of filename, since dirname() may modify
    const char *dir = dirname(tmpname); // Directory for file
    useconds_t waittime = FITS_OPEN_RETRY_WAIT_MIN; // Wait time between retries (usec)

    for (int i = 0; (i < FITS_OPEN_RETRIES) && ((dirstat = access(dir, F_OK)) != 0); i++) {
        if (errno != ENOENT) {
            // The error is more serious than NFS-mount delays
            int thisErrno = errno;      // Error number
            char errorBuf[MAX_STRING_LENGTH], *errorMsg;
#if (((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE) ||  __APPLE__)
            strerror_r(thisErrno, errorBuf, MAX_STRING_LENGTH);
            errorMsg = errorBuf;
#else
            errorMsg = strerror_r(thisErrno, errorBuf, MAX_STRING_LENGTH);
#endif
            psError(PS_ERR_IO, true, "Directory (%s) for requested file is not accessible: %s",
                    dir, errorMsg);

            psFree(tmpname);
            return NULL;
        }

        usleep(waittime);
        // double waittime until we get to the max value
        if (waittime < FITS_OPEN_RETRY_WAIT_MAX) {
            waittime *= 2;
        }
    }
    if (dirstat != 0) {
        psError(PS_ERR_IO, true, "Directory (%s) for requested file is not accessible, timed out", dir);
        psFree(tmpname);
        return NULL;
    }
    psFree(tmpname);

    /* check the mode to determine how to open/create file */
    int iomode;
    bool newFile;
    if (strcmp(mode,"r") == 0) {
        iomode = READONLY;
        newFile = false;
    } else if (strcmp(mode,"rw") == 0 || strcmp(mode,"r+") == 0) {
        iomode = READWRITE;
        newFile = false;
    } else if (strcmp(mode,"w") == 0 || strcmp(mode,"w+") == 0) {
        iomode = READWRITE;
        newFile = true;
    } else if (strcmp(mode,"a") == 0|| strcmp(mode,"a+") == 0) {
        iomode = READWRITE;
        newFile = (access(name, F_OK) != 0);
    } else {
        // mode is not valid
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified mode, '%s', is invalid.  Supported modes are r, r+, rw, w, w+, a, or a+.",
                mode);
        return NULL;
    }

    int status = 0;                     // CFITSIO status
    fitsfile *fptr = NULL;              // Pointer to the FITS file

    if (newFile) {
        /* Check if an existing file is in the way before creating file */
        if (access(name, F_OK) == 0) {
            // file exists, delete old one first
            if (remove(name)) {
                int thisErrno = errno;
                char errorBuf[64], *errorMsg;
#if (((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE) ||  __APPLE__)
                strerror_r (errno, errorBuf, 64);
                errorMsg = errorBuf;
# else
                errorMsg = strerror_r (errno, errorBuf, 64);
# endif
                psError(PS_ERR_IO, true, "Failed to delete a previously-existing file (%s), error %d: %s",
                        name, thisErrno, errorMsg);
                //fprintf(stderr, "errno: %d, %s, %s : %lx, %lx\n", thisErrno, errorMsg, errorBuf, (long int) errorMsg, (long int) errorBuf);
                return NULL;
            }
        }
        if (!access(name, F_OK)) {
            psError (PS_ERR_IO, true, "deleted file still exists!");
            return NULL;
        }

        #if ( CFITSIO_DISKFILE == 1 )
        (void)fits_create_diskfile
        #else
        (void)fits_create_file
        #endif
        (&fptr, name, &status);
        if (fptr == NULL || status != 0) {
            psFitsDumpErrors (PS_ERR_IO, _("Could not create file,'%s'"), name);
            return NULL;
        }
    } else {
        #if ( CFITSIO_DISKFILE == 1 )
        (void)fits_open_diskfile
        #else
        (void)fits_open_file
        #endif
        (&fptr, name, iomode, &status);
        if (fptr == NULL || status != 0) {
            // MEH -- if cfitsio error and have status, really want to know it as well and should be added where lacking elsewhere
	    psFitsDumpErrors(PS_ERR_IO, _("Could not open file,'%s %d'"), name,status);
            return NULL;
        }
    }

    psFits* fits = psAlloc(sizeof(psFits));
    psMemSetDeallocator(fits, (psFreeFunc)fitsFree);

    fits->fd = fptr;
    fits->writable = (iomode == READWRITE);

    fits->options = NULL;

    return fits;
}


static void fitsOptionsFree(psFitsOptions *options)
{
    psFree(options->extword);
}


psFitsOptions *psFitsOptionsAlloc(void)
{
    psFitsOptions *options = psAlloc(sizeof(psFitsOptions)); // Options, to return
    psMemSetDeallocator(options, (psFreeFunc)fitsOptionsFree);

    options->extword = NULL;

    options->conventions.compression = true;
    options->conventions.psBitpix = true;

    options->floatType = PS_FITS_FLOAT_NONE;

    options->bitpix = 0;

    options->scaling = PS_FITS_SCALE_NONE;
    options->fuzz = true;
    options->bscale = 1.0;
    options->bzero = 0.0;
    options->mean = NAN;
    options->stdev = NAN;
    options->stdevBits = 4;
    options->stdevNum = 5.0;

    return options;
}

static void psFitsCompressionFree(psFitsCompression *comp)
{
    PS_ASSERT_PTR_NON_NULL(comp,);
    psFree(comp->tilesize);
}

psFitsCompression* psFitsCompressionAlloc(
    psFitsCompressionType type,         ///< type of compression
    psVector *tilesize,                 ///< vector defining compression tile size
    int noisebits,                      ///< noise bits
    int scale,                          ///< hcompress scale
    int smooth                          ///< hcompress smothing
)
{
    psFitsCompression *comp = psAlloc(sizeof(psFitsCompression));
    psMemSetDeallocator(comp, (psFreeFunc) psFitsCompressionFree);

    comp->type = type;
    comp->tilesize = psVectorCopy(NULL, tilesize, PS_DATA_S64);
    comp->noisebits = noisebits;
    comp->scale = scale;
    comp->smooth = smooth;

    return comp;
}

bool psMemCheckFits(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)fitsFree );
}

bool psFitsSetExtnameWord(psFits *fits, const char *extword)
{
    PS_ASSERT_PTR_NON_NULL(fits,    false);
    PS_ASSERT_PTR_NON_NULL(extword, false);

    if (!fits->options) {
        fits->options = psFitsOptionsAlloc();
    }

    psFree(fits->options->extword);
    fits->options->extword = psStringCopy(extword);
    return true;
}

// Files compressed with cfitsio's "imcopy" program may have multiple EXTNAME keywords, with the first set to
// COMPRESSED_IMAGE.  However, fits_movnam_hdu won't find the second (proper) value of EXTNAME, and so can
// fail to find a perfectly legitimate extension, simply because imcopy does something silly.  However, we
// really want to be able to read these files (MegaCam data are shipped as imcopy-compressed images).
// Therefore, we implement our own version of moving to an extension specified by name.  The pure cfitsio
// version is used if "conventions.compression" handling is turned off in the psFits structure.
static bool fitsMoveExtName(const psFits* fits, // FITS file
                            const char* extname, // Extension name
                            bool errors // Generate errors?
    )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_STRING_NON_EMPTY(extname, false);

    int status = 0;

    psFitsOptions *options = fits->options; // FITS options
    if (options && !options->conventions.compression && !options->extword) {
        // User wants to use cfitsio.  Good luck to them!
        if (fits_movnam_hdu(fits->fd, ANY_HDU, (char*)extname, 0, &status) != 0) {
            if (errors) {
                psFitsError(status, true, _("Could not find HDU '%s'"), extname);
            }
            return false;
        }
        return true;
    }

    // Ignore EXTNAME with value COMPRESSED_IMAGE?
    bool ignoreCI = (!options || (options->conventions.compression && (strcmp(extname, "COMPRESSED_IMAGE") != 0)));

    char *extword = (options && options->extword) ? options->extword : "EXTNAME"; // Word for extension name

#if 0
    // XXX Future optimisation: loop through from the current HDU to the end, then from the start to the
    // current position.  This will save seeking through the file multiple times.
    int currentExt = psFitsGetExtNum(fits); // Current extension number
    int numExt = psFitsGetSize(fits);   // Total number of extensions
#endif

    for (int i = 1; true; i++) {
        int hdutype = 0;
        if (fits_movabs_hdu(fits->fd, i, &hdutype, &status)) {
            // We've run off the end
            if (errors) {
                psFitsError(status, true, _("Could not find HDU with %s = '%s'"), extword, extname);
            }
            return false;
        }
        // Is there a keyword called 'extword'? (read as string regardless of type)
        char name[MAX_STRING_LENGTH];  // Name of extension
        if (fits_read_keyword(fits->fd, extword, name, NULL, &status)) {
            // It doesn't exist in the header.
            // This isn't the extension you're looking for.  Move along.
            status = 0;
            continue;
        }
        char *fixed = p_psFitsHeaderParseString(name); // Parsed version (removing quotes and spaces)

        if (ignoreCI && strcmp(fixed, "COMPRESSED_IMAGE") == 0) {
            // Read it again, Sam
            if (fits_read_keyword(fits->fd, extword, name, NULL, &status)) {
                status = 0;
                continue;
            }
            fixed = p_psFitsHeaderParseString(name);
        }

        if (strcmp(fixed, extname) == 0) {
            // We've arrived
            return true;
        }
    }
    psAbort("Should never reach here.");
}


bool psFitsMoveExtName(const psFits* fits, const char* extname)
{
    return fitsMoveExtName(fits, extname, true);
}

bool psFitsMoveExtNameClean(const psFits* fits, const char* extname)
{
    return fitsMoveExtName(fits, extname, false);
}

bool psFitsMoveExtNum(const psFits* fits,
                      int extnum,
                      bool relative)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);

    int status = 0;
    int hdutype = 0;

    if (relative) {
        fits_movrel_hdu(fits->fd, extnum, &hdutype, &status);
        if (status != 0) {
            psFitsDumpErrors (PS_ERR_LOCATION_INVALID, _("Could not move %d HDUs from current position"), extnum);
            return false;
        }
    } else {
        fits_movabs_hdu(fits->fd, extnum+1, &hdutype, &status);
        if (status != 0) {
            psFitsDumpErrors (PS_ERR_LOCATION_INVALID, _("Could not move to specified HDU #%d."), extnum);
            return false;
        }
    }

    return true;
}

bool psFitsMoveLast(psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);

    int size = psFitsGetSize(fits);
    if (size == 0) { // empty file -- no action needed
        return true;
    } else {
        return psFitsMoveExtNum(fits,size-1,false);
    }
}

int psFitsGetExtNum(const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    int hdunum;
    return fits_get_hdu_num(fits->fd,&hdunum) - 1;
}

psString psFitsGetExtName(const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    int status = 0;
    char name[MAX_STRING_LENGTH];

    psFitsOptions *options = fits->options; // FITS options
    char *extword = (!options || !options->extword) ? defaultExtword : options->extword;

    if (fits_read_key_str(fits->fd, extword, name, NULL, &status) != 0) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true, _("Header keyword %s is not found"), extword);
        return NULL;
    }
    return psStringCopy(name);
}

bool psFitsSetExtName(psFits* fits, const char* name)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    int status = 0;

    psFitsOptions *options = fits->options; // FITS options
    char *extword = (!options || !options->extword) ? defaultExtword : options->extword;

    if (fits_update_key_str(fits->fd, extword, (char*)name, NULL, &status) != 0) {
        psFitsDumpErrors (PS_ERR_IO, _("Could not write data to file %s"), name);
        return false;
    }

    return true;
}

bool psFitsDeleteExtNum(psFits* fits,
                        int extnum,
                        bool relative)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);

    // move to the specified HDU
    if (!psFitsMoveExtNum(fits, extnum, relative) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Failed to delete HDU #%d", extnum);
        return false;
    }

    int status = 0;

    // OK, now let's delete the HDU
    if (fits_delete_hdu(fits->fd, NULL, &status) != 0) {
        psFitsDumpErrors(PS_ERR_IO, _("Could not write data to file extnum %d"), extnum);
        return false;
    }

    return true;
}

bool psFitsDeleteExtName(psFits* fits,
                         const char* extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_STRING_NON_EMPTY(extname, false);

    // move to the specified HDU
    if (! psFitsMoveExtName(fits,extname) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Failed to delete HDU with the name '%s'",
                extname);
        return false;
    }


    int status = 0;

    // OK, now let's delete the HDU
    if (fits_delete_hdu(fits->fd, NULL, &status) != 0) {
        psFitsDumpErrors(PS_ERR_IO, _("Could not write data to file extname %s"), extname);
        return false;
    }

    return true;
}

int psFitsGetSize(const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, 0);

    int num = 0;
    int status = 0;

    if (fits_get_num_hdus(fits->fd, &num, &status) != 0) {
        psFitsDumpErrors(PS_ERR_LOCATION_INVALID, _("Failed to determine the number of HDUs"));
        return 0;
    }

    return num;
}

psFitsType psFitsGetExtType(const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, PS_FITS_TYPE_NONE);

    int status = 0;
    int hdutype = PS_FITS_TYPE_NONE;

    if (fits_get_hdu_type(fits->fd, &hdutype, &status) != 0) {
        psFitsDumpErrors(PS_ERR_LOCATION_INVALID, _("Failed to determine an HDU type"));
        return PS_FITS_TYPE_NONE;
    }

    if (hdutype == PS_FITS_TYPE_IMAGE &&
            psFitsGetExtNum(fits) > 0 &&
            isHDUEmpty(fits)) {
        return PS_FITS_TYPE_ANY;
    }

    return hdutype;
}

bool psFitsTruncate(psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);

    int newEnd = psFitsGetExtNum(fits);

    psFitsMoveLast(fits);
    int end = psFitsGetExtNum(fits);

    // delete HDUs from end to beginning position + 1;
    for (int lcv=end;lcv > newEnd; lcv--) {
        if (! psFitsDeleteExtNum(fits,lcv,false)) {
            // failed to delete an HDU!?
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to truncate file.  HDU #%d out of %d could not be deleted.",
                    lcv,end);
            return false;
        }
    }

    return true;
}


bool psFitsSetCompression(
    psFits* fits,                       ///< psFits object to close
    psFitsCompressionType type,         ///< type of compression
    psVector *tilesize,
    int noisebits,                      ///< noise bits
    int scale,                          ///< hcompress scale
    int smooth                          ///< hcompress smothing
)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);

    // convert psFitsCompressionType to cfitsio compression types
    int comptype;
    switch (type) {
        case PS_FITS_COMPRESS_NONE:
            comptype = 0x0;
            break;
        case PS_FITS_COMPRESS_GZIP:
            comptype = GZIP_1;
            break;
        case PS_FITS_COMPRESS_RICE:
            comptype = RICE_1;
            break;
        case PS_FITS_COMPRESS_HCOMPRESS:
            comptype = HCOMPRESS_1;
            break;
        case PS_FITS_COMPRESS_PLIO:
            comptype = PLIO_1;
            break;
        default:
            psError(PS_ERR_UNKNOWN, true, "invalid psFitsCompressionType");
            return false;
    }

    int status = 0;
    if (fits_set_compression_type(fits->fd, comptype, &status)) {
        psFitsDumpErrors(PS_ERR_BAD_FITS, "Error while configuring compression");
        return false;
    }

    // if we are setting a trivial compression (NONE), don't bother with the other parameters
    if (type == PS_FITS_COMPRESS_NONE) {
        return true;
    }

    PS_ASSERT_VECTOR_NON_NULL(tilesize, false);

    // convert a psVector into the (long *) array that cfitsio requires
    psVector *dim = NULL;
    if (sizeof(long) == sizeof(psS64)) {
        dim = psVectorCopy(NULL, tilesize, PS_DATA_S64);
        fits_set_tile_dim(fits->fd, psVectorLength(dim), (long *)dim->data.S64, &status);
    } else if (sizeof(long) == sizeof(psS32)) {
        dim = psVectorCopy(NULL, tilesize, PS_DATA_S32);
        fits_set_tile_dim(fits->fd, psVectorLength(dim), (long *)dim->data.S32, &status);
    } else {
        psAbort("can't map (long) type to a psLib type");
    }
    psFree(dim);
    // status check belongs to fits_set_tile_dim() call
    if (status) {
        fits_set_compression_type(fits->fd, 0x0, &status);
        psFitsDumpErrors(PS_ERR_BAD_FITS, "Error while configuring compression");
        return false;
    }

    // noise bits are irrelevant (not allowed) for PLIO.  XXX actually, it is the data type
    // that is the restriction; data must be 32 or 64 bit for noise bits to be valid.
    if (type != PS_FITS_COMPRESS_PLIO) {
        if (fits_set_noise_bits(fits->fd, noisebits, &status)) {
            fits_set_compression_type(fits->fd, 0x0, &status);
            psFitsDumpErrors(PS_ERR_BAD_FITS, "Error while configuring compression");
            return false;
        }
    }

#if FITS_HCOMP
    if (fits_set_hcomp_scale(fits->fd, scale, &status)) {
        fits_set_compression_type(fits->fd, 0x0, &status);
        psError(PS_ERR_BAD_FITS, status, "Error while configuring compression");
        return false;
    }
    if (fits_set_hcomp_smooth(fits->fd, smooth, &status)) {
        fits_set_compression_type(fits->fd, 0x0, &status);
        psError(PS_ERR_BAD_FITS, status, "Error while configuring compression");
        return false;
    }
#endif // FITS_HCOMP

    return true;
}

psFitsCompression *psFitsCompressionGet(psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    int status = 0;                     // cfitsio status

    psFitsCompressionType type = psFitsCompressionGetType(fits);
    if (type < 0) {
        psError(PS_ERR_UNKNOWN, false, "Unable to get compression type.");
        return NULL;
    }

    psElemType tileType;                // Type corresponding to "long"
    if (sizeof(long) == sizeof(psS64)) {
        tileType = PS_TYPE_S64;
    } else if (sizeof(long) == sizeof(psS32)) {
        tileType = PS_TYPE_S32;
    } else {
        psAbort("can't map (long) type to a psLib type");
    }

    psVector *tiles = psVectorAlloc(3, tileType); // Tile sizes
    if (fits_get_tile_dim(fits->fd, 3, (long*)tiles->data.U8, &status)) {
        psFitsError(status, true, "Unable to get compression tile sizes.");
        psFree(tiles);
        return NULL;
    }

    int noisebits;                      // Noise bits for compression
    if (fits_get_noise_bits(fits->fd, &noisebits, &status)) {
        psFitsError(status, true, "Unable to get compression noise bits.");
        psFree(tiles);
        return NULL;
    }

    int hscale = 0, hsmooth = 0;        // Scaling and smoothing for HCOMPRESS

#if FITS_HCOMP
    if (fits_get_hcomp_scale(fits->fd, &hscale, &status)) {
        psFitsError(status, true, "Unable to get HCOMPRESS scaling.");
        psFree(tiles);
        return NULL;
    }
    if (fits_get_hcomp_smooth(fits->fd, &hsmooth, &status)) {
        psFitsError(status, true, "Unable to get HCOMPRESS smoothing.");
        psFree(tiles);
        return NULL;
    }
#endif // FITS_HCOMP

    psFitsCompression *compress = psFitsCompressionAlloc(type, tiles, noisebits, hscale, hsmooth);
    psFree(tiles);                      // Drop reference

    return compress;
}

psFitsCompressionType psFitsCompressionGetType(psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, -1);

    int status = 0;                     // cfitsio status
    int comptype = 0;                   // cfitsio compression type

    if (fits_get_compression_type(fits->fd, &comptype, &status)) {
        psFitsError(status, true, "Unable to get compression type.");
        return -1;
    }

    psFitsCompressionType type;
    switch (comptype) {
      case 0:
        type = PS_FITS_COMPRESS_NONE;
        break;
      case GZIP_1:
        type = PS_FITS_COMPRESS_GZIP;
        break;
      case RICE_1:
        type = PS_FITS_COMPRESS_RICE;
        break;
      case HCOMPRESS_1:
        type = PS_FITS_COMPRESS_HCOMPRESS;
        break;
      case PLIO_1:
        type = PS_FITS_COMPRESS_PLIO;
        break;
      default:
        psError(PS_ERR_UNKNOWN, true, "cfitsio reports unknown compression type.");
        return -1;
    }

    return type;
}


bool psFitsCompressionApply(
    psFits* fits,                       ///< psFits object to close
    psFitsCompression *comp             ///< options object
)
{
    return psFitsSetCompression(fits, comp->type, comp->tilesize, comp->noisebits, comp->scale, comp->smooth);
}


psDataType p_psFitsTypeFromCfitsio(int datatype)
{
    switch (datatype) {
      case TBYTE:
        return PS_TYPE_U8;
      case TSBYTE:
        return PS_TYPE_S8;
      case TSHORT:
        return PS_TYPE_S16;
      case TUSHORT:
        return PS_TYPE_U16;
      case TLONG:
        if (sizeof(long) == 8) {
            return PS_TYPE_S64;
        }
        // no break
      case TINT:
        return PS_TYPE_S32;
      case TULONG:
        if (sizeof(unsigned long) == 8) {
            return PS_TYPE_U64;
        }
        // no break
      case TUINT:
        return PS_TYPE_U32;
      case TLONGLONG:
        return PS_TYPE_S64;
      case TFLOAT:
        return PS_TYPE_F32;
      case TDOUBLE:
        return PS_TYPE_F64;
      case TLOGICAL:
        return PS_TYPE_BOOL;
      case TSTRING:
        return PS_DATA_STRING;
      default:
        psError(PS_ERR_IO, true, "Unknown FITS datatype, %d.", datatype);
        return 0;
    }
}

bool p_psFitsTypeToCfitsio(psDataType type, int* bitPix, double* bZero, int* dataType)
{
    int bitpix;
    int datatype;
    double bzero = 0.0;

    switch (type) {

    case PS_TYPE_U8:
        bitpix = BYTE_IMG;
        // bzero = -1.0 * INT8_MIN;
        datatype = TBYTE;
        break;

    case PS_TYPE_S8:
        bitpix = BYTE_IMG;
        bzero = +1.0 * INT8_MIN;
        datatype = TSBYTE;
        break;

    case PS_TYPE_U16:
        bitpix = SHORT_IMG;
        bzero = -1.0 * INT16_MIN;
        datatype = TUSHORT;
        break;

    case PS_TYPE_S16:
        bitpix = SHORT_IMG;
        datatype = TSHORT;
        break;

    case PS_TYPE_U32:
        bitpix = LONG_IMG;
        bzero = -1.0 * INT32_MIN;
        datatype = TUINT;
        break;

    case PS_TYPE_S32:
        bitpix = LONG_IMG;
        datatype = TINT;
        break;

    case PS_TYPE_U64:
        bitpix = LONGLONG_IMG;
        bzero = -1.0 * INT64_MIN;
        datatype = TLONGLONG;
        break;

    case PS_TYPE_S64:
        bitpix = LONGLONG_IMG;
        datatype = TLONGLONG;
        break;

    case PS_TYPE_F32:
        bitpix = FLOAT_IMG;
        datatype = TFLOAT;
        break;

    case PS_TYPE_F64:
        bitpix = DOUBLE_IMG;
        datatype = TDOUBLE;
        break;

    case PS_DATA_STRING:
        bitpix = BYTE_IMG;
        datatype = TSTRING;
        break;

    case PS_DATA_BOOL:
        bitpix = BYTE_IMG;
        datatype = TLOGICAL;
        break;

    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified type, %s, is not supported."),
                    typeStr);
            if (bitPix) {
                *bitPix = 0;
            }
            if (dataType) {
                *dataType = 0;
            }
            if (bZero) {
                *bZero = 0;
            }
            return false;
        }
    }

    // pass back the requested parameters  (NULL parameters are not set, of course).
    if (bitPix != NULL) {
        *bitPix = bitpix;
    }

    if (dataType != NULL) {
        *dataType = datatype;
    }

    if (bZero != NULL) {
        *bZero = bzero;
    }

    return true;
}


psFitsCompressionType psFitsCompressionTypeFromString(const char *string)
{
    if (!string || strlen(string) == 0) {
        psWarning("Unable to identify compression type --- none set.");
        return PS_FITS_COMPRESS_NONE;
    }

    // use strncmp so that we can resolve the string valuves that cfitsio puts into headers (RICE_1 GZIP_1)
    // into psFitsCompressionType
    if (strcmp(string, "NONE") == 0) return PS_FITS_COMPRESS_NONE;
    if (strncmp(string, "GZIP", 4) == 0) return PS_FITS_COMPRESS_GZIP;
    if (strncmp(string, "RICE", 4) == 0) return PS_FITS_COMPRESS_RICE;
    if (strncmp(string, "HCOMPRESS", 9) == 0) return PS_FITS_COMPRESS_HCOMPRESS;
    if (strncmp(string, "PLIO", 4) == 0) return PS_FITS_COMPRESS_PLIO;

    psWarning("Unable to identify compression type (%s) --- none set.", string);
    return PS_FITS_COMPRESS_NONE;
}



#if 0
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
    double mean, stdev;                 ///< Mean and standard deviation of image
    int stdevBits;                      ///< Number of bits to sample a standard deviation (for SCALE_STDEV_*)
    float stdevNum;                     ///< Number of standard deviations to pad off the edge
} psFitsOptions;

bool psFitsCopyCompression(psFits *target, psFits *source)
{
    PS_ASSERT_FITS_NON_NULL(target, false);
    PS_ASSERT_FITS_NON_NULL(source, false);

    psMetadata *header = psMetadataReadHeader(NULL, source); // Header of source file

    bool mdok;                          // Status of MD lookup
    psString compTypeStr = psMetadataLookupStr(&mdok, header, "ZCMPTYPE"); // Compression type
    psFitsCompressionType compType = psFitsCompressionTypeFromString(compTypeStr); // Compression type

    if (!target->options) {
        target->options = psFitsOptionsAlloc();
    }

    target->options->floatType = psFitsFloatImageCheck(source); // Custom floating-point type





    target->options = psFitsOptionsAlloc();
    target->options->scaling = PS_FITS_SCALE_MANUAL;
    target->options->fuzz = false;
    target->options->bitpix = bitpix;
    target->options->bscale = bscale;
    target->options->bzero = bzero;

    psFitsSetCompression(sfile->fits, compType, tiles, 8, 0, 0);




    // Get current BITPIX, BSCALE, BZERO, EXTNAME
    // Probably not necessary to look the numerical values up in this
    // way, but guards against changes to psLib and cfitsio FITS
    // handling.
    psMetadataItem *bitpixItem = psMetadataLookup(in->header, "BITPIX");
    psAssert(bitpixItem, "Every FITS image should have BITPIX");
    int bitpix = psMetadataItemParseS32(bitpixItem);
    psMetadataItem *bscaleItem = psMetadataLookup(in->header, "BSCALE");

    float bscale;
    if (!bscaleItem) {
        psWarning("BSCALE isn't set; defaulting to unity");
        bscale = 1.0;
    } else {
        bscale = psMetadataItemParseF32(bscaleItem);
    }
    psMetadataItem *bzeroItem = psMetadataLookup(in->header, "BZERO");
    float bzero;
    if (!bzeroItem) {
        psWarning("BZERO isn't set; defaulting to zero");
        bzero = 0.0;
    } else {
        bzero = psMetadataItemParseF32(bzeroItem);
    }
#endif

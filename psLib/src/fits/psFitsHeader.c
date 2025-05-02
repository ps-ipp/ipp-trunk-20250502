/** @file  psFitsHeader.c
 *
 *  @brief Contains Fits header I/O routines
 *
 *  @ingroup FileIO
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.51 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-17 20:23:02 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <strings.h>

#include "psAbort.h"
#include "psAssert.h"
#include "psFits.h"
#include "psError.h"

#include "psImageStructManip.h"
#include "psMemory.h"
#include "psString.h"
#include "psLogMsg.h"
#include "psTrace.h"
#include "psVector.h"

#define MAX_STRING_LENGTH 256           // maximum length string for FITS routines
#define NUM_EMPTY_KEYS 8                // Number of keywords before header is considered practically empty

// list of FITS header keys to ignore; NULL-terminated
static const char* ignoreFitsKeys[] = { "", NULL };

// List of FITS header keys that may be duplicated; NULL-terminated
static const char *duplicateFitsKeys[] = { "COMMENT", "HIERARCH", "HISTORY", NULL };

// List of FITS header keys that are put in the comment by cfitsio; but we want them in the value.
static const char *commentFitsKeys[] = { "COMMENT", "HISTORY", NULL };

// List of FITS header keys not to write (handled by cfitsio); NULL-terminated
static const char *noWriteFitsKeys[] = { "SIMPLE", "XTENSION", "BITPIX", "NAXIS", "EXTNAME", "BSCALE",
                                         "BZERO", "TFIELDS", "PCOUNT", "GCOUNT", "PSBITPIX", "BLANK", NULL };

// List of the start of FITS header keys not to write (handled by cfitsio); NULL-terminated
static const char *noWriteFitsKeyStarts[] = { "NAXIS", "TTYPE", "TFORM", "TZERO", "TSCAL", NULL };

// List of compressed FITS header keys not to write (handled by cfitsio); NULL-terminated
static const char *noWriteCompressedKeys[] = { "ZBITPIX", "ZIMAGE", "ZBITPIX", "ZCMPTYPE", "ZSIMPLE",
                                               "ZEXTEND", "ZBLANK", "ZDATASUM", "ZHECKSUM", NULL };

// List of the start of FITS header keys not to write (handled by cfitsio); NULL-terminated
static const char *noWriteCompressedKeyStarts[] = { "ZNAXIS", "ZTILE", "ZNAME", "ZVAL", NULL };

// List of FITS header keys that may be present if the header is considered "empty"; NULL-terminated
static const char *emptyKeys[] = { "SIMPLE", "BITPIX", "NAXIS", "EXTEND", "COMMENT", "CHECKSUM", "DATASUM",
                                   NULL  };

// How to translate between keywords
typedef struct {
    const char *from;                   // Translate from this keyword
    const char *to;                     // Translate to this keyword
} keywordTranslation;

// Translation for compressed image headers           FROM        TO
// The "From" and "To" are appropriate for reading.
static keywordTranslation compressTranslation[] = { { "ZSIMPLE",  "SIMPLE" },
                                                    { "ZTENSION", "XTENSION" },
                                                    { "ZBITPIX",  "BITPIX" },
                                                    { "ZNAXIS",   "NAXIS"  },
                                                    { "ZNAXIS1",  "NAXIS1" },
                                                    { "ZNAXIS2",  "NAXIS2" },
                                                    { "ZNAXIS3",  "NAXIS3" },
                                                    { "ZEXTEND",  "EXTEND" },
                                                    { "ZBLOCKED", "BLOCKED"},
                                                    { "ZHECKSUM", "CHECKSUM"},
                                                    { "ZDATASUM", "DATASUM" },
                                                    { NULL,       NULL } };


// Compare a keyword with a list of keywords; return true if it's in the list
static bool keywordInList(const char *keyword, // Keyword to check
                          const char *list[] // List of keywords
    )
{
    for (const char **check = list; *check; ++check) {
        if (strcmp(keyword, *check) == 0) {
            return true;
        }
    }
    return false;
}

// Compare a keyword with a list of keyword beginnings; return true if the keyword starts with one of these
static bool keywordStartsWith(const char *keyword, // Keyword to check
                              const char *list[] // List of keyword beginnings
                              )
{
    bool writeKey = true;   // Write this keyword?
    for (int i = 0; list[i] && writeKey; i++) {
        if (strncmp(keyword, list[i], strlen(list[i])) == 0) {
            return true;
        }
    }
    return false;
}


// Translate one keyword to another.
// This is appropriate for reading
static const char *keywordTranslate(const char *keyword, // Keyword to check
                                    keywordTranslation translation[] // Translation list
                                    )
{
    for (keywordTranslation *trans = translation; (*trans).from; ++trans) {
        if (strcmp(keyword, (*trans).from) == 0) {
            // Translate it
            return (*trans).to;
        } else if (strcmp(keyword, (*trans).to) == 0) {
            // Ignore it completely --- something else will translate to it
            return NULL;
        }
    }
    // It translates to itself
    return keyword;
}

// Translate back the other way.
// This is appropriate for writing.
static const char *keywordUntranslate(const char *keyword, // Keyword to check
                                      keywordTranslation translation[] // Translation list
    )
{
    for (keywordTranslation *trans = translation; (*trans).to; ++trans) {
        if (strcmp(keyword, (*trans).to) == 0) {
            // Translate it
            return (*trans).from;
        }
    }
    // It translates to itself
    return keyword;
}

bool psFitsCheckCompressedImagePHU(const psFits *fits, psMetadata *header)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (fits->options && !fits->options->conventions.compression) {
        // User has turned off compression conventions; doesn't want any nasty surprises
        return false;
    }

    if (psFitsGetExtNum(fits) != 0) {
        // It's not the PHU, so it can't be the PHU for a single compressed image!
        return false;
    }

    if (psFitsGetSize(fits) == 1) {
        // No extension present
        return false;
    }

    int numKeys;                        // Number of keywords in the header
    int status = 0;                     // CFITSIO status
    fits_get_hdrspace(fits->fd, &numKeys, 0, &status);
    if (numKeys > NUM_EMPTY_KEYS) {
        return false;
    }

    int bitpix, naxis;                  // Bits per pixel and number of axes
    long naxes[MAX_COMPRESS_DIM];       // Dimensions
    fits_get_img_param(fits->fd, MAX_COMPRESS_DIM, &bitpix, &naxis, naxes, &status);
    if (naxis != 0) {
        return false;
    }

    if (!header) {
        for (int i = 1; i <= numKeys; i++) {
            // Just want to read the keyword names, without parsing the values and stuffing into a metadata
            char keyName[MAX_STRING_LENGTH];// Keyword name
            char keyValue[MAX_STRING_LENGTH]; // Corresponding value
            char keyComment[MAX_STRING_LENGTH]; // Corresponding comment
            fits_read_keyn(fits->fd, i, keyName, keyValue, keyComment, &status);
            if (!keywordInList(keyName, emptyKeys)) {
                return false;
            }
        }
    } else {
        psMetadataIterator *iter = psMetadataIteratorAlloc(header, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *item;           // Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            if (!keywordInList(item->name, emptyKeys)) {
                psFree(iter);
                return false;
            }
        }
        psFree(iter);
    }

    if (!psFitsMoveExtNum(fits, 1, false)) {
        psWarning("Unable to examine first extension as suspect compressed image.");
        return false;
    }

    if (fits_is_compressed_image(fits->fd, &status)) {
        return true;
    }

    // It's not a single compressed image PHU --- move back to the PHU for the user
    if (!psFitsMoveExtNum(fits, 0, false)) {
        psWarning("Unable to examine first extension as suspect compressed image.");
        return false;
    }

    return false;
}

char *p_psFitsHeaderParseString(char *string)
{
    if (!string || strlen(string) == 0) {
        return string;
    }

    char *fixed = string;       // Fixed version of the string
    // remove the single-quotes at front/end
    if (fixed[0] == '\'' && fixed[strlen(string)-1] == '\'') {
        string[strlen(string)-1] = '\0'; // Remove the trailing quote
        fixed += 1; // Advance past the leading quote
    }
    // Remove trailing spaces, which are not significant, according to the FITS standard
    // http://archive.stsci.edu/fits/fits_standard/node31.html
    char *lastSpace = NULL; // The last space in the string
    while (strlen(fixed) > 0 && (lastSpace = strrchr(fixed, ' ')) && lastSpace[1] == '\0') {
        // This is a trailing space, not a leading space.
        lastSpace[0] = '\0'; // Truncate the string here
    }

    return fixed;
}

// Read the header
static psMetadata *readHeader(const psFits *fits // FITS file from which to read header
                              )
{
    psAssert(fits, "impossible");

    psMetadata *header = psMetadataAlloc(); // Header, to return

    // Get number of key names
    int numKeys = 0;                    // Number of keywords
    int keyNum = 0;                     // Current key number
    int status = 0;                     // Status of cfitsio calls
    fits_get_hdrpos(fits->fd, &numKeys, &keyNum, &status);

    bool compressed = false;            // Is this a compressed image?
    if ((!fits->options || fits->options->conventions.compression) &&
        fits_is_compressed_image(fits->fd, &status)) {
        compressed = true;
    }

    // Get each key name. Keywords start at one.
    for (int i = 1; i <= numKeys; i++) {
        char keyName[MAX_STRING_LENGTH];// Keyword name
        char keyValue[MAX_STRING_LENGTH]; // Corresponding value
        char keyComment[MAX_STRING_LENGTH]; // Corresponding comment
        fits_read_keyn(fits->fd, i, keyName, keyValue, keyComment, &status);

        // Check to see if the keyword should be ignored
        if (keywordInList(keyName, ignoreFitsKeys)) {
            // We're done here; skip to the next key
            continue;
        }

        const char *keyNameTrans = keyName;   // Translated name of keyword
        if (compressed) {
            keyNameTrans = keywordTranslate(keyName, compressTranslation);
            if (!keyNameTrans) {
                // It's to be ignored (it will be replaced by something else)
                continue;
            }
        }

        // Check to see if the keyword should be duplicated
        int dupFlag = 0;                // Duplicate flag
        if (keywordInList(keyName, duplicateFitsKeys)) {
            dupFlag = PS_META_DUPLICATE_OK;
        }

        bool success;                   // Was the add to the metadata successful?

        // Certain keywords (COMMENT and HISTORY) are put in the comment rather than value by cfitsio.
        if (keywordInList(keyName, commentFitsKeys)) {
            success = psMetadataAddStr(header, PS_LIST_TAIL, keyNameTrans, dupFlag, NULL, keyComment);
        } else {
            char keyType;                   // Type of key; from cfitsio
            if (keyValue[0] != 0) { // blank values are not handled by fits_get_keytype
                fits_get_keytype(keyValue, &keyType, &status);
            } else {
                keyType = 'C';
            }
            psTrace("psLib.fits", 3, "Reading keyword %s, type %c\n", keyName, keyType);
            if (status != 0) {
                break;
            }

            switch (keyType) {
              case 'X': // bit
              case 'B': // byte
                success = psMetadataAddS8(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                          atoi(keyValue));
                break;
              case 'I': // short int.
                // This is the default type that cfitsio reports whenever it doesn't know what it is.
                // Trap NAN, INF and -INF, which cfitsio doesn't handle.
                if (strncasecmp(keyValue, "NAN", 3) == 0) {
                    success = psMetadataAddF32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment, NAN);
                } else if (strncasecmp(keyValue, "INF", 3) == 0) {
                    success = psMetadataAddF32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                               INFINITY);
                } else if (strncasecmp(keyValue, "-INF", 4) == 0) {
                    success = psMetadataAddF32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                               -INFINITY);
                } else {
                    long long value = atoll(keyValue); // Value
                    if (value > PS_MIN_S32 && value < PS_MAX_S32) {
                        success = psMetadataAddS32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                                   value);
                    } else {
                        success = psMetadataAddS64(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                                   value);
                    }
                }
                break;
              case 'J': // int.
                success = psMetadataAddS32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                           atoi(keyValue));
                break;
              case 'U': // unsigned int.
                success = psMetadataAddU32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                           atol(keyValue));
                break;

              case 'K': // long long (64-bit) integer
                success = psMetadataAddS64(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                           atoll(keyValue));
                break;
              case 'F': // float/double
                success = psMetadataAddF64(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                           atof(keyValue));
                break;
              case 'C': {
                  char *keyValueFixed = p_psFitsHeaderParseString(keyValue); // Fixed version of the string

                  // Need to trap NAN, INF and -INF written by psFitsWriteHeader: cfitsio won't write these,
                  // so we write them as strings, and then have to trap them on read.
                  if (strcasecmp(keyValueFixed, "NAN") == 0) {
                      success = psMetadataAddF32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                                 NAN);
                  } else if (strcasecmp(keyValueFixed, "INF") == 0) {
                      success = psMetadataAddF32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                                 INFINITY);
                  } else if (strcasecmp(keyValueFixed, "-INF") == 0) {
                      success = psMetadataAddF32(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                                 -INFINITY);
                  } else if (compressed && strcmp(keyName, "EXTNAME") == 0 &&
                             strcmp(keyValueFixed, "COMPRESSED_IMAGE") == 0) {
                      // Ignore EXTNAME=COMPRESSED_IMAGE if compression convention is to be respected
                      success = true;
                  } else {
                      success = psMetadataAddStr(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment,
                                                 keyValueFixed);
                  }
                  break;
              }
              case 'L': {               // Logical (boolean)
                  bool temp = (keyValue[0] == 'T') ? 1 : 0;
                  success = psMetadataAddBool(header, PS_LIST_TAIL, keyNameTrans, dupFlag, keyComment, temp);
                  break;
              }
              default:
                psError(PS_ERR_IO, true, _("Specified FITS metadata type, %c, is not supported."), keyType);
                psFree(header);
                return NULL;
            }
        }

        if (!success) {
            psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s --> %s."),
                    keyName, keyNameTrans);
            psFree(header);
            return NULL;
        }

    }

    if (status != 0) {
        psFitsError(status, true, _("Failed to add metadata item."));
        psFree(header);
        return false;
    }

    return header;
}


psMetadata* psFitsReadHeader(psMetadata* out,
                             const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    psMetadata *header = readHeader(fits); // Header
    if (!header) {
        return NULL;
    }

    // Explore the potential case that this is an empty PHU, and the first extension contains the sole image,
    // which is compressed.
    if (psFitsCheckCompressedImagePHU(fits, header)) {
        // This is really what we want, not the empty PHU
        psTrace("psLib.fits", 1,
                "This PHU should really be a compressed image --- getting that header instead.");
        psFree(header);
        header = readHeader(fits);
        if (!header) {
            return NULL;
        }
    }

    if (!out) {
        return header;
    }

    // Need to move header onto the nominated metadata
    psMetadataIterator *iter = psMetadataIteratorAlloc(header, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;           // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        // Need to look for MULTI, which won't be picked up using the iterator.
        psMetadataItem *multiCheckItem = psMetadataLookup(header, item->name);
        psAssert(multiCheckItem, "impossible");
        unsigned int flag = 0;      // Flag to indicate MULTI; otherwise default action
        if (multiCheckItem->type == PS_DATA_METADATA_MULTI) {
            flag = PS_META_DUPLICATE_OK;
        }
        if (!psMetadataAddItem(out, item, PS_LIST_TAIL, flag)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add header item %s to extant metadata.",
                    item->name);
            psFree(iter);
            psFree(header);
            return NULL;
        }
    }
    psFree(iter);
    psFree(header);
    return out;
}

psMetadata* psFitsReadHeaderSet(psMetadata* out, const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    if (!out) {
        out = psMetadataAlloc();
    }

    int size = psFitsGetSize(fits);

    int origPosition = psFitsGetExtNum(fits);

    for (int lcv=0; lcv < size; lcv++) {
        psFitsMoveExtNum(fits, lcv, false);

        char* name = NULL;
        if (lcv == 0) {
            name = psStringCopy("PHU");
        } else {
            name = psFitsGetExtName(fits);
        }

        psMetadata* header = psFitsReadHeader(NULL, fits);
        if (name != NULL && header != NULL) {
            psMetadataAddMetadata(out, PS_LIST_TAIL, name, 0, "FITS Header", header);
        } else {
            psWarning("Failed to read HDU#%d header data.",
                      lcv);
        }

        psFree(name);
        psFree(header);
    }

    // reposition to the original position
    psFitsMoveExtNum(fits, origPosition, false);

    return out;
}

static bool fitsWriteHeader(psFits *fits, // The FITS file handle
                            const psMetadata *output, // Metadata that is to be output into the FITS file
                            bool keyStarts, // Write out the key starts?
                            bool phuImage   // Are we writing a PHU image?
                           )
{
    int status = 0;                     // Status of cfitsio calls
    int extnum = psFitsGetExtNum(fits); // Number of extension
    bool simple = true;                 // If SIMPLE is T, then the file should conform to the FITS standard
    psFitsCompressionType compress = psFitsCompressionGetType(fits); // Compression type
    if (extnum == 0) {

        // We allow the user to write SIMPLE, but it must be boolean
        psMetadataItem *simpleItem = psMetadataLookup(output, "SIMPLE"); // SIMPLE in the header
        if (simpleItem) {
            if (simpleItem->type != PS_DATA_BOOL || !simpleItem->data.B) {
                int value = false;          // Temporary holder for boolean
                psWarning("Writing SIMPLE=F to FITS header by request");
                fits_update_key(fits->fd, TLOGICAL, "SIMPLE", &value,
                                "File does not conform to FITS standard", &status);
                simple = false;
            }
            // Uncompressed SIMPLE = T is taken care of by cfitsio.
        }
    }

    bool compressing = ((!fits->options || fits->options->conventions.compression) &&
                        compress != PS_FITS_COMPRESS_NONE) ? true : false; // Are we compressing?
    if (compressing) {
        psMetadataItem *simpleItem = psMetadataLookup(output, "SIMPLE"); // SIMPLE in the header
        if (simpleItem) {
            if (simpleItem->type != PS_DATA_BOOL) {
                psError(PS_ERR_BAD_PARAMETER_TYPE, true, "SIMPLE in a FITS header must be of boolean type: "
                        "not %x --- assuming FALSE.\n", simpleItem->type);
                simple = false;
            } else {
                simple = simpleItem->data.B;
            }
        }
        if (simple && phuImage && extnum == 1) {
            // ZSIMPLE is required for decompression with funpack, etc.  For funpack to work, ZSIMPLE needs to
            // go early in the FITS header (otherwise we get "Extension doesn't start with SIMPLE or XTENSION
            // keyword.").  We put it after ZIMAGE by reading ZIMAGE (which sets the insertion pointer) and
            // then inserting ZSIMPLE.
            char comment[FLEN_CARD];    // Comment for ZIMAGE; unused
            int value;                  // Value for ZIMAGE; unused
            psTrace("psLib.fits", 3, "Writing header ZSIMPLE to preserve PHU");
            fits_read_key(fits->fd, TLOGICAL, "ZIMAGE", &value, comment, &status);
            fits_insert_key_log(fits->fd, "ZSIMPLE", simple, "Uncompressed file's conforms to FITS", &status);
        }
    }

    // Traverse the metadata list and add each key.
    psListIterator* iter = psListIteratorAlloc(output->list, PS_LIST_HEAD, true); // Iterator
    psMetadataItem* item;               // Item from iteration
    while ((item = psListGetAndIncrement(iter))) {
        char *name = item->name;        // Keyword name to use when writing out
        // Check to see if the item should be ignored
        if (simple) {
            // We ignore particular (required) keywords, because these are written by CFITSIO
            // Furthermore, users tend to supply FITS headers that are wrong (e.g., after binning down the
            // image, the NAXISn haven't been changed; or after converting to F32, the BITPIX hasn't been
            // changed) so we'll take care of that for them.

            // Also block out TTYPEn, NAXISn, etc --- keywords that start with a certain sequence.
            // We want to do this when writing an image or table, since it guarantees that the NAXISn etc
            // that go in are correct.  However, when we're writing a "blank" HDU (header only), we want
            // to preserve NAXISn etc for reference, so we don't do this.

            if (keywordInList(name, noWriteFitsKeys) ||
                (keyStarts && keywordStartsWith(name, noWriteFitsKeyStarts))) {
                psTrace("psLib.fits", 3, "Not writing FITS keyword %s", name);
                continue;
            }

            // Options for compression
            if (compressing) {
                // Check to see if the keyword should be translated
                name = (char*)keywordUntranslate(name, compressTranslation); // Casting away const for cfitsio

                if (keywordInList(name, noWriteCompressedKeys) ||
                    (keyStarts && keywordStartsWith(name, noWriteCompressedKeyStarts))) {
                    psTrace("psLib.fits", 3, "Not writing FITS keyword %s", name);
                    continue;
                }
            } else if (keywordInList(name, noWriteCompressedKeys) ||
                       (keyStarts && keywordStartsWith(name, noWriteCompressedKeyStarts))) {
                psTrace("psLib.fits", 3, "Not writing FITS keyword %s", name);
                continue;
            }
        }

        if (strcmp(name, "COMMENT") == 0) {
            if (item->type != PS_DATA_STRING) {
                psWarning("COMMENT header is not of type STRING (%x) --- ignored.", item->type);
            } else {
                psTrace("psLib.fits", 5, "Writing header COMMENT: %s", item->data.str);
                fits_write_comment(fits->fd, item->data.str, &status);
            }
        } else if (strcmp(name,  "HISTORY") == 0) {
            if (item->type != PS_DATA_STRING) {
                psWarning("COMMENT header is not of type STRING (%x) --- ignored.", item->type);
            } else {
                psTrace("psLib.fits", 5, "Writing header HISTORY: %s", item->data.str);
                fits_write_history(fits->fd, item->data.str, &status);
            }
        } else {
            // A regular FITS header
            switch (item->type) {
              case PS_DATA_BOOL: {
                  int value = item->data.B;
                  psTrace("psLib.fits", 5, "Writing BOOL header %s: %d", name, value);
                  fits_update_key(fits->fd, TLOGICAL, name, &value, item->comment, &status);
                  break;
              }
              case PS_DATA_S8:
                psTrace("psLib.fits", 5, "Writing S8 header %s: %d", name, (int)item->data.S8);
                fits_update_key(fits->fd, TBYTE, name, &item->data.S8, item->comment, &status);
                break;
              case PS_DATA_S16:
                psTrace("psLib.fits", 5, "Writing S16 header %s: %d", name, (int)item->data.S16);
                fits_update_key(fits->fd, TSHORT, name, &item->data.S16, item->comment, &status);
                break;
              case PS_DATA_S32:
                psTrace("psLib.fits", 5, "Writing S32 header %s: %d", name, (int)item->data.S32);
                fits_update_key(fits->fd, TINT, name, &item->data.S32, item->comment, &status);
                break;
              case PS_DATA_S64:
                psTrace("psLib.fits", 5, "Writing S64 header %s: %" PRId64, name, item->data.S64);
                fits_update_key(fits->fd, TLONGLONG, name, &item->data.S64, item->comment, &status);
                break;
              case PS_DATA_U8: {
                  unsigned short int temp = item->data.U8;
                psTrace("psLib.fits", 5, "Writing U8 header %s: %d", name, (int)item->data.U8);
                  fits_update_key(fits->fd, TUSHORT, name, &temp, item->comment, &status);
                  break;
              }
              case PS_DATA_U16:
                psTrace("psLib.fits", 5, "Writing U16 header %s: %d", name, (int)item->data.U16);
                fits_update_key(fits->fd, TUSHORT, name, &item->data.U16, item->comment, &status);
                break;
              case PS_DATA_U32:
                psTrace("psLib.fits", 5, "Writing U32 header %s: %d", name, (unsigned int)item->data.U32);
                fits_update_key(fits->fd, TUINT, name, &item->data.U32, item->comment, &status);
                break;
              case PS_DATA_U64:
                // CFITSIO doesn't support unsigned 64-bit integers; attempt to write as signed
                if (item->data.U64 > PS_MAX_S64) {
                    psWarning("Unable to write 64-bit unsigned integer for item %s to header: "
                              "value %" PRIu64 " out of range", name, item->data.U64);
                    break;
                }
                psS64 temp = item->data.U64; // Signed version
                psTrace("psLib.fits", 5, "Writing U64 header %s: %" PRIu64, name, item->data.U64);
                fits_update_key(fits->fd, TLONGLONG, name, &temp, item->comment, &status);
                break;
              case PS_DATA_F32: {
                  int infCheck = 0;         // Result of isinf()
                  psTrace("psLib.fits", 5, "Writing F32 header %s: %f", name, item->data.F32);
                  if (isnan(item->data.F32)) {
                      fits_update_key(fits->fd, TSTRING, name, "NaN", item->comment, &status);
                  } else if ((infCheck = isinf(item->data.F32)) != 0) {
                      if (infCheck == 1) {
                          fits_update_key(fits->fd, TSTRING, name, "Inf", item->comment, &status);
                      } else {
                          fits_update_key(fits->fd, TSTRING, name, "-Inf", item->comment, &status);
                      }
                  } else {
                      fits_update_key(fits->fd, TFLOAT, name, &item->data.F32, item->comment,
                                      &status);
                  }
                  break;
              }
              case PS_DATA_F64: {
                  int infCheck = 0;         // Result of isinf()
                  psTrace("psLib.fits", 5, "Writing F32 header %s: %lf", name, item->data.F64);
                  if (isnan(item->data.F64)) {
                      fits_update_key(fits->fd, TSTRING, name, "NaN", item->comment, &status);
                  } else if ((infCheck = isinf(item->data.F64)) != 0) {
                      if (infCheck == 1) {
                          fits_update_key(fits->fd, TSTRING, name, "Inf", item->comment, &status);
                      } else {
                          fits_update_key(fits->fd, TSTRING, name, "-Inf", item->comment, &status);
                      }
                  } else {
                      fits_update_key(fits->fd, TDOUBLE, name, &item->data.F64, item->comment,
                                      &status);
                  }
                  break;
              }
              case PS_DATA_STRING:
                psTrace("psLib.fits", 5, "Writing STR header %s: %s", name, item->data.str);
                fits_update_key(fits->fd, TSTRING, name, item->data.V, item->comment, &status);
                break;
              default:  // all other META types are ignored
                psError(PS_ERR_IO, true,
                        "Unable to write metadata item %s of type %x to FITS header",
                        item->name, item->type);
                return false;
            }
        }

        if (status != 0) {
            char fitsErr[MAX_STRING_LENGTH];
            (void)fits_get_errstatus(status, fitsErr);
            psError(PS_ERR_IO, true,
                    _("Could not write metadata item %s of type %x to FITS header.\nCFITSIO Error: %s"),
                    item->name, item->type, fitsErr);
            return false;
        }
    }

    psFree(iter);

    return true;
}

bool psFitsWriteHeader(psFits *fits,
                       const psMetadata *output
                      )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_METADATA_NON_NULL(output, false);

    return fitsWriteHeader(fits, output, true, false);
}

bool psFitsWriteHeaderImage(psFits *fits,
                            const psMetadata *output,
                            bool phuImage
                            )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_METADATA_NON_NULL(output, false);

    return fitsWriteHeader(fits, output, true, phuImage);
}

bool psFitsWriteBlank(psFits* fits,
                      const psMetadata* output,
                      const char *extname
                     )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);

    // We allow output == NULL in order to write a minimal header.

    // Create a dummy image HDU for the primary HDU
    int status = 0;                 // Status of cfitsio

    psFitsMoveLast(fits);

    int hdus = psFitsGetSize(fits);     // Number of HDUs in file
    if (hdus == 0) {
        // We're creating the first image
        fits_create_img(fits->fd, 16, 0, NULL, &status);
    } else {
        // Insert after the current position
        fits_insert_img(fits->fd, 16, 0, NULL, &status);
    }

    if (status) {
        char fitsErr[MAX_STRING_LENGTH];
        (void)fits_get_errstatus(status, fitsErr);
        psError(PS_ERR_IO, true, "Unable to create blank header.\n%s\n", fitsErr);
        return false;
    }

    if (output && !fitsWriteHeader(fits, output, false, false)) {
        psError(PS_ERR_IO, false, "Unable to write FITS header.\n");
        return false;
    }

    if (extname && strlen(extname)) {
        psFitsSetExtName(fits, extname);
    }

    char buffer[10];
    fits_write_img(fits->fd, TSHORT, 1, 0, buffer, &status);

    return true;
}

bool psFitsHeaderValidate(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    // Traverse the metadata list and inspect at each key
    psListIterator* iter = psListIteratorAlloc(header->list, PS_LIST_HEAD, true); // Iterator
    psMetadataItem* item;               // Item from iteration
    bool valid = true;                  // Are all items valid?
    while ((item = psListGetAndIncrement(iter))) {
        if (item->type > PS_DATA_STRING) { // i.e., a non-primitive type
            valid = false;
        }

        if (strlen(item->name) > 8) {
            item->name[8] = '\0'; // truncate to 8 characters
        }

        fits_uppercase(item->name); // make uppercase

        // now, let's see if CFITSIO thinks this is a good keyword...
        int status = 0;                 // Status from cfitsio calls
        if (fits_test_keyword(item->name,&status) != 0) {
            valid = false;
        }
    }
    psFree(iter);

    return valid;
}

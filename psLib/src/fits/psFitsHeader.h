/* @file  psFitsHeader.h
 * @brief Contains Fits header I/O routines
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-10-03 21:27:21 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_FITSHEADER_H
#define PS_FITSHEADER_H

/// @addtogroup FileIO Input/Output
/// @{

#include "psFits.h"
#include "psMetadata.h"


/// Determine whether the current HDU is an empty PHU with a compressed image following.
///
/// In that case, what should be treated as an image PHU is technically an empty PHU with a binary table
/// extension.  We test the current position, number of extensions, the FITS headers and presence of a
/// following compressed image to determine if this is the case.  If so, the FITS file pointer is left
/// pointing at the compressed image.
bool psFitsCheckCompressedImagePHU(const psFits *fits, ///< FITS file pointer
                                   const psMetadata *header ///< Header, or NULL
    );

/// Parse a string read from the FITS header.

/// Removes the quotes, and any trailing spaces.  NOTE: the returned string is NOT on the psLib memory system
/// --- it's simply a hacked version of the input string.  Note also that the input string is MODIFIED.
char *p_psFitsHeaderParseString(char *string ///< String to parse
    );

/** Reads the header of the current HDU.
 *
 *  @return psMetadata*   the header data
 */
psMetadata* psFitsReadHeader(
    psMetadata* out,
    ///< The psMetadata to add the header data.  If null, a new psMetadata is created.
    const psFits* fits                 ///< the psFits object
);

/** Reads the header of all HDUs.  The current HDU is not changed.
 *
 *  @return psMetadata*      the header data set as a number of metadata entries
 */
psMetadata* psFitsReadHeaderSet(
    psMetadata* out,                         ///< output metadata or NULL if new psMetadata is to be created.
    const psFits* fits                       ///< the psFits object
);

/** Writes the values of the metadata to the current HDU header.
 *  Doesn't check if the header has to be created.
 *
 * @return bool         if TRUE, the write was successful, otherwise FALSE.
 */
bool psFitsWriteHeader(
    psFits* fits,                       ///< the psFits object
    const psMetadata* output            ///< the psMetadata data in which to write
);

/** Write a header for an image
 *  Principal difference with psFitsWriteHeader is this allows writing ZSIMPLE for uncompressing a single image
 */
bool psFitsWriteHeaderImage(
    psFits *fits,                       ///< FITS file
    const psMetadata *output,           ///< Header to output
    bool phuImage                       ///< Is this image supposed to be in the PHU?
    );


/** Writes a "blank" --- a header only, with no image or table.
 *
 *  @return bool        if TRUE, the write was successful, otherwise FALSE.
 */
bool psFitsWriteBlank(
    psFits* fits,                       ///< the psFits object
    const psMetadata* output,           ///< the psMetadata data in which to write
    const char *extname
);

/** psFitsHeaderValidate validates the supplied header so that it is in
 *  compliance to the FITS standard for header keyword names and types.
 *
 *  @return bool        TRUE if the resulting header conforms to the FITS
 *                      standard, otherwise FALSE
 */
bool psFitsHeaderValidate(psMetadata *header);

/// @}
#endif // #ifndef PS_FITS_H

/* @file pmHDU.h
 * @brief Define a header data unit (from a FITS file), with functions to read and write
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 *
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_HDU_H
#define PM_HDU_H

#include <pslib.h>
#include "pmConfig.h"

/// @addtogroup Camera Camera Layout
/// @{

#define PM_HDU_COVARIANCE_KEYWORD "PS_COVAR" // FITS keyword to indicate presence of a covariance matrix

/// An instance of the FITS Header Data Unit
///
/// Of course, it is not an exact replica of a FITS HDU --- they have no mask and variance data, but these are
/// stored here for convenience --- it keeps all the relevant data about the image in one place.
typedef struct {
    psString extname;                   ///< The extension name
    bool blankPHU;                      ///< Is this a blank FITS Primary Header Unit, i.e., no data?
    psMetadata *format;                 ///< The camera format
    psMetadata *header;                 ///< The FITS header, or NULL if primary for FITS; or section info
    psArray *images;                    ///< Pixel data
    psArray *variances;                 ///< Variance in the pixel data, or NULL
    psArray *masks;                     ///< Mask for the pixel data, or NULL
} pmHDU;


/// Allocator for pmHDU
pmHDU *pmHDUAlloc(const char *extname);   ///< Extension name, or NULL for PHU
bool psMemCheckHDU(psPtr ptr);

/// Read the HDU header only
///
/// Moves to the appropriate extension
bool pmHDUReadHeader(pmHDU *hdu,        ///< HDU for which to read header
                     psFits *fits       ///< FITS file from which to read
                    );

/// Read the HDU header and pixels
///
/// Moves to the appropriate extension
bool pmHDURead(pmHDU *hdu,              ///< HDU to read
               psFits *fits             ///< FITS file to read from
              );

/// Read the HDU header and mask
///
/// Moves to the appropriate extension
bool pmHDUReadMask(pmHDU *hdu,          ///< HDU to read
                   psFits *fits         ///< FITS file to read from
                  );

/// Read the HDU header and variance map
///
/// Moves to the appropriate extension
bool pmHDUReadVariance(pmHDU *hdu,        ///< HDU to read
                       psFits *fits       ///< FITS file to read from
    );

/// Write the HDU header and pixels
bool pmHDUWrite(pmHDU *hdu,             ///< HDU to write
                psFits *fits,           ///< FITS file to write to
                const pmConfig *config  ///< Configuration
    );

/// Write the HDU header and mask
bool pmHDUWriteMask(pmHDU *hdu,         ///< HDU to write
                    psFits *fits,       ///< FITS file to write to
                    const pmConfig *config  ///< Configuration
    );

/// Write the HDU header and variance map
bool pmHDUWriteVariance(pmHDU *hdu,       ///< HDU to write
                        psFits *fits,     ///< FITS file to write to
                        const pmConfig *config  ///< Configuration
    );


/// Read identifiers from FITS header
bool pmHDUReadIdentifiers(psS64 *imageId, ///< Image identifer, returned
                          psS64 *sourceId, ///< Source identifier, returned
                          const pmHDU *hdu ///< HDU from which to read
    );

/// Write identifiers to FITS header
bool pmHDUWriteIdentifiers(pmHDU *hdu, ///< HDU to which to write
                           psS64 imageId, ///< Image identifer
                           psS64 sourceId ///< Source identifier
                           );

/// @}
#endif

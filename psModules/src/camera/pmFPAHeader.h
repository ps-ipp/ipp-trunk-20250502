/*  @file pmFPAHeader.h
 *  @brief Functions read FITS headers for FPA components
 *
 *  @author Paul Price, IfA
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-06-17 22:16:38 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_HEADER_H
#define PM_FPA_HEADER_H

/// @addtogroup Camera Camera Layout
/// @{

/// Read the FITS header (and ingest concepts) for an FPA, if it exists at this level
///
/// Returns false if there was a problem.  Returns true if it successfully read the header, or if the header
/// was already there.  No iteration to lower levels is performed.
bool pmFPAReadHeader(pmFPA *fpa,        ///< FPA for which to read header
                     psFits *fits,       ///< FITS file handle
                     pmConfig *config   ///< Configuration
                    );

/// Read the FITS header (and ingest concepts) for a chip, if it exists at this level
///
/// Returns false if there was a problem.  Returns true if it successfully read the header, or if the header
/// was already there.  No iteration to lower levels is performed.
bool pmChipReadHeader(pmChip *chip,     ///< Chip for which to read header
                      psFits *fits,      ///< FITS file handle
                     pmConfig *config   ///< Configuration
                     );

/// Read the FITS header (and ingest concepts) for a cell, if it exists at this level
///
/// Returns false if there was a problem.  Returns true if it successfully read the header, or if the header
/// was already there.  No iteration to lower levels is performed.
bool pmCellReadHeader(pmCell *cell,     ///< Cell for which to read header
                      psFits *fits,      ///< FITS file handle
                     pmConfig *config   ///< Configuration
                     );
/// @}
#endif

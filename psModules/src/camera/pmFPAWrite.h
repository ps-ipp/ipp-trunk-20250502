/* @file pmFPAWrite.h
 * @brief Write FPA components to a FITS file
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 *
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_WRITE_H
#define PM_FPA_WRITE_H

#include <pmConfig.h>

/// @addtogroup Camera Camera Layout
/// @{

/// Write a readout incrementally
///
/// Writes a readout to a FITS file, perhaps incrementally (if it has been read in using pmReadoutReadNext).
/// Relies on the FITS header to specify how many image planes there are, and the width and height.
bool pmReadoutWriteNext(pmReadout *readout, ///< Readout to write
                        psFits *fits,   ///<  FITS file to which to write
                        int z           ///<  Image plane to write
    );

/// Write a cell to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU pixels if required.  A blank (i.e., image-less header) is
/// written only if specifically requested.  Writes the concepts to the various locations, and then the HDU to
/// the FITS file.  This function should be called at the beginning of the output cell loop with blank=true in
/// order to produce the correct file structure.
bool pmCellWrite(pmCell *cell,          ///<  Cell to write
                 psFits *fits,          ///<  FITS file to which to write
                 pmConfig *config,      ///<  Configuration
                 bool blank             ///<  Write a blank PHU?
    );

/// Write a chip to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU pixels if required.  A blank (i.e., image-less header) is
/// written only if specifically requested.  Writes the concepts to the various locations, and then the HDU to
/// the FITS file, optionally recursing to lower levels.  This function should be called at the beginning of
/// the output chip loop with blank=true and recurse=false in order to produce the correct file structure.
bool pmChipWrite(pmChip *chip,          ///<  Chip to write
                 psFits *fits,          ///<  FITS file to which to write
                 pmConfig *config,      ///<  Configuration
                 bool blank,            ///<  Write a blank PHU?
                 bool recurse           ///<  Recurse to lower levels?
    );

/// Write an FPA to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU pixels if required.  A blank (i.e., image-less header) is
/// written only if specifically requested.  Writes the concepts to the various locations, and then the HDU to
/// the FITS file, optionally recursing to lower levels.  This function should be called at the beginning of
/// the output FPA loop with blank=true and recurse=false in order to produce the correct file structure.
bool pmFPAWrite(pmFPA *fpa,             ///<  FPA to write
                psFits *fits,           ///<  FITS file to which to write
                pmConfig *config,       ///<  Configuration
                bool blank,             ///<  Write a blank PHU?
                bool recurse            ///<  Recurse to lower levels?
    );

/// Write a cell mask to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU mask pixels if required.  A blank (i.e., image-less
/// header) is written only if specifically requested.  Writes the concepts to the various locations, and then
/// the HDU mask to the FITS file.  This function should be called at the beginning of the output cell loop
/// with blank=true in order to produce the correct file structure.
bool pmCellWriteMask(pmCell *cell,      ///<  Cell to write
                     psFits *fits,      ///<  FITS file to which to write
                     pmConfig *config,  ///<  Configuration
                     bool blank         ///<  Write a blank PHU?
    );

/// Write a chip mask to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU mask pixels if required.  A blank (i.e., image-less
/// header) is written only if specifically requested.  Writes the concepts to the various locations, and then
/// the HDU mask to the FITS file, optionally recursing to lower levels.  This function should be called at
/// the beginning of the output chip loop with blank=true and recurse=false in order to produce the correct
/// file structure.
bool pmChipWriteMask(pmChip *chip,      ///<  Chip to write
                     psFits *fits,      ///<  FITS file to which to write
                     pmConfig *config,  ///<  Configuration
                     bool blank,        ///<  Write a blank PHU?
                     bool recurse       ///<  Recurse to lower levels?
    );

/// Write an FPA mask to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU mask pixels if required.  A blank (i.e., image-less
/// header) is written only if specifically requested.  Writes the concepts to the various locations, and then
/// the HDU mask to the FITS file, optionally recursing to lower levels.  This function should be called at
/// the beginning of the output FPA loop with blank=true and recurse=false in order to produce the correct
/// file structure.
bool pmFPAWriteMask(pmFPA *fpa,         ///<  FPA to write
                    psFits *fits,       ///<  FITS file to which to write
                    pmConfig *config,   ///<  Configuration
                    bool blank,         ///<  Write a blank PHU?
                    bool recurse        ///<  Recurse to lower levels?
    );

/// Write a cell variance to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU variance pixels if required.  A blank (i.e., image-less
/// header) is written only if specifically requested.  Writes the concepts to the various locations, and then
/// the HDU variance to the FITS file.  This function should be called at the beginning of the output cell
/// loop with blank=true in order to produce the correct file structure.
bool pmCellWriteVariance(pmCell *cell,    ///<  Cell to write
                         psFits *fits,    ///<  FITS file to which to write
                         pmConfig *config, ///<  Configuration
                         bool blank       ///<  Write a blank PHU?
    );

/// Write a chip variance to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU variance pixels if required.  A blank (i.e., image-less
/// header) is written only if specifically requested.  Writes the concepts to the various locations, and then
/// the HDU variance to the FITS file, optionally recursing to lower levels.  This function should be called
/// at the beginning of the output chip loop with blank=true and recurse=false in order to produce the correct
/// file structure.
bool pmChipWriteVariance(pmChip *chip,    ///<  Chip to write
                         psFits *fits,    ///<  FITS file to which to write
                         pmConfig *config, ///<  Configuration
                         bool blank,      ///<  Write a blank PHU?
                         bool recurse     ///<  Recurse to lower levels?
    );

/// Write an FPA variance to a FITS file
///
/// Generates CELL.TRIMSEC, CELL.BIASSEC and the HDU variance pixels if required.  A blank (i.e., image-less
/// header) is written only if specifically requested.  Writes the concepts to the various locations, and then
/// the HDU variance to the FITS file, optionally recursing to lower levels.  This function should be called
/// at the beginning of the output FPA loop with blank=true and recurse=false in order to produce the correct
/// file structure.
bool pmFPAWriteVariance(pmFPA *fpa,       ///<  FPA to write
                        psFits *fits,     ///<  FITS file to which to write
                        pmConfig *config, ///<  Configuration
                        bool blank,       ///<  Write a blank PHU?
                        bool recurse      ///<  Recurse to lower levels?
    );


/// Write a FITS table from the cell's analysis metadata.
///
/// The FITS table (a psArray of psMetadatas) from the cell's analysis metadata (under "name") is written to
/// the FITS file, at an extension specified by the name, chip name and cell name ("NAME_CHIP_CELL").  If a
/// header is present in the analysis metadata ("name.HEADER"), then it is written also.
int pmCellWriteTable(psFits *fits,      ///< FITS file to which to write
                     const pmCell *cell, ///< Cell containing FITS table in the analysis metadata
                     const char *name   ///< Name for the table data, and the extension name
    );

int pmChipWriteTable(psFits *fits,      ///< FITS file to which to write
                     const pmChip *chip, ///< Chip containing cells with tables to write
                     const char *name   ///< Name for the table data, and the extension name
    );

int pmFPAWriteTable(psFits *fits,       ///< FITS file to which to write
                    const pmFPA *fpa,   ///< FPA containing cells with tables to write
                    const char *name    ///< Name for the table data, and the extension name
    );

/// Write covariance matrix to a FITS file
///
/// The covariance matrices for a cell are written to an independent extension, named after the chip and cell
/// name.
bool pmCellWriteCovariance(psFits *fits,///< FITS file to which to write
                           const pmCell *cell ///< Cell for which to write covariance
    );

bool pmChipWriteCovariance(psFits *fits,///< FITS file to which to write
                           const pmChip *chip ///< Chip for which to write covariance
    );

bool pmFPAWriteCovariance(psFits *fits,///< FITS file to which to write
                          const pmFPA *fpa ///< FPA for which to write covariance
    );

// Update the header before writing to be consistent
//
// XXX Would like a better name
bool pmFPAUpdateNames(pmFPA *fpa,       ///< FPA
                      pmChip *chip,     ///< Chip, or NULL
                      pmCell *cell,     ///< Cell, or NULL
                      psS64 imageId,    ///< Image identifier
                      psS64 sourceId    ///< Source identifier
    );

/// @}
#endif

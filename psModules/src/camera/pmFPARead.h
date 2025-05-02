/* @file pmFPARead.h
 * @brief Functions to read FPA components from a FITS file
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_READ_H
#define PM_FPA_READ_H

#include <pmConfig.h>

/// @addtogroup Camera Camera Layout
/// @{

/// Check to see if there is more to read when reading a readout incrementally
bool pmReadoutMore(pmReadout *readout,  ///< Readout of interest
                   psFits *fits,        ///< FITS file from which to read
                   int z,               ///< Readout number/plane; zero-offset indexing
		   int *zMax,		///< Max plane number in this cell
                   int numScans,        ///< Number of scans (rows/cols) to read
                   pmConfig *config     ///< Configuration
    );

/// Read a chunk of a readout.
///
/// Allows reading the readout incrementally
bool pmReadoutReadChunk(pmReadout *readout, ///< Readout of interest
                        psFits *fits,   ///< FITS file from which to read
                        int z,          ///< Readout number/plane; zero-offset indexing
		   int *zMax,		///< Max plane number in this cell
                        int numScans,   ///< Number of scans (rows/cols) to read
                        int overlap,    ///< Overlap between consecutive reads
                        pmConfig *config ///< Configuration
    );

/// Read the entire readout
bool pmReadoutRead(pmReadout *readout,  ///< Readout of interest
                   psFits *fits,        ///< FITS file from which to read
                   int z,               ///< Readout number/plane; zero-offset indexing
                   pmConfig *config     ///< Configuration
    );

/// Read a readout incrementally --- this is maintained temporarily only for backwards compatibility; it has
/// been replaced by pmReadoutRead, pmReadoutReadChunk and pmReadoutMore.
///
/// Multiple calls to this function moves through a readout within a cell incrementally.  It is required to
/// pass the readout previously acquired (or a newly-allocated one) in order to preserve state information
/// (where the read is at).  Only a maximum of numRows rows are read at a time.  Returns true if pixels are
/// read, and false otherwise.  To facilitate looping, no error is generated for reading a plane that doesn't
/// exist.  Note that this function doesn't put pixels in the HDU.  It is therefore NOT COMPATIBLE with
/// pmCellWrite, pmChipWrite, and pmFPAWrite (or any function that uses or creates an HDU for that matter).
/// Use pmReadoutWriteNext to write the data that's read by this function.  This function is intended for
/// reading in many readouts into memory at once (e.g., for stacking) where the input is not written out.
bool pmReadoutReadNext(bool *status,    // non-error exit condition?
                       pmReadout *readout, // Readout into which to read
                       psFits *fits,    // FITS file from which to read
                       int z,           // Readout number/plane; zero-offset indexing
                       int numRows,      // The number of rows to read
                       pmConfig *config
    );

/// Return the number of readouts within a cell
///
/// This function is type-independent (doesn't matter if you are interested in the image/mask/variance).
int pmCellNumReadouts(pmCell *cell, psFits *fits, pmConfig *config);

/// Read an entire cell
///
/// Reads the appropriate HDU, ingests concepts from the header, and portions pixels into readouts.  Pixels
/// are converted to F32.
bool pmCellRead(pmCell *cell,           // Cell to read into
                psFits *fits,           // FITS file from which to read
                pmConfig *config        // Configuration
    );

/// Read a chip
///
/// Iterates over component cells, reading each with pmCellRead.
bool pmChipRead(pmChip *chip,           // Chip to read into
                psFits *fits,           // FITS file from which to read
                pmConfig *config        // Configuration
    );

/// Read an FPA
///
/// Iterates over component chips, reading each with pmChipRead.
bool pmFPARead(pmFPA *fpa,              // FPA to read into
               psFits *fits,            // FITS file from which to read
               pmConfig *config         // Configuration
    );

// Mask functions follow

/// Check to see if there is more to read when reading a readout incrementally into the mask
bool pmReadoutMoreMask(pmReadout *readout, ///< Readout of interest
                       psFits *fits,    ///< FITS file from which to read
                       int z,           ///< Readout number/plane; zero-offset indexing
		       int *zMax,	///< Max plane number in this cell
                       int numScans,    ///< Number of scans (rows/cols) to read
                       pmConfig *config ///< Configuration
    );

/// Read a chunk of a readout into the mask
///
/// Allows reading the readout incrementally
bool pmReadoutReadChunkMask(pmReadout *readout, ///< Readout of interest
                            psFits *fits, ///< FITS file from which to read
                            int z,      ///< Readout number/plane; zero-offset indexing
			    int *zMax,		///< Max plane number in this cell
                            int numScans, ///< Number of scans (rows/cols) to read
                            int overlap, ///< Overlap between consecutive reads
                            pmConfig *config ///< Configuration
    );

/// Read the entire readout into the mask
bool pmReadoutReadMask(pmReadout *readout, ///< Readout of interest
                       psFits *fits,    ///< FITS file from which to read
                       int z,           ///< Readout number/plane; zero-offset indexing
                       pmConfig *config ///< Configuration
    );

/// Read an entire cell into the mask
///
/// Same as pmCellRead, but reads into the mask element of the readouts.
bool pmCellReadMask(pmCell *cell,       // Cell to read into
                    psFits *fits,       // FITS file from which to read
                    pmConfig *config    // Configuration
    );

/// Read an entire chip into the mask
///
/// Same as pmChipRead, but reads into the mask element of the readouts.
bool pmChipReadMask(pmChip *chip,       // Chip to read into
                    psFits *fits,       // FITS file from which to read
                    pmConfig *config    // Configuration
    );

/// Read an entire FPA into the mask
///
/// Same as pmFPARead, but reads into the mask element of the readouts.
bool pmFPAReadMask(pmFPA *fpa,          // FPA to read into
                   psFits *fits,        // FITS file from which to read
                   pmConfig *config     // Configuration
    );

// Variance functions follow

/// Check to see if there is more to read when reading a readout incrementally into the variance
bool pmReadoutMoreVariance(pmReadout *readout, ///< Readout of interest
                           psFits *fits, ///< FITS file from which to read
                           int z,       ///< Readout number/plane; zero-offset indexing
			   int *zMax,	///< Max plane number in this cell
                           int numScans, ///< Number of scans (rows/cols) to read
                           pmConfig *config ///< Configuration
    );

/// Read a chunk of a readout into the variance
///
/// Allows reading the readout incrementally
bool pmReadoutReadChunkVariance(pmReadout *readout, ///< Readout of interest
                                psFits *fits, ///< FITS file from which to read
                                int z,    ///< Readout number/plane; zero-offset indexing
		   int *zMax,		///< Max plane number in this cell
                                int numScans, ///< Number of scans (rows/cols) to read
                                int overlap, ///< Overlap between consecutive reads
                                pmConfig *config ///< Configuration
    );

/// Read the entire readout into the variance
bool pmReadoutReadVariance(pmReadout *readout, ///< Readout of interest
                           psFits *fits,  ///< FITS file from which to read
                           int z,         ///< Readout number/plane; zero-offset indexing
                           pmConfig *config ///< Configuration
    );

/// Read an entire cell into the variance
///
/// Same as pmCellRead, but reads into the variance element of the readouts.
bool pmCellReadVariance(pmCell *cell,     // Cell to read into
                        psFits *fits,     // FITS file from which to read
                        pmConfig *config  // Configuration
    );

/// Read an entire chip into the variance
///
/// Same as pmChipRead, but reads into the variance element of the readouts.
bool pmChipReadVariance(pmChip *chip,     // Chip to read into
                        psFits *fits,     // FITS file from which to read
                        pmConfig *config  // Configuration
    );

/// Read an entire FPA into the variance
///
/// Same as pmFPARead, but reads into the variance element of the readouts.
bool pmFPAReadVariance(pmFPA *fpa,        // FPA to read into
                       psFits *fits,      // FITS file from which to read
                       pmConfig *config   // Configuration
    );

/// Read cell headers
///
/// Same as pmCellRead, but reads only the headers of the readouts.
bool pmCellReadHeaderSet(pmCell *cell,  // Cell to read into
                         psFits *fits,  // FITS file from which to read
                         pmConfig *config // Configuration
    );

/// Read chip headers
///
/// Same as pmChipRead, but reads only the headers of the readouts.
bool pmChipReadHeaderSet(pmChip *chip,  // Chip to read into
                      psFits *fits,     // FITS file from which to read
                      pmConfig *config  // Configuration
                     );

/// Read FPA headers
///
/// Same as pmFPARead, but reads only the headers of the readouts.
bool pmFPAReadHeaderSet(pmFPA *fpa,     // FPA to read into
                        psFits *fits,   // FITS file from which to read
                        pmConfig *config // Configuration
    );

/// Read a FITS table into the cell
///
/// Given a name, which is combined with the chip and cell to identify the extension name ("NAME_CHIP_CELL"),
/// read the FITS table into the cell analysis metadata (with key being the provided name).  The header is
/// also read and included in the cell analysis metadata under "name.HEADER".
int pmCellReadTable(pmCell *cell,       ///< Cell for which to read table
                    psFits *fits,       ///< FITS file from which to read the table
                    const char *name    ///< Specifies the extension name, and target in the analysis metadata
                   );

/// Read a FITS table into the component cells
///
/// Iterates over component cells, calling pmCellReadTable.
int pmChipReadTable(pmChip *chip,       ///< Chip for which to read table
                    psFits *fits,       ///< FITS file from which to read the table
                    const char *name    ///< Specifies the extension name, and target in the analysis metadata
                   );

/// Read a FITS table into the component cells
///
/// Iterates over component chips, calling pmChipReadTable.
int pmFPAReadTable(pmFPA *fpa,          ///< FPA for which to read table
                   psFits *fits,        ///< FITS file from which to read the table
                   const char *name     ///< Specifies the extension name, and target in the analysis metadata
                  );

/// Read covariance matrices for a cell
bool pmCellReadCovariance(pmCell *cell, ///< Cell for which to read covariance matrices
                          psFits *fits  ///< FITS file from which to read
    );

/// Read covariance matrices for a chip
bool pmChipReadCovariance(pmChip *chip, ///< Chip for which to read covariance matrices
                          psFits *fits  ///< FITS file from which to read
    );

/// Read covariance matrices for a cell
bool pmFPAReadCovariance(pmFPA *fpa,    ///< FPA for which to read covariance matrices
                         psFits *fits   ///< FITS file from which to read
    );



/// @}
#endif

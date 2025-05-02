/*  @file pmFPAFlags.h
 *  @brief Functions for setting and checking the status flags within the FPA hierarchy
 * 
 *  @author George Gusciora, MHPCC
 *  @author Paul Price, IfA
 *  @author Eugene Magnier, IfA
 * 
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-05-03 20:04:01 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_FLAGS_H
#define PM_FPA_FLAGS_H

/// @addtogroup Camera Camera Layout
/// @{

// Functions to turn on/off the file_exists flags

/// Set the file_exists flag for an FPA and components
bool pmFPASetFileStatus(pmFPA *fpa,     ///< FPA for which to set status
                        bool status     ///< Status to set
                       );

/// Set the file_exists flag for a chip and components
bool pmChipSetFileStatus(pmChip *chip,  ///< Chip for which to set status
                         bool status    ///< Status to set
                        );

/// Set the file_exists flag for a cell and components
bool pmCellSetFileStatus(pmCell *cell,  ///< Cell for which to set status
                         bool status    ///< Status to set
                        );

bool pmReadoutSetFileStatus(pmReadout *readout, bool status);

bool pmFPAviewSetFileStatus (pmFPA *fpa, const pmFPAview *view, bool status);

// Functions to check the file_exists flags

/// Return the file_exists status for an FPA and components
bool pmFPACheckFileStatus(const pmFPA *fpa ///< FPA for which to check status
                         );

/// Return the file_exists status for a chip and components
bool pmChipCheckFileStatus(const pmChip *chip ///< Chip for which to check status
                          );

/// Return the file_exists status for a chip and components
bool pmCellCheckFileStatus(const pmCell *cell ///< Cell for which to check status
                          );

// Functions to turn on/off the data_exists flags

/// Set the data_exists flag for an FPA and components
bool pmFPASetDataStatus(pmFPA *fpa,     ///< FPA for which to set status
                        bool status     ///< Status to set
                       );

/// Set the data_exists flag for a chip and components
bool pmChipSetDataStatus(pmChip *chip,  ///< Chip for which to set status
                         bool status    ///< Status to set
                        );

/// Set the data_exists flag for a cell and components
bool pmCellSetDataStatus(pmCell *cell,  ///< Cell for which to set status
                         bool status    ///< Status to set
                        );

bool pmReadoutSetDataStatus (pmReadout *readout, bool status);

bool pmFPAviewSetDataStatus (pmFPA *fpa, const pmFPAview *view, bool status);

// Functions the check the data_exists flags

/// Check data_exists for the element of this fpa at this view
bool pmFPAviewCheckDataStatus (const pmFPA *fpa, ///< FPA to check
			       const pmFPAview *view ///< check for this view 
  );

/// Check data_exists for this fpa
bool pmFPACheckDataStatus (const pmFPA *fpa ///< FPA to check
  );

/// Check data_exists for this chip
bool pmChipCheckDataStatus (const pmChip *chip ///< Chip to check
);

/// Check data_exists for this cell
bool pmCellCheckDataStatus (const pmCell *cell ///< Cell to check
);

/// Check data_exists for this readout
bool pmReadoutCheckDataStatus (const pmReadout *readout ///< Readout to check
);


// Functions to set the process flags

/// Select a chip within an FPA for processing
///
/// If exclusive is true, the specified chip is the only chip to be processed.  A negative value for chipNum
/// is valid and, in combinations with exclusive, de-selects all chips.
bool pmFPASelectChip(pmFPA *fpa,        ///< FPA containing the chip of interest
                     int chipNum,       ///< Chip number to select
                     bool exclusive     ///< Process this chip exclusive of the others?
                    );

/// Select a chip within a chip for processing
///
/// If exclusive is true, the specified cell is the only chip to be processed.  A negative value for cellNum
/// is valid and, in combinations with exclusive, de-selects all chips.
bool pmChipSelectCell(pmChip *chip,     ///< Chip containing the cell of interest
                      int cellNum,      ///< Cell number to select
                      bool exclusive    ///< Process this cell exclusive of the others?
                     );

/// Exclude a chip within an FPA from processing
int pmFPAExcludeChip(pmFPA *fpa,        ///< FPA containing the chip of interest
                     int chipNum        ///< Chip number to exclude
                    );

/// Exclude all chips within an FPA from processing
bool pmFPAExcludeChips(pmFPA *fpa        ///< FPA containing the chip of interest
                    );

bool pmChipSelectCells(pmChip *chip);

/// Exclude a cell within a chip from processing
int pmChipExcludeCell(pmChip *chip,     ///< Chip containing the chip of interest
                      int cellNum       ///< Cell number to exclude
                     );
/// @}
#endif

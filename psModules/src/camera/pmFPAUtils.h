/* @file pmFPAUtils.h
 * @brief Utility functions for FPAs
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_UTILS_H
#define PM_FPA_UTILS_H

/// @addtogroup Camera Camera Layout
/// @{

/// Find a chip by name; return the index
///
/// Looks for a chip within the FPA with CHIP.NAME matching the provided name.  Returns the index of the chip,
/// or -1 if it was not found.
int pmFPAFindChip(const pmFPA *fpa,     ///< FPA in which to find the chip
                  const char *name      ///< Name of the chip
                 );

/// Find a cell by name; return the index
///
/// Looks for a cell within the chip with CELL.NAME matching the provided name.  Returns the index of the
/// cell, or -1 if it was not found.
int pmChipFindCell(const pmChip *chip,  // Chip in which to find the cell
                   const char *name     // Name of the cell
                  );
/// @}
#endif

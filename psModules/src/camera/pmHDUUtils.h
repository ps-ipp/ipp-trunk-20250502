/* @file pmHDUUtils.h
 * @brief Utility functions for working with an HDU
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-07-15 20:25:00 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_HDU_UTILS_H
#define PM_HDU_UTILS_H

#include <pmHDU.h>
#include <pmFPA.h>

/// @addtogroup Camera Camera Layout
/// @{

/// Get the first HDU encountered in the hierarchy
pmHDU *pmHDUGetFirst (const pmFPA *fpa);

/// Get the lowest HDU in the hierarchy
///
/// The lowest HDU in the hierarchy will be the one with the actual pixels (if all levels are supplied).
pmHDU *pmHDUGetLowest(const pmFPA *fpa, ///< The FPA
                      const pmChip *chip, ///< The chip, or NULL
                      const pmCell *cell ///< The cell, or NULL
                     );

/// Get the highest HDU in the hierarchy
///
/// The highest HDU in the hierarchy will be the PHU (might get NULL if not all levels are supplied)
pmHDU *pmHDUGetHighest(const pmFPA *fpa, ///< The FPA
                       const pmChip *chip, ///< The chip, or NULL
                       const pmCell *cell ///< The cell, or NULL
                      );

/// Given an FPA, return the HDU (or NULL if all HDUs reside below the FPA)
pmHDU *pmHDUFromFPA(const pmFPA *fpa    ///< FPA for which to find HDU
                   );

/// Given a chip, return the HDU (or NULL if it resides below the chip)
pmHDU *pmHDUFromChip(const pmChip *chip ///< Chip for which to find HDU
                    );

/// Given a cell, return the HDU
pmHDU *pmHDUFromCell(const pmCell *cell ///< Cell for which to find HDU
                    );

/// Given a readout, return the HDU
pmHDU *pmHDUFromReadout(const pmReadout *readout ///< Readout for which to find HDU
                       );

/// Print details about an HDU
///
/// This is intended for testing or development use.
void pmHDUPrint(FILE *fd,               ///< File descriptor to which to print
                const pmHDU *hdu,       ///< HDU to print
                int level,              ///< Level at which to print
                bool header             ///< Print header?
               );
/// @}
#endif

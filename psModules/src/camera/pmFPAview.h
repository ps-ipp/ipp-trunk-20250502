/* @file pmFPA.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-25 22:05:58 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_VIEW_H
#define PM_FPA_VIEW_H

/// @addtogroup Camera Camera Layout
/// @{

#include "pmFPA.h"
#include "pmFPALevel.h"

/// Identifier for FPA components
///
/// This structure allows the identification of a single component of the focal plane hierarchy (or multiple,
/// if we consider selecting all those components below the selected component).  Components are identified on
/// the basis of their chip, cell, readout index.  An index of -1 means all components at that level.
/// Additionally, since readouts may be read piecemeal, there are additional indices for these.
typedef struct
{
    int chip;                           ///< Number of the chip, or -1 for all
    int cell;                           ///< Number of the cell, or -1 for all
    int readout;                        ///< Number of the readout, or -1 for all
    int nRows;                          ///< Maximum number of rows per readout segment read, or 0 for all
    int iRows;                          ///< Starting point for this read
}
pmFPAview;

/// Allocator for pmFPAview
pmFPAview *pmFPAviewAlloc(int nRows);   ///< Maximum number of rows per readout segment read, or 0 for all
bool psMemCheckFPAview(psPtr ptr);

/// Reset a view to select all components
bool pmFPAviewReset(pmFPAview *view     ///< View to reset
                   );

// return a view restricted to the level (must be >= the input level)
pmFPAview *pmFPAviewForLevel(pmFPALevel level, const pmFPAview *input);

/// Determine the current view level
///
/// Returns the level appropriate for the view
pmFPALevel pmFPAviewLevel(const pmFPAview *view ///< View to examine
                         );

// Lookups

/// Return the currently selected chip for this view
///
/// Returns NULL if the selection is not specific or invalid
pmChip *pmFPAviewThisChip(const pmFPAview *view, ///< Current view
                          const pmFPA *fpa ///< FPA containing chip
                         );

/// Return the currently selected cell for this view
///
/// Returns NULL if the selection is not specific or invalid
pmCell *pmFPAviewThisCell(const pmFPAview *view, ///< Current view
                          const pmFPA *fpa ///< FPA containing cell
                         );

/// Return the currently selected readout for this view
///
/// Returns NULL if the selection is not specific or invalid
pmReadout *pmFPAviewThisReadout(const pmFPAview *view, ///< Current view
                                const pmFPA *fpa ///< FPA containing readout
                               );

// Incrementors

/// Advance view to the next chip
///
/// Returns NULL if there is no next
pmChip *pmFPAviewNextChip(pmFPAview *view, ///< Current view
                          const pmFPA *fpa, ///< FPA containing chips
                          int nStep     ///< Number of chips to increment
                         );

/// Advance view to the next cell
///
/// Returns NULL if there is no next
pmCell *pmFPAviewNextCell(pmFPAview *view, ///< Current view
                          const pmFPA *fpa, ///< FPA containing cells
                          int nStep     ///< Number of cells to increment
                         );

/// Advance view to the next readout
///
/// Returns NULL if there is no next
pmReadout *pmFPAviewNextReadout(pmFPAview *view, ///< Current view
                                const pmFPA *fpa, ///< FPA containing readouts
                                int nStep ///< Number of readouts to increment
                               );

/// Return the HDU corresponding to the current view
///
/// Uses the pmHDUFrom* functions, combined with the view.
pmHDU *pmFPAviewThisHDU(const pmFPAview *view, ///< Current view
                        const pmFPA *fpa ///< FPA for view
                       );

/// Return the blank Primary HDU corresponding to the current view, if any
///
/// Similar to pmFPAviewThisHDU, except returns NULL if no HDU is found, or the HDU is not a blank Primary HDU
pmHDU *pmFPAviewThisPHU(const pmFPAview *view, ///< Current view
                        const pmFPA *fpa ///< FPA for view
                       );

/// Generate a view, given a chip, cell, readout.
///
/// Uses the pointer value in the array of the parent to locate the child
pmFPAview *pmFPAviewGenerate(const pmFPA *fpa, ///< FPA of interest
                             const pmChip *chip, ///< Chip of interest, or NULL
                             const pmCell *cell, ///< Cell of interest, or NULL
                             const pmReadout *reaodut ///< Readout of interest, or NULL
    );


/// Determine the view suitable for the top level of the provided FPA
pmFPAview *pmFPAviewTop(const pmFPA *fpa ///< FPA of interest
    );

/// @}
#endif

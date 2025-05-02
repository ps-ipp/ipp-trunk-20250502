/* @file pmFPACopy.h
 * @brief Functions to copy FPA components.
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-04-12 02:51:44 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_COPY_H
#define PM_FPA_COPY_H

/// @addtogroup Camera Camera Layout
/// @{

/// Copy an FPA and components, including the pixels, to a different representation of the same camera
///
/// This function is useful for converting between different representations of the same camera.  For example,
/// between Megacam "RAW" (one amp per extension) and Megacam "SPLICED" formats (two amps = 1 chip per
/// extension, spliced together).  Components are spliced together as necessary.
bool pmFPACopy(pmFPA *target,           ///< The target FPA
               const pmFPA *source      ///< The source FPA, to be copied
              );

/// Copy a chip and components, including the pixels, to a different representation of the same camera
///
/// This function is useful for converting between different representations of the same camera.  For example,
/// between Megacam "RAW" (one amp per extension) and Megacam "SPLICED" formats (two amps = 1 chip per
/// extension, spliced together).  Components are spliced together as necessary.
bool pmChipCopy(pmChip *target,         ///< The target chip
                const pmChip *source    ///< The source chip, to be copied
               );

/// Copy a cell and components, including the pixels, to a different representation of the same camera
///
/// This function is useful for converting between different representations of the same camera.  For example,
/// between Megacam "RAW" (one amp per extension) and Megacam "SPLICED" formats (two amps = 1 chip per
/// extension, spliced together).  Components are spliced together as necessary.
bool pmCellCopy(pmCell *target,         ///< The target cell
                const pmCell *source    ///< The source cell, to be copied
               );


/// Copy an FPA, but not the pixels, to a different representation of the same camera
///
/// This function the same as pmFPACopy, except that the pixels are not copied (though images of sufficient
/// size are allocated in the target).  Changes the CELL.XBIN and CELL.YBIN according to the provided binning
/// factors.
bool pmFPACopyStructure(pmFPA *target,   ///< The target FPA
                        const pmFPA *source, ///< The source FPA, to be copied
                        int xBin, int yBin ///< Binning factors in x and y
                       );

/// Copy a chip, but not the pixels, to a different representation of the same camera
///
/// This function the same as pmChipCopy, except that the pixels are not copied (though images of sufficient
/// size are allocated in the target).  Changes the CELL.XBIN and CELL.YBIN according to the provided binning
/// factors.
bool pmChipCopyStructure(pmChip *target, ///< The target chip
                         const pmChip *source, ///< The source chip, to be copied
                         int xBin, int yBin ///< Binning factors in x and y
                        );

/// Copy a cell, but not the pixels, to a different representation of the same camera
///
/// This function the same as pmCellCopy, except that the pixels are not copied (though images of sufficient
/// size are allocated in the target).  Changes the CELL.XBIN and CELL.YBIN according to the provided binning
/// factors.
bool pmCellCopyStructure(pmCell *target, ///< The target cell
                         const pmCell *source, ///< The source cell, to be copied
                         int xBin, int yBin ///< Binning factors in x and y
                        );

pmChip *pmChipDuplicate(pmFPA *fpa, const pmChip *source);

/// @}
#endif

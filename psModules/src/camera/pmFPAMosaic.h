/* @file pmFPAMosaic.h
 * @brief Functions to mosaic FPA components into a single entity
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CHIP_MOSAIC_H
#define PM_CHIP_MOSAIC_H

/// @addtogroup Camera Camera Layout
/// @{

/// Mosaic all cells within a chip
///
/// Mosaics all cells within the source into a single cell within the target (which must have only a single
/// cell).  Cells are placed on the chip according to the CELL.X0 and CELL.Y0 offsets.  This is useful for
/// getting an image of the chip on the sky.  The mosaicking is done so as to avoid performing a deep copy of
/// the pixels, if possible.
bool pmChipMosaic(pmChip *target,       ///< Target chip --- may contain only a single cell
                  const pmChip *source, ///< Source chip whose cells will be mosaicked
                  bool deepCopy,        ///< Require a deep copy (disregard 'nice' chip)
                  psImageMaskType blank      ///< Mask value to give blank pixels
    );

/// Mosaic all cells within an FPA
///
/// Mosaics all cells within the source into a single chip with single cell within the target (which must have
/// only a single chip with single cell).  Cells are placed on the FPA according to the CHIP.X0, CHIP.Y0,
/// CELL.X0 and CELL.Y0 offsets.  This is useful for getting an image of the FPA on the sky.  The mosaicking
/// is done so as to avoid performing a deep copy of the pixels, if possible.
bool pmFPAMosaic(pmFPA *target, ///< Target FPA --- may contain only a single chip with a single cell
                 const pmFPA *source,   ///< FPA whose chips and cells will be mosaicked
                 bool deepCopy,         ///< Require a deep copy (disregard 'nice' chip)
                 psImageMaskType blank       ///< Mask value to give blank pixels
                );
/// @}
#endif

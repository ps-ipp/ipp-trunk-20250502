/* @file  pmPFAExtent.h
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-21 22:01:32 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_EXTENT_H
#define PM_FPA_EXTENT_H

/// @addtogroup Camera Camera Layout
/// @{

/// Return the extent of a readout
///
/// The extent is determined from an image, if present, or the CELL.TRIMSEC otherwise.
psRegion *pmReadoutExtent(const pmReadout *readout ///< The readout of interest
                         );

/// Return the extent of a cell
///
/// The extent is determined from the extent of the component readouts, plus CELL.X0,Y0
psRegion *pmCellExtent(const pmCell *cell ///< The cell of interest
                      );

// return chip pixels included in all cells
psRegion *pmChipPixels(const pmChip *chip);

/// Return the extent of a chip
///
/// The extent is determined from the extent of the component cells, plus CHIP.X0,Y0
psRegion *pmChipExtent(const pmChip *chip ///< The chip of interest
                      );

// return FPA pixels included in all chips
// this FPA grid has 0,0 at the 0,0 corner of one chip, and is NOT the same
// as the astrometry focal plane coordinate system
psRegion *pmFPAPixels(const pmFPA *fpa);

/// Return the extent of an FPA
///
/// The extent is determined from the extent of the component chips.
psRegion *pmFPAExtent(const pmFPA *fpa ///< The FPA of interest
                     );

/// @}
#endif

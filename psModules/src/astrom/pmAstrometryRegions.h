/* @file  pmAstrometryRegion.h
 * @brief functions to detemine fpa,chip,etc boundaries from astrometry
 *
 * @author EAM, IfA
 * @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-21 21:59:57 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROMETRY_REGIONS_H
#define PM_ASTROMETRY_REGIONS_H

// used by pmAstrometryDistortion.c, pmAstrometryWCS.c, pmAstrometryModel.c
// # define EXTRA_ORDERS 6

/// @addtogroup Astrometry
/// @{

// cell pixels corresponding to readout boundary
psRegion *pmAstromReadoutInCell (pmReadout *readout);

// chip pixels corresponding to cell boundary
psRegion *pmAstromCellInChip (pmCell *cell);

// FP pixels corresponding to chip boundary
// since the chip may be rotated in the fpa, this region does not correspond 
// exactly to the pixel grid of the chip
psRegion *pmAstromChipInFP (pmChip *chip);

// return FPA pixels included in all chips
// this FPA grid has 0,0 at the mosaic center and is used for astrometric reference.
psRegion *pmAstromFPAExtent(const pmFPA *fpa);

// chip pixels corresponding to cell boundary
psRegion *pmAstromFPInTP (pmFPA *fpa);

/// @}
#endif // PM_ASTROMETRY_DISTORTION_H

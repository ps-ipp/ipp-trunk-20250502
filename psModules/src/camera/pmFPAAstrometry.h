#ifndef PM_FPA_ASTROMETRY_H
#define PM_FPA_ASTROMETRY_H

#include "pslib.h"
#include "pmFPA.h"

/** Find cooresponding cell for given FPA coordinate
 *
 *  @return pmCell*    the cell cooresponding to the coord in FPA
 */
pmCell* pmCellInFPA(
    const psPlane* coord,              ///< the coordinate in FPA plane
    const pmFPA* FPA                   ///< the FPA to search for the cell
);


/** Find cooresponding chip for given FPA coordinate
 *
 *  @return pmChip*    the chip cooresponding to coord
 */
pmChip* pmChipInFPA(
    const psPlane* coord,              ///< the coordinate in FPA plane
    const pmFPA* FPA                   ///< the FPA to search for the cell
);


/** Find cooresponding cell for given Chip coordinate
 *
 *  @return pmCell*    the cell cooresponding to coord
 */
pmCell* pmCellInChip(
    const psPlane* coord,              ///< the coordinate in Chip plane
    const pmChip* chip                 ///< the chip to search for the cell
);


/** Translate a cell coordinate into a chip coordinate
 *
 *  @return psPlane*    the resulting chip coordinate
 */
psPlane* pmCoordCellToChip(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within Cell
    const pmCell* cell                 ///< the Cell in interest
);


/** Translate a chip coordinate into a FPA coordinate
 *
 *  @return psPlane*    the resulting FPA coordinate
 */
psPlane* pmCoordChipToFPA(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within Chip
    const pmChip* chip                 ///< the chip in interest
);


/** Translate a FPA coordinate into a Tangent Plane coordinate
 *
 *  @return psPlane*    the resulting Tangent Plane coordinate
 */
psPlane* pmCoordFPAToTP(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within FPA
    double color,                      ///< Color of source
    double magnitude,                  ///< Magnitude of source
    const pmFPA* fpa                   ///< the FPA in interest
);


/** Translate a Tangent Plane coordinate into a Sky coordinate
 *
 *  @return psSphere*    the resulting Sky coordinate
 */
psSphere* pmCoordTPToSky(
    psSphere* out,                     ///< a sphere struct to recycle. If NULL, a new struct is created
    const psPlane* in,                ///< the coordinate within Tangent Plane
    const psProjection *projection
);

/** Translate a cell coordinate into a FPA coordinate
 *
 *  @return psPlane*    the resulting FPA coordinate
 */
psPlane* pmCoordCellToFPA(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within cell
    const pmCell* cell                 ///< the cell in interest
);


/** Translate a cell coordinate into a Sky coordinate
 *
 *  @return psSphere*    the resulting Sky coordinate
 */
psSphere* pmCoordCellToSky(
    psSphere* out,                     ///< a sphere struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within cell
    double color,                      ///< Color of source
    double magnitude,                  ///< Magnitude of source
    const pmCell* cell                 ///< the cell in interest
);


/** Translate a cell coordinate into a Sky coordinate using a 'quick and
 *  dirty' method
 *
 *  @return psSphere*    the resulting Sky coordinate
 */
psSphere* pmCoordCellToSkyQuick(
    psSphere* out,                     ///< a sphere struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within cell
    const pmCell* cell                 ///< the cell in interest
);


/** Translate a Sky coordinate into a Tangent Plane coordinate
 *
 *  @return psPlane*    the resulting Tangent Plane coordinate
 */
psPlane* pmCoordSkyToTP(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psSphere* in,                ///< the sky coordinate
    const psProjection *projection
);

/** Translate a Tangent Plane coordinate into a FPA coordinate
 *
 *  @return psPlane*    the resulting FPA coordinate
 */
psPlane* pmCoordTPToFPA(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the coordinate within tangent plane
    double color,                      ///< Color of source
    double magnitude,                  ///< Magnitude of source
    const pmFPA* fpa                   ///< the FPA of interest
);


/** Translate a FPA coordinate into a chip coordinate
 *
 *  @return psPlane*    the resulting chip coordinate
 */
psPlane* pmCoordFPAToChip(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the FPA coordinate
    const pmChip* chip                 ///< the chip of interest
);


/** Translate a chip coordinate into a cell coordinate
 *
 *  @return psPlane*    the resulting cell coordinate
 */
psPlane* pmCoordChipToCell(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psPlane* in,                 ///< the Chip coordinate
    const pmCell* cell                 ///< the cell of interest
);


/** Translate a sky coordinate into a cell coordinate
 *
 *  @return psPlane*    the resulting cell coordinate
 */
psPlane* pmCoordSkyToCell(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psSphere* in,                ///< the Sky coordinate
    float color,                       ///< Color of source
    float magnitude,                   ///< Magnitude of source
    const pmCell* cell                 ///< the cell of interest
);


/** Translate a sky coordinate into a cell coordinate using a 'quick and
 *  dirty' method
 *
 *  @return psPlane*    the resulting cell coordinate
 */
psPlane* pmCoordSkyToCellQuick(
    psPlane* out,                      ///< a plane struct to recycle. If NULL, a new struct is created
    const psSphere* in,                ///< the Sky coordinate
    const pmCell* cell                 ///< the cell of interest
);


#endif

/* @file  pmPhotObj.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_PHOT_OBJ_H
# define PM_PHOT_OBJ_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{


/** pmPhotObj data structure
 *
 *  A Photometry Object is a connected set of source measurements with a common
 *  connection.  Each source that comprises the object represents the detection of some
 *  astronomical object on a single image.  The object represents the related collection
 *  of measurements.  The fits are coupled in some way.  For example, they may all have
 *  the same position, but independent fluxes.  Or, they may have a common set of
 *  positions and shape parameters.  Or the position in each image may be related by a
 *  function.
 *
 *  XXX is thre any info that is neaded for each object beyond that carried by the sources
 *  (besides ID)?
 */
typedef struct {
    int id;                            ///< ID for output (generated on write OR set on read)
    psArray *sources;
    int flags;
    float x;
    float y;
    float flux;				// max of peak->rawFlux for all matched sources
} pmPhotObj;

pmPhotObj *pmPhotObjAlloc(void);

bool pmPhotObjAddSource(pmPhotObj *object, pmSource *source);

int pmPhotObjSortByFlux (const void **a, const void **b);
int pmPhotObjSortByX (const void **a, const void **b);

/// @}
# endif /* PM_PHOT_OBJ_H */


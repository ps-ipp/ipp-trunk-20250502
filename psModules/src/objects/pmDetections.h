/* @file  pmDetections.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-01 00:00:17 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_DETECTIONS_H
# define PM_DETECTIONS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmDetections structure
 *
 * A strcture to carry the combined footprint and peak information
 *
 */
typedef struct {
  psArray *footprints;        // collection of footprints in the image
  psArray *oldFootprints;     // collection of footprints previously found
  psArray *peaks;             // collection of all peaks contained by the footprints
  psArray *oldPeaks;          // collection of all peaks previously found
  psArray *newSources;        // collection of sources
  psArray *allSources;        // collection of sources
  int last;
} pmDetections;

pmDetections *pmDetectionsAlloc (void);

/// @}
# endif /* PM_DETECTIONS_H */

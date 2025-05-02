/* @file  pmAstrometryUtils.h
 * @brief utility functions for transform and distort functions
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-12-19 18:57:05 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROMETRY_UTILS_H
#define PM_ASTROMETRY_UTILS_H

/// @addtogroup Astrometry
/// @{

psPlane *psPlaneTransformGetCenter (psPlaneTransform *trans, double tol);
psPlaneTransform *psPlaneTransformSetCenter (psPlaneTransform *output, psPlaneTransform *input, double Xo, double Yo);
psPlaneTransform *psPlaneTransformRotate (psPlaneTransform *output, psPlaneTransform *input, double theta);

psPlaneTransform *psPlaneTransformIdentity (int order);
psPlaneDistort *psPlaneDistortIdentity (int order);

bool psPlaneTransformIsDiagonal (psPlaneTransform *transform);
bool psPlaneDistortIsDiagonal (psPlaneDistort *distort);

int pmAstrometryGetExtraOrders (void);
int pmAstrometrySetExtraOrders (int orders);

/// @}
#endif

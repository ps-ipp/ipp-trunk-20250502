/* @file  pmAstrometryDistortion.h
 * @brief This file defines the basic types for measuring and fitting the focal-plane distortion.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-18 22:07:17 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROMETRY_DISTORTION_H
#define PM_ASTROMETRY_DISTORTION_H

/// @addtogroup Astrometry
/// @{

/* The following data structure carries the information about the residual
 * gradient of source positions in the tangent plane (pmAstromObj.TP) as a
 * function of position in the focal plane (pmAstromObj.FP).
 */
typedef struct
{
    psPlane FP;
    psPlane dTPdL;
    psPlane dTPdM;
}
pmAstromGradient;

pmAstromGradient *pmAstromGradientAlloc (void);

/* The following function determines the position residual, in the tangent
 * plane, as a function of position in the focal plane, for a collection of raw
 * measurements and matched reference stars. The configuration data must include
 * the bin size over which the gradient is measured (keyword: ASTROM.GRAD.BOX).
 * The function returns an array of pmAstromGradient structures, defined below.
 */
psArray *pmAstromMeasureGradients(
    psArray *gradients,
    psArray *rawstars,
    psArray *refstars,
    psArray *matches,
    psRegion *region,
    int Nx, int Ny
);

/* The gradient set measured above can be fitted with a pair of 2D
 * polynomials. The resulting fits can then be related back to the implied
 * polynomials which represent the distortion. The following function performs
 * the fit and applies the result to the distortion transformation of the
 * supplied pmFPA structure. The configuration variable supplies the polynomial
 * order (keyword: ASTROM.DISTORT.ORDER).
 */
bool pmAstromFitDistortion(
    pmFPA *fpa,
    psArray *gradients,
    double pixelScale);

/// @}
#endif // PM_ASTROMETRY_DISTORTION_H

/* @file  psEllipse.h
 * @brief functions to manipulate sparse matrices equations
 *
 * $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * $Date: 2008-10-08 21:51:34 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_ELLIPSE_H
#define PS_ELLIPSE_H

#include "psError.h"

/// @addtogroup MathOps Mathematical Operations
/// @{

// Different representations of an ellipse

/// Ellipse defined in terms of axes
typedef struct {
    double major;                       ///< Major axis
    double minor;                       ///< Minor axis
    double theta;                       ///< Position angle
} psEllipseAxes;

/// Ellipse defined in terms of moments
typedef struct {
    double x2;                          ///< Moment of xx (Mxx)
    double y2;                          ///< Moment of yy (Myy)
    double xy;                          ///< Moment of xy (Mxy)
} psEllipseMoments;

/// Ellipse defined in terms of Gaussian shape parameters
typedef struct {
    double sx;                          ///< Shape parameter in x
    double sy;                          ///< Shape parameter in y
    double sxy;                         ///< Shape parameter in xy
} psEllipseShape;

/// Ellipse defined in terms of polarizations
typedef struct {
    double e0;                          ///< Scale (Mxx + Myy)
    double e1;                          ///< Polarization 1 (Mxx - Myy)
    double e2;                          ///< Polarization 1 (2Mxy)
} psEllipsePol;

// Conversions between elliptical shape representations

/// Convert axes to moments representation
psEllipseMoments psEllipseAxesToMoments(psEllipseAxes axes ///< Axes of ellipse
                                        );

/// Convert moments to axes representation
psEllipseAxes psEllipseMomentsToAxes(psEllipseMoments moments, ///< Moments of ellipse
                                     double maxAR ///< Maximum allowed axis ratio
                                     );

/// Convert axes to shape representation
psEllipseShape psEllipseAxesToShape(psEllipseAxes axes ///< Axes of ellipse
                                    );

/// Convert shape to axes representation
psEllipseAxes psEllipseShapeToAxes(psEllipseShape shape, ///< Shape of ellipse
                                   double maxAR ///< Maximum allowed axis ratio
                                   );

/// Convert shape to axes representation and compute errors
psEllipseAxes psEllipseShapeToAxesWithErrors(psEllipseShape shape, ///< Shape of ellipse
                                   psEllipseShape errors, ///<errors on shape params
                                   double maxAR, ///< Maximum allowed axis ratio
                                   psEllipseAxes *pShapeErrors ///< propagated errors on axes
                                   );

/// Convert axes to polarization representation
psEllipsePol psEllipseAxesToPol(psEllipseAxes axes ///< Axes of ellipse
                                );

/// Convert polarization to axes representation
psEllipseAxes psEllipsePolToAxes(const psEllipsePol pol, ///< Polarization of ellipse
				 const float minMinorAxis ///< Minimum allowed minor axis
    );

/// Convert shape to polarization representation
psEllipsePol psEllipseShapeToPol(psEllipseShape shape ///< Shape of ellipse
                                 );

/// Convert shape to polarization representation
psEllipseShape psEllipsePolToShape(psEllipsePol pol ///< Shape of ellipse
                                 );

/// @}
#endif

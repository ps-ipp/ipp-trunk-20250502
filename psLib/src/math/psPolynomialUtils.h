/* @file  psPolynomialUtils.h
 * @brief extra psPolynomial-related functions
 *
 * $Revision: 1.6 $ $Name: not supported by cvs2svn $
 * $Date: 2009-01-27 06:39:38 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_POLYNOMIAL_UTILS_H
#define PS_POLYNOMIAL_UTILS_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psVector.h"
#include "psImage.h"
#include "psCoord.h"
#include "psStats.h"
#include "psPolynomial.h"

// perform vector clip-fit based on significance of deviations
bool psVectorChiClipFitPolynomial4D(
    psPolynomial4D *poly,               // Polynomial to fit
    psStats *stats,                     // Statistics to use in clipping
    const psVector *mask,               // Mask for input values
    psVectorMaskType maskValue,		// Mask value
    const psVector *f,                  // Value of the function, f(x,y,z,t)
    const psVector *fErr,               // Error in the value
    const psVector *x,                  // x ordinate
    const psVector *y,                  // y ordinate
    const psVector *z,                  // z ordinate
    const psVector *t                   // t ordinate
);

// fit a 2D 2nd order polynomial to the 9 pixels centered on (x,y)
psPolynomial2D *psImageBicubeFit(const psImage *image, // Image to fit
                                 int x, int y // Pixels of centre of fit
                                );

// detemine the min(max) of the special 2D 2nd order polynomial
psPlane psImageBicubeMin(const psPolynomial2D *poly // The polynomial
                        );
psPolynomial2D *psPolynomial2D_dX (psPolynomial2D *out, psPolynomial2D *poly);
psPolynomial2D *psPolynomial2D_dY (psPolynomial2D *out, psPolynomial2D *poly);

/// @}
#endif /* PS_POLY_UTILS_H */

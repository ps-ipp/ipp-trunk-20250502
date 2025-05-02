/* @file pmNonLinear.h
 * @brief Perform non-linear correction through polynomial or table lookup
 *
 * @author George Gusciora, MHPCC
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2004 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_NON_LINEAR_H
#define PM_NON_LINEAR_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// Correct non-linearity through polynomial
///
/// Applies a polynomial to the flux of each pixel in the input image to determine the corrected flux.
pmReadout *pmNonLinearityPolynomial(pmReadout *in, ///< Input image, to correct
                                    const psPolynomial1D *coeff ///< Polynomial for non-linearity correction
                                   );

/// Correct non-linearity through table lookup
///
/// For each pixel in the input image, performs linear interpolation on the table (from the two vectors) to
/// determine the corrected flux.
pmReadout *pmNonLinearityLookup(pmReadout *in, ///< Input image, to correct
                                const psVector *inFlux, ///< Table column with input fluxes
                                const psVector *outFlux ///< Table column with output fluxes
                               );
bool pmNonLinearityApply(pmReadout *inputReadout, psArray *Ltab);
psF32 pmNonLinearityMeasure(psF32 flux, psVector *correction_fluxes, psVector *correction_factors);
/// @}
#endif

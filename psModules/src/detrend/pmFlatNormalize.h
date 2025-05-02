/* @file pmFlatNormalize.h
 * @brief Normalize flat-field measurements
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2004-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FLAT_NORMALIZE_H
#define PM_FLAT_NORMALIZE_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// Normalize flat-field measurements
///
/// We have f_ij = g_i s_j where f_ij is the flux recorded for chip i and integration j, g_i is the gain for
/// the i-th chip, s_j is the flux of the source in the j-th integration.  An initial guess for the chip gains
/// might be helpful, but is not necessary.  The matrix of background measurements contains the background for
/// the flat fields used in the combination, as a function of exposure (rows) and chip (columns).  The
/// exposure fluxes and chip gains are modified upon return with the solved values.  Returns true if the
/// solution converged.
bool pmFlatNormalize(psVector **expFluxesPtr, ///< Flux in each exposure, or NULL; modified
                     psVector **chipGainsPtr, ///< Initial guess of the chip gains or NULL; modified
                     const psImage *bgMatrix ///< Background measurements: rows are exposures, cols are chips
                    );

/// @}
#endif

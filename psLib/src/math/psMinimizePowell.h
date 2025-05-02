/* @file  psMinimizePowell.c
 * @brief basic minimization functions
 *
 * This file will contain function prototypes for various Powell
 * chi-squared minimization routines.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-23 22:47:23 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifndef PS_MINIMIZE_POWELL_H
#define PS_MINIMIZE_POWELL_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psMinimizeLMM.h"
#include "psVector.h"
#include "psMemory.h"
#include "psArray.h"
#include "psImage.h"
#include "psMatrix.h"
#include "psPolynomial.h"
#include "psSpline.h"
#include "psStats.h"
#include "psTrace.h"
#include "psError.h"
#include "psConstants.h"

/** Specifies the format of a user-defined function that the general Powell
 *  minimizer routine will accept.
 *
 *  @return float:   the single float value of the function given the parameters
 *      and coordinate vectors.
*/
typedef
float (*psMinimizePowellFunc)(
    const psVector *params,            ///< Parameters used to evaluate the function
    const psArray *coords              ///< Coordinates at which to evaluate
);

/** Minimizes a specified function based on the Powell method.
 *
 *  @return bool:   True if successful.
 */
bool psMinimizePowell(
    psMinimization *min,               ///< Minimization specification
    psVector *params,                  ///< "Best guess" for parameters that minimize func
    const psVector *paramMask,         ///< Parameters to be held fixed by minimizer
    const psArray *coords,             ///< Measurement coordinates
    psMinimizePowellFunc func          ///< Specified function
);

/** Specifies the format of a user-defined function that the general Powell chi-
 *  squared minimizer routine will accept.
 *
 *  @return psVector*:    Calculated values given the parameters and coordinates.
*/
typedef
psVector *(*psMinimizeChi2PowellFunc)(
    const psVector *params,            ///< Parameters used to evaluate the function
    const psArray *coords              ///< Coordinates at which to evaluate
);

/** Minimizes a specified function based on the Powell chi-squared method.
 *
 *  @return bool:   True is successful.
 */
bool psMinimizeChi2Powell(
    psMinimization *min,               ///< Minimization specification
    psVector *params,                  ///< "Best guess" for parameters that minimize func
    psMinConstraint *constraint,
    const psArray *coords,             ///< Measurement coordinates
    const psVector *value,             ///< Measured values at the coordinates
    const psVector *error,             ///< Errors in the measure values (or NULL)
    psMinimizeChi2PowellFunc model     ///< Specified function
);

/// @}
#endif // #ifndef PS_MINIMIZE_POWELL_H

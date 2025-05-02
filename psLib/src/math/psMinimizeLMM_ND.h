/* @file  psMinimizeLMM.h
 * @brief Levenberg-Marqardt minimization of N-D functions of N-D variables.  
 * @ingroup Math
 *
 *  Levenberg-Marqardt minimization of an N-dimensional function of N-diminsional independent
 *  variables.  This code is based on the 1-D function version of N-D variables in psMinimizeLMM.c 
 *
 *  @author EAM, IfA
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_MINIMIZE_LMM_ND_H
#define PS_MINIMIZE_LMM_ND_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/* Format of a user-defined function that the general Levenberg-Marquardt minimizer
 * routine will accept.
 *
 * @return bool: success / failure status.  the N-D function value is returned to the
 * pre-allocated vector 'value' and the derivatives of the parameters are returned to the
 * pre-allocated vector 'deriv', iff defined
 * 
 */
typedef bool (*psMinimizeLMNDChi2Func)(
    psVector *value,                   ///< values of the function
    psVector *deriv,                   ///< derivatives of the function
    const psVector *params,            ///< the parameters used to evaluate the function
    const psVector *x                  ///< positions for evaluation
    );

/*  Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psMinimization structure, false otherwise.
 */
bool psMemCheckMinimization(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Minimizes a specified function based on the Levenberg-Marquardt method.
 *
 *  @return bool:   True if successful.
 */
bool psMinimizeLMChi2(
    psMinimization *min,               ///< Minimization specification
    psImage *covar,                    ///< Covariance matrix
    psVector *params,                  ///< "Best Guess" for the parameters that minimize func
    psMinConstraint *constraint, ///< Constraints on the parameters
    const psArray *x,                  ///< Measurement ordinates of multiple vectors
    const psVector *y,                 ///< Measurement coordinates
    const psVector *yWt,               ///< Errors in the measurement coordinates
    psMinimizeLMChi2Func func          ///< Specified function
);

bool psMinimizeGaussNewtonDelta (
    psVector *delta,
    const psVector *params,
    const psVector *paramMask,
    const psArray  *x,
    const psVector *y,
    const psVector *yErr,
    psMinimizeLMChi2Func func
);

/** Function used to set parameters for generating "best guess" in minimizing Chi-Squared value.
 *
 *  @return psF32:    Chi-squared value for new guess
 */
psF32 psMinLM_SetABX (
    psImage  *alpha,                   ///< alpha guess
    psVector *beta,                    ///< beta guess
    const psVector *params,            ///< params guess
    const psVector *paramMask,         ///< param mask
    const psArray  *x,                 ///< Measurement ordinates
    const psVector *y,                 ///< Measurement coordinates
    const psVector *dy,                ///< Weights calculated from y-errors
    psMinimizeLMChi2Func func          ///< Specified function
);


bool psMinLM_GuessABP(
    psImage  *Alpha,
    psVector *Beta,
    psVector *Params,
    const psImage  *alpha,
    const psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    psMinimizeLMLimitFunc checkLimits,
    psF32 lambda,
    psF32 *dLinear
);

psF32 psMinLM_dLinear(
    const psVector *Beta,
    const psVector *beta,
    psF32 lambda);

// allocate alpha and beta for unmasked parameters only 
bool psMinLM_AllocAB (psImage **Alpha, psVector **Beta, const psVector *params, const psVector *paramMask);

/// @}
#endif // #ifndef PS_MINIMIZE_LMM_H

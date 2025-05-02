/* @file  psMinimizeLMM.c
 * @brief basic minimization functions
 *
 * This file will contain function prototypes for various Levenberg-Marquadt
 * minimization routines.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-09-28 00:35:20 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_MINIMIZE_LMM_H
#define PS_MINIMIZE_LMM_H

/// @addtogroup MathOps Mathematical Operations
/// @{

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

# define PS_MINIMIZE_LMM_GAIN_FACTOR_MODE 0
# define PS_MINIMIZE_LMM_CHISQ_CONVERGENCE 1

#define PS_DETERMINE_BRACKET_STEP_SIZE 0.10
#define PS_MAX_LMM_ITERATIONS 100
#define PS_MAX_MINIMIZE_ITERATIONS 100
#define P_PSMINIMIZATION_SET_MAXITER(m,val) { *(int*)&m->maxIter  = val; }
#define P_PSMINIMIZATION_SET_MIN_TOL(m,val) { *(float*)&m->minTol = val; }
#define P_PSMINIMIZATION_SET_MAX_TOL(m,val) { *(float*)&m->maxTol = val; }

                typedef enum {
                    PS_MINIMIZE_BETA_LIMIT,
                    PS_MINIMIZE_PARAM_MIN,
                    PS_MINIMIZE_PARAM_MAX
                } psMinConstraintMode;

/** Specifies the format of a user-defined function that the general Levenberg-
 *  Marquardt minimizer routine will accept.
 *
 *  @return float:   the single float value of the function given the parameters,
 *       positions, and derivatives.
 */
typedef
float (*psMinimizeLMChi2Func)(
    psVector *deriv,                   ///< derivatives of the function
    const psVector *params,            ///< the parameters used to evaluate the function
    const psVector *x                  ///< positions for evaluation
);

/** Specifies the format of a user-defined function which check the parameters
 *  against the allowed limits.  used by the general Levenberg-Marquardt minimizer.
 *
 *  @return float:   the single float value of the function given the parameters,
 *       positions, and derivatives.
 */
typedef
bool (*psMinimizeLMLimitFunc)(
    psMinConstraintMode mode,   ///< which limit to check
    int nParam,    ///< which param to check
    float *params,   ///< current param value set
    float *beta       ///< current beta value, if needed
);

/** A data structure for minimization routines.
 *
 *  Contains numerical analysis parameters/values
 */
typedef struct
{
    const int maxIter;			///< Convergence limit
    const float minTol;			///< Convergence Tolerance (stop if we reach this value)
    const float maxTol;			///< Max Tolerance (accept fit if last improvement was this good)
    float value;                       ///< Value of function at minimum
    int iter;                          ///< Number of iterations to date
    float lastDelta;                   ///< The last difference for the fit
    float rParSigma;		       ///< last fractional change in the parameters
    float maxChisqDOF;		       ///< for Chisq minimization, require that we reach here before checking tolerance
    int gainFactorMode;
    bool chisqConvergence;
    bool isInteractive;
    bool useReweighting;
}
psMinimization;

/** A data structure for minimization routines.
 *
 *
 */
typedef struct
{
    psVector *paramMask;                ///< valid / invalid parameters
    psMinimizeLMLimitFunc checkLimits; ///< user-supplied function to test the parameter limits
}
psMinConstraint;

psMinConstraint *psMinConstraintAlloc(void) PS_ATTR_MALLOC;

/** Allocates a psMinimization structure.
 *
 *  @return psMinimization* :   a new psMinimization struct
 */
psMinimization *psMinimizationAlloc(
    int maxIter,                       ///< Number of minimization iterations to perform.
    float minTol,		       ///< stop if tolerance is less than this
    float maxTol		       ///< accept fit if tolerance is less than this
) PS_ATTR_MALLOC;

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

/** Minimizes a specified function based on the Levenberg-Marquardt method.
 *
 *  @return bool:   True if successful.
 */
bool psMinimizeLMChi2_Old(
    psMinimization *min,               ///< Minimization specification
    psImage *covar,                    ///< Covariance matrix
    psVector *params,                  ///< "Best Guess" for the parameters that minimize func
    psMinConstraint *constraint, ///< Constraints on the parameters
    const psArray *x,                  ///< Measurement ordinates of multiple vectors
    const psVector *y,                 ///< Measurement coordinates
    const psVector *yWt,               ///< Errors in the measurement coordinates
    psMinimizeLMChi2Func func          ///< Specified function
);

/** Minimizes a specified function based on the Levenberg-Marquardt method
    Uses alternative convergence criterion.
 *
 *  @return bool:   True if successful.
 */
bool psMinimizeLMChi2_Alt(
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
    bool  reweight,                    ///< if true, we calculate the reweighting for each point based on distance from model
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

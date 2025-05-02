/** @file  psMinimize.c
 *  \brief basic minimization functions
 *  @ingroup Math
 *
 *  This file will contain functions to minimize an arbitrary function at
 *  a data point, fit an arbitrary function to a set of data points, and
 *  fit a 1-D polynomial to a set of data points.
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.36 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "psAbort.h"
#include "psAssert.h"
#include "psMinimizeLMM.h"
#include "psImage.h"
#include "psImageStructManip.h"
#include "psLogMsg.h"
/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* GLOBAL VARIABLES                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* FILE STATIC VARIABLES                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - LOCAL                                           */
/*****************************************************************************/

// Alpha & Beta only represent unmasked values
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
    psF32 *dLinear)
{
    PS_ASSERT_VECTOR_TYPE(Alpha,     PS_TYPE_F32,  false);
    PS_ASSERT_VECTOR_TYPE(Beta,      PS_TYPE_F32,  false);
    PS_ASSERT_VECTOR_TYPE(Params,    PS_TYPE_F32,  false);
    PS_ASSERT_VECTOR_TYPE(alpha,     PS_TYPE_F32,  false);
    PS_ASSERT_VECTOR_TYPE(beta,      PS_TYPE_F32,  false);
    PS_ASSERT_VECTOR_TYPE(params,    PS_TYPE_F32,  false);
    if (paramMask) {
        PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, false);
    }

    assert (alpha->numCols == beta->n);
    assert (alpha->numCols == alpha->numRows);

# define TESTGJ 0
# if (TESTGJ)
    lambda = 0.0;
#endif

    // set new guess values, applying (1+lambda) scaling to pivots
    Beta = psVectorCopy(Beta, beta, PS_TYPE_F32);
    Alpha = psImageCopy(Alpha, alpha, PS_TYPE_F32);
    for (int j = 0; j < Alpha->numCols; j++) {
        Alpha->data.F32[j][j] = alpha->data.F32[j][j] * (1.0 + lambda);
    }

    // error and clear above if kept?
    if (!psMatrixGJSolve(Alpha, Beta)) {
        psTrace ("psLib.math", 4, "singular matrix in Guess ABP\n");
        return(false);
    }

// XXX check that the GJ solver works:
# if (TESTGJ)
    psImage *out = psImageAlloc (alpha->numRows, alpha->numCols, PS_TYPE_F32);
    for (int oy = 0; oy < out->numRows; oy++) {
        for (int ox = 0; ox < out->numCols; ox++) {
            float value = 0;
            for (int i = 0; i < alpha->numCols; i++) {
                value += alpha->data.F32[i][ox]*Alpha->data.F32[oy][i];
            }
            out->data.F32[oy][ox] = value;
        }
    }

    psVector *vect = psVectorAlloc (beta->n, PS_TYPE_F32);
    for (int oy = 0; oy < vect->n; oy++) {
        float value = 0;
        for (int i = 0; i < alpha->numCols; i++) {
            value += alpha->data.F32[oy][i]*Beta->data.F32[i];
        }
        vect->data.F32[oy] = value;
    }

    psFree (out);
    psFree (vect);

# endif

    // check for non-finite entries in result
    for (int i = 0; i < Beta->n; i++) {
        if (!isfinite(Beta->data.F32[i])) {
            // psError(PS_ERR_BAD_PARAMETER_VALUE, 3, "Fit value diverges: vector[%d] is %.2f\n", i, Beta->data.F32[i]);
            return false;
        }
    }

    // measure linear model prediction
    // (we must do this before truncating Beta below)
    if (dLinear) {
        *dLinear = psMinLM_dLinear(Beta, beta, lambda);
    }

    // full-length Beta for checkLimits functions
    psVector *tmpBeta = psVectorAlloc(params->n, PS_TYPE_F32);
    psVectorInit (tmpBeta, 0.0);

    // set tmpBeta values which are not masked
    for (int j = 0, n = 0; j < params->n; j++) {
        if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) continue;
        tmpBeta->data.F32[j] = Beta->data.F32[n];
        n++;
    }

    // apply Beta to get new Params values
    for (int j = 0; j < params->n; j++) {
        if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) {
            Params->data.F32[j] = params->data.F32[j];
            continue;
        }
        // apply beta limits
        if (checkLimits) {
            checkLimits (PS_MINIMIZE_BETA_LIMIT, j, Params->data.F32, tmpBeta->data.F32);
        }

        Params->data.F32[j] = params->data.F32[j] - tmpBeta->data.F32[j];

        // compare new params to param limits
        if (checkLimits) {
            checkLimits (PS_MINIMIZE_PARAM_MIN,  j, Params->data.F32, tmpBeta->data.F32);
            checkLimits (PS_MINIMIZE_PARAM_MAX,  j, Params->data.F32, tmpBeta->data.F32);
        }
    }

    // apply tmpBeta after limits have been checked
    for (int j = 0, n = 0; j < params->n; j++) {
        if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) continue;
        Beta->data.F32[n] = tmpBeta->data.F32[j];
        n++;
    }

    psFree (tmpBeta);
    return(true);
}

bool psMinimizeGaussNewtonDelta(
    psVector *delta,
    const psVector *params,
    const psVector *paramMask,
    const psArray  *x,
    const psVector *y,
    const psVector *yWt,
    psMinimizeLMChi2Func func)
{
    psTrace("psLib.math", 3, "---- begin ----\n");

    // allocate internal arrays (current vs Guess)
    psImage *Alpha = NULL;
    psVector *Beta = NULL;

    psVectorInit (delta, 0.0);

    // Alpha & Beta only contain elements to represent the unmasked parameters
    // if none are available, return false
    if (!psMinLM_AllocAB (&Alpha, &Beta, params, paramMask)) {
        return false;
    }

    psImage *alpha   = psImageAlloc(Alpha->numCols, Alpha->numRows, PS_TYPE_F32);
    psVector *Params = psVectorAlloc(params->n, PS_TYPE_F32);

    psVector *dy     = NULL;
    bool retValue = true;

    // the user provides the error or NULL.  we need to convert
    // to appropriate weights
    if (yWt != NULL) {
        dy = (psVector *) yWt;
    } else {
        dy = psVectorAlloc(y->n, PS_TYPE_F32);
        psVectorInit(dy, 1.0);
    }

    // XXX should we give up if chisq is nan?
    // do not apply reweighting
    psF32 chisq = psMinLM_SetABX(alpha, Beta, params, paramMask, x, y, dy, false, func);
    if (isnan(chisq)) {
        psTrace ("psLib.math", 5, "psMinLM_SetABX() returned a NAN chisq.\n");
        psVectorInit (delta, NAN);
        retValue = false;
    }

    psTrace("psLib.math", 5, "psMinLM_SetABX() was succesful\n");
    // dump some useful info if trace is defined
    if (psTraceGetLevel("psLib.math") >= 6) {
        p_psImagePrint(psTraceGetDestination(), alpha, "alpha guess (0)");
        p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (0)");
        p_psVectorPrint(psTraceGetDestination(), params, "params guess (0)");
    }

    bool status = psMinLM_GuessABP(Alpha, delta, Params, alpha, Beta, params, paramMask, NULL, 0.0, NULL);
    if (!status) {
        psTrace ("psLib.math", 5, "psMinLM_GuessABP() returned FALSE.\n");
        psVectorInit (delta, NAN);
        retValue = false;
    }
    psTrace("psLib.math", 5, "psMinLM_GuessABP() was succesful\n");
    if (psTraceGetLevel("psLib.math") >= 6) {
        p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (1)");
        p_psVectorPrint(psTraceGetDestination(), delta, "delta guess (1)");
        p_psVectorPrint(psTraceGetDestination(), Params, "params guess (1)");
    }

    psFree(alpha);
    psFree(Alpha);
    psFree(Beta);
    psFree(Params);
    if (yWt == NULL) {
        psFree(dy);
    }
    psTrace("psLib.math", 3, "---- end ----\n");
    return(retValue);
}

// measure linear model prediction
psF32 psMinLM_dLinear(
    const psVector *Beta,
    const psVector *beta,
    psF32 lambda)
{

    /* get linear model prediction */
    psF32 dLinear = 0;
    psF32 *B = Beta->data.F32;
    psF32 *b = beta->data.F32;

    float dh = 0.0, sh = 0.0, Sh = 0.0;

    // beta only counts unmasked parameters
    for (int i = 0; i < beta->n; i++) {
        dh = lambda*B[i] + b[i];
        sh = 0.5*B[i]*dh;
        Sh += sh;
        dLinear += lambda*PS_SQR(B[i]) + B[i]*b[i];
    }
    return(0.5*dLinear);
}

// NOTE : reweight is true, the implementation below calculates the alpha,beta terms
// including a modified weight, but the chi-square value is calculated usig standard
// weights.  

// alpha, beta, params are already allocated
psF32 psMinLM_SetABX(
    psImage  *alpha,
    psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    const psArray  *x,
    const psVector *y,
    const psVector *dy,
    bool  reweight, // if true, we calculate the reweighting for each point based on distance from model
    psMinimizeLMChi2Func func)
{
    PS_ASSERT_IMAGE_NON_NULL(alpha, NAN);
    PS_ASSERT_VECTOR_NON_NULL(beta, NAN);
    PS_ASSERT_VECTOR_NON_NULL(params, NAN);
    PS_ASSERT_PTR_NON_NULL(x, NAN);
    PS_ASSERT_VECTOR_NON_NULL(y, NAN);
    PS_ASSERT_VECTOR_NON_NULL(dy, NAN);

    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, NAN);
    if (paramMask) {
        PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, NAN);
    }

    psVector *deriv = psVectorAlloc(params->n, PS_TYPE_F32);

    // zero alpha, beta, and chisq for summing below
    psImageInit (alpha, 0.0);
    psVectorInit (beta, 0.0);
    psF32 chisq = 0.0;

    // calculate chisq, alpha, beta. alpha & beta only represent unmasked parameters; skip
    // masked ones
    for (psS32 i = 0; i < y->n; i++) {
        psF32 ymodel = func(deriv, params, (psVector *) x->data[i]);
        psF32 delta  = ymodel - y->data.F32[i];
        psF32 dChi2  = PS_SQR(delta) * dy->data.F32[i];
	psF32 wtmod  = reweight ? 1.0 / (1.0 + 5.69*dChi2) : 1.0; // see fit1d_irls.c:weight_cauchy
        chisq += dChi2;

	// if we are doing an IRLS-style fit, we downweight points based on the dChisq
	// value above: this means setting an additional weight term

	// XXX remove this later:
	// psVector *tmp = x->data[i];
	// fprintf (stderr, "%f %f  %f %f  %f\n", tmp->data.F32[0], tmp->data.F32[1], y->data.F32[i], dy->data.F32[i], ymodel);

        if (isnan(dy->data.F32[i])) goto escape;
        if (isnan(delta)) goto escape;
        if (isnan(chisq)) goto escape;

        // we track alpha,beta and params,deriv separately
        for (int j = 0, J = 0; j < params->n; j++) {
            if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) continue;

            psF32 weight = deriv->data.F32[j] * dy->data.F32[i] * wtmod;

            for (int k = 0, K = 0; k <= j; k++) {
                if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[k])) continue;
                alpha->data.F32[J][K] += weight * deriv->data.F32[k];
                K++;
            }
            beta->data.F32[J] += weight * delta;
            J++;
        }
    }

    // calculate lower-left half of alpha
    for (int j = 1; j < alpha->numCols; j++) {
        for (int k = 0; k < j; k++) {
            alpha->data.F32[k][j] = alpha->data.F32[j][k];
        }
    }

    psFree(deriv);
    return(chisq);

escape:
    psFree(deriv);
    return NAN;
}

/******************************************************************************
psMinimizeLMChi2():  wrapper to call either _Old or _Alt
  *****************************************************************************/
bool psMinimizeLMChi2(
    psMinimization *min,
    psImage *covar,
    psVector *params,
    psMinConstraint *constraint,
    const psArray *x,
    const psVector *y,
    const psVector *yWt,
    psMinimizeLMChi2Func func)
{
    bool status = psMinimizeLMChi2_Alt(
	min,
	covar,
	params,
	constraint,
	x,
	y,
	yWt,
	func);
    return status;
}

/******************************************************************************
psMinimizeLMChi2():  This routine will take an procedure which calculates an
arbitrary function and it's derivative and minimize the chi-squared match
between that function at the specified coords and the specified value at those
coords.

This requires F32 input data; all internal calls use F32.
XXX Make an F64 version?
  *****************************************************************************/
bool psMinimizeLMChi2_Old(
    psMinimization *min,
    psImage *covar,
    psVector *params,
    psMinConstraint *constraint,
    const psArray *x,
    const psVector *y,
    const psVector *yWt,
    psMinimizeLMChi2Func func)
{
    psTrace("psLib.math", 3, "---- begin ----\n");
    PS_ASSERT_PTR_NON_NULL(min, false);
    PS_ASSERT_VECTOR_NON_NULL(params, false);
    PS_ASSERT_VECTOR_NON_EMPTY(params, false);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    psVector *paramMask = NULL;
    if (constraint != NULL) {
        paramMask = constraint->paramMask;
        if (paramMask != NULL) {
            PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, false);
            PS_ASSERT_VECTORS_SIZE_EQUAL(params, paramMask, false);
        }
    }
    PS_ASSERT_PTR_NON_NULL(x, false);
    for (psS32 i = 0 ; i < x->n ; i++) {
        psVector *coord = (psVector *) (x->data[i]);
        PS_ASSERT_VECTOR_NON_NULL(coord, false);
        PS_ASSERT_VECTOR_TYPE(coord, PS_TYPE_F32, false);
    }
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_NON_EMPTY(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, false);
    if (yWt != NULL) {
        PS_ASSERT_VECTOR_TYPE(yWt, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, yWt, false);
    }
    PS_ASSERT_PTR_NON_NULL(func, false);

    psMinimizeLMLimitFunc checkLimits = NULL;
    if (constraint) {
        checkLimits = constraint->checkLimits;
    }

    // this function has test and current values for several things
    // the current best value is in lower case
    // the next guess value is in upper case

    // allocate internal arrays (current vs Guess)
    psImage *Alpha = NULL;
    psVector *Beta = NULL;

    // Alpha & Beta only contain elements to represent the unmasked parameters
    if (!psMinLM_AllocAB (&Alpha, &Beta, params, paramMask)) {
        psAbort ("programming error: no unmasked parameters to be fit\n");
    }

    psImage *alpha   = psImageAlloc(Alpha->numCols, Alpha->numRows, PS_TYPE_F32);
    psVector *beta   = psVectorAlloc(Beta->n, PS_TYPE_F32);
    psVector *Params = psVectorAlloc(params->n, PS_TYPE_F32);

    psVector *dy     = NULL;
    psF32 Chisq = 0.0;
    psF32 lambda = 0.001;
    psF32 dLinear = 0.0;

    // the user provides the error or NULL.  we need to convert
    // to appropriate weights
    if (yWt != NULL) {
        dy = (psVector *) yWt;
    } else {
        dy = psVectorAlloc(y->n, PS_TYPE_F32);
        psVectorInit(dy, 1.0);
    }

    // number of degrees of freedom for this fit
    int nDOF = dy->n - params->n;

    // calculate initial alpha and beta, set chisq (min->value)
    // do not apply IRLS reweighting yet, even if requested
    min->value = psMinLM_SetABX(alpha, beta, params, paramMask, x, y, dy, false, func);
    if (isnan(min->value)) {
        min->iter = min->maxIter;
	psFree(alpha);
	psFree(Alpha);
	psFree(beta);
	psFree(Beta);
	psFree(Params);
        return(false);
    }
    // dump some useful info if trace is defined
    if (psTraceGetLevel("psLib.math") >= 6) {
        p_psImagePrint(psTraceGetDestination(), alpha, "alpha guess (0)");
        p_psVectorPrint(psTraceGetDestination(), beta, "beta guess (0)");
    }
    if (psTraceGetLevel("psLib.math") >= 5) {
        p_psVectorPrint(psTraceGetDestination(), params, "params guess (0)");
    }

    // iterate until: (a) nIter = min->iter or (b) (chisq / ndof) < maxChisq and deltaChisq < minTol (but don't stop unless Chisq is finite)
    bool done = (min->iter >= min->maxIter);
    while (!done) {
        psTrace("psLib.math", 5, "Iteration number %d.  (max iterations is %d).\n", min->iter, min->maxIter);
        psTrace("psLib.math", 5, "Last delta is %f.  stop if < %f, accept if < %f\n", min->lastDelta, min->minTol, min->maxTol);
        psTrace("psLib.math.dLinear", 5, "Iteration number %d.  (max iterations is %d).\n", min->iter, min->maxIter);
        psTrace("psLib.math.dLinear", 5, "Last delta is %f.  stop if < %f, accept if < %f\n", min->lastDelta, min->minTol, min->maxTol);

        // set a new guess for Alpha, Beta, Params
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, checkLimits, lambda, &dLinear)) {
            min->iter ++;
	    if (min->iter >=  min->maxIter) break;
            lambda *= 10.0;
            continue;
        }

        // dump some useful info if trace is defined
        if (psTraceGetLevel("psLib.math") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "Alpha guess (1)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "Beta guess (1)");
            p_psVectorPrint(psTraceGetDestination(), beta, "beta current (1)");
        }
        if (psTraceGetLevel("psLib.math") >= 5) {
            p_psVectorPrint(psTraceGetDestination(), Params, "params guess (1)");
        }
	if (psTraceGetLevel("psLib.math.dLinear") >= 6) {
	  p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (1)");
	  p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (1)");
	  p_psVectorPrint(psTraceGetDestination(), Params, "params guess (1)");
	  p_psVectorPrint(psTraceGetDestination(), params, "params guess (1)");
	}	  

        // calculate Chisq for new guess, update Alpha & Beta
        Chisq = psMinLM_SetABX(Alpha, Beta, Params, paramMask, x, y, dy, min->useReweighting, func);
        if (isnan(Chisq)) {
            min->iter ++;
	    if (min->iter >= min->maxIter) break;
            lambda *= 10.0;
            continue;
        }

        // convergence criterion:
        // compare the delta (min->value - Chisq) with the
        // expected delta from the linear model (dLinear)
        // accept new guess if it is an improvement (rho > 0), or else increase lambda
        psF32 rho = (min->value - Chisq) / dLinear;

        psTrace("psLib.math", 5, "last chisq: %f, new chisq %f, delta: %f, dLinear: %f, rho: %f, lambda: %g, nDOF: %d\n", min->value, Chisq, min->lastDelta, dLinear, rho, lambda, nDOF);

        psTrace("psLib.math.dLinear", 5, "last chisq: %f, new chisq %f, delta: %f, dLinear: %f, rho: %f, lambda: %g\n", min->value, Chisq, min->lastDelta, dLinear, rho, lambda);
	if (psTraceGetLevel("psLib.math.dLinear") >= 6) {
	  p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (2)");
	  p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (2)");
	  p_psVectorPrint(psTraceGetDestination(), Params, "params guess (2)");
	}	  
        // dump some useful info if trace is defined
        if (psTraceGetLevel("psLib.math") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (2)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (2)");
        }

        /* rho is positive if the new chisq is smaller; allow for some insignificant change (slight negative rho) */
        if (rho >= -1e-6) {
            min->lastDelta = (min->value - Chisq) / nDOF;
            min->value = Chisq;
            alpha  = psImageCopy(alpha, Alpha, PS_TYPE_F32);
            beta   = psVectorCopy(beta, Beta, PS_TYPE_F32);
            params = psVectorCopy(params, Params, PS_TYPE_F32);
            lambda *= 0.25;
        } else {
            lambda *= 10.0;
        }
        min->iter++;

	done = (min->iter >= min->maxIter);
	
	// check for convergence:
	float chisqDOF = Chisq / nDOF;
	if (!isfinite(min->maxChisqDOF) || ((chisqDOF < min->maxChisqDOF) && isfinite(min->lastDelta))) {
	    done |= (min->lastDelta < min->minTol);
	}
    }
    psTrace("psLib.math", 5, "chisq: %f, last delta: %f, Niter: %d\n", min->value, min->lastDelta, min->iter);

    // construct & return the covariance matrix (if requested)
    if (covar != NULL) {
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, NULL, 0.0, NULL)) {
            psTrace ("psLib.math", 5, "failure to calculate covariance matrix\n");
        }
        // set covar values which are not masked
        psImageInit (covar, 0.0);
        for (int j = 0, J = 0; j < params->n; j++) {
            if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) {
                covar->data.F32[j][j] = 1.0;
                continue;
            }
            for (int k = 0, K = 0; k < params->n; k++) {
                if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[k])) continue;
                covar->data.F32[j][k] = Alpha->data.F32[J][K];
                K++;
            }
            J++;
        }
    }

    // free the internal temporary data
    psFree(alpha);
    psFree(Alpha);
    psFree(beta);
    psFree(Beta);
    psFree(Params);
    if (yWt == NULL) {
        psFree(dy);
    }

    // if the last improvement was at least as good as maxTol, accept the fit:
    if (min->lastDelta <= min->maxTol) {
	psTrace("psLib.math", 6, "---- end (true) ----\n");
        return(true);
    }
    psTrace("psLib.math", 6, "---- end (false) ----\n");
    return(false);
}

/******************************************************************************
psMinimizeLMChi2(): This routine takes a function-pointer (func) which calculates an arbitrary
function and it's derivatives and minimizes the chi-squared match between that function at the
specified points and the specified value at those points.

The original version of this function used a convergence criterion based on the change in
chisq.  this has problems since it depends on the choice of points used to measure the fit.
(consider a gaussian on a background : it 100 pixels are used -- and some or most contribute to
the chisq -- and the delta-chisq is 10%, then the same change in model fit will yield a
delta-chisq of 1% if 1000 pixels are used (all but the 100 measuring the background)).

This implementation uses changes to the parameters and stops if they are no longer significant.

This requires F32 input data; all internal calls use F32.
  *****************************************************************************/
bool psMinimizeLMChi2_Alt(
    psMinimization *min,
    psImage *covar,
    psVector *params,
    psMinConstraint *constraint,
    const psArray *x,
    const psVector *y,
    const psVector *yWt,
    psMinimizeLMChi2Func func)
{
    psTrace("psLib.math", 3, "---- begin ----\n");
    PS_ASSERT_PTR_NON_NULL(min, false);
    PS_ASSERT_VECTOR_NON_NULL(params, false);
    PS_ASSERT_VECTOR_NON_EMPTY(params, false);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    psVector *paramMask = NULL;
    if (constraint != NULL) {
        paramMask = constraint->paramMask;
        if (paramMask != NULL) {
            PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, false);
            PS_ASSERT_VECTORS_SIZE_EQUAL(params, paramMask, false);
        }
    }
    PS_ASSERT_PTR_NON_NULL(x, false);
    for (psS32 i = 0 ; i < x->n ; i++) {
        psVector *coord = (psVector *) (x->data[i]);
        PS_ASSERT_VECTOR_NON_NULL(coord, false);
        PS_ASSERT_VECTOR_TYPE(coord, PS_TYPE_F32, false);
    }
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_NON_EMPTY(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, false);
    if (yWt != NULL) {
        PS_ASSERT_VECTOR_TYPE(yWt, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(y, yWt, false);
    }
    PS_ASSERT_PTR_NON_NULL(func, false);

    psMinimizeLMLimitFunc checkLimits = NULL;
    if (constraint) {
        checkLimits = constraint->checkLimits;
    }

    // this function has test and current values for several things
    // the current best value is in lower case
    // the next guess value is in upper case

    // allocate internal arrays (current vs Guess)
    psImage *Alpha = NULL;
    psVector *Beta = NULL;

    // Alpha & Beta only contain elements to represent the unmasked parameters
    if (!psMinLM_AllocAB (&Alpha, &Beta, params, paramMask)) {
        psAbort ("programming error: no unmasked parameters to be fit\n");
    }

    int nFitParams = Beta->n;
    psImage *alpha   = psImageAlloc(nFitParams, nFitParams, PS_TYPE_F32);
    psVector *beta   = psVectorAlloc(nFitParams, PS_TYPE_F32);
    psVector *Params = psVectorAlloc(params->n, PS_TYPE_F32);

    psVector *dy     = NULL;
    psF32 Chisq = 0.0;
    psF32 lambda = 0.001;
    psF32 dLinear = 0.0;
    psF32 nu = 2.0;

    // the user provides the error or NULL.  we need to convert
    // to appropriate weights
    if (yWt != NULL) {
        dy = (psVector *) yWt;
    } else {
        dy = psVectorAlloc(y->n, PS_TYPE_F32);
        psVectorInit(dy, 1.0);
    }

    // number of degrees of freedom for this fit
    int nDOF = dy->n - nFitParams;

    // calculate initial alpha and beta, set chisq (min->value)
    // do not apply IRLS reweighting yet, even if requested
    min->value = psMinLM_SetABX(alpha, beta, params, paramMask, x, y, dy, false, func);
    if (isnan(min->value)) {
        min->iter = min->maxIter;
	psFree(alpha);
	psFree(Alpha);
	psFree(beta);
	psFree(Beta);
	psFree(Params);
        return(false);
    }
    // dump some useful info if trace is defined
    if (psTraceGetLevel("psLib.math") >= 6) {
        p_psImagePrint(psTraceGetDestination(), alpha, "alpha guess (0)");
        p_psVectorPrint(psTraceGetDestination(), beta, "beta guess (0)");
    }
    if (psTraceGetLevel("psLib.math") >= 5) {
        p_psVectorPrint(psTraceGetDestination(), params, "params guess (0)");
    }

    bool done = (min->iter >= min->maxIter);
    while (!done) {
        psTrace("psLib.math", 5, "Iteration number %d.  (max iterations is %d).\n", min->iter, min->maxIter);

	if (min->chisqConvergence) {
	  psTrace("psLib.math", 5, "Last delta is %f.  stop if < %f, accept if < %f\n", min->lastDelta, min->minTol, min->maxTol);
	} else {
	  psTrace("psLib.math", 5, "Last delta is %f.  stop if < %f, accept if < %f\n", min->rParSigma, min->minTol*nFitParams, min->maxTol*nFitParams);
	}

        // set a new guess for Alpha, Beta, Params
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, checkLimits, lambda, &dLinear)) {
            min->iter ++;
	    if (min->iter >=  min->maxIter) break;
            lambda *= 10.0;
            // ALT? lambda *= 2.0;
            continue;
        }

        // dump some useful info if trace is defined
        if (psTraceGetLevel("psLib.math") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "Alpha guess (1)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "Beta guess (1)");
            p_psVectorPrint(psTraceGetDestination(), beta, "beta current (1)");
        }
        if (psTraceGetLevel("psLib.math") >= 5) {
            p_psVectorPrint(psTraceGetDestination(), Params, "params guess (1)");
        }

	// calculate the parameter change (rParDelta) and error radius (rParSigma)
	//    rParDelta : radius of parameter change;
	//    rParSigma : radius of parameter error 
	
	// note that (before SetABX) Alpha[i][i] is the covariance matrix and
	// Beta is the actual parameter change for this pass

	// note that Alpha & Beta only represent unmasked parameters, while params and Params have all 

	// dParSigma = Alpha[i][i] : error (squared) on parameter i
	// dParDelta = Params->data.F32[i] - params->data.F32[i]     : change on parameter i
	float rParSigma = 0.0;
        for (int j = 0, J = 0; j < Params->n; j++) {
	    if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) {
		continue;
	    }
	    rParSigma += PS_SQR(Params->data.F32[j] - params->data.F32[j]) / Alpha->data.F32[J][J];
	    J++;
	}
	rParSigma = sqrt(rParSigma);
	psTrace("psLib.math", 5, "rParSigma: %f, Niter: %d\n", rParSigma, min->iter);
	// fprintf (stderr, "rParSigma: %f, Niter: %d\n", rParSigma, min->iter);

        // calculate Chisq for new guess, update Alpha & Beta
	// if requested, modify the weights IRLS-style
        Chisq = psMinLM_SetABX(Alpha, Beta, Params, paramMask, x, y, dy, min->useReweighting, func);
        if (isnan(Chisq)) {
            min->iter ++;
	    if (min->iter >= min->maxIter) break;
            lambda *= 10.0;
            // ALT lambda *= 2.0;
            continue;
        }

        // convergence criterion:
        // compare the delta (min->value - Chisq) with the
        // expected delta from the linear model (dLinear)
        // accept new guess if it is an improvement (rho > 0), or else increase lambda
        psF32 rho = (min->value - Chisq) / dLinear;

        psTrace("psLib.math", 5, "last chisq: %f, new chisq %f, delta: %f, dLinear: %f, rho: %f, lambda: %g, nDOF: %d\n", min->value, Chisq, min->lastDelta, dLinear, rho, lambda, nDOF);

        psTrace("psLib.math.dLinear", 5, "last chisq: %f, new chisq %f, delta: %f, dLinear: %f, rho: %f, lambda: %g\n", min->value, Chisq, min->lastDelta, dLinear, rho, lambda);

        // dump some useful info if trace is defined
        if (psTraceGetLevel("psLib.math") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (2)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (2)");
        }

	// change in chisq/nDOF since last minimum
	min->lastDelta = (min->value - Chisq) / nDOF;

        // rho is positive if the new chisq is smaller; allow for some insignificant change (slight negative rho)

	// XXX the old version of lambda changes:
	// XXX : Madsen gives suggestion for better use of rho
        // rho is positive if the new chisq is smaller
        if (rho >= -1e-6) {
            min->value = Chisq;
            alpha  = psImageCopy(alpha, Alpha, PS_TYPE_F32);
            beta   = psVectorCopy(beta, Beta, PS_TYPE_F32);
            params = psVectorCopy(params, Params, PS_TYPE_F32);
        } 
	switch (min->gainFactorMode) {
	  case 0:
	    if (rho >= -1e-6) {
	      lambda *= 0.25;
	    } else {
	      lambda *= 10.0;
	    }
	    break;

	  case 1:
	    // adjust the gain ratio (lambda) based on rho
	    if (rho < 0.25) {
	      lambda *= 2.0;
	    } 
	    if (rho > 0.75) {
	      lambda *= 0.333;
	    }
	    break;

	  case 2:
	    if (rho > 0.0) {
	      lambda *= PS_MAX(0.33, (1.0 - pow(2.0*rho - 1.0, 3.0)));
	      nu = 2.0;
	    } else {
	      lambda *= nu;
	      nu *= 2.0;
	    }
	    break;
	}
        min->iter++;

	// ending conditions:
	// 1) hard limit : too many iterations
	done = (min->iter >= min->maxIter);
	
	// 2) require deltaChi > 1e-6 (ie, chisq is decreasing, but accept an insignificant change)
	if (min->lastDelta < -1e-6) {
	    continue;
	}

	// save this value in case we stop iterating
	min->rParSigma = rParSigma;

	// 2) require chisqDOF < maxChisqDOF (if maxChisqDOF is not NAN)
	// keep iterating regardless of rParSigma in this case
	float chisqDOF = Chisq / nDOF;
	if (isfinite(min->maxChisqDOF) && (chisqDOF > min->maxChisqDOF)) {
	    continue;
	}

	// delta-chisq or rParSigma ?
	if (min->chisqConvergence) {
	  done |= (min->lastDelta < min->minTol);
	} else {
	  done |= (rParSigma < min->minTol*nFitParams);
	}
    }
    psTrace("psLib.math", 5, "chisq: %f, last delta: %f, rParSigma: %f, Niter: %d\n", min->value, min->lastDelta, min->rParSigma, min->iter);

    // construct & return the covariance matrix (if requested)
    if (covar != NULL) {
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, NULL, 0.0, NULL)) {
            psTrace ("psLib.math", 5, "failure to calculate covariance matrix\n");
        }
        // set covar values which are not masked
        psImageInit (covar, 0.0);
        for (int j = 0, J = 0; j < params->n; j++) {
            if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) {
                covar->data.F32[j][j] = 1.0;
                continue;
            }
            for (int k = 0, K = 0; k < params->n; k++) {
                if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[k])) continue;
                covar->data.F32[j][k] = Alpha->data.F32[J][K];
                K++;
            }
            J++;
        }
    }

    // free the internal temporary data
    psFree(alpha);
    psFree(Alpha);
    psFree(beta);
    psFree(Beta);
    psFree(Params);
    if (yWt == NULL) {
        psFree(dy);
    }

    // if the last improvement was at least as good as maxTol, accept the fit:
    if (min->chisqConvergence) {
      if (min->lastDelta <= min->maxTol) {
	psTrace("psLib.math", 6, "---- end (true) ----\n");
        return(true);
      }
    } else {
      if (min->rParSigma <= min->maxTol*nFitParams) {
	psTrace("psLib.math", 6, "---- end (true) ----\n");
        return(true);
      }
    }
    psTrace("psLib.math", 6, "---- end (false) ----\n");
    return(false);
}

bool psMinLM_AllocAB (psImage **Alpha, psVector **Beta, const psVector *params, const psVector *paramMask) {

    assert (Alpha);
    assert (Beta);
    assert (params);

    int nParams = params->n;

    // count unmasked parameters
    if (paramMask) {
        nParams = 0;
        for (int i = 0; i < paramMask->n; i++) {
            if (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
            nParams ++;
        }
    }

    if (nParams == 0) {
        return false;
    }

    *Alpha = psImageAlloc(nParams, nParams, PS_TYPE_F32);
    *Beta  = psVectorAlloc(nParams, PS_TYPE_F32);
    return true;
}

static void minimizationFree(psMinimization *min)
{
    // There are no dynamically allocated items
}

psMinimization *psMinimizationAlloc(int maxIter, float minTol, float maxTol)
{
    PS_ASSERT_INT_NONNEGATIVE(maxIter, NULL);

    psMinimization *min = psAlloc(sizeof(psMinimization));
    psMemSetDeallocator(min, (psFreeFunc)minimizationFree);

    P_PSMINIMIZATION_SET_MAXITER(min,maxIter);
    P_PSMINIMIZATION_SET_MIN_TOL(min,minTol);
    P_PSMINIMIZATION_SET_MAX_TOL(min,maxTol);

    min->value = 0.0;
    min->iter = 0;
    min->lastDelta = NAN;
    min->rParSigma = NAN;
    min->maxChisqDOF = NAN;

    // we default to the old algorithm for convergence
    min->chisqConvergence = true;
    min->gainFactorMode = 0;
    min->isInteractive = false;
    min->useReweighting = false;
    
    return(min);
}

bool psMemCheckMinimization(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return( psMemGetDeallocator(ptr) == (psFreeFunc)minimizationFree );
}


static void constraintFree(psMinConstraint *tmp)
{
    if (tmp == NULL)
        return;

    psFree (tmp->paramMask);
}

psMinConstraint* psMinConstraintAlloc(void)
{
    psMinConstraint *tmp = psAlloc(sizeof(psMinConstraint));
    psMemSetDeallocator(tmp, (psFreeFunc)constraintFree);
    tmp->paramMask = NULL;
    tmp->checkLimits = NULL;

    return(tmp);
}

bool psMemCheckConstraint(psPtr tmp)
{
    return(psMemGetDeallocator(tmp) == (psFreeFunc) constraintFree);
}

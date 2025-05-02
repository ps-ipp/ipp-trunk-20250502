/** @file  psMinimizeLMM_ND.c
 *  \brief Levenberg-Marqardt minimization of N-D functions of N-D variables.  
 *  @ingroup Math
 *
 *  Levenberg-Marqardt minimization of an N-dimensional function of N-diminsional independent
 *  variables.  This code is based on the 1-D function version of N-D variables in psMinimizeLMM.c 
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <float.h>
#include <math.h>
#include "psAbort.h"
#include "psAssert.h"
#include "psVector.h"
#include "psMemory.h"
#include "psArray.h"
#include "psImage.h"
#include "psMatrix.h"
#include "psTrace.h"
#include "psError.h"
#include "psConstants.h"
#include "psImage.h"
#include "psLogMsg.h"

// the main user API: minimize the chisq of the N-D function
bool psMinimizeLMChi2(
    psMinimization *min,
    psImage *covar,
    psVector *params,
    psMinConstraint *constraint,
    const psArray *x,
    const psArray *y,
    const psArray *yWt,
    psMinimizeLMNDChi2Func func)
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
    PS_ASSERT_PTR_NON_NULL(y, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, false);
    for (psS32 i = 0 ; i < y->n ; i++) {
        psVector *value = (psVector *) (y->data[i]);
        PS_ASSERT_VECTOR_NON_NULL(value, false);
        PS_ASSERT_VECTOR_TYPE(value, PS_TYPE_F32, false);
    }
    if (yWt != NULL) {
	PS_ASSERT_VECTORS_SIZE_EQUAL(x, yWt, false);
	for (psS32 i = 0 ; i < yWt->n ; i++) {
	    psVector *value = (psVector *) (yWt->data[i]);
	    PS_ASSERT_VECTOR_NON_NULL(value, false);
	    PS_ASSERT_VECTOR_TYPE(value, PS_TYPE_F32, false);
	}
    }
    PS_ASSERT_PTR_NON_NULL(func, false);

    psMinimizeLMLimitFunc checkLimits = NULL;
    if (constraint) {
        checkLimits = constraint->checkLimits;
    }

    // This function has 'test' and 'current' values for several things (alpha, beta,
    // etc).  The current best value is in lower case; the next guess value is in upper
    // case

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

    // calculate initial alpha and beta, set chisq (min->value)
    min->value = psMinLM_SetABX(alpha, beta, params, paramMask, x, y, dy, func);
    if (isnan(min->value)) {
        min->iter = min->maxIter;
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

    // iterate until the tolerance is reached, or give up
    while ((min->iter < min->maxIter) && ((min->lastDelta > min->tol) || !isfinite(min->lastDelta))) {
        psTrace("psLib.math", 5, "Iteration number %d.  (max iterations is %d).\n", min->iter, min->maxIter);
        psTrace("psLib.math", 5, "Last delta is %f.  Min->tol is %f.\n", min->lastDelta, min->tol);

        // set a new guess for Alpha, Beta, Params
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, checkLimits, lambda, &dLinear)) {
            min->iter ++;
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

        // calculate Chisq for new guess, update Alpha & Beta
        Chisq = psMinLM_SetABX(Alpha, Beta, Params, paramMask, x, y, dy, func);
        if (isnan(Chisq)) {
            min->iter ++;
            lambda *= 10.0;
            continue;
        }

        // convergence criterion:
        // compare the delta (min->value - Chisq) with the
        // expected delta from the linear model (dLinear)
        // accept new guess if it is an improvement (rho > 0), or else increase lambda
        psF32 rho = (min->value - Chisq) / dLinear;

        psTrace("psLib.math", 5, "last chisq: %f, new chisq %f, delta: %f, dLinear: %f, rho: %f, lambda: %f\n", min->value,
                Chisq, min->lastDelta, dLinear, rho, lambda);

        // dump some useful info if trace is defined
        if (psTraceGetLevel("psLib.math") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (2)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (2)");
        }

        /* if (Chisq < min->value) {  */
        if (rho > 0.0) {
            min->lastDelta = (min->value - Chisq) / (dy->n - params->n);
            min->value = Chisq;
            alpha  = psImageCopy(alpha, Alpha, PS_TYPE_F32);
            beta   = psVectorCopy(beta, Beta, PS_TYPE_F32);
            params = psVectorCopy(params, Params, PS_TYPE_F32);
            lambda *= 0.25;
        } else {
            lambda *= 10.0;
        }
        min->iter++;
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
    if (min->iter == min->maxIter) {
        psTrace("psLib.math", 3, "---- end (false) ----\n");
        return(false);
    }
    psTrace("psLib.math", 3, "---- end (true) ----\n");
    return(true);
}

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
    psF32 chisq = psMinLM_SetABX(alpha, Beta, params, paramMask, x, y, dy, func);
    if (isnan(chisq)) {
        psTrace ("psLib.math", 5, "psMinLM_SetABX() returned a NAN chisq.\n");
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

// alpha, beta, params are already allocated
psF32 psMinLM_SetABX(
    psImage  *alpha,
    psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    const psArray  *x,
    const psArray  *y,
    const psArray *dy,
    psMinimizeLMNDChi2Func func)
{
    PS_ASSERT_IMAGE_NON_NULL(alpha, NAN);
    PS_ASSERT_VECTOR_NON_NULL(beta, NAN);
    PS_ASSERT_VECTOR_NON_NULL(params, NAN);
    PS_ASSERT_PTR_NON_NULL(x, NAN);
    PS_ASSERT_PTR_NON_NULL(y, NAN);
    PS_ASSERT_PTR_NON_NULL(dy, NAN);

    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    if (paramMask) {
        PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, false);
    }

    psF32 chisq;
    psF32 delta;
    psF32 weight;

    int nValue = ((psVector *)(y->data[0]))->n;

    psVector *deriv = psVectorAlloc(params->n, PS_TYPE_F32);
    psVector *ymodel = psVectorAlloc(nValue, PS_TYPE_F32);

    // zero alpha, beta, and chisq for summing below
    psImageInit (alpha, 0.0);
    psVectorInit (beta, 0.0);
    chisq = 0.0;

    // calculate chisq, alpha, beta. alpha & beta only represent unmasked parameters; skip
    // masked ones
    for (psS32 i = 0; i < y->n; i++) {
        func (ymodel, deriv, params, (psVector *) x->data[i]);

	psVector *yvalue = y->data[i];
	psVector *dyvalue = dy->data[i];

	for (int k = 0; k < nValue; k++) {
	    delta = ymodel->data.F32[k] - yvalue->data.F32[k];
	    chisq += PS_SQR(delta) * dyvalue->data.F32[k];

	    assert (!isnan(dyvalue->data.F32[k]));
	    assert (!isnan(delta));
	    assert (!isnan(chisq));
	}

	// we track alpha,beta and params,deriv separately
        for (int j = 0, J = 0; j < params->n; j++) {
            if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) continue;

            weight = deriv->data.F32[j] * dy->data.F32[i];

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

psMinimization *psMinimizationAlloc(int maxIter,
                                    float tol)
{
    PS_ASSERT_INT_NONNEGATIVE(maxIter, NULL);

    psMinimization *min = psAlloc(sizeof(psMinimization));
    psMemSetDeallocator(min, (psFreeFunc)minimizationFree);
    P_PSMINIMIZATION_SET_MAXITER(min,maxIter);
    P_PSMINIMIZATION_SET_TOL(min,tol);
    min->value = 0.0;
    min->iter = 0;
    min->lastDelta = NAN;

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

psMinConstraint* psMinConstraintAlloc()
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

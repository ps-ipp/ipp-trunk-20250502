/** @file  pmSourceFitModel.c
 *
 *  fit single source models to image pixels
 *
 *  @author EAM, IfA
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:31:25 $
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourcePhotometry.h"

#include "pmSourceFitModel.h"
#include "pmSourceFitSet.h"

// save as static values so they may be set externally
// static psF32 PM_SOURCE_FIT_MODEL_NUM_ITERATIONS = 15;
// static psF32 PM_SOURCE_FIT_MODEL_TOLERANCE = 0.1;
// static psF32 PM_SOURCE_FIT_MODEL_WEIGHT = 1.0;
// static bool  PM_SOURCE_FIT_MODEL_PIX_WEIGHTS = true;

/********************* Source Model Set Functions ***************************/

// these functions currently need to use this static variable because of the way psMinimizeLMM
// is implemented.  We could re-work that structure, but for now it is probably easier to make
// this thread safe by pre-allocating separate static variables for each thread.

static psArray *fitSets = NULL;
static pthread_mutex_t fitSetInitMutex = PTHREAD_MUTEX_INITIALIZER;

// call this before launching the threads
bool pmSourceFitSetInit (int nThreads) {

    if (!fitSets) {
        fitSets = psArrayAlloc (PS_MAX (1, nThreads));
    }

    // the allocated elements should be NULL on psArrayAlloc,
    // and a previously allocated array of fitSets should have been cleared
    // before pmSourceFitSetInit is called
    for (int i = 0; i < fitSets->n; i++) {
        psAssert(fitSets->data[i] == NULL, "failure to init or clear fitSets?");
    }
    return true;
}

void pmSourceFitSetDone (void) {
    psFree(fitSets);
    fitSets = NULL;
}

static void pmSourceFitSetDataFree (pmSourceFitSetData *set) {
    if (!set) return;

    psFree (set->modelSet);
    psFree (set->paramSet);
    psFree (set->derivSet);
    return;
}

pmSourceFitSetData *pmSourceFitSetDataAlloc (psArray *modelSet) {
    PS_ASSERT_PTR_NON_NULL(modelSet, NULL);

    pmSourceFitSetData *set = (pmSourceFitSetData *) psAlloc(sizeof(pmSourceFitSetData));
    psMemSetDeallocator(set, (psFreeFunc) pmSourceFitSetDataFree);

    set->modelSet  = psMemIncrRefCounter (modelSet);
    set->paramSet  = psArrayAlloc (modelSet->n);
    set->derivSet  = psArrayAlloc (modelSet->n);
    set->nParamSet = 0;
    set->thread    = pthread_self();

    for (int i = 0; i < modelSet->n; i++) {
        pmModel *model = modelSet->data[i];

        int nParams = pmModelClassParameterCount (model->type);

        set->paramSet->data[i] = psVectorCopy (NULL, model->params, PS_DATA_F32);
        set->derivSet->data[i] = psVectorAlloc (nParams, PS_DATA_F32);
        psVectorInit (set->derivSet->data[i], 0.0);

        set->nParamSet += nParams;
    }

    return set;
}

bool psMemCheckSourceFitSetData(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceFitSetDataFree);
}

pmSourceFitSetData *pmSourceFitSetDataSet (psArray *modelSet)
{
    psAssert(fitSets, "pmSourceFitSetInit not called");

    // find the fitSet used by this thread
    pthread_t id = pthread_self();

    // If our ID is already on the stack, abort.
    // We do need to lock on this because someone might pull one of the fitSets out from under us
    pthread_mutex_lock(&fitSetInitMutex);
    for (int i = 0; i < fitSets->n; i++) {
        pmSourceFitSetData *thisSet = fitSets->data[i];
        if (!thisSet) continue;
        if (thisSet->thread == id) {
            psAbort("pmSourceFitSetDataSet() called but previous entry not cleared");
        }
    }

    // Find an open slot
    for (int i = 0; i < fitSets->n; i++) {
        if (fitSets->data[i]) continue;
        pmSourceFitSetData *thisSet = fitSets->data[i] = pmSourceFitSetDataAlloc(modelSet);
        pthread_mutex_unlock(&fitSetInitMutex);
        return thisSet;
    }
    pthread_mutex_unlock(&fitSetInitMutex);
    psAbort("no empty slot for new pmSourceFitSetData");
    return NULL;
}

pmSourceFitSetData *pmSourceFitSetDataGet (void) {

    psAssert(fitSets, "pmSourceFitSetInit not called");

    // Find the fitSet used by this thread
    // We do need to lock on this because someone might pull one of the fitSets out from under us
    pthread_t id = pthread_self();
    pthread_mutex_lock(&fitSetInitMutex);
    for (int i = 0; i < fitSets->n; i++) {
        pmSourceFitSetData *thisSet = fitSets->data[i];
        if (!thisSet) continue;
        if (thisSet->thread == id) {
            pthread_mutex_unlock(&fitSetInitMutex);
            return thisSet;
        }
    }
    psAbort("pmSourceFitSetDataGet() called, but no entry found");
}

void pmSourceFitSetDataClear (void)
{
    psAssert (fitSets, "pmSourceFitSetInit not called");

    // Find the fitSet used by this thread
    // We do need to lock on this because someone might pull one of the fitSets out from under us
    pthread_t id = pthread_self();
    pthread_mutex_lock(&fitSetInitMutex);
    for (int i = 0; i < fitSets->n; i++) {
        pmSourceFitSetData *thisSet = fitSets->data[i];
        if (!thisSet) continue;
        if (thisSet->thread == id) {
            psFree(thisSet);
            fitSets->data[i] = NULL;
            pthread_mutex_unlock(&fitSetInitMutex);
            return;
        }
    }
    psAbort("pmSourceFitSetDataClear() called, but no entry found");

    return;
}

// this function is called with the full set of parameters and the beta values in a single vector
bool pmSourceFitSetCheckLimits (psMinConstraintMode mode, int nParam, float *params,
                                float *betas)
{
    PS_ASSERT_PTR_NON_NULL(fitSets, false);
    pmSourceFitSetData *thisSet = pmSourceFitSetDataGet();

    // nParam is the parameter in the full sequence.  determine which single model this comes from
    int nModel = -1;
    int nParamOne = -1;
    int nParamBase = 0;
    for (int i = 0; i < thisSet->modelSet->n; i++) {
        psVector *param = thisSet->paramSet->data[i];
        if ((nParamBase <= nParam) && (nParam < nParamBase + param->n)) {
            nModel = i;
            nParamOne = nParam - nParamBase;
            break;
        }
        nParamBase += param->n;
    }
    assert (nModel > -1);

    pmModel *model = thisSet->modelSet->data[nModel];

    // pass the single model function a pointer to the start of that model's sequence
    float *paramOne = params + nParamBase;
    float *betaOne = betas + nParamBase;
    bool status = model->class->modelLimits (mode, nParamOne, paramOne, betaOne);
    return status;
}

// merge parameters from FitSet models into single param and deriv vectors
bool pmSourceFitSetJoin (psVector *deriv, psVector *param, pmSourceFitSetData *set)
{
    PS_ASSERT_PTR_NON_NULL(set, false);
    PS_ASSERT_PTR_NON_NULL(set->paramSet, false);
    PS_ASSERT_PTR_NON_NULL(set->derivSet, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(set->paramSet, set->derivSet, false);
    int n = 0;
    int sum = 0;
    for (int i = 0; i < set->paramSet->n; i++) {
        sum+= set->paramSet->n;
    }

    // Must assert that the deriv and param psVectors are large enough, or else
    // a seg fault occurs.
    // XXX: Put the correct error call in here:
    if (0) {
        if (deriv->n < sum || param->n < sum) {
            PS_ASSERT_PTR_NON_NULL(0, false);
        }
    }

    for (int i = 0; i < set->paramSet->n; i++) {

        psVector *paramOne = set->paramSet->data[i];
        psVector *derivOne = set->derivSet->data[i];

        // one or the other (param or deriv) must be set
        assert ((deriv != NULL) || (param != NULL));

        // if we are setting derive, derivOne and paramOne must be same length
        assert ((deriv == NULL) || (paramOne->n == derivOne->n));

        for (int j = 0; j < paramOne->n; j++, n++) {
            if (param) {
                param->data.F32[n] = paramOne->data.F32[j];
            }
            if (deriv) {
                deriv->data.F32[n] = derivOne->data.F32[j];
            }
        }
    }
    return true;
}

// distribute parameters from single param and deriv vectors into FitSet models
bool pmSourceFitSetSplit (pmSourceFitSetData *set, const psVector *deriv, const psVector *param)
{
    PS_ASSERT_VECTOR_NON_NULL(param, false);
    PS_ASSERT_PTR_NON_NULL(set, false);
    PS_ASSERT_PTR_NON_NULL(set->paramSet, false);
    PS_ASSERT_PTR_NON_NULL(set->derivSet, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(set->paramSet, set->derivSet, false);

    int n = 0;
    for (int i = 0; i < set->paramSet->n; i++) {

        psVector *paramOne = set->paramSet->data[i];
        psVector *derivOne = set->derivSet->data[i];
        assert ((deriv == NULL) || (paramOne->n == derivOne->n));

        for (int j = 0; j < paramOne->n; j++, n++) {
            paramOne->data.F32[j] = param->data.F32[n];
            if (deriv) {
                derivOne->data.F32[j] = deriv->data.F32[n];
            }
        }
    }
    return true;
}

// set the model parameters for this fit set
bool pmSourceFitSetValues (pmSourceFitSetData *set, 
			   const psVector *dparam, const psVector *param, const psImage *covar, 
			   pmSource *source, psMinimization *myMin, int nPix, 
			   bool fitStatus, pmSourceFitOptions *options, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(set, false);
    PS_ASSERT_PTR_NON_NULL(set->paramSet, false);
    PS_ASSERT_VECTOR_NON_NULL(dparam, false);
    PS_ASSERT_VECTOR_NON_NULL(param, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_PTR_NON_NULL(myMin, false);

    bool onPic = true;

    int n = 0;
    int nStart = 0;
    for (int i = 0; i < set->paramSet->n; i++) {

        pmModel *model = set->modelSet->data[i];

        for (int j = 0; j < model->params->n; j++, n++) {
            model->params->data.F32[j] = param->data.F32[n];
            model->dparams->data.F32[j] = dparam->data.F32[n];
            psTrace ("psModules.objects", 4, "%f +/- %f", param->data.F32[n], dparam->data.F32[n]);
        }
	if (options->saveCovariance) {
	    // we only save the covar matrix for this object with itself (ignore cross terms between objects)
	    model->covar = psImageAlloc(model->params->n, model->params->n, PS_TYPE_F32);
	    for (int ix = 0; ix < model->params->n; ix++) {
		for (int iy = 0; iy < model->params->n; iy++) {
		    model->covar->data.F32[iy][ix] = covar->data.F32[nStart+iy][nStart+ix];
		}
	    }
	}
	nStart += model->params->n;
        psTrace ("psModules.objects", 4, " src %d", i);

	model->nIter = myMin->iter;
	// model->nPar is set by pmSourceFitSetMasks

        // save the resulting chisq, nDOF, nIter
        // these are not unique for any one source
	if (options->poissonErrors) {
	    model->chisq = myMin->value;
	    model->nPix  = nPix;
	    model->nDOF  = nPix - model->nPar;
	    model->chisqNorm = model->chisq / model->nDOF;
	} else {
	    pmSourceChisqUnsubtracted (source, model, maskVal);
	}

        // set the model success or failure status
        model->flags |= PM_MODEL_STATUS_FITTED;
	if (!fitStatus) {
	  if (isnan(myMin->value)) {
	    model->flags |= PM_MODEL_STATUS_NAN_CHISQ;
	  } else {
	    model->flags |= PM_MODEL_STATUS_NONCONVERGE;
	  }
	}

        // models can go insane: reject these
        onPic &= (model->params->data.F32[PM_PAR_XPOS] >= source->pixels->col0);
        onPic &= (model->params->data.F32[PM_PAR_XPOS] <  source->pixels->col0 + source->pixels->numCols);
        onPic &= (model->params->data.F32[PM_PAR_XPOS] >= source->pixels->row0);
        onPic &= (model->params->data.F32[PM_PAR_XPOS] <  source->pixels->row0 + source->pixels->numRows);
        if (!onPic) model->flags |= PM_MODEL_STATUS_OFFIMAGE;
    }
    return true;
}

// generic psMinLMM-style function for fitting: split the parameters across the models, call
// each model function one-at-a-time, the join the derivatives for on-going evaluation
psF32 pmSourceFitSetFunction(psVector *deriv, const psVector *param, const psVector *x)
{
    pmSourceFitSetData *thisSet = pmSourceFitSetDataGet();

    float chisqSum = 0.0;
    float chisqOne = 0.0;
    pmSourceFitSetSplit (thisSet, deriv, param);

    for (int i = 0; i < thisSet->modelSet->n; i++) {

        pmModel *model = thisSet->modelSet->data[i];

        psVector *paramOne = thisSet->paramSet->data[i];
        psVector *derivOne = thisSet->derivSet->data[i];

        chisqOne = model->class->modelFunc (derivOne, paramOne, x);
        chisqSum += chisqOne;
    }
    pmSourceFitSetJoin (deriv, NULL, thisSet);

    return (chisqSum);
}

// XXX allow the mode to be a function of the object (eg, S/N)
bool pmSourceFitSetMasks (psMinConstraint *constraint, pmSourceFitSetData *set,
                          pmSourceFitMode mode)
{
    PS_ASSERT_PTR_NON_NULL(set, false);
    PS_ASSERT_PTR_NON_NULL(constraint, false);

    // unmask everyone
    psVectorInit (constraint->paramMask, 0);

    int n = 0;
    for (int i = 0; i < set->paramSet->n; i++) {
        psVector *paramOne = set->paramSet->data[i];
        pmModel  *modelOne = set->modelSet->data[i];

        switch (mode) {
          case PM_SOURCE_FIT_NORM:
            // mask all but Xo,Yo,Io
            for (int j = 0; j < paramOne->n; j++) {
                if (j == PM_PAR_I0) continue;
                constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n + j] = 1;
		modelOne->nPar = 1;
            }
            break;
          case PM_SOURCE_FIT_PSF:
            // mask all but Xo,Yo,Io
            for (int j = 0; j < paramOne->n; j++) {
                if (j == PM_PAR_XPOS) continue;
                if (j == PM_PAR_YPOS) continue;
                if (j == PM_PAR_I0) continue;
                constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n + j] = 1;
		modelOne->nPar = 3;
            }
            break;
          case PM_SOURCE_FIT_EXT:
            // EXT model fits all params (except sky)
            constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n + PM_PAR_SKY] = 1;
	    modelOne->nPar = paramOne->n - 1;
            break;
          default:
            psAbort("invalid fitting mode");
        }
        n += paramOne->n;
    }
    return true;
}

bool pmSourcePrintModelSet (FILE *file, psArray *modelSet) {

    for (int i = 0; i < modelSet->n; i++) {
        pmModel *model = modelSet->data[i];
        int nParams = pmModelClassParameterCount (model->type);
        for (int j = 0; j < nParams; j++) {
            fprintf (file, "%d %d  : %f %f\n", i, j, model->params->data.F32[j], model->dparams->data.F32[j]);
        }
    }
    return true;
}

bool pmSourceFitSetPrint (FILE *file, pmSourceFitSetData *set) {

    for (int i = 0; i < set->paramSet->n; i++) {
        psVector *paramOne = set->paramSet->data[i];
        psVector *derivOne = set->derivSet->data[i];
        for (int j = 0; j < paramOne->n; j++) {
            fprintf (file, "%d %d  : %f %f\n", i, j, paramOne->data.F32[j], derivOne->data.F32[j]);
        }
    }
    return true;
}

bool pmSourceFitSet (pmSource *source,
                     psArray *modelSet,
		     pmSourceFitOptions *options,
                     psImageMaskType maskVal)
{
    psTrace("psModules.objects", 10, "---- %s begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_PTR_NON_NULL(source->maskObj, false);
    PS_ASSERT_PTR_NON_NULL(source->variance, false);

    bool fitStatus = true;
    bool onPic     = true;

    // maximum number of valid pixels
    int nPix = source->pixels->numRows * source->pixels->numCols;

    // construct the coordinate and value entries
    psArray *x = psArrayAllocEmpty(nPix);
    psVector *y = psVectorAllocEmpty(nPix, PS_TYPE_F32);
    psVector *yErr = psVectorAllocEmpty(nPix, PS_TYPE_F32);

    // fill in the coordinate and value entries
    nPix = 0;
    for (psS32 i = 0; i < source->pixels->numRows; i++) {
        for (psS32 j = 0; j < source->pixels->numCols; j++) {
            // skip masked points
            if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j] & maskVal) {
                continue;
            }
            // skip zero-variance points
            if (source->variance->data.F32[i][j] == 0) {
                continue;
            }
            // skip nan values in image
            if (!isfinite(source->pixels->data.F32[i][j])) {
                continue;
            }

            psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

            // Convert i/j to image space:
            // 0.5 PIX: the coordinate values must be in pixel coords, not index
            coord->data.F32[0] = (psF32) (j + 0.5 + source->pixels->col0);
            coord->data.F32[1] = (psF32) (i + 0.5 + source->pixels->row0);
            x->data[nPix] = (psPtr *) coord;
            y->data.F32[nPix] = source->pixels->data.F32[i][j];

            // psMinimizeLMChi2 takes wt = 1/dY^2.  suggestion from RHL is to use the local sky
            // as variance to avoid the bias from systematic errors here we would just use the
            // source sky variance
            if (options->poissonErrors) {
		yErr->data.F32[nPix] = 1.0 / source->variance->data.F32[i][j];
	    } else {
		yErr->data.F32[nPix] = 1.0 / options->weight;
	    }
	    nPix++;
	}
    }
    x->n = nPix;
    y->n = nPix;
    yErr->n = nPix;

// create the FitSet for this thread and set the initial parameter guesses
    pmSourceFitSetData *thisSet = pmSourceFitSetDataSet(modelSet);

// define param and deriv vectors for complete set of parameters
    psVector *params = psVectorAlloc (thisSet->nParamSet, PS_TYPE_F32);

// set the param and deriv vectors based on the curent values
    pmSourceFitSetJoin (NULL, params, thisSet);

// create the minimization constraints
    psMinConstraint *constraint = psMinConstraintAlloc();
    constraint->paramMask = psVectorAlloc (thisSet->nParamSet, PS_TYPE_VECTOR_MASK);
    constraint->checkLimits = pmSourceFitSetCheckLimits;

    pmSourceFitSetMasks (constraint, thisSet, options->mode);

// force the floating parameters to fall within the contraint ranges
    for (int i = 0; i < params->n; i++) {
	pmSourceFitSetCheckLimits (PS_MINIMIZE_PARAM_MIN, i, params->data.F32, NULL);
	pmSourceFitSetCheckLimits (PS_MINIMIZE_PARAM_MAX, i, params->data.F32, NULL);
    }

    if (psTraceGetLevel("psModules.objects") >= 5) {
	for (int i = 0; i < params->n; i++) {
	    fprintf (stderr, "%d %f %d\n", i, params->data.F32[i], constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
	}
    }

    if (nPix <  thisSet->nParamSet + 1) {
	psTrace (__func__, 4, "insufficient valid pixels\n");
	psTrace("psModules.objects", 10, "---- %s() end : fail pixels ----\n", __func__);
	for (int i = 0; i < modelSet->n; i++) {
	    pmModel *model = modelSet->data[i];
	    model->flags |= PM_MODEL_STATUS_BADARGS;
	}
	psFree (x);
	psFree (y);
	psFree (yErr);
	psFree (params);
	psFree(constraint);
	pmSourceFitSetDataClear(); // frees thisSet and removes if from the array of fitSets
	return(false);
    }

    psMinimization *myMin = psMinimizationAlloc (options->nIter, options->minTol, options->maxTol);
    myMin->gainFactorMode = options->gainFactorMode;
    myMin->chisqConvergence = options->chisqConvergence;
    myMin->isInteractive = options->isInteractive;

    psImage *covar = psImageAlloc (params->n, params->n, PS_TYPE_F32);

    fitStatus = psMinimizeLMChi2(myMin, covar, params, constraint, x, y, yErr, pmSourceFitSetFunction);
    if (!fitStatus) {
	psTrace("psModules.objects", 4, "Failed to fit model (%ld components)\n", modelSet->n);
    }

    // parameter errors from the covariance matrix
    psVector *dparams = psVectorAlloc (thisSet->nParamSet, PS_TYPE_F32);
    for (int i = 0; i < dparams->n; i++) {
	if ((constraint->paramMask != NULL) && constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i])
	    continue;
	dparams->data.F32[i] = sqrt(covar->data.F32[i][i]);
    }

    // get the Gauss-Newton distance for fixed model parameters
    if (constraint->paramMask != NULL) {
	psVector *delta = psVectorAlloc (params->n, PS_TYPE_F32);
	psVector *altmask = psVectorAlloc (params->n, PS_TYPE_VECTOR_MASK);
	altmask->data.PS_TYPE_VECTOR_MASK_DATA[0] = 1;
	for (int i = 1; i < dparams->n; i++) {
	    altmask->data.PS_TYPE_VECTOR_MASK_DATA[i] = (constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) ? 0 : 1;
	}
	psMinimizeGaussNewtonDelta(delta, params, altmask, x, y, yErr, pmSourceFitSetFunction);

	for (int i = 0; i < dparams->n; i++) {
	    if (!constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i])
		continue;
	    // note that delta is the value *subtracted* from the parameter
	    // to get the new guess.  for dparams to represent the direction
	    // of motion, we need to take -delta
	    dparams->data.F32[i] = -delta->data.F32[i];
	}
	psFree (delta);
	psFree (altmask);
    }

    pmSourceFitSetValues (thisSet, dparams, params, covar, source, myMin, y->n, fitStatus, options, maskVal);
    psTrace ("psModules.objects", 5, "onPic: %d, fitStatus: %d, nIter: %d, chisq: %f, nPix: %ld\n", onPic, fitStatus, myMin->iter, myMin->value, y->n);

    source->mode |= PM_SOURCE_MODE_FITTED;

    psFree(x);
    psFree(y);
    psFree(yErr);
    psFree(myMin);
    psFree(covar);
    psFree(constraint);
    psFree(params);
    psFree(dparams);
    pmSourceFitSetDataClear(); // frees thisSet and removes if from the array of fitSets

    bool rc = (onPic && fitStatus);
    psTrace("psModules.objects", 10, "---- %s end (%d) ----\n", __func__, rc);
    return(rc);
}

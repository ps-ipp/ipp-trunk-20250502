/** @file  pmSourceFitModel.c
 *
 *  fit single source models to image pixels
 *
 *  @author EAM, IfA
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.31 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:29:09 $
 *
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

void pmSourceFitOptionsFree(pmSourceFitOptions *opt)
{
    return;
}

pmSourceFitOptions *pmSourceFitOptionsAlloc(void) {

    pmSourceFitOptions *opt = (pmSourceFitOptions *) psAlloc(sizeof(pmSourceFitOptions));
    psMemSetDeallocator(opt, (psFreeFunc) pmSourceFitOptionsFree);

    opt->mode = PM_SOURCE_FIT_PSF;
    opt->nIter  = 15;
    opt->minTol = 0.01;
    opt->maxTol = 1.00;
    opt->weight = 1.00;
    opt->nsigma = 5.00;
    opt->maxChisqDOF = NAN;
    opt->poissonErrors = true;
    opt->saveCovariance = false;

    // we default to the old algorithm
    opt->gainFactorMode = 0;
    opt->chisqConvergence = true;
    opt->isInteractive = false;
    opt->useReweighting = false;

    return opt;
}

bool pmSourceFitModel (pmSource *source,
                       pmModel *model,
                       pmSourceFitOptions *options,
                       psImageMaskType maskVal)
{
    psTrace("psModules.objects", 10, "---- %s begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_PTR_NON_NULL(source->maskObj, false);

    // XXX if variance is NULL, use pixels instead (wrong, but not badly wrong)
    // PS_ASSERT_PTR_NON_NULL(source->variance, false);

    psBool fitStatus = true;
    psBool onPic     = true;
    psBool rc        = true;

    // maximum number of valid pixels
    psS32 nPix = source->pixels->numRows * source->pixels->numCols;

    // arrays to hold the data to be fitted
    psArray *x = psArrayAllocEmpty(nPix);
    psVector *y = psVectorAllocEmpty(nPix, PS_TYPE_F32);
    psVector *yErr = psVectorAllocEmpty(nPix, PS_TYPE_F32);

    // XXX for a test, skip the central pixel in the sersic fit
    bool skipCenter = false && (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));
    float Xo = model->params->data.F32[PM_PAR_XPOS];
    float Yo = model->params->data.F32[PM_PAR_YPOS];

    // if variance is NULL, we pretend pixels == variance
    float **vWgt = source->variance ? source->variance->data.F32 : source->pixels->data.F32;

    // fill in the coordinate and value entries
    nPix = 0;
    for (psS32 i = 0; i < source->pixels->numRows; i++) {
        for (psS32 j = 0; j < source->pixels->numCols; j++) {
            // skip masked points
            if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j] & maskVal) {
                continue;
            }
            // skip zero-variance points
            if (vWgt[i][j] == 0) {
                continue;
            }
            // skip nan values in image
            if (!isfinite(source->pixels->data.F32[i][j])) {
		fprintf (stderr, "WARNING: unmasked nan in image : %x vs %x\n", source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j], maskVal);
                continue;
            }

            // skip nan values in image
            if (!isfinite(vWgt[i][j])) {
		fprintf (stderr, "WARNING: unmasked nan in variance : %x vs %x\n", source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j], maskVal);
		continue;
            }

            // Convert i/j to image space:
	    // 0.5 PIX: the coordinate values must be in pixel coords, not index	    
            float Xv = (psF32) (j + 0.5 + source->pixels->col0);
            float Yv = (psF32) (i + 0.5 + source->pixels->row0);

	    // XXX possible skip of center pixel:
	    if (skipCenter) {
		float r = hypot(Xv - Xo, Yv - Yo);
		if (r < 0.75) {
		    continue;
		}
	    }

            psVector *coord = psVectorAlloc(2, PS_TYPE_F32);
            coord->data.F32[0] = Xv;
            coord->data.F32[1] = Yv;
            x->data[nPix] = (psPtr *) coord;
            y->data.F32[nPix] = source->pixels->data.F32[i][j];

            // psMinimizeLMChi2 takes wt = 1/dY^2.  suggestion from RHL is to use the local sky
            // as variance to avoid the bias from systematic errors here we would just use the
            // source sky variance
            if (options->poissonErrors) {
                yErr->data.F32[nPix] = 1.0 / vWgt[i][j];
            } else {
                yErr->data.F32[nPix] = 1.0 / options->weight;
            }
            nPix++;
        }
    }
    x->n = nPix;
    y->n = nPix;
    yErr->n = nPix;

    psVector *params = model->params;
    psVector *dparams = model->dparams;

    // create the minimization constraints
    psMinConstraint *constraint = psMinConstraintAlloc();
    constraint->paramMask = psVectorAlloc (params->n, PS_TYPE_VECTOR_MASK);
    constraint->checkLimits = model->class->modelLimits;

    // set parameter mask based on fitting mode
    int nParams = 0;
    switch (options->mode) {
      case PM_SOURCE_FIT_NORM:
        // NORM-only model fits only source normalization (Io)
        nParams = 1;
        psVectorInit (constraint->paramMask, 1);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
        break;
      case PM_SOURCE_FIT_PSF:
        // PSF model only fits x,y,Io
        nParams = 3;
        psVectorInit (constraint->paramMask, 1);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_XPOS] = 0;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_YPOS] = 0;
        break;
      case PM_SOURCE_FIT_EXT:
        // EXT model fits all shape params and Io (not Xo, Yo, sky)
        nParams = params->n - 3;
        psVectorInit (constraint->paramMask, 0);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_XPOS] = 1;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_YPOS] = 1;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SKY] = 1;
        break;
      case PM_SOURCE_FIT_TRAIL:
        // special mode for pmModel_TRAIL: Io, Xo, Yo, Length, and Theta (not Io or Sigma)
        nParams = params->n - 3;
        psVectorInit (constraint->paramMask, 0);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SKY] = 1;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SIGMA] = 1;
        break;
      case PM_SOURCE_FIT_INDEX:
        // PSF model only fits Io, index (PAR7) -- only Io for models with < 8 params
	psVectorInit (constraint->paramMask, 1);
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
        if (params->n == 7) {
	    nParams = 1;
	} else {
	    nParams = 2;
	    constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_7] = 0;
	}
	break;
      case PM_SOURCE_FIT_NO_INDEX:
        // PSF model only fits Io, Sxx, Sxy, Syy
	psVectorInit (constraint->paramMask, 0);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_XPOS] = 1;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_YPOS] = 1;
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SKY] = 1;
        if (params->n == 7) {
	    nParams = params->n - 3;
	} else {
	    nParams = params->n - 4;
	    constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_7] = 1;
	}
	break;
      default:
	psAbort("invalid fitting mode");
    }
    // force the floating parameters to fall within the contraint ranges
    for (int i = 0; i < params->n; i++) {
	model->class->modelLimits (PS_MINIMIZE_PARAM_MIN, i, params->data.F32, NULL);
	model->class->modelLimits (PS_MINIMIZE_PARAM_MAX, i, params->data.F32, NULL);
    }

    if (nPix <  nParams + 1) {
        psTrace ("psModules.objects", 4, "insufficient valid pixels\n");
        model->flags |= PM_MODEL_STATUS_BADARGS;
        psFree (x);
        psFree (y);
        psFree (yErr);
        psFree (constraint);
        return(false);
    }

    psMinimization *myMin = psMinimizationAlloc (options->nIter, options->minTol, options->maxTol);
    myMin->gainFactorMode = options->gainFactorMode;
    myMin->chisqConvergence = options->chisqConvergence;
    myMin->isInteractive = options->isInteractive;
    myMin->useReweighting = options->useReweighting;

    psImage *covar = psImageAlloc (params->n, params->n, PS_TYPE_F32);

    fitStatus = psMinimizeLMChi2(myMin, covar, params, constraint, x, y, yErr, model->class->modelFunc);
    for (int i = 0; i < dparams->n; i++) {
        if ((constraint->paramMask != NULL) && constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i])
            continue;
        dparams->data.F32[i] = sqrt(covar->data.F32[i][i]);
        psTrace ("psModules.objects", 4, "%f +/- %f", params->data.F32[i], dparams->data.F32[i]);
    }
    if (options->saveCovariance) {
	psFree (model->covar);
	model->covar = psMemIncrRefCounter(covar);
    }
    model->nIter = myMin->iter;
    model->nPar = nParams;

    psTrace ("psModules.objects", 4, "niter: %d, chisq: %f", myMin->iter, myMin->value);

    // save the resulting chisq, nDOF, nIter
    // NOTE: if (!options->poissonErrors) chisq will be wrong : recalculate
    if (options->poissonErrors) {
	model->chisq = myMin->value;
	model->nPix  = y->n;
	model->nDOF  = y->n - model->nPar;
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

    if (myMin->chisqConvergence) {
      if (myMin->lastDelta > myMin->minTol) model->flags |= PM_MODEL_STATUS_WEAK_FIT;
    } else {
      if (myMin->rParSigma > myMin->minTol*nParams) model->flags |= PM_MODEL_STATUS_WEAK_FIT;
    }

    // get the Gauss-Newton distance for fixed model parameters
    // hold the fitted parameters fixed; mask sky which is not fitted at all
    if (constraint->paramMask != NULL) {
        psVector *delta = psVectorAlloc (params->n, PS_TYPE_F32);
        psVector *altmask = psVectorAlloc (params->n, PS_TYPE_VECTOR_MASK);
        altmask->data.PS_TYPE_VECTOR_MASK_DATA[0] = 1;
        for (int i = 1; i < dparams->n; i++) {
            altmask->data.PS_TYPE_VECTOR_MASK_DATA[i] = (constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) ? 0 : 1;
        }
        psMinimizeGaussNewtonDelta(delta, params, altmask, x, y, yErr, model->class->modelFunc);

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

    // models can go insane: reject these
    onPic &= (params->data.F32[PM_PAR_XPOS] >= source->pixels->col0);
    onPic &= (params->data.F32[PM_PAR_XPOS] <  source->pixels->col0 + source->pixels->numCols);
    onPic &= (params->data.F32[PM_PAR_YPOS] >= source->pixels->row0);
    onPic &= (params->data.F32[PM_PAR_YPOS] <  source->pixels->row0 + source->pixels->numRows);
    if (!onPic) {
        model->flags |= PM_MODEL_STATUS_OFFIMAGE;
    }

    source->mode |= PM_SOURCE_MODE_FITTED;

    psFree(x);
    psFree(y);
    psFree(yErr);
    psFree(myMin);
    psFree(covar);
    psFree(constraint);

    rc = (onPic && fitStatus);
    psTrace("psModules.objects", 10, "---- %s(%d) end ----\n", __func__, rc);
    return(rc);
}


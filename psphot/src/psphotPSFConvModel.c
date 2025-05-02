# include "psphotInternal.h"
# define USE_DELTA_PSF 0

// save as static values so they may be set externally
static psF32 PM_SOURCE_FIT_MODEL_NUM_ITERATIONS = 15;
static psF32 PM_SOURCE_FIT_MODEL_MIN_TOL = 0.1;
static psF32 PM_SOURCE_FIT_MODEL_MAX_TOL = 2.0;

// input source has both modelPSF and modelEXT.  on successful exit, we set the
// modelConv to contain the fitted parameters, and the modelFlux to contain the 
// convolved model image.

// XXX need to generalize this -- number of fitted parameters must be flexible based on the fitOptions

pmModel *psphotPSFConvModel (pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {
    
    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // make sure we save a cached copy of the psf flux
    pmSourceCachePSF (source, maskVal);

    // convert the cached cached psf model for this source to a psKernel
    psKernel *psf = psphotKernelFromPSF (source, psfSize);
    if (!psf) return NULL;

# if (USE_DELTA_PSF)
    psImageInit (psf->image, 0.0);
    psf->image->data.F32[(int)(0.5*psf->image->numRows)][(int)(0.5*psf->image->numCols)] = 1.0;
# endif

    // generate copy of the model
    // XXX we could modify the parameter values or even the model 
    // here based on the observed seeing (some lookup table...)

    // use the source moments, etc to guess basic model parameters
    pmModel *modelConv = pmSourceModelGuess (source, modelType);
    if (!modelConv) {
	psFree (psf);
	return NULL;
    }

    // adjust the pixels based on the footprint
    float radius = psphotSetRadiusEXT (readout, source, markVal);
    if (!pmSourceMoments (source, radius, 0.25*radius, 0.0, maskVal)) return false;

    // XXX test : modify the Io, SXX, SYY terms based on the psf SXX, SYY terms:
    psEllipseShape psfShape;
    psfShape.sx  = source->modelPSF->params->data.F32[PM_PAR_SXX] / M_SQRT2;
    psfShape.sxy = source->modelPSF->params->data.F32[PM_PAR_SXY];
    psfShape.sy  = source->modelPSF->params->data.F32[PM_PAR_SYY] / M_SQRT2;
    psEllipseAxes psfAxes = psEllipseShapeToAxes (psfShape, 20.0);

    // XXX test : modify the Io, SXX, SYY terms based on the psf SXX, SYY terms:
    psEllipseShape extShape;
    extShape.sx  = modelConv->params->data.F32[PM_PAR_SXX] / M_SQRT2;
    extShape.sxy = modelConv->params->data.F32[PM_PAR_SXY];
    extShape.sy  = modelConv->params->data.F32[PM_PAR_SYY] / M_SQRT2;
    psEllipseAxes extAxes = psEllipseShapeToAxes (extShape, 20.0);

    // decrease the initial guess ellipse by psf_minor axis:
    psEllipseAxes extAxesMod;
    extAxesMod.major = sqrt (PS_MAX (1.0, PS_SQR(extAxes.major) - PS_SQR(psfAxes.minor)));
    extAxesMod.minor = sqrt (PS_MAX (1.0, PS_SQR(extAxes.minor) - PS_SQR(psfAxes.minor)));
    extAxesMod.theta = extAxes.theta;

    psEllipseShape extShapeMod = psEllipseAxesToShape (extAxesMod);
    modelConv->params->data.F32[PM_PAR_SXX] = extShapeMod.sx * M_SQRT2;
    modelConv->params->data.F32[PM_PAR_SXY] = extShapeMod.sxy;
    modelConv->params->data.F32[PM_PAR_SYY] = extShapeMod.sy * M_SQRT2;

    // increase the initial guess central intensity by 2pi r^2:
    modelConv->params->data.F32[PM_PAR_I0] *= (1.0 + PS_SQR(psfAxes.minor) / PS_SQR(extAxesMod.minor));

    psVector *params  = modelConv->params;
    psVector *dparams = modelConv->dparams;

    // create the minimization constraints
    psMinConstraint *constraint = psMinConstraintAlloc();
    constraint->paramMask = psVectorAlloc (params->n, PS_TYPE_VECTOR_MASK);
    constraint->checkLimits = modelConv->modelLimits;

    // set parameter mask based on fitting mode
    // we fit a model without a floating sky term
    int nParams = params->n - 1;
    psVectorInit (constraint->paramMask, 0);
    constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SKY] = 1;

    // force the floating parameters to fall within the contraint ranges
    for (int i = 0; i < params->n; i++) {
	modelConv->modelLimits (PS_MINIMIZE_PARAM_MIN, i, params->data.F32, NULL);
	modelConv->modelLimits (PS_MINIMIZE_PARAM_MAX, i, params->data.F32, NULL);
    }

    // set up the minimization process
    psMinimization *myMin = psMinimizationAlloc (PM_SOURCE_FIT_MODEL_NUM_ITERATIONS, PM_SOURCE_FIT_MODEL_MIN_TOL, PM_SOURCE_FIT_MODEL_MAX_TOL);

    psImage *covar = psImageAlloc (params->n, params->n, PS_TYPE_F32);

    bool fitStatus = psphotModelWithPSF_LMM (myMin, covar, params, constraint, source, psf, modelConv->modelFunc);
    for (int i = 0; i < dparams->n; i++) {
        if (psTraceGetLevel("psphot") >= 4) {
            fprintf (stderr, "%f ", params->data.F32[i]);
        }
        if ((constraint->paramMask != NULL) && constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i])
            continue;
        dparams->data.F32[i] = sqrt(covar->data.F32[i][i]);
    }
    psTrace ("psphot", 4, "niter: %d, chisq: %f", myMin->iter, myMin->value);

    // renormalize output model image (generated by fitting process)
    float Io = params->data.F32[PM_PAR_I0];
    for (int iy = 0; iy < source->modelFlux->numRows; iy++) {
	for (int ix = 0; ix < source->modelFlux->numCols; ix++) {
	    source->modelFlux->data.F32[iy][ix] /= Io;
	}
    }

    // save the resulting chisq, nDOF, nIter
    modelConv->chisq = myMin->value;
    modelConv->nIter = myMin->iter;

    // XXX I actually need to count the number of unmasked pixels here
    modelConv->nDOF  = source->pixels->numCols*source->pixels->numRows  -  nParams;

    modelConv->flags |= PM_MODEL_STATUS_FITTED;
    if (!fitStatus) modelConv->flags |= PM_MODEL_STATUS_NONCONVERGE;

    // models can go insane: reject these
    bool onPic = true;
    onPic &= (params->data.F32[PM_PAR_XPOS] >= source->pixels->col0);
    onPic &= (params->data.F32[PM_PAR_XPOS] <  source->pixels->col0 + source->pixels->numCols);
    onPic &= (params->data.F32[PM_PAR_YPOS] >= source->pixels->row0);
    onPic &= (params->data.F32[PM_PAR_YPOS] <  source->pixels->row0 + source->pixels->numRows);
    if (!onPic) modelConv->flags |= PM_MODEL_STATUS_OFFIMAGE;

    source->mode |= PM_SOURCE_MODE_FITTED; // XXX is this needed?

    psFree(psf);
    psFree(myMin);
    psFree(covar);
    psFree(constraint);

    return modelConv;
}

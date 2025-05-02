/** @file  pmModelUtils.c
 *
 *  Functions to manipulate object models
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-08 21:53:08 $
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
#include "pmSourceFitModel.h"
#include "pmPSF.h"

#include "pmErrorCodes.h"

/*****************************************************************************
pmModelFromPSF (*modelEXT, *psf):  use the model position parameters to
construct a realization of the PSF model at the object coordinates
 *****************************************************************************/
pmModel *pmModelFromPSF (pmModel *modelEXT, const pmPSF *psf)
{
    PS_ASSERT_PTR_NON_NULL(psf, NULL);
    PS_ASSERT_PTR_NON_NULL(modelEXT, NULL);

    // allocate a new pmModel to hold the PSF version
    pmModel *modelPSF = pmModelAlloc (psf->type);

    // set model parameters for this source based on PSF information
    if (!modelEXT->class->modelFromPSF (modelPSF, modelEXT, psf)) {
        psTrace ("psModules.objects", 3, "Failed to set model params from PSF");
        psFree(modelPSF);
        return NULL;
    }
    // note that model->residuals is just a reference
    modelPSF->residuals = psf->residuals;

    return (modelPSF);
}

// instantiate a model for the PSF at this location with peak flux
// NOTE: psf and (Xo,Yo) are defined wrt chip coordinates
pmModel *pmModelFromPSFforXY (const pmPSF *psf, float Xo, float Yo, float Io)
{
    PS_ASSERT_PTR_NON_NULL(psf, NULL);

    // allocate a new pmModel to hold the PSF version
    pmModel *modelPSF = pmModelAlloc (psf->type);

    // set model parameters for this source based on PSF information
    if (!modelPSF->class->modelParamsFromPSF (modelPSF, psf, Xo, Yo, Io)) {
        psFree(modelPSF);
        return NULL;
    }

    // note that model->residuals is just a reference
    modelPSF->residuals = psf->residuals;

    return (modelPSF);
}

// set this model to have the requested flux
bool pmModelSetFlux (pmModel *model, float flux) {
    PS_ASSERT_PTR_NON_NULL(model, NULL);
    PS_ASSERT_PTR_NON_NULL(model->params, NULL);

    // set Io to be 1.0
    model->params->data.F32[PM_PAR_I0] = 1.0;

    // determine the normalized flux
    float normFlux = model->class->modelFlux (model->params);
    assert (isfinite(normFlux));
    assert (normFlux > 0);

    // set the desired normalization
    model->params->data.F32[PM_PAR_I0] = flux / normFlux;

    return true;
}

bool pmModelUseReff (pmModelType type) {

    pmModelClass *class = pmModelClassSelect (type);
    psAssert (class, "undefined model class?");
    bool useReff = class->useReff;
    return useReff;
}

// this function and the one below handle the two cases, where the model shape is uses R_eff or Sigma
bool pmModelAxesToParams (float *Sxx, float *Sxy, float *Syy, psEllipseAxes axes, bool useReff)  {

    // restrict axex to 0.5 here not below 
    if (axes.minor < 0.2) axes.minor = 0.2;
    if (axes.major < 0.2) axes.major = 0.2;

    psEllipseShape shape = psEllipseAxesToShape (axes);

    if (!isfinite(shape.sx))  return false;
    if (!isfinite(shape.sy))  return false;
    if (!isfinite(shape.sxy)) return false;

    // set the shape parameters
    if (useReff) {
	// *Sxx  = PS_MAX(0.5, shape.sx);
	// *Syy  = PS_MAX(0.5, shape.sy);
	*Sxx  = shape.sx;
	*Syy  = shape.sy;
	*Sxy  = shape.sxy * 2.0;
    } else {
	// *Sxx  = PS_MAX(0.5, M_SQRT2*shape.sx);
	// *Syy  = PS_MAX(0.5, M_SQRT2*shape.sy);
	*Sxx  = M_SQRT2*shape.sx;
	*Syy  = M_SQRT2*shape.sy;
	*Sxy  = shape.sxy;
    }

    return true;
}

bool pmModelParamsToAxes (psEllipseAxes *axes, float Sxx, float Sxy, float Syy, bool useReff)  {

    psEllipseShape shape;

    // set the shape parameters
    if (useReff) {
	shape.sx  = Sxx;
	shape.sy  = Syy;
	shape.sxy = Sxy / 2.0;
    } else {
	shape.sx  = Sxx / M_SQRT2;
	shape.sy  = Syy / M_SQRT2;
	shape.sxy = Sxy;
    }

    if ((shape.sx == 0) || (shape.sy == 0)) {
        axes->major = 0.0;
        axes->minor = 0.0;
        axes->theta = 0.0;
    } else {
	// axes ratio < 20
	// replace with maxAR argument?
	*axes = psEllipseShapeToAxes (shape, 20.0);
    }

    return true;
}

// Reff says if this is a model which uses R_eff (like exp or dev) instead of Sigma
// set the parameter values SXX, SXY, SYY
// Scale allows some models to increase the guess size relative to Mxx,Myy
bool pmModelSetShape (float *Sxx, float *Sxy, float *Syy, pmMoments *moments, bool useReff, float Scale) {

    psEllipseMoments emoments;
    emoments.x2 = moments->Mxx;
    emoments.xy = moments->Mxy;
    emoments.y2 = moments->Myy;

    // force the axis ratio to be < 20.0
    psEllipseAxes axes = psEllipseMomentsToAxes (emoments, 20.0);

    if (!isfinite(axes.major)) return false;
    if (!isfinite(axes.minor)) return false;
    if (!isfinite(axes.theta)) return false;

    // set a lower limit to avoid absurd solutions..
    // NOTE: I should set the lower limit based on the PSF size, if known
    float Rmajor = PS_MAX(1.0, Scale * axes.major);
    float Rminor = Rmajor * (axes.minor / axes.major);
    axes.major = Rmajor;
    axes.minor = Rminor;

    // EAM 2022.02.05 : Mrf is often much too large, disable this for now
    // Mxx, Mxy, Myy define the elliptical shape, but Mrf defines the width 
    // float scale = (isfinite(moments->Mrf) && (moments->Mrf > 0.0)) ? moments->Mrf / axes.major : 1.0;
    // axes.major *= scale;
    // axes.minor *= scale;

    pmModelAxesToParams (Sxx, Sxy, Syy, axes, useReff);

    return true;
}

bool pmModelSetNorm (float *Io, pmSource *source) {

    *Io = source->peak->rawFlux;

#ifndef ALLOW_NONFINITE_PEAK
    // Gene says fail of peak !finite
    if (!isfinite(*Io)) return false;
#else 
    // This is the way it used to be. Somtimes an infinite value Io made it's way down the pipeline
    // causing assertion failures
    if (!isfinite(*Io) && !source->moments) return false;

    *Io = source->moments->Peak;
    if (!isfinite(*Io)) return false;
#endif

    return true;
}

bool pmModelSetPosition (float *Xo, float *Yo, pmSource *source) {

    bool useMoments = pmSourcePositionUseMoments(source);
    
    if (useMoments) {
	*Xo = source->moments->Mx;
	*Yo = source->moments->My;
    } else {
	*Xo = source->peak->xf;
	*Yo = source->peak->yf;
    }

    return true;
}

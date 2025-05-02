/******************************************************************************
 * this file defines the TEST1 source shape model.  Note that these model functions are loaded
 * by pmModelGroup.c using 'include', and thus need no 'include' statements of their own.  The
 * models use a psVector to represent the set of parameters, with the sequence used to specify
 * the meaning of the parameter.  The meaning of the parameters may thus vary depending on the
 * specifics of the model.  All models which are used a PSF representations share a few
 * parameters, for which # define names are listed in pmModel.h:

 * PM_PAR_SKY 0   - local sky : note that this is unused and may be dropped in the future
 * PM_PAR_I0 1    - central intensity
 * PM_PAR_XPOS 2  - X center of object
 * PM_PAR_YPOS 3  - Y center of object
 * PM_PAR_SXX 4   - X^2 term of elliptical contour (sqrt(2) * SigmaX)
 * PM_PAR_SYY 5   - Y^2 term of elliptical contour (sqrt(2) * SigmaY)
 * PM_PAR_SXY 6   - X*Y term of elliptical contour
 *****************************************************************************/

# define PM_MODEL_FUNC            pmModelFunc_TEST1
# define PM_MODEL_FLUX            pmModelFlux_TEST1
# define PM_MODEL_GUESS           pmModelGuess_TEST1
# define PM_MODEL_LIMITS          pmModelLimits_TEST1
# define PM_MODEL_RADIUS          pmModelRadius_TEST1
# define PM_MODEL_FROM_PSF        pmModelFromPSF_TEST1
# define PM_MODEL_PARAMS_FROM_PSF pmModelParamsFromPSF_TEST1
# define PM_MODEL_FIT_STATUS      pmModelFitStatus_TEST1

// the model is a function of the pixel coordinate (pixcoord[0,1] = x,y)
psF32 PM_MODEL_FUNC(psVector *deriv,
                    const psVector *params,
                    const psVector *pixcoord)
{
    psF32 *PAR = params->data.F32;

    // XXX this is fitting sigma_x/sqrt(2), sigma_y/sqrt(2)
    psF32 X  = pixcoord->data.F32[0] - PAR[PM_PAR_XPOS];
    psF32 Y  = pixcoord->data.F32[1] - PAR[PM_PAR_YPOS];
    psF32 px = X / PAR[PM_PAR_SXX];
    psF32 py = Y / PAR[PM_PAR_SYY];
    psF32 z  = PS_SQR(px) + PS_SQR(py) + PAR[PM_PAR_SXY]*X*Y;
    psF32 t  = 1 + z + z*z/2.0;
    psF32 r  = 1.0 / (t + z*z*z/6.0); /* exp (-Z) */
    psF32 f  = PAR[PM_PAR_I0]*r + PAR[PM_PAR_SKY];

    if (deriv != NULL) {
        psF32 *dPAR = deriv->data.F32;
        psF32 q = PAR[PM_PAR_I0]*r*r*t;
        dPAR[PM_PAR_SKY]  = +1.0;
        dPAR[PM_PAR_I0]   = +r;
        dPAR[PM_PAR_XPOS] = q*(2.0*px/PAR[PM_PAR_SXX] + Y*PAR[PM_PAR_SXY]);
        dPAR[PM_PAR_YPOS] = q*(2.0*py/PAR[PM_PAR_SYY] + X*PAR[PM_PAR_SXY]);
        dPAR[PM_PAR_SXX]  = +2.0*q*px*px/PAR[PM_PAR_SXX];
        dPAR[PM_PAR_SYY]  = +2.0*q*py*py/PAR[PM_PAR_SYY];
        dPAR[PM_PAR_SXY]  = -q*X*Y;
    }
    return(f);
}

// define the parameter limits
bool PM_MODEL_LIMITS (psMinConstraintMode mode, int nParam, float *params, float *beta)
{
    float beta_lim = 0;
    float params_min = 0;
    float params_max = 0;

    switch (mode) {
      case PS_MINIMIZE_BETA_LIMIT:
        switch (nParam) {
          case PM_PAR_SKY:  beta_lim = 1000;  break;
          case PM_PAR_I0:   beta_lim = 3e6;   break;
          case PM_PAR_XPOS: beta_lim = 5;     break;
          case PM_PAR_YPOS: beta_lim = 5;     break;
          case PM_PAR_SXX:  beta_lim = 0.5;   break;
          case PM_PAR_SYY:  beta_lim = 0.5;   break;
          case PM_PAR_SXY:  beta_lim = 0.5;   break;
          default:
            psAbort("invalid parameter %d for beta test", nParam);
        }
        if (fabs(beta[nParam]) > fabs(beta_lim)) {
            beta[nParam] = (beta[nParam] > 0) ? fabs(beta_lim) : -fabs(beta_lim);
            return false;
        }
        return true;
      case PS_MINIMIZE_PARAM_MIN:
        switch (nParam) {
          case PM_PAR_SKY:  params_min = -1000; break;
          case PM_PAR_I0:   params_min =     0; break;
          case PM_PAR_XPOS: params_min =  -100; break;
          case PM_PAR_YPOS: params_min =  -100; break;
          case PM_PAR_SXX:  params_min =   0.5; break;
          case PM_PAR_SYY:  params_min =   0.5; break;
          case PM_PAR_SXY:  params_min =  -5.0; break;
          default:
            psAbort("invalid parameter %d for param min test", nParam);
        }
        if (params[nParam] < params_min) {
            params[nParam] = params_min;
            return false;
        }
        return true;
      case PS_MINIMIZE_PARAM_MAX:
        switch (nParam) {
          case PM_PAR_SKY:  params_max =   1e5; break;
          case PM_PAR_I0:   params_max =   1e8; break;
          case PM_PAR_XPOS: params_max =   1e4; break;
          case PM_PAR_YPOS: params_max =   1e4; break;
          case PM_PAR_SXX:  params_max =   100; break;
          case PM_PAR_SYY:  params_max =   100; break;
          case PM_PAR_SXY:  params_max =  +5.0; break;
          default:
            psAbort("invalid parameter %d for param max test", nParam);
        }
        if (params[nParam] > params_max) {
            params[nParam] = params_max;
            return false;
        }
        return true;
      default:
        psAbort("invalid choice for limits");
    }
    psAbort("should not reach here");
    return false;
}

// make an initial guess for parameters
bool PM_MODEL_GUESS (pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal)
{
    pmMoments *moments = source->moments;
    pmPeak    *peak    = source->peak;
    psF32     *PAR  = model->params->data.F32;

    psEllipseMoments emoments;
    emoments.x2 = moments->Mxx;
    emoments.y2 = moments->Myy;
    emoments.xy = moments->Mxy;

    // force the axis ratio to be < 20.0
    psEllipseAxes axes = psEllipseMomentsToAxes (emoments, 20.0);
    psEllipseShape shape = psEllipseAxesToShape (axes);

    PAR[PM_PAR_SKY] = moments->Sky;
    PAR[PM_PAR_I0]   = peak->rawFlux;
    PAR[PM_PAR_XPOS] = peak->xf;
    PAR[PM_PAR_YPOS] = peak->yf;
    PAR[PM_PAR_SXX] = PS_MAX(0.5, M_SQRT2*shape.sx);
    PAR[PM_PAR_SYY] = PS_MAX(0.5, M_SQRT2*shape.sy);
    PAR[PM_PAR_SXY] = shape.sxy;
    PAR[PM_PAR_SXY] = 0.0;  // XXX we can get this right if we do the integral

    return(true);
}

psF64 PM_MODEL_FLUX(const psVector *params)
{
    float norm, z;
    psEllipseShape shape;

    psF32 *PAR = params->data.F32;

    shape.sx  = PAR[PM_PAR_SXX] / sqrt(2.0);
    shape.sy  = PAR[PM_PAR_SYY] / sqrt(2.0);
    shape.sxy = PAR[PM_PAR_SXY];

    // Area is equivalent to 2 pi sigma^2
    psEllipseAxes axes = psEllipseShapeToAxes (shape, 20.0);
    psF64 Area = 2.0 * M_PI * axes.major * axes.minor;

    // the area needs to be multiplied by the integral of f(z)
    norm = 0.0;

# define DZ 0.25

    float f0 = 1.0;
    float f1, f2;
    for (z = DZ; z < 50; z += DZ) {
        f1 = 1.0 / (1 + z + z*z/2.0 + z*z*z/6.0);
        z += DZ;
        f2 = 1.0 / (1 + z + z*z/2.0 + z*z*z/6.0);
        norm += f0 + 4*f1 + f2;
        f0 = f2;
    }
    norm *= DZ / 3.0;

    psF64 Flux = PAR[PM_PAR_I0] * Area * norm;

    return(Flux);
}

// define this function so it never returns Inf or NaN
// return the radius which yields the requested flux
psF64 PM_MODEL_RADIUS (const psVector *params, psF64 flux)
{
    psEllipseShape shape;

    if (flux <= 0)
        return (1.0);
    if (params->data.F32[PM_PAR_I0] <= 0)
        return (1.0);
    if (flux >= params->data.F32[PM_PAR_I0])
        return (1.0);

    psF32 *PAR = params->data.F32;

    shape.sx  = PAR[PM_PAR_SXX] / sqrt(2.0);
    shape.sy  = PAR[PM_PAR_SYY] / sqrt(2.0);
    shape.sxy = PAR[PM_PAR_SXY];

    // this estimates the radius assuming f(z) is roughly exp(-z)
    psEllipseAxes axes = psEllipseShapeToAxes (shape, 20.0);
    psF64 radius = axes.major * sqrt (2.0 * log(params->data.F32[PM_PAR_I0] / flux));

    if (isnan(radius)) psAbort("error in code: never return invalid radius");
    if (radius < 0) psAbort("error in code: never return invalid radius");

    return (radius);
}

bool PM_MODEL_FROM_PSF (pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf)
{
    psF32 *out = modelPSF->params->data.F32;
    psF32 *in  = modelFLT->params->data.F32;

    // we require these two parameters to exist
    assert (psf->params->n > PM_PAR_YPOS);
    assert (psf->params->n > PM_PAR_XPOS);

    for (int i = 0; i < psf->params->n; i++) {
        if (psf->params->data[i] == NULL) {
            out[i] = in[i];
        } else {
            pmTrend2D *trend = psf->params->data[i];
            out[i] = pmTrend2DEval(trend, in[PM_PAR_XPOS], in[PM_PAR_YPOS]);
        }
    }

    // the 2D model for SXY actually fits SXY / (SXX^-2 + SYY^-2); correct here
    out[PM_PAR_SXY] = pmPSF_SXYtoModel (out);

    return(true);
}

// construct the PSF model from the FLT model and the psf
// XXX is this sufficiently general do be a global function, not a pmModelClass function?
bool PM_MODEL_PARAMS_FROM_PSF (pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io)
{
    psF32 *PAR = model->params->data.F32;

    // we require these two parameters to exist
    assert (psf->params->n > PM_PAR_YPOS);
    assert (psf->params->n > PM_PAR_XPOS);

    PAR[PM_PAR_SKY]  = 0.0;
    PAR[PM_PAR_I0]   = Io;
    PAR[PM_PAR_XPOS] = Xo;
    PAR[PM_PAR_YPOS] = Yo;

    // supply the model-fitted parameters, or copy from the input
    for (int i = 0; i < psf->params->n; i++) {
        if (i == PM_PAR_SKY) continue;
        pmTrend2D *trend = psf->params->data[i];
        PAR[i] = pmTrend2DEval(trend, Xo, Yo);
    }

    // the 2D PSF model fits polarization terms (E0,E1,E2)
    // convert to shape terms (SXX,SYY,SXY)
    // XXX user-defined value for limit?
    bool useReff = pmModelUseReff (model->type);
    if (!pmPSF_FitToModel (PAR, 0.1, useReff)) {
        psError(PM_ERR_PSF, false, "Failed to fit object at (r,c) = (%.1f,%.1f)", Xo, Yo);
        return false;
    }

    // apply the model limits here: this truncates excessive extrapolation
    // XXX do we need to do this still?  should we put in asserts to test?
    for (int i = 0; i < psf->params->n; i++) {
        // apply the limits to all components or just the psf-model parameters?
        if (psf->params->data[i] == NULL)
            continue;

        bool status = true;
        status &= PM_MODEL_LIMITS (PS_MINIMIZE_PARAM_MIN, i, PAR, NULL);
        status &= PM_MODEL_LIMITS (PS_MINIMIZE_PARAM_MAX, i, PAR, NULL);
        if (!status) {
            psTrace ("psModules.objects", 5, "Hitting parameter limits at (r,c) = (%.1f, %.1f)", Xo, Yo);
            model->flags |= PM_MODEL_STATUS_LIMITS;
        }
    }
    return(true);
}

// XXX double-check these definitions below
// this test is invalid if the parameters are derived
// from the PSF model
bool PM_MODEL_FIT_STATUS (pmModel *model)
{
    psF32 dP;
    bool  status;

    psF32 *PAR  = model->params->data.F32;
    psF32 *dPAR = model->dparams->data.F32;

    dP = 0;
    dP += PS_SQR(dPAR[PM_PAR_SXX] / PAR[PM_PAR_SXX]);
    dP += PS_SQR(dPAR[PM_PAR_SYY] / PAR[PM_PAR_SYY]);
    dP = sqrt (dP);

    status = true;
    status &= (dP < 0.5);
    status &= (PAR[PM_PAR_I0] > 0);
    status &= ((dPAR[PM_PAR_I0]/PAR[PM_PAR_I0]) < 0.5);

    if (status)
        return true;
    return false;
}

# undef PM_MODEL_FUNC
# undef PM_MODEL_FLUX
# undef PM_MODEL_GUESS
# undef PM_MODEL_LIMITS
# undef PM_MODEL_RADIUS
# undef PM_MODEL_FROM_PSF
# undef PM_MODEL_PARAMS_FROM_PSF
# undef PM_MODEL_FIT_STATUS

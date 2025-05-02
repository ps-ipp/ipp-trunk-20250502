/******************************************************************************
 * this file defines the RGAUSS source shape model (XXX need a better name!).  Note that these
 * model functions are loaded by pmModelClass.c using 'include', and thus need no 'include'
 * statements of their own.  The models use a psVector to represent the set of parameters, with
 * the sequence used to specify the meaning of the parameter.  The meaning of the parameters
 * may thus vary depending on the specifics of the model.  All models which are used as a PSF
 * representations share a few parameters, for which # define names are listed in pmModel.h:

   power-law with fitted slope
   1 / (1 + z + z^alpha)

 * PM_PAR_SKY 0   - local sky : note that this is unused and may be dropped in the future
 * PM_PAR_I0 1    - central intensity
 * PM_PAR_XPOS 2  - X center of object
 * PM_PAR_YPOS 3  - Y center of object
 * PM_PAR_SXX 4   - X^2 term of elliptical contour (sqrt(2) / SigmaX)
 * PM_PAR_SYY 5   - Y^2 term of elliptical contour (sqrt(2) / SigmaY)
 * PM_PAR_SXY 6   - X*Y term of elliptical contour
 * PM_PAR_7   7   - power-law slope (alpha)
 *****************************************************************************/

#include <stdio.h>
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
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmModel_RGAUSS.h"

# define PM_MODEL_NPARAM          8
# define PM_MODEL_FUNC            pmModelFunc_RGAUSS
# define PM_MODEL_FLUX            pmModelFlux_RGAUSS
# define PM_MODEL_GUESS           pmModelGuess_RGAUSS
# define PM_MODEL_LIMITS          pmModelLimits_RGAUSS
# define PM_MODEL_RADIUS          pmModelRadius_RGAUSS
# define PM_MODEL_SET_FWHM        pmModelSetFWHM_RGAUSS
# define PM_MODEL_FROM_PSF        pmModelFromPSF_RGAUSS
# define PM_MODEL_PARAMS_FROM_PSF pmModelParamsFromPSF_RGAUSS
# define PM_MODEL_FIT_STATUS      pmModelFitStatus_RGAUSS
# define PM_MODEL_SET_LIMITS      pmModelSetLimits_RGAUSS

// the model is a function of the pixel coordinate (pixcoord[0,1] = x,y)
// 0.5 PIX: the parameters are defined in terms of pixel coords, so the incoming pixcoords
// values need to be pixel coords

// Lax parameter limits
static float paramsMinLax[] = { -1.0e3, 1.0e-2, -100, -100, 0.5, 0.5, -1.0, 1.25 };
static float paramsMaxLax[] = { 1.0e5, 1.0e9, 1.0e5, 1.0e5, 100, 100, 1.0, 4.0 };

// Moderate parameter limits
static float *paramsMinModerate = paramsMinLax;
static float *paramsMaxModerate = paramsMaxLax;

// Strict parameter limits
static float *paramsMinStrict = paramsMinLax;
static float *paramsMaxStrict = paramsMaxLax;

// Parameter limits to use
static float *paramsMinUse = paramsMinLax;
static float *paramsMaxUse = paramsMaxLax;
static float betaUse[] = { 1000, 3e6, 5, 5, 0.5, 0.5, 0.5, 0.5 };

static bool limitsApply = true;         // Apply limits?

psF32 PM_MODEL_FUNC (psVector *deriv,
                     const psVector *params,
                     const psVector *pixcoord)
{
    psF32 *PAR = params->data.F32;

    psF32 X  = pixcoord->data.F32[0] - PAR[PM_PAR_XPOS];
    psF32 Y  = pixcoord->data.F32[1] - PAR[PM_PAR_YPOS];
    psF32 px = X / PAR[PM_PAR_SXX];
    psF32 py = Y / PAR[PM_PAR_SYY];
    psF32 z  = PS_SQR(px) + PS_SQR(py) + X*Y*PAR[PM_PAR_SXY];

    assert (z >= 0);

    psF32 p  = pow(z, PAR[PM_PAR_7] - 1.0);
    psF32 r  = 1.0 / (1 + z + z*p);
    psF32 f  = PAR[PM_PAR_I0]*r + PAR[PM_PAR_SKY];

    if (deriv != NULL) {
        psF32 *dPAR = deriv->data.F32;

        // note difference from a pure gaussian: q = params->data.F32[PM_PAR_I0]*r
        psF32 t = PAR[PM_PAR_I0]*r*r;
        psF32 q = t*(1 + PAR[PM_PAR_7]*p);

        dPAR[PM_PAR_SKY] = +1.0;
        dPAR[PM_PAR_I0] = +r;
        dPAR[PM_PAR_XPOS] = q*(2.0*px/PAR[PM_PAR_SXX] + Y*PAR[PM_PAR_SXY]);
        dPAR[PM_PAR_YPOS] = q*(2.0*py/PAR[PM_PAR_SYY] + X*PAR[PM_PAR_SXY]);
        dPAR[PM_PAR_SXX] = +2.0*q*px*px/PAR[PM_PAR_SXX];
        dPAR[PM_PAR_SYY] = +2.0*q*py*py/PAR[PM_PAR_SYY];
        dPAR[PM_PAR_SXY] = -q*X*Y;

        // this model derivative is undefined at z = 0.0, but the limit is zero as z -> 0.0
        dPAR[PM_PAR_7] = (z == 0.0) ? 0.0 : -5.0*t*log(z)*p*z;
    }
    return(f);
}

// define the parameter limits
// AR_MAX is the maximum allowed axis ratio
// AR_RATIO is ((1-R)/(1+R))^2 where R = AR_MAX^(-2)
# define AR_MAX 20.0
# define AR_RATIO 0.99

bool PM_MODEL_LIMITS (psMinConstraintMode mode, int nParam, float *params, float *beta)
{
    if (!limitsApply) {
        return true;
    }
    psAssert(nParam >= 0 && nParam < PM_MODEL_NPARAM, "Parameter index is out of bounds");

    // we need to calculate the limits for SXY specially
    float q2 = NAN;
    if (nParam == PM_PAR_SXY) {
	// NOTE: the factor of 2 is needed to convert par[SXX,SYY] to shape.sx,sy
        float f1 = 2.0 / PS_SQR(params[PM_PAR_SYY]) + 2.0 / PS_SQR(params[PM_PAR_SXX]);
        float f2 = 2.0 / PS_SQR(params[PM_PAR_SYY]) - 2.0 / PS_SQR(params[PM_PAR_SXX]);
        float q1 = PS_SQR(f1)*AR_RATIO - PS_SQR(f2);
        q1 = (q1 < 0.0) ? 0.0 : q1;
        // if q1 < 0.0, f2 ~ f1, we have a very large axis ratio near 45deg..  Saturate at that
        // angle and let f2,f1 fight it out
        q2 = 0.5*sqrtf(q1);
    }

    switch (mode) {
      case PS_MINIMIZE_BETA_LIMIT: {
          psAssert(beta, "Require beta to limit beta");
          float limit = betaUse[nParam];
          if (nParam == PM_PAR_SXY) {
              limit *= q2;
          }
          if (fabs(beta[nParam]) > fabs(limit)) {
              beta[nParam] = (beta[nParam] > 0) ? fabs(limit) : -fabs(limit);
              psTrace("psModules.objects", 5, "|beta[nParam==%d]| > |beta_lim|; %g v. %g",
                      nParam, beta[nParam], limit);
              return false;
          }
          return true;
      }
      case PS_MINIMIZE_PARAM_MIN: {
          psAssert(params, "Require parameters to limit parameters");
          psAssert(paramsMinUse, "Require parameter limits to limit parameters");
          float limit = paramsMinUse[nParam];
          if (nParam == PM_PAR_SXY) {
              limit *= q2;
          }
          if (params[nParam] < limit) {
              params[nParam] = limit;
              psTrace("psModules.objects", 5, "params[nParam==%d] < params_min; %g v. %g",
                      nParam, params[nParam], limit);
              return false;
          }
          return true;
      }
      case PS_MINIMIZE_PARAM_MAX: {
          psAssert(params, "Require parameters to limit parameters");
          psAssert(paramsMaxUse, "Require parameter limits to limit parameters");
          float limit = paramsMaxUse[nParam];
          if (nParam == PM_PAR_SXY) {
              limit *= q2;
          }
          if (params[nParam] > limit) {
              params[nParam] = limit;
              psTrace("psModules.objects", 5, "params[nParam==%d] > params_max; %g v. %g",
                      nParam, params[nParam], limit);
              return false;
          }
          return true;
      }
      default:
        psAbort("invalid choice for limits");
    }
    psAbort("should not reach here");
    return false;
}


// make an initial guess for parameters
// 0.5 PIX: moments and peaks are in pixel coords, thus so are model parameters
bool PM_MODEL_GUESS (pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal)
{
    psF32 *PAR  = model->params->data.F32;

    // sky is set to 0.0
    PAR[PM_PAR_SKY]  = 0.0;

    // set the shape parameters
    // the last parameter is the scaling factor for Moments to shape parameter radius guess
    if (!pmModelSetShape(&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], source->moments, false, 2.0)) {
      return false;
    }

    // set the model normalization
    if (!pmModelSetNorm(&PAR[PM_PAR_I0], source)) {
      return false;
    }

    // set the model position
    if (!pmModelSetPosition(&PAR[PM_PAR_XPOS], &PAR[PM_PAR_YPOS], source)) {
      return false;
    }

    // extra parameter
    PAR[PM_PAR_7]    = 1.5;

    return(true);
}

// integrate the model to get the full flux
psF64 PM_MODEL_FLUX (const psVector *params)
{
    float z, norm;

    psF32 *PAR = params->data.F32;

    psEllipseAxes axes;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], false);
    float AspectRatio = axes.minor / axes.major;

    // flux = 2 \pi \int f(r) r dr
    norm = 0.0;

    # define DR 0.25

    // f = f(r) * r
    float f0 = 0.0;
    float f1, f2;
    for (float r = DR; r < 150; r += DR) {
	z = 0.5 * PS_SQR(r / axes.major);
        f1 = r / (1 + z + pow(z, PAR[PM_PAR_7]));
        r += DR;
	z = 0.5 * PS_SQR(r / axes.major);
        f2 = r / (1 + z + pow(z, PAR[PM_PAR_7]));
        norm += f0 + 4*f1 + f2;
        f0 = f2;
    }
    norm *= DR / 3.0;

    psF64 Flux = PAR[PM_PAR_I0] * norm * 2.0 * M_PI * AspectRatio;

    return(Flux);
}

// define this function so it never returns Inf or NaN
// return the radius which yields the requested flux
psF64 PM_MODEL_RADIUS (const psVector *params, psF64 flux)
{
    psF64 z;

    psF32 *PAR = params->data.F32;

    if (flux <= 0)
        return (1.0);
    if (PAR[PM_PAR_I0] <= 0)
        return (1.0);
    if (flux >= PAR[PM_PAR_I0])
        return (1.0);

    psEllipseAxes axes;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], false);
    psF64 sigma = axes.major;

    psF64 limit = flux / PAR[PM_PAR_I0];

    // use the fact that f is monotonically decreasing
    z = 0;

    // choose a z value guaranteed to be beyond our limit
    float z0 = pow((1.0 / limit), (1.0 / PAR[PM_PAR_7]));
    psAssert (isfinite(z0), "fix this code: z0 should not be nan for Io = %f, flux = %f, major = %f (%f, %f, %f), par 7 = %f", 
	      PAR[PM_PAR_I0], flux, axes.major, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], PAR[PM_PAR_7]);

    float z1 = (1.0 / limit);
    psAssert (isfinite(z1), "fix this code: z1 should not be nan for Io = %f, flux = %f, major = %f (%f, %f, %f), par 7 = %f", 
	      PAR[PM_PAR_I0], flux, axes.major, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], PAR[PM_PAR_7]);

    z1 = PS_MAX (z0, z1);
    z0 = 0.0;

    // starting guess:
    z = 0.5*(z0 + z1);
    float dz = 1.0;

    for (int i = 0; (i < 10) && (fabs(dz) > 0.0001); i++) {
	// use Newton-Raphson to minimize f(z) - limit = 0
	float q = (1 + z + pow(z,PAR[PM_PAR_7]));
	float dqdz = (1.0 + PAR[PM_PAR_7]*pow(z,PAR[PM_PAR_7] - 1.0));

	float f = 1.0 / q;
	float dfdz = -dqdz * f / q;

	dz = (f - limit) / dfdz;

	// fprintf (stderr, "%f %f %f : %f %f\n", f, z, dz, dfdz, q);
	z -= dz;
	z = PS_MAX(z, 0.0);
    }

    psF64 radius = sigma * sqrt (2.0 * z);

    if (isnan(radius))
        psAbort("error in code: radius is NaN");

    return (radius);
}

psF64 PM_MODEL_SET_FWHM (const psVector *params, psF64 sigma) {
  return (NAN);
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

    // the 2D PSF model fits polarization terms (E0,E1,E2)
    // convert to shape terms (SXX,SYY,SXY)
    bool useReff = modelPSF->class->useReff;
    if (!pmPSF_FitToModel (out, 0.1, useReff)) {
        psTrace("psModules.objects", 5, "Failed to fit object at (r,c) = (%.1f,%.1f)", in[PM_PAR_YPOS], in[PM_PAR_XPOS]);
        return false;
    }

    // apply the model limits here: this truncates excessive extrapolation
    // XXX do we need to do this still?  should we put in asserts to test?
    for (int i = 0; i < psf->params->n; i++) {
        // apply the limits to all components or just the psf-model parameters?
        if (psf->params->data[i] == NULL)
            continue;

        bool status = true;
        status &= PM_MODEL_LIMITS(PS_MINIMIZE_PARAM_MIN, i, out, NULL);
        status &= PM_MODEL_LIMITS(PS_MINIMIZE_PARAM_MAX, i, out, NULL);
        if (!status) {
            psTrace ("psModules.objects", 5, "Hitting parameter limits at (r,c) = (%.1f, %.1f)",
                     in[PM_PAR_XPOS], in[PM_PAR_YPOS]);
            modelPSF->flags |= PM_MODEL_STATUS_LIMITS;
        }
    }

    return true;
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
        if (i == PM_PAR_I0) continue;
        if (i == PM_PAR_XPOS) continue;
        if (i == PM_PAR_YPOS) continue;
        pmTrend2D *trend = psf->params->data[i];
        PAR[i] = pmTrend2DEval(trend, Xo, Yo);
    }

    // the 2D PSF model fits polarization terms (E0,E1,E2)
    // convert to shape terms (SXX,SYY,SXY)
    bool useReff = model->class->useReff;
    if (!pmPSF_FitToModel (PAR, 0.1, useReff)) {
        psTrace ("psModules.objects", 3, "Failed to fit object at (r,c) = (%.1f,%.1f)", Xo, Yo);
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

bool PM_MODEL_FIT_STATUS (pmModel *model)
{
    bool  status;

    psF32 *PAR  = model->params->data.F32;
    psF32 *dPAR = model->dparams->data.F32;

    status = true;
    status &= (PAR[PM_PAR_I0] > 0);
    status &= ((dPAR[PM_PAR_I0]/PAR[PM_PAR_I0]) < 0.5);

    return status;
}


void PM_MODEL_SET_LIMITS(pmModelLimitsType type)
{
    switch (type) {
      case PM_MODEL_LIMITS_NONE:
        paramsMinUse = NULL;
        paramsMaxUse = NULL;
        limitsApply = true;
        break;
      case PM_MODEL_LIMITS_IGNORE:
        paramsMinUse = NULL;
        paramsMaxUse = NULL;
        limitsApply = false;
        break;
      case PM_MODEL_LIMITS_LAX:
        paramsMinUse = paramsMinLax;
        paramsMaxUse = paramsMaxLax;
        limitsApply = true;
        break;
      case PM_MODEL_LIMITS_MODERATE:
        paramsMinUse = paramsMinModerate;
        paramsMaxUse = paramsMaxModerate;
        limitsApply = true;
        break;
      case PM_MODEL_LIMITS_STRICT:
        paramsMinUse = paramsMinStrict;
        paramsMaxUse = paramsMaxStrict;
        limitsApply = true;
        break;
      default:
        psAbort("Unrecognised model limits type: %x", type);
    }
    return;
}

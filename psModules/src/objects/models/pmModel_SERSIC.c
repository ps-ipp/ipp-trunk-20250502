/******************************************************************************
 * this file defines the SERSIC source shape model.  Note that these model functions are loaded
 * by pmModelClass.c using 'include', and thus need no 'include' statements of their own.  The
 * models use a psVector to represent the set of parameters, with the sequence used to specify
 * the meaning of the parameter.  The meaning of the parameters may thus vary depending on the
 * specifics of the model.  All models which are used as a PSF representations share a few
 * parameters, for which # define names are listed in pmModel.h:

   f = exp(-z^n)

   * PM_PAR_SKY 0   - local sky : note that this is unused and may be dropped in the future
   * PM_PAR_I0 1    - central intensity
   * PM_PAR_XPOS 2  - X center of object
   * PM_PAR_YPOS 3  - Y center of object
   * PM_PAR_SXX 4   - X^2 term of elliptical contour (SigmaX / sqrt(2))
   * PM_PAR_SYY 5   - Y^2 term of elliptical contour (SigmaY / sqrt(2))
   * PM_PAR_SXY 6   - X*Y term of elliptical contour
   * PM_PAR_7   7   - normalized sersic parameter

   * note that a Sersic model is usually defined in terms of R_e, the half-light radius.  This
     construction does not include a factor of 2 in the X^2 term, etc, like for a Gaussian.
     Conversion from SXX, SYY, SXY to R_major, R_minor, theta can be done by using:
     shape.sx = SXX / sqrt(2), shape.sy = SYY / sqrt(2), shape.sxy = SXY, then calling
     psEllipseShapeToAxes, and multiplying the values of axes.major, axes.minor by sqrt(2)

   note that a standard sersic model uses exp(-K*(z^(1/2n) - 1).  the additional elements (K,
   the -1 offset) are absorbed in this model by the normalization, the exponent, and the
   radial scale.  We fit the elements in this form, then re-normalize them on output.
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
#include "pmModel_CentralPixel.h"

#include "pmModel_SERSIC.h"

# define PM_MODEL_NPARAM          8
# define PM_MODEL_FUNC            pmModelFunc_SERSIC
# define PM_MODEL_FLUX            pmModelFlux_SERSIC
# define PM_MODEL_GUESS           pmModelGuess_SERSIC
# define PM_MODEL_LIMITS          pmModelLimits_SERSIC
# define PM_MODEL_RADIUS          pmModelRadius_SERSIC
# define PM_MODEL_SET_FWHM        pmModelSetFWHM_SERSIC
# define PM_MODEL_FROM_PSF        pmModelFromPSF_SERSIC
# define PM_MODEL_PARAMS_FROM_PSF pmModelParamsFromPSF_SERSIC
# define PM_MODEL_FIT_STATUS      pmModelFitStatus_SERSIC
# define PM_MODEL_SET_LIMITS      pmModelSetLimits_SERSIC

// the model is a function of the pixel coordinate (pixcoord[0,1] = x,y)
// 0.5 PIX: the parameters are defined in terms of pixel coords, so the incoming pixcoords
// values need to be pixel coords

// Lax parameter limits
static float paramsMinLax[] = { -1.0e3, 1.0e-2, -100, -100, 0.001, 0.001, -1.0, 0.0625 };
static float paramsMaxLax[] = { 1.0e5, 1.0e9, 1.0e5, 1.0e5, 100, 100, 1.0, 1.0 };

// Moderate parameter limits
static float *paramsMinModerate = paramsMinLax;
static float *paramsMaxModerate = paramsMaxLax;

// Strict parameter limits
static float *paramsMinStrict = paramsMinLax;
static float *paramsMaxStrict = paramsMaxLax;

// Parameter limits to use
static float *paramsMinUse = paramsMinLax;
static float *paramsMaxUse = paramsMaxLax;
static float betaUse[] = { 2, 3e6, 5, 5, 10.0, 10.0, 0.5, 1.0};

static bool limitsApply = true;         // Apply limits?

// # include "pmModel_SERSIC.CP.h"

psF32 PM_MODEL_FUNC (psVector *deriv,
                     const psVector *params,
                     const psVector *pixcoord)
{
    psF32 *PAR = params->data.F32;

    psF32 X  = pixcoord->data.F32[0] - PAR[PM_PAR_XPOS];
    psF32 Y  = pixcoord->data.F32[1] - PAR[PM_PAR_YPOS];
    psF32 px = X / PAR[PM_PAR_SXX];
    psF32 py = Y / PAR[PM_PAR_SYY];
    psF32 z  = PS_SQR(px) + PS_SQR(py) + PAR[PM_PAR_SXY]*X*Y;

    // If the elliptical contour is defined in a valid way, we should never trigger this
    // assert.  Other models (like PGAUSS) don't use fractional powers, and thus do not have
    // NaN values for negative values of z
    psAssert (z >= 0, "do not allow negative z values in model");

    float Sindex = 0.5 / PAR[PM_PAR_7];
    float kappa = pmSersicKappa (Sindex);

    float q = kappa*pow(z,PAR[PM_PAR_7]);
    psF32 f0 = exp(-q);

    assert (isfinite(q));

    psF32 radius = hypot(X, Y);
    if (radius <= 1.5) {
	// Nsub ~ 10*index^2 + 1
	psEllipseAxes axes = pmPSF_ModelToAxes(PAR, true); // SERSIC model uses Reff
	int Nsub = 2 * ((int)(6.0*Sindex / axes.minor)) + 1;
	Nsub = PS_MIN (Nsub, 121);
	Nsub = PS_MAX (Nsub, 11);
	f0 = pmModelCP_SersicSubpix (X, Y, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], Sindex, Nsub);
    }
    if (!isfinite(f0)) {
        fprintf(stderr, "f0 is not finite for %f %f %f %f %f.  Parameters: \n", X, Y, radius, z, q);
        fprintf(stderr, "%f %f %f %f %f %f %f %f\n", PAR[0], PAR[1], PAR[2], PAR[3], PAR[4],
            PAR[5], PAR[6], PAR[7]);
    }
    assert (isfinite(f0));

    psF32 f1 = PAR[PM_PAR_I0]*f0;
    psF32 f = PAR[PM_PAR_SKY] + f1;

    assert (isfinite(f1));
    assert (isfinite(f));

    if (deriv != NULL) {
        psF32 *dPAR = deriv->data.F32;

        dPAR[PM_PAR_SKY]  = +1.0;
        dPAR[PM_PAR_I0]   = +f0;

	if (z > 0.01) {
	  float z1 = f1*kappa*PAR[PM_PAR_7]*pow(z,PAR[PM_PAR_7]-1.0);
	  dPAR[PM_PAR_XPOS] = +1.0*z1*(2.0*px + Y*PAR[PM_PAR_SXY]);
	  dPAR[PM_PAR_YPOS] = +1.0*z1*(2.0*py + X*PAR[PM_PAR_SXY]);
	  dPAR[PM_PAR_SXX]  = +2.0*z1*px*px/PAR[PM_PAR_SXX];
	  dPAR[PM_PAR_SYY]  = +2.0*z1*py*py/PAR[PM_PAR_SYY];
	  dPAR[PM_PAR_SXY]  = -1.0*z1*X*Y;
	  dPAR[PM_PAR_7]    = -1.0*f1*q*log(z);
	} else {
	  // gradient -> 0 for z -> 0, but has undef form
	  float z1 = f1*kappa*PAR[PM_PAR_7]*pow(z,PAR[PM_PAR_7]);
	  dPAR[PM_PAR_XPOS] = +1.0*z1*(2.0/PAR[PM_PAR_SXX] + PAR[PM_PAR_SXY]);
	  dPAR[PM_PAR_YPOS] = +1.0*z1*(2.0/PAR[PM_PAR_SYY] + PAR[PM_PAR_SXY]);
	  dPAR[PM_PAR_SXX]  = +2.0*z1*px/PAR[PM_PAR_SXX]/PAR[PM_PAR_SXX];
	  dPAR[PM_PAR_SYY]  = +2.0*z1*py/PAR[PM_PAR_SYY]/PAR[PM_PAR_SYY];
	  dPAR[PM_PAR_SXY]  = -1.0*z1;
	  // dPAR[PM_PAR_7]    = -1.0*f1*q*log(z + 0.0001);
	  dPAR[PM_PAR_7]    = -1.0*f1*q*log(z + 0.0001); // factor of 16 to reduce the gain
	}
        assert (isfinite(dPAR[PM_PAR_7]));
    }
    return (f);
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
        float f1 = 1.0 / PS_SQR(params[PM_PAR_SYY]) + 1.0 / PS_SQR(params[PM_PAR_SXX]);
        float f2 = 1.0 / PS_SQR(params[PM_PAR_SYY]) - 1.0 / PS_SQR(params[PM_PAR_SXX]);
        float q1 = PS_SQR(f1)*AR_RATIO - PS_SQR(f2);
        q1 = (q1 < 0.0) ? 0.0 : q1;
        // if q1 < 0.0, f2 ~ f1, we have a very large axis ratio near 45deg..  Saturate at that
        // angle and let f2,f1 fight it out
	// NOTE: the factor of 2 is needed to convert par[SXX,SYY] to shape.sx,sy
        q2 = 2.0*0.5*sqrtf(q1);
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
    pmMoments *moments = source->moments;
    psF32     *PAR  = model->params->data.F32;

    // sky is set to 0.0
    PAR[PM_PAR_SKY]  = 0.0;

    // the other parameters depend on the guess for PAR_7
    if (!isfinite(PAR[PM_PAR_7])) {
	PAR[PM_PAR_7]    = 0.25;
    }    
    float index = 0.5 / PAR[PM_PAR_7];

    // the scale-length is a function of the moments and the index:
    // Rmajor_guess = Rmajor_moments * Scale * Zero
    float Scale = 0.70 + 0.053 * PAR[PM_PAR_7];
    float Zero  = 1.16 - 0.615 * PAR[PM_PAR_7];

    // Sersic shape is a bit special
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
    float Rmajor = PS_MAX(1.0, Scale * axes.major + Zero);
    float Rminor = Rmajor * (axes.minor / axes.major);

    // fprintf (stderr, "guess index: %f : %f, %f -> %f, %f\n", index, axes.major, axes.minor, Rmajor, Rminor);

    axes.major = Rmajor;
    axes.minor = Rminor;

    pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], axes, true);

    float bn = 1.9992*index - 0.3271;
    // float fR = 1.0 / (sqrt(2.0) * pow (bn, index));
    float Io = exp(0.5*bn);

    // set the model normalization (adjust for Sersic best guess)
    if (!pmModelSetNorm(&PAR[PM_PAR_I0], source)) {
      return false;
    }
    PAR[PM_PAR_I0] /= Io;

    // set the model position
    if (!pmModelSetPosition(&PAR[PM_PAR_XPOS], &PAR[PM_PAR_YPOS], source)) {
      return false;
    }

    return(true);
}

// A sersic model has an integral flux which can be analytically represented
psF64 PM_MODEL_FLUX (const psVector *params)
{
    psF32 *PAR = params->data.F32;

    psEllipseAxes axes;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], true);

    float Sindex = 0.5 / PAR[PM_PAR_7];
    float norm = pmSersicNorm (Sindex);

    float flux = PAR[PM_PAR_I0] * 2.0 * M_PI * axes.major * axes.minor * norm;

    return(flux);
}

// define this function so it never returns Inf or NaN
// return the radius which yields the requested flux
psF64 PM_MODEL_RADIUS (const psVector *params, psF64 flux)
{
    psF32 *PAR = params->data.F32;

    if (flux <= 0)
        return (1.0);
    if (PAR[PM_PAR_I0] <= 0)
        return (1.0);
    if (flux >= PAR[PM_PAR_I0])
        return (1.0);

    psEllipseAxes axes;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], true);

    float Sindex = 0.5 / PAR[PM_PAR_7];
    float kappa = pmSersicKappa (Sindex);

    // f = Io exp(-z^n) -> z^n = ln(Io/f)
    psF64 zn = log(PAR[PM_PAR_I0] / flux) / kappa;
    psF64 radius = axes.major * pow(zn, Sindex);

    psAssert (isfinite(radius), "fix this code: radius should not be nan for Io = %f, flux = %f, major = %f (%f, %f, %f), par 7 = %f", 
	      PAR[PM_PAR_I0], flux, axes.major, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], PAR[PM_PAR_7]);
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
    // XXX user-defined value for limit?
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

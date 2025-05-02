/******************************************************************************
 * this file defines the TRAIL source shape model.  This represents an infinitely thin
 * line of length convolved with a Gaussian PSF.  The models use a psVector to represent
 * the set of parameters, with the sequence used to specify the meaning of the parameter.
 * The meaning of the parameters may thus vary depending on the specifics of the model.
 * All models which are used as a PSF representations share a few parameters, for which #
#include "pmModelClass.h"
 * define names are listed in pmModel.h:

   pure Gaussian:
   exp(-z)

 * PM_PAR_SKY 0    - local sky : note that this is unused and may be dropped in the future
 * PM_PAR_I0 1     - flux normalization
 * PM_PAR_XPOS 2   - X center of object
 * PM_PAR_YPOS 3   - Y center of object
 * PM_PAR_LENGTH 4 - trail length
 * PM_PAR_THETA 5  - position angle
 * PM_PAR_SIGMA 6  - PSF Gaussian sigma (not fitted?)
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

#include "pmModel_TRAIL.h"

# define PM_MODEL_NPARAM          7
# define PM_MODEL_FUNC            pmModelFunc_TRAIL
# define PM_MODEL_FLUX            pmModelFlux_TRAIL
# define PM_MODEL_GUESS           pmModelGuess_TRAIL
# define PM_MODEL_LIMITS          pmModelLimits_TRAIL
# define PM_MODEL_RADIUS          pmModelRadius_TRAIL
# define PM_MODEL_SET_FWHM        pmModelSetFWHM_TRAIL
# define PM_MODEL_FROM_PSF        pmModelFromPSF_TRAIL
# define PM_MODEL_PARAMS_FROM_PSF pmModelParamsFromPSF_TRAIL
# define PM_MODEL_FIT_STATUS      pmModelFitStatus_TRAIL
# define PM_MODEL_SET_LIMITS      pmModelSetLimits_TRAIL

// Lax parameter limits 
static float paramsMinLax[] = { -1.0e3, 1.0e-2, -1.0e2, -1.0e2,   0.5, -3.3, -0.5 };
static float paramsMaxLax[] = {  1.0e5, 1.00+9, +1.0e5, +1.0e5, 150.0, +3.3 , 5.0 };

// Moderate parameter limits
static float *paramsMinModerate = paramsMinLax;
static float *paramsMaxModerate = paramsMaxLax;

// Strict parameter limits
static float *paramsMinStrict = paramsMinLax;
static float *paramsMaxStrict = paramsMaxLax;

// Parameter limits to use
static float *paramsMinUse = paramsMinLax;
static float *paramsMaxUse = paramsMaxLax;
static float betaUse[] = { 1000, 3e6, 5, 5, 2.0, 0.1, 0.1 };

static bool limitsApply = true;         // Apply limits?

// the model is a function of the pixel coordinate (pixcoord[0,1] = x,y)
// 0.5 PIX: the parameters are defined in terms of pixel coords, so the incoming pixcoords
// values need to be pixel coords
psF32 PM_MODEL_FUNC(psVector *deriv,
                    const psVector *params,
                    const psVector *pixcoord)
{
    psF32 *PAR = params->data.F32;

    psF32 X  = pixcoord->data.F32[0] - PAR[PM_PAR_XPOS];
    psF32 Y  = pixcoord->data.F32[1] - PAR[PM_PAR_YPOS];
    psF32 ST = sin(PAR[PM_PAR_THETA]);
    psF32 CT = cos(PAR[PM_PAR_THETA]);

    psF32 S2 = 2.0 * PS_SQR(PAR[PM_PAR_SIGMA]);

    psF32 Zp = (X*CT + Y*ST + 0.5*PAR[PM_PAR_LENGTH]) / sqrt(S2);
    psF32 Zm = (X*CT + Y*ST - 0.5*PAR[PM_PAR_LENGTH]) / sqrt(S2);

    // psF32 Zp = (X*CT + Y*ST + 0.5*PAR[PM_PAR_LENGTH]) / sqrt(S2);
    // psF32 Zm = (X*CT + Y*ST - 0.5*PAR[PM_PAR_LENGTH]) / sqrt(S2);

    psF32 Ep = erf(Zp);
    psF32 Em = erf(Zm);

    psF32 Rxy = Y*CT - X*ST;
    psF32 Gxy = exp(-Rxy*Rxy/S2);

    psF32 Pxy = Gxy * (Ep - Em);
    psF32 f = Pxy * PAR[PM_PAR_I0] + PAR[PM_PAR_SKY];

    if (deriv != NULL) {
        psF32 *dPAR = deriv->data.F32;

        dPAR[PM_PAR_SKY]    = 1.0;
        dPAR[PM_PAR_I0]     = Pxy;

	float dGdR = -2.0 * Rxy * Gxy / S2; // -R Gxy / (2 Sigma^2)

	// are these signs correct? I think so: (dR/dXo = -dR/dX); dRdX below is actually dR/dXo
	float dRdX = +ST;
	float dRdY = -CT;
	float dRdT = -Y*ST - X*CT;

	float dGdX = dGdR * dRdX;
	float dGdY = dGdR * dRdY;
	float dGdT = dGdR * dRdT;

	// are these signs correct? I think so: (dR/dXo = -dR/dX); dRdX below is actually dR/dXo
	float dZpdX = -CT / sqrt(S2);
	float dZmdX = dZpdX; // float dZmdX = -CT / sqrt(S2); dZmdX = dZpdX

	float dZpdY = -ST / sqrt(S2); // float dZmdY = -ST / sqrt(S2); dZmdY = dZpdY
	float dZmdY = dZpdY;

	float dZpdL = +0.5 / sqrt(S2);
	float dZmdL = -0.5 / sqrt(S2);

	float dZpdT = (-X*ST + Y*CT) / sqrt(S2);
	float dZmdT = dZpdT; // dZpdT = dZmdT

	float dEdZp = exp (-Zp*Zp) * M_2_SQRTPI;
	float dEdZm = exp (-Zm*Zm) * M_2_SQRTPI;

	float dEpdX = dEdZp * dZpdX;
	float dEmdX = dEdZm * dZmdX;

	float dEpdY = dEdZp * dZpdY;
	float dEmdY = dEdZm * dZmdY;

	float dEpdL = dEdZp * dZpdL;
	float dEmdL = dEdZm * dZmdL;

	float dEpdT = dEdZp * dZpdT;
	float dEmdT = dEdZm * dZmdT;

	float dPdX = dGdX * (Ep - Em) + Gxy * (dEpdX - dEmdX);
	float dPdY = dGdY * (Ep - Em) + Gxy * (dEpdY - dEmdY);

	// dGdL is 0.0 because dRdL is 0.0
	float dPdL = Gxy * (dEpdL - dEmdL);

	float dPdT = dGdT * (Ep - Em) + Gxy * (dEpdT - dEmdT);

        dPAR[PM_PAR_XPOS]   = PAR[PM_PAR_I0] * dPdX;
        dPAR[PM_PAR_YPOS]   = PAR[PM_PAR_I0] * dPdY;

        dPAR[PM_PAR_LENGTH] = PAR[PM_PAR_I0] * dPdL;
        dPAR[PM_PAR_THETA]  = PAR[PM_PAR_I0] * dPdT;
        dPAR[PM_PAR_SIGMA]  = 0;	// we don't actually allow this to vary, so we do not need to calculate it

	//	for (int i = 0; i < 7; i++) {
	//	  if (isnan(dPAR[i])) {
	//	    fprintf (stderr, "*");
	//	  }
	//	}
    }
    //    if (isnan(f)) {
    //      fprintf (stderr, "!");
    //    }
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

    switch (mode) {
      case PS_MINIMIZE_BETA_LIMIT: {
          psAssert(beta, "Require beta to limit beta");
          float limit = betaUse[nParam];
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

# define NA 21
# define NR 21
static float flux[NA][NR];
static float npix[NA][NR];

bool pmTrailGetAngle (float *To, float *Io, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal, float sigma) {

  float Xo, Yo;
  if (!pmModelSetPosition(&Xo, &Yo, source)) return false;

  psImage *image = source->pixels;
  psImage *mask = source->maskObj;
  psF32 **imData = image->data.F32;
  psImageMaskType **mkData = mask->data.PS_TYPE_IMAGE_MASK_DATA;

  // do a loop over the pixels, generating (dX,dY) dot (cos(theta),sin(theta))
  int dP;
  int dX = Xo - mask->col0;
  dP = mask->numCols - dX;
  int DX = PS_MAX(dP, dX);
  int NX = mask->numCols;

  int dY = Yo - mask->row0;
  dP = mask->numRows - dY;
  int DY = PS_MAX(dP, dY);
  int NY = mask->numRows;

  // just hard wire this for now...
  float radius = 10.0;
  float radius2 = PS_SQR(radius);

  // we have an array of Angles x Radii
  float dT = M_PI / NA;
  for (int na = 0; na < NA; na++) {
    memset (flux[na], 0, NR*sizeof(float));
    memset (npix[na], 0, NR*sizeof(float));
  }

  // we skip any pixels [real or virtual] outside of the specified radius (nominally the aperture radius)
  // ix and iy track pixels relative to the centroid
  for (int ix = -DX; ix < DX + 1; ix++) {
    if (ix > radius) continue;
    int mx = ix + dX;
    for (int iy = -DY; iy < DY + 1; iy++) {
      if (iy > radius) continue;
      if (ix*ix + iy*iy > radius2) continue;
      int my = iy + dY;
      
      // include count only the unmasked pixels within the image area
      if (mx < 0) continue;
      if (my < 0) continue;
      if (mx >= NX) continue;
      if (my >= NY) continue;
      
      // count pixels which are masked only with bad pixels
      if (mkData[my][mx] & maskVal)continue;

      // we have defined NA to be 21
      int na = 0;
      for (float angle = 0.0; na < NA; angle += dT, na ++) {

	// XXX optimization : pre-compute the angle sines and cosines
	float Rad = (ix * cos(angle)) + (iy * sin(angle));
	int nr = PS_MAX (PS_MIN (NR, Rad + 0.5*NR), 0);

	flux[na][nr] += imData[my][mx];
	npix[na][nr] ++;
      }
    }
  }

  // generate a psf model (with integral of 1.0)
  float Ao = 1.0 / sqrt(2*M_PI) / sigma;
  float psf[NR];
  for (int nr = 0; nr < NR; nr++) {
    psf[nr] = Ao*exp(-0.5*PS_SQR((nr - 0.5*NR)/sigma));
  }

  float fangle[NA];
  for (int na = 0; na < NA; na++) {
    float fpsf = 0.0;
    for (int nr = 0; nr < NR; nr++) {
      fpsf += psf[nr]*flux[na][nr];
    }
    fangle[na] = fpsf;
    // fprintf (stderr, "fpsf: %f, theta = %f\n", fpsf, PS_DEG_RAD*dT*na);
  }

  float peak = fangle[0];
  int pbin = 0;
  for (int na = 0; na < NA; na++) {
    if (fangle[na] > peak) {
      peak = fangle[na];
      pbin = na;
    }
  }

  // result is peak, pbin -> angle
  *To = dT * pbin;
  *Io = peak * Ao;
  
  return true;
}

// make an initial guess for parameters
// 0.5 PIX: moments and peaks are in pixel coords, thus so are model parameters
bool PM_MODEL_GUESS (pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal)
{
    psF32 *PAR  = model->params->data.F32;

    // sky is set to 0.0
    PAR[PM_PAR_SKY]  = 0.0;

    psF32 *psfPAR  = source->modelPSF->params->data.F32;
    bool useReff = source->modelPSF->class->useReff;

    psEllipseAxes psfAxes;
    pmModelParamsToAxes (&psfAxes, psfPAR[PM_PAR_SXX], psfPAR[PM_PAR_SXY], psfPAR[PM_PAR_SYY], useReff);

    psEllipseMoments emoments;
    emoments.x2 = source->moments->Mxx;
    emoments.xy = source->moments->Mxy;
    emoments.y2 = source->moments->Myy;

    // force the axis ratio to be < 20.0
    psEllipseAxes axes = psEllipseMomentsToAxes (emoments, 20.0);

    if (!isfinite(axes.major)) return false;
    if (!isfinite(axes.minor)) return false;
    if (!isfinite(axes.theta)) return false;

    float size = NAN;
    if (!isfinite(source->moments->Mrf)) {
      size = axes.major;
    } else {
      size = (axes.major > sqrt(source->moments->Mrf)) ? axes.major : sqrt(source->moments->Mrf);
    }

    float theta, peak;
    pmTrailGetAngle (&theta, &peak, source, maskVal, markVal, psfAxes.major);

    // axes.major is a sigma in the major direction; scale to 
    PAR[PM_PAR_LENGTH] = 1.5*2.35*size; // a tophat of length L has L = 1.5 * 2.35 * sigma
    PAR[PM_PAR_THETA] = axes.theta; // theta in radians
    PAR[PM_PAR_SIGMA] = psfAxes.major; // psf major axes (sigma of the psf)

    // set the model normalization
    // if (!pmModelSetNorm(&PAR[PM_PAR_I0], source)) {
    //   return false;
    // }
    PAR[PM_PAR_I0] = peak;

    // set the model position
    if (!pmModelSetPosition(&PAR[PM_PAR_XPOS], &PAR[PM_PAR_YPOS], source)) {
       return false;
    }

    return(true);
}

psF64 PM_MODEL_FLUX (const psVector *params)
{
    psF32 *PAR = params->data.F32;
    psF64 Flux = PAR[PM_PAR_I0] * PAR[PM_PAR_LENGTH] * PAR[PM_PAR_SIGMA] * 2.0 * sqrt(2.0 * M_PI);
    return(Flux);
}

// return the radius which yields the requested flux
// this function is never allowed to return <= 0
psF64 PM_MODEL_RADIUS (const psVector *params, psF64 flux)
{
    psF32 *PAR = params->data.F32;

    // PAR_LENGTH is the unconvolved length.  add a bit for safety
    return (0.5*PAR[PM_PAR_LENGTH] + 2);
}

psF64 PM_MODEL_SET_FWHM (const psVector *params, psF64 sigma) {
  return (NAN);
}

// construct the PSF model from the FLT model and the psf
bool PM_MODEL_FROM_PSF (pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf)
{
    psWarning ("do you really want to use a trail as a PSF model??");
    return false;
}

// generate a model based on a the psf model
bool PM_MODEL_PARAMS_FROM_PSF (pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io)
{
    psWarning ("do you really want to use a trail as a PSF model??");
    return false;
}

// check the status of the fitted model
// this test is invalid if the parameters are derived
// from the PSF model
// XXX how is this used?  it prevents forced photometry from ever being 'successful'
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

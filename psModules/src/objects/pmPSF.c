/** @file  pmPSF.c
 *
 * This file contains typedefs for the Point-Spread Function and prototypes
 * for functions that calculate the PSF.
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.39 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/

#include <strings.h>  // for strcasecmp
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAMaskWeight.h"
#include "psVectorBracket.h"

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

#include "pmErrorCodes.h"


#define MAX_AXIS_RATIO 20.0             // Maximum axis ratio for PSF model

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

static void pmPSFOptionsFree (pmPSFOptions *options) {

    if (!options) return;

    psFree (options->stats);
    psFree (options->fitOptions);
    return;
}

pmPSFOptions *pmPSFOptionsAlloc (void) {

    pmPSFOptions *options = (pmPSFOptions *) psAlloc(sizeof(pmPSFOptions));
    psMemSetDeallocator(options, (psFreeFunc) pmPSFOptionsFree);

    options->type          = 0;
    options->stats         = NULL;

    options->psfTrendMode  = PM_TREND_NONE;
    options->psfTrendNx    = 0;
    options->psfTrendNy    = 0;
    options->psfFieldNx    = 0;
    options->psfFieldNy    = 0;
    options->psfFieldXo    = 0;
    options->psfFieldYo    = 0;

    options->poissonErrorsPhotLMM = true;
    options->poissonErrorsPhotLin = false;
    options->poissonErrorsParams  = true;

    options->chiFluxTrend = true;
    options->fitOptions    = NULL; // XXX this has to be set before calling pmPSF fit functions

    options->fitRadius = NAN;
    options->apRadius = NAN;
    return options;
}

bool psMemCheckPSFOptions(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmPSFOptionsFree);
}

/*****************************************************************************
pmPSFFree(psf): function to free a pmPSF structure
 *****************************************************************************/
static void pmPSFFree (pmPSF *psf)
{
    if (psf == NULL) {
        return;
    }

    psFree (psf->ChiTrend);
    psFree (psf->psfTrendStats);
    psFree (psf->ApTrend);
    psFree (psf->FluxScale);
    psFree (psf->growth);
    psFree (psf->params);
    psFree (psf->residuals);
    return;
}

/*****************************************************************************
 pmPSFAlloc (type): allocate a pmPSF.

 NOTE: PSF model parameters which are not modeled on an image are set to NULL
 in psf->params.

 These are normally:

 X-center
 Y-center
 Sky background value
 Object Normalization
 *****************************************************************************/
pmPSF *pmPSFAlloc (const pmPSFOptions *options)
{
    PS_ASSERT_PTR_NON_NULL(options, NULL);
    int Nparams;

    pmPSF *psf = (pmPSF *) psAlloc(sizeof(pmPSF));
    psMemSetDeallocator(psf, (psFreeFunc) pmPSFFree);

    psf->type     = options->type;

    psf->chisq    = 0.0;
    psf->ApResid  = 0.0;
    psf->dApResid = 0.0;
    psf->skyBias  = 0.0;
    psf->skySat   = 0.0;
    psf->nPSFstars  = 0;
    psf->nApResid   = 0;

    psf->poissonErrorsPhotLMM = options->poissonErrorsPhotLMM;
    psf->poissonErrorsPhotLin = options->poissonErrorsPhotLin;
    psf->poissonErrorsParams = options->poissonErrorsParams;

    // the ApTrend components are (x, y).  It may be represented with a polynomial or with a
    // psImageMap.  We set it initially to NULL. the user must allocate it before using.
    // psf->ApTrend = pmTrend2DAlloc (PM_TREND_MAP, Nx, Ny, 1, 1, stats);
    psf->ApTrend = NULL;

    // the flux scale is the relationship between the integrated flux and the peak flux for a
    // psf model.  this is a 2D function of position, and is modeled with pmTrend2D after the
    // psf model is determined for an image.  until it is determined, the flux calculation
    // integrates the sources
    psf->FluxScale = NULL;

    if (psf->poissonErrorsPhotLMM) {
        psf->ChiTrend = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 1);
    } else {
        psf->ChiTrend = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 2);
    }

    // don't define a growth curve : user needs to choose radius bins
    psf->growth = NULL;

    // by default, we do not construct the residual image
    psf->residuals = NULL;

    Nparams = pmModelClassParameterCount (options->type);
    if (!Nparams) {
        psError(PS_ERR_UNKNOWN, true, "Undefined pmModelType");
        return(NULL);
    }
    psf->params = psArrayAlloc(Nparams);

    // save the trend stats on the psf for use in pmPSFFromPSFtry
    psf->psfTrendStats = psMemIncrRefCounter (options->stats);

    // the psf parameters may have 2D variations represented as either a polynomial (ordinary
    // or chebychev) or as an image map.  The size of the image map is determined by pmPSFtry
    // by minimizing the scatter in the fitted data and the rms error in the complete image
    // map.  In this case, the user-supplied options of psfTrendNx and psfTrendNy are the
    // maximum value used for these axes.  For the polynomial terms, the order is not currently
    // set, dynamically.  The value of psfTrendNx and psfTrendNy are used.

    psImageBinning *binning = psImageBinningAlloc();
    binning->nXruff = options->psfTrendNx;
    binning->nYruff = options->psfTrendNy;
    binning->nXfine = options->psfFieldNx;
    binning->nYfine = options->psfFieldNy;

    // for polynomial representations, nXruff, nYruff are the order number, and may be 0.
    // in this case, we cannot set the psImageBinning scale because 0 would be invalid.  these
    // elements are only used by the image map representation
    if (options->psfTrendMode == PM_TREND_MAP) {
        psImageBinningSetScale (binning, PS_IMAGE_BINNING_CENTER);
        psImageBinningSetSkipByOffset (binning, options->psfFieldXo, options->psfFieldYo);
    }

    // trendNx & trendNy are used in pmPSFtry as the max for these values
    psf->psfTrendMode = options->psfTrendMode;
    psf->trendNx      = options->psfTrendNx;
    psf->trendNy      = options->psfTrendNy;
    psf->fieldNx      = options->psfFieldNx;
    psf->fieldNy      = options->psfFieldNy;
    psf->fieldXo      = options->psfFieldXo;
    psf->fieldYo      = options->psfFieldYo;

    // define the parameter trends
    if (options->psfTrendMode != PM_TREND_NONE) {
        for (int i = 0; i < psf->params->n; i++) {
            if (i == PM_PAR_SKY) continue;
            if (i == PM_PAR_I0) continue;
            if (i == PM_PAR_XPOS) continue;
            if (i == PM_PAR_YPOS) continue;

            psf->params->data[i] = pmTrend2DNoImageAlloc (options->psfTrendMode, binning, options->stats);
        }
    }
    psFree (binning);
    return psf;
}

bool psMemCheckPSF(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmPSFFree);
}

// the PSF models the \sigma_{xy} variation of the elliptical contour as a function of position in the image with a
// polynomial.  an individual object has a contour of the form (x^2/2sx^2) + (y^2/2sy^2) + sxy*x*y
// these are the values of the model->params.  the psf->params term for sxy is actually fitted
// to sxy/(sxx^-2 + syy^-2)^2

// XXX this is only an approximate solution.  A better solution would be to fit the second moment, Mxy. the
// problem here is that converting from Mxy,SXX,SYY -> SXY is a third order problem:
// Mxy = SXY * (SXX^-4 + SYY^-4 - 2 SXY ^2)

// XXX deprecated
// input: model->param, output: psf->param[PM_PAR_SXY]
// XXX double pmPSF_SXYfromModel (psF32 *modelPar)
// XXX {
// XXX     PS_ASSERT_PTR_NON_NULL(modelPar, NAN);
// XXX 
// XXX     double SXX = modelPar[PM_PAR_SXX];
// XXX     double SYY = modelPar[PM_PAR_SYY];
// XXX     double SXY = modelPar[PM_PAR_SXY];
// XXX 
// XXX     double par = SXY / PS_SQR(1.0 / PS_SQR(SXX) + 1.0 / PS_SQR(SYY));
// XXX     return (par);
// XXX }

// XXX deprecated
// input: fitted psf->param, output: model->param[PM_PAR_SXY]
// XXX double pmPSF_SXYtoModel (psF32 *fittedPar)
// XXX {
// XXX     PS_ASSERT_PTR_NON_NULL(fittedPar, NAN);
// XXX 
// XXX     double SXX = fittedPar[PM_PAR_SXX];
// XXX     double SYY = fittedPar[PM_PAR_SYY];
// XXX     double fit = fittedPar[PM_PAR_SXY];
// XXX 
// XXX     double SXY = fit * PS_SQR(1.0 / PS_SQR(SXX) + 1.0 / PS_SQR(SYY));
// XXX 
// XXX     assert (!isnan(SXY));
// XXX 
// XXX     return SXY;
// XXX }

// The PSF modelling function fits the polarization terms e0, e1, e2:

// the FIT is the 2D representation of the shape using polarization parameters for the elliptical contour
// the MODEL is the realized psf model for a given location

// convert the parameters (in situ) used in the fitted source model to the parameters used in
// the 2D PSF model
bool pmPSF_FitToModel (psF32 *fittedPar, float minMinorAxis, bool useReff)
{
    PS_ASSERT_PTR_NON_NULL(fittedPar, false);

    psEllipsePol pol;

    pol.e0 = fittedPar[PM_PAR_E0];
    pol.e1 = fittedPar[PM_PAR_E1];
    pol.e2 = fittedPar[PM_PAR_E2];

    psEllipseAxes axes = psEllipsePolToAxes (pol, minMinorAxis);
    if (!isfinite(axes.major) || !isfinite(axes.minor) || !isfinite(axes.theta)) {
        psTrace("psModules.objects", 5, "Failed to convert e[012] (%g,%g,%g) to axes", pol.e0, pol.e1, pol.e2);
        return false;
    }

    pmModelAxesToParams (&fittedPar[PM_PAR_SXX], &fittedPar[PM_PAR_SXY], &fittedPar[PM_PAR_SYY], axes, useReff);
    return true;
}

// convert the parameters (in situ) used in the 2D PSF model fit into the parameters used in
// the source model
psEllipsePol pmPSF_ModelToFit (psF32 *modelPar, bool useReff)
{
    // must assert non-NULL input parameter
    psEllipsePol pol;
    pol.e0 = NAN;
    pol.e1 = NAN;
    pol.e2 = NAN;
    PS_ASSERT_PTR_NON_NULL(modelPar, pol);

    psEllipseAxes axes;
    pmModelParamsToAxes (&axes, modelPar[PM_PAR_SXX], modelPar[PM_PAR_SXY], modelPar[PM_PAR_SYY], useReff);

    pol = psEllipseAxesToPol (axes);

    return pol;
}

// convert the parameters used in the fitted source model to the psEllipseAxes representation
// (major,minor,theta)
psEllipseAxes pmPSF_ModelToAxes (psF32 *modelPar, bool useReff)
{
    psEllipseAxes axes;
    axes.major = NAN;
    axes.minor = NAN;
    axes.theta = NAN;

    PS_ASSERT_PTR_NON_NULL(modelPar, axes);

    pmModelParamsToAxes (&axes, modelPar[PM_PAR_SXX], modelPar[PM_PAR_SXY], modelPar[PM_PAR_SYY], useReff);
    return axes;
}

// convert the psEllipseAxes representation (major,minor,theta) to the parameters used in the
// fitted source model
bool pmPSF_AxesToModel (psF32 *modelPar, psEllipseAxes axes, bool useReff)
{
    PS_ASSERT_PTR_NON_NULL(modelPar, false);

    modelPar[PM_PAR_SXX] = 0.0;
    modelPar[PM_PAR_SYY] = 0.0;
    modelPar[PM_PAR_SXY] = 0.0;
    
    if ((axes.major <= 0) || (axes.minor <= 0)) {
        return true;
    }
    
    pmModelAxesToParams (&modelPar[PM_PAR_SXX], &modelPar[PM_PAR_SXY], &modelPar[PM_PAR_SYY], axes, useReff);
    return true;
}

// generate a psf model of the requested type, with fixed shape
pmPSF *pmPSFBuildSimple (char *typeName, float sxx, float syy, float sxy, ...)
{

    va_list ap;
    va_start(ap, sxy);

    pmPSFOptions *options = pmPSFOptionsAlloc ();
    options->type = pmModelClassGetType (typeName);
    options->psfTrendMode = PM_TREND_POLY_ORD;
    options->psfTrendNx = 0;
    options->psfTrendNy = 0;

    pmPSF *psf = pmPSFAlloc (options);

    psVector *par = psVectorAlloc (psf->params->n, PS_TYPE_F32);
    par->data.F32[PM_PAR_SXX] = sxx;
    par->data.F32[PM_PAR_SYY] = syy;
    par->data.F32[PM_PAR_SXY] = sxy;

    bool useReff = pmModelUseReff (options->type);
    psEllipsePol pol = pmPSF_ModelToFit(par->data.F32, useReff);

    pmTrend2D *trend = NULL;

    // set the psf shape parameters
    trend = psf->params->data[PM_PAR_E0];
    trend->poly->coeff[0][0] = pol.e0;

    trend = psf->params->data[PM_PAR_E1];
    trend->poly->coeff[0][0] = pol.e1;

    trend = psf->params->data[PM_PAR_E2];
    trend->poly->coeff[0][0] = pol.e2;

    for (int i = PM_PAR_SXY + 1; i < psf->params->n; i++) {
        trend = psf->params->data[i];
        trend->poly->coeff[0][0] = (psF32)va_arg(ap, psF64);
    }
    va_end(ap);

    psFree (par);
    psFree (options);
    return psf;
}


float pmPSFtoFWHM(const pmPSF *psf, float x, float y)
{
    PS_ASSERT_PTR_NON_NULL(psf, NAN);

    pmModel *model = pmModelFromPSFforXY(psf, x, y, 1.0); // Model of source
    if (!model) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine PSF model at %f,%f\n", x, y);
        return NAN;
    }

    // get the model full-width at half-max
    float fwhmMajor = 2*model->class->modelRadius (model->params, 0.5);

# if (0)
    psF32 *params = model->params->data.F32; // Model parameters
    psEllipseAxes axes = pmPSF_ModelToAxes(params, MAX_AXIS_RATIO, model->class->useReff); // Ellipse axes

    // Curiously, the minor axis can be larger than the major axis, so need to check.
    float fwhm = 2.355 * PS_MAX(axes.minor, axes.major); // FWHM, converted from sigma

    psFree(model);

    return fwhm;
# else

    psFree(model);

    return fwhmMajor;
# endif
}

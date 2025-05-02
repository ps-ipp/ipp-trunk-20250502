/** @file  pmSourcePhotometry.c
 *
 *  @author EAM, IfA; GLG, MHPCC
 *
 *  @version $Revision: 1.50 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:28:54 $
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
#include "pmErrorCodes.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAMaskWeight.h"

#include "pmConfigMask.h"
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

#include "pmSourcePhotometry.h"

# define DO_SKY 0

static float AP_MIN_SN = 0.0;

// make this a bit more clever and dynamic
static psImageMaskType maskSuspect   = 0;
static psImageMaskType maskSpike     = 0;
static psImageMaskType maskStarCore  = 0;
static psImageMaskType maskBurntool  = 0;
static psImageMaskType maskConvPoor  = 0;
static psImageMaskType maskGhost     = 0;
static psImageMaskType maskGlint     = 0;
static psImageMaskType maskCrosstalk = 0;
static psImageMaskType maskCTE       = 0;

bool pmSourceMagnitudesInit (pmConfig *config, psMetadata *recipe)
{
    PS_ASSERT_PTR_NON_NULL(recipe, false);
    bool status;

    // we are going to test specially against these poor values
    if (config) {
	maskSpike     = pmConfigMaskGet("SPIKE", config);
	maskStarCore  = pmConfigMaskGet("STARCORE", config);
	maskBurntool  = pmConfigMaskGet("BURNTOOL", config);
	maskConvPoor  = pmConfigMaskGet("CONV.POOR", config);
	maskGhost     = pmConfigMaskGet("GHOST", config);
	maskGlint     = pmConfigMaskGet("GHOST", config);
	maskCrosstalk = pmConfigMaskGet("CROSSTALK", config);
	maskCTE       = pmConfigMaskGet("CTE", config);
	maskSuspect   = maskSpike | maskStarCore | maskBurntool | maskConvPoor;
    }

    float limit = psMetadataLookupF32 (&status, recipe, "AP_MIN_SN");
    if (status) {
        AP_MIN_SN = limit;
    }
    return true;
}

/**
   this function is used to calculate the three defined source magnitudes:
   - apMag  : only if S/N > AP_MIN_SN
   : is optionally corrected for curve-of-growth if:
   - the option is selected (mode & PM_SOURCE_PHOT_GROWTH)
   - psfMag : all sources with non-NULL modelPSF
   : is optionally corrected for aperture residual if:
   - the option is selected (mode & PM_SOURCE_PHOT_APCORR)
   - extMag : all sources with non-NULL modelEXT
**/

// XXX masked region should be (optionally) elliptical
// if mode is PM_SOURCE_PHOT_PSFONLY, we skip all other magnitudes
bool pmSourceMagnitudes (pmSource *source, pmPSF *psf, pmSourcePhotometryMode mode, psImageMaskType maskVal, psImageMaskType markVal, float radius)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    // PS_ASSERT_PTR_NON_NULL(psf, false);

    int status = false;
    float x, y;
    float SN;

    source->psfMag    = NAN;
    source->extMag    = NAN;
    source->psfMagErr = NAN;
    source->apMag     = NAN;
    source->apMagRaw  = NAN;
    source->apFlux    = NAN;
    source->apFluxErr = NAN;

    pmModelStatus badModel = PM_MODEL_STATUS_NONE;
    badModel |= PM_MODEL_STATUS_BADARGS;
    badModel |= PM_MODEL_STATUS_OFFIMAGE;
    badModel |= PM_MODEL_STATUS_NAN_CHISQ;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GUESS;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GRID;
    badModel |= PM_MODEL_PCM_FAIL_GUESS;

    // XXXXXX review:
    // Select the 'best' model -- this is used for PSF_QF,_PERFECT & ???. isPSF is true if this
    // object is a PSF (not extended).  We must have a valid model.  XXX NOTE: allow aperture
    // magnitudes for sources without a model

    // select the psf model
    pmModel *modelPSF = source->modelPSF;
    if (modelPSF == NULL) {
        psTrace ("psModules.objects", 3, "fail mag : no valid PSF model");
        return false;
    }

    // get the error on the PSF model magnitude
    if (modelPSF->dparams->data.F32[PM_PAR_I0] > 0) {
        SN = fabs(modelPSF->params->data.F32[PM_PAR_I0] / modelPSF->dparams->data.F32[PM_PAR_I0]);
        source->psfMagErr = 1.0 / SN;
    } else {
        SN = NAN;
        source->psfMagErr = NAN;
    }
    // the source position is used to recenter the aperture for ap photometry
    x = modelPSF->params->data.F32[PM_PAR_XPOS];
    y = modelPSF->params->data.F32[PM_PAR_YPOS];

    // measure PSF model photometry
    status = pmSourcePhotometryModel (&source->psfMag, &source->psfFlux, modelPSF);
    source->psfFluxErr = fabs(source->psfFlux * source->psfMagErr);

# if (0)
    // XXX NOTE: old code to use the flux scale.  test & turn this back on?  if so, need to save with psf model
    // the source peak pixel is guaranteed to be on the image, and only minimally different from the source center
    double fluxScale = pmTrend2DEval (psf->FluxScale, (float)source->peak->x, (float)source->peak->y);
    psAssert (isfinite(fluxScale), "how can the flux scale be invalid? source at %d, %d\n", source->peak->x, source->peak->y);
    psAssert (fluxScale > 0.0, "how can the flux scale be negative? source at %d, %d\n", source->peak->x, source->peak->y);
    source->psfFlux = fluxScale * modelPSF->params->data.F32[PM_PAR_I0];
    source->psfFluxErr = fluxScale * modelPSF->dparams->data.F32[PM_PAR_I0];
    source->psfMag = -2.5*log10(source->psfFlux);
# endif

    if (mode == PM_SOURCE_PHOT_PSFONLY) {
	return true;
    }

    // get the EXT model photometry (all EXT models)
    // if we have a collection of model fits, check if one of them is a pointer to modelEXT
    if (source->modelFits) {
        bool foundEXT = false;
        for (int i = 0; i < source->modelFits->n; i++) {
            pmModel *model = source->modelFits->data[i];
	    if (model->flags & badModel) continue;
            status = pmSourcePhotometryModel (&model->mag, NULL, model);
            if (model == source->modelEXT) foundEXT = true;
	    float SN = fabs(model->params->data.F32[PM_PAR_I0] / model->dparams->data.F32[PM_PAR_I0]);
	    model->magErr = 1.0 / SN;
        }
        if (foundEXT) {
            source->extMag = source->modelEXT->mag;
        } else {
            status = pmSourcePhotometryModel (&source->extMag, NULL, source->modelEXT);
        }
    } else {
        if (source->modelEXT) {
            status = pmSourcePhotometryModel (&source->extMag, NULL, source->modelEXT);
        }
    }

    // Correct psfMag to match aperture magnitude system (NOTE : Growth curve is already applied to ApTrend)
    if ((mode & PM_SOURCE_PHOT_APCORR) && psf && psf->ApTrend) {
        // the source peak pixel is guaranteed to be on the image, and only minimally different from the source center
        double apTrend = pmTrend2DEval (psf->ApTrend, (float)source->peak->x, (float)source->peak->y);
        source->psfMag += apTrend;
	source->psfFlux *= pow(10.0, -0.4*apTrend);
	source->psfFluxErr *= pow(10.0, -0.4*apTrend);
    }

    // measure the contribution of included pixels to the PSF model fit
    if (mode & PM_SOURCE_PHOT_WEIGHT) {
        pmSourcePixelWeight (source, modelPSF, source->maskObj, maskVal, radius);
    }

    // measure the contribution of included pixels
    if (mode & PM_SOURCE_PHOT_DIFFSTATS) {
        pmSourceMeasureDiffStats (source, maskVal, markVal);
    }

    pmSourceNeighborFlags (source);

    // measure the aperture magnitude, if (SN > AP_MIN_SN)
    if (!isfinite(SN)) {
        psTrace ("psModules.objects", 3, "fail mag : bad SN: %f (limit: %f)", SN, AP_MIN_SN);
        return false;
    }

    // measure the aperture magnitude, if (SN > AP_MIN_SN)
    if (SN < AP_MIN_SN) {
        psTrace ("psModules.objects", 3, "skip ap mag : SN < limit : %f vs %f)", SN, AP_MIN_SN);
        return true;
    }

    // if we measure aperture magnitudes, the source must not currently be subtracted!
    psAssert (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED), "cannot measure ap mags if source is subtracted!");

    // if we are measuring aperture photometry and applying the growth correction,
    // we need to shift the flux in the selected pixels (but not the mask)
    psImage *flux = NULL;
    psImage *variance = NULL;
    psImage *mask = NULL; // Star flux and mask images, to photometer
    if (mode & PM_SOURCE_PHOT_INTERP) {
        float dx = 0.5 - x + (int)x;
        float dy = 0.5 - y + (int)y;
        x += dx;
        y += dy;

        // if (!psImageShiftMask(&flux, &mask, source->pixels, source->maskObj, maskVal, dx, dy, NAN, 0xff, PS_INTERPOLATE_LANCZOS2)) {
	// if (!psImageShiftMask(&flux, &mask, source->pixels, source->maskObj, maskVal, dx, dy, NAN, 0xff, PS_INTERPOLATE_BIQUADRATIC)) {
	if (!psImageShiftMask(&flux, &mask, source->pixels, source->maskObj, maskVal, dx, dy, NAN, 0xff, PS_INTERPOLATE_BILINEAR)) {
            // Not much we can do about it
            psErrorClear();
            psTrace ("psModules.objects", 3, "fail shift");
            return false;
        }
	// NOTE : previous to r36860, we failed to pass a variance to pmSourcePhotometryAperSource, making apFluxErr wrong.
	// I should interpolate the source variance to match the signal, but failing to do so only makes a tiny 
	// (~1-2%) error in the variance, and thus only a small error in the error on the aperture flux.  
        variance = source->variance;
    } else {
        flux = source->pixels;
        variance = source->variance;
        mask = source->maskObj;
    }

    // measure object aperture photometry
    status = pmSourcePhotometryAperSource (source, modelPSF, flux, variance, mask, maskVal);
    if (!status) {
        psTrace ("psModules.objects", 3, "fail mag : bad Ap Mag");
    }

    // for PSFs, correct both apMag and psfMag to same system, consistent with infinite flux star in aperture RADIUS
    // if the aper mag is NAN, the flux < 0.  this can happen for sources near the
    // detection limits (esp near bright neighbors)
    source->apMag = source->apMagRaw;
    if (isfinite (source->apMag) && psf) {
        if (psf->growth && (mode & PM_SOURCE_PHOT_GROWTH)) {
	    float apOffset = pmGrowthCurveCorrect (psf->growth, source->apRadius);
            source->apMag = source->apMagRaw + apOffset;
	    source->apFlux *= pow(10.0, -0.4*apOffset);
	    source->apFluxErr *= pow(10.0, -0.4*apOffset);
        }
    }
    if (mode & PM_SOURCE_PHOT_INTERP) {
        psFree(flux);
        psFree(mask);
    }

    return status;
}

/*
  aprMag' - fitMag = flux*skySat + r^2*rflux*skyBias + ApTrend(x,y)
  (aprMag - flux*skySat - r^2*rflux*skyBias) - fitMAg = ApTrend(x,y)
  (aprMag - flux*skySat - r^2*rflux*skyBias) = fitMAg + ApTrend(x,y)

*/

bool pmSourceNeighborFlags (pmSource *source) {

    return false;

    // source must have a peak to have a footprint
    if (!source) return false;
    if (!source->peak) return false;
    if (!source->peak->footprint) return false;
    if (!source->peak->footprint->peaks) return false;
    if (!source->peak->footprint->peaks->n) return false;

    // find the brightest peak (first peak)
    pmPeak *brightPeak = source->peak->footprint->peaks->data[0];

    // are we the brightest peak?
    if (source->peak == brightPeak) return true;

    // if not, raise a flag:
    source->mode2 |= PM_SOURCE_MODE2_HAS_BRIGHTER_NEIGHBOR;

    // but, this is a common situation.  more interesting is if the ratio flux_n / (r^2 flux_o) is large

    float radius2 = PS_SQR(source->peak->xf - brightPeak->xf) + PS_SQR(source->peak->yf - brightPeak->yf);

    float ratio = brightPeak->rawFlux / (source->peak->rawFlux * radius2);

    if (ratio > 1) {
	source->mode2 |= PM_SOURCE_MODE2_BRIGHT_NEIGHBOR_1;
    }

    if (ratio > 10) {
	source->mode2 |= PM_SOURCE_MODE2_BRIGHT_NEIGHBOR_10;
    }

    return true;
}

// return source model magnitude
bool pmSourcePhotometryModel (float *fitMag, float *fitFlux, pmModel *model)
{
    psAssert (fitMag || fitFlux, "at least one of magnitude or flux must be requested (not NULL)");
    if (model == NULL) return false;

    float mag  = NAN;
    float flux = NAN;

    // measure fitMag
    flux = model->class->modelFlux (model->params);
    if (flux > 0) {
        mag = -2.5*log10(flux);
    }
    if (fitMag) {
        *fitMag = mag;
    }
    if (fitFlux) {
        *fitFlux = flux;
    }

    if (flux <= 0) return false;
    if (!isfinite(flux)) return false;

    return (true);
}

// return source aperture magnitude
bool pmSourcePhotometryAperSource (pmSource *source, pmModel *model, psImage *image, psImage *variance, psImage *mask, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(image, false);
    PS_ASSERT_PTR_NON_NULL(mask, false);

    if (DO_SKY) {
	PS_ASSERT_PTR_NON_NULL(model, false);
    }

    bool status;
    status = pmSourcePhotometryAper(&source->apNpixels, &source->apMagRaw, &source->apFlux, &source->apFluxErr, model, image, variance, mask, maskVal);
    if (status) {
	source->mode |= PM_SOURCE_MODE_AP_MAGS;
    }
    return status;
}

// return source aperture magnitude
bool pmSourcePhotometryAper (int *nPixOut, float *apMag, float *apFluxOut, float *apFluxErr, pmModel *model, psImage *image, psImage *variance, psImage *mask, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(apMag, false);
    PS_ASSERT_PTR_NON_NULL(image, false);
    PS_ASSERT_PTR_NON_NULL(mask, false);

    if (DO_SKY) {
	PS_ASSERT_PTR_NON_NULL(model, false);
    }

    float sky = 0;
    float apFlux = 0;
    float apFluxVar = 0;
    int nPix = 0;

    if (DO_SKY) {
        sky = model->params->data.F32[PM_PAR_SKY];
    }

    psF32 **imData = image->data.F32;
    psImageMaskType **mkData = mask->data.PS_TYPE_IMAGE_MASK_DATA;
    psF32 **varData = (variance) ? variance->data.F32 : image->data.F32; // if variance is not supplied, assume gain of 1.0, no read noise

    // measure apFlux and apFluxVar, save apMag if not NAN
    // XXX note that these fluxes/mags are uncorrected for masked pixels
    // XXX raise a bit if the aperture has a masked pixel (not marked)?
    for (int iy = 0; iy < image->numRows; iy++) {
	for (int ix = 0; ix < image->numCols; ix++) {
            if (mkData[iy][ix] & maskVal) continue;
            apFlux += imData[iy][ix] - sky;
            apFluxVar += varData[iy][ix];
	    nPix ++;
        }
    }
    
    if (apFluxOut) *apFluxOut = apFlux;
    if (apFluxErr) *apFluxErr = sqrt(fabs(apFluxVar));
    if (nPixOut) *nPixOut = nPix;

    if (apFlux <= 0) {
        *apMag = NAN;
    } else {
	*apMag = -2.5*log10(apFlux);
    }
    return true;
}

// return source aperture magnitude
bool pmSourcePixelWeight (pmSource *source, pmModel *model, psImage *mask, psImageMaskType maskVal, float radius)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    source->pixWeightNotBad = NAN;
    source->pixWeightNotPoor = NAN;

    PS_ASSERT_PTR_NON_NULL(mask, false);
    PS_ASSERT_PTR_NON_NULL(model, false);

    float modelSum = 0;
    float notBadSum = 0;
    float notPoorSum = 0;
    float sky = 0;
    float value;

    float spikeSum = 0;
    float starcoreSum = 0;
    float burntoolSum = 0;
    float convpoorSum = 0;
    float ghostSum = 0;
    float cteSum = 0;

    int Xo, Yo, dP;
    int dX, DX, NX;
    int dY, DY, NY;

    float radius2 = PS_SQR(radius);

    // we only care about the value of the object model, not the local sky
    if (DO_SKY) {
        sky = model->params->data.F32[PM_PAR_SKY];
    } else {
        sky = 0;
    }

    // the model function returns the source flux at a position
    psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

    psVector *params = model->params;

    Xo = params->data.F32[PM_PAR_XPOS];
    Yo = params->data.F32[PM_PAR_YPOS];

    dX = Xo - mask->col0;
    dP = mask->numCols - dX;
    DX = PS_MAX(dX, dP);
    NX = mask->numCols;

    dY = Yo - mask->row0;
    dP = mask->numRows - dY;
    DY = PS_MAX(dY, dP);
    NY = mask->numRows;

    psImageMaskType maskBad = maskVal;
    maskBad &= ~maskSuspect;

    psImageMaskType maskPoor = maskVal | maskSuspect;

    // measure modelSum and validSum.  this function is applied to a sources' subimage.  the
    // value of DX is chosen (see above) to cover the full possible size of the subimage if it
    // were not by an edge; ie, if the source is cut in half by an image edge, we correctly
    // count the virtual pixels off the edge in normalizing the value of the pixWeight

    // we skip any pixels [real or virtual] outside of the specified radius (nominally the aperture radius)
    for (int ix = -DX; ix < DX + 1; ix++) {
	if (ix > radius) continue;
        int mx = ix + dX;
        for (int iy = -DY; iy < DY + 1; iy++) {
	    if (iy > radius) continue;
	    if (ix*ix + iy*iy > radius2) continue;
            int my = iy + dY;

            coord->data.F32[0] = (psF32) (ix + Xo);
            coord->data.F32[1] = (psF32) (iy + Yo);

            // for the full model, add all points
            value = fabs(model->class->modelFunc (NULL, params, coord) - sky);
            modelSum += value;

            // include count only the unmasked pixels within the image area
            if (mx < 0) continue;
            if (my < 0) continue;
            if (mx >= NX) continue;
            if (my >= NY) continue;

	    // count pixels which are masked only with bad pixels
            if (!(mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskBad)) {
		notBadSum += value;
	    }

	    // count pixels which are masked with an mask bit (bad or poor)
            if (!(mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskPoor)) {
		notPoorSum += value;
	    }

	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskSpike) {
		spikeSum += value;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskStarCore) {
		starcoreSum += value;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskGhost) {
		ghostSum += value;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskBurntool) {
		burntoolSum += value;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskConvPoor) {
		convpoorSum += value;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskCTE) {
		cteSum += value;
	    }

        }
    }
    psFree (coord);

    source->pixWeightNotBad  = notBadSum  / modelSum;
    source->pixWeightNotPoor = notPoorSum / modelSum;

    if ((spikeSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_SPIKE;
    }
    if ((starcoreSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_STARCORE;
    }
    if ((ghostSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_GHOST;
    }
    if ((burntoolSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_BURNTOOL;
    }
    if ((convpoorSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_CONVPOOR;
    }
    if ((cteSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_CTE;
    }

    if (isfinite(source->pixWeightNotBad) && isfinite(source->pixWeightNotPoor)) {
	psAssert (source->pixWeightNotPoor <= source->pixWeightNotBad, "error: all bad pixels should also be poor");
    }

    // Check that if the peak is on/off a ghost, glint, or diffraction spike.  In regular IPP
    // processing, these values are only set in the image mask after the 'camera' stage

    int xChip = source->peak->x;
    int yChip = source->peak->y;

    // need to access the parent if we are looking at a subimage (likely)
    psImage *chipImage = (source->pixels == NULL) ? source->pixels : (psImage *) source->pixels->parent;

    bool onChip = true;
    onChip &= (xChip >= 0);
    onChip &= (xChip < chipImage->numCols);
    onChip &= (yChip >= 0);
    onChip &= (yChip < chipImage->numRows);
    if (!onChip) {
	// if the source is off the edge of the chip, raise a different bit?
	source->mode |= PM_SOURCE_MODE_OFF_CHIP;
    } else {
	int xMask = xChip - mask->col0;
	int yMask = yChip - mask->row0;
	psImageMaskType maskValue = mask->data.PS_TYPE_IMAGE_MASK_DATA[yMask][xMask];
	if (maskValue & maskGhost) {
	    source->mode |= PM_SOURCE_MODE_ON_GHOST;
	}
	pmSourceMode PM_SOURCE_MODE_ON_GLINT = PM_SOURCE_MODE_ON_GHOST;
	if (maskValue & maskGlint) {
	    source->mode |= PM_SOURCE_MODE_ON_GLINT;
	}
	if (maskValue & maskSpike) {
	    source->mode |= PM_SOURCE_MODE_ON_SPIKE;
	}
	if (maskValue & maskCrosstalk) {
	    source->mode2 |= PM_SOURCE_MODE2_ON_CROSSTALK;
	}
    }
    return (true);
}

// return source aperture magnitude
bool pmSourceMaskEval (pmSource *source, psImage *mask, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    source->pixWeightNotBad = NAN;
    source->pixWeightNotPoor = NAN;

    PS_ASSERT_PTR_NON_NULL(mask, false);

    float modelSum = 0;
    float spikeSum = 0;
    float starcoreSum = 0;
    float burntoolSum = 0;
    float convpoorSum = 0;
    float ghostSum = 0;
    float cteSum = 0;

    int Xo, Yo, dP;
    int dX, DX, NX;
    int dY, DY, NY;

    float radius=10.;
    float radius2 = PS_SQR(radius);

    // the model function returns the source flux at a position
    psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

    Xo = source->peak->x;
    Yo = source->peak->y;

    dX = Xo - mask->col0;
    dP = mask->numCols - dX;
    DX = PS_MAX(dX, dP);
    NX = mask->numCols;

    dY = Yo - mask->row0;
    dP = mask->numRows - dY;
    DY = PS_MAX(dY, dP);
    NY = mask->numRows;

    psImageMaskType maskBad = maskVal;
    maskBad &= ~maskSuspect;

    // were not by an edge; ie, if the source is cut in half by an image edge, we correctly
    // count the virtual pixels off the edge in normalizing the value of the pixWeight

    // we skip any pixels [real or virtual] outside of the specified radius (nominally the aperture radius)
    for (int ix = -DX; ix < DX + 1; ix++) {
	if (ix > radius) continue;
        int mx = ix + dX;
        for (int iy = -DY; iy < DY + 1; iy++) {
	    if (iy > radius) continue;
	    if (ix*ix + iy*iy > radius2) continue;
            int my = iy + dY;

            coord->data.F32[0] = (psF32) (ix + Xo);
            coord->data.F32[1] = (psF32) (iy + Yo);

            modelSum += 1.;
            // include count only the unmasked pixels within the image area
            if (mx < 0) continue;
            if (my < 0) continue;
            if (mx >= NX) continue;
            if (my >= NY) continue;

	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskSpike) {
		spikeSum += 1.;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskStarCore) {
		starcoreSum += 1.;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskGhost) {
		ghostSum += 1.;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskBurntool) {
		burntoolSum += 1.;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskConvPoor) {
		convpoorSum += 1.;
	    }
	    // count pixels which are masked with an mask bit (bad or poor)
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[my][mx] & maskCTE) {
                cteSum += 1.;
            }
        }
    }
    psFree (coord);

    if ((spikeSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_SPIKE;
    }
    if ((starcoreSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_STARCORE;
    }
    if ((ghostSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_GHOST;
    }
    if ((burntoolSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_BURNTOOL;
    }
    if ((convpoorSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_CONVPOOR;
    }
    if ((cteSum/modelSum) > 0.25) {
	source->mode2 |= PM_SOURCE_MODE2_ON_CTE;
    }

    // Check that if the peak is on/off a ghost, glint, or diffraction spike.  In regular IPP
    // processing, these values are only set in the image mask after the 'camera' stage

    // need to access the parent if we are looking at a subimage (likely)
    psImage *chipImage = (source->pixels == NULL) ? source->pixels : (psImage *) source->pixels->parent;

    bool onChip = true;
    onChip &= (Xo >= 0);
    onChip &= (Xo < chipImage->numCols);
    onChip &= (Yo >= 0);
    onChip &= (Yo < chipImage->numRows);
    if (!onChip) {
	// if the source is off the edge of the chip, raise a different bit?
	source->mode |= PM_SOURCE_MODE_OFF_CHIP;
    } else {
	int xMask = Xo - mask->col0;
	int yMask = Yo - mask->row0;
	psImageMaskType maskValue = mask->data.PS_TYPE_IMAGE_MASK_DATA[yMask][xMask];
	if (maskValue & maskGhost) {
	    source->mode |= PM_SOURCE_MODE_ON_GHOST;
	}
	pmSourceMode PM_SOURCE_MODE_ON_GLINT = PM_SOURCE_MODE_ON_GHOST;
	if (maskValue & maskGlint) {
	    source->mode |= PM_SOURCE_MODE_ON_GLINT;
	}
	if (maskValue & maskCrosstalk) {
	    source->mode2 |= PM_SOURCE_MODE2_ON_CROSSTALK;
	}
	if (maskValue & maskSpike) {
	    source->mode |= PM_SOURCE_MODE_ON_SPIKE;
	}
    }
    return (true);
}

# define FLUX_LIMIT 3.0

// measure stats that may be used in difference images for distinguishing real sources from bad residuals
bool pmSourceMeasureDiffStats (pmSource *source, psImageMaskType maskVal, psImageMaskType markVal)
{
    PS_ASSERT_PTR_NON_NULL(source, false);

    if (source->diffStats == NULL) {
        source->diffStats = pmSourceDiffStatsAlloc();
    }

    float fGood = 0.0;
    float fBad  = 0.0;
    int   nGood = 0;
    int   nMask = 0;
    int   nBad  = 0;

    psImage *flux     = source->pixels;
    psImage *variance = source->variance;
    psImage *mask     = source->maskObj;

    if (!flux || !variance || !mask) {
        return false;
    }

    // NOTE: until 2010.10.01, these measurements included a 3sigma-per-pixel significance
    // this followed what we understood as the definition given to us
    // by Armin, but it always seemed a poor idea -- a faint source is unlikely to have any 3sigma pixels.
    // changed to remove the per-pixel filter.

    for (int iy = 0; iy < flux->numRows; iy++) {
        for (int ix = 0; ix < flux->numCols; ix++) {
	    // only count up the stats in the unmarked region (ie, the aperture)
	    // skip the marked pixels; these are not relevant
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & markVal) {
                continue;
            }
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) {
                nMask ++;
                continue;
            }

            float value = flux->data.F32[iy][ix];

            if (value > 0.0) {
                nGood ++;
                fGood += fabs(value);
            } else {
                nBad ++;
                fBad += fabs(value);
            }
        }
    }

    source->diffStats->nGood      = nGood;
    source->diffStats->fRatio     = (fGood + fBad         == 0.0) ? NAN : fGood / (fGood + fBad);
    source->diffStats->nRatioBad  = (nGood + nBad         == 0)   ? NAN : nGood / (float) (nGood + nBad);
    source->diffStats->nRatioMask = (nGood + nMask        == 0)   ? NAN : nGood / (float) (nGood + nMask);
    source->diffStats->nRatioAll  = (nGood + nMask + nBad == 0)   ? NAN : nGood / (float) (nGood + nMask + nBad);

    return (true);
}

# if (0)
double pmSourceCrossProduct (const pmSource *Mi,
                             const pmSource *Mj,
                             const bool unweighted_sum) // should the cross product be weighted?
{
    PS_ASSERT_PTR_NON_NULL(Mi, NAN);
    PS_ASSERT_PTR_NON_NULL(Mj, NAN);

    int Xs, Xe, Ys, Ye;
    int xi, xj, yi, yj;
    int xIs, xJs, yIs, yJs;
    int xIe, yIe;
    double flux, wt;

    const psImage *Pi = Mi->pixels;
    assert (Pi != NULL);
    const psImage *Pj = Mj->pixels;
    assert (Pj != NULL);

    const psImage *Wi = Mi->variance;
    if (!unweighted_sum) {
        assert (Wi != NULL);
    }

    const psImage *Ti = Mi->maskObj;
    assert (Ti != NULL);
    const psImage *Tj = Mj->maskObj;
    assert (Tj != NULL);

    Xs = PS_MAX (Pi->col0, Pj->col0);
    Xe = PS_MIN (Pi->col0 + Pi->numCols, Pj->col0 + Pj->numCols);

    Ys = PS_MAX (Pi->row0, Pj->row0);
    Ye = PS_MIN (Pi->row0 + Pi->numRows, Pj->row0 + Pj->numRows);

    xIs = Xs - Pi->col0;
    xJs = Xs - Pj->col0;
    yIs = Ys - Pi->row0;
    yJs = Ys - Pj->row0;

    xIe = Xe - Pi->col0;
    yIe = Ye - Pi->row0;

    // note that this is addressing the same image pixels,
    // though only if both are source not model images
    flux = 0;
    for (yi = yIs, yj = yJs; yi < yIe; yi++, yj++) {
        for (xi = xIs, xj = xJs; xi < xIe; xi++, xj++) {
            if (Ti->data.PS_TYPE_IMAGE_MASK_DATA[yi][xi])
                continue;
            if (Tj->data.PS_TYPE_IMAGE_MASK_DATA[yj][xj])
                continue;

            if (unweighted_sum) {
                flux += (Pi->data.F32[yi][xi] * Pj->data.F32[yj][xj]);
            } else {
                wt = Wi->data.F32[yi][xi];
                if (wt > 0) {
                    flux += (Pi->data.F32[yi][xi] * Pj->data.F32[yj][xj]) / wt;
                }
            }
        }
    }
    return flux;
}

double pmSourceCrossWeight(const pmSource *Mi,
                           const pmSource *Mj,
                           const bool unweighted_sum) // should the cross product be weighted?
{
    PS_ASSERT_PTR_NON_NULL(Mi, NAN);
    PS_ASSERT_PTR_NON_NULL(Mj, NAN);

    int Xs, Xe, Ys, Ye;
    int xi, xj, yi, yj;
    int xIs, xJs, yIs, yJs;
    int xIe, yIe;
    double flux, wt;

    const psImage *Pi = Mi->pixels;
    assert (Pi != NULL);
    const psImage *Pj = Mj->pixels;
    assert (Pj != NULL);

    const psImage *Wi = Mi->variance;
    if (!unweighted_sum) {
        assert (Wi != NULL);
    }

    const psImage *Ti = Mi->maskObj;
    assert (Ti != NULL);
    const psImage *Tj = Mj->maskObj;
    assert (Tj != NULL);

    Xs = PS_MAX (Pi->col0, Pj->col0);
    Xe = PS_MIN (Pi->col0 + Pi->numCols, Pj->col0 + Pj->numCols);

    Ys = PS_MAX (Pi->row0, Pj->row0);
    Ye = PS_MIN (Pi->row0 + Pi->numRows, Pj->row0 + Pj->numRows);

    xIs = Xs - Pi->col0;
    xJs = Xs - Pj->col0;
    yIs = Ys - Pi->row0;
    yJs = Ys - Pj->row0;

    xIe = Xe - Pi->col0;
    yIe = Ye - Pi->row0;

    // note that this is addressing the same image pixels,
    // though only if both are source not model images
    flux = 0;
    for (yi = yIs, yj = yJs; yi < yIe; yi++, yj++) {
        for (xi = xIs, xj = xJs; xi < xIe; xi++, xj++) {
            if (Ti->data.PS_TYPE_IMAGE_MASK_DATA[yi][xi])
                continue;
            if (Tj->data.PS_TYPE_IMAGE_MASK_DATA[yj][xj])
                continue;

            if (unweighted_sum) {
                flux++;
            } else {
                wt = Wi->data.F32[yi][xi];
                if (wt > 0) {
                    flux += 1.0 / wt;
                }
            }
        }
    }
    return flux;
}

double pmSourceWeight(const pmSource *Mi,
                      int term,
                      const bool unweighted_sum) // should the cross product be weighted?
{
    PS_ASSERT_PTR_NON_NULL(Mi, NAN);
    double flux = 0, wt = 0, factor = 0;

    const psImage *Pi = Mi->pixels;
    assert (Pi != NULL);
    const psImage *Wi = Mi->variance;
    if (!unweighted_sum) {
        assert (Wi != NULL);
    }
    const psImage *Ti = Mi->maskObj;
    assert (Ti != NULL);

    // note that this is addressing the same image pixels,
    // though only if both are source not model images
    for (int yi = 0; yi < Pi->numRows; yi++) {
        for (int xi = 0; xi < Pi->numCols; xi++) {
            if (Ti->data.PS_TYPE_IMAGE_MASK_DATA[yi][xi])
                continue;
            if (!unweighted_sum) {
                wt = Wi->data.F32[yi][xi];
                if (wt == 0)
                    continue;
            }

            switch (term) {
	      case 0:
                factor = 1;
                break;
	      case 1:
                factor = xi + Pi->col0;
                break;
	      case 2:
                factor = yi + Pi->row0;
                break;
	      default:
                psAbort("invalid term for pmSourceWeight");
            }

            if (unweighted_sum) {
                flux += (factor * Pi->data.F32[yi][xi]);
            } else {
                flux += (factor * Pi->data.F32[yi][xi]) / wt;
            }
            // fprintf (stderr, "Pi: %f, flux: %f\n", Pi->data.F32[yi][xi], flux);
        }
    }
    return flux;
}
# endif

// determine chisq, nPix, nDOF, chisqNorm : model->nPar must be set
bool pmSourceChisq (pmModel *model, psImage *image, psImage *mask, psImage *variance, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(model, false);
    PS_ASSERT_PTR_NON_NULL(image, false);
    PS_ASSERT_PTR_NON_NULL(mask, false);
    PS_ASSERT_PTR_NON_NULL(variance, false);

    double dC = 0.0;
    int Npix = 0;
    for (int j = 0; j < image->numRows; j++) {
        for (int i = 0; i < image->numCols; i++) {
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[j][i] & maskVal)
                continue;
            if (variance->data.F32[j][i] <= 0)
                continue;
            dC += PS_SQR (image->data.F32[j][i]) / variance->data.F32[j][i];
            Npix ++;
        }
    }
    model->nPix = Npix;
    model->nDOF = Npix - model->nPar;
    model->chisq = dC;
    model->chisqNorm = dC / model->nDOF;

    return (true);
}


// return source aperture magnitude
bool pmSourceChisqUnsubtracted (pmSource *source, pmModel *model, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(model, false);

    float dC = 0.0;
    int Npix = 0;

    // the model function returns the source flux at a position
    psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

    psVector *params = model->params;
    psImage  *image = source->pixels;
    psImage  *mask = source->maskObj;
    psImage  *variance = source->variance;

    int dX = image->col0;
    int dY = image->row0;

    for (int iy = 0; iy < image->numRows; iy++) {
        for (int ix = 0; ix < image->numCols; ix++) {

	    // skip pixels which are masked
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) continue;

            if (variance->data.F32[iy][ix] <= 0) continue;

            coord->data.F32[0] = (psF32) ix + dX + 0.5;
            coord->data.F32[1] = (psF32) iy + dY + 0.5;

            // for the full model, add all points
            float value = model->class->modelFunc (NULL, params, coord);

	    // fprintf (stderr, "%d, %d : %f, %f : %f - %f : %f\n", 
	    // ix, iy, coord->data.F32[0], coord->data.F32[1], image->data.F32[iy][ix], value, dC);

            dC += PS_SQR (image->data.F32[iy][ix] - value) / variance->data.F32[iy][ix];
            Npix ++;
        }
    }
    model->nPix = Npix;
    model->nDOF = Npix - model->nPar;
    model->chisq = dC;
    model->chisqNorm = dC / model->nDOF;

    psFree (coord);
    return (true);
}

bool pmSourceChisqModelFlux (pmSource *source, pmModel *model, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(model, false);

    float dC = 0.0;
    int Npix = 0;

    psVector *params = model->params;
    psImage  *image = source->pixels;
    psImage  *modelFlux = source->modelFlux;
    psImage  *mask = source->maskObj;
    psImage  *variance = source->variance;

    float Io = params->data.F32[PM_PAR_I0];

    for (int iy = 0; iy < image->numRows; iy++) {
        for (int ix = 0; ix < image->numCols; ix++) {

	    // skip pixels which are masked
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) continue;

            if (variance->data.F32[iy][ix] <= 0) continue;

            dC += PS_SQR (image->data.F32[iy][ix] - Io*modelFlux->data.F32[iy][ix]) / variance->data.F32[iy][ix];
            Npix ++;
        }
    }
    model->nPix = Npix;
    model->nDOF = Npix - model->nPar;
    model->chisq = dC;
    model->chisqNorm = dC / model->nDOF;

    return (true);
}

double pmSourceModelWeight(const pmSource *Mi, int term, const pmSourceFitVarMode fitVarMode, const float covarFactor, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(Mi, NAN);
    double flux = 0;
    double wt = 1.0;
    double factor = 0;

    const psImage *Pi = Mi->modelFlux;
    assert (Pi != NULL);

    const psImage *Wi = NULL;
    switch (fitVarMode) {
      case PM_SOURCE_PHOTFIT_CONST:
	break;
      case PM_SOURCE_PHOTFIT_IMAGE_VAR:
      case PM_SOURCE_PHOTFIT_MODEL_SKY:
	Wi = Mi->variance;
	psAssert (Wi, "programming error");
	break;
      case PM_SOURCE_PHOTFIT_MODEL_VAR:
	Wi = Mi->modelVar;
	psAssert (Wi, "programming error");
	break;
      case PM_SOURCE_PHOTFIT_NONE:
	psAbort("programming error");
    }	
    const psImage *Ti = Mi->maskObj;
    assert (Ti != NULL);

    for (int yi = 0; yi < Pi->numRows; yi++) {
	for (int xi = 0; xi < Pi->numCols; xi++) {
	    if (Ti->data.PS_TYPE_IMAGE_MASK_DATA[yi][xi] & maskVal)
		continue;
	    if (fitVarMode != PM_SOURCE_PHOTFIT_CONST) {
		wt = covarFactor * Wi->data.F32[yi][xi];
		if (wt == 0) continue;
	    }
	    switch (term) {
	      case 0:
		factor = 1;
		break;
	      case 1:
		factor = xi + Pi->col0;
		break;
	      case 2:
		factor = yi + Pi->row0;
		break;
	      default:
		psAbort("invalid term for pmSourceWeight");
	    }

	    // wt is 1.0 for CONST
	    flux += (factor * Pi->data.F32[yi][xi]) / wt;
	}
    }
    return flux;
}

double pmSourceModelDotModel (const pmSource *Mi, const pmSource *Mj, const pmSourceFitVarMode fitVarMode, const float covarFactor, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(Mi, NAN);
    PS_ASSERT_PTR_NON_NULL(Mj, NAN);
    int Xs, Xe, Ys, Ye;
    int xi, xj, yi, yj;
    int xIs, xJs, yIs, yJs;
    int xIe, yIe;
    double flux;
    double wt = 1.0;

    const psImage *Pi = Mi->modelFlux;
    assert (Pi != NULL);
    const psImage *Pj = Mj->modelFlux;
    assert (Pj != NULL);

    const psImage *Wi = NULL;
    switch (fitVarMode) {
      case PM_SOURCE_PHOTFIT_CONST:
	break;
      case PM_SOURCE_PHOTFIT_IMAGE_VAR:
      case PM_SOURCE_PHOTFIT_MODEL_SKY:
	Wi = Mi->variance;
	psAssert (Wi, "programming error");
	break;
      case PM_SOURCE_PHOTFIT_MODEL_VAR:
	Wi = Mi->modelVar;
	psAssert (Wi, "programming error");
	break;
      case PM_SOURCE_PHOTFIT_NONE:
	psAbort("programming error");
    }	

    const psImage *Ti = Mi->maskObj;
    assert (Ti != NULL);
    const psImage *Tj = Mj->maskObj;
    assert (Tj != NULL);

    Xs = PS_MAX (Pi->col0, Pj->col0);
    Xe = PS_MIN (Pi->col0 + Pi->numCols, Pj->col0 + Pj->numCols);

    Ys = PS_MAX (Pi->row0, Pj->row0);
    Ye = PS_MIN (Pi->row0 + Pi->numRows, Pj->row0 + Pj->numRows);

    xIs = Xs - Pi->col0;
    xJs = Xs - Pj->col0;
    yIs = Ys - Pi->row0;
    yJs = Ys - Pj->row0;

    xIe = Xe - Pi->col0;
    yIe = Ye - Pi->row0;

    // note that weight is addressing the same image pixels
    flux = 0;
    for (yi = yIs, yj = yJs; yi < yIe; yi++, yj++) {
        for (xi = xIs, xj = xJs; xi < xIe; xi++, xj++) {
            if (Ti->data.PS_TYPE_IMAGE_MASK_DATA[yi][xi] & maskVal)
                continue;
            if (Tj->data.PS_TYPE_IMAGE_MASK_DATA[yj][xj] & maskVal)
                continue;

	    float value = (Pi->data.F32[yi][xi] * Pj->data.F32[yj][xj]);
	    switch (fitVarMode) {
	      case PM_SOURCE_PHOTFIT_CONST:
		wt = 1.0;
		break;
	      case PM_SOURCE_PHOTFIT_IMAGE_VAR:
	      case PM_SOURCE_PHOTFIT_MODEL_SKY:
	      case PM_SOURCE_PHOTFIT_MODEL_VAR:
		wt = covarFactor * Wi->data.F32[yi][xi];
		break;
	      case PM_SOURCE_PHOTFIT_NONE:
		psAbort("programming error");
	    }
	    // skip pixels with nonsense weight values
	    if (wt <= 0) continue;

	    flux += value / wt;
        }
    }
    return flux;
}

double pmSourceDataDotModel (const pmSource *Mi, const pmSource *Mj, const pmSourceFitVarMode fitVarMode, const float covarFactor, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(Mi, NAN);
    PS_ASSERT_PTR_NON_NULL(Mj, NAN);
    int Xs, Xe, Ys, Ye;
    int xi, xj, yi, yj;
    int xIs, xJs, yIs, yJs;
    int xIe, yIe;
    double flux;
    double wt = 1.0;

    const psImage *Pi = Mi->pixels;
    assert (Pi != NULL);
    const psImage *Pj = Mj->modelFlux;
    assert (Pj != NULL);

    const psImage *Wi = NULL;
    switch (fitVarMode) {
      case PM_SOURCE_PHOTFIT_CONST:
	break;
      case PM_SOURCE_PHOTFIT_IMAGE_VAR:
      case PM_SOURCE_PHOTFIT_MODEL_SKY:
	Wi = Mi->variance;
        psAssert (Wi, "programming error");
	break;
      case PM_SOURCE_PHOTFIT_MODEL_VAR:
	Wi = Mi->modelVar;
        psAssert (Wi, "programming error");
	break;
      case PM_SOURCE_PHOTFIT_NONE:
	psAbort("programming error");
    }	

    const psImage *Ti = Mi->maskObj;
    assert (Ti != NULL);
    const psImage *Tj = Mj->maskObj;
    assert (Tj != NULL);

    Xs = PS_MAX (Pi->col0, Pj->col0);
    Xe = PS_MIN (Pi->col0 + Pi->numCols, Pj->col0 + Pj->numCols);

    Ys = PS_MAX (Pi->row0, Pj->row0);
    Ye = PS_MIN (Pi->row0 + Pi->numRows, Pj->row0 + Pj->numRows);

    xIs = Xs - Pi->col0;
    xJs = Xs - Pj->col0;
    yIs = Ys - Pi->row0;
    yJs = Ys - Pj->row0;

    xIe = Xe - Pi->col0;
    yIe = Ye - Pi->row0;

    // note that weight is addressing the same image pixels,
    flux = 0;
    for (yi = yIs, yj = yJs; yi < yIe; yi++, yj++) {
        for (xi = xIs, xj = xJs; xi < xIe; xi++, xj++) {
            if (Ti->data.PS_TYPE_IMAGE_MASK_DATA[yi][xi] & maskVal)
                continue;
            if (Tj->data.PS_TYPE_IMAGE_MASK_DATA[yj][xj] & maskVal)
                continue;

	    float value = (Pi->data.F32[yi][xi] * Pj->data.F32[yj][xj]);
	    switch (fitVarMode) {
	      case PM_SOURCE_PHOTFIT_CONST:
		wt = 1.0;
		break;
	      case PM_SOURCE_PHOTFIT_IMAGE_VAR:
	      case PM_SOURCE_PHOTFIT_MODEL_SKY:
	      case PM_SOURCE_PHOTFIT_MODEL_VAR:
                wt = covarFactor * Wi->data.F32[yi][xi];
		break;
	      case PM_SOURCE_PHOTFIT_NONE:
		psAbort("programming error");
	    }
            // skip pixels with nonsense weight values
	    if (wt <= 0) continue;

	    flux += value / wt;
        }
    }
    return flux;
}

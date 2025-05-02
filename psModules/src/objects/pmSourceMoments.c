/** @file  pmSource.c
 *
 *  Functions to define and manipulate sources on images
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA: significant modifications.
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:31:25 $
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
#include <strings.h>
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

/******************************************************************************
pmSourceMoments(source, radius): this function takes a subImage defined in the
pmSource data structure, along with the peak location, and determines the
various moments associated with that peak.

Uses the following elements:
    pmSource
    pmSource->peak
    pmSource->pixels
    pmSource->variance (optional)
    pmSource->mask (optional)

The peak calculations are done in image coords, not subImage coords.

this version optionally clips input pixels on S/N 
*****************************************************************************/
# define VALID_RADIUS(X,Y,RAD2) (((RAD2) >= (PS_SQR(X) + PS_SQR(Y))) ? 1 : 0)

static bool beVerbose = false;
void pmSourceMomentsSetVerbose(bool state){ beVerbose = state; }

bool pmSourceMomentsHighOrder    (pmSource *source, float radius, float sigma, float minSN, psImageMaskType maskVal);
bool pmSourceMomentsRadialMoment (pmSource *source, float radius, float minKronRadius, psImageMaskType maskVal);
bool pmSourceMomentsKronFluxes   (pmSource *source, float sigma,  float minSN, psImageMaskType maskVal);

// if mode & EXTERNAL or mode2 & MATCHED, do not re-calculate the centroid (use peak as centroid)
bool pmSourceMoments(pmSource *source, float radius, float sigma, float minSN, float minKronRadius, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_FLOAT_LARGER_THAN(radius, 0.0, false);

    if (source->moments == NULL) {
      source->moments = pmMomentsAlloc();
    }

    // a note about coordinates: coordinates of objects throughout psphot refer to the primary
    // image coordinates.  the source->pixels image has an offset relative to its parent of
    // col0,row0: a pixel (x,y) in the primary image has coordinates of (x-col0, y-row0) in
    // this subimage.  we subtract off the peak coordinates, adjusted to this subimage, to have
    // minimal round-off error in the sums.  since these values are subtracted just to minimize
    // the dynamic range and are added back below, the exact value does not matter. these are
    // (int) so they can be used in the image index below.

    // XXX // do 2 passes : the first pass should use a somewhat smaller radius and no sigma window to 
    // XXX // get an unbiased (but probably noisy) centroid
    // XXX if (!pmSourceMomentsGetCentroid (source, 0.75*radius, 0.0, minSN, maskVal, source->peak->xf, source->peak->yf)) {
    // XXX 	return false;
    // XXX }
    // XXX // second pass applies the Gaussian window and uses the centroid from the first pass
    // XXX if (!pmSourceMomentsGetCentroid (source, radius, sigma, minSN, maskVal, source->moments->Mx, source->moments->My)) {
    // XXX 	return false;
    // XXX }

    // If we use a large radius for the centroid, it will be biased by any neighbors.  The flux
    // of any object drops pretty quickly outside 1-2 sigmas.  (The exception is bright
    // saturated stars, for which we need to use a very large radius here)
    // NOTE: if (mode & EXTERNAL) or (mode2 & MATCHED), do not re-calculate the centroid (use peak as centroid)
    // (we still call this function because it sets moments->Sum,SN,Peak,nPixels
    if (!pmSourceMomentsGetCentroid (source, 1.5*sigma, 0.0, minSN, maskVal, source->peak->xf, source->peak->yf)) {
	return false;
    }

    pmSourceMomentsHighOrder (source, radius, sigma, minSN, maskVal);

    // now calculate the 1st radial moment (for kron flux) using symmetrical averaging
    pmSourceMomentsRadialMoment (source, radius, minKronRadius, maskVal);

    // now calculate the kron flux values using the 1st radial moment
    pmSourceMomentsKronFluxes (source, sigma, minSN, maskVal);

    psTrace ("psModules.objects", 4, "Mrf: %f  KronFlux: %f  Mxx: %f  Mxy: %f  Myy: %f  Mxxx: %f  Mxxy: %f  Mxyy: %f  Myyy: %f  Mxxxx: %f  Mxxxy: %f  Mxxyy: %f  Mxyyy: %f  Mxyyy: %f\n",
	     source->moments->Mrf,   source->moments->KronFlux, 
	     source->moments->Mxx,   source->moments->Mxy,   source->moments->Myy,
	     source->moments->Mxxx,  source->moments->Mxxy,  source->moments->Mxyy,  source->moments->Myyy,
	     source->moments->Mxxxx, source->moments->Mxxxy, source->moments->Mxxyy, source->moments->Mxyyy, source->moments->Myyyy);

    psTrace ("psModules.objects", 3, "peak %f %f (%f = %f) Mx: %f  My: %f  Sum: %f  Mxx: %f  Mxy: %f  Myy: %f  Npix: %d\n",
	     source->peak->xf, source->peak->yf, 
	     source->peak->rawFlux, sqrt(source->peak->detValue), 
	     source->moments->Mx, source->moments->My, 
	     source->moments->Sum, 
	     source->moments->Mxx, source->moments->Mxy, source->moments->Myy, 
	     source->moments->nPixels);

    return(true);
}

bool pmSourceMomentsGetCentroid(pmSource *source, float radius, float sigma, float minSN, psImageMaskType maskVal, float xGuess, float yGuess) { 

    // First Pass: calculate the first moments (these are subtracted from the coordinates below)
    // Sum = SUM (z - sky)
    // X1  = SUM (x - xc)*(z - sky)
    // .. etc

    float sky = 0.0;

    float peakPixel = -PS_MAX_F32;
    psS32 numPixels = 0;
    float Sum = 0.0;
    float Var = 0.0;
    float X1 = 0.0;
    float Y1 = 0.0;
    float R2 = PS_SQR(radius);
    float minSN2 = PS_SQR(minSN);
    float rsigma2 = 0.5 / PS_SQR(sigma);

    float xPeak = xGuess - source->pixels->col0; // coord of peak in subimage
    float yPeak = yGuess - source->pixels->row0; // coord of peak in subimage

    // we are guaranteed to have a valid pixel and variance at this location (right? right?)
    // float weightNorm = source->pixels->data.F32[yPeak][xPeak] / sqrt (source->variance->data.F32[yPeak][xPeak]);
    // psAssert (isfinite(source->pixels->data.F32[yPeak][xPeak]), "peak must be on valid pixel");
    // psAssert (isfinite(source->variance->data.F32[yPeak][xPeak]), "peak must be on valid pixel");
    // psAssert (source->variance->data.F32[yPeak][xPeak] > 0, "peak must be on valid pixel");

    // the moments [Sum(x*f) / Sum(f)] are calculated in pixel index values, and should
    // not depend on the fractional pixel location of the source.  However, the aperture
    // (radius) and the Gaussian window (sigma) depend subtly on the fractional pixel
    // position of the expected centroid

    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	float yDiff = row + 0.5 - yPeak;
	if (fabs(yDiff) > radius) continue;

	float *vPix = source->pixels->data.F32[row];
	float *vWgt = source->variance ? source->variance->data.F32[row] : source->pixels->data.F32[row];

	psImageMaskType *vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[row];
	// psImageMaskType *vMsk = (source->maskView == NULL) ? NULL : source->maskView->data.PS_TYPE_IMAGE_MASK_DATA[row];

	for (psS32 col = 0; col < source->pixels->numCols ; col++, vPix++, vWgt++) {
	    if (vMsk) {
		if (*vMsk & maskVal) {
		    vMsk++;
		    continue;
		}
		vMsk++;
	    }
	    if (isnan(*vPix)) continue;

	    float xDiff = col + 0.5 - xPeak;
	    if (fabs(xDiff) > radius) continue;

	    // radius is just a function of (xDiff, yDiff)
	    float r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
	    if (r2 > R2) continue;

	    float pDiff = *vPix - sky;
	    float wDiff = *vWgt;

	    // skip pixels below specified significance level.  for a PSFs, this
	    // over-weights the wings of bright stars compared to those of faint stars.
	    // for the estimator used for extended source analysis (where the window
	    // function is allowed to be arbitrarily large), we need to clip to avoid
	    // negative second moments.
	    if (PS_SQR(pDiff) < minSN2*wDiff) continue; // 
	    if ((minSN > 0.0) && (pDiff < 0)) continue; // 

	    // Apply a Gaussian window function.  Be careful with the window function.  S/N
	    // weighting over weights the sky for faint sources
	    if (sigma > 0.0) {
		float z  = r2*rsigma2;
		assert (z >= 0.0);
		float weight  = exp(-z);

		wDiff *= weight;
		pDiff *= weight;
	    } 

	    Var += wDiff;
	    Sum += pDiff;

	    float xWght = xDiff * pDiff;
	    float yWght = yDiff * pDiff;

	    X1  += xWght;
	    Y1  += yWght;

	    peakPixel = PS_MAX (*vPix, peakPixel);
	    numPixels++;
	}
    }

    // we only update the centroid if the position is not supplied from elsewhere
    bool skipCentroid = false;
    skipCentroid |= (source->mode  & PM_SOURCE_MODE_EXTERNAL); // skip externally supplied positions
    skipCentroid |= (source->mode2 & PM_SOURCE_MODE2_MATCHED); // skip sources defined by other image positions

    // if we have less than (1/4) of the possible pixels (in circle or box), force a retry
    int minPixels = PS_MIN(0.75*R2, source->pixels->numCols*source->pixels->numRows/4.0);

    // XXX EAM - the limit is a bit arbitrary.  make it user defined?
    if (numPixels < minPixels) {
	psTrace ("psModules.objects", 3, "insufficient valid pixels (%d vs %d; %f) for source\n", numPixels, minPixels, Sum);
	return (false);
    }

    // only skip negative sum sources if the sources are not forced
    if (!skipCentroid && (Sum <= 0)) {
	psTrace ("psModules.objects", 3, "insufficient significant pixels (%d vs %d; %f) for source\n", numPixels, minPixels, Sum);
	return (false);
    }

    // calculate the first moment.
    float Mx = X1/Sum;
    float My = Y1/Sum;
    if ((fabs(Mx) > 2.0) || (fabs(My) > 2.0)) {
	psTrace ("psModules.objects", 3, " big centroid swing; ok peak? %d, %d\n", source->peak->x, source->peak->y);
    }
    psTrace ("psModules.objects", 5, "id: %d, sky: %f  Mx: %f  My: %f  Sum: %f  X1: %f  Y1: %f  Npix: %d\n", source->id, sky, Mx, My, Sum, X1, Y1, numPixels);

    // add back offset of peak in primary image
    // also offset from pixel index to pixel coordinate
    // (the calculation above uses pixel index instead of coordinate)
    // 0.5 PIX: moments are calculated using the pixel index and converted here to pixel coords

    if (skipCentroid) {
	source->moments->Mx = source->peak->xf;
	source->moments->My = source->peak->yf;
    } else {
	if ((fabs(Mx) > radius) || (fabs(My) > radius)) {
	    psTrace ("psModules.objects", 3, "extreme centroid swing; invalid peak %d, %d\n", source->peak->x, source->peak->y);
	    return (false);
	}
	source->moments->Mx = Mx + xGuess;
	source->moments->My = My + yGuess;
    }

    source->moments->Sum = Sum;
    source->moments->SN  = Sum / sqrt(Var);
    source->moments->Peak = peakPixel;
    source->moments->nPixels = numPixels;

    return true;
}

float pmSourceMinKronRadius(psArray *sources, float PSF_SN_LIM) {

    psVector *radii = psVectorAllocEmpty(100, PS_TYPE_F32);

    for (int i = 0; i < sources->n; i++) {
	pmSource *src = sources->data[i]; // Source of interest
	if (!src || !src->moments) {
	    continue;
	}

	if (src->mode & PM_SOURCE_MODE_BLEND) {
	    continue;
	}

	if (!src->moments->nPixels) continue;

	if (src->moments->SN < PSF_SN_LIM) continue;

	// XXX put in Mxx,Myy cut based on clump location

	psVectorAppend(radii, src->moments->Mrf);
    }

    // find the peak in this image
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

    if (!psVectorStats (stats, radii, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to get image statistics.\n");
	psFree(stats);
	return NAN;
    }

    float minRadius = stats->sampleMedian;

    psFree(radii);
    psFree(stats);
    return minRadius;
}

bool pmSourceMomentsHighOrder (pmSource *source, float radius, float sigma, float minSN, psImageMaskType maskVal) {

    // this function assumes the sky has been well-subtracted for the image
    float Sum = 0.0;
    float R2 = PS_SQR(radius);
    float minSN2 = PS_SQR(minSN);
    float rsigma2 = 0.5 / PS_SQR(sigma);

    // Now calculate higher-order moments, using the above-calculated first moments to adjust coordinates
    // Xn  = SUM (x - xc)^n * (z - sky) -- note that sky is 0.0 by definition here
    float XX = 0.0;
    float XY = 0.0;
    float YY = 0.0;
    float XXX = 0.0;
    float XXY = 0.0;
    float XYY = 0.0;
    float YYY = 0.0;
    float XXXX = 0.0;
    float XXXY = 0.0;
    float XXYY = 0.0;
    float XYYY = 0.0;
    float YYYY = 0.0;

    float Xo = source->moments->Mx;
    float Yo = source->moments->My;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    float xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    float yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    // calculate the higher-order moments using Xo,Yo
    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	float yDiff = row - yCM;
	if (fabs(yDiff) > radius) continue;

	float *vPix = source->pixels->data.F32[row];
	float *vWgt = source->variance ? source->variance->data.F32[row] : source->pixels->data.F32[row];

	psImageMaskType  *vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[row];

	for (psS32 col = 0; col < source->pixels->numCols ; col++, vPix++, vWgt++) {
	    if (vMsk) {
		if (*vMsk & maskVal) {
		    vMsk++;
		    continue;
		}
		vMsk++;
	    }
	    if (isnan(*vPix)) continue;

	    float xDiff = col - xCM;
	    if (fabs(xDiff) > radius) continue;

	    // radius is just a function of (xDiff, yDiff)
	    float r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
	    if (r2 > R2) continue;

	    float fDiff = *vPix;
	    float pDiff = fDiff;
	    float wDiff = *vWgt;

	    // skip pixels below specified significance level.  this is allowed, but should be
	    // avoided -- the over-weights the wings of bright stars compared to those of faint
	    // stars.
	    if (PS_SQR(pDiff) < minSN2*wDiff) continue;
	    if ((minSN > 0.0) && (pDiff < 0)) continue; 

	    // Apply a Gaussian window function.  Be careful with the window function.  S/N
	    // weighting over-weights the sky for faint sources
	    if (sigma > 0.0) {
		float z = r2 * rsigma2;
		assert (z >= 0.0);
		float weight  = exp(-z);

		wDiff *= weight;
		pDiff *= weight;
	    } 

	    Sum += pDiff;

	    float r = sqrt(r2);

	    float x = xDiff * pDiff;
	    float y = yDiff * pDiff;

	    float xx = xDiff * x;
	    float xy = xDiff * y;
	    float yy = yDiff * y;

	    // the factor of 1/r makes these appropriate for measuring
	    // \sum r^2 cos(3 theta), sin(3 theta) (psphotImageQuality.c:107)
	    float xxx = xDiff * xx / r;
	    float xxy = xDiff * xy / r;
	    float xyy = xDiff * yy / r;
	    float yyy = yDiff * yy / r;

	    // the factor of 1/r^2 makes these appropriate for measuring
	    // \sum r^2 cos(4 theta), sin(4 theta) (psphotImageQuality.c:107)
	    float xxxx = xDiff * xxx / r2;
	    float xxxy = xDiff * xxy / r2;
	    float xxyy = xDiff * xyy / r2;
	    float xyyy = xDiff * yyy / r2;
	    float yyyy = yDiff * yyy / r2;

	    XX  += xx;
	    XY  += xy;
	    YY  += yy;

	    XXX  += xxx;
	    XXY  += xxy;
	    XYY  += xyy;
	    YYY  += yyy;

	    XXXX  += xxxx;
	    XXXY  += xxxy;
	    XXYY  += xxyy;
	    XYYY  += xyyy;
	    YYYY  += yyyy;
	}
    }
    // NOT needed : source->moments->wSum = Sum;

    source->moments->Mxx = XX/Sum;
    source->moments->Mxy = XY/Sum;
    source->moments->Myy = YY/Sum;

    source->moments->Mxxx = XXX/Sum;
    source->moments->Mxxy = XXY/Sum;
    source->moments->Mxyy = XYY/Sum;
    source->moments->Myyy = YYY/Sum;

    source->moments->Mxxxx = XXXX/Sum;
    source->moments->Mxxxy = XXXY/Sum;
    source->moments->Mxxyy = XXYY/Sum;
    source->moments->Mxyyy = XYYY/Sum;
    source->moments->Myyyy = YYYY/Sum;

    return true;
}

bool pmSourceMomentsRadialMoment (pmSource *source, float radius, float minKronRadius, psImageMaskType maskVal) {


    float **vPix = source->pixels->data.F32;
    psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

    float RF = 0.0;
    float RH = 0.0;
    float RS = 0.0;

    // centroid around which to calculate the moments
    float Xo = source->moments->Mx;
    float Yo = source->moments->My;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    float xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    float yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    float R2 = PS_SQR(radius);

    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	float yDiff = row - yCM;
	if (fabs(yDiff) > radius) continue;

	// coordinate of mirror pixel
	int yFlip = yCM - yDiff;
	if (yFlip < 0) continue;
	if (yFlip >= source->pixels->numRows) continue;

	for (psS32 col = 0; col < source->pixels->numCols ; col++) {
	    // check mask and value for this pixel
	    if (vMsk && (vMsk[row][col] & maskVal)) continue;
	    if (isnan(vPix[row][col])) continue;

	    float xDiff = col - xCM;
	    if (fabs(xDiff) > radius) continue;

	    // coordinate of mirror pixel
	    int xFlip = xCM - xDiff;
	    if (xFlip < 0) continue;
	    if (xFlip >= source->pixels->numCols) continue;

	    // check mask and value for mirror pixel
	    if (vMsk && (vMsk[yFlip][xFlip] & maskVal)) continue;
	    if (isnan(vPix[yFlip][xFlip])) continue;

	    // radius is just a function of (xDiff, yDiff)
	    float r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
	    if (r2 > R2) continue;

	    float fDiff1 = vPix[row][col];
	    float fDiff2 = vPix[yFlip][xFlip];
	    float pDiff = (fDiff1 > 0.0) ? sqrt(fabs(fDiff1*fDiff2)) : -sqrt(fabs(fDiff1*fDiff2));

	    // Kron Flux uses the 1st radial moment (NOT Gaussian windowed?)
	    float r = sqrt(r2);
	    float rf = r * pDiff;
	    float rh = sqrt(r) * pDiff;
	    float rs = 0.5 * (fDiff1 + fDiff2);

	    RF  += rf;
	    RH  += rh;
	    RS  += rs;
	}
    }

    source->moments->Mrh = RH/RS;

    // if Mrf = RF/RS (first radial moment) is very small, we are getting into low-significance
    // territory.  saturate at minKronRadius.  conversely, if Mrf is >> radius for faint
    // sources, we are clearly making an error.  saturate at radius.
    float kronRefRadius = MAX(minKronRadius, RF/RS);
    if (source->moments->SN < 10) {
	kronRefRadius = MIN(radius, kronRefRadius);
    }

    // if source is externally supplied and it already has a finite Mrf do not change it
    if (! ((source->mode & PM_SOURCE_MODE_EXTERNAL) && isfinite(source->moments->Mrf))) {
        source->moments->Mrf = kronRefRadius;
    }

    return true;
}

bool pmSourceMomentsKronFluxes (pmSource *source, float sigma, float minSN, psImageMaskType maskVal) {

    float **vPix = source->pixels->data.F32;
    float **vWgt = source->variance ? source->variance->data.F32 : source->pixels->data.F32;
    psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

    float radKinner = 1.0*source->moments->Mrf;
    float radKron   = 2.5*source->moments->Mrf;
    float radKouter = 4.0*source->moments->Mrf;

    int nKronPix = 0;
    int nCorePix = 0;
    int nInner = 0;
    int nOuter = 0;
    
    float Sum = 0.0;
    float Var = 0.0;
    float SumCore = 0.0;
    float VarCore = 0.0;
    float SumInner = 0.0;
    float SumOuter = 0.0;

    // centroid around which to calculate the moments
    float Xo = source->moments->Mx;
    float Yo = source->moments->My;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    float xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    float yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    float minSN2 = PS_SQR(minSN);

    // calculate the Kron flux, and related fluxes (NO symmetrical averaging)
    for (psS32 row = 0; row < source->pixels->numRows ; row++) {
	
	float yDiff = row - yCM;
	if (fabs(yDiff) > radKouter) continue;
	
	for (psS32 col = 0; col < source->pixels->numCols ; col++) {
	    // check mask and value for this pixel
	    if (vMsk && (vMsk[row][col] & maskVal)) continue;
	    if (isnan(vPix[row][col])) continue;
	    
	    float xDiff = col - xCM;
	    if (fabs(xDiff) > radKouter) continue;
	    
	    // radKron is just a function of (xDiff, yDiff)
	    float r2  = PS_SQR(xDiff) + PS_SQR(yDiff);

	    float fDiff1 = vPix[row][col];
	    float pDiff = fDiff1;
	    float wDiff = vWgt[row][col];
				    
	    // skip pixels below specified significance level.  this is allowed, but should be
	    // avoided -- the over-weights the wings of bright stars compared to those of faint
	    // stars.
	    if (PS_SQR(pDiff) < minSN2*wDiff) continue;
	    
	    float r  = sqrt(r2);
	    if (r < radKron) {
		Sum += pDiff;
		Var += wDiff;
		nKronPix ++;
	    }

	    // use sigma (fixed by psf) not a radKron based value
	    if (r < sigma) {
		SumCore += pDiff;
		VarCore += wDiff;
		nCorePix ++;
	    }

	    if ((r > radKinner) && (r < radKron)) {
		SumInner += pDiff;
		nInner ++;
	    }
	    if ((r > radKron)  && (r < radKouter)) {
		SumOuter += pDiff;
		nOuter ++;
	    }
	}
    }
    // *** should I rescale these fluxes by pi R^2 / nNpix?
    // XXX source->moments->KronCore    = SumCore       * M_PI *  PS_SQR(sigma) / nCorePix;
    // XXX source->moments->KronCoreErr = sqrt(VarCore) * M_PI *  PS_SQR(sigma) / nCorePix;
    // XXX source->moments->KronFlux    = Sum           * M_PI *  PS_SQR(radKron) / nKronPix;
    // XXX source->moments->KronFluxErr = sqrt(Var)     * M_PI *  PS_SQR(radKron) / nKronPix;
    // XXX source->moments->KronFinner  = SumInner      * M_PI * (PS_SQR(radKron)   - PS_SQR(radKinner)) / nInner;
    // XXX source->moments->KronFouter  = SumOuter      * M_PI * (PS_SQR(radKouter) -   PS_SQR(radKron)) / nOuter;

    source->moments->KronCore    = SumCore;
    source->moments->KronCoreErr = sqrt(VarCore);
    source->moments->KronFlux    = Sum;
    source->moments->KronFluxErr = sqrt(Var);
    source->moments->KronFinner  = SumInner;
    source->moments->KronFouter  = SumOuter;

    // XXX not sure I should save this here...
    source->moments->KronFluxPSF    = source->moments->KronFlux;
    source->moments->KronFluxPSFErr = source->moments->KronFluxErr;
    source->moments->KronRadiusPSF  = source->moments->Mrf;

    return true;
}

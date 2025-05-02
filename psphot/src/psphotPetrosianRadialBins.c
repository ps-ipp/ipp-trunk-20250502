# include "psphotInternal.h"
float InterpolateValues (float X0, float Y0, float X1, float Y1, float X);

// convert the flux vs elliptical radius to annular bins

// we are guaranteed to be limited by either the seeing (1 - few pixels) or by the pixels
// themselves.  this function does not attempt to measure the radial profiles accurately
// for radii that are smaller than ~2 pixels

// for small radii, we are measuring the mean surface brightness in non-overlapping radial
// bins.  for large radii (r > 2 pixels), we are measuring the surface brightness for a
// radius range \alpha r_i < i < \beta r_i, but performing this measurement for radii more
// finely spaced than r_{i+1} = r_i * \beta / \alpha.  for the integration, we need to
// track the non-overlapping radius values.

// Photo interpolates the image of interest to place the peak on the center of the central
// pixel, and then uses the exact fractions of the pixels in each of the first few annuli.
// Seems like a reasonable thing, but is there any significance to the difference?

// XXX move the resulting elements from profile to extpars->petrosian?
bool psphotPetrosianRadialBins (pmSource *source, float radiusMax, float skynoise) {

    psAssert (source, "missing source");
    psAssert (source->extpars, "missing extpars");
    psAssert (source->extpars->ellipticalFlux, "missing ellipticalFlux");

    psVector *radius = source->extpars->ellipticalFlux->radiusElliptical;
    psVector *flux = source->extpars->ellipticalFlux->fluxElliptical;

    // sort incoming vectors by radius
    pmSourceRadialProfileSortPair (radius, flux);

    if (!source->extpars->petProfile) {
	source->extpars->petProfile = pmSourceRadialProfileAlloc();
    }
    pmSourceRadialProfile *profile = source->extpars->petProfile;

    // float skyModelErrorSQ = PS_SQR(skynoise);

    int nMax = radiusMax;

    // radBin stores the centers of the radial bins, 
    // radMin, radMax store the bounds
    psVector *radMin  = psVectorAllocEmpty(nMax, PS_TYPE_F32);
    psVector *radMax  = psVectorAllocEmpty(nMax, PS_TYPE_F32);
    psVector *radAlp  = psVectorAllocEmpty(nMax, PS_TYPE_F32);
    psVector *radBet  = psVectorAllocEmpty(nMax, PS_TYPE_F32);

    psVector *binSB      = psVectorAllocEmpty(nMax, PS_TYPE_F32); // surface brightness of radial bin
    psVector *binSBstdev = psVectorAllocEmpty(nMax, PS_TYPE_F32); // surface brightness of radial bin
    psVector *binRad  	 = psVectorAllocEmpty(nMax, PS_TYPE_F32); // mean radius of radial bin
    psVector *binArea 	 = psVectorAllocEmpty(nMax, PS_TYPE_F32); // area of radial bin (contiguous, non-overlapping)
    psVector *binFill 	 = psVectorAllocEmpty(nMax, PS_TYPE_F32); // fraction of radial bin with valid pixels

    psVectorInit (binSB, 0.0);
    psVectorInit (binSBstdev, 0.0);
    psVectorInit (binRad, 0.0);

    // generate radial bin bounds
    radMin->data.F32[0] = 0.0;
    radMax->data.F32[0] = 1.0;
    radAlp->data.F32[0] = 0.0;
    radBet->data.F32[0] = 1.0;
    
    radMin->data.F32[1] = 1.0;
    radMax->data.F32[1] = 1.5;
    radAlp->data.F32[1] = 1.0;
    radBet->data.F32[1] = 1.5;
    
    radMin->data.F32[2] = 1.5;
    radMax->data.F32[2] = 2.0;
    radAlp->data.F32[2] = 1.5;
    radBet->data.F32[2] = 2.0;
    
# define PETROSIAN_ALPHA 0.8
# define PETROSIAN_BETA 1.25
# define POWER_LAW_SPACING true
    
    // power-law spacing with overlapping boundaries at the geometric mid-points
    float rBeta = sqrt(PETROSIAN_BETA);
    for (int i = 3; radBet->data.F32[i-1] < radiusMax; i++) {
	if (POWER_LAW_SPACING) {
	    radMin->data.F32[i] = radMax->data.F32[i-1];
	    radMax->data.F32[i] = radMin->data.F32[i] * PETROSIAN_BETA;
	    radAlp->data.F32[i] = radMin->data.F32[i] / rBeta;
	    radBet->data.F32[i] = radMax->data.F32[i] * rBeta;
	} else {
	    radMin->data.F32[i] = radMax->data.F32[i-1];
	    radMax->data.F32[i] = radMin->data.F32[i] + 1;
	    float rMid = 0.5*(radMin->data.F32[i] + radMax->data.F32[i]);
	    radAlp->data.F32[i] = rMid * PETROSIAN_ALPHA;
	    radBet->data.F32[i] = rMid * PETROSIAN_BETA;
	}
	radMin->n = radMax->n = radAlp->n = radBet->n = i + 1;
    }

    // generate radial area-weighted mean radius & non-overlapping areas
    for (int i = 0; i < radMin->n; i++) {
	float rMin = radMin->data.F32[i];
	float rMax = radMax->data.F32[i];
	
	float rMin2 = rMin*rMin;
	float rMin3 = rMin2*rMin;

	float rMax2 = rMax*rMax;
	float rMax3 = rMax2*rMax;

	float rBin = 2.0 * (rMax3 - rMin3) / (rMax2 - rMin2) / 3.0;
	
	// XXX calculate area-weighted radius rather than asserting?
	binRad->data.F32[i] = rBin;
	binArea->data.F32[i] = M_PI * (rMax2 - rMin2);

	psTrace ("psphot", 6, "%3d  %5.1f %5.1f : %5.1f : %5.1f %5.1f\n", 
		 i, radAlp->data.F32[i], radMin->data.F32[i], binRad->data.F32[i],
		 radMax->data.F32[i], radBet->data.F32[i]);
    }

    // storage vector for stats
    psVector *values = psVectorAllocEmpty (flux->n, PS_TYPE_F32);
    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);

    bool done = false;
    int nOut = 0;
    float Rmin = radAlp->data.F32[nOut];
    float Rmax = radBet->data.F32[nOut];
    float Rnxt = radAlp->data.F32[nOut+1];  // minimum radius for next range
    int iNext = 0;
    for (int i = 0; !done && (i < radius->n); i++) {
	if (radius->data.F32[i] < Rnxt) {
	  iNext = i;
	}
	if (radius->data.F32[i] > Rmax) {
	    // calculate the value for the nOut bin
	    float value; // , dvalue;
	    if (values->n > 0) {
		psVectorStats (stats, values, NULL, NULL, 0);
		value = stats->robustMedian;
		// dvalue = stats->robustStdev;
	    } else {
		value = NAN;
		// dvalue = NAN;
	    }

	    binSB->data.F32[nOut] = value;
	    // binSBstdev->data.F32[nOut] = sqrt(PS_SQR(dvalue) / values->n + skyModelErrorSQ);
	    binSBstdev->data.F32[nOut] = skynoise / sqrt(values->n);
	    binFill->data.F32[nOut] = values->n / binArea->data.F32[nOut];

	    // error in the SB is the stdev per bin / sqrt (number of pixels) 
	    // added in quadrature to a fraction of the local sky (not the 
	    // residual flux, but the sky from the sky model)

	    psTrace ("psphot", 5, "%3d  %5.1f %5.1f : %5.1f  %5.2f\n", nOut, radAlp->data.F32[nOut], radBet->data.F32[nOut], binSB->data.F32[nOut], binSBstdev->data.F32[nOut]);

	    nOut ++;
	    if (nOut >= radAlp->n) break;
	    Rmin = radAlp->data.F32[nOut];
	    Rmax = radBet->data.F32[nOut];
	    Rnxt = (nOut < nMax - 1) ? radAlp->data.F32[nOut+1] : Rmax;  // minimum radius for next range
	    values->n = 0;
	    psStatsInit(stats);
	    i = iNext;
	}
	if (radius->data.F32[i] < Rmin) {
	    continue;
	}
	psVectorAppend (values, flux->data.F32[i]);
    }
    binSB->n = binSBstdev->n = binRad->n = binArea->n = nOut;
    // XXX I think this misses the last radial bin -- do we care?

    // interpolate any bins that were empty (extrapolate to center if needed)
    if (!isfinite(binSB->data.F32[0]) && !isfinite(binSB->data.F32[1])) {
	psWarning ("center 2 bins of source at %f, %f are NAN, skipping this source", source->peak->xf, source->peak->yf);
	// XXX raise a flag
	psFree(binSB);
	psFree(binSBstdev);
	psFree(binRad);
	psFree(binArea);
	psFree(binFill);
	psFree(radMin);
	psFree(radMax);
	psFree(radAlp);
	psFree(radBet);
	psFree(values);
	psFree(stats);
	source->mode2 |= PM_SOURCE_MODE2_RADBIN_NAN_CENTER;
	return true;
    }

    // if center bin is empty assume same SB as next radius (probably true due to PSF)
    if (!isfinite(binSB->data.F32[0])) {
	binSB->data.F32[0] = binSB->data.F32[1];
	binSBstdev->data.F32[0] = binSBstdev->data.F32[1];
    }

    // interpolate any bins that were empty (if center if needed)
    for (int i = 1; i < binSB->n - 1; i++) {
	if (isfinite(binSB->data.F32[i])) continue;
	binSB->data.F32[i] = InterpolateValues (binRad->data.F32[i-1], binSB->data.F32[i-1], binRad->data.F32[i+1], binSB->data.F32[i+1], binRad->data.F32[i]);
	binSBstdev->data.F32[i] = InterpolateValues (binRad->data.F32[i-1], binSBstdev->data.F32[i-1], binRad->data.F32[i+1], binSBstdev->data.F32[i+1], binRad->data.F32[i]);
    }

    psFree(profile->binSB);
    psFree(profile->binSBstdev);
    psFree(profile->radialBins);
    psFree(profile->area);
    psFree(profile->binFill);

    // save the vectors
    profile->radialBins = binRad;
    profile->area       = binArea;
    profile->binFill    = binFill;
    profile->binSB      = binSB;
    profile->binSBstdev = binSBstdev;

    // psphotPetrosianVisualProfileRadii (radius, flux, binRad, binSB, source->peak->flux, 0.0);

    psFree(radMin);
    psFree(radMax);
    psFree(radAlp);
    psFree(radBet);
    psFree(values);
    psFree(stats);

    return true;
}

// the area-weighted mean radius is given by:

// integral r * 2 pi r dr / integral 2 pi r dr

// = 2/3 pi (r_max^3 - r_min^3)  / pi (r_max^2 - r_min^2) 
// = 2/3 (r_max^3 - r_min^3) / (r_max^2 - r_min^2)


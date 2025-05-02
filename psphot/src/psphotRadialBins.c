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
bool psphotRadialBins (psMetadata *recipe, pmSource *source, float radiusMax, float skynoise) {

    psAssert (source, "missing source");
    psAssert (source->extpars, "missing extpars");
    psAssert (source->extpars->ellipticalFlux, "missing ellipticalFlux");

    psVector *radius = source->extpars->ellipticalFlux->radiusElliptical;
    psVector *flux = source->extpars->ellipticalFlux->fluxElliptical;

    // sort incoming vectors by radius
    pmSourceRadialProfileSortPair (radius, flux);

    if (!source->extpars->radProfile) {
	source->extpars->radProfile = pmSourceRadialProfileAlloc();
    }
    pmSourceRadialProfile *profile = source->extpars->radProfile;

    float skyModelErrorSQ = PS_SQR(skynoise);
    psEllipseAxes axes = source->extpars->axes;
    float AxialRatio = axes.minor / axes.major;

    // radMin, radMax store the bounds of the annuli
    bool status = false;
    psVector *radMin = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.LOWER");
    psVector *radMax = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.UPPER");
    psAssert (radMin, "RADIAL.ANNULAR.BINS.LOWER is missing from recipe");
    psAssert (radMin->n, "RADIAL.ANNULAR.BINS.LOWER is empty in recipe");
    psAssert (radMax, "RADIAL.ANNULAR.BINS.UPPER is missing from recipe");
    psAssert (radMax->n, "RADIAL.ANNULAR.BINS.UPPER is empty in recipe");

    psVector *binSB      = psVectorAllocEmpty(radMin->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *binSBstdev = psVectorAllocEmpty(radMin->n, PS_TYPE_F32); // surface brightness error of radial bin
    psVector *binSum     = psVectorAllocEmpty(radMin->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *binFill    = psVectorAllocEmpty(radMin->n, PS_TYPE_F32); // surface brightness of radial bin
    psVector *binRad  	 = psVectorAllocEmpty(radMin->n, PS_TYPE_F32); // mean radius of radial bin
    psVector *binArea 	 = psVectorAllocEmpty(radMin->n, PS_TYPE_F32); // area of radial bin (contiguous, non-overlapping)

    psVectorInit (binSB, 0.0);
    psVectorInit (binSBstdev, 0.0);
    psVectorInit (binSum, 0.0);
    psVectorInit (binFill, 0.0);

    psVectorInit (binRad, 0.0);

    // generate radial area-weighted mean radius & non-overlapping areas
    for (int i = 0; i < radMin->n; i++) {
	float rMin2 = PS_SQR(radMin->data.F32[i]);
	float rMax2 = PS_SQR(radMax->data.F32[i]);

	float rMin3 = rMin2*radMin->data.F32[i];
	float rMax3 = rMax2*radMax->data.F32[i];

	float rBin = 2.0 * (rMax3 - rMin3) / (rMax2 - rMin2) / 3.0;
	
	// XXX calculate area-weighted radius rather than asserting?
	binRad->data.F32[i] = rBin;
	binArea->data.F32[i] = M_PI * (rMax2 - rMin2);
    }

    // storage vector for stats
    psVector *values = psVectorAllocEmpty (flux->n, PS_TYPE_F32);
    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);

    float fluxSum = 0.0;
    int nPixSum = 0;

    bool done = false;
    int nOut = 0;
    float Rmin = radMin->data.F32[nOut];
    float Rmax = radMax->data.F32[nOut];
    float Rnxt = radMin->data.F32[nOut+1];  // minimum radius for next range
    int iNext = 0;
    int iLast = -1;
    for (int i = 0; !done && (i < radius->n); i++) {
	if (radius->data.F32[i] < Rnxt) {
	    iNext = i; // on the next pass, we will start back here to ensure we hit all pixels in the next bin
	}
	if (radius->data.F32[i] > Rmax) {
	    // calculate the surface brightness for bin 'nOut'
	    float value, dvalue;
	    if (values->n > 0) {
		psVectorStats (stats, values, NULL, NULL, 0);
		value = stats->robustMedian;
		dvalue = stats->robustStdev;
	    } else {
		value = NAN;
		dvalue = NAN;
	    }

	    binSB->data.F32[nOut] = value;
	    binSBstdev->data.F32[nOut] = sqrt(PS_SQR(dvalue) / values->n + skyModelErrorSQ);

	    // calculate the total flux for bin 'nOut'
	    float Area = M_PI*AxialRatio*PS_SQR(Rmax);
	    binSum->data.F32[nOut] = fluxSum;
	    binFill->data.F32[nOut] = nPixSum / Area;

	    psTrace ("psphot", 5, "radial bins: %3d  %5.1f %5.1f : %7.1f  %6.2f : %8.1f %4.2f %6.1f\n", 
		     nOut, radMin->data.F32[nOut], radMax->data.F32[nOut], 
		     binSB->data.F32[nOut], binSBstdev->data.F32[nOut], 
		     binSum->data.F32[nOut], binFill->data.F32[nOut], Area);

	    nOut ++;
	    if (nOut >= radMin->n) break;
	    Rmin = radMin->data.F32[nOut];
	    Rmax = radMax->data.F32[nOut];
	    Rnxt = (nOut < radMin->n - 1) ? radMin->data.F32[nOut+1] : Rmax;  // minimum radius for next range
	    values->n = 0;
	    psStatsInit(stats);
	    iLast = i;
	    i = iNext;
	}
	if (radius->data.F32[i] < Rmin) {
	    continue;
	}
	psVectorAppend (values, flux->data.F32[i]);

	if (i > iLast) {
	    fluxSum += flux->data.F32[i];
	    nPixSum ++;
	}
    }
    binSB->n = binSBstdev->n = binSum->n = binRad->n = binArea->n = nOut;

    // interpolate any bins that were empty (extrapolate to center if needed)
    if (!isfinite(binSB->data.F32[0]) && !isfinite(binSB->data.F32[1])) {
	psWarning ("center 2 bins of source at %f, %f are NAN, skipping this source", source->peak->xf, source->peak->yf);
	// XXX raise a flag
	psFree(binSB);
	psFree(binSBstdev);

	psFree(binSum);
	psFree(binFill);

	psFree(binRad);
	psFree(binArea);
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

    psFree(profile->binSum);
    psFree(profile->binFill);

    psFree(profile->radialBins);
    psFree(profile->area);

    // save the vectors
    profile->binSB      = binSB;
    profile->binSBstdev = binSBstdev;

    profile->binSum     = binSum;
    profile->binFill    = binFill;

    profile->radialBins = binRad;
    profile->area       = binArea;

    psFree(values);
    psFree(stats);

    return true;
}

// If the number of sources is large, edit the recipe to remove radial bins beyond
// a value specified in the recipe
bool psphotLimitRadialApertures(psMetadata *recipe, long nSources) {

    bool status = false;
    long sourceLimit = psMetadataLookupS32 (&status, recipe, "RADIAL.NUM.SOURCES.LIMIT");
    if (!status) {
        sourceLimit = 50000;
    }
    if (nSources < sourceLimit) {
        return true;
    }
    psF32 maxRadius = psMetadataLookupF32 (&status, recipe, "RADIAL.SOURCES.OVER.LIMIT.RADIUS");
    if (!status) {
        maxRadius = 20.0;
    }
    psLogMsg ("psphot", PS_LOG_INFO, "Number of objects: %ld is greater than limit %ld. Limiting radial annular bins to %.3f pixels\n", 
            nSources, sourceLimit, maxRadius);

    psVector *radMin = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.LOWER");
    psVector *radMax = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.UPPER");
    if (!radMin || !radMin->n) {
	psError (PSPHOT_ERR_CONFIG, true, "error in definition of annular bins (radMin missing or empty)");
	return false;
    }
    if (!radMax || !radMax->n) {
	psError (PSPHOT_ERR_CONFIG, true, "error in definition of annular bins (radMax missing or empty)");
	return false;
    }
    if (radMax->n != radMin->n) {
	psError (PSPHOT_ERR_CONFIG, true, "length of radMin %ld and radMax %ld not equal)", radMin->n, radMax->n);
	return false;
    }
    int i = 0;
    psF32 lastRadius = 0;
    for (; i < radMax->n; i++) {
        if (radMax->data.F32[i] > maxRadius) {
            break;
        }
        lastRadius = radMax->data.F32[i];
    }
    if (i == radMax->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "Radius of all bins is within the limit. lastRadius: %.3f\n", lastRadius);
        return true;
    }
    psLogMsg ("psphot", PS_LOG_INFO, "  radius limit exceeded at bin %d of %ld. New max radius is %.3f\n",
            i, radMax->n, lastRadius);

    psVector *radMinNew = psVectorRealloc(radMin, i);
    if (radMinNew != radMin) {
        psMetadataAddVector(recipe, PS_LIST_TAIL, "RADIAL.ANNULAR.BINS.LOWER", PS_META_REPLACE, "", radMinNew);
        // XXX: I don't need this so I?
        // psFree(radMinNew);
    }

    psVector *radMaxNew = psVectorRealloc(radMax, i);
    if (radMaxNew != radMax) {
        psMetadataAddVector(recipe, PS_LIST_TAIL, "RADIAL.ANNULAR.BINS.UPPER", PS_META_REPLACE, "", radMaxNew);
        // XXX: I don't need to free this do I?
        // psFree(radMaxNew);
    }

    return true;
}

// the area-weighted mean radius is given by:

// integral r * 2 pi r dr / integral 2 pi r dr

// = 2/3 pi (r_max^3 - r_min^3)  / pi (r_max^2 - r_min^2) 
// = 2/3 (r_max^3 - r_min^3) / (r_max^2 - r_min^2)


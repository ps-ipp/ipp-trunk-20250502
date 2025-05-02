# include "psphotInternal.h"

// Given the Radial Profiles (radii, fluxes) determine the radius for each profile at the desired isophote

bool psphotRadiiFromProfiles (pmSource *source, float fluxMin, float fluxMax) {

    psAssert (source, "missing source");
    psAssert (source->extpars, "missing extpars");
    psAssert (source->extpars->radFlux, "missing radFlux");

    pmSourceRadialFlux *profile = source->extpars->radFlux;

    psFree(profile->isophotalRadii);
    profile->isophotalRadii = psVectorAlloc(profile->theta->n, PS_TYPE_F32);

    for (int i = 0; i < profile->theta->n; i++) {
	psVector *radii = profile->radii->data[i];
	psVector *fluxes = profile->fluxes->data[i];
	float radius = psphotRadiusFromProfile (source, radii, fluxes, fluxMin, fluxMax);

	// psphotPetrosianVisualProfileByAngle (radii, fluxes, radius);

	// warn on NAN?
	profile->isophotalRadii->data.F32[i] = radius;
    }
    return true;
}

float psphotRadiusFromProfile (pmSource *source, psVector *radius, psVector *flux, float fluxMin, float fluxMax) {

    // 'flux' is a noisy sample of the galaxy radial profile at points 'radius'
    // rebin flux into samples defined by the isophote Fo = 0.5*(fluxMax + fluxMin).  the noisy
    // sample is cleaned by rebinning to a well-matched radial binning

    // base selections on fluxes defined by the flux range dF
    float fluxRange = fluxMax - fluxMin;

    // examine data in the two ranges Fm - Fo and Fo - Fp to define the bin size
    // XXX reconsider the fractional isophote value
    float Fm = fluxMin + 0.10*fluxRange;
    float Fo = fluxMin + 0.25*fluxRange;
    float Fp = fluxMin + 0.50*fluxRange;
    int Rbin = 1;
      
    // find the median radius of the points in the flux range Fm - Fp:
    { 
	// storage vector for stats
	psVector *values = psVectorAllocEmpty (flux->n, PS_TYPE_F32);
	psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);

	for (int i = 0; i < flux->n; i++) {
	    if (!isfinite(flux->data.F32[i])) continue;
	    if (flux->data.F32[i] < Fm) continue;
	    if (flux->data.F32[i] > Fp) continue;
	    
	    psVectorAppend (values, radius->data.F32[i]);
	}
	if (values->n > 1) {
	    psVectorStats (stats, values, NULL, NULL, 0);

	    // if we have a valid range, rebin with bin size 1/2 of median radius
	    if (isfinite(stats->sampleMedian)) {
		Rbin = MAX(1, 0.5*stats->sampleMedian);
	    }
	}
	psFree (values);
	psFree (stats);
    }
    Rbin = 3;

    psVector *fluxBinned = NULL;
    psVector *radiusBinned = NULL;

    // do not bother rebinning if the bin size is only 2 or less
    if (Rbin <= 2) {
	fluxBinned = psMemIncrRefCounter (flux);
	radiusBinned = psMemIncrRefCounter (radius);
    } else {
	// storage vector for stats
	psVector *values = psVectorAllocEmpty (flux->n, PS_TYPE_F32);
	psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
  
	// rebinned vectors
	fluxBinned = psVectorAllocEmpty (flux->n, PS_TYPE_F32);
	radiusBinned = psVectorAllocEmpty (flux->n, PS_TYPE_F32);

	// sort the flux by the radius
	pmSourceRadialProfileSortPair (radius, flux);

	int nOut = 0;
	radiusBinned->data.F32[nOut] = (nOut + 0.5)*Rbin;
	float Rmin = radiusBinned->data.F32[nOut] - 0.5*Rbin;
	float Rmax = radiusBinned->data.F32[nOut] + 0.5*Rbin;

	for (int i = 0; i < flux->n; i++) {
	    if (radius->data.F32[i] < Rmin) {
		// XXX not sure how we can hit this, if there is full coverage of radiusBinned
		continue;
	    }
	    if (radius->data.F32[i] > Rmax) {
		// calculate the value for the nOut bin
		// XXX need to fix this as well psStats (stats, values);
		float value;
		if (values->n > 0) {
		    psVectorStats (stats, values, NULL, NULL, 0);
		    value = stats->sampleMedian;
		} else {
		    value = NAN;
		}
		fluxBinned->data.F32[nOut] = value;
		nOut ++;
		radiusBinned->data.F32[nOut] = (nOut + 0.5)*Rbin;
		Rmin = radiusBinned->data.F32[nOut] - 0.5*Rbin;
		Rmax = radiusBinned->data.F32[nOut] + 0.5*Rbin;
		values->n = 0;
		psStatsInit(stats);
	    }
	    if (!isfinite(flux->data.F32[i])) continue;
	    psVectorAppend (values, flux->data.F32[i]);
	}
	fluxBinned->n = nOut;
	radiusBinned->n = nOut;
	psFree (values);
	psFree(stats);
    }

    float Ro = NAN;
    bool above = true;
    for (int i = 0; i < fluxBinned->n; i++) {

	if (!isfinite(fluxBinned->data.F32[i])) continue;

	// find the largest radius that matches the flux transition
	if (above && (fluxBinned->data.F32[i] < Fo)) {
	    // XXX is there a macro in psLib that does this interpolation?
	    if (i == 0) { 
		psTrace ("psphot", 4, "bogus radial profile for source at %f, %f, skipping", source->peak->xf, source->peak->yf);
		psFree (fluxBinned);
		psFree (radiusBinned);
		return NAN;
	    } 
	    Ro = radiusBinned->data.F32[i-1] + (radiusBinned->data.F32[i] - radiusBinned->data.F32[i-1]) * (Fo - fluxBinned->data.F32[i-1]) / (fluxBinned->data.F32[i] - fluxBinned->data.F32[i-1]);
	    above = FALSE;
	}
  
	if (!above && (fluxBinned->data.F32[i] >= Fo)) {
	    above = TRUE;
	}
    }

    // show the results
    // psphotPetrosianVisualProfileRadii (radius, flux, radiusBinned, fluxBinned, fluxMax, Ro);

    psFree(fluxBinned);
    psFree(radiusBinned);
    return Ro;
}




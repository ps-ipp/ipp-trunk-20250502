# include "psphotInternal.h"

# define PETROSIAN_RATIO 0.2
# define PETROSIAN_RADII 2.0

// generate the Petrosian radius and flux from the mean surface brightness (r_i)

float InterpolateValuesQuadratic (float *Xin, float *Yin, float X);
float InterpolateValues     (float X0, float Y0, float X1, float Y1, float X);
float InterpolateValuesErrX (float X0, float Y0, float X1, float Y1, float X, float dX0, float dX1);
float InterpolateValuesErrY (float X0, float Y0, float X1, float Y1, float X, float dY0, float dY1);

bool psphotPetrosianStats (pmSource *source) {

    psAssert (source, "missing source");
    psAssert (source->extpars, "missing extpars");
    psAssert (source->extpars->petProfile, "missing petProfile");

    pmSourceRadialProfile *profile = source->extpars->petProfile;

    if (!profile->binSB) {
	psLogMsg ("psphot", PS_LOG_DETAIL, "no petrosian profile, skipping source %f, %f", source->peak->xf, source->peak->yf);
	source->mode2 |= PM_SOURCE_MODE2_PETRO_NO_PROFILE;
	return true;
    }

    psVector *binSB      = profile->binSB;
    psVector *binSBstdev = profile->binSBstdev;
    psVector *binRad     = profile->radialBins;
    psVector *area       = profile->area;
    psVector *binFill    = profile->binFill;

    psVector *fluxSum     = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *fluxSumErr2 = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *refRadius   = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *petRatio    = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *petRatioErr = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *meanSB      = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *areaSum     = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);
    psVector *apixSum     = psVectorAllocEmpty(binSB->n, PS_TYPE_F32);

    float petRadius = NAN;
    float petRadiusErr = NAN;
    float petFlux = NAN;
    float petFluxErr = NAN;
    float petArea = NAN;
    float petApix = NAN;

    bool anyPetro = false;
    // bool manyPetro = false;  XXX not used
    bool above = true;
    float Asum = 0.0;
    float Psum = 0.0;
    float Fsum = 0.0;
    float dFsum2 = 0.0;

    float nSigma = 2.0;
    int lowestSignificantRadius = 0;
    float lowestSignificantRatio = 1.0;

    // find the Petrosian Radius and Petrosian Flux

    int nOut = 0;
    for (int i = 0; i < binSB->n; i++) {
	// skip nan bins (do not contribute to flux or area)
	if (!isfinite(binSB->data.F32[i])) continue;

	float Area = area->data.F32[i];
	Asum += Area; 			// Asum is the cumulative area interior to this bin
	Fsum += binSB->data.F32[i] * Area;
	dFsum2 += PS_SQR(binSBstdev->data.F32[i] * Area);
	Psum += Area*binFill->data.F32[i]; // Psum is the cumulative number of pixels interior to this bin

	float areaInner = 0.5 * Area;
	float fluxInner = 0.5 * Area * binSB->data.F32[i];
	float fluxInnerErr2 = PS_SQR(binSBstdev->data.F32[i] * 0.5 * Area);
	if (nOut > 0) {
	    areaInner += areaSum->data.F32[nOut-1];
	    fluxInner += fluxSum->data.F32[nOut-1];
	    fluxInnerErr2 += fluxSumErr2->data.F32[nOut-1];
	}

	// ratio = binSB / meanSB
	// meanSB = flux / area
	// flux = sum(binSB(i) * area(i)
	// fluxErr^2 = sum(binSBerr(i)^2 area(i)^2)
	// meanSBerr^2 = fluxErr^2 / area^2
	// (ratioErr/ratio)^2 = (binSBerr/binSB)^2 + (meanSBerr/meanSB)^2

	psVectorAppend(meanSB, (fluxInner / areaInner));

	float ratio = binSB->data.F32[i] / meanSB->data.F32[nOut];
	psVectorAppend(petRatio, ratio);

	float meanSBerr = sqrt(fluxInnerErr2) / areaInner;
	float ratioErr = fabs(ratio) * sqrt(PS_SQR(binSBstdev->data.F32[i]/binSB->data.F32[i]) + PS_SQR(meanSBerr/meanSB->data.F32[nOut]));

	psVectorAppend(petRatioErr, ratioErr);

	psVectorAppend(areaSum, Asum);
	psVectorAppend(apixSum, Psum);
	psVectorAppend(fluxSum, Fsum);
	psVectorAppend(fluxSumErr2, dFsum2);
	psVectorAppend(refRadius, binRad->data.F32[i]);

	psTrace ("psphot", 4, "%3d : %5.2f : %5.3f %5.3f : %5.3f %5.3f : %5.3f %5.3f : %5.3f %5.3f : %5.1f %5.1f  %5.1f\n", 
		 i, refRadius->data.F32[nOut], 
		 binSB->data.F32[i], binSBstdev->data.F32[i], 
		 meanSB->data.F32[nOut], meanSBerr, 
		 petRatio->data.F32[nOut], petRatioErr->data.F32[nOut], 
		 fluxSum->data.F32[nOut], sqrt(fluxSumErr2->data.F32[nOut]), areaSum->data.F32[nOut], apixSum->data.F32[nOut], areaInner);
    
	// anytime we transition below the PETROSIAN_RATIO, calculate the radius and flux
	// we will keep and report the last (largest radius) value
	if (above && (petRatio->data.F32[nOut] < PETROSIAN_RATIO) && (petRatio->data.F32[nOut] > nSigma*petRatioErr->data.F32[nOut])) {
	    // interpolate Rvec between i-1 and i to PETROSIAN_RATIO to get flux (Fvec) and radius (rvec)
	    if (i == 0) { 
		// assume Fmax @ R = 0.0
		petRadius    = InterpolateValues     (1.0, 0.0, petRatio->data.F32[nOut], refRadius->data.F32[nOut], PETROSIAN_RATIO);
		petRadiusErr = InterpolateValuesErrX (1.0, 0.0, petRatio->data.F32[nOut], refRadius->data.F32[nOut], PETROSIAN_RATIO, 0.0, petRatioErr->data.F32[nOut]);
		source->mode2 |= PM_SOURCE_MODE2_PETRO_RATIO_ZEROBIN;
	    } else {
	      // petRadius    = InterpolateValues     (petRatio->data.F32[nOut-1], refRadius->data.F32[nOut-1], petRatio->data.F32[nOut], refRadius->data.F32[nOut], PETROSIAN_RATIO);
	      if (nOut > 1) {
		petRadius    = InterpolateValuesQuadratic (&petRatio->data.F32[nOut-2], &refRadius->data.F32[nOut-2],   PETROSIAN_RATIO);
	      } else {
		petRadius    = InterpolateValuesQuadratic (&petRatio->data.F32[nOut-3], &refRadius->data.F32[nOut-3], PETROSIAN_RATIO);
	      }
# if (PS_TRACE_ON)
	      float petRadiusLinear = InterpolateValues     (petRatio->data.F32[nOut-1], refRadius->data.F32[nOut-1], petRatio->data.F32[nOut], refRadius->data.F32[nOut], PETROSIAN_RATIO);
	      if (fabs(petRadius - petRadiusLinear) > fabs(refRadius->data.F32[nOut] - refRadius->data.F32[nOut-1])) { 
		fprintf (stderr, "big difference : %f vs %f\n", petRadius, petRadiusLinear); 
	      }
#endif
	      petRadiusErr = InterpolateValuesErrX (petRatio->data.F32[nOut-1], refRadius->data.F32[nOut-1], petRatio->data.F32[nOut], refRadius->data.F32[nOut], PETROSIAN_RATIO, petRatioErr->data.F32[nOut-1], petRatioErr->data.F32[nOut]);
	    }
	    above = false;
	    // if (anyPetro) manyPetro = true;
	    anyPetro = true;
	}
    
	// anytime we transition below the PETROSIAN_RATIO, calculate the radius and flux
	// we will keep and report the last (largest radius) value
	// find the last signficant measurement of the petrosian ratio
	if (above && (petRatio->data.F32[nOut] < lowestSignificantRatio) && (petRatio->data.F32[nOut] > nSigma*petRatioErr->data.F32[nOut])) {
	    lowestSignificantRadius = nOut;
	    lowestSignificantRatio = petRatio->data.F32[nOut];
	}
    
	// reset on transitions up, but do not re-calculate rad_90, flux_90
	if (!above && (petRatio->data.F32[nOut] >= PETROSIAN_RATIO)) {
	    above = true;
	}
	nOut ++;
    }

    // if we failed to reach the PETROSIAN_RATIO, use the lowest significant ratio instead (flag this!)
    if (!anyPetro) {
	petRadius = refRadius->data.F32[lowestSignificantRadius];
	petRadiusErr = NAN;
	if (!isfinite(petRadius)) {
	    fprintf (stderr, "nan pet radius\n");
	}
	source->mode2 |= PM_SOURCE_MODE2_PETRO_INSIG_RATIO;
    }

    // now measure the flux within PETROSIAN_RADII * petRadius 
    float apRadius = PETROSIAN_RADII * petRadius;
    for (int i = 0; i < refRadius->n; i++) {
	// XXX use bisection to do this faster:
	if (refRadius->data.F32[i] > apRadius) {
	    if (i == 0) {
		psWarning ("does this case make any sense? (refRadius[0] %f > apRadius %f)", refRadius->data.F32[i], apRadius);
		continue;
	    } else {
		petFlux    = InterpolateValues     (refRadius->data.F32[i-1], fluxSum->data.F32[i-1], refRadius->data.F32[i], fluxSum->data.F32[i], apRadius);
		petFluxErr = InterpolateValuesErrY (refRadius->data.F32[i-1], fluxSum->data.F32[i-1], refRadius->data.F32[i], fluxSum->data.F32[i], apRadius, sqrt(fluxSumErr2->data.F32[i-1]), sqrt(fluxSumErr2->data.F32[i]));
		petArea    = InterpolateValues     (refRadius->data.F32[i-1], areaSum->data.F32[i-1], refRadius->data.F32[i], areaSum->data.F32[i], apRadius);
		petApix    = InterpolateValues     (refRadius->data.F32[i-1], apixSum->data.F32[i-1], refRadius->data.F32[i], apixSum->data.F32[i], apRadius);
		if (!isfinite(petFlux)) {
		    fprintf (stderr, "nan pet flux\n");
		}
		break;
	    }
	}
    }

    // now measure the radii R90 and R50 where flux = 0.9 (or 0.5) * petFlux;
    float flux90 = 0.9 * petFlux;
    float flux50 = 0.5 * petFlux;
    float R50 = NAN;
    float R90 = NAN;
    float R50err = NAN;
    float R90err = NAN;
    bool found50 = false;
    bool found90 = false;

    // XXX use bisection to do this faster:
    for (int i = 0; !(found50 && found90) && i < refRadius->n; i++) {
	if (!found50 && (fluxSum->data.F32[i] > flux50)) {
	    if (i == 0) {
		R50    = InterpolateValues     (fluxSum->data.F32[i], refRadius->data.F32[i], fluxSum->data.F32[i+1], refRadius->data.F32[i+1], flux50);
		R50err = InterpolateValuesErrX (fluxSum->data.F32[i], refRadius->data.F32[i], fluxSum->data.F32[i+1], refRadius->data.F32[i+1], flux50, sqrt(fluxSumErr2->data.F32[i]), sqrt(fluxSumErr2->data.F32[i+1]));
		found50 = true;
	    } else {
		R50    = InterpolateValues     (fluxSum->data.F32[i-1], refRadius->data.F32[i-1], fluxSum->data.F32[i], refRadius->data.F32[i], flux50);
		R50err = InterpolateValuesErrX (fluxSum->data.F32[i-1], refRadius->data.F32[i-1], fluxSum->data.F32[i], refRadius->data.F32[i], flux50, sqrt(fluxSumErr2->data.F32[i-1]), sqrt(fluxSumErr2->data.F32[i]));
		found50 = true;
	    }
	}
	if (!found90 && (fluxSum->data.F32[i] > flux90)) {
	    if (i == 0) {
		R90    = InterpolateValues     (fluxSum->data.F32[i], refRadius->data.F32[i], fluxSum->data.F32[i+1], refRadius->data.F32[i+1], flux90);
		R90err = InterpolateValuesErrX (fluxSum->data.F32[i], refRadius->data.F32[i], fluxSum->data.F32[i+1], refRadius->data.F32[i+1], flux90, sqrt(fluxSumErr2->data.F32[i]), sqrt(fluxSumErr2->data.F32[i+1]));
		found90 = true;
	    } else {
		R90    = InterpolateValues     (fluxSum->data.F32[i-1], refRadius->data.F32[i-1], fluxSum->data.F32[i], refRadius->data.F32[i], flux90);
		R90err = InterpolateValuesErrX (fluxSum->data.F32[i-1], refRadius->data.F32[i-1], fluxSum->data.F32[i], refRadius->data.F32[i], flux90, sqrt(fluxSumErr2->data.F32[i-1]), sqrt(fluxSumErr2->data.F32[i]));
		found90 = true;
	    }
	}
    }


    // XXX save flags (anyPetro, manyPetro)
    source->extpars->petrosianRadius = petRadius;
    source->extpars->petrosianFlux   = petFlux;
    source->extpars->petrosianR50    = R50;
    source->extpars->petrosianR90    = R90;
    source->extpars->petrosianFill   = petApix / petArea;
    
    // XXX add the errors
    source->extpars->petrosianRadiusErr = petRadiusErr;
    source->extpars->petrosianFluxErr   = petFluxErr;
    source->extpars->petrosianR50Err    = R50err;
    source->extpars->petrosianR90Err    = R90err;

    // fprintf (stderr, "source @ %f,%f\n", source->peak->xf, source->peak->yf);
    psphotPetrosianVisualStats (binRad, binSB, refRadius, meanSB, petRatio, petRatioErr, fluxSum, petRadius, PETROSIAN_RATIO, petFlux, apRadius);

    psFree(fluxSum);
    psFree(fluxSumErr2);
    psFree(refRadius);
    psFree(petRatio);
    psFree(petRatioErr);
    psFree(meanSB);
    psFree(areaSum);
    psFree(apixSum);

    return true;
}

// Lagrange's form of the interpolating polynomial...
float InterpolateValuesQuadratic (float *Xin, float *Yin, float X) {

  float dx01 = Xin[0] - Xin[1];
  float dx02 = Xin[0] - Xin[2];
  float dx12 = Xin[1] - Xin[2];

  float dx0  = X - Xin[0];
  float dx1  = X - Xin[1];
  float dx2  = X - Xin[2];

  float y0 = Yin[0]*dx1*dx2/(dx01*dx02);
  float y1 = Yin[1]*dx0*dx2/(dx01*dx12); // need - sign
  float y2 = Yin[2]*dx0*dx1/(dx02*dx12); 

  float Y = y0 - y1 + y2;
  return Y;
}

float InterpolateValues (float X0, float Y0, float X1, float Y1, float X) {
    float dydx = (Y1 - Y0) / (X1 - X0);
    float Y = Y0 + dydx * (X - X0);
    return Y;
}

float InterpolateValuesErrX (float X0, float Y0, float X1, float Y1, float X, float dX0, float dX1) {

    float dydx = (Y1 - Y0) / (X1 - X0);
    float dxdx = (X  - X0) / (X1 - X0);
    
    float dY = sqrt(PS_SQR(dX1*dydx*dxdx) + PS_SQR(dX0*dydx*(dxdx - 1.0)));
    return dY;
}

float InterpolateValuesErrY (float X0, float Y0, float X1, float Y1, float X, float dY0, float dY1) {

    float dxdx = (X  - X0) / (X1 - X0);
    
    float dY = sqrt(PS_SQR(dY1*dxdx) + PS_SQR(dY0*(1.0 - dxdx)));
    return dY;
}


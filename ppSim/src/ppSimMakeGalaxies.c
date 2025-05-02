# include "ppSim.h"

bool ppSimSetGalaxyPeak (ppSimGalaxy *galaxy, pmModelType type, float index, float seeing);

bool ppSimMakeGalaxies(psArray *galaxies, pmFPA *fpa, pmConfig *config, const psRandom *rng) {

    bool mdok;
    assert (galaxies);

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    bool galaxyFake = psMetadataLookupBool(&mdok, recipe, "GALAXY.FAKE"); // Density of fakes
    if (!galaxyFake) return true;

    float galaxyAlpha     = psMetadataLookupF32(&mdok, recipe, "GALAXY.LUM"); // Galaxy luminosity func slope
    float brightMag       = psMetadataLookupF32(&mdok, recipe, "GALAXY.MAG"); // Galaxy brightest magnitude
    float galaxyDensity   = psMetadataLookupF32(&mdok, recipe, "GALAXY.DENSITY"); // Density of fakes

    bool galaxyGrid  	  = psMetadataLookupBool(&mdok, recipe, "GALAXY.GRID"); // Density of fakes
    int galaxyGridDX 	  = psMetadataLookupS32 (&mdok, recipe, "GALAXY.GRID.DX"); // Density of fakes
    int galaxyGridDY 	  = psMetadataLookupS32 (&mdok, recipe, "GALAXY.GRID.DY"); // Density of fakes
    bool galaxyGridRandom = psMetadataLookupBool(&mdok, recipe, "GALAXY.GRID.RANDOM"); // Density of fakes
    // float galaxyGridPeak  = psMetadataLookupF32 (&mdok, recipe, "GALAXY.GRID.PEAK"); // peak flux of fakes
    float galaxyGridMag   = psMetadataLookupF32 (&mdok, recipe, "GALAXY.GRID.MAG"); // peak flux of fakes
    
    float galaxyGridXoff  = psMetadataLookupF32 (&mdok, recipe, "GALAXY.GRID.XOFF"); // centroid offset
    float galaxyGridYoff  = psMetadataLookupF32 (&mdok, recipe, "GALAXY.GRID.YOFF"); // centroid offset

    float galaxyRmajorMax = psMetadataLookupF32(&mdok, recipe, "GALAXY.RMAJOR.MAX"); // Density of fakes
    float galaxyRmajorMin = psMetadataLookupF32(&mdok, recipe, "GALAXY.RMAJOR.MIN"); // Density of fakes

    float galaxyARatioMax = psMetadataLookupF32(&mdok, recipe, "GALAXY.ARATIO.MAX"); // Density of fakes
    float galaxyARatioMin = psMetadataLookupF32(&mdok, recipe, "GALAXY.ARATIO.MIN"); // Density of fakes

    float galaxyThetaMax  = psMetadataLookupF32(&mdok, recipe, "GALAXY.THETA.MAX"); // Density of fakes
    float galaxyThetaMin  = psMetadataLookupF32(&mdok, recipe, "GALAXY.THETA.MIN"); // Density of fakes
    galaxyThetaMax *= PS_RAD_DEG;
    galaxyThetaMin *= PS_RAD_DEG;

    float galaxyIndexMin  = psMetadataLookupF32(&mdok, recipe, "GALAXY.INDEX.MIN"); // Density of fakes
    float galaxyIndexMax  = psMetadataLookupF32(&mdok, recipe, "GALAXY.INDEX.MAX"); // Density of fakes

    // float darkRate 	  = psMetadataLookupF32(&mdok, recipe, "DARK.RATE"); // Dark rate
    float expTime  	  = psMetadataLookupF32(&mdok, recipe, "EXPTIME"); // Exposure time
    float zp       	  = psMetadataLookupF32(&mdok, recipe, "ZEROPOINT"); // Photometric zero point
    float seeing   	  = psMetadataLookupF32(&mdok, recipe, "SEEING"); // Seeing sigma (pix)
    float scale    	  = psMetadataLookupF32(&mdok, recipe, "PIXEL.SCALE"); // Plate scale (arcsec/pixel)
    float skyRate  	  = psMetadataLookupF32(&mdok, recipe, "SKY.RATE"); // Sky rate
    if (isnan(skyRate)) {
	float skyMags = psMetadataLookupF32(&mdok, recipe, "SKY.MAGS");  assert (mdok);
	skyRate = scale * scale * ppSimMagToFlux (skyMags, zp);
	// skyMags is in mags / square arcsec so scale must be arcsec / pixel
    }

    // determine the galaxy model
    char *modelName = psMetadataLookupStr(&mdok, recipe, "GALAXY.MODEL"); // galaxy model name
    pmModelType type = pmModelClassGetType (modelName);
    if (type == -1) {
	psError (PS_ERR_UNKNOWN, false, "invalid model name");
        return false;
    }

    if (galaxyDensity <= 0) return true;

    if (galaxyGridRandom) {
	long A, B;
	A = time(NULL);
	for (B = 0; A == time(NULL); B++);
	srand48(B);
    }

    // Size of FPA
    psRegion *bounds = ppSimFPABounds (fpa);
    // Size of focal plane (can't I get this from the config?)
    int xSize = bounds->x1 - bounds->x0;
    int ySize = bounds->y1 - bounds->y0;
    psFree(bounds);

    // Grabbing read noise from the recipe rather than the cell, which is a potential danger, but it
    // shouldn't be too bad.
    // float readnoise = psMetadataLookupF32(&mdok, recipe, "READNOISE"); // Default read noise
    // if (!mdok) {
    // 	psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find READNOISE in recipe.");
    // 	psFree(bounds);
    // 	return false;
    // }

    // faintest and brightest magnitudes for random galaxies
    // float brightFlux = ppSimMagToFlux (brightMag, zp) * expTime;
    float faintMag = brightMag + 5.0;
    if (brightMag > faintMag) {
	psLogMsg("ppSim", PS_LOG_INFO, "Image noise is above brightest random galaxy --- no random galaxy added.");
	return true;
    }

    // Normalisation, set by the specified stellar density at the specified bright magnitude
    float refSum = galaxyDensity * xSize * ySize * PS_SQR(scale / 3600.0);
    float normLum =  refSum / (galaxyAlpha * logf(10.0) * powf(10.0, (galaxyAlpha * brightMag)));
    float normScale = normLum * galaxyAlpha * logf(10.0);

    // Total number of galaxy down to the faint flux end
    long nTotal = 0;
    if (galaxyAlpha < 0.1) {
	nTotal = 100;
    } else {
	nTotal = normScale * powf (10.0, (galaxyAlpha * faintMag));
    }

    if (galaxyGrid) {
	fprintf (stderr, "galaxy grid inst mag: %f\n", galaxyGridMag - zp - 2.5*log10(expTime));
	nTotal = (int)(xSize / galaxyGridDX) * (int)(ySize / galaxyGridDY);
    
	psLogMsg("ppSim", PS_LOG_INFO, "Adding grid of %ld galaxies\n", nTotal);

	float galaxyIndexSlope = (galaxyIndexMax - galaxyIndexMin);
	float galaxyThetaSlope = (galaxyThetaMax - galaxyThetaMin);
	float galaxyARatioSlope = (galaxyARatioMax - galaxyARatioMin);
	float galaxyRmajorSlope = (galaxyRmajorMax - galaxyRmajorMin);

	int i = 0;
	// float refNorm = 1.0;
	// float ourNorm = 1.0;

	for (long iy = 0.5*galaxyGridDY; iy < ySize; iy += galaxyGridDY) {
	    for (long ix = 0.5*galaxyGridDX; ix < xSize; ix += galaxyGridDX) {
		ppSimGalaxy *galaxy = ppSimGalaxyAlloc ();

		// make fpa center of distribution
		// center the galaxy peak on the center of a pixel
		galaxy->x    = ix + 0.5 + galaxyGridXoff;
		galaxy->y    = iy + 0.5 + galaxyGridYoff;

		float galaxyGridFlux = expTime * powf(10.0, -0.4 * (galaxyGridMag - zp));

		// galaxyIndex from user should be for function of this form: exp(-r^(1/n))
		// galaxy->index = (1/2n)

		float rndValue;

		rndValue = galaxyGridRandom ? drand48() : i / (float) nTotal;
		float index = (galaxyIndexMin  + rndValue * galaxyIndexSlope);
		galaxy->index = 0.5/index; // factor of 0.5 because the Sersic model creates exp(-z^n), not exp(-r^n)

		rndValue = galaxyGridRandom ? drand48() : i / (float) nTotal;
		float scale = (galaxyRmajorMin + rndValue * galaxyRmajorSlope);
		galaxy->Rmaj  = scale;

		rndValue = galaxyGridRandom ? drand48() : i / (float) nTotal;
		galaxy->Rmin  = (galaxyARatioMin + rndValue * galaxyARatioSlope) * galaxy->Rmaj;

		rndValue = galaxyGridRandom ? drand48() : i / (float) nTotal;
		galaxy->theta = (galaxyThetaMin  + rndValue * galaxyThetaSlope);
		
		galaxy->flux = galaxyGridFlux;
		ppSimSetGalaxyPeak (galaxy, type, index, seeing);

		// if (0) {
		//     fprintf (stderr, "Rmaj: %f, scale: %f, index: %f, bn: %f, Ro: %f, Io: %f, theta: %f\n", galaxy->Rmaj, scale, index, bn, fR, Io, galaxy->theta);
		// }

		psArrayAdd (galaxies, 100, galaxy);
		i++;
	    }
	}
    } else {    

	fprintf (stderr, "galaxy ref inst mag: %f\n", brightMag - zp - 2.5*log10(expTime));
	float galaxyIndexSlope = (galaxyIndexMax - galaxyIndexMin);
	float galaxyThetaSlope = (galaxyThetaMax - galaxyThetaMin);
	float galaxyARatioSlope = (galaxyARatioMax - galaxyARatioMin);
	float galaxyRmajorSlope = (galaxyRmajorMax - galaxyRmajorMin);

	psLogMsg("ppSim", PS_LOG_INFO, "Adding %ld galaxies down to %f mag\n", nTotal, faintMag);

	for (long i = 0; i < nTotal; i++) {
	    ppSimGalaxy *galaxy = ppSimGalaxyAlloc ();

	    // make fpa center of distribution
	    galaxy->x    = psRandomUniform(rng) * xSize; // x position
	    galaxy->y    = psRandomUniform(rng) * ySize; // y position

	    float index = (sqrt(psRandomUniform(rng)) * galaxyIndexSlope  + galaxyIndexMin);
	    galaxy->index = 0.5/index; // factor of 0.5 because the Sersic model creates exp(-z^n), not exp(-r^n)

	    float scale = (psRandomUniform(rng) * galaxyRmajorSlope + galaxyRmajorMin);
	    galaxy->Rmaj  = scale;
	    galaxy->Rmin  = (PS_SQR(psRandomUniform(rng)) * galaxyARatioSlope + galaxyARatioMin) * galaxy->Rmaj;
	    galaxy->theta = (psRandomUniform(rng) * galaxyThetaSlope  + galaxyThetaMin);

	    // XXX probably should limit the allowed thinness of a galaxy
	    float galaxyMag =  PS_MIN (faintMag, PS_MAX (brightMag, (log10((i + 1.0) / normScale) / galaxyAlpha)));
	    
	    galaxy->flux = ppSimMagToFlux (galaxyMag, zp) * expTime;
	    ppSimSetGalaxyPeak (galaxy, type, index, seeing);

	    if (1) {
		fprintf (stderr, "Rmaj: %f, Rmin: %f, theta: %f, scale: %f, index: %f, peak: %f, mag: %f\n", galaxy->Rmaj, galaxy->Rmin, galaxy->theta*180.0/M_PI, scale, index, galaxy->peak, -2.5*log10(galaxy->flux));
	    }

	    psArrayAdd (galaxies, 100, galaxy);
	}
    }
    return true;
}

bool ppSimSetGalaxyPeak (ppSimGalaxy *galaxy, pmModelType type, float index, float seeing) {

    bool isSersicType = false;
    if (type == pmModelClassGetType ("PS_MODEL_DEV")) {
	index = 4.0;
	isSersicType = true;
    }
    if (type == pmModelClassGetType ("PS_MODEL_EXP")) {
	index = 1.0;
	isSersicType = true;
    }
    if (type == pmModelClassGetType ("PS_MODEL_SERSIC")) {
	isSersicType = true;
    }

    // exponential
    // f = I0 exp (sqrt(z)) (index = 1.0)

    // f = I0 exp (-0.5*z) 

    if (isSersicType) {
	// for a sersic model, 
	// float bn = 1.9992*index - 0.3271;
	// float Io = exp(bn);
		    
	// the integral of a Sersic has an analytical form as follows:
	// float logGamma = lgamma(2.0*index);
	// float bnFactor = pow(bn, 2.0*index);
	// float norm = 2.0 * M_PI * PS_SQR(galaxy->Rmaj) * index * Io * exp(logGamma) / bnFactor;
		    
	// find the flux of a sersic with peak = 1.0:
	float norm = pmSersicNorm (index);
	float flux = 2.0 * M_PI * galaxy->Rmaj * galaxy->Rmin * norm;

	// XXX probably should limit the allowed thinness of a galaxy
	galaxy->peak = galaxy->flux / flux;
	return true;
    }

    if (type == pmModelClassGetType ("PS_MODEL_TRAIL")) {
	// galaxy->Rmaj -> PM_PAR_LENGTH
	// seeing -> PM_PAR_SIGMA
	galaxy->Rmin = seeing; // Rmin is used for sigma
	galaxy->peak = galaxy->flux / (galaxy->Rmaj * galaxy->Rmin * 2.0 * sqrt(2.0 * M_PI));
	return true;
    }

    galaxy->peak = galaxy->flux / (2 * M_PI * galaxy->Rmaj * galaxy->Rmin);
    return true;
}

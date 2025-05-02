# include "ppSim.h"

# define ESCAPE(MSG) { \
  psError(PS_ERR_BAD_PARAMETER_VALUE, true, MSG); \
  return false; }

bool ppSimMakeStars(psArray *stars, pmFPA *fpa, pmConfig *config, const psRandom *rng) {

    bool status;
    assert (stars);

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

    bool starsFake = psMetadataLookupBool(&status, recipe, "STARS.FAKE"); // Density of fakes
    if (!starsFake) return true;

    bool starsReal     = psMetadataLookupBool(&status, recipe, "STARS.REAL"); // were real stars generated?
    bool matchDensity  = psMetadataLookupBool(&status, recipe, "MATCH.DENSITY"); // match real star density?

    float starsAlpha   = psMetadataLookupF32(&status, recipe, "STARS.LUM"); // Star luminosity func slope
    float starsDensity = psMetadataLookupF32(&status, recipe, "STARS.DENSITY"); // Density of fakes
    float brightMag    = psMetadataLookupF32(&status, recipe, "STARS.MAG"); // Star brightest magnitude
    float nSigmaLim    = psMetadataLookupF32(&status, recipe, "STARS.SIGMA.LIM"); // significance of faintest stars

    float darkRate     = psMetadataLookupF32(&status, recipe, "DARK.RATE"); // Dark rate
    float expTime      = psMetadataLookupF32(&status, recipe, "EXPTIME"); // Exposure time
    float zp           = psMetadataLookupF32(&status, recipe, "ZEROPOINT"); // Photometric zero point
    float seeing       = psMetadataLookupF32(&status, recipe, "SEEING"); // Seeing SIGMA (pixels)
    float scale        = psMetadataLookupF32(&status, recipe, "PIXEL.SCALE"); // Plate scale (arcsec/pixel)
    // there has been some confusion over pixel scale: PIXEL.SCALE is supplied in arcsec / pixel.
    // In some other places (ppSimLoadStars.c, ppSimUtils.c) it is needed in radius for WCS conversion.
    // in the past, it was converted to radians on load (but applied inconsisently elsewhere)
    // Now the radian version is carried as scaleRad to be more explicit

    if (isnan(darkRate)) darkRate = 0.0;
    if (isnan(expTime))  ESCAPE("EXPTIME is not defined");
    if (isnan(zp))       ESCAPE("ZEROPOINT is not defined");
    if (isnan(seeing))   ESCAPE("SEEING is not defined");
    if (isnan(scale))    ESCAPE("PIXEL.SCALE is not defined");

    bool flatLum       = psMetadataLookupBool(&status,recipe, "STARS.FLAT.LUM"); // use a flat luminosity function? (dn/dmag = const)
    float flatNum      = psMetadataLookupS32(&status, recipe, "STARS.FLAT.NUM"); // amplitude of flat luminosity function

    float skyRate = ppSimGetSkyRate (recipe);

    // Size of FPA
    psRegion *bounds = ppSimFPABounds (fpa);
    // Size of focal plane (can't I get this from the config?)
    int xSize = bounds->x1 - bounds->x0;
    int ySize = bounds->y1 - bounds->y0;
    psFree(bounds);

    // choose reference magnitude & density to set normalization
    float refMag = 0;
    float refSum = 0;
    if (starsReal && matchDensity) {
	refMag = psMetadataLookupF32(&status, fpa->concepts, "STARS.REAL.MAG.PEAK"); // Star brightest magnitude
	refSum = psMetadataLookupF32(&status, fpa->concepts, "STARS.REAL.SUM.PEAK"); // Star brightest magnitude

	// if we tried and failed to load reference stars, set more artificial limits
	if (!status) {
	    refMag = brightMag;
	    refSum = starsDensity * xSize * ySize * PS_SQR(scale / 3600.0);
	}
    } else {
	refMag = brightMag;
	refSum = starsDensity * xSize * ySize * PS_SQR(scale / 3600.0);
    }
    psTrace("ppSim", 6, "refMag: %f, refSum: %f\n", refMag, refSum);

    if (refSum <= 0) return true;

    // Grabbing read noise from the recipe rather than the cell, which is a potential danger, but it
    // shouldn't be too bad.
    float readnoise = psMetadataLookupF32(&status, recipe, "READNOISE"); // Default read noise
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find READNOISE in recipe.");
        psFree(bounds);
        return false;
    }

    // Faintest and brightest (integrated) fluxes (actually fluence, since integrated) for random stars

    // faint limit is set by detection limit, in turn set by sky noise
    float skySigma = sqrtf(PS_SQR(readnoise) + (darkRate + skyRate) * expTime);
    float skyNoise = ppSimStarSkyNoise (skySigma, seeing);

    // total flux of faintest star:
    float faintCounts = nSigmaLim * skyNoise;
    float faintMag = ppSimFluxToMag ((faintCounts / expTime), zp);

    float brightCounts = ppSimMagToFlux (brightMag, zp) * expTime; // Bright limit is specified by user as mag

    psTrace("ppSim", 6, "Faint limit: %f counts = %f mags\n", faintCounts, faintMag);
    psTrace("ppSim", 6, "Bright limit: %f counts = %f mags\n", brightCounts, brightMag);
    if (brightCounts < faintCounts) {
        psLogMsg("ppSim", PS_LOG_INFO,
                 "Image noise is above brightest random star --- no random stars added.");
        return true;
    }

    // given log_10 (dN / dmag) = alpha mag + beta
    // or dN / dmag = No 10^(alpha mag), then:
    // N(m < Mo) = alpha * No * log(10.0) * 10^(alpha*Mo)

    // XXX this needs to handle the case of a flat distribution for efficiency testing

    // Normalization, set by the specified stellar density at the specified bright magnitude
    float norm = refSum / (starsAlpha * logf(10.0) * powf(10.0, (starsAlpha * refMag)));
    float normScale = norm * starsAlpha * logf(10.0);

    // Total number of stars down to the faint flux end
    long nTotal = 0;
    if (flatLum) {
	nTotal = flatNum;
    } else {
	nTotal = normScale * powf (10.0, (starsAlpha * faintMag));
    }

    psLogMsg("ppSim", PS_LOG_INFO, "Adding %ld stars between %f and %f mag\n", nTotal, brightMag, faintMag);

    if (nTotal > 1e7) {
      psLogMsg("ppSim", PS_LOG_INFO, "Trying to add %d stars (far more than 10M stars!), giving up\n", (int) nTotal);
      exit (2);
    }

    long oldSize = stars->n;
    psArrayRealloc (stars, stars->n + nTotal);

    for (long i = 0; i < nTotal; i++) {
        ppSimStar *star = ppSimStarAlloc ();

        // make fpa center of distribution
        star->x    = psRandomUniform(rng) * xSize; // x position
        star->y    = psRandomUniform(rng) * ySize; // y position

	float starMag = 0;
	if (flatLum) {
	    starMag = psRandomUniform(rng) * (faintMag - brightMag) + brightMag;
	} else {
	    starMag = PS_MIN (faintMag, PS_MAX (brightMag, (log10((i + 1.0) / normScale) / starsAlpha)));
	}
	
        star->flux = ppSimMagToFlux (starMag, zp) * expTime;
        star->peak = ppSimStarFluxToPeak (star->flux, seeing);

        stars->data[oldSize + i] = star;
    }
    stars->n = oldSize + nTotal;

    return true;
}

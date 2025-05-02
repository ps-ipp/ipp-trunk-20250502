# include "ppSim.h"

# define ESCAPE(MSG) { \
  psError(PS_ERR_BAD_PARAMETER_VALUE, true, MSG); \
  return false; }

bool ppSimMakeStarCluster(psArray *stars, pmFPA *fpa, pmConfig *config, const psRandom *rng) {

    bool status;
    assert (stars);

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

    bool starsCluster = psMetadataLookupBool(&status, recipe, "STARS.CLUSTER"); // add fake stars in a cluster?
    if (!starsCluster) return true;

    // cluster stars are added with magnitudes distributed based on the given luminosity function:
    float starsAlpha   = psMetadataLookupF32(&status, recipe, "STARS.LUM"); // Star luminosity func slope
    float brightMag    = psMetadataLookupF32(&status, recipe, "STARS.MAG"); // Star brightest magnitude
    float nSigmaLim    = psMetadataLookupF32(&status, recipe, "STARS.SIGMA.LIM"); // significance of faintest stars

    float starsClusterDensity = psMetadataLookupF32(&status, recipe, "STARS.CLUSTER.DENSITY"); // Peak density of cluster (units?)
    float starsClusterSigma   = psMetadataLookupF32(&status, recipe, "STARS.CLUSTER.SIGMA");   // Sigma of cluster distribution
    float starsClusterXcenter = psMetadataLookupF32(&status, recipe, "STARS.CLUSTER.XCENTER"); // Xo of cluster distribution
    float starsClusterYcenter = psMetadataLookupF32(&status, recipe, "STARS.CLUSTER.YCENTER"); // Yo of cluster distribution

    float expTime      = psMetadataLookupF32(&status, recipe, "EXPTIME"); // Exposure time
    float zp           = psMetadataLookupF32(&status, recipe, "ZEROPOINT"); // Photometric zero point
    float seeing       = psMetadataLookupF32(&status, recipe, "SEEING"); // Seeing SIGMA (pixels)
    float scale        = psMetadataLookupF32(&status, recipe, "PIXEL.SCALE"); // Plate scale (arcsec/pixel)

    if (isnan(expTime))  ESCAPE("EXPTIME is not defined");
    if (isnan(zp))       ESCAPE("ZEROPOINT is not defined");
    if (isnan(seeing))   ESCAPE("SEEING is not defined");
    if (isnan(scale))    ESCAPE("PIXEL.SCALE is not defined");

    float skyRate = psMetadataLookupF32(&status, recipe, "SKY.RATE"); // Sky rate (counts / sec)
    if (isnan(skyRate)) {
	float skyMags = psMetadataLookupF32(&status, recipe, "SKY.MAGS");  assert (status);
	skyRate = scale * scale * ppSimMagToFlux (skyMags, zp);
    }

    // Size of FPA
    psRegion *bounds = ppSimFPABounds (fpa);
    // Size of focal plane (can't I get this from the config?)
    int xSize = bounds->x1 - bounds->x0;
    int ySize = bounds->y1 - bounds->y0;
    psFree(bounds);

    // choose reference magnitude & density to set normalization
    float refMag = 0;
    float refSum = 0;
    refMag = brightMag;

    // Integral of a 2D Gauss with sigma and Io: 2.0 * M_PI * PS_SQR(sigma) * Io

    // central density is in stars per square degree but
    // starsClusterSigma is in pixels (or arcsec?)
    refSum = starsClusterDensity * 2.0 * M_PI * PS_SQR(starsClusterSigma * scale / 3600.0);
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
    // float skySigma = sqrtf(PS_SQR(readnoise) + (darkRate + skyRate) * expTime);
    // XXX add darkRate back in?
    float skySigma = sqrtf(PS_SQR(readnoise) + (skyRate) * expTime);
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
    // if (flatLum) {
    // 	nTotal = flatNum;
    // } else {
    // 	nTotal = normScale * powf (10.0, (starsAlpha * faintMag));
    // }

    nTotal = normScale * powf (10.0, (starsAlpha * faintMag));


    psLogMsg("ppSim", PS_LOG_INFO, "Adding %ld stars between %f and %f mag\n", nTotal, brightMag, faintMag);

    long oldSize = stars->n;
    psArrayRealloc (stars, stars->n + nTotal);

    for (long i = 0; i < nTotal; i++) {
        ppSimStar *star = ppSimStarAlloc ();

	// make 10 tries to select a star in the image range
	for (int j = 0; j < 10; j++) {
	  float Xo = ppSimRandomGaussian (rng, starsClusterXcenter, starsClusterSigma);
	  if (Xo < 0) continue;
	  if (Xo > xSize) continue;
	  star->x = Xo;
	  break;
	}
	for (int j = 0; j < 10; j++) {
	  float Yo = ppSimRandomGaussian (rng, starsClusterYcenter, starsClusterSigma);
	  if (Yo < 0) continue;
	  if (Yo > ySize) continue;
	  star->y = Yo;
	  break;
	}

	float starMag = 0;
	// if (flatLum) {
	//     starMag = psRandomUniform(rng) * (faintMag - brightMag) + brightMag;
	// } else {
	//     starMag = PS_MIN (faintMag, PS_MAX (brightMag, (log10((i + 1.0) / normScale) / starsAlpha)));
	// }
	
	starMag = PS_MIN (faintMag, PS_MAX (brightMag, (log10((i + 1.0) / normScale) / starsAlpha)));

        star->flux = ppSimMagToFlux (starMag, zp) * expTime;
        star->peak = ppSimStarFluxToPeak (star->flux, seeing);

        stars->data[oldSize + i] = star;
    }
    stars->n = oldSize + nTotal;

    return true;
}

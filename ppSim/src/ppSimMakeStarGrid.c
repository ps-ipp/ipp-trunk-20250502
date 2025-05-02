# include "ppSim.h"

# define ESCAPE(MSG) { \
  psError(PS_ERR_BAD_PARAMETER_VALUE, true, MSG); \
  return false; }

bool ppSimMakeStarGrid(psArray *stars, pmFPA *fpa, pmConfig *config, const psRandom *rng) {

    bool status;
    assert (stars);

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

    bool starsGrid = psMetadataLookupBool(&status, recipe, "STARS.GRID"); // Grid of stars
    if (!starsGrid) return true;

    float starMag = psMetadataLookupF32(&status, recipe, "STARS.GRID.MAG"); // magnitude of grid stars
    float dX  	  = psMetadataLookupS32(&status, recipe, "STARS.GRID.DX"); // grid spacing in X
    float dY  	  = psMetadataLookupS32(&status, recipe, "STARS.GRID.DY"); // grid spacing in Y

    float expTime      = psMetadataLookupF32(&status, recipe, "EXPTIME"); // Exposure time
    float zp           = psMetadataLookupF32(&status, recipe, "ZEROPOINT"); // Photometric zero point
    float seeing       = psMetadataLookupF32(&status, recipe, "SEEING"); // Seeing SIGMA (pixels)
    float scale        = psMetadataLookupF32(&status, recipe, "PIXEL.SCALE"); // Plate scale (arcsec/pixel)

    if (isnan(expTime))  ESCAPE("EXPTIME is not defined");
    if (isnan(zp))       ESCAPE("ZEROPOINT is not defined");
    if (isnan(seeing))   ESCAPE("SEEING is not defined");
    if (isnan(scale))    ESCAPE("PIXEL.SCALE is not defined");

    // Size of FPA
    psRegion *bounds = ppSimFPABounds (fpa);

    // Size of focal plane (can't I get this from the config?)
    int xSize = bounds->x1 - bounds->x0;
    int ySize = bounds->y1 - bounds->y0;
    psFree(bounds);

    int nX = xSize / dX;
    int oX = 0.5*dX;

    int nY = ySize / dY;
    int oY = 0.5*dY;

    psLogMsg("ppSim", PS_LOG_INFO, "Adding %d stars with %f mag\n", nX * nY, starMag);

    long oldSize = stars->n;
    psArrayRealloc (stars, stars->n + (nX * nY));

    int i = 0;
    for (int ix = 0; ix < nX; ix ++) {
	for (int iy = 0; iy < nY; iy ++) {

	    ppSimStar *star = ppSimStarAlloc ();

	    // make fpa center of distribution
	    star->x    = oX + dX*ix; // x position
	    star->y    = oY + dY*iy; // y position

	    star->flux = ppSimMagToFlux (starMag, zp) * expTime;
	    star->peak = ppSimStarFluxToPeak (star->flux, seeing);

	    stars->data[oldSize + i] = star;
	    i++;
	}
    }
    stars->n = oldSize + i;

    return true;
}

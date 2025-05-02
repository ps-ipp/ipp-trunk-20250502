# include "ppSim.h"

bool ppSimLoadStars (psArray *stars, pmFPA *fpa, pmConfig *config) {

    bool mdok;
    assert (stars);

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    bool starsReal = psMetadataLookupBool(&mdok, recipe, "STARS.REAL"); // Density of fakes
    if (!starsReal) return true;

    // Read catalogue stars using psastro
    psMetadata *astroRecipe = psMetadataLookupPtr(NULL, config->recipes, PSASTRO_RECIPE);
    if (!astroRecipe) {
        psError(PSASTRO_ERR_CONFIG, false, "Can't find recipe %s", PSASTRO_RECIPE);
        return NULL;
    }

    // relevant metadata
    float zp      = psMetadataLookupF32(NULL, recipe, "ZEROPOINT"); // Photometric zero point
    float ra0     = psMetadataLookupF32(NULL, recipe, "RA");        // Boresight RA (radians)
    float dec0    = psMetadataLookupF32(NULL, recipe, "DEC");       // Boresight Dec (radians)
    float pa      = psMetadataLookupF32(NULL, recipe, "PA");        // Position angle (radians)
    float seeing  = psMetadataLookupF32(NULL, recipe, "SEEING");    // Seeing SIGMA (pixels)
    float scale   = psMetadataLookupF32(NULL, recipe, "PIXEL.SCALE"); // Plate scale (arcsec/pixel)
    float scaleRad = scale * M_PI / 3600.0 / 180.0; // Plate scale in radians/pixel for WCS below
    float expTime = psMetadataLookupF32(NULL, recipe, "EXPTIME");   // Exposure time (sec)

    // Size of FPA
    psRegion *bounds = ppSimFPABounds (fpa);
    float radius = 0.5 * PS_MAX(bounds->x1 - bounds->x0, bounds->y1 - bounds->y0) * scaleRad;

    float x0fpa = 0.5*(bounds->x0 + bounds->x1);
    float y0fpa = 0.5*(bounds->y0 + bounds->y1);

    psFree(bounds);

    psMetadataAdd(astroRecipe, PS_LIST_TAIL, "RA_MIN",  PS_DATA_F32 | PS_META_REPLACE, "",
                  ra0 - radius / cos(dec0));
    psMetadataAdd(astroRecipe, PS_LIST_TAIL, "RA_MAX",  PS_DATA_F32 | PS_META_REPLACE, "",
                  ra0 + radius / cos(dec0));
    psMetadataAdd(astroRecipe, PS_LIST_TAIL, "DEC_MIN", PS_DATA_F32 | PS_META_REPLACE, "", dec0 - radius);
    psMetadataAdd(astroRecipe, PS_LIST_TAIL, "DEC_MAX", PS_DATA_F32 | PS_META_REPLACE, "", dec0 + radius);
    psArray *refStars = psastroLoadRefstars(config, "PPSIM.OUTPUT");
    if (!refStars) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find reference stars.");
        psFree(refStars);
        return NULL;
    }
    psLogMsg("ppSim", PS_LOG_INFO, "Adding %ld reference stars", refStars->n);

    long oldSize = stars->n;
    stars = psArrayRealloc (stars, refStars->n);

    psProjection *proj = psProjectionAlloc(ra0, dec0, scaleRad, scaleRad, PS_PROJ_TAN); // Projection

    // Conversion loop
    for (long i = 0; i < refStars->n; i++) {
        ppSimStar *star = ppSimStarAlloc ();

        pmAstromObj *ref = refStars->data[i]; // Reference star
        star->ra  = ref->sky->r; // RA of star
        star->dec = ref->sky->d; // Dec of star
        star->mag = ref->Mag;       // Magnitude of star

        // Convert to x,y position on tangent plane, in pixels
        psPlane plane;                  // Plane (detector) coordinates
        psSphere sphere;                // Sphere (sky) coordinates
        sphere.r = star->ra;
        sphere.d = star->dec;
        psProject(&plane, &sphere, proj);

        // Apply rotation, make FPA center of boresite
        star->x = cos(pa) * plane.x - sin(pa) * plane.y + x0fpa;
        star->y = sin(pa) * plane.x + cos(pa) * plane.y + y0fpa;

        // Convert magnitude to peak flux
        star->flux = ppSimMagToFlux (star->mag, zp) * expTime;
        star->peak = ppSimStarFluxToPeak (star->flux, seeing);
	star->external = TRUE; // stars loaded from catalog are external

        stars->data[oldSize + i] = star;

        psTrace("ppSim", 10, "Adding catalogue star: %.1f,%.1f --> %.2f\n", star->x, star->y, star->flux);
    }
    stars->n = oldSize + refStars->n;
    psFree(proj);

    pmLumFunc *lumfunc = psastroLuminosityFunction (refStars, NULL);
    psFree(refStars);

    psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "FPA.RA", PS_META_REPLACE, "Right ascension", ra0);
    psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "FPA.DEC", PS_META_REPLACE, "Declination", dec0);

    if (lumfunc) {
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.MAG.MIN",   PS_META_REPLACE, "min valid magnitude",             lumfunc->mMin);
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.MAG.MAX",   PS_META_REPLACE, "max valid magnitude",             lumfunc->mMax);
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.LF.SLOPE",  PS_META_REPLACE, "log-mag histogram slope",         lumfunc->slope);
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.LF.OFFSET", PS_META_REPLACE, "log-mag histogram offset",        lumfunc->offset);
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.MAG.PEAK",  PS_META_REPLACE, "magnitude of peak bin",           lumfunc->mPeak);
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.NUM.PEAK",  PS_META_REPLACE, "number of stars in peak bin", lumfunc->nPeak);
        psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "STARS.REAL.SUM.PEAK",  PS_META_REPLACE, "sum of stars up to peak bin", lumfunc->sPeak);
        psFree (lumfunc);
    }

    return stars;
}

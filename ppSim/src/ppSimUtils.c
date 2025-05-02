# include "ppSim.h"

// Generate a header containing WCS keywords
// this function is called with only one of fpa, chip, cell not NULL
bool ppSimInitHeader(pmConfig *config,
                     pmFPA *fpa,
                     pmChip *chip,
                     pmCell *cell)
{
    bool mdok;

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    float ra0   = psMetadataLookupF32(NULL, recipe, "RA");  // Boresight RA (radians)
    float dec0  = psMetadataLookupF32(NULL, recipe, "DEC"); // Boresight Dec (radians)
    float pa    = psMetadataLookupF32(NULL, recipe, "PA");  // Position angle (radians)
    float scale = psMetadataLookupF32(NULL, recipe, "PIXEL.SCALE"); // plate scale in arcsec / pixel
    float scaleRad = scale * M_PI / 3600.0 / 180.0; // convert plate scale to radians/pixel

    int binning = psMetadataLookupS32(NULL, recipe, "BINNING"); // Binning in x and y

    float x0 = 0.0, y0 = 0.0;
    int xParity = 0, yParity = 0;
    if (cell) {
        int x0Chip = psMetadataLookupS32(NULL, cell->parent->concepts, "CHIP.X0");
        int y0Chip = psMetadataLookupS32(NULL, cell->parent->concepts, "CHIP.Y0");
        int xParityChip = psMetadataLookupS32(NULL, cell->parent->concepts, "CHIP.XPARITY");
        int yParityChip = psMetadataLookupS32(NULL, cell->parent->concepts, "CHIP.YPARITY");

        int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
        int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
        int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
        int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

        x0 = PPSIM_FPA_TO_CELL(0.0, x0Cell, xParityCell, binning, x0Chip, xParityChip);
        y0 = PPSIM_FPA_TO_CELL(0.0, y0Cell, yParityCell, binning, y0Chip, yParityChip);
        xParity = xParityCell * xParityChip;
        yParity = yParityCell * yParityChip;
    }
    if (chip) {
        int x0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0");
        int y0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0");
        int xParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY");
        int yParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY");

        x0 = PPSIM_FPA_TO_CELL(0.0, 0, 1, binning, x0Chip, xParityChip);
        y0 = PPSIM_FPA_TO_CELL(0.0, 0, 1, binning, y0Chip, yParityChip);
        xParity = xParityChip;
        yParity = yParityChip;
    }
    if (fpa) {
        psRegion *bounds = ppSimFPABounds (fpa);
        x0 = 0.5 * (bounds->x1 - bounds->x0);
        y0 = 0.5 * (bounds->y1 - bounds->y0);
        xParity = 1;
        yParity = 1;
        psFree (bounds);
    }
    assert(xParity != 0 && yParity != 0);

    psMetadata *header = psMetadataAlloc(); // Header, to return
    pmAstromWCS *wcs = pmAstromWCSAlloc(1, 1); // WCS structure
    wcs->toSky = psProjectionAlloc(ra0, dec0, scaleRad * xParity, scaleRad * yParity, PS_PROJ_TAN);
    wcs->cdelt1 = scaleRad * PM_DEG_RAD * xParity;
    wcs->cdelt2 = scaleRad * PM_DEG_RAD * yParity;
    wcs->crpix1 = x0;
    wcs->crpix2 = y0;
    wcs->trans->x->coeff[1][0] = cos(pa) * wcs->cdelt1;
    wcs->trans->x->coeff[0][1] = -sin(pa) * wcs->cdelt1;
    wcs->trans->y->coeff[1][0] = sin(pa) * wcs->cdelt2;
    wcs->trans->y->coeff[0][1] = cos(pa) * wcs->cdelt2;

    // These aren't used by pmAstromWCStoHeader, but set them anyway
    wcs->trans->x->coeff[0][0] = ra0;
    wcs->trans->y->coeff[0][0] = dec0;
    wcs->trans->x->coeff[1][1] = 0.0;
    wcs->trans->y->coeff[1][1] = 0.0;

    pmAstromWCStoHeader(header, wcs);
    psFree(wcs);

    if (cell) {
        cell->hdu->header = header;
        cell->data_exists = true;
    }
    if (chip) {
        chip->hdu->header = header;
        chip->data_exists = true;
    }
    if (fpa) {
        fpa->hdu->header = header;
    }

    return true;
}

char *ppSimTypeToString (ppSimType type) {

    char *typeStr;

    switch (type) {
      case PPSIM_TYPE_BIAS:   typeStr = psStringCopy ("BIAS");   break;
      case PPSIM_TYPE_DARK:   typeStr = psStringCopy ("DARK");   break;
      case PPSIM_TYPE_FLAT:   typeStr = psStringCopy ("FLAT");   break;
      case PPSIM_TYPE_OBJECT: typeStr = psStringCopy ("OBJECT"); break;
      default:
        psAbort("Should never get here.");
    }
    return (typeStr);
}

ppSimType ppSimTypeFromString (char *typeStr) {

    if (!strcasecmp (typeStr, "BIAS"))   return PPSIM_TYPE_BIAS;
    if (!strcasecmp (typeStr, "DARK"))   return PPSIM_TYPE_DARK;
    if (!strcasecmp (typeStr, "FLAT"))   return PPSIM_TYPE_FLAT;
    if (!strcasecmp (typeStr, "OBJECT")) return PPSIM_TYPE_OBJECT;
    psAbort("Should never get here.");
    // XXX raise error instead of aborting
}

bool ppSimUpdateConceptsFPA (pmFPA *fpa, pmConfig *config) {

    bool mdok;

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    float expTime = psMetadataLookupF32(NULL, recipe, "EXPTIME"); // Exposure time
    float detTemp = psMetadataLookupF32(NULL, recipe, "DET.TEMP"); // Detector Temperature

    const char *filter = psMetadataLookupStr(NULL, recipe, "FILTER"); // Filter name
    if (!filter) {
        filter = "NONE";
    }

    char *typeStr = psMetadataLookupStr(NULL, recipe, "IMAGE.TYPE"); // Type of image to simulate
    char *obs_mode = psMetadataLookupStr(NULL, recipe, "OBS_MODE");

    psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.OBSTYPE",  PS_META_REPLACE, "Observation type", typeStr);
    psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.OBJECT",   PS_META_REPLACE, "Observation name", typeStr);
    psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.OBS.MODE", PS_META_REPLACE, "Observation mode", obs_mode);
    psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.EXPOSURE", PS_META_REPLACE, "Exposure time (sec)", expTime);
    psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.TEMP",     PS_META_REPLACE, "Detector Temperature (C)", detTemp);
    psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.FILTERID", PS_META_REPLACE, "Filter name", filter);
    psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.FILTER",   PS_META_REPLACE, "Filter name", filter);
    psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.AIRMASS",  PS_META_REPLACE, "airmass (dummy value)", 1.0);

    // test time 
    psTime *obstime = psTimeFromMJD (55440.0);
    psMetadataAddTime(fpa->concepts, PS_LIST_TAIL, "FPA.TIME", PS_META_REPLACE, "sample time", obstime);

    return true;
}

bool ppSimUpdateConceptsCell (pmCell *cell, pmConfig *config) {

    bool mdok;

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    int binning = psMetadataLookupS32(NULL, recipe, "BINNING"); // Binning in x and y
    float expTime = psMetadataLookupF32(NULL, recipe, "EXPTIME"); // Exposure time

    psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.EXPOSURE", PS_META_REPLACE, "Exposure time (sec)", expTime);
    psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.DARKTIME", PS_META_REPLACE, "Dark time (sec)", expTime);
    psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.XBIN", PS_META_REPLACE, "Binning in x", binning);
    psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.YBIN", PS_META_REPLACE, "Binning in y", binning);

    // test time 
    psTime *obstime = psTimeFromMJD (55440.0);
    psMetadataAddTime(cell->concepts, PS_LIST_TAIL, "CELL.TIME", PS_META_REPLACE, "sample time", obstime);

    return true;
}

bool ppSimRecipeValidation (pmConfig *config) {

    bool status;

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

    int binning = psMetadataLookupS32(&status, recipe, "BINNING"); // Binning in x and y
    if (binning <= 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Binning (%d) is non-positive.", binning);
        exit(PS_EXIT_CONFIG_ERROR);
    }
    return true;
}

// Get a value from the command-line arguments and add it to recipe options
float ppSimArgToRecipeF32(bool *status,
                          psMetadata *options,    // Target to which to add value
                          const char *recipeName, // Name for value in the recipe
                          psMetadata *arguments,  // Command-line arguments
                          const char *argName       // Argument name in the command-line arguments
    )
{
    bool myStatus;
    float value = psMetadataLookupF32(&myStatus, arguments, argName); // Value of interest
    if (status) { *status = myStatus; }
    if (isnan(value)) return value;

    psMetadataAddF32(options, PS_LIST_TAIL, recipeName, PS_META_REPLACE, NULL, value);
    return value;
}

// Get a value from the command-line arguments and add it to recipe options
int ppSimArgToRecipeS32(bool *status,
                        psMetadata *options,    // Target to which to add value
                        const char *recipeName, // Name for value in the recipe
                        psMetadata *arguments,  // Command-line arguments
                        const char *argName         // Argument name in the command-line arguments
    )
{
    bool myStatus;
    int value = psMetadataLookupS32(&myStatus, arguments, argName); // Value of interest
    if (status) { *status = myStatus; }

    psMetadataAddS32(options, PS_LIST_TAIL, recipeName, PS_META_REPLACE, NULL, value);
    return value;
}

// Get a value from the command-line arguments and add it to recipe options
bool ppSimArgToRecipeBool(bool *status,
                          psMetadata *options,    // Target to which to add value
                          const char *recipeName, // Name for value in the recipe
                          psMetadata *arguments,  // Command-line arguments
                          const char *argName       // Argument name in the command-line arguments
    )
{
    bool myStatus;
    bool value = psMetadataLookupS32(&myStatus, arguments, argName); // Value of interest
    if (status) { *status = myStatus; }

    psMetadataAddBool(options, PS_LIST_TAIL, recipeName, PS_META_REPLACE, NULL, value);
    return value;
}

// Get a value from the command-line arguments and add it to recipe options
// if it is not specified, do not override the existing recipe value
char *ppSimArgToRecipeStr(bool *status,
                          psMetadata *options,    // Target to which to add value
                          const char *recipeName, // Name for value in the recipe
                          psMetadata *arguments,  // Command-line arguments
                          const char *argName       // Argument name in the command-line arguments
    )
{
    bool myStatus;

    char *value = psMetadataLookupStr(&myStatus, arguments, argName); // Value of interest
    if (status) {
        *status = myStatus;
    }
    if (!value) return NULL;
    psMetadataAddStr(options, PS_LIST_TAIL, recipeName, PS_META_REPLACE, NULL, value);
    return value;
}

float ppSimGetZeroPoint(psMetadata *recipe, const char *filter)
{
    PS_ASSERT_METADATA_NON_NULL(recipe, NAN);
    PS_ASSERT_STRING_NON_EMPTY(filter, NAN);

    // use the filter to get the zeropoint from the recipe
    psMetadata *zeropoints = psMetadataLookupMetadata(NULL, recipe, "ZEROPTS");
    if (!zeropoints) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find ZEROPTS in recipe");
        return NAN;
    }

    return psMetadataLookupF32(NULL, zeropoints, filter);
}

float ppSimGetSkyRate (psMetadata *recipe) {

  bool status; 

  float skyRate      = psMetadataLookupF32(&status, recipe, "SKY.RATE"); // Sky rate
  if (!isnan(skyRate)) {
    float zp       = psMetadataLookupF32(&status, recipe, "ZEROPOINT");   assert (status);
    float scale    = psMetadataLookupF32(&status, recipe, "PIXEL.SCALE"); assert (status);
    float skyMags  = psMetadataLookupF32(&status, recipe, "SKY.MAGS");    assert (status);
    skyRate = scale * scale * ppSimMagToFlux (skyMags, zp);
  }
  return skyRate;
}

psArray *ppSimSelectSources (pmConfig *config, const pmFPAview *view, const char *filename) {

    bool status;

    pmReadout *readout = pmFPAfileThisReadout (config->files, view, filename);
    PS_ASSERT_PTR_NON_NULL (readout, NULL);

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    return sources;
}

bool ppSimDefinePixels (psArray *sources, pmReadout *readout, psMetadata *recipe) {

    bool status;

    float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    if (!status) return NULL;

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        // allocate image, weight, mask arrays for each peak (square of radius OUTER)
        pmSourceDefinePixels (source, readout, source->peak->x, source->peak->y, OUTER);
    }
    return true;
}


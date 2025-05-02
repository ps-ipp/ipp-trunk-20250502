# include "ppSim.h"
static char *defaultModel = "PS_MODEL_SERSIC";

// this is used to measure the inserted flux as a difference of 2 large numbers : must use
// 'double' or we get floating point errors and trends on a scale of ~0.005 mags at > -10 mags
// instrumental
double imageSum (psImage *image) {
    double sum = 0.0;
    for (int iy = 0; iy < image->numRows; iy++) {
	for (int ix = 0; ix < image->numCols; ix++) {
	    sum += image->data.F32[iy][ix];
	}
    }
    return sum;
}

bool ppSimInsertGalaxies (pmReadout *readout, psImage *expCorr, psArray *galaxies, pmConfig *config) {

    bool mdok;

    assert (readout);
    assert (galaxies);
    
    if (!galaxies->n) { return true; }

    pmCell *cell = readout->parent;
    pmChip *chip = cell->parent;

    // XXX this is an estimate of the sky noise based on the inputs to the image simulation.
    // XXX update this to allow the estimate based on the measured sky background
    // XXX this is missing the gain.
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    float expTime   = psMetadataLookupF32(NULL, recipe, "EXPTIME"); // Exposure time
    float darkRate  = psMetadataLookupF32(NULL, recipe, "DARK.RATE"); // Dark rate

    float readnoise = psMetadataLookupF32(NULL, cell->concepts, "CELL.READNOISE");// CCD read noise, e
    if (isnan(readnoise)) {
	psWarning("CELL.READNOISE is not set; reverting to recipe value READNOISE.");
	readnoise = psMetadataLookupF32(&mdok, recipe, "READNOISE");
	if (!mdok) {
	    psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find READNOISE in recipe.");
	    return false;
	}
    }

    float skyRate = psMetadataLookupF32(NULL, recipe, "SKY.RATE"); // Sky rate
    if (isnan(skyRate)) {
	float zp      = psMetadataLookupF32(&mdok, recipe, "ZEROPOINT"); assert (mdok);
	float scale   = psMetadataLookupF32(&mdok, recipe, "PIXEL.SCALE"); assert (mdok);
	float skyMags = psMetadataLookupF32(&mdok, recipe, "SKY.MAGS");  assert (mdok);
	skyRate = scale * scale * ppSimMagToFlux (skyMags, zp);
    }
    
    // Rough noise estimate, appropriate for entire cell (use for source radius?)
    float roughNoise = sqrtf(PS_SQR(readnoise) + (darkRate + skyRate) * expTime);

    int x0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0");
    int y0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0");
    int xParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY");
    int yParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY");

    int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
    int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
    int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    int binning = psMetadataLookupS32(NULL, recipe, "BINNING"); // Binning in x and y

    // determine the galaxy model
    char *modelName = psMetadataLookupStr(&mdok, recipe, "GALAXY.MODEL"); // galaxy model name
    if (modelName == NULL) {
	modelName = defaultModel;
    }
    pmModelType type = pmModelClassGetType (modelName);
    if (type == -1) {
	psError (PS_ERR_UNKNOWN, false, "invalid model name");
	return false;
    }

    int nParam = pmModelClassParameterCount (type);

    pmPSF *psf = psMetadataLookupPtr (&mdok, chip->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing PSPHOT.PSF on chip.analysis");

    int dX = PM_CELL_TO_CHIP (0.0, x0Cell, xParityCell, binning);
    int dY = PM_CELL_TO_CHIP (0.0, y0Cell, yParityCell, binning);

    pmDetections *detections = psMetadataLookupPtr (&mdok, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
	detections = pmDetectionsAlloc();
	detections->allSources = psArrayAllocEmpty (galaxies->n);
	psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_ARRAY | PS_META_REPLACE, "psphot detections", detections);
    } else {
	psMemIncrRefCounter (detections);
    }
    psArray *sources = sources = detections->allSources;

    // output filename
    char outname[1024];
    char *outroot = psMetadataLookupStr(&mdok, config->arguments, "OUTPUT");
    sprintf (outname, "%s.dat", outroot);

    FILE *outfile = fopen (outname, "a");

    // add sources to the readout image & weight
    for (long i = 0; i < galaxies->n; i++) {
	ppSimGalaxy *galaxy = galaxies->data[i];

	// galaxy->x,y are in fpa coordinates

	// Position on the cell and peak flux
	float xChip = PM_FPA_TO_CHIP(galaxy->x, x0Chip, xParityChip);
	float yChip = PM_FPA_TO_CHIP(galaxy->y, y0Chip, yParityChip);

	// Position on the cell and peak flux
	float xCell = PM_CHIP_TO_CELL(xChip, x0Cell, xParityCell, binning);
	float yCell = PM_CHIP_TO_CELL(yChip, y0Cell, yParityCell, binning);

	if (xCell < 0) continue;
	if (yCell < 0) continue;
	if (xCell > readout->image->numCols) continue;
	if (yCell > readout->image->numRows) continue;
	// XXX need to apply col0, row0 if readout is a subarray

	// XXX apply the expCorr to the galaxy->flux before setting the model flux

	// instantiate a model for the PSF at this location, set desired flux
	pmModel *model = pmModelAlloc (type);
	
	psF32 *PAR = model->params->data.F32;

	PAR[PM_PAR_I0]   = galaxy->peak;
	PAR[PM_PAR_SKY]  = 0.0;
	PAR[PM_PAR_XPOS] = xChip;
	PAR[PM_PAR_YPOS] = yChip;

	if (type == pmModelClassGetType ("PS_MODEL_TRAIL")) {
	    PAR[PM_PAR_LENGTH] = galaxy->Rmaj;
	    PAR[PM_PAR_SIGMA]  = galaxy->Rmin;
	    PAR[PM_PAR_THETA]  = galaxy->theta;
	} else {
	    psEllipseAxes axes;
	    axes.major       = galaxy->Rmaj;
	    axes.minor       = galaxy->Rmin;
	    axes.theta       = galaxy->theta;
	    pmPSF_AxesToModel (PAR, axes, model->class->useReff);
	}
	psF64 Area = 2.0 * M_PI * galaxy->Rmaj * galaxy->Rmin;

	if (nParam == 8) {
	    PAR[PM_PAR_7] = galaxy->index;
	}

	// XXX let the flux limit be a user-defined number of sky sigmas (not just 1.0)
	float radius = model->class->modelRadius (model->params, 0.1*roughNoise);
	radius = PS_MAX (radius, 1.0);

	// XXX the exp(-r^0.25) models can go way out if allowed...
	radius = PS_MIN (radius, 300.0); 

	// construct a source, with model flux pixels set, based on the model
	pmSource *source = pmSourceFromModel (model, readout, radius, PM_SOURCE_TYPE_EXTENDED);

	galaxy->flux = model->class->modelFlux (model->params);

	// XXX set the mag & err values (should this be done in pmSourceFromModel?)
	// XXX i should be applying the gain and the correct effective area
	source->psfMag = -2.5*log10(galaxy->flux);
	source->extMag = -2.5*log10(galaxy->flux);
	source->psfMagErr = sqrt(Area*PS_SQR(roughNoise) + galaxy->flux) / galaxy->flux;	
	
	// insert the source flux in the image
	double sum1 = imageSum(source->pixels);
	pmSourceAddWithOffset (source, PM_MODEL_OP_FULL, 0xff, dX, dY);
	double sum2 = imageSum(source->pixels);
	float flux = sum2 - sum1;

	// fprintf (stderr, "flux: %f, sum1: %f, sum2: %f, flux2: %f, dflux: %f\n", galaxy->flux, sum1, sum2, sum2 - sum1, galaxy->flux - sum2 + sum1);

	float par8 = (model->params->n == 8) ? model->params->data.F32[7] : 0.0;
	fprintf (outfile, "%8.3f %8.3f %10.2f  %2d  %7.3f %5.3f  %5.3f %5.3f %5.3f %5.3f\n", galaxy->x, galaxy->y, flux, 1, source->psfMag, source->psfMagErr, galaxy->Rmaj, galaxy->Rmin, galaxy->theta, par8);

	psArrayAdd (sources, 100,source);
    }
    fclose (outfile);

    // XXX many leaks in here, i think 
    psFree (detections);

    return true;
}

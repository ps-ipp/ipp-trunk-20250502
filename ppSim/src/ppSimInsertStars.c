# include "ppSim.h"

// Reset a pointer: free and set to NULL
#define RESET(PTR) \
    psFree(PTR); \
    PTR = NULL;

double imageSum (psImage *image);

bool ppSimInsertStars (pmReadout *readout, psImage *expCorr, psArray *stars, pmConfig *config) {

    bool mdok;

    assert (readout);
    assert (stars);

    if (!stars->n) { return true; }

    pmCell *cell = readout->parent;
    pmChip *chip = cell->parent;

    // XXX this is an estimate of the sky noise based on the inputs to the image simulation.
    // XXX update this to allow the estimate based on the measured sky background
    // XXX this is missing the gain.
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    bool psfConvolve = psMetadataLookupBool(NULL, recipe, "PSF.CONVOLVE"); // smooth the image with the PSF?

    float expTime   = psMetadataLookupF32(NULL, recipe, "EXPTIME"); // Exposure time
    float darkRate  = psMetadataLookupF32(NULL, recipe, "DARK.RATE"); // Dark rate
    float nSigmaLim = psMetadataLookupF32(NULL, recipe, "STARS.SIGMA.LIM"); // significance of faintest stars

    float readnoise = psMetadataLookupF32(NULL, cell->concepts, "CELL.READNOISE");// CCD read noise, e
    if (isnan(readnoise)) {
        psWarning("CELL.READNOISE is not set; reverting to recipe value READNOISE.");
        readnoise = psMetadataLookupF32(&mdok, recipe, "READNOISE");
        if (!mdok) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find READNOISE in recipe.");
            return false;
        }
    }

    float skyRate = ppSimGetSkyRate (recipe);

    // Rough noise estimate, appropriate for entire cell (use for source radius?)
    float roughNoise = sqrtf(PS_SQR(readnoise) + (darkRate + skyRate) * expTime);
    float skyFlux = skyRate*expTime;

    int x0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0");
    int y0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0");
    int xParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY");
    int yParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY");

    int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
    int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
    int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    int binning = psMetadataLookupS32(NULL, recipe, "BINNING"); // Binning in x and y

    pmPSF *psf = psMetadataLookupPtr (&mdok, chip->analysis, "PSPHOT.PSF");
    assert (psf);

    int dX = PM_CELL_TO_CHIP (0.0, x0Cell, xParityCell, binning);
    int dY = PM_CELL_TO_CHIP (0.0, y0Cell, yParityCell, binning);

    psArray *sources = psArrayAllocEmpty (stars->n);

    // output filename
    char outname[1024];
    char *outroot = psMetadataLookupStr(&mdok, config->arguments, "OUTPUT");
    sprintf (outname, "%s.dat", outroot);

    FILE *outfile = fopen (outname, "w");
    psAssert (outfile, "cannot write output");

    float radius = -1.0;

    // add sources to the readout image & variance
    psTrace("ppSim", 1, "Inserting %ld stars...\n", stars->n);
    for (long i = 0; i < stars->n; i++) {
        ppSimStar *star = stars->data[i];
        psTrace("ppSim", 10, "Inserting star at %.1f,%.1f --> %.2f\n", star->x, star->y, star->flux);

        // star->x,y are in fpa coordinates

        // Position on the cell and peak flux
        float xChip = PM_FPA_TO_CHIP(star->x, x0Chip, xParityChip);
        float yChip = PM_FPA_TO_CHIP(star->y, y0Chip, yParityChip);

        // Position on the cell and peak flux
        float xCell = PM_CHIP_TO_CELL(xChip, x0Cell, xParityCell, binning);
        float yCell = PM_CHIP_TO_CELL(yChip, y0Cell, yParityCell, binning);

        // XXX Note, the below does not put the edges of stars on the readout if they fall slightly off
        // This will be visible as cut-off stars at amplifier boundaries (e.g., Megacam)
        if (xCell < 0) continue;
        if (yCell < 0) continue;
        if (xCell > readout->image->numCols) continue;
        if (yCell > readout->image->numRows) continue;
        // XXX need to apply col0, row0 if readout is a subarray

        // Apply the expCorr to the star->flux before setting the model flux
        float flux = star->flux * expCorr->data.F32[(int)yCell][(int)xCell];

	// instantiate a model for the PSF at this location, set desired flux
	pmModel *model = pmModelFromPSFforXY (psf, xChip, yChip, 1.0);
	pmModelSetFlux (model, flux);

	// we must set the radius on the first object, and leave it there for everyone else
	// (inserting flux with a variable radius introduces a flux-dependent bias)
	if (radius < 0) {
	  radius = model->class->modelRadius (model->params, nSigmaLim * roughNoise);
	  radius = PS_MAX (radius,  1.0);
	  radius = PS_MIN (radius, 50.0);
	}

	// construct a source, with model flux pixels set, based on the model
	pmSource *source = pmSourceFromModel (model, readout, radius, PM_SOURCE_TYPE_STAR);

	// XXX set the mag & err values (should this be done in pmSourceFromModel?)
	// XXX i should be applying the gain and the correct effective area
	psEllipseAxes axes = pmPSF_ModelToAxes (model->params->data.F32, model->class->useReff);
	float Area = 2.0 * M_PI * axes.major * axes.minor;

	source->psfMag = -2.5*log10(star->flux);
	source->psfMagErr = sqrt(Area*PS_SQR(roughNoise) + flux) / flux;
	
	// set the expected model errors
	model->dparams->data.F32[PM_PAR_I0] = source->psfMagErr * model->params->data.F32[PM_PAR_I0];

	float par8 = (model->params->n == 8) ? model->params->data.F32[7] : 0.0;
	float starFlux = 0.0;

	// if psfConvolve is TRUE, we will (elsewhere) convolve the image we a PSF
	// in this case, simply place delta functions in the image
	if (psfConvolve) {
	    readout->image->data.F32[(int)(yCell)][(int)(xCell)] += flux;
	} else {
	    // insert the source flux in the image and measure inserted flux
	    // this is a difference of 2 large numbers : must use 'double' or we get floating
	    // point errors and trends on a scale of ~0.005 mags at > -10 mags instrumental
	    double sum1 = imageSum(source->pixels);
	    pmSourceAddWithOffset (source, PM_MODEL_OP_FULL, 0xff, dX, dY);
	    double sum2 = imageSum(source->pixels);
	    starFlux = sum2 - sum1;
	    
	    // insert the source flux in the noise image
	    pmSourceAddWithOffset (source, PM_MODEL_OP_FULL | PM_MODEL_OP_NOISE, 0xff, dX, dY);
	    
	    // Blow away the image parts of the source, which makes the memory explode
	    RESET(source->pixels);
	    RESET(source->variance);
	    RESET(source->maskObj);
	    RESET(source->maskView);
	    RESET(source->modelFlux);
	    RESET(source->psfImage);
	    RESET(source->blends);
	}
	fprintf (outfile, "%8.3f %8.3f %10.5f  %2d  %7.3f %5.3f  %5.3f %5.3f %5.3f %5.3f  :  %f %f\n", star->x, star->y, starFlux, 0, source->psfMag, source->psfMagErr, axes.major, axes.minor, axes.theta, par8, model->params->data.F32[PM_PAR_I0], radius);

	// save the approx sky (exact if sky is flat) for external comparisons
	source->sky = skyFlux;

	// mark the externally supplied stars:x
	if (star->external) {
	  source->mode2 |= PM_SOURCE_MODE2_MATCHED;
	}

	// add the sources to the source array
	psArrayAdd (sources, 100,source);
	psFree(source);                 // Drop reference
    }
    fclose (outfile);

    pmDetections *detections = pmDetectionsAlloc();
    detections->allSources = sources;

    // save detections on the readout->analysis
    if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "fake sources", detections)) {
	psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
	return false;
    }
    psFree(detections);

    // XXX many leaks in here, i think
    return true;
}

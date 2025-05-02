# include "psphotInternal.h"

bool psphotLoadSRCTEXT (pmFPA *fpa, pmConfig *config) {

    pmChip *chip;

    // we have a psArray of filenames, but there is no information to tie the filenames to the
    // corresponding chip/cell/readout.  Fortunately: (a) psphot only really operates on chips
    // and (b) we can make some basic assumptions:

    // 1) if there is only a single entry in the array, read the file and attach the results to
    // each of the chip->analysis metadata entries (just an extra ref counter).

    // 2) if there are multiple entries, the number of filenames must match the number of
    // chips.

    psArray *files = psMetadataLookupPtr(NULL, config->arguments, "SRCTEXT");

    // XXX is this allowed?  how do we get an empty entry in arguments?
    if (!files->n) return true;

    // we use a default model type of GAUSS to define the sources
    pmModelType modelType = pmModelClassGetType("PS_MODEL_GAUSS");

    psArray *sourceArrays = psArrayAllocEmpty(files->n);
    for (int i = 0; i < files->n; i++) {

	char *filename = files->data[i];

	// each file contains a list of X Y coordinates and nothing else?
	FILE *f = fopen (filename, "r");
	if (f == NULL) {
            psError(PSPHOT_ERR_CONFIG, true, "Failed to read source text list %s", filename);
	    return false;
	}

	double X, Y;

	psArray *sources = psArrayAllocEmpty (100);
	while (fscanf (f, "%lf %lf", &X, &Y) != EOF) {

	    psEllipseAxes axes;
	    
	    pmSource *source = pmSourceAlloc ();
	    pmModel *model = pmModelAlloc (modelType);
	    source->modelPSF  = model;
	    source->type = PM_SOURCE_TYPE_STAR;

	    // NOTE: A SEGV here because "model" is NULL is probably caused by not initialising the models.
	    psF32 *PAR = model->params->data.F32;
	    psF32 *dPAR = model->dparams->data.F32;

	    // NOTE: most of these values are irrelevant for loaded source positions
	    source->seq       = source->id;
	    PAR[PM_PAR_XPOS]  = X;
	    PAR[PM_PAR_YPOS]  = Y;
	    dPAR[PM_PAR_XPOS] = 0.0;
	    dPAR[PM_PAR_YPOS] = 0.0;
	    axes.major        = 1.0;
	    axes.minor        = 1.0;
	    axes.theta        = 0.0;

	    PAR[PM_PAR_SKY]   = 0.0;
	    dPAR[PM_PAR_SKY]  = 0.0;
	    source->sky       = PAR[PM_PAR_SKY];
	    source->skyErr    = dPAR[PM_PAR_SKY];

	    source->psfMag    = 0.0;
	    source->psfMagErr    = 0.0;
	    source->apMag     = 0.0;

	    PAR[PM_PAR_I0]    = 1.0;
	    dPAR[PM_PAR_I0]   = 0.0;

	    pmPSF_AxesToModel (PAR, axes, model->class->useReff);

	    float peakFlux    = 1.0;

	    source->peak = pmPeakAlloc(PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], peakFlux, PM_PEAK_LONE);
	    source->peak->dx   = dPAR[PM_PAR_XPOS];
	    source->peak->dy   = dPAR[PM_PAR_YPOS];
	    source->peak->xf   = PAR[PM_PAR_XPOS]; // the alloc function takes the pixel index,
	    source->peak->yf   = PAR[PM_PAR_YPOS]; // but we know the pixel coordinate

	    source->pixWeightNotBad = 1.0;
	    source->pixWeightNotPoor = 1.0;
	    source->crNsigma  = 0.0;
	    source->extNsigma = 0.0;
	    source->apRadius  = 0.0;

	    model->chisq      = 0.0;
	    model->nDOF       = 0;
	    model->nPix       = 0;

	    source->moments = pmMomentsAlloc ();
	    source->moments->Mxx = 0.0;
	    source->moments->Mxy = 0.0;
	    source->moments->Myy = 0.0;

            //source->mode = 0;
            // MEH -- has to be PSFSTAR to accommodate changed made in psphot:psphotReadoutCleanupReadout and psphotPSFstats to psphotPSFstatsSources
            source->mode = 64;
	    
	    psArrayAdd (sources, 100, source);
	    psFree(source);
	}
	psArrayAdd (sourceArrays, 100, sources);
	psFree (sources);
    }

    if (files->n == 1) {
	psArray *sources = sourceArrays->data[0];
	pmFPAview *view = pmFPAviewAlloc (0);
	while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
	    if (!chip->process) { continue; }
	    psMetadataAddPtr (chip->analysis, PS_LIST_TAIL, "PSPHOT.SOURCES.TEXT", PS_DATA_ARRAY | PS_META_REPLACE, "loaded source from textfile", sources);
	}
	psFree (view);
    } else {
	int nSrc = 0;
	pmFPAview *view = pmFPAviewAlloc (0);
	while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
	    if (!chip->process) { continue; }

	    if (nSrc >= sourceArrays->n) {
		psError(PSPHOT_ERR_CONFIG, true, "mismatch between number of source lists and number of chips (readouts)");
		return false;
	    }
	    psArray *sources = sourceArrays->data[nSrc];
	    psMetadataAddPtr (chip->analysis, PS_LIST_TAIL, "PSPHOT.SOURCES.TEXT", PS_DATA_ARRAY | PS_META_REPLACE, "loaded source from textfile", sources);
	    nSrc ++;
	}
	psFree (view);
    }
    psFree (sourceArrays);
    return true;
}

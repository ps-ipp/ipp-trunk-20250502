# include "psphotInternal.h"

bool psphotTestPSF (pmReadout *readout, psArray *sources, psMetadata *recipe) {

    bool            status;
    char           *modelName;
    pmPSF          *psf = NULL;
    psArray        *stars = NULL;

    psTimerStart ("psphot");

    psphotSaveImage (NULL, readout->image,  "image.fits");

    // check if a PSF model is supplied by the user
    psf = psMetadataLookupPtr (NULL, readout->analysis, "PSPHOT.PSF");
    if (psf != NULL) return psf;

    // examine PSF sources in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // array to store candidate PSF stars
    int NSTARS = psMetadataLookupS32 (&status, recipe, "PSF_MAX_NSTARS");
    if (!status) {
        NSTARS = PS_MIN (sources->n, 200);
        psWarning("PSF_MAX_NSTARS is not set in the recipe --- defaulting to %d\n", NSTARS);
    }

    // use poissonian errors or local-sky errors
    bool POISSON_ERRORS = psMetadataLookupBool (&status, recipe, "POISSON_ERRORS");
    if (!status) {
        POISSON_ERRORS = true;
        psWarning("POISSON_ERRORS is not set in the recipe --- defaulting to true.\n");
    }
    pmSourceFitModelInit (15, 0.1, 1.0, POISSON_ERRORS);

    // how to model the PSF variations across the field
    // XXX make a default value?  or not?
    psMetadata *md = psMetadataLookupMetadata (&status, recipe, "PSF.TREND.MASK");
    psPolynomial2D *psfTrendMask;
    if (!status || !md) {
        psWarning("PSF.TREND.MASK is not set in the recipe --- defaulting to use zeroth order.\n");
        psfTrendMask = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 0, 0);
    } else {
        psfTrendMask = psPolynomial2DfromMetadata (md);
        if (!psfTrendMask) {
            psError(PSPHOT_ERR_PSF, true, "Unable to construct polynomial from PSF.TREND.MASK in the recipe");
            return false;
        }
    }

    stars = psArrayAllocEmpty (sources->n);

    // select the candidate PSF stars (pointers to original sources)
    for (int i = 0; (i < sources->n) && (stars->n < NSTARS); i++) {
        pmSource *source = sources->data[i];
        // if (source->mode & PM_SOURCE_MODE_PSFSTAR) psArrayAdd (stars, 200, source);
        psArrayAdd (stars, 200, source);
    }
    psLogMsg ("psphot.pspsf", 4, "selected candidate %ld PSF objects\n", stars->n);

    if (stars->n == 0) {
        psError(PSPHOT_ERR_PSF, true, "Failed to find any PSF candidates");
        return NULL;
    }

    // get the fixed PSF fit radius
    // XXX EAM : check that PSF_FIT_RADIUS < SKY_OUTER_RADIUS
    float RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_FIT_RADIUS");
    if (!status) {
        psWarning("PSF_FIT_RADIUS is not set in the recipe --- defaulting to 20.0\n");
        RADIUS = 20.0;
    }

    // for this test, require a single model
    psMetadataItem *mdi = psMetadataLookup (recipe, "PSF_MODEL");
    if (mdi == NULL) psAbort("missing PSF_MODEL selection");
    if (mdi->type != PS_DATA_STRING) psAbort("choose a single PSF_MODEL");
    modelName = mdi->data.V;

    pmPSFtestModel (stars, modelName, RADIUS, POISSON_ERRORS, psfTrendMask);

    psphotSaveImage (NULL, readout->image,  "resid.fits");
    psphotSaveImage (NULL, readout->mask,   "mask.fits");
    psphotSaveImage (NULL, readout->weight, "weight.fits");
    
    return true;
}

bool pmPSFtestModel (psArray *sources, char *modelName, float RADIUS, bool poissonErrors, psPolynomial2D *psfTrendMask)
{
    bool status;
    float x;
    float y;

    pmModelType type = pmModelSetType (modelName);
    pmPSF *psf = pmPSFAlloc (type, poissonErrors, psfTrendMask);
    if (psf == NULL) psAbort("unknown model");

    FILE *f = fopen ("params.dat", "w");

    // stage 1:  fit an independent model (freeModel) to all sources
    psTimerStart ("fit");
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        pmModel  *model  = pmSourceModelGuess (source, psf->type);
        x = source->peak->x;
        y = source->peak->y;

        // set temporary object mask and fit object
        // fit model as EXT, not PSF
        psImageKeepCircle (source->mask, x, y, RADIUS, "OR", markVal);
        status = pmSourceFitModel (source, model, PM_SOURCE_FIT_EXT);
        psImageKeepCircle (source->mask, x, y, RADIUS, "AND", PS_NOT_IMAGE_MASK(markVal));

	// write fitted parameters to file
	fprintf (f, "%f ", model->params->data.F32[PM_PAR_XPOS]);
	fprintf (f, "%f ", model->params->data.F32[PM_PAR_YPOS]);

	fprintf (f, "%f ", model->params->data.F32[PM_PAR_SXX]);
	fprintf (f, "%f ", model->params->data.F32[PM_PAR_SYY]);
	fprintf (f, "%f ", model->params->data.F32[PM_PAR_SXY]);

	fprintf (f, "%f %d\n", model->chisq, model->nIter);

	// subtract model flux
	pmModelSub (source->pixels, source->mask, model, PM_MODEL_OP_FULL);
    }
    fclose (f);
    psLogMsg ("psphot.psftest", 4, "fit ext: %f sec for %ld sources\n", psTimerMark ("fit"), sources->n);
    return true;
}

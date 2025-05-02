# include "psphotInternal.h"
bool psphotPSFstatsSources (pmReadout *readout, psArray *sources, pmPSF *psf);

// generate a PSF model for inputs without PSF models already loaded
bool psphotChoosePSF (pmConfig *config, const pmFPAview *view, const char *filerule, bool newSources)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Choose PSF ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
        if (!psphotChoosePSFReadout (config, view, filerule, i, recipe, newSources)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to choose a psf model for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// try PSF models and select best option
bool psphotChoosePSFReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool newSources) {

    bool status;

    psTimerStart ("psphot.choose.psf");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // do not generate a PSF if we already were supplied one
    if (psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF")) {
        psLogMsg ("psphot", PS_LOG_DETAIL, "psf model supplied for input file %d", index);
        return true;
    }

    if (psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
        psLogMsg ("psphot", PS_LOG_DETAIL, "skipping choose PSF for input file %d", index);
        return true;
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = newSources ? detections->newSources : detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping PSF model");
        return false;
    }

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    psAssert (maskVal, "missing mask value?");

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    psAssert (markVal, "missing mark value?");

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // examine PSF sources in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // structure to store user options defining the psf
    pmPSFOptions *options = pmPSFOptionsAlloc();

    // load user options from the recipe. no need to check existence -- they are
    // array to store candidate PSF stars
    int NSTARS = psMetadataLookupS32 (&status, recipe, "PSF_MAX_NSTARS");
    assert (status);

    // supply the measured sky variance for optional constant errors (non-poissonian)
    float SKY_SIG = psMetadataLookupF32 (&status, recipe, "SKY_SIG");
    assert (status);

    // use poissonian errors or local-sky errors
    options->poissonErrorsPhotLMM = psMetadataLookupBool (&status, recipe, "POISSON.ERRORS.PHOT.LMM");
    assert (status);

    // use poissonian errors or local-sky errors
    options->poissonErrorsPhotLin = psMetadataLookupBool (&status, recipe, "POISSON.ERRORS.PHOT.LIN");
    assert (status);

    // use poissonian errors or local-sky errors
    options->poissonErrorsParams = psMetadataLookupBool (&status, recipe, "POISSON.ERRORS.PARAMS");
    assert (status);

    // how to model the PSF variations across the field
    options->psfTrendMode = pmTrend2DModeFromString (psMetadataLookupStr (&status, recipe, "PSF.TREND.MODE"));
    assert (status);

    options->psfTrendNx = psMetadataLookupS32 (&status, recipe, "PSF.TREND.NX");
    assert (status);

    options->psfTrendNy = psMetadataLookupS32 (&status, recipe, "PSF.TREND.NY");
    assert (status);

    // get the fixed PSF fit radius
    // XXX check that PSF_FIT_RADIUS < SKY_OUTER_RADIUS
    // options->radius = psMetadataLookupF32 (&status, recipe, "PSF_FIT_RADIUS");
    // assert (status);

    // values on readout->analysis if we have calculated a Gaussian window function, use that
    // for both the PSF fit radius and the aperture radius (scaling SIGMA), otherwise base the value on the recipe value for MOMENTS_GAUSS_SIGMA
    float gaussSigma = psMetadataLookupF32 (&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
    if (!status) {
        gaussSigma = psMetadataLookupF32 (&status, recipe, "MOMENTS_GAUSS_SIGMA");
    }
    float fitScale = psMetadataLookupF32(&status, recipe, "PSF_FIT_RADIUS_SCALE");
    float apScale = psMetadataLookupF32(&status, recipe, "PSF_APERTURE_SCALE");
    options->fitRadius = (int)(fitScale*gaussSigma);
    options->apRadius = (int)(apScale*gaussSigma);

    // XXX use the same radii for standard analysis as for the PSF creation
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "PSF_FIT_RADIUS", PS_META_REPLACE, "fit radius", options->fitRadius);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "PSF_APERTURE", PS_META_REPLACE, "psf aperture", options->apRadius);

    // dimensions of the field for which the PSF is defined
    options->psfFieldNx = readout->image->numCols;
    options->psfFieldNy = readout->image->numRows;
    options->psfFieldXo = readout->image->col0;
    options->psfFieldYo = readout->image->row0;

    int fitIter = psMetadataLookupS32(&status, recipe, "PSF_FIT_ITER"); // Maximum number of fit iterations
    if (!status || fitIter <= 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "PSF_FIT_ITER is not positive");
        return false;
    }
    float fitMinTol = psMetadataLookupF32 (&status, recipe, "PSF_FIT_MIN_TOL"); // Fit tolerance
    if (!status || !isfinite(fitMinTol) || fitMinTol <= 0) {
	fitMinTol = psMetadataLookupF32 (&status, recipe, "PSF_FIT_TOL"); // Fit tolerance
	if (!status || !isfinite(fitMinTol) || fitMinTol <= 0) {
	    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "PSF_FIT_MIN_TOL (and PSF_FIT_TOL) not defined or positive");
	    return false;
	}
    }
    float fitMaxTol = psMetadataLookupF32 (&status, recipe, "PSF_FIT_MAX_TOL"); // Fit tolerance
    if (!status || !isfinite(fitMaxTol) || fitMaxTol <= 0) {
	fitMaxTol = 1.0;
    }
    float maxChisqDOF = psMetadataLookupF32 (&status, recipe, "PSF_FIT_MAX_CHISQ"); // Fit tolerance

    bool chisqConvergence = psMetadataLookupBool (&status, recipe, "LMM_FIT_CHISQ_CONVERGENCE"); // Fit tolerance
    if (!status) {
	// default to the old method (chisqConvergence)
	chisqConvergence = true;
    }

    bool useReweighting = psMetadataLookupBool (&status, recipe, "LMM_FIT_USE_REWEIGHTING"); // Fit tolerance
    if (!status) {
	useReweighting = false;
    }

    int gainFactorMode = psMetadataLookupS32 (&status, recipe, "LMM_FIT_GAIN_FACTOR_MODE"); // Fit tolerance
    if (!status) {
	// default to the old method (chisqConvergence)
	gainFactorMode = 0;
    }

    // options which modify the behavior of the model fitting
    options->fitOptions                = pmSourceFitOptionsAlloc();
    options->fitOptions->nIter         = fitIter;
    options->fitOptions->minTol        = fitMinTol;
    options->fitOptions->maxTol        = fitMaxTol;
    options->fitOptions->maxChisqDOF   = maxChisqDOF;
    options->fitOptions->poissonErrors = options->poissonErrorsPhotLMM;
    options->fitOptions->weight        = PS_SQR(SKY_SIG);
    options->fitOptions->mode          = PM_SOURCE_FIT_PSF;
    options->fitOptions->covarFactor   = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix
    
    options->fitOptions->gainFactorMode   = gainFactorMode;
    options->fitOptions->chisqConvergence = chisqConvergence;
    options->fitOptions->useReweighting   = useReweighting;

    psArray *stars = psArrayAllocEmpty (sources->n);

    // select the candidate PSF stars (pointers to original sources)
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        if (source->tmpFlags & PM_SOURCE_TMPF_CANDIDATE_PSFSTAR) {
            // keep NSTARS PSF stars
            if (stars->n < NSTARS) {
                psArrayAdd (stars, 200, source);
            }
        }
    }

    // check that the identified psf stars sufficiently cover the region; if not, extend the
    // limits somewhat
    psphotCheckStarDistribution (stars, sources, options);

    psLogMsg ("psphot.pspsf", PS_LOG_DETAIL, "selected %ld candidate PSF objects\n", stars->n);

    if (stars->n < 50) {
        // ROBUST is too agressive if we only have a small number of PSF stars
        options->stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    } else {
        options->stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    }

    // get the list pointers for the PSF_MODEL entries
    psArray *modelNames = NULL;
    psMetadataItem *item = psMetadataLookup (recipe, "PSF_MODEL");
    if (item == NULL) psAbort("missing PSF_MODEL selection");

    if (item->type == PS_DATA_STRING) {
        modelNames = psArrayAlloc(1);
        modelNames->data[0] = psStringCopy (item->data.V);
    } else {
        if (item->type != PS_DATA_METADATA_MULTI) psAbort("missing PSF_MODEL selection");
        modelNames = psListToArray (item->data.list);
    }

    // generate a psf model using the first selection
    if (stars->n == 0) {
        psError(PSPHOT_ERR_PSF, false, "Failed to fit any models when choosing PSF");
        bool mdok;                      // Status of MD lookup
        if (!psMetadataLookupBool(&mdok, recipe, PSPHOT_RECIPE_PSF_FAKE_ALLOW)) {
            return false;
        }
        psErrorStackPrint(stderr, "Using guess PSF model");
        psErrorClear();

        psFree(options);

        // no sources are used as PSF stars

        // XXX set sxx, etc from FWHM in recipe
        pmPSF *psf = pmPSFBuildSimple (modelNames->data[0], 1.0, 1.0, 0.0, 1.0);
        psf->fieldNx = readout->image->numCols;
        psf->fieldNy = readout->image->numRows;
        psFree (modelNames);

        bool status = true;
        status &= psphotMakeFluxScale (readout->image, recipe, psf);
        status &= psphotPSFstats (readout, psf);
        if (!status) {
            psError(PSPHOT_ERR_PSF, false, "Failed to fit any models when choosing PSF");
            psFree (psf);
            return NULL;
        }

        // XXX set DSX_MEAN, etc?
        return psf;
    }

    // set up an array to store the results
    psArray *models = psArrayAlloc (modelNames->n);

    // try each model option listed in config
    // pmPSFtryModel makes a local copy of the sources -- those points are not the same as those for 'sources'
    for (int i = 0; i < modelNames->n; i++) {
        char *modelName = modelNames->data[i];
        pmPSFtry *try = pmPSFtryModel (stars, modelName, options, maskVal, markVal); // Attempt at fit
        if (!try) {
            // No big deal --- we'll try another model
            if (i < modelNames->n - 1) {
                psErrorClear();
            }
            continue;
        }
        models->data[i] = try;
    }

    // select the best of the models
    // here we are using the clippedStdev on the metric as the indicator
    int bestN = -1;
    float bestM = 0.0;
    for (int i = 0; i < models->n; i++) {
        pmPSFtry *try = models->data[i];
        if (try == NULL) {
            psTrace ("psphot", 3, "PSF model %d is NULL", i);
            continue;
        }
        float M = try->psf->dApResid;
        if (bestN < 0 || M < bestM) {
            bestM = M;
            bestN = i;
        }
    }

    // use the best model:
    if (bestN < 0) {
        psFree (models);
        psFree (stars);

        psErrorStackPrint (stderr, "Failed to fit any models when choosing PSF");
        psErrorClear();

        bool mdok;                      // Status of MD lookup
        if (!psMetadataLookupBool(&mdok, recipe, PSPHOT_RECIPE_PSF_FAKE_ALLOW)) {
            psFree (modelNames);
            psFree(options);
            return NULL;
        }

        // generate a psf model using the first selection
        psLogMsg ("psphot.pspsf", PS_LOG_INFO, "Using guess PSF model");

        // no sources are used as PSF stars

        // XXX set sxx, etc from FWHM in recipe
        pmPSF *psf = pmPSFBuildSimple (modelNames->data[0], 1.0, 1.0, 0.0, 1.0);
        psf->fieldNx = readout->image->numCols;
        psf->fieldNy = readout->image->numRows;
        psFree (modelNames);

        bool status = true;
        status &= psphotMakeFluxScale (readout->image, recipe, psf);
        status &= psphotPSFstats (readout, psf);
        if (!status) {
            psError(PSPHOT_ERR_PSF, false, "Failed to fit any models when choosing PSF");
            psFree (psf);
            return NULL;
        }

        // XXX set DSX_MEAN, etc?
        return psf;
    }

    psFree (modelNames);
    psFree (stars);

    pmPSFtry *try = models->data[bestN];

    // set the PSFSTAR flag for stars used for the PSF model
    int nKeep = 0;
    for (int i = 0; i < try->sources->n; i++) {
        pmSource *source = try->sources->data[i];
        if (try->mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
	source->mode |= PM_SOURCE_MODE_PSFSTAR;

	// this source was used: find the parent and set its PSFSTAR flag
	pmSource *realSource = source->parent;
	psAssert (realSource, "pmPSFtryAlloc should have set the parent pointers");
	realSource->mode |= PM_SOURCE_MODE_PSFSTAR;
	nKeep ++;
    }
    psLogMsg ("psphot.pspsf", PS_LOG_DETAIL, "used %d of %ld candidate PSF objects\n", nKeep, try->sources->n);

    // build a PSF residual image
    // we need to use the 'try' set because they have the associated models defined
    if (!psphotMakeResiduals (try->sources, recipe, try->psf, maskVal)) {
        psError(PSPHOT_ERR_PSF, false, "Unable to construct residual table for PSF");
        psFree (models);
        psFree(options);
        return NULL;
    }

    // XXX does this work here?
    psphotVisualShowPSFStars (recipe, try->psf, try->sources);

    // build the flux-to-magnitude conversion information
    if (!psphotMakeFluxScale (readout->image, recipe, try->psf)) {
        psError(PSPHOT_ERR_PSF, false, "Unable to construct flux scale for PSF");
        psFree (models);
        psFree(options);
        return NULL;
    }

    // build curve-of-growth vector for this psf
    if (!psphotMakeGrowthCurve (readout, recipe, try->psf, try->sources)) {
        psError(PSPHOT_ERR_PSF, false, "Unable to construct flux scale for PSF");
        psFree (models);
        psFree(options);
        return NULL;
    }

    // test dump of psf star data and psf-subtracted image
    if (psTraceGetLevel("psphot.psfstars") > 5) {
        psphotDumpPSFStars (readout, try, options->fitRadius, maskVal, markVal);
    }

    // save only the best model;
    pmPSF *psf = psMemIncrRefCounter(try->psf);
    psFree (models);

    // if (!psphotPSFstats (readout, psf)) {

    if (!psphotPSFstatsSources (readout, sources, psf)) {
        psError(PSPHOT_ERR_PSF, false, "cannot measure PSF shape terms");
        psFree(options);
        psFree(psf);
        return NULL;
    }



    char *modelName = pmModelClassGetName (psf->type);
    psLogMsg ("psphot.pspsf", PS_LOG_WARN, "select psf model: %f sec\n", psTimerMark ("psphot.choose.psf"));
    psLogMsg ("psphot.pspsf", PS_LOG_INFO, "psf model %s, ApResid: %f +/- %f (%d x %d model)\n", modelName, psf->ApResid, psf->dApResid, psf->trendNx, psf->trendNy);

    psFree (options);

    // save PSF on readout->analysis
    if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot psf model", psf)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "problem saving sources on readout");
        return false;
    }
    psFree (psf);

    psphotVisualShowPSFModel (readout, psf);

    return true;
}

// measure average parameters of the PSF model
bool psphotPSFstats (pmReadout *readout, pmPSF *psf) {

    psEllipseShape shape;
    psEllipseAxes axes;

    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(psf, false);

    psImage *image = readout->image;
    PS_ASSERT_PTR_NON_NULL(image, false);

    // XXX a minor hack: measure the values for a grid of points across the image, determine mean, UQ, LQ, etc:
    psVector *fwhmMajor = psVectorAllocEmpty (100, PS_DATA_F32);
    psVector *fwhmMinor = psVectorAllocEmpty (100, PS_DATA_F32);
    psVector *psfExtra1 = psVectorAllocEmpty (100, PS_DATA_F32);
    psVector *psfExtra2 = psVectorAllocEmpty (100, PS_DATA_F32);

    for (float ix = -0.4; ix <= +0.4; ix += 0.1) {
        for (float iy = -0.4; iy <= +0.4; iy += 0.1) {

            // use the center of the center pixel of the image
            float xc = ix*image->numCols + 0.5*image->numCols + image->col0 + 0.5;
            float yc = iy*image->numRows + 0.5*image->numRows + image->row0 + 0.5;

            // create modelPSF from this model
            pmModel *modelPSF = pmModelFromPSFforXY (psf, xc, yc, 1.0);
            if (!modelPSF) {
                fprintf (stderr, "?");
                continue;
            }

            // get the model full-width at half-max
            float FWHM_MAJOR = 2*modelPSF->class->modelRadius (modelPSF->params, 0.5);

            // XXX make sure this is consistent with the re-definition of PM_PAR_SXX
            shape.sx  = modelPSF->params->data.F32[PM_PAR_SXX];
            shape.sy  = modelPSF->params->data.F32[PM_PAR_SYY];
            shape.sxy = modelPSF->params->data.F32[PM_PAR_SXY];
            axes = psEllipseShapeToAxes (shape, 20.0);

            float FWHM_MINOR = FWHM_MAJOR * (axes.minor / axes.major);
            if (!isfinite(FWHM_MAJOR) || !isfinite(FWHM_MINOR)) {
                fprintf (stderr, "!");
		psFree (modelPSF);
                continue;
            }
            psVectorAppend (fwhmMajor, FWHM_MAJOR);
            psVectorAppend (fwhmMinor, FWHM_MINOR);

	    if (modelPSF->params->n > 7) {
		psVectorAppend (psfExtra1, modelPSF->params->data.F32[7]);
	    }
	    if (modelPSF->params->n > 8) {
		psVectorAppend (psfExtra2, modelPSF->params->data.F32[8]);
	    }
            psFree (modelPSF);

	    // float fwhmtest = pmPSFtoFWHM(psf, xc, yc);
	    // fprintf (stderr, "fwhm: %f, %f : %f\n", FWHM_MAJOR, FWHM_MINOR, fwhmtest);
        }
    }

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_SAMPLE_QUARTILE);
    if (!psVectorStats (stats, fwhmMajor, NULL, NULL, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failure to measure stats for FWHM MAJOR");
	goto escape;
    }

    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MAJ",   PS_META_REPLACE, "PSF FWHM Major axis (mean)", stats->sampleMean);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_SG",   PS_META_REPLACE, "PSF FWHM Major axis (sigma)", stats->sampleStdev);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_LQ",   PS_META_REPLACE, "PSF FWHM Major axis (lower quartile)", stats->sampleLQ);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_UQ",   PS_META_REPLACE, "PSF FWHM Major axis (upper quartile)", stats->sampleUQ);

    float fwhmMaj = stats->sampleMean;  // FWHM on major axis

    if (!psVectorStats (stats, fwhmMinor, NULL, NULL, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failure to measure stats for FWHM MINOR");
	goto escape;
    }
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MIN",   PS_META_REPLACE, "PSF FWHM Minor axis (mean)", stats->sampleMean);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_SG",   PS_META_REPLACE, "PSF FWHM Minor axis (sigma)", stats->sampleStdev);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_LQ",   PS_META_REPLACE, "PSF FWHM Minor axis (lower quartile)", stats->sampleLQ);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_UQ",   PS_META_REPLACE, "PSF FWHM Minor axis (upper quartile)", stats->sampleUQ);

    float fwhmMin = stats->sampleMean;  // FWHM on minor axis

    if (readout->parent) {

	// we now have 2 possible measurements of the seeing : the PSF model based version and
	// the Moments based value we need to define a definitive "CHIP.SEEING" value.  This
	// value is used by the PSF-matching programs (ppSub and ppStack).  The moments-based
	// value is probably more representative of the value needed by those tools (it is also
	// more stable?)
	bool status = false;
	float fwhmMajorMoments = psMetadataLookupF32(&status, readout->analysis, "IQ_FW1");
	float fwhmMinorMoments = psMetadataLookupF32(&status, readout->analysis, "IQ_FW1");

        pmChip *chip = readout->parent->parent; // Parent chip
        psAssert(chip, "Cell should be attached to a chip.");
        psMetadataItem *item = psMetadataLookup(chip->concepts, "CHIP.SEEING"); // Item with chip
        item->data.F32 = 0.5 * (fwhmMajorMoments + fwhmMinorMoments);

        psLogMsg ("psphot", PS_LOG_DETAIL, "fwhm (psf): %f,%f (moments): %f,%f", fwhmMaj, fwhmMin, fwhmMajorMoments, fwhmMinorMoments);
    }

    if (psfExtra1->n) {
	if (!psVectorStats (stats, psfExtra1, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats for PSF EXTRA 1");
	    goto escape;
	}
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "PSF_EXT1", PS_META_REPLACE, "PSF extra param 1", stats->sampleMean);
	psLogMsg ("psphot", PS_LOG_DETAIL, "PSF extra parameter 1: %f +/- %f", stats->sampleMean, stats->sampleStdev);
    }

    if (psfExtra2->n) {
	if (!psVectorStats (stats, psfExtra2, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats for PSF EXTRA 2");
	    goto escape;
	}
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "PSF_EXT2", PS_META_REPLACE, "PSF extra param 2", stats->sampleMean);
	psLogMsg ("psphot", PS_LOG_DETAIL, "PSF extra parameter 2: %f +/- %f", stats->sampleMean, stats->sampleStdev);
    }

    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "ANGLE",    PS_META_REPLACE, "PSF angle",           axes.theta);
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NPSFSTAR", PS_META_REPLACE, "Number of stars used to make PSF", psf->nPSFstars);

    // psLogMsg ("psphot", PS_LOG_DETAIL, "PSF angle: %f, nstars: %d", axes.theta, psf->nPSFstars);

    char *psfModelName = pmModelClassGetName(psf->type);
    psMetadataAddStr(readout->analysis,  PS_LIST_TAIL, "PSFMODEL", PS_META_REPLACE, "PSF Model Name", psfModelName);
    psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "PSF_OK",   PS_META_REPLACE, "Valid PSF Model?", true);

    int nParams = pmModelClassParameterCount(psf->type);
    psMetadataAddS32(readout->analysis, PS_LIST_TAIL, "PSF_NPAR",   PS_META_REPLACE, "Number of PSF parameters", nParams);

    psFree (fwhmMajor);
    psFree (fwhmMinor);
    psFree (psfExtra1);
    psFree (psfExtra2);
    psFree (stats);
    return true;

escape:
    psFree (fwhmMajor);
    psFree (fwhmMinor);
    psFree (psfExtra1);
    psFree (psfExtra2);
    psFree (stats);
    return false;
}

// measure average parameters of the PSF model
bool psphotPSFstatsSources (pmReadout *readout, psArray *sources, pmPSF *psf) {

    psEllipseShape shape;
    psEllipseAxes axes;

    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(psf, false);

    psImage *image = readout->image;
    PS_ASSERT_PTR_NON_NULL(image, false);

    // XXX a minor hack: measure the values for a grid of points across the image, determine mean, UQ, LQ, etc:
    psVector *fwhmMajor = psVectorAllocEmpty (100, PS_DATA_F32);
    psVector *fwhmMinor = psVectorAllocEmpty (100, PS_DATA_F32);
    psVector *psfExtra1 = psVectorAllocEmpty (100, PS_DATA_F32);
    psVector *psfExtra2 = psVectorAllocEmpty (100, PS_DATA_F32);

    // count how many PSF stars are defined
    int nPSFstars = 0; // count the number of PSF stars
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;
    }

    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// If no sources are PSFSTARs, then let's just use all with S/N > 0.1.
	// (This can happen if all are supplied)
	if (nPSFstars) {
	    if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;
	} else {
	    if (source->psfMagErr > 0.1) continue;
	}

	float xc = source->peak->xf;
	float yc = source->peak->yf;

	// create modelPSF from this model
	pmModel *modelPSF = pmModelFromPSFforXY (psf, xc, yc, 1.0);
	if (!modelPSF) {
	    fprintf (stderr, "?");
	    continue;
	}

	// get the model full-width at half-max
	float FWHM_MAJOR = 2*modelPSF->class->modelRadius (modelPSF->params, 0.5);

	// XXX make sure this is consistent with the re-definition of PM_PAR_SXX
	shape.sx  = modelPSF->params->data.F32[PM_PAR_SXX];
	shape.sy  = modelPSF->params->data.F32[PM_PAR_SYY];
	shape.sxy = modelPSF->params->data.F32[PM_PAR_SXY];
	axes = psEllipseShapeToAxes (shape, 20.0);

	float FWHM_MINOR = FWHM_MAJOR * (axes.minor / axes.major);
	if (!isfinite(FWHM_MAJOR) || !isfinite(FWHM_MINOR)) {
	    fprintf (stderr, "!");
	    psFree (modelPSF);
	    continue;
	}
	psVectorAppend (fwhmMajor, FWHM_MAJOR);
	psVectorAppend (fwhmMinor, FWHM_MINOR);

	if (modelPSF->params->n > 7) {
	    psVectorAppend (psfExtra1, modelPSF->params->data.F32[7]);
	}
	if (modelPSF->params->n > 8) {
	    psVectorAppend (psfExtra2, modelPSF->params->data.F32[8]);
	}
	psFree (modelPSF);

	// float fwhmtest = pmPSFtoFWHM(psf, xc, yc);
	// fprintf (stderr, "fwhm: %f, %f : %f\n", FWHM_MAJOR, FWHM_MINOR, fwhmtest);
    }

    // XXX should we catch this case and exit immediately? (need to raise an error?)
    if (0 && !fwhmMajor->n) {
	psLogMsg ("psphot", PS_LOG_DETAIL, "no stars on the chip to measure PSF FWHM");
	goto escape;
    }

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_SAMPLE_QUARTILE);
    if (!psVectorStats (stats, fwhmMajor, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats for FWHM MAJOR");
	goto escape;
    }

    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MAJ",   PS_META_REPLACE, "PSF FWHM Major axis (mean)", stats->sampleMean);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_SG",   PS_META_REPLACE, "PSF FWHM Major axis (sigma)", stats->sampleStdev);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_LQ",   PS_META_REPLACE, "PSF FWHM Major axis (lower quartile)", stats->sampleLQ);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_UQ",   PS_META_REPLACE, "PSF FWHM Major axis (upper quartile)", stats->sampleUQ);

    float fwhmMaj = stats->sampleMean;  // FWHM on major axis

    if (!psVectorStats (stats, fwhmMinor, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats for FWHM MINOR");
	goto escape;
    }
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MIN",   PS_META_REPLACE, "PSF FWHM Minor axis (mean)", stats->sampleMean);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_SG",   PS_META_REPLACE, "PSF FWHM Minor axis (sigma)", stats->sampleStdev);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_LQ",   PS_META_REPLACE, "PSF FWHM Minor axis (lower quartile)", stats->sampleLQ);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_UQ",   PS_META_REPLACE, "PSF FWHM Minor axis (upper quartile)", stats->sampleUQ);

    float fwhmMin = stats->sampleMean;  // FWHM on minor axis

    if (readout->parent) {

	// we now have 2 possible measurements of the seeing : the PSF model based version and
	// the Moments based value we need to define a definitive "CHIP.SEEING" value.  This
	// value is used by the PSF-matching programs (ppSub and ppStack).  The moments-based
	// value is probably more representative of the value needed by those tools (it is also
	// more stable?)
	bool status = false;
	float fwhmMajorMoments = psMetadataLookupF32(&status, readout->analysis, "IQ_FW1");
	float fwhmMinorMoments = psMetadataLookupF32(&status, readout->analysis, "IQ_FW1");

	pmChip *chip = readout->parent->parent; // Parent chip
	psAssert(chip, "Cell should be attached to a chip.");
	psMetadataItem *item = psMetadataLookup(chip->concepts, "CHIP.SEEING"); // Item with chip
	item->data.F32 = 0.5 * (fwhmMajorMoments + fwhmMinorMoments);

	psLogMsg ("psphot", PS_LOG_DETAIL, "fwhm (psf): %f,%f (moments): %f,%f", fwhmMaj, fwhmMin, fwhmMajorMoments, fwhmMinorMoments);
    }

    if (psfExtra1->n) {
	if (!psVectorStats (stats, psfExtra1, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats for PSF EXTRA 1");
	    goto escape;
	}
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "PSF_EXT1", PS_META_REPLACE, "PSF extra param 1", stats->sampleMean);
	psLogMsg ("psphot", PS_LOG_DETAIL, "PSF extra parameter 1: %f +/- %f", stats->sampleMean, stats->sampleStdev);
    }

    if (psfExtra2->n) {
	if (!psVectorStats (stats, psfExtra2, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats for PSF EXTRA 2");
	    goto escape;
	}
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "PSF_EXT2", PS_META_REPLACE, "PSF extra param 2", stats->sampleMean);
	psLogMsg ("psphot", PS_LOG_DETAIL, "PSF extra parameter 2: %f +/- %f", stats->sampleMean, stats->sampleStdev);
    }

    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "ANGLE",    PS_META_REPLACE, "PSF angle",           axes.theta);
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NPSFSTAR", PS_META_REPLACE, "Number of stars used to make PSF", psf->nPSFstars);

    // psLogMsg ("psphot", PS_LOG_DETAIL, "PSF angle: %f, nstars: %d", axes.theta, psf->nPSFstars);

    char *psfModelName = pmModelClassGetName(psf->type);
    psMetadataAddStr(readout->analysis,  PS_LIST_TAIL, "PSFMODEL", PS_META_REPLACE, "PSF Model Name", psfModelName);
    psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "PSF_OK",   PS_META_REPLACE, "Valid PSF Model?", true);

    int nParams = pmModelClassParameterCount(psf->type);
    psMetadataAddS32(readout->analysis, PS_LIST_TAIL, "PSF_NPAR",   PS_META_REPLACE, "Number of PSF parameters", nParams);

    psFree (fwhmMajor);
    psFree (fwhmMinor);
    psFree (psfExtra1);
    psFree (psfExtra2);
    psFree (stats);
    return true;

escape:
    // failed to measure the PSF stats
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MAJ",   PS_META_REPLACE, "PSF FWHM Major axis (mean)",           NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_SG",   PS_META_REPLACE, "PSF FWHM Major axis (sigma)",          NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_LQ",   PS_META_REPLACE, "PSF FWHM Major axis (lower quartile)", NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_UQ",   PS_META_REPLACE, "PSF FWHM Major axis (upper quartile)", NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MIN",   PS_META_REPLACE, "PSF FWHM Minor axis (mean)",           NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_SG",   PS_META_REPLACE, "PSF FWHM Minor axis (sigma)",          NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_LQ",   PS_META_REPLACE, "PSF FWHM Minor axis (lower quartile)", NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_UQ",   PS_META_REPLACE, "PSF FWHM Minor axis (upper quartile)", NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "ANGLE",      PS_META_REPLACE, "PSF angle",                            NAN);
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NPSFSTAR",   PS_META_REPLACE, "Number of stars used to make PSF", 0);
    psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "PSF_OK",     PS_META_REPLACE, "Valid PSF Model?", false);

    psFree (fwhmMajor);
    psFree (fwhmMinor);
    psFree (psfExtra1);
    psFree (psfExtra2);
    psFree (stats);
    return false;
}

// determine approximate PSF shape parameters based on the moments
bool psphotMomentsStats (pmReadout *readout, psArray *sources) {

    // without the PSF model, we can only rely on the source->moments
    // as a measure of the FWHM values

    int FWHM_N = 0;
    double FWHM_X = 0.0;
    double FWHM_Y = 0.0;
    double FWHM_T = 0.0;

    psEllipseMoments moments;
    psEllipseAxes axes;

    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(sources, false);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        if (!source) continue;
        if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;
        if (!source->moments) {
            continue;
        }

        moments.x2 = source->moments->Mxx;
        moments.y2 = source->moments->Myy;
        moments.xy = source->moments->Mxy;

        // limit axis ratio < 20.0
        axes = psEllipseMomentsToAxes (moments, 20.0);

        FWHM_X += axes.major * 2.35;
        FWHM_Y += axes.minor * 2.35;
        FWHM_T += axes.theta;
        FWHM_N ++;
    }

    FWHM_X /= FWHM_N;
    FWHM_Y /= FWHM_N;
    FWHM_T /= FWHM_N;

    if (readout->parent) {
        pmChip *chip = readout->parent->parent; // Parent chip
        psAssert(chip, "Cell should be attached to a chip.");
        psMetadataItem *item = psMetadataLookup(chip->concepts, "CHIP.SEEING"); // Item with chip
        item->data.F32 = 0.5 * (FWHM_X + FWHM_Y);
    }

    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MAJ",   PS_META_REPLACE, "PSF FWHM Major axis (mean)", FWHM_X);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_SG",   PS_META_REPLACE, "PSF FWHM Major axis (sigma)", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_LQ",   PS_META_REPLACE, "PSF FWHM Major axis (lower quartile)", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MJ_UQ",   PS_META_REPLACE, "PSF FWHM Major axis (upper quartile)", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FWHM_MIN",   PS_META_REPLACE, "PSF FWHM Minor axis (mean)", FWHM_Y);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_SG",   PS_META_REPLACE, "PSF FWHM Minor axis (sigma)", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_LQ",   PS_META_REPLACE, "PSF FWHM Minor axis (lower quartile)", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "FW_MN_UQ",   PS_META_REPLACE, "PSF FWHM Minor axis (upper quartile)", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "ANGLE",      PS_META_REPLACE, "PSF angle",           FWHM_T);
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NPSFSTAR",   PS_META_REPLACE, "Number of stars used to make PSF", 0);
    psMetadataAddStr(readout->analysis,  PS_LIST_TAIL, "PSFMODEL",   PS_META_REPLACE, "PSF Model Name", "NONE");
    psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "PSF_OK",     PS_META_REPLACE, "Valid PSF Model?", false);

    return true;
}

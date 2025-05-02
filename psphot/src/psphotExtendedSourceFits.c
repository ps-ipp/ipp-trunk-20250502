# include "psphotInternal.h"
bool psphotSetWindowTrail (float *fitRadius, float *windowRadius, pmReadout *readout, pmSource *source, psImageMaskType markVal, float newRadius);

// for now, let's store the detections on the readout->analysis for each readout
bool psphotExtendedSourceFits (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Extended Source Fits ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // perform full extended source non-linear fits?
    if (!psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS")) {
        psLogMsg ("psphot", PS_LOG_INFO, "skipping extended source fits\n");
        return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
	if (!psphotExtendedSourceFitsReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on to fit extended sources for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// non-linear model fitting for extended sources
bool psphotExtendedSourceFitsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;
    int Next = 0;
    int Nconvolve = 0;
    int NconvolvePass = 0;
    int Nplain = 0;
    int NplainPass = 0;
    int Nfaint = 0;
    int Nfail = 0;

    psphotSersicModelClassInit();

    psTimerStart ("psphot.extended");

    psphotFitInitExtended();

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    psLogMsg("psphot", PS_LOG_INFO, "extended source fits for image %d", index);
    psphotVisualShowImage(readout);

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source size");
	psphotSersicModelClassCleanup();
	return true;
    }

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }
    // do not thread if we are trying to study the fitting process
    if (psTraceGetLevel ("psphot.psphotFitEXT") >= 6) {
	nThreads = 0;
    }

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels
    assert (markVal);

    // source fitting parameters for extended source fits
    int fitIter = psMetadataLookupS32(&status, recipe, "EXT_FIT_ITER"); // Max number of fit iterations
    assert (status && fitIter > 0);

    float fitMinTol = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MIN_TOL"); // Fit tolerance
    if (!status || !isfinite(fitMinTol) || fitMinTol <= 0) {
	fitMinTol = psMetadataLookupF32 (&status, recipe, "PSF_FIT_TOL"); // Fit tolerance
	if (!status || !isfinite(fitMinTol) || fitMinTol <= 0) {
	    psAbort("PSF_FIT_MIN_TOL (and PSF_FIT_TOL) not defined or positive");
	}
    }

    float fitMaxTol = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MAX_TOL"); // Fit tolerance
    if (!status || !isfinite(fitMaxTol) || fitMaxTol <= 0) {
	fitMaxTol = 1.0;
    }

    float fitNsigmaConv = psMetadataLookupF32 (&status, recipe, "EXT_FIT_NSIGMA_CONV"); // number of sigma for the convolution
    if (!status || !isfinite(fitNsigmaConv) || fitNsigmaConv <= 0) {
	fitNsigmaConv = 5.0;
    }

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

    // perform full extended source non-linear fits?
    bool isInteractive = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS_INTERACTIVE");
    if (!status) isInteractive = false;

    // Define source fitting parameters for extended source fits
    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
    fitOptions->mode           = PM_SOURCE_FIT_EXT_AND_SKY;
    fitOptions->saveCovariance = true;  // XXX make this a user option?
    fitOptions->covarFactor    = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix
    fitOptions->nIter          = fitIter;
    fitOptions->minTol         = fitMinTol;
    fitOptions->maxTol         = fitMaxTol;
    fitOptions->nsigma         = fitNsigmaConv;

    fitOptions->gainFactorMode   = gainFactorMode;
    fitOptions->chisqConvergence = chisqConvergence;
    fitOptions->isInteractive    = isInteractive;
    fitOptions->useReweighting   = useReweighting;

    // use poissonian errors or local-sky errors
    fitOptions->poissonErrors = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS_POISSON");
    if (!status) fitOptions->poissonErrors = true;

    // Save pointer to fitOptions for possible use later reinstantiating the pcm models on the output readout
    // and during radial apertures measurements.
    psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PCM_FIT_OPTIONS", PS_DATA_UNKNOWN | PS_META_REPLACE, "pcm fit options", 
        fitOptions);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // select the collection of desired models
    psMetadata *allModels = psMetadataLookupMetadata (&status, recipe, "EXTENDED_SOURCE_MODELS");
    if (!status) {
        psWarning ("extended source model fits requested but model model is missing (EXTENDED_SOURCE_MODELS)\n");
	psFree (fitOptions);
	psphotSersicModelClassCleanup();
        return true;
    }
    if (allModels->list->n == 0) {
        psWarning ("extended source model fits requested but no models are specified\n");
	psFree (fitOptions);
	psphotSersicModelClassCleanup();
        return true;
    }

    psMetadata *models = NULL;
    char *modelSelection = psMetadataLookupStr (&status, recipe, "EXTENDED_SOURCE_MODELS_SELECTION");
    if (!modelSelection || !status) {
	models = psMemIncrRefCounter(allModels);
	goto got_models;
    } 

    if (!strcasecmp(modelSelection, "ALL")) {
	models = psMemIncrRefCounter(allModels);
	goto got_models;
    }

    if (!strcasecmp(modelSelection, "NONE")) {
	psWarning ("extended source model fits requested but no models are selected (EXTENDED_SOURCE_MODELS_SELECTION = NONE)\n");
	psFree (fitOptions);
	psphotSersicModelClassCleanup();
	return true;
    }

    psArray *selection = psStringSplitArray (modelSelection, ",", false);
    if (selection->n == 0) {
	psWarning ("extended source model fits requested but model selection string is empty (EXTENDED_SOURCE_MODELS_SELECTION)\n");
	psFree (fitOptions);
	psFree (selection);
	psphotSersicModelClassCleanup();
	return true;
    }
    models = psMetadataAlloc();
    for (int i = 0; i < selection->n; i++) {
	psMetadata *model = psMetadataLookupMetadata (&status, allModels, selection->data[i]);
	if (!model) {
	    psWarning ("extended source model selection string includes an invalid name %s\n", (char *) selection->data[i]);
	    continue;
	}
	psMetadataAddMetadata (models, PS_LIST_TAIL, selection->data[i], PS_META_REPLACE, "extended model", model);
    }
    psFree (selection);

    if (models->list->n == 0) {
	psWarning ("extended source model fits requested but no valid models in selection string (EXTENDED_SOURCE_MODELS_SELECTION = %s)\n", modelSelection);
	psFree (fitOptions);
	psFree (models);
	psphotSersicModelClassCleanup();
	return true;
    }

got_models:

    psphotInitRadiusEXT (recipe, readout);

    // validate the model entries
    psMetadataIterator *iter = psMetadataIteratorAlloc (models, PS_LIST_HEAD, NULL);
    psMetadataItem *item = NULL;
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {

	if (item->type != PS_DATA_METADATA) {
	    // XXX we could cull the bad entries or build a validated model folder
	    psAbort ("Invalid type for EXTENDED_SOURCE_MODEL entry %s, not a metadata folder", item->name);
	}

	psMetadata *model = (psMetadata *) item->data.md;

	// check on the model type
	char *modelName = psMetadataLookupStr (&status, model, "MODEL");
	int modelType = pmModelClassGetType (modelName);
	if (modelType < 0) {
	    psAbort ("Unknown model class for EXTENDED_SOURCE_MODEL entry %s: %s", item->name, modelName);
	}
	psMetadataAddS32 (model, PS_LIST_TAIL, "MODEL_TYPE", PS_META_REPLACE, "", modelType);

	// check on the SNLIM, set a float value
	char *SNword = psMetadataLookupStr (&status, model, "SNLIM");
	if (!status) {
	    psAbort("SNLIM not defined for extended source model %s\n", item->name);
	}
	float SNlim = atof (SNword);
	psMetadataAddF32 (model, PS_LIST_TAIL, "SNLIM_VALUE", PS_META_REPLACE, "", SNlim);

	// check on the PSF-Convolution status
	char *convolvedWord = psMetadataLookupStr (&status, model, "PSF_CONVOLVED");
	if (!status || (strcasecmp (convolvedWord, "true") && strcasecmp (convolvedWord, "false"))) {
	    psAbort ("PSF_CONVOLVED entry invalid or missing for EXTENDED_SOURCE_MODEL entry %s", item->name);
	}
	bool convolved = !strcasecmp (convolvedWord, "true");
	psMetadataAddBool (model, PS_LIST_TAIL, "PSF_CONVOLVED_VALUE", PS_META_REPLACE, "", convolved);

	if (convolved) {
	    psLogMsg ("psphot", PS_LOG_INFO, "using convolved model class %s (%s)", modelName, item->name);
	} else {
	    psLogMsg ("psphot", PS_LOG_INFO, "using simple    model class %s (%s)", modelName, item->name);
	}
    }
    psFree (iter);

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion *AnalysisRegion = psRegionAlloc(0,0,0,0);
    *AnalysisRegion = psRegionForImage(readout->image, psRegionFromString (region));
    if (psRegionIsNaN (*AnalysisRegion)) psAbort("analysis region mis-defined");

    // what fraction of the PSF is used? (radius in pixels : 2 -> 5x5 box)
    int psfSize = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");
    assert (status);

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_EXTENDED_FIT");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, models);

            // Allocate a metadata iterator here because psMetadataIteratorAlloc/Free are not thread safe
            psMetadataIterator *iter = psMetadataIteratorAlloc (models, PS_LIST_HEAD, NULL);
            psArrayAdd(job->args, 1, iter);
            psArrayAdd(job->args, 1, AnalysisRegion);
            psArrayAdd(job->args, 1, fitOptions);

            PS_ARRAY_ADD_SCALAR(job->args, psfSize, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal, PS_TYPE_IMAGE_MASK);

            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Next
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nconvolve
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for NconvolvePass
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nplain
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for NplainPass
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfaint
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfail

// XXX TEST 
	    if (!isInteractive) {
		if (!psThreadJobAddPending(job)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		    psFree(AnalysisRegion);
		    psFree (fitOptions);
		    psFree (models);
		    psphotSersicModelClassCleanup();
		    return false;
		} 
	    } else {
		// run without threading
		if (!psphotExtendedSourceFits_Threaded(job)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		    psFree(AnalysisRegion);
		    psFree (fitOptions);
		    psFree (models);
		    psphotSersicModelClassCleanup();
		    return false;
		}
		psScalar *scalar = NULL;
		scalar = job->args->data[9];
		Next += scalar->data.S32;
		scalar = job->args->data[10];
		Nconvolve += scalar->data.S32;
		scalar = job->args->data[11];
		NconvolvePass += scalar->data.S32;
		scalar = job->args->data[12];
		Nplain += scalar->data.S32;
		scalar = job->args->data[13];
		NplainPass += scalar->data.S32;
		scalar = job->args->data[14];
		Nfaint += scalar->data.S32;
		scalar = job->args->data[15];
		Nfail += scalar->data.S32;
		psFree(job->args->data[3]); // iterator allocated above
		psFree(job);
	    }
	}

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
	    psFree(AnalysisRegion);
	    psFree (fitOptions);
	    psFree (models);
	    psphotSersicModelClassCleanup();
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
                psScalar *scalar = NULL;
                scalar = job->args->data[9];
                Next += scalar->data.S32;
                scalar = job->args->data[10];
                Nconvolve += scalar->data.S32;
                scalar = job->args->data[11];
                NconvolvePass += scalar->data.S32;
                scalar = job->args->data[12];
                Nplain += scalar->data.S32;
                scalar = job->args->data[13];
                NplainPass += scalar->data.S32;
                scalar = job->args->data[14];
                Nfaint += scalar->data.S32;
                scalar = job->args->data[15];
                Nfail += scalar->data.S32;
                psFree(job->args->data[3]); // metadata iterator allocated above
            }
            psFree(job);
	}
    }
    psFree (cellGroups);
    psFree(AnalysisRegion);
    psFree (fitOptions);
    psFree (models);

    psphotSersicModelClassCleanup();

    psphotVisualShowResidualImage (readout, false);

    psLogMsg ("psphot", PS_LOG_WARN, "extended source model fits: %f sec for %d objects\n", psTimerMark ("psphot.extended"), Next);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d convolved models (%d passed)\n", Nconvolve, NconvolvePass);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d plain models (%d passed)\n", Nplain, NplainPass);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d too faint to fit, %d failed\n", Nfaint, Nfail);

    psphotFitSummaryExtended();

    return true;
}

// non-linear model fitting for extended sources
bool psphotExtendedSourceFits_Threaded (psThreadJob *job) {

    bool status;
    int Next = 0;
    int Nfaint = 0;
    int Nfail = 0;
    int Nconvolve = 0;
    int NconvolvePass = 0;
    int Nplain = 0;
    int NplainPass = 0;
    float fitRadius, windowRadius;
    psScalar *scalar = NULL;

    // arguments: readout, sources, models, region, psfSize, maskVal, markVal
    pmReadout *readout       = job->args->data[0];
    psArray *sources         = job->args->data[1];
    psMetadata *models       = job->args->data[2];
    psMetadataIterator *iter = job->args->data[3];
    psRegion *region         = job->args->data[4];
    pmSourceFitOptions *fitOptions = job->args->data[5];

    int psfSize             = PS_SCALAR_VALUE(job->args->data[6],S32);
    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[7],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal = PS_SCALAR_VALUE(job->args->data[8],PS_TYPE_IMAGE_MASK_DATA);

    // psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);

    pmModelStatus badModel = PM_MODEL_STATUS_NONE;
    badModel |= PM_MODEL_STATUS_BADARGS;
    badModel |= PM_MODEL_STATUS_OFFIMAGE;
    badModel |= PM_MODEL_STATUS_NAN_CHISQ;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GUESS;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GRID;
    badModel |= PM_MODEL_PCM_FAIL_GUESS;

    // choose the sources of interest
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];

	// rules for measuring petrosian parameters for specific objects are set in
	// psphotChooseAnalysisOptions.c
	if (!(source->tmpFlags & PM_SOURCE_TMPF_EXT_FIT)) continue;

	// limit selection by analysis region (XXX move this into psphotChooseAnalysisOption?)
        if (source->peak->x < region->x0) continue;
        if (source->peak->y < region->y0) continue;
        if (source->peak->x > region->x1) continue;
        if (source->peak->y > region->y1) continue;

        // replace object in image
        if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
            pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
        }
        Next ++;

	// set the fit radius based on the first radial moment (also sets the mask pixels)
	psphotSetRadiusMomentsExact(&fitRadius, &windowRadius, readout, source, markVal); // NOTE : 6 allocs

	// UPDATE : we have changed the moments calculation.  There is now an iteration within 
	// psphotKronMasked to determine moments appropriate for a larger object.  The values
	// Mrf, KronFlux, and KronFluxErr are calculated for the iterated radius.  The other
	// values are left at the psf-based values.

        // allocate the array to store the model fits
        if (source->modelFits == NULL) {
            source->modelFits = psArrayAllocEmpty (models->list->n);
        }

# ifdef TEST_OBJECT
	bool testObject = false;
	testObject |= ((fabs(source->peak->xf -  179) < 5) && (fabs(source->peak->yf - 1138) < 5));
	if (testObject) {
	    fprintf (stderr, "test object @ %f, %f\n", source->peak->xf, source->peak->yf);
	    // psTraceSetLevel ("psModules.objects.pmPCM_MinimizeChisq", 5);
	    // psTraceSetLevel ("psphot.psphotExtendedSourceFits_Threaded", 5);
	}
# endif 

        // loop here over the models chosen for each source (exclude by S/N)
        // Reset the iterator
        psMetadataIteratorSet(iter, PS_LIST_HEAD);
        psMetadataItem *item = NULL;
        while ((item = psMetadataGetAndIncrement (iter)) != NULL) {

          // XXX this should have been forced above
          assert (item->type == PS_DATA_METADATA);
          psMetadata *model = (psMetadata *) item->data.md;

          // check the SNlim and skip model if source is too faint
          float FIT_SN_LIM = psMetadataLookupF32 (&status, model, "SNLIM_VALUE");
          assert (status);

	  // limit selection to some SN limit for specific models (this value only applies if > EXTENDED_SOURCE_SN_LIM)
	  if (isfinite(FIT_SN_LIM)) {
	    if (source->moments->KronFlux < FIT_SN_LIM * source->moments->KronFluxErr) {
	      Nfaint ++;
	      continue;
	    }
	  }

	  source->mode2 |= PM_SOURCE_MODE2_EXT_FITS_RUN;

          // check on the model type
          pmModelType modelType = psMetadataLookupS32 (&status, model, "MODEL_TYPE");
          assert (status);

          // check on the PSF-Convolution status
          bool convolved = psMetadataLookupBool (&status, model, "PSF_CONVOLVED_VALUE");
          assert (status);

          // fit the model as convolved or not
          pmModel *modelFit = NULL;
          if (convolved) {
	      // NOTE : 4 more allocs to here
              modelFit = psphotFitPCM (readout, source, fitOptions, modelType, maskVal, markVal, psfSize); // NOTE : 2313 allocs in here
              if (!modelFit) {
                  psTrace ("psphot", 5, "failed to fit psf-conv model for object at %f, %f", source->moments->Mx, source->moments->My);
		  Nfail ++;
		  source->mode2 |= PM_SOURCE_MODE2_EXT_FITS_FAIL;
                  continue;
              }
              psTrace ("psphot", 4, "fit psf-conv model for %f, %f : %s chisq = %f (npix: %d, niter: %d)\n", 
		       source->moments->Mx, source->moments->My, pmModelClassGetName (modelFit->type), modelFit->chisq, modelFit->nPix, modelFit->nIter);
              Nconvolve ++;
              if (!(modelFit->flags & badModel)) {
                  float *PAR = modelFit->params->data.F32;
                  psEllipseAxes axes = pmPSF_ModelToAxes (PAR, modelFit->class->useReff);
                  if (axes.major >= 100) {
                      Nfail ++;
                      source->mode2 |= PM_SOURCE_MODE2_EXT_FITS_FAIL;
                      psFree(modelFit);
                      continue;
                  }
                  NconvolvePass ++;
		  source->mode |= PM_SOURCE_MODE_EXTENDED_FIT;
              }
          } else {
	      bool doneFits = false;
	      while (!doneFits) {
		  psFree (source->modelFlux);
		  source->modelFlux = NULL;
		  modelFit = psphotFitEXT (modelFit, readout, source, fitOptions, modelType, maskVal, markVal);
		  if (!modelFit) {
		      psTrace ("psphot", 5, "failed to fit plain model for object at %f, %f", source->moments->Mx, source->moments->My);
		      Nfail ++;
		      doneFits = true;
		      source->mode2 |= PM_SOURCE_MODE2_EXT_FITS_FAIL;
		      continue;
		  }
		  psTrace ("psphot", 4, "fit plain model for %f, %f : %s chisq = %f (npix: %d, niter: %d)\n", source->moments->Mx, source->moments->My, pmModelClassGetName (modelFit->type), modelFit->chisq, modelFit->nPix, modelFit->nIter);
		  Nplain ++;
		  if (!(modelFit->flags & badModel)) {
		      NplainPass ++;
		      source->mode |= PM_SOURCE_MODE_EXTENDED_FIT;
		  }
		  doneFits = true;

		  if (modelType == pmModelClassGetType("PS_MODEL_TRAIL")) {
		      // calculate the object end points:
		      float *PAR = modelFit->params->data.F32;
		      float Xmin = PAR[PM_PAR_XPOS] - 0.5*PAR[PM_PAR_LENGTH]*sin(PAR[PM_PAR_THETA]);
		      float Ymin = PAR[PM_PAR_YPOS] - 0.5*PAR[PM_PAR_LENGTH]*cos(PAR[PM_PAR_THETA]);
		      float Xmax = PAR[PM_PAR_XPOS] + 0.5*PAR[PM_PAR_LENGTH]*sin(PAR[PM_PAR_THETA]);
		      float Ymax = PAR[PM_PAR_YPOS] + 0.5*PAR[PM_PAR_LENGTH]*cos(PAR[PM_PAR_THETA]);

		      if ((fabs(source->peak->xf - 2572) < 20) && (fabs(source->peak->yf - 5874) < 20)) {
			  fprintf (stderr, "src vs fit : %d %d - %d %d | %f %f - %f %f\n", 
				   source->pixels->col0, source->pixels->row0, 
				   source->pixels->col0 + source->pixels->numCols, source->pixels->row0 + source->pixels->numRows, 
				   Xmin, Ymin, Xmax, Ymax);
		      }
		      if (PAR[PM_PAR_LENGTH] > 0.9*fitRadius) {
			  doneFits = false;
			  fprintf (stderr, "update window : %f %f : %f -> %f\n", source->peak->xf, source->peak->yf, fitRadius, 2*fitRadius);
			  psphotSetWindowTrail (&fitRadius, &windowRadius, readout, source, markVal, fitRadius*2.0);
			  source->mode2 |= PM_SOURCE_MODE2_EXT_FITS_RETRY;
		      }
		  }
	      }
          }
	  psAssert (modelFit, "modelFit not set?");

          // test for fit quality / result
	  modelFit->fitRadius = fitRadius;
          psArrayAdd (source->modelFits, 4, modelFit);

          psFree (modelFit);
        }

	// we are allowed to fit both stars and non-stars here -- if we have fitted
	// something which we think is a star, we should use that model to subtract the
	// object from the image.
        if (source->type == PM_SOURCE_TYPE_STAR) {
	  // ensure the modelPSF is cached
	  pmSourceCacheModel (source, maskVal);
          pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
          continue;
        }

        // evaluate the relative quality of the models, choose one
	// the PSF model might be the best fit : allow it to succeed
        float minChisq = NAN;
        int minModel = -1;
        for (int i = 0; i < source->modelFits->n; i++) {
            pmModel *model = source->modelFits->data[i];

	    // skip the really bad fits
            if (!(model->flags & PM_MODEL_STATUS_FITTED)) continue;
            if (model->flags & badModel) continue;

            // if (model->flags & (PM_MODEL_STATUS_NONCONVERGE)) continue;

            if ((minModel < 0) || (model->chisq < minChisq)) {
                minChisq = model->chisq;
                minModel = i;
            }
        }

	// clear the circular mask
	psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); 

        if (minModel == -1) {
          // no valid extended fit; re-subtract the object, leave local sky
          psTrace ("psphot", 5, "failed to fit extended source model to object at %f, %f", source->moments->Mx, source->moments->My);

	  // ensure the modelEXT is cached
	  pmSourceCacheModel (source, maskVal);

# if (0 && PS_TRACE_ON)
	  pmModel *model = source->modelFits->data[0];
	  int flags = 0xffffffff;
	  if (model) {
	    flags = model->flags;
	  }
          fprintf (stderr, "failed to fit extended source model to object %d @ %f, %f (%x)\n", source->id, source->moments->Mx, source->moments->My, flags);
#endif
          pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

	  source->mode2 |= PM_SOURCE_MODE2_EXT_FITS_NONE;
          continue;
        }

        // save the best extended model in modelEXT
        psFree (source->modelEXT);
        source->modelEXT = psMemIncrRefCounter (source->modelFits->data[minModel]);
	source->type = PM_SOURCE_TYPE_EXTENDED;
	source->mode |= PM_SOURCE_MODE_EXTMODEL;
	source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;

	// adjust the window so the subtraction covers the faint wings
	psphotSetRadiusMoments(&fitRadius, &windowRadius, readout, source, markVal);

	// cache the model flux
	if (source->modelEXT->isPCM) {
	    // fprintf (stderr, "subtract PCM extended source model for object %d @ %f, %f\n", source->id, source->moments->Mx, source->moments->My);
	    pmPCMCacheModel (source, maskVal, psfSize, fitOptions->nsigma);
	} else {
	    // fprintf (stderr, "subtract non-PCM extended source model for object %d @ %f, %f\n", source->id, source->moments->Mx, source->moments->My);
	    pmSourceCacheModel (source, maskVal);
	}
        source->modelEXT->flags |= PM_MODEL_BEST_FIT;

        // subtract the best fit from the object, leave local sky
        pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

        psTrace ("psphot", 4, "best ext model for %f, %f : %s chisq = %f\n", source->moments->Mx, source->moments->My, pmModelClassGetName (source->modelEXT->type), source->modelEXT->chisq);
        psTrace ("psphot", 5, "extended source model for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);

# ifdef TEST_OBJECT
	if (testObject) {
	  // psTraceSetLevel ("psModules.objects.pmPCM_MinimizeChisq", 0);
	  // psTraceSetLevel ("psphot.psphotExtendedSourceFits_Threaded", 0);
	}
# endif
    }

    // change the value of a scalar on the array (wrap this and put it in psArray.h)
    scalar = job->args->data[9];
    scalar->data.S32 = Next;

    scalar = job->args->data[10];
    scalar->data.S32 = Nconvolve;

    scalar = job->args->data[11];
    scalar->data.S32 = NconvolvePass;

    scalar = job->args->data[12];
    scalar->data.S32 = Nplain;

    scalar = job->args->data[13];
    scalar->data.S32 = NplainPass;

    scalar = job->args->data[14];
    scalar->data.S32 = Nfaint;

    scalar = job->args->data[15];
    scalar->data.S32 = Nfail;

    return true;
}

# define PAD_WINDOW 3.0
bool psphotSetWindowTrail (float *fitRadius, float *windowRadius, pmReadout *readout, pmSource *source, psImageMaskType markVal, float newRadius) {

    psRegion newRegion;

    psAssert (source, "source not defined??");
    psAssert (source->moments, "moments not defined??");

    *fitRadius = newRadius;
    *windowRadius = *fitRadius + PAD_WINDOW;

    // check to see if new region is completely contained within old region
    newRegion = psRegionForSquare (source->peak->xf, source->peak->yf, *windowRadius);
    newRegion = psRegionForImage (readout->image, newRegion);

    // redefine the pixels to match
    pmSourceRedefinePixelsByRegion (source, readout, newRegion);

    // clear the circular mask
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); 

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, source->peak->xf, source->peak->yf, *fitRadius, "OR", markVal);

    return true;
}

# include "psphotInternal.h"

static bool psphotStackUpdateLoadSources (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotDumpImages (pmConfig *config, const pmFPAview *view, const char *filerule, char *base);

// relevant filesets:
# define STACK_RAW "PSPHOT.STACK.INPUT.RAW"
# define STACK_OUT "PSPHOT.STACK.OUTPUT.IMAGE"

bool psphotStackUpdateReadout (pmConfig *config, const pmFPAview *view) {

    psArray *objects = NULL;

    // measure the total elapsed time in psphotReadout.  dtime is the elapsed time used jointly
    // by the multiple threads, not the total time used by all threads.
    psTimerStart ("psphotReadout");

    pmModelClassSetLimits(PM_MODEL_LIMITS_LAX); // allow models to have ugly fits (eg, central cusp)

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }
    // optional break-point for processing
    char *breakPt = psMetadataLookupStr (NULL, recipe, "BREAK_POINT");
    psAssert (breakPt, "configuration error: set BREAK_POINT");

    // load WCS 
    if (!psphotStackLoadWCS(config, view, STACK_RAW)) {
        psError (PSPHOT_ERR_CONFIG, false, "trouble loading WCS for %s", STACK_RAW);
        return false;
    }

    // set the photcode for each image
    if (!psphotAddPhotcode (config, view, STACK_RAW)) {
        psError (PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // Generate the mask and weight images (if not supplied) and set mask bits. 
    if (!psphotSetMaskAndVariance (config, view, STACK_RAW)) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "NOTHING")) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    if (!psphotLoadBackgroundModel (config, view, STACK_RAW)) {
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!psphotSubtractBackground (config, view, STACK_RAW)) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "BACKMDL")) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // Load the psf from the previous run. 
    // Note this must be done before we load the sources because it is used.
    // I believe only to set the residuals
    if (!psphotLoadPSF (config, view, STACK_RAW)) {
        // this only happens if we had a programming error in psphotLoadPSF
        psError (PSPHOT_ERR_UNKNOWN, false, "error loading psf model");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "PSFMODEL")) {
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // Copy the sources loaded to the sourcesReadout to STACK_RAW, updating various
    // measurements which are not recorded in the cmf files
    if (!psphotStackUpdateLoadSources (config, view, STACK_RAW)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "error finishing sources");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "UPDATELOADEDSOURCES")) {
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    if (!psphotDeblendSatstars (config, view, STACK_RAW)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed on satstar deblend analysis");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    if (!psphotMergeSources (config, view, STACK_RAW)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to merge sources");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    if (!psphotRemoveAllSources (config, view, STACK_RAW, false)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "error subtracting sources");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "REMOVESOURCES")) {
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    // Connect loaded sources back into Objects
    objects = psphotLinkSources (config, view, STACK_RAW);
    if (!objects) {
        psError (PSPHOT_ERR_UNKNOWN, false, "error linking sources into objects");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "LINKSOURCES")) {
        psFree(objects);
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    // XXX: Do we need to choose analysis options?
    // How about other tmpFlags?

    // create source children for the OUT filerule (for radial aperture photometry and output) 
    // NOTE: The new source children have image arrays pointing to the readout associated with 
    // STACK_OUT.  in psphotStackMatchPSFsetup, we copy the current pixel values from RAW to OUT, 
    // but keep the pointers the same so we do not break these source image references
    // XXX NOTE : if we use the pre-20130914 psphotStackReadout code, we need to use 'false' for the
    // sourcesSubtracted argument
    // XXX: do this here so that the variance is availble to rebuild the models
    psphotStackMatchPSFsetup (config, view, STACK_OUT, STACK_RAW);
    psArray *objectsOut = psphotSourceChildrenByObject (config, view, STACK_OUT, STACK_RAW, objects, true);
    if (!objectsOut) {
	psFree(objects);
	psError (PSPHOT_ERR_UNKNOWN, false, "failure in peak analysis");
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    // These arrays are no longer needed. The inputs' sources arrays have references to the data
    psFree (objects);
    psFree (objectsOut);

    if (!strcasecmp (breakPt, "SOURCECHILDREN")) {
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    bool radial_apertures = psMetadataLookupBool(NULL, recipe, "RADIAL_APERTURES");
    if (radial_apertures) {
        // measure circular, radial apertures (objects sorted by S/N)
        // this forces photometry on the undetected sources from other images

//	Moved this up above
//	psphotStackMatchPSFsetup (config, view, STACK_OUT, STACK_RAW);
	psphotDumpImages (config, view, STACK_RAW, "raw.t0");
	psphotDumpImages (config, view, STACK_OUT, "out.t0");

        int nRadialEntries = psphotStackMatchPSFsEntries(config, view, STACK_OUT);

        for (int entry = 0; entry < nRadialEntries; entry++) {
            // NOTE: entry 0 is the unmatched image set

	    char line[256];

            // measure circular, radial apertures (objects sorted by S/N)
            psphotRadialApertures (config, view, STACK_OUT, entry); 
	    snprintf (line, 256, "%s.%d", "out.t1", entry);
	    psphotDumpImages (config, view, STACK_OUT, line);

            // replace the flux in the image so it is returned to its original state
            psphotReplaceAllSources (config, view, STACK_OUT, false);
	    snprintf (line, 256, "%s.%d", "out.t2", entry);
	    psphotDumpImages (config, view, STACK_OUT, line);

	    if (entry < nRadialEntries - 1) {
		// smooth to the next FWHM
		// this function does nothing if the targetFWHM is smaller than the currentFWHM
		psphotStackMatchPSFsNext(config, view, STACK_OUT, entry);
		snprintf (line, 256, "%s.%d", "out.t3", entry);
		psphotDumpImages (config, view, STACK_OUT, line);
		psMemDump("matched");

		// re-measure the PSF for the smoothed image (using entries in 'allSources')
		if (!psphotChoosePSF (config, view, STACK_OUT, false)) {
                    psLogMsg ("psphot", 3, "failure to construct a psf model in radial aperture loop for entry :%d", entry);
                    return psphotReadoutCleanup (config, view, STACK_RAW);
                }

		// this is necessary to update the models based on the new PSF
		psphotResetModels (config, view, STACK_OUT);

		// this is necessary to get the right normalization for the new models
		// and to subtract the sources
		psphotFitSourcesLinear (config, view, STACK_OUT, false, false);

#ifdef notyet
            psphotRemoveAllSources (config, view, STACK_OUT, false);
#endif

		snprintf (line, 256, "%s.%d", "out.t4", entry);
		psphotDumpImages (config, view, STACK_OUT, line);
	    }
        }
    }

    psphotCopyEfficiency (config, view, STACK_OUT, STACK_RAW);

    // replace background in residual image
    psphotSkyReplace (config, view, STACK_RAW);

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, STACK_RAW);
    psphotSourceFreePixels (config, view, STACK_OUT);

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, STACK_RAW);
}

static void copyHeaderValues (psMetadata *analysis, psMetadata *sourcesHeader) {
    // copy parameters from measurment of background model from input soources file header
    // to the analysis.
#define COPY_PARAM(_key) \
        psMetadataAddF32(analysis, PS_LIST_TAIL, _key,  PS_META_REPLACE, _key, \
            psMetadataLookupF32(NULL, sourcesHeader, _key) )

        // These three aren't saved to the header. These values should be close. 
        // XXX: right?

        psMetadataAddF32(analysis, PS_LIST_TAIL, "SKY_MEAN",  PS_META_REPLACE, "SKY_MEAN",
            psMetadataLookupF32(NULL, sourcesHeader, "MSKY_MN" ));
        psMetadataAddF32(analysis, PS_LIST_TAIL, "SKY_STDEV",  PS_META_REPLACE, "SKY_STDEV",
            psMetadataLookupF32(NULL, sourcesHeader, "MSKY_DEV" ));
        psMetadataAddF32(analysis, PS_LIST_TAIL, "SKY_DQ",  PS_META_REPLACE, "SKY_DQ",
            psMetadataLookupF32(NULL, sourcesHeader, "MSKY_DQ" ));

        COPY_PARAM( "MSKY_MN");
        COPY_PARAM( "MSKY_SIG" );
        COPY_PARAM( "MSKY_DEV" );
        COPY_PARAM( "MSKY_DQ" );
        COPY_PARAM( "MSKY_MAX" );
        COPY_PARAM( "MSKY_MIN" );
        COPY_PARAM( "MSKY_NX" );
        COPY_PARAM( "MSKY_NY" );
        // COPY_PARAM( "CHIP_SEEING" );
}

static bool psphotStackUpdateLoadSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {
    bool status = true;
    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmFPAfile *sourcesFile = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.SOURCES", index);
    pmReadout *sourcesReadout = pmFPAviewThisReadout(view, sourcesFile->fpa);
    psAssert (sourcesReadout, "missing sources readout?");

    pmDetections *detectionsIn = psMetadataLookupPtr (&status, sourcesReadout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detectionsIn, "missing input detections?");

    psArray *sources = detectionsIn->allSources;
    psAssert (sources, "missing sources?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
        detections = pmDetectionsAlloc();
        psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_UNKNOWN | PS_META_REPLACE, "psphot detections",
            detections);
        psFree(detections);
    } else {
        // What is this doing here? 
        psAssert (detections == NULL, "not expecting detections to exist already. watsup?");
    }

    // copy sources from input detections
    detections->newSources = psMemIncrRefCounter (sources);

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping sources update");
        return true;
    }

    pmDetEff *de = psMetadataLookupPtr(&status, sourcesReadout->analysis, PM_DETEFF_ANALYSIS); // Detection efficiency
    if (de) {
        if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, PM_DETEFF_ANALYSIS, PS_META_REPLACE | PS_DATA_UNKNOWN, 
                "Detection efficiency", de)) {
            psError (PSPHOT_ERR_CONFIG, false, "problem saving Detection efficiency on readout");
            return false;
        }
    }

    psMetadata *sourcesHeader = psMetadataLookupPtr(&status, sourcesReadout->analysis, "INPUT.SOURCES.HEADER");
    psAssert (sourcesHeader, "missing sourcesHeader?");
    copyHeaderValues(readout->analysis, sourcesHeader);

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // This code currently assumes that all extended source models are PCM.
    // Examine the recipe and make sure that this is true.
    psMetadata *allModels = psMetadataLookupMetadata (&status, recipe, "EXTENDED_SOURCE_MODELS");
    if (!status) {
        psAssert (allModels, "failed to find list of extended source models\n");
    }
    psMetadataIterator *iter = psMetadataIteratorAlloc (allModels, PS_LIST_HEAD, NULL);
    psMetadataItem *item = NULL;
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
        if (item->type != PS_DATA_METADATA) {
            psAbort ("Invalid type for EXTENDED_SOURCE_MODEL entry %s, not a metadata folder", item->name);
        }
        psMetadata *model = (psMetadata *) item->data.md;
        // char *modelName = psMetadataLookupStr (&status, model, "MODEL");

        char *convolvedWord = psMetadataLookupStr (&status, model, "PSF_CONVOLVED");
        if (!status || (strcasecmp (convolvedWord, "true") && strcasecmp (convolvedWord, "false"))) {
            psAbort ("PSF_CONVOLVED entry invalid or missing for EXTENDED_SOURCE_MODEL entry %s", item->name);
        }
        bool convolved = !strcasecmp (convolvedWord, "true");
        psAssert (convolved, "EXTENDED SOURCE model %s is not convolved. This code is not ready for that\n", item->name);
    }
    psFree (iter);

    // setup the PSF fit radius details
    psphotInitRadiusPSF (recipe, readout);
    // and for extended source fitting
    psphotInitRadiusEXT (recipe, readout);

    // not really fitting but need to get some things set up to instantiate the pcm models

    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
    fitOptions->mode = PM_SOURCE_FIT_EXT_AND_SKY; 
    fitOptions->covarFactor    = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix
    // XXX: change all of this status checking to asserts. All should be defined with the current recipe.
    float fitNsigmaConv = psMetadataLookupF32 (&status, recipe, "EXT_FIT_NSIGMA_CONV"); // number of sigma for the convolution
    if (!status || !isfinite(fitNsigmaConv) || fitNsigmaConv <= 0) {
            fitNsigmaConv = 5.0;
    }
    fitOptions->nsigma         = fitNsigmaConv;
    // Poisson or Constant weight for chisq tests?
    fitOptions->poissonErrors = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS_POISSON");
    if (!status) fitOptions->poissonErrors = true;

    int psfSize  = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");
    assert (status);
    
    // Remember these data for later
    psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PCM_FIT_OPTIONS", PS_DATA_UNKNOWN | PS_META_REPLACE, "pcm fit options", fitOptions);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        source->imageID = index;    // XXX: This is usually set in psphotSourceStatsReadout
        pmModel *modelPSF = source->modelPSF;
        // free any previous radial aperture measurements
        psFree(source->radialAper);

        // Guess Models ususally does this
        if (modelPSF) {
            psphotCheckRadiusPSF (readout, source,  modelPSF, markVal);
            source->modelPSF->residuals = psf->residuals;
        }

        if (!(source->mode & PM_SOURCE_MODE_PSFMODEL)) {
            // The cmf loaders unconditionallly create psf models even if the source did not have one.
            // fprintf(stderr, "source %d %5d should not have a psf model 0x%08X%08X\n", index, source->seq, source->mode2, source->mode);
            psFree(source->modelPSF) ;
            modelPSF = NULL;
            // This bit was set once even though there was no model
            source->mode &= ~(PM_SOURCE_MODE_PSFSTAR);
            // At least once I saw a source with an extended model but no psf model
            psFree(source->modelEXT);
            source->modelEXT = NULL;
        }

        // make sure that the window radius is large enough.
        // Should we do this regardless of whether or not the source is extended?
        float fitRadius = NAN;
        float windowRadius = NAN;
        if (!psphotSetRadiusMoments (&fitRadius, &windowRadius, readout, source, markVal)) {
            psError (PSPHOT_ERR_UNKNOWN, false, "failed to set radius for ext source");
            return false;
        }
        // We are getting some sources near the edge of skycells which are marked as having
        // been fit with saturated star model but have peak coordinates far outside the
        // image. When this happens an error is left on the stack.
        psErrorClear();
        
        // If we find a good model we will cache it below.
        bool goodModel = false;

        // the cmf readers do not set PM_PAR_I0 for the models so we calculate it here.
        // We may want to put this code in the cmf readers but for now I don't want to change psphot operation
        // for anything except psphotStack -updatemode.
        if (modelPSF && isfinite(source->psfFlux)) {
            // The cmf reader was incorrectly setting the sky parameter for psf models
            modelPSF->params->data.F32[PM_PAR_SKY]  = 0.0;
            modelPSF->dparams->data.F32[PM_PAR_SKY] = 0.0;

            // Calculate flux for normalized model
            modelPSF->params->data.F32[PM_PAR_I0] = 1.0;
            float normFlux = modelPSF->class->modelFlux(modelPSF->params);
            float psfFlux = source->psfFlux;
            // back out the ApTrend
            double apTrend = pmTrend2DEval (psf->ApTrend, (float)source->peak->x, (float)source->peak->y);
            double adjustment = pow(10.0, -0.4*apTrend);
            psAssert ( (isfinite(adjustment) && adjustment != 0.0), "Invalid adjustment value: %f", adjustment);

            psfFlux = psfFlux / adjustment;
            float newI0 =  psfFlux / normFlux;

            psAssert(isfinite (newI0) , "got infinite I0 for psf model");

            // new I0 for psf model looks good. 
            modelPSF->params->data.F32[PM_PAR_I0] = newI0;
            goodModel = true;
        }

        // It is rather expensive setting the I0 all of the extended models. Lets only initialize
        // the extended model if it will be used for subtraction.

        bool isPSF = false;
        pmModel *model = pmSourceGetModel (&isPSF, source);
        if (model && !isPSF) {
            // selected model is extended
            goodModel = false;
            // calculate flux from the previously measured model magnitude. It will be nan if the
            // flux was negative unfortunately so we lose those.
            if (isfinite(model->mag)) {
                float modelFlux = pow(10., -0.4 * model->mag);

                model->fitRadius = fitRadius;

                // XXX: We are assuming here that we only use PCM models. 
                // I Should be getting that information from the recipe.
                pmPCMdata *pcm = pmPCMinit (source, fitOptions, model, maskVal, psfSize);
                if (pcm) {
                    model->params->data.F32[PM_PAR_I0] = 1.0;
                    float normFlux = model->class->modelFlux(model->params);
                    float I0 = modelFlux / normFlux;
                    if (isfinite(I0)) {
                        model->params->data.F32[PM_PAR_I0] = I0;
                        goodModel = true;
                        // Usually this it is set when doing the fit. Since we are instantiating the model we
                        // set it explicitly here. 
                        model->isPCM = true;
                    }
                    psFree(pcm);
                } else {
                    // psError (PSPHOT_ERR_UNKNOWN, false, "failed to to initialize PCM for model");
                    // return false;
                    // fall through
                }
            }
            if (!goodModel) {
                // We do not have a usable extended model for this source.
                // The original flux was probably negative.
                // This is somewhat rare. I saw it in about .2% of extended sources in my first test.
                // Set source type to star to prevent source subtraction from crashing by trying to use
                // an incomplete extended model.
                // XXX: this may cause model psf to be used in places.
                // XXX However we aren't going to cache it below so...
                source->type = PM_SOURCE_TYPE_STAR;
                psFree(source->modelEXT);
                source->modelEXT = NULL;
                model = NULL;
            }
        }

        if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
            // XXX: hacking here. Psf fitting can't find any stars unless moments->nPixels is above
            // some minimum. We don't measure the moments and this value is not saved in the cmf.
            // Is this a reasonable guess?
            source->moments->nPixels = source->modelPSF->nPix;
            source->tmpFlags |= PM_SOURCE_TMPF_CANDIDATE_PSFSTAR;
        }

        // psphotDeblendSatstars needs a value for signal to noise. 
        psF32 kronFlux = source->moments->KronFlux;
        source->moments->SN = 0;
        if (isfinite(kronFlux) && isfinite(source->moments->KronFluxErr) && isfinite(source->psfMag)) {
            // This is how we set the SN column in the CFF files...
            source->moments->SN = kronFlux/source->moments->KronFluxErr;
        } else if (isfinite(source->psfFlux) && isfinite(source->psfFluxErr)) {
            // kron no good try this. It's just used for sorting
            source->moments->SN = source->psfFlux/source->psfFluxErr;
        }

        if (goodModel && model) {
            // Yipee! cache the model
            if (model->isPCM) {
                pmPCMCacheModel (source, maskVal, psfSize, fitNsigmaConv);
            } else {
                pmSourceCacheModel (source, maskVal);
            }
        }
    }

    psFree(fitOptions);

    return true;
}

static bool psphotStackUpdateLoadSources (pmConfig *config, const pmFPAview *view, const char *filerule) {
    bool status = true;

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image
        if (!psphotStackUpdateLoadSourcesReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on to guess models for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;

}

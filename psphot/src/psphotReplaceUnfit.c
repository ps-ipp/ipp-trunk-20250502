# include "psphotInternal.h"

// replace the flux for sources which failed
bool psphotReplaceUnfitSources (psArray *sources, psImageMaskType maskVal) {

    pmSource *source;

    psTimerStart ("psphot.replace");

    for (int i = 0; i < sources->n; i++) {
	source = sources->data[i];

	// replace other sources?
	if (source->mode & PM_SOURCE_MODE_FAIL) goto replace;
	continue;

    replace:
        pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
    }
    psLogMsg ("psphot.replace", 3, "replace unfitted models: %f sec (%ld objects)\n", psTimerMark ("psphot.replace"), sources->n);
    return true;
}

// for now, let's store the detections on the readout->analysis for each readout
bool psphotReplaceAllSources (pmConfig *config, const pmFPAview *view, const char *filerule, bool ignoreState)
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        // ignore return value. False means "sources not subtracted", not a failure condition
	(void) psphotReplaceAllSourcesReadout (config, view, filerule, i, recipe, ignoreState);
    }
    return true;
}

// the return state indicates if any sources were actually replaced
bool psphotReplaceAllSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filename, int index, psMetadata *recipe, bool ignoreState) {

    bool status;

    psTimerStart ("psphot.replace");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filename, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    if (psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
        psLogMsg ("psphot", PS_LOG_DETAIL, "skipping replace all sources for input file %d", index);
        return false;
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    // if no work to do, should just return false
    if (!sources) return false;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    int NpixTotal = 0;

    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];

	if (ignoreState) {
	    // rely on the type of source to decide if we subtract it or not

	    // skip non-astronomical objects (very likely defects)
	    if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	    if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
      
	    // do not include CRs in the full ensemble fit
	    if (source->mode & PM_SOURCE_MODE_CR_LIMIT) continue;
	
	    // do not include MOMENTS_FAILURES in the fit
	    if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;
	} else {
	    // if we respect the state, do not replace unsubtracted sources
	    if (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) continue;
	}

	pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	NpixTotal += source->pixels->numCols * source->pixels->numRows;
    }

    psphotVisualShowImage(readout);
    psLogMsg ("psphot.replace", PS_LOG_INFO, "replaced models for %ld objects: %f sec (%d pixels)\n", sources->n, psTimerMark ("psphot.replace"), NpixTotal);
    return true;
}

// for now, let's store the detections on the readout->analysis for each readout
bool psphotRemoveAllSources (pmConfig *config, const pmFPAview *view, const char *filerule, bool ignoreState)
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotRemoveAllSourcesReadout (config, view, filerule, i, recipe, ignoreState)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to replace all sources for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotRemoveAllSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filename, int index, psMetadata *recipe, bool ignoreState) {

    bool status;

    psTimerStart ("psphot.replace");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filename, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    //psAssert (sources, "missing sources?");
    if (!sources) return true;
    
    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];

	if (ignoreState) {
	    // rely on the type of source to decide if we subtract it or not

	    // skip non-astronomical objects (very likely defects)
	    if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	    if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
      
	    // do not include CRs in the full ensemble fit
	    if (source->mode & PM_SOURCE_MODE_CR_LIMIT) continue;
	
	    // do not include MOMENTS_FAILURES in the fit
	    if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;
	} else {
	    // if we respect the state, only remove unsubtracted sources
	    if ((source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) continue;
	}

	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }

    psphotVisualShowImage(readout);
    psLogMsg ("psphot.replace", PS_LOG_INFO, "replaced models for %ld objects: %f sec\n", sources->n, psTimerMark ("psphot.replace"));
    return true;
}

bool psphotRemoveAllSourcesByArray (const psArray *sources, const psMetadata *recipe) {

    bool status;
    pmSource *source;

    psTimerStart ("psphot.replace");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    for (int i = 0; i < sources->n; i++) {
	source = sources->data[i];

	// replace other sources?
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) continue;

	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }
    psLogMsg ("psphot.replace", PS_LOG_INFO, "replaced models for %ld objects: %f sec\n", sources->n, psTimerMark ("psphot.replace"));
    return true;
}

// modify the sources to point at the corresponding pixels for the given filerule
bool psphotRedefinePixels (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotRedefinePixelsReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to replace all sources for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotRedefinePixelsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;
    pmSource *source;

    psTimerStart ("psphot.replace");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // XXX the sources have already been copied (merge into here?)
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");

    for (int i = 0; i < sources->n; i++) {
	source = sources->data[i];

	// sources have not yet been subtracted in this image (but this flag may be raised)
	source->tmpFlags &= ~PM_SOURCE_TMPF_SUBTRACTED;
	if (!source->modelPSF) continue;

	float Xo = source->modelPSF->params->data.F32[PM_PAR_XPOS];
	float Yo = source->modelPSF->params->data.F32[PM_PAR_YPOS];
	float radius = source->modelPSF->fitRadius;

	// force a redefine to this image
	pmSourceFreePixels(source);
	pmSourceRedefinePixels (source, readout, Xo, Yo, radius);
    }
    return true;
}

// for now, let's store the detections on the readout->analysis for each readout
bool psphotResetModels (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotResetModelsReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to replace all sources for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotResetModelsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;
    pmSource *source;

    psTimerStart ("psphot.replace");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    if (psMetadataLookupBool (&status, readout->analysis, "PSPHOT.SKIP.INPUT")) {
        psLogMsg ("psphot", PS_LOG_DETAIL, "skipping reset models for input file %d", index);
        return true;
    }

    pmSourceFitOptions *fitOptions = NULL;
    if (psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS")) {
        fitOptions = psMetadataLookupPtr (&status, readout->analysis, "PCM_FIT_OPTIONS");
        psAssert (fitOptions, "missing pcm fit options");
    }

    // XXX the sources have already been copied (merge into here?)
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");

    // what fraction of the PSF is used? (radius in pixels : 2 -> 5x5 box)
    int psfSize = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");
    assert (status);

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    for (int i = 0; i < sources->n; i++) {
	source = sources->data[i];

	// *** we need to cache the 'best' model, and we have 3 cases:
	// 1) model is the psf model --> generate from the new psf
	// 2) model is an unconvolved extended model --> just cache the copy (not perfect)
	// 3) model is a convolved extended model --> re-generate

	// use the 'best' model to cache the model (PSF or EXT : EXT may point at one of modelFits
	bool isPSF = false;
	pmModel *model = pmSourceGetModel(&isPSF, source);
	if (!model) continue;

	float radius = model->fitRadius; // save for future use below

	// regenerate the PSF if the model is a PSF, or if we need the PSF for a PCM
	if (isPSF || model->isPCM) {
            if (!source->modelPSF) continue;
	    // the guess central intensity comes from the peak:
	    float Io = source->peak->rawFlux;
	    float Xo = source->modelPSF->params->data.F32[PM_PAR_XPOS];
	    float Yo = source->modelPSF->params->data.F32[PM_PAR_YPOS];

	    // generate a model for this object with Io = 1.0
	    pmModel *modelPSF = pmModelFromPSFforXY(psf, Xo, Yo, Io);
	    if (modelPSF == NULL) {
		psWarning ("Failed to determine PSF model for source at (%f,%f), skipping", Xo, Yo);
		continue;
	    }

	    // set the source PSF model
	    psFree (source->modelPSF);
	    source->modelPSF = modelPSF;
	    source->modelPSF->fitRadius = radius;
	    //  model = source->modelPSF;
	}

	if (model->isPCM) {
            pmPCMdata *pcm = pmPCMinit (source, fitOptions, model, maskVal, psfSize);
            if (pcm) {
                pmPCMCacheModel (source, maskVal, psfSize, pcm->nsigma);
                psFree(pcm);
            } else {
                psFree(source->modelEXT);
                source->modelEXT = NULL;
            }
	  
	} else {
            pmSourceCacheModel (source, maskVal);  // ALLOC x14 (!)
        }
    }

    psLogMsg ("psphot.replace", PS_LOG_INFO, "subtracted models for %ld objects: %f sec\n", sources->n, psTimerMark ("psphot.replace"));
    return true;
}


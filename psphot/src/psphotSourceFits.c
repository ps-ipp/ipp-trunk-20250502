# include "psphotInternal.h"

// given a source with an existing modelPSF, attempt a full PSF fit, subtract if successful

static int NfitBlend = 0;

static int NfitPSF = 0;
static int NfitIterPSF = 0;

static int NfitDBL = 0;
static int NfitIterDBL = 0;
static int NfitPixDBL = 0;

static int NfitEXT = 0;
static int NfitIterEXT = 0;
static int NfitPixEXT = 0;

static int NfitPCM = 0;
static int NfitIterPCM = 0;
static int NfitPixPCM = 0;

bool psphotPCMfitCheckSize (pmPCMdata *pcm, pmSource *source, psImageMaskType maskVal, float psfSize);
bool psphotPCMfitRetry (pmPCMdata *pcm, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, float psfSize);

bool psphotFitInit (int nThreads) {
    psTimerStart ("psphot.fits");
    pmSourceFitSetInit (nThreads);
    return true;
}

bool psphotFitSummary () {

    psLogMsg ("psphot.pspsf", PS_LOG_INFO, "fitted %5d psf (%d iter), %5d blend: %5d ext (%d iter, %d pix), %5d dbl (%d iter, %d pix) : %6.2f sec\n",
	      NfitPSF, NfitIterPSF, NfitBlend, NfitEXT, NfitIterEXT, NfitPixEXT, NfitDBL, NfitIterDBL, NfitPixDBL, psTimerMark ("psphot.fits"));
    return true;
}

bool psphotFitSummaryExtended () {

    psLogMsg ("psphot.pspsf", PS_LOG_INFO, "fitted %5d ext (%d iter, %d pix), %5d pcm (%d iter, %d pix)\n",
	      NfitEXT, NfitIterEXT, NfitPixEXT, NfitPCM, NfitIterPCM, NfitPixPCM);
    return true;
}

bool psphotFitInitExtended () {
    NfitEXT = 0;
    NfitIterEXT = 0;
    NfitPixEXT = 0;
    NfitPCM = 0;
    NfitIterPCM = 0;
    NfitPixPCM = 0;
    return true;
}

bool psphotFitBlend (pmReadout *readout, pmSource *source, pmPSF *psf, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal) {

    float x, y, dR;

    pmSourceFitOptions options = *fitOptions;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // if this source is not a possible blend, just fit as PSF
    if ((source->blends == NULL) || (source->mode & PM_SOURCE_MODE_SATSTAR)) {
        bool status = psphotFitPSF (readout, source, psf, fitOptions, maskVal, markVal);
        return status;
    }

    // save the PSF model from the Ensemble fit
    pmModel *PSF = pmModelCopy (source->modelPSF);

    if (isnan(PSF->params->data.F32[PM_PAR_I0])) psAbort("nan in blend fit primary");

    x = PSF->params->data.F32[PM_PAR_XPOS];
    y = PSF->params->data.F32[PM_PAR_YPOS];

    psArray *modelSet = psArrayAllocEmpty (source->blends->n + 1);
    psArrayAdd (modelSet, 16, PSF);

    psArray *sourceSet = psArrayAllocEmpty (source->blends->n + 1);
    psArrayAdd (sourceSet, 16, source);

    psTrace ("psphot", 4, "fitting blended source at %f %f : %f\n", source->peak->xf, source->peak->yf, source->peak->rawFlux);

    // we need to include all blends in the fit (unless primary is saturated?)
    dR = 0;
    for (int i = 0; i < source->blends->n; i++) {
        pmSource *blend = source->blends->data[i];

        // find the blend which is furthest from source
        dR = PS_MAX (dR, hypot (blend->peak->xf - x, blend->peak->yf - y));

        // create the model and guess parameters for this blend
        pmModel *model = pmModelAlloc (PSF->type);
        for (int j = 0; j < model->params->n; j++) {
            model->params->data.F32[j] = PSF->params->data.F32[j];
            model->dparams->data.F32[j] = PSF->dparams->data.F32[j];
        }

        // XXX assume local sky is 0.0?
        model->params->data.F32[PM_PAR_I0] = blend->peak->rawFlux;
        model->params->data.F32[PM_PAR_XPOS] = blend->peak->xf;
        model->params->data.F32[PM_PAR_YPOS] = blend->peak->yf;

        // these should never be invalid values
        // XXX drop these tests eventually
        if (isnan(model->params->data.F32[PM_PAR_I0]))   psAbort("nan in blend fit");
        if (isnan(model->params->data.F32[PM_PAR_XPOS])) psAbort("nan in blend fit");
        if (isnan(model->params->data.F32[PM_PAR_YPOS])) psAbort("nan in blend fit");

        // add this blend to the list
        psArrayAdd (modelSet, 16, model);
        psArrayAdd (sourceSet, 16, blend);

	psTrace ("psphot", 5, "adding source at %f %f : %f\n", blend->peak->xf, blend->peak->yf, blend->peak->rawFlux);

        // free to avoid double counting model
        psFree (model);
    }

    // extend source radius as needed
    psphotCheckRadiusPSFBlend (readout, source, PSF, markVal, dR);

    // fit PSF model
    options.mode = PM_SOURCE_FIT_PSF;
    pmSourceFitSet (source, modelSet, &options, maskVal);

    // clear the circular mask
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); 

    if (!isfinite(PSF->params->data.F32[PM_PAR_I0])) psAbort("nan in fit");

    // evaluate the blend objects, subtract if good, free otherwise
    for (int i = 1; i < modelSet->n; i++) {
        pmSource *blend = sourceSet->data[i];
        pmModel *model  = modelSet->data[i];

	if (!isfinite(model->params->data.F32[PM_PAR_I0])) psAbort("nan in fit");

        // if this one failed, skip it
        if (!psphotEvalPSF (blend, model)) {
            psTrace ("psphot", 4, "failed on blend %d of %ld\n", i, modelSet->n);
            continue;
        }

        // otherwise, supply the resulting model to the corresponding blend
        psFree(blend->modelPSF);
        blend->modelPSF = psMemIncrRefCounter (model);
        psTrace ("psphot", 5, "fitted blend as PSF\n");

        // build cached model and subtract
        pmSourceCacheModel (blend, maskVal);
        pmSourceSub (blend, PM_MODEL_OP_FULL, maskVal);
        blend->mode |=  PM_SOURCE_MODE_BLEND_FIT;
    }
    NfitBlend += modelSet->n;

    // evaluate the primary object
    if (!psphotEvalPSF (source, PSF)) {
        psTrace ("psphot", 4, "failed on blend 0 of %ld\n", modelSet->n);
        psFree (PSF);
        psFree (modelSet);
        psFree (sourceSet);
        return false;
    }
    psFree (modelSet);
    psFree (sourceSet);

    // save the new, successful model
    psFree (source->modelPSF);
    source->modelPSF = PSF;
    psTrace ("psphot", 5, "fitted primary as PSF\n");

    // build cached model and subtract
    pmSourceCacheModel (source, maskVal);
    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    source->mode |=  PM_SOURCE_MODE_BLEND_FIT;
    source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;
    return true;
}

bool psphotFitPSF (pmReadout *readout, pmSource *source, pmPSF *psf, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal) {

    pmSourceFitOptions options = *fitOptions;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    NfitPSF ++;

    // save the PSF model from the Ensemble fit
    pmModel *PSF = pmModelCopy (source->modelPSF);
    if (isnan(PSF->params->data.F32[1])) psAbort("nan in psf fit");

    // extend source radius as needed
    psphotCheckRadiusPSF (readout, source, PSF, markVal);

    // fit PSF model (set/unset the pixel mask)
    options.mode = PM_SOURCE_FIT_PSF;
    pmSourceFitModel (source, PSF, &options, maskVal);
    NfitIterPSF += PSF->nIter;

    if (!isfinite(PSF->params->data.F32[PM_PAR_I0])) psAbort("nan in fit");

    // clear the circular mask
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); 

    // does the PSF model succeed?
    if (!psphotEvalPSF (source, PSF)) {
        psFree (PSF);
        return false;
    }

    // free old model, save new model
    psFree (source->modelPSF);
    source->modelPSF = PSF;
    psTrace ("psphot", 5, "fitted as PSF\n");

    // build cached model and subtract
    source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;
    pmSourceCacheModel (source, maskVal);
    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    return true;
}

// save a local, static copy of the EXT model type so we don't have to lookup for each object
static pmModelType modelTypeEXT;

bool psphotInitLimitsEXT (psMetadata *recipe, pmReadout *readout) {

    bool status;

    // extended source model descriptions
    char *modelNameEXT = psMetadataLookupStr (&status, recipe, "EXT_MODEL");
    modelTypeEXT = pmModelClassGetType (modelNameEXT);

    psphotInitRadiusEXT (recipe, readout);

    return true;
}

bool psphotFitBlob (pmReadout *readout, pmSource *source, psArray *newSources, pmPSF *psf, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal) {

    float fitRadius, windowRadius;
    bool okEXT, okDBL;
    pmModel *ONE = NULL;
    pmSource *tmpSrc = NULL;
    pmModel *EXT = NULL;
    psArray *DBL = NULL;
    // pmMoments psfMoments;

    // skip the source if we don't think it is extended
    // XXX are these robust, or do we have better info from the source-size analysis??
    if (source->type == PM_SOURCE_TYPE_UNKNOWN) return false;
    if (source->type == PM_SOURCE_TYPE_DEFECT) return false;
    if (source->type == PM_SOURCE_TYPE_SATURATED) return false;

    // set the radius based on the footprint (also sets the mask pixels)
    if (!psphotSetRadiusMoments(&fitRadius, &windowRadius, readout, source, markVal)) return false;
    // fprintf (stderr, "rad: %6.1f %6.1f  | %5.2f %5.2f %5.2f  ", source->peak->xf, source->peak->yf, source->moments->Mrf, fitRadius, windowRadius);

    psTrace ("psphot", 5, "trying blob...\n");

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    float chiEXT = NAN;
    float chiDBL = NAN;

    // this temporary source is used as a place-holder by the psphotEval functions below
    tmpSrc = pmSourceAlloc ();
    {
	// DBL will always be defined, but DBL->data[n] might not
	DBL = psphotFitDBL (readout, source, fitOptions, maskVal, markVal);
	if (!DBL) goto escape;
	if (!DBL->n) goto escape;

	okDBL  = psphotEvalDBL (tmpSrc, DBL->data[0]);
	okDBL &= psphotEvalDBL (tmpSrc, DBL->data[1]);
	okDBL = false; // XXX this is failing badly...
	// XXX should I keep / save the flags set in the eval functions?

	// correct first model chisqs for flux trend
	ONE = DBL->data[0];
	if (ONE) {
	    psAssert (isfinite(ONE->params->data.F32[PM_PAR_I0]), "nan in fit");
	    chiDBL = ONE->chisqNorm; // save chisq for double-star/galaxy comparison
	    ONE->fitRadius = fitRadius;
	}

	// correct second model chisqs for flux trend
	ONE = DBL->data[1];
	if (ONE) {
	    psAssert (isfinite(ONE->params->data.F32[PM_PAR_I0]), "nan in fit");
	    ONE->fitRadius = fitRadius;
	}
    }

    { 
	// XXX need to handle failures better here
	EXT = psphotFitEXT (NULL, readout, source, fitOptions, modelTypeEXT, maskVal, markVal);
	if (!EXT) goto escape;
	if (!isfinite(EXT->params->data.F32[PM_PAR_I0])) goto escape;

	okEXT = psphotEvalEXT (tmpSrc, EXT);
	chiEXT = EXT ? EXT->chisqNorm : NAN;
	EXT->fitRadius = fitRadius;
    }

    // clear the circular mask
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); 

    psFree (tmpSrc);

    // (void) psTraceSetLevel("psModules.objects.pmSourceFitSet", 0);

    if (okEXT && okDBL) {
        // XXX EAM : a bogus bias: need to examine this better
        if (3*chiEXT > chiDBL) goto keepDBL;
        goto keepEXT;
    }

    if (okEXT && !okDBL) goto keepEXT;
    if (!okEXT && okDBL) goto keepDBL;

    psTrace ("psphot", 4, "both failed: blob chisq: %f vs %f for %f,%f\n", chiEXT, chiDBL, source->peak->xf, source->peak->yf);

    // both models failed; reject them both
    // XXX -- change type flags to psf in this case, and make sure we subtract it?
    // reset the psf moments
    // XXX *source->moments = psfMoments;

    psFree (EXT);
    psFree (DBL);
    return false;

keepEXT:
    psTrace ("psphot", 4, "goto EXT : blob chisq: %f vs %f for %f,%f\n", chiEXT, chiDBL, source->peak->xf, source->peak->yf);
    // sub EXT
    psFree (DBL);

    // save new model
    // XXX save the correct radius...
    source->modelEXT = EXT;
    source->type = PM_SOURCE_TYPE_EXTENDED;
    source->mode |= PM_SOURCE_MODE_EXTMODEL;
    source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;

    // build cached model and subtract
    pmSourceCacheModel (source, maskVal);
    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

# if (PS_TRACE_ON)   
    psTrace ("psphot", 5, "blob as EXT: %f %f\n", EXT->params->data.F32[PM_PAR_XPOS], EXT->params->data.F32[PM_PAR_YPOS]);
    if (psTraceGetLevel("psphot") >= 6) {
	psLogMsg ("psphot", 1, "source 2:\n");
	for (int i = 0; i < source->modelEXT->params->n; i++) {
	    psLogMsg ("psphot", 1, "PAR %d : %f +/- %f\n", i, source->modelEXT->params->data.F32[i], source->modelEXT->dparams->data.F32[i]);
	}
    }
# endif

    // reset the psf moments
    // XXX *source->moments = psfMoments;
    return true;

keepDBL:
    psTrace ("psphot", 4, "goto DBL : blob chisq: %f vs %f for %f,%f\n", chiEXT, chiDBL, source->peak->xf, source->peak->yf);
    // sub DLB
    psFree (EXT);

    // drop old model, save new second model...
    psFree (source->modelPSF);
    source->modelPSF = psMemIncrRefCounter (DBL->data[0]);
    source->mode     |= PM_SOURCE_MODE_PAIR;
    source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;

    // copy most data from the primary source (modelEXT, blends stay NULL)
    pmSource *newSrc = pmSourceCopy (source);
    newSrc->modelPSF = psMemIncrRefCounter (DBL->data[1]);
    newSrc->peak->footprint = source->peak->footprint; // just a reference; the peak does not own the footprint
    psArrayAdd(newSrc->peak->footprint->peaks, 1, newSrc->peak); // the footprint owns the peak

    // build cached models and subtract
    pmSourceCacheModel (source, maskVal);
    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    pmSourceCacheModel (newSrc, maskVal);
    pmSourceSub (newSrc, PM_MODEL_OP_FULL, maskVal);

# if (PS_TRACE_ON)   
    psTrace ("psphot", 5, "blob as DBL: %f %f\n", ONE->params->data.F32[PM_PAR_XPOS], ONE->params->data.F32[PM_PAR_YPOS]);
    if (psTraceGetLevel("psphot") >= 6) {
	psLogMsg ("psphot", 1, "source 1:\n");
	for (int i = 0; i < newSrc->modelPSF->params->n; i++) {
	    psLogMsg ("psphot", 1, "PAR %d : %f +/- %f\n", i, newSrc->modelPSF->params->data.F32[i], newSrc->modelPSF->dparams->data.F32[i]);
	}
	psLogMsg ("psphot", 1, "source 2:\n");
	for (int i = 0; i < source->modelPSF->params->n; i++) {
	    psLogMsg ("psphot", 1, "PAR %d : %f +/- %f\n", i, source->modelPSF->params->data.F32[i], source->modelPSF->dparams->data.F32[i]);
	}
    }
# endif

    psArrayAdd (newSources, 100, newSrc);
    psFree (newSrc);
    psFree (DBL);
    return true;

escape:
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); 
    psFree (tmpSrc);
    psFree (EXT);
    psFree (DBL);
    return false;
}

// fit a double PSF source to an extended blob
psArray *psphotFitDBL (pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal) {

    float dx, dy;
    pmModel *DBL;
    pmModel *PSF;
    psEllipseAxes axes;
    psEllipseMoments moments;
    psArray *modelSet;

    pmSourceFitOptions options = *fitOptions;

    NfitDBL ++;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // XXX this is really poor: if we don't have moments for the source, we have no guess.
    // force the measurement?
    psAssert (source->moments, "moments are re-calculated for any extended source");
	
    // make a guess at the position of the two sources
    moments.x2 = source->moments->Mxx;
    moments.xy = source->moments->Mxy;
    moments.y2 = source->moments->Myy;
    axes = psEllipseMomentsToAxes (moments, 20.0);

    if (isnan(axes.major)) return NULL;
    if (isnan(axes.minor)) return NULL;
    if (isnan(axes.theta)) return NULL;

    // XXX this is really arbitrary: 4 pixel separation?
    dx = 2 * cos (axes.theta);
    dy = 2 * sin (axes.theta);

    // save the PSF model from the Ensemble fit
    PSF = source->modelPSF;
    if (isnan(PSF->params->data.F32[1])) psAbort("nan in dbl fit");

    modelSet = psArrayAlloc (2);

    DBL = pmModelCopy (PSF);
    DBL->params->data.F32[PM_PAR_I0]  *= 0.5;
    DBL->params->data.F32[PM_PAR_XPOS] = source->peak->xf + dx;
    DBL->params->data.F32[PM_PAR_YPOS] = source->peak->yf + dy;
    modelSet->data[0] = DBL;

    DBL = pmModelCopy (PSF);
    DBL->params->data.F32[PM_PAR_I0]  *= 0.5;
    DBL->params->data.F32[PM_PAR_XPOS] = source->peak->xf - dx;
    DBL->params->data.F32[PM_PAR_YPOS] = source->peak->yf - dy;
    modelSet->data[1] = DBL;

    // fit PSF model (set/unset the pixel mask)
    options.mode = PM_SOURCE_FIT_PSF;
    pmSourceFitSet (source, modelSet, &options, maskVal);
    NfitIterDBL += DBL->nIter;
    NfitPixDBL += DBL->nDOF;

    return (modelSet);
}

pmModel *psphotFitEXT (pmModel *guessModel, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal) {

    if ((source->moments->Mxx < 1e-3) || (source->moments->Myy < 1e-3)) {
        psTrace ("psphot", 5, "problem source: moments: %f %f\n", source->moments->Mxx, source->moments->Myy);
    }

    pmSourceFitOptions options = *fitOptions;

    NfitEXT ++;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // use the source moments, etc to guess basic model parameters
    pmModel *model = guessModel ? guessModel : pmSourceModelGuess (source, modelType, maskVal, markVal);
    if (!model) {
	psTrace ("psphot", 5, "failed to generate a model for source: moments: %f %f\n", source->moments->Mxx, source->moments->Myy);
	return NULL;
    }

# if (PS_TRACE_ON) 
    if ( (source->peak->xf > 5500) &&
	 (source->peak->yf < 500) &&
	 psTraceGetLevel ("psphot") >= 6) {

      // Moments-based shapes parameters
      psEllipseMoments moments;
      moments.x2 = source->moments->Mxx;
      moments.xy = source->moments->Mxy;
      moments.y2 = source->moments->Myy;
      // force the axis ratio to be < 20.0
      psEllipseAxes axes = psEllipseMomentsToAxes (moments, 20.0);

      // PSF shape parameters
      psEllipseShape psfShape;
      psfShape.sx  = source->modelPSF->params->data.F32[PM_PAR_SXX] / M_SQRT2;
      psfShape.sxy = source->modelPSF->params->data.F32[PM_PAR_SXY];
      psfShape.sy  = source->modelPSF->params->data.F32[PM_PAR_SYY] / M_SQRT2;
      psEllipseAxes psfAxes = psEllipseShapeToAxes (psfShape, 20.0);

      fprintf (stderr, "--- guess values ---\n");
      fprintf (stderr, "(x,y): %f, %f  Mxx: %f, Myy: %f, Mxy: %f -> major: %f, minor: %f, theta: %f (%f deg)\n", source->peak->xf, source->peak->yf, source->moments->Mxx, source->moments->Myy, source->moments->Mxy, axes.major, axes.minor, axes.theta, axes.theta*PS_DEG_RAD);
      fprintf (stderr, "psf: major: %f, minor: %f, theta: %f (%f deg)\n", psfAxes.major, psfAxes.minor, psfAxes.theta, psfAxes.theta*PS_DEG_RAD);
      for (int i = 0; i < model->params->n; i++) {
	fprintf (stderr, "par %d: %f\n", i, model->params->data.F32[i]);
      }
    }
    if (psTraceGetLevel ("psphot") >= 7) {
      // psTraceSetLevel("psLib.math.psMinimizeLMChi2", 5);
    }
# endif

    // for sersic models, use a grid search to choose an index, then float the params there
    if (modelType == pmModelClassGetType("PS_MODEL_SERSIC")) {
    	psphotFitSersicIndex (model, readout, source, fitOptions, maskVal, markVal);
    }

    options.mode = PM_SOURCE_FIT_EXT;
    if (modelType == pmModelClassGetType("PS_MODEL_SERSIC")) {
	options.mode = PM_SOURCE_FIT_NO_INDEX;
    }
    if (modelType == pmModelClassGetType("PS_MODEL_TRAIL")) {
	options.mode = PM_SOURCE_FIT_TRAIL;
    }

    pmSourceFitModel (source, model, &options, maskVal);
    NfitIterEXT += model->nIter;
    NfitPixEXT += model->nDOF;

# if (PS_TRACE_ON) 
    if ((source->peak->xf > 5500) &&
	(source->peak->yf < 500) &&
	psTraceGetLevel ("psphot") >= 6) {
      fprintf (stderr, "chisq: %f, nIter: %d, radius: %f, npix: %d\n", model->chisqNorm, model->nIter, model->fitRadius, model->nPix);
      fprintf (stderr, "--- fitted values ---\n");
      for (int i = 0; i < model->params->n; i++) {
	fprintf (stderr, "par %d: %f\n", i, model->params->data.F32[i]);
      }
    }
    // psTraceSetLevel("psLib.math.psMinimizeLMChi2", 0);
# endif

    return (model);
}

# define TIMING 0
# define EXTRA_VERBOSE 0

bool psphotSersicModelGuessPCM (pmPCMdata *pcm, pmSource *source, psImageMaskType maskVal, float psfSize);
bool psphotFitSersicShapeAndIndex (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);
bool psphotFitSersicShapeAndIndexGrid (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);
bool psphotFitSersicShapeAndIndexGridAuto (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);

pmModel *psphotFitPCM (pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {

    if ((source->moments->Mxx < 1e-3) || (source->moments->Myy < 1e-3)) {
        psTrace ("psphot", 5, "problem source: moments: %f %f\n", source->moments->Mxx, source->moments->Myy);
    }

    pmSourceFitOptions options = *fitOptions;

    NfitPCM ++;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // allocate the model (this can only fail on a config error)
    pmModel *model = pmModelAlloc(modelType);
    psAssert (model, "invalid extended model name");

    float t1, t2, t3, t4, t5;
    t1 = t2 = t3 = t4 = t5 = 0.0;
    if (TIMING) { psTimerStart ("psphotFitPCM"); }

    // if we are ever (in a given psphot implementation) going to fit a parameter, we must set the options here to include
    // that parameter (otherwise pmPCMupdate will fail to allocate the dmodelFlux image)
    // thus, if the sersic analysis below uses an index fit, need to use this EXT_AND_SKY mode for init

    options.mode = PM_SOURCE_FIT_EXT;
    if (modelType == pmModelClassGetType("PS_MODEL_SERSIC")) {
	options.mode = PM_SOURCE_FIT_EXT_AND_SKY;
    }
    if (modelType == pmModelClassGetType("PS_MODEL_DEV")) {
	options.mode = PM_SOURCE_FIT_SHAPE;
	options.mode = PM_SOURCE_FIT_EXT_AND_SKY;
    }
    if (modelType == pmModelClassGetType("PS_MODEL_EXP")) {
	options.mode = PM_SOURCE_FIT_EXT_AND_SKY;
    }

    pmPCMdata *pcm = pmPCMinit (source, &options, model, maskVal, psfSize);
    if (!pcm) {
	psTrace ("psphot", 5, "failed to generate a model for source: moments: %f %f\n", source->moments->Mxx, source->moments->Myy);
        model->flags |= PM_MODEL_STATUS_BADARGS; // XXX this is probably already set in pmPCMinit
	return model;
    }
    if (TIMING) { t1 = psTimerMark ("psphotFitPCM"); }

    // NOTE : 65 allocs to here
    // get the guess for sersic models 
    if (modelType == pmModelClassGetType("PS_MODEL_SERSIC")) {
	// use the source moments, etc to guess basic model parameters
	if (!psphotSersicModelGuessPCM (pcm, source, maskVal, psfSize)) {
	    psFree (pcm);
	    model->flags |= PM_MODEL_SERSIC_PCM_FAIL_GUESS;
	    return model;
	}
	if (TIMING) { t2 = psTimerMark ("psphotFitPCM"); }

	// psphotFitSersicShapeAndIndex (pcm, readout, source, fitOptions, maskVal, markVal, psfSize);
	options.mode = PM_SOURCE_FIT_NO_INDEX;
	if (!psphotFitSersicShapeAndIndexGridAuto (pcm, readout, source, &options, maskVal, markVal, psfSize)) {
	    psFree (pcm);
	    model->flags |= PM_MODEL_SERSIC_PCM_FAIL_GRID;
	    psError(PS_ERR_UNKNOWN, true, "Failed to find a index & shape");
	    psErrorClear (); // clear the polynomial error
	    return model;
	}
    } else {
	// use the source moments, etc to guess basic model parameters
	if (!pmSourceModelGuessPCM (pcm, source, maskVal, markVal)) {
	    psFree (pcm);
	    model->flags |= PM_MODEL_PCM_FAIL_GUESS;
	    return model;
	}
    }

    if (TIMING) { t3 = psTimerMark ("psphotFitPCM"); }

    // psTraceSetLevel("psLib.math.psMinimizeLMChi2", 5);
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);  // NOTE : 1687 allocs in here
    NfitIterPCM += pcm->modelConv->nIter;
    NfitPixPCM += pcm->modelConv->nDOF;
    if (TIMING) { t4 = psTimerMark ("psphotFitPCM"); }

    // XXX we might make this more efficient by setting NITER to be fairly small.  if we hit the iteration
    // limit, then we could do a small grid search on the size and try again from the best fit 

    if (options.isInteractive) psphotPCMfitCheckSize (pcm, source, maskVal, psfSize);
    // if (pcm->modelConv->nIter == fitOptions->nIter) {
    // 	psphotPCMfitRetry (pcm, source, &options, maskVal, markVal, psfSize);
    // }
    if (TIMING) { t5 = psTimerMark ("psphotFitPCM"); }

    if (TIMING) {
	int nPixBig = source->pixels->numCols * source->pixels->numRows;
    	fprintf (stderr, "psphotFitPCM : nIter: %2d, radius: %6.1f, npix: %5d of %5d, t1: %6.4f, t2: %6.4f, t3: %6.4f, t4: %6.4f, t5: %6.4f\n", model->nIter, model->fitRadius, model->nPix, nPixBig, t1, t2, t3, t4, t5);
    }
    if (EXTRA_VERBOSE && !TIMING) {
	int nPixBig = source->pixels->numCols * source->pixels->numRows;
	float *PAR = model->params->data.F32;
	fprintf (stderr, "source %d : %f - %f %f - %f %f %f - %f | nIter: %2d, radius: %6.1f, npix: %5d of %5d, chisq %f\n", source->id, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1], model->nIter, model->fitRadius, model->nPix, nPixBig, model->chisqNorm);
    }

    psFree (pcm);

    return model;
}

// note that these should be 1/2n of the standard sersic index
// float indexGuess[] = {0.8, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5, 6.0};
// float indexGuess[] = {0.5, 0.33, 0.25, 0.167, 0.125, 0.083};
float indexGuess[] = {1.0, 2.0, 3.0, 4.0};
# define N_INDEX_GUESS 4

// A sersic model is very sensitive to the index.  attempt to find the index first by grid search in just the index
// for a sersic model, attempt to fit just the index and normalization with a modest number of iterations
bool psphotFitSersicIndex (pmModel *model, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal) {

    assert (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));

    pmSourceFitOptions options = *fitOptions;
    
    // fit EXT (not PSF) model (set/unset the pixel mask)
    options.mode = PM_SOURCE_FIT_NO_INDEX;
    options.nIter = 4;

    int iMin = -1;
    float xMin = NAN;
    float chiSquare[N_INDEX_GUESS];

    for (int i = 0; i < N_INDEX_GUESS; i++) {
	model->params->data.F32[PM_PAR_7] = 0.5/indexGuess[i];

	if (!model->class->modelGuess(model, source, maskVal, markVal)) {
	    model->flags |= PM_MODEL_STATUS_BADARGS;
	    return false;
	}

	pmSourceFitModel (source, model, &options, maskVal);
	// fprintf (stderr, "index: %f, chisq: %f, nIter: %d, radius: %f, npix: %d\n", indexGuess[i], model->chisqNorm, model->nIter, model->fitRadius, model->nPix);

	chiSquare[i] = model->chisqNorm;
	if (i == 0) {
	    xMin = chiSquare[i];
	    iMin = i;
	} else {
	    if (chiSquare[i] < xMin) {
		xMin = chiSquare[i];
		iMin = i;
	    }
	}
    }
    assert (iMin >= 0);

    model->flags = PM_MODEL_STATUS_NONE; // do not attempt to handle failures here, let the next iteration deal with it
    model->params->data.F32[PM_PAR_7] = 0.5/indexGuess[iMin];
    model->class->modelGuess(model, source, maskVal, markVal);

    return true;
}

// A sersic model is very sensitive to the index.  attempt to find the index first by grid search in just the index
// for a sersic model, attempt to fit just the index and normalization with a modest number of iterations
bool psphotFitSersicIndexPCM (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {

    pmModel *model = pcm->modelConv;

    assert (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));

    pmSourceFitOptions options = *fitOptions;
    
    // fit EXT (not PSF) model (set/unset the pixel mask)
    options.mode = PM_SOURCE_FIT_NO_INDEX;
    options.nIter = 4;

    // update the pcm elements if we have changed the circumstance (here, options.mode)
    pmPCMupdate(pcm, source, &options, model);

    int iMin = -1;
    float xMin = NAN;
    float chiSquare[N_INDEX_GUESS];

    for (int i = 0; i < N_INDEX_GUESS; i++) {
	model->params->data.F32[PM_PAR_7] = indexGuess[i];
	
	if (!model->class->modelGuess(model, source, maskVal, markVal)) {
	    model->flags |= PM_MODEL_STATUS_BADARGS;
	    return false;
	}

# if (0)
	// this block is to test the relative speed of straight and PCM fits
	pmSourceFitModel (source, model, &options, maskVal);
# else
	pmSourceModelGuessPCM(pcm, source, maskVal, markVal);
	pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
# endif
	fprintf (stderr, "index: %f, chisq: %f, nIter: %d, radius: %f, npix: %d\n", indexGuess[i], model->chisqNorm, model->nIter, model->fitRadius, model->nPix);
	// fprintf (stderr, "chisq: %f, nIter: %d, radius: %f, npix: %d\n", model->chisqNorm, model->nIter, model->fitRadius, model->nPix);

	chiSquare[i] = model->chisq;
	if (i == 0) {
	    xMin = chiSquare[i];
	    iMin = i;
	} else {
	    if (chiSquare[i] < xMin) {
		xMin = chiSquare[i];
		iMin = i;
	    }
	}
    }
    assert (iMin >= 0);

    model->flags = PM_MODEL_STATUS_NONE; // do not attempt to handle failures here, let the next iteration deal with it
    model->params->data.F32[PM_PAR_7] = indexGuess[iMin];

    pmSourceModelGuessPCM(pcm, source, maskVal, markVal);

    return true;
}

// 0.5 / n for (1.0, 1.25, 1.66, 2.0, 3.33, 4.0)
// float indexGuessInv[] = {0.5, 0.4, 0.3, 0.25, 0.20, 0.15, 0.125};

// 0.5 / n for (0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0)
float indexGuessInv[] = {1.00, 0.50, 0.333, 0.25, 0.166, 0.125, 0.10, 0.0833};
float indexGuessR1q[] = {1.06, 1.19, 1.335, 1.48, 1.840, 2.290, 2.84, 3.5300};
# define N_INDEX_GUESS_INV 8

// we are going to guess in fractions about the R1-based guess
float reffGuess[] = {0.8, 0.9, 1.0, 1.12, 1.25};
# define N_REFF_GUESS 5

// A sersic model is very sensitive to the index.  attempt to find the index first by grid search in just the index
// for a sersic model, attempt to fit just the index and normalization with a modest number of iterations
bool psphotSersicModelGuessPCM (pmPCMdata *pcm, pmSource *source, psImageMaskType maskVal, float psfSize) {

    // we get a reasonable guess from:
    // * Reff = Kron R1 / Q(index) -- Q comes from Graham & Driver 
    // * Rmajor / Rminor & Theta from moments
    // * Io from total Kron flux

    // the guesses are used to fill in PAR:
    psF32 *PAR = pcm->modelConv->params->data.F32;

    // convert the moments to Major,Minor,Theta
    psEllipseMoments moments;

    if (!isfinite(source->moments->Mrf)) return false;
    if (!isfinite(source->moments->Mxx)) return false;
    if (!isfinite(source->moments->Mxy)) return false;
    if (!isfinite(source->moments->Myy)) return false;

    moments.x2 = source->moments->Mxx;
    moments.y2 = source->moments->Myy;
    moments.xy = source->moments->Mxy;
    
    // limit axis ratio < 20.0
    psEllipseAxes momentAxes = psEllipseMomentsToAxes (moments, 20.0);

    // set the model position
    if (!pmModelSetPosition(&PAR[PM_PAR_XPOS], &PAR[PM_PAR_YPOS], source)) {
      return false;
    }

    // sky is zero (no longer fitted, but not yet deprecated)
    PAR[PM_PAR_SKY]  = 0.0;

    // for the index loop, use Io = 1.0, use fitted values to determine Io
    PAR[PM_PAR_I0] = 1.0;

    float xMin = NAN;
    float iMin = NAN;
    float sMin = NAN;
    float rMin = NAN;

    // loop over index and Reff, keeping the ARatio and Theta constant?
    // loop over index guesses and find the best fit
    for (int j = 0; j < N_REFF_GUESS; j++) {
	for (int i = 0; i < N_INDEX_GUESS_INV; i++) {
	    PAR[PM_PAR_7] = indexGuessInv[i];

	    psEllipseAxes guessAxes;
	    guessAxes.major = reffGuess[j] * source->moments->Mrf / indexGuessR1q[i];
	    guessAxes.minor = guessAxes.major * (momentAxes.minor / momentAxes.major);
	    guessAxes.theta = momentAxes.theta;

	    if (!isfinite(guessAxes.major)) return false;
	    if (!isfinite(guessAxes.minor)) return false;
	    if (!isfinite(guessAxes.theta)) return false;

	    // convert the major,minor,theta to shape parameters for an Reff-like model
	    pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], guessAxes, true);

	    // generated the modelFlux
	    // XXX note that this does not add sky to model
	    pmPCMMakeModel (source, pcm->modelConv, pcm->nsigma, maskVal, psfSize);
	
	    float YY = 0.0;
	    float YM = 0.0;
	    float MM = 0.0;
	    bool usePoisson = false;

	    for (int iy = 0; iy < source->pixels->numRows; iy++) {
		for (int ix = 0; ix < source->pixels->numCols; ix++) {
		    // skip masked points
		    if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) {
			continue;
		    }
		    // skip zero-variance points
		    if (source->variance->data.F32[iy][ix] == 0) {
			continue;
		    }
		    // skip nan value points
		    if (!isfinite(source->pixels->data.F32[iy][ix])) {
			continue;
		    }

		    float fy = source->pixels->data.F32[iy][ix];
		    float fm = source->modelFlux->data.F32[iy][ix];
		    float wt = (usePoisson) ? 1.0 / source->variance->data.F32[iy][ix] : 1.0;

		    YY += PS_SQR(fy) * wt;
		    YM += fm * fy * wt;
		    MM += PS_SQR(fm) * wt;
		}
	    }

	    float Io = YM / MM;
	    float Chisq = YY - 2 * Io * YM + Io * Io * MM;
	    if (isnan(xMin) || (Chisq < xMin)) {
		xMin = Chisq;
		iMin = Io;
		sMin = indexGuessInv[i];
		rMin = reffGuess[j] / indexGuessR1q[i];
	    }
            if (EXTRA_VERBOSE) {
                fprintf (stderr, "%d | %f %f %f %f | %f %f %f %f", i, indexGuessInv[i], reffGuess[j], Io, Chisq, sMin, rMin, iMin, xMin);
                fprintf (stderr, "\n");
            }
	}
    }

    {
	psEllipseAxes guessAxes;
	guessAxes.major = rMin * source->moments->Mrf;
	guessAxes.minor = guessAxes.major * (momentAxes.minor / momentAxes.major);
	guessAxes.theta = momentAxes.theta;

	if (!isfinite(guessAxes.major)) return false;
	if (!isfinite(guessAxes.minor)) return false;
	if (!isfinite(guessAxes.theta)) return false;

	// convert the major,minor,theta to shape parameters for an Reff-like model
	pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], guessAxes, true);
    }

    PAR[PM_PAR_I0] = iMin;
    PAR[PM_PAR_7] = sMin;

    return true;
}

// we have a set of guess parameters, do a small number of iterations fitting only SHAPE then only INDEX 
bool psphotFitSersicShapeAndIndex (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {

    pmModel *model = pcm->modelConv;

    assert (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));

    pmSourceFitOptions options = *fitOptions;
    
    for (int i = 0; i < 3; i++) {
      // fit EXT (not PSF) model (set/unset the pixel mask)
      options.mode = PM_SOURCE_FIT_SHAPE;
      options.nIter = 2;

      // update the pcm elements if we have changed the circumstance (here, options.mode)
      pmPCMupdate(pcm, source, &options, model);
      
      pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) {
	float *PAR = model->params->data.F32;
	fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
      }
      
      // fit EXT (not PSF) model (set/unset the pixel mask)
      options.mode = PM_SOURCE_FIT_INDEX;
      // options.mode = PM_SOURCE_FIT_EXT_AND_SKY;
      options.nIter = 30;
      
      // update the pcm elements if we have changed the circumstance (here, options.mode)
      pmPCMupdate(pcm, source, &options, model);
      
      pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) {
	float *PAR = model->params->data.F32;
	fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
      }
    }

    // update the pcm elements if we have changed the circumstance (here, options.mode)
    pmPCMupdate(pcm, source, fitOptions, model);

    return true;
}

// we have a set of guess parameters, do a small number of iterations fitting only SHAPE then only INDEX 
bool psphotFitSersicShapeAndIndexGridAuto (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {

    pmModel *model = pcm->modelConv;

    assert (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));

    pmSourceFitOptions options = *fitOptions;
    
    psF32 *PAR = pcm->modelConv->params->data.F32;

    options.mode = PM_SOURCE_FIT_SHAPE;
    options.nIter = 7;
    
    // update the pcm elements if we have changed the circumstance (here, options.mode)
    pmPCMupdate(pcm, source, &options, model);
    
    // we have been provided a guess at the index (P[7]) from the list of indexGuessInv

    // find the matching indexGuessInv
    int nStart = -1;
    for (int i = 0; i < N_INDEX_GUESS_INV; i++) {
	if (fabs(PAR[PM_PAR_7] - indexGuessInv[i]) < 0.01) {
	    nStart = i;
	    break;
	}
    }
    if (nStart == -1) {
	fprintf (stderr, "WARNING: could not find start guess %f\n", PAR[PM_PAR_7]);
	return false;
    }

    psVector *chi2 = psVectorAllocEmpty (16, PS_TYPE_F32);
    psVector *Sidx = psVectorAllocEmpty (16, PS_TYPE_F32);

    float Sm = NAN, Sp = NAN, So = NAN;
    if (nStart == 0) {
	Sm = indexGuessInv[nStart];
	So = 0.5*(indexGuessInv[nStart + 1] + indexGuessInv[nStart]);
	Sp = indexGuessInv[nStart + 1];
    } else if (nStart == N_INDEX_GUESS_INV - 1) {
	Sp = indexGuessInv[nStart];
	So = 0.5*(indexGuessInv[nStart - 1] + indexGuessInv[nStart]);
	Sm = indexGuessInv[nStart - 1];
    } else {
	Sm = 0.5*(indexGuessInv[nStart - 1] + indexGuessInv[nStart]);
	So = indexGuessInv[nStart];
	Sp = 0.5*(indexGuessInv[nStart + 1] + indexGuessInv[nStart]);
    }
    
    PAR[PM_PAR_7] = Sm;
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
    psVectorAppend (chi2, model->chisqNorm);

    PAR[PM_PAR_7] = So;
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
    psVectorAppend (chi2, model->chisqNorm);

    PAR[PM_PAR_7] = Sp;
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
    psVectorAppend (chi2, model->chisqNorm);

    psPolynomial1D *poly = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 2);
    if (!psVectorFitPolynomial1D (poly, NULL, 0, chi2, NULL, Sidx)) {
        psError(PS_ERR_UNKNOWN, true, "Failed to find a good chisq parabola");
	psFree (chi2);
	psFree (Sidx);
	psFree (poly);
	return false;
    }

    // where is the minimum of this polynomial fit?
    float Smin = -0.5 * poly->coeff[1] / poly->coeff[2] / 100.0;

    // constrain Smin to be in a valid range: allow the fitted range to go a bit beyond the 3 trial points, but no further
    float Smx = Sm - 0.25*(So - Sm);
    float Spx = Sp + 0.25*(Sp - So);
    Smin = PS_MAX(PS_MIN(Smin, Smx), Spx);
    PAR[PM_PAR_7] = Smin;

    // XXX I could set the error on PAR_7 here if I knew how to roughly convert these chisq values to true chisq values

    // return to the original fitting mode (fitOptions)
    pmPCMupdate(pcm, source, fitOptions, model);

    psFree (chi2);
    psFree (Sidx);
    psFree (poly);

    return true;
}

 
// we have a set of guess parameters, do a small number of iterations fitting only SHAPE then only INDEX 
bool psphotFitSersicShapeAndIndexGridAutoScaled (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {

    pmModel *model = pcm->modelConv;

    assert (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));

    pmSourceFitOptions options = *fitOptions;
    
    psF32 *PAR = pcm->modelConv->params->data.F32;

    options.mode = PM_SOURCE_FIT_SHAPE;
    options.nIter = 5;
    
    // update the pcm elements if we have changed the circumstance (here, options.mode)
    pmPCMupdate(pcm, source, &options, model);
    
    float parStart[8];
    for (int i = 0; i < 8; i++) parStart[i] = PAR[i];

    // we start with a guess at the index (P[7])

    // get chisq for P[7], P[7]*1.1, P[7]*1.25 (or *0.75 depending on the result of 1.1)

    psVector *chi2 = psVectorAllocEmpty (16, PS_TYPE_F32);
    psVector *Sidx = psVectorAllocEmpty (16, PS_TYPE_F32);

    PAR[PM_PAR_7] = parStart[7];
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
    psVectorAppend (chi2, model->chisqNorm);

    float fI = 1.1;
    PAR[PM_PAR_7] = parStart[7]*fI;
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
    psVectorAppend (chi2, model->chisqNorm);

    if (chi2->data.F32[1] < chi2->data.F32[0]) {
      fI = 1.3;
    } else {
      fI = 1.0 / 1.3;
    }
    
    PAR[PM_PAR_7] = parStart[7]*fI;
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
    psVectorAppend (chi2, model->chisqNorm);

    // can we fit the 3 pts with a parabola?
    int nTry = 0;
    psPolynomial1D *poly = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 2);
    while (!psVectorFitPolynomial1D (poly, NULL, 0, chi2, NULL, Sidx)) {
      psErrorClear (); // clear the polynomial error
      if (nTry > 4) {
        psError(PS_ERR_UNKNOWN, true, "Failed to find a good chisq parabola");
	psFree (chi2);
	psFree (Sidx);
	psFree (poly);
	return false;
      }
      fI = (fI < 1.0) ? fI / 1.3 : fI * 1.3;
      PAR[PM_PAR_7] = parStart[7]*fI;
      pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
      psVectorAppend (Sidx, 100*PAR[PM_PAR_7]);
      psVectorAppend (chi2, model->chisqNorm);
      nTry ++;
    }

    // where is the minimum of this polynomial fit?
    float Smin = -0.5 * poly->coeff[1] / poly->coeff[2] / 100.0;

    // constrain Smin to be in a valid range (1.0 - 0.1, corresponding to 0.5 (Gauss) to 5.0 (slightly peakier than Dev)
    Smin = PS_MAX(PS_MIN(Smin, 1.0), 0.1);
    PAR[PM_PAR_7] = Smin;

    // pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    // if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    
    //// for (int i = 0; i < 8; i++) PAR[i] = parStart[i];
    //// 
    //// for (float fI = 0.0; fI < 0.15; fI += 0.01) {
    ////   PAR[PM_PAR_7] = parStart[7] - fI;
    //// 
    ////   // fit EXT (not PSF) model (set/unset the pixel mask)
    ////   
    ////   pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    ////   if (TIMING) {
    //// 	float *PAR = model->params->data.F32;
    //// 	fprintf (stderr, "%d %f : %f - %f %f %f - %f\n", model->nIter, model->chisqNorm, PAR[7], PAR[4], PAR[5], PAR[6], PAR[1]);
    ////   }
    //// }

    // return to the original fitting mode (fitOptions)
    pmPCMupdate(pcm, source, fitOptions, model);

    psFree (chi2);
    psFree (Sidx);
    psFree (poly);

    return true;
}

 
// we have a set of guess parameters, do a small number of iterations fitting only SHAPE then only INDEX 
bool psphotFitSersicShapeAndIndexGrid (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize) {

    pmModel *model = pcm->modelConv;

    assert (model->type == pmModelClassGetType("PS_MODEL_SERSIC"));

    pmSourceFitOptions options = *fitOptions;
    
    psF32 *PAR = pcm->modelConv->params->data.F32;

    options.mode = PM_SOURCE_FIT_SHAPE;
    options.nIter = 10;
    
    // update the pcm elements if we have changed the circumstance (here, options.mode)
    pmPCMupdate(pcm, source, &options, model);
    
    psVector *chi2 = psVectorAllocEmpty (16, PS_TYPE_F32);
    psVector *Sidx = psVectorAllocEmpty (16, PS_TYPE_F32);

    float par7[] = {0.100, 0.125, 0.150, 0.175, 0.200, 0.225, 0.250};
    for (int i = 0; i < 7; i++) {
      PAR[PM_PAR_7] = par7[i];
      pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
      if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
      psVectorAppend (Sidx, PAR[PM_PAR_7]);
      psVectorAppend (chi2, model->chisqNorm);
    }

    psPolynomial1D *poly = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 2);
    if (!psVectorFitPolynomial1D (poly, NULL, 0, chi2, NULL, Sidx)) {
      psError(PS_ERR_UNKNOWN, true, "Failed to find a good chisq parabola");
      psFree (chi2);
      psFree (Sidx);
      psFree (poly);
      return false;
    }

    // where is the minimum of this polynomial fit?
    fprintf (stderr, "fit1d: %f + %f x + %f x^2\n", poly->coeff[0], poly->coeff[1], poly->coeff[2]);
    float Smin = -0.5 * poly->coeff[1] / poly->coeff[2];

    // constrain Smin to be in a valid range (1.0 - 0.1, corresponding to 0.5 (Gauss) to 5.0 (slightly peakier than Dev)
    Smin = PS_MAX(PS_MIN(Smin, 1.0), 0.1);
    PAR[PM_PAR_7] = Smin;
    pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    if (EXTRA_VERBOSE) fprintf (stderr, "%d >>> %d %f : %f - %f %f - %f %f %f - %f\n", source->id, model->nIter, model->chisqNorm, PAR[7], PAR[2], PAR[3], PAR[4], PAR[5], PAR[6], PAR[1]);
    
    //// for (int i = 0; i < 8; i++) PAR[i] = parStart[i];
    //// 
    //// for (float fI = 0.0; fI < 0.15; fI += 0.01) {
    ////   PAR[PM_PAR_7] = parStart[7] - fI;
    //// 
    ////   // fit EXT (not PSF) model (set/unset the pixel mask)
    ////   
    ////   pmSourceFitPCM (pcm, source, &options, maskVal, markVal, psfSize);
    ////   if (TIMING) {
    //// 	float *PAR = model->params->data.F32;
    //// 	fprintf (stderr, "%d %f : %f - %f %f %f - %f\n", model->nIter, model->chisqNorm, PAR[7], PAR[4], PAR[5], PAR[6], PAR[1]);
    ////   }
    //// }

    // return to the original fitting mode (fitOptions)
    pmPCMupdate(pcm, source, fitOptions, model);

    psFree (chi2);
    psFree (Sidx);
    psFree (poly);

    return true;
}

// # define N_REFF_CHECK 11
// float drefCheck[] = {-0.02, -0.04, -0.06, 0.0, 0.85, 0.90, 0.95, 1.00, 1.05, 1.10, 1.15, 1.20, 1.25};

// we have an initial fit, check to see if the current size is besst
bool psphotPCMfitCheckSize (pmPCMdata *pcm, pmSource *source, psImageMaskType maskVal, float psfSize) {

    // PAR is already at my current best guess
    psF32 *PAR = pcm->modelConv->params->data.F32;

    // store best guess as a shape
    psEllipseAxes centerAxes;
    pmModelParamsToAxes (&centerAxes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], true);

    float xMin = NAN;
    float iMin = NAN;
    float rMin = NAN;

    // loop over Reff, keeping the ARatio and Theta constant
    for (int j = -4; j <= 4; j++) {

	float dref = j * 0.01;

	psEllipseAxes guessAxes;
	guessAxes.major = centerAxes.major + dref;
	guessAxes.minor = guessAxes.major * centerAxes.minor / centerAxes.major;
	guessAxes.theta = centerAxes.theta;

	if (!isfinite(guessAxes.major)) return false;
	if (!isfinite(guessAxes.minor)) return false;
	if (!isfinite(guessAxes.theta)) return false;

	// convert the major,minor,theta to shape parameters for an Reff-like model
	pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], guessAxes, true);

	// generated the modelFlux
	    // XXX note that this does not add sky to model
	pmPCMMakeModel (source, pcm->modelConv, pcm->nsigma, maskVal, psfSize);
	
	float YY = 0.0;
	float YM = 0.0;
	float MM = 0.0;
	bool usePoisson = false;

	for (int iy = 0; iy < source->pixels->numRows; iy++) {
	    for (int ix = 0; ix < source->pixels->numCols; ix++) {
		// skip masked points
		if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) {
		    continue;
		}
		// skip zero-variance points
		if (source->variance->data.F32[iy][ix] == 0) {
		    continue;
		}
		// skip nan value points
		if (!isfinite(source->pixels->data.F32[iy][ix])) {
		    continue;
		}

		float fy = source->pixels->data.F32[iy][ix];
		float fm = source->modelFlux->data.F32[iy][ix];
		float wt = (usePoisson) ? 1.0 / source->variance->data.F32[iy][ix] : 1.0;

		YY += PS_SQR(fy) * wt;
		YM += fm * fy * wt;
		MM += PS_SQR(fm) * wt;
	    }
	}

	float Io = YM / MM;
	float Chisq = YY - 2 * Io * YM + Io * Io * MM;
	if (isnan(xMin) || (Chisq < xMin)) {
	    xMin = Chisq;
	    iMin = Io;
	    rMin = dref;
	}
	// fprintf (stderr, "%d | %f %f %f | %f %f %f\n", j, dref, Io, Chisq, rMin, iMin, xMin);
    }

    psEllipseAxes guessAxes;
    guessAxes.major = centerAxes.major + rMin;
    guessAxes.minor = guessAxes.major * centerAxes.minor / centerAxes.major;
    guessAxes.theta = centerAxes.theta;

    if (!isfinite(guessAxes.major)) return false;
    if (!isfinite(guessAxes.minor)) return false;
    if (!isfinite(guessAxes.theta)) return false;

    // convert the major,minor,theta to shape parameters for an Reff-like model
    pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], guessAxes, true);
    PAR[PM_PAR_I0] = iMin;

    return true;
}

// we have an initial fit, check to see if the current size is besst
bool psphotPCMfitRetry (pmPCMdata *pcm, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, float psfSize) {

    // PAR is already at my current best guess
    psF32 *PAR = pcm->modelConv->params->data.F32;

    // store best guess as a shape
    psEllipseAxes centerAxes;
    pmModelParamsToAxes (&centerAxes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], true);

    // retry with axes smaller by 1 pixel
    psEllipseAxes guessAxes;
    guessAxes.major = centerAxes.major - 0.08;
    guessAxes.minor = guessAxes.major * centerAxes.minor / centerAxes.major;
    guessAxes.theta = centerAxes.theta;

    if (!isfinite(guessAxes.major)) return false;
    if (!isfinite(guessAxes.minor)) return false;
    if (!isfinite(guessAxes.theta)) return false;

    // convert the major,minor,theta to shape parameters for an Reff-like model
    pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], guessAxes, true);

    // generated the modelFlux
	    // XXX note that this does not add sky to model
    pmPCMMakeModel (source, pcm->modelConv, pcm->nsigma, maskVal, psfSize);
	
    float YY = 0.0;
    float YM = 0.0;
    float MM = 0.0;
    bool usePoisson = false;

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    // skip masked points
	    if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) {
		continue;
	    }
	    // skip zero-variance points
	    if (source->variance->data.F32[iy][ix] == 0) {
		continue;
	    }
	    // skip nan value points
	    if (!isfinite(source->pixels->data.F32[iy][ix])) {
		continue;
	    }

	    float fy = source->pixels->data.F32[iy][ix];
	    float fm = source->modelFlux->data.F32[iy][ix];
	    float wt = (usePoisson) ? 1.0 / source->variance->data.F32[iy][ix] : 1.0;

	    YY += PS_SQR(fy) * wt;
	    YM += fm * fy * wt;
	    MM += PS_SQR(fm) * wt;
	}
    }

    float Io = YM / MM;
    PAR[PM_PAR_I0] = Io;

    pmSourceFitPCM (pcm, source, fitOptions, maskVal, markVal, psfSize);  // NOTE : 1687 allocs in here

    return true;
}



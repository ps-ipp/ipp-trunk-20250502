# include "psphotInternal.h"
# define PM_SOURCE_FIT_PSF_X_EXT PM_SOURCE_FIT_PSF_AND_SKY

bool psphotModelTestReadout (pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status;

    psTimerStart ("modelTest");

    pmModelClassSetLimits(PM_MODEL_LIMITS_LAX);

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    // remove cruft from the input analysis structure
    if (!psphotCleanInputs (config, view, filerule)) {
        psError (PSPHOT_ERR_PROG, false, "trouble setting up the inputs");
        return false;
    }

    // set the photcode for this image
    if (!psphotAddPhotcode (config, view, filerule)) {
        psError (PSPHOT_ERR_CONFIG, false, "trouble defining the photcode");
        return false;
    }

    // Generate the mask and weight images, including the user-defined analysis region of interest
    if (!psphotSetMaskAndVariance (config, view, filerule)) {
        return psphotReadoutCleanup(config, view, filerule);
    }

    // generate a background model (median, smoothed image)
    if (!psphotModelBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    if (!psphotSubtractBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    // load the psf model, if suppled.  FWHM_MAJ,FWHM_MIN,etc are determined and saved on
    // readout->analysis. NOTE: this function currently only loads from PSPHOT.PSF.LOAD
    if (!psphotLoadPSF (config, view, filerule)) { // ??? need to supply 2 ?
        psError (PSPHOT_ERR_UNKNOWN, false, "error loading psf model");
        return psphotReadoutCleanup (config, view, filerule);
    }

    float MIN_KRON_RADIUS = 5.0;

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // what fraction of the PSF is used? (radius in pixels : 2 -> 5x5 box)
    int psfSize = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");
    assert (status);

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // use poissonian errors or local-sky errors
    bool POISSON_ERRORS = psMetadataLookupBool (&status, recipe, filerule);
    if (!status) POISSON_ERRORS = true;

    // find the various fitting parameters (try test values first)
    float INNER = psMetadataLookupF32 (&status, recipe, "TEST_FIT_INNER_RADIUS");
    if (!status || !isfinite(INNER)) {
        INNER = psMetadataLookupF32 (&status, recipe, "SKY_INNER_RADIUS");
    }
    float OUTER = psMetadataLookupF32 (&status, recipe, "TEST_FIT_OUTER_RADIUS");
    if (!status || !isfinite(OUTER)) {
        OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    }
    float RADIUS = psMetadataLookupF32 (&status, recipe, "TEST_FIT_RADIUS");
    if (!status || !isfinite(RADIUS)) {
        RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_FIT_RADIUS");
    }
    float mRADIUS = psMetadataLookupF32 (&status, recipe, "TEST_MOMENTS_RADIUS");
    if (!status || !isfinite(mRADIUS)) {
        mRADIUS = psMetadataLookupF32 (&status, recipe, "PSF_MOMENTS_RADIUS");
    }

    // define the source of interest
    float xObj     = psMetadataLookupF32 (&status, recipe, "TEST_FIT_X");
    float yObj     = psMetadataLookupF32 (&status, recipe, "TEST_FIT_Y");
    if (!isfinite(xObj) || !isfinite(yObj)) psAbort ("object position is not defined");

    // construct the source structures
    pmSource *source = pmSourceAlloc();
    source->peak = pmPeakAlloc (xObj, yObj, 0, 0);
    pmSourceDefinePixels (source, readout, xObj, yObj, OUTER);

    // find the model: supplied by user or first in the PSF_MODEL list
    char *modelName  = psMetadataLookupStr (&status, recipe, "TEST_FIT_MODEL");
    int modelType = pmModelClassGetType (modelName);
    if (modelType < 0) psAbort("unknown model %s", modelName);
    source->type = PM_SOURCE_TYPE_EXTENDED;

    bool TEST_FIT_CONVOLVED = psMetadataLookupBool (&status, recipe, "TEST_FIT_CONVOLVED");

    // find the local sky
    status = pmSourceLocalSky (source, PS_STAT_SAMPLE_MEDIAN, INNER, maskVal, markVal);
    if (!status) psAbort("pmSourceLocalSky error");

    bool testMomentsRadius = psMetadataLookupBool (&status, config->arguments, "TEST_MOMENTS_RADIUS");
    if (testMomentsRadius) { 
	// XXX I want to test an iterative aperture for brighter sources
	float radius = mRADIUS;
	for (int i = 0; i < 10; i++) {

	    // get the source moments
	  status = pmSourceMoments (source, radius, 0.25*radius, 0.0, MIN_KRON_RADIUS, maskVal);
	    if (!status) psAbort("psSourceMoments error");

	    float oldRadius = radius;
	    radius = source->moments->Mrf * RADIUS;
	    
	    fprintf (stderr, "%d %f  %f  %f\n", i, oldRadius, radius, source->moments->Mrf);
	}
	exit (0);
    }

    // get the source moments
    status = pmSourceMoments (source, mRADIUS, 0.25*mRADIUS, 0.0, MIN_KRON_RADIUS, maskVal);
    if (!status) psAbort("psSourceMoments error");

    fprintf (stderr, "sum: %f @ (%f, %f)\n", source->moments->Sum, source->moments->Mx, source->moments->My);
    fprintf (stderr, "moments: %f, %f - %f\n", source->moments->Mxx, source->moments->Myy, source->moments->Mxy);

    psEllipseMoments moments;
    moments.x2 = source->moments->Mxx;
    moments.y2 = source->moments->Myy;
    moments.xy = source->moments->Mxy;
    psEllipseAxes axes = psEllipseMomentsToAxes (moments, 20.0);

    fprintf (stderr, "axes: %f @ (%f, %f)\n", axes.theta*180/M_PI, axes.major, axes.minor);

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    if (psf) {
      // set PSF parameters for this model (apply 2D shape model to coordinates Xo, Yo)
      source->peak->rawFlux = source->moments->Peak;
      float Io = source->moments->Peak;
      source->modelPSF = pmModelFromPSFforXY(psf, xObj, yObj, Io);
    }

    // get the initial model parameter guess
    pmModel *model = pmSourceModelGuess (source, modelType, maskVal, markVal);
    if (!model) {
      fprintf (stderr, "failed to generate model guess\n");
      exit (2);
    }

    source->modelEXT = model;

    // if any parameters are defined by the user, take those values
    int nParams = pmModelClassParameterCount (modelType);
    psF32 *params = model->params->data.F32;
    params[PM_PAR_XPOS] = xObj; // XXX use the user-supplied value,
    params[PM_PAR_YPOS] = yObj; // XXX or use the centroid
    for (int i = 0; i < nParams; i++) {
        if (i == PM_PAR_XPOS) continue;
        if (i == PM_PAR_YPOS) continue;

	char name[32];
        sprintf (name, "TEST_FIT_PAR%d", i);
        float value = psMetadataLookupF32 (&status, recipe, name);
        if (status && isfinite (value)) {
            params[i] = value;
        }
    }

    float area = params[4]*params[5];
    fprintf (stderr, "peak: %f @ (%f, %f)\n", source->moments->Sum*area, (double)source->peak->x, (double)source->peak->y);

    if (modelType == pmModelClassGetType("PS_MODEL_TRAIL")) {
	fprintf (stderr, "guess: %f @ (%f, %f)\n", params[6]*180/M_PI, params[4], params[5]);
    } else {
	bool useReff = pmModelUseReff (modelType);
	pmModelParamsToAxes (&axes, params[PM_PAR_SXX], params[PM_PAR_SXY], params[PM_PAR_SYY], useReff);
	fprintf (stderr, "guess: %f @ (%f, %f) : %f\n", axes.theta*180/M_PI, axes.major, axes.minor, params[PM_PAR_SXY]);
    }

    fprintf (stderr, "input parameters: \n");
    for (int i = 0; i < nParams; i++) {
        fprintf (stderr, "%d : %f\n", i, params[i]);
    }

    // define the pixels used for the fit
    psImageKeepCircle (source->maskObj, xObj, yObj, RADIUS, "OR", markVal);
    psphotSaveImage (NULL, source->maskObj, "mask1.fits");

    float SKY_SIG = psMetadataLookupF32(&status, readout->analysis, "SKY_STDEV");

    // options which modify the behavior of the model fitting
    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
    fitOptions->nIter         = psMetadataLookupS32(&status, recipe, "PSF_FIT_ITER"); // Maximum number of fit iterations
    fitOptions->minTol        = psMetadataLookupF32 (&status, recipe, "PSF_FIT_MIN_TOL"); // Fit tolerance
    fitOptions->maxTol        = psMetadataLookupF32 (&status, recipe, "PSF_FIT_MAX_TOL"); // Fit tolerance
    fitOptions->maxChisqDOF   = psMetadataLookupF32 (&status, recipe, "PSF_FIT_MAX_CHISQ"); // Fit tolerance
    fitOptions->poissonErrors = POISSON_ERRORS;
    fitOptions->weight        = PS_SQR(SKY_SIG);
    fitOptions->mode          = PM_SOURCE_FIT_EXT;
    fitOptions->covarFactor   = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix

    bool chisqConvergence = psMetadataLookupBool (&status, recipe, "LMM_FIT_CHISQ_CONVERGENCE"); // Fit tolerance
    if (!status) chisqConvergence = true;
    fitOptions->chisqConvergence = chisqConvergence;

    bool useReweighting = psMetadataLookupBool (&status, recipe, "LMM_FIT_USE_REWEIGHTING"); // Fit tolerance
    if (!status) useReweighting = true;
    fitOptions->useReweighting = useReweighting;

    int gainFactorMode = psMetadataLookupS32 (&status, recipe, "LMM_FIT_GAIN_FACTOR_MODE"); // Fit tolerance
    if (!status) gainFactorMode = 0;
    fitOptions->gainFactorMode = gainFactorMode;

    if (modelType == pmModelClassGetType("PS_MODEL_SERSIC")) {
	fitOptions->mode = PM_SOURCE_FIT_NO_INDEX;
    }
    if (modelType == pmModelClassGetType("PS_MODEL_TRAIL")) {
	fitOptions->mode = PM_SOURCE_FIT_TRAIL;
    }

    if (TEST_FIT_CONVOLVED) {
      pmPCMdata *pcm = pmPCMinit (source, fitOptions, model, maskVal, psfSize);
      if (modelType == pmModelClassGetType("PS_MODEL_SERSIC")) {
	psphotSersicModelClassGuessPCM (pcm, source);
      } else {
	pmSourceModelGuessPCM (pcm, source, maskVal, markVal);
      }

      // if we provide a test guess value we want to use that value!
      for (int i = 0; i < nParams; i++) {
	  if (i == PM_PAR_XPOS) continue;
	  if (i == PM_PAR_YPOS) continue;

	  char name[32];
	  sprintf (name, "TEST_FIT_PAR%d", i);
	  float value = psMetadataLookupF32 (&status, recipe, name);
	  if (status && isfinite (value)) {
	      params[i] = value;
	  }
      }

      pmPCMupdate(pcm, source, fitOptions, model);
      pmSourceFitPCM (pcm, source, fitOptions, maskVal, markVal, psfSize);
      psFree (pcm);
    } else {
      status = pmSourceFitModel (source, model, fitOptions, maskVal);
    }

    // measure the source mags
    float fitMag = NAN;
    float fitFlux = NAN;
    float obsMag = NAN;
    pmSourcePhotometryModel (&fitMag, &fitFlux, model);
    pmSourcePhotometryAper  (NULL, &obsMag, NULL, NULL, model, source->pixels, source->variance, source->maskObj, maskVal);
    fprintf (stderr, "ap: %f, fit: %f, apmifit: %f, nIter: %d\n", obsMag, fitMag, obsMag - fitMag, model->nIter);

    // write out positive object
    psphotSaveImage (NULL, source->pixels, "object.fits");

    // subtract object, leave local sky
    // pmModelSub (source->pixels, source->maskObj, model, PM_MODEL_OP_FULL, maskVal);
    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

    fprintf (stderr, "output parameters: \n");
    for (int i = 0; i < nParams; i++) {
        fprintf (stderr, "%d : %f\n", i, params[i]);
    }

    if (modelType == pmModelClassGetType("PS_MODEL_TRAIL")) {
	fprintf (stderr, "result: %f @ (%f, %f)\n", params[6]*180/M_PI, params[4], params[5]);
    } else {
	bool useReff = pmModelUseReff (modelType);
	pmModelParamsToAxes (&axes, params[PM_PAR_SXX], params[PM_PAR_SXY], params[PM_PAR_SYY], useReff);
	fprintf (stderr, "result: %f @ (%f, %f) : %f\n", axes.theta*180/M_PI, axes.major, axes.minor, params[PM_PAR_SXY]);
    }

    // write out
    psphotSaveImage (NULL, source->pixels, "resid.fits");
    psphotSaveImage (NULL, source->maskObj, "mask.fits");

    psLogMsg ("psphot", PS_LOG_INFO, "model test : %f sec\n", psTimerMark ("modelTest"));

    exit (0);
}

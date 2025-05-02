# include "psphotInternal.h"

// this should be called by every program that links against libpsphot
bool psphotInit (void) {
    psphotErrorRegister();              // register our error codes/messages
    psphotModelClassInit ();            // load implementation-specific models
    psphotSetThreads ();
    return true;
}

# if (0)
// TEST CODE, can be removed
bool psphotDumpFlux (pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status = false;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    static int npass;
    char filename[64];
    snprintf (filename, 64, "mags.%d.dat", npass);
    FILE *ftest = fopen (filename, "w");
    for (int j = 0; j < sources->n; j++) {
	pmSource *source = sources->data[j];

	float psfMag;
	status = pmSourcePhotometryModel (&psfMag, NULL, source->modelPSF);

	float psfMagNorm;
	float Io = source->modelPSF->params->data.F32[PM_PAR_I0];
	source->modelPSF->params->data.F32[PM_PAR_I0] = 1.0;
	status = pmSourcePhotometryModel (&psfMagNorm, NULL, source->modelPSF);
	source->modelPSF->params->data.F32[PM_PAR_I0] = Io;

	// double apTrend = pmTrend2DEval (psf->ApTrend, (float)source->peak->x, (float)source->peak->y);
	fprintf (ftest, "%d %d %d  %f %f %f %f  %f %f\n", j, source->peak->x, source->peak->y, source->modelPSF->params->data.F32[PM_PAR_I0], source->modelPSF->params->data.F32[PM_PAR_SXX], source->modelPSF->params->data.F32[PM_PAR_SYY], source->modelPSF->params->data.F32[PM_PAR_SXY], psfMag, psfMagNorm);
    }
    fclose (ftest);
    npass++;

    return true;
}
# endif

bool psphotReadout(pmConfig *config, const pmFPAview *view, const char *filerule) {

    // measure the total elapsed time in psphotReadout.  dtime is the elapsed time used jointly
    // by the multiple threads, not the total time used by all threads.
    psTimerStart ("psphotReadout");

    pmModelClassSetLimits(PM_MODEL_LIMITS_LAX);

    // select the current recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }
    // optional break-point for processing
    char *breakPt = psMetadataLookupStr (NULL, recipe, "BREAK_POINT");
    psAssert (breakPt, "configuration error: set BREAK_POINT");

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
    if (!strcasecmp (breakPt, "NOTHING")) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    // generate a background model (median, smoothed image)
    if (!psphotModelBackground (config, view, filerule)) {
      // XXX this should result in a bad quality flag
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!psphotSubtractBackground (config, view, filerule)) {
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!strcasecmp (breakPt, "BACKMDL")) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    // load the psf model, if suppled.  FWHM_MAJ,FWHM_MIN,etc are determined and saved on
    // readout->analysis. NOTE: this function currently only loads from PSPHOT.PSF.LOAD
    if (!psphotLoadPSF (config, view, filerule)) { // ??? need to supply 2 ?
        psError (PSPHOT_ERR_UNKNOWN, false, "error loading psf model");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // find the detections (by peak and/or footprint) in the image.
    if (!psphotFindDetections (config, view, filerule, true)) { // pass 1
        // this only happens if we had an error in psphotFindDetections
        psError (PSPHOT_ERR_UNKNOWN, false, "failure in peak analysis");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // construct sources and measure moments and other basic stats (saved on detections->newSources)
    // all sources use the auto-scaled window appropriate to a PSF, except for the saturated
    // stars : these use a larger window (3x the basic window)
    if (!psphotSourceStats (config, view, filerule, true)) { // pass 1
        psError(PSPHOT_ERR_UNKNOWN, false, "failure to generate sources");
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!strcasecmp (breakPt, "PEAKS")) {
      psphotDumpTest (config, view, filerule);
      return psphotReadoutCleanup (config, view, filerule);
    }

    // mark blended peaks PS_SOURCE_BLEND (detections->newSources)
    // XXX I've deactivated this because it was preventing galaxies close to stars from being
    // XXX fitted as an extended source.
    if (false && !psphotBasicDeblend (config, view, filerule)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed on deblend analysis");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // classify sources based on moments, brightness.  if a PSF model has been loaded, the PSF
    // clump defined for it is used not measured (detections->newSources)
    if (!psphotRoughClass (config, view, filerule)) { // pass 1
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to determine rough classifications");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // find and subtract radial profile models for saturated stars (XXX change name eventually)
    if (!psphotDeblendSatstars (config, view, filerule)) {
	psError (PSPHOT_ERR_UNKNOWN, false, "failed on satstar deblend analysis");
	return psphotReadoutCleanup (config, view, filerule);
    }

    // if we were not supplied a PSF model, determine the IQ stats here (detections->newSources)
    if (!psphotImageQuality (config, view, filerule)) { // pass 1
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to measure image quality");
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!strcasecmp (breakPt, "MOMENTS")) {
      psphotDumpTest (config, view, filerule);
        return psphotReadoutCleanup (config, view, filerule);
    }

    // use bright stellar objects to measure PSF if we were supplied a PSF for any input file,
    // this step is skipped
    if (!psphotChoosePSF (config, view, filerule, true)) { // pass 1
        psLogMsg ("psphot", 3, "failure to construct a psf model");
        return psphotReadoutCleanup (config, view, filerule);
    }
    if (!strcasecmp (breakPt, "PSFMODEL")) {
        return psphotReadoutCleanup (config, view, filerule);
    }

    // include externally-supplied sources
    // XXX fix this in the new multi-input context
    // psphotLoadExtSources (config, view, filerule); // pass 1

    // merge the newly selected sources into the existing list
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, filerule);

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    // pass 1
    if (!psphotGuessModels (config, view, filerule)) {
        psLogMsg ("psphot", 3, "failure to Guess Model - pass 1");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // linear PSF fit to source peaks, subtract the models from the image (in PSF mask)
    psphotFitSourcesLinear (config, view, filerule, false, true); // pass 1 (detections->allSources)

    // measure the radial profiles to the sky
    psphotRadialProfileWings (config, view, filerule);

    // re-measure the kron mags with models subtracted.  this pass uses a circular window of size PSF_MOMENTS_RADIUS (same window used to measure the psf-scale moments)
    
    // but this is chosen above to be appropriate for the PSF objects (not galaxies)
    // psphotKronMasked(config, view, filerule);
    psphotKronIterate(config, view, filerule, 1);

    // identify CRs and extended sources (only unmeasured sources are measured)
    psphotSourceSize (config, view, filerule, true); // pass 1 (detections->allSources)
    if (!strcasecmp (breakPt, "ENSEMBLE")) {
        goto finish;
    }

    // non-linear PSF and EXT fit to brighter sources
    // replace model flux, adjust mask as needed, fit, subtract the models (full stamp)
    // XXX: can leave faulted job in done queue
    psphotBlendFit (config, view, filerule); // pass 1 (detections->allSources)

    // replace all sources
    psphotReplaceAllSources (config, view, filerule, false); // pass 1 (detections->allSources)

    // linear fit to include all sources (subtract again)
    // NOTE : apply to ALL sources (extended + psf)
    psphotFitSourcesLinear (config, view, filerule, true, true); // pass 2 (detections->allSources)

    // if we only do one pass, skip to extended source analysis
    if (!strcasecmp (breakPt, "PASS1")) goto pass1finish;

    // NOTE: possibly re-measure background model here with objects subtracted / or masked

    // NOTE: this block performs the 2nd pass low-significance PSF detection stage
    { 
	// add noise for subtracted objects & subtracted saturated stars
	psphotAddNoise (config, view, filerule); // pass 1 (detections->allSources)

	// find fainter sources
	// NOTE: finds new peaks and new footprints, OLD and FULL set are saved on detections
	psphotFindDetections (config, view, filerule, false); // pass 2 (detections->peaks, detections->footprints)

	// remove noise for subtracted objects (ie, return to normal noise level)
	// NOTE: this needs to operate only on the OLD sources
        bool footprintUseUnsubtracted = psMetadataLookupBool(NULL, recipe, "FOOTPRINT_USE_UNSUBTRACTED");
        // Note: if footprintUseUnsubtracted is true the noise was already subtracted in psphotFindDetections()
        if (!footprintUseUnsubtracted) {
	    psphotSubNoise (config, view, filerule); // pass 1 (detections->allSources)
        }

	// define new sources based on only the new peaks & measure moments
	// NOTE: new sources are saved on detections->newSources
	psphotSourceStats (config, view, filerule, false); // pass 2 (detections->newSources)

	// set source type
	// NOTE: apply only to detections->newSources
	if (!psphotRoughClass (config, view, filerule)) { // pass 2 (detections->newSources)
	    psLogMsg ("psphot", 3, "failed to find a valid PSF clump for image");
	    return psphotReadoutCleanup (config, view, filerule);
	}

	// replace all sources so fit below applies to all at once
	// NOTE: apply only to OLD sources (which have been subtracted)
	psphotReplaceAllSources (config, view, filerule, false); // pass 2

	// merge the newly selected sources into the existing list
	// NOTE: merge OLD and NEW
	psphotMergeSources (config, view, filerule); // (detections->newSources + detections->allSources -> detections->allSources)

	// Construct an initial model for each object, set the radius to fitRadius, set circular
	// fit mask.  NOTE: only applied to sources without guess models
	psphotGuessModels (config, view, filerule); // pass 1

	// NOTE: apply to ALL sources
	psphotFitSourcesLinear (config, view, filerule, true, true); // pass 3 (detections->allSources)
    }

    // NOTE: this block performs the 2nd pass low-significance EXT detection stage (smooth or rebin by NxN times PSF size)
    if (0) { 
	// add noise for subtracted objects
	psphotAddNoise (config, view, filerule); // pass 1 (detections->allSources)

	// find fainter sources
	// NOTE: finds new peaks and new footprints, OLD and FULL set are saved on detections
	psphotFindDetections (config, view, filerule, false); // pass 2 (detections->peaks, detections->footprints)

	// remove noise for subtracted objects (ie, return to normal noise level)
	// NOTE: this needs to operate only on the OLD sources
	psphotSubNoise (config, view, filerule); // pass 1 (detections->allSources)

	// define new sources based on only the new peaks
	// NOTE: new sources are saved on detections->newSources
	psphotSourceStats (config, view, filerule, false); // pass 2 (detections->newSources)

	// set source type
	// NOTE: apply only to detections->newSources
	if (!psphotRoughClass (config, view, filerule)) { // pass 2 (detections->newSources)
	    psLogMsg ("psphot", 3, "failed to find a valid PSF clump for image");
	    return psphotReadoutCleanup (config, view, filerule);
	}

	// replace all sources so fit below applies to all at once
	// NOTE: apply only to OLD sources (which have been subtracted)
	psphotReplaceAllSources (config, view, filerule, false); // pass 2

	// merge the newly selected sources into the existing list
	// NOTE: merge OLD and NEW
	psphotMergeSources (config, view, filerule); // (detections->newSources + detections->allSources -> detections->allSources)

	// Construct an initial model for each object, set the radius to fitRadius, set circular
	// fit mask.  NOTE: only applied to sources without guess models
	psphotGuessModels (config, view, filerule); // pass 1

	// NOTE: apply to ALL sources
	psphotFitSourcesLinear (config, view, filerule, true, true); // pass 3 (detections->allSources)
    }

pass1finish:

    // measure the radial profiles to the sky (only measures new objects)
    psphotRadialProfileWings (config, view, filerule);

    // re-measure the kron mags with models subtracted
    // psphotKronMasked(config, view, filerule);
    psphotKronIterate(config, view, filerule, 2);

    // measure source size for the remaining sources
    // NOTE: applies only to NEW (unmeasured) sources
    psphotSourceSize (config, view, filerule, false); // pass 2 (detections->allSources)

    // XXX currently we are doing both the analysis of the size and the assessment of "fit ext"
    // in source size.  this overloads the bit MODE_EXT_LIMIT to mean "fit ext" not just
    // "bigger than a PSF"

    // decide which source(s) are to be fitted with the extended source analysis code.
    psphotChooseAnalysisOptions (config, view, filerule);

    psphotExtendedSourceAnalysis (config, view, filerule); // pass 1 (detections->allSources)
    psphotExtendedSourceFits (config, view, filerule); // pass 1 (detections->allSources)
    // measure some parameters for galaxy science
    psphotGalaxyParams (config, view, filerule);
    psphotRadialApertures(config, view, filerule, 0);

finish:

    // plot positive sources
    // psphotSourcePlots (readout, sources, recipe);

    // measure aperture photometry corrections
    if (!psphotApResid (config, view, filerule)) { // pass 1 (detections->allSources)
        psLogMsg ("psphot", 3, "failed on psphotApResid");
        return psphotReadoutCleanup (config, view, filerule);
    }

    // calculate source magnitudes
    if (!psphotMagnitudes(config, view, filerule)) { // pass 1 (detections->allSources)
	psErrorStackPrint(stderr, "Unable to do magnitudes.");
        psErrorClear();
    }

    // calculate lensing parameters
    if (!psphotLensing(config, view, filerule)) {
	psErrorStackPrint(stderr, "Unable to do lensing parameters.");
        psErrorClear();
    }

    if (!psphotEfficiency(config, view, filerule)) { // pass 1
        psErrorStackPrint(stderr, "Unable to determine detection efficiencies from fake sources");
        psErrorClear();
    }

    // replace failed sources?
    // psphotReplaceUnfitSources (sources);

    // replace background in residual image
    if (!psphotSkyReplace (config, view, filerule)) { // pass 1
	psErrorStackPrint(stderr, "Unable to replace sky");
	psErrorClear();
    }

    // drop the references to the image pixels held by each source
    if (!psphotSourceFreePixels (config, view, filerule)) { // pass 1
	psErrorStackPrint(stderr, "Unable to free source pixels");
	psErrorClear();
    }

    psLogMsg ("psphot.readout", PS_LOG_WARN, "complete psphot readout : %f sec\n", psTimerMark ("psphotReadout"));

    // create the exported-metadata and free local data
    return psphotReadoutCleanup(config, view, filerule);
}

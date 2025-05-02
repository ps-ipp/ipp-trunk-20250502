# include "psphotInternal.h"

static void psphotLogMemoryStats(const char *heading);

// relevant filesets:
# define STACK_RAW "PSPHOT.STACK.INPUT.RAW"
# define STACK_OUT "PSPHOT.STACK.OUTPUT.IMAGE"

// XXX STACK_OUT currently is a copy of STACK_RAW, but should be a pointer to is as in psphot (single)

// TEST CODE, can be removed
bool psphotDumpImages (pmConfig *config, const pmFPAview *view, const char *filerule, char *base) {

    // XXX do nothing
    return true; 

    int num = psphotFileruleCount(config, "PSPHOT.INPUT");

    for (int i = 0; i < num; i++) {
	// find the currently selected readout
	pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
	psAssert (file, "missing file?");

	pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
	psAssert (readout, "missing readout?");

	char line[256];
	snprintf (line, 256, "%s.%d.im.fits", base, i);
	psphotSaveImage (NULL, readout->image, line);

	snprintf (line, 256, "%s.%d.wt.fits", base, i);
	psphotSaveImage (NULL, readout->variance, line);

	snprintf (line, 256, "%s.%d.mk.fits", base, i);
	psphotSaveImage (NULL, readout->mask, line);
    }
    // psphotSaveImage leaves an error on the stack
    psErrorClear();
    return true;
}

bool psphotStackVisualFilerule(pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status = false;

    int num = psphotFileruleCount(config, filerule);

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->allSources;
        psAssert (sources, "missing sources?");

	psphotVisualShowResidualImage (readout, true);
	psphotVisualShowObjectRegions (readout, recipe, sources);
    }
    return true;
}

bool psphotStackReadout (pmConfig *config, const pmFPAview *view) {

    psArray *objects = NULL; // used below after 'pass1finish' label

    // measure the total elapsed time in psphotReadout.  dtime is the elapsed time used jointly
    // by the multiple threads, not the total time used by all threads.
    psTimerStart ("psphotReadout");

    psphotLogMemoryStats("Start");

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

    // generate a background model (median, smoothed image)
    if (!psphotModelBackground (config, view, STACK_RAW)) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!psphotSubtractBackground (config, view, STACK_RAW)) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "BACKMDL")) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

#ifdef MAKE_CHISQ_IMAGE
    // also make the chisq detection image
    if (!psphotStackChisqImage(config, view, STACK_RAW, STACK_RAW)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failure to generate chisq image");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "CHISQ")) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
#endif

    // find the detections (by peak and/or footprint) in the image.
    // This finds the detections on Chisq image as well as the individuals
    if (!psphotFindDetections (config, view, STACK_RAW, true)) { // pass 1
        psError (PSPHOT_ERR_UNKNOWN, false, "failure in peak analysis");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // construct sources and measure basic stats (saved on detections->newSources)
    if (!psphotSourceStats (config, view, STACK_RAW, true)) { // pass 1
        psError(PSPHOT_ERR_UNKNOWN, false, "failure to generate sources");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "PEAKS")) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    // psphotDumpTest (config, view, STACK_RAW);
    psMemDump("sourcestats");
    psphotLogMemoryStats("sourcestats");

    // classify sources based on moments, brightness
    // only run this on detections from the input images, not chisq image
    if (!psphotRoughClass (config, view, STACK_RAW)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to determine rough classifications");
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // find and subtract radial profile models for saturated stars (XXX change name eventually)
    if (!psphotDeblendSatstars (config, view, STACK_RAW)) {
	psError (PSPHOT_ERR_UNKNOWN, false, "failed on satstar deblend analysis");
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // if we were not supplied a PSF model, determine the IQ stats here (detections->newSources)
    // only run this on detections from the input images, not chisq image
    if (!psphotImageQuality (config, view, STACK_RAW)) { // pass 1
        psError (PSPHOT_ERR_UNKNOWN, false, "failed to measure image quality");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "MOMENTS")) {
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // use bright stellar objects to measure PSF
    if (!psphotChoosePSF (config, view, STACK_RAW, true)) { // pass 1
        psLogMsg ("psphot", 3, "failure to construct a psf model");
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }
    if (!strcasecmp (breakPt, "PSFMODEL")) {
        return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // merge the newly selected sources into the existing list
    // NOTE: merge OLD and NEW
    psphotMergeSources (config, view, STACK_RAW);

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    psphotGuessModels (config, view, STACK_RAW);

    // linear PSF fit to source peaks, subtract the models from the image (in PSF mask)
    psphotFitSourcesLinear (config, view, STACK_RAW, false, false);
    psphotStackVisualFilerule(config, view, STACK_RAW);

    // measure the radial profiles to the sky
    psphotRadialProfileWings (config, view, STACK_RAW);

    // re-measure the kron mags with models subtracted.  this pass starts with a circular
    // window of size PSF_MOMENTS_RADIUS (same window used to measure the psf-scale moments)
    // but iterates to an appropriately larger size
    psphotLogMemoryStats("before.kron.1");
    psphotKronIterate(config, view, STACK_RAW, 1);
    psphotLogMemoryStats("after.kron.1");
	
    // identify CRs and extended sources
    psphotSourceSize (config, view, STACK_RAW, true);

    // non-linear PSF and EXT fit to brighter sources
    // replace model flux, adjust mask as needed, fit, subtract the models (full stamp)
    psphotBlendFit (config, view, STACK_RAW); // pass 1 (detections->allSources)

    // replace all sources (do NOT ignore subtraction state)
    psphotReplaceAllSources (config, view, STACK_RAW, false); // pass 1 (detections->allSources)

    psphotLogMemoryStats("pass1");

    // if we only do one pass, skip to extended source analysis
    if (!strcasecmp (breakPt, "PASS1")) goto pass1finish;

    // linear fit to include all sources (subtract again)
    // NOTE : apply to ALL sources (extended + psf)
    // NOTE 2 : this function subtracts the models from the given filerule
    psphotFitSourcesLinear (config, view, STACK_RAW, true, false); // pass 2 (detections->allSources)

    // NOTE: possibly re-measure background model here with objects subtracted / or masked

    // NOTE: this block performs the 2nd pass low-significance PSF detection stage
    { 
	// add noise for subtracted objects
	psphotAddNoise (config, view, STACK_RAW); // pass 1 (detections->allSources)

	// find fainter sources
	// NOTE: finds new peaks and new footprints, OLD and FULL set are saved on detections
	psphotFindDetections (config, view, STACK_RAW, false); // pass 2 (detections->peaks, detections->footprints)

	// remove noise for subtracted objects (ie, return to normal noise level)
	// NOTE: this needs to operate only on the OLD sources
        // NOTE: if fooprintsUseUnsubtracted, the noise has already been removed by psphotFindDetections
        bool footprintsUseUnsubtracted = psMetadataLookupBool(NULL, recipe, "FOOTPRINT_USE_UNSUBTRACTED");
        if (!footprintsUseUnsubtracted) {
    	    psphotSubNoise (config, view, STACK_RAW); // pass 1 (detections->allSources)
        }

	// define new sources based on only the new peaks & measure moments
	// NOTE: new sources are saved on detections->newSources
	psphotSourceStats (config, view, STACK_RAW, false); // pass 2 (detections->newSources)

	// set source type
	// NOTE: apply only to detections->newSources
	if (!psphotRoughClass (config, view, STACK_RAW)) { // pass 2 (detections->newSources)
	    psLogMsg ("psphot", 3, "failed to find a valid PSF clump for image");
	    return psphotReadoutCleanup (config, view, STACK_RAW);
	}

	// replace all sources so fit below applies to all at once
	// NOTE: apply only to OLD sources (which have been subtracted)
	psphotReplaceAllSources (config, view, STACK_RAW, false); // pass 2

	// merge the newly selected sources into the existing list
	// NOTE: merge OLD and NEW
	// XXX check on free of sources...
	psphotMergeSources (config, view, STACK_RAW); // (detections->newSources + detections->allSources -> detections->allSources)

	// Construct an initial model for each object, set the radius to fitRadius, set circular
	// fit mask.  NOTE: only applied to sources without guess models
	psphotGuessModels (config, view, STACK_RAW);
    }

    // gcc doesn't like the label to refer to a declaration so we declare this here
    bool splitLinearFit;

pass1finish:

    splitLinearFit = psMetadataLookupBool(NULL, recipe, "PSPHOT.STACK.SPLIT.LINEAR.FIT");
    if (splitLinearFit) {
        psLogMsg ("psphot", 3, "splitting fit of detected and matched soures\n");
        // Fit the detected sources separately from matched ones that we are about to create.
        // NOTE: apply to ALL sources but only include sources with postitive flux in the fit
        psphotFitSourcesLinear (config, view, STACK_RAW, true, true); // pass 3 (detections->allSources)
    }

    psphotLogMemoryStats("prematch");

    // generate the objects (objects unify the sources from the different images) NOTE: could
    // this just match the detections for the chisq image, and not bother measuring the source
    // stats in that case...?
    objects = psphotMatchSources (config, view, STACK_RAW);
    psMemDump("matchsources");

    // check the source density. If it too high change the number of radial bins
    // in the recipe.
    psphotLimitRadialApertures(recipe, objects->n);

    // Construct an initial model for each object, set the radius to fitRadius, set circular
    // fit mask.  NOTE: only applied to sources without guess models
    psphotGuessModels (config, view, STACK_RAW);

    psphotStackObjectsUnifyPosition (objects);

    // psphotStackObjectsSelectForAnalysis (config, view, STACK_RAW, objects);

    // final linear fit. NOTE: if splitLinearFit is true above, this pass will only fit
    // the unsubtracted (matched) sources (the sources that we fit above are subtracted)
    psphotFitSourcesLinear (config, view, STACK_RAW, true, false); // pass 4 (detections->allSources)

    // measure the radial profiles to the sky (only measures new objects)
    psphotRadialProfileWings (config, view, STACK_RAW);

    // re-measure the kron mags with models subtracted
    // psphotKronMasked(config, view, STACK_SRC);
    psphotLogMemoryStats("before.kron.2");
    psphotKronIterate(config, view, STACK_RAW, 2);
    psphotLogMemoryStats("after.kron.2");

    // measure source size for the remaining sources
    // NOTE: applies only to NEW (unmeasured) sources
    psphotSourceSize (config, view, STACK_RAW, false); // pass 2 (detections->allSources)

    psMemDump("psfstats");

    // drop matched sources without any useful measurements and set kron radii for the ones
    // we decide to keep
    psphotFilterMatchedSources (config, view, STACK_RAW, objects);

    // measure kron fluxes for the matched sources only
    psphotKronIterate(config, view, STACK_RAW, 3);

    // decide which source(s) are to be fitted with the extended source analysis code.
    psphotChooseAnalysisOptionsByObject (config, view, STACK_RAW, objects);

    // measure elliptical apertures, petrosians (objects sorted by S/N)
    // psphotExtendedSourceAnalysisByObject (config, objects, view, STACK_SRC); // pass 1 (detections->allSources)
    psphotExtendedSourceAnalysis (config, view, STACK_RAW); // pass 1 (detections->allSources)

    // measure non-linear extended source models (exponential, deVaucouleur, Sersic) (sources sorted by S/N)
    psphotExtendedSourceFits (config, view, STACK_RAW); // pass 1 (detections->allSources)

    // measure some parameters for galxy science
    psphotGalaxyParams (config, view, STACK_RAW); // pass 1 (detections->allSources)

    // create source children for the OUT filerule (for radial aperture photometry and output) 
    // NOTE: The new source children have image arrays pointing to the readout associated with 
    // STACK_OUT.  in psphotStackMatchPSFsetup, we copy the current pixel values from RAW to OUT, 
    // but keep the pointers the same so we do not break these source image references
    // XXX NOTE : if we use the pre-20130914 psphotStackReadout code, we need to use 'false' for the
    // sourcesSubtracted argument

    // do this here so that the variance image is available to reset the models
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

    bool radial_apertures = psMetadataLookupBool(NULL, recipe, "RADIAL_APERTURES");
    if (radial_apertures) {
        // measure circular, radial apertures (objects sorted by S/N)
        // this forces photometry on the undetected sources from other images

	// set up the FWHM vector
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
		snprintf (line, 256, "%s.%d", "out.t4", entry);
		psphotDumpImages (config, view, STACK_OUT, line);
	    }
        }
    }

    // measure aperture photometry corrections
    if (!psphotApResid (config, view, STACK_RAW)) {
        psLogMsg ("psphot", 3, "failed on psphotApResid");
	return psphotReadoutCleanup (config, view, STACK_RAW);
    }

    // calculate source magnitudes
    psphotMagnitudes(config, view, STACK_RAW);

    // calculate lensing parameters
    if (!psphotLensing(config, view, STACK_RAW)) {
	psErrorStackPrint(stderr, "Unable to do lensing parameters.");
        psErrorClear();
    }

    if (!psphotEfficiency(config, view, STACK_RAW)) {
        psErrorStackPrint(stderr, "Unable to determine detection efficiencies from fake sources");
        psErrorClear();
    }
    psphotCopyEfficiency (config, view, STACK_OUT, STACK_RAW);

    psphotLogMemoryStats("final");
#if (0)
    psphotSourceMemory(config, view, STACK_RAW);
    psphotSourceMemory(config, view, STACK_OUT);
#endif

    // replace background in residual image
    psphotSkyReplace (config, view, STACK_RAW);

    // drop the references to the image pixels held by each source
    psphotSourceFreePixels (config, view, STACK_RAW);
    psphotSourceFreePixels (config, view, STACK_OUT);

#ifdef MAKE_CHISQ_IMAGE
    // remove chisq image from config->file:PSPHOT.INPUT
    psphotStackRemoveChisqFromInputs(config, STACK_RAW);
#endif

    // create the exported-metadata and free local data
    return psphotReadoutCleanup (config, view, STACK_RAW);
}

bool psphotStackLoadWCS(pmConfig *config, const pmFPAview *view, const char *filerule) {

    int num = psphotFileruleCount(config, filerule);

    for (int i=0; i<num; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");
    
        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmHDU *hdu = pmHDUFromReadout(readout);
        psAssert (hdu, "input missing hdu?");

        if (!pmAstromReadWCS(file->fpa, readout->parent->parent, hdu->header, 1.0)) {
            psError (PSPHOT_ERR_UNKNOWN, false, "failed to read WCS from header for input %d", i);
            return false;
        }
    }
    return true;
}

// read the memory usage data from /proc and log them out
// This will only work on a system that has /proc (not MacOS for instance) but since this function just
// tries to open and read from a file it is safe
// XXX: refine this and move it to psLib
void psphotLogMemoryStats(const char *heading) {

    // file containing memory statistics for this process proc. See proc(5)
    const char* statm_path = "/proc/self/statm";

    FILE *f = fopen(statm_path,"r");
    if (!f) {
        psLogMsg ("psphot", PS_LOG_WARN, "failed to open %s", statm_path);
        return;
    }

    unsigned long vmSize, resident, share, text, lib, data;

    int nread;
    nread = fscanf(f,"%ld %ld %ld %ld %ld %ld", &vmSize, &resident, &share, &text, &lib, &data);
    fclose(f);
    if (nread != 6) {
        psLogMsg ("psphot", PS_LOG_WARN, "failed to read 6 items from %s", statm_path);
        return;
    }
  
    // XXX: assuming 4 KB page size here
#   define PAGES_TO_MB(_v) (_v * 4.096 / 1024.)
    psLogMsg ("psphot", PS_LOG_INFO, "Memory usage at %20s: Total VmSize: %8.2f MB   Resident %8.2f MB   Data: %8.2f MB\n",
        heading, PAGES_TO_MB(vmSize), PAGES_TO_MB(resident), PAGES_TO_MB(data));
}

/* here is the process:

 * we have two image sets:
 * RAW : unconvolved image stacks

 * OUT : psf-matched output image (there may be more than one of
 * these.  we will generate the first matched image by selecting the
 * target PSF and doing a full psf-maching process (as used by ppStack
 * and ppSub).  But, additional target output files should use a
 * simple gaussian convolution kernel determind from therms of the
 * current and the target).

 * the output should be / could be one of the matched images, but not
 * all.  should we ensure the first gets written out, and ot save the
 * others (or only optionally).

 * by default, we probably only sve the cmf ffile outputs.

 * load the RAW image (unconvolved stacks)
 * add photcode to the output headers / readout->analysis
 * generate mask and variance image (this is probably never needed in
   practice: we always load an input mask & var.
 * generate & subtract a model background for ?? (RAW? CNV? OUT? all?)
 * load a PSF (probably not yet working)

 * generate the CHISQ image from the RAW input images (why save on OUT?)

 * find detections on RAW

 * copy detections to OUT

 * generate source stats (moments) for OUT

 * match sources across inputs (on OUT?)

 * generate source stats for the new constructions

 * rough class (star, galaxy, cosmic, etc)

 * Image quality

 * generate PSF

 * guess models

 * merge sources (new -> old)

 * linear fit to the psf

 * find ApResid

 * assign common positions

 * radial apertures (** this should be on the PSF-matched images

 * extended analysis (elliptical profile & petrosian)

 * extended fits (sersic, etc)

 * psphot magnitudes


 ******

 the above is all wrong:  first, we should be doing the full
 morphology analysis (ExtendedAnalysis & ExtendedFits) on the CNV or
 RAW image (as desired optionally), etc.

 In the discussion below, 'BST' (best) means optionally RAW or CNV

 * detection : RAW & CHISQ (of RAW)
 * moments : used by psf analysis & classification (BST)
 * rough class : uses moments, not pixels
 * image quality : uses moments as well
 * generate PSF : (BST)
 * guess models (BST)
 * linear fit (BST)
 * find ApResid (BST) -- uses sources not pixels
 * extended analysis (BST)
 * extended fits (BST)
 * detection efficiency (BST)
 
 * somehow need to copy the sources so they point at the pixels on the
 * OUT image 

 * foreach target PSF
   * radial aperture
   * convolve to next target PSF

   * somehow need to organize the output file to have the values from
   * the different PSFs in separate tables (with header info to
   * specify the size of that PSF)

   */


// generate a vector fwhmValues where the first has the fwhm of the raw image and the
// successive entries have the target values

bool psphotStackMatchPSFsetup (pmConfig *config, const pmFPAview *view, const char *filerule, const char *filerulePSF) {

    bool status;

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image

	if (!psphotStackMatchPSFsetupReadout (config, view, filerule, filerulePSF, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to define target PSF sizes");
	    return false;
	}
    }

    return true;
}

float psphotPSFseeing (pmPSF *psf, pmReadout *readout, int index);

// copy the pixels from RAW to OUT (

bool psphotStackMatchPSFsetupReadout (pmConfig *config, const pmFPAview *view, const char *fileruleOut, const char *fileruleRaw, int index) {

    bool status;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);

    // find the currently selected readout
    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, fileruleOut, index); // File of interest
    psAssert (fileOut, "missing file?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    psAssert (readoutOut, "missing readout?");

    // find the currently selected readout
    pmFPAfile *fileRaw = pmFPAfileSelectSingle(config->files, fileruleRaw, index); // File of interest
    psAssert (fileRaw, "missing file?");

    pmReadout *readoutRaw = pmFPAviewThisReadout(view, fileRaw->fpa);
    psAssert (readoutRaw, "missing readout?");

    readoutOut->image = psImageCopy(readoutOut->image, readoutRaw->image, PS_TYPE_F32);
    if (readoutRaw->variance) {
	readoutOut->variance = psImageCopy(readoutOut->variance, readoutRaw->variance, PS_TYPE_F32);
    }
    if (readoutRaw->mask) {
	readoutOut->mask = psImageCopy(readoutOut->mask, readoutRaw->mask, PS_TYPE_IMAGE_MASK);
    }

    // pmChip *chipRaw = pmFPAviewThisChip(view, fileRaw->fpa); // The chip holds the PSF
    // psAssert (chipRaw, "missing chip");

    pmPSF *psf = psMetadataLookupPtr(&status, readoutRaw->analysis, "PSPHOT.PSF"); // PSF
    if (!psf) {
	// we should have a PSF by this point in psphot
	psError(PSPHOT_ERR_PROG, true, "Unable to find PSF.");
	return false;
    }

    float fwhmRaw = psphotPSFseeing (psf, readoutRaw, index);

    psVector *fwhmValues = psVectorAllocEmpty(10, PS_TYPE_F32);
    psVectorAppend(fwhmValues, fwhmRaw);

    // is a single target FWHM specified, or a set of values?  set up the vector options->targetSeeing and the local 1st value
    float targetSeeing = psMetadataLookupF32 (&status, recipe, "PSPHOT.STACK.TARGET.PSF.FWHM");
    if (!status) {
	psVector *targetSeeing = psMetadataLookupVector(&status, recipe, "PSPHOT.STACK.TARGET.PSF.FWHM"); // Magnitude offsets
	psAssert (status, "missing psphot recipe value PSPHOT.STACK.TARGET.PSF.FWHM");
	for (int i = 0; i < targetSeeing->n; i++) {
	    psVectorAppend(fwhmValues, targetSeeing->data.F32[i]);
	}	    
    } else {
        psVectorAppend(fwhmValues, targetSeeing);
    }

    psMetadataAddVector(readoutOut->analysis, PS_LIST_TAIL, "STACK.PSF.FWHM.VALUES", PS_META_REPLACE, "PSF sizes", fwhmValues);
    psFree (fwhmValues);

    return true;
}

float psphotPSFseeing (pmPSF *psf, pmReadout *readout, int index) {

    psImage *image = readout->image;

    int Nx = image->numCols;
    int Ny = image->numRows;

    float sumFWHM = 0.0;		  // FWHM for image
    int numFWHM = 0;			  // Number of FWHM measurements
    for (float x = 0; x < Nx; x += 0.25*Nx) {
	for (float y = 0; y < Ny; y += 0.25*Ny) {
	    float fwhm = pmPSFtoFWHM(psf, x, y);
	    if (isfinite(fwhm)) {
		sumFWHM += fwhm;
		numFWHM++;
	    }
	}
    }
    if (numFWHM == 0) {
	psLogMsg("ppStack", PS_LOG_INFO, "Unable to measure PSF FWHM for image %d --- rejected.", index);
	return NAN;
    } 

    float fwhm = sumFWHM / (float) numFWHM;
    psLogMsg ("psphotStack", PS_LOG_INFO, "Input Seeing for %d: %f\n", index, fwhm);

    return fwhm;
}

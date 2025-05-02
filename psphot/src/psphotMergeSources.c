# include "psphotInternal.h"

// Mask to apply for PSF sources : only exclude bad sources -- we will re-test for extendedness
#define PSF_SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_SATSTAR | PM_SOURCE_MODE_BLEND | \
                         PM_SOURCE_MODE_BADPSF | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_SATURATED | \
                         PM_SOURCE_MODE_CR_LIMIT)

// for now, let's store the detections on the readout->analysis for each readout
bool psphotMergeSources (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (!psphotMergeSourcesReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to merge sources for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// add newly selected sources to the existing list of sources
bool psphotMergeSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *newSources = detections->newSources;
    psAssert (newSources, "missing sources?");

    if (!detections->allSources) {
        detections->allSources = psArrayAllocEmpty(newSources->n);
    }
    psArray *allSources = detections->allSources;

    for (int i = 0; i < newSources->n; i++) {
        pmSource *source = newSources->data[i];
        psArrayAdd (allSources, 100, source);
    }

    psFree (detections->newSources);
    detections->newSources = NULL;

    return true;
}

// Merge the externally supplied sources with the existing sources.  Mark them as having mode
// PM_SOURCE_MODE_EXTERNAL.

// XXX this function needs to be updated slightly for psphotFullForce:
// * load the additional parameters to guide the new concepts

// XXX This function needs to be updated to loop over set of input files.  At the moment, we
// only expect a single entry for PSPHOT.INPUT.CMF and PSPHOT.SOURCES.TEXT, so we can only
// associate input sources with a single entry for the filerule
bool psphotLoadExtSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    bool status;
    pmDetections *extCMF = NULL;
    pmDetections *extCFF = NULL;
    psArray *extSourcesTXT = NULL;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
	detections = pmDetectionsAlloc();
	detections->newSources = psArrayAllocEmpty (100);
	// save detections on the readout->analysis
	if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detections)) {
	    psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
	    return false;
	}
    } else {
	psMemIncrRefCounter(detections); // so we can free the detections below
    }

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    // load data from input CMF file:
    {
        pmReadout *readoutCMF = pmFPAfileThisReadout (config->files, view, "PSPHOT.INPUT.CMF");
        if (!readoutCMF) goto loadCFF;

        extCMF = psMetadataLookupPtr (NULL, readoutCMF->analysis, "PSPHOT.DETECTIONS");
        if (extCMF) {
            for (int i = 0; i < extCMF->allSources->n; i++) {
                pmSource *source = extCMF->allSources->data[i];
                source->mode |= PM_SOURCE_MODE_EXTERNAL;
		source->tmpFlags = 0;
		// these flags are used to track the state of analysis of the sources, but
		// at this point, these sources are now effectively new so the flags
		// should be cleared.

                // the supplied peak flux needs to be re-normalized
                source->peak->rawFlux = 1.0;
                source->peak->smoothFlux = 1.0;
                source->peak->detValue = 1.0;

                // drop the loaded source modelPSF
                psFree (source->modelPSF);
                source->modelPSF = NULL;
		source->imageID = index;

                psArrayAdd (detections->newSources, 100, source);
            }
        }
    }

loadCFF:
    // load data from input CFF file:
    {
        pmReadout *readoutCFF = pmFPAfileThisReadout (config->files, view, "PSPHOT.INPUT.CFF");
        if (!readoutCFF) goto loadTXT;

        extCFF = psMetadataLookupPtr (NULL, readoutCFF->analysis, "PSPHOT.DETECTIONS");
        if (extCFF) {
            psF32 exptime = psMetadataLookupF32(NULL, readout->parent->concepts, "CELL.EXPOSURE");
            for (int i = 0; i < extCFF->allSources->n; i++) {
                pmSource *source = extCFF->allSources->data[i];

		// setting this bit not only tracks the inputs, it makes pmSourceMoments
		// keep the Mx,My values for the centroid.  
                source->mode |= PM_SOURCE_MODE_EXTERNAL;
		source->tmpFlags = 0;
		// these flags are used to track the state of analysis of the sources, but
		// at this point, these sources are now effectively new so the flags
		// should be cleared.

		// source->peak->detValue,rawFlux,smoothFlux all set to input flux value which is scaled
                // to 1 second exposure time. Scale to this image's exposure.
                source->peak->rawFlux    *= exptime;
                source->peak->smoothFlux *= exptime;
                source->peak->detValue   *= exptime;
		// source->peak->xf,yf, moments->Mx,My all set to input position

                // drop the loaded source modelPSF
                psFree (source->modelPSF);
                source->modelPSF = NULL;
		source->imageID = index;

                psArrayAdd (detections->newSources, 100, source);
            }
        }
    }

loadTXT:

    // load data from input TXT file:
    {
        pmChip *chipTXT = pmFPAfileThisChip (config->files, view, filerule);
        if (!chipTXT) goto finish;

        extSourcesTXT = psMetadataLookupPtr (NULL, chipTXT->analysis, "PSPHOT.SOURCES.TEXT");
        if (extSourcesTXT) {
            for (int i = 0; i < extSourcesTXT->n; i++) {
                pmSource *source = extSourcesTXT->data[i];
                source->mode |= PM_SOURCE_MODE_EXTERNAL;
		source->tmpFlags = 0;
		// these flags are used to track the state of analysis of the sources, but
		// at this point, these sources are now effectively new so the flags
		// should be cleared.

                // the supplied peak flux needs to be re-normalized
                source->peak->rawFlux = 1.0;
                source->peak->smoothFlux = 1.0;
                source->peak->detValue = 1.0;

                // drop the loaded source modelPSF
                psFree (source->modelPSF);
                source->modelPSF = NULL;
		source->imageID = index;

                psArrayAdd (detections->newSources, 100, source);
            }
        }
    }

finish:

    psFree (detections);

    if (!(extCMF || extCFF || extSourcesTXT)) {
        psLogMsg ("psphot", 3, "no external sources for this readout");
        return true;
    }

    int nCMF = extCMF        ? extCMF->allSources->n        : 0;
    int nCFF = extCFF        ? extCFF->allSources->n        : 0;
    int nTXT = extSourcesTXT ? extSourcesTXT->n             : 0;

    psLogMsg ("psphot", 3, "%d external sources (%d cmf, %d cff, %d text) merged to yield %ld total sources",
              nCMF + nCFF + nTXT, nCMF, nCFF, nTXT, sources->n);
    return true;
}
bool psphotLoadExtSources(pmConfig *config, const pmFPAview *view, const char *filerule) {
    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (!psphotLoadExtSourcesReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to load sources for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// copy the known sources (as external) to the detection list of the given filerule
bool psphotAddKnownSources (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources) {

    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // determine properties (sky, moments) of initial sources
    float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    psAssert (status, "missing SKY_OUTER_RADIUS in recipe?");

    // XXX this seems like an arbitrary number...
    OUTER = PS_MAX(OUTER, 20.0); // XXX Guarantee that we can encompass the max moments radius

    // find the currently selected readout 
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
	detections = pmDetectionsAlloc();
	detections->newSources = psArrayAllocEmpty (100);
	// save detections on the readout->analysis
	if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detections)) {
	    psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
	    return false;
	}
    } else {
	psMemIncrRefCounter(detections); // so we can free the detections below
    }

    // copy the sources from inSources to the new detection structure
    for (int i = 0; i < inSources->n; i++) {
      pmSource *inSource = inSources->data[i];

      pmSource *newSource = pmSourceCopy(inSource);
      newSource->mode |= PM_SOURCE_MODE_EXTERNAL;
      newSource->tmpFlags = 0;
      // these flags are used to track the state of analysis of the sources, but
      // at this point, these sources are now effectively new so the flags
      // should be cleared.
      
      // drop the loaded source modelPSF
      psFree (newSource->modelPSF);
      // source->modelPSF = NULL;  check this!

      // drop the references to the original image pixels:
      pmSourceFreePixels (newSource);

      // allocate image, weight, mask for the new image for each peak (square of radius OUTER)
      pmSourceDefinePixels (newSource, readout, newSource->peak->x, newSource->peak->y, OUTER);

      newSource->imageID = 0;
      // XXX reset the source ID? raised questions about the meaning of this ID...
      // P_PM_SOURCE_SET_ID(source, i);

      psArrayAdd (detections->newSources, 100, newSource);
    }
    psLogMsg ("psphot", 3, "%ld known sources supplied", detections->newSources->n);

    psFree (detections);
    return true;
}

// extract the input sources corresponding to this readout
// XXX this function needs to be updated to work with the new context of psphot inputs
psArray *psphotLoadPSFSources (pmConfig *config, const pmFPAview *view) {

    bool status;

    // find the currently selected readout
    pmReadout  *readout = pmFPAfileThisReadout (config->files, view, "PSPHOT.INPUT.CMF");
    if (!readout) {
        psLogMsg ("psphot", 3, "readout not found");
        return NULL;
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
        psLogMsg ("psphot", 3, "no psf sources for this readout");
        return NULL;
    }

    psArray *sources = detections->allSources;
    if (!sources) {
        psLogMsg ("psphot", 3, "no psf sources for this readout");
        return NULL;
    }

    return sources;
}

// this function is used to fix sources which were loaded externally, but have passed from
// psphotDetectionsFromSources to psphotSourceStats and are now stored on
// detections->newSources.
bool psphotRepairLoadedSources (pmConfig *config, const pmFPAview *view, const char *filerule) {

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (NULL, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
        psError(PSPHOT_ERR_CONFIG, false, "missing detections");
        return false;
    }

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    // peak flux is wrong : set based on previous image
    // use the peak measured in the moments analysis:
    for (int i = 0; i < sources->n; i++) {
      pmSource *source = sources->data[i];
      source->peak->rawFlux = source->moments->Peak;
      source->peak->smoothFlux = source->moments->Peak;
    }

    return true;
}

// generate the detection structure for the supplied array of sources
// XXX this currently assumes there is a single input file
bool psphotDetectionsFromSources (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *sources) {

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = pmDetectionsAlloc();

    detections->peaks = psArrayAllocEmpty(100);

    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }
    float snMin = psMetadataLookupF32(NULL, recipe, "MOMENTS_SN_MIN");
    if (!isfinite(snMin)) {
        return false;
    }

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        pmModel *model = source->modelPSF;

        if (source->mode & PSF_SOURCE_MASK || !isfinite(source->psfMag)) {
            continue;
        }

        // use the existing peak information, otherwise generate a new peak
        if (source->peak) {
            source->peak->assigned = false; // So the moments will be measured
            psArrayAdd (detections->peaks, 100, source->peak);
            continue;
        }

        float flux = powf(10.0, -0.4 * source->psfMag);
        float xpos = model->params->data.F32[PM_PAR_XPOS];
        float ypos = model->params->data.F32[PM_PAR_YPOS];

        pmPeak *peak = pmPeakAlloc(xpos, ypos, flux, PM_PEAK_LONE);
        peak->xf = xpos;
        peak->yf = ypos;

        psArrayAdd (detections->peaks, 100, peak);
        psFree (peak);
    }

    psLogMsg ("psphot", 3, "%ld PSF sources loaded", detections->peaks->n);
    psphotVisualShowSources (sources);
    psphotVisualShowPeaks (detections);

    // save detections on the readout->analysis
    if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detectinos", detections)) {
        psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
        return false;
    }
    psFree (detections);

    return true;
}

// generate the detection structure for the supplied array of sources
// XXX this function is currently unused
bool psphotSetSourceParams (pmConfig *config, psArray *sources, pmPSF *psf) {

    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        pmModel *model = source->modelPSF;

        if (source->mode & PSF_SOURCE_MASK || !isfinite(source->psfMag)) {
            continue;
        }

        float flux = powf(10.0, -0.4 * source->psfMag);
        float xpos = model->params->data.F32[PM_PAR_XPOS];
        float ypos = model->params->data.F32[PM_PAR_YPOS];

        pmPeak *peak = pmPeakAlloc(xpos, ypos, flux, PM_PEAK_LONE);
        peak->xf = xpos;
        peak->yf = ypos;
        peak->rawFlux = flux; // this are being set wrong, but does it matter?
        peak->smoothFlux = flux; // this are being set wrong, but does it matter?

        source->peak = peak;
    }

    psLogMsg ("psphot", 3, "%ld PSF sources loaded", sources->n);

    return true;
}

bool psphotCheckExtSources (pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status;

    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    // XXX allSources of newSources?
    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (sources->n) {
        // the user wants to make the psf from these stars; define them as psf stars:
        for (int i = 0; i < sources->n; i++) {
            pmSource *source = sources->data[i];
            source->mode |= PM_SOURCE_MODE_PSFSTAR;
        }
        // force psphotChoosePSF to use all loaded sources
        psMetadataAddS32 (recipe, PS_LIST_TAIL, "PSF_MAX_NSTARS", PS_META_REPLACE, "max number of sources for PSF model", sources->n);

        // measure stats of externally specified sources
        if (!psphotSourceStatsUpdate (sources, config, readout)) {
            psError(PSPHOT_ERR_CONFIG, false, "failure to measure stats of existing sources");
            return false;
        }
    } else {

        // find the detections (by peak and/or footprint) in the image.
        if (!psphotFindDetections (config, view, filerule, true)) {
            psError(PSPHOT_ERR_CONFIG, false, "unable to find detections in this image");
            return psphotReadoutCleanup (config, view, filerule);
        }

        // construct sources and measure basic stats
        psphotSourceStats (config, view, filerule, true);

        // find blended neighbors of very saturated stars
        psphotDeblendSatstars (config, view, filerule);

        // mark blended peaks PS_SOURCE_BLEND
        if (!psphotBasicDeblend (config, view, filerule)) {
            psLogMsg ("psphot", 3, "failed on deblend analysis");
            return psphotReadoutCleanup (config, view, filerule);
        }

        // classify sources based on moments, brightness
        if (!psphotRoughClass (config, view, filerule)) {
            psLogMsg ("psphot", 3, "failed to find a valid PSF clump for image");
            return psphotReadoutCleanup (config, view, filerule);
        }
    }

    return true;
}

// copy the detections from one pmFPAfile to another
bool psphotCopySources (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc)
{
    bool status = true;

    int num = psphotFileruleCount(config, ruleSrc);

    // skip the chisq image because it is a duplicate of the detection version
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
        if (!psphotCopySourcesReadout (config, view, ruleOut, ruleSrc, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to copy sources from %s to %s entry %d", ruleSrc, ruleOut, i);
            return false;
        }
    }
    return true;
}

// add newly selected sources to the existing list of sources
bool psphotCopySourcesReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, ruleSrc, index); // File of interest
    psAssert (fileSrc, "missing file?");

    pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
    psAssert (readoutSrc, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readoutSrc->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    // find the currently selected readout
    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, ruleOut, index); // File of interest
    psAssert (fileOut, "missing file?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    psAssert (readoutOut, "missing readout?");

    // save detections on the readout->analysis
    // XXX this replaced any existing entry; allow this operation to merge?
    if (!psMetadataAddPtr (readoutOut->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detections)) {
	psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
	return false;
    }

    // loop over the sources, redefine their pixels to point at the new filerule image,
    // copy the source data, and add a reference back to the original source
    

    return true;
}

// copy the newPeaks from the detections of one pmFPAfile to another
bool psphotCopyPeaks (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc)
{
    bool status = true;

    int num = psphotFileruleCount(config, ruleSrc);

    // skip the chisq image because it is a duplicate of the detection version
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
        if (!psphotCopyPeaksReadout (config, view, ruleOut, ruleSrc, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to copy peaks from %s to %s entry %d", ruleSrc, ruleOut, i);
            return false;
        }
    }
    return true;
}

// add newly detected peaks to the existing list of sources
bool psphotCopyPeaksReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, ruleSrc, index); // File of interest
    psAssert (fileSrc, "missing file?");

    pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
    psAssert (readoutSrc, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readoutSrc->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    // find the currently selected readout
    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, ruleOut, index); // File of interest
    psAssert (fileOut, "missing file?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    psAssert (readoutOut, "missing readout?");

    // generate a new detection structure for the output filerule
    pmDetections *detectionsOut = psMetadataLookupPtr (&status, readoutOut->analysis, "PSPHOT.DETECTIONS");
    psAssert (detectionsOut, "missing PSPHOT.DETECTIONS?");

    psAssert (detectionsOut->peaks, "programming error");
    psAssert (!detectionsOut->oldPeaks, "programming error");
    psAssert (detections->peaks, "programming error");

    // save the OUT existing peaks on oldPeaks
    detectionsOut->oldPeaks = detectionsOut->peaks;
    detectionsOut->peaks = psArrayAllocEmpty(detections->peaks->n);

    for (int i = 0; i < detections->peaks->n; i++) {
	psAssert (detections->peaks->data[i], "programming error");
	pmPeak *peak = pmPeakAlloc (0, 0, 0.0, PM_PEAK_LONE);
	pmPeakCopy(peak, detections->peaks->data[i]);
	psArrayAdd (detectionsOut->peaks, 100, peak);
	psFree (peak);
    }
    return true;
}

// create source parents children from ruleSrc for ruleOut for orphans
bool psphotSourceParents (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc)
{
    bool status = true;

    int num = psphotFileruleCount(config, ruleSrc);

    // skip the chisq image because it is a duplicate of the detection version
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
        if (!psphotSourceParentsReadout (config, view, ruleOut, ruleSrc, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to copy sources from %s to %s entry %d", ruleSrc, ruleOut, i);
            return false;
        }
    }
    return true;
}

// create source parents from ruleSrc for ruleOut for orphaned children for this readout.  
bool psphotSourceParentsReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index) {

    bool status;
    int nParents = 0;
    int nNonOrphans = 0;

    // find the currently selected readout
    pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, ruleSrc, index); // File of interest
    psAssert (fileSrc, "missing file?");

    pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
    psAssert (readoutSrc, "missing readout?");

    pmDetections *detectionsSrc = psMetadataLookupPtr (&status, readoutSrc->analysis, "PSPHOT.DETECTIONS");
    psAssert (detectionsSrc, "missing detections?");

    psArray *sourcesSrc = detectionsSrc->allSources;
    psAssert (sourcesSrc, "missing sources?");

    // find the currently selected readout
    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, ruleOut, index); // File of interest
    psAssert (fileOut, "missing file?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    psAssert (readoutOut, "missing readout?");

    // generate a new detection structure for the output filerule
    pmDetections *detectionsOut = psMetadataLookupPtr (&status, readoutOut->analysis, "PSPHOT.DETECTIONS");
    psAssert (detectionsOut, "missing PSPHOT.DETECTIONS?");

    // loop over the sources, redefine their pixels to point at the new filerule image,
    // copy the source data, and add a reference back to the original source
    
    // copy the sources from sourceSrcs to the new detection structure
    for (int i = 0; i < sourcesSrc->n; i++) {
      pmSource *sourceSrc = sourcesSrc->data[i];
      if (sourceSrc->parent) {
	  nNonOrphans ++;
	  continue; // Not an orphan
      }

      pmSource *sourceOut = pmSourceCopy(sourceSrc);
      sourceOut->parent = sourceSrc;
      
      // keep the original source flags
      sourceOut->seq      = sourceSrc->seq;
      sourceOut->type     = sourceSrc->type;
      sourceOut->mode     = sourceSrc->mode;
      sourceOut->mode2    = sourceSrc->mode2;
      sourceOut->tmpFlags = sourceSrc->tmpFlags;

      // does this copy all model data? (NO)
      sourceOut->modelPSF = pmModelCopy(sourceSrc->modelPSF);
      sourceOut->modelEXT = pmModelCopy(sourceSrc->modelEXT);

      if (sourceSrc->modelFits) {
	  sourceOut->modelFits = psArrayAlloc(sourceSrc->modelFits->n);
	  for (int j = 0; j < sourceSrc->modelFits->n; j++) {
	      sourceOut->modelFits->data[j] = pmModelCopy(sourceSrc->modelFits->data[j]);
	  }
      }

      // drop the references to the original image pixels:
      pmSourceFreePixels (sourceOut);

      // allocate image, weight, mask for the new image for each peak
      if (sourceOut->modelPSF) {
	pmSourceRedefinePixels (sourceOut, readoutOut, sourceOut->peak->x, sourceOut->peak->y, sourceOut->modelPSF->fitRadius);
      } else {
        // if we have no pixels we can't use it to determine the psf so make sure this bit is off
        sourceOut->tmpFlags &= ~PM_SOURCE_TMPF_CANDIDATE_PSFSTAR;
       }

      // child sources have not been subtracted in this image, but this flag may be raised if
      // they were subtracted in the parent's image
      sourceOut->tmpFlags &= ~PM_SOURCE_TMPF_SUBTRACTED;

      nParents ++;
      psArrayAdd (detectionsOut->allSources, 100, sourceOut);
      psFree (sourceOut);
    }
    psLogMsg ("psphot", 3, "%d parents created, %d unorphaned children, %ld input vs %ld output", nParents, nNonOrphans, sourcesSrc->n, detectionsOut->allSources->n);

    return true;
}

// create source children from ruleSrc for ruleOut
bool psphotSourceChildren (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc)
{
    bool status = true;

    int num = psphotFileruleCount(config, ruleSrc);

    // skip the chisq image because it is a duplicate of the detection version
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
        if (!psphotSourceChildrenReadout (config, view, ruleOut, ruleSrc, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to copy sources from %s to %s entry %d", ruleSrc, ruleOut, i);
            return false;
        }
    }
    return true;
}

// Create source children from ruleSrc for ruleOut for this entry.  Currently, this is only
// used by psphotStackReadout (sources go on allSources so that psphotChoosePSF can be called
// repeatedly).
bool psphotSourceChildrenReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, ruleSrc, index); // File of interest
    psAssert (fileSrc, "missing file?");

    pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
    psAssert (readoutSrc, "missing readout?");

    pmDetections *detectionsSrc = psMetadataLookupPtr (&status, readoutSrc->analysis, "PSPHOT.DETECTIONS");
    psAssert (detectionsSrc, "missing detections?");

    psArray *sourcesSrc = detectionsSrc->allSources;
    psAssert (sourcesSrc, "missing sources?");

    // find the currently selected readout
    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, ruleOut, index); // File of interest
    psAssert (fileOut, "missing file?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    psAssert (readoutOut, "missing readout?");

    pmDetections *detectionsOutOld = psMetadataLookupPtr (&status, readoutOut->analysis, "PSPHOT.DETECTIONS");
    psArray *oldFootprints = detectionsOutOld ? detectionsOutOld->footprints : NULL;

    // replace any existing DETECTION container on readoutOut->analysis with the new one
    pmDetections *detectionsOut = pmDetectionsAlloc();
    if (oldFootprints) {
        // ... but hang on to any existing footprints so that they can be merged with new footprints in pass 2
        detectionsOut->footprints = psMemIncrRefCounter(oldFootprints);
    }
    detectionsOut->allSources = psArrayAllocEmpty (100);
    if (!psMetadataAddPtr (readoutOut->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detectionsOut)) {
	psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
	return false;
    }

    // loop over the sources, redefine their pixels to point at the new filerule image,
    // copy the source data, and add a reference back to the original source
    
    // copy the sources from sourceSrcs to the new detection structure
    for (int i = 0; i < sourcesSrc->n; i++) {
      pmSource *sourceSrc = sourcesSrc->data[i];

      pmSource *sourceOut = pmSourceCopy(sourceSrc);
      sourceOut->parent = sourceSrc;
      
      // keep the original source flags
      sourceOut->seq      = sourceSrc->seq;
      sourceOut->type     = sourceSrc->type;
      sourceOut->mode     = sourceSrc->mode;
      sourceOut->mode2    = sourceSrc->mode2;
      sourceOut->tmpFlags = sourceSrc->tmpFlags;

      // does this copy all model data? (NO)
      sourceOut->modelPSF = pmModelCopy(sourceSrc->modelPSF);
      sourceOut->modelEXT = pmModelCopy(sourceSrc->modelEXT);

      if (sourceSrc->modelFits) {
	  sourceOut->modelFits = psArrayAlloc(sourceSrc->modelFits->n);
	  for (int j = 0; j < sourceSrc->modelFits->n; j++) {
	      sourceOut->modelFits->data[j] = pmModelCopy(sourceSrc->modelFits->data[j]);
	  }
      }

      // drop the references to the original image pixels:
      pmSourceFreePixels (sourceOut);

      // XXX do we need to skip the Chisq image sources?

      // allocate image, weight, mask for the new image for each peak
      pmSourceRedefinePixels (sourceOut, readoutOut, sourceOut->peak->x, sourceOut->peak->y, sourceOut->windowRadius);

      // child sources have not been subtracted in this image, but this flag may be raised if
      // they were subtracted in the parent's image
      sourceOut->tmpFlags &= ~PM_SOURCE_TMPF_SUBTRACTED;

      psArrayAdd (detectionsOut->allSources, 100, sourceOut);
      psFree (sourceOut);
    }
    psLogMsg ("psphot", 3, "created %ld children", detectionsOut->allSources->n);

    psFree(detectionsOut); // a copy remains on the analysis metadata

    return true;
}

// create source children associated with 'filerule' from the objectsSrc.  returns a new object
// array containing the child sources.  XXX currently, this is only used by psphotStackReadout
// (sources go on allSources so that psphotChoosePSF can be called repeatedly)
psArray *psphotSourceChildrenByObject (pmConfig *config, const pmFPAview *view, const char *fileruleOut, const char *fileruleSrc, psArray *objectsSrc, bool sourcesSubtracted) {

    bool status;

    int nImages = psphotFileruleCount(config, fileruleOut);

    // generate look-up arrays for detections and readouts
    psArray *detArrays = psArrayAlloc(nImages);
    psArray *readouts = psArrayAlloc(nImages);
    psArray *fitOptionsArray = psArrayAlloc(nImages);

    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);
    int psfSize  = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");
    assert (status);

    for (int i = 0; i < nImages; i++) {

	// find the currently selected readout
	pmFPAfile *file = pmFPAfileSelectSingle(config->files, fileruleOut, i); // File of interest
	psAssert (file, "missing file?");

	pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
	psAssert (readout, "missing readout?");

	pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, fileruleSrc, i); // File of interest
	psAssert (file, "missing file?");

	pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
	psAssert (readoutSrc, "missing readout?");


	// create DETECTIONS containers for each image, in case one lacks it
	pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
	if (!detections) {
	    detections = pmDetectionsAlloc();
	    detections->allSources = psArrayAllocEmpty (100);
	    detections->peaks = psArrayAllocEmpty (100);
	    // save detections on the readout->analysis
	    if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detections)) {
		psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
		return NULL;
	    }
	    psFree(detections); // a copy remains on the analysis metadata
	    psAssert (detections, "missing detections?");
	}
        pmSourceFitOptions *fitOptions = NULL;
        if (psMetadataLookupBool(&status, recipe, "EXTENDED_SOURCE_FITS")) {
            fitOptions = psMetadataLookupPtr (&status, readoutSrc->analysis, "PCM_FIT_OPTIONS");
            psAssert (fitOptions, "missing pcm fit options");
            psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PCM_FIT_OPTIONS", PS_DATA_UNKNOWN | PS_META_REPLACE, "pcm fit options", fitOptions);
        }

	// we need to save the new sources on the detection arrays of the appropriate image
	detArrays->data[i] = psMemIncrRefCounter(detections);
	readouts->data[i] = psMemIncrRefCounter(readout);
        if (fitOptions) {
            fitOptionsArray->data[i] = psMemIncrRefCounter(fitOptions);
        }
    }

    psArray *objectsOut = psArrayAlloc(objectsSrc->n);

    // copy all sources for each object
    for (int k = 0; k < objectsSrc->n; k++) {

        pmPhotObj *objectSrc = objectsSrc->data[k];
	if (!objectSrc) continue;
	if (!objectSrc->sources) continue;

	pmPhotObj *objectOut = pmPhotObjAlloc();
	objectsOut->data[k] = objectOut;

	objectOut->flux = objectSrc->flux;
	objectOut->x    = objectSrc->x;
	objectOut->y    = objectSrc->y;
	
	objectOut->sources = psArrayAlloc(objectSrc->sources->n);

	// copy the sources from sourceSrcs to the new detection structure
	// loop over the sources, redefine their pixels to point at the new filerule image,
	// copy the source data, and add a reference back to the original source
	for (int i = 0; i < objectSrc->sources->n; i++) {

	    pmSource *sourceSrc = objectSrc->sources->data[i];

	    pmSource *sourceOut = pmSourceCopy(sourceSrc);
	    sourceOut->parent = sourceSrc;

	    // save on the output object array at the same location
	    objectOut->sources->data[i] = sourceOut;

	    // keep the original source flags and sequence ID (if set)
	    sourceOut->seq      = sourceSrc->seq;
	    sourceOut->type     = sourceSrc->type;
	    sourceOut->mode     = sourceSrc->mode;
	    sourceOut->mode2    = sourceSrc->mode2;
	    sourceOut->tmpFlags = sourceSrc->tmpFlags;

	    // does this copy all model data? (NO)
	    sourceOut->modelPSF = pmModelCopy(sourceSrc->modelPSF);

            bool foundModelEXT = false;
	    if (sourceSrc->modelFits) {
		sourceOut->modelFits = psArrayAlloc(sourceSrc->modelFits->n);
		for (int j = 0; j < sourceSrc->modelFits->n; j++) {
                    pmModel *modelSrc = sourceSrc->modelFits->data[j];
		    pmModel *modelOut = sourceOut->modelFits->data[j] = pmModelCopy(modelSrc);
                    if (modelSrc == sourceSrc->modelEXT) {
                        foundModelEXT = true;
                        sourceOut->modelEXT = psMemIncrRefCounter (modelOut);
                    }
                    modelOut->isPCM = modelSrc->isPCM;
                }
	    }
            if (!foundModelEXT && sourceSrc->modelEXT) {
                // Will this ever happen?
                sourceOut->modelEXT = pmModelCopy(sourceSrc->modelEXT);
            }

	    // drop the references to the original image pixels:
	    pmSourceFreePixels (sourceOut);

	    // set the output readout
	    int index = sourceOut->imageID;
	    if (index >= readouts->n) continue; // skip the sources generated by the chisq image
	    pmReadout *readout = readouts->data[index];

            pmSourceFitOptions *fitOptions = fitOptionsArray->data[index];

	    // allocate image, weight, mask for the new image for each peak
	    if (sourceOut->modelPSF) {
                pmSourceRedefinePixels (sourceOut, readout, sourceOut->peak->x, sourceOut->peak->y, 
                                                                        sourceSrc->windowRadius);
	    } else {
                // if we have no pixels we can't use it to determine the psf so make sure this bit is off
                sourceOut->tmpFlags &= ~PM_SOURCE_TMPF_CANDIDATE_PSFSTAR;
            }

	    // child sources have not been subtracted in this image, but this flag may be raised if
	    // they were subtracted in the parent's image
	    // XXX NOTE : in the pre-20130914 version of psphotStack, we carried a copy of the pixels 
	    // generated before the subtraction took place (and then we smoothed to match the desired PSF).  
	    // in the new version, we copy the image after subtraction; we need to distinguish these cases
	    if (!sourcesSubtracted) {
		sourceOut->tmpFlags &= ~PM_SOURCE_TMPF_SUBTRACTED;
	    } else {
                bool isPSF = false;
                pmModel *model = pmSourceGetModel (&isPSF, sourceOut);
                if (model && sourceSrc->modelFlux) {
                    if (model->isPCM) {
                        pmPCMdata *pcm = pmPCMinit (sourceOut, fitOptions, model, maskVal, psfSize);
                        if (pcm) {
                            // pmPCMMakeModel (sourceOut, model, pcm->nsigma, maskVal, psfSize);
                            pmPCMCacheModel (sourceOut, maskVal, psfSize, pcm->nsigma);
                            psFree(pcm);
                        } else {
                            // What to do here? 
                            // psAssert (pcm, "pmPCMinit failed!");
                            psFree (sourceOut->modelEXT);
                            sourceOut->modelEXT = NULL;
                        }
                    } else {
                        pmSourceCacheModel (sourceOut, maskVal);
                    }
                }
            }

	    // set the output detections:
	    pmDetections *detectionsOut = detArrays->data[index];
	    psArrayAdd (detectionsOut->allSources, 100, sourceOut);
	    psArrayAdd (detectionsOut->peaks, 100, sourceOut->peak);
	}
    }

    for (int i = 0; i < nImages; i++) {
	pmDetections *detections = detArrays->data[i];
	psLogMsg ("psphot", 3, "%ld source children for image %d", detections->allSources->n, i);
    }

    psFree (detArrays);
    psFree (readouts);
    psFree (fitOptionsArray);

    return objectsOut;
}

bool psphotCopyEfficiency (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc)
{
    bool status = true;

    int num = psphotFileruleCount(config, ruleSrc);

    // skip the chisq image because it is a duplicate of the detection version
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
        if (!psphotCopyEfficiencyReadout (config, view, ruleOut, ruleSrc, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to copy detection efficiency from %s to %s entry %d", ruleSrc, ruleOut, i);
            return false;
        }
    }
    return true;
}

// add newly selected sources to the existing list of sources
bool psphotCopyEfficiencyReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, ruleSrc, index); // File of interest
    psAssert (fileSrc, "missing file?");

    pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
    psAssert (readoutSrc, "missing readout?");

    // find the currently selected readout
    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, ruleOut, index); // File of interest
    psAssert (fileOut, "missing file?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    psAssert (readoutOut, "missing readout?");

    pmDetEff *de = psMetadataLookupPtr(&status, readoutSrc->analysis, PM_DETEFF_ANALYSIS); // Detection efficiency
    if (!status || !de) {
        // nothing there
        return true;
    }

    // save DetEff on the readoutOut->analysis
    if (!psMetadataAddPtr (readoutOut->analysis, PS_LIST_TAIL, PM_DETEFF_ANALYSIS, PS_META_REPLACE | PS_DATA_UNKNOWN, "Detection efficiency", de)) {
	psError (PSPHOT_ERR_CONFIG, false, "problem saving Detection efficiency on readout");
	return false;
    }

    return true;
}

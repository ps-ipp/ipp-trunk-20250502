# include "psphotInternal.h"

// for now, let's store the detections on the readout->analysis for each readout
bool psphotReadoutCleanup (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    // remove internal pmFPAfiles, if created
    if (psErrorCodeLast() == (psErrorCode) PSPHOT_ERR_DATA) {
        psErrorStackPrint(stderr, "Error in the psphot readout analysis");
        psErrorClear();
    }
    if (psErrorCodeLast() != PS_ERR_NONE) {
        return false;
    }

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotReadoutCleanupReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on psphotReadoutCleanup for %s entry %d", filerule, i);
	    return false;
	}
    }

    // XXX move this to top of loop?
    pmKapaClose ();

    return true;
}

// psphotReadoutCleanup is called on exit from psphotReadout.  If the last raised error is
// not a DATA error, then there was a serious problem.  Only in this case, or if the fail
// on the stats measurement, do we return false
bool psphotReadoutCleanupReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status = true;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // when psphotReadoutCleanup is called, these are not necessarily defined
    pmPSF        *psf        = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psArray      *sources    = detections ? detections->allSources : NULL;
    // XXX where do we free these, in here (psMetadataRemove?)

    // XXX this is currently only set by psphotModelBackground / psphotModelBackgroundReadoutFileIndex / psphotModelBackgroundReadout
    // if the image background cannot be measured (no valid pixels)
    int quality = psMetadataLookupS32 (&status, readout->analysis, "PSPHOT_QUALITY");
    if (quality) {
        // if there is no stats file this will be a no-op
        psphotStatsFileSetQuality(quality);
    }

    // use the psf-model to measure FWHM stats
    if (psf) {
      if (!psphotPSFstatsSources (readout, sources, psf)) {
            psError(PSPHOT_ERR_PROG, false, "Failed to measure PSF shape parameters");
            return false;
        }
    }
    // otherwise, use the source moments to measure FWHM stats
    if (!psf && sources) {
        if (!psphotMomentsStats (readout, sources)) {
            psError(PSPHOT_ERR_PROG, false, "Failed to measure Moment shape parameters");
            return false;
        }
    }

    // Check to see if any sources were detected
    // This is not necessarily a quality error: e.g., ppSub
    if (0 && !psf && !sources) {
      psError(PSPHOT_ERR_DATA, false, "Unable to detect sources in the image");
      return false;
    }

    // Check to see if the image quality was measured
    // XXX not sure we want / need this test
    if (0 && !psf) {
        bool mdok;                      // Status of MD lookup
        int nIQ = psMetadataLookupS32(&mdok, recipe, "IQ_NSTAR"); // Number of stars for IQ measurement
        if (!mdok || nIQ <= 0) {
            psError(PSPHOT_ERR_DATA, false, "Unable to measure image quality");
            return false;
        }
    }

    // create an output header with stats results currently saved on readout->analysis
    psMetadata *header = psphotDefineHeader (readout->analysis);

    // write NSTARS to the image header
    psphotSetHeaderNstars (header, sources);

    // save the results of the analysis
    // this should happen way up stream (when needed?)
    psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSPHOT.HEADER",  PS_DATA_METADATA | PS_META_REPLACE, "header stats", header);

    if (psf) {
	// XXX this seems a little silly : we saved the psf on readout->analysis above, but now
	// we are moving it to chip->analysis.
        // save the psf for possible output.  if there was already an entry, it was loaded from external sources
        // the new one may have been updated or modified, so replace the existing entry.  We
        // are required to save it on the chip, but this will cause problems if we ever want to
        // run psphot on an unmosaiced image
        pmCell *cell = readout->parent;
        pmChip *chip = cell->parent;
        psMetadataAdd (chip->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_DATA_UNKNOWN | PS_META_REPLACE,  "psphot psf", psf);
    }

    if (psErrorCodeLast() != PS_ERR_NONE) {
        psErrorStackPrint(stderr, "unexpected remaining errors");
        abort();
    }

    psFree (header);
    if (quality) return false;

    return true;
}

// for now, let's store the detections on the readout->analysis for each readout
bool psphotReadoutCleanupMinimal (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    // remove internal pmFPAfiles, if created
    if (psErrorCodeLast() == (psErrorCode) PSPHOT_ERR_DATA) {
        psErrorStackPrint(stderr, "Error in the psphot readout analysis");
        psErrorClear();
    }
    if (psErrorCodeLast() != PS_ERR_NONE) {
        return false;
    }

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotReadoutCleanupReadoutMinimal (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on psphotReadoutCleanupMinimal for %s entry %d", filerule, i);
	    return false;
	}
    }

    // XXX move this to top of loop?
    pmKapaClose ();

    return true;
}

// psphotReadoutCleanupMinimal is called on exit from psphotReadoutMinimal.  If the last raised error is
// not a DATA error, then there was a serious problem.  Only in this case, or if the fail
// on the stats measurement, do we return false
bool psphotReadoutCleanupReadoutMinimal (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status = true;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // when psphotReadoutCleanupMinimal is called, these are not necessarily defined
    pmPSF        *psf        = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psArray      *sources    = detections ? detections->allSources : NULL;
    // XXX where do we free these, in here (psMetadataRemove?)

    // create an output header with stats results currently saved on readout->analysis
    psMetadata *header = psphotDefineHeader (readout->analysis);

    // write NSTARS to the image header
    psphotSetHeaderNstars (header, sources);

    // save the results of the analysis
    // this should happen way up stream (when needed?)
    psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSPHOT.HEADER",  PS_DATA_METADATA | PS_META_REPLACE, "header stats", header);

    if (psf) {
	// XXX this seems a little silly : we saved the psf on readout->analysis above, but now
	// we are moving it to chip->analysis.
        // save the psf for possible output.  if there was already an entry, it was loaded from external sources
        // the new one may have been updated or modified, so replace the existing entry.  We
        // are required to save it on the chip, but this will cause problems if we ever want to
        // run psphot on an unmosaiced image
        pmCell *cell = readout->parent;
        pmChip *chip = cell->parent;
        psMetadataAdd (chip->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_DATA_UNKNOWN | PS_META_REPLACE,  "psphot psf", psf);
    }

    if (psErrorCodeLast() != PS_ERR_NONE) {
        psErrorStackPrint(stderr, "unexpected remaining errors");
        abort();
    }

    psFree (header);
    return true;
}

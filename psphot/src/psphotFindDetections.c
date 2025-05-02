# include "psphotInternal.h"

// we store the detections on the readout->analysis for each readout this function finds new
// peaks and new footprints.  any old peaks are saved on oldPeaks.  the resulting footprint set
// contains all footprints (old and new)
bool psphotFindDetections (pmConfig *config, const pmFPAview *view, const char *filerule, bool firstPass)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Find Detections ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotFindDetectionsReadout (config, view, filerule, i, recipe, firstPass)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to find initial detections for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// smooth the image, search for peaks, optionally define footprints based on the peaks
bool psphotFindDetectionsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool firstPass) {

    bool status;
    int pass;
    float NSIGMA_PEAK = 25.0;
    int NMAX = 0;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels
    psAssert (markVal, "missing mark value?");

    maskVal |= markVal;

    // Use the new pmFootprints approach?
    const bool useFootprints = psMetadataLookupBool(NULL, recipe, "USE_FOOTPRINTS");
    const bool footprintUseUnsubtracted = psMetadataLookupBool(NULL, recipe, "FOOTPRINT_USE_UNSUBTRACTED");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    // on the initial pass, detections have not yet been allocated or saved on readout->analysis
    if (!detections) {
	// create the container
        detections = pmDetectionsAlloc();
	
	// save detections on the readout->analysis
	if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot detections", detections)) {
	    psError (PSPHOT_ERR_CONFIG, false, "problem saving detections on readout");
	    return false;
	}
    } else {
	psMemIncrRefCounter(detections); // so we can free the detections below
    }

    // first pass vs other: if this is the first pass, the code will use PEAKS_NSIGMA_LIMIT and
    // only attempt to detect PEAKS_NMAX entries.  If 'firstPass' is false, the code will
    // attempt to replace the subtracted sources in order to measure the footprints.  After
    // replacement, it is necessary to regenerate the significance image.  If no sources are
    // available, the code will skip the significance image regeneration step.

    bool replaceSourcesForFootprints = false;
    if (firstPass) {
        pass = 1;
        NSIGMA_PEAK = psMetadataLookupF32 (&status, recipe, "PEAKS_NSIGMA_LIMIT"); PS_ASSERT (status, NULL);
        NMAX = psMetadataLookupS32 (&status, recipe, "PEAKS_NMAX"); PS_ASSERT (status, NULL);
    } else {
        pass = 2;
        replaceSourcesForFootprints = footprintUseUnsubtracted;    
        NSIGMA_PEAK = psMetadataLookupF32 (&status, recipe, "PEAKS_NSIGMA_LIMIT_2"); PS_ASSERT (status, NULL);
        NMAX = 0; // unlimited number of peaks in final pass: allow a limit (PEAKS_NMAX_2) ?
    }

    float threshold = PS_SQR(NSIGMA_PEAK);

    // move the old peaks array (if it exists) to oldPeaks XXX generically, we should be able
    // to call this function an arbitrary number of times the old peaks are saved so they can
    // be freed later -- the have to be freed after psphotFindFootprints is called below, since
    // they are also owned by the oldFootprints, which are in turn merged into the new
    // footprints.  (what about the source->peak entry?)
    
    assert (detections->oldPeaks == NULL);
    detections->oldPeaks = detections->peaks;
    detections->peaks = NULL;

    // generate the smoothed significance image
    pmReadout *significance = psphotSignificanceImage (readout, recipe, maskVal);

    // display the log significance image
    psphotVisualShowLogSignificance (significance->variance, 0.0, 4.5);

    // display the significance image
    psphotVisualShowSignificance (significance->variance, 0.98*threshold, 1.02*threshold);

    // detect the peaks in the significance image
    int totalPeaks = 0;
    detections->peaks = psphotFindPeaks (significance, readout, recipe, threshold, NMAX, &totalPeaks, firstPass);
    psMetadataAddF32  (readout->analysis, PS_LIST_TAIL, "PEAK_THRESHOLD", PS_META_REPLACE, "Peak Detection Threshold", threshold);
    if (!detections->peaks) {
	// we only get a NULL peaks array due to a programming or config error. 
	// this will result in a failure.
	psFree (detections);
	psError (PSPHOT_ERR_CONFIG, false, "failed on peak search");
        return false;
    }
    // hard limit on number of peaks we will accept. (To avoid memory overload in psphotStack)
    int maxPeaks = psMetadataLookupS32 (&status, recipe, "PEAKS_NMAX_TOTAL"); PS_ASSERT (status, NULL);
    if (maxPeaks && (totalPeaks > maxPeaks)) {
	psFree (detections);
	psError (PSPHOT_ERR_DATA, true, "Too many peaks %d found PEAKS_NMAX_TOTAL: %d", totalPeaks, maxPeaks);
        return false;
    }

    // optionally merge peaks into footprints
    if (useFootprints) {
        if (replaceSourcesForFootprints) {
	    bool modified = false;
	    // subtract the noise for all sources including satstars
	    modified |= psphotAddOrSubNoiseReadout(config, view, filerule, index, recipe, false);
            modified |= psphotReplaceAllSourcesReadout (config, view, filerule, index, recipe, false);

	    // add in the satstars
	    modified |= psphotAddOrSubSatstarsReadout (config, view, filerule, index, recipe, true);

	    if (modified) {
		psFree (significance);
		significance = psphotSignificanceImage (readout, recipe, maskVal);
	    }

	    // display the significance image
	    psphotVisualShowSignificance (significance->variance, 0.98*threshold, 1.02*threshold);
        }

	psphotFindFootprints (detections, significance, readout, recipe, threshold, pass, maskVal);

        if (replaceSourcesForFootprints) {
            psphotRemoveAllSourcesReadout (config, view, filerule, index, recipe, false);

	    // subtract the satstars
	    psphotAddOrSubSatstarsReadout (config, view, filerule, index, recipe, false);
        }
    }

    // XXX do a second (or third?) pass with rebinning (to detected more extended sources)

    psFree (significance);

    // display the peaks and footprints
    psphotVisualShowPeaks (detections);
    psphotVisualShowFootprints (detections);

    psFree (detections);

    return true;
}

// if we use the footprints, the output peaks list contains both old and new peaks,
// otherwise it only contains the new peaks.

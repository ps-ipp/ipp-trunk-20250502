# include "psphotInternal.h"

bool psphotFindFootprints (pmDetections *detections, pmReadout *significance, pmReadout *readout, psMetadata *recipe, const float threshold, const int pass, psImageMaskType maskVal) {

    bool status;

    psTimerStart ("psphot.footprints");

    int npixMin = psMetadataLookupS32(&status, recipe, "FOOTPRINT_NPIXMIN");
    PS_ASSERT (status, false);

    int growRadius = 0;
    if (pass == 1) {
        growRadius = psMetadataLookupS32(&status, recipe, "FOOTPRINT_GROW_RADIUS");
    } else {
        growRadius = psMetadataLookupS32(&status, recipe, "FOOTPRINT_GROW_RADIUS_2");
    }
    PS_ASSERT (status, false);

    // find the raw footprints in the smoothed significance image & assign the peaks to those footprints
    psArray *footprints = pmFootprintsFind (significance->variance, threshold, npixMin);

    if (pmFootprintsAssignPeaks(footprints, detections->peaks) != PS_ERR_NONE) {
	psAbort ("inconsistent peaks and footprints");
    }

    // footprints now owns the peaks; after culling (below), we will rebuild the peaks array
    psFree (detections->peaks);

    psLogMsg ("psphot", PS_LOG_MINUTIA, "found %ld footprints: %f sec\n", footprints->n, psTimerMark ("psphot.footprints"));

    // optionally grow footprints isotropically by growRadius pixels
    if (growRadius > 0) {
        bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading for psImageConvolve
        psArray *tmp = pmFootprintArrayGrow(footprints, growRadius);
        psImageConvolveSetThreads(oldThreads);
        psLogMsg ("psphot", PS_LOG_MINUTIA, "grow footprint coverage by %d pixels, %ld footprints become %ld footprints: %f sec\n", growRadius, footprints->n, tmp ? tmp->n : 0, psTimerMark ("psphot.footprints"));
        if (tmp) {
            psFree(footprints);
            footprints = tmp;
        } else {
            psLogMsg ("psphot", PS_LOG_WARN, "pmFootprintArray grow returned NULL\n");
        }
            
    }

    if (pass == 2) {
        // merge in old peaks;
        const int includePeaks = 0x1 | 0x2; // i.e. both new and old footprints

        psArray *mergedFootprints = pmFootprintArraysMerge(detections->footprints, footprints, includePeaks);
        psLogMsg ("psphot", PS_LOG_MINUTIA, "merged %ld new footprints with %ld existing ones: %f sec\n", footprints->n, (detections->footprints ? detections->footprints->n : 0), psTimerMark ("psphot.footprints"));

        psFree(footprints);
        psFree(detections->footprints);

	// replace the merged footprints on the detection structure
        detections->footprints = mergedFootprints;
    } else {
        detections->footprints = footprints;
    }

    psphotCullPeaks(readout, significance, recipe, detections->footprints);
    detections->peaks = pmFootprintArrayToPeaks(detections->footprints);

    // psphotFootprintSaddles (readout, detections->footprints);
    // 
    // int nSaddle = 0;
    // for (int i = 0; i < detections->peaks->n; i++) {
    // 	pmPeak *peak = detections->peaks->data[i];
    // 	
    // 	if (peak->saddlePoints) nSaddle += peak->saddlePoints->n;
    // }
    // fprintf (stderr, "%d saddle points for %ld peaks\n", nSaddle, detections->peaks->n);

    psLogMsg ("psphot", PS_LOG_WARN, "%ld peaks, %ld total footprints: %f sec\n", detections->peaks->n, detections->footprints->n, psTimerMark ("psphot.footprints"));

    return detections;
}

bool psphotCheckFootprints (pmDetections *detections) {

    // check for messed up footprints in the old peaks
    for (int i = 0; i < detections->oldPeaks->n; i++) {
	pmPeak *peak = detections->oldPeaks->data[i];
	pmFootprint *footprint = peak->footprint;
	if (!footprint) continue;
	for (int j = 0; j < footprint->spans->n; j++) {
	    pmSpan *sp = footprint->spans->data[j];
	    psAssert (sp, "missing span");
	}
    }
    return true;
}

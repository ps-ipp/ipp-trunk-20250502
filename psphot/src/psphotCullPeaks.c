# include "psphotInternal.h"

/*
 * Cull a set of peaks contained in a psArray of pmFootprints
 */
psErrorCode
psphotCullPeaks(const pmReadout *readout,   // the image wherein lives the footprint
		const pmReadout *signifR, // smoothed image and significance
                const psMetadata *recipe,
                psArray *footprints) {  // array of pmFootprints

    bool status = false;

    float nsigma_delta = psMetadataLookupF32(&status, recipe, "FOOTPRINT_CULL_NSIGMA_DELTA");
    if (!status) {
        nsigma_delta = 0; // min.
    }
    float nsigma_min = psMetadataLookupF32(&status, recipe, "FOOTPRINT_CULL_NSIGMA_MIN");
    if (!status) {
        nsigma_min = 0;
    }
    float fPadding = psMetadataLookupF32(&status, recipe, "FOOTPRINT_CULL_NSIGMA_PAD");
    if (!status) {
        fPadding = 0;
    }
    const float skyStdev = psMetadataLookupF32(&status, readout->analysis, "SKY_STDEV");
    psAssert (status, "cannot find SKY_STDEV in readout->analysis");

    // the sky has been smoothed, so we need to scale down the raw sky stdev by this amount:
    float scaleFactor = psMetadataLookupF32(&status, readout->analysis, "SIGNIFICANCE_SCALE_FACTOR");
    if (!status) scaleFactor = 1.0;

    const float MIN_THRESHOLD = nsigma_min*skyStdev / sqrt(scaleFactor);
    
    // for saturated stars, we should be somewhat more agressive about culling: instead of
    // letting the height of the intervening col (saddle point) be set by the S/N of the image
    // pixel, it should be set to a fraction of the saturation value.  example for a
    // near-saturation pixel:
    // flux = 40k, sigma = 200
    // nsigma_delta = 4.0, nsigma_min = 1.0, fPadding = 0.01, 
    // fStdev = 0.005, stdev_pad = 0.011*40k = 400
    // threshold = 40k - 4*400 = 38400 
    // this gives too tight of a tolerance on the bright stars
    // in practice, we should make the threshold much lower.  
    // below I am using 0.05 * saturation (eg, 2000 DN above sky in a GPC1 image)

    float SATURATION = NAN;

    // do not completely trust the values in the header...
    float CELL_SATURATION = psMetadataLookupF32 (&status, readout->parent->concepts, "CELL.SATURATION");
    float MIN_SATURATION = psMetadataLookupF32 (&status, recipe, "DEBLEND_MIN_SATURATION");
    if (!status || !isfinite(MIN_SATURATION)) {
	MIN_SATURATION = 40000.0;
    }
    if (!isfinite(CELL_SATURATION)) {
	SATURATION = MIN_SATURATION;
    } else {
	SATURATION = PS_MAX(MIN_SATURATION, CELL_SATURATION);
    }
    float SAT_TEST_LEVEL = 0.50*SATURATION;
    float SAT_THRESHOLD  = 0.05*SATURATION;

# if (PM_PEAKS_CULL_WITH_SMOOTHED_IMAGE)
    psLogMsg ("psphot", PS_LOG_INFO, "Culling peaks from footprints using the smoothed image");
    bool setNaNsToZero = psMetadataLookupF32 (&status, recipe, "FOOTPRINT_SET_NANS_TO_ZERO");
    if (setNaNsToZero) {
        // Set NaN pixels to zero so that they do not considered in the footprint analysis
        // XXX: Currently the caller does nothing further with the significance readout so
        // modifiying the image does no harm
        // XXX: Note: signifR->weight is not used by pmFootprintCullPeaks so we don't
        // need to touch it
        psImageClipNaN(signifR->image, 0);
    }
        

# else
    psLogMsg ("psphot", PS_LOG_INFO, "Culling peaks from footprints using the raw (unsmoothed) image");
# endif

    for (int i = 0; i < footprints->n; i++) {
	pmFootprint *fp = footprints->data[i];
	if (fp->peaks == NULL) continue;
	if (fp->peaks->n == 0) continue;

	if (fp->npix > 30000) {
	    psLogMsg("psphot.cull.footprints", PS_LOG_WARN, "big footprint: %f %f to %f %f (%d pix)\n", fp->bbox.x0, fp->bbox.y0, fp->bbox.x1, fp->bbox.y1, fp->npix);
	}
	psTimerStart ("psphot.cull.footprints");

	pmPeak *brightPeak = fp->peaks->data[0];
	float max_threshold = SAT_TEST_LEVEL;
	if (brightPeak->rawFlux > SAT_TEST_LEVEL) {
	    max_threshold = SAT_THRESHOLD;
	    brightPeak->type = PM_PEAK_SUSPECT_SATURATION;
	}

# if (PM_PEAKS_CULL_WITH_SMOOTHED_IMAGE)
	// New (post r30869) style of culling using the smoothed image and variance (S/N)
	// if we cull using the significance image, then the definition of variance is different (thus the bool in arg 8)
	if (pmFootprintCullPeaks(signifR->image, signifR->variance, fp, nsigma_delta, fPadding, MIN_THRESHOLD, max_threshold, true) != PS_ERR_NONE) {
	    return psError(PS_ERR_UNKNOWN, false, "Culling pmFootprint %d", fp->id);
	}
# else 
	// Old (pre r30869) style of culling using the raw image and variance
	if (pmFootprintCullPeaks(readout->image, readout->variance, fp, nsigma_delta, fPadding, MIN_THRESHOLD, max_threshold, false) != PS_ERR_NONE) {
	     return psError(PS_ERR_UNKNOWN, false, "Culling pmFootprint %d", fp->id);
	}
# endif


	float dtime = psTimerMark ("psphot.cull.footprints");
	if (dtime > 1.0) {
	    psLogMsg("psphot.cull.footprints", PS_LOG_WARN, "slow cull for %d (%f sec)\n", i, dtime);
	}
    }
    return PS_ERR_NONE;
}

// psLogMsg ("psphot", PS_LOG_INFO, "%ld peaks, %ld total footprints: %f sec\n", detections->peaks->n, detections->footprints->n, 

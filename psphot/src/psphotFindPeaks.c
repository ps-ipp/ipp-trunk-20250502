# include "psphotInternal.h"

// Find peaks in the significance image above a threshold significance level.  The significance
// image must be constructed to represent (S/N)^2.  If nMax is non-zero, only return a maximum
// of nMax peaks
psArray *psphotFindPeaks (pmReadout *significance, pmReadout *readout, psMetadata *recipe, const float threshold, const int nMax, int *totalPeaks, bool firstPass) {

    bool status = false;

    psTimerStart ("psphot.peaks");

    psArray *peaks = NULL;

    bool useSignalImage = psMetadataLookupF32 (&status, recipe, "PEAKS_USE_SIGNAL_IMAGE"); PS_ASSERT (status, NULL);

    if (firstPass && useSignalImage) {
	// find the approximate smoothed signal image that corresponds to the desired threshold

	// find all pixels within 25% of the threshold:
	float minThresh = threshold * 0.80;
	float maxThresh = threshold * 1.25;

	psImage *smooth_sn = significance->variance;
	psImage *smooth_im = significance->image;

	psVector *SNvalues = psVectorAllocEmpty (1000, PS_TYPE_F32);
	for (int iy = 0; iy < smooth_im->numRows; iy++) {
	    for (int ix = 0; ix < smooth_im->numCols; ix++) {
		// select all valid pixels within the S/N range
		if (!isfinite(smooth_sn->data.F32[iy][ix])) continue;
		if (!isfinite(smooth_im->data.F32[iy][ix])) continue;
		if (smooth_sn->data.F32[iy][ix] < minThresh) continue;
		if (smooth_sn->data.F32[iy][ix] > maxThresh) continue;
		psVectorAppend (SNvalues, smooth_im->data.F32[iy][ix]);
            }
        }

	// what is the median of the selected values?
	psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);
	if (!psVectorStats(stats, SNvalues, NULL, NULL, 0)) {
	    // psVectorStats will only fail due to a programming error (e.g., invalid vector type)
	    psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
	    psFree (SNvalues);
	    return NULL;
	}
	psFree (SNvalues);

	if (!isfinite(stats->sampleMedian)) {
	    // could not determine relationship between SN threshold and image values
	    // XXX fall-back could be the standard analysis above.
	    psLogMsg ("psphot", PS_LOG_INFO, "failure to map SN to image values");
	    psFree (stats);
	    goto use_significance;
	}
	
	float alt_threshold = stats->sampleMedian;
	psFree (stats);

	peaks = pmPeaksInImage (significance->image, alt_threshold);
	if (peaks == NULL) {
	    // we only get a NULL peaks array due to a programming or config error. 
	    // this will result in a failure.
	    psError(PSPHOT_ERR_DATA, false, "no peaks found in this image");
	    return NULL;
	}
    }

use_significance:
    if (!peaks) {
	// peaks is NULL at this point if we did not use signal image (by choice or by failure).
	// find the peaks in the smoothed image
	// NOTE : significance->variance actually carries the detection S/N image
	peaks = pmPeaksInImage (significance->variance, threshold);
	if (peaks == NULL) {
	    // we only get a NULL peaks array due to a programming or config error. 
	    // this will result in a failure.
	    psError(PSPHOT_ERR_DATA, false, "no peaks found in this image");
	    return NULL;
	}
    }

    // return the total number of peaks found before the nMax limit is applied
    if (totalPeaks) {
        *totalPeaks = peaks->n;
    }

    if (peaks->n == 0) {
        // XXX do we need to set something in the readout->analysis to indicate that
        // we tried and failed to find peaks (something in the header data)
	psLogMsg ("psphot", PS_LOG_INFO, "no peaks found in this image");
	return peaks;
    }

    // Convert the peak values to S/N = sqrt(significance).
    // Get the peak flux from the unsmoothed image.
    // Rescale the peak position errors using the peak variance
    // The peak pixel coords are guaranteed to be on the image
    int row0 = readout->image->row0;
    int col0 = readout->image->col0;
    for (int i = 0; i < peaks->n; i++) {
        pmPeak *peak = peaks->data[i];
        peak->rawFlux = readout->image->data.F32[peak->y-row0][peak->x-col0];
	peak->rawFluxStdev = sqrt(readout->variance->data.F32[peak->y-row0][peak->x-col0]);
        peak->smoothFlux = significance->image->data.F32[peak->y-row0][peak->x-col0];
	peak->smoothFluxStdev = peak->smoothFlux / sqrt(significance->variance->data.F32[peak->y-row0][peak->x-col0]);
	// NOTE smoothFluxStdev is actually (sqrt(variance) / covar_factor)

	// do we need this or not?
        // peak->SN = sqrt(peak->detValue);

	if (readout->variance && isfinite (peak->dx)) {
	    peak->dx *= sqrt(readout->variance->data.F32[peak->y-row0][peak->x-col0]);
	}
	if (readout->variance && isfinite (peak->dy)) {
	    peak->dy *= sqrt(readout->variance->data.F32[peak->y-row0][peak->x-col0]);
	}
    }

    // limit the total number of returned peaks as specified
    psArraySort (peaks, pmPeaksSortByRawFluxDescend);
    if (nMax && (peaks->n > nMax)) {
	psArray *tmpPeaks = psArrayAllocEmpty (nMax);
	for (int i = 0; i < nMax; i++) {
	    psArrayAdd (tmpPeaks, 100, peaks->data[i]);
	}
	psFree (peaks);
	peaks = tmpPeaks;
    }

    // optional dump of all peak data 
    char *output = psMetadataLookupStr (&status, recipe, "PEAKS_OUTPUT_FILE");
    if (output && strcasecmp (output, "NONE")) {
        pmPeaksWriteText (peaks, output);
    }
    psLogMsg ("psphot", PS_LOG_WARN, "%ld peaks: %f sec\n", peaks->n, psTimerMark ("psphot.peaks"));

    return peaks;

}

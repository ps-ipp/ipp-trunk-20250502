# include "ppStatsInternal.h"

static bool warnNonFinite = false;     // Have we warned about non-finite values?

psExit ppStatsReadout(psMetadata *cellResults, // Metadata holding the chip results
                      pmReadout *readout,       // Cell for which to get statistics
                      int nReadout,     // readout number
                      ppStatsData *data,        // The data
                      const pmConfig *config // Configuration
    )
{
    assert(cellResults);
    assert(readout);
    assert(data);
    assert(config);

    /*** psphot and psastro put their results on the readout->analysis metadata (PSPHOT.HEADER,
         PSASTRO.HEADER).  we need to pull quantities of interest from those locations. to do
         this, we need to select the appropriate readout.  ***/

    // Extract Header and Concept values from the Cell and Readout->analysis level

    psString readoutName = NULL;
    psStringAppend (&readoutName, "READOUT.%02d", nReadout);

    // find or generate a metadata to hold the results
    bool mdok;                          // Status of MD lookup
    psMetadata *readoutResults = psMemIncrRefCounter(psMetadataLookupMetadata(&mdok, cellResults, readoutName));
    if (!mdok || !readoutResults) {
        readoutResults = psMetadataAlloc();
    }

    // Extract Header values
    if (psListLength(data->headers)) {
        // extract from data->analysis output MD entries
        if (psListLength(data->analysis)) {
            p_ppStatsGetAnalysis (readoutResults, data->headers, readout->analysis, data->analysis);
        }
    }

    // Do we want to measure pixel statistics?
    if (!data->doStats && !psListLength(data->summary)) {
        // Nothing further to do --- don't want to waste our time reading the data
        goto readoutDone;
    }

    if (!readout->image) {
        psLogMsg(__func__, PS_LOG_WARN, "No image associated with readout --- ignored.\n");
        goto readoutDone;
    }

    // Measure basic image statistics (means, stdevs, etc).
    if (data->sample <= 0.0) {
        if (!psImageStats(data->stats, readout->image, readout->mask, data->maskVal)) {
            psWarning("Unable to perform statistics on readout %s --- ignored.\n", readoutName);
            goto statsDone;
        }
    } else {
        // Apply sampling
        psImage *image = readout->image; // The image of interest
        psImage *mask = readout->mask; // The mask image
        int numSamples = data->sample * image->numCols * image->numRows; // Number of samples
        int sampleSpace = 1.0 / data->sample; // Space between samples
        psVector *sampleValues = psVectorAlloc(numSamples, PS_TYPE_F32); // Vector of samples
        psVector *sampleMask = psVectorAlloc(numSamples, PS_TYPE_VECTOR_MASK);  // Corresponding mask
	psVectorInit(sampleMask, 0);

        for (int i = 0; i < numSamples; i++) {
            int j = i * sampleSpace;
            int y = j / image->numCols;
            int x = j % image->numCols;
            sampleValues->data.F32[i] = image->data.F32[y][x];

	    // ignore the sampleMask if there is no input mask
	    if (!mask) continue;

	    // if this pixel is masked, set the sample mask
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & data->maskVal) {
		sampleMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
		continue;
	    }

	    // mask any unmasked NAN/INF values
            if (!isfinite(sampleValues->data.F32[i])) {
		// warn for the first unmasked NAN/INF value
		if (!warnNonFinite) {
		    psWarning("Unmasked non-finite value detected at %d,%d; suppressing further warnings", x, y);
		    warnNonFinite = true;
		}
                sampleMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
            } 
        }
        if (!psVectorStats(data->stats, sampleValues, NULL, sampleMask, 1)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats for readout %s", readoutName);
	}
	// XXX raise if we get a NAN? psWarning("Unable to perform statistics on readout %s.\n", readoutName);
        psFree(sampleValues);
        psFree(sampleMask);
    }

#define WRITE_STAT(SYMBOL, NAME, SOURCE) \
    if (data->stats->options & SYMBOL) { \
        psMetadataAddF32(readoutResults, PS_LIST_TAIL, NAME, 0, NULL, data->stats->SOURCE); \
    }

    WRITE_STAT(PS_STAT_SAMPLE_MEAN,     "SAMPLE_MEAN",     sampleMean);
    WRITE_STAT(PS_STAT_SAMPLE_MEDIAN,   "SAMPLE_MEDIAN",   sampleMedian);
    WRITE_STAT(PS_STAT_SAMPLE_STDEV,    "SAMPLE_STDEV",    sampleStdev);
    WRITE_STAT(PS_STAT_SAMPLE_SKEWNESS, "SAMPLE_SKEWNESS", sampleSkewness);
    WRITE_STAT(PS_STAT_SAMPLE_KURTOSIS, "SAMPLE_KURTOSIS", sampleKurtosis);
    WRITE_STAT(PS_STAT_SAMPLE_QUARTILE, "SAMPLE_LQ",       sampleLQ);
    WRITE_STAT(PS_STAT_SAMPLE_QUARTILE, "SAMPLE_UQ",       sampleUQ);
    WRITE_STAT(PS_STAT_ROBUST_MEDIAN,   "ROBUST_MEDIAN",   robustMedian);
    WRITE_STAT(PS_STAT_ROBUST_STDEV,    "ROBUST_STDEV",    robustStdev);
    WRITE_STAT(PS_STAT_ROBUST_QUARTILE, "ROBUST_LQ",       robustLQ);
    WRITE_STAT(PS_STAT_ROBUST_QUARTILE, "ROBUST_UQ",       robustUQ);
    WRITE_STAT(PS_STAT_ROBUST_QUARTILE, "ROBUST_N50",      robustN50);
    WRITE_STAT(PS_STAT_FITTED_MEAN,     "FITTED_MEAN",     fittedMean);
    WRITE_STAT(PS_STAT_FITTED_STDEV,    "FITTED_STDEV",    fittedStdev);
    WRITE_STAT(PS_STAT_CLIPPED_MEAN,    "CLIPPED_MEAN",    clippedMean);
    WRITE_STAT(PS_STAT_CLIPPED_STDEV,   "CLIPPED_STDEV",   clippedStdev);

    // measure other types of statistics tests

statsDone:
    // count saturated pixels
    if (psListLength(data->summary) > 0) {
        bool get_nSatPixels = false;
        bool get_fSatPixels = false;
        bool findNumGood = false;       // Return the number of good pixels?
        bool findFracGood = false;      // Return the fraction of good pixels?

        psListIterator *iterator = psListIteratorAlloc(data->summary, PS_LIST_HEAD, false);
        psString choice;
        while ((choice = psListGetAndIncrement(iterator))) {
            if (!strcasecmp(choice, "SAT_PIXEL_NUM"))  get_nSatPixels = true;
            if (!strcasecmp(choice, "SAT_PIXEL_FRAC")) get_fSatPixels = true;
            if (!strcasecmp(choice, "GOOD_PIXEL_NUM")) findNumGood = true;
            if (!strcasecmp(choice, "GOOD_PIXEL_FRAC")) findFracGood = true;
        }

        if (get_nSatPixels || get_fSatPixels) {
            // Find the saturation point
            float saturation = psMetadataLookupF32(&mdok, readout->parent->concepts,
                                                   "CELL.SATURATION"); // Saturation level
            if (!mdok || isnan(saturation)) {
                psWarning("CELL.SATURATION is not set --- unable to measure N_SAT_PIXELS.\n");
                if (get_nSatPixels) {
                    psMetadataAddS32(readoutResults, PS_LIST_TAIL, "SAT_PIXEL_NUM", 0, NULL, 0);
                }
                if (get_fSatPixels) {
                    psMetadataAddF32(readoutResults, PS_LIST_TAIL, "SAT_PIXEL_FRAC", 0, NULL, NAN);
                }
            } else {
                int nSatPixels = 0;
                for (int j = 0; j < readout->image->numRows; j++) {
                    for (int i = 0; i < readout->image->numCols; i++) {
                        if (readout->image->data.F32[j][i] >= saturation) {
                            nSatPixels ++;
                        }
                    }
                }
                if (get_nSatPixels) {
                    psMetadataAddS32(readoutResults, PS_LIST_TAIL, "SAT_PIXEL_NUM", 0,
                                     "Number of saturated pixels", nSatPixels);
                }
                if (get_fSatPixels) {
                    psMetadataAddF32(readoutResults, PS_LIST_TAIL, "SAT_PIXEL_FRAC", 0,
                                     "Fraction of saturated pixels",
                                     nSatPixels / (float)(readout->image->numRows * readout->image->numCols));
                }
            }
        }

        if (findNumGood || findFracGood) {
            if (!readout->mask || data->maskVal == 0) {
                psWarning("Number or fraction of good pixels requested, but no mask provided");
                if (findNumGood) {
                    psMetadataAddS32(readoutResults, PS_LIST_TAIL, "GOOD_PIXEL_NUM", 0, NULL, 0);
                }
                if (findFracGood) {
                    psMetadataAddF32(readoutResults, PS_LIST_TAIL, "GOOD_PIXEL_FRAC", 0, NULL, NAN);
                }
            } else {
                int numBad = 0;            // Number of bad pixels
                for (int j = 0; j < readout->mask->numRows; j++) {
                    for (int i = 0; i < readout->mask->numCols; i++) {
                        if (readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[j][i] & data->maskVal) {
                            numBad++;
                        }
                    }
                }

                int numTotal = readout->mask->numRows * readout->mask->numCols; // Total number of pixels
                int numGood = numTotal - numBad; // Number of good pixels
                if (findNumGood) {
                    psMetadataAddS32(readoutResults, PS_LIST_TAIL, "GOOD_PIXEL_NUM", 0,
                                     "Number of good pixels", numGood);
                }
                if (findFracGood) {
                    psMetadataAddF32(readoutResults, PS_LIST_TAIL, "GOOD_PIXEL_FRAC", 0,
                                     "Fraction of good pixels", numGood / (float)numTotal);
                }
            }
        }
    }

readoutDone:
    // Add the readout results to the cell
    p_ppStatsAddToHierarchy(readoutResults, cellResults, readoutName, "Results for readout");
    psFree (readoutResults);
    psFree (readoutName);
    return PS_EXIT_SUCCESS;
}

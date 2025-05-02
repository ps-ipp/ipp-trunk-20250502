#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmFPA.h"
#include "pmRemnance.h"

#define SIZE 30                         // Size of accumulation patch
#define THRESHOLD 20.0                   // Threshold above background

bool pmRemnance(pmReadout *ro,           ///< Readout with input image
                psImageMaskType maskVal,      ///< Value of mask
                psImageMaskType maskRem,       ///< Value to give remance
                int size,               ///< Size of accumulation patches
                float threshold         ///< Threshold for masking
    )
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);
    PM_ASSERT_READOUT_MASK(ro, false);

    psImage *image = ro->image;
    psImage *mask = ro->mask; // Mask and image from readout

    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics
    if (!psImageBackground(stats, NULL, image, mask, maskVal, rng)) {
        // Probably means the entire readout is bad
        psErrorClear();
        psWarning("Unable to calculate image statistics: masking entire readout.");
        psBinaryOp(mask, mask, "|", psScalarAlloc(maskRem, PS_TYPE_IMAGE_MASK));
        psFree(stats);
        psFree(rng);
        return true;
    }
    psFree(rng);
    float bgMean = stats->robustMedian; // Background level
    float bgStdev = stats->robustStdev; // Background stdev

    stats->options = PS_STAT_SAMPLE_MEDIAN;

    int numMasked = 0;                  // Number of pixels masked
    int number = ceil(numRows / (float)SIZE); // Number of steps up the columns
    psVector *values = psVectorAlloc(numRows, PS_TYPE_F32); // Values below center
    for (int x = 0; x < numCols; x++) {
        int maxMask = 0;                // Maximum row to which to mask
        int numValues = 0;              // Number of values
        bool done = false;              // Are we done yet?
        for (int i = 0, min = 0, max = size; i < number && !done; i++, min += size, max += size) {

            if (max > numRows) {
                max = numRows;
            }
            for (int y = min; y < max; y++) {
                if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                    continue;
                }
                values->data.F32[numValues++] = image->data.F32[y][x];
            }
            values->n = numValues;
            if (!psVectorStats(stats, values, NULL, NULL, 0)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		return false;
	    }
	    if (isnan(stats->sampleMedian)) {
                maxMask = max;
                continue;
            }
            float median = stats->sampleMedian;

            if (median > bgMean + threshold * bgStdev / sqrtf(numValues)) {
                maxMask = max;
            } else {
                done = true;
            }
        }

        if (maxMask > 0) {
            maxMask += size;
            if (maxMask > numRows) {
                maxMask = numRows;
            }
            for (int y = 0; y < maxMask; y++) {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskRem;
            }
            numMasked += maxMask;
        }

    }
    psFree(values);
    psFree(stats);


    psMetadataAddS32(ro->analysis, PS_LIST_TAIL, PM_REMNANCE_ANALYSIS_NUM, 0,
                     "Number of remnance pixels masked", numMasked);

    return true;
}

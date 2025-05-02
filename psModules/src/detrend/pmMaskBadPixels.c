#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPAMaskWeight.h"
#include "pmMaskBadPixels.h"

bool pmMaskBadPixels(pmReadout *input, const pmReadout *mask, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(input, false);
    PS_ASSERT_PTR_NON_NULL(input->mask, false);
    PS_ASSERT_IMAGE_TYPE(input->mask, PS_TYPE_IMAGE_MASK, false);

    PS_ASSERT_PTR_NON_NULL(mask, false);
    PS_ASSERT_PTR_NON_NULL(mask->mask, false);
    PS_ASSERT_IMAGE_TYPE(mask->mask, PS_TYPE_IMAGE_MASK, false);

    psImage *inMask = input->mask;
    psImage *exMask = mask->mask;

    // Add mask MD5 to header
    pmHDU *hdu = pmHDUFromReadout(input);  // HDU of interest
    psVector *md5 = psImageMD5(mask->mask); // md5 hash
    psString md5string = psMD5toString(md5); // String
    psFree(md5);
    psStringPrepend(&md5string, "MASK image MD5: ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     md5string, "");
    psFree(md5string);

    int rowMax = input->row0 + inMask->numRows;
    int colMax = input->col0 + inMask->numCols;

    if (mask->row0 > input->row0 || mask->col0 > input->col0 ||
            mask->row0 + exMask->numRows < rowMax || mask->col0 + exMask->numCols < colMax) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Input image size exceeds that of mask image: (%d, %d) vs (%d, %d)",
                inMask->numRows, inMask->numCols, exMask->numRows, exMask->numCols);
        return false;
    }

    // Determine total offset based on image offset with chip offset
    // XXX if we choose to correct for the readout location, apply input->col0,row0 here
    int offCol = input->col0 - mask->col0;
    int offRow = input->row0 - mask->row0;

    // masks are both of type PS_TYPE_IMAGE_MASK
    psImageMaskType **exVal = exMask->data.PS_TYPE_IMAGE_MASK_DATA;
    psImageMaskType **inVal = inMask->data.PS_TYPE_IMAGE_MASK_DATA;

    // apply exMask values
    if (maskVal) {
        // set raised pixels in exMask which are selected by maskVal
        for (int j = 0; j < inMask->numRows; j++) {
            int xJ = j - offRow;
            for (int i = 0; i < inMask->numCols; i++) {
                int xI = i - offCol;
                inVal[j][i] |= (exVal[xJ][xI]);
            }
        }
    }

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now, used for reporting
    psString timeString = psTimeToISO(time); // String with time
    psFree(time);
    psStringPrepend(&timeString, "Static mask (selecting %x) applied at ", maskVal);
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     timeString, "");
    psFree(timeString);

    return true;
}


bool pmMaskFlagSuspectPixelsBySigma(pmReadout *output, const pmReadout *readout, float median, float stdev,
				    float rej, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, false);
    PS_ASSERT_IMAGE_NON_EMPTY(readout->image, false);
    PS_ASSERT_IMAGE_TYPE(readout->image, PS_TYPE_F32, false);
    if (readout->mask) {
        PS_ASSERT_IMAGE_NON_EMPTY(readout->mask, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(readout->image, readout->mask, false);
        PS_ASSERT_IMAGE_TYPE(readout->mask, PS_TYPE_IMAGE_MASK, false);
    }
    PS_ASSERT_PTR_NON_NULL(output, false);

    bool mdok;                          // Status of MD lookup
    psImage *suspect = psMetadataLookupPtr(&mdok, output->analysis, PM_MASK_ANALYSIS_SUSPECT); // Suspect img
    if (suspect) {
        PS_ASSERT_IMAGE_NON_EMPTY(suspect, false);
        PS_ASSERT_IMAGE_TYPE(suspect, PS_TYPE_F32, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(readout->image, suspect, false);
        psMemIncrRefCounter(suspect);
    } else {
        suspect = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
        psImageInit(suspect, 0);
        psMetadataAddImage(output->analysis, PS_LIST_TAIL, PM_MASK_ANALYSIS_SUSPECT, PS_META_REPLACE,
                           "Suspect pixels", suspect);
        psMetadataAddS32(output->analysis, PS_LIST_TAIL, PM_MASK_ANALYSIS_NUM, PS_META_REPLACE,
                         "Number of input images", 0);
    }

    if (!isfinite(median) || !isfinite(stdev)) {
        // If we get down here and the statistics are missing, then we should go and mask the entire image
        psWarning("Missing statistics --- flagging entire image as suspect.");
        psBinaryOp (suspect, suspect, "+", psScalarAlloc(1.0, PS_TYPE_F32));
        return true;
    }

    psImage *image = readout->image;    // Image of interest
    psImage *mask = readout->mask;      // Corresponding mask

    psTrace ("psModules.detrend", 3, "suspect: %f +/- %f\n", median, stdev);

    // XXX this loop could result in pixels with suspect = 0.0 but no valid input pixels (all
    // masked).  need to track the number of good as well as suspect pixels?
    for (int y = 0; y < image->numRows; y++) {
        for (int x = 0; x < image->numCols; x++) {
            if (fabs((image->data.F32[y][x] - median) / stdev) < rej) continue;
	    if (mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) continue;
	    suspect->data.F32[y][x] += 1.0;
        }
    }
    psFree(suspect);                    // Drop reference

    psMetadataItem *numItem = psMetadataLookup(output->analysis, PM_MASK_ANALYSIS_NUM); // Item with number
    assert(numItem);
    numItem->data.S32++;

    return true;
}

bool pmMaskFlagSuspectPixelsByValue(pmReadout *output, const pmReadout *readout, 
				    float min, float max, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, false);
    PS_ASSERT_IMAGE_NON_EMPTY(readout->image, false);
    PS_ASSERT_IMAGE_TYPE(readout->image, PS_TYPE_F32, false);
    if (readout->mask) {
        PS_ASSERT_IMAGE_NON_EMPTY(readout->mask, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(readout->image, readout->mask, false);
        PS_ASSERT_IMAGE_TYPE(readout->mask, PS_TYPE_IMAGE_MASK, false);
    }
    PS_ASSERT_PTR_NON_NULL(output, false);

    bool mdok;                          // Status of MD lookup
    psImage *suspect = psMetadataLookupPtr(&mdok, output->analysis, PM_MASK_ANALYSIS_SUSPECT); // Suspect img
    if (suspect) {
        PS_ASSERT_IMAGE_NON_EMPTY(suspect, false);
        PS_ASSERT_IMAGE_TYPE(suspect, PS_TYPE_F32, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(readout->image, suspect, false);
        psMemIncrRefCounter(suspect);
    } else {
        suspect = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
        psImageInit(suspect, 0);
        psMetadataAddImage(output->analysis, PS_LIST_TAIL, PM_MASK_ANALYSIS_SUSPECT, PS_META_REPLACE,
                           "Suspect pixels", suspect);
        psMetadataAddS32(output->analysis, PS_LIST_TAIL, PM_MASK_ANALYSIS_NUM, PS_META_REPLACE,
                         "Number of input images", 0);
    }

    psImage *image = readout->image;    // Image of interest
    psImage *mask = readout->mask;      // Corresponding mask

    psTrace ("psModules.detrend", 3, "suspect: < %f or > %f\n", min, max);
    int nFlagged = 0;

    // XXX this loop could result in pixels with suspect = 0.0 but no valid input pixels (all
    // masked).  need to track the number of good as well as suspect pixels?
    for (int y = 0; y < image->numRows; y++) {
        for (int x = 0; x < image->numCols; x++) {
	    bool above = image->data.F32[y][x] > max;
	    bool below = image->data.F32[y][x] < min;
	    if (!above && !below) continue;
	    if (mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) continue;
	    suspect->data.F32[y][x] += 1.0;
	    nFlagged ++;
        }
    }
    psFree(suspect);                    // Drop reference

    psMetadataItem *numItem = psMetadataLookup(output->analysis, PM_MASK_ANALYSIS_NUM); // Item with number
    assert(numItem);
    numItem->data.S32++;

    psTrace ("psModules.detrend", 3, "Number of flagged pixels: %d\n", nFlagged);

    return true;
}

// the maskVal supplied here is the value SET for this mask (ie, it is not used to avoid pixels)
bool pmMaskIdentifyBadPixels(pmReadout *output, psImageMaskType maskVal, float thresh, pmMaskIdentifyMode mode)
{
    PS_ASSERT_PTR_NON_NULL(output, false);
    psImage *suspects = psMetadataLookupPtr(NULL, output->analysis, PM_MASK_ANALYSIS_SUSPECT); // Suspect img
    if (!suspects) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find image with suspected bad pixels.");
        return false;
    }
    PS_ASSERT_IMAGE_NON_EMPTY(suspects, false);
    PS_ASSERT_IMAGE_TYPE(suspects, PS_TYPE_F32, false);
    if (output->mask) {
        PS_ASSERT_IMAGE_NON_EMPTY(output->mask, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(output->mask, suspects, false);
        PS_ASSERT_IMAGE_TYPE(output->mask, PS_TYPE_IMAGE_MASK, false);
    } else {
        output->mask = psImageAlloc(suspects->numCols, suspects->numRows, PS_TYPE_IMAGE_MASK);
    }
    int num = psMetadataLookupS32(NULL, output->analysis, PM_MASK_ANALYSIS_NUM); // Number of inputs
    PS_ASSERT_INT_POSITIVE(num, false);

    float limit = NAN;                  // Limit for masking
    switch (mode) {
      case PM_MASK_ID_VALUE:
        limit = thresh;
        break;

      case PM_MASK_ID_FRACTION:
        limit = thresh * num;
        break;

      case PM_MASK_ID_SIGMA: {
        psStats *stats = psStatsAlloc(PS_STAT_CLIPPED_STDEV); // Statistics
        stats->clipSigma = 5.0;
        stats->clipIter = 1;
        if (!psImageStats(stats, suspects, NULL, 0) || !isfinite(stats->clippedStdev)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to perform statistics.\n");
            psFree(stats);
            return NULL;
        }
        limit = thresh * stats->clippedStdev;
        psTrace ("psModules.detrend", 3, "bad: %f -> %f\n", stats->clippedStdev, limit);
        psFree(stats);
        break;
      }

      case PM_MASK_ID_POISSON: {
        psStats *stats = psStatsAlloc(PS_STAT_MAX); // Statistics
        if (!psImageStats(stats, suspects, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to perform statistics.\n");
            psFree(stats);
            return NULL;
        }
        psHistogram *histo = psHistogramAlloc(-0.5, stats->max + 0.5, stats->max + 1);
        psFree(stats);
        if (!psImageHistogram(histo, suspects, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to generate histogram.\n");
            psFree(histo);
            return NULL;
        }

        // Find the mode.  Since this is a Poisson distribution (more or less), this should also be the mean
        // and variance.
        int max = 0;                    // Index of the mode
        for (int i = 0; i < histo->nums->n; i++) {
            if (histo->nums->data.F32[i] > histo->nums->data.F32[max]) {
                max = i;
            }
        }
	psFree(histo);

        // Since the mode is most likely zero, we add one to get something realistic.  Then "thresh" is
        // negative, so we subtract instead of add.
        limit = max + 1.0 - thresh * sqrtf((float)max + 1.0);

        psTrace ("psModules.detrend", 3, "bad: mode: %d, stdev: %f, limit: %f\n",
                 max, sqrtf((float)max + 1.0), limit);
        break;
      }
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Invalid mask identify mode");
        return NULL;
    }

    if (psTraceGetLevel("psModules.detrend") > 9) {
        psStats *stats = psStatsAlloc(PS_STAT_MIN | PS_STAT_MAX); // Statistics
        psImageStats(stats, suspects, NULL, 0);
        psHistogram *histo = psHistogramAlloc(-0.5, stats->max + 0.5, stats->max + 1);
        psImageHistogram(histo, suspects, NULL, 0);
        for (int i = 0; i < histo->nums->n; i++) {
            printf("%f --> %f : %f\n", histo->bounds->data.F32[i], histo->bounds->data.F32[i + 1],
                   histo->nums->data.F32[i]);
        }
        psFree(stats);
        psFree(histo);
        printf("Threshold: %f\n", limit);
    }

    psTrace ("psModules.detrend", 3, "bad pixel threshold: %f", limit);

    psImage *badpix = output->mask;     // Bad pixel mask
    psImageInit(badpix, 0);

    for (int y = 0; y < suspects->numRows; y++) {
        for (int x = 0; x < suspects->numCols; x++) {
            if (suspects->data.F32[y][x] >= limit) {
                badpix->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = maskVal;
            }
        }
    }

    return true;
}

pmMaskIdentifyMode pmMaskIdentifyModeFromString (const char *string) {

    if (!strcasecmp(string, "VALUE")) {
      return PM_MASK_ID_VALUE;
    }
    if (!strcasecmp(string, "FRACTION")) {
      return PM_MASK_ID_FRACTION;
    }
    if (!strcasecmp(string, "SIGMA")) {
      return PM_MASK_ID_SIGMA;
    }
    if (!strcasecmp(string, "POISSON")) {
      return PM_MASK_ID_POISSON;
    }
    return PM_MASK_ID_NONE;
}

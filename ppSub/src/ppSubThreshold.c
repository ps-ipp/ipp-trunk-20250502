/** @file ppSubThreshold.c
 *
 *  @brief Mask pixels below threshold
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.3 $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"

// Apply low threshold to image
static bool lowThreshold(
    pmReadout *ro,                      // Readout to threshold
    float thresh,                       // Threshold to apply
    psImageMaskType maskIgnore,         // Ignore pixels with this mask
    psImageMaskType maskThresh,         // Give pixels this mask if below threshold
    psRegion *region,                   // Region of interest
    const char *description             // Description of image
    )
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);
    PM_ASSERT_READOUT_MASK(ro, false);
    PS_ASSERT_FLOAT_LARGER_THAN(thresh, 0.0, false);

    psImage *image = ro->image;         // Image
    psImage *mask = ro->mask;           // Mask

    psImage *subImage = psImageSubset(image, *region); // Image with region of interest
    psImage *subMask = psImageSubset(mask, *region);  // Maks with region of interest

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);                               // Random number generator
    if (!psImageBackground(stats, NULL, subImage, subMask, maskIgnore, rng)) {
        psError(PPSUB_ERR_DATA, false, "Unable to determine threshold.");
        psFree(rng);
        psFree(stats);
        return false;
    }
    psFree(rng);

    psFree(subImage);
    psFree(subMask);

    float threshold = stats->robustMedian - thresh * stats->robustStdev; // Threshold below which to clip
    psFree(stats);
    psLogMsg("ppSub", PS_LOG_INFO, "Masking pixels below %f in %s", threshold, description);

    for (int y = region->y0; y < region->y1; y++) {
        for (int x = region->x0; x < region->x1; x++) {
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskIgnore) {
                continue;
            }
            if (image->data.F32[y][x] < threshold) {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskThresh;
            }
        }
    }

    return true;
}


bool ppSubLowThreshold(ppSubData *data)
{
    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");

    psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Bits to mask in inputs
    psImageMaskType maskThresh = pmConfigMaskGet("LOW", config); // Bits to mask for low pixels

    float thresh = psMetadataLookupF32(NULL, recipe, "LOW.THRESHOLD"); // Threshold for masking
    if (!isfinite(thresh)) {
        return true;
    }

    pmFPAview *view = ppSubViewReadout(); // View to readout

    // Input images
    bool noConvolve = psMetadataLookupBool(NULL, recipe, "NOCONVOLVE");

    pmReadout *in;
    if (noConvolve) {
      in = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT"); // Input image
    }
    else {
      in = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV"); // Input image
    }
    if (!in) {
        psError(PPSUB_ERR_UNKNOWN, false, "Unable to find readout.");
	psFree(view);
        return false;
    }

    pmReadout *ref;
    if (noConvolve) {
      ref = pmFPAfileThisReadout(config->files, view, "PPSUB.REF"); // Reference image
    }
    else {
      ref = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV"); // Reference image
    }
    if (!ref) {
        psError(PPSUB_ERR_UNKNOWN, false, "Unable to find readout.");
	psFree(view);
        return false;
    }

    psMetadataIterator *regIter = psMetadataIteratorAlloc(in->analysis, PS_LIST_HEAD,
                                                          "^" PM_SUBTRACTION_ANALYSIS_REGION "$");
    psMetadataItem *regItem;        // Item with region
    while ((regItem = psMetadataGetAndIncrement(regIter))) {
        psAssert(regItem->type == PS_DATA_REGION && regItem->data.V, "Expect region type");
        psRegion *region = regItem->data.V; // Region of interest
        if (!lowThreshold(in, thresh, maskVal, maskThresh, region, "input convolved image")) {
            psError(psErrorCodeLast(), false, "Unable to threshold input image.");
	    psFree(view);
            return false;
        }
        if (!lowThreshold(ref, thresh, maskVal, maskThresh, region, "reference convolved image")) {
            psError(psErrorCodeLast(), false, "Unable to threshold input image.");
	    psFree(view);
            return false;
        }
    }
    psFree(regIter);

    psFree(view);

    return true;
}

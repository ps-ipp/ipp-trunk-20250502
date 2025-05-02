/** @file pswarpPixelFraction.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "pswarp.h"

bool pswarpPixelsLit(const pmReadout *readout, psMetadata *stats, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, false);
    PS_ASSERT_IMAGE_TYPE(readout->image, PS_TYPE_F32, false);
    if (!readout->mask) {
        // Can't do anything
        return true;
    }
    PS_ASSERT_IMAGE_NON_NULL(readout->mask, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(readout->mask, readout->image, false);
    PS_ASSERT_IMAGE_TYPE(readout->mask, PS_TYPE_IMAGE_MASK, false);

    if (!stats) {
        // No point in continuing --- we record results to the statistics
        return true;
    }
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_METADATA_NON_NULL(config->arguments, false);

    bool status;

    // load the recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    // output mask bits
    psImageMaskType maskValue = psMetadataLookupImageMask(&status, recipe, "MASK.OUTPUT");
    psAssert(status, "MASK.OUTPUT was not defined");

    psImage *image = readout->image;    ///< Image of interest
    psImage *mask = readout->mask;      ///< Mask image

    int numCols = image->numCols, numRows = image->numRows; ///< Size of image

    // Range of valid pixels
    int xMin = INT_MAX, xMax = -INT_MAX, yMin = INT_MAX, yMax = -INT_MAX;

    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
	    if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskValue) { continue; }
	    xMin = PS_MIN (xMin, x);
	    xMax = PS_MAX (xMax, x);
	    yMin = PS_MIN (yMin, y);
	    yMax = PS_MAX (yMax, y);
        }
    }

    if (stats) {
	// XXX with multiple inputs (eg, output stacks -> exposure), these only represent the last input
        psMetadataAddS32(stats, PS_LIST_TAIL, "RANGE.XMIN", PS_META_REPLACE, "Minimum valid x value", xMin);
        psMetadataAddS32(stats, PS_LIST_TAIL, "RANGE.XMAX", PS_META_REPLACE, "Maximum valid x value", xMax);
        psMetadataAddS32(stats, PS_LIST_TAIL, "RANGE.YMIN", PS_META_REPLACE, "Minimum valid y value", yMin);
        psMetadataAddS32(stats, PS_LIST_TAIL, "RANGE.YMAX", PS_META_REPLACE, "Maximum valid y value", yMax);
    }

    return true;
}


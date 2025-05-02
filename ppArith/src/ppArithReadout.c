/** @file ppArithReadout.c
 *
 *  @brief
 *
 *  @ingroup ppArith
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 19:45:30 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppArith.h"

bool ppArithReadout(pmReadout *output, const pmReadout *input1, const pmReadout *input2, float const2,
                    const pmConfig *config, const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(output, false);
    PS_ASSERT_PTR_NON_NULL(input1, false);
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    bool mdok;                          // Status of MD lookup
    bool isMask = psMetadataLookupBool(&mdok, config->arguments, "MASK");

    psImage *inImage1;  // Input image 1
    psImage *outImage;  // Output image
    if (isMask) {
        inImage1 = input1->mask;
        outImage = output->mask;
        if (!outImage) {
            output->mask = outImage = psImageAlloc(inImage1->numCols, inImage1->numRows, PS_TYPE_IMAGE_MASK);
        }
        if (!output->image) {
            // Generate an image to serve as a backdrop for the mask when doing statistics
            output->image = psImageAlloc(inImage1->numCols, inImage1->numRows, PS_TYPE_F32);
            psImageInit(output->image, 1.0);
        }
    } else {
        inImage1 = input1->image;
        outImage = output->image;
        if (!outImage) {
            output->mask = outImage = psImageAlloc(inImage1->numCols, inImage1->numRows, PS_TYPE_IMAGE_MASK);
        }
    }
    PS_ASSERT_IMAGE_NON_NULL(inImage1, false);

    psImage *inImage2 = NULL;           // Input image 2
    if (input2) {
        inImage2 = isMask ? input2->mask : input2->image;
        PS_ASSERT_IMAGE_NON_NULL(inImage2, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(inImage2, inImage1, false);
        PS_ASSERT_IMAGE_TYPE(inImage2, inImage1->type.type, false);
    }

#if 0
    pmCell *outCell = output->parent;   // Output cell
    pmChip *outChip = outCell->parent;  // Output chip
    pmFPA *outFPA = outChip->parent;    // Output FPA
    pmHDU *outHDU = pmHDUGetLowest(outFPA, outChip, outCell); // Output HDU
    if (!outHDU->header) {
        outHDU->header = psMetadataAlloc();
    }
#endif

    // Look up appropriate values
    const char *op = psMetadataLookupStr(NULL, config->arguments, "OPERATION"); // Operation to perform

    if (input2) {
        psBinaryOp(outImage, inImage1, op, inImage2);
    }  else if (!isnan(const2)) {
        psBinaryOp(outImage, inImage1, op, psScalarAlloc(const2, inImage1->type.type));
    } else {
        psUnaryOp(outImage, inImage1, op);
    }

    return true;
}

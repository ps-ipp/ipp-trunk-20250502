/** @file ppSubBackground.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

bool ppSubBackground(pmConfig *config)
{
    psAssert(config, "Require configuration");

    bool mdok; // Status of metadata lookups

    psMetadata *ppSubRecipe = psMetadataLookupPtr(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSub
    psAssert(ppSubRecipe, "Need PPSUB recipe");
    psMetadata *psphotRecipe = psMetadataLookupPtr(NULL, config->recipes, PSPHOT_RECIPE); // Recipe for psphot
    psAssert(psphotRecipe, "Need PSPHOT recipe for binning");

    bool doApplyMaskNaN = psMetadataLookupBool(&mdok, ppSubRecipe, "APPLY.PIXELNAN"); // NaN the pixels underneath masks

    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask

    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *outRO = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT"); // Output image

    // Generate the background model
    if (!psphotModelBackground(config, view, "PPSUB.OUTPUT")) {
      psError(psErrorCodeLast(), false, "Unable to model background");
      psFree(view);
      return false;
    }

    // select the model readout (should now exist)
    pmReadout *modelRO = pmFPAfileThisReadout(config->files, view, "PSPHOT.BACKMDL");
    if (!modelRO) {
      psError(psErrorCodeLast(), false, "Unable to find background model");
      psFree(view);
      return false;
    }
    psFree(view);

    psImageBinning *binning = psMetadataLookupPtr(&mdok, modelRO->analysis, "PSPHOT.BACKGROUND.BINNING"); // Binning for model
    psImage *modelImage = modelRO->image; // Background model
    psImage *image = outRO->image; // Image of interest
    psImage *mask = outRO->mask; // Mask of interest

    // Do the background subtraction
    int numCols = image->numCols, numRows = image->numRows; // Size of image
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
	    // special case 1: NAN the masked pixels
            if(doApplyMaskNaN) {
              if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskBad) {
                image->data.F32[y][x] = NAN;
		continue;
              } 
            }
	    // special case 2: NAN & mask pixels without a valid background model
	    float value = psImageUnbinPixel(x + 0.5, y + 0.5, modelImage, binning); // Background value
	    if (!isfinite(value)) {
	      image->data.F32[y][x] = NAN;
	      mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskBad;
	      continue;
	    } 
	    image->data.F32[y][x] -= value;
        }
    }

    return true;
}

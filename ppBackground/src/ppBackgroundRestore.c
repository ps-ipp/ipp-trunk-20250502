#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppBackground.h"

psImageBinning *ppBackgroundBinningByRecipe(const psImage *image, // Image for which to generate a bg model
					    const pmConfig *config, // Configuration
					    psString recipe_name,
					    psString Xbin_name,
					    psString Ybin_name
					    )
{
  bool status = true;

  psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, recipe_name);
  assert (recipe);

  // I have the fine image size, I know the binning factor, determine the ruff image size
  psImageBinning *binning = psImageBinningAlloc();
  binning->nXfine = image->numCols;
  binning->nYfine = image->numRows;
  binning->nXbin  = psMetadataLookupS32(&status, recipe, Xbin_name);
  binning->nYbin  = psMetadataLookupS32(&status, recipe, Ybin_name);

  psImageBinningSetRuffSize(binning, PS_IMAGE_BINNING_CENTER);
  psImageBinningSetSkip(binning, image);

  return binning;
}

  


bool ppBackgroundRestore(pmChip *chip, const pmChip *background, const pmChip *pattern,
                         const pmFPAview *oldView, pmConfig *config, psImageMaskType maskBad)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(oldView, false);

    pmFPAview *view = pmFPAviewAlloc(0); // View to readout
    *view = *oldView;
    view->cell = 0;
    view->readout = 0;

    pmReadout *ro = pmFPAviewThisReadout(view, chip->parent);
    if (!ro || !ro->data_exists) {
        psError(PPBACKGROUND_ERR_CONFIG, true, "Readout has no data");
        return false;
    }
    const psImage *image = ro->image, *mask = ro->mask;   // Input image
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    if (background) {
        pmReadout *bgRO = pmFPAviewThisReadout(view, background->parent); // Readout with background

	psImageBinning *binning;
	psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPBACKGROUND_RECIPE); // Recipe
	if (!recipe) {
	  binning = psphotBackgroundBinning(image, config);
	}
	else {
	  binning = ppBackgroundBinningByRecipe(image,config,
						psMetadataLookupStr(NULL,recipe,"BINNING_RECIPE"),
						psMetadataLookupStr(NULL,recipe,"BINNING_XNAME"),
						psMetadataLookupStr(NULL,recipe,"BINNING_YNAME"));
	}
						
	fprintf(stderr,"%d %d %d %d\n",binning->nXfine,binning->nYfine,binning->nXbin,binning->nYbin);
        if (!binning) {
            psError(psErrorCodeLast(), false, "Unable to find background binning");
            return false;
        }
        if (binning->nXfine != numCols || binning->nYfine != numRows) {
            psError(PPBACKGROUND_ERR_CONFIG, true,
                    "Unbinned background model and input don't match (%dx%d vs %dx%d)",
                    binning->nXfine, binning->nYfine, numCols, numRows);
            return false;
        }

        psImage *bgImage = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Background
        if (!psImageUnbin(bgImage, bgRO->image, binning)) {
            psError(psErrorCodeLast(), false, "Unable to unbin background model");
            psFree(binning);
            return false;
        }
        psFree(binning);

        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
	      //	      if ((y % 250 == 0)&&(x % 250 == 0)) {
	      //		printf("%d %d %g\n",x,y,bgImage->data.F32[y][x]);
	      //	      }
                image->data.F32[y][x] += bgImage->data.F32[y][x];
		
            }
        }
        psFree(bgImage);
    }

    if (pattern) {
        pmReadout *patternRO = pmFPAviewThisReadout(view, pattern->parent); // Readout with pattern
        psImage *patternImage = patternRO->image; // Image with pattern
        psImage *patternMask = patternRO->mask;   // Mask for pattern
        // The sign is flipped for the continuity correction.
        bool mdok;
        bool isContinuity = psMetadataLookupBool(&mdok, pattern->hdu->header, "PTRN_CON");
        
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (isContinuity) {
                    image->data.F32[y][x] += patternImage->data.F32[y][x];
                } else {
                    image->data.F32[y][x] -= patternImage->data.F32[y][x];
                }
                mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= patternMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x];
            }
        }
    }

    psImage *auxMask = psMetadataLookupPtr(NULL, ro->analysis, "EXPNUM");
    if (auxMask) {
        if (auxMask->numCols != mask->numCols || auxMask->numRows != mask->numRows) {
            psError(PPBACKGROUND_ERR_DATA, true, "auxiliary mask size (%d x %d) does not match input size (%d x %d)",
                auxMask->numCols, auxMask->numRows, numCols, numRows);
            return false;
        }
        // Arno's masks are floating point, fix later and zero means bad
        #define AUXMASK_DATA F32
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (auxMask->data.AUXMASK_DATA[y][x] == 0) {
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskBad;
                }
            }
        }
    }


    ro->data_exists = true;
    ro->parent->data_exists = true;
    ro->parent->parent->data_exists = true;

    return true;
}




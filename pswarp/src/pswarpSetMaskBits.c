/** @file pswarpSetMaskBits.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 03:10:36 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

/** This function is called by the stand-alone pswarp program to set the mask values in the
 * config file.  It sets the named mask values MASK.PSPHOT and MARK.PSPHOT in the PSPHOT
 * recipe.  Functions or programs which call psphotReadout as a library function must set these
 * named mask values in the PSPHOT recipe on their own.  This function should only be called
 * after the first header of the input mask image has been loaded and the named mask bits
 * updated in the config metadata.
 */
 
bool pswarpSetMaskBits (pmConfig *config)
{
    psImageMaskType maskIn = 0x00;                      // mask for the input image
    psImageMaskType markIn = 0x00;                      // mark for the input image
    psImageMaskType maskOut = 0x00;                     // mask for the output image

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PSWARP_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");
    bool doDetectCTE = psMetadataLookupBool(NULL, recipe, "DETECT.CTE"); // Do detections on pixels underneath CTE masks

    // this function sets the required single-image mask bits
    if(!doDetectCTE) {
      if (!pmConfigMaskSetBits (&maskIn, &markIn, config)) {
          psError (psErrorCodeLast(), false, "Unable to define the mask bit values");
          return false;
      }
    } else {
      psMetadata *maskrecipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
      if (!maskrecipe) {
          psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
          return false;
      }
      if (!pswarpMaskSetInMetadata(&maskIn, &markIn, maskrecipe)) {
          psError (psErrorCodeLast(), false, "Unable to determine the mask value");
          return false;
      }
    }
    
    // mask for non-linear flat regions (default to DETECTOR if not defined)
    psImageMaskType badMask = pmConfigMaskGet("CONV.BAD", config);
    if (!badMask) {
        badMask = 0x01;
        pmConfigMaskSet (config, "CONV.BAD", badMask);
    }
    maskOut |= badMask;

    // mask for non-linear flat regions (default to DETECTOR if not defined)
    psImageMaskType poorMask = pmConfigMaskGet("CONV.POOR", config);
    if (!poorMask) {
        poorMask = 0x02;
        pmConfigMaskSet (config, "CONV.POOR", poorMask);
    }
    maskOut |= poorMask;

    // the output image includes all of the bits from the input image
    maskOut |= maskIn;

    // search for an unset bit to use for MARK:
    psImageMaskType markOut   = 0x00;
    psImageMaskType markTrial = 0x01;

    int nBits = sizeof(psImageMaskType) * 8;
    for (int i = 0; !markOut && (i < nBits); i++) {
        if (maskOut & markTrial) {
            markTrial <<= 1;
        } else {
            markOut = markTrial;
        }
    }
    if (!markOut) {
        psError(PSWARP_ERR_CONFIG, true, "Unable to define the MARK bit mask: all bits taken!");
        return false;
    }

    // update the pswarp recipe
    psMetadata *warpRecipe = psMetadataLookupPtr (NULL, config->recipes, PSWARP_RECIPE);
    if (!warpRecipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    // set maskOut and markOut in the psphot recipe
    // NOTE: psphot works on the output images, not input images, so set the MARK and MASK correctly here
    psMetadataAddImageMask (warpRecipe, PS_LIST_TAIL, "MASK.INPUT",  PS_META_REPLACE, "user-defined mask", maskIn);
    psMetadataAddImageMask (warpRecipe, PS_LIST_TAIL, "MARK.INPUT",  PS_META_REPLACE, "user-defined mask", markIn);
    psMetadataAddImageMask (warpRecipe, PS_LIST_TAIL, "MASK.OUTPUT", PS_META_REPLACE, "user-defined mask", maskOut);
    psMetadataAddImageMask (warpRecipe, PS_LIST_TAIL, "MARK.OUTPUT", PS_META_REPLACE, "user-defined mask", markOut);

    // update the psphot recipe
    psMetadata *psphotRecipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!psphotRecipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    // set maskOut and markOut in the psphot recipe
    // NOTE: psphot works on the output images, not input images, so set the MARK and MASK correctly here
    psMetadataAddImageMask (psphotRecipe, PS_LIST_TAIL, "MARK.PSPHOT", PS_META_REPLACE, "user-defined mask", markOut);
    psMetadataAddImageMask (psphotRecipe, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "user-defined mask", maskOut);

    return true;
}

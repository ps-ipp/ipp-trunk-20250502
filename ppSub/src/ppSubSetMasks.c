/** @file ppSubSetMasks.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
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

// Structure to hold the properties of a mask value
typedef struct {
    char *badMaskName;                  // name for "bad" (i.e., mask me please) pixels
    char *fallbackName;                 // Fallback name in case a bad mask name is not defined
    psImageMaskType defaultMaskValue;   // Default value in case a bad mask name and its fallback are not defined
    bool isBad; // include this value as part of the MASK.VALUE entry (generically bad)
} pmConfigMaskInfo;

static pmConfigMaskInfo skycellmasks[] = {
    // Features of the detector
    { "DETECTOR",  NULL,       0x01, true }, // Something is wrong with the detector
    { "FLAT",      "DETECTOR", 0x01, true }, // Pixel doesn't flat-field properly
    { "DARK",      "DETECTOR", 0x01, true }, // Pixel doesn't dark-subtract properly
    { "BLANK",     "DETECTOR", 0x01, true }, // Pixel doesn't contain valid data
    { "CTE",       "DETECTOR", 0x01, false }, // Pixel has poor CTE
    { "BURNTOOL",  NULL,       0x04, false }, // Pixel has been touched by burntool
    // Invalid signal ranges
    { "SAT",       NULL,       0x02, true  }, // Pixel is saturated or non-linear
    { "LOW",       "SAT",      0x02, true  }, // Pixel is low
    { "SUSPECT",   NULL,       0x04, false }, // Pixel is suspected of being bad
    // Non-astronomical structures
    { "CR",        NULL,       0x08, true  }, // Pixel contains a cosmic ray
    { "SPIKE",     NULL,       0x08, false  }, // Pixel contains a diffraction spike
    { "GHOST",     NULL,       0x08, false  }, // Pixel contains an optical ghost
    { "STREAK",    NULL,       0x08, false  }, // Pixel contains a streak
    { "CROSSTALK", NULL,       0x08, false  }, // Pixel contains crosstalk data
    { "STARCORE",  NULL,       0x08, false  }, // Pixel contains a bright star core
    // Effects of convolution and interpolation
    { "CONV.BAD",  NULL,       0x02, true  }, // Pixel is bad after convolution with a bad pixel
    { "CONV.POOR", NULL,       0x04, false }, // Pixel is poor after convolution with a bad pixel
};

bool ppSubSetMasks(pmConfig *config)
{
    psAssert(config, "Require configuration");

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");
    bool doApplyMaskNaN = psMetadataLookupBool(NULL, recipe, "APPLY.PIXELNAN"); // NaN the pixels underneath masks

    psImageMaskType maskValue, markValue; // Mask values
    if(doApplyMaskNaN) {
      if (!pmConfigMaskSetBits(&maskValue, &markValue, config)) {
          psError(PPSUB_ERR_CONFIG, false, "Unable to determine mask value.");
          return false;
      }
    } else {
      psMetadata *maskrecipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
      if (!maskrecipe) {
          psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
          return false;
      }
      if (!ppSubMaskSetInMetadata(&maskValue, &markValue, maskrecipe)) {
          psError(PPSUB_ERR_CONFIG, false, "Unable to determine mask value.");
          return false;
      }
    }

    // Set the mask bits needed by psphot (in psphot recipe)
    psphotSetMaskRecipe(config, maskValue, markValue);


    psImageMaskType satValue = pmConfigMaskGet("SAT", config);
    psAssert(satValue, "SAT must be non-zero");

    psImageMaskType lowValue = pmConfigMaskGet("LOW", config);
    if (!lowValue) {
        // Look up old name for backward compatability
        lowValue = pmConfigMaskGet("BAD", config);
    }
    psAssert(lowValue, "LOW or BAD must be non-zero");

    // Input images
    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *inRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT"); // Input readout
    pmReadout *refRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF"); // Reference readout

    psImage *input = inRO->image;       // Input image
    psImage *reference = refRO->image;  // Reference image
    PS_ASSERT_IMAGES_SIZE_EQUAL(input, reference, false);
    int numCols = input->numCols, numRows = input->numRows; // Size of image

    // Generate masks if they don't exist
    if (!inRO->mask) {
        if (psMetadataLookupBool(NULL, recipe, "MASK.GENERATE")) {
            pmReadoutSetMask(inRO, satValue, lowValue);
        } else {
            inRO->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(inRO->mask, 0);
        }
    }
    if (!refRO->mask) {
        if (psMetadataLookupBool(NULL, recipe, "MASK.GENERATE")) {
            pmReadoutSetMask(refRO, satValue, lowValue);
        } else {
            refRO->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(refRO->mask, 0);
        }
    }

    // Mask the NAN values (USE BLANK instead of SAT?)
    if (!pmReadoutMaskInvalid(inRO, maskValue, satValue)) {
        psError(PPSUB_ERR_DATA, false, "Unable to mask non-finite pixels in input.");
	psFree(view);
        return false;
    }
    if (!pmReadoutMaskInvalid(refRO, maskValue, satValue)) {
        psError(PPSUB_ERR_DATA, false, "Unable to mask non-finite pixels in reference.");
	psFree(view);
        return false;
    }

#if 0
    // Interpolation over bad pixels: this takes a while!
    bool mdok = false;
    psString interpModeStr = psMetadataLookupStr(&mdok, recipe, "INTERPOLATION"); // Interpolation mode
    psImageInterpolateMode interpMode = psImageInterpolateModeFromString(interpModeStr); // Interp
    if (interpMode == PS_INTERPOLATE_NONE) {
        psError(PPSUB_ERR_CONFIG, false, "Unknown interpolation mode: %s", interpModeStr);
	psFree(view);
        return false;
    }
    float poorFrac = psMetadataLookupF32(&mdok, recipe, "POOR.FRACTION"); // Fraction for "poor"

    psImageMaskType maskPoor = pmConfigMaskGet("CONV.POOR", config); // Bits to mask for poor pixels
    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels

    // Interpolate over bad pixels, so the bad pixels don't explode
    if (!pmReadoutInterpolateBadPixels(inRO, maskVal, interpMode, poorFrac, maskPoor, maskBad)) {
        psError(PPSUB_ERR_DATA, false, "Unable to interpolate bad pixels for input image.");
	psFree(view);
        return false;
    }
    if (!pmReadoutInterpolateBadPixels(refRO, maskVal, interpMode, poorFrac, maskPoor, maskBad)) {
        psError(PPSUB_ERR_DATA, false, "Unable to interpolate bad pixels for reference image.");
	psFree(view);
        return false;
    }
    maskVal |= maskBad;
#endif

    psFree(view);
    return true;
}

bool ppSubMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
    )
{
    PS_ASSERT_METADATA_NON_NULL(source, false);

    // Ensure all the bad mask names exist, and set the value to catch all bad pixels
    psImageMaskType maskValue = 0;           // Value to mask to catch all the bad pixels
    psImageMaskType allMasks = 0;            // Value to mask to catch all masked bits (to set MARK)

    int nMasks = sizeof (skycellmasks) / sizeof (pmConfigMaskInfo);

    for (int i = 0; i < nMasks; i++) {
        bool mdok;                      // Status of MD lookup
        psImageMaskType value = psMetadataLookupImageMaskFromGeneric(&mdok, source, skycellmasks[i].badMaskName); // Value of mask
        if (!mdok) {
            psWarning ("problem with mask value %s\n", skycellmasks[i].badMaskName);
        }

        if (!value) {
            if (skycellmasks[i].fallbackName) {
                value = psMetadataLookupImageMaskFromGeneric(&mdok, source, skycellmasks[i].fallbackName);
            }
            if (!value) {
                value = skycellmasks[i].defaultMaskValue;
            }
            psMetadataAddImageMask(source, PS_LIST_TAIL, skycellmasks[i].badMaskName, PS_META_REPLACE, NULL, value);
        }
        if (skycellmasks[i].isBad) {
            maskValue |= value;
        }
        allMasks |= value;
    }

    // search for an unset bit to use for MARK:
    psImageMaskType markValue = 0x00;
    psImageMaskType markTrial = 0x01;

    int nBits = sizeof(psImageMaskType) * 8;
    for (int i = 0; !markValue && (i < nBits); i++) {
        if (allMasks & markTrial) {
            markTrial <<= 1;
        } else {
            markValue = markTrial;
        }
    }
    if (!markValue) {
        psError (PS_ERR_UNKNOWN, true, "Unable to define the MARK bit mask: all bits taken!");
        return false;
    }

    // update the list with the results
    psMetadataAddImageMask(source, PS_LIST_TAIL, "MASK.VALUE", PS_META_REPLACE, NULL, maskValue);
    psMetadataAddImageMask(source, PS_LIST_TAIL, "MARK.VALUE", PS_META_REPLACE, NULL, markValue);

    if (outMaskValue) {
        *outMaskValue = maskValue;
    }
    if (outMarkValue) {
        *outMarkValue = markValue;
    }

    return true;
}


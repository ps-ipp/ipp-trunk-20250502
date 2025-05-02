#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// Structure to hold the properties of a mask value
typedef struct {
    char *badMaskName;                  // name for "bad" (i.e., mask me please) pixels
    char *fallbackName;                 // Fallback name in case a bad mask name is not defined
    psImageMaskType defaultMaskValue;   // Default value in case a bad mask name and its fallback are not defined
    bool isBad; // include this value as part of the MASK.VALUE entry (generically bad)
} pmConfigMaskInfo;

static pmConfigMaskInfo ppimagemasks[] = {
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

bool ppImageSetMaskBits (pmConfig *config, ppImageOptions *options) {


    // Look up recipe values
    psMetadata *pprecipe = psMetadataLookupMetadata(NULL, config->recipes, RECIPE_NAME);

    psAssert(pprecipe, "We checked this earlier, so it should be here.");
    bool doDetectCTE = psMetadataLookupBool(NULL, pprecipe, "DETECT.CTE"); // Do detections on pixels underneath CTE masks

    // this function sets the required single-image mask bits
    if(!doDetectCTE) {
      if (!pmConfigMaskSetBits (&options->maskValue, &options->markValue, config)) {
          psError (PS_ERR_UNKNOWN, true, "Unable to define the mask bit values");
          return false;
      }
    } else {
      psMetadata *maskrecipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
      if (!maskrecipe) {
          psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
          return false;
      }
      if (!ppImageMaskSetInMetadata(&options->maskValue, &options->markValue, maskrecipe)) {
          psError (PS_ERR_UNKNOWN, true, "Unable to determine the mask value");
          return false;
      }
    }

    // psTimerStart("set.mask.bits");

    // at this point we know we have valid values for required entries SAT, BAD, FLAT, BLANK:

    // mask for bad flat corrections
    options->flatMask = pmConfigMaskGet("FLAT", config);
    psAssert (options->flatMask, "flat mask not set");

    // mask for bad dark corrections
    options->darkMask = pmConfigMaskGet("DARK", config);
    psAssert (options->darkMask, "dark mask not set");

    // mask for non-existent data  (default to DETECTOR if not defined)
    options->blankMask = pmConfigMaskGet("BLANK", config);
    psAssert (options->blankMask, "blank mask not set");
    if (options->overscan) {
	options->overscan->maskVal  = options->blankMask;
    }

    // mask for saturated data  (default to RANGE if not defined)
    options->satMask = pmConfigMaskGet("SAT", config);
    psAssert (options->satMask, "sat mask not set");

    // mask for below-range data  (default to RANGE if not defined)
    options->lowMask = pmConfigMaskGet("LOW", config);
    if (!options->lowMask) {
        // look up old name for backward compatability
        options->lowMask = pmConfigMaskGet("BAD", config);
    }
    psAssert (options->lowMask, "low mask not set");

    // mask for suspect regions due to burntool if we need to.
    if (options->doMaskBurntool) {
      options->burntoolMask = pmConfigMaskGet("BURNTOOL",config);
      psAssert (options->burntoolMask, "burntool mask not set");
    }
    
    // save MASK and MARK on the PSPHOT recipe
    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    if (!recipe) {
        psError(PSPHOT_ERR_CONFIG, false, "missing recipe %s", PSPHOT_RECIPE);
        return false;
    }

    // set maskValue and markValue in the psphot recipe
    psMetadataAddImageMask(recipe, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "user-defined mask",
                           options->maskValue);
    psMetadataAddImageMask(recipe, PS_LIST_TAIL, "MARK.PSPHOT", PS_META_REPLACE, "user-defined mask",
                           options->markValue);

    // psLogMsg ("ppImage", 5, "set mask bits: %f sec\n", psTimerMark ("set.mask.bits"));

    return true;
}


bool ppImageMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
    )
{
    PS_ASSERT_METADATA_NON_NULL(source, false);

    // Ensure all the bad mask names exist, and set the value to catch all bad pixels
    psImageMaskType maskValue = 0;           // Value to mask to catch all the bad pixels
    psImageMaskType allMasks = 0;            // Value to mask to catch all masked bits (to set MARK)

    int nMasks = sizeof (ppimagemasks) / sizeof (pmConfigMaskInfo);

    for (int i = 0; i < nMasks; i++) {
        bool mdok;                      // Status of MD lookup
        psImageMaskType value = psMetadataLookupImageMaskFromGeneric(&mdok, source, ppimagemasks[i].badMaskName); // Value of mask
        if (!mdok) {
            psWarning ("problem with mask value %s\n", ppimagemasks[i].badMaskName);
        }

        if (!value) {
            if (ppimagemasks[i].fallbackName) {
                value = psMetadataLookupImageMaskFromGeneric(&mdok, source, ppimagemasks[i].fallbackName);
            }
            if (!value) {
                value = ppimagemasks[i].defaultMaskValue;
            }
            psMetadataAddImageMask(source, PS_LIST_TAIL, ppimagemasks[i].badMaskName, PS_META_REPLACE, NULL, value);
        }
        if (ppimagemasks[i].isBad) {
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


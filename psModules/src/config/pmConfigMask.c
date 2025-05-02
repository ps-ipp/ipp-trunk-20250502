#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmConfigMask.h"

// Structure to hold the properties of a mask value
typedef struct {
    char *badMaskName;                  // name for "bad" (i.e., mask me please) pixels
    char *fallbackName;                 // Fallback name in case a bad mask name is not defined
    psImageMaskType defaultMaskValue;   // Default value in case a bad mask name and its fallback are not defined
    bool isBad; // include this value as part of the MASK.VALUE entry (generically bad)
} pmConfigMaskInfo;

static pmConfigMaskInfo masks[] = {
    // Features of the detector
    { "DETECTOR",  NULL,       0x01, true  }, // Something is wrong with the detector
    { "FLAT",      "DETECTOR", 0x01, true  }, // Pixel doesn't flat-field properly
    { "DARK",      "DETECTOR", 0x01, true  }, // Pixel doesn't dark-subtract properly
    { "BLANK",     "DETECTOR", 0x01, true  }, // Pixel doesn't contain valid data
    { "CTE",       "DETECTOR", 0x01, true  }, // Pixel has poor CTE
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

// The functions in this file do not force the recipe or header values to be stored as the same
// type as psImageMaskType : they only check that the given values will fit in the space
// provided by psImageMaskType.  This should allow some backwards compatibility (old 8-bit
// masks will work with a 16-bit system), but will catch unhandled conflicts (trying to fit 16
// bits in 8-bits of space).

// XXX this file does not have psError vs psWarning worked out correctly.  some of the
// failure modes should result in errors, not just warnings.

// pmConfigMaskSetInMetadata examines named mask values and set the bits for maskValue and
// markValue.  Ensures that the below-named mask values are set, and calculates the mask value
// to catch all of the mask values marked as 'bad'.  Supplies the fallback name if the primary
// name is not found, or the default values if the fallback name is not found.

bool pmConfigMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
    )
{
    PS_ASSERT_METADATA_NON_NULL(source, false);

    // Ensure all the bad mask names exist, and set the value to catch all bad pixels
    psImageMaskType maskValue = 0;           // Value to mask to catch all the bad pixels
    psImageMaskType allMasks = 0;            // Value to mask to catch all masked bits (to set MARK)

    int nMasks = sizeof (masks) / sizeof (pmConfigMaskInfo);

    for (int i = 0; i < nMasks; i++) {
        bool mdok;                      // Status of MD lookup
        psImageMaskType value = psMetadataLookupImageMaskFromGeneric(&mdok, source, masks[i].badMaskName); // Value of mask
        if (!mdok) {
            psWarning ("problem with mask value %s\n", masks[i].badMaskName);
        }

        if (!value) {
            if (masks[i].fallbackName) {
                value = psMetadataLookupImageMaskFromGeneric(&mdok, source, masks[i].fallbackName);
            }
            if (!value) {
                value = masks[i].defaultMaskValue;
            }
            psMetadataAddImageMask(source, PS_LIST_TAIL, masks[i].badMaskName, PS_META_REPLACE, NULL, value);
        }
        if (masks[i].isBad) {
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

// Get a mask value by name(s)
psImageMaskType pmConfigMaskGetFromMetadata(psMetadata *source, // Source of masks
                                            const char *masks // Mask values to get
    )
{
    psImageMaskType mask = 0;                // Mask value, to return

    psArray *names = psStringSplitArray(masks, " ,;", false); // Array of symbolic names
    for (int i = 0; i < names->n; i++) {
        const char *name = names->data[i]; // Symbolic name of interest
        bool mdok;                      // Status of MD lookup
        psImageMaskType value = psMetadataLookupImageMaskFromGeneric(&mdok, source, name);
        if (!mdok) {
            // Try and generate the value if we can
            if (strcmp(name, "MASK.VALUE") == 0 || strcmp(name, "MARK.VALUE") == 0) {
                if (!pmConfigMaskSetInMetadata(NULL, NULL, source)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to set mask bits.");
                    return 0;
                }
                value = psMetadataLookupImageMaskFromGeneric(&mdok, source, name);
                psAssert(mdok, "Should have generated mask value");
            } else {
                psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find mask value for %s", name);
                psFree(names);
                return 0;
            }
        }
        mask |= value;
    }
    psFree(names);

    return mask;
}

// lookup an image mask value by name from a psMetadata, without requiring the entry to
// be of type psImageMaskType, but verifying that it will fit in psImageMaskType
psImageMaskType psMetadataLookupImageMaskFromGeneric(bool *status, const psMetadata *md, const char *name)
{
    if (!md) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Metadata is NULL.");
        if (status) {
            *status = false;
        }
        return 0;
    }
    if (!name) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Keyword is NULL.");
        if (status) {
            *status = false;
        }
        return 0;
    }
    *status = true;

    // select the mask bit name from the header
    psMetadataItem *item = psMetadataLookup(md, name);
    if (!item) {
        if (status) {
            *status = false;
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find keyword %s when parsing mask", name);
        }
        return 0;
    }

    // the value may be any of the U8, U16, U32, U64 types : accept the value regardless of type size
    psU64 fullValue = 0;
    switch (item->type) {
      case PS_DATA_U8:
        fullValue = item->data.U8;
        break;
      case PS_DATA_U16:
        fullValue = item->data.U16;
        break;
      case PS_DATA_U32:
        fullValue = item->data.U32;
        break;
      case PS_DATA_U64:
        fullValue = item->data.U64;
        break;
      case PS_DATA_S8:
        fullValue = item->data.S8;
        break;
      case PS_DATA_S16:
        fullValue = item->data.S16;
        break;
      case PS_DATA_S32:
        fullValue = item->data.S32;
        break;
      case PS_DATA_S64:
        fullValue = item->data.S64;
        break;
      default:
        if (status) {
            *status = false;
        } else {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    "Mask entry %s in metadata is not of a mask type", name);
        }
        return 0;
    }

    // will the incoming value fit within the current image mask type?
    if (fullValue > PS_MAX_IMAGE_MASK_TYPE) {
        if (status) {
            *status = false;
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Mask entry %s in metadata is larger than allowed by the psImageMaskType", name);
        }
        return 0;
    }
    psImageMaskType value = fullValue;
    // XXX validate that value is a 2^n value?

    return value;
}

// Remove from the header keywords starting with the provided string
int pmConfigMaskRemoveHeaderKeywords(psMetadata *header, // Header from which to remove keywords
                                     const char *start // Remove keywords that start with this string
    )
{
    psString regex = NULL;              // Regular expression for keywords
    psStringAppend(&regex, "^%s[0-9][0-9]", start);
    psMetadataIterator *iter = psMetadataIteratorAlloc(header, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);
    psMetadataItem *item;               // Item from iteration
    int num = 0;                        // Number of items removed
    while ((item = psMetadataGetAndIncrement(iter))) {
        psMetadataRemoveKey(header, item->name);
        num++;
    }
    psFree(iter);
    return num;
}

// look up the named mask value(s) from the MASKS recipe in the config system
psImageMaskType pmConfigMaskGet(const char *masks, const pmConfig *config)
{
    psAssert(config, "Require configuration");
    PS_ASSERT_STRING_NON_EMPTY(masks, 0);

    bool mdok;                          // Status of MD lookup
    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, "MASKS"); // The recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
        return 0;
    }

    psImageMaskType mask = pmConfigMaskGetFromMetadata (recipe, masks);
    return mask;
}

bool pmConfigMaskSet(const pmConfig *config, const char *maskName, psImageMaskType maskValue)
{
    psAssert(config, "Require configuration");
    PS_ASSERT_STRING_NON_EMPTY(maskName, false);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
        return false;
    }

    bool status = psMetadataAddImageMask(recipe, PS_LIST_TAIL, maskName, PS_META_REPLACE, NULL, maskValue);
    return status;
}


// replace the named masks in the recipe with values in the header:
// replace only the names in the header in the recipe
bool pmConfigMaskReadHeader(pmConfig *config, const psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_METADATA_NON_NULL(header, false);

    bool status = false;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
        return false;
    }

    // MASK.VALUE and MARK.VALUE aren't usually set in the recipe, but may be set in the header: create fake
    // versions so that it won't complain later
    if (!psMetadataLookup(recipe, "MASK.VALUE")) {
        psMetadataAddImageMask(recipe, PS_LIST_TAIL, "MASK.VALUE", 0, "Bits to mask", 0);
    }
    if (!psMetadataLookup(recipe, "MARK.VALUE")) {
        psMetadataAddImageMask(recipe, PS_LIST_TAIL, "MARK.VALUE", 0, "Bits for marking", 0);
    }

    // How many mask values do we need to read?  We raise an error if this is not found,
    // unless the MASK.FORCE is set to true in the camera config
    int nMask = psMetadataLookupS32(&status, header, "MSKNUM");
    if (!status) {
        if (psMetadataLookupBool(&status, config->camera, "MASK.FORCE")) {
            psWarning("No mask values in header.  Assuming MASKS recipe is accurate because of MASK.FORCE");
            return true;
        }
        psError(PS_ERR_UNKNOWN, true, "Unable to find MSKNUM in header.");
        return false;
    }

    // Loop over the expected number of header mask names.  For each named mask value, there
    // should be a pair of header keywords, one for the name and one for the value
    char namekey[80];                   // Keyword name for symbolic name of mask entry
    char valuekey[80];                  // Keyword name for value of mask entry
    for (int i = 0; i < nMask; i++) {
        snprintf(namekey,  64, "MSKNAM%02d", i);
        snprintf(valuekey, 64, "MSKVAL%02d", i);

        char *name = psMetadataLookupStr(&status, header, namekey);
        if (!status || !name) {
            psWarning("Unable to find header keyword %s when parsing mask", namekey);
            continue;
        }

        psImageMaskType headerValue = psMetadataLookupImageMaskFromGeneric (&status, header, valuekey);
        if (!status) {
            psWarning("Failed to get mask value %s from header, skipping", valuekey);
            continue;
        }

        // since we may read multiple mask files, we need to warn (or error?) if any of the
        // header mask values conflict with other header mask values; However, the original
        // mask values from the recipe do not need to match the header values.

        // when we add a header mask value, we will also add the NAME.ALREADY entry; check for
        // the NAME.ALREADY entry to see if we have previously added this mask value from a
        // header.

        psString nameAlready = NULL;    // Name of key with ".ALREADY" added
        psStringAppend(&nameAlready, "%s.ALREADY", name);
        bool already = psMetadataLookupBool(&status, recipe, nameAlready); // Already read this one?

        bool inRecipe = false;
        psImageMaskType recipeValue = psMetadataLookupImageMaskFromGeneric (&inRecipe, recipe, name);
        if (!inRecipe) {
            psWarning("Mask value %s is not defined in the recipe", name);
        }

        if (already) {
            assert (inRecipe); // XXX makes no sense for NAME.ALREADY to be in without NAME
            if (recipeValue != headerValue) {
                psWarning("New mask header value does not match previously loaded entry: %x vs %x", headerValue, recipeValue);
                psMetadataAddImageMask(recipe, PS_LIST_TAIL, name, PS_META_REPLACE, "Bitmask bit value", headerValue);
                // XXX alternatively, error here
            }
        } else {
            psMetadataAddBool(recipe, PS_LIST_TAIL, nameAlready, 0, "Already read this mask value", true);
            psMetadataAddImageMask(recipe, PS_LIST_TAIL, name, PS_META_REPLACE, "Bitmask bit value", headerValue);
        }

        psFree(nameAlready);
    }

    return true;
}

// write the named mask bits to the header
bool pmConfigMaskWriteHeader(const pmConfig *config, psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_METADATA_NON_NULL(header, false);

    pmConfigMaskRemoveHeaderKeywords(header, "MSKNAM");
    pmConfigMaskRemoveHeaderKeywords(header, "MSKVAL");
    if (psMetadataLookup(header, "MSKNUM")) {
        psMetadataRemoveKey(header, "MSKNUM");
    }

    char namekey[80];
    char valuekey[80];

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
        return false;
    }

    int nMask = 0;

    psMetadataIterator *iter = psMetadataIteratorAlloc(recipe, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {

        // XXX this would give a false positive for mask which include '.ALREADY' in their names
        char *ptr = strstr (item->name, ".ALREADY");
        if (ptr) continue;

        psU64 fullValue = 0;
        switch (item->type) {
          case PS_DATA_U8:
            fullValue = item->data.U8;
            break;
          case PS_DATA_U16:
            fullValue = item->data.U16;
            break;
          case PS_DATA_U32:
            fullValue = item->data.U32;
            break;
          case PS_DATA_U64:
            fullValue = item->data.U64;
            break;
          default:
            psWarning("mask recipe entry %s is not a bit value\n", item->name);
            continue;
        }
        assert (fullValue <= PS_MAX_IMAGE_MASK_TYPE); // this should have been asserted on read...

        snprintf(namekey,  64, "MSKNAM%02d", nMask);
        snprintf(valuekey, 64, "MSKVAL%02d", nMask);

        psMetadataAddStr(header, PS_LIST_TAIL, namekey, 0, "Bitmask bit name", item->name);
        psMetadataAddImageMask(header, PS_LIST_TAIL, valuekey, 0, "Bitmask bit value", fullValue);
        nMask++;
    }
    psFree(iter);

    psMetadataAddS32(header, PS_LIST_TAIL, "MSKNUM", 0, "Bitmask bit count", nMask);
    return true;
}


bool pmConfigMaskSetBits(psImageMaskType *outMaskValue, psImageMaskType *outMarkValue, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
        return false;
    }

    bool status = pmConfigMaskSetInMetadata(outMaskValue, outMarkValue, recipe);
    return status;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FPA version of mask functions.  These are not ready to go yet.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0

bool pmFPAMaskWriteHeader(psMetadata *header, const pmFPA *fpa)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    // clear out the header of the associated keywords:
    pmConfigMaskRemoveHeaderKeywords(header, "MSKNAM");
    pmConfigMaskRemoveHeaderKeywords(header, "MSKVAL");
    if (psMetadataLookup(header, "MSKNUM")) {
        psMetadataRemoveKey(header, "MSKNUM");
    }

    char namekey[80], valuekey[80];     // Mask name and mask value header keywords
    int numMask = 0;                    // Number of mask entries

    psMetadataIterator *iter = psMetadataIteratorAlloc(fpa->masks, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_TYPE_IMAGE_MASK) {
            psWarning("mask recipe entry %s is not of a mask type (%x)", item->name, item->type);
            continue;
        }

        snprintf(namekey,  64, "MSKNAM%02d", numMask);
        snprintf(valuekey, 64, "MSKVAL%02d", numMask);

        psMetadataAddStr(header, PS_LIST_TAIL, namekey, 0, "Bitmask bit name", item->name);
        psMetadataAddImageMask(header, PS_LIST_TAIL, valuekey, 0, "Bitmask bit value", item->data.PS_TYPE_IMAGE_MASK_DATA);
        numMask++;
    }
    psFree(iter);

    return psMetadataAddS32(header, PS_LIST_TAIL, "MSKNUM", 0, "Number of named mask entries", numMask);
}

bool pmFPAMaskSetValues(psImageMaskType *outMaskValue, psImageMaskType *outMarkValue, pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    return maskSetValues(outMaskValue, outMarkValue, fpa->masks);
}

psImageMaskType pmFPAMaskGet(const pmFPA *fpa, const char *masks, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, 0);
    PS_ASSERT_STRING_NON_EMPTY(masks, 0);
    PS_ASSERT_PTR_NON_NULL(config, 0);

    if (fpa->masks) {
        return pmConfigMaskGetFromMetadata(fpa->masks, masks);
    }
    return pmConfigMaskGet(masks, config);
}

bool pmFPAMaskSet(pmFPA *fpa, const char *maskName, psImageMaskType maskValue)
{
    PS_ASSERT_PTR_NON_NULL(fpa, 0);
    PS_ASSERT_STRING_NON_EMPTY(maskName, false);

    if (!fpa->masks) {
        fpa->masks = psMetadataAlloc();
    }
    return psMetadataAddImageMask(fpa->masks, PS_LIST_TAIL, maskName, PS_META_REPLACE, NULL, maskValue);
}

bool pmFPAMaskReadHeader(pmFPA *fpa, const psMetadata *header, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_METADATA_NON_NULL(header, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    if (!fpa->masks) {
        fpa->masks = psMetadataAlloc();
    }

    bool mdok;                          // Status of MD lookup
    int numMask = psMetadataLookupS32(&mdok, header, "MSKNUM"); // Number of mask values in header
    if (!mdok) {
        if (psMetadataLookupBool(&mdok, config->camera, "MASK.FORCE")) {
            psWarning("No mask values in header.  Assuming MASKS recipe is accurate because of MASK.FORCE");
            numMask = 0;
        } else {
            psError(PS_ERR_UNKNOWN, true, "Unable to find MSKNUM in header.");
            return false;
        }
    }

    char namekey[80];                   // Keyword name for symbolic name of mask entry
    char valuekey[80];                  // Keyword name for value of mask entry
    for (int i = 0; i < numMask; i++) {
        snprintf(namekey,  64, "MSKNAM%02d", i);
        snprintf(valuekey, 64, "MSKVAL%02d", i);

        char *name = psMetadataLookupStr(&mdok, header, namekey);
        if (!mdok || !name) {
            psWarning("Unable to find header keyword %s when parsing mask", namekey);
            continue;
        }
        psImageMaskType bit = psMetadataLookupImageMask(&mdok, header, valuekey);
        if (!mdok) {
            psWarning("Unable to find header keyword %s when parsing mask", namekey);
            continue;
        }

        // XXX validate that bit is a 2^n value?

        psMetadataItem *item = psMetadataLookup(fpa->masks, name); // Item in recipe with current value
        if (item) {
            psAssert(item->type == PS_TYPE_IMAGE_MASK, "Mask entry %s is not of a mask type (%x)",
                     name, item->type);
            if (item->data.PS_TYPE_IMAGE_MASK_DATA != bit) {
                psWarning("New mask entry %s doesn't match previously loaded entry: %x vs %x",
                          name, bit, item->data.PS_TYPE_IMAGE_MASK_DATA);
            }
        } else {
            psMetadataAddImageMask(fpa->masks, PS_LIST_TAIL, name, 0, NULL, bit);
        }
    }

    // Now copy everything else from the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "MASKS"); // The recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find MASKS recipe.");
        return false;
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(recipe, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_TYPE_IMAGE_MASK) {
            psWarning("Recipe mask entry %s is not of a mask type (%x)", item->name, item->type);
            continue;
        }
        if (!psMetadataLookup(fpa->masks, item->name)) {
            psMetadataAddImageMask(fpa->masks, PS_LIST_TAIL, item->name, 0, item->comment,
                            item->data.PS_TYPE_IMAGE_MASK_DATA);
        }
    }
    psFree(iter);

    return true;
}
#endif

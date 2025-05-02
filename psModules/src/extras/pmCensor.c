#include <pslib.h>
#include <pmCensor.h>

bool
pmCensorGetMasks(pmConfig *config, psU32 *pMaskCensor, psU32 *pMaskStreak)
{
    bool status;
    psMetadata *masks = psMetadataLookupMetadata(&status, config->recipes, "MASKS");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to lookup MASKS in recipes.\n");
        return false;
    }


    // Older sets of mask values had SPIKE and STARCORE with the same value as STREAK
    // handle this case by making sure the streak bit is set
    psU32 streak = psMetadataLookupU32(&status, masks, "STREAK");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to lookup mask value for STREAK in recipes.\n");
        return false;
    }
    *pMaskStreak = streak;

    // first look for the value in the recipes
    psU32 maskNoCensor = psMetadataLookupU32(&status, config->camera, "MASK.NO.CENSOR");
    if (status) {
        // All done
        *pMaskCensor = ~maskNoCensor;
        return true;
    }

    // Not defined. So we must compute it.

    // preserve pixels that are only CONV.POOR
    // check old name first
    psU32 convPoor = psMetadataLookupU32(&status, masks, "POOR.WARP");
    if (!status) {
        convPoor = psMetadataLookupU32(&status, masks, "CONV.POOR");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup either CONV.POOR and POOR.WARP in mask recipe.\n");
            return false;
        }
    }
    // preserve pixels that are only suspect
    psU32 suspect = psMetadataLookupU32(&status, masks, "SUSPECT");
    if (!status) {
        psWarning("failed to lookup SUSPECT in mask recipe.\n");
        suspect = 0;
    }
    // preserve pixels that are in a diffraction spike
    psU32 spike = psMetadataLookupU32(&status, masks, "SPIKE");
    if (!status) {
        psWarning("failed to lookup SPIKE in mask recipe.\n");
        spike = 0;
    }
    // preserve pixels that are in core of a bright star
    psU32 starcore = psMetadataLookupU32(&status, masks, "STARCORE");
    if (!status) {
        psWarning("failed to lookup STARCORE in mask recipe.\n");
        starcore = 0;
    }

    // The value we return is the set of bits to censor. 
    // Including STREAK handles the case where any of these other bits have the same value as STREAK.
    // That was true in older 8 bit mask configurations.

    *pMaskCensor = ~(convPoor | suspect | starcore | spike) | streak;

    return true;
}

bool pmCensorMasked(pmConfig *config, psImage *image, psImage *mask, psImage *variance, long *pNumCensored)
{
    *pNumCensored = 0;

    psU32 censorMask;
    psU32 streakMask;
    if (!pmCensorGetMasks(config, &censorMask, &streakMask)) {
        psError(PS_ERR_UNKNOWN, false, "failed to lookup set of mask bits to censor.\n");
        return false;
    }

    if (~censorMask == 0) {
        // censoring masked pixels is not required
        return true;
    }

    double exciseValue;
    if (image->type.type == PS_TYPE_U16) {
        exciseValue = 0x7fff;
    } else if (image->type.type == PS_TYPE_U16) {
        exciseValue = 0xffff;
    } else if (image->type.type == PS_TYPE_F32) {
        exciseValue = NAN;
    } else {
        // XXX: sooner or later we'll get passed another image type
        psError(PS_ERR_PROGRAMMING, true, "unexpected image type: %d\n", image->type.type);
        return false;
    }

    for (int y=0; y<image->numRows; y++) {
        for (int x=0; x<image->numCols; x++) {
            psU16 maskVal = psImageGet(mask, x, y);
            if (maskVal & censorMask) {
                (*pNumCensored)++;
                psImageSet(image, x, y, exciseValue);
                if (variance) {
                    psImageSet(variance, x, y, exciseValue);
                }
            }
        }
    }

    return true;
}

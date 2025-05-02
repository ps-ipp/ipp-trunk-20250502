#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSim.h"

// Mode for bad pixels
typedef enum {
    BADPIX_ERROR,                       // Error: mode not set
    BADPIX_RANDOM,                      // Random values for bad pixels
    BADPIX_NAN,                         // NAN values for bad pixels
} badpixMode;


// The idea is that the pattern should be completely *deterministic* (despite the use of 'random' numbers ---
// with the seed specified, they should be deterministic) so that multiple calls of this function will result
// in the same pattern.  These are set to (really) random values so that they do not dark-subtract or
// flat-field.

bool ppSimBadPixels(pmReadout *readout, const pmConfig *config, psRandom *rng)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, false);
    PS_ASSERT_PTR_NON_NULL(readout->parent, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    bool mdok;                          // Status of MD lookup
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPSIM_RECIPE);
        return false;
    }

    psU64 seed = psMetadataLookupU64(&mdok, recipe, "BADPIX.SEED"); // Seed for RNG
    if (seed == 0 || !mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "BADPIX.SEED not set, or zero.");
        return false;
    }

    float frac = psMetadataLookupF32(&mdok, recipe, "BADPIX.FRAC"); // Fraction of bad pixels
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "BADPIX.FRAC not set.");
        return false;
    }
    if (frac == 0.0) {
        // Nothing to do
        return true;
    }

    int size = psMetadataLookupS32(&mdok, recipe, "BADPIX.SIZE"); // (Half-)size of bad pixels
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "BADPIX.SIZE not set.");
        return false;
    }

    psString modeString = psMetadataLookupStr(&mdok, recipe, "BADPIX.MODE"); // Mode for bad pixels
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "BADPIX.MODE not set.");
        return false;
    }
    badpixMode mode = BADPIX_ERROR;
    if (strcmp(modeString, "RANDOM") == 0) {
        mode = BADPIX_RANDOM;
    } else if (strcmp(modeString, "NAN") == 0) {
        mode = BADPIX_NAN;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "BADPIX.MODE not recognised.");
        return false;
    }

    psRandom *pseudoRNG = psRandomAllocSpecific(PS_RANDOM_TAUS, seed); // Pseudo-random number generator

    psImage *image = readout->image;    // Image of interest
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    for (int y = 0; y < numRows; y++) {
        int vMin = PS_MAX(y - size, 0), vMax = PS_MIN(y + size, numRows - 1); // Extent of CR
        for (int x = 0; x < numCols; x++) {
            if (psRandomUniform(pseudoRNG) < frac) {
                int uMin = PS_MAX(x - size, 0), uMax = PS_MIN(x + size, numCols - 1); // Extent of CR
                for (int v = vMin; v <= vMax; v++) {
                    for (int u = uMin; u <= uMax; u++) {
                        switch (mode) {
                          case BADPIX_RANDOM:
                            image->data.F32[v][u] *= psRandomGaussian(rng);
                            break;
                          case BADPIX_NAN:
                            image->data.F32[v][u] = NAN;
                            break;
                          default:
                            psAbort("Unrecognised mode: %x", mode);
                        }
                    }
                }
            }
        }
    }

    psFree(pseudoRNG);

    return true;
}

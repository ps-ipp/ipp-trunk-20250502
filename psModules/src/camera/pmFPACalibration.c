#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmConfig.h"
#include "pmDetrendDB.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"

#include "pmFPACalibration.h"

float pmFPADarkNorm(const pmFPA *fpa, const pmFPAview *view, float expTime)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NAN);
    PS_ASSERT_PTR_NON_NULL(view, NAN);

    psMetadata *darkNorms = psMetadataLookupMetadata(NULL, fpa->camera, "DARK.NORM"); // Dark normalisations
    if (!darkNorms) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find DARK.NORM in camera configuration.");
        return NAN;
    }

    const char *key = psMetadataLookupStr(NULL, fpa->camera, "DARK.NORM.KEY"); // Key for normalisation
    if (!key || strlen(key) == 0) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find DARK.NORM.KEY in camera configuration.");
        return NAN;
    }

    psString keyResolved = pmFPANameFromRule(key, fpa, view); // Resolved version
    if (!keyResolved || strlen(keyResolved) == 0) {
        psError(PS_ERR_UNKNOWN, false, "Unable to resolve DARK.NORM.KEY: %s", key);
        return NAN;
    }

    psMetadata *polyMD = psMetadataLookupMetadata(NULL, darkNorms, keyResolved); // Metadata with polynomial
    if (!polyMD) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find %s polynomial in the DARK.NORM metadata", keyResolved);
        psFree(keyResolved);
        return NAN;
    }

    psPolynomial1D *poly = psPolynomial1DfromMetadata(polyMD); // Polynomial
    if (!poly) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine polynomial from %s in the DARK.NORM metadata",
                keyResolved);
        psFree(keyResolved);
        return NAN;
    }
    psFree(keyResolved);

    float value = psPolynomial1DEval(poly, expTime);

    psFree(poly);

    return value;
}

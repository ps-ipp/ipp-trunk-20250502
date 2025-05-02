#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmSourceMasks.h"

#define ADD_MASK(HEADER, NAME, COMMENT) \
    psMetadataAddU32(HEADER, PS_LIST_TAIL, "SOURCE.MASK." #NAME, \
                     PS_META_REPLACE, COMMENT, (psU32)PM_SOURCE_MODE_##NAME);

bool pmSourceMasksHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    ADD_MASK(header, PSFMODEL        , "Fit with PSF model");
    ADD_MASK(header, EXTMODEL        , "Fit with extended model");
    ADD_MASK(header, FITTED          , "Fit with non-linear model");
    ADD_MASK(header, FAIL            , "Non-linear fit failed");
    ADD_MASK(header, POOR            , "Non-linear fit poor");
    ADD_MASK(header, PAIR            , "Fit with double PSF");
    ADD_MASK(header, PSFSTAR         , "Used to define PSF model");
    ADD_MASK(header, SATSTAR         , "Model peak is above saturation");
    ADD_MASK(header, BLEND           , "Blended with other sources");
    ADD_MASK(header, EXTERNAL        , "Based on supplied input position");
    ADD_MASK(header, BADPSF          , "Unable to estimate object PSF");
    ADD_MASK(header, DEFECT          , "Suspected defect");
    ADD_MASK(header, SATURATED       , "Suspected saturated (bleed trail)");
    ADD_MASK(header, CR_LIMIT        , "Suspected cosmic ray");
    ADD_MASK(header, EXT_LIMIT       , "Suspected extended");
    ADD_MASK(header, MOMENTS_FAILURE , "Failed to measure moments");
    ADD_MASK(header, SKY_FAILURE     , "Failed to measure local sky");
    ADD_MASK(header, SKYVAR_FAILURE  , "Failed to measure sky variance");
    ADD_MASK(header, BELOW_MOMENTS_SN, "Moments not measured due to low S/N");
    ADD_MASK(header, BIG_RADIUS      , "Small radius has poor moments");
    ADD_MASK(header, AP_MAGS         , "Measured aperture magnitude");
    ADD_MASK(header, BLEND_FIT       , "Fit as a blend");
    ADD_MASK(header, EXTENDED_FIT    , "Fit with full extended fit");
    ADD_MASK(header, EXTENDED_STATS  , "Calculated extended aperture stats");
    ADD_MASK(header, LINEAR_FIT      , "Fit with linear fit");
    ADD_MASK(header, NONLINEAR_FIT   , "Fit with non-linear fit");
    ADD_MASK(header, RADIAL_FLUX     , "Calculated radial flux measurements");
    ADD_MASK(header, SIZE_SKIPPED    , "Could not be determine size");
    ADD_MASK(header, ON_SPIKE        , "Source lands in bright star spike");
    ADD_MASK(header, ON_GHOST        , "Source lands in bright star ghost / glint");
    ADD_MASK(header, OFF_CHIP        , "Source centroid lands off chip edge");

    return true;
}

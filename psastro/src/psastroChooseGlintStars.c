/** @file psastroChooseGlintStars.c
 *
 *  @brief: Select stars by magnitude which are likely glint sources
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.21 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroChooseGlintStars (pmConfig *config, psArray *refs, const char *source) {

    bool status;
    float zeropt, exptime;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!\n");
        return false;
    }

    bool REFSTAR_MASK_GLINTS = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_GLINTS");
    if (!REFSTAR_MASK_GLINTS) return true;

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, source);
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data!\n");
        return false;
    }
    pmFPA *fpa = input->fpa;

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, NULL, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }

    // select the limiting magnitude
    double GLINT_MAX_MAG = psMetadataLookupF32 (&status, recipe, "GLINT_MAX_MAG");

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    GLINT_MAX_MAG += MagOffset;

    // There is no selection based on the location of the star. We need the full astrometry
    // solution before we can trust the star positions.

    // the refstars is a subset within range of this chip
    psArray *glintStars = psArrayAllocEmpty (100);

    // select the reference objects brighter than the cutoff magnitude within range of the FPA
    for (int i = 0; i < refs->n; i++) {
	pmAstromObj *ref = refs->data[i];
	if (ref->Mag > GLINT_MAX_MAG) continue;

	pmAstromObj *glint = pmAstromObjCopy(ref);
	psArrayAdd (glintStars, 100, glint);
	psFree (glint);
    }
    psTrace ("psastro", 4, "Added %ld glint stars\n", glintStars->n);

    psMetadataAdd (fpa->analysis, PS_LIST_TAIL, "PSASTRO.GLINT.STARS", PS_DATA_ARRAY, "possible glint stars", glintStars);
    psFree (glintStars);

    return true;
}

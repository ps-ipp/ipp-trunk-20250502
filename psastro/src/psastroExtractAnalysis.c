/** @file psastroExtractAnalysis.c
 *
 *  @brief 
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

/**
 * create a mask or mask regions based on the collection of reference stars that * are in the vicinity of each chip
 */
bool psastroExtractAnalysis (pmConfig *config) {

    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    // force this recipe value to be false (would cause an error in psastroChooseRefstars -- no rawstars!)
    psMetadataAddBool (recipe, PS_LIST_TAIL, "PSASTRO.MATCH.LUMFUNC", PS_META_REPLACE, "do not match luminosity functions", false);

    psLogMsg ("psastro", PS_LOG_INFO, "loading reference stars");

    // load the reference stars overlapping the data stars
    psArray *refs = psastroLoadRefstars(config, "PSASTRO.EXTRACT.ASTROM");
    if (!refs) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to load reference data\n");
        return false;
    }
    if (refs->n == 0) {
        psError(PSASTRO_ERR_REFSTARS, true, "no reference stars found");
        psFree(refs);
        return false;
    }

    if (!psastroChooseRefstars (config, refs, "PSASTRO.EXTRACT.ASTROM", false)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to select reference data for chips\n");
        psFree(refs);
        return false;
    }
    psFree(refs);

    // convert star positions to ghost positions and add to the readout->analysis data
    psastroExtractGhosts (config);

    psastroExtractStars (config);
    return true;
}

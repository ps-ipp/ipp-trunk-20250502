/** @file psastroAnalysis.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

/// Turn save on/off for a file
static void fileSave(pmConfig *config,  // Configuration
                     const char *name,  // Name of file
                     bool save          // Save file?
    )
{
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, name, 0); // File of interest
    if (!file) {
        psErrorClear();
        return;
    }
    file->save = save;
    return;
}


bool psastroAnalysis (pmConfig *config, psMetadata *stats) {

    bool status;
    int nStars;

    // measure the total elapsed time in psastroAnalysis.
    psTimerStart ("psastroAnalysis");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    if (!psastroUseModel (config, recipe)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to set model astrometry\n");
        return false;
    }

    // interpret the available initial astrometric information
    // apply the initial guess
    if (!psastroAstromGuess (&nStars, config)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to determine initial astrometry guess\n");
        return false;
    }
    if (nStars == 0) {
        // This is likely a data quality issue
        psWarning("No stars for astrometry analysis --- suspect bad data quality");
        if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
            psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE,
                             "No stars for astrometry", PSASTRO_ERR_DATA);
        }
        fileSave(config, "PSASTRO.OUTPUT", false);
        fileSave(config, "PSASTRO.OUTPUT.MASK", false);
        fileSave(config, "PSASTRO.OUT.REFSTARS", false);
        psErrorClear();
        return true;
    }

    if (!psastroRemoveClumpsRawstars(config)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to remove RAWSTAR clumps\n");
        return false;
    }

    // load the reference stars overlapping the data stars
    psArray *refs = psastroLoadRefstars(config, "PSASTRO.INPUT");
    if (!refs) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to load reference data\n");
        return false;
    }
    if (refs->n == 0) {
        psError(PSASTRO_ERR_REFSTARS, true, "no reference stars found");
        psFree(refs);
        return false;
    }

    bool skipastro = psMetadataLookupBool (&status, config->arguments, "PSASTRO.SKIP.ASTRO");

    if (!psastroChooseRefstars (config, refs, "PSASTRO.INPUT", skipastro)) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to select reference data for chips\n");
        psFree(refs);
        return false;
    }

    if (!psastroChooseGlintStars (config, refs, "PSASTRO.INPUT")) {
        psError (PSASTRO_ERR_UNKNOWN, false, "failed to select glint stars from reference star list\n");
        psFree(refs);
        return false;
    }
    psFree (refs);  // refs of interest are saved on readout->analysis

    // check the command-line arguments first
    bool chipastro = psMetadataLookupBool (&status, config->arguments, "PSASTRO.CHIP.MODE");
    if (!status) {
        chipastro = psMetadataLookupBool (&status, recipe, "PSASTRO.CHIP.MODE");
    }
    bool mosastro  = psMetadataLookupBool (&status, config->arguments, "PSASTRO.MOSAIC.MODE");
    if (!status) {
        mosastro  = psMetadataLookupBool (&status, recipe, "PSASTRO.MOSAIC.MODE");
    }

    if (skipastro) {
        chipastro = false;
        mosastro = false;
        psLogMsg ("psastro", 3, "skip astrometry mode, accepting input astrometry\n");
    } else if (!chipastro && !mosastro) {
        psLogMsg ("psastro", 3, "no astrometry mode selected, assuming chip astrometry\n");
        chipastro = true;
    }

    if (chipastro) {
        if (!psastroChipAstrom (config)) {
            // This is likely a data quality issue
            psWarning("Failed single chip astrometry --- suspect bad data quality");
            if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
                psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE,
                                 "Single chip astrometry failed", PSASTRO_ERR_DATA);
            }
            fileSave(config, "PSASTRO.OUTPUT", false);
            fileSave(config, "PSASTRO.OUTPUT.MASK", false);
            fileSave(config, "PSASTRO.OUT.REFSTARS", false);
            psErrorClear();
            return true;
        }
    }

    psLogMsg("psastro", 3, "TIMEMARK: psastroChipAstrom: %f sec\n", psTimerMark ("complete"));

    if (mosastro) {
	if (!psastroMosaicAstrom (config, stats)) {
            // This is likely a data quality issue
            psWarning("Failed mosaic astrometry --- suspect bad data quality");
            if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
                psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE,
                                 "Mosaic astrometry failed", PSASTRO_ERR_DATA);
            }
            fileSave(config, "PSASTRO.OUTPUT", false);
            fileSave(config, "PSASTRO.OUTPUT.MASK", false);
            fileSave(config, "PSASTRO.OUT.REFSTARS", false);
            psErrorClear();
            return true;
        }
    }

    psLogMsg("psastro", 3, "TIMEMARK: psastroMosaicAstrom: %f sec\n", psTimerMark ("complete"));

    if (!skipastro) {
        if (!psastroZeroPoint (config)) {
            psError(psErrorCodeLast(), false, "Failed to calculate zero point.");
            return false;
        }

        if (!psastroAstromGuessCheck (config)) {
            psError(psErrorCodeLast(), false, "Failed to check astrometry guess.");
            return false;
        }
    }

    if (!psastroMaskUpdates (config, stats)) {
        psError(psErrorCodeLast(), false, "Failed to generate dynamic masks.");
        return false;
    }

    return true;
}

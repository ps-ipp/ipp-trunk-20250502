/** @file psastroExtractGhosts.c
 *
 *  @brief calculate ghost FPA and Chip positions for the stars loaded on the FPA
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define ESCAPE(MSG) {							\
	psError(PS_ERR_UNKNOWN, false, "I/O failure in psastroMaskUpdate: %s", MSG); \
	psFree (view);							\
	return false;							\
    }

/**
 * calculate ghost FPA and Chip positions for the stars loaded on the FPA
 */
bool psastroExtractGhosts (pmConfig *config) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float zeropt, exptime;

    psLogMsg ("psastro", PS_LOG_INFO, "determine ghost positions");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    double EXTRACT_MAX_MAG = psMetadataLookupF32 (&status, recipe, "EXTRACT_MAX_MAG");

    // we have two input pmFPAfiles: PSASTRO.EXTRACT.INPUT and PSASTRO.EXTRACT.ASTROM.  both are in chip-mosaic or fpa format
    // we have already loaded the headers from PSASTRO.EXTRACT.ASTROM and determined the astrometry terms

    // select the input astrometry data (also carries the refstars)
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.EXTRACT.ASTROM");
    if (!astrom) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }
    pmFPA *fpa = astrom->fpa;

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, NULL, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    EXTRACT_MAX_MAG += MagOffset;

    pmFPAview *view = pmFPAviewAlloc (0);

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->fromFPA) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // select the raw objects for this readout (loaded in psastroExtract.c)
                psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
                if (refstars == NULL) { continue; }

                // identify the bright stars of interest
                for (int i = 0; i < refstars->n; i++) {
                    pmAstromObj *ref = refstars->data[i];
                    if (ref->Mag > EXTRACT_MAX_MAG) continue;

		    // XXX can make a more clever model...
		    double xFPA = -1*ref->FP->x;
		    double yFPA = -1*ref->FP->y;

		    double xChip, yChip;
		    pmChip *ghostChip = psastroFindChip (&xChip, &yChip, fpa, xFPA, yFPA);
		    if (!ghostChip) continue;
		    if (!ghostChip->cells) continue;
		    if (!ghostChip->cells->n) continue;
		    pmCell *ghostCell = ghostChip->cells->data[0];
		    if (!ghostCell) continue;
		    if (!ghostCell->readouts) continue;
		    if (!ghostCell->readouts->n) continue;
		    pmReadout *ghostReadout = ghostCell->readouts->data[0];
		    if (!ghostReadout) continue;

		    psArray *ghosts = psMetadataLookupPtr (&status, ghostReadout->analysis, "PSASTRO.GHOSTS");
		    if (ghosts == NULL) { 
			ghosts = psArrayAllocEmpty (100);
			if (!psMetadataAdd (ghostReadout->analysis, PS_LIST_TAIL, "PSASTRO.GHOSTS", PS_DATA_ARRAY, "astrometry matches", ghosts)) {
			  psError(PSASTRO_ERR_CONFIG, false, "failure to add ghosts to readout");
			  return false;
			}
			psFree (ghosts);
		    }

		    pmAstromObj *ghost = pmAstromObjAlloc ();
		    ghost->FP->x   = xFPA;
		    ghost->FP->y   = yFPA;
		    ghost->chip->x = xChip;
		    ghost->chip->y = yChip;
		    ghost->TP->x   = ref->FP->x; // XXX this is a bit of sleazy over-loading
		    ghost->TP->y   = ref->FP->y;
		    ghost->Mag     = ref->Mag;
		    
		    psArrayAdd (ghosts, 100, ghost);
		    psFree (ghost);
                }
            }
        }
    }

    psastroExtractFreeChipBounds();

    psFree (view);
    return true;
}

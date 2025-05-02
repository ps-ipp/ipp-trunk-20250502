/** @file psastroMosaicSetMatch.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroMosaicSetMatch (pmFPA *fpa, psMetadata *recipe, int iteration) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    pmFPAview *view = pmFPAviewAlloc (0);
    char radiusWord[64];

    // use small radius to match stars (assume starting astrometry is good)
    bool status = false;
    sprintf (radiusWord, "PSASTRO.MOSAIC.RADIUS.N%d", iteration);
    double RADIUS = psMetadataLookupF32 (&status, recipe, radiusWord);
    if (!status) {
        psAbort("Failed to lookup matching radius: %s", radiusWord);
    }

    int uniqIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.MOSAIC.UNIQ.ITER");
    if (!status) {
        psAbort("Failed to lookup matching PSASTRO.MOSAIC.UNIQ.ITER");
    }

    if (RADIUS <= 0.0) {
        if (iteration == 0) {
            psError(PS_ERR_IO, false, "Invalid match radius for first iteration: %s", radiusWord);
            psFree (view);
            return false;
        }
        psWarning ("skipping match for iteration %d\n", iteration);
        psFree (view);
        return true;
    }

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->fromFPA) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            // XXX there can only be one readout per chip, right?
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // select the raw objects for this readout
                psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
                if (rawstars == NULL) { continue; }

                // select the raw objects for this readout
                psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
                if (refstars == NULL) { continue; }
                psTrace ("psastro", 4, "Trying %ld refstars\n", refstars->n);

                psArray *matches = pmAstromRadiusMatchChip (rawstars, refstars, RADIUS);
                psTrace ("psastro", 4, "Matched %ld refstars\n", matches->n);

		if (iteration >= uniqIter) {
		    psArray *unique = pmAstromRadiusMatchUniq (rawstars, refstars, matches);
		    if (!unique) {
			psLogMsg ("psastro", 3, "failed to generate a uniq set of matched sources\n");
			return false; // this can only happen if alloc fails, not a data quality problem
		    }
		    psFree (matches);
		    matches = unique;
		}

                pmAstromVisualPlotMosaicMatches(rawstars, refstars, matches, iteration, recipe);

                // XXX drop the old one
                psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.MATCH", PS_DATA_ARRAY | PS_META_REPLACE, "astrometry matches", matches);
                psFree (matches);
            }
        }
    }
    psFree (view);
    return true;
}

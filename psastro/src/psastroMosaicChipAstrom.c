/** @file psastroMosaiciChipAstrom.c
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
# define NONLIN_TOL 0.001 ///< tolerance in pixels

bool psastroMosaicChipAstrom (pmFPA *fpa, psMetadata *stats, psMetadata *recipe, int iteration) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    pmFPAview *view = pmFPAviewAlloc (0);

    float minGoodChipFraction = psMetadataLookupF32 (&status, recipe, "PSASTRO.MOSAIC.MIN.GOOD.CHIP.FRACTION");
    if (!status) { 
      minGoodChipFraction = 0.1;      
      psWarning ("failed to find PSASTRO.MOSAIC.MIN.GOOD.CHIP.FRACTION in recipe, assuming default of %f", minGoodChipFraction); 
    }

    int nChipGood = 0;
    int nChipTotal = 0;

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
	nChipTotal ++;
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->toFPA) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            // XXX there can only be one readout per chip, right?
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // save WCS and analysis metadata in update header
		// (pull or create local view to entry on readout->analysis)
		psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
		if (!updates) {
		    updates = psMetadataAlloc ();
		    psMetadataAddMetadata (readout->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
		    psFree (updates);
		}

                if (!psastroMosaicOneChip (chip, readout, recipe, updates, iteration)) {
                    readout->data_exists = false;
                    psError(PS_ERR_UNKNOWN, false, "failed to find a solution for %d,%d,%d\n", view->chip, view->cell, view->readout);
		    psErrorStackPrint(stderr, "failure for one chip\n");
		    psErrorClear();
		    continue;
                }

                // create the header keywords to descripe the results
                if (!pmAstromWriteBilevelChip (updates, chip, NONLIN_TOL)) {
                    readout->data_exists = false;
                    psError(PS_ERR_UNKNOWN, false, "invalid solution for %d,%d,%d\n", view->chip, view->cell, view->readout);
		    psErrorStackPrint(stderr, "failure for one chip\n");
		    psErrorClear();
		    continue;
                }
		nChipGood ++;
            }
        }
    }
    psFree (view);

    // if basically the entire exposure is bad, return false (calling function sets bad quality)
    float fGood = nChipGood / ((float) nChipTotal);
    psLogMsg ("psastro", PS_LOG_INFO, "%d good chips of %d = %.2f\n", nChipGood, nChipTotal, fGood);
    if (fGood < minGoodChipFraction) {
      psWarning ("Too few good chips (%d of %d = %.2f), setting bad quality for this exposure\n", nChipGood, nChipTotal, fGood);
      // NOTE: set bad data quality here
      if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
	  psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Mosaic astrometry failed", PSASTRO_ERR_DATA);
      }
    }

    return true;
}

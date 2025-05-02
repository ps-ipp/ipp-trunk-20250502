/** @file psastroChipAstrom.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.29 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define NONLIN_TOL 0.001 ///< tolerance in pixels

bool psastroChipAstrom (pmConfig *config) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    // In this function, we are fitting the chip->FPA model, keeping the toTPA static.  
    // If the toTPA model has order > 1, then the WCS terms need to use the BiLevel
    // model.  It is possible for this conversion to fail.  However, if we are going to try
    // again with the mosaic model (fitting both), then we need to react to failure differently
    bool mosastro  = psMetadataLookupBool (&status, config->arguments, "PSASTRO.MOSAIC.MODE");
    if (!status) {
        mosastro  = psMetadataLookupBool (&status, recipe, "PSASTRO.MOSAIC.MODE");
    }

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPA *fpa = input->fpa;

    int numGoodChips = 0;               // Number of chips for which astrometry succeeds

    bool useBilevelWCS = (fpa->toTPA->x->nX > 1) || (fpa->toTPA->x->nY > 1);

    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        int numGoodCells = 0;           // Number of cells for which astrometry succeeds
        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            if (!chip->fromFPA) { continue; }

            // process each of the readouts
            int numGoodRO = 0;          // Number of readouts for which astrometry succeeds
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // select the raw objects for this readout
                psArray *rawstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
                if (rawstars == NULL) { continue; }

                // select the raw objects for this readout
                psArray *gridrawstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.GRID.RAWSTARS.SUBSET");
                if (gridrawstars == NULL) {
                    gridrawstars = rawstars;
                } else {
                    // the absolute minimum number of stars is 4 (for order = 1)
                    if (gridrawstars->n < 4) {
                        readout->data_exists = false;
                        psLogMsg ("psastro", 3, "insufficient gird rawstars (%ld)", gridrawstars->n);
                        continue;
                    }
                }

                psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
                if (refstars == NULL) { continue; }

# if (0)
		static int Nchip = 0;
		if (1) {
		  // XXX test
		  char filename[64];
		  snprintf (filename, 64, "refstars.%02d.dat", Nchip);
		  FILE *outfile = fopen (filename, "w");
		  assert (outfile);
		  for (int nn = 0; nn < refstars->n; nn++) {
		    pmAstromObj *ref = refstars->data[nn];
		    fprintf (outfile, "%lf %lf  %lf %lf  %lf %lf  %lf %lf : %f %f\n", 
			     ref->sky->r*PS_DEG_RAD, ref->sky->d*PS_DEG_RAD,
			     ref->TP->x, ref->TP->y, 
			     ref->FP->x, ref->FP->y, 
			     ref->chip->x, ref->chip->y, ref->Mag, ref->magCal);
		  }
		  fclose (outfile);
		}
		if (1) {
		  // XXX test
		  char filename[64];
		  snprintf (filename, 64, "rawstars.%02d.dat", Nchip);
		  FILE *outfile = fopen (filename, "w");
		  assert (outfile);
		  for (int nn = 0; nn < gridrawstars->n; nn++) {
		    pmAstromObj *ref = gridrawstars->data[nn];
		    fprintf (outfile, "%lf %lf  %lf %lf  %lf %lf  %lf %lf : %f %f\n", 
			     ref->sky->r*PS_DEG_RAD, ref->sky->d*PS_DEG_RAD,
			     ref->TP->x, ref->TP->y, 
			     ref->FP->x, ref->FP->y, 
			     ref->chip->x, ref->chip->y, ref->Mag, ref->magCal);
		  }
		  fclose (outfile);
		  Nchip ++;
		}
# endif

                // the absolute minimum number of stars is 4 (for order = 1)
                if ((rawstars->n < 4) || (refstars->n < 4)) {
                    readout->data_exists = false;
                    psLogMsg ("psastro", 3, "insufficient rawstars (%ld) or refstars (%ld)", rawstars->n, refstars->n);
                    continue;
                }

                // save WCS and analysis metadata in update header
		// (pull or create local view to entry on readout->analysis)
		psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
		if (!updates) {
		    updates = psMetadataAlloc ();
		    psMetadataAddMetadata (readout->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
		    psFree (updates);
		}

                // XXX update the header with info to reflect the failure
                if (!psastroOneChipGrid (fpa, chip, refstars, gridrawstars, recipe, updates)) {
                    readout->data_exists = false;
                    psLogMsg ("psastro", 3, "failed to find a solution\n");
                    continue;
                }
                // XXX update the header with info to reflect the failure
                if (!psastroOneChipFit (fpa, chip, readout, refstars, rawstars, recipe, updates)) {
                    readout->data_exists = false;
                    psLogMsg ("psastro", 3, "failed to find a solution\n");
                    continue;
                }

                numGoodRO++;

                psU64 astrom_chip_val = 1;
                astrom_chip_val <<= view->chip;
                psMetadataAddU64 (updates, PS_LIST_TAIL, "ASTROM_CHIPS", PS_META_REPLACE, "chips that passed astrometry", astrom_chip_val);

                // write the elapsed time here; this will be updated in psastroMosaicAstrometry, if called
                psMetadataAddF32 (updates, PS_LIST_TAIL, "DT_ASTR", PS_META_REPLACE, "elapsed psastro time", psTimerMark ("psastroAnalysis"));
		
		// if the toTPA distortion model uses higher-order terms, then we need to use BiLevel astrometry 
		if (useBilevelWCS) {
		    // create the header keywords to descripe the results
		    if (!pmAstromWriteBilevelChip (updates, chip, NONLIN_TOL)) {
			if (!mosastro) { readout->data_exists = false; } // unless we are going to try again, give up on this chip
			psError(PS_ERR_UNKNOWN, false, "invalid solution for %d,%d,%d\n", view->chip, view->cell, view->readout);
			psErrorStackPrint(stderr, "failure to generate WCS keywords for one chip\n");
			psErrorClear();
			continue;
		    }
		} else {
		    fpa->wcsCDkeys = psMetadataLookupBool(&status, recipe , "PSASTRO.WCS.USECDKEYS");
		    pmAstromWriteWCS (updates, fpa, chip, NONLIN_TOL);
		}

                if (psTraceGetLevel("psastro.dump") > 0) {

                    char *filename = NULL;
                    char *chipname = psMetadataLookupStr (&status, chip->concepts, "CHIP.NAME");

                    psStringAppend (&filename, "rawstars.ch.%s.dat", chipname);
                    psastroDumpStars (rawstars, filename);
                    psFree (filename);
                    filename = NULL;

                    psStringAppend (&filename, "refstars.ch.%s.dat", chipname);
                    psastroDumpStars (refstars, filename);
                    psFree (filename);
                    filename = NULL;
                }
            }
            if (numGoodRO > 0) {
                numGoodCells++;
            }
        }
        if (numGoodCells > 0) {
            numGoodChips++;
        }
    }

    if (useBilevelWCS) {
	// save WCS and analysis metadata in update header.
	// (pull or create local view to entry on readout->analysis)
	psMetadata *updates = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
	if (!updates) {
	    updates = psMetadataAlloc ();
	    psMetadataAddMetadata (fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
	    psFree (updates);
	}
	if (!pmAstromWriteBilevelMosaic (updates, fpa, NONLIN_TOL)) {
	    psError(psErrorCodeLast(), false, "Failed to save header terms");
	    return false;
	}
    }

    if (fpa->chips->n == 1 && numGoodChips == 0) {
        psError(PSASTRO_ERR_UNKNOWN, false, "Failed to fit single chip.");
        return false;
    }

    if (numGoodChips == 0) {
        psError(PSASTRO_ERR_UNKNOWN, false, "Failed to fit any chips");
        return false;
    }

    if (!psastroFixChips (config, recipe)) {
        psError(PSASTRO_ERR_UNKNOWN, false, "failed to align problematic chips");
        return false;
    }

    psFree (view);
    return true;
}

/* coordinate frame hierachy
   pixels (on a given readout)
   cell
   chip
   FP (focal plane)
   TP (tangent plane)
   sky (ra, dec)
*/

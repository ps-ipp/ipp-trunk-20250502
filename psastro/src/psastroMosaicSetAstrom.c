/** @file psastroMosaicSetAstrom.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroMosaicSetAstrom (pmFPA *fpa) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    pmFPAview *view = pmFPAviewAlloc (0);

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
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

		// select the raw objects for this readout
		psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS");
		if (rawstars == NULL) { continue; }

		for (int i = 0; i < rawstars->n; i++) {
		    pmAstromObj *raw = rawstars->data[i];
	
		    psPlaneTransformApply (raw->FP, chip->toFPA, raw->chip);
		    psPlaneTransformApply (raw->TP, fpa->toTPA, raw->FP);
		    psDeproject (raw->sky, raw->TP, fpa->toSky);
		}

		psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS");
		if (refstars == NULL) { continue; }

		for (int i = 0; i < refstars->n; i++) {
		    pmAstromObj *ref = refstars->data[i];
	
		    psProject (ref->TP, ref->sky, fpa->toSky);
		    psPlaneTransformApply (ref->FP, fpa->fromTPA, ref->TP);
		    psPlaneTransformApply (ref->chip, chip->fromFPA, ref->FP);
		}
	    }
	}
    }
    psFree (view);
    return true;
}

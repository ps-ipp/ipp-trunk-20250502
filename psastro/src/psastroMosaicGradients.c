/** @file psastroMosaicGradients.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
static int nPass = 0;

bool psastroMosaicDistortionFromGradients (pmFPA *fpa, psMetadata *recipe) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    psArray *gradients = NULL;

    // Measure the gradient as a function of position.  This must be performed between the
    // corrected ref->TP and the observed raw->FP, for which the distortion is a perturbation.

    pmFPAview *view = pmFPAviewAlloc (0);

    int nXcell = psMetadataLookupS32 (&status, recipe, "PSASTRO.MOSAIC.GRADIENT.NX");
    int nYcell = psMetadataLookupS32 (&status, recipe, "PSASTRO.MOSAIC.GRADIENT.NY");

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
	if (!chip->toFPA) { continue; }
	
	psRegion *region = pmChipExtent (chip);

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

		psArray *match = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
		if (match == NULL) { continue; }

		// measure the local gradients for this set of stars
		// XXX 2,2 are the number of test boxes on the chip.  put this in the recipe
		gradients = pmAstromMeasureGradients (gradients, rawstars, refstars, match, region, nXcell, nYcell);
	    }
	}
	psFree (region);
    }

    // if desired, dump the gradients to a file
    if (psTraceGetLevel("psastro.dump") > 0) { 
	char name[80];
	sprintf (name, "gradients.%d.dat", nPass); 
	psastroDumpGradients (gradients, name); 
	nPass ++;
    }

    // Fit the gradient field and convert to the distortion terms.

    // allocate mosaic-level polynomial transformation and set masks needed by DVO
    int order = psMetadataLookupF32 (&status, recipe, "PSASTRO.MOSAIC.ORDER");
    if (!status) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to find mosaic distortion fit order\n");
	psFree (gradients);
	psFree (view);
        return false;
    }

    // forward transformations (chip->fpa->tpa->sky) use Ordinary polynomials
    psFree (fpa->toTPA);
    fpa->toTPA = psPlaneTransformAlloc (order, order, PS_POLYNOMIAL_ORD);
    for (int i = 0; i <= fpa->toTPA->x->nX; i++) {
        for (int j = 0; j <= fpa->toTPA->x->nY; j++) {
            if (i + j > order) {
		fpa->toTPA->x->coeffMask[i][j] = PS_POLY_MASK_SET;
		fpa->toTPA->y->coeffMask[i][j] = PS_POLY_MASK_SET;
            }
        }
    }

    // physical pixel scale in microns per pixel (FP is in physical units, chip is in pixels)
    double pixelScale = psMetadataLookupF32 (&status, recipe, "PSASTRO.PIXEL.SCALE");
    if (!status) {
	psError(PS_ERR_IO, false, "Failed to lookup pixel scale"); 
	psFree (gradients);
	psFree (view);
	return false; 
    } 

    // fit the measured gradients with the telescope distortion model (polynomial order based on toTPA)
    if (!pmAstromFitDistortion (fpa, gradients, pixelScale)) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to fit the distortion terms\n");
	psFree (gradients);
	psFree (view);
        return false;
    }
	
    psFree (gradients);
    psFree (view);
    return true;
}


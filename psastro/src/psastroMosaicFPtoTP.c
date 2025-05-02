/** @file psastroMosaicFPtoTP.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

psPlaneTransform *psastroMosaicFitRotAndScale (pmFPA *fpa) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    // XXX first pass: fit and remove any linear fp->tp transformation
    // accumulate FP(x,y) & TP(x,y) in a single set of vectors

    psVector *X  = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *Y  = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *x  = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *y  = psVectorAllocEmpty(100, PS_TYPE_F32);

    pmFPAview *view = pmFPAviewAlloc (0);

    // int nPts = 0;
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
		psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
		if (rawstars == NULL) { continue; }

		// select the raw objects for this readout
		psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
		if (refstars == NULL) { continue; }

		psArray *match = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
		if (match == NULL) { continue; }

		// take the matched stars, first fit
		for (int i = 0; i < match->n; i++) {
		    pmAstromMatch *pair = match->data[i];
		    pmAstromObj *rawStar = rawstars->data[pair->raw];
		    pmAstromObj *refStar = refstars->data[pair->ref];

		    // independent variables
		    X->data.F32[X->n] = refStar->TP->x;
		    Y->data.F32[Y->n] = refStar->TP->y;

		    // fitted values
		    x->data.F32[x->n] = rawStar->FP->x;
		    y->data.F32[y->n] = rawStar->FP->y;

		    psVectorExtend (X, 100, 1);
		    psVectorExtend (Y, 100, 1);
		    psVectorExtend (x, 100, 1);
		    psVectorExtend (y, 100, 1);
		}
	    }
	}
    }
    // x->n = y->n = X->n = Y->n = nPts;

    // linear fit without xy cross term
    psPlaneTransform *map = psPlaneTransformAlloc (1, 1, PS_POLYNOMIAL_ORD);
    map->x->coeffMask[1][1] = PS_POLY_MASK_SET;
    map->y->coeffMask[1][1] = PS_POLY_MASK_SET;

    // constant errors
    psVector *mask = psVectorAlloc (X->n, PS_TYPE_VECTOR_MASK);
    psVectorInit (mask, 0);

    // the stats options supplied are used to perform the clip fitting
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    stats->clipIter = 1;

    // fit TP-to-FP transformation

    // we run 3 cycles clipping in each of x and y, with only one iteration each.
    // XXX use the stats lookups functions to get the width and center
    for (int i = 0; i < 3; i++) {
	if (!psVectorClipFitPolynomial2D (map->x, stats, mask, 0xff, x, NULL, X, Y)) {
            psError(PS_ERR_UNKNOWN, false, "failure in clip-fitting for x\n");
	    psFree (map);
	    map = NULL;
	    goto escape;
	}
        psTrace ("psastro", 3, "x resid: %f +/- %f (%ld of %ld)\n", stats->clippedMean, stats->clippedStdev, stats->clippedNvalues, x->n);

        if (!psVectorClipFitPolynomial2D (map->y, stats, mask, 0xff, y, NULL, X, Y)) {
            psError(PS_ERR_UNKNOWN, false, "failure in clip-fitting for y\n");
	    psFree (map);
	    map = NULL;
	    goto escape;
	}
        psTrace ("psastro", 3, "y resid: %f +/- %f (%ld of %ld)\n", stats->clippedMean, stats->clippedStdev, stats->clippedNvalues, y->n);
    }

escape:
    psFree (x);
    psFree (y);
    psFree (X);
    psFree (Y);
    psFree (mask);
    psFree (view);
    psFree (stats);

    return (map);
}

// apply the rotation and scale to all stars in PSASTRO.REFSTARS (also adjusts
// PSASTRO.REFSTARS.SUBSET since they are the same pointers)
bool psastroMosaicApplyRotAndScale (pmFPA *fpa, psPlaneTransform *TPtoFP) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    pmFPAview *view = pmFPAviewAlloc (0);
    psPlane newTP;

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
		psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS");
		if (refstars == NULL) { continue; }

		// take the matched stars, first fit
		for (int i = 0; i < refstars->n; i++) {
		    pmAstromObj *refStar = refstars->data[i];

		    // Correct the current reference star TP coordinates to the nearly-FP
		    // system.  We note two points here: 1) the corrected TP coordinates are
		    // NOT consistent with the sky coordinates, 2) the remaining differnce
		    // between the new TP reference coords and the observerd FP coordinates is
		    // only distortion

		    psPlaneTransformApply (&newTP, TPtoFP, refStar->TP);
		    refStar->TP->x = newTP.x;
		    refStar->TP->y = newTP.y;
		}
	    }
	}
    }
    psFree (view);
    return true;
}

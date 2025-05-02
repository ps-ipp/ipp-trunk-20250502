/** @file psastroUtils.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.25 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define RENORM 0

// fix this to look up the value in the chip concepts
static double getChipPixelScale (pmChip *chip) {
    return 10.0;
}

// I have an FPA structure with multiple chips.  we have loaded or measured astrometry for each
// chip with independent pixel/degree scaling values.  These will naturally compensate locally
// somewhat for the telescope distortion.  to measure the telescope distortion, we need to
// force the chips to have the same pixel scale and measure the difference from that solution.
// Convert an FPA with disparate pixel scales to a common pixel scale (perhaps depending on the
// chip -- eg, TC3)

bool psastroMosaicCommonScale (pmFPA *fpa, psMetadata *recipe) {

    // options : use the MIN or MAX chip as global reference or supplied pixel scales
    // (microns/pixel), which may depend on the chip

    float pixelScaleUse = 1.0, pixelScale1 = 1.0,  pixelScale2 = 1.0,  pixelScale = 1.0;
    psVector *oldScale = psVectorAllocEmpty (fpa->chips->n, PS_TYPE_F32);

    char *option = psMetadataLookupStr (NULL, recipe, "PSASTRO.COMMON.SCALE.OPTION");
    if (option == NULL) {
        psError(PSASTRO_ERR_DATA, false, "no choice set for common scale option\n");
        return false;
    }

    bool useExternal = true;
    int nobj = 0;

    // find the min or max scale chip
    if (!strcasecmp (option, "MIN") || !strcasecmp (option, "MAX")) {

        bool useMax = !strcasecmp (option, "MAX");
        pixelScaleUse = (useMax) ? FLT_MIN : FLT_MAX;

        for (int i = 0; i < fpa->chips->n; i++) {
            pmChip *chip = fpa->chips->data[i];
            if (!chip->process || !chip->file_exists) { continue; }
            if (!chip->toFPA) { continue; }

            if (chip->cells->n == 0) { continue; }
            pmCell *cell = chip->cells->data[0];
            if (!cell->process || !cell->file_exists) { continue; }

            if (cell->readouts->n == 0) { continue; }
            pmReadout *readout = cell->readouts->data[0];
            if (! readout->data_exists) { continue; }

            pixelScale1 = hypot (chip->toFPA->x->coeff[1][0], chip->toFPA->x->coeff[0][1]);
            pixelScale2 = hypot (chip->toFPA->y->coeff[1][0], chip->toFPA->y->coeff[0][1]);
            pixelScale = 0.5*(pixelScale1 + pixelScale2);
            oldScale->data.F32[nobj++] = pixelScale;
            pixelScaleUse = (useMax) ? PS_MAX (pixelScale, pixelScaleUse) : PS_MIN (pixelScale, pixelScaleUse);
        }
        useExternal = false;
        oldScale->n = nobj;
    }


    // rescale each chip by the reference scale
    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->toFPA) { continue; }

        psPlaneTransform *toFPA = chip->toFPA;
        psPlaneTransform *fromFPA = chip->fromFPA;

        pixelScale1 = hypot (toFPA->x->coeff[1][0], toFPA->x->coeff[0][1]);
        pixelScale2 = hypot (toFPA->y->coeff[1][0], toFPA->y->coeff[0][1]);

        if (useExternal) {
            pixelScaleUse = getChipPixelScale (chip);
        }

        for (int i = 0; i <= toFPA->x->nX; i++) {
            for (int j = 0; j <= toFPA->x->nX; j++) {
                toFPA->x->coeff[i][j] *= pixelScaleUse/pixelScale1;
                toFPA->y->coeff[i][j] *= pixelScaleUse/pixelScale2;
                fromFPA->x->coeff[i][j] *= pixelScale1/pixelScaleUse;
                fromFPA->y->coeff[i][j] *= pixelScale2/pixelScaleUse;
            }
        }
    }
    psastroMosaicSetAstrom (fpa);
    if (!useExternal) {
        pmAstromVisualPlotCommonScale (fpa, oldScale);
    }
    psFree (oldScale);
    return true;
}

bool psastroUpdateChipToFPA (pmFPA *fpa, pmChip *chip) {

    psRegion *region = pmChipPixels (chip);
    psFree (chip->fromFPA);
    chip->fromFPA = psPlaneTransformInvert (NULL, chip->toFPA, *region, 50, 4);
    psFree (region);

    // XXX EAM 2022.09.22 : for a specific case, psPlaneTransformInvert fails.
    // This probably means the solution was poor in any case.  
    // Some options:
    // 1) skip the chip in psastroAstromGuessCheck (supply bad corners)
    // 2) mark this chip as bad (return an error here and trap in psastroMosaicOneChip)
    // 3) warn of the failure:
    if (!chip->fromFPA) {
	char *name = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
	psLogMsg ("psastro", PS_LOG_INFO, "WARNING: failure to invert toFPA for %s", name);
    }

    // loop over cells in this chip
    for (int nCell = 0; nCell < chip->cells->n; nCell++) {
	pmCell *cell = chip->cells->data[nCell];
	if (!cell->process || !cell->file_exists) { continue; }

	// loop over readouts in this cell
	for (int nRead = 0; nRead < cell->readouts->n; nRead++) {
	    pmReadout *readout = cell->readouts->data[nRead];
	    if (! readout->data_exists) { continue; }

	    // select the raw objects for this readout
	    psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS");
	    if (rawstars) { 
		for (int i = 0; i < rawstars->n; i++) {
		    pmAstromObj *raw = rawstars->data[i];
		    psPlaneTransformApply (raw->FP, chip->toFPA, raw->chip);
		    psPlaneTransformApply (raw->TP, fpa->toTPA, raw->FP);
		    psDeproject (raw->sky, raw->TP, fpa->toSky);
		}
	    }

	    psArray *calstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.CALSTARS");
	    if (calstars) { 
		for (int i = 0; i < calstars->n; i++) {
		    pmAstromObj *cal = calstars->data[i];
		    psPlaneTransformApply (cal->FP, chip->toFPA, cal->chip);
		    psPlaneTransformApply (cal->TP, fpa->toTPA, cal->FP);
		    psDeproject (cal->sky, cal->TP, fpa->toSky);
		}
	    }

	    psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS");
	    if (refstars) { 
		for (int i = 0; i < refstars->n; i++) {
		    pmAstromObj *ref = refstars->data[i];
		    psPlaneTransformApply (ref->chip, chip->fromFPA, ref->FP);
		    // if chip->fromFPA is NULL (non-invertable), the action is skipped
		}
	    }
	}
    }

    return true;
}

# if 0

bool psastroSelectBrightStars (pmFPA *fpa, psMetadata *config) {

    bool status;

    // add exclusions for objects on some basis?
    int MAX_NSTARS = psMetadataLookupS32 (&status, config, "PSASTRO.STARS.MAX");

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        for (int j = 0; j < chip->cells->n; j++) {
            pmCell *cell = chip->cells->data[j];
            for (int k = 0; k < cell->readouts->n; k++) {
                pmReadout *readout = cell->readouts->data[k];

                psArray *stars = psMetadataLookupPtr (&status, readout->analysis, "STARS.FULLSET");
                stars = psArraySort (stars, pmAstromObjSortByMag);

                int nSubset = PS_MIN (MAX_NSTARS, stars->n);
                psArray *subset = psArrayAlloc (nSubset);

                for (int i = 0; i < nSubset; i++) {
                    subset->data[i] = stars->data[i];
                }
                psMetadataAdd (readout->analysis, PS_LIST_TAIL, "STARS.SUBSET", PS_DATA_ARRAY, "stars from analysis", subset);
            }
        }
    }
    return true;
}

psPlaneDistort *psPlaneDistortCopy (psPlaneDistort *input) {

    psPlaneDistort *output = psPlaneDistortAlloc (input->x->nX, input->x->nY, input->x->nZ, input->x->nT);

    for (int i = 0; i < input->x->nX; i++) {
        for (int j = 0; j < input->x->nY; j++) {
            for (int k = 0; k < input->x->nZ; k++) {
                for (int m = 0; m < input->x->nT; m++) {
                    // x-terms
                    output->x->mask[i][j][k][m]     = input->x->mask[i][j][k][m];
                    output->x->coeff[i][j][k][m]    = input->x->coeff[i][j][k][m];
                    output->x->coeffErr[i][j][k][m] = input->x->coeffErr[i][j][k][m];
                    // y-terms
                    output->y->mask[i][j][k][m]     = input->y->mask[i][j][k][m];
                    output->y->coeff[i][j][k][m]    = input->y->coeff[i][j][k][m];
                    output->y->coeffErr[i][j][k][m] = input->y->coeffErr[i][j][k][m];
                }
            }
        }
    }
    return (output);
}

psPlaneTransform *psPlaneTransformCopy (psPlaneTransform *input) {

    psPlaneTransform *output = psPlaneTransformAlloc (input->x->nX, input->x->nY, input->x->type);

    for (int i = 0; i < input->x->nX; i++) {
        for (int j = 0; j < input->x->nY; j++) {
            // x-terms
            output->x->mask[i][j]     = input->x->mask[i][j];
            output->x->coeff[i][j]    = input->x->coeff[i][j];
            output->x->coeffErr[i][j] = input->x->coeffErr[i][j];
            // y-terms
            output->y->mask[i][j]     = input->y->mask[i][j];
            output->y->coeff[i][j]    = input->y->coeff[i][j];
            output->y->coeffErr[i][j] = input->y->coeffErr[i][j];
        }
    }
    return (output);
}

psProjection *psProjectionCopy (psProjection *input) {

    psProjection *output = psProjectionAlloc (input->R, input->D, input->Xs, input->Ys, input->type);
    return (output);
}

// returns the rotation term, forcing positive parity
double psPlaneTransformGetRotation (psPlaneTransform *map) {

    if (map->x->nX < 1) return 0;
    if (map->x->nY < 1) return 0;

    if (map->y->nX < 1) return 0;
    if (map->y->nY < 1) return 0;

    double pc1_1 = map->x->coeff[1][0];
    double pc1_2 = map->x->coeff[0][1];
    double pc2_1 = map->y->coeff[1][0];
    double pc2_2 = map->y->coeff[0][1];

    double px = SIGN (pc1_1);
    double py = SIGN (pc2_2);

    // both x and y terms imply an angle. take the average
    double t1 = -atan2 (px*pc1_2, px*pc1_1);
    double t2 = +atan2 (py*pc2_1, py*pc2_2);

    // careful near -pi,+pi boundary...
    if (t1 - t2 > M_PI/2) t2 += 2*M_PI;
    if (t2 - t1 > M_PI/2) t1 += 2*M_PI;

    double theta = 0.5*(t1 + t2);
    while (theta < M_PI) theta += 2*M_PI;
    while (theta > M_PI) theta -= 2*M_PI;

    return (theta);
}

# endif

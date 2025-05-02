/** @file psastroAstromGuess.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.35 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define DEBUG 0

/**
 * \brief This function loads the header WCS astrometry terms into the fpa 
 * terms and applies the astrometry to the detected objects.
 * 
 * This function assumes the initial astrometry arrives in the form of WCS 
 * keywords in the headers corresponding to the chips.
 */
bool psastroAstromGuess (int *nStars, pmConfig *config) {

    bool newFPA = true;
    bool status = false;
    double RAmin  = +FLT_MAX;
    double RAmax  = -FLT_MAX;
    double DECmin = +FLT_MAX;
    double DECmax = -FLT_MAX;

    double RAminSky = NAN;
    double RAmaxSky = NAN;

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    *nStars = 0;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!");
        return false;
    }

    // have we already supplied the astrometry from the model?
    bool useModel = psMetadataLookupBool (&status, config->arguments, "PSASTRO.USE.MODEL");
    if (!status) {
        useModel = psMetadataLookupBool (&status, recipe, "PSASTRO.USE.MODEL");
    }

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    // physical pixel scale in microns per pixel
    double pixelScale = psMetadataLookupF32 (&status, recipe, "PSASTRO.PIXEL.SCALE");
    if (!status) {
        psError(PS_ERR_IO, true, "Failed to lookup pixel scale");
        return false;
    }

    psVector *cornerL = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerM = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerP = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerQ = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerR = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerD = psVectorAllocEmpty (100, PS_TYPE_F32);

    pmFPA *fpa = input->fpa;

    // this call only works if we have loaded a model : seg fault if not
    if (DEBUG && useModel) psastroDumpCorners ("corners.up.guess1.dat", "corners.dn.guess1.dat", fpa);

    // load mosaic-level astrometry?
    bool bilevelAstrometry = false;
    if (!useModel) {
        psastroAstromGuessSetFPA (fpa, &bilevelAstrometry);
    }

    pmFPAview *view = pmFPAviewAlloc (0);
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists || !chip->data_exists) { continue; }

        if (!useModel) {
            if (!psastroAstromGuessSetChip (fpa, chip, view, pixelScale, bilevelAstrometry)) continue;
        }

	if (!chip->toFPA || !chip->fromFPA) {
	  char *name = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
	  fprintf (stderr, "no astrom model for %s, skipping\n", name);
	  continue;
	}

        if (newFPA) {
            newFPA = false;
            while (fpa->toSky->R <        0) fpa->toSky->R += 2.0*M_PI;
            while (fpa->toSky->R > 2.0*M_PI) fpa->toSky->R -= 2.0*M_PI;
            RAminSky = fpa->toSky->R - M_PI;
            RAmaxSky = fpa->toSky->R + M_PI;
        }

        // report and save the current best guess for the chip 0,0 pixel coordinates
        {
            psPlane ptCH, ptFP, ptTP;
            psSphere ptSky;

            ptCH.x = 0;
            ptCH.y = 0;
            psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
            psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
            psDeproject (&ptSky, &ptTP, fpa->toSky);
            psLogMsg ("psastro", 3, "0,0 pix for chip %3d = %f,%f\n", view->chip, DEG_RAD*ptSky.r, DEG_RAD*ptSky.d);

            psVectorAppend (cornerL, ptFP.x);
            psVectorAppend (cornerM, ptFP.y);
            psVectorAppend (cornerP, ptTP.x);
            psVectorAppend (cornerQ, ptTP.y);
            psVectorAppend (cornerR, ptSky.r);
            psVectorAppend (cornerD, ptSky.d);
        }

        // apply the new WCS guess data to all of the data in the readouts
        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS");
                if (rawstars == NULL) { continue; }

                *nStars += rawstars->n;
                for (int i = 0; i < rawstars->n; i++) {
                    pmAstromObj *raw = rawstars->data[i];

                    psPlaneTransformApply (raw->FP, chip->toFPA, raw->chip);
                    psPlaneTransformApply (raw->TP, fpa->toTPA, raw->FP);
                    psDeproject (raw->sky, raw->TP, fpa->toSky);

                    // rationalize ra to sky range centered on boresite
                    while (raw->sky->r < RAminSky) raw->sky->r += 2.0*M_PI;
                    while (raw->sky->r > RAmaxSky) raw->sky->r -= 2.0*M_PI;

                    RAmin = PS_MIN (raw->sky->r, RAmin);
                    RAmax = PS_MAX (raw->sky->r, RAmax);

                    DECmin = PS_MIN (raw->sky->d, DECmin);
                    DECmax = PS_MAX (raw->sky->d, DECmax);
                }

                // dump or plot the resulting projected positions
                if (psTraceGetLevel("psastro.dump") > 0) {
                    psastroDumpRawstars (rawstars, fpa, chip);
                }

                pmAstromVisualPlotRawStars(rawstars, fpa, chip, recipe);

                if (psTraceGetLevel("psastro.plot") > 0) {
                    psastroPlotRawstars (rawstars, fpa, chip, recipe);
                }

                // Next if we are using a different set of stars for grid search fill out their pmAstromObjs
                psArray *grid_rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.GRID.RAWSTARS");
                if (grid_rawstars == NULL || grid_rawstars == rawstars) { continue; }

                for (int i = 0; i < grid_rawstars->n; i++) {
                    pmAstromObj *raw = grid_rawstars->data[i];

                    psPlaneTransformApply (raw->FP, chip->toFPA, raw->chip);
                    psPlaneTransformApply (raw->TP, fpa->toTPA, raw->FP);
                    psDeproject (raw->sky, raw->TP, fpa->toSky);

                    // rationalize ra to sky range centered on boresite
                    while (raw->sky->r < RAminSky) raw->sky->r += 2.0*M_PI;
                    while (raw->sky->r > RAmaxSky) raw->sky->r -= 2.0*M_PI;

                    RAmin = PS_MIN (raw->sky->r, RAmin);
                    RAmax = PS_MAX (raw->sky->r, RAmax);

                    DECmin = PS_MIN (raw->sky->d, DECmin);
                    DECmax = PS_MAX (raw->sky->d, DECmax);
                }
                // XXX: should we plot grid_rawstars?
            }
        }
    }

    if (DEBUG) psastroDumpCorners ("corners.up.guess2.dat", "corners.dn.guess2.dat", fpa);

    // how many total sources are available to us?
    psMetadataAddS32 (recipe, PS_LIST_TAIL, "NTOTSTAR",  PS_META_REPLACE, "", *nStars);
    if (*nStars == 0) {
        psLogMsg ("psastro", 2, "no sources available for astrometry\n");
        psFree (view);
        return true;
    }

    psLogMsg ("psastro", 2, "loaded raw data from %f,%f to %f,%f\n",
              DEG_RAD*RAmin, DEG_RAD*DECmin,
              DEG_RAD*RAmax, DEG_RAD*DECmax);

    psMetadataAddF32 (recipe, PS_LIST_TAIL, "RA_MIN",  PS_META_REPLACE, "", RAmin);
    psMetadataAddF32 (recipe, PS_LIST_TAIL, "RA_MAX",  PS_META_REPLACE, "", RAmax);
    psMetadataAddF32 (recipe, PS_LIST_TAIL, "DEC_MIN", PS_META_REPLACE, "", DECmin);
    psMetadataAddF32 (recipe, PS_LIST_TAIL, "DEC_MAX", PS_META_REPLACE, "", DECmax);

    psMetadataAddVector (input->fpa->analysis, PS_LIST_TAIL, "CORNER.L", PS_META_REPLACE, "corner pixel", cornerL);
    psMetadataAddVector (input->fpa->analysis, PS_LIST_TAIL, "CORNER.M", PS_META_REPLACE, "corner pixel", cornerM);
    psMetadataAddVector (input->fpa->analysis, PS_LIST_TAIL, "CORNER.P", PS_META_REPLACE, "corner pixel", cornerP);
    psMetadataAddVector (input->fpa->analysis, PS_LIST_TAIL, "CORNER.Q", PS_META_REPLACE, "corner pixel", cornerQ);
    psMetadataAddVector (input->fpa->analysis, PS_LIST_TAIL, "CORNER.R", PS_META_REPLACE, "corner pixel", cornerR);
    psMetadataAddVector (input->fpa->analysis, PS_LIST_TAIL, "CORNER.D", PS_META_REPLACE, "corner pixel", cornerD);

    psFree (cornerL);
    psFree (cornerM);
    psFree (cornerP);
    psFree (cornerQ);
    psFree (cornerR);
    psFree (cornerD);

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

bool psastroAstromGuessSetChip (pmFPA *fpa, pmChip *chip, const pmFPAview *view, double pixelScale, bool bilevelAstrometry) {

    // read WCS data from the corresponding header
    pmHDU *hdu = pmFPAviewThisHDU (view, fpa);
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelChip (chip, hdu->header)) {
            psWarning("Could not get WCS information from header for chip %d, skipping", view->chip);
            return false;
        }
    } else {
        if (!pmAstromReadWCS (fpa, chip, hdu->header, pixelScale)) {
            psWarning("Could not get WCS information from header for chip %d, skipping", view->chip);
            return false;
        }
    }
    return true;
}

bool psastroAstromGuessSetFPA (pmFPA *fpa, bool *bilevelAstrometry) {

    pmFPAview *view = pmFPAviewAlloc (0);
    pmHDU *phu = pmFPAviewThisPHU (view, fpa);

    *bilevelAstrometry = false;

    // load mosaic-level astrometry?
    if (phu) {
        char *ctype = psMetadataLookupStr (NULL, phu->header, "CTYPE1");
        if (ctype) {
            *bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
        }
    }
    if (*bilevelAstrometry) {
        pmAstromReadBilevelMosaic (fpa, phu->header);
    }
    psFree (view);
    return true;
}

// we made a guess at the beginning; how does the guess compare with the result?
bool psastroAstromGuessCheck (pmConfig *config) {

    bool status;

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    pmFPA *fpa = input->fpa;

    psVector *cornerLo = psMetadataLookupPtr (&status, input->fpa->analysis, "CORNER.L");
    psVector *cornerMo = psMetadataLookupPtr (&status, input->fpa->analysis, "CORNER.M");
    psVector *cornerPo = psMetadataLookupPtr (&status, input->fpa->analysis, "CORNER.P");
    psVector *cornerQo = psMetadataLookupPtr (&status, input->fpa->analysis, "CORNER.Q");
    psVector *cornerRo = psMetadataLookupPtr (&status, input->fpa->analysis, "CORNER.R");
    psVector *cornerDo = psMetadataLookupPtr (&status, input->fpa->analysis, "CORNER.D");

    if (cornerLo->n < 3) return true;

    psVector *cornerLn = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerMn = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerPn = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerQn = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerRn = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerDn = psVectorAllocEmpty (100, PS_TYPE_F32);

    psVector *cornerMK = psVectorAllocEmpty (100, PS_TYPE_VECTOR_MASK);

    if (DEBUG) psastroDumpCorners ("corners.up.guess3.dat", "corners.dn.guess3.dat", fpa);

    pmChip *chip = NULL;
    pmFPAview *view = pmFPAviewAlloc (0);

    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        if (!chip->process || !chip->file_exists || !chip->data_exists) { continue; }

	// if a chip fails during the calibration, the associated readout->data_exists
	// gets set to false.  This may be the wrong solution, but it does not break this
	// analysis here.  Note this is not the chip->data_exists field tested above.
	// chip->data_exists is only false if the chip data is missing.

	// chip->fromFPA can be NULL if the inversion fails, but I'm not sure
	// chip->toFPA can be NULL unless it was not in the original model
	if (!chip->toFPA) {
	  char *name = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
	  fprintf (stderr, "no astrom model for %s, skipping\n", name);
	  continue;
	}
	if (!chip->fromFPA) {
	  char *name = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
	  fprintf (stderr, "inversion faiulre for %s (%d), will be skipped\n", name, view->chip);
	}

        // skip chips for which the astrometry failed (NASTRO == 0)
        if (!chip->cells->n) goto skip_chip;
        pmCell *cell = chip->cells->data[0];
        if (!cell) goto skip_chip;

        if (!cell->readouts->n) goto skip_chip;
        pmReadout *readout = cell->readouts->data[0];
        if (!readout) goto skip_chip;

        psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
        if (!updates) goto skip_chip;

	// in psastroOneChipFit & psastroMosaicOneCihp, astrometry failures are marked with NASTRO = 0
	// these should be ignored when checking the overall solution
        int nAstro = psMetadataLookupS32 (&status, updates, "NASTRO");
        if (!nAstro) goto skip_chip;

	// it is not clear when astError = 0.0
        float astError = psMetadataLookupF32 (&status, updates, "CERROR");
        if (fabs(astError) < 1e-6) goto skip_chip;

	// XXX EAM 2022.09.22 : a more robust analysis would put the corner points
	// on the chip metadata so we can be certain the old and new corner values
	// are correctly matched.

        psPlane ptCH, ptFP, ptTP;
        psSphere ptSky;

        ptCH.x = 0;
        ptCH.y = 0;
        psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
        psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
        psDeproject (&ptSky, &ptTP, fpa->toSky);
        psLogMsg ("psastro", 3, "0,0 pix for chip %3d = %f,%f\n", view->chip, DEG_RAD*ptSky.r, DEG_RAD*ptSky.d);

        // new corner locations based on the calibrated astrometry
        psVectorAppend (cornerLn, ptFP.x);
        psVectorAppend (cornerMn, ptFP.y);
        psVectorAppend (cornerPn, ptTP.x);
        psVectorAppend (cornerQn, ptTP.y);
        psVectorAppend (cornerRn, ptSky.r);
        psVectorAppend (cornerDn, ptSky.d);
        psVectorAppend (cornerMK, 0);
        continue;

    skip_chip:
        // new corner locations based on the calibrated astrometry
        psVectorAppend (cornerLn, 0.0);
        psVectorAppend (cornerMn, 0.0);
        psVectorAppend (cornerPn, 0.0);
        psVectorAppend (cornerQn, 0.0);
        psVectorAppend (cornerRn, 0.0);
        psVectorAppend (cornerDn, 0.0);
        psVectorAppend (cornerMK, 1);
    }

    // compare the old R,D values projected to the same tangent plane as the new R,D values:

    psVector *cornerPs = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *cornerQs = psVectorAllocEmpty (100, PS_TYPE_F32);

    for (int i = 0; i < cornerRo->n; i++) {

        psPlane ptTP;
        psSphere ptSky;

        ptSky.r = cornerRo->data.F32[i];
        ptSky.d = cornerDo->data.F32[i];

        psProject (&ptTP, &ptSky, fpa->toSky);
        psVectorAppend (cornerPs, ptTP.x);
        psVectorAppend (cornerQs, ptTP.y);
    }

    psPlaneTransform *map = psPlaneTransformAlloc (1, 1, PS_POLYNOMIAL_ORD);
    map->x->coeffMask[1][1] = PS_POLY_MASK_SET;
    map->y->coeffMask[1][1] = PS_POLY_MASK_SET;

    // fit the valid chips, mask the invalid chips
    psVectorFitPolynomial2D (map->x, cornerMK, 1, cornerPn, NULL, cornerPs, cornerQs);
    psVectorFitPolynomial2D (map->y, cornerMK, 1, cornerQn, NULL, cornerPs, cornerQs);

    // apply the linear fit...
    psVector *cornerPf = psPolynomial2DEvalVector (map->x, cornerPs, cornerQs);
    psVector *cornerQf = psPolynomial2DEvalVector (map->y, cornerPs, cornerQs);

    // ...and calculate the residual between Pn,Qn and Pf,Qf
    psVector *cornerPd = (psVector *) psBinaryOp (NULL, cornerPn, "-", cornerPf);
    psVector *cornerQd = (psVector *) psBinaryOp (NULL, cornerQn, "-", cornerQf);

    pmAstromVisualPlotAstromGuessCheck (cornerPo, cornerQo, cornerPn, cornerQn, cornerPd, cornerQd);

    psStats *statsP = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
    psStats *statsQ = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);

    if (!psVectorStats (statsP, cornerPd, NULL, cornerMK, 1)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }
    if (!psVectorStats (statsQ, cornerQd, NULL, cornerMK, 1)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }

    float angle = atan2 (map->y->coeff[1][0], map->x->coeff[1][0]);
    float scale = hypot (map->y->coeff[1][0], map->x->coeff[1][0]);

    psLogMsg ("psastro", 3, "boresite offset  : %f,%f\n", map->x->coeff[0][0], map->y->coeff[0][0]);
    psLogMsg ("psastro", 3, "boresite angle   : %f, scale: %f", angle*PS_DEG_RAD, scale);
    psLogMsg ("psastro", 3, "boresite scatter : %f,%f\n", statsP->sampleStdev, statsQ->sampleStdev);

    // write the elapsed time here; this will be updated in psastroMosaicAstrometry, if called
    psMetadata *header = psMetadataLookupMetadata (&status, input->fpa->analysis, "PSASTRO.HEADER");
    if (!header) {
        header = psMetadataAlloc();
        psMetadataAddMetadata (input->fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", header);
        psFree (header);  // drop this reference
    }

    psMetadataAddF32 (header, PS_LIST_TAIL, "AST_R0", PS_META_REPLACE, "boresite offset in RA (TP units)", map->x->coeff[0][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "AST_D0", PS_META_REPLACE, "boresite offset in DEC (TP units)", map->y->coeff[0][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "AST_T0", PS_META_REPLACE, "boresite angle (degrees)", angle*PS_DEG_RAD);
    psMetadataAddF32 (header, PS_LIST_TAIL, "AST_S0", PS_META_REPLACE, "boresite scale correction", scale);
    psMetadataAddF32 (header, PS_LIST_TAIL, "AST_RS", PS_META_REPLACE, "boresite scatter in RA (TP units)", statsP->sampleStdev);
    psMetadataAddF32 (header, PS_LIST_TAIL, "AST_DS", PS_META_REPLACE, "boresite scatter in DEC (TP units)", statsQ->sampleStdev);

    if (DEBUG) {
        FILE *f = fopen ("corners.dat", "w");
        for (int i = 0; i < cornerRo->n; i++) {
            fprintf (f, "%10.6f %10.6f  %9.2f %9.2f  %9.2f %9.2f  |  %10.6f %10.6f  %9.2f %9.2f  %9.2f %9.2f\n",
                     cornerRn->data.F32[i], cornerDn->data.F32[i], cornerPn->data.F32[i], cornerQn->data.F32[i], cornerLn->data.F32[i], cornerMn->data.F32[i],
                     cornerRo->data.F32[i], cornerDo->data.F32[i], cornerPo->data.F32[i], cornerQo->data.F32[i], cornerLo->data.F32[i], cornerMo->data.F32[i]);
        }
        fclose (f);
    }

    psFree (cornerPf);
    psFree (cornerQf);
    psFree (cornerPd);
    psFree (cornerQd);

    psFree (statsP);
    psFree (statsQ);

    psFree (cornerMK);

    psFree (cornerLn);
    psFree (cornerMn);
    psFree (cornerPn);
    psFree (cornerQn);
    psFree (cornerRn);
    psFree (cornerDn);
    psFree (cornerPs);
    psFree (cornerQs);
    psFree (map);
    psFree (view);


    return true;
}

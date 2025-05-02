/** @file psastroFixChips.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define NONLIN_TOL 0.001 ///< tolerance in pixels
# define DEBUG 0

// XXX I think the 'badAstrom' tests may need to be adjusted: see eg the nominal rotation of
// the chips
bool psastroFixChips (pmConfig *config, psMetadata *recipe) {

    bool status;
    FILE *f;
    char *chipName;

    bool fixChips = psMetadataLookupBool (&status, config->arguments, "PSASTRO.FIX.CHIPS");
    if (!status) {
        fixChips = psMetadataLookupBool (&status, recipe, "PSASTRO.FIX.CHIPS");
    }
    if (!fixChips) return true;

    // identify reference astrometry table
    // if not defined, correction was not requested; skip step
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.MODEL");
    if (!astrom) return true;

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    // acceptable limits
    double pixelTol = psMetadataLookupF32 (&status, recipe, "PSASTRO.PIXEL.TOLERANCE");
    if (!status) psAbort ("missing recipe value");

    double angleTol = psMetadataLookupF32 (&status, recipe, "PSASTRO.ANGLE.TOLERANCE");
    if (!status) psAbort ("missing recipe value");

    // make sure the astrometry model is loaded.  de-activate all files except PSASTRO.MODEL.
    // supply the input concepts so the model is defined using the RA, DEC, POSANGLE of the input
    // image.
    psFree (astrom->fpa->concepts);
    astrom->fpa->concepts = psMemIncrRefCounter (input->fpa->concepts);
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.MODEL");

    pmFPAview *view = pmFPAviewAlloc (0);

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
        psError (PS_ERR_IO, false, "Can't load the astrometry model file");
        return false;
    }

    // We now have a set of chip solutions and a set of predictions.  Measure the overall offset and
    // rotation of the two systems by comparing the chip corners, projected onto the focal plane
    // (all 4 to prevent solutions tied to a single corner)

    psVector *xObs = psVectorAllocEmpty (4*input->fpa->chips->n, PS_TYPE_F32);
    psVector *yObs = psVectorAllocEmpty (4*input->fpa->chips->n, PS_TYPE_F32);
    psVector *xRef = psVectorAllocEmpty (4*input->fpa->chips->n, PS_TYPE_F32);
    psVector *yRef = psVectorAllocEmpty (4*input->fpa->chips->n, PS_TYPE_F32);

    int nPts = 0;

    if (DEBUG) {
        f = fopen ("corners.raw.dat", "w");
        chipName = NULL;
    }

    pmChip *obsChip = NULL;
    while ((obsChip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        if (!obsChip->process || !obsChip->file_exists || !obsChip->data_exists) { continue; }

        // XXX we are currently inconsistent with marking the good vs the bad data
        // psastroChipAstrom sets data_exists to false if the fit is bad.  this is
        // probably wrong since it implies there is no data!

        // skip chips for which the astrometry failed (NASTRO == 0)
        if (!obsChip->cells->n) continue;
        pmCell *cell = obsChip->cells->data[0];
        if (!cell) continue;

        if (!cell->readouts->n) continue;
        pmReadout *readout = cell->readouts->data[0];
        if (!readout) continue;

        psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
        if (!updates) continue;

        int nAstro = psMetadataLookupS32 (&status, updates, "NASTRO");
        if (!nAstro) continue;

        // set the chip astrometry using the astrom file
        pmChip *refChip = pmFPAviewThisChip (view, astrom->fpa);

        psRegion *region = pmChipPixels (obsChip);
        psPlane ptCP, ptFP;

        ptCP.x = region->x0; ptCP.y = region->y0;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        xObs->data.F32[nPts] = ptFP.x;
        yObs->data.F32[nPts] = ptFP.y;
        psPlaneTransformApply (&ptFP, refChip->toFPA, &ptCP);
        xRef->data.F32[nPts] = ptFP.x;
        yRef->data.F32[nPts] = ptFP.y;

        if (DEBUG) {
            chipName = psMetadataLookupStr(NULL, obsChip->concepts, "CHIP.NAME");
            fprintf (f, "%s  %f %f  %f %f\n", chipName, xObs->data.F32[nPts], yObs->data.F32[nPts], xRef->data.F32[nPts], yRef->data.F32[nPts]);
        }
        nPts ++;

        ptCP.x = region->x0; ptCP.y = region->y1;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        xObs->data.F32[nPts] = ptFP.x;
        yObs->data.F32[nPts] = ptFP.y;
        psPlaneTransformApply (&ptFP, refChip->toFPA, &ptCP);
        xRef->data.F32[nPts] = ptFP.x;
        yRef->data.F32[nPts] = ptFP.y;

        if (DEBUG) {
            chipName = psMetadataLookupStr(NULL, obsChip->concepts, "CHIP.NAME");
            fprintf (f, "%s  %f %f  %f %f\n", chipName, xObs->data.F32[nPts], yObs->data.F32[nPts], xRef->data.F32[nPts], yRef->data.F32[nPts]);
        }
        nPts ++;

        ptCP.x = region->x1; ptCP.y = region->y1;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        xObs->data.F32[nPts] = ptFP.x;
        yObs->data.F32[nPts] = ptFP.y;
        psPlaneTransformApply (&ptFP, refChip->toFPA, &ptCP);
        xRef->data.F32[nPts] = ptFP.x;
        yRef->data.F32[nPts] = ptFP.y;

        if (DEBUG) {
            chipName = psMetadataLookupStr(NULL, obsChip->concepts, "CHIP.NAME");
            fprintf (f, "%s  %f %f  %f %f\n", chipName, xObs->data.F32[nPts], yObs->data.F32[nPts], xRef->data.F32[nPts], yRef->data.F32[nPts]);
        }
        nPts ++;

        ptCP.x = region->x1; ptCP.y = region->y0;
        psPlaneTransformApply (&ptFP, obsChip->toFPA, &ptCP);
        xObs->data.F32[nPts] = ptFP.x;
        yObs->data.F32[nPts] = ptFP.y;
        psPlaneTransformApply (&ptFP, refChip->toFPA, &ptCP);
        xRef->data.F32[nPts] = ptFP.x;
        yRef->data.F32[nPts] = ptFP.y;

        if (DEBUG) {
            chipName = psMetadataLookupStr(NULL, obsChip->concepts, "CHIP.NAME");
            fprintf (f, "%s  %f %f  %f %f\n", chipName, xObs->data.F32[nPts], yObs->data.F32[nPts], xRef->data.F32[nPts], yRef->data.F32[nPts]);
        }
        nPts ++;

        psFree (region);
    }
    xObs->n = yObs->n = xRef->n = yRef->n = nPts;
    if (DEBUG) fclose (f);

    psPlaneTransform *map = psPlaneTransformAlloc (1, 1, PS_POLYNOMIAL_ORD);

    psVector *mask = psVectorAlloc (nPts, PS_TYPE_VECTOR_MASK);
    psVectorInit (mask, 0);

    psStats *stats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
    stats->clipIter = 1;

    for (int i = 0; i < 3; i++) {
        psVectorClipFitPolynomial2D (map->x, stats, mask, 0xff, xObs, NULL, xRef, yRef);
        psTrace ("psModules.astrom", 3, "x resid: %f +/- %f (%ld of %ld)\n", stats->clippedMean, stats->clippedStdev, stats->clippedNvalues, xObs->n);

        psVectorClipFitPolynomial2D (map->y, stats, mask, 0xff, yObs, NULL, xRef, yRef);
        psTrace ("psModules.astrom", 3, "y resid: %f +/- %f (%ld of %ld)\n", stats->clippedMean, stats->clippedStdev, stats->clippedNvalues, yObs->n);
    }

    // loop over all chips, select the outliers, and replace the measured astrometry with the model
    // the measured transformation above must be applied to make the comparison, and also then applied to the
    // model transformation

    if (DEBUG) {
        f = fopen ("corners.fit.dat", "w");
        for (int i = 0; i < xObs->n; i++) {
            psPlane obsCoord, refCoord;
            refCoord.x = xRef->data.F32[i];
            refCoord.y = yRef->data.F32[i];
            psPlaneTransformApply (&obsCoord, map, &refCoord);
            fprintf (f, "%f %f  %f %f  %f %f\n", xObs->data.F32[i], yObs->data.F32[i], xRef->data.F32[i], yRef->data.F32[i], obsCoord.x, obsCoord.y);
        }
        fclose (f);
    }

    psFree (xRef);
    psFree (yRef);
    psFree (mask);
    psFree (stats);

    psFree (view);
    view = pmFPAviewAlloc (0);

    while ((obsChip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, obsChip->file_exists, obsChip->process);
        if (!obsChip->process || !obsChip->file_exists || !obsChip->data_exists) { continue; }

        // set the chip astrometry using the astrom file
        pmChip *refChip = pmFPAviewThisChip (view, astrom->fpa);

        // bad Astrometry test:  ref pixel or angle outside nominal

        psPlane refPixel = {0.0, 0.0, 0.0, 0.0};
        psPlane obsCoord, refCoord, tmpCoord;

        // find location of 0,0 pixel in focal plane coords for this chip
        psPlaneTransformApply (&obsCoord, obsChip->toFPA, &refPixel);

        // find location of 0,0 pixel in focal plane coords for ref chip
        // apply the global field rotation and offset before comparing
        psPlaneTransformApply (&tmpCoord, refChip->toFPA, &refPixel);
        psPlaneTransformApply (&refCoord, map, &tmpCoord);

        psPlane offPixel = {100.0, 0.0, 100.0, 0.0};
        psPlane obsOffPt, refOffPt;

        // find location of 0,0 pixel in focal plane coords for this chip
        psPlaneTransformApply (&obsOffPt, obsChip->toFPA, &offPixel);

        // find location of 0,0 pixel in focal plane coords for ref chip
        psPlaneTransformApply (&tmpCoord, refChip->toFPA, &offPixel);
        psPlaneTransformApply (&refOffPt, map, &tmpCoord);

        double obsAngle = PM_DEG_RAD*atan2 (obsOffPt.y - obsCoord.y, obsOffPt.x - obsCoord.x);
        double refAngle = PM_DEG_RAD*atan2 (refOffPt.y - refCoord.y, refOffPt.x - refCoord.x);

        bool badAstrom = false;
        badAstrom |= fabs(obsCoord.x - refCoord.x) > pixelTol;
        badAstrom |= fabs(obsCoord.y - refCoord.y) > pixelTol;
        badAstrom |= fabs(obsAngle   - refAngle)   > angleTol;

        fprintf (stderr, "chip %d, angle: %f, pixel: %f,%f  : ", view->chip, obsAngle - refAngle, obsCoord.x - refCoord.x, obsCoord.y - refCoord.y);
	if (badAstrom) {
	    fprintf (stderr, "BAD ASTROM\n");
	} else {
	    fprintf (stderr, "GOOD ASTROM\n");
	}

        // XXX for now, just use first readout
        pmCell *cell = obsChip->cells->data[0];
        pmReadout *readout = cell->readouts->data[0];

        // update the header (pull or create local view to entry on readout->analysis)
        psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
        if (!updates) {
            updates = psMetadataAlloc ();
            psMetadataAddMetadata (readout->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
            psFree (updates);
        }

        psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_DX", PS_META_REPLACE, "chip x offset wrt model", obsCoord.x - refCoord.x);
        psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_DY", PS_META_REPLACE, "chip y offset wrt model", obsCoord.y - refCoord.y);
        psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_DT", PS_META_REPLACE, "chip rot offset wrt model", obsAngle - refAngle);

	continue;

        // for successful chips, save the measured offsets in the header
        if (!badAstrom) continue;

        // XXX for now, let's just fail on the bad chips.  In the future, let's try to recover, but we still need to
        // catch the failures relative to the model
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTRO", PS_META_REPLACE, "number of astrometry stars", 0);
        continue;

        psLogMsg ("psastro", PS_LOG_INFO, "fixing chip %d, angle: %f, pixel: %f,%f\n",
                  view->chip, obsAngle - refAngle, obsCoord.x - refCoord.x, obsCoord.y - refCoord.y);

        psFree (obsChip->toFPA);
        psFree (obsChip->fromFPA);

        // apply the exiting fromTPA transformation to make the new toFPA consistent with the toTPA layter
        // XXX this only works if toTPA is at most a linear transformation
        psPlaneTransform *toFPA = psPlaneTransformAlloc(refChip->toFPA->x->nX, refChip->toFPA->x->nY, PS_POLYNOMIAL_ORD);
        for (int i = 0; i <= refChip->toFPA->x->nX; i++) {
            for (int j = 0; j <= refChip->toFPA->x->nY; j++) {
                double f1 = refChip->toFPA->x->coeffMask[i][j] ? 0.0 : map->x->coeff[1][0]*refChip->toFPA->x->coeff[i][j];
                double f2 = refChip->toFPA->y->coeffMask[i][j] ? 0.0 : map->x->coeff[0][1]*refChip->toFPA->y->coeff[i][j];
                toFPA->x->coeff[i][j] = f1 + f2;

                double g1 = refChip->toFPA->x->coeffMask[i][j] ? 0.0 : map->y->coeff[1][0]*refChip->toFPA->x->coeff[i][j];
                double g2 = refChip->toFPA->y->coeffMask[i][j] ? 0.0 : map->y->coeff[0][1]*refChip->toFPA->y->coeff[i][j];
                toFPA->y->coeff[i][j] = g1 + g2;
            }
        }
        toFPA->x->coeff[0][0] += map->x->coeff[0][0];
        toFPA->y->coeff[0][0] += map->y->coeff[0][0];

        psRegion *region = pmChipPixels (obsChip);
        obsChip->toFPA   = toFPA;
	// NOTE: when we call psPlaneTransformInvert here, we do not increase the order as in other places
	// WHY NOT?
        obsChip->fromFPA = psPlaneTransformInvert(NULL, obsChip->toFPA, *region, 50, 0);
        psFree (region);

        // use the new position to re-try the match fit
        // select the raw objects for this readout
        psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS");
        if (rawstars == NULL) { continue; }

        // select the raw objects for this readout
        psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS");
        if (refstars == NULL) { continue; }

        // the absolute minimum number of stars is 4 (for order = 1)
        if ((rawstars->n < 4) || (refstars->n < 4)) {
            readout->data_exists = false;
            psLogMsg ("psastro", 3, "insufficient rawstars (%ld) or refstars (%ld)",
                      rawstars->n, refstars->n);
            continue;
        }

        psastroUpdateChipToFPA (input->fpa, obsChip);

        // XXX update the header with info to reflect the failure
        if (!psastroOneChipFit (input->fpa, obsChip, readout, refstars, rawstars, recipe, updates)) {
            readout->data_exists = false;
            psLogMsg ("psastro", 3, "failed to find a solution\n");
            continue;
        }

        pmAstromWriteWCS (updates, input->fpa, obsChip, NONLIN_TOL);
    }

    pmAstromVisualPlotFixChips (input, xObs, yObs);
    psFree (xObs);
    psFree (yObs);
    psFree (map);
    psFree (view);
    return true;
}

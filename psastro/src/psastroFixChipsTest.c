/** @file psastroFixChipsTest.c
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
# define NONLIN_TOL 0.001 /* tolerance in pixels */

bool psPlaneTransformPrint (FILE *f, psPlaneTransform *in) {

  fprintf (f, "x (%d,%d):\n", in->x->nX, in->x->nY);
  for (int i = 0; i <= in->x->nX; i++) {
    fprintf (f, " %d : ", i);
    for (int j = 0; j <= in->x->nY; j++) {
      fprintf (f, "%f ", in->x->coeff[i][j]);
    }
    fprintf (f, "\n");
  }
  fprintf (f, "y (%d,%d):\n", in->y->nX, in->y->nY);
  for (int i = 0; i <= in->y->nX; i++) {
    fprintf (f, " %d : ", i);
    for (int j = 0; j <= in->y->nY; j++) {
      fprintf (f, "%f ", in->y->coeff[i][j]);
    }
    fprintf (f, "\n");
  }
  return TRUE;
}

// XXX the designed version is not working correctly. Here we load a model which just has
// the CRPIX1, CRPIX2 values from another image. Compare those positions to the current
// postions, make an average (DC) correction, then fix the outliers.
bool psastroFixChipsTest (pmConfig *config, psMetadata *recipe) {

    bool status;

    bool fixChips = psMetadataLookupBool (&status, config->arguments, "PSASTRO.FIX.CHIPS");
    if (!status) {
	fixChips = psMetadataLookupBool (&status, recipe, "PSASTRO.FIX.CHIPS");
    }
    if (!fixChips) return true;

    // identify reference astrometry table
    // if not defined, correction was not requested; skip step
    char *modelFile = psMetadataLookupStr (&status, recipe, "PSASTRO.ROUGH.MODEL");
    if (!modelFile) return true;

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

    // acceptable limits
    double pixelTol = psMetadataLookupF32 (&status, recipe, "PSASTRO.PIXEL.TOLERANCE");
    if (!status) psAbort ("missing recipe value");

    FILE *f = fopen (modelFile, "r");
    if (f == NULL) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find model data %s", modelFile);
	return false;
    }

    float Xo, Yo;

    psArray *refPos = psArrayAllocEmpty (100);
    while (fscanf (f, "%*s %*d %*d %*f %f %f", &Xo, &Yo) != EOF) {
	psPlane *pos = psPlaneAlloc();
	pos->x = Xo;
	pos->y = Yo;
	psArrayAdd (refPos, 100, pos);
	psFree (pos);
    }
    assert (refPos->n == 60);

    pmFPAview *view = pmFPAviewAlloc (0);

    // loop over all chips, get the current CRPIX1,CRPIX2 values
    pmChip *obsChip = NULL;
    psArray *obsPos = psArrayAllocEmpty (100);
    while ((obsChip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
	psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, obsChip->file_exists, obsChip->process);
	if (!obsChip->process || !obsChip->file_exists || !obsChip->data_exists) { continue; }

	// extract CRPIX1, CRPIX2:
	// XXX for now, pull from the header:
	// pmCell *cell = obsChip->cells->data[0];
	// pmReadout *readout = cell->readouts->data[0];
	// psMetadata *header = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
	// Xo = psMetadataLookupF32 (&status, header, "CRPIX1");
	// Yo = psMetadataLookupF32 (&status, header, "CRPIX2");

	pmAstromWCS *wcs = pmAstromWCSfromFPA(input->fpa, obsChip, NONLIN_TOL);
	Xo = wcs->crpix1;
	Yo = wcs->crpix2;
	psFree (wcs);

	psPlane *pos = psPlaneAlloc();
	pos->x = Xo;
	pos->y = Yo;
	psArrayAdd (obsPos, 100, pos);
	psFree (pos);
    }
    psFree (view);
    assert (obsPos->n == 60);

    // get the offsets
    psVector *dX = psVectorAlloc (obsPos->n, PS_TYPE_F32);
    psVector *dY = psVectorAlloc (obsPos->n, PS_TYPE_F32);
    for (int i = 0; i < obsPos->n; i++) {
	psPlane *obs = obsPos->data[i];
	psPlane *ref = refPos->data[i];
    
	if (i < 30) {
	    dX->data.F32[i] = obs->x - ref->x;
	    dY->data.F32[i] = obs->y - ref->y;
	} else {
	    dX->data.F32[i] = ref->x - obs->x;
	    dY->data.F32[i] = ref->y - obs->y;
	}
	fprintf (stderr, "chip %d : %f - %f : %f, %f - %f: %f\n", i, obs->x, ref->x, dX->data.F32[i], obs->y, ref->y, dY->data.F32[i]);
    }
    
    // get the average offset
    psStats *stats;
    stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    if (!psVectorStats (stats, dX, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }
    Xo = stats->robustMedian;
    fprintf (stderr, "offset x: %f +/- %f\n", stats->robustMedian, stats->robustStdev);
    psFree (stats);

    stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    if (!psVectorStats (stats, dY, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }
    Yo = stats->robustMedian;
    fprintf (stderr, "offset y: %f +/- %f\n", stats->robustMedian, stats->robustStdev);
    psFree (stats);

    // measure the rotation of the good chips
    psVector *dT = psVectorAllocEmpty (obsPos->n, PS_TYPE_F32);
    for (int i = 0; i < dX->n; i++) {
	bool badAstrom = false;
	badAstrom |= (fabs(dX->data.F32[i] - Xo) > pixelTol);
	badAstrom |= (fabs(dY->data.F32[i] - Yo) > pixelTol);
	if (badAstrom) continue;

	pmChip *chip = input->fpa->chips->data[i];
	float theta = atan2 (chip->toFPA->x->coeff[0][1], chip->toFPA->x->coeff[1][0]);
	if (i >= 30) {
	  theta -= M_PI;
	}
	dT->data.F32[dT->n] = theta;
	dT->n ++;
    }
    stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    if (!psVectorStats (stats, dT, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }
    float To = stats->robustMedian;
    fprintf (stderr, "offset t: %f +/- %f\n", stats->robustMedian, stats->robustStdev);
    psFree (stats);

    // find the discrepant chips and reset
    for (int i = 0; i < dX->n; i++) {
	bool badAstrom = false;
	badAstrom |= (fabs(dX->data.F32[i] - Xo) > pixelTol);
	badAstrom |= (fabs(dY->data.F32[i] - Yo) > pixelTol);
	if (!badAstrom) continue;

	fprintf (stderr, "fixing chip %d, offset: %f %f\n",
		 i, dX->data.F32[i] - Xo, dY->data.F32[i] - Yo);

	// XXX this stuff below should be applied to each readout...
	// XXX for now, just use first readout
	pmFPA  *fpa  = input->fpa;
	pmChip *chip = fpa->chips->data[i];
	pmCell *cell = chip->cells->data[0];
	pmReadout *readout = cell->readouts->data[0];

	pmAstromWCS *wcs = pmAstromWCSfromFPA(fpa, chip, NONLIN_TOL);

	psPlane *ref = refPos->data[i];
	wcs->crpix1 = ref->x + Xo;
	wcs->crpix2 = ref->y + Yo;

	pmAstromWCStoFPA (fpa, chip, wcs, pixelScale);
	psFree (wcs);

	// apply average rotation
	psPlaneTransform *toFPA = psPlaneTransformRotate (NULL, chip->toFPA, To);
	psFree (chip->toFPA);
	chip->toFPA = toFPA;

	psPlaneTransform *fromFPA = psPlaneTransformRotate (NULL, chip->fromFPA, -To);
	psFree (chip->fromFPA);
	chip->fromFPA = fromFPA;

	// XXX did we get the offset right?
	wcs = pmAstromWCSfromFPA(fpa, chip, NONLIN_TOL);
	psFree (wcs);

	// save WCS and analysis metadata in update header
	// (pull or create local view to entry on readout->analysis)
	psMetadata *updates = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
	if (!updates) {
	    updates = psMetadataAlloc ();
	    psMetadataAddMetadata (readout->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
	    psFree (updates);
	}

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

	psastroUpdateChipToFPA (fpa, chip);

	// XXX skip the re-fit step for a test
	continue;

	// XXX update the header with info to reflect the failure
	if (!psastroOneChipFit (fpa, chip, readout, refstars, rawstars, recipe, updates)) {
	    readout->data_exists = false;
	    psLogMsg ("psastro", 3, "failed to find a solution\n");
	    continue;
	}

	pmAstromWriteWCS (updates, fpa, chip, NONLIN_TOL);
    }

    psFree (dT);
    psFree (dX);
    psFree (dY);
    psFree (refPos);
    psFree (obsPos);
  
    return true;
}

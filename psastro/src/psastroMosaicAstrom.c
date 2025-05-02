/** @file psastroMosaicAstrom.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.30 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define NONLIN_TOL 0.001 /* tolerance in pixels */

bool psastroMosaicFit (pmFPA *fpa, psMetadata *stats, psMetadata *recipe, const char *rootname, int pass);
bool psastroProjectionRefit (pmFPA *fpa, psMetadata *recipe);

// XXX require this fpa to have multiple chip extensions and a PHU?
// EAM 2021.02.18 : addings 'stats' to set bad quality as appropriate 
bool psastroMosaicAstrom (pmConfig *config, psMetadata *stats) {

    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	// recipe or programming error
        psError(PSASTRO_ERR_CONFIG, false, "Can't find PSASTRO recipe!\n");
        return false;
    }

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
	// recipe or programming error
        psError(PSASTRO_ERR_CONFIG, false, "Can't find input data!\n");
        return false;
    }

    pmFPA *fpa = input->fpa;

    char *outroot = psMetadataLookupStr (&status, config->arguments, "OUTPUT");
    if (!status || !outroot) psAbort ("Can't find outroot on config->arguments");

    int nIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.MOSAIC.CHIP.NITER");
    if (!status) psAbort ("missing config value");

    // if projection is not TAN, fit the measured projection here and combine toTPA/fromTPA functions
    bool fitMosaicDistortion = true;
    if ((fpa->toSky->type != PS_PROJ_TAN) && (fpa->toSky->type != PS_PROJ_DIS)) {
      if (!psastroProjectionRefit (fpa, recipe)) psAbort ("failed to refit distortion");
      fitMosaicDistortion = false;
    }

    // this should be in a loop with nIter =
    for (int iter = 0; fitMosaicDistortion && (iter < nIter); iter++) {
	// NOTE: data quality (psastroMosaicDistortion) or recipe / config error
        if (!psastroMosaicFit (fpa, stats, recipe, outroot, iter)) return false;
    }

    // now fit the chips under the common distortion with higher-order terms
    // first, re-perform the match with a slightly tighter circle
    if (!psastroMosaicSetMatch (fpa, recipe, nIter)) {
      // recipe or alloc error
      psError(PSASTRO_ERR_UNKNOWN, false, "failed to match raw and ref stars for mosaic (pass %d)", nIter);
      return false;
    }
    if (!psastroMosaicChipAstrom (fpa, stats, recipe, nIter)) {
      // this cannot actually return false
      psError(PSASTRO_ERR_UNKNOWN, false, "failed to measure chip astrometry in mosaic mode (pass %d)", nIter);
      return false;
    }
    
    if (psTraceGetLevel("psastro.dump") > 0) {
      // the last filename (see filenames in psastroMosaicFit)
      char filename[256];
      snprintf (filename, 256, "%s.%d.dat", outroot, 2*nIter + 2);
      psastroDumpMatches (fpa, filename);
    }
    
    // save WCS and analysis metadata in update header.
    // (pull or create local view to entry on readout->analysis)
    psMetadata *updates = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
    if (!updates) {
        updates = psMetadataAlloc ();
        psMetadataAddMetadata (fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
        psFree (updates);
    }
    if (!pmAstromWriteBilevelMosaic (updates, fpa, NONLIN_TOL)) {
	// error here is a data quality problem (and too bad to write smf)
	psWarning ("Failed to save header terms");
	if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
	    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Mosaic astrometry failed", PSASTRO_ERR_DATA);
	}
	return false;
    }

    // write the elapsed time here; this will be updated in psastroMosaicAstrometry, if called
    psMetadataAddF32 (updates, PS_LIST_TAIL, "DT_ASTR", PS_META_REPLACE, "elapsed psastro time", psTimerMark ("psastroAnalysis"));

    // update the headers based on the results
    // XXX need to add global summary statistics
    // psastroMosaicHeaders (config);

    return true;
}

/* coordinate frame hierachy
 * pixels (on a given readout)
 * cell
 * chip
 * FP (focal plane)
 * TP (tangent plane)
 * sky (ra, dec)
 */

// 1: match 4,5
// 2: match 6,7
// 3: match 8,9
bool psastroMosaicFit (pmFPA *fpa, psMetadata *stats, psMetadata *recipe, const char *rootname, int pass) {

    char filename[256];

    // given the existing per-chip astrometry, determine matches between raw and ref stars
    // is this needed? yes, if we didn't do SingleChip astrometry first
    if (!psastroMosaicSetMatch (fpa, recipe, pass)) {
        psError(PSASTRO_ERR_UNKNOWN, false, "failed to match raw and ref stars for mosaic (pass %d)", pass);
	// this can only fail on alloc failure in pmAstromRadiusMatchUniq
        return false;
    }

    if ((pass == 0) && (psTraceGetLevel("psastro.dump.psastroMosaicAstrom") > 1)) {
        snprintf (filename, 256, "%s.0.dat", rootname);
        psastroDumpMatches (fpa, filename);
    }

    // fitted chips will follow the local plate-scale, hiding the distortion
    // modify the chip->toFPA scaling to match knowledge about pixel scale,
    // then recalculate raw and ref positions
    if (!psastroMosaicCommonScale (fpa, recipe)) {
	// this can only return false if the recipe value is not set
        psError(PSASTRO_ERR_UNKNOWN, false, "failed to set a common scale for the chips (pass %d)", pass);
        return false;
    }

    if ((pass == 0) && (psTraceGetLevel("psastro.dump.psastroMosaicAstrom") > 1)) {
        snprintf (filename, 256, "%s.1.dat", rootname);
        psastroDumpMatches (fpa, filename);
    }

    // fit the distortion by fitting its gradient
    // apply the new distortion terms up and down
    // refit the per-chip terms with linear fits only
    // NOTE: failure here is a data quality problem
    if (!psastroMosaicDistortion (fpa, recipe, pass)) {
	psWarning ("failed to measure mosaic gradients (pass %d)", pass);
	if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
	    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Mosaic astrometry failed", PSASTRO_ERR_DATA);
	}
        return false;
    }

    snprintf (filename, 256, "%s.%d.dat", rootname, 2*pass + 2);
    if (psTraceGetLevel("psastro.dump.psastroMosaicAstrom") > 1) { psastroDumpMatches (fpa, filename); }

    // measure the astrometry for the chips under the distortion term
    if (!psastroMosaicChipAstrom (fpa, stats, recipe, pass)) {
	// this cannot actually return false
        psError(PSASTRO_ERR_UNKNOWN, false, "failed to measure chip astrometry in mosaic mode (pass %d)", pass);
        return false;
    }

    if (psTraceGetLevel("psastro.dump.psastroMosaicAstrom") > 1) {
        snprintf (filename, 256, "%s.%d.dat", rootname, 2*pass + 3);
        psastroDumpMatches (fpa, filename);
    }

    return true;
}

// we have a fpa->toSky projection which is NOT of type TAN along with a toTPA which is
// the Identity transform.  we want to convert this to TAN projection plus a non-identity
// transformation to take the non-TAN components.

// we are going to generate a set of FP->(x,y) using the current projection + fromTPA
// transformations along with a set of TP->(x,y) values using the desired TAN projection,
// then we will fit TP vs FP

bool psastroProjectionRefit (pmFPA *fpa, psMetadata *recipe) {

  bool status;

    // allocate mosaic-level polynomial transformation and set masks needed by DVO
    int order = psMetadataLookupF32 (&status, recipe, "PSASTRO.MOSAIC.ORDER");
    if (!status) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to find mosaic distortion fit order\n");
        return false;
    }

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    pmFPAview *view = pmFPAviewAlloc (0);

    psVector *L = psVectorAllocEmpty (1000, PS_TYPE_F32);
    psVector *M = psVectorAllocEmpty (1000, PS_TYPE_F32);

    psVector *P = psVectorAllocEmpty (1000, PS_TYPE_F32);
    psVector *Q = psVectorAllocEmpty (1000, PS_TYPE_F32);

    psProjection *toSkyTan = psProjectionAlloc (fpa->toSky->R, fpa->toSky->D, fpa->toSky->Xs, fpa->toSky->Ys, PS_PROJ_TAN);

    float xMin = NAN;
    float xMax = NAN;
    float yMin = NAN;
    float yMax = NAN;

    bool firstObject = true;

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) continue;
	
	while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) continue;

	    // process each of the readouts
	    // XXX there can only be one readout per chip, right?
	    while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
		if (! readout->data_exists) continue;

		// select the raw objects for this readout
		psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
		if (refstars == NULL) continue;
		psTrace ("psastro", 4, "Trying %ld refstars\n", refstars->n);

		psArray *matches = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
		if (matches == NULL) continue;

		// we are looking over the matched refstars only to be sure we are
		// covering the valid space of the projection + transformation, and not
		// beyond

		for (int i = 0; i < matches->n; i++) {
		    pmAstromMatch *match = matches->data[i];
		    pmAstromObj *ref = refstars->data[match->ref];

		    psPlane fpOld, tpNew, tpOld;

		    psProject (&tpOld, ref->sky, fpa->toSky); // find the focal-plane coord of this RA,DEC coord using the ref chip projection
		    psPlaneTransformApply (&fpOld, fpa->fromTPA, &tpOld);

		    psProject (&tpNew, ref->sky, toSkyTan); // find the focal-plane coord of this RA,DEC coord using the ref chip projection

		    psVectorAppend (L, fpOld.x);
		    psVectorAppend (M, fpOld.y);

		    psVectorAppend (P, tpNew.x);
		    psVectorAppend (Q, tpNew.y);

		    if (firstObject) {
		      xMin = xMax = fpOld.x;
		      yMin = yMax = fpOld.y;
		      firstObject = false;
		    }

		    xMin = PS_MIN (xMin, fpOld.x);
		    xMax = PS_MAX (xMax, fpOld.x);
		    yMin = PS_MIN (yMin, fpOld.y);
		    yMax = PS_MAX (yMax, fpOld.y);
		}
	    }
	}
    }

    // the original transforms are (1, 1), but we will need higher order to fit the distortions.
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

    psVectorFitPolynomial2D (fpa->toTPA->x, NULL, 0, P, NULL, L, M);
    psVectorFitPolynomial2D (fpa->toTPA->y, NULL, 0, Q, NULL, L, M);
    
    psRegion fitRegion = psRegionSet (xMin, xMax, yMin, yMax);

    // psPlaneTransformInvert will generate a new fromTPA (order increased vs toTPA)
    psFree (fpa->fromTPA);
    fpa->fromTPA = psPlaneTransformInvert (NULL, fpa->toTPA, fitRegion, 100, 4);

    psFree (fpa->toSky);
    fpa->toSky = toSkyTan;

    psFree (L);
    psFree (M);
    psFree (P);
    psFree (Q);

    psFree (view);

    if (!psastroMosaicSetAstrom (fpa)) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to apply mosaic distortion terms\n");
        return false;
    }

    return true;
}


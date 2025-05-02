/** @file psastroOneChipFit.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define REQUIRED_RECIPE_VALUE(VALUE, NAME, TYPE)\
  VALUE = psMetadataLookup##TYPE (&status, recipe, NAME); \
  if (!status) { \
   psAbort ("Failed to find %s in recipe", NAME); }

bool psastroOneChipFit (pmFPA *fpa, pmChip *chip, pmReadout *readout, psArray *refstars, psArray *rawstars, psMetadata *recipe, psMetadata *updates) {

    bool status;

    // default value for match/fit : radius is in pixels
    REQUIRED_RECIPE_VALUE (double RADIUS, "PSASTRO.MATCH.RADIUS", F32);

    // run the match/fit sequence NITER times
    REQUIRED_RECIPE_VALUE (int nIter, "PSASTRO.MATCH.FIT.NITER", S32);

    // for iterations >= uniqIter, require a single match per reference
    REQUIRED_RECIPE_VALUE (int uniqIter, "PSASTRO.MATCH.UNIQ.ITER", S32);

    // correct radius to FP units (physical pixel scale in microns per pixel)
    REQUIRED_RECIPE_VALUE (double pixelScale, "PSASTRO.PIXEL.SCALE", F32);
    RADIUS *= pixelScale;

    // select the desired chip order
    REQUIRED_RECIPE_VALUE (int defaultOrder, "PSASTRO.CHIP.ORDER", S32);

    // allowed limits for valid solutions
    REQUIRED_RECIPE_VALUE (float maxError, "PSASTRO.MAX.ERROR", F32);
    REQUIRED_RECIPE_VALUE (float maxStdev, "PSASTRO.MAX.STDEV", F32);
    REQUIRED_RECIPE_VALUE (int   minNstar, "PSASTRO.MIN.NSTAR", S32);

    psArray *match = NULL;
    psStats *fitStats = NULL;
    pmAstromFitResults *results = NULL;

    for (int iter = 0; iter < nIter; iter++) {

        char name[128];

        sprintf (name, "PSASTRO.MATCH.RADIUS.N%d", iter);
        float radius = psMetadataLookupF32 (&status, recipe, name);
        radius *= pixelScale;
        if (!status || (radius == 0.0)) {
            radius = RADIUS;
        }

        sprintf (name, "PSASTRO.ONE.CHIP.ORDER.N%d", iter);
        int order = psMetadataLookupS32 (&status, recipe, name);
        if (!status) {
            order = defaultOrder;
        }

        // use small radius to match stars
        match = pmAstromRadiusMatchFP (rawstars, refstars, radius);
        if (match == NULL) {
            psLogMsg ("psastro", 3, "failed to find radius-matched sources\n");
	    psastroChipFailureHeader (updates);
            return false;
        }

	if (iter >= uniqIter) {
	    psArray *unique = pmAstromRadiusMatchUniq (rawstars, refstars, match);
	    if (!unique) {
		psLogMsg ("psastro", 3, "failed to generate a uniq set of matched sources\n");
		psastroChipFailureHeader (updates);
		return false;
	    }
	    psFree (match);
	    match = unique;
	}

	// XXX check if we correctly applied the new transformation:
	if (psTraceGetLevel("psastro.dump") > 0) {
	  char *filename = NULL;
	  char *chipname = psMetadataLookupStr (&status, chip->concepts, "CHIP.NAME");
	  psStringAppend (&filename, "match.pref.%s.%d.dat", chipname, iter);
	  psastroDumpMatchedStars (filename, rawstars, refstars, match);
	  psFree (filename);
	  filename = NULL;
	}


        // modify the order to correspond to the actual number of matched stars:
        int Ndof_min = 3;
        int order_max = 0.5*(sqrt(4*match->n - 4*Ndof_min + 1) - 3);
        order = PS_MIN (order, order_max);

	// order 0 : Ro -> nterms = 1 * 2;
	// order 1 : Ro, Rx, Ry -> nterms = 3 * 2;
	// order 2 : Ro, Rx, Ry, Rxx, Rxy, Ryy -> nterms = 6 * 2;
	// order 3 : Ro, Rx, Ry, Rxx, Rxy, Ryy, Rxxx, Rxxy, Rxyy, Ryyy -> nterms = 10 * 2
	// 2*(N+1)*(N+2)/2 = (N+1)*(N+2) = nterms;
	// (order+1)(order+2) + ndof = nvalues
	// order^2 + 3*order + 2 + ndof = nvalue;
	// order^2 + 3*order + 2 + ndof - nvalue = 0;
	// 2*order = -3 +/- sqrt (9 - 4*(2 - nvalue + ndof));
	// 2*order = -3 +/- sqrt (9 - 8 + 4*nvalue - 4*ndof);
	// 2*order = (sqrt (1 + 4*nvalue - 4*ndof) - 3);

        // if ((match->n < 11) && (order >= 3)) order = 2;
        // if ((match->n <  7) && (order >= 2)) order = 1;
        // if ((match->n <  4) && (order >= 1)) order = 0;

        if (order < 1) {
            psLogMsg ("psastro", 3, "insufficient stars or invalid order: %ld stars", match->n);
            psFree (match);
	    psastroChipFailureHeader (updates);
            return false;
        }

        // Create output toFPA; set masks appropriate to the Elixir DVO astrometry format.
	// Forward transformations (chip->fpa->tpa->sky) use Ordinary polynomials
        psFree (chip->toFPA);
        chip->toFPA = psPlaneTransformAlloc (order, order, PS_POLYNOMIAL_ORD);
        for (int i = 0; i <= chip->toFPA->x->nX; i++) {
            for (int j = 0; j <= chip->toFPA->x->nY; j++) {
                if (i + j > order) {
                    chip->toFPA->x->coeffMask[i][j] = PS_POLY_MASK_SET;
                    chip->toFPA->y->coeffMask[i][j] = PS_POLY_MASK_SET;
                }
            }
        }

        // XXX allow statistic to be set by the user
        // fitStats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
        fitStats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
        fitStats->clipSigma = psMetadataLookupF32 (&status, recipe, "PSASTRO.CHIP.NSIGMA");
        fitStats->clipIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.CHIP.NITER");

        // improved fit for astrometric terms
        results = pmAstromMatchFit (chip->toFPA, rawstars, refstars, match, fitStats, recipe);
        if (!results) {
            psLogMsg ("psastro", 3, "failed to perform the matched fit\n");
            psFree (match);
            psFree (fitStats);
	    psastroChipFailureHeader (updates);
            return false;
        }

        // determine fromFPA transformation and apply new transformation to raw & ref stars
	// this applies the transformation to all stars (including subset)
        psastroUpdateChipToFPA (fpa, chip); // updates PSASTRO.RAWSTARS and PSASTRO.REFSTARS

	// XXX check if we correctly applied the new transformation:
	if (psTraceGetLevel("psastro.dump") > 0) {
	  char *filename = NULL;
	  char *chipname = psMetadataLookupStr (&status, chip->concepts, "CHIP.NAME");
	  psStringAppend (&filename, "match.post.%s.%d.dat", chipname, iter);
	  psastroDumpMatchedStars (filename, rawstars, refstars, match);
	  psFree (filename);
	  filename = NULL;
	}

        // toSky converts from FPA & TPA units (microns) to sky units (radians)
        float plateScale = 0.5*(fpa->toSky->Xs + fpa->toSky->Ys)*3600.0*PM_DEG_RAD;

	float rawXstdev = psStatsGetValue (results->xStats, psStatsStdevOption(results->xStats->options));
	float rawYstdev = psStatsGetValue (results->yStats, psStatsStdevOption(results->yStats->options));

        float astError = 0.5*(rawXstdev + rawYstdev) * plateScale;
        int astNstar = results->yStats->clippedNvalues;
        psLogMsg ("psastro", PS_LOG_INFO, "pass %d, error: %f arcsec, Nstars: %d", iter, astError, astNstar);

        if (iter < nIter - 1) {
            psFree (fitStats);
            psFree (results);
            psFree (match);
        }
    }

    // toSky converts from FPA & TPA units (microns) to sky units (radians)
    float plateScale = 0.5*(fpa->toSky->Xs + fpa->toSky->Ys)*3600.0*PM_DEG_RAD;

    float rawXstdev = psStatsGetValue (results->xStats, psStatsStdevOption(results->xStats->options));
    float rawYstdev = psStatsGetValue (results->yStats, psStatsStdevOption(results->yStats->options));

    // pixError is the average 1D scatter in pixels ('results' are in FPA units = microns)
    float pixError = 0.5*(rawXstdev + rawYstdev) / pixelScale;

    // astError is the average 1D scatter in arcsec ('results' are in FPA units = microns)
    float astError = 0.5*(rawXstdev + rawYstdev) * plateScale;

    // astStdev is the average 1D stdev of median residuals in arcsec ('results' are in FPA units = microns)
    // the median residuals are calculated in a grid of N x N bins 
    float astStdev = 0.5*(results->dXstdev + results->dYstdev) * plateScale;

    // x and y are forced to use the same subset of values:
    int astNstar = results->yStats->clippedNvalues;

    bool validSolution = true;

    // XXX should these result in errors or be handled another way?
    psLogMsg ("psastro", PS_LOG_INFO, "astrometry solution: error: %f arcsec, Nstars: %d, stdev: %f arcsec", astError, astNstar, astStdev);
    if (astError > maxError) {
        psLogMsg("psastro", PS_LOG_INFO, "residual error is too large, failed to find a solution: %f > %f", astError, maxError);
        validSolution = false;
    }
    if (astStdev > maxStdev) {
	psLogMsg("psastro", PS_LOG_INFO, "residual stdev is too large, failed to find a solution: %f > %f", astStdev, maxStdev);
        validSolution = false;
    }
    if (astNstar < minNstar) {
        psLogMsg("psastro", PS_LOG_INFO, "solution uses too few stars: %d < %d", astNstar, minNstar);
        validSolution = false;
    }
    if (!chip->fromFPA) {
        psLogMsg("psastro", PS_LOG_INFO, "toFPA/fromFPA inversion failure");
        validSolution = false;
    }

    // DVO expects NASTRO = 0 if we fail to find a solution
    psMetadataAddF32 (updates, PS_LIST_TAIL, "PERROR",   PS_META_REPLACE, "astrometry error (pixels)", pixError);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "CERROR",   PS_META_REPLACE, "astrometry error (arcsec)", astError);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "CERSTD",   PS_META_REPLACE, "astrometry stdev (arcsec)", astStdev);
    if (validSolution) {
        psMetadataAddF32 (updates, PS_LIST_TAIL, "CPRECISE", PS_META_REPLACE, "astrometry precision (arcsec)", astError/sqrt(astNstar));
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTRO",   PS_META_REPLACE, "number of astrometry stars", astNstar);
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTUSED", PS_META_REPLACE, "number of astrometry stars", astNstar);
    } else {
	psastroChipFailureHeader (updates);
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTUSED", PS_META_REPLACE, "number of astrometry stars", astNstar);
    }
    psMetadataAddF32 (updates, PS_LIST_TAIL, "EQUINOX",  PS_META_REPLACE, "equinox of ref catalog", 2000.0); // XXX this is bogus: should be defined based on equinox of refstars

    // additional error measurements:
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_CDX",   PS_META_REPLACE, "chip astrometry X stdev (arcsec)", rawXstdev * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_CSX",   PS_META_REPLACE, "chip astrometry X systematic err (arcsec)", results->dXsys * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_CRX",   PS_META_REPLACE, "chip astrometry X 10-90 percentile (arcsec)", results->dXrange * plateScale);

    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_CDY",   PS_META_REPLACE, "chip astrometry Y stdev (arcsec)", rawYstdev * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_CSY",   PS_META_REPLACE, "chip astrometry Y systematic err (arcsec)", results->dYsys * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_CRY",   PS_META_REPLACE, "chip astrometry Y 10-90 percentile (arcsec)", results->dYrange * plateScale);

    // XXX check if we correctly applied the new transformation:
    if (psTraceGetLevel("psastro.dump") > 0) {
        psastroDumpRawstars (rawstars, fpa, chip);
        psastroDumpMatchedStars ("match.dat", rawstars, refstars, match);
        psastroDumpStars (refstars, "refstars.cal.dat");
    }

    pmAstromVisualPlotOneChipFit (rawstars, refstars, match, recipe);

    if (psTraceGetLevel("psastro.plot") > 0) {
        psastroPlotOneChipFit (rawstars, refstars, match, recipe);
    }

    // save the match table for zero points and other tests
    psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.MATCH", PS_DATA_ARRAY | PS_META_REPLACE, "astrometry matches", match);

    psFree (match);
    psFree (results);
    psFree (fitStats);

    return validSolution;
}

bool psastroChipFailureHeader (psMetadata *updates) {
    psMetadataAddF32 (updates, PS_LIST_TAIL, "CPRECISE", PS_META_REPLACE, "astrometry precision (arcsec)", 0.0);
    psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTRO",   PS_META_REPLACE, "number of astrometry stars", 0);
    psMetadataAddU64 (updates, PS_LIST_TAIL, "ASTROM_CHIPS", PS_META_REPLACE, "chips that passed astrometry", 0);
    return true;
}



// psastroWriteStars ("raw.1.dat", rawstars);
// psastroWriteStars ("ref.1.dat", refstars);
// psastroWriteTransform (chip->toFPA);


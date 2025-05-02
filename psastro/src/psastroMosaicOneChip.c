/** @file psastroMosaicOneChip.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define REQUIRED_RECIPE_VALUE(VALUE, NAME, TYPE, MESSAGE)\
  VALUE = psMetadataLookup##TYPE (&status, recipe, NAME); \
  if (!status) { \
   psError(PSASTRO_ERR_CONFIG, false, MESSAGE); \
   return false; }

bool psastroMosaicOneChip (pmChip *chip, pmReadout *readout, psMetadata *recipe, psMetadata *updates, int iteration) {

    bool status;
    char errorWord[64];
    char stdevWord[64];
    char orderWord[64];

    PS_ASSERT_PTR_NON_NULL(chip,    false);
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(recipe,  false);
    PS_ASSERT_PTR_NON_NULL(updates, false);

    // select the raw objects for this readout
    psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
    if (rawstars == NULL) return false;

    psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
    if (refstars == NULL) return false;

    psArray *match = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
    if (match == NULL) return false;

    // correct radius to FP units (physical pixel scale in microns per pixel)
    REQUIRED_RECIPE_VALUE (double pixelScale, "PSASTRO.PIXEL.SCALE", F32, "Failed to lookup pixel scale");

    // allowed limits for valid solutions
    // CERROR is currently tested during the iterations, but not CERSTD
    snprintf (errorWord, 64, "PSASTRO.MOSAIC.MAX.ERROR.N%d", iteration);
    snprintf (stdevWord, 64, "PSASTRO.MOSAIC.MAX.STDEV.N%d", iteration);
    REQUIRED_RECIPE_VALUE (float maxError,                  errorWord, F32, "failed to find single-chip max allowed error\n");
    REQUIRED_RECIPE_VALUE (float maxStdev,                  stdevWord, F32, "failed to find single-chip max allowed stdev\n");
    REQUIRED_RECIPE_VALUE (int   minNstar, "PSASTRO.MOSAIC.MIN.NSTAR", S32, "failed to find single-chip min allowed stars\n");

    // set the order of the per-chip fit (higher order only if iteration > 0)
    REQUIRED_RECIPE_VALUE (int defaultOrder, "PSASTRO.MOSAIC.CHIP.ORDER", S32, "failed to find mosaic chip-level fit default order\n");

    snprintf (orderWord, 64, "PSASTRO.MOSAIC.CHIP.ORDER.N%d", iteration);
    int order = psMetadataLookupS32 (&status, recipe, orderWord);
    if (!status || (order == -1)) {
        order = defaultOrder;
    }

    // modify the order to correspond to the actual number of matched stars:
    int Ndof_min = 3;
    int order_max = 0.5*(sqrt(4*match->n - 4*Ndof_min + 1) - 3);
    order = PS_MIN (order, order_max);

    // if ((match->n < 17) && (order >= 3)) order = 2;
    // if ((match->n < 13) && (order >= 2)) order = 1;
    // if ((match->n <  9) && (order >= 1)) order = 0;

    if (order < 0) {
        psLogMsg ("psastro", 3, "insufficient stars (%ld) or invalid order (%d)", match->n, order);
        return false;
    }

    psLogMsg ("psastro", PS_LOG_DETAIL, "mosaic fit chip order %d", order);

    // create output toFPA; set masks appropriate to the Elixir DVO astrometry format if we are
    // fitting 0th order, use the current polynomial of whatever order, with higher order
    // coefficients frozen to the current values
    if (order == 0) {
        // set FIT mask for all higher order terms of the existing solution
        // any existing SET masks will be retained.
        for (int i = 0; i <= chip->toFPA->x->nX; i++) {
            for (int j = 0; j <= chip->toFPA->x->nY; j++) {
                if (i + j > 0) {
                    chip->toFPA->x->coeffMask[i][j] |= PS_POLY_MASK_FIT;
                    chip->toFPA->y->coeffMask[i][j] |= PS_POLY_MASK_FIT;
                }
            }
        }
    } else {
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
    }

    // XXX allow statistic to be set by the user
    // only clip if we are fitting the chip parameters.
    psStats *fitStats = NULL;
    if (FALSE && (order == 0)) {
      fitStats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
      fitStats->clipSigma = psMetadataLookupF32 (&status, recipe, "PSASTRO.MOSAIC.CHIP.NSIGMA");
      fitStats->clipIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.MOSAIC.CHIP.NITER");
    } else {
      fitStats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
      fitStats->clipSigma = psMetadataLookupF32 (&status, recipe, "PSASTRO.MOSAIC.CHIP.NSIGMA");
      fitStats->clipIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.MOSAIC.CHIP.NITER");
    }

    // need to pass in an update header, sent in from above
    pmAstromFitResults *results = pmAstromMatchFit (chip->toFPA, rawstars, refstars, match, fitStats, recipe);
    if (!results) {
        psError(PSASTRO_ERR_DATA, false, "failed to perform the matched fit\n");
        return false;
    }

    // toSky converts from FPA & TPA units (microns) to sky units (radians)
    pmFPA *fpa = chip->parent;
    float plateScale = 0.5*(fpa->toSky->Xs + fpa->toSky->Ys)*3600.0*PM_DEG_RAD;

    float rawXstdev = psStatsGetValue (results->xStats, psStatsStdevOption(results->xStats->options));
    float rawYstdev = psStatsGetValue (results->yStats, psStatsStdevOption(results->yStats->options));

    // pixError is the average 1D scatter in pixels ('results' are in FPA units = microns)
    float pixError = 0.5*(rawXstdev + rawYstdev) / pixelScale;

    // astError is the average 1D scatter in arcsec ('results' are in FPA units = microns)
    float astError = 0.5*(rawXstdev + rawYstdev) * plateScale;
    int astNstar = results->yStats->clippedNvalues;

    // astStdev is the average 1D stdev of median residuals in arcsec ('results' are in FPA units = microns)
    // the median residuals are calculated in a grid of N x N bins 
    float astStdev = 0.5*(results->dXstdev + results->dYstdev) * plateScale;

    // if we clip away too many stars, the order may be invalid
    if (order == 3) { minNstar = PS_MAX (15, minNstar); }
    if (order == 2) { minNstar = PS_MAX (11, minNstar); }
    if (order == 1) { minNstar = PS_MAX ( 8, minNstar); }

    // determine fromFPA transformation and apply new transformation to raw & ref stars
    psastroUpdateChipToFPA (fpa, chip);

    bool validSolution = true;

    // We have options to exclude chips on the basis of NASTRO, CERROR, CERSTD
    psLogMsg ("psastro", PS_LOG_INFO, "astrometry solution: error: %f arcsec, Nstars: %d, stdev: %f arcsec", astError, astNstar, astStdev);
    if ((maxError > 0) && (astError > maxError)) {
        psLogMsg("psastro", PS_LOG_INFO, "residual error is too large, failed to find a solution: %f > %f", astError, maxError);
        validSolution = false;
    }
    if ((maxStdev > 0) && (astStdev > maxStdev)) {
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

    // DVO expects NASTRO = 0 if we fail to find a solution, NASTUSED is the true number
    psMetadataAddF32 (updates, PS_LIST_TAIL, "PERROR",   PS_META_REPLACE, "astrometry error (pixels)", pixError);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "CERROR",   PS_META_REPLACE, "astrometry error (arcsec)", astError);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "CERSTD",   PS_META_REPLACE, "astrometry stdev (arcsec)", astStdev);
    if (validSolution) {
        psMetadataAddF32 (updates, PS_LIST_TAIL, "CPRECISE", PS_META_REPLACE, "astrometry precision (arcsec)", astError/sqrt(astNstar));
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTRO",   PS_META_REPLACE, "number of astrometry stars", astNstar);
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTUSED", PS_META_REPLACE, "number of astrometry stars", astNstar);
    } else {
        psMetadataAddF32 (updates, PS_LIST_TAIL, "CPRECISE", PS_META_REPLACE, "astrometry precision (arcsec)", 0.0);
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTRO",   PS_META_REPLACE, "number of astrometry stars", 0);
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NASTUSED", PS_META_REPLACE, "number of astrometry stars", astNstar);
    }
    psMetadataAddF32 (updates, PS_LIST_TAIL, "EQUINOX",  PS_META_REPLACE, "", 2000.0); // XXX this is bogus: should be defined based on equinox of refstars

    // additional error measurements:
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_MDX",   PS_META_REPLACE, "mosaic astrometry X stdev (arcsec)", rawXstdev * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_MSX",   PS_META_REPLACE, "mosaic astrometry X systematic err (arcsec)", results->dXsys * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_MRX",   PS_META_REPLACE, "mosaic astrometry X 10-90 percentile (arcsec)", results->dXrange * plateScale);

    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_MDY",   PS_META_REPLACE, "mosaic astrometry Y stdev (arcsec)", rawYstdev * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_MSY",   PS_META_REPLACE, "mosaic astrometry Y systematic err (arcsec)", results->dYsys * plateScale);
    psMetadataAddF32 (updates, PS_LIST_TAIL, "AST_MRY",   PS_META_REPLACE, "mosaic astrometry Y 10-90 percentile (arcsec)", results->dYrange * plateScale);

    // plot results
    pmAstromVisualPlotMosaicOneChip(rawstars, refstars, match, recipe);

    psFree (fitStats);
    psFree (results);
    return validSolution;
}

/** @file psastroOneChipGrid.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define REQUIRED_RECIPE_VALUE(VALUE, NAME, TYPE)\
  VALUE = psMetadataLookup##TYPE (&status, recipe, NAME); \
  if (!status) { \
   psAbort ("Failed to find %s in recipe", NAME); }

bool psastroOneChipGrid (pmFPA *fpa, pmChip *chip, psArray *refstars, psArray *rawstars, psMetadata *recipe, psMetadata *updates) {

    bool status;
    pmAstromStats *stats = NULL;
    char *chipname = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");


    // do we need to get a rough initial match?
    REQUIRED_RECIPE_VALUE (bool gridSearch, "PSASTRO.GRID.SEARCH", Bool);
    if (!gridSearch) return true;

    // do we need to get a rough initial match?
    int nMaxRaw = psMetadataLookupS32(&status, recipe, "PSASTRO.GRID.NRAW.MAX");
    if (!status) {
	nMaxRaw = psMetadataLookupS32(&status, recipe, "PSASTRO.GRID.NSTAR.MAX");
    }
    int nMaxRef = psMetadataLookupS32(&status, recipe, "PSASTRO.GRID.NREF.MAX");
    if (!status) {
	nMaxRef = refstars->n;
    }

    // generate the bright subset of nMaxRaw entries of rawstars (already sorted by S/N)
    psArray *rawGridStars = psArrayAlloc (PS_MIN (nMaxRaw, rawstars->n));
    for (int i = 0; (i < nMaxRaw) && (i < rawstars->n); i++) {
	rawGridStars->data[i] = psMemIncrRefCounter (rawstars->data[i]);
    }

    // generate the bright subset of nMaxRef entries of refstars (already sorted by S/N)
    psArray *refGridStars = psArrayAlloc (PS_MIN (nMaxRef, refstars->n));
    for (int i = 0; (i < nMaxRef) && (i < refstars->n); i++) {
	refGridStars->data[i] = psMemIncrRefCounter (refstars->data[i]);
    }

    psLogMsg ("psastro", 3, "grid search for chip %s using %ld raw vs %ld ref stars\n",chipname,rawGridStars->n, refGridStars->n);

    // find initial offset / rotation / scale
    pmAstromStats *gridStats = pmAstromGridMatch (rawGridStars, refGridStars, recipe);
    if (gridStats == NULL) {
	psLogMsg ("psastro", 3, "failed to find a grid match solution for chip %s\n",chipname);
	psFree (rawGridStars);
	psFree (refGridStars);
	psastroChipFailureHeader (updates);
	return false;
    }
    psLogMsg ("psastro", 3, "basic grid search result for chip %s - offset: %f,%f pixels, rotation: %f deg\n",chipname, gridStats->offset.x, gridStats->offset.y, DEG_RAD*gridStats->angle);

# if (1) 
    // tweak the position by finding peak of matches stars
    stats = pmAstromGridTweak (rawGridStars, refGridStars, recipe, gridStats);
    if (stats == NULL) {
	psLogMsg ("psastro", 3, "failed to measure tweaked grid solution\n");
	psFree (gridStats);
	psFree (rawGridStars);
	psFree (refGridStars);
	psastroChipFailureHeader (updates);
	return false;
    }
    psLogMsg ("psastro", 3, "tweak grid search result for chip %s - offset: %f,%f pixels, rotation: %f deg\n",chipname, stats->offset.x, stats->offset.y, DEG_RAD*stats->angle);
# else
    // EAM TEST: skip tweak
    stats = pmAstromStatsAlloc();
   *stats = *gridStats;
# endif

    // adjust the chip.toFPA terms only
    pmAstromGridApply (chip->toFPA, stats);
    psastroUpdateChipToFPA (fpa, chip); // updates PSASTRO.RAWSTARS and PSASTRO.REFSTARS (see note below)
    psFree (gridStats);
    psFree (rawGridStars);
    psFree (refGridStars);
    psFree (stats);

    return true;
}

/* if psastroUpdateChiptoFPA fails to invert the toFPA transformation, 
   the ref->chip coordinates will not be set.  This is not a problem
   at this stage since they are not used in the calculation.  Later
   passes can still yield a valid solution.
*/

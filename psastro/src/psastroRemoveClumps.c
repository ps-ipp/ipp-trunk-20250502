/** @file psastroRemoveClumps.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroRemoveClumpsRawstars (pmConfig *config) {

    bool status;

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    int nIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.REFSTAR.CLUMP.NITER");
    if (!status) nIter = 3;

    psF32 clumpScale = psMetadataLookupS32 (&status, recipe, "PSASTRO.REFSTAR.CLUMP.SCALE");
    if (!status) clumpScale = 150;

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPA *fpa = input->fpa;

    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            if (!chip->fromFPA) { continue; }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // select the raw objects for this readout
                psArray *rawstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.RAWSTARS");
                if (rawstars == NULL) { continue; }

		// generate a reduced subset excluding the clumps
		// XXX do we need both RAWSTARS and SUBSET? 
		// XXX put these parameters in the recipe, please
		psArray *subset = psastroRemoveClumpsIterate(rawstars, clumpScale, nIter);
		psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.RAWSTARS.SUBSET", PS_DATA_ARRAY, "astrometry objects", subset);
		psFree (subset);

                psArray *gridstars = psMetadataLookupPtr(&status, readout->analysis, "PSASTRO.GRID.RAWSTARS");
                if ((gridstars == rawstars) || (gridstars == NULL)) { continue; }

		psArray *gridstars_subset = psastroRemoveClumpsIterate(gridstars, clumpScale, nIter);
		psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.GRID.RAWSTARS.SUBSET", PS_DATA_ARRAY, "astrometry objects", gridstars_subset);
		psFree (gridstars_subset);
	    }
	}
    }
    psFree (view);
    return true;
}

psArray *psastroRemoveClumpsIterate (psArray *input, int scale, int nIter) {

    psArray *newset = psMemIncrRefCounter (input);
    for (int i = 0; i < nIter; i++) {
	psArray *subset = psastroRemoveClumps (newset, scale);
	psFree (newset);
	newset = subset;
    }
    return newset;
}    

/**
 * look for and exclude objects in clumps in the input list
 */
psArray *psastroRemoveClumps (psArray *input, int scale) {

    if (!input) return NULL;

    if (!input->n) {
	psArray *output = psArrayAllocEmpty (16);
	return output;
    }

    // determine data range
    pmAstromObj *obj = input->data[0];
    float Xmin = obj->FP->x;
    float Xmax = obj->FP->x;
    float Ymin = obj->FP->y;
    float Ymax = obj->FP->y;
    for (int i = 0; i < input->n; i++) {
        obj = (pmAstromObj *)input->data[i];
        if (!isfinite(obj->FP->x)) continue;
        if (!isfinite(obj->FP->y)) continue;
        Xmin = PS_MIN (Xmin, obj->FP->x);
        Xmax = PS_MAX (Xmax, obj->FP->x);
        Ymin = PS_MIN (Ymin, obj->FP->y);
        Ymax = PS_MAX (Ymax, obj->FP->y);
    }

    int nX = (Xmax - Xmin) / scale + 10;
    int nY = (Ymax - Ymin) / scale + 10;
    psImage *count = psImageAlloc (nX, nY, PS_TYPE_U32);
    psImageInit (count, 0);

    // accumulate 2D histogram in image
    for (int i = 0; i < input->n; i++) {
        obj = (pmAstromObj *)input->data[i];
        if (!isfinite(obj->FP->x)) continue;
        if (!isfinite(obj->FP->y)) continue;
        int Xi = PS_MIN (PS_MAX((obj->FP->x - Xmin) / scale + 5, 0), count->numCols);
        int Yi = PS_MIN (PS_MAX((obj->FP->y - Ymin) / scale + 5, 0), count->numRows);
        count->data.U32[Yi][Xi] ++;
    }

    // determine image statistics
    psStats *stats = psStatsAlloc (PS_STAT_MAX | PS_STAT_MAX | PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    if (!psImageStats(stats, count, NULL, 0)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to get image statistics.\n");
        psFree(stats);
        psFree(count);
        return NULL;
    }

    if (stats->max < 1) {
        psError(PS_ERR_UNKNOWN, false, "no valid sources in image\n");
        psFree(stats);
        psFree(count);
        return NULL;
    }

    // XXX make this a user option
    float limit = PS_MAX (5.0*stats->sampleStdev, 5.0);
    psTrace ("psastro", 4, "skipping stars in cells with more than %f stars\n", limit);

    pmAstromVisualPlotRemoveClumps (input, count, scale, limit);

    // find and exclude objects in bad pixels
    psArray *output = psArrayAllocEmpty (input->n);
    for (int i = 0; i < input->n; i++) {
        obj = (pmAstromObj *)input->data[i];
        if (!isfinite(obj->FP->x)) continue;
        if (!isfinite(obj->FP->y)) continue;
        int Xi = PS_MIN (PS_MAX((obj->FP->x - Xmin) / scale + 5, 0), count->numCols);
        int Yi = PS_MIN (PS_MAX((obj->FP->y - Ymin) / scale + 5, 0), count->numRows);
        if (count->data.U32[Yi][Xi] > limit) continue;
        psArrayAdd (output, 16, obj);
    }

    psFree(stats);
    psFree(count);
    return output;
}

# if (0)
/**
 * make a list of the outlier pixels
 */
psArray *badpix = psArrayAllocEmpty (16);
for (int iy = 0; iy < count->numRows; iy++) {
    for (int ix = 0; ix < count->numCols; ix++) {
        if (count->data.U32[iy][ix] > limit) {
            psPlane *pixel = psPlaneAlloc();
            pixel->x = ix;
            pixel->y = iy;
            psArrayAdd (badpix, 16, pixel);
            psFree (pixel);
        }
    }
}

if (badpix->n == 0) {
    psArray *output = psMemIncrRefCounter (input);
    psFree (stats);
    psFree (count);
    psFree (badpix);
    return output;
}
# endif


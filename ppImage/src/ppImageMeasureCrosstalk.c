#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

#define ESCAPE(MESSAGE) {				\
	psError(PS_ERR_UNKNOWN, false, MESSAGE);	\
	psFree(view);					\
	return false;					\
    }

// For the moment, this implementation is VERY GPC-specific

bool ppImageMeasureCrosstalk(pmConfig *config, ppImageOptions *options, pmFPAview *view)
{
    bool status;
    const float CUTOFF = 60000;
    const int MIN_MATCHED_PIXELS = 500;  // require a significant area of high signal
    char name[64];

    if (!options->doCrosstalkMeasure) return true;

    psTimerStart("crosstalk");

    // confirm that this camera is GPC1

    // find the currently selected chip
    pmChip *chip = pmFPAfileThisChip(config->files, view, "PPIMAGE.INPUT");
    pmFPA *fpa = chip->parent;
    if (chip->cells->n != 64) return true;

    // for each cell (xyNM), we want to find the pixels with values above a cutoff (40k, 60k, ?)
    // for all of the other cells in the row (xyJM), we want to measure the (robust) median flux in the pixels
    // corresponding to the high pixels in xyNM, and compare with the median flux for the cell

    psArray *vectorSet = psArrayAlloc(8);
    psArray *cellRows = psArrayAlloc(8);
    psArray *imageRows = psArrayAlloc(8);
    for (int i = 0; i < 8; i++) {
	psArray *cellRow = psArrayAlloc(8);
	cellRows->data[i] = cellRow;
	psArray *imageRow = psArrayAlloc(8);
	imageRows->data[i] = imageRow;
	psVector *vector = psVectorAllocEmpty(100, PS_TYPE_F32);
	vectorSet->data[i] = vector;
    }

    // assign the cells to arrays by row and column
    for (int i = 0; i < chip->cells->n; i++) {

	pmCell *cell = chip->cells->data[i];

	// skip the video cells and empty cells (cell->readouts->n != 1)
	if (cell->readouts->n != 1) {
	    psWarning ("Skipping Empty and Video Cell for ppImageCrosstalk");
	    continue;
	}

	// place the image from this cell in the 2D array:
	const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME");	    
	int row = cellName[3] - '0';
	int col = cellName[2] - '0';
	assert (row >= 0 && row < 8);
	assert (col >= 0 && col < 8);

	pmReadout *readout = cell->readouts->data[0];
	psImage *image = readout->image;

	psArray *cellRow = cellRows->data[row];
	cellRow->data[col] = psMemIncrRefCounter(cell);

	psArray *imageRow = imageRows->data[row];
	imageRow->data[col] = psMemIncrRefCounter(image);
    }

    psVector *sample = NULL;
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN);

    // measure the background for each cell
    for (int row = 0; row < cellRows->n; row++) {
	psArray *cellRow = cellRows->data[row];
	psArray *imageRow = imageRows->data[row];
	for (int col = 0; col < cellRow->n; col++) {
	    pmCell *cell = cellRow->data[col];
	    if (!cell) continue;
	    psImage *image = imageRow->data[col];
	    if (!image) continue; // XXX assert on this?

	    psStatsInit (stats);
	    if (!psImageBackground (stats, &sample, image, NULL, 0, rng)) continue;
	    psMetadataAddF32 (cell->analysis, PS_LIST_TAIL, "XTALK.REF", PS_META_REPLACE, "crosstalk measurement", stats->robustMedian);
	    // fprintf (stderr, "xtalk.ref (xy%d%d) : %f\n", col, row, stats->robustMedian);
	}
    }

    for (int row = 0; row < cellRows->n; row++) {
	psArray *cellRow = cellRows->data[row];
	psArray *imageRow = imageRows->data[row];
	for (int col = 0; col < cellRow->n; col++) {
	    psImage *image = imageRow->data[col];
	    if (!image) continue;

	    // initialize the storage vectors
	    for (int i = 0; i < 8; i++) {
		psVector *vector = vectorSet->data[i];
		vector->n = 0;
	    }

	    int nBright = 0;

	    // generate a vector for each of the other cells in this row
	    // containing only the pixels for which this cell is > CUTOFF
	    for (int iy = 0; iy < image->numRows; iy++) {
		for (int ix = 0; ix < image->numCols; ix++) {

		    if (image->data.F32[iy][ix] < CUTOFF) continue;
		    nBright ++;

		    // this is a pixel of interest in the target cell; extract the matched pixels
			
		    for (int i = 0; i < 8; i++) {
			if (i == col) continue;

			psImage *matchImage = imageRow->data[i];
			if (!matchImage) continue;

			psVector *vector = vectorSet->data[i];
			psVectorAppend (vector, matchImage->data.F32[iy][ix]);
		    }
		}
	    }
	    // fprintf (stderr, "nBright for xy%d%d : %d\n", col, row, nBright);
	    if (nBright < MIN_MATCHED_PIXELS) continue;

	    // pmCell *refCell = cellRow->data[col];
	    // float refBackground = psMetadataLookupF32 (&status, refCell->analysis, "XTALK.REF");
	    // float swing = CUTOFF - refBackground;

	    // now we need to measure the median for these vectors
	    for (int i = 0; i < 8; i++) {
		if (i == col) continue;
		psVector *vector = vectorSet->data[i];
		if (vector->n < MIN_MATCHED_PIXELS) continue;

		pmCell *cell = cellRow->data[i];
		if (!cell) continue; // XXX assert on this?

		psStatsInit (stats);
		if (!psVectorStats (stats, vector, NULL, NULL, 0)) {
		    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		    return false;
		}
		    
		float background = psMetadataLookupF32 (&status, cell->analysis, "XTALK.REF");
		float xtalk = (stats->robustMedian - background) / CUTOFF;

		// Put version information into the header
		pmHDU *hdu = pmHDUGetHighest(fpa, chip, cell);
		if (hdu) {
		    // add this to the PHU header 
		    snprintf (name, 64, "XT_%d%d_%d%d", col, row, i, row);
		    psMetadataAddF32 (hdu->header, PS_LIST_TAIL, name, PS_META_REPLACE, "crosstalk measurement", xtalk);
		}

		// keyword for resulting value:
		snprintf (name, 64, "XTALK_%d%d", i, row);

		psMetadataAddF32 (cell->analysis, PS_LIST_TAIL, name, PS_META_REPLACE, "crosstalk measurement", xtalk);
		// fprintf (stderr, "xtalk (xy%d%d on xy%d%d) : %f -> delta is %f, slope if %e\n", col, row, i, row, stats->robustMedian, stats->robustMedian - background, xtalk);
	    }
	}
    }
    psLogMsg ("ppImage", 3, "crosstalk measurement: %f sec\n", psTimerMark ("crosstalk"));

    psFree (vectorSet);
    psFree (cellRows);
    psFree (imageRows);
    psFree (stats);
    psFree (rng);
    psFree (sample);

    // need to free all sorts of things here....
    return true;
}


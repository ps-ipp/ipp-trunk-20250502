# include "psphotInternal.h"

// examine the x,y distribution of the psfstars and extend the selection if needed
// desired region and coverage are specified by pmPSFOptions
// even if stars->n is at the limit, we will add more sources to fill out the area

bool psphotCheckStarDistribution (psArray *stars, psArray *sources, pmPSFOptions *options) {

    // count the number of PSFSTAR sources in each cell.  for polynomial representations, we
    // use an image (Norder + 1) x (Norder + 1)
    int Nx = options->psfTrendNx;
    int Ny = options->psfTrendNy;
    if (options->psfTrendMode != PM_TREND_MAP) {
	Nx ++;
	Ny ++;
    }

    // set up and image and an image binning structure to cover the field
    psImage *nCell = psImageAlloc (Nx, Ny, PS_TYPE_S32);
    psImageInit (nCell, 0);

    psImageBinning *binning = psImageBinningAlloc();
    binning->nXruff = Nx;
    binning->nYruff = Ny;
    binning->nXfine = options->psfFieldNx;
    binning->nYfine = options->psfFieldNy;

    psImageBinningSetScale (binning, PS_IMAGE_BINNING_CENTER);
    psImageBinningSetSkipByOffset (binning, options->psfFieldXo, options->psfFieldYo);

    // where are the PSF stars located?
    for (int i = 0; i < stars->n; i++) {
	
	pmSource *source = stars->data[i];
        if (source->peak == NULL) continue;
        if (!(source->tmpFlags & PM_SOURCE_TMPF_CANDIDATE_PSFSTAR)) continue;

	int binX = psImageBinningGetRuffX (binning, source->peak->xf);
	int binY = psImageBinningGetRuffY (binning, source->peak->yf);

	assert (binX >= 0);
	assert (binY >= 0);
	assert (binX <  nCell->numCols);
	assert (binY <  nCell->numRows);

	nCell->data.S32[binY][binX] ++;
    }
	
    // do any cells have too few PSFSTAR sources?
    // we use 3 as a minimum (slightly arbitrary...)
    for (int iy = 0; iy < nCell->numRows; iy++) { 
	for (int ix = 0; ix < nCell->numCols; ix++) { 
	    if (nCell->data.S32[iy][ix] < 3) {
		int nNew = psphotSupplementStars (stars, sources, binning, ix, iy);
		if (nNew) {
		    psLogMsg ("pmObjects", 3, "added %d fainter PSF candidates to %d,%d\n", nNew, ix, iy);
		} else {
		    psLogMsg ("pmObjects", 3, "tried to add to %d,%d, but no more sources are available\n", ix, iy);		    
		}
	    }
	}
    }

    psFree (nCell);
    psFree (binning);
    return true;
}

// select more possible PSF stars in the specified cell
// sources should be sorted by SN before calling this function
int psphotSupplementStars (psArray *stars, psArray *sources, psImageBinning *binning, int ix, int iy) {

    int nNew = 0;

    float Xs = psImageBinningGetFineX (binning, ix);
    float Xe = psImageBinningGetFineX (binning, ix + 1);

    float Ys = psImageBinningGetFineY (binning, iy);
    float Ye = psImageBinningGetFineY (binning, iy + 1);

    for (int i = 0; i < sources->n; i++) {

	// add sources which are marked as stars, in S/N order until...
	pmSource *source = sources->data[i];
        if (source->peak == NULL) continue;
        if (source->moments == NULL) continue;
        if (source->tmpFlags & PM_SOURCE_TMPF_CANDIDATE_PSFSTAR) continue;
        if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
        if (source->type != PM_SOURCE_TYPE_STAR) continue;

	float x = source->peak->xf;
	float y = source->peak->yf;

	if (x < Xs) continue;
	if (y < Ys) continue;
	if (x > Xe) continue;
	if (y > Ye) continue;

	source->tmpFlags |= PM_SOURCE_TMPF_CANDIDATE_PSFSTAR;
	psArrayAdd (stars, 200, source);

	nNew ++;
    }
    return nNew;
}

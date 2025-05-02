#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// XXX I originally coded this to create a new pmFPAfile, but in retrospect it makes more sense
// to treat this function as an operation on the input image

// XXX make the choice of stats optional
bool ppImageCheckCTE(pmConfig *config, ppImageOptions *options, pmFPAview *view)
{
    bool status;

    // this step is complete optional.
    if (!options->checkCTE) {
        return true;
    }

    // psTimerStart("check.cte");

    // add recipe options supplied on command line
    psMetadata *recipe  = psMetadataLookupPtr(&status, config->recipes, RECIPE_NAME);

    // find the currently selected readout
    pmReadout *inReadout = pmFPAfileThisReadout(config->files, view, "PPIMAGE.INPUT");

    psImage *image    = inReadout->image;
    psImage *mask     = inReadout->mask;
    // psImage *variance = inReadout->variance;

    // I have the fine image size, I know the binning factor, determine the ruff image size
    psImageBinning *binning = psImageBinningAlloc();
    binning->nXfine = image->numCols;
    binning->nYfine = image->numRows;
    binning->nXbin  = psMetadataLookupS32 (&status, recipe, "CTE.XBIN");
    binning->nYbin  = psMetadataLookupS32 (&status, recipe, "CTE.YBIN");

    psImageBinningSetRuffSize(binning, PS_IMAGE_BINNING_CENTER);
    psImageBinningSetSkip(binning, image);

    psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN);
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    psImageBackground (stats, NULL, image, mask, 0xffff, rng);
    float cellMedian = stats->robustMedian;
    psFree (stats);

    // stats = psStatsAlloc (PS_STAT_SAMPLE_STDEV);
    // psStats *statsDefaults = psStatsAlloc (PS_STAT_SAMPLE_STDEV);

    stats = psStatsAlloc (PS_STAT_ROBUST_STDEV);
    psStats *statsDefaults = psStatsAlloc (PS_STAT_ROBUST_STDEV);

    // measure median and variance for subimages
    psRegion ruffRegion = {0,0,0,0};
    psRegion fineRegion = {0,0,0,0};
    for (int iy = 0; iy < binning->nYruff; iy++) {
        for (int ix = 0; ix < binning->nXruff; ix++) {

            // convert the ruff grid cell to the equivalent fine grid cell
            ruffRegion = psRegionSet (ix, ix + 1, iy, iy + 1);
            fineRegion = psImageBinningSetFineRegion (binning, ruffRegion);
            fineRegion = psRegionForImage (image, fineRegion);
            if (fineRegion.x0 >= image->numCols) continue;
	    if (fineRegion.y0 >= image->numRows) continue;

            psImage *subset  = psImageSubset (image, fineRegion);
            if (!subset->numCols || !subset->numRows) {
                psFree (subset);
                continue;
            }
            psImage *submask = NULL;
            if (mask) {
                submask = psImageSubset (mask, fineRegion);
            }

            // reset the default values
            statsDefaults->tmpData = stats->tmpData; // XXX this is fairly hackish: tmpData is internal storage for stats; the assign drops the reference...
            *stats = *statsDefaults;
            statsDefaults->tmpData = NULL;

            psImageStats (stats, subset, submask, 0xffff);

	    // XXX need to apply the gain as well
	    float normVariance = PS_SQR(stats->robustStdev) / cellMedian;
	    // float normVariance = PS_SQR(stats->sampleStdev) / cellMedian;
	    // if (!isfinite(normVariance)) fprintf (stderr, "** normVariance is nan **\n");

	    // apply resulting value to the input pixels
	    for (int jy = fineRegion.y0; jy < fineRegion.y1; jy++) {
	      if (jy < 0) continue;
	      if (jy >= image->numRows) continue;
	      for (int jx = fineRegion.x0; jx < fineRegion.x1; jx++) {
		if (jx < 0) continue;
		if (jx >= image->numCols) continue;
		image->data.F32[jy][jx] = normVariance;
	      }
	    }

            psFree (subset);
            psFree (submask);
        }
    }

    psFree (rng);
    psFree (binning);
    psFree (stats);
    psFree (statsDefaults);

    // psLogMsg ("ppImage", 5, "check CTE: %f sec\n", psTimerMark ("check.cte"));

    return true;
}

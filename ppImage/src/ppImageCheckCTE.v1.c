#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

bool ppImageCheckCTE(pmConfig *config, ppImageOptions *options, pmFPAview *view)
{
    bool status;

    // this step is complete optional.
    if (!options->checkCTE) {
        return true;
    }

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

    pmCell *inCell  = pmFPAfileThisCell (config->files, view, "PPIMAGE.INPUT");
    pmCell *outCell = pmFPAfileThisCell (config->files, view, "PPIMAGE.CTEMAP");
    if (!pmCellCopyStructure(outCell, inCell, binning->nXbin, binning->nYbin)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to copy cell structure.");
        return false;
    }

    pmReadout *outRO = pmFPAfileThisReadout(config->files, view, "PPIMAGE.CTEMAP");
    psImage *output = outRO->image;

    // Don't care about the bias: get rid of it, if present
    psFree(outRO->bias);
    outRO->bias = psListAlloc(NULL);
    psMetadataItem *biassec = psMetadataLookup(outCell->concepts, "CELL.BIASSEC");
    psFree(biassec->data.V);
    biassec->data.V = psListAlloc(NULL);

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    psStats *statsDefaults = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);

    // measure median and variance for subimages
    psRegion ruffRegion = {0,0,0,0};
    psRegion fineRegion = {0,0,0,0};
    for (int iy = 0; iy < output->numRows; iy++) {
        for (int ix = 0; ix < output->numCols; ix++) {

            // convert the ruff grid cell to the equivalent fine grid cell
            ruffRegion = psRegionSet (ix, ix + 1, iy, iy + 1);
            fineRegion = psImageBinningSetFineRegion (binning, ruffRegion);
            fineRegion = psRegionForImage (image, fineRegion);
            if (fineRegion.x0 >= image->numCols || fineRegion.x1 >= image->numCols ||
                fineRegion.y0 >= image->numRows || fineRegion.y1 >= image->numRows) {
                continue;
            }

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

            psImageStats (stats, subset, submask, 0);

            // XXX need to apply the gain as well
            output->data.F32[iy][ix] = PS_SQR(stats->sampleStdev) / stats->sampleMedian;

            psFree (subset);
            psFree (submask);
        }
    }

    psFree (binning);
    psFree (stats);
    psFree (statsDefaults);

    return true;
}

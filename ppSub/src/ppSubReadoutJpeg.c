#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"

bool ppSubReadoutJpeg(pmConfig *config)
{
    psAssert(config, "Require configuration");

    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *outRO = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT"); // Readout to jpeg

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");

    // Generate binned JPEGs
    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels

    int bin1 = psMetadataLookupS32(NULL, recipe, "BIN1"); // First binning level
    int bin2 = psMetadataLookupS32(NULL, recipe, "BIN2"); // Second binning level

    // Target cells
    pmCell *cell1 = pmFPAfileThisCell(config->files, view, "PPSUB.OUTPUT.JPEG1"); // Rebinned cell once
    pmCell *cell2 = pmFPAfileThisCell(config->files, view, "PPSUB.OUTPUT.JPEG2"); // Rebinned cell twice
    psFree(view);

    pmReadout *ro1 = pmReadoutAlloc(cell1), *ro2 = pmReadoutAlloc(cell2); // Binned readouts
    if (!pmReadoutRebin(ro1, outRO, maskBad, bin1, bin1)) {
        psError(PPSUB_ERR_DATA, false, "Unable to bin output (1st binning)");
        psFree(ro1);
        psFree(ro2);
        return false;
    }
    if (!pmReadoutRebin(ro2, ro1, 0, bin2, bin2)) {
        psError(PPSUB_ERR_DATA, false, "Unable to bin output (2nd binning)");
        psFree(ro1);
        psFree(ro2);
        return false;
    }
    psFree(ro1);
    psFree(ro2);

    return true;
}

bool ppSubResidualSampleJpeg(pmConfig *config)
{
    return true;

    pmFPAview *view = ppSubViewReadout(); // View to readout

    // we save sample difference stamps on the convolved image readout->analysis metadata
    pmReadout *inConv = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV"); // Input convolved

    psArray *samples = psArrayAllocEmpty(16); // Array of sample stamp images

    // we may have multiple entries with the same name; pull them off into a separate array:
    psString regex = NULL;          // Regular expression
    psStringAppend(&regex, "^%s$", "SUBTRACTION.SAMPLE.STAMP.SET");
    psMetadataIterator *iter = psMetadataIteratorAlloc(inConv->analysis, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);

    psMetadataItem *item = NULL;// Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        assert(item->type == PS_DATA_ARRAY);
        psArray *sampleStamps = item->data.V;
        samples = psArrayAdd(samples, 16, sampleStamps);
    }
    psFree(iter);
    psAssert (samples, "no sample stamps?");
    psAssert (samples->n, "no sample stamps?");

    // get the kernel sizes
    psArray *kernels = samples->data[0];
    psAssert (kernels, "no valid kernel?");
    psAssert (kernels->n, "no valid kernel?");

    psImage *kernel = kernels->data[0];
    psAssert (kernel, "missing valid kernel?");

    int DX = kernel->numCols;
    int DY = kernel->numRows;

    // each array contains up to 9 sample stamps.  generate an image mosaicking these together in 3x3 blocks.
    int innerBorder = 1;
    int outerBorder = 2;
    int NXblock = sqrt(samples->n);
    int NYblock = samples->n / NXblock;
    if (samples->n % NXblock) NYblock ++;

    int NXpix = NXblock * (3 * (DX + innerBorder) + outerBorder);
    int NYpix = NYblock * (3 * (DY + innerBorder) + outerBorder);

    // output cell
    pmCell *cell = pmFPAfileThisCell(config->files, view, "PPSUB.OUTPUT.RESID.JPEG");
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(NXpix, NYpix, PS_TYPE_F32);

    cell->data_exists = true;
    cell->parent->data_exists = true;

    psImageInit (readout->image, 0.0);

    for (int i = 0; i < samples->n; i++) {

        int xBlock = i % NXblock;
        int yBlock = i / NYblock;

        psArray *kernels = samples->data[i];

        for (int j = 0; j < kernels->n; j++) {

            psImage *kernel = kernels->data[j];

            int xSub = j % 3;
            int ySub = j / 3;

            int xPix = xBlock * (3 * (DX + innerBorder) + outerBorder) + xSub * (DX + innerBorder);
            int yPix = yBlock * (3 * (DX + innerBorder) + outerBorder) + ySub * (DY + innerBorder);

            for (int y = 0; y < kernel->numRows; y++) {
                for (int x = 0; x < kernel->numCols; x++) {
                    readout->image->data.F32[y + yPix][x + xPix] = kernel->data.F32[y][x];
                }
            }
        }
    }

    {
        psFits *fits = psFitsOpen ("resid.stamps.fits", "w");
        psFitsWriteImage (fits, NULL, readout->image, 0, NULL);
        psFitsClose (fits);
    }

    psFree(view);
    return true;
}

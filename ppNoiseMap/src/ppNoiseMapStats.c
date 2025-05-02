#include "ppNoiseMap.h"

bool ppNoiseMapStats(pmReadout *out, const pmReadout *in, psImageMaskType maskVal, int xBin, int yBin)
{
    PM_ASSERT_READOUT_NON_NULL(out, false);
    PM_ASSERT_READOUT_NON_NULL(in, false);

    psImage *inImage = in->image, *inMask = in->mask; // Input image
    int numColsIn = inImage->numCols, numRowsIn = inImage->numRows; // Size of input image

    psImageBinning *binning = psImageBinningAlloc(); // Binning instructions
    binning->nXbin = xBin;
    binning->nYbin = yBin;
    binning->nXfine = numColsIn;
    binning->nYfine = numRowsIn;
    binning->nXskip = 0;
    binning->nYskip = 0;
    psImageBinningSetRuffSize(binning, PS_IMAGE_BINNING_CENTER);

    int numColsOut = binning->nXruff, numRowsOut = binning->nYruff; // Size of output image

    // re-use or re-generate the image
    psImage *outImage;                  // Output image
    if (out->image && out->image->numCols >= numColsOut && out->image->numRows >= numRowsOut) {
        outImage = out->image;
    } else {
        outImage = out->image = psImageRecycle(out->image,  numColsOut, numRowsOut, PS_TYPE_F32);
    }

    psImage *outMask;                   // Output mask
    if (out->mask && out->mask->numCols >= numColsOut && out->mask->numRows >= numRowsOut) {
        outMask = out->mask;
    } else {
        outMask = out->mask = psImageRecycle(out->mask,  numColsOut, numRowsOut, PS_TYPE_IMAGE_MASK);
    }

    int nPixels = binning->nXbin * binning->nYbin;
    psVector *values = psVectorAlloc (nPixels, PS_TYPE_F32);
    psStats *stats = psStatsAlloc (PS_STAT_ROBUST_STDEV);
    
    int xLast = numColsIn - 1, yLast = numRowsIn - 1; // Last index
    int yStart = psImageBinningGetFineY(binning, 0); // Starting input y for binning
    for (int yOut = 0; yOut < numRowsOut; yOut++) {
        int yStop = psImageBinningGetFineY(binning, yOut + 1); // Stopping input y for binning
        yStop = PS_MIN(yStop, yLast);
        int xStart = psImageBinningGetFineX(binning, 0); // Starting input x for binning
        for (int xOut = 0; xOut < numColsOut; xOut++) {
            int xStop = psImageBinningGetFineX(binning, xOut + 1); // Stopping input x for binning
            xStop = PS_MIN(xStop, xLast);

	    // save the pixels for this subcell into the vector, 
            int numPix = 0;             // Number of pixels
            for (int y = yStart; y < yStop; y++) {
                for (int x = xStart; x < xStop; x++) {
                    if (inMask && (inMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) {
                        continue;
                    }
                    values->data.F32[numPix] = inImage->data.F32[y][x];
                    numPix++;
                }
            }
	    values->n = numPix;

	    // measure the stats for this subcell
	    // Values to set
            float imageValue;
	    psImageMaskType maskValue;
	    psStatsInit (stats);
	    if (!psVectorStats (stats, values, NULL, NULL, 0)) {
		psWarning ("failure to measure stats for subcell %d,%d\n", xOut, yOut);
                imageValue = NAN;
                maskValue = maskVal;
	    } else {
                imageValue = stats->robustStdev;
                maskValue = 0;
            } 
            outImage->data.F32[yOut][xOut] = imageValue;
            outMask->data.PS_TYPE_IMAGE_MASK_DATA[yOut][xOut] = maskValue;
            xStart = xStop;
        }
        yStart = yStop;
    }

    psFree(binning);
    psFree(values);
    psFree(stats);

    out->data_exists = true;
    if (out->parent) {
        pmCell *outCell = out->parent;  // Output cell
        outCell->data_exists = outCell->parent->data_exists = true;

        // We would copy the concepts from the input cell, except that is done by pmFPACopy,
        // pmChipCopyStructure, etc.  This function just does the mechanics of binning.
        // We don't even update the CELL.XBIN, CELL.YBIN because that would apply the correction twice.
    }

    return true;
}

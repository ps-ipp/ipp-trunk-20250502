#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmFPA.h"
#include "pmFPABin.h"

bool pmReadoutRebin(pmReadout *out, const pmReadout *in, psImageMaskType maskVal, int xBin, int yBin)
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

    int Nbits = (int) (ceil(log(maskVal)/log(2)) + 1);
    int *bitcounter = malloc(sizeof(int) * Nbits);
    int pxlcount;

    int xLast = numColsIn - 1, yLast = numRowsIn - 1; // Last index
    int yStart = psImageBinningGetFineY(binning, 0); // Starting input y for binning
    for (int yOut = 0; yOut < numRowsOut; yOut++) {
	int yStop = psImageBinningGetFineY(binning, yOut + 1); // Stopping input y for binning
	yStop = PS_MIN(yStop, yLast);
	int xStart = psImageBinningGetFineX(binning, 0); // Starting input x for binning
	for (int xOut = 0; xOut < numColsOut; xOut++) {
	    int xStop = psImageBinningGetFineX(binning, xOut + 1); // Stopping input x for binning
	    xStop = PS_MIN(xStop, xLast);

	    float sum = 0.0;            // Sum of pixels
	    int numPix = 0;             // Number of pixels

	    for (int j = 0; j < Nbits; j++) { // Reset bit counter
		bitcounter[j] = 0;
	    }
	    pxlcount = 0;
	    
	    for (int y = yStart; y < yStop; y++) {
		for (int x = xStart; x < xStop; x++) {
		    if (false && inMask && (inMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] != 0)) {
			for (int j = 0; j < Nbits; j++) {
			    psImageMaskType M = (psImageMaskType) pow(2,j);
			    if (inMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & M) {
				bitcounter[j]++;
			    }
			}
		    }
		  
		    if (inMask && (inMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal)) {
			continue;
		    }
		    if (!isfinite(inImage->data.F32[y][x])) {
			continue;
		    }
		    sum += inImage->data.F32[y][x];
		    numPix++;


		}
	    }
	    
	    // Values to set
	    float imageValue;
	    psImageMaskType maskValue;
	    if (numPix > 0) {
		imageValue = sum / numPix;
		maskValue = 0;
	    } else {
		imageValue = NAN;
		maskValue = maskVal;
	    }
	    outImage->data.F32[yOut][xOut] = imageValue;
	    if (true) {
		outMask->data.PS_TYPE_IMAGE_MASK_DATA[yOut][xOut] = maskValue;
	    } else {
		outMask->data.PS_TYPE_IMAGE_MASK_DATA[yOut][xOut] = 0;
	    }
	    // this loop is pointless if pxlcount == 0 (all masked)
	    if (false) {
		if (pxlcount) {
		    for (int j = 0; j < Nbits; j++) {
			if (bitcounter[j] > 0.5 * pxlcount) {
			    outMask->data.PS_TYPE_IMAGE_MASK_DATA[yOut][xOut] |= (1 << j);
			}
		    }
		} else {
		    outMask->data.PS_TYPE_IMAGE_MASK_DATA[yOut][xOut] = maskValue;
		}
	    }
	    xStart = xStop;
	}
	yStart = yStop;
    }

    psFree(binning);

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

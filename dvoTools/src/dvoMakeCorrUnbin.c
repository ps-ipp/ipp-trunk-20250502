#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoMakeCorr.h"

bool dvoMakeCorrUnbin (pmConfig *config, pmFPAview *view, char *outName, psImage *inImage, pmChip *inChip, char *refName) {
    
    bool status;

    pmFPAfile *refFile = psMetadataLookupPtr (&status, config->files, refName);
    pmFPAfile *outFile = psMetadataLookupPtr (&status, config->files, outName);
    assert (refFile);
    assert (outFile);

    pmCell *refCell = pmFPAviewThisCell (view, refFile->fpa);
    assert (refCell);

    pmChip *refChip = pmFPAviewThisChip (view, refFile->fpa);
    assert (refChip);

    // determine the output array size based on the reference TRIMSEC values
    psRegion *trimsec = psMetadataLookupPtr(NULL, refCell->concepts, "CELL.TRIMSEC");
    assert (trimsec);

    // I have the fine and ruff image sizes, determine the binning factor
    psImageBinning *binning = psImageBinningAlloc();
    binning->nXruff = inImage->numCols;
    binning->nYruff = inImage->numRows;
    binning->nXfine = trimsec->x1 - trimsec->x0;
    binning->nYfine = trimsec->y1 - trimsec->y0;
    psImageBinningSetScale (binning, PS_IMAGE_BINNING_CENTER);
    psImageBinningSetSkip(binning, inImage);

    // generate a local view of the 0th readout
    pmFPAview *myView = pmFPAviewAlloc (0);
    *myView = *view;
    myView->readout = 0;

    // construct the supporing pmFPA/pmChip/pmCell structures
    pmReadout *outData = pmFPAviewThisReadout (myView, outFile->fpa);
    if (outData == NULL) {
	pmChip *outChip = pmFPAviewThisChip (myView, outFile->fpa);
	pmCell *outCell = pmFPAviewThisCell (myView, outFile->fpa);
	assert (outCell);

	pmChipCopyStructure (outChip, refChip, 1, 1);

	outData = pmReadoutAlloc (outCell);
	psFree (outData); // free the extra reference
	assert (outData != NULL);
    }

    // generate the output (fine-scale) image array
    outData->image = psImageRecycle (outData->image, binning->nXfine, binning->nYfine, PS_TYPE_F32);

    // linear interpolation to full fine scale
    if (!psImageUnbin (outData->image, inImage, binning)) {
	psError (PS_ERR_UNKNOWN, true, "failed to unbin image");
	psFree (myView);
	psFree (binning);
	return false;
    }

    // the input image is in magnitudes.  convert here to a multiplicative factor...
    // the calculation here must be consistent with the calculation of the grid correction
    // in relphot GridOps.c:259 (setMgrid)
    for (int j = 0; j < outData->image->numRows; j++) {
	for (int i = 0; i < outData->image->numCols; i++) {
	    float value = outData->image->data.F32[j][i]; 
	    outData->image->data.F32[j][i] = pow(10.0, -0.4*value);
	    // outData->image->data.F32[j][i] = pow(10.0, 0.4*value);
	}
    }

    psFree (myView);
    psFree (binning);
 
    return true;
}

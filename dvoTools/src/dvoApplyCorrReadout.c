#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoApplyCorr.h"

// this function is operating on each cell (well, readout) of the flat to be corrected.  The
// input chip is supplied, but may be in a different format (eg, chip-mosaic vs single cell)

bool dvoApplyCorrReadout (pmCell *inCell, pmChip *corrChip) {
    
    // XXX for now, let's just assume the input is per-cell, and the correction is per-chip
    // Also, let's assume they are both binned 1x1

    bool status;

    pmCell *corrCell = corrChip->cells->data[0];
    pmReadout *corrRO = corrCell->readouts->data[0];
    psImage *corrImage = corrRO->image;

    pmReadout *inRO = inCell->readouts->data[0];
    psImage *inImage = inRO->image;

    int x0 = psMetadataLookupS32(&status, inCell->concepts, "CELL.X0"); // Position of (0,0) on chip
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "CELL.X0 hasn't been set for cell.\n");
        return false;
    }
    int y0 = psMetadataLookupS32(&status, inCell->concepts, "CELL.Y0"); // Position of (0,0) on chip
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "CELL.Y0 hasn't been set for cell.\n");
        return false;
    }

    int xParity = psMetadataLookupS32(&status, inCell->concepts, "CELL.XPARITY"); // Parity in x
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "CELL.XPARITY hasn't been set for cell.\n");
        return false;
    }
    int yParity = psMetadataLookupS32(&status, inCell->concepts, "CELL.YPARITY"); // Parity in y
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "CELL.YPARITY hasn't been set for cell.\n");
        return false;
    }

    // The corr image is a multiplicative factor.  Need to convert the input flat pixel
    // coordinates to the correction cell/pixel coords.
    for (int j = 0; j < inImage->numRows; j++) {
	int jC = y0 + j*yParity;
	for (int i = 0; i < inImage->numCols; i++) {
	    int iC = x0 + i*xParity;
	    inImage->data.F32[j][i] *= corrImage->data.F32[jC][iC]; 
	}
    }
    return true;
}

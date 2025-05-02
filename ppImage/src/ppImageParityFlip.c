#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// Update a concept to the assumed value
#define FIX_CONCEPT(SOURCE, NAME, TYPE, VALUE) {        \
psMetadataItem *item = psMetadataLookup(SOURCE, NAME); \
item->data.TYPE = VALUE; }

// flip the image to have 'native' (detector) or 'raw' (readout) orientation 
bool ppImageParityFlip (pmConfig *config, const ppImageOptions *options, const pmFPAview *view, bool native) {

    bool status;
    int xParity, yParity;
    int xParityRaw, yParityRaw;
    int xParityTarget, yParityTarget;

    if (!options->applyParity) return true;

    // psTimerStart("parity.flip");

    // find the currently selected readout
    pmReadout *readout = pmFPAfileThisReadout(config->files, view, "PPIMAGE.INPUT");

    pmCell *cell = readout->parent;

    if (native) {
	// find the current (raw) parity
	xParity = psMetadataLookupS32(&status, cell->concepts, "CELL.XPARITY");
	if (!status || (xParity != -1 && xParity != 1)) {
	    psWarning("CELL.XPARITY is not set for the input cell; assuming 1.\n");
	    FIX_CONCEPT(cell->concepts, "CELL.XPARITY", S32, 1);
	    xParity = 1;
	}
	yParity = psMetadataLookupS32(&status, cell->concepts, "CELL.YPARITY");
	if (!status || (yParity != -1 && yParity != 1)) {
	    psWarning("CELL.YPARITY is not set for the input cell; assuming 1.\n");
	    FIX_CONCEPT(cell->concepts, "CELL.YPARITY", S32, 1);
	    yParity = 1;
	}

	// save the raw parity
	psMetadataAddS32 (cell->concepts, PS_LIST_TAIL, "CELL.XPARITY.RAW", PS_META_REPLACE, "original parity", xParity);
	psMetadataAddS32 (cell->concepts, PS_LIST_TAIL, "CELL.YPARITY.RAW", PS_META_REPLACE, "original parity", yParity);

	xParityTarget = 1;
	yParityTarget = 1;
    } else {
	// find the current (native) parity
	xParity = psMetadataLookupS32(&status, cell->concepts, "CELL.XPARITY");
	psAssert (status, "CELL.XPARITY not set : cannot call ppImageParityFlip (native = false) without first calling it with native = true");

	yParity = psMetadataLookupS32(&status, cell->concepts, "CELL.YPARITY");
	psAssert (status, "CELL.YPARITY not set : cannot call ppImageParityFlip (native = false) without first calling it with native = true");

	// find the raw parity
	xParityRaw = psMetadataLookupS32(&status, cell->concepts, "CELL.XPARITY.RAW");
	psAssert (status, "CELL.XPARITY not set : cannot call ppImageParityFlip (native = false) without first calling it with native = true");

	yParityRaw = psMetadataLookupS32(&status, cell->concepts, "CELL.YPARITY.RAW");
	psAssert (status, "CELL.YPARITY not set : cannot call ppImageParityFlip (native = false) without first calling it with native = true");

	xParityTarget = xParityRaw;
	yParityTarget = yParityRaw;
    }

    // for this case, nothing to be done:
    if ((xParity == xParityTarget) && (yParity == yParityTarget)) return true;

    psImage *image = readout->image;
    psImage *var   = readout->variance;
    psImage *mask  = readout->mask;

    if (var) psAssert (image->numCols == var->numCols,  "image and variance sizes are mismatched");
    if (var) psAssert (image->numRows == var->numRows,  "image and variance sizes are mismatched");
    if (mask) psAssert (image->numCols == mask->numCols, "image and mask sizes are mismatched");
    if (mask) psAssert (image->numRows == mask->numRows, "image and mask sizes are mismatched");

    int Nx = image->numCols;
    int Ny = image->numRows;

    // we need working buffers for the image, variance, and mask:
    psF32 *imrow = (psF32 *) psAlloc (Nx*sizeof(psF32));
    psF32 *wtrow = (psF32 *) psAlloc (Nx*sizeof(psF32));
    psImageMaskType *mkrow = (psImageMaskType *) psAlloc (Nx*sizeof(psImageMaskType));

    // the three cases (+1,-1), (-1,+1), (-1,-1) should be handled independently

    // flip only in x-direction
    if ((xParity != xParityTarget) && (yParity == yParityTarget)) {
	for (int iy = 0; iy < Ny; iy++) {
	    for (int ix = 0; ix < Nx; ix++) {
		imrow[Nx-1-ix] = image->data.F32[iy][ix];
	    }
	    for (int ix = 0; (var != NULL) && (ix < Nx); ix++) {
		wtrow[Nx-1-ix] = var->data.F32[iy][ix];
	    }
	    for (int ix = 0; (mask != NULL) && (ix < Nx); ix++) {
		mkrow[Nx-1-ix] = mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix];
	    }
	    memcpy (image->data.F32[iy], imrow, Nx*sizeof(psF32));
	    if (var) memcpy (var->data.F32[iy],   wtrow, Nx*sizeof(psF32));
	    if (mask) memcpy (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy], mkrow, Nx*sizeof(psImageMaskType));
	}
    }
    // flip only in y-direction
    if ((xParity == xParityTarget) && (yParity != yParityTarget)) {
	for (int iy = 0; iy < Ny/2; iy++) {
	    memcpy (imrow, image->data.F32[iy], Nx*sizeof(psF32));
	    memcpy (image->data.F32[iy], image->data.F32[Ny-1-iy], Nx*sizeof(psF32));
	    memcpy (image->data.F32[Ny-1-iy], imrow, Nx*sizeof(psF32));

	    if (var) {
		memcpy (wtrow, var->data.F32[iy], Nx*sizeof(psF32));
		memcpy (var->data.F32[iy], var->data.F32[Ny-1-iy], Nx*sizeof(psF32));
		memcpy (var->data.F32[Ny-1-iy], wtrow, Nx*sizeof(psF32));
	    }

	    if (mask) {
		memcpy (mkrow, mask->data.PS_TYPE_IMAGE_MASK_DATA[iy], Nx*sizeof(psImageMaskType));
		memcpy (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy], mask->data.PS_TYPE_IMAGE_MASK_DATA[Ny-1-iy], Nx*sizeof(psImageMaskType));
		memcpy (mask->data.PS_TYPE_IMAGE_MASK_DATA[Ny-1-iy], mkrow, Nx*sizeof(psImageMaskType));
	    }
	}
    }
    // flip in both directions
    if ((xParity != xParityTarget) && (yParity != yParityTarget)) {
	for (int iy = 0; iy < Ny/2; iy++) {
	    for (int ix = 0; ix < Nx; ix++) {
		imrow[Nx-1-ix] = image->data.F32[iy][ix];
	    }
	    for (int ix = 0; var && (ix < Nx); ix++) {
		wtrow[Nx-1-ix] = var->data.F32[iy][ix];
	    }
	    for (int ix = 0; mask && (ix < Nx); ix++) {
		mkrow[Nx-1-ix] = mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix];
	    }

	    for (int ix = 0; ix < Nx; ix++) {
		image->data.F32[iy][Nx-1-ix] = image->data.F32[Ny-1-iy][ix];
	    }
	    for (int ix = 0; var && (ix < Nx); ix++) {
		var->data.F32[iy][Nx-1-ix] = var->data.F32[Ny-1-iy][ix];
	    }
	    for (int ix = 0; mask && (ix < Nx); ix++) {
		mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][Nx-1-ix] = mask->data.PS_TYPE_IMAGE_MASK_DATA[Ny-1-iy][ix];
	    }

	    memcpy (image->data.F32[Ny-1-iy], imrow, Nx*sizeof(psF32));
	    if (var) memcpy (var->data.F32[Ny-1-iy], wtrow, Nx*sizeof(psF32));
	    if (mask) memcpy (mask->data.PS_TYPE_IMAGE_MASK_DATA[Ny-1-iy], mkrow, Nx*sizeof(psImageMaskType));
	}
    }
    // psLogMsg ("ppImage", 5, "parity flip: %f sec\n", psTimerMark ("parity.flip"));

    FIX_CONCEPT(cell->concepts, "CELL.XPARITY", S32, xParityTarget);
    FIX_CONCEPT(cell->concepts, "CELL.YPARITY", S32, yParityTarget);

    psFree (imrow);
    psFree (wtrow);
    psFree (mkrow);
    return true;
}

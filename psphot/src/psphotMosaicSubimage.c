# include "psphotInternal.h"

// insert the source image into the outimage at Xo, Yo
bool psphotMosaicSubimage (psImage *outImage, pmSource *source, int Xo, int Yo, int DX, int DY, bool normalize) {

    psRegion inRegion, outRegion;
    psImage *inImage = source->pixels;

    // identify the region in the output image
    outRegion = psRegionSet (Xo, Xo + DX, Yo, Yo + DX);
    outRegion = psRegionForImage (outImage, outRegion);
    int DXo = outRegion.x1 - outRegion.x0;
    int DYo = outRegion.y1 - outRegion.y0;
    if (DXo <= 0) return false;
    if (DYo <= 0) return false;
    
    // center the input source in the output box
    int dX = (DXo - 1) / 2;
    int dY = (DYo - 1) / 2;

    // int xo = inImage->col0 + inImage->numCols / 2;
    // int yo = inImage->row0 + inImage->numRows / 2;

    int xo = source->peak->xf;
    int yo = source->peak->yf;

    // adjust region to overlay input image pixels
    inRegion = psRegionSet (xo - dX, xo + dX + 1, yo - dY, yo + dY + 1);
    inRegion = psRegionForImage (inImage, inRegion);

    float peak = source->peak->rawFlux;

    psImage *subImage = psImageSubset (inImage, inRegion);
    if (!subImage) {
	psErrorClear(); // XXX I think there is an error in psphotVisual that is supplying bad images
	return false;
    }

    psImage *newImage = psImageAlloc (subImage->numCols, subImage->numRows, PS_TYPE_F32);
    for (int iy = 0; iy < newImage->numRows; iy++) {
	for (int ix = 0; ix < newImage->numCols; ix++) {
	    if (normalize) {
		newImage->data.F32[iy][ix] = subImage->data.F32[iy][ix] / peak;
	    } else {
		newImage->data.F32[iy][ix] = subImage->data.F32[iy][ix];
	    }
	}
    }

    psImageOverlaySection (outImage, newImage, Xo, Yo, "=");

    psFree (subImage);
    psFree (newImage);
    return true;
}

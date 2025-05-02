/** @file psastroExtractStar.c
 *
 *  @brief 
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

psImage *psastroExtractStar (psImage *input, double x, double y, double dX, double dY) {

    // skip if no pixels are on the image
    // skip if center is not on the image
    // if bounds fall off image, paste into a full-size image.

    if (x < 0) return NULL;
    if (y < 0) return NULL;
    if (x >= input->numCols) return NULL;
    if (y >= input->numRows) return NULL;

    int xo = x; // index of center pixel (eg, 6.00 to 6.99 -> 6)
    int yo = y;

    // build an output image of size 2dX + 1, 2dY + 1
    // psRegion fullRegion = psRegionSet (xo - dX, x + dX + 1, y - dY, y + dY + 1);
    // psRegion realRegion = psRegionForImage (input, fullRegion);
    // psImage *subset = psImageSubset (input, realRegion);

    psImage *subset = psImageAlloc (2*dX+1, 2*dY+1, PS_TYPE_F32);
    psImageInit (subset, 0.0);

    // fill in the subset image with the values from the subset
    // I must already have done this elsewhere...

    for (int iy = yo - dY; iy < yo + dY + 1; iy++) {
	for (int ix = xo - dX; ix < xo + dX + 1; ix++) {

	    if (ix < input->col0) continue;
	    if (iy < input->row0) continue;
	    if (ix >= input->numCols) continue;
	    if (iy >= input->numRows) continue;
	
	    int jx = ix + dX - xo; // runs from 0 to 2dX
	    int jy = iy + dY - yo;
	    subset->data.F32[jy][jx] = input->data.F32[iy][ix];
	}
    }
    return subset;
}

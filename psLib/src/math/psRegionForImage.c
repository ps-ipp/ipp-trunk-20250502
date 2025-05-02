#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include "psError.h"
#include "psRegion.h"
#include "psImage.h"
#include "psRegionForImage.h"

// set actual region based on image parameters:
// - compensate for negative upper limits
// - force range to be on this image
// - saturate on upper and lower limits of image
// - flip x0,x1 if x0>x1
// - flip y0,y1 if y0>y1
// psRegion in refers to coordinates in the *parent* image
psRegion psRegionForImage(const psImage *image,
                          psRegion in)
{

    //    if (image == NULL) {
    //        return in;
    //    }
    PS_ASSERT_IMAGE_NON_NULL(image, in);
    //if the region is [0,0,0,0], the whole image (or subimage) is to be included.
    if (in.x0 == 0 && in.x1 == 0 && in.y0 == 0 && in.y1 == 0) {
        in.x0 = image->col0;
        in.x1 = image->col0 + image->numCols;
        in.y0 = image->row0;
        in.y1 = image->row0 + image->numRows;
        return (in);
    }

    // convert non-positive upper-limits
    // XXX note that the upper limit in these cases is defined relative to the subimage
    // also note that truncation limits to the valid subimage pixels
    in.x1 = (in.x1 <= 0) ? (image->col0 + image->numCols + in.x1) : in.x1;
    in.y1 = (in.y1 <= 0) ? (image->row0 + image->numRows + in.y1) : in.y1;

    // force the upper-limits to be on the image
    in.x1 = PS_MIN(image->col0 + image->numCols, in.x1);
    in.y1 = PS_MIN(image->row0 + image->numRows, in.y1);

    // force the lower-limits to be on the image
    in.x0 = PS_MAX(image->col0, in.x0);
    in.y0 = PS_MAX(image->row0, in.y0);
    in.x0 = PS_MIN(image->col0 + image->numCols, in.x0);
    in.y0 = PS_MIN(image->row0 + image->numRows, in.y0);

    // flip start and end if out of order
    if (in.x0 > in.x1) {
        psError (PS_ERR_BAD_PARAMETER_VALUE, true,
                 "Invalid region in psRegionForImage.  x0 > x1.  Values have been swapped.\n");
        PS_SWAP (in.x0, in.x1);
    }
    if (in.y0 > in.y1) {
        psError (PS_ERR_BAD_PARAMETER_VALUE, true,
                 "Invalid region in psRegionForImage.  y0 > y1.  Values have been swapped.\n");
        PS_SWAP (in.y0, in.y1);
    }

    return (in);
}


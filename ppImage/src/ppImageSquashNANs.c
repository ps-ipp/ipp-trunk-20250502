#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

# define LO_16BIT -32768
# define HI_16BIT  32768
# define MX_16BIT  65536

bool ppImageSquashNANsImage (psImage *image, float minValue, float maxValue, float offset);

bool ppImageSquashNANs(pmConfig *config, ppImageOptions *options, pmFPAview *view)
{
    // this step is completely optional.
    if (!options->squashNANs) {
	return true;
    }
    
    psTimerStart("squash.NANs");

    // find the currently selected readout
    pmReadout *readout = pmFPAfileThisReadout(config->files, view, "PPIMAGE.INPUT");

    psImage *image = readout->image;    // Readout's image

    // We are going to force the range to be (-999 to 2^16 - 1002, ie, 1 less on each end than 2^16. this allows for float round errors
    int bzero = LO_16BIT - 0;
    float minValue = HI_16BIT + bzero + 1; 
    float maxValue = MX_16BIT + minValue - 3;

    if (image->parent) {
	psImage *parent = (psImage *) image->parent;
	ppImageSquashNANsImage (parent, minValue, maxValue, 500.0);
    } else {
	ppImageSquashNANsImage (image, minValue, maxValue, 500.0);

	// squash NANs in overscan regions
	psList *overscans = readout->bias; // List of the overscan images
	psListIterator *iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	psImage *overscan = NULL; // Overscan image from iterator
	while ((overscan = psListGetAndIncrement(iter))) {
	    ppImageSquashNANsImage (overscan, minValue, maxValue, 500.0);
	}
	psFree(iter);
    } 

    psLogMsg ("ppImage", 5, "squash NANs: %f sec\n", psTimerMark ("squash.NANs"));
    return true;
}

bool ppImageSquashNANsImage (psImage *image, float minValue, float maxValue, float offset) {
  
    int numCols = image->numCols, numRows = image->numRows; // Size of image
  
    for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	    if (!isfinite(image->data.F32[y][x])) {
		image->data.F32[y][x] = 1.0;
	    } else {
		image->data.F32[y][x] = PS_MAX (minValue, PS_MIN (maxValue, image->data.F32[y][x] + 500.0));
	    }
	}
    }
    return true;
}


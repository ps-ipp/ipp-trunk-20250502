# include "psphotInternal.h"
# ifndef ROUND
# define ROUND(X) ((int) ((X) + 0.5*SIGN(X)))
# endif

psVector *getSaddlePoint (pmReadout *readout, int X1, int Y1, int X2, int Y2, bool swapcoords);

// for each footprint with multiple detections, find the complete set of saddles, that is, the
// local minima which lie on the lines between the peaks (same as a 'Coll' but less homophonic
// with 'cull')
bool psphotFootprintSaddles(pmReadout *readout, psArray *footprints) {  // array of pmFootprints
    
# if (PM_PEAKS_CULL_WITH_SMOOTHED_IMAGE)
    psLogMsg ("psphot", PS_LOG_INFO, "Finding saddle points from footprints using the smoothed image");
# else
    psLogMsg ("psphot", PS_LOG_INFO, "Finding saddle points from footprints using the raw (unsmoothed) image");
# endif

    int xMax = readout->image->numCols - 1;
    int yMax = readout->image->numRows - 1;

    // FILE *f = fopen ("saddlepoints.dat", "w");

    for (int i = 0; i < footprints->n; i++) {
	pmFootprint *fp = footprints->data[i];
	if (fp->peaks == NULL) continue;
	if (fp->peaks->n < 2) continue;

	// loop over all peaks in the footprint:
	for (int is = 0; is < fp->peaks->n; is++) {

	    pmPeak *startPeak = fp->peaks->data[is];
	    if (!startPeak->saddlePoints) startPeak->saddlePoints = psArrayAllocEmpty (4);
	
	    // loop over all peaks in the footprint, skipping this one
	    for (int ie = is + 1; ie < fp->peaks->n; ie++) {
		if (ie == is) continue;

		pmPeak *endPeak = fp->peaks->data[ie];
		if (!endPeak->saddlePoints) endPeak->saddlePoints = psArrayAllocEmpty (4);

		// step across all pixels between the startPeak and the endPeak, using a
		// bresenham-like line-drawing operation
		
		int X1 = PS_MIN(xMax, PS_MAX(0, ROUND(startPeak->xf)));
		int Y1 = PS_MIN(yMax, PS_MAX(0, ROUND(startPeak->yf)));
		int X2 = PS_MIN(xMax, PS_MAX(0, ROUND(endPeak->xf)));
		int Y2 = PS_MIN(yMax, PS_MAX(0, ROUND(endPeak->yf)));
		
		int dX = X2 - X1;
		int dY = Y2 - Y1;

		bool FlipCoords = (abs(dX) < abs(dY));
		bool FlipDirect = FlipCoords ? (Y1 > Y2) : (X1 > X2);

		psVector *saddlePoint = NULL;
		if (!FlipDirect && !FlipCoords) saddlePoint = getSaddlePoint (readout, X1, Y1, X2, Y2, false);
		if ( FlipDirect && !FlipCoords) saddlePoint = getSaddlePoint (readout, X2, Y2, X1, Y1, false);
		if (!FlipDirect &&  FlipCoords) saddlePoint = getSaddlePoint (readout, Y1, X1, Y2, X2, true);
		if ( FlipDirect &&  FlipCoords) saddlePoint = getSaddlePoint (readout, Y2, X2, Y1, X1, true);
		psAssert (saddlePoint, "failed to get saddlePoint");

		// fprintf (f, "%f %f | %d %d | %f %f\n", startPeak->xf, startPeak->yf, saddlePoint->data.S32[0], saddlePoint->data.S32[1], endPeak->xf, endPeak->yf);

		psArrayAdd (startPeak->saddlePoints, 4, saddlePoint);
		psArrayAdd (endPeak->saddlePoints, 4, saddlePoint);
		psFree (saddlePoint);
		// NOTE : all saddlePoints for the startPeak are also saddlePoints for the endPeak
	    }
	}
    }
    // fclose (f);
    return true;
}

// psLogMsg ("psphot", PS_LOG_INFO, "%ld peaks, %ld total footprints: %f sec\n", detections->peaks->n, detections->footprints->n, 

psVector *getSaddlePoint (pmReadout *readout, int X1, int Y1, int X2, int Y2, bool swapcoords) {

    psAssert (X1 >= 0, "invalid");
    psAssert (Y1 >= 0, "invalid");
    psAssert (X1 < readout->image->numCols, "invalid");
    psAssert (Y1 < readout->image->numRows, "invalid");

    psAssert (X2 >= 0, "invalid");
    psAssert (Y2 >= 0, "invalid");
    psAssert (X2 < readout->image->numCols, "invalid");
    psAssert (Y2 < readout->image->numRows, "invalid");

    psVector *saddlePoint = psVectorAlloc(2, PS_TYPE_F32);

    int dX = X2 - X1;
    int dY = Y2 - Y1;
		
    float minValue = swapcoords ? readout->image->data.F32[X1][Y1] : readout->image->data.F32[Y1][X1];
    saddlePoint->data.S32[0] = swapcoords ? Y1 : X1;
    saddlePoint->data.S32[1] = swapcoords ? X1 : Y1;

    int Y = Y1;
    int e = 0;
    for (int X = X1; X <= X2; X++) {
	if (swapcoords) {
	    float newValue = readout->image->data.F32[X][Y];
	    if (newValue < minValue) {
		minValue = newValue;
		saddlePoint->data.S32[0] = Y;
		saddlePoint->data.S32[1] = X;
	    }
	} else {
	    float newValue = readout->image->data.F32[Y][X];
	    if (newValue < minValue) {
		minValue = newValue;
		saddlePoint->data.S32[0] = X;
		saddlePoint->data.S32[1] = Y;
	    }
	}
	e += dY;
	int e2 = 2 * e;
	if (e2 > dX) {
	    Y++;
	    e -= dX;
	} 
	if (e2 < -dX) {
	    Y--;
	    e += dX;
	}
    }
    return (saddlePoint);
}

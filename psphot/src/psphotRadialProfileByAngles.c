# include "psphotInternal.h"

// Given a source at (x,y), generate a collection of radial profiles at even angular separations

// These functions are used to calculate the stats in a rectangle at arbitrary orientation.
// XXX Move these elsewhere (psLib?)
float psphotMeanSectorValue (psImage *image, float x, float y, float dL, float dW, float theta);
psVector *psphotBoxValues (psImage *image, float x0, float y0, float dL, float dW, float theta);
psVector *psphotLineValues (psImage *image, double x1, double y1, double x2, double y2, int dW);
psVector *psphotLineValuesBresen (psImage *image, int X1, int Y1, int X2, int Y2, int dW, int swapcoords);

bool psphotRadialProfilesByAngles (pmSource *source, int Nsec, float Rmax) {

    psAssert (source->extpars, "define extpars");

    // we want to have an even number of sectors so we can do 180 deg symmetrizing
    Nsec = (Nsec % 2) ? Nsec + 1 : Nsec;
    float dtheta = 2.0*M_PI / Nsec;

    if (!source->extpars->radFlux) {
	source->extpars->radFlux = pmSourceRadialFluxAlloc();
    }
    pmSourceRadialFlux *profile = source->extpars->radFlux;
    psFree(profile->radii);
    psFree(profile->fluxes);
    psFree(profile->theta);

    profile->radii = psArrayAllocEmpty(Nsec);
    profile->fluxes = psArrayAllocEmpty(Nsec);
    profile->theta = psVectorAllocEmpty(Nsec, PS_TYPE_F32);

    for (int i = 0; i < Nsec; i++) {

	float theta = i*dtheta;

	psVector *radius = psVectorAllocEmpty(Rmax, PS_TYPE_F32);
	psVector *flux   = psVectorAllocEmpty(Rmax, PS_TYPE_F32);

	// Start at Xo,Yo and find the x,y locations for r_i, theta where r_i initially
	// increments by 1 pixel.  At large radii (r*dtheta > 2) use stats in a box rather than
	// sub-pixel interpolation

	int dR = 1.0;
	for (float r = 0; r < Rmax; r += dR) {

	    float Xo = source->peak->xf;
	    float Yo = source->peak->yf;

	    // Xo,Yo are referenced to pixels with bounds i+0.0, i+1.0
	    float x = r * cos (theta) + Xo;
	    float y = r * sin (theta) + Yo;
	    dR = 2*(int)(0.5*r*sin(dtheta)) + 1;

	    if (x < 0) goto badvalue;
	    if (y < 0) goto badvalue;
	    if (x >= source->pixels->parent->numCols) goto badvalue;
	    if (y >= source->pixels->parent->numRows) goto badvalue;

	    float value = NAN;
	    if (dR < 2) {
		// value is NAN if we run off the image
		// 0.5 PIX: this function takes pixel coords; source peak is in pixel coords
		value = psImageInterpolatePixelBilinear(x, y, source->pixels);
	    } else {
		// 0.5 PIX: this function takes pixel coords; source peak is in pixel coords
		value = psphotMeanSectorValue(source->pixels, x, y, dR, dR, theta);
	    }

	    // keep the all values (even NAN) so all vectors are matched in length
	    psVectorAppend (radius, r);
	    psVectorAppend (flux, value);
	    continue;
	    
	badvalue:
	    psVectorAppend (radius, r);
	    psVectorAppend (flux, NAN);
	}

	psArrayAdd (profile->radii, 100, radius);
	psArrayAdd (profile->fluxes, 100, flux);
	psVectorAppend (profile->theta, theta);

	// psphotPetrosianVisualProfileByAngle (radius, flux);

	psFree(radius);
	psFree(flux);
    }

    for (int i = 0; i < Nsec / 2; i++) {

	psVector *r1 = profile->radii->data[i];
	psVector *r2 = profile->radii->data[i+Nsec/2];

	psVector *f1 = profile->fluxes->data[i];
	psVector *f2 = profile->fluxes->data[i+Nsec/2];

	psAssert (r1->n == r2->n, "mis-matched vectors");
	psAssert (f1->n == f2->n, "mis-matched vectors");

	// we have a pair of vectors i, i+Nsec/2; replace them with the finite minimum of the pair
	for (int j = 0; j < r1->n; j++) {
	    
	    float flux;

	    if (!isfinite(f1->data.F32[j]) && !isfinite(f2->data.F32[j])) {
		flux = NAN;
		goto setflux;
	    }

	    if (!isfinite(f1->data.F32[j])) {
		flux = f2->data.F32[j];
		goto setflux;
	    }
	    if (!isfinite(f2->data.F32[j])) {
		flux = f1->data.F32[j];
		goto setflux;
	    }

	    flux = PS_MIN(f1->data.F32[j], f2->data.F32[j]);

	setflux:
	    f1->data.F32[j] = flux;
	    f2->data.F32[j] = flux;
	}
    }    
    return true;
}

float psphotMeanSectorValue (psImage *image, float x, float y, float dL, float dW, float theta) {

    psVector *values = psphotBoxValues (image, x, y, dL, dW, theta);
    if (!values) goto escape;
    if (!values->n) goto escape;
    
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    psVectorStats (stats, values, NULL, NULL, 0);

    float value = stats->sampleMedian;

    psFree (stats);
    psFree (values);
    
    return value;

escape:
    psFree(values);
    return NAN;    
}

psVector *psphotBoxValues (psImage *image, float x0, float y0, float dL, float dW, float theta) {

    // extract pixels from a series of lines (from -0.5*dW to +0.5*dW) of length dL, 
    // centered on x0, y0 in parent pixel coordinates (not pixel indicies)

    float xs = x0 - image->col0 - 0.5*dL*cos(theta);
    float ys = y0 - image->row0 - 0.5*dL*sin(theta);

    float xe = xs + 0.5*dL*cos(theta);
    float ye = ys + 0.5*dL*sin(theta);

    psVector *values = psphotLineValues (image, xs, ys, xe, ye, (int) dW);
    return values;
}

/**
 * identify the quadrant and draw the correct line
 */
psVector *psphotLineValues (psImage *image, double x1, double y1, double x2, double y2, int dW) {

  int FlipDirect, FlipCoords;
  int X1, Y1, X2, Y2, dX, dY;

  /* rather than draw the line from float positions, we find the closest
     integer end-points and draw the line between those pixels */

  X1 = ROUND(x1);
  Y1 = ROUND(y1);
  X2 = ROUND(x2);
  Y2 = ROUND(y2);

  dX = X2 - X1;
  dY = Y2 - Y1;

  FlipCoords = (abs(dX) < abs(dY));
  FlipDirect = FlipCoords ? (y1 > y2) : (x1 > x2);

  psVector *values = NULL;
  if (!FlipDirect && !FlipCoords) values = psphotLineValuesBresen (image, X1, Y1, X2, Y2, dW, FALSE);
  if ( FlipDirect && !FlipCoords) values = psphotLineValuesBresen (image, X2, Y2, X1, Y1, dW, FALSE);
  if (!FlipDirect &&  FlipCoords) values = psphotLineValuesBresen (image, Y1, X1, Y2, X2, dW, TRUE);
  if ( FlipDirect &&  FlipCoords) values = psphotLineValuesBresen (image, Y2, X2, Y1, X1, dW, TRUE);

  return values;
}

/**
 * use the Bresenham line drawing technique
 * integer-only Bresenham line-draw version which is fast
 */
psVector *psphotLineValuesBresen (psImage *image, int X1, int Y1, int X2, int Y2, int dW, int swapcoords) {

    int X, Y, dX, dY;
    int e, e2;

    psVector *values = psVectorAllocEmpty(100, PS_TYPE_F32);

    dX = X2 - X1;
    dY = Y2 - Y1;

    Y = Y1;
    e = 0;
    for (X = X1; X <= X2; X++) {
        if (X > 0) {
            if (swapcoords) {
                if (X >= image->numRows) continue;
                for (int y = Y - dW; y <= Y + dW; y++) {
                    if (y < 0) continue;
                    if (y >= image->numCols) continue;
                    psVectorAppend(values, image->data.F32[X][y]);
                }
            } else {
                if (X >= image->numCols) continue;
                for (int y = Y - dW; y <= Y + dW; y++) {
                    if (y < 0) continue;
                    if (y >= image->numRows) continue;
                    psVectorAppend(values, image->data.F32[y][X]);
                }
            }
        }
        e += dY;
        e2 = 2 * e;
        if (e2 > dX) {
            Y++;
            e -= dX;
        }
        if (e2 < -dX) {
            Y--;
            e += dX;
        }
    }
    return values;
}


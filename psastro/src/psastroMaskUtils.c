/** @file psastroMaskUpdates.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "I/O failure in psastroMaskUpdate"); \
  psFree (view); \
  return false; \
}

// XXX this is going to be very slow...
pmCell *pmCellInChip (pmChip *chip, float x, float y) {

# if (0)
    int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
    int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
    int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    // XXX fix the binning : currently not selected from concepts
    // int xBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN"); // Binning in x and y
    // int yBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN"); // Binning in x and y
    int xBin = 1;
    int yBin = 1;

    // Position on the cell
    float xCell = PM_CHIP_TO_CELL(xChip, x0Cell, xParityCell, xBin);
    float yCell = PM_CHIP_TO_CELL(yChip, y0Cell, yParityCell, yBin);
# endif

    for (int i = 0; i < chip->cells->n; i++) {

        pmCell *cell = chip->cells->data[i];
        psRegion *region = pmCellExtent (cell);

        if (x < region->x0) goto skip;
        if (x > region->x1) goto skip;
        if (y < region->y0) goto skip;
        if (y > region->y1) goto skip;

        psFree (region);
        return cell;

    skip:
        psFree (region);
    }
    return NULL;
}

/**
 * convert chip coords to cell coords, given known cell
 */
bool pmCellCoordsForChip (float *xCell, float *yCell, pmCell *cell, float xChip, float yChip) {

    int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
    int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
    int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    // XXX fix the binning : currently not selected from concepts
    // int xBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN"); // Binning in x and y
    // int yBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN"); // Binning in x and y
    int xBin = 1;
    int yBin = 1;

    // Position on the cell
    // ((pos)*(binning)*(cellParity) + (cell0))
    // XXX this is probably totally wrong now....
    // ((pos) - (cell0))*(cellParity)/(binning))
    *xCell = (xChip - x0Cell)*xParityCell/xBin;
    *yCell = (yChip - y0Cell)*yParityCell/yBin;

    return true;
}

/**
 * convert chip coords to cell coords, given known cell
 */
bool pmChipCoordsForCell (float *xChip, float *yChip, pmCell *cell, float xCell, float yCell) {

    int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
    int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
    int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    // XXX fix the binning : currently not selected from concepts
    // int xBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN"); // Binning in x and y
    // int yBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN"); // Binning in x and y
    int xBin = 1;
    int yBin = 1;

    // Position on the cell
    // ((pos)*(binning)*(cellParity) + (cell0))
    // XXX this is probably totally wrong now....
    // ((pos) - (cell0))*(cellParity)/(binning))
    *xChip = xCell*xBin*xParityCell + x0Cell;
    *yChip = yCell*yBin*yParityCell + y0Cell;

    return true;
}

bool psastroMaskCircle (psImage *mask, psImageMaskType value, float x0, float y0, float dX, float dY) {

    for (int ix = -dX; ix <= +dX; ix++) {
        int jx = ix + x0;
        if (jx < 0) continue;
        if (jx >= mask->numCols) continue;
        for (int iy = -dY; iy <= +dY; iy++) {
            int jy = iy + y0;
            if (jy < 0) continue;
            if (jy >= mask->numRows) continue;

            double r2 = PS_SQR(ix/dX) + PS_SQR(iy/dY);
            if (r2 > 1.0) continue;

            mask->data.PS_TYPE_IMAGE_MASK_DATA[jy][jx] |= value;
        }
    }
    return true;
}


void psastroMinMaxForShape (float *min, float *max, float y, psEllipseShape shape) {

	// XXX optimize this
	float A = 1.0;
	float B = 2.0 * shape.sxy*y*PS_SQR(shape.sx);
	float C = PS_SQR(y*shape.sx/shape.sy) - PS_SQR(shape.sx);

	float T1 = PS_SQR(B);
	float T2 = 4.0*A*C;
	float R = T1 - T2;
	if (R < 0) R = 0;

	*min =  (-B - sqrt (R)) / (2.0*A);
	*max =  (-B + sqrt (R)) / (2.0*A);
}

bool psastroMaskEllipse (psImage *mask, psImageMaskType value, float x0, float y0, psEllipseAxes axes) {

    psEllipseShape shape = psEllipseAxesToShape (axes);

    // phi is the coordinate along the elliptical path
    // phiMin, phiMax are the points on the path for Ymin and Ymax
    float phiMin = atan2 (-axes.minor*cos(axes.theta), axes.major*sin(axes.theta));
    float phiMax = phiMin + M_PI;

    float Ymin = -axes.major*cos(phiMin)*sin(axes.theta) + axes.minor*sin(phiMin)*cos(axes.theta);
    float Ymax = -axes.major*cos(phiMax)*sin(axes.theta) + axes.minor*sin(phiMax)*cos(axes.theta);
    if (Ymin > Ymax) PS_SWAP (Ymin, Ymax);
  
    for (int iy = Ymin; iy <= Ymax; iy++) {
	int jy = iy + y0;
	if (jy < 0) continue;
	if (jy >= mask->numRows) continue;

	float Xmin, Xmax;
	psastroMinMaxForShape (&Xmin, &Xmax, iy, shape);
	fprintf (stderr, "mask %d : %f -> %f == %f -> %f (%x)\n", jy, Xmin, Xmax, Xmin + x0, Xmax + x0, value);

	for (int ix = Xmin; ix <= Xmax; ix++) {
	    int jx = ix + x0;
	    if (jx < 0) continue;
	    if (jx >= mask->numCols) continue;
	    mask->data.PS_TYPE_IMAGE_MASK_DATA[jy][jx] |= value;
	}
    }
    return true;
}

bool psastroMaskEllipticalAnnulus (psImage *mask, psImageMaskType value, float x0, float y0, psEllipseAxes eInner, psEllipseAxes eOuter) {

    // skip the masking if the outer ellipse is nonsensical
    psEllipseShape sOuter = psEllipseAxesToShape (eOuter);
    if (isnan(sOuter.sx) || isnan(sOuter.sy) || isnan(sOuter.sxy)) return false;

    psEllipseShape sInner = psEllipseAxesToShape (eInner);
    if (isnan(sInner.sx) || isnan(sInner.sy) || isnan(sInner.sxy)) {
	// use a solid ellipse if the inner ellipse is nonsensical
	sInner.sx = 0.1;
	sInner.sy = 0.1;
	sInner.sxy = 0.0;
    }

    // phi is the coordinate along the elliptical path
    // phiMin, phiMax are the points on the path for Ymin and Ymax
    float phiMinInner = atan2 (-eInner.minor*cos(eInner.theta), eInner.major*sin(eInner.theta));
    float phiMaxInner = phiMinInner + M_PI;
    float phiMinOuter = atan2 (-eOuter.minor*cos(eOuter.theta), eOuter.major*sin(eOuter.theta));
    float phiMaxOuter = phiMinOuter + M_PI;

    float YminInner = -eInner.major*cos(phiMinInner)*sin(eInner.theta) + eInner.minor*sin(phiMinInner)*cos(eInner.theta);
    float YmaxInner = -eInner.major*cos(phiMaxInner)*sin(eInner.theta) + eInner.minor*sin(phiMaxInner)*cos(eInner.theta);
    if (YminInner > YmaxInner) PS_SWAP (YminInner, YmaxInner);

    float YminOuter = -eOuter.major*cos(phiMinOuter)*sin(eOuter.theta) + eOuter.minor*sin(phiMinOuter)*cos(eOuter.theta);
    float YmaxOuter = -eOuter.major*cos(phiMaxOuter)*sin(eOuter.theta) + eOuter.minor*sin(phiMaxOuter)*cos(eOuter.theta);
    if (YminOuter > YmaxOuter) PS_SWAP (YminOuter, YmaxOuter);
  
    for (int iy = YminOuter; iy <= YmaxOuter; iy++) {
	int jy = iy + y0;
	if (jy < 0) continue;
	if (jy >= mask->numRows) continue;

	float XminOuter, XmaxOuter;
	psastroMinMaxForShape (&XminOuter, &XmaxOuter, iy, sOuter);

	if ((iy > YmaxInner) || (iy < YminInner)) {
	    for (int ix = XminOuter; ix <= XmaxOuter; ix++) {
		int jx = ix + x0;
		if (jx < 0) continue;
		if (jx >= mask->numCols) continue;
		mask->data.PS_TYPE_IMAGE_MASK_DATA[jy][jx] |= value;
	    }
	} else {
	    float XminInner, XmaxInner;
	    psastroMinMaxForShape (&XminInner, &XmaxInner, iy, sInner);

	    for (int ix = XminOuter; ix <= XminInner; ix++) {
		int jx = ix + x0;
		if (jx < 0) continue;
		if (jx >= mask->numCols) continue;
		mask->data.PS_TYPE_IMAGE_MASK_DATA[jy][jx] |= value;
	    }
	    for (int ix = XmaxInner; ix <= XmaxOuter; ix++) {
		int jx = ix + x0;
		if (jx < 0) continue;
		if (jx >= mask->numCols) continue;
		mask->data.PS_TYPE_IMAGE_MASK_DATA[jy][jx] |= value;
	    }
	}
    }
    return true;
}

bool psastroMaskBox (psImage *mask, psImageMaskType value, float x0, float y0, float dL, float dW, float theta) {

    // draw a series of lines (from -0.5*dW to +0.5*dW) of length dL, starting at x0, y0, angle theta

    float xs = x0;
    float ys = y0;

    float xe = xs + dL*cos(theta);
    float ye = ys + dL*sin(theta);

    psastroMaskLine (mask, value, xs, ys, xe, ye, (int) dW);
    return true;
}

/**
 * identify the quadrant and draw the correct line
 */
void psastroMaskLine (psImage *mask, psImageMaskType value, double x1, double y1, double x2, double y2, int dW) {

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

  if (!FlipDirect && !FlipCoords) psastroMaskLineBresen (mask, value, X1, Y1, X2, Y2, dW, FALSE);
  if ( FlipDirect && !FlipCoords) psastroMaskLineBresen (mask, value, X2, Y2, X1, Y1, dW, FALSE);
  if (!FlipDirect &&  FlipCoords) psastroMaskLineBresen (mask, value, Y1, X1, Y2, X2, dW, TRUE);
  if ( FlipDirect &&  FlipCoords) psastroMaskLineBresen (mask, value, Y2, X2, Y1, X1, dW, TRUE);

  return;
}

/**
 * use the Bresenham line drawing technique
 * integer-only Bresenham line-draw version which is fast
 */
void psastroMaskLineBresen (psImage *mask, psImageMaskType value, int X1, int Y1, int X2, int Y2, int dW, int swapcoords) {

    int X, Y, dX, dY;
    int e, e2;

    dX = X2 - X1;
    dY = Y2 - Y1;

    Y = Y1;
    e = 0;
    for (X = X1; X <= X2; X++) {
        if (X > 0) {
            if (swapcoords) {
                if (X >= mask->numRows) continue;
                for (int y = Y - dW; y <= Y + dW; y++) {
                    if (y < 0) continue;
                    if (y >= mask->numCols) continue;
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[X][y] |= value;
                }
            } else {
                if (X >= mask->numCols) continue;
                for (int y = Y - dW; y <= Y + dW; y++) {
                    if (y < 0) continue;
                    if (y >= mask->numRows) continue;
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][X] |= value;
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
    return;
}

void psastroMaskRectangle (psImage *mask, psImageMaskType value, int x0, int y0, int x1, int y1) {

    int xs = PS_MAX (0, PS_MIN (mask->numCols, PS_MIN (x0, x1)));
    int xe = PS_MAX (0, PS_MIN (mask->numCols, PS_MAX (x0, x1)));
    int ys = PS_MAX (0, PS_MIN (mask->numRows, PS_MIN (y0, y1)));
    int ye = PS_MAX (0, PS_MIN (mask->numRows, PS_MAX (y0, y1)));

    for (int iy = ys; iy < ye; iy++) {
        for (int ix = xs; ix < xe; ix++) {
            mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= value;
        }
    }
}



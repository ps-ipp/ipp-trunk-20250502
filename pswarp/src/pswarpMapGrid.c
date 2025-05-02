/** @file pswarpMapGrid.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

FILE *fout = NULL;

/**
 * pswarpMapGridFromImage builds a set (a grid) of locally-linear maps which convert the source
 * coordinates (src) to destination coordinates (dest).  we construct a grid with superpixel
 * spacing of nXpix, nYpix.  The transformation for each grid cell is valid for the superpixel.
 * The grid over-fills the source image so every source image pixel is guaranteed to have a map.
 */
pswarpMapGrid *pswarpMapGridFromImage (pmReadout *dest, pmReadout *src, int nXpix, int nYpix) {

    int i, ni;
    int j, nj;

    // start counting from the center of the superpixels
    double xMin = 0.5*nXpix;
    double yMin = 0.5*nYpix;

    // the map is defined for coordinates in the image parent frame.
    int Nx = src->image->numCols + src->image->col0;
    int Ny = src->image->numRows + src->image->row0;

    // always allocate an extra superpixel to handle spillover
    int nXpts = (int)(Nx / nXpix) + 1;
    int nYpts = (int)(Ny / nYpix) + 1;

    // create the grid of maps
    pswarpMapGrid *grid = pswarpMapGridAlloc (nXpts, nYpts);

    // measure the map for the center of each superpixel
    for (ni = 0, i = xMin; ni < nXpts; i += nXpix, ni++) {
        for (nj = 0, j = yMin; nj < nYpts; j += nYpix, nj++) {
//	  pswarpMapSetLocalModel (grid->maps[ni][nj], dest, src, i, j, nXpix, nYpix);
            pswarpMapSetLocalModel (grid->maps[ni][nj], dest, src, i, j);
        }
    }

    if (0) {
	// test the transformation sky<->chip for chip in smf:
	if (fout == NULL) { fout = fopen ("map.grid.txt", "w");	}

	pmCell *cell = dest->parent;
	pmChip *chip = cell->parent;
	pmFPA *fpa = chip->parent;

	int NxChip = dest->image->numCols;
	int NyChip = dest->image->numRows;

	// XXX save these as static for speed?
	psPlane *CH0 = psPlaneAlloc();
	psPlane *CH1 = psPlaneAlloc();
	
	psPlane *FP0 = psPlaneAlloc();
	psPlane *TP0 = psPlaneAlloc();

	psPlane *FP1 = psPlaneAlloc();
	psPlane *TP1 = psPlaneAlloc();

	psSphere *sky = psSphereAlloc();

	// measure the map for the center of each superpixel
	for (i = 0; i < NxChip; i += 100) {
	    for (j = 0; j < NyChip; j += 100) {
		
		CH0->x = i;
		CH0->y = j;
		psPlaneTransformApply(FP0, chip->toFPA, CH0);
		psPlaneTransformApply (TP0, fpa->toTPA, FP0);
		psDeproject (sky, TP0, fpa->toSky);
		psProject (TP1, sky, fpa->toSky);
		psPlaneTransformApply (FP1, fpa->fromTPA, TP1);
		psPlaneTransformApply (CH1, chip->fromFPA, FP1);

		fprintf (fout, "%f %f > %f %f > %f %f > %f %f | ", CH0->x, CH0->y, FP0->x, FP0->y, TP0->x, TP0->y, sky->r*180/M_PI, sky->d*180/M_PI);
		fprintf (fout, "%f %f < %f %f < %f %f \n", TP1->x, TP1->y, FP1->x, FP1->y, CH1->x, CH1->y);

	    }
	}
	fclose (fout);

	psFree (CH0);
	psFree (FP0);
	psFree (TP0);

	psFree (CH1);
	psFree (FP1);
	psFree (TP1);

	psFree (sky);
    }

    grid->nXpix = nXpix;
    grid->nYpix = nYpix;
    grid->xMin = xMin;
    grid->yMin = yMin;
    return grid;
}

/**
 * set the grid coordinate (gridX,gridY) for the given source image coordinate (ix,iy)
 * XXX return true if the result is on the src image, false otherwise (???)
 */
bool pswarpMapGridSetGrid (pswarpMapGrid *grid, int ix, int iy, int *gridX, int *gridY) {

    *gridX = 0.5 + (ix - grid->xMin) / (double) grid->nXpix;
    *gridY = 0.5 + (iy - grid->yMin) / (double) grid->nYpix;

    return true;
}

/**
 * given the specified grid coordinate (gridX, gridY), return the min and max coordinates for the tile
 */
bool pswarpMapGridCoordRange (pswarpMapGrid *grid, int gridX, int gridY, psPlane *min, psPlane *max) {

    min->x = (gridX - 0.5)*grid->nXpix + grid->xMin;
    min->y = (gridY - 0.5)*grid->nYpix + grid->yMin;

    max->x = min->x + grid->nXpix;
    max->y = min->y + grid->nYpix;

    return true;
}

/**
 * given the specified grid coordinate (gridX), return the x-coordinate for the source image
 * corresponding to the next grid cell
 */
int pswarpMapGridNextGrid_X (pswarpMapGrid *grid, int gridX) {

    int nextX = (gridX + 0.5)*grid->nXpix + grid->xMin;
    return nextX;
}

/**
 * given the specified grid coordinate (gridY), return the y-coordinate for the source image
 * corresponding to the next grid cell
 */
int pswarpMapGridNextGrid_Y (pswarpMapGrid *grid, int gridY) {

    int nextY = (gridY + 0.5)*grid->nYpix + grid->yMin;
    return nextY;
}

/**
 * measure the max error accumulated in applying one grid point to its neighbors
 * XXX double-check this
 */
double pswarpMapGridMaxError (pswarpMapGrid *grid) {

    double xRaw, yRaw;
    double xRef, yRef;
    double maxError = 0;

    for (int i = 0; i < grid->nXpts - 1; i++) {
        for (int j = 0; j < grid->nYpts - 1; j++) {

            // measure the output coordinates for the next grid position using the current grid map
            // compare with the coordinates measured using the next grid map
            pswarpMapApply (&xRaw, &yRaw, grid->maps[i][j], grid->maps[i][j]->xo + grid->nXpix, grid->maps[i][j]->yo);
            pswarpMapApply (&xRef, &yRef, grid->maps[i+1][j], grid->maps[i][j]->xo + grid->nXpix, grid->maps[i][j]->yo);

            double posError = hypot (xRaw-xRef, yRaw-yRef);
            maxError = PS_MAX (maxError, posError);
        }
    }
    return maxError;
}

/**
 * given the source coordinate (inX,inY), return the destination coordinate (outX,outY)
 */
bool pswarpMapApply (double *outX, double *outY, pswarpMap *map, double inX, double inY) {

    *outX = map->Xo + map->Xx*inX + map->Xy*inY;
    *outY = map->Yo + map->Yx*inX + map->Yy*inY;

    return true;
}

/**
 * determine the (linear) map for the given pixel (ix,iy) from source image (src) to the destination image (dest)
 * pixel is in src coords. input and output pixel coordinates are in the parent frame of the image (Note that the
 * astrometric transformations are supplied for the parent image coordinate frame.
 */
// bool pswarpMapSetLocalModel (pswarpMap *map, pmReadout *dest, pmReadout *src, int ix, int iy, int dX, int dY) {
bool pswarpMapSetLocalModel (pswarpMap *map, pmReadout *dest, pmReadout *src, int ix, int iy) {

    pmCell *cell = NULL;

    cell = src->parent;
    pmChip *chipSrc = cell->parent;
    pmFPA *fpaSrc = chipSrc->parent;

    cell = dest->parent;
    pmChip *chipDest = cell->parent;
    pmFPA *fpaDest = chipDest->parent;

    // XXX save these as static for speed?
    psPlane *offset = psPlaneAlloc();

    psPlane *FP = psPlaneAlloc();
    psPlane *TP = psPlaneAlloc();
    psSphere *sky = psSphereAlloc();

    psPlane *V00 = psPlaneAlloc();
    psPlane *V10 = psPlaneAlloc();
    psPlane *V01 = psPlaneAlloc();

    // XXX need to include readout->cell->chip offsets
    // XXX note that 'dest' is the SMF and 'src' is the skycell CMF
    // this is the lower accuracy direction...

    /** V(0,0) position */
    offset->x = ix;
    offset->y = iy;
    psPlaneTransformApply(FP, chipSrc->toFPA, offset);
    psPlaneTransformApply (TP, fpaSrc->toTPA, FP);
    psDeproject (sky, TP, fpaSrc->toSky);
    psProject (TP, sky, fpaDest->toSky);
    psPlaneTransformApply (FP, fpaDest->fromTPA, TP);
    psPlaneTransformApply (V00, chipDest->fromFPA, FP);

    /** V(1,0) position */
//  offset->x = ix + dX;
    offset->x = ix + 1;
    offset->y = iy;
    psPlaneTransformApply(FP, chipSrc->toFPA, offset);
    psPlaneTransformApply (TP, fpaSrc->toTPA, FP);
    psDeproject (sky, TP, fpaSrc->toSky);
    psProject (TP, sky, fpaDest->toSky);
    psPlaneTransformApply (FP, fpaDest->fromTPA, TP);
    psPlaneTransformApply (V10, chipDest->fromFPA, FP);

    /** V(0,1) position */
    offset->x = ix;
    offset->y = iy + 1;
//  offset->y = iy + dY;
    psPlaneTransformApply(FP, chipSrc->toFPA, offset);
    psPlaneTransformApply (TP, fpaSrc->toTPA, FP);
    psDeproject (sky, TP, fpaSrc->toSky);
    psProject (TP, sky, fpaDest->toSky);
    psPlaneTransformApply (FP, fpaDest->fromTPA, TP);
    psPlaneTransformApply (V01, chipDest->fromFPA, FP);

    map->Xx = V10->x - V00->x;
    map->Xy = V01->x - V00->x;
    map->Xo = V00->x - map->Xx*ix - map->Xy*iy;

    map->Yx = V10->y - V00->y;
    map->Yy = V01->y - V00->y;
    map->Yo = V00->y - map->Yx*ix - map->Yy*iy;

    map->xo = ix;
    map->yo = iy;
    
    psFree (offset);
    psFree (FP);
    psFree (TP);
    psFree (sky);

    psFree (V00);
    psFree (V10);
    psFree (V01);

    return true;
}

static void pswarpMapFree (pswarpMap *map) {
  return;
}

pswarpMap *pswarpMapAlloc(void) {

  pswarpMap *map = (pswarpMap *) psAlloc (sizeof(pswarpMap));
  psMemSetDeallocator(map, (psFreeFunc) pswarpMapFree);

  return map;
}

static void pswarpMapGridFree (pswarpMapGrid *grid) {

    if (grid == NULL) return;
    if (grid->maps == NULL) return;

    for (int i = 0; i < grid->nXpts; i++) {
        for (int j = 0; j < grid->nYpts; j++) {
            psFree (grid->maps[i][j]);
        }
        psFree (grid->maps[i]);
    }
    psFree (grid->maps);
    return;
}

pswarpMapGrid *pswarpMapGridAlloc (int nXpts, int nYpts) {

  pswarpMapGrid *grid = (pswarpMapGrid *) psAlloc (sizeof(pswarpMapGrid));
  psMemSetDeallocator(grid, (psFreeFunc) pswarpMapGridFree);

  grid->maps = psAlloc (nXpts*sizeof(void **));
  for (int i = 0; i < nXpts; i++) {
      grid->maps[i] = psAlloc (nYpts*sizeof(void *));
      for (int j = 0; j < nYpts; j++) {
          grid->maps[i][j] = pswarpMapAlloc();
      }
  }
  grid->nXpts = nXpts;
  grid->nYpts = nYpts;

  grid->nXpix = 0;
  grid->nYpix = 0;

  return grid;
}


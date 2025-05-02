/** These utility functions help threading source analysis on readouts
 *  @author Eugene Magnier, IfA
 *  @date June 05, 2013
 */

/* Include Files  */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAMaskWeight.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourcePhotometry.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"

// the strategy here is to divide the image into 2x2 blocks of cells and cycle through
// the four discontiguous sets of cells, threading all within a set and blocking between
// sets 

// we divide the image region into 2*2 blocks of size Nx*Ny, the image will have 
// Cx*Cy blocks so that (2Nx)Cx = numCols, (2Ny)Cy = numRows.  We want to choose Cx and
// Cy so that (2Nx)Cx * (2Ny)Cy = 4 * NFILL * nThreads -- each of the four sets of cells
// has enough cells to allow NFILL cells for each thread (to better distribute heavy and
// light load cells
    
// the array runs from readout->image->col0 to readout->image->col0 + readout->image->numCols 

// we save these for threaded analysis runs
static int Xo = 0;
static int Yo = 0;
static int Nx = 1;
static int Ny = 1;

bool pmReadoutChooseCellSizes (int *Cx, int *Cy, pmReadout *readout, int nThreads) {

    int nCells = nThreads * 2*2; // number of cells in a single set
    int C = sqrt(nCells) + 0.5;
    
    // we need to assign Cx and Cy based on the dimensionality of the image
    // crude way to find most evenly balanced factors of nCells:
    for (int i = C; i >= 1; i--) {
	int C1 = nCells / C;
	int C2 = nCells / C1;
	if (C1*C2 != nCells) continue;

	if (readout->image->numRows > readout->image->numCols) {
	    *Cx = PS_MAX (C1, C2);
	    *Cy = PS_MIN (C1, C2);
	} else {
	    *Cx = PS_MAX (C1, C2);
	    *Cy = PS_MIN (C1, C2);
	}

	Xo = readout->image->col0;
	Yo = readout->image->row0;
	Nx = readout->image->numCols / (*Cx*2);
	Ny = readout->image->numRows / (*Cy*2);

	return true;
    }
    *Cx = 1;
    *Cy = 1; 

    Xo = readout->image->col0;
    Yo = readout->image->row0;
    Nx = readout->image->numCols / (*Cx*2);
    Ny = readout->image->numRows / (*Cy*2);

    return true;
}

bool pmReadoutCoordToCell (int *group, int *cell, float x, float y, int Cx, int Cy) {
  
    // XXX need to handle edges
    int ix = (x - Xo)/(2*Nx);
    ix = PS_MAX (0, PS_MIN (ix, Cx - 1));

    int iy = (y - Yo)/(2*Ny);
    iy = PS_MAX (0, PS_MIN (iy, Cy - 1));

    int jx = (((int)(x - Xo))%(2*Nx))/Nx;
    jx = PS_MAX (0, PS_MIN (jx, Nx - 1));

    int jy = (((int)(y - Yo))%(2*Ny))/Ny;
    jy = PS_MAX (0, PS_MIN (jy, Ny - 1));

    *group = jx + 2*jy;
    *cell  = ix + Cx*iy;

    return true;
}

// we have 2x2 * Cx*Cy cells for the image.  we need a function to convert an x,y
// coordinate pair into the index for these cells

// first, how shall we number them?  is there one index for all cells, or one set of
// indices for each of the 2x2 cell groups?  

psArray *pmReadoutAssignSourcesToCells (int Cx, int Cy, psArray *sources) {

    psArray *cellGroups = psArrayAlloc (4);
    for (int i = 0; i < cellGroups->n; i++) {
	psArray *cells = psArrayAlloc (Cx*Cy);
	cellGroups->data[i] = cells;
	for (int j = 0; j < cells->n; j++) {
	    psArray *cellSources = psArrayAllocEmpty (50);
	    cells->data[j] = cellSources;
	}
    }

    for (int i = 0; i < sources->n; i++) {
    
	int group = 0;
	int cell = 0;

	pmSource *source = sources->data[i];

	pmReadoutCoordToCell (&group, &cell, source->peak->xf, source->peak->yf, Cx, Cy);
       
	psArray *cells = cellGroups->data[group];
	psArray *cellSources = cells->data[cell];
	
	psArrayAdd (cellSources, 100, source);
    }
	
    return cellGroups;
}

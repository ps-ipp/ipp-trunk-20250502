/** @file psastroFindChipGPC.c
 *
 *  @brief calculate chip position for the given FPA coords for GPC-like cameras
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

/* this code assumes the chips are roughly square and arranged in a grid.
   this allows us to calculate a 2D index from the (xFPA, yFPA) coordinate
   to the nearest chip center.  

*/

# include "psastroInternal.h"

static psVector *chipXmin = NULL;
static psVector *chipXmax = NULL;
static psVector *chipYmin = NULL;
static psVector *chipYmax = NULL;

static psImage *chipCenters = NULL;

// XXX these should not be defined as global (only used in psastroChipBoundsGPC)
// static double XmaxFPA = -FLT_MAX;
// static double YmaxFPA = -FLT_MAX;

// these are used in gsastroFindChipGPC to find the chip match:
static double XminFPA   = +FLT_MAX;
static double YminFPA   = +FLT_MAX;
static double xInvScale = NAN;
static double yInvScale = NAN;

bool psastroChipBoundsGPC (pmFPA *fpa) {

    chipXmin = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);
    chipXmax = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);
    chipYmin = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);
    chipYmax = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);

    // XXX these should not be declared local (override the global values)
    // double XminFPA = +FLT_MAX;
    // double YminFPA = +FLT_MAX;

    double XmaxFPA = -FLT_MAX;
    double YmaxFPA = -FLT_MAX;

    // this loop selects the matched stars for all chips
    for (int i = 0; i < fpa->chips->n; i++) {

      pmChip *chip = fpa->chips->data[i];
      if (!chip->process || !chip->file_exists) { continue; }
      if (!chip->fromFPA) { continue; }

      // determine RA,DEC of 4 corners, use to find RA_MIN,MAX, DEC_MIN,MAX
      psRegion *region = pmChipPixels (chip);
      psPlane ptCH[4], ptFP;

      ptCH[0].x = region->x0;
      ptCH[0].y = region->y0;
      ptCH[1].x = region->x1;
      ptCH[1].y = region->y0;
      ptCH[2].x = region->x1;
      ptCH[2].y = region->y1;
      ptCH[3].x = region->x0;
      ptCH[3].y = region->y1;
      psFree (region);
      
      double Xmin = +FLT_MAX;
      double Xmax = -FLT_MAX;
      double Ymin = +FLT_MAX;
      double Ymax = -FLT_MAX;

      for (int j = 0; j < 4; j++) {
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH[j]);
	Xmin = PS_MIN (ptFP.x, Xmin);
	Xmax = PS_MAX (ptFP.x, Xmax);
	Ymin = PS_MIN (ptFP.y, Ymin);
	Ymax = PS_MAX (ptFP.y, Ymax);
      }

      // fpa-range for the given chip
      chipXmin->data.F32[i] = Xmin;
      chipXmax->data.F32[i] = Xmax;
      chipYmin->data.F32[i] = Ymin;
      chipYmax->data.F32[i] = Ymax;

      XminFPA = PS_MIN (XminFPA, Xmin);
      XmaxFPA = PS_MAX (XmaxFPA, Xmax);
      YminFPA = PS_MIN (YminFPA, Ymin);
      YmaxFPA = PS_MAX (YmaxFPA, Ymax);
    }

    chipCenters = psImageAlloc (8, 8, PS_TYPE_U8);
    psImageInit (chipCenters, -1);

    double xRangeFPA = XmaxFPA - XmaxFPA;
    xInvScale = 8 / xRangeFPA;

    double yRangeFPA = YmaxFPA - YmaxFPA;
    yInvScale = 8 / yRangeFPA;

    // (XminFPA) to (XminFPA + xScaleFPA) : bin = 0
    // (XminFPA + xScaleFPA) to (XminFPA + 2xScaleFPA) : bin = 0
    
    // identify the bin in chipCenters for each active chip
    // all others are left as -1 
    for (int i = 0; i < fpa->chips->n; i++) {

      pmChip *chip = fpa->chips->data[i];
      if (!chip->process || !chip->file_exists) { continue; }
      if (!chip->fromFPA) { continue; }

      // determine RA,DEC of 4 corners, use to find RA_MIN,MAX, DEC_MIN,MAX
      psRegion *region = pmChipPixels (chip);
      psPlane ptCH, ptFP;

      ptCH.x = 0.5*(region->x0 + region->x1);
      ptCH.y = 0.5*(region->y0 + region->y1);
      psFree (region);

      psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);

      // find the bins, saturate at (0,7)
      int xBin = MIN(MAX((ptFP.x - XminFPA) * xInvScale, 0), 7);
      int yBin = MIN(MAX((ptFP.y - YminFPA) * yInvScale, 0), 7);
    
      // the current value should be -1
      psAssert (chipCenters->data.U8[yBin][xBin] == -1, "chip collision?");
      chipCenters->data.U8[yBin][xBin] = i;
    }
    return true;
}

pmChip *psastroCheckChip (double *xChip, double *yChip, pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    // is this coordinate on this chip?
    if (xFPA <  chipXmin->data.F32[nChip]) return NULL;
    if (xFPA >= chipXmax->data.F32[nChip]) return NULL;
    if (yFPA <  chipYmin->data.F32[nChip]) return NULL;
    if (yFPA >= chipYmax->data.F32[nChip]) return NULL;
	
    pmChip *chip = fpa->chips->data[nChip];
    psRegion *region = pmChipPixels (chip);
    
    psPlane ptCH, ptFP;
    ptFP.x = xFPA;
    ptFP.y = yFPA;
    psPlaneTransformApply (&ptCH, chip->fromFPA, &ptFP);
    
    // is it really on this chip?
    if (ptCH.x <  region->x0) { psFree (region); return NULL; }
    if (ptCH.x >= region->x1) { psFree (region); return NULL; }
    if (ptCH.y <  region->y0) { psFree (region); return NULL; }
    if (ptCH.y >= region->y1) { psFree (region); return NULL; }
    psFree (region);
    
    *xChip = ptCH.x;
    *yChip = ptCH.y;
    return chip;
}

pmChip *psastroFindChipGPC (double *xChip, double *yChip, pmFPA *fpa, double xFPA, double yFPA) {

    *xChip = NAN;
    *yChip = NAN;

    if (!chipXmin) {
	psastroChipBounds (fpa);
    }

    int xBin = MIN(MAX((xFPA - XminFPA) * xInvScale, 0), 7);
    int yBin = MIN(MAX((yFPA - YminFPA) * yInvScale, 0), 7);

    int nChip = chipCenters->data.U8[yBin][xBin];

    // this gets us to the relevant chip immediately

    // However, we need to check if the coordinate is actually in the chip.
    // if not, we should also check the nearest chip in X and Y (and maybe both)

    // we also need to handle the case where the result is not a valid chip
    if (nChip == -1) {
      psAbort ("fix me");
    }

    pmChip *chip = psastroCheckChip (xChip, yChip, fpa, nChip, xFPA, yFPA);
    if (chip) return chip;

    // need to try a neighbor

    double xFrac = (xFPA - XminFPA) * xInvScale - xBin - 0.5; // xFrac ranges from -0.5 to +0.5 
    double yFrac = (yFPA - YminFPA) * yInvScale - yBin - 0.5; // yFrac ranges from -0.5 to +0.5 

    int xBinNew = -1;
    int yBinNew = -1;
    
    // which edge is closer, X or Y?
    if (fabs(xFrac) > fabs(yFrac)) {
	if (xFrac > 0.0) {
	    xBinNew = MIN(xBin + 1, 7);
	} else {
	    xBinNew = MAX(xBin - 1, 0);
	}
	int nChipNew = chipCenters->data.U8[yBin][xBinNew];
	pmChip *chipNew = psastroCheckChip (xChip, yChip, fpa, nChipNew, xFPA, yFPA);
	if (chipNew) return chipNew; 
    } else {
	if (yFrac > 0.0) {
	    yBinNew = MIN(yBin + 1, 7);
	} else {
	    yBinNew = MAX(yBin - 1, 0);
	}
	int nChipNew = chipCenters->data.U8[yBinNew][xBin];
	pmChip *chipNew = psastroCheckChip (xChip, yChip, fpa, nChipNew, xFPA, yFPA);
	if (chipNew) return chipNew; 
    }

    // that did not work, now try the other dimension
    if (fabs(xFrac) <= fabs(yFrac)) {
	if (xFrac > 0.0) {
	    xBinNew = MIN(xBin + 1, 7);
	} else {
	    xBinNew = MAX(xBin - 1, 0);
	}
	int nChipNew = chipCenters->data.U8[yBin][xBinNew];
	pmChip *chipNew = psastroCheckChip (xChip, yChip, fpa, nChipNew, xFPA, yFPA);
	if (chipNew) return chipNew; 
    } else {
	if (yFrac > 0.0) {
	    yBinNew = MIN(yBin + 1, 7);
	} else {
	    yBinNew = MAX(yBin - 1, 0);
	}
	int nChipNew = chipCenters->data.U8[yBinNew][xBin];
	pmChip *chipNew = psastroCheckChip (xChip, yChip, fpa, nChipNew, xFPA, yFPA);
	if (chipNew) return chipNew; 
    }

    // that still did not work, now shift in both dimensions
    if (xFrac > 0.0) {
	xBinNew = MIN(xBin + 1, 7);
    } else {
	xBinNew = MAX(xBin - 1, 0);
    }
    if (yFrac > 0.0) {
	yBinNew = MIN(yBin + 1, 7);
    } else {
	yBinNew = MAX(yBin - 1, 0);
    }
    int nChipNew = chipCenters->data.U8[yBinNew][xBinNew];
    pmChip *chipNew = psastroCheckChip (xChip, yChip, fpa, nChipNew, xFPA, yFPA);
    if (chipNew) return chipNew; 
    
    // XXX give up or use the chip-by-chip method?
    return NULL;
}

bool psastroExtractFreeChipBoundsGPC () {
  
  psFree (chipXmin);
  psFree (chipXmax);
  psFree (chipYmin);
  psFree (chipYmax);
  return true;
}

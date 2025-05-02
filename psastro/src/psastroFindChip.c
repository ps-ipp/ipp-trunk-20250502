/** @file psastroFindChip.c
 *
 *  @brief calculate chip position for the given FPA coords
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

static psVector *chipXmin = NULL;
static psVector *chipXmax = NULL;
static psVector *chipYmin = NULL;
static psVector *chipYmax = NULL;

bool psastroChipBounds (pmFPA *fpa) {

    chipXmin = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);
    chipXmax = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);
    chipYmin = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);
    chipYmax = psVectorAlloc (fpa->chips->n, PS_TYPE_F32);

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
    }

    return true;
}

pmChip *psastroFindChip (double *xChip, double *yChip, pmFPA *fpa, double xFPA, double yFPA) {

    *xChip = NAN;
    *yChip = NAN;

    if (!chipXmin) {
	psastroChipBounds (fpa);
    }

    for (int i = 0; i < fpa->chips->n; i++) {

	if (xFPA <  chipXmin->data.F32[i]) continue;
	if (xFPA >= chipXmax->data.F32[i]) continue;
	if (yFPA <  chipYmin->data.F32[i]) continue;
	if (yFPA >= chipYmax->data.F32[i]) continue;

	pmChip *chip = fpa->chips->data[i];
	psRegion *region = pmChipPixels (chip);

	psPlane ptCH, ptFP;
	ptFP.x = xFPA;
	ptFP.y = yFPA;
	psPlaneTransformApply (&ptCH, chip->fromFPA, &ptFP);

	if (ptCH.x <  region->x0) goto next_chip;
	if (ptCH.x >= region->x1) goto next_chip;
	if (ptCH.y <  region->y0) goto next_chip;
	if (ptCH.y >= region->y1) goto next_chip;
	psFree (region);

	*xChip = ptCH.x;
	*yChip = ptCH.y;
	return chip;

    next_chip:
	psFree (region);
    }

    return NULL;
}

// identify chips which land on this column (FP coords)
bool psastroFindChipInXrange (pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    if (!chipXmin || !chipXmax) {
	psAbort ("chip bounds not set");
    }

    if (xFPA <  chipXmin->data.F32[nChip]) return false;
    if (xFPA >= chipXmax->data.F32[nChip]) return false;
    return true;
}

// identify chips which land on this row (FP coords)
bool psastroFindChipInYrange (pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    if (!chipYmin || !chipYmax) {
	psAbort ("chip bounds not set");
    }

    if (yFPA <  chipYmin->data.F32[nChip]) return false;
    if (yFPA >= chipYmax->data.F32[nChip]) return false;
    return true;
}

// return the FPA coordinates of the Y edges of the chip
bool psastroFindChipYedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip) {

    *yFPAs = chipYmin->data.F32[nChip];
    *yFPAe = chipYmax->data.F32[nChip];
    return true;
}

// return the FPA coordinates of the X edges of the chip
bool psastroFindChipXedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip) {

    *yFPAs = chipXmin->data.F32[nChip];
    *yFPAe = chipXmax->data.F32[nChip];
    return true;
}

// convert FPA to Chip coordinates
bool psastroFPAtoChip (double *xChip, double *yChip, pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    pmChip *chip = fpa->chips->data[nChip];

    psPlane ptCH, ptFP;
    ptFP.x = xFPA;
    ptFP.y = yFPA;
    psPlaneTransformApply (&ptCH, chip->fromFPA, &ptFP);

    *xChip = ptCH.x;
    *yChip = ptCH.y;
    return true;
}

bool psastroExtractFreeChipBounds () {
  
  psFree (chipXmin);
  psFree (chipXmax);
  psFree (chipYmin);
  psFree (chipYmax);
  return true;
}

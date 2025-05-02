/** @file  pmSourceIO_Ghosts.c
 *
 *  @author TdB, IfA
 *
 *  @version $Revision: 1.0 $ $Name: not supported by cvs2svn $
 *  @date $Date: 20020-03-30  $
 *
 *  Copyright 2020 University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"
#include "pmErrorCodes.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"

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
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"

#include "pmSourceIO.h"

#include "pmAstrometryObjects.h"
#include "pmAstrometryWCS.h"

# include "pmSourceInternal.h"
static psVector *chipXmin = NULL;
static psVector *chipXmax = NULL;
static psVector *chipYmin = NULL;
static psVector *chipYmax = NULL;

# define ESCAPE(MSG) { \
  psLogMsg ("psastro", PS_LOG_INFO, MSG); \
  return false; }

# define GET_2D_POLY(NAME,OUT) \
    md = psMetadataLookupMetadata (&status, ghostModel, NAME); \
    if (!md) { \
	psError(PM_ERR_CONFIG, true, "Missing %s in model file %s", NAME, ghostFile); \
	goto escape; \
    } \
    OUT = psPolynomial2DfromMetadata(md); \
    if (!OUT) { \
	psError(PM_ERR_CONFIG, true, "Trouble interpretting %s in model file %s", NAME, ghostFile); \
	goto escape; \
    }

# define GET_1D_POLY(NAME,OUT) \
    md = psMetadataLookupMetadata (&status, ghostModel, NAME); \
    if (!md) { \
	psError(PM_ERR_CONFIG, true, "Missing %s in model file %s", NAME, ghostFile); \
	goto escape; \
    } \
    OUT = psPolynomial1DfromMetadata(md);	\
    if (!OUT) { \
	psError(PM_ERR_CONFIG, true, "Trouble interpretting %s in model file %s", NAME, ghostFile); \
	goto escape; \
    }

/**
 * calculate ghost FPA and Chip positions for the stars loaded on the FPA
 */


bool pmSourceIO_WriteGhosts (psFits *fits, pmFPA *fpa, pmConfig *config) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float zeropt, exptime, MAX_MAG;
    psMetadata *md = NULL;
    psPolynomial2D *centerX = NULL;
    psPolynomial2D *centerY = NULL;
    psPolynomial1D *mirrorRad = NULL;
    psPolynomial1D *outerMajor = NULL;
    psPolynomial1D *outerMinor = NULL;
    psPolynomial1D *innerMajor = NULL;
    psPolynomial1D *innerMinor = NULL;
    psMetadata *ghostModel = NULL;

    psLogMsg ("psastro", PS_LOG_INFO, "writing ghost positions");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PM_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    bool REFSTAR_MASK_GHOST = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_GHOST");
    if (!REFSTAR_MASK_GHOST) return true;
    bool GHOST_OUTPUT = psMetadataLookupBool(&status, recipe, "PSASTRO.SAVE.GHOSTS");
    if (!GHOST_OUTPUT) return true;

    char *ghostFile = psMetadataLookupStr (&status, recipe, "GHOST_MODEL");
    if (!strcasecmp(ghostFile, "NONE")) return true;

    if (!pmConfigFileRead (&ghostModel, ghostFile, "GHOST MODEL")) {
	psError(PM_ERR_CONFIG, true, "Trouble loading ghost model");
        return false;
    }

    pmFPAview *view = pmFPAviewAlloc (0);

    // We need to check whether we are dealing with an old style ghost_model, or a new style model. Check if the mirror_rad polynomial exists
    float mirCheck = 0;
    md = psMetadataLookupMetadata (&status, ghostModel, "GHOST.MIRROR.RAD"); 
    if (!md) { psLogMsg ("psastro", PS_LOG_INFO, "No ghost mirror_rad polynomial found. Assuming old-style ghost masking"); } 
    if (md) {
        GET_1D_POLY ("GHOST.MIRROR.RAD", mirrorRad);
        mirCheck = 1;
    }  

    GET_2D_POLY ("GHOST.CENTER.X", centerX);
    GET_2D_POLY ("GHOST.CENTER.Y", centerY);

    GET_1D_POLY ("GHOST.OUTER.MAJOR", outerMajor);
    GET_1D_POLY ("GHOST.OUTER.MINOR", outerMinor);
    GET_1D_POLY ("GHOST.INNER.MAJOR", innerMajor);
    GET_1D_POLY ("GHOST.INNER.MINOR", innerMinor);

    // select the input astrometry data (also carries the refstars)
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!astrom) {
        psError(PM_ERR_CONFIG, true, "Can't find input data");
	goto escape;
    }
    pmFPA *fpa_ast = astrom->fpa;

    // really error-out here?  or just skip?
    if (!pmSourceZeroPointFromRecipeGlint (&zeropt, &exptime, &MAX_MAG,NULL, fpa_ast, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
	goto escape;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    MAX_MAG += MagOffset;

    psArray *table = psMetadataLookupPtr (&status, fpa->analysis, "GHOST_POSITIONS");
    psMemIncrRefCounter (table);
    if (!table) {
        table = psArrayAllocEmpty (0x1000);

 	// this loop selects the matched stars for all chips
 	while ((chip = pmFPAviewNextChip (view, fpa_ast, 1)) != NULL) {
 	    psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
 	    if (!chip->process || !chip->file_exists) { continue; }
 	    if (!chip->fromFPA) { continue; }

 	    while ((cell = pmFPAviewNextCell (view, fpa_ast, 1)) != NULL) {
 		psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
 		if (!cell->process || !cell->file_exists) { continue; }

 		// process each of the readouts
 		while ((readout = pmFPAviewNextReadout (view, fpa_ast, 1)) != NULL) {
 		    if (! readout->data_exists) { continue; }

 		    // select the raw objects for this readout (loaded in psastroChooseRefstars.c)
 		    // XXX : note that we place limits on the refstar sample in psastroChooseRefstars.c:
 		    // 1) on chip and 2) < PSASTRO.MAX.NREF. magnitude limits and clump exclusion are only 
 		    psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
 		    if (refstars == NULL) { continue; }

 		    // identify the bright stars of interest
 		    for (int i = 0; i < refstars->n; i++) {
 			pmAstromObj *ref = refstars->data[i];
 			if (ref->Mag > MAX_MAG) continue;

 			pmSourceGhost *ghost = pmSourceGhostAlloc ();
 			ghost->srcFP->x = ref->FP->x; 
 			ghost->srcFP->y = ref->FP->y;

                        //TdB: first mirror the reference star positions (around the 0,0 pixel) using the radial offset coefficients and the ghost/star angle
		        double rSrc = hypot (ref->FP->x, ref->FP->y);
     	                double theta0 = atan2(ref->FP->y,ref->FP->x);

			// EAM: XXX we should just use the existence of mirrorRad (!= NULL) instead of carrying a different bool
                        if (mirCheck) {
                            //TdB: first mirror the reference star positions (around the 0,0 pixel) using the radial offset coefficients and the ghost/star angle
		            double ghost_offset_rad = psPolynomial1DEval (mirrorRad, rSrc);
		            double ghost_x_fpa_mirror = ref->FP->x + ((ref->FP->x*-1.)/abs(ref->FP->x)*abs(cos(theta0)*ghost_offset_rad));
		            double ghost_y_fpa_mirror = ref->FP->y + ((ref->FP->y*-1.)/abs(ref->FP->y)*abs(sin(theta0)*ghost_offset_rad));

		            // Now use the mirrored position together with the 2D ghost center fitting to get the actual ghost position in FPA coords 
		            ghost->FP->x = ghost_x_fpa_mirror + psPolynomial2DEval(centerX, ghost_x_fpa_mirror, ghost_y_fpa_mirror);
		            ghost->FP->y = ghost_y_fpa_mirror + psPolynomial2DEval(centerY, ghost_x_fpa_mirror, ghost_y_fpa_mirror);
                        } else {
                            //Use the old-style ghost position determination
                            ghost->FP->x = -ref->FP->x + psPolynomial2DEval(centerX, -ref->FP->x, -ref->FP->y);
                            ghost->FP->y = -ref->FP->y + psPolynomial2DEval(centerY, -ref->FP->x, -ref->FP->y);
                        }

 			ghost->inner.major = psPolynomial1DEval (innerMajor, rSrc);
 			ghost->inner.minor = psPolynomial1DEval (innerMinor, rSrc);
 			ghost->inner.theta = atan2(ref->FP->y, ref->FP->x);

 			ghost->outer.major = psPolynomial1DEval (outerMajor, rSrc);
 			ghost->outer.minor = psPolynomial1DEval (outerMinor, rSrc);
 			ghost->outer.theta = atan2(ref->FP->y, ref->FP->x);

 			// report instrumental ghost star mags
 			ghost->Mag = ref->Mag - MagOffset;
 			
 			// XXX this code yields a single chip: we need to provide results for any chips
 			// which encompass the full size of the ghost
 			pmChip *ghostChip = pmSourceFindChip (&ghost->chip->x, &ghost->chip->y, fpa, -ghost->srcFP->x, -ghost->srcFP->y);
 			ghostChip = pmSourceFindChip (&ghost->chip->x, &ghost->chip->y, fpa, ghost->FP->x, ghost->FP->y);

                        //do a rudimentary check of whether the ghost is on the pixel FPA
                        if (abs(ghost->FP->y) > 21000.)  continue;
                        if (abs(ghost->FP->x) > 21000.)  continue;

                        //save the ghost positions
 			if (ghostChip) {			
 			    psMetadata *row = psMetadataAlloc ();
 			    psMetadataAdd (row, PS_LIST_TAIL, "X_STAR_FPA",     PS_DATA_F32,    "x coord on focal plane",	  ref->FP->x);
 			    psMetadataAdd (row, PS_LIST_TAIL, "Y_STAR_FPA",     PS_DATA_F32,    "y coord on focal plane",	  ref->FP->y);
 			    psMetadataAdd (row, PS_LIST_TAIL, "STAR_CHIP_NAME", PS_DATA_STRING, "star chip name",	 psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"));
 			    psMetadataAdd (row, PS_LIST_TAIL, "MAG_REF",        PS_DATA_F32,    "reference star magnitude",	  ghost->Mag);
 			    psMetadataAdd (row, PS_LIST_TAIL, "X_GHOST_FPA",    PS_DATA_F32,    "x coord on focal plane",	  ghost->FP->x);
 			    psMetadataAdd (row, PS_LIST_TAIL, "Y_GHOST_FPA",    PS_DATA_F32,    "y coord on focal plane",	  ghost->FP->y);
 			    psMetadataAdd (row, PS_LIST_TAIL, "X_GHOST_CHIP",   PS_DATA_F32,    "x coord on chip",	          ghost->chip->x);
 			    psMetadataAdd (row, PS_LIST_TAIL, "Y_GHOST_CHIP",   PS_DATA_F32,    "y coord on chip",	          ghost->chip->y);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_CHIP_NAME",PS_DATA_STRING, "ghost chip name",	 psMetadataLookupStr(NULL,ghostChip->concepts,"CHIP.NAME"));
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_INNER_MAJ",PS_DATA_F32,    "inner major axis (pixels)",	  ghost->inner.major);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_INNER_MIN",PS_DATA_F32,    "inner minor axis (pixels)",	  ghost->inner.minor);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_INNER_ANG",PS_DATA_F32,    "inner angle (degrees)",	  ghost->inner.theta/PM_RAD_DEG);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_OUTER_MAJ",PS_DATA_F32,    "outer major axis (pixels)",	  ghost->outer.major);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_OUTER_MIN",PS_DATA_F32,    "outer minor axis (pixels)",	  ghost->outer.minor);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_OUTER_ANG",PS_DATA_F32,    "outer angle (degrees)",	  ghost->outer.theta/PM_RAD_DEG);
			   
 			    psArrayAdd (table, 100, row);
 			    psFree (row);
						
 			}
 			else {
 			    psMetadata *row = psMetadataAlloc ();
 			    psMetadataAdd (row, PS_LIST_TAIL, "X_STAR_FPA",     PS_DATA_F32,    "x coord on focal plane",	  ref->FP->x);
 			    psMetadataAdd (row, PS_LIST_TAIL, "Y_STAR_FPA",     PS_DATA_F32,    "y coord on focal plane",	  ref->FP->y);
 			    psMetadataAdd (row, PS_LIST_TAIL, "STAR_CHIP_NAME", PS_DATA_STRING, "star chip name",	 psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"));
 			    psMetadataAdd (row, PS_LIST_TAIL, "MAG_REF",        PS_DATA_F32,    "reference star magnitude",	  ghost->Mag);
 			    psMetadataAdd (row, PS_LIST_TAIL, "X_GHOST_FPA",    PS_DATA_F32,    "x coord on focal plane",	  ghost->FP->x);
 			    psMetadataAdd (row, PS_LIST_TAIL, "Y_GHOST_FPA",    PS_DATA_F32,    "y coord on focal plane",	  ghost->FP->y);
 			    psMetadataAdd (row, PS_LIST_TAIL, "X_GHOST_CHIP",   PS_DATA_F32,    "x coord on chip",	          ghost->chip->x);
 			    psMetadataAdd (row, PS_LIST_TAIL, "Y_GHOST_CHIP",   PS_DATA_F32,    "y coord on chip",	          ghost->chip->y);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_CHIP_NAME",PS_DATA_STRING, "ghost chip name",	 "NONE");
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_INNER_MAJ",PS_DATA_F32,    "inner major axis (pixels)",	  ghost->inner.major);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_INNER_MIN",PS_DATA_F32,    "inner minor axis (pixels)",	  ghost->inner.minor);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_INNER_ANG",PS_DATA_F32,    "inner angle (degrees)",	  ghost->inner.theta/PM_RAD_DEG);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_OUTER_MAJ",PS_DATA_F32,    "outer major axis (pixels)",	  ghost->outer.major);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_OUTER_MIN",PS_DATA_F32,    "outer minor axis (pixels)",	  ghost->outer.minor);
 			    psMetadataAdd (row, PS_LIST_TAIL, "GHOST_OUTER_ANG",PS_DATA_F32,    "outer angle (degrees)",	  ghost->outer.theta/PM_RAD_DEG);
			   
 			    psArrayAdd (table, 100, row);
 			    psFree (row);
 			}		  
 						
 		    }
 		}
 	    }
 	}

    	psFree (centerX);
    	psFree (centerY);
    	psFree (innerMajor);
    	psFree (innerMinor);
    	psFree (outerMajor);
    	psFree (outerMinor);
    	psFree (mirrorRad);
    	psFree (ghostModel);
    	psFree (view);

        if (table->n == 0) {
            psFree(table);
            return true;
        }
    }

    if (!psFitsWriteTable(fits, NULL, table, "GHOST_POSITIONS")) {
        psError(psErrorCodeLast(), false, "writing GHOST_POSITIONS\n");
        psFree(table);
        return false;
    }
    psFree(table);
    return true;
    
escape:
    psFree (centerX);
    psFree (centerY);
    psFree (innerMajor);
    psFree (innerMinor);
    psFree (outerMajor);
    psFree (outerMinor);
    psFree (mirrorRad);
    psFree (ghostModel);
    psFree (view);
    return false;    
}

static void pmSourceGhostFree (pmSourceGhost *ghost) {

    if (ghost == NULL) return;

    psFree (ghost->srcFP);
    psFree (ghost->FP);
    psFree (ghost->chip);

    return;
}

pmSourceGhost *pmSourceGhostAlloc (void) {

    pmSourceGhost *ghost = (pmSourceGhost *) psAlloc(sizeof(pmSourceGhost));
    psMemSetDeallocator(ghost, (psFreeFunc) pmSourceGhostFree);

    ghost->srcFP = psPlaneAlloc();
    ghost->FP    = psPlaneAlloc();
    ghost->chip  = psPlaneAlloc();
    
    ghost->Mag   = 0.0;

    ghost->inner.major = 0.0;
    ghost->inner.minor = 0.0;
    ghost->inner.theta = 0.0;

    ghost->outer.major = 0.0;
    ghost->outer.minor = 0.0;
    ghost->outer.theta = 0.0;

    return ghost;
}


bool pmSourceChipBounds (pmFPA *fpa) {

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

pmChip *pmSourceFindChip (double *xChip, double *yChip, pmFPA *fpa, double xFPA, double yFPA) {

    *xChip = NAN;
    *yChip = NAN;

    if (!chipXmin) {
	pmSourceChipBounds (fpa);
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
bool pmSourceFindChipInXrange (pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    if (!chipXmin || !chipXmax) {
	psAbort ("chip bounds not set");
    }

    if (xFPA <  chipXmin->data.F32[nChip]) return false;
    if (xFPA >= chipXmax->data.F32[nChip]) return false;
    return true;
}

// identify chips which land on this row (FP coords)
bool pmSourceFindChipInYrange (pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    if (!chipYmin || !chipYmax) {
	psAbort ("chip bounds not set");
    }

    if (yFPA <  chipYmin->data.F32[nChip]) return false;
    if (yFPA >= chipYmax->data.F32[nChip]) return false;
    return true;
}

// return the FPA coordinates of the Y edges of the chip
bool pmSourceFindChipYedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip) {

    *yFPAs = chipYmin->data.F32[nChip];
    *yFPAe = chipYmax->data.F32[nChip];
    return true;
}

// return the FPA coordinates of the X edges of the chip
bool pmSourceFindChipXedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip) {

    *yFPAs = chipXmin->data.F32[nChip];
    *yFPAe = chipXmax->data.F32[nChip];
    return true;
}

// convert FPA to Chip coordinates
bool pmSourceFPAtoChip (double *xChip, double *yChip, pmFPA *fpa, int nChip, double xFPA, double yFPA) {

    pmChip *chip = fpa->chips->data[nChip];

    psPlane ptCH, ptFP;
    ptFP.x = xFPA;
    ptFP.y = yFPA;
    psPlaneTransformApply (&ptCH, chip->fromFPA, &ptFP);

    *xChip = ptCH.x;
    *yChip = ptCH.y;
    return true;
}

bool pmSourceExtractFreeChipBounds () {
  
  psFree (chipXmin);
  psFree (chipXmax);
  psFree (chipYmin);
  psFree (chipYmax);
  return true;
}



/** @file  pmSourceIO_Glints.c
 *
 *  @author TdB, IfA
 *
 *  @version $Revision: 1.0 $ $Name: not supported by cvs2svn $
 *  @date $Date: 20020-03-18  $
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

# define ESCAPE(MSG) { \
  psLogMsg ("psastro", PS_LOG_INFO, MSG); \
  return false; }

bool pmSourceIO_WriteGlints (psFits *fits, pmFPA *fpa, pmConfig *config) {

    bool status;
    float zeropt, exptime;

    psLogMsg ("psastro", PS_LOG_INFO, "writing glint positions");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PM_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    bool REFSTAR_MASK_GLINTS = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_GLINTS");
    if (!REFSTAR_MASK_GLINTS) return true;

    // select relevant keywords
    double GLINT_MAX_MAG = psMetadataLookupF32 (&status, recipe, "GLINT_MAX_MAG");
    double GLINT_LENGTH_MAG_SLOPE = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_MAG_SLOPE");
    double GLINT_LENGTH_MAG_ZERO = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_MAG_ZERO");
    double glintWidth = psMetadataLookupF32 (&status, recipe, "GLINT_WIDTH");

    //we will use one of the new keywords to differentiate between an old and new style glint treatment
    float glintCheck = 0;
    double GLINT_LENGTH_POS_SLOPE = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_POS_SLOPE");
    if (!status) { glintCheck = 1; }
    double GLINT_LENGTH_POS_REF = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_POS_REF");
    double GLINT_LENGTH_POS_CUT = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_POS_CUT");
    double GLINT_ANGLE_POS_SLOPE = psMetadataLookupF32 (&status, recipe, "GLINT_ANGLE_POS_SLOPE");
    double GLINT_ANGLE_POS_REF = psMetadataLookupF32 (&status, recipe, "GLINT_ANGLE_POS_REF");

    bool GLINT_OUTPUT = psMetadataLookupBool(&status, recipe, "PSASTRO.SAVE.GLINTS");
    if (!GLINT_OUTPUT) return true;

    // select the set of glint regions (GLINT.REGION is a MULTI of METADATA items)
    psMetadataItem *glintRegions = psMetadataLookup (recipe, "GLINT.REGION");
    if (!status) {
        psWarning ("glint masking requested but glint regions are missing (GLINT.REGION)\n");
        return true;
    }
    if (glintRegions->type != PS_DATA_METADATA_MULTI) {
        psWarning ("GLINT.REGION is not a MULTI\n");
        return true;
    }

    // select the input astrometry data (also carries the glintStars)
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!astrom) {
        psError(PM_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    pmFPA *fpa_ast = astrom->fpa;
    if (!pmSourceZeroPointFromRecipeGlint (&zeropt, &exptime, NULL,&GLINT_MAX_MAG, fpa_ast, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }
    float MagOffset = zeropt + 2.5*log10(exptime);
    GLINT_MAX_MAG += MagOffset;
    GLINT_LENGTH_MAG_ZERO += MagOffset;

    // select the raw objects for this readout (loaded in psastroExtract.c)
    psArray *glintStars = psMetadataLookupPtr (&status, fpa_ast->analysis, "PSASTRO.GLINT.STARS");
    if (glintStars == NULL) { 
        psLogMsg ("psastro", PS_LOG_INFO, "no glint stars found");
        return false;
    }

    psArray *table = psMetadataLookupPtr (&status, fpa->analysis, "GLINT_POSITIONS");
    psMemIncrRefCounter (table);
    if (!table) {
        table = psArrayAllocEmpty (0x1000);

        for (int i = 0; i < glintStars->n; i++) {
	    pmAstromObj *star = glintStars->data[i];
	    if (star->Mag > GLINT_MAX_MAG) continue; // XXX should not be needed...

	    // project glint star to the focal-plane
	    psProject (star->TP, star->sky, fpa_ast->toSky);
	    psPlaneTransformApply (star->FP, fpa_ast->fromTPA, star->TP);

	    // find the GLINT.REGION this star lands in (if any)
	    psListIterator *glintIter = psListIteratorAlloc(glintRegions->data.list, PS_LIST_HEAD, false);
	    psMetadataItem *glintItem = NULL;
	    while ((glintItem = psListGetAndIncrement (glintIter))) {
	        if (glintItem->type != PS_DATA_METADATA) {
		    psWarning ("GLINT.REGION entry is not a metadata folder");
	    	    continue;
	        }
	    
	        char *glintRegionString = psMetadataLookupStr (&status, glintItem->data.md, "REGION");
	        if (!glintRegionString) {
		    // psWarning ("GLINT.REGION entry is missing REGION entry");
		    continue;
	        }
	        psRegion glintRegion = psRegionFromString (glintRegionString);

	        // select stars that land in this region
	        if (star->FP->x < glintRegion.x0) continue;
	        if (star->FP->x > glintRegion.x1) continue;
	        if (star->FP->y < glintRegion.y0) continue;
	        if (star->FP->y > glintRegion.y1) continue;

	        char *glintType = psMetadataLookupStr (&status, glintItem->data.md, "GLINT.TYPE");
	        if (!status) {
		    psWarning ("GLINT.REGION entry is missing TYPE entry");
		    continue;
	        }

	        double glintLength = GLINT_LENGTH_MAG_SLOPE*(GLINT_LENGTH_MAG_ZERO - star->Mag);
                double glintAngle = 0.;

                //Besides brightness, the length of the glints also depends on the position of the star compared to the focal plane. But, seemingly only for stars closer than about 30k pixels
                if(glintCheck) {
	            if ((!strcasecmp(glintType, "TOP") || !strcasecmp(glintType, "BOTTOM")) && abs(star->FP->y) < GLINT_LENGTH_POS_CUT ){ 
                      glintLength /= GLINT_LENGTH_POS_SLOPE*(GLINT_LENGTH_POS_REF - abs(star->FP->y));
                    }
	            if ((!strcasecmp(glintType, "LEFT") || !strcasecmp(glintType, "RIGHT")) && abs(star->FP->x) < GLINT_LENGTH_POS_CUT ) {
                      glintLength /= GLINT_LENGTH_POS_SLOPE*(GLINT_LENGTH_POS_REF - abs(star->FP->x));
                    }
                    //also compute the angle of the glint, which depends on position parallel to the FPX
	            if (!strcasecmp(glintType, "TOP") || !strcasecmp(glintType, "BOTTOM") ){ 
	              glintAngle = PM_RAD_DEG * (GLINT_ANGLE_POS_SLOPE*((GLINT_ANGLE_POS_REF - star->FP->x)/1000.));
                    }
	            if (!strcasecmp(glintType, "LEFT") || !strcasecmp(glintType, "RIGHT") ) {
 	              glintAngle = PM_RAD_DEG * (GLINT_ANGLE_POS_SLOPE*((GLINT_ANGLE_POS_REF - star->FP->y)/1000.));
                    }
                }

                //do a rudimentary check of whether the glint enters the pixel FPA, and exclude very short ones
                if (!strcasecmp(glintType, "TOP")    && ((star->FP->y - glintLength) > 19500.))  continue;
                if (!strcasecmp(glintType, "BOTTOM") && ((star->FP->y + glintLength) < -19500.))  continue;
                if (!strcasecmp(glintType, "LEFT")   && ((star->FP->x + glintLength) < -19500.))  continue;
                if (!strcasecmp(glintType, "RIGHT")  && ((star->FP->x - glintLength) > 19500.))  continue;
            
                //Set a glint type enum as well
                double glintTypeEnum = 0;
                if (!strcasecmp(glintType, "TOP"))  glintTypeEnum = 0;
                if (!strcasecmp(glintType, "BOTTOM"))  glintTypeEnum = 1;
                if (!strcasecmp(glintType, "LEFT"))  glintTypeEnum = 2;
                if (!strcasecmp(glintType, "RIGHT"))  glintTypeEnum = 3;

                //save the glint positions
                psMetadata *row = psMetadataAlloc ();
                psMetadataAdd (row, PS_LIST_TAIL, "X_FPA_START", PS_DATA_F32, "x coord on focal plane",       star->FP->x);
                psMetadataAdd (row, PS_LIST_TAIL, "Y_FPA_START", PS_DATA_F32, "y coord on focal plane",       star->FP->y);
                psMetadataAdd (row, PS_LIST_TAIL, "MAG_REF",     PS_DATA_F32, "reference star magnitude",     star->Mag);
                psMetadataAdd (row, PS_LIST_TAIL, "GLINT_LENGTH",PS_DATA_F32, "glint length (pixels)",        glintLength);
                psMetadataAdd (row, PS_LIST_TAIL, "GLINT_WIDTH", PS_DATA_F32, "glint width (pixels)",         glintWidth);
                psMetadataAdd (row, PS_LIST_TAIL, "GLINT_ANGLE", PS_DATA_F32, "glint angle (degrees)",        glintAngle/PM_RAD_DEG);
                psMetadataAdd (row, PS_LIST_TAIL, "GLINT_TYPE",  PS_DATA_STRING, "glint type",                glintType);
                psMetadataAdd (row, PS_LIST_TAIL, "GLINT_TYPE_ENUM", PS_DATA_F32, "glint type enum",          glintTypeEnum);

                psArrayAdd (table, 100, row);
                psFree (row);
            }
        }

        if (table->n == 0) {
            psFree(table);
            return true;
        }
    }

    if (!psFitsWriteTable(fits, NULL, table, "GLINT_POSITIONS")) {
        psError(psErrorCodeLast(), false, "writing GLINT_POSITIONS\n");
        psFree(table);
        return false;
    }
    psFree(table);
    return true;
}

bool pmSourceZeroPointFromRecipeGlint (float *zeropt, float *exptime, float *ghostMaxMag, double *glintMaxMag, pmFPA *fpa, psMetadata *recipe) {

    bool status;

    // select the filter; default to fixed photcode and mag limit otherwise
    char *filter = psMetadataLookupStr (&status, fpa->concepts, "FPA.FILTERID");
    if (!status) ESCAPE ("missing FPA.FILTER in concepts");

    *exptime = psMetadataLookupF32 (&status, fpa->concepts, "FPA.EXPOSURE");
    if (!status) ESCAPE ("missing FPA.EXPOSURE in concepts");

    // we need to select the PHOTCODE.DATA folder that matches our filter
    psMetadataItem *item = psMetadataLookup (recipe, "PHOTCODE.DATA");
    if (!item) ESCAPE ("PHOTCODE.DATA folders missing");
    if (item->type != PS_DATA_METADATA_MULTI) ESCAPE ("PHOTCODE.DATA not a multi");

    // PHOTCODE.DATA is a multi of metadata items
    psListIterator *iter = psListIteratorAlloc(item->data.list, PS_LIST_HEAD, false);

    psMetadataItem *refItem = NULL;
    while ((refItem = psListGetAndIncrement (iter))) {
        if (refItem->type != PS_DATA_METADATA) ESCAPE ("PHOTCODE.DATA entry is not a metadata folder");

        char *refFilter = psMetadataLookupStr (&status, refItem->data.md, "FILTER");
        if (!status) {
            // psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing FILTER");
            continue;
        }

        // does this entry match the current filter?
        if (strcmp (refFilter, filter)) continue;

        psLogMsg ("psastro", PS_LOG_DETAIL, "PHOTCODE.DATA found for filter %s", filter);

        *zeropt = psMetadataLookupF32 (&status, refItem->data.md, "ZEROPT");
        if (!status) {
            psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing ZEROPT");
            continue;
        }
        if (ghostMaxMag) {
            *ghostMaxMag = psMetadataLookupF32 (&status, refItem->data.md, "GHOST_MAX_MAG");
            if (!status) {
                psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing GHOST_MAX_MAG");
                continue;
            }
	    //MEH null is a pain.. so only log if set
            psLogMsg ("psastro", PS_LOG_INFO, "found GHOST_MAX_MAG %f",*ghostMaxMag);
        }
        if (glintMaxMag) {
            float MaxMag = psMetadataLookupF32 (&status, refItem->data.md, "GLINT_MAX_MAG");
            if (status) {
                *glintMaxMag = MaxMag ;
	        //MEH null is a pain.. so only log if set
                psLogMsg ("psastro", PS_LOG_INFO, "found GLINT_MAX_MAG %f",*glintMaxMag);
            }
        }

	//MEH what zpt is set to 
        psLogMsg ("psastro", PS_LOG_INFO, "found ZEROPT  %f",*zeropt);
        psFree (iter);
        return true;
    }
    psFree (iter);
    return false;
}



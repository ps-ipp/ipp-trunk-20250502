/** @file psastroLoadGhosts.c
 *
 *  @brief calculate ghost FPA and Chip positions for the stars loaded on the FPA based on the model
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

// These are only used locally:
bool psastroLoadGhostsGPC (pmConfig *config, psMetadata *recipe, psMetadata *ghostModel, char *ghostFile);
bool psastroLoadGhostsHSC (pmConfig *config, psMetadata *recipe, psMetadata *ghostModel, char *ghostFile);

# define GET_2D_POLY(NAME,OUT) \
    md = psMetadataLookupMetadata (&status, ghostModel, NAME); \
    if (!md) { \
	psError(PSASTRO_ERR_CONFIG, true, "Missing %s in model file %s", NAME, ghostFile); \
	goto escape; \
    } \
    OUT = psPolynomial2DfromMetadata(md); \
    if (!OUT) { \
	psError(PSASTRO_ERR_CONFIG, true, "Trouble interpretting %s in model file %s", NAME, ghostFile); \
	goto escape; \
    }

# define GET_1D_POLY(NAME,OUT) \
    md = psMetadataLookupMetadata (&status, ghostModel, NAME); \
    if (!md) { \
	psError(PSASTRO_ERR_CONFIG, true, "Missing %s in model file %s", NAME, ghostFile); \
	goto escape; \
    } \
    OUT = psPolynomial1DfromMetadata(md);	\
    if (!OUT) { \
	psError(PSASTRO_ERR_CONFIG, true, "Trouble interpretting %s in model file %s", NAME, ghostFile); \
	goto escape; \
    }

// To select good stars, I want them to have a PSFmodel fit (PM_SOURCE_MODE_PSFMODEL & PM_SOURCE_MODE_FITTED) and exlcude things that are CR, EXT, FAIL, POOR 
// Stars are allowed to be saturated, as long as the PSF model fits
# define PHOT_SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_BLEND | PM_SOURCE_MODE_BADPSF | \
                           PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT | \
                           PM_SOURCE_MODE_POOR) // Mask to apply to sources for rejection

/**
 * calculate ghost FPA and Chip positions for the stars loaded on the FPA
 */

bool psastroLoadGhosts (pmConfig *config) {

    bool status;

    psLogMsg ("psastro", PS_LOG_INFO, "determine ghost positions");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    // do we want to mask ghosts?
    bool REFSTAR_MASK_GHOST = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_GHOST");
    if (!REFSTAR_MASK_GHOST) return true;

    // choose ghost model file
    char *ghostFile = psMetadataLookupStr (&status, recipe, "GHOST_MODEL");
    if (!strcasecmp(ghostFile, "NONE")) return true;

    // load ghost model metadata structure
    psMetadata *ghostModel = NULL;
    if (!pmConfigFileRead (&ghostModel, ghostFile, "GHOST MODEL")) {
	psError(PSASTRO_ERR_CONFIG, true, "Trouble loading ghost model");
        return false;
    }

    // test for items which are used by either the GPC or HSC ghost models:
    psMetadataItem *item = NULL;

    // test for required HSC ghost model elements
    item = psMetadataLookup(ghostModel, "GHOST.MODEL.HSC");
    if (item) {
	status = psastroLoadGhostsHSC (config, recipe, ghostModel, ghostFile);
	if (!status) goto escape;
    }
    
    // test for required GPC ghost model elements
    item = psMetadataLookup(ghostModel, "GHOST.CENTER.X");
    if (item) {
	status = psastroLoadGhostsGPC (config, recipe, ghostModel, ghostFile);
	if (!status) goto escape;
    }
    
escape:
    psFree (ghostModel);
    return status;
}
 
bool psastroLoadGhostsGPC (pmConfig *config, psMetadata *recipe, psMetadata *ghostModel, char *ghostFile) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float zeropt, exptime,MAX_MAG, INSTR_MAX_MAG;
    psMetadata *md = NULL;
    psPolynomial2D *centerX = NULL;
    psPolynomial2D *centerY = NULL;
    psPolynomial1D *mirrorRad = NULL;
    psPolynomial1D *outerMajor = NULL;
    psPolynomial1D *outerMinor = NULL;
    psPolynomial1D *innerMajor = NULL;
    psPolynomial1D *innerMinor = NULL;

    pmFPAview *view = pmFPAviewAlloc (0);

    //We need to check whether we are dealing with an old style ghost_model, or a new style model. Check if the mirror_rad polynomial exists
    int mirCheck = 0;
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
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
	goto escape;
    }
    pmFPA *fpa = astrom->fpa;

    // select the reference mask fpa :: we use this to determine cell boundaries
    pmFPAfile *refMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.REFMASK");
    if (!refMask) {
      psError(PSASTRO_ERR_CONFIG, true, "Can't find mask reference");
      return false;
    }
    // Activate the reference mask to generate an FPA structure we can use to map stars down to cells
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.REFMASK");

    // raise an error if the config is broken
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, &INSTR_MAX_MAG, NULL, fpa, recipe)) {
        psError(PSASTRO_ERR_CONFIG, true, "failed to load zeropt data from recipe");
	goto escape;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    MAX_MAG = INSTR_MAX_MAG + MagOffset;

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->fromFPA) { continue; }

        // Get the current chip's name, and parse out the grid location.
        pmChip *refChip  = pmFPAviewThisChip (view, refMask->fpa);
        const char *chipName = psMetadataLookupStr(NULL,refChip->concepts, "CHIP.NAME");
        int X = chipName[2] - '0';
        int Y = chipName[3] - '0';

        //Check if we are in the central 8 chips, where old ghost-style locations work better
        int cenChip = 0;
        if ( (Y >= 3)&&(Y <=4)&&(X >= 2)&&(X <= 5) ) {cenChip = 0;}

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                //First, select bright sources for ghosts from the detections. Later on, do the same using the refcat
                // That way we can include things like movers creating crosstalk features.
                psArray *calstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.CALSTARS");
                if (calstars == NULL) { continue; }

                // identify the bright stars of interest
                for (int i = 0; i < calstars->n; i++) {
                    pmAstromObj *cal = calstars->data[i];
		    psTrace("psastro.ghost",5,"Begin ghost %d/%ld: MAX_MAG: %g; ref_mag: %g @ (%.10g,%.10g)",i,calstars->n,INSTR_MAX_MAG,cal->Mag,cal->sky->r * 180 / M_PI,cal->sky->d * 180.0 / M_PI);

                    if (cal->Mag > INSTR_MAX_MAG) {
                        continue;
                    }

		    psastroGhost *ghost = psastroGhostAlloc ();
		    ghost->srcFP->x = cal->FP->x; 
		    ghost->srcFP->y = cal->FP->y;

		    double rSrc = hypot (cal->FP->x, cal->FP->y);
     	            double theta0 = atan2(cal->FP->y,cal->FP->x);

                    if((mirCheck) & (!cenChip)) {
                         //TdB: first mirror the reference star positions (around the 0,0 pixel) using the radial offset coefficients and the ghost/star angle
		        double ghost_offset_rad = psPolynomial1DEval (mirrorRad, rSrc);
		        double ghost_x_fpa_mirror = cal->FP->x + ((cal->FP->x*-1.)/abs(cal->FP->x)*abs(cos(theta0)*ghost_offset_rad));
		        double ghost_y_fpa_mirror = cal->FP->y + ((cal->FP->y*-1.)/abs(cal->FP->y)*abs(sin(theta0)*ghost_offset_rad));

		        // Now use the mirrored position together with the 2D ghost center fitting to get the actual ghost position in FPA coords 
		        ghost->FP->x = ghost_x_fpa_mirror + psPolynomial2DEval(centerX, ghost_x_fpa_mirror, ghost_y_fpa_mirror);
		        ghost->FP->y = ghost_y_fpa_mirror + psPolynomial2DEval(centerY, ghost_x_fpa_mirror, ghost_y_fpa_mirror);
                    }  
                    if((!mirCheck) | cenChip) {
                        //Use the old-style ghost position determination
                        ghost->FP->x = -cal->FP->x + psPolynomial2DEval(centerX, -cal->FP->x, -cal->FP->y);
                        ghost->FP->y = -cal->FP->y + psPolynomial2DEval(centerY, -cal->FP->x, -cal->FP->y);
                    }

		    ghost->inner.major = psPolynomial1DEval (innerMajor, rSrc);
		    ghost->inner.minor = psPolynomial1DEval (innerMinor, rSrc);
		    ghost->inner.theta = atan2(cal->FP->y, cal->FP->x);

		    ghost->outer.major = psPolynomial1DEval (outerMajor, rSrc);
		    ghost->outer.minor = psPolynomial1DEval (outerMinor, rSrc);
		    ghost->outer.theta = atan2(cal->FP->y, cal->FP->x);

		    // report instrumental ghost star mags
		    ghost->Mag = cal->Mag;
		    
		    // XXX this code yields a single chip: we need to provide results for any chips
		    // which encompass the full size of the ghost
		    pmChip *ghostChip = psastroFindChip (&ghost->chip->x, &ghost->chip->y, fpa, -ghost->srcFP->x, -ghost->srcFP->y);
/* 		    float star_chip_x = ghost->chip->x; */
/* 		    float star_chip_y = ghost->chip->y; */
		    ghostChip = psastroFindChip (&ghost->chip->x, &ghost->chip->y, fpa, ghost->FP->x, ghost->FP->y);
                    //psLogMsg ("psastro", PS_LOG_INFO, "-> DETEC model chip position: %f, %f %f, %f %g\n", ghost->chip->x, ghost->chip->y,ghost->srcFP->x, ghost->srcFP->y,cal->Mag+MagOffset);

		    if (ghostChip) {
		      psTrace("psastro.ghost",5,"   in ghost: %d/%ld: ref: (%f,%f) or (%s) ghost: (%f,%f) or (%f,%f,%s) %f inner: (%f,%f,%f) outer: (%f,%f,%f)",
			      i,calstars->n,
			      cal->FP->x,cal->FP->y,
			      psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"),
			      ghost->FP->x,ghost->FP->y,
			      ghost->chip->x,ghost->chip->y,psMetadataLookupStr(NULL,ghostChip->concepts,"CHIP.NAME"),
			      rSrc,
			      ghost->inner.major,ghost->inner.minor,ghost->inner.theta,
			      ghost->outer.major,ghost->outer.minor,ghost->outer.theta);
/* 		      psWarning("   GHOST_DATA: %d/%ld: ref: (%f,%f) or (%f,%f,%s) ghost: (%f,%f) or (%f,%f,%s) %f inner: (%f,%f,%f) outer: (%f,%f,%f)", */
/* 			      i,calstars->n, */
/* 			      cal->FP->x,cal->FP->y, */
/* 			      ref_chip_x,ref_chip_y,psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"), */
/* 			      ghost->FP->x,ghost->FP->y, */
/* 			      ghost->chip->x,ghost->chip->y,psMetadataLookupStr(NULL,ghostChip->concepts,"CHIP.NAME"), */
/* 			      rSrc, */
/* 			      ghost->inner.major,ghost->inner.minor,ghost->inner.theta, */
/* 			      ghost->outer.major,ghost->outer.minor,ghost->outer.theta); */
		    }
		    else {
		      psTrace("psastro.ghost",5,"   in ghost: %d/%ld: ref: (%f,%f) ghost: (%f,%f) or (%f,%f,%s) inner: (%f,%f,%f) outer: (%f,%f,%f)",
			      i,calstars->n,
			      cal->FP->x,cal->FP->y,
			      ghost->FP->x,ghost->FP->y,
			      ghost->chip->x,ghost->chip->y,"NONE",
			      ghost->inner.major,ghost->inner.minor,ghost->inner.theta,
			      ghost->outer.major,ghost->outer.minor,ghost->outer.theta);
		    }		      
		      			    
		    if (!ghostChip) goto skip;
		    if (!ghostChip->cells) goto skip;
		    if (!ghostChip->cells->n) goto skip;

		    
		    pmCell *ghostCell = ghostChip->cells->data[0];
		    if (!ghostCell) goto skip;
		    if (!ghostCell->readouts) goto skip;
		    if (!ghostCell->readouts->n) goto skip;
		    pmReadout *ghostReadout = ghostCell->readouts->data[0];
		    if (!ghostReadout) goto skip;

		    psArray *ghosts = psMetadataLookupPtr (&status, ghostReadout->analysis, "PSASTRO.GHOSTS");
		    if (ghosts == NULL) { 
			ghosts = psArrayAllocEmpty (100);
			if (!psMetadataAdd (ghostReadout->analysis, PS_LIST_TAIL, "PSASTRO.GHOSTS", PS_DATA_ARRAY, "astrometry matches", ghosts)) {
			  psError(PSASTRO_ERR_CONFIG, false, "failure to add ghosts to readout");
			  goto escape;
			}
			psFree (ghosts);
		    }

		    psArrayAdd (ghosts, 100, ghost);

		skip:
		    
		    psFree (ghost);
                }

                // select the raw objects for this readout (loaded in psastroChooseRefstars.c)
		// XXX : note that we place limits on the refstar sample in psastroChooseRefstars.c:
		// 1) on chip and 2) < PSASTRO.MAX.NREF. magnitude limits and clump exclusion are only 
                psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
                if (refstars == NULL) { continue; }

                // identify the bright stars of interest
                for (int i = 0; i < refstars->n; i++) {
                    pmAstromObj *ref = refstars->data[i];
		    psTrace("psastro.ghost",5,"Begin ghost %d/%ld: MAX_MAG: %g; ref_mag: %g @ (%.10g,%.10g)",i,refstars->n,MAX_MAG,ref->Mag,ref->sky->r * 180 / M_PI,ref->sky->d * 180.0 / M_PI);

                    if (ref->Mag > MAX_MAG) continue;

		    // Ghost model:
		    // X(ghost) = -X + dX(X,Y) --> GHOST.CENTER.X
		    // Y(ghost) = -Y + dY(X,Y) --> GHOST.CENTER.Y
		    // R1inner  = R1inner_0 + r dR1inner
		    // R2inner  = R2inner_0 + r dR1inner
		    // R1outer  = R1outer_0 + r dR1inner
		    // R2outer  = R2outer_0 + r dR1inner

		    psastroGhost *ghost = psastroGhostAlloc ();
		    ghost->srcFP->x = ref->FP->x; 
		    ghost->srcFP->y = ref->FP->y;

		    double rSrc = hypot (ref->FP->x, ref->FP->y);
     	            double theta0 = atan2(ref->FP->y,ref->FP->x);

                    if((mirCheck) & (!cenChip)) {
                         //TdB: first mirror the reference star positions (around the 0,0 pixel) using the radial offset coefficients and the ghost/star angle
		        double ghost_offset_rad = psPolynomial1DEval (mirrorRad, rSrc);
		        double ghost_x_fpa_mirror = ref->FP->x + ((ref->FP->x*-1.)/abs(ref->FP->x)*abs(cos(theta0)*ghost_offset_rad));
		        double ghost_y_fpa_mirror = ref->FP->y + ((ref->FP->y*-1.)/abs(ref->FP->y)*abs(sin(theta0)*ghost_offset_rad));

		        // Now use the mirrored position together with the 2D ghost center fitting to get the actual ghost position in FPA coords 
		        ghost->FP->x = ghost_x_fpa_mirror + psPolynomial2DEval(centerX, ghost_x_fpa_mirror, ghost_y_fpa_mirror);
		        ghost->FP->y = ghost_y_fpa_mirror + psPolynomial2DEval(centerY, ghost_x_fpa_mirror, ghost_y_fpa_mirror);
                    }  
                    if((!mirCheck) | cenChip) {
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
		    pmChip *ghostChip = psastroFindChip (&ghost->chip->x, &ghost->chip->y, fpa, -ghost->srcFP->x, -ghost->srcFP->y);
		    // fprintf (stderr, "raw chip position: %f, %f ", ghost->chip->x, ghost->chip->y);
/* 		    float ref_chip_x = ghost->chip->x; */
/* 		    float ref_chip_y = ghost->chip->y; */
		    ghostChip = psastroFindChip (&ghost->chip->x, &ghost->chip->y, fpa, ghost->FP->x, ghost->FP->y);
                    //psLogMsg ("psastro", PS_LOG_INFO, "-> model chip position: %f, %f %f, %f %g\n", ghost->chip->x, ghost->chip->y,ghost->srcFP->x, ghost->srcFP->y,ref->Mag);

		    if (ghostChip) {
		      psTrace("psastro.ghost",5,"   in ghost: %d/%ld: ref: (%f,%f) or (%s) ghost: (%f,%f) or (%f,%f,%s) %f inner: (%f,%f,%f) outer: (%f,%f,%f)",
			      i,refstars->n,
			      ref->FP->x,ref->FP->y,
			      psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"),
			      ghost->FP->x,ghost->FP->y,
			      ghost->chip->x,ghost->chip->y,psMetadataLookupStr(NULL,ghostChip->concepts,"CHIP.NAME"),
			      rSrc,
			      ghost->inner.major,ghost->inner.minor,ghost->inner.theta,
			      ghost->outer.major,ghost->outer.minor,ghost->outer.theta);
/* 		      psWarning("   GHOST_DATA: %d/%ld: ref: (%f,%f) or (%f,%f,%s) ghost: (%f,%f) or (%f,%f,%s) %f inner: (%f,%f,%f) outer: (%f,%f,%f)", */
/* 			      i,refstars->n, */
/* 			      ref->FP->x,ref->FP->y, */
/* 			      ref_chip_x,ref_chip_y,psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"), */
/* 			      ghost->FP->x,ghost->FP->y, */
/* 			      ghost->chip->x,ghost->chip->y,psMetadataLookupStr(NULL,ghostChip->concepts,"CHIP.NAME"), */
/* 			      rSrc, */
/* 			      ghost->inner.major,ghost->inner.minor,ghost->inner.theta, */
/* 			      ghost->outer.major,ghost->outer.minor,ghost->outer.theta); */
		    }
		    else {
		      psTrace("psastro.ghost",5,"   in ghost: %d/%ld: ref: (%f,%f) ghost: (%f,%f) or (%f,%f,%s) inner: (%f,%f,%f) outer: (%f,%f,%f)",
			      i,refstars->n,
			      ref->FP->x,ref->FP->y,
			      ghost->FP->x,ghost->FP->y,
			      ghost->chip->x,ghost->chip->y,"NONE",
			      ghost->inner.major,ghost->inner.minor,ghost->inner.theta,
			      ghost->outer.major,ghost->outer.minor,ghost->outer.theta);
		    }		      
		      			    
		    if (!ghostChip) goto skipref;
		    if (!ghostChip->cells) goto skipref;
		    if (!ghostChip->cells->n) goto skipref;

		    
		    pmCell *ghostCell = ghostChip->cells->data[0];
		    if (!ghostCell) goto skipref;
		    if (!ghostCell->readouts) goto skipref;
		    if (!ghostCell->readouts->n) goto skipref;
		    pmReadout *ghostReadout = ghostCell->readouts->data[0];
		    if (!ghostReadout) goto skipref;

		    psArray *ghosts = psMetadataLookupPtr (&status, ghostReadout->analysis, "PSASTRO.GHOSTS");
		    if (ghosts == NULL) { 
			ghosts = psArrayAllocEmpty (100);
			if (!psMetadataAdd (ghostReadout->analysis, PS_LIST_TAIL, "PSASTRO.GHOSTS", PS_DATA_ARRAY, "astrometry matches", ghosts)) {
			  psError(PSASTRO_ERR_CONFIG, false, "failure to add ghosts to readout");
			  goto escape;
			}
			psFree (ghosts);
		    }

		    psArrayAdd (ghosts, 100, ghost);

		skipref:
		    
		    psFree (ghost);
                }

            }
        }
    }

    psastroExtractFreeChipBounds();

    psFree (centerX);
    psFree (centerY);
    psFree (innerMajor);
    psFree (innerMinor);
    psFree (outerMajor);
    psFree (outerMinor);
    psFree (view);
    return true;

escape:
    psFree (centerX);
    psFree (centerY);
    psFree (innerMajor);
    psFree (innerMinor);
    psFree (outerMajor);
    psFree (outerMinor);
    psFree (view);
    return false;
}
 
// This function adds the ghost mask elements to the ghostReadout->analysis structures.
// We return false and raise an error on a config problem.
bool psastroLoadGhostsHSC (pmConfig *config, psMetadata *recipe, psMetadata *ghostModel, char *ghostFile) {

  bool status;
  pmChip *chip = NULL;
  pmCell *cell = NULL;
  pmReadout *readout = NULL;
  float zeropt, exptime, MAX_MAG;
  psVector *C_terms = NULL;
  psVector *R_terms = NULL;
  float glintPixelScale = 0.0;
  
  // Allocate FPA and refstars
  pmFPAview *view = pmFPAviewAlloc (0);

  // select the input astrometry data (also carries the refstars)
  pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
  if (!astrom) {
    psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
    goto escape;
  }
  pmFPA *fpa = astrom->fpa;
  
  char *filter = psMetadataLookupStr (&status, fpa->concepts, "FPA.FILTERID");

  psMetadataItem *item = psMetadataLookup(ghostModel, "GHOST.MODEL.HSC");
  if (!item) {
      psError(PSASTRO_ERR_CONFIG, true, "GHOST.MODEL.HSC data missing");
      goto escape;
  }
  if (item->type != PS_DATA_METADATA_MULTI) {
      psError(PSASTRO_ERR_CONFIG, true, "GHOST.MODEL.HSC not multi");
      goto escape;
  }
  
  psListIterator *iter = psListIteratorAlloc(item->data.list,PS_LIST_HEAD, false);
  psMetadataItem *refItem = NULL;
  while ((refItem = psListGetAndIncrement(iter))) {
    if (refItem->type != PS_DATA_METADATA) {
	psError(PSASTRO_ERR_CONFIG, true, "GHOST.MODEL.HSC entry not metadata");
	goto escape;
    }
    char *refFilter = psMetadataLookupStr (&status, refItem->data.md, "FILTER");
    if (!status) {
      continue;
    }
    if (strcmp(refFilter, filter)) continue;
    
    C_terms = psMetadataLookupPtr(&status, refItem->data.md, "C_TERMS");
    R_terms = psMetadataLookupPtr(&status, refItem->data.md, "R_TERMS");
    glintPixelScale = psMetadataLookupF32(&status, refItem->data.md, "SCALE");
  }

  // if a ghost model is not defined for a filter, skip HSC ghost model for that filter (no error)
  if (!R_terms) {
    psLogMsg ("psastro", PS_LOG_INFO, "failed to find HSC ghost model for filter %s", filter);
    goto giveup;
  }
  
  // if a filter is defined, but the recipe elements are missing, the config is broken.
  if (!psastroZeroPointFromRecipe (&zeropt, &exptime, &MAX_MAG, NULL, fpa, recipe)) {
	psError(PSASTRO_ERR_CONFIG, true, "failed to load zeropt data from recipe");
	goto escape;
  }
  
  // recipe values are given in instrumental magnitudes
  // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
  float MagOffset = zeropt + 2.5*log10(exptime);
  MAX_MAG += MagOffset;
  
  // this loop selects the matched stars for all chips
  while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
    psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
    if (!chip->process || !chip->file_exists) { continue; }
    if (!chip->fromFPA) { continue; }
    
    while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
      psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
      if (!cell->process || !cell->file_exists) { continue; }
      
      // process each of the readouts
      while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
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
	  
	  psastroGhost *ghost = psastroGhostAlloc ();
	  
	  double R_TPA  = sqrt(pow(ref->TP->y,2) + pow(ref->TP->x,2));
	  double theta0 = atan2(ref->TP->y,ref->TP->x);
	  double star_radius_deg = R_TPA * glintPixelScale;
	  
	  //	  psTrace("psastro.ghost",5,
	  psLogMsg("psastro",PS_LOG_INFO,
		  "Begin ghost %d/%ld: MAX_MAG: %g; ref_mag: %g @ (%.10g,%.10g) TPA: (%f %f) R: %f %f %f",
		  i,refstars->n,MAX_MAG,ref->Mag,ref->sky->r * 180 / M_PI,ref->sky->d * 180.0 / M_PI,ref->TP->x,ref->TP->y,
		  R_TPA,star_radius_deg,theta0);
	  
	  if (star_radius_deg > 0.5) { continue; }
	  
	  // Calculate expected position.
	  double C = C_terms->data.F32[0] + C_terms->data.F32[1] * star_radius_deg + C_terms->data.F32[2] * pow(star_radius_deg,2) +
	    C_terms->data.F32[3] * pow(star_radius_deg,3) + C_terms->data.F32[4] * pow(star_radius_deg,4) +
	    C_terms->data.F32[5] * pow(star_radius_deg,5) + C_terms->data.F32[6] * pow(star_radius_deg,6) +
	    C_terms->data.F32[7] * pow(star_radius_deg,7);
	  double R = R_terms->data.F32[0] + R_terms->data.F32[1] * star_radius_deg + R_terms->data.F32[2] * pow(star_radius_deg,2) +
	    R_terms->data.F32[3] * pow(star_radius_deg,3) + R_terms->data.F32[4] * pow(star_radius_deg,4) +
	    R_terms->data.F32[5] * pow(star_radius_deg,5) + R_terms->data.F32[6] * pow(star_radius_deg,6) +
	    R_terms->data.F32[7] * pow(star_radius_deg,7);
	  R = R / 0.015;
	  
	  psPlane *fp = psPlaneAlloc();
	  psPlane *tp = psPlaneAlloc();
	  
	  tp->x = 13.5 * (C * cos(theta0) / 0.015 + 12.78);
	  tp->y = 13.5 * (C * sin(theta0) / 0.015 + 57.74);
	  psPlaneTransformApply(fp,fpa->fromTPA, tp);
	  
	  ghost->srcFP->x = ref->FP->x; 
	  ghost->srcFP->y = ref->FP->y;
	  ghost->FP->x    = fp->x;
	  ghost->FP->y    = fp->y;
	  
	  ghost->inner.major = 0.0;
	  ghost->inner.minor = 0.0;
	  ghost->outer.major = R;
	  ghost->outer.minor = R;
	  
	  pmChip *ghostChip = psastroFindChip (&ghost->chip->x, &ghost->chip->y, fpa, ghost->FP->x, ghost->FP->y);
	  
	  if (ghostChip) {
	    psLogMsg("psastro", PS_LOG_INFO,
		    "   in ghost: %d/%ld: ref: (%f,%f) or (%s) ghost: (%f,%f) or (%f,%f,%s) %f inner: (%f,%f,%f) outer: (%f,%f,%f)",
		    i,refstars->n,
		    ref->FP->x,ref->FP->y,
		    psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME"),
		    ghost->FP->x,ghost->FP->y,
		    ghost->chip->x,ghost->chip->y,psMetadataLookupStr(NULL,ghostChip->concepts,"CHIP.NAME"),
		    C,
		    ghost->inner.major,ghost->inner.minor,ghost->inner.theta,
		    ghost->outer.major,ghost->outer.minor,ghost->outer.theta);
	  } else {
	    psTrace("psastro.ghost", 5,
		    "   in ghost: %d/%ld: ref: (%f,%f) ghost: (%f,%f) or (%f,%f,%s) inner: (%f,%f,%f) outer: (%f,%f,%f)",
		    i,refstars->n,
		    ref->FP->x,ref->FP->y,
		    ghost->FP->x,ghost->FP->y,
		    ghost->chip->x,ghost->chip->y,"NONE",
		    ghost->inner.major,ghost->inner.minor,ghost->inner.theta,
		    ghost->outer.major,ghost->outer.minor,ghost->outer.theta);
	  }		      
	  
	  if (!ghostChip) goto skip;
	  if (!ghostChip->cells) goto skip;
	  if (!ghostChip->cells->n) goto skip;
	  
	  
	  pmCell *ghostCell = ghostChip->cells->data[0];
	  if (!ghostCell) goto skip;
	  if (!ghostCell->readouts) goto skip;
	  if (!ghostCell->readouts->n) goto skip;
	  pmReadout *ghostReadout = ghostCell->readouts->data[0];
	  if (!ghostReadout) goto skip;
	  
	  psArray *ghosts = psMetadataLookupPtr (&status, ghostReadout->analysis, "PSASTRO.GHOSTS");
	  if (ghosts == NULL) { 
	    ghosts = psArrayAllocEmpty (100);
	    if (!psMetadataAdd (ghostReadout->analysis, PS_LIST_TAIL, "PSASTRO.GHOSTS", PS_DATA_ARRAY, "astrometry matches", ghosts)) {
	      psError(PSASTRO_ERR_CONFIG, false, "failure to add ghosts to readout");
	      goto escape;
	    }
	    psFree (ghosts);
	  }
	  psArrayAdd (ghosts, 100, ghost);
	  
	skip:
	  psFree (ghost);
	}
      }
    }
  }
  
  psastroExtractFreeChipBounds();
  
giveup:
  psFree (C_terms);
  psFree (R_terms);
  psFree (view);
  return true;
  
 escape:
  psFree (C_terms);
  psFree (R_terms);
  psFree (view);
  return false;
}

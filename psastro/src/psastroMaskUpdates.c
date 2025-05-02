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


int colStart(psU16 corner_list,int i,int x_0,int y_0,int R,int max) {
  if ((corner_list == 0x04)||(corner_list == 0x05)) {
    return(0);
  }
  else {
    return((int) fabs(x_0 - sqrt(pow(R,2) - pow(y_0 - i,2))));
  }    
}
int colEnd(psU16 corner_list,int i,int x_0,int y_0,int R,int max) {
  if ((corner_list == 0x04)||(corner_list == 0x05)) {
    int v = (int) fabs(x_0 - sqrt(pow(R,2) - pow(y_0 - i,2)));
    if (v > max) {
      return(max);
    }
    else {
      return(v);
    }
  }
  else {
    return(max);
  }    
  
}
int rowStart(psU16 corner_list,int j,int x_0,int y_0,int R,int max) {
  if ((corner_list == 0x01)||(corner_list == 0x02)||
      (corner_list == 0x03)||(corner_list == 0x0b)||
      (corner_list == 0x07)||(corner_list == 0x0f)) {
    return(0);
  }
  else {
    return((int) fabs(y_0 - sqrt(pow(R,2) - pow(x_0 - j,2))));
  }
}
int rowEnd(psU16 corner_list,int j,int x_0,int y_0,int R, int max) {
  if (corner_list == 0x0f) {
    return(max);
  }
  else if ((corner_list == 0x01)||(corner_list == 0x02)||
	   (corner_list == 0x03)||(corner_list == 0x0b)||
	   (corner_list == 0x07)) {
    int v = (int) fabs(y_0 - sqrt(pow(R,2) - pow(x_0 - j,2)));
    if (v > max) {
      return(max);
    }
    else {
      return(v);
    }
  }
  else {
    return(max);
  }
  
}

/* #define MASK_DEBUG 1 */

/*
 * create a mask or mask regions based on the collection of reference stars that * are in the vicinity of each chip
 */
bool psastroMaskUpdates (pmConfig *config, psMetadata *stats) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float zeropt, exptime, GHOST_MAX_MAG;

    psImageMaskType ghostMaskValue = pmConfigMaskGet("GHOST", config); // Mask value for ghost pixels
    psImageMaskType glintMaskValue = pmConfigMaskGet("GHOST", config); // Mask value for glint pixels (overload ghost)
    psImageMaskType spikeMaskValue = pmConfigMaskGet("SPIKE", config); // Mask value for ghost pixels
    psImageMaskType starMaskValue  = pmConfigMaskGet("STARCORE", config); // Mask value for ghost pixels
    psImageMaskType crosstalkMaskValue = pmConfigMaskGet("CROSSTALK", config); // Mask value for crosstalk ghosts

    // psImageMaskType maskBlank  = pmConfigMaskGet("BLANK", config); // Mask value for blank pixels

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    // this step is optional
    bool REFSTAR_MASK                      = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK");
    if (!REFSTAR_MASK) return true;

    // convert star positions to crosstalk artifact positions and add to the readout->analysis data
    bool CROSSTALK_MASK                    = psMetadataLookupBool (&status, recipe, "CROSSTALK_MASK");
    if (CROSSTALK_MASK && !psastroLoadCrosstalk (config)) {
      psError(PSASTRO_ERR_CONFIG, false, "Error loading crosstalk data");
      return(false);
    }

    // convert star positions to ghost positions and add to the readout->analysis data
    if (!psastroLoadGhosts (config)) {
        psError(PSASTRO_ERR_CONFIG, false, "Error loading ghosts");
        return false;
    }
    bool COUNT_GHOSTS = psMetadataLookupF32 (&status, recipe, "REFSTAR_COUNT_GHOSTS");
    int nGhosts = 0;

    // convert star positions to glint positions and add to the fpa->analysis data
    if (!psastroLoadGlints (config)) {
        psError(PSASTRO_ERR_CONFIG, false, "Error loading glints");
        return false;
    }
    psLogMsg ("psastro", PS_LOG_INFO, "generating a bright-star mask");
 
    bool REFSTAR_MASK_BLEED                = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_BLEED");
    bool REFSTAR_MASK_BLEED_ORIENTATION_X  = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_BLEED_ORIENTATION_X");
    bool REFSTAR_MASK_BLEED_ORIENTATION_Y  = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_BLEED_ORIENTATION_Y");

    
    double REFSTAR_MASK_MAX_MAG            = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_MAX_MAG");
    double REFSTAR_MASK_SATSTAR_MAG_MAX    = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_MAG_MAX");
    double REFSTAR_MASK_SATSTAR_RAD_OFFSET = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_RAD_OFFSET");
    double REFSTAR_MASK_SATSTAR_EXP        = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_EXP");
    double REFSTAR_MASK_SATSTAR_POS_ZERO   = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_POS_ZERO");
    double REFSTAR_MASK_SATSPIKE_MAG_SLOPE = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_MAG_SLOPE");
    double REFSTAR_MASK_SATSPIKE_MAG_MAX   = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_MAG_MAX");
    double REFSTAR_MASK_SATSPIKE_WIDTH     = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_WIDTH");
    double REFSTAR_MASK_SATSPIKE_L0        = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_L0");
    double REFSTAR_MASK_SATSPIKE_WIDTH_SLOPE = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_WIDTH_SLOPE");
    double REFSTAR_MASK_SATSPIKE_OFFSET    = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_OFFSET");
    double REFSTAR_MASK_BLEED_MAG_MAX      = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_BLEED_MAG_MAX");
    double REFSTAR_MASK_BLEED_MAG_SLOPE    = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_BLEED_MAG_SLOPE");

    double REFSTAR_MASK_BLEED_MAG_MAX_X    = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_BLEED_MAG_MAX_X");
    double REFSTAR_MASK_BLEED_MAG_SLOPE_X  = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_BLEED_MAG_SLOPE_X");
    double REFSTAR_MASK_BLEED_MAG_MAX_Y    = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_BLEED_MAG_MAX_Y");
    double REFSTAR_MASK_BLEED_MAG_SLOPE_Y  = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_BLEED_MAG_SLOPE_Y");

    //double REFSTAR_MASK_CROSSTALK_MAG_MAX  = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_CROSSTALK_MAG_MAX");
    //double REFSTAR_MASK_CROSSTALK_MAG_SLOPE= psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_CROSSTALK_MAG_SLOPE");

    // Mask stats variables
    psU16 staticMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.STATIC");
    psU16 magicMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.MAGIC");
    psU16 dynamicMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.DYNAMIC");
    psU16 advisoryMaskVal = psMetadataLookupU32(&status, recipe, "MASKSTAT.ADVISORY");

    psS32 Npix_ref_valid = 0;
    psS32 Npix_ref_static = 0;
    psS32 Npix_ref_magic = 0;
    psS32 Npix_ref_dynamic = 0;
    psS32 Npix_ref_advisory = 0;

    psS32 Npix_max_valid = 0;
    psS32 Npix_max_static = 0;
    psS32 Npix_max_magic = 0;
    psS32 Npix_max_dynamic = 0;
    psS32 Npix_max_advisory = 0;
    
    psU16 corner_list = 0x00;
    
    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }
    pmFPA *fpa = input->fpa;

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, &GHOST_MAX_MAG, NULL, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }

    psTrace("psastro.masks",2,"Configuration loaded for masking.");
    
    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    REFSTAR_MASK_MAX_MAG += MagOffset;
    REFSTAR_MASK_SATSTAR_MAG_MAX += MagOffset;
    REFSTAR_MASK_SATSPIKE_MAG_MAX += MagOffset;
    REFSTAR_MASK_BLEED_MAG_MAX += MagOffset;
    REFSTAR_MASK_BLEED_MAG_MAX_X += MagOffset;
    REFSTAR_MASK_BLEED_MAG_MAX_Y += MagOffset;
    
    GHOST_MAX_MAG += MagOffset;

    psTrace("psastro.masks",2,"Magnitudes: max_mag: %f satstar: %f satspike: %f bleed: %f ghost %f\n",
	    REFSTAR_MASK_MAX_MAG,REFSTAR_MASK_SATSTAR_MAG_MAX,REFSTAR_MASK_SATSPIKE_MAG_MAX,REFSTAR_MASK_BLEED_MAG_MAX,GHOST_MAX_MAG);
    
    // select the output mask image :: we mosaic to chip mosaic format
    pmFPAfile *outMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.OUTPUT.MASK");
    if (!outMask) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find output mask");
        return false;
    }
    pmFPA *fpaMask = outMask->fpa;

    // Get camera specific mask stat options
    psF32 FOV_REF = psMetadataLookupF32(&status, fpaMask->camera, "FOV_REF");
    psF32 FOV_MAX = psMetadataLookupF32(&status, fpaMask->camera, "FOV_MAX");
    psS32 NPIX_REF = psMetadataLookupS32(&status, fpaMask->camera, "NPIX_REF");
    psS32 NPIX_MAX = psMetadataLookupS32(&status, fpaMask->camera, "NPIX_MAX");
    
    // select the reference mask fpa :: we use this to determine cell boundaries
    pmFPAfile *refMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.REFMASK");
    if (!refMask) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find mask reference");
        return false;
    }

    psLogMsg ("psastro", PS_LOG_INFO, "loading output and reference mask files");

    // double POSANGLE = PM_RAD_DEG * psMetadataLookupF64 (&status, fpa->concepts, "FPA.POSANGLE");
    // psAssert (status, "POSANGLE missing");
    double ROTANGLE = PM_RAD_DEG * psMetadataLookupF64 (&status, fpa->concepts, "FPA.ROTANGLE");
    psAssert (status, "ROTANGLE missing");
    int ROT_PARITY = psMetadataLookupS32(&status, recipe, "PSASTRO.MODEL.ROT.PARITY");
    if (!status) psAbort ("Can't find recipe option PSASTRO.MODEL.ROT.PARITY");

    //select the astromeric rotation angle from the header
    psMetadata *header = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
    if (!header) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO header");
        return false;
    }
    float AST_T0 = PM_RAD_DEG * psMetadataLookupF32(&status, header, "AST_T0");

    // de-activate all files except PSASTRO.INPUT.MASK and PSASTRO.OUTPUT.MASK
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.INPUT.MASK");
    pmFPAfileActivate (config->files, true, "PSASTRO.OUTPUT.MASK");
    pmFPAfileActivate (config->files, true, "PSASTRO.REFMASK");

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPAview *viewMask = pmFPAviewAlloc (0);

    // open/load files as needed
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
#ifdef MASK_DEBUG
    psImage *masktest = psImageAlloc(10500,10500,PS_TYPE_U16);
    psImageMaskType **maskIData = masktest->data.PS_TYPE_IMAGE_MASK_DATA;
#endif
    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->fromFPA) { continue; }

        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

        pmChip *refChip  = pmFPAviewThisChip (view, refMask->fpa);

        // load sequence for mask corresponding to this chip (XXX this is needed if the input mask is not the same format as the astrometry file
        *viewMask = *view;
        while ((cell = pmFPAviewNextCell (viewMask, fpaMask, 1)) != NULL) {
            psTrace ("psastro", 4, "Mask Cell %d: %x %x\n", viewMask->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_BEFORE)) ESCAPE;

            while ((readout = pmFPAviewNextReadout (viewMask, fpaMask, 1)) != NULL) {
                if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_BEFORE)) ESCAPE;
                if (! readout->data_exists) { continue; }
            }
        }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // the input mask is a chip-mosaic image
            // the output mask is a chip-mosaic image
            // we mark the masked pixels in the chip space, BUT
            // we need to find the ends of the cells for the bleeds

            // we mask pixels on the input mask image (chip-mosaic)
            // pmCell *cellMask = pmFPAviewThisCell(view, outMask->fpa);
            // pmReadout *readoutMask = NULL;
            // if (cellMask->readouts->n) {
            //     readoutMask = cellMask->readouts->data[0];
            // }

            // process each of the readouts
            // XXX there can only be one readout per chip in astrometry, right?
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // XXX why not do this?
                pmReadout *readoutMask = pmFPAviewThisReadout (view, outMask->fpa);
                if (!readoutMask) continue;

                // select the raw objects for this readout
                // XXX : note that we place limits on the refstar sample in psastroChooseRefstars.c:
                // 1) on chip and 2) < PSASTRO.MAX.NREF. magnitude limits and clump exclusion are only
                // applied to the SUBSETs
                psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
                if (!refstars) continue;

		psTrace("psastro.masks",2,"In Readout loop.");
                // we need to generate the following masks regions:
                // 1) circle around the saturated stars (scaled by magnitude)
                // 2) diffraction spikes in direction ROT - ROTo
                // 3) bleed trail in the direction of the readout
                //    update: with 'burntool' applied to the data, the bleed trail mask is not needed.

                for (int i = 0; i < refstars->n; i++) {
                    pmAstromObj *ref = refstars->data[i];

                    if (COUNT_GHOSTS) {
                        if (ref->Mag > GHOST_MAX_MAG) {
                            nGhosts ++;
                        }
                    }
		    psTrace("psastro.masks",4,"In refstar loop: %d/%ld %f %f\n",
			    i,refstars->n,ref->Mag,REFSTAR_MASK_MAX_MAG);
                    if (ref->Mag > REFSTAR_MASK_MAX_MAG) {continue;}


                    // the reference magnitudes have been converted from the instrumental
                    // values supplied in the recipe to apparent mags (above)

                    // CIRCLE around the stars (scaled by magnitude)
                    float radius = REFSTAR_MASK_SATSTAR_RAD_OFFSET + pow(REFSTAR_MASK_SATSTAR_EXP,(REFSTAR_MASK_SATSTAR_MAG_MAX - ref->Mag));

                    // XXX for now, assume cell binning is 1x1 relative to chip
                    psastroMaskCircle (readoutMask->mask, starMaskValue, ref->chip->x, ref->chip->y, radius, radius);

                    for (float theta = 0.0; theta < 2*M_PI; theta += M_PI / 2.0) {
                        float Theta = theta - (ROT_PARITY * ROTANGLE) - AST_T0 - REFSTAR_MASK_SATSTAR_POS_ZERO;

                        // LINE for boundaries of the saturation spikes (scaled by magnitude)
                        //float MAG_MAX = (theta == 0.0) || (theta == M_PI) ? REFSTAR_MASK_SATSPIKE_MAG_MAX1 : REFSTAR_MASK_SATSPIKE_MAG_MAX2;

                        float MAG_MAX = REFSTAR_MASK_SATSPIKE_MAG_MAX;
			float spikeLength = pow(10,REFSTAR_MASK_SATSPIKE_MAG_SLOPE * (MAG_MAX - ref->Mag)) - REFSTAR_MASK_SATSPIKE_OFFSET;
                        float spikeWidth = 0.5*REFSTAR_MASK_SATSPIKE_WIDTH;
			if (spikeLength < 0.0) {
			  spikeLength = 0.0;
			}
			// XXX we can make the width depend on the spike as well...
			// CZW: 2011-02-18 it does so now. It's a weak function (slope = 0.01), but it should help
			if (spikeLength > REFSTAR_MASK_SATSPIKE_L0) {
			  spikeWidth += 0.5 * (spikeLength - REFSTAR_MASK_SATSPIKE_L0) * REFSTAR_MASK_SATSPIKE_WIDTH_SLOPE;
			}

                        // The length should also be a function of the image background level
			psTrace("psastro.masks",4,"Masking: Radius: %f Theta: %f Length: %f Width: %f\n",
				radius,Theta,spikeLength,spikeWidth);
			psTrace("psastro.masks",4,"Also: %f %f %f %f\n",ref->chip->x,ref->chip->y,ROTANGLE,REFSTAR_MASK_SATSTAR_POS_ZERO);
                        psastroMaskBox (readoutMask->mask, spikeMaskValue, ref->chip->x, ref->chip->y, spikeLength, spikeWidth, Theta);
                    }

                    // This masking option was needed for persistent charge trails in GPC1; it
                    // has since been replaced with 'burntool', which is applied upon readout
                    // by the camera software, and therefore is aware of the image sequence.
                    if (REFSTAR_MASK_BLEED) {
                        // convert x,y chip coordinates to cells in maskChip
                        pmCell *refCell = pmCellInChip (refChip, ref->chip->x, ref->chip->y);

                        // LINE for boundaries of the bleed lines
                        if (refCell) {
                            float xCell = 0.0;
                            float yCell = 0.0;
			    float xStart = 0.0;
			    float xEnd   = 0.0;
			    float yStart = 0.0;
			    float yEnd   = 0.0;
			    //                            float xEnd = 0.0;
			    //                            float yEnd = 0.0;
			    float width = 0.0;
			    float length = 0.0;
                            // find coordinate of star on cell
                            pmCellCoordsForChip (&xCell, &yCell, refCell, ref->chip->x, ref->chip->y);
                            // find coordinate of end-point on chip

			    if (REFSTAR_MASK_BLEED_ORIENTATION_X) {
			      length = pow(10,REFSTAR_MASK_BLEED_MAG_SLOPE_X*(REFSTAR_MASK_BLEED_MAG_MAX_X - ref->Mag));
			      width  = REFSTAR_MASK_BLEED_MAG_SLOPE  *(REFSTAR_MASK_BLEED_MAG_MAX - ref->Mag);

			      pmChipCoordsForCell (&xStart, &yStart, refCell, xCell - length, yCell - 0.5 * width);
			      pmChipCoordsForCell (&xEnd, &yEnd,     refCell, xCell + length, yCell + 0.5 * width);
			      psastroMaskRectangle (readoutMask->mask, spikeMaskValue, (int) xStart, (int) yStart, (int) xEnd, (int) yEnd + 1);
			    }
			    if (REFSTAR_MASK_BLEED_ORIENTATION_Y) {
			      length = pow(10,REFSTAR_MASK_BLEED_MAG_SLOPE_Y*(REFSTAR_MASK_BLEED_MAG_MAX_Y - ref->Mag));
			      width  = REFSTAR_MASK_BLEED_MAG_SLOPE  *(REFSTAR_MASK_BLEED_MAG_MAX - ref->Mag);

			      pmChipCoordsForCell (&xStart, &yStart, refCell, xCell - 0.5 * width, yCell - length);
			      pmChipCoordsForCell (&xEnd, &yEnd,     refCell, xCell + 0.5 * width, yCell + length);
			      psastroMaskRectangle (readoutMask->mask, spikeMaskValue, (int) xStart, (int) yStart, (int) xEnd + 1, (int) yEnd);
			    }
			    
                        }
                    }

                }

                // select the ghost object for this readout (loaded in psastroExtractGhosts.c).
                // These differ from the reference stars since the star position is not
                // contained by the readout; instead, the ghost position is predicted based on
                // the ghost model, and the ghost positions associated with a given readout are
                // supplied here.
                psArray *ghosts = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.GHOSTS");
                if (ghosts) {
                    // mask the ghosts on this readout
                    for (int i = 0; i < ghosts->n; i++) {
                        psastroGhost *ghost = ghosts->data[i];
                        //offset the ghost angles with the astrometric rotation angle
                        ghost->inner.theta = ghost->inner.theta - AST_T0;
                        ghost->outer.theta = ghost->outer.theta - AST_T0;
                        // XXX bright vs faint ghost bits? (OR with SUSPECT)
                        psastroMaskEllipticalAnnulus (readoutMask->mask, ghostMaskValue, ghost->chip->x, ghost->chip->y, ghost->inner, ghost->outer);
                    }
                }

                // Select the glint mask regions for this readout (loaded in
                // psastroChooseGlintStars.c).  These glint regions are defined as rectangular
                // boxes and are generated for each chip based on the position of the bright
                // stars beyond the edge of the focal plane.  This masking is currently very
                // GPC1-specific
                psArray *glints = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.GLINTS");
                if (glints) {
                    // mask the glints on this readout
                    for (int i = 0; i < glints->n; i++) {
                        psVector *glint = glints->data[i];
                        //psLogMsg ("psastro", 3, "glint: %f %f %f %f\n", glint->x0, glint->y0, glint->x1, glint->y1);
                        //psastroMaskRectangle (readoutMask->mask, glintMaskValue, glint->x0, glint->y0, glint->x1, glint->y1);
		        psastroMaskBox (readoutMask->mask, glintMaskValue, glint->data.F32[0], glint->data.F32[1],
				    glint->data.F32[2], glint->data.F32[3], glint->data.F32[4]);

                    }
                }
		// Because it's very GPC1-specific, handle alternate glint types here
		psArray *hsc_glints = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.GLINTS.HSC");
		if (hsc_glints) {
		  for (int i = 0; i < hsc_glints->n; i++) {
		    // Contains (start_x, start_y, length, width, theta)
		    psVector *glint = hsc_glints->data[i];

		    psTrace("psastro.masks", 4, "Masking glint %d on chip %s with params (%f,%f,%f,%f,%f)\n",
			    i, psMetadataLookupStr(&status,readout->parent->parent->concepts,"CHIP.NAME"),
			    glint->data.F32[0], glint->data.F32[1],
			    glint->data.F32[2], glint->data.F32[3], glint->data.F32[4]);

		    psastroMaskBox (readoutMask->mask, glintMaskValue, glint->data.F32[0], glint->data.F32[1],
				    glint->data.F32[2], glint->data.F32[3], glint->data.F32[4]);
		  }
		}
		
                // Select the crosstalk artifact regions for this readout, and mask a circular region
                // corresponding to the source star

                psArray *crosstalks = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.CROSSTALKS");
                if (crosstalks) {
                  for (int i = 0; i < crosstalks->n; i++) {
                    pmAstromObj *ref = crosstalks->data[i];
                    //float radius = REFSTAR_MASK_CROSSTALK_MAG_SLOPE * (REFSTAR_MASK_CROSSTALK_MAG_MAX - ref->Mag);
                    float radius = REFSTAR_MASK_SATSTAR_RAD_OFFSET + pow(REFSTAR_MASK_SATSTAR_EXP,(REFSTAR_MASK_SATSTAR_MAG_MAX - (ref->Mag+MagOffset)));

                    psTrace("psastro.crosstalk",2,"Masking star on Chip %s @ (%f,%f) Magnitude: %f Radius %f\n",
                            psMetadataLookupStr(&status,readout->parent->parent->concepts,"CHIP.NAME"),
                            ref->chip->x,ref->chip->y,ref->Mag,radius);
                    // XXX for now, assume cell binning is 1x1 relative to chip
                    //For the moment, make the crostalk masks into ovals by doing a simple 1:2 scaling with radius. Not ideal, but better until we can get more examples.
                    psastroMaskCircle (readoutMask->mask, crosstalkMaskValue, ref->chip->x, ref->chip->y, radius/2., radius);
                  }
                }
		// Crosstalk Bleeds
                psArray *bleedcrosstalks = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.CROSSTALKS.SPIKES");
                if (bleedcrosstalks) {
                  for (int i = 0; i < bleedcrosstalks->n; i++) {
                    pmAstromObj *ref = bleedcrosstalks->data[i];
                    float width = REFSTAR_MASK_BLEED_MAG_SLOPE * (REFSTAR_MASK_BLEED_MAG_MAX - ref->Mag);
                    psTrace("psastro.crosstalk",2,"Masking spike on Chip %s @ (%f,%f) Magnitude: %f Radius %f\n",
                            psMetadataLookupStr(&status,readout->parent->parent->concepts,"CHIP.NAME"),
                            ref->chip->x,ref->chip->y,ref->Mag,width);
                    // XXX for now, assume cell binning is 1x1 relative to chip
		    pmCell *refCell = pmCellInChip(refChip,ref->chip->x,ref->chip->y);
		    if (refCell) {
		      float xCell = 0.0;
		      float yCell = 0.0;
		      pmCellCoordsForChip (&xCell, &yCell, refCell, ref->chip->x, ref->chip->y);
		      int ySize = psMetadataLookupS32(NULL,refCell->concepts,"CELL.YSIZE");
/* 		      psWarning("Masking CTspike on Chip %s @ (%f,%f) Magnitude: %f (%f %f) Radius %f Z: %d %d %d %d\n", */
/* 				psMetadataLookupStr(&status,readout->parent->parent->concepts,"CHIP.NAME"), */
/* 				ref->chip->x,ref->chip->y,ref->Mag,REFSTAR_MASK_BLEED_MAG_SLOPE,REFSTAR_MASK_BLEED_MAG_MAX,width, */
/* 				(int) (ref->chip->x - 0.5 * width),   (int) (ref->chip->y - yCell), */
/* 				(int) (ref->chip->x+0.5 * width + 1), (int) (ref->chip->y + (ySize - yCell))); */
		      psastroMaskRectangle (readoutMask->mask, crosstalkMaskValue,
					    (int) (ref->chip->x - 0.5 * width),   (int) (ref->chip->y - yCell),
					    (int) (ref->chip->x+0.5 * width + 1), (int) (ref->chip->y + (ySize - yCell)));
		    }
                  }
                }

                // this probably should move into a function of its own:
                {
                    // select the raw objects for this readout, flag is they fall in a mask
                    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
                    if (!detections) continue;

                    psArray *inSources = detections->allSources;
                    psAssert (inSources, "missing sources?");

                    // create a replacement output array:
                    // psArray *outSources = psAllocArrayEmpty(100);

                    // XXX finish this: raise a bit for stars that land on certain types of masks;
                    // others (eg, bright star core) should be ignored.
                    for (int i = 0; i < inSources->n; i++) {
                        pmSource *source = inSources->data[i];

                        int xChip = source->peak->x;
                        int yChip = source->peak->y;

                        bool onChip = true;
                        onChip &= (xChip >= 0);
                        onChip &= (xChip < readoutMask->mask->numCols);
                        onChip &= (yChip >= 0);
                        onChip &= (yChip < readoutMask->mask->numRows);
                        if (!onChip) {
                            // if the source is off the edge of the chip, raise a different bit?
                            source->mode |= PM_SOURCE_MODE_OFF_CHIP;
                            continue;
                        }

                        psImageMaskType value = readoutMask->mask->data.PS_TYPE_IMAGE_MASK_DATA[yChip][xChip];
                        if (value & ghostMaskValue) {
                            source->mode |= PM_SOURCE_MODE_ON_GHOST;
                        }
                        // XXX note that for now, glint and ghost are identical
                        pmSourceMode PM_SOURCE_MODE_ON_GLINT = PM_SOURCE_MODE_ON_GHOST;
                        if (value & glintMaskValue) {
                            source->mode |= PM_SOURCE_MODE_ON_GLINT;
                        }
                        if (value & spikeMaskValue) {
                            source->mode |= PM_SOURCE_MODE_ON_SPIKE;
                        }
                    }
                }
            }
	}

	// Do the mask stats bit from cell down.
	if (psMetadataLookupBool(&status,recipe,"MASK.STATS")) {
	  if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
	  while ((cell = pmFPAviewNextCell(view, fpa, 1)) != NULL) {
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
	    if (!cell->process || !cell->file_exists) {continue; }
	    while ((readout = pmFPAviewNextReadout(view, fpa, 1)) != NULL) {
	      pmReadout *readoutMask = pmFPAviewThisReadout (view, outMask->fpa);
	      if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_BEFORE)) ESCAPE;
	      if (!readoutMask->data_exists) {continue; }

	      psPlane coordFPA;
	      psPlane coordCell;
	      
	      psImage *mask = readoutMask->mask;
	      if (!mask) {continue;}
	      psImageMaskType **maskData = mask->data.PS_TYPE_IMAGE_MASK_DATA;
	      // Dance coordinates around
	      // Calculate which corners fall within the field of view.  If this chip is fully contained, we can
	      // do a simple scan instead of checking it falls within the FOV.
	      // 0x04   0x08
	      // 0x01   0x02
	      corner_list = 0;
	      
	      coordCell.x = 0.0;
	      coordCell.y = 0.0;
	      psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell);
	      if (pow(coordFPA.x,2) + pow(coordFPA.y,2) <= pow(FOV_REF,2)) {
		corner_list = corner_list | 0x01;
	      }
	      coordCell.x = (1.0 * mask->numCols - 1.0);
	      psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell);
	      if (pow(coordFPA.x,2) + pow(coordFPA.y,2) <= pow(FOV_REF,2)) {
		corner_list = corner_list | 0x02;
	      }
	      coordCell.y = 1.0 * (mask->numRows - 1);
	      psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell);
	      if (pow(coordFPA.x,2) + pow(coordFPA.y,2) <= pow(FOV_REF,2)) {
		corner_list = corner_list | 0x08;
	      }
	      coordCell.x = 0.0;
	      psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell);
	      if (pow(coordFPA.x,2) + pow(coordFPA.y,2) <= pow(FOV_REF,2)) {
		corner_list = corner_list | 0x04;
	      }

	      // Scan over the valid regions of the image and count masked pixels
	      for (int i = 0; i < mask->numRows - 1; i++) {
		for (int j = 0; j < mask->numCols - 1; j++) {
		  coordCell.x = j;
		  coordCell.y = i;
		  coordFPA.x = 0.0;
		  coordFPA.y = 0.0;
		  int region = 0;

		  if (corner_list == 0x0f) {
		    Npix_ref_valid++;
		    Npix_max_valid++;
		    region = 1;
/* #ifdef MASK_DEBUG */
/* 		    psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell); */
/* 		    maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] = */
/* 		      maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] + 0x01; */
/* #endif */
		  }
		  if (!region) {
		    psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell);
		    if (pow(coordFPA.x,2) + pow(coordFPA.y,2) <= pow(FOV_REF,2)) {
		      Npix_ref_valid++;
		      Npix_max_valid++;
		      region = 1;
/* #ifdef MASK_DEBUG */
/* 		      maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] = */
/* 			maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] +  0x01; */
/* #endif */
		    }
		    else if (pow(coordFPA.x,2) + pow(coordFPA.y,2) <= pow(FOV_MAX,2)) {
		      Npix_max_valid++;
		      region = 2;
/* #ifdef MASK_DEBUG  */
/* 		      maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] = */
/* 			maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] + 0x01; */
/* #endif */
		    }
		  }
		  if (!region) {
		    continue;
		  }
		  
		  if (maskData[i][j] & staticMaskVal) {
#ifdef MASK_DEBUG
		    psPlaneTransformApply(&coordFPA,chip->toFPA,&coordCell);
		    maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] =
		      maskIData[(int) (coordFPA.y/4 + 5250)][(int) (coordFPA.x/4 + 5250)] + 1;
#endif
		    if (region == 1) {
		      Npix_ref_static++;
		      Npix_max_static++;
		    }
		    if (region == 2) {
		      Npix_max_static++;
		    }
		    continue;
		  }
		  if (maskData[i][j] & dynamicMaskVal) {
		    if (region == 1) {
		      Npix_ref_dynamic++;
		      Npix_max_dynamic++;
		    }
		    if (region == 2) {
		      Npix_max_dynamic++;
		    }
		    continue;
		  }
		  if (maskData[i][j] & magicMaskVal) {
		    if (region == 1) {
		      Npix_ref_magic++;
		      Npix_max_magic++;
		    }
		    if (region == 2) {
		      Npix_max_magic++;
		    }
		    continue;
		  }
		  if (maskData[i][j] & advisoryMaskVal) {
		    if (region == 1) {
		      Npix_ref_advisory++;
		      Npix_max_advisory++;
		    }
		    if (region == 2) {
		      Npix_max_advisory++;
		    }
		    continue;
		  }
		}
	      }
	    }
	  }
	}
        // output sequence for mask corresponding to this chip (XXX this may not be needed...)
        *viewMask = *view;
        while ((cell = pmFPAviewNextCell (viewMask, outMask->fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Mask Cell %d: %x %x\n", viewMask->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            while ((readout = pmFPAviewNextReadout (viewMask, outMask->fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }
                if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_AFTER)) ESCAPE;
            }
            if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_AFTER)) ESCAPE;
        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

#ifdef MASK_DEBUG
    psFits *maskFits = psFitsOpen("/data/ipp007.0/watersc1/mask.test.fits","w");
    psFitsWriteImage(maskFits,NULL,masktest,1,"mask");

    psFree(maskFits);
    psFree(masktest);
#endif
    if (COUNT_GHOSTS) {
        // save nGhosts to update header.
        psMetadata *updates = psMetadataLookupMetadata (&status, fpa->analysis, "PSASTRO.HEADER");
        if (!updates) {
            updates = psMetadataAlloc ();
            psMetadataAddMetadata (fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
            psFree (updates);
        }
        psMetadataAddS32 (updates, PS_LIST_TAIL, "NGHOSTS", PS_META_REPLACE, "total expected ghosts", nGhosts);
    }

    Npix_ref_static += (NPIX_REF - Npix_ref_valid);
    Npix_max_static += (NPIX_MAX - Npix_max_valid);
    psMetadataAddS32(stats,PS_LIST_TAIL, "MASKFRAC_REF_NPIX", 0,
		     "Number of valid pixels", Npix_ref_valid);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_REF_STATIC", 0,
		     "Fraction of pixels statically masked", (float) Npix_ref_static / NPIX_REF);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_REF_DYNAMIC", 0,
		     "Fraction of pixels dynamically masked", (float) Npix_ref_dynamic / NPIX_REF);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_REF_MAGIC", 0,
		     "Fraction of pixels magically masked", (float) Npix_ref_magic / NPIX_REF);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_REF_ADVISORY", 0,
		     "Fraction of pixels masked as an advisory", (float) Npix_ref_advisory / NPIX_REF);

    psMetadataAddS32(stats,PS_LIST_TAIL, "MASKFRAC_MAX_NPIX", 0,
		     "Number of valid pixels", Npix_max_valid);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAX_STATIC", 0,
		     "Fraction of pixels statically masked", (float) Npix_max_static / NPIX_MAX);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAX_DYNAMIC", 0,
		     "Fraction of pixels dynamically masked", (float) Npix_max_dynamic / NPIX_MAX);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAX_MAGIC", 0,
		     "Fraction of pixels magically masked", (float) Npix_max_magic / NPIX_MAX);
    psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAX_ADVISORY", 0,
		     "Fraction of pixels masked as an advisory", (float) Npix_max_advisory / NPIX_MAX);
    
    // deactivate all files
    pmFPAfileActivate (config->files, false, NULL);

    psFree (view);
    psFree (viewMask);
    return true;
}


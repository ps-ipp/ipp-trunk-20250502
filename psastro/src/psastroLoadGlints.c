/** @file psastroLoadGlints.c
 *
 *  @brief calculate glint FPA and Chip positions for the stars loaded on the FPA
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.7 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define ESCAPE(MSG) {							\
	psError(PS_ERR_UNKNOWN, false, "I/O failure in psastroMaskUpdate: %s", MSG); \
	psFree (view);							\
	return false;							\
    }

/**
 * calculate glint FPA and Chip positions for the stars loaded on the FPA
 */
bool psastroLoadGlints (pmConfig *config) {

    bool status;
    float zeropt, exptime;
    psVector *x_glint = psVectorAlloc(2,PS_TYPE_F32);
    psVector *y_glint = psVectorAlloc(2,PS_TYPE_F32);

    psLogMsg ("psastro", PS_LOG_INFO, "determine glint positions");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    bool REFSTAR_MASK_GLINTS = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_GLINTS");
    if (!REFSTAR_MASK_GLINTS) return true;

    // select relevant keywords
    double GLINT_MAX_MAG = psMetadataLookupF32 (&status, recipe, "GLINT_MAX_MAG");
    double GLINT_LENGTH_MAG_SLOPE = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_MAG_SLOPE");
    double GLINT_LENGTH_MAG_ZERO = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_MAG_ZERO");
    double glintWidth = psMetadataLookupF32 (&status, recipe, "GLINT_WIDTH");
    double pixelScale = psMetadataLookupF32 (&status, recipe, "PSASTRO.PIXEL.SCALE");

    //we will use one of the new keywords to differentiate between an old and new style glint treatment
    float glintCheck = 0;
    double GLINT_LENGTH_POS_SLOPE = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_POS_SLOPE");
    if (!status) {
        psLogMsg ("psastro", PS_LOG_INFO, "Assuming old-style glint masking");
        glintCheck = 1;
    }
    double GLINT_LENGTH_POS_REF = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_POS_REF");
    double GLINT_LENGTH_POS_CUT = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_POS_CUT");
    double GLINT_LENGTH_MIN_FPA = psMetadataLookupF32 (&status, recipe, "GLINT_LENGTH_MIN_FPA");
    if (!status) GLINT_LENGTH_MIN_FPA=1000.;
    double GLINT_ANGLE_POS_SLOPE = psMetadataLookupF32 (&status, recipe, "GLINT_ANGLE_POS_SLOPE");
    double GLINT_ANGLE_POS_REF = psMetadataLookupF32 (&status, recipe, "GLINT_ANGLE_POS_REF");


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
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }
    pmFPA *fpa = astrom->fpa;

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, &GLINT_MAX_MAG, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    GLINT_MAX_MAG += MagOffset;
    //GLINT_LENGTH_MAG_ZERO += MagOffset;

    // select the raw objects for this readout (loaded in psastroExtract.c)
    psArray *glintStars = psMetadataLookupPtr (&status, fpa->analysis, "PSASTRO.GLINT.STARS");
    if (glintStars == NULL) { 
        psLogMsg ("psastro", PS_LOG_INFO, "no glint stars found");
        return false;
    }

    // set up the chip boundary vectors.
    psastroChipBounds (fpa);

    // find the possible glint stars, and convert the position to FPA coordinates.
    // search for stars within the glint regions
    for (int i = 0; i < glintStars->n; i++) {

	pmAstromObj *star = glintStars->data[i];
	if (star->Mag > GLINT_MAX_MAG) continue; 

	// project glint star to the focal-plane
	psProject (star->TP, star->sky, fpa->toSky);
	psPlaneTransformApply (star->FP, fpa->fromTPA, star->TP);
	//fprintf (stderr, "glint: %7.2f @ %8.1f, %8.1f (%f %f) %8.1f %8.1f\n", star->Mag, star->FP->x, star->FP->y, star->sky->r * PS_DEG_RAD, star->sky->d * PS_DEG_RAD, star->TP->x,star->TP->y);

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
	    if (star->FP->x < glintRegion.x0) {continue;}
	    if (star->FP->x > glintRegion.x1) {continue;}
	    if (star->FP->y < glintRegion.y0) {continue;}
	    if (star->FP->y > glintRegion.y1) {continue;}

	    char *glintType = psMetadataLookupStr (&status, glintItem->data.md, "GLINT.TYPE");
	    if (!status) {
		psWarning ("GLINT.REGION entry is missing TYPE entry");
		continue;
	    }

	    //double glintLength = GLINT_LENGTH_MAG_SLOPE*(GLINT_LENGTH_MAG_ZERO - star->Mag);
            //glint length should depend on the brightness on image, i.e. in instrumental mag. The same instrumental mag in different filters should likely give the same glint length.
	    double glintLength = GLINT_LENGTH_MAG_SLOPE*(GLINT_LENGTH_MAG_ZERO - (star->Mag-MagOffset));
            double glintAngle = 0.;

            //Besides brightness, the length of the glints also depends on the position of the star compared to the focal plane. But, seemingly only for stars closer than about 30k pixels
            if(!glintCheck) {
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

            //do a rudimentary check of whether the glint enters the pixel FPA
            if (!strcasecmp(glintType, "TOP")    && ((star->FP->y - glintLength) > 20000.))  {continue;}
            if (!strcasecmp(glintType, "BOTTOM") && ((star->FP->y + glintLength) < -20000.))  {continue;}
            if (!strcasecmp(glintType, "LEFT")   && ((star->FP->x + glintLength) < -20000.))  {continue;}
            if (!strcasecmp(glintType, "RIGHT")  && ((star->FP->x - glintLength) > 20000.))  {continue;}

	    if (!strcasecmp(glintType, "TOP") || !strcasecmp(glintType, "BOTTOM")) {
		// We want to find the coordinates of the glint end points. However, the glint is straight off the pixel focal plane and has an angle only on the focal plane. So, first find the edge
		double xFPA0 = star->FP->x;
		double yFPA0 = star->FP->y;
		double xFPA1;
		double yFPA1;
                //angles for TOP and LEFT have been flipped in the fitting
		if (!strcasecmp(glintType, "TOP")) {
                    //find the edge chip and determine the FPA coords of the edge. Then, grab the edge coord and new glint length
		    for (int nChip = 0; nChip < fpa->chips->n; nChip++) {
 			pmChip *chip = fpa->chips->data[nChip];
 			if (!chip) {continue;}

		   	if (!psastroFindChipInXrange (fpa, nChip, star->FP->x, 20000.)) {
		   	  continue;
		   	}
		   	if (!psastroFindChipInYrange (fpa, nChip, star->FP->x, 20000.)) {
		   	  continue;
		   	}

		        // FPA coordinates of intersections with chip edges 
		        double yFPAs, yFPAe;
		        psastroFindChipYedges (&yFPAs, &yFPAe, fpa, nChip);
		        if (yFPAs > yFPAe) PS_SWAP (yFPAs, yFPAe);

  		        xFPA0 = star->FP->x;
		        yFPA0 = yFPAe;
                        glintLength-= (star->FP->y-yFPAe);
                    }

                    xFPA1 = xFPA0 + glintLength*sin(glintAngle*-1.);
                    yFPA1 = yFPA0 - glintLength*cos(glintAngle*-1.);

		} else {
                    //find the edge chip and determine the FPA coords of the edge. Then, grab the edge coord and new glint length
		    for (int nChip = 0; nChip < fpa->chips->n; nChip++) {
 			pmChip *chip = fpa->chips->data[nChip];
 			if (!chip) {continue;}

		   	if (!psastroFindChipInXrange (fpa, nChip, star->FP->x, -20000.)) {
		   	  continue;
		   	}
		   	if (!psastroFindChipInYrange (fpa, nChip, star->FP->x, -20000.)) {
		   	  continue;
		   	}

		        // FPA coordinates of intersections with chip edges 
		        double yFPAs, yFPAe;
		        psastroFindChipYedges (&yFPAs, &yFPAe, fpa, nChip);
		        if (yFPAs > yFPAe) PS_SWAP (yFPAs, yFPAe);

  		        xFPA0 = star->FP->x;
		        yFPA0 = yFPAs;
                        glintLength-= (yFPAs-star->FP->y);
                    }

                    xFPA1 = xFPA0 - glintLength*sin(glintAngle);
                    yFPA1 = yFPA0 + glintLength*cos(glintAngle);
		}

                if(glintLength < GLINT_LENGTH_MIN_FPA) {continue;}

	        x_glint->data.F32[0] = xFPA0;
	        y_glint->data.F32[0] = yFPA0;
	        x_glint->data.F32[1] = xFPA1;
	        y_glint->data.F32[1] = yFPA1;


                //we need to loop over each corner to select the chips that can have the glint on it
	        for (int glint_point = 0; glint_point < 2; glint_point++) {
		    for (int nChip = 0; nChip < fpa->chips->n; nChip++) {

 			pmChip *chip = fpa->chips->data[nChip];
 			if (!chip) {continue;}
                        const char *chipName = psMetadataLookupStr(NULL,chip->concepts, "CHIP.NAME");
                        int X = chipName[2] - '0';

		   	if (!psastroFindChipInXrange (fpa, nChip, x_glint->data.F32[glint_point], y_glint->data.F32[glint_point])) {
		   	  continue;
		   	}

		        // FPA coordinates of intersections with chip edges 
		        double yFPAs, yFPAe;
		        psastroFindChipYedges (&yFPAs, &yFPAe, fpa, nChip);

                        //For ease of reference, we adopt a frame in which coords with 0 are always lower in the relevant axis
		        if (yFPAs > yFPAe) PS_SWAP (yFPAs, yFPAe);
			if (yFPA0 > yFPA1) {
			    PS_SWAP (xFPA0, xFPA1);
			    PS_SWAP (yFPA0, yFPA1);
                        } 

		        // does this glint cross this chip?
		        if (yFPA0 > yFPAe) {continue;}
		        if (yFPA1 < yFPAs) {continue;}

                        //find the y-coord positions for this chip
                        double ycFPA0, ycFPA1; 
		        ycFPA0 = PS_MAX (yFPA0, yFPAs);
    		        ycFPA1 = PS_MIN (yFPA1, yFPAe);

                        //now calculate the proper x-coord positions given the angle, for this chip
                        double xcFPA0 = 0.0, xcFPA1 = 0.0;
			double angle = 0.0, chip_angle = 0.0, glint_length = 0.0; 
		  	angle = atan2(xFPA1 - xFPA0,yFPA1 - yFPA0);
                        xcFPA0 = xFPA0 + (ycFPA0-yFPA0)*tan(angle);
                        xcFPA1 = xFPA0 + (ycFPA1-yFPA0)*tan(angle);	

                        //use this to calculate the actual glint length as it wll be on this chip
 		  	glint_length = sqrt(pow(ycFPA1 - ycFPA0,2) + pow(xcFPA1 - xcFPA0,2));

                        //also get the proper orientation of the glint angle, and determine the starting position on this chip. Remember that half of the focal plane is flipped
		  	double xChip0, yChip0;
	                if (!strcasecmp(glintType, "TOP")) {
                          if(X<=3){
                            chip_angle = PM_RAD_DEG * (-90. - glintAngle/PM_RAD_DEG);
                          } else {
                            chip_angle = PM_RAD_DEG * (90. - glintAngle/PM_RAD_DEG);
                          }
		  	  psastroFPAtoChip (&xChip0, &yChip0, fpa, nChip, xcFPA1, ycFPA1);
                        }
	                if (!strcasecmp(glintType, "BOTTOM")) {
                          if(X<=3){
                            chip_angle = PM_RAD_DEG * (glintAngle/PM_RAD_DEG +90.);
                          } else {
                            chip_angle = PM_RAD_DEG * (glintAngle/PM_RAD_DEG -90.);
                          }
		  	  psastroFPAtoChip (&xChip0, &yChip0, fpa, nChip, xcFPA0, ycFPA0);
                        }

		   	// select the 0th readout of the 0th cell for this chip
		   	if (!chip->cells) {continue;}
		   	if (!chip->cells->n) {continue;}
		   	pmCell *glintCell = chip->cells->data[0];
		   	if (!glintCell) {continue;}
		   	if (!glintCell->readouts) {continue;}
		   	if (!glintCell->readouts->n) {continue;}
		   	pmReadout *glintReadout = glintCell->readouts->data[0];
		   	if (!glintReadout) {continue;}
		   	
		   	// save the glints on the readout->analysis metadata, creating if needed
		   	psArray *glints = psMetadataLookupPtr (&status, glintReadout->analysis, "PSASTRO.GLINTS");
		   	if (glints == NULL) { 
		   	  glints = psArrayAllocEmpty (100);
		   	  if (!psMetadataAdd (glintReadout->analysis, PS_LIST_TAIL, "PSASTRO.GLINTS", PS_DATA_ARRAY, "astrometry matches", glints)) {
		   	    psWarning("failure to add glints to readout");
		   	    psFree (glints);
		   	    continue;
		   	}
		   	  psFree (glints);
		   	}
                        psVector *glint = psVectorAlloc(5,PS_TYPE_F32);
		   	glint->data.F32[0] = xChip0;
		   	glint->data.F32[1] = yChip0;
		   	glint->data.F32[2] = glint_length;
		   	glint->data.F32[3] = glintWidth;
		   	glint->data.F32[4] = chip_angle;

		   	psArrayAdd (glints, 100, glint);

		   	psFree (glint);
	  	    }
		}
	    }

	    if (!strcasecmp(glintType, "LEFT") || !strcasecmp(glintType, "RIGHT")) {
		// We want to find the coordinates of the glint end points. However, the glint is straight off the pixel focal plane and has an angle only on the focal plane. So, first find the edge
		double xFPA0 = star->FP->x;
		double yFPA0 = star->FP->y;
		double xFPA1;
		double yFPA1;
                //angles for TOP and LEFT have been flipped in the fitting
		if (!strcasecmp(glintType, "RIGHT")) {
                    //find the edge chip and determine the FPA coords of the edge. Then, grab the edge coord and new glint length
		    for (int nChip = 0; nChip < fpa->chips->n; nChip++) {
 			pmChip *chip = fpa->chips->data[nChip];
 			if (!chip) {continue;}

		   	if (!psastroFindChipInXrange (fpa, nChip, 20000.,star->FP->y)) {
		   	  continue;
		   	}
		   	if (!psastroFindChipInYrange (fpa, nChip, 20000.,star->FP->y)) {
		   	  continue;
		   	}

		        // FPA coordinates of intersections with chip edges 
		        double xFPAs, xFPAe;
		        psastroFindChipXedges (&xFPAs, &xFPAe, fpa, nChip);
		        if (xFPAs > xFPAe) PS_SWAP (xFPAs, xFPAe);

  		        xFPA0 = xFPAe;
		        yFPA0 = star->FP->y;
                        glintLength-= (star->FP->x-xFPAe);
                    }

                    xFPA1 = xFPA0 - glintLength*cos(glintAngle);
                    yFPA1 = yFPA0 - glintLength*sin(glintAngle);

		} else {
                    //find the edge chip and determine the FPA coords of the edge. Then, grab the edge coord and new glint length
		    for (int nChip = 0; nChip < fpa->chips->n; nChip++) {
 			pmChip *chip = fpa->chips->data[nChip];
 			if (!chip) {continue;}

		   	if (!psastroFindChipInXrange (fpa, nChip, -20000.,star->FP->y)) {
		   	  continue;
		   	}
		   	if (!psastroFindChipInYrange (fpa, nChip, -20000.,star->FP->y)) {
		   	  continue;
		   	}

		        // FPA coordinates of intersections with chip edges 
		        double xFPAs, xFPAe;
		        psastroFindChipXedges (&xFPAs, &xFPAe, fpa, nChip);
		        if (xFPAs > xFPAe) PS_SWAP (xFPAs, xFPAe);

  		        xFPA0 = xFPAs;
		        yFPA0 = star->FP->y;
                        glintLength-= (xFPAs-star->FP->x);
                    }

                    xFPA1 = xFPA0 + glintLength*cos(glintAngle*-1.);
                    yFPA1 = yFPA0 + glintLength*sin(glintAngle*-1.);
		}

                if(glintLength < GLINT_LENGTH_MIN_FPA) {continue;}

	        x_glint->data.F32[0] = xFPA0;
	        y_glint->data.F32[0] = yFPA0;
	        x_glint->data.F32[1] = xFPA1;
	        y_glint->data.F32[1] = yFPA1;

                //we need to loop over each corner to select the chips that can have the glint on it
	        for (int glint_point = 0; glint_point < 2; glint_point++) {
		    for (int nChip = 0; nChip < fpa->chips->n; nChip++) {

 			pmChip *chip = fpa->chips->data[nChip];
 			if (!chip) {continue;}

		   	if (!psastroFindChipInYrange (fpa, nChip, x_glint->data.F32[glint_point], y_glint->data.F32[glint_point])) {
		   	  continue;
		   	}

		        // FPA coordinates of intersections with chip edges 
		        double xFPAs, xFPAe;
		        psastroFindChipXedges (&xFPAs, &xFPAe, fpa, nChip);

                        //For ease of reference, we adopt a frame in which coords with 0 are always lower in the relevant axis
		        if (xFPAs > xFPAe) PS_SWAP (xFPAs, xFPAe);
			if (xFPA0 > xFPA1) {
			    PS_SWAP (xFPA0, xFPA1);
			    PS_SWAP (yFPA0, yFPA1);
                        } 

		        // does this glint cross this chip?
		        if (xFPA0 > xFPAe) {continue;}
		        if (xFPA1 < xFPAs) {continue;}

                        //find the x-coord positions for this chip
                        double xcFPA0, xcFPA1; 
		        xcFPA0 = PS_MAX (xFPA0, xFPAs);
		        xcFPA1 = PS_MIN (xFPA1, xFPAe);

                        //now calculate the proper x-coord positions given the angle, for this chip
                        double ycFPA0 = 0.0, ycFPA1 = 0.0;
			double glint_length = 0.0, angle = 0.0, chip_angle = 0.0;
		  	angle = atan2(xFPA1 - xFPA0,yFPA1 - yFPA0);
                        ycFPA0 = yFPA0 + (xcFPA0-xFPA0)/tan(angle);
                        ycFPA1 = yFPA0 + (xcFPA1-xFPA0)/tan(angle);

                        //use this to calculate the actual glint length as it wll be on this chip
 		  	glint_length = sqrt(pow(ycFPA1 - ycFPA0,2) + pow(xcFPA1 - xcFPA0,2));

                        //also get the proper orientation of the glint angle, and determine the starting position on this chip. Remember that half of the focal plane is flipped
		  	double xChip0, yChip0;
	                if (!strcasecmp(glintType, "LEFT")) {
                          chip_angle = PM_RAD_DEG * (180. - glintAngle/PM_RAD_DEG);
  		  	  psastroFPAtoChip (&xChip0, &yChip0, fpa, nChip, xcFPA0, ycFPA0);
                        }
	                if (!strcasecmp(glintType, "RIGHT")) {
                          chip_angle = PM_RAD_DEG * (glintAngle/PM_RAD_DEG +180.);
		  	  psastroFPAtoChip (&xChip0, &yChip0, fpa, nChip, xcFPA1, ycFPA1);
                        }

		   	// select the 0th readout of the 0th cell for this chip
		   	if (!chip->cells) {continue;}
		   	if (!chip->cells->n) {continue;}
		   	pmCell *glintCell = chip->cells->data[0];
		   	if (!glintCell) {continue;}
		   	if (!glintCell->readouts) {continue;}
		   	if (!glintCell->readouts->n) {continue;}
		   	pmReadout *glintReadout = glintCell->readouts->data[0];
		   	if (!glintReadout) {continue;}
		   	
		   	// save the glints on the readout->analysis metadata, creating if needed
		   	psArray *glints = psMetadataLookupPtr (&status, glintReadout->analysis, "PSASTRO.GLINTS");
		   	if (glints == NULL) { 
		   	  glints = psArrayAllocEmpty (100);
		   	  if (!psMetadataAdd (glintReadout->analysis, PS_LIST_TAIL, "PSASTRO.GLINTS", PS_DATA_ARRAY, "astrometry matches", glints)) {
		   	    psWarning("failure to add glints to readout");
		   	    psFree (glints);
		   	    continue;
		   	}
		   	  psFree (glints);
		   	}
		   	
		   	psVector *glint = psVectorAlloc(5,PS_TYPE_F32);
		   	glint->data.F32[0] = xChip0;
		   	glint->data.F32[1] = yChip0;
		   	glint->data.F32[2] = glint_length;
		   	glint->data.F32[3] = glintWidth;
		   	glint->data.F32[4] = chip_angle;

		   	psArrayAdd (glints, 100, glint);

		   	psFree (glint);
	  	    }
		}
	    }


	    if (!strcasecmp(glintType, "HSC")) {
	      // It's inefficient to keep looking these up.
	      double GLINT_RADIUS_INNER = psMetadataLookupF32(&status, glintItem->data.md, "GLINT.RADIUS.INNER");
	      double GLINT_RADIUS_OUTER = psMetadataLookupF32(&status, glintItem->data.md, "GLINT.RADIUS.OUTER");
	      
	      // double R_FPA = sqrt(pow(star->FP->y,2) + pow(star->FP->x,2));
	      double R_FPA =  sqrt(pow(star->TP->y,2) + pow(star->TP->x,2));
	      //	      R_FPA *= 1.30;
	      if (R_FPA < GLINT_RADIUS_INNER) { continue; }
	      if (R_FPA > GLINT_RADIUS_OUTER) { continue; }
	      psTrace("psastro.masks",4,"HSC_GLINT_STARS: %f %f %f\n",
		      //		      star->FP->x,star->FP->y,star->Mag);
		      star->TP->x,star->TP->y,star->Mag);
	      psVector *C_terms = NULL;
	      psVector *R_terms = NULL;
	      double scale_factor = 1.0;
	      
	      char *filter = psMetadataLookupStr (&status, fpa->concepts, "FPA.FILTERID");
	      psMetadataItem *item = psMetadataLookup(glintItem->data.md, "GLINT.FILTER.TERM");
	      if (!item) {
		psLogMsg ("psastro", PS_LOG_INFO, "GLINT.FILTER.TERM data missing");
		return false;
	      }
	      if (item->type != PS_DATA_METADATA_MULTI) {
		psLogMsg ("psastro", PS_LOG_INFO, "GLINT.FILTER.TERM not multi");
		return false;
	      }
	      psListIterator *iter = psListIteratorAlloc(item->data.list,PS_LIST_HEAD, false);
	      psMetadataItem *refItem = NULL;
	      while ((refItem = psListGetAndIncrement(iter))) {
		if (refItem->type != PS_DATA_METADATA) {
		  psLogMsg ("psastro", PS_LOG_INFO, "GLINT.FILTER.TERM entry not metadata");
		  return false;
		}
		char *refFilter = psMetadataLookupStr (&status, refItem->data.md, "FILTER");
		if (!status) {
		  // psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing FILTER");
		  continue;
		}
		if (strcmp(refFilter, filter)) continue;

		C_terms = psMetadataLookupPtr(&status, refItem->data.md, "C_TERMS");
		R_terms = psMetadataLookupPtr(&status, refItem->data.md, "R_TERMS");
		scale_factor = psMetadataLookupF32(&status, refItem->data.md, "FACTOR");
		double limit = psMetadataLookupF32(&status, refItem->data.md, "LIMIT");
		limit += MagOffset;
		if (star->Mag > limit) { continue; }
	      }
	      // check 
	      double glintPixelScale = psMetadataLookupF32(&status, glintItem->data.md, "GLINT.PIXEL.SCALE");
	      //	      psMetadata *GLINT_PARAMETER_SET = psMetadataLookupPtr(&status, glintItem->data.md, "GLINT.FILTER.TERM");
	      //	      psMetadata *GLINT_PARAMETERS    = psMetadataLookupPtr(&status, GLINT_PARAMETER_SET, filter);
	      

	      //	      double theta0 = atan2(star->FP->y,star->FP->x);
	      double theta0 = atan2(star->TP->y,star->TP->x);
	      double star_radius_deg = R_FPA * glintPixelScale;

	      double C = C_terms->data.F32[0] + C_terms->data.F32[1] * star_radius_deg + C_terms->data.F32[2] * pow(star_radius_deg,2);
	      double R = R_terms->data.F32[0] + R_terms->data.F32[1] * star_radius_deg + R_terms->data.F32[2] * pow(star_radius_deg,2);

	      double inner_edge_angle, outer_edge_angle;
	      if (star_radius_deg < 0.8831) {
		outer_edge_angle = 0.0;
	      } else if (star_radius_deg < 0.9368) {
		outer_edge_angle = 46.2985 * sqrt(star_radius_deg - 0.8831);
	      } else {
		outer_edge_angle = -2.67 + 14.3 * star_radius_deg;
	      }
	      if (star_radius_deg < 0.964) {
		inner_edge_angle = -243.866 * pow(star_radius_deg - 0.932,2) + 2.4636;
	      } else {
		inner_edge_angle = 66.2061 * sqrt(star_radius_deg - 0.9635);
	      }

	      inner_edge_angle = inner_edge_angle * PS_RAD_DEG;
	      outer_edge_angle = outer_edge_angle * PS_RAD_DEG;
	      
	      // These define the ends of a single arc in arc-centric coordinates
	      double x1 = C - R * cos(inner_edge_angle);
	      double y1 =     R * sin(inner_edge_angle);
	      double x2 = C - R * cos(outer_edge_angle);
	      double y2 =     R * sin(outer_edge_angle);

	      // Create the endpoints for the pair of arcs in device coordinates (in millimeters)
	      double theta1 = theta0 + M_PI / 2.0;
	      double x1s = (x1 * cos(theta1) - y1 * sin(theta1)) * scale_factor;
	      double y1s = (y1 * cos(theta1) + x1 * sin(theta1)) * scale_factor;
	      double x1e = (x2 * cos(theta1) - y2 * sin(theta1)) * scale_factor;
	      double y1e = (y2 * cos(theta1) + x2 * sin(theta1)) * scale_factor;
	      
	      double x2s = (x1 * cos(theta1) + y1 * sin(theta1)) * scale_factor;
	      double y2s = (-1.0 * y1 * cos(theta1) + x1 * sin(theta1)) * scale_factor;
	      double x2e = (x2 * cos(theta1) + y2 * sin(theta1)) * scale_factor;
	      double y2e = (-1.0 * y2 * cos(theta1) + x2 * sin(theta1)) * scale_factor;

	      psVector *x_start = psVectorAlloc(4,PS_TYPE_F32);
	      psVector *y_start = psVectorAlloc(4,PS_TYPE_F32);
	      psVector *x_end   = psVectorAlloc(4,PS_TYPE_F32);
	      psVector *y_end   = psVectorAlloc(4,PS_TYPE_F32);
	      	      
	      // CZW: I know this looks like a typo, but trust me, it's not a typo.
	      psPlane *fp = psPlaneAlloc();
	      psPlane *tp = psPlaneAlloc();

	      fprintf(stderr,"HSC GLINT %f: x1x2(%f %f) (%f %f) x1sx1e (%f %f %f %f) x2sx2e (%f %f %f %f)\n",
		      star->Mag,
		      x1,y1,x2,y2,
		      x1s,y1s,x1e,y1e,
		      x2s,y2s,x2e,y2e);
		      
	      // NOTE: the calculations below appear to expect (x1s, y1s) in millimeters, as mentioned above.
	      // They are then scaled to camera pixel units using 0.015 millimeters / pixel, and then to
	      // TPA units using 13.5 pixels / pixel.  I do not know where the (12.78, 57.75) mm offsets
	      // come from (displacement of reference chip?)
		
	      tp->x = 13.5 * (y1s / 0.015 + 12.78);
	      tp->y = 13.5 * (x1s / -0.015 + 57.74);
	      psPlaneTransformApply(fp,fpa->fromTPA, tp);
	      x_start->data.F32[0] = fp->x;
	      y_start->data.F32[0] = fp->y;

	      tp->x = 13.5 * (y1e / 0.015 + 12.78);
	      tp->y = 13.5 * (x1e / -0.015 + 57.74);
	      psPlaneTransformApply(fp,fpa->fromTPA, tp);
	      x_end->data.F32[0]   = fp->x;
	      y_end->data.F32[0]   = fp->y;

	      x_start->data.F32[1] = x_end->data.F32[0];  
	      y_start->data.F32[1] = y_end->data.F32[0];  
	      x_end->data.F32[1]   = x_start->data.F32[0];
	      y_end->data.F32[1]   = y_start->data.F32[0];

	      tp->x = 13.5 * (y2s / 0.015 + 12.78);
	      tp->y = 13.5 * (x2s / -0.015 + 57.74);
	      psPlaneTransformApply(fp,fpa->fromTPA, tp);
	      x_start->data.F32[2] = fp->x;
	      y_start->data.F32[2] = fp->y;

	      tp->x = 13.5 * (y2e / 0.015 + 12.78);
	      tp->y = 13.5 * (x2e / -0.015 + 57.74);
	      psPlaneTransformApply(fp,fpa->fromTPA, tp);
	      x_end->data.F32[2]   = fp->x;
	      y_end->data.F32[2]   = fp->y;
	      
	      x_start->data.F32[3] = x_end->data.F32[2];  
	      y_start->data.F32[3] = y_end->data.F32[2];  
	      x_end->data.F32[3]   = x_start->data.F32[2];
	      y_end->data.F32[3]   = y_start->data.F32[2];

	      psFree(fp);
	      psFree(tp);
	      
	      for (int glint_point = 0; glint_point < 4; glint_point++) {
		for (int nChip = 0; nChip < fpa->chips->n; nChip++) {
		  pmChip *chip = fpa->chips->data[nChip];
		  if (!chip) continue;
		  
		  if (!psastroFindChipInYrange (fpa, nChip, x_start->data.F32[glint_point], y_start->data.F32[glint_point])) {
		    continue;
		  }
		  if (!psastroFindChipInXrange (fpa, nChip, x_start->data.F32[glint_point], y_start->data.F32[glint_point])) {
		    continue;
		  }

		  double xChip0, yChip0, xChip1, yChip1, chip_angle, glint_length;
		  psastroFPAtoChip (&xChip0, &yChip0, fpa, nChip, x_start->data.F32[glint_point], y_start->data.F32[glint_point]);
		  psastroFPAtoChip (&xChip1, &yChip1, fpa, nChip, x_end->data.F32[glint_point], y_end->data.F32[glint_point]);

		  chip_angle = atan2(yChip1 - yChip0, xChip1 - xChip0);
		  glint_length = sqrt(pow(yChip1 - yChip0,2) + pow(xChip1 - xChip0,2));

		  // select the 0th readout of the 0th cell for this chip
		  if (!chip->cells) continue;
		  if (!chip->cells->n) continue;
		  pmCell *glintCell = chip->cells->data[0];
		  if (!glintCell) continue;
		  if (!glintCell->readouts) continue;
		  if (!glintCell->readouts->n) continue;
		  pmReadout *glintReadout = glintCell->readouts->data[0];
		  if (!glintReadout) continue;
		  
		  // save the glints on the readout->analysis metadata, creating if needed
		  psArray *glints = psMetadataLookupPtr (&status, glintReadout->analysis, "PSASTRO.GLINTS.HSC");
		  if (glints == NULL) { 
		    glints = psArrayAllocEmpty (100);
		    if (!psMetadataAdd (glintReadout->analysis, PS_LIST_TAIL, "PSASTRO.GLINTS.HSC", PS_DATA_ARRAY, "astrometry matches", glints)) {
		      psWarning("failure to add glints to readout");
		      psFree (glints);
		      continue;
		  }
		    psFree (glints);
		  }
		  
		  fprintf (stderr, "glint %s : %d %f,%f to %f,%f (%f %f %f)\n", glintType, nChip, xChip0, yChip0, xChip1, yChip1, glint_length, glintWidth, chip_angle);
		  psTrace("psastro.masks",4,"HSC_GLINT: Star: %f %f Glint CR %f %f Parameters: %f %f %f Ends: %f %f -> %f %f Chip: %f %f -> %f %f @ %s %f\n",
			  star->FP->x,star->FP->y,
			  C,R,
			  inner_edge_angle,outer_edge_angle,theta0,
			  x_start->data.F32[glint_point],y_start->data.F32[glint_point],x_end->data.F32[glint_point],y_end->data.F32[glint_point],
			  xChip0,yChip0,xChip1,yChip1,
			  psMetadataLookupStr(&status,glintReadout->parent->parent->concepts,"CHIP.NAME"),chip_angle);
		  psVector *glint = psVectorAlloc(5,PS_TYPE_F32);
		  glint->data.F32[0] = xChip0;
		  glint->data.F32[1] = yChip0;
		  glint->data.F32[2] = glint_length * pixelScale / pixelScale;
		  glint->data.F32[3] = glintWidth;
		  glint->data.F32[4] = chip_angle;

		  psArrayAdd (glints, 100, glint);

		  psFree (glint);
		} // End loop over chips
	      } // End loop over glint endpoints

	      psFree(x_start);
	      psFree(x_end);
	      psFree(y_start);
	      psFree(y_end);
	    } // End HSC glint block
	    
	}
    }
    psastroExtractFreeChipBounds();
    return true;
}

// XXX need to place the glints on the right analysis...
// psMetadataAdd (fpa->analysis, PS_LIST_TAIL, "PSASTRO.GLINT.STARS", PS_DATA_ARRAY, "possible glint stars", glintStars);
// psFree (glintStars);



# if (0)

	    // depending on the glint type, we need to find either the chips in the row or in the column.
	    if (!strcasecmp(glintType, "LEFT") || !strcasecmp(glintType, "RIGHT")) {
		for (int nChip = 0; nChip < fpa->chips->n; nChip++) {

		    double xChip0, yChip0;
		    if (!psastroFindChipInYrange (&xChip0, &yChip0, fpa, nChip, star->FP->x, star->FP->y)) {
			continue;
		    }

		    pmChip *chip = fpa->chips->data[nChip];

		    // select the 0th readout of the 0th cell for this chip
		    if (!chip) continue;
		    if (!chip->cells) continue;
		    if (!chip->cells->n) continue;
		    pmCell *glintCell = chip->cells->data[0];
		    if (!glintCell) continue;
		    if (!glintCell->readouts) continue;
		    if (!glintCell->readouts->n) continue;
		    pmReadout *glintReadout = glintCell->readouts->data[0];
		    if (!glintReadout) continue;

		    // save the glints on the readout->analysis metadata, creating if needed
		    psArray *glints = psMetadataLookupPtr (&status, glintReadout->analysis, "PSASTRO.GLINTS");
		    if (glints == NULL) { 
			glints = psArrayAllocEmpty (100);
			if (!psMetadataAdd (glintReadout->analysis, PS_LIST_TAIL, "PSASTRO.GLINTS", PS_DATA_ARRAY, "astrometry matches", glints)) {
			    psWarning("failure to add glints to readout");
			    continue;
			}
			psFree (glints);
		    }

		    // bounds of this chip
		    psRegion *region = pmChipPixels (chip);

		    // find the coordinate of the end point
		    double xChip1, yChip1;
		    if (!strcasecmp(glintType, "RIGHT")) {
			if (!psastroFindChipInYrange (&xChip1, &yChip1, fpa, nChip, star->FP->x - glintLength, star->FP->y)) {
			    psAbort ("inconsistent chip position result"); 
			}
		    } else {
			// find the coordinate of the end point
			if (!psastroFindChipInYrange (&xChip1, &yChip1, fpa, nChip, star->FP->x + glintLength, star->FP->y)) {
			    psAbort ("inconsistent chip position result"); 
			}
		    }

		    // we have the location in chip coordinates of the two glint end-points.
		    // check if this chip overlaps this glint
		    if (xChip0 > xChip1) PS_SWAP (xChip0, xChip1);
		    if (xChip1 < region->x0) continue;
		    if (xChip0 > region->x1) continue;

		    // this glint touches this chip. calculate the start and end
		    // coordinates on this chip
		    double yChip;
		    double xChipS = PS_MAX (xChip0, region->x0);
		    double xChipE = PS_MIN (xChip1, region->x1);

		    // if the line has any tilt (in chip coordinates), interpolate for Y:
		    if (fabs(region->y1 - region->y0) > 1.0) {
			double yChipS = yChip0 + (yChip1 - yChip0) * (xChipS - xChip0) / (xChip1 - xChip0);
			double yChipE = yChip0 + (yChip1 - yChip0) * (xChipE - xChip0) / (xChip1 - xChip0);
			yChip = 0.5*(yChipS + yChipE);
		    } else {
			yChip = 0.5*(yChip0 + yChip1);
		    }

		    fprintf (stderr, "glint %s : %f,%f to %f,%f (%f - %f @ %f)\n", glintType, xChip0, yChip0, xChip1, yChip1, xChipS, xChipE, yChip);
		    psRegion *glint = psRegionAlloc(xChipS, xChipE, yChip - 0.5*glintWidth, yChip + 0.5*glintWidth);
		    psArrayAdd (glints, 100, glint);
		    psFree (glint);
		    psFree (region);
		}
	    }
# endif

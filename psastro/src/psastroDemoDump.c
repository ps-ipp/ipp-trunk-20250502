/** @file psastroMosaicDemoDump.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

/**
 * this function is used for test purposes 
 * (-trace psastro.dump.psastroAstromGuess 1)
 */
bool psastroDumpStars (psArray *stars, char *filename) {

    FILE *f = fopen (filename, "w");

    for (int i = 0; i < stars->n; i++) {
	pmAstromObj *obj = stars->data[i];

	// write out the upward projections
	fprintf (f, "%d  %f %f  %f  %f %f  %f %f  %f %f\n", i,
		 obj->sky->r, obj->sky->d, obj->Mag, 
		 obj->TP->x, obj->TP->y, 
		 obj->FP->x, obj->FP->y, 
		 obj->chip->x, obj->chip->y);
    }
    fclose (f);
    return true;
}

/** this function is used for test purposes 
 * (-trace psastro.dump.psastroAstromGuess 1)
 */
bool psastroDumpRawstars (psArray *rawstars, pmFPA *fpa, pmChip *chip) {

    char *filename = NULL;
    char *chipname = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");

    psStringAppend (&filename, "rawstars.up.%s.dat", chipname);
    FILE *f1 = fopen (filename, "w");
    psFree (filename);
    filename = NULL;

    psStringAppend (&filename, "rawstars.dn.%s.dat", chipname);
    FILE *f2 = fopen (filename, "w");
    psFree (filename);
    filename = NULL;

    for (int i = 0; i < rawstars->n; i++) {
	pmAstromObj *raw = rawstars->data[i];

	psPlane *fp = psPlaneAlloc();
	psPlane *tp = psPlaneAlloc();
	psPlane *ch = psPlaneAlloc();
			
	psProject (tp, raw->sky, fpa->toSky);
	psPlaneTransformApply (fp, fpa->fromTPA, tp);
	psPlaneTransformApply (ch, chip->fromFPA, fp);
			
	// write out the upward projections
	fprintf (f1, "%d  %f %f  %f  %f %f  %f %f  %f %f\n", i,
		 raw->sky->r, raw->sky->d, raw->Mag, 
		 raw->TP->x, raw->TP->y, 
		 raw->FP->x, raw->FP->y, 
		 raw->chip->x, raw->chip->y);
		
	// write out the downward projections
	fprintf (f2, "%d  %f %f  %f  %f %f  %f %f  %f %f\n", i,
		 raw->sky->r, raw->sky->d, raw->Mag, 
		 tp->x, tp->y, 
		 fp->x, fp->y, 
		 ch->x, ch->y);
		
	psFree (fp);
	psFree (tp);
	psFree (ch);
    }

    fclose (f1);
    fclose (f2);
    return true;
}

bool psastroDumpMatchedStars (char *filename, psArray *rawstars, psArray *refstars, psArray *match) {
    
    FILE *f = fopen (filename, "w");

    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

	fprintf (f, "%f %f  %f %f  %f %f  %f %f  %f   |   ",  
		 DEG_RAD*raw->sky->r, DEG_RAD*raw->sky->d, 
		 raw->TP->x, raw->TP->y, 
		 raw->FP->x, raw->FP->y, 
		 raw->chip->x, raw->chip->y, raw->Mag);

	fprintf (f, "%f %f  %f %f  %f %f  %f %f  %f\n", 
		 DEG_RAD*ref->sky->r, DEG_RAD*ref->sky->d, 
		 ref->TP->x, ref->TP->y, 
		 ref->FP->x, ref->FP->y, 
		 ref->chip->x, ref->chip->y, ref->Mag);
    }
    fclose (f);

    return true;
}

/**
 * this function is used for test purposes 
 * (-trace psastro.dump.psastroLoadRefstars 1)
 */
bool psastroDumpRefstars (psArray *refstars, char *filename) {

    FILE *f = fopen (filename, "w");

    for (int i = 0; i < refstars->n; i++) {
	pmAstromObj *ref = refstars->data[i];

	// write out the refstar data
	fprintf (f, "%d  %f %f  %f\n", i,
		 ref->sky->r, ref->sky->d, ref->Mag);
    }

    fclose (f);
    return true;
}

bool psastroDumpMatches (pmFPA *fpa, char *filename) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    pmFPAview *view = pmFPAviewAlloc (0);

    FILE *f = fopen (filename, "w");

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) continue;
	
	char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");

	while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) continue;

	    // process each of the readouts
	    // XXX there can only be one readout per chip, right?
	    while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
		if (! readout->data_exists) continue;

		// select the raw objects for this readout
		psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
		if (rawstars == NULL) continue;

		// select the raw objects for this readout
		psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
		if (refstars == NULL) continue;
		psTrace ("psastro", 4, "Trying %ld refstars\n", refstars->n);

		psArray *matches = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
		if (matches == NULL) continue;

		for (int i = 0; i < matches->n; i++) {
		    pmAstromMatch *match = matches->data[i];

		    pmAstromObj *raw = rawstars->data[match->raw];
		    fprintf (f, "%s  %f %f  %f %f  %f %f  %f %f  %f   |   ",  
			     chipName, DEG_RAD*raw->sky->r, DEG_RAD*raw->sky->d, 
			     raw->TP->x, raw->TP->y, 
			     raw->FP->x, raw->FP->y, 
			     raw->chip->x, raw->chip->y, raw->Mag);

		    pmAstromObj *ref = refstars->data[match->ref];
		    fprintf (f, "%f %f  %f %f  %f %f  %f %f  %f\n", 
			     DEG_RAD*ref->sky->r, DEG_RAD*ref->sky->d, 
			     ref->TP->x, ref->TP->y, 
			     ref->FP->x, ref->FP->y, 
			     ref->chip->x, ref->chip->y, ref->Mag);
		}
	    }
	}
    }
    fclose (f);
    psFree (view);
    return true;
}

/**
 * this function is used for test purposes (-trace psastro.dump 1)
 */
bool psastroDumpGradients (psArray *gradients, char *filename) {

    FILE *f = fopen (filename, "w");

    for (int i = 0; i < gradients->n; i++) {
	pmAstromGradient *gradient = gradients->data[i];

	// write out the refstar data
	fprintf (f, "%d  %f %f   %f %f  %f %f\n", i,
		 gradient->FP.x, gradient->FP.y, 
		 gradient->dTPdL.x, gradient->dTPdL.y, 
		 gradient->dTPdM.x, gradient->dTPdM.y);
    }

    fclose (f);
    return true;
}

bool psastroDumpCorners (char *filenameU, char *filenameD, pmFPA *fpa) {

  // XXX test output of chip corners based on model
  FILE *fu = fopen (filenameU, "w");
  FILE *fd = fopen (filenameD, "w");

  pmFPAview *view = pmFPAviewAlloc (0);

  float fpaAngle = PM_DEG_RAD * atan2 (fpa->toTPA->y->coeff[1][0], fpa->toTPA->x->coeff[1][0]);

  fprintf (fu, "# boresite: %f, %f @ %f\n", fpa->toSky->R*PS_DEG_RAD, fpa->toSky->D*PS_DEG_RAD, fpaAngle);
  fprintf (fd, "# boresite: %f, %f @ %f\n", fpa->toSky->R*PS_DEG_RAD, fpa->toSky->D*PS_DEG_RAD, fpaAngle);

  pmChip *chip = NULL;
  while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        if (!chip->process || !chip->file_exists || !chip->data_exists) { continue; }

	if (!chip->toFPA || !chip->fromFPA) {
	  char *name = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
	  fprintf (stderr, "no astrom model for %s, skipping\n", name);
	  continue;
	}

	// XXX write out the four corners for a test
	psRegion *region = pmChipPixels (chip);
	psPlane ptCP, ptFP, ptTP;
	psSphere ptSky;

	// UP 0,0
	ptCP.x = region->x0; ptCP.y = region->y0;
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCP);
	psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	psDeproject (&ptSky, &ptTP, fpa->toSky);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// DOWN 0,0
	psProject (&ptTP, &ptSky, fpa->toSky);
	psPlaneTransformApply (&ptFP, fpa->fromTPA, &ptTP);
	psPlaneTransformApply (&ptCP, chip->fromFPA, &ptFP);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// UP 1,0
	ptCP.x = region->x1; ptCP.y = region->y0;
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCP);
	psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	psDeproject (&ptSky, &ptTP, fpa->toSky);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// DOWN 1,0
	psProject (&ptTP, &ptSky, fpa->toSky);
	psPlaneTransformApply (&ptFP, fpa->fromTPA, &ptTP);
	psPlaneTransformApply (&ptCP, chip->fromFPA, &ptFP);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// UP 1,1
	ptCP.x = region->x1; ptCP.y = region->y1;
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCP);
	psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	psDeproject (&ptSky, &ptTP, fpa->toSky);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// DOWN 1,1
	psProject (&ptTP, &ptSky, fpa->toSky);
	psPlaneTransformApply (&ptFP, fpa->fromTPA, &ptTP);
	psPlaneTransformApply (&ptCP, chip->fromFPA, &ptFP);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// UP 0,1
	ptCP.x = region->x0; ptCP.y = region->y1;
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCP);
	psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	psDeproject (&ptSky, &ptTP, fpa->toSky);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// DOWN 0,1
	psProject (&ptTP, &ptSky, fpa->toSky);
	psPlaneTransformApply (&ptFP, fpa->fromTPA, &ptTP);
	psPlaneTransformApply (&ptCP, chip->fromFPA, &ptFP);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// UP 0,0
	ptCP.x = region->x0; ptCP.y = region->y0;
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCP);
	psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	psDeproject (&ptSky, &ptTP, fpa->toSky);
	fprintf (fu, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	// DOWN 0,0
	psProject (&ptTP, &ptSky, fpa->toSky);
	psPlaneTransformApply (&ptFP, fpa->fromTPA, &ptTP);
	psPlaneTransformApply (&ptCP, chip->fromFPA, &ptFP);
	fprintf (fd, "%10.6f %10.6f  %8.1f %8.1f  %8.1f %8.1f  %8.1f %8.1f\n", ptSky.r, ptSky.d, ptTP.x, ptTP.y, ptFP.x, ptFP.y, ptCP.x, ptCP.y);

	psFree (region);
  }

  fclose (fu);
  fclose (fd);
  psFree (view);
  return true;
}

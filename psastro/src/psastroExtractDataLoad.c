/** @file psastroExtractDataLoad.c
 *
 *  @brief
 *
 *  @ingroup psastroExtract
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroExtractDataLoad"); \
  psFree (view); \
  return false; \
}
  
/**
 * this loop loads the header data from the input files, using the output 
 * pmFPAfile to guide the chip selection and related issues
 * all of the different astrometry analysis modes use the same data load loop
 */
bool psastroExtractDataLoad (pmConfig *config) {

    bool status;
    pmChip *chip;

    bool newFPA = true;
    double RAmin  = +FLT_MAX;
    double RAmax  = -FLT_MAX;
    double DECmin = +FLT_MAX;
    double DECmax = -FLT_MAX;

    double RAminSky = NAN;
    double RAmaxSky = NAN;

    psTimerStart ("psastro");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!\n");
	return false;
    }

    // physical pixel scale in microns per pixel
    double pixelScale = psMetadataLookupF32 (&status, recipe, "PSASTRO.PIXEL.SCALE");
    if (!status) {
	psError(PS_ERR_IO, true, "Failed to lookup pixel scale"); 
	return false; 
    } 

    // select the input data sources
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.EXTRACT.ASTROM");
    if (!astrom) psAbort ("PSASTRO.EXTRACT.ASTROM not listed in config->files");
    pmFPA *fpa = astrom->fpa;

    pmFPAfileActivate(config->files, false, NULL);
    pmFPAfileActivate(config->files, true, "PSASTRO.EXTRACT.ASTROM");

    pmFPAview *view = pmFPAviewAlloc (0);

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    // load the headers from the astrometry files
    while ((chip = pmFPAviewNextChip (view, astrom->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process) { continue; }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
    }
    psLogMsg ("psastro", 3, "load headers : %f sec\n", psTimerMark ("psastro"));

    // check PHU header to see if we are using mosaic-level or per-chip astrometry
    bool bilevelAstrometry = false;
    pmHDU *phu = pmFPAviewThisPHU (view, astrom->fpa);
    if (phu) {
      char *ctype = psMetadataLookupStr (NULL, phu->header, "CTYPE1");
      if (ctype) bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
    }
    if (bilevelAstrometry) {
      pmAstromReadBilevelMosaic (astrom->fpa, phu->header);
    } 

    // apply the header astrometry to the astrometry structures
    while ((chip = pmFPAviewNextChip (view, astrom->fpa, 1)) != NULL) {
      psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
      if (!chip->process || !chip->file_exists || !chip->data_exists) { continue; }

      if (newFPA) {
	newFPA = false;
	while (fpa->toSky->R <        0) fpa->toSky->R += 2.0*M_PI;
	while (fpa->toSky->R > 2.0*M_PI) fpa->toSky->R -= 2.0*M_PI;
	RAminSky = fpa->toSky->R - M_PI;
	RAmaxSky = fpa->toSky->R + M_PI;
      }

      // read WCS data from the corresponding header
      pmHDU *hdu = pmFPAviewThisHDU (view, astrom->fpa);
      int nAstro = psMetadataLookupS32 (&status, hdu->header, "NASTRO");
      if (!nAstro) continue;

      if (bilevelAstrometry) {
	if (!pmAstromReadBilevelChip (chip, hdu->header)) {
	  psWarning("Could not get WCS information from header for chip %d, skipping", view->chip); 
	  continue;
	} 
      } else {
	if (!pmAstromReadWCS (astrom->fpa, chip, hdu->header, pixelScale)) {
	  psWarning("Could not get WCS information from header for chip %d, skipping", view->chip); 
	  continue;
	} 
      }
      
      // determine RA,DEC of 4 corners, use to find RA_MIN,MAX, DEC_MIN,MAX
      psRegion *region = pmChipPixels (chip);
      psPlane ptCH[4], ptFP, ptTP;
      psSphere ptSky;
      ptCH[0].x = region->x0;
      ptCH[0].y = region->y0;
      ptCH[1].x = region->x1;
      ptCH[1].y = region->y0;
      ptCH[2].x = region->x1;
      ptCH[2].y = region->y1;
      ptCH[3].x = region->x0;
      ptCH[3].y = region->y1;
      psFree (region);
      
      for (int i = 0; i < 4; i++) {
	psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH[i]);
	psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	psDeproject (&ptSky, &ptTP, fpa->toSky);

	// rationalize ra to sky range centered on boresite
	while (ptSky.r < RAminSky) ptSky.r += 2.0*M_PI;
	while (ptSky.r > RAmaxSky) ptSky.r -= 2.0*M_PI;

	RAmin = PS_MIN (ptSky.r, RAmin);
	RAmax = PS_MAX (ptSky.r, RAmax);
	
	DECmin = PS_MIN (ptSky.d, DECmin);
	DECmax = PS_MAX (ptSky.d, DECmax);

	psLogMsg ("psastro", 2, "chip %d (corner %d) : %f %f  -> %f %f -> %f %f -> %f %f\n",
		  view->chip, i, ptCH[i].x, ptCH[i].y, ptFP.x, ptFP.y, ptTP.x, ptTP.y, 
		  DEG_RAD*ptSky.r, DEG_RAD*ptSky.d);
      }
    }
    psLogMsg ("psastro", 3, "convert wcs terms to internal format : %f sec\n", psTimerMark ("psastro"));

    psLogMsg ("psastro", 2, "loaded raw data from %f,%f to %f,%f\n",
              DEG_RAD*RAmin, DEG_RAD*DECmin,
              DEG_RAD*RAmax, DEG_RAD*DECmax);

    psMetadataAddF32 (recipe, PS_LIST_TAIL, "RA_MIN",  PS_META_REPLACE, "", RAmin);
    psMetadataAddF32 (recipe, PS_LIST_TAIL, "RA_MAX",  PS_META_REPLACE, "", RAmax);
    psMetadataAddF32 (recipe, PS_LIST_TAIL, "DEC_MIN", PS_META_REPLACE, "", DECmin);
    psMetadataAddF32 (recipe, PS_LIST_TAIL, "DEC_MAX", PS_META_REPLACE, "", DECmax);

    psFree (view);
    return true;
}

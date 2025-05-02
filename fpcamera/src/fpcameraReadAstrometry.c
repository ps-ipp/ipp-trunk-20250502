# include "fpcamera.h"

# define ESCAPE(ERROR, MSG) { psError(ERROR, false, MSG); psFree(view); return false; }

/* \brief this function loads the astrometry calibration from the input smf file */
bool fpcameraReadAstrometry (pmFPAfile *input, pmConfig *config) {

    bool status;
    pmChip *chip = NULL;
    pmFPAview *view = NULL;

    // select the current recipe (just needed for pixel scale)
    // XXX this is defined in psastro.config : use that value instead of one defined in fpcamera.config?
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, FPCAMERA_RECIPE);
    if (!recipe) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't find FPCAMERA recipe!");

    // physical pixel scale in microns per pixel (used in case of non-bilevel astrometry)
    double pixelScale = psMetadataLookupF32 (&status, recipe, "FPCAMERA.PIXEL.SCALE");
    if (!status) ESCAPE(PS_ERR_IO, "Failed to lookup pixel scale"); 

    // de-activate all files except FPCAMERA.INPUT.ASTROM (where we get the astrometric calibration)
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "FPCAMERA.INPUT.ASTROM");

    view = pmFPAviewAlloc (0);

    // load headers for astrometry calibration
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_DATA, "failed to load header at FPA level");

    // add in chip headers
    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process) { continue; }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_DATA, "failed to load header at Chip level");
    }
    psLogMsg ("psastro", 3, "load headers : %f sec\n", psTimerMark ("psastro"));

    // reset to loop over all chips:
    pmFPAviewReset (view);

    // check PHU header to see if we are using mosaic-level or per-chip astrometry
    bool bilevelAstrometry = false;
    pmHDU *phu = pmFPAviewThisPHU (view, input->fpa);
    if (phu) {
	char *ctype = psMetadataLookupStr (NULL, phu->header, "CTYPE1");
	if (ctype) bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
    }
    if (bilevelAstrometry) {
	pmAstromReadBilevelMosaic (input->fpa, phu->header);
    } 

    // we need the min/max RA & DEC for each of the chips and for the entire FOV
    double rMinFPA = +FLT_MAX;
    double rMaxFPA = -FLT_MAX;
    double dMinFPA = +FLT_MAX;
    double dMaxFPA = -FLT_MAX;

    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
	psTrace ("fpcamera", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	if (!chip->process || !chip->file_exists || !chip->data_exists) { continue; }

	// read WCS data from the corresponding header
	pmHDU *hdu = pmFPAviewThisHDU (view, input->fpa);
	int nAstro = psMetadataLookupS32 (&status, hdu->header, "NASTRO");
	if (!nAstro) continue;
	// XXX use additional information to identify poor quality astrometry?

	if (bilevelAstrometry) {
	    if (!pmAstromReadBilevelChip (chip, hdu->header)) {
		psWarning("Could not get WCS information from header for chip %d, skipping", view->chip); 
		continue;
	    } 
	} else {
	    if (!pmAstromReadWCS (input->fpa, chip, hdu->header, pixelScale)) {
		psWarning("Could not get WCS information from header for chip %d, skipping", view->chip); 
		continue;
	    } 
	}

	// we need the min/max RA & DEC for each of the chips and for the entire FOV
	double rMinChip = +FLT_MAX;
	double rMaxChip = -FLT_MAX;
	double dMinChip = +FLT_MAX;
	double dMaxChip = -FLT_MAX;
	
	// this region defines the data area of the chip
	psRegion *region = pmChipPixels (chip);
	psPlane ptCH[4];
	
	// save the 4 corners
	ptCH[0].x = region->x0; ptCH[0].y = region->y0;
	ptCH[1].x = region->x1; ptCH[1].y = region->y0;
	ptCH[2].x = region->x1; ptCH[2].y = region->y1;
	ptCH[3].x = region->x0;	ptCH[3].y = region->y1;
	psFree (region);
	
        // report and save the current best guess for the chip 0,0 pixel coordinates
        for (int i = 0; i < 4; i++) {
            psPlane ptFP, ptTP;
            psSphere ptSky;

	    // 4 corners
            psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH[i]);
            psPlaneTransformApply (&ptTP, input->fpa->toTPA, &ptFP);
            psDeproject (&ptSky, &ptTP, input->fpa->toSky);
	    rMinChip = PS_MIN(rMinChip, DEG_RAD*ptSky.r);
	    rMaxChip = PS_MAX(rMaxChip, DEG_RAD*ptSky.r);
	    dMinChip = PS_MIN(dMinChip, DEG_RAD*ptSky.d);
	    dMaxChip = PS_MAX(dMaxChip, DEG_RAD*ptSky.d);
        }
	psMetadataAddF32 (chip->analysis, PS_LIST_TAIL, "RA_MIN",  PS_META_REPLACE, "", rMinChip);
	psMetadataAddF32 (chip->analysis, PS_LIST_TAIL, "RA_MAX",  PS_META_REPLACE, "", rMaxChip);
	psMetadataAddF32 (chip->analysis, PS_LIST_TAIL, "DEC_MIN", PS_META_REPLACE, "", dMinChip);
	psMetadataAddF32 (chip->analysis, PS_LIST_TAIL, "DEC_MAX", PS_META_REPLACE, "", dMaxChip);
	psLogMsg ("fpcamera", 3, "chip %3d = (%f,%f) - (%f,%f)\n", view->chip, rMinChip, dMinChip, rMaxChip, dMaxChip);

	rMinFPA = PS_MIN(rMinFPA, rMinChip);
	rMaxFPA = PS_MAX(rMaxFPA, rMaxChip);
	dMinFPA = PS_MIN(dMinFPA, dMinChip);
	dMaxFPA = PS_MAX(dMaxFPA, dMaxChip);
    }
    psMetadataAddF32 (input->fpa->analysis, PS_LIST_TAIL, "RA_MIN",  PS_META_REPLACE, "", rMinFPA);
    psMetadataAddF32 (input->fpa->analysis, PS_LIST_TAIL, "RA_MAX",  PS_META_REPLACE, "", rMaxFPA);
    psMetadataAddF32 (input->fpa->analysis, PS_LIST_TAIL, "DEC_MIN", PS_META_REPLACE, "", dMinFPA);
    psMetadataAddF32 (input->fpa->analysis, PS_LIST_TAIL, "DEC_MAX", PS_META_REPLACE, "", dMaxFPA);

    psLogMsg ("fpcamera", 3, "FPA FOV = (%f,%f) - (%f,%f)\n", rMinFPA, dMinFPA, rMaxFPA, dMaxFPA);
    psLogMsg ("fpcamera", 3, "convert wcs terms to internal format : %f sec\n", psTimerMark ("fpcamera"));

    psFree (view);
    return true;
}

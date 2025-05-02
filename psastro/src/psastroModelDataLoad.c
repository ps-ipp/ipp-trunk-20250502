/** @file psastroModelDataLoad.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroModelDataLoad"); \
  psFree (view); \
  return false; \
}
  
/**
 * this loop loads the header data from the input files, using the output 
 * pmFPAfile to guide the chip selection and related issues
 * all of the different astrometry analysis modes use the same data load loop
 */
bool psastroModelDataLoad (pmConfig *config) {

    bool status;
    pmChip *chip;

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
    pmFPAfile *output = psMetadataLookupPtr (NULL, config->files, "PSASTRO.OUT.MODEL");
    if (!output) psAbort ("PSASTRO.OUT.MODEL not listed in config->files");

    pmFPAview *view = pmFPAviewAlloc (0);

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    while ((chip = pmFPAviewNextChip (view, output->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process) { continue; }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
    }
    psLogMsg ("psastro", 3, "load headers : %f sec\n", psTimerMark ("psastro"));

    // we should have a number of different files stored in config->files as PSASTRO.WCS
    psMetadataItem *item = psMetadataLookup (config->files, "PSASTRO.WCS");
    if (item == NULL) psAbort("missing PSASTRO.WCS entries in config->files");
    if (item->type != PS_DATA_METADATA_MULTI) psAbort("unexpected type for PSASTRO.WCS");
    psArray *files = psListToArray (item->data.list);

    // convert the headers for the input file into fpa astrometry terms
    for (int i = 0; i < files->n; i++) {
	psMetadataItem *file = files->data[i];
	pmFPAfile *input = file->data.V;

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

	while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
	    psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	    if (!chip->process || !chip->file_exists || !chip->data_exists) { continue; }

	    // read WCS data from the corresponding header
	    pmHDU *hdu = pmFPAviewThisHDU (view, input->fpa);
	    int nAstro = psMetadataLookupS32 (&status, hdu->header, "NASTRO");
	    if (!nAstro) continue;

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
	}
    }
    psLogMsg ("psastro", 3, "convert wcs terms to internal format : %f sec\n", psTimerMark ("psastro"));

    psFree (view);
    return true;
}

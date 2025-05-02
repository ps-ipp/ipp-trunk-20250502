# include "psastroInternal.h"

bool psastroMosaicHeaders (pmConfig *config) {

    bool status = false;
    pmChip *chip = NULL;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
 	psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!\n");
	return false;
    }


    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find input data!\n");
	return false;
    }

    char *mosastro = psMetadataLookupStr (NULL, config->arguments, "MOSASTRO");

    double plateScale = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLATE.SCALE");
    if (!status) plateScale = 1.0;

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPA *fpa = input->fpa;

    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        // read WCS data from the corresponding header
        pmHDU *hdu = pmFPAviewThisHDU (view, fpa);

        pmAstromWriteBilevelChip (chip->toFPA, hdu->header, plateScale);
    }

    psMetadata *mosaic = pmAstromWriteBilevelMosaic (fpa->toSky, fpa->toTPA, plateScale);

    // XXX what is the EXTNAME??
    psFits *fits = psFitsOpen (mosastro, "w");
    psFitsWriteBlank(fits, mosaic, "");
    psFitsClose (fits);

    psFree (view);
    return true;
}

/* coordinate frame hierachy
   pixels (on a given readout)
   cell
   chip
   FP (focal plane)
   TP (tangent plane)
   sky (ra, dec)
*/

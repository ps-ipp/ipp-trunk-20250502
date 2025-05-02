/** @file psastroExtractAnalysis.c
 *
 *  @brief 
 *
 *  @ingroup psastroExtract
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
 * create a mask or mask regions based on the collection of reference stars that * are in the vicinity of each chip
 */
bool psastroExtractStars (pmConfig *config) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float zeropt, exptime;
    char extname[81];

    psLogMsg ("psastro", PS_LOG_INFO, "extracting bright-star subrasters");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    double EXTRACT_MAX_MAG = psMetadataLookupF32 (&status, recipe, "EXTRACT_MAX_MAG");

    // we have two input pmFPAfiles: PSASTRO.EXTRACT.INPUT and PSASTRO.EXTRACT.ASTROM.  both are in chip-mosaic or fpa format
    // we have already loaded the headers from PSASTRO.EXTRACT.ASTROM and determined the astrometry terms

    // select the input astrometry data (also carries the refstars)
    pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.EXTRACT.ASTROM");
    if (!astrom) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }
    pmFPA *fpa = astrom->fpa;

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.EXTRACT.INPUT");
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
        return false;
    }

    // only load data from INPUT
    pmFPAfileActivate(config->files, false, NULL);
    pmFPAfileActivate(config->files, true, "PSASTRO.EXTRACT.INPUT");

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, NULL, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
        return false;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    EXTRACT_MAX_MAG += MagOffset;

    int nExt = 0;
    int nExtGhost = 0;

    // open the output file handle: we are just saving a series of extensions to this file
    char *fileroot = psMetadataLookupStr (&status, config->arguments, "OUTPUT");
    if (!fileroot) {
	psLogMsg ("psastro", PS_LOG_INFO, "output fileroot not supplied");
	return false;
    }

    char *filename = NULL;
    psStringAppend (&filename, "%s.st.fits", fileroot);
    psFits *outStars = psFitsOpen (filename, "w");
    if (!outStars) {
	psError(PS_ERR_IO, false, "error opening file %s\n", filename);
	return false;
    }
    psFree (filename);

    filename = NULL;
    psStringAppend (&filename, "%s.gh.fits", fileroot);
    psFits *outGhosts = psFitsOpen (filename, "w");
    if (!outGhosts) {
	psError(PS_ERR_IO, false, "error opening file %s\n", filename);
	return false;
    }
    psFree (filename);

    pmFPAview *view = pmFPAviewAlloc (0);

    // generate a (very) basic header:
    psMetadata *phu = psMetadataAlloc ();
    // select the filter; default to fixed photcode and mag limit otherwise
    char *filter = psMetadataLookupStr (&status, fpa->concepts, "FPA.FILTERID");
    if (!status) ESCAPE("missing FPA.FILTER in concepts");

    psMetadataAddStr (phu, PS_LIST_TAIL, "FILTER",  0, "filter", filter);
    psMetadataAddF32 (phu, PS_LIST_TAIL, "EXPTIME", 0, "exptime", exptime);
    psMetadataAddF32 (phu, PS_LIST_TAIL, "ZEROPT",  0, "zeropt", zeropt);

    psFitsWriteBlank (outStars, phu, NULL);
    psFitsWriteBlank (outGhosts, phu, NULL);
    psFree (phu);

    // open/load files as needed
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE("failed on FPA BEFORE");

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!chip->fromFPA) { continue; }

        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE("failed on Chip BEFORE");

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // select the raw objects for this readout (loaded in psastroExtract.c)
                psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
                if (refstars == NULL) { continue; }

                // identify the bright stars of interest
                for (int i = 0; i < refstars->n; i++) {
                    pmAstromObj *ref = refstars->data[i];
                    if (ref->Mag > EXTRACT_MAX_MAG) continue;

		    pmReadout *inReadout = pmFPAviewThisReadout (view, input->fpa);
		    psImage *subraster = psastroExtractStar (inReadout->image, ref->chip->x, ref->chip->y, 200.0, 200.0);
		    if (!subraster) continue;
		    
                    // generate a (very) basic header:
                    psMetadata *header = psMetadataAlloc ();
                    psMetadataAddF32 (header, PS_LIST_TAIL, "CHIP_X",  0, "chip coordinate",        ref->chip->x);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "CHIP_Y",  0, "chip coordinate",        ref->chip->y);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "FPA_X",   0, "focal-plane coordinate", ref->FP->x);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "FPA_Y",   0, "focal-plane coordinate", ref->FP->y);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "MAG",     0, "magnitude",              ref->Mag);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "INST_MAG",0, "instrumental magnitude", ref->Mag - MagOffset);

		    snprintf (extname, 80, "extname.%05d", nExt);
		    psFitsWriteImage (outStars, header, subraster, 0, extname);
		    nExt ++;
		    
		    psFree (header);
		    psFree (subraster);
                }

                // select the raw objects for this readout (loaded in psastroExtract.c)
                psArray *ghosts = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.GHOSTS");
                if (ghosts == NULL) { continue; }

                // identify the bright stars of interest
                for (int i = 0; i < ghosts->n; i++) {
                    pmAstromObj *ghost = ghosts->data[i];

		    pmReadout *inReadout = pmFPAviewThisReadout (view, input->fpa);
		    psImage *subraster = psastroExtractStar (inReadout->image, ghost->chip->x, ghost->chip->y, 400.0, 400.0);
		    if (!subraster) continue;
		    
                    // generate a (very) basic header:
                    psMetadata *header = psMetadataAlloc ();
                    psMetadataAddF32 (header, PS_LIST_TAIL, "CHIP_X",  0, "chip coordinate",        ghost->chip->x);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "CHIP_Y",  0, "chip coordinate",        ghost->chip->y);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "FPA_X",   0, "focal-plane coordinate", ghost->FP->x);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "FPA_Y",   0, "focal-plane coordinate", ghost->FP->y);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "FPA_X0",  0, "star focal-plane coordinate", ghost->TP->x); // note : over-loaded value 
                    psMetadataAddF32 (header, PS_LIST_TAIL, "FPA_Y0",  0, "star focal-plane coordinate", ghost->TP->y); // note : over-loaded value 
                    psMetadataAddF32 (header, PS_LIST_TAIL, "MAG",     0, "magnitude",              ghost->Mag);
                    psMetadataAddF32 (header, PS_LIST_TAIL, "INST_MAG",0, "instrumental magnitude", ghost->Mag - MagOffset);

		    snprintf (extname, 80, "extname.%05d", nExtGhost);
		    psFitsWriteImage (outGhosts, header, subraster, 0, extname);
		    nExtGhost ++;
		    
		    psFree (header);
		    psFree (subraster);
                }
                if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE("failed on Readout AFTER");;
            }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE("failed on Cell AFTER");;
        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE("failed on Chip AFTER");;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE("failed on FPA AFTER");;

    psFitsClose (outStars);
    psFitsClose (outGhosts);
    psFree (view);
    return true;
}

/** @file pswarpLoop.c
 *
 *  @brief mail processing loop for pswarp
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

// We need a new PSF model for the warped frame.  It would be good to generate this analytically, but
// that's going to be tricky.  We have a list of sources, so we use those to redetermine the PSF model.

bool pswarpMakePSF (pmConfig *config, pmFPAfile *output, psMetadata *stats) {

    bool status = false;

    // load the recipe
    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    if (!psMetadataLookupBool(&status, recipe, "PSF")) {
	psLogMsg("pswarp", PS_LOG_INFO, "Skipping PSF measurement");
	return true;
    }

    // XXX move above the loop
    pmModelClassSetLimits(PM_MODEL_LIMITS_STRICT);
		
    // supply the readout and fpa of interest to psphot
    pmFPAfile *photFile = psMetadataLookupPtr(&status, config->files, "PSPHOT.INPUT");
    pmFPACopy(photFile->fpa, output->fpa);

    pmFPAview *view = pmFPAviewAlloc(0); ///< View into skycell

    pmChip *chip;
    while ((chip = pmFPAviewNextChip (view, photFile->fpa, 1)) != NULL) {
        psTrace ("pswarpMakePSF", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        pmCell *cell;
        while ((cell = pmFPAviewNextCell (view, photFile->fpa, 1)) != NULL) {
            psTrace ("pswarpMakePSF", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            pmReadout *readout;
            while ((readout = pmFPAviewNextReadout(view, photFile->fpa, 1)) != NULL) {
                if (!readout->data_exists) {
                    continue;
                }

		// grab the sources of interest from the storage location (pmFPAfile PSPHOT.INPUT.CMF)
		psArray *sources = psphotLoadPSFSources (config, view);
		if (!sources) {
		    // psError(psErrorCodeLast(), false, "No sources supplied to measure PSF");
		    psLogMsg ("psphot", 3, "no psf sources for this readout (%d %d %d)", view->chip, view->cell, view->readout);
		    continue;
		}

		// measure the PSF using these sources
		if (!psphotReadoutFindPSF(config, view, "PSPHOT.INPUT", sources)) {
		    // A failure is likely a data quality issue.  raise a quality flag, but do not skip other readouts
		    // XXX Split into multiple cases using error codes?
		    psErrorStackPrint(stderr, "Unable to determine PSF for readout (%d %d %d)", view->chip, view->cell, view->readout);
		    psWarning("Unable to determine PSF --- suspect bad data quality : readout (%d %d %d)", view->chip, view->cell, view->readout);
		    if (stats) {
			int currentQuality = psMetadataLookupS32(&status, stats, "QUALITY");
			if (currentQuality == 0) {
			    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Unable to determine PSF", psErrorCodeLast());
			}
		    }
		    psErrorClear();
		}
	    }
	}

	// Ensure seeing is carried over from the PSPHOT.INPUT chip to the output chip
	pmChip *outChip = pmFPAviewThisChip(view, output->fpa); // Chip with seeing
	psMetadataItem *item = psMetadataLookup(outChip->concepts, "CHIP.SEEING"); // Concept with seeing
	item->data.F32 = psMetadataLookupF32(&status, chip->concepts, "CHIP.SEEING");
    }

    psFree(view);
    return true;
}

# if (0)
bool pswarpMakePSF_test () {

#define PSF_SIZE 20         ///< Half-size of PSF
#define PSF_FLUX 10000      ///< Central flux for PSF
    pmChip *photChip = pmFPAviewThisChip(view, photFile->fpa);
    pmPSF *psf = psMetadataLookupPtr(NULL, photChip->analysis, "PSPHOT.PSF");
    psImage *image = psImageAlloc(2 * PSF_SIZE + 1, 2 * PSF_SIZE + 1, PS_TYPE_F32);
    psImageInit(image, 0);
    pmModel *model = pmModelFromPSFforXY(psf, PSF_SIZE, PSF_SIZE, PSF_FLUX);
    pmModelAdd(image, NULL, model, PM_MODEL_OP_FULL, 0);
    psFree(model);
    psFits *fits = psFitsOpen("psf.fits", "w");
    psFitsWriteImage(fits, NULL, image, 0, NULL);
    psFitsClose(fits);
    psFree(image);
    return true;
}
# endif

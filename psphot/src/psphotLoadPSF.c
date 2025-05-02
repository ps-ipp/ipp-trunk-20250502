# include "psphotInternal.h"

// PSPHOT.PSF.LOAD vs input file -- see note at top
bool psphotLoadPSF (pmConfig *config, const pmFPAview *view, const char *filerule) {

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

        // Generate the mask and weight images, including the user-defined analysis region of interest
        if (!psphotLoadPSFReadout (config, view, filerule, "PSPHOT.PSF.LOAD", i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to load PSF model for PSPHOT.PSF.LOAD entry %d", i);
            return false;
        }
    }
    return true;
}

// NOTE : pmPSF_IO.c functions must load the psf model onto the chip->analysis metadata because
// the I/O operation likely occurs before the readout exists.  This implementation assumes that
// a single psf model is valid for the entire set of readouts (not valid for a time series of readouts)

// XXX for now (2010.01.27), the supporting programs do not define multiple PSPHOT.PSF.LOAD
// files to go with multiple input files.  as a result, the implementation below is
// currently going to work for the case of a single input file, but will fail if we try with a
// stack of images.

// load an externally supplied psf model
bool psphotLoadPSFReadout (pmConfig *config, const pmFPAview *view, const char *outFilename, const char *inFilename, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *inFile = pmFPAfileSelectSingle(config->files, inFilename, index); // File of interest
    if (inFile == NULL) {
        psLogMsg ("psphot", 3, "no psf supplied for this chip");
        return true;
    }

    // find the currently selected readout
    pmFPAfile *outFile = pmFPAfileSelectSingle(config->files, outFilename, index); // File of interest
    if (outFile == NULL) {
        psLogMsg ("psphot", 3, "no psf supplied for this chip");
        return true;
    }

    // find the currently selected chip
    pmChip *chip = pmFPAviewThisChip (view, inFile->fpa);
    if (!chip) return false;

    // find the currently selected readout
    pmReadout *readout = pmFPAviewThisReadout (view, outFile->fpa);
    if (!readout) return false;

    // check if a PSF model is supplied by the user
    pmPSF *psf = psMetadataLookupPtr (&status, chip->analysis, "PSPHOT.PSF");
    if (psf == NULL) {
        psLogMsg ("psphot", 3, "no psf supplied for this chip");
        return true;
    }

    if (!psphotPSFstats (readout, psf)) {
        psAbort("cannot measure PSF shape terms");
    }

    psLogMsg ("psphot", 3, "using externally supplied PSF model for this readout");

    // save PSF on readout->analysis
    if (!psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot psf model", psf)) {
        psError (PSPHOT_ERR_UNKNOWN, false, "problem saving sources on readout");
        return false;
    }

    return true;
}

# include "psphotInternal.h"

pmPSF *selectPSF (pmFPAfile *fileSrc, const pmFPAview *view, psphotStackOptions *options, int index);
bool determineSeeing (pmPSF *psf, psphotStackOptions *options, int index);

// set up the PSF-matching parameters describing the input images
bool psphotStackMatchPSFsPrepare (pmConfig *config, const pmFPAview *view, psphotStackOptions *options, int index) {

    if (!options->convolve) {
	psLogMsg ("psphotStack", PS_LOG_INFO, "psf-matching NOT selected");
	return true;
    }

    pmFPAfile *fileSrc = psphotStackGetConvolveSource(config, options, index);
    if (!fileSrc) {
	psError(PSPHOT_ERR_CONFIG, false, "desired convolution source is missing");
	return false;
    }

    pmPSF *psf = selectPSF (fileSrc, view, options, index);
    if (!psf) {
	psError(PSPHOT_ERR_PSF, false, "Problem with PSF.");
	return false;
    }

    determineSeeing (psf, options, index);

    // load the sources (used to find reference sources for the kernel stamps)
    {
	pmFPAfile *inputSrc = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.SOURCES", index); // File of interest
	pmReadout *readout = pmFPAviewThisReadout(view, inputSrc->fpa); // Readout with sources
	pmDetections *detections = psMetadataLookupPtr(NULL, readout->analysis, "PSPHOT.DETECTIONS"); // Sources
	if (!detections || !detections->allSources) {
            // An input must have sources. If we got here without any the input file was likely corrupt
            psError(PSPHOT_ERR_IO, true, "Input %d has no sources.\n\tCheck all instances of: %s", index, inputSrc->origname);
            return false;
	}
	psAssert (detections->allSources, "missing sources?");
	options->sourceLists->data[index] = psMemIncrRefCounter(detections->allSources);
    }

    return true;
}

// select the corresponding input psf
pmPSF *selectPSF (pmFPAfile *fileSrc, const pmFPAview *view, psphotStackOptions *options, int index) {

    bool status;

    pmChip *chip = pmFPAviewThisChip(view, fileSrc->fpa); // The chip holds the PSF
    pmPSF *psf = psMetadataLookupPtr(&status, chip->analysis, "PSPHOT.PSF"); // PSF
    if (!psf) {
	// XXX if we were not supplied a PSF, we should be able to generate one by calling psphot
	psError(PSPHOT_ERR_PROG, true, "Unable to find PSF.");
	return NULL;
    }
    options->psfs->data[index] = psMemIncrRefCounter(psf);

    // find the image size
    pmCell *cell = pmFPAviewThisCell(view, fileSrc->fpa); // Cell of interest
    pmHDU *hdu = pmHDUFromCell(cell);
    assert(hdu && hdu->header);
    int naxis1 = psMetadataLookupS32(NULL, hdu->header, "NAXIS1"); // Number of columns
    int naxis2 = psMetadataLookupS32(NULL, hdu->header, "NAXIS2"); // Number of rows
    psAssert ((naxis1 > 0) && (naxis2 > 0), "Unable to determine size of image from PSF.");
    if (!options->numCols) {
	options->numCols = naxis1;
    } else {
	if (options->numCols != naxis1) {
	    psError (PSPHOT_ERR_CONFIG, true, "mismatched sizes (NAXIS1) for input images");
	    return NULL;
	}
    }
    if (!options->numRows) {
	options->numRows = naxis2;
    } else {
	if (options->numRows != naxis2) {
	    psError (PSPHOT_ERR_CONFIG, true, "mismatched sizes (NAXIS2) for input images");
	    return NULL;
	}
    }
    return psf;
}

// determine the input seeing
bool determineSeeing (pmPSF *psf, psphotStackOptions *options, int index) {

    // XXX set this based on the mode (+1 for poly, +0 for map)
    float xNum = PS_MAX(psf->trendNx, 1); 
    float yNum = PS_MAX(psf->trendNy, 1); // Number of realisations
    float sumFWHM = 0.0;		  // FWHM for image
    int numFWHM = 0;			  // Number of FWHM measurements
    for (float y = 0; y < yNum; y += 1.0) {
	float yPos = options->numRows * ((y + 0.5) / yNum);
	for (float x = 0; x < xNum; x++) {
	    float xPos = options->numCols * ((x + 0.5) / xNum);
	    float fwhm = pmPSFtoFWHM(psf, xPos, yPos); // FWHM for image
	    if (isfinite(fwhm)) {
		sumFWHM += fwhm;
		numFWHM++;
	    }
	}
    }
    if (numFWHM == 0) {
	options->inputSeeing->data.F32[index] = NAN;
	options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[index] = 0x02;
	psLogMsg("ppStack", PS_LOG_INFO, "Unable to measure PSF FWHM for image %d --- rejected.", index);
    } else {
	options->inputSeeing->data.F32[index] = sumFWHM / (float)numFWHM;
    }
    psLogMsg ("psphotStack", PS_LOG_INFO, "Input Seeing for %d: %f\n", index, options->inputSeeing->data.F32[index]);
    return true;
}


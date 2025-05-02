/** @file pswarpUpdateMetadata.c
 *
 *  @brief update generic metadata info for warp
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"
#define WCS_NONLIN_TOL 0.001            // Non-linear tolerance for header WCS

// once the output fpa elements have been built, loop over the fpa and generate metadata
bool pswarpUpdateMetadata (pmFPA *output, pmFPA *skycell, pmFPA *input, pmFPA *astrom, pmConfig *config, bool fullImage)  {

    pmFPAview *view = pmFPAviewAlloc(0);
    
    // check output astrometry mode
    bool bilevelAstrometry = psMetadataLookupBool (NULL, skycell->analysis, "ASTROMETRY.BILEVEL");

    psString refcat = NULL;

    if ((output)&&(output->analysis)) {    
	psMetadataItem *refItem = psMetadataLookup(output->analysis, "REFERENCE_CATALOG");
	if (refItem) {
	    refcat = psMetadataLookupStr (NULL, output->analysis, "REFERENCE_CATALOG");
	}
    }
    pmChip *chip;
    while ((chip = pmFPAviewNextChip (view, output, 1)) != NULL) {
        psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

	// use this in output metadata info (MD5 sums)
	const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");

        pmCell *cell;
        while ((cell = pmFPAviewNextCell (view, output, 1)) != NULL) {
            psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

	    // use this in output metadata info (MD5 sums)
	    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME");

            // process each of the readouts
            pmReadout *readout;
            while ((readout = pmFPAviewNextReadout(view, output, 1)) != NULL) {
		// skip empty output readouts
                if (!readout->data_exists) continue;
    
		if (fullImage) {
		    // Set covariance matrix for output
		    bool status = false;
		    psList *covariances = psMetadataLookupPtr(&status, readout->analysis, PSWARP_ANALYSIS_COVARIANCES); // Covariance matrices
		    psAssert(covariances, "Should be there");
		    psArray *covars = psListToArray(covariances); // Array of covariance matrices
		    if (covars->n) {
			psKernel *covar = psImageCovarianceAverage(covars);
			psMetadataRemoveKey(readout->analysis, PSWARP_ANALYSIS_COVARIANCES);

			// Correct covariance matrix scale for the mean (square root of the) Jacobian
			double jacobian = psMetadataLookupF64(NULL, readout->analysis, PSWARP_ANALYSIS_JACOBIAN); // Jacobian
			int goodPixels = psMetadataLookupS32(NULL, readout->analysis, PSWARP_ANALYSIS_GOODPIX);   // Good pixels
			jacobian /= goodPixels;
			readout->covariance = psImageCovarianceScale(covar, jacobian);
			psFree(covar);

			if (readout->variance) {
			    psImageCovarianceTransfer(readout->variance, readout->covariance);
			}

			psFree(covars);
		    }
		}

		// Add MD5 information for readout
		psString headerName = NULL; ///< Header name for MD5
		psVector *md5 = psImageMD5(readout->image); ///< md5 hash
		psString md5string = psMD5toString(md5); ///< String
		
		pmHDU *hdu = pmHDUFromReadout(readout);
		psStringAppend(&headerName, "MD5_%s_%s_%d", chipName, cellName, view->readout);
		psMetadataAddStr(hdu->header, PS_LIST_TAIL, headerName, PS_META_REPLACE, "Image MD5", md5string);
		psFree(md5);
		psFree(md5string);
		psFree(headerName);

		bool status;
		psString keyword = NULL;
		psString mapstring = NULL;

		// we (should) have an array of chipnames on the analysis
		psArray *chipnames = psMetadataLookupPtr(&status, readout->analysis, PSWARP_ANALYSIS_CHIPNAMES); 
		for (int i = 0; chipnames && (i < chipnames->n); i++) {
		    psStringAppend (&keyword, "SRC_%04d", i);
		    psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image", chipnames->data[i]);
		    psFree (keyword);
		}

		// we (should) have an array of chipnames on the analysis
		psArray *chipRegions = psMetadataLookupPtr(&status, readout->analysis, PSWARP_ANALYSIS_CHIPREGIONS); 
		for (int i = 0; chipRegions && (i < chipRegions->n); i++) {
		    psStringAppend (&keyword, "SEC_%04d", i);
		    psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image", chipRegions->data[i]);
		    psFree (keyword);
		}

		// we (should) also have an array of backwards maps (warp->chip) for the above chips
		psArray *backmaps  = psMetadataLookupPtr(&status, readout->analysis, PSWARP_ANALYSIS_BACKMAPS); // 
		for (int i = 0; backmaps && (i < backmaps->n); i++) {
		    psPlaneTransform *map = backmaps->data[i];		  
		    psStringAppend (&keyword, "MPX_%04d", i);
		    psStringAppend (&mapstring, "[%8.2f,%8.4f,%8.4f]", map->x->coeff[0][0], map->x->coeff[1][0], map->x->coeff[0][1]);
		    psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "warp to image map", mapstring);
		    psFree (keyword);
		    psFree (mapstring);
		    psStringAppend (&keyword, "MPY_%04d", i);
		    psStringAppend (&mapstring, "[%8.2f,%8.4f,%8.4f]", map->y->coeff[0][0], map->y->coeff[1][0], map->y->coeff[0][1]);
		    psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "warp to image map", mapstring);
		    psFree (keyword);
		    psFree (mapstring);
		}
	    }

	    if (fullImage) {
		psList *inputCells = psMetadataLookupPtr (NULL, cell->analysis, "INPUT.CELLS");
		if (!pmConceptsAverageCells(cell, inputCells, NULL, NULL, false)) {
		    psError(psErrorCodeLast(), false, "Unable to average cell concepts.");
		    psFree(view);
		    return false;
		}
	    }

	    // XXX Is this too ad-hoc?
	    psRegion *trimsec = psMetadataLookupPtr(NULL, cell->concepts, "CELL.TRIMSEC"); ///< Trim section
	    trimsec->x0 = trimsec->x1 = trimsec->y0 = trimsec->y1 = 0; ///< All pixels
	}

	// update astrometry headers
	pmHDU *hdu = pmHDUFromChip(chip); ///< HDU for source
	if (!hdu || !hdu->header) {
	    psError(PM_ERR_PROG, false, "Unable to find header for output.");
	    psFree(view);
	    return false;
	}

	if (bilevelAstrometry) {
	    if (!pmAstromWriteBilevelChip(hdu->header, chip, WCS_NONLIN_TOL)) {
		psError(psErrorCodeLast(), false, "Unable to read bilevel chip astrometry for skycell.");
		psFree(view);
		return false;
	    }
	} else {
	    // we use a default FPA pixel scale of 1.0
	    if (!pmAstromWriteWCS(hdu->header, output, chip, WCS_NONLIN_TOL)) {
		psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for skycell.");
		psFree(view);
		return false;
	    }
	}
    }

    if (!psMetadataCopy(output->concepts, input->concepts)) {
	psError(psErrorCodeLast(), false, "Unable to copy FPA concepts from input to output.");
	psFree(view);
	return false;
    }
    
    // Update ZP from the astrometry
    if (fullImage) {
	psMetadataItem *item = psMetadataLookup(output->concepts, "FPA.ZP");
	item->data.F32 = psMetadataLookupF32(NULL, astrom->concepts, "FPA.ZP");
    }

    if (refcat) {
	if ((output)&&(output->hdu)&&(output->hdu->header)) {
	    psMetadataAddStr(output->hdu->header, PS_LIST_TAIL, "PSREFCAT", PS_META_REPLACE,
			     "Reference catalog used for calibration", refcat);
	}
    }
    
    // apply the bilevel astrometry elements to the target
    if (bilevelAstrometry) {
	pmHDU *phu = pmFPAviewThisPHU(view, output); ///< Astrometry PHU
	if (!phu->header) {
	    phu->header = psMetadataAlloc ();
	}
	if (!pmAstromWriteBilevelMosaic(phu->header, output, WCS_NONLIN_TOL)) {
	    psError(psErrorCodeLast(), false, "Unable to write bilevel mosaic astrometry for skycell.");
	    psFree(view);
	    return false;
	}
    }

    psFree(view);
    return true;
}

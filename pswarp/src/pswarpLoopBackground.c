/** @file pswarpLoopBackground.c
 *
 *  @brief background model processing loop for pswarp
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

#define WCS_NONLIN_TOL 0.001            // Non-linear tolerance for header WCS
#define TESTING 0                       // Testing output?

// Loop over the inputs, warp them to the output skycell and then write out the output.
bool pswarpLoopBackground(pmConfig *config, psMetadata *stats)
{
    bool status;
    bool mdok;                          // Status of MD lookup

    if (!psMetadataLookupBool(NULL,config->arguments,"BACKGROUND.MODEL")) return true;

    int xGrid = psMetadataLookupS32(&mdok,config->arguments,"BKG.XGRID");
    int yGrid = psMetadataLookupS32(&mdok,config->arguments,"BKG.YGRID");

    // WHAT IS THIS?? should it be disabled at the end?
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INTERPOLATION.MODE", PS_META_REPLACE, "", 8);

    // load the recipe
    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    // select the input data background model
    pmFPAfile *output = psMetadataLookupPtr(&status, config->files, "PSWARP.OUTPUT.BKGMODEL");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find output data!\n");
        return false;
    }

    // use the external astrometry source if supplied
    pmFPAfile *skycell = psMetadataLookupPtr(&status, config->files, "PSWARP.SKYCELL");
    if (!skycell) {
        psError(PSWARP_ERR_DATA, true, "Cannot find output astrometry.");
        return false;
    }

    pmFPAview *view = pmFPAviewAlloc(0);

    int nInputs = psMetadataLookupS32(&status, config->arguments, "NUM_INPUTS");
    if (!status) {
        psError(PSWARP_ERR_DATA, true, "number of inputs is not defined (programming error)");
	psFree (view);
        return false;
    }

    // we are only reading the background models, not writing anything
    pmFPAfileActivate(config->files, false, NULL);
    pmFPAfileActivate(config->files, true, "PSWARP.BKGMODEL");

    psString refcat = NULL;
    // loop over this section once per input group
    for (int i = 0; i < nInputs; i++) {

	// select the input data source : we are reading & transforming pixels for the background model
	pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PSWARP.BKGMODEL", i);
	if (!input) {
	    psError(PSWARP_ERR_CONFIG, true, "Can't find input data!\n");
	    return false;
	}

	// select the input data sources
	pmFPAfile *astrom = pmFPAfileSelectSingle(config->files, "PSWARP.ASTROM", i);
	if (!astrom) {
	    astrom = input;
	}

	// check input astrometry mode
	bool bilevelAstrometry = psMetadataLookupBool (NULL, astrom->fpa->analysis, "ASTROMETRY.BILEVEL");
	if (bilevelAstrometry) {
	    // top-level elements of the input model astrometry
	    input->fpa->toTPA = psMemIncrRefCounter (astrom->fpa->toTPA);
	    input->fpa->fromTPA = psMemIncrRefCounter (astrom->fpa->fromTPA);
	    input->fpa->toSky = psMemIncrRefCounter (astrom->fpa->toSky);
	}

	pmFPAviewReset (view);

	pmFPAview *viewT2 = pmFPAviewAlloc(0);
	if (!pmFPAfileIOChecks(config, viewT2, PM_FPA_BEFORE)) {
	    psError(psErrorCodeLast(), false, "Unable to read files.");
	    psFree (viewT2);
	    goto FAIL;
	}

	pmChip *chip;
	while ((chip = pmFPAviewNextChip (viewT2, input->fpa, 1)) != NULL) {
	    if (!chip->process || !chip->file_exists) { continue; }
	    if (!pmFPAfileIOChecks(config, viewT2, PM_FPA_BEFORE)) {
		psError(psErrorCodeLast(), false, "Unable to read files.");
		psFree (viewT2);
		goto FAIL;
	    }

	    // adjust output astrometry
	    double xBin = NAN;
	    double yBin = NAN;
	    pswarpGetInputScales (&xBin, &yBin, config, viewT2, chip);
	    double fluxRatio = xBin * yBin / (float) (xGrid * yGrid);
	    
	    if (!pswarpModifyChipAstrom (config, viewT2, chip, astrom, bilevelAstrometry, xBin, yBin)) {
		psError(psErrorCodeLast(), false, "failed to set BKGMODEL astrometry.");
		psFree (viewT2);
		goto FAIL;
	    }

	    pmCell *cell;
	    while ((cell = pmFPAviewNextCell (viewT2, input->fpa, 1)) != NULL) {
		psTrace ("pswarp", 4, "ACell %d: %x %x %d\n", viewT2->cell, cell->file_exists, cell->process,psErrorCodeLast());
		if (!cell->process || !cell->file_exists) { continue; }
		if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_BEFORE)) {
		    psError(psErrorCodeLast(), false, "Unable to read files.");
		    psFree (viewT2);
		    goto FAIL;
		}

		// process each of the readouts
		pmReadout *readout;
		while ((readout = pmFPAviewNextReadout(viewT2, input->fpa, 1)) != NULL) {
		    if (!pmFPAfileIOChecks(config, viewT2, PM_FPA_BEFORE)) {
			psError(psErrorCodeLast(), false, "Unable to read files.");
			psFree (viewT2);
			goto FAIL;
		    }
		    if (!readout->data_exists) {
			continue;
		    }

		    if (astrom != input) {
			pmReadout *astromRO = pmFPAviewThisReadout(viewT2, astrom->fpa); // Readout for astrometry
			if ((!refcat)&&(astromRO)) {
			    if ((astromRO->parent->parent->hdu->header)&&(output->fpa->analysis)) {
				psMetadataItem *refItem = psMetadataLookup(astromRO->parent->parent->hdu->header, "PSREFCAT");
				if (refItem) {
				    refcat = psMetadataLookupStr(NULL, astromRO->parent->parent->hdu->header, "PSREFCAT");
				    psMetadataAddStr(output->fpa->analysis, PS_LIST_TAIL, "REFERENCE_CATALOG", PS_META_REPLACE,
						     "Reference catalog used for calibration.", refcat);
				}
			    }
			}
		    }
		    
		    // re-normalize the BKGMODEL pixels by modified astrometry
		    for (int x = 0; x < readout->image->numCols; x++) {
			for (int y = 0; y < readout->image->numRows; y++) {
			    readout->image->data.F32[y][x] *= fluxRatio;
			}
		    }
		
		    // transform the actual BKGMODEL pixels from readout to output
		    pswarpTransformToTarget (output->fpa, readout, config, true);

		    if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_AFTER)) { // Readout
			psError(psErrorCodeLast(), false, "Unable to close files.");
			psFree (viewT2);
			goto FAIL;
		    }
		}

		if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_AFTER)) { // Cell
		    psError(psErrorCodeLast(), false, "Unable to close files.");
		    psFree (viewT2);
		    goto FAIL;
		}
	    }

	    if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_AFTER)) { // Chip
		psError(psErrorCodeLast(), false, "Unable to close files.");
		psFree (viewT2);
		goto FAIL;
	    }
	}

	if (!pmFPAfileIOChecks (config, viewT2, PM_FPA_AFTER)) { // FPA
	    psError(psErrorCodeLast(), false, "Unable to close files.");
	    psFree (viewT2);
	    goto FAIL;
	}

	if (!pswarpUpdateMetadata (output->fpa, skycell->fpa, input->fpa, astrom->fpa, config, false)) {
	    psError(psErrorCodeLast(), false, "problem generating statistics.");
	    psFree (viewT2);
	    goto FAIL;
	}
	psFree (viewT2);
    }

    psFree (view);
    return true;

FAIL:
    psFree (view);
    return false;
}

// **** input == (small) BKGMODEL, astrom == (fullsize) ASTROM
// use astrometry from input ref astrometry and BINNING info from input model to modify the astrometry 
// elements of input chip.  scale is the area of an input pixel is raw image pixels
bool pswarpGetInputScales (double *xBin, double *yBin, pmConfig *config, pmFPAview *view, pmChip *chip) {

    // Pull information from the header of the background files so we can use it to set values.
    pmHDU *hdu = pmFPAviewThisHDU(view, chip->parent);
    psMetadata *header = hdu->header;
	
    int IMAXIS1 = psMetadataLookupS32(NULL,header,"IMNAXIS1");
    int IMAXIS2 = psMetadataLookupS32(NULL,header,"IMNAXIS2");
    int NAXIS1 = psMetadataLookupS32(NULL,header,"NAXIS1");
    int NAXIS2 = psMetadataLookupS32(NULL,header,"NAXIS2");
    char *CCDSUM = psMetadataLookupStr(NULL,header,"CCDSUM");
    int CCDSUM1 = atoi(strtok(CCDSUM," "));
    int CCDSUM2 = atoi(strtok(NULL," "));
	
    psMetadataAddF32(config->arguments,PS_LIST_TAIL,"BKG_WARP_XOFFSET", PS_META_REPLACE, "xoffset for background model data", (NAXIS1 * CCDSUM1 - IMAXIS1) / (2.0 * CCDSUM1));
    psMetadataAddF32(config->arguments,PS_LIST_TAIL,"BKG_WARP_YOFFSET", PS_META_REPLACE, "yoffset for background model data", (NAXIS2 * CCDSUM2 - IMAXIS2) / (2.0 * CCDSUM2));

    psTrace("pswarp",5,"%d %d %d %d %d %d %g %g %d %d",
	    psMetadataLookupS32(NULL,header,"IMNAXIS1"),
	    psMetadataLookupS32(NULL,header,"IMNAXIS2"),
	    psMetadataLookupS32(NULL,header,"NAXIS1"),
	    psMetadataLookupS32(NULL,header,"NAXIS2"),
	    CCDSUM1,CCDSUM2,
	    psMetadataLookupF32(NULL,config->arguments,"BKG_WARP_XOFFSET"),
	    psMetadataLookupF32(NULL,config->arguments,"BKG_WARP_YOFFSET"),
	    psMetadataLookupS32(NULL,config->arguments,"BKG.XGRID"),
	    psMetadataLookupS32(NULL,config->arguments,"BKG.YGRID"));

    *xBin = CCDSUM1;
    *yBin = CCDSUM2;

    return true;
}

// **** input == (small) BKGMODEL, astrom == (fullsize) ASTROM
// use astrometry from input ref astrometry and BINNING info from input model to modify the astrometry 
// elements of input chip.  scale is the area of an input pixel is raw image pixels
bool pswarpModifyChipAstrom (pmConfig *config, pmFPAview *view, pmChip *chip, pmFPAfile *astrom, bool bilevelAstrometry, double xBin, double yBin) {

    // read WCS data from the corresponding header 
    pmHDU *hdu = pmFPAviewThisHDU (view, astrom->fpa);

    // generate a WCS structure from the ASTROM header keywords
    pmAstromWCS *WCS = pmAstromWCSfromHeader(hdu->header);

    // re-scale the terms of the WCS to match the BKGMODEL scale
    double cd1f = xBin;
    double cd2f = yBin;

    WCS->cdelt1 *= cd1f;
    WCS->cdelt2 *= cd2f;
    WCS->crpix1 = WCS->crpix1 / cd1f;
    WCS->crpix2 = WCS->crpix2 / cd2f;

    // WCS->trans->x->nX/nY
    // adjust the scale of the WCS coeffs
    for (int q = 0; q <= WCS->trans->x->nX; q++) {
	for (int r = 0; r <= WCS->trans->x->nY; r++) {
	    WCS->trans->x->coeff[q][r] *= pow(cd1f,q) * pow(cd2f,r);
	}
    }
    for (int q = 0; q <= WCS->trans->y->nX; q++) {
	for (int r = 0; r <= WCS->trans->y->nY; r++) {
	    WCS->trans->y->coeff[q][r] *= pow(cd1f,q) * pow(cd2f,r);
	}
    }

    // this is a bit crude : use a locally temporary header to store the WCS information
    // which is then applied to the input chip (why not modify the input chip header?)
    psMetadata *tmpHeader = psMetadataAlloc();

    // write the modified WCS to the header
    // XXX this should probably not modify the astrom header
    pmAstromWCStoHeader (tmpHeader, WCS);
	
    // use the modified header to update the WCS elements of the BKGMODEL fpa/chip structures
    if (bilevelAstrometry) {
	if (!pmAstromReadBilevelChip (chip, tmpHeader)) {
	    psError(psErrorCodeLast(), false, "Unable to read bilevel chip astrometry for input FPA.");
	    return false;
	}
    } else {
	// we use a default FPA pixel scale of 1.0
	if (!pmAstromReadWCS (chip->parent, chip, tmpHeader, 1.0)) {
	    psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for input FPA.");
	    return false;
	}
    }

    return true;
}

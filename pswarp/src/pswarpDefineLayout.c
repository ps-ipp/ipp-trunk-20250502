/** @file pswarpDefineLayout.c
 *
 *  @brief load input & output astrometry, determine overlaps
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

bool pswarpGenerateOutputCell (pmChip *chip, pmCell *cell, pmCell *refcell, float xBin, float yBin);

// load the astrometry header info and generate the tranformations 
bool pswarpDefineLayout (pmConfig *config) {

    bool status = false;

    pmFPAfile *output = psMetadataLookupPtr(&status, config->files, "PSWARP.OUTPUT");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find output data!\n");
        return false;
    }
    // select the input data sources
    pmFPAfile *skycell = psMetadataLookupPtr (&status, config->files, "PSWARP.SKYCELL");
    if (!skycell) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find skycell data!\n");
        return false;
    }
    if (!pswarpLoadAstrometry (output, skycell, config)) {
        psError(PSWARP_ERR_CONFIG, false, "problem loading output astrometry\n");
        return false;
    }

    // if a background model is supplied and the output is requested, then 
    // this will exist.  if not, skip related features 
    pmFPAfile *bkgModel = psMetadataLookupPtr(&status, config->files, "PSWARP.OUTPUT.BKGMODEL");

    // chips are not processed unless we have determined they overlap the inputs
    pmFPAExcludeChips (output->fpa);
    if (bkgModel) {
        pmFPAExcludeChips (bkgModel->fpa);
    }

    int nInputs = psMetadataLookupS32(&status, config->arguments, "NUM_INPUTS");
    if (!status) {
        psError(PSWARP_ERR_DATA, true, "number of inputs is not defined (programming error)");
        return false;
    }

    for (int i = 0; i < nInputs; i++) {
	// place input astrometry transformations in 'input'
	pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PSWARP.INPUT", i);
	if (!input) {
	    psError(PSWARP_ERR_CONFIG, false, "Can't find input data!\n");
	    return false;
	}

	// input astrometry may be embedded in 'input' or supplied separately
	pmFPAfile *astrom = pmFPAfileSelectSingle(config->files, "PSWARP.ASTROM", i);
	if (!astrom) {
	    astrom = input;
	}
	if (astrom->camera != input->camera) {
	    // XXX this may cause an error for output / skycell
	    psError(PSWARP_ERR_DATA, true, "Input camera and astrometry camera do not match.");
	    return false;
	}
	if (!pswarpLoadAstrometry (input, astrom, config)) {
	    psError(PSWARP_ERR_CONFIG, false, "problem loading input astrometry\n");
	    return false;
	}

	// given the input data, determine which output elements should be generated
	// and generate the appropriate images

	// find the R,D center for the skycell, use for common projection
	psProjection *frame = pswarpLocalFrame (skycell->fpa);

	// generate Lmin,max, Mmin,max for both datasets
	pswarpBounds *srcBounds = pswarpMakeBounds (input->fpa, frame);
	pswarpBounds *tgtBounds = pswarpMakeBounds (skycell->fpa, frame);

	psFree (frame);

	// find the output (tgt) chips which overlap the input (src) chips
	pswarpFindOverlap (input->fpa, output->fpa, srcBounds, tgtBounds);
	psFree (srcBounds);
	psFree (tgtBounds);
    }

    // The loop below generates the output pixels. XXX Should this be deferred until we
    // actually need them?  

    // Generate the output chips (pixels on output->fpa, concepts from skycell->fpa)
    pmFPAview *view = pmFPAviewAlloc(0);

    // check input astrometry mode
    bool bilevelAstrometry = psMetadataLookupBool (NULL, skycell->fpa->analysis, "ASTROMETRY.BILEVEL");
    
    // XXX this is a bit muddled : we are setting up the astrometry elements for the output
    // background model here instead of in pswarpLoadAstrometry, where it would be more natural
    // the only explanation I have is that 1) I need to have the binning factors for the input image, but 
    // 2) at this point I don't yet have the input background models loaded and 3) the source
    // for the binning factor of the input and output background models is different
    // (header->CCDSUMi for input, recipe / pmFPAfile.xbin.ybin for the other)

    // if we have an output background model, generate hdu, etc and supply astrometry
    if (bkgModel && bilevelAstrometry) {
	// top-level elements of the bkgModel astrometry
	bkgModel->fpa->toTPA   = psMemIncrRefCounter (skycell->fpa->toTPA);
	bkgModel->fpa->fromTPA = psMemIncrRefCounter (skycell->fpa->fromTPA);
	bkgModel->fpa->toSky   = psMemIncrRefCounter (skycell->fpa->toSky);
    }

    pmChip *chip;
    while ((chip = pmFPAviewNextChip (view, output->fpa, 1)) != NULL) {
	psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	if (!chip->process) { continue; }
	pmCell *cell;
	while ((cell = pmFPAviewNextCell (view, output->fpa, 1)) != NULL) {
	    psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
	    if (!cell->process) { continue; }

	    pmCell *refcell = pmFPAviewThisCell(view, skycell->fpa); ///< Target cell

	    if (!pswarpGenerateOutputCell (chip, cell, refcell, output->xBin, output->yBin)){
		psError(PSWARP_ERR_DATA, false, "failed to generate output cell");
		psFree(view);
		return false;
	    }

	    if (bkgModel) {
		pmChip *bkgChip = pmFPAviewThisChip(view, bkgModel->fpa); ///< Target cell
		pmCell *bkgCell = pmFPAviewThisCell(view, bkgModel->fpa); ///< Target cell
		if (!pswarpGenerateOutputCell (bkgChip, bkgCell, refcell, bkgModel->xBin, bkgModel->yBin)) {
		    psError(PSWARP_ERR_DATA, false, "failed to generate output cell");
		    psFree(view);
		    return false;
		}
		if (!pswarpModifyChipAstrom (config, view, bkgChip, skycell, bilevelAstrometry, bkgModel->xBin, bkgModel->yBin)) {
		    psError(PSWARP_ERR_DATA, false, "problem with output astrometry");
		    psFree(view);
		    return false;
		}
	    }
	}
    }
    psFree (view);
    return true;
}

bool pswarpGenerateOutputCell (pmChip *chip, pmCell *cell, pmCell *refcell, float xBin, float yBin) {

    bool status = false;

    // we've got the output astrom header
    pmHDU *hdu = pmHDUFromCell(refcell); ///< HDU for source
    if (!hdu || !hdu->header) {
	psError(PM_ERR_PROG, false, "Unable to find header for sky cell.");
	return false;
    }
    int numCols = psMetadataLookupS32(&status, hdu->header, "NAXIS1") / xBin; ///< Number of output columns
    int numRows = psMetadataLookupS32(&status, hdu->header, "NAXIS2") / yBin; ///< Number of output rows
    if ((numCols == 0) || (numRows == 0)) {
	psError(PSWARP_ERR_DATA, false, "output cell has invalid dimensions %d x %d", numCols, numRows);
	return false;
    }
		
    // generate the pixels for output->fpa
    pmReadout *readout = pmReadoutAlloc(cell); ///< output readout
    readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);

    psImageInit(readout->image, NAN);
    psFree(readout);                // Drop reference (saved on cell)
		
    // copy the image concepts from the skycell 
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.XBIN");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.YBIN");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.XSIZE");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.YSIZE");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.XPARITY");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.YPARITY");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.X0");
    psMetadataItemSupplement(&status, cell->concepts, refcell->concepts, "CELL.Y0");

    chip->data_exists = true;
    chip->file_exists = true;
    chip->process     = true;

    cell->data_exists = true;
    cell->file_exists = true;
    cell->process     = true;

    // copy the basic headers across from astrom ref to output
    pmHDU *outHDU = pmHDUFromCell (cell);           ///< HDU for the output warped image
    outHDU->header = psMetadataCopy(outHDU->header, hdu->header);
    pswarpVersionHeader(outHDU->header);
    return true;
}

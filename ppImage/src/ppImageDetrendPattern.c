#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

#define ESCAPE(STATUS,...) {				\
        psError(PS_ERR_UNKNOWN, STATUS, __VA_ARGS__);   \
        psFree(view);                                   \
        return false;                                   \
    }

static bool doPatternForView (bool *doit, const pmConfig *config, const pmChip *chip, const pmFPAview *view, const char *recipename, const char *recipevalue);

bool ppImageDetrendPatternRowApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options);
bool ppImageDetrendPatternContinuityApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options);
bool ppImageDetrendPatternCellApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options);
bool ppImageDetrendPatternDeadCellsApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options);

// Apply the desired pattern corrections (if any).  This function is passed the chip for the
// image being processed, but may interact with pmFPAfiles containing the pattern reference information
bool ppImageDetrendPatternApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options)
{
    assert(inputView->chip != -1);
    assert(inputView->cell == -1);
    assert(inputView->readout == -1);

    if (!ppImageDetrendPatternRowApply (config, chip, inputView, options)) {
	return false;
    }
    if (!ppImageDetrendPatternContinuityApply (config, chip, inputView, options)) {
	return false;
    }
    if (!ppImageDetrendPatternCellApply (config, chip, inputView, options)) {
	return false;
    }
    if (!ppImageDetrendPatternDeadCellsApply (config, chip, inputView, options)) {
	return false;
    }
    return(true);
}

bool ppImageDetrendPatternRowApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options)
{
    bool status;

    if (!options->doPatternRow) return true;
    
    assert(inputView->chip != -1);
    assert(inputView->cell == -1);
    assert(inputView->readout == -1);

    // PATTERN.ROW is selected by the recipe.  If it is selected, we search for the table
    // PATTERN.ROW.SUBSET.  If this is found in our format file, we use that version;
    // otherwise, we use the table provided in the recipe file "ppImage.config".  Within that
    // table, we select the entry that matches our CHIP.NAME.  This will be either a boolean or
    // a string of bits.  If it is a boolean, it specified whether or not to correct the entire
    // chip; if it is a string, the bits specify which cells to correct (sequence is order of
    // CELLS in the format:CHIPS metadata table)

    // New 2021 October : During ppImageParseCamera, we have loaded a file identified by
    // PPIMAGE.PATTERN.ROW.AMP on config->files.  This file contains a collection of FITS
    // tables, one per chip.  Each table lists the typical bias-drift amplitude for cell,
    // measured from a collection of dark images.  These values are passed (via
    // cell->analysis) to pmPatternRow which chooses to apply the correction based on the
    // ratio of the poisson noise due to the median background (+ read noise) and the
    // typical amplitude.  Tests show that if the amplitude / noise < 1/6, then the
    // row-by-row variations are insignificant.

    // We also check the chip header for the boolean 'PTRN_ROW' : if this is true, we have
    // already applied this correct to this data, so we simply skip the correction for this
    // chip.

    pmHDU *hdu = pmHDUFromChip(chip);
    if (psMetadataLookupBool(&status, hdu->header, "PTRN_ROW")) {
	psLogMsg("ppImage", PS_LOG_INFO, "Not performing row pattern correction as it has already been done.");
	return true;
    }

    const char *chipName = psMetadataLookupStr(&status, chip->concepts, "CHIP.NAME");
    psLogMsg("ppImage", PS_LOG_INFO, "Performing row pattern correction for %s\n", chipName);

    // XXX we use input->fpa below, but could we just use chip->parent?
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPIMAGE.INPUT");

    // grab the PATTERN.ROW.AMP file
    pmFPAfile *PRAfile = psMetadataLookupPtr (&status, config->files, "PPIMAGE.PATTERN.ROW.AMP");
    if (!PRAfile) {
	psLogMsg("ppImage", PS_LOG_INFO, "Pattern Row Amplitude file not found, applying to all (with limits from ROW.SUBSET) ");
    }
	
    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    pmCell *cell = NULL;
    while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
	// const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");
	// const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME");
	// psWarning ("Looping through %s, %s\n", chipName, cellName);

	if (!cell->process || !cell->file_exists) {
	    continue;
	}

	// this forces pmFPAfileRead of the PATTERN.ROW.AMP file (XXX but is this needed?)
	if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	    ESCAPE(false, "load failure for Cell");
	}
	    
	if (!cell->data_exists) {
	    continue;
	}

	if (cell->readouts->n > 1) {
	    psWarning ("Skipping Video Cell for ppImageDetrendPatternApply");
	    continue;
	}

	// grab the corresponding cell
	if (PRAfile) {
	    pmCell *PRAcell = pmFPAviewThisCell (view, PRAfile->fpa);
	    psAssert (PRAcell, "found Pattern Row Amplitude file, but not cell?");

	    // find the nominal signal amplitude (check the ghost and/or crosstalk recipe file)
	    float amplitude = psMetadataLookupF32 (&status, PRAcell->analysis, "PTN.ROW.AMP");
	    if (!status) amplitude = NAN;
	    
	    // put the value on the science cell
	    psMetadataAddF32 (cell->analysis, PS_LIST_TAIL, "PTN.ROW.AMP", PS_META_REPLACE, "", amplitude);
	}

	bool doPattern = false;
	if (!doPatternForView(&doPattern, config, chip, view, RECIPE_NAME, "PATTERN.ROW.SUBSET")) {
	    ESCAPE(false, "Unable to determine whether row pattern matching should be applied.");
	}
	if (!doPattern) continue;

	// switch to test threaded version
	if (true) {
	    // I need to allocate a view here to be freed by the
	    // called function below.  
	    pmFPAview *myView = pmFPAviewAlloc(0); // View for local processing
	    *myView = *view;

	    // allocate a job, construct the arguments for this job
	    psThreadJob *job = psThreadJobAlloc("PPIMAGE_PATTERN_ROW_CELL");
	    psArrayAdd(job->args, 1, config);
	    psArrayAdd(job->args, 1, input->fpa);
	    psArrayAdd(job->args, 1, chip);
	    psArrayAdd(job->args, 1, cell);
	    psArrayAdd(job->args, 1, myView);
	    psArrayAdd(job->args, 1, options);
	    if (!psThreadJobAddPending(job)) {
		return false;
	    }
	} else {
	    // bump the counter since it must be freed by the function below.  
	    psMemIncrRefCounter (view); // View for local processing
	    if (!ppImageDetrendPatternApplyCell (config, input->fpa, chip, cell, view, options)) {
		return false;
	    }
	}
    }

    // wait here for the threaded jobs to finish
    // if no threads are allocated, this 
    if (!psThreadPoolWait(true, true)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to apply bias correction.");
	return false;
    }

    psMetadataAddBool(hdu->header, PS_LIST_TAIL, "PTRN_ROW",PS_META_REPLACE,"PATTERN.ROW correction applied",true);
    psFree(view);
   
    return true;
}

bool ppImageDetrendPatternContinuityApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options)
{
    bool status;

    if (!options->doPatternContinuity) return true;

    assert(inputView->chip != -1);
    assert(inputView->cell == -1);
    assert(inputView->readout == -1);

    // see the comment for PATTERN.ROW; the same rules apply for PATTERN.CELL

    int numCells = chip->cells->n;       // Number of cells
    psVector *tweak = psVectorAlloc(numCells, PS_TYPE_U8); // Tweak cell?
    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    pmHDU *hdu = pmHDUFromChip(chip);
    if (psMetadataLookupBool(&status, hdu->header, "PTRN_CON")) {
	psLogMsg("ppImage", PS_LOG_INFO, "Not performing cell continuity correction as it has already been done.");
	return true;
    }

    const char *chipName = psMetadataLookupStr(&status, chip->concepts, "CHIP.NAME");
    psLogMsg("ppImage", PS_LOG_INFO, "Performing pattern continuity correction for %s\n", chipName);

    for (int i = 0; i < chip->cells->n; i++) {
	view->cell = i;

	pmCell *cell = chip->cells->data[i]; // Cell of interest

	if (cell->readouts->n > 1) {
	    psLogMsg("ppImage", PS_LOG_INFO, "Not performing cell continuity correction on video cell.");
	    continue;
	}

	bool doPattern = false;
	if (!doPatternForView(&doPattern, config, chip, view, RECIPE_NAME, "PATTERN.CONTINUITY.SUBSET")) {
	    ESCAPE(false, "Unable to determine whether row pattern matching should be applied.");
	}

	if (doPattern) {
	    tweak->data.U8[i] = 0xFF;
	}
    }

    // Tweak the cells
    if (!pmPatternContinuity(chip, tweak, options->patternCellBG, options->patternCellMean,
			     options->maskValue, options->darkMask,options->patternContinuityEdgeWidth)) {
	psFree(tweak);
	psFree(view);
	return false;
    }
    psFree(tweak);
    psFree(view);

    psMetadataAddBool(hdu->header, PS_LIST_TAIL, "PTRN_CON",PS_META_REPLACE,"PATTERN.CONTINUITY correction applied",true);
    return true;
}

bool ppImageDetrendPatternCellApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options)
{
    bool status;

    if (!options->doPatternCell) return true;

    assert(inputView->chip != -1);
    assert(inputView->cell == -1);
    assert(inputView->readout == -1);

    int numCells = chip->cells->n;       // Number of cells
    psVector *tweak = psVectorAlloc(numCells, PS_TYPE_U8); // Tweak cell?
    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    pmHDU *hdu = pmHDUFromChip(chip);
    if (psMetadataLookupBool(&status, hdu->header, "PTRN_CEL")) {
	psLogMsg("ppImage", PS_LOG_INFO, "Not performing cell pattern correction as it has already been done.");
	return true;
    }

    const char *chipName = psMetadataLookupStr(&status, chip->concepts, "CHIP.NAME");
    psLogMsg("ppImage", PS_LOG_INFO, "Performing cell pattern correction for %s\n", chipName);

    for (int i = 0; i < chip->cells->n; i++) {
	view->cell = i;

	pmCell *cell = chip->cells->data[i]; // Cell of interest

	if (cell->readouts->n > 1) {
	    psLogMsg("ppImage", PS_LOG_INFO, "Not performing cell pattern correction on video cell.");
	    continue;
	}

	bool doPattern = false;
	if (!doPatternForView(&doPattern, config, chip, view, RECIPE_NAME, "PATTERN.CELL.SUBSET")) {
	    ESCAPE(false, "Unable to determine whether row pattern matching should be applied.");
	}
	if (doPattern) {
	    tweak->data.U8[i] = 0xFF;
	}
    }

    // Tweak the cells
    if (!pmPatternCell(chip, tweak, options->patternCellBG, options->patternCellMean,
		       options->maskValue, options->darkMask)) {
	psFree(tweak);
	psFree(view);
	return false;
    }
    psFree(tweak);
    psFree(view);

    psMetadataAddBool(hdu->header, PS_LIST_TAIL, "PTRN_CEL",PS_META_REPLACE,"PATTERN.CELL correction applied",true);
    return true;
}

bool ppImageDetrendPatternDeadCellsMask (pmChip *chip, psImageMaskType maskVal) {

    int numCells = chip->cells->n;       // Number of cells

    // now mask bad cells
    for (int i = 0; i < numCells; i++) {
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest

	psImage *mask = ro->mask; // mask of interest
	int numCols = mask->numCols, numRows = mask->numRows; // Size of image
	
	for (int y = 0; y < numRows; y++) {
	    for (int x = 0; x < numCols; x++) {
		mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskVal;
	    }
	}
    }
    return true;
}

bool ppImageDetrendPatternDeadCellsXY42 (pmChip *chip, psImageMaskType maskVal) {

    bool status;

    // extract the MEDIAN_CELL_BACKGROUND values to check for problems
    int numCells = chip->cells->n;       // Number of cells
    psVector *cellBackground = psVectorAllocEmpty (numCells, PS_DATA_F32);

    for (int i = 0; i < numCells; i++) {
	pmCell *cell = chip->cells->data[i];;

	if (!cell->process || !cell->file_exists || !cell->data_exists) continue;
	
	// select the HDU for this cell
	pmHDU *cellHDU = pmHDUFromCell(cell);  // HDU of interest
	
	psF32 BackValue = psMetadataLookupF32(&status, cellHDU->header, "BACK_VAL");
	if (!status) continue;

	psF32 BackError = psMetadataLookupF32(&status, cellHDU->header, "BACK_ERR");
	if (!status) continue;

	if (BackValue == 0.0) continue;
	if (BackError == 0.0) continue;

	psVectorAppend (cellBackground, BackValue / BackError);
    }

    if (cellBackground->n < 0.375*numCells) {
	// Chip is bad, mask the whole thing
	ppImageDetrendPatternDeadCellsMask (chip, maskVal);
	psFree (cellBackground);
	return true;
    }

    // Second, calculate the median
    psVectorSortInPlace (cellBackground);
    int midPt = cellBackground->n / 2.0;
    float median = cellBackground->n % 2 ? cellBackground->data.F32[midPt] : 0.5*(cellBackground->data.F32[midPt] + cellBackground->data.F32[midPt-1]);
    psFree (cellBackground);

    // XXX hardwired number
    if (median < 3.0) {
	// Chip is bad, mask the whole thing
	ppImageDetrendPatternDeadCellsMask (chip, maskVal);
    }
    psFree (cellBackground);
    return true;
}

// to apply the dead cell pattern, we need to transfer the pattern for this chip from the
// pmFPAfile for the pattern to the one for the image being processed.  
bool ppImageDetrendPatternDeadCellsApply(pmConfig *config, pmChip *chip, const pmFPAview *inputView, ppImageOptions *options)
{
    bool status;

    if (!options->doPatternDeadCells) return true;

    assert(inputView->chip != -1);
    assert(inputView->cell == -1);
    assert(inputView->readout == -1);

    const char *chipName = psMetadataLookupStr(&status, chip->concepts, "CHIP.NAME");
    psLogMsg("ppImage", PS_LOG_INFO, "Performing cell pattern correction for %s\n", chipName);

    pmHDU *hdu = pmHDUFromChip(chip);
    if (psMetadataLookupBool(&status, hdu->header, "PTRN_DED")) {
	psLogMsg("ppImage", PS_LOG_INFO, "Not performing dead cell pattern correction as it has already been done.");
	return true;
    }

    if (!strcmp (chipName, "XY40") || !strcmp (chipName, "XY42")) {
	// special case : check for BACK_VAL / BACK_ERR > 3 or < 3
	psLogMsg("ppImage", PS_LOG_INFO, "Using special case for XY40 and XY42");
	ppImageDetrendPatternDeadCellsXY42 (chip, options->blankMask);
	return true;
    }

    pmFPAfile *pattern = psMetadataLookupPtr(&status, config->files, "PPIMAGE.PATTERN.DEAD.CELLS");
    if (!pattern) {
	psLogMsg("ppImage", PS_LOG_INFO, "Pattern Dead Cells file not found, skipping");
	return true;
    }

    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    pmChip *patternChip = pmFPAviewThisChip (view, pattern->fpa);

    // grab the pattern from the input pattern file chip
    psImage *deadCellPattern = (psImage *) psMetadataLookupPtr (&status, patternChip->analysis, "PTN.DEAD.CELL");
    if (!deadCellPattern) {
        psLogMsg("psModules.detrend", PS_LOG_DETAIL, "No DEAD CELL pattern for chip, skipping\n");
	psFree (view);
	return true;
    }

    // copy the pattern pointer to the chip of the image being processed
    psMetadataAddImage (chip->analysis, PS_LIST_TAIL, "PTN.DEAD.CELL", PS_META_REPLACE, "", deadCellPattern);

    // extract the MEDIAN_CELL_BACKGROUND values to check for problems
    int numCells = chip->cells->n;       // Number of cells
    psVector *cellBackground = psVectorAllocEmpty (numCells, PS_DATA_F32);

    pmCell *cell = NULL;
    while ((cell = pmFPAviewNextCell(view, chip->parent, 1)) != NULL) {
	if (!cell->process || !cell->file_exists || !cell->data_exists) {
	    psVectorAppend (cellBackground, NAN);
	    continue;
	}
	
	// select the HDU for this cell
	pmHDU *cellHDU = pmHDUFromCell(cell);  // HDU of interest
	
	psF32 value = psMetadataLookupF32(&status, cellHDU->header, "BACK_VAL");
	if (!status) {
	    psVectorAppend (cellBackground, NAN);
	    continue;
	}
	psVectorAppend (cellBackground, value);
    }

    // match cellBackground pattern to registered patterns and mask as needed
    if (!pmPatternDeadCells(chip, cellBackground, options->blankMask)) {
	psFree(cellBackground);
	psFree(view);
	return false;
    }
    psFree(cellBackground);
    psFree(view);

    psMetadataAddBool(hdu->header, PS_LIST_TAIL, "PTRN_DED", PS_META_REPLACE, "PATTERN.DEADCELLS applied", true);
    return true;
}

static bool doPatternForView (bool *doit, const pmConfig *config, const pmChip *chip, const pmFPAview *view, const char *recipeName, const char *recipeValue) {

    *doit = false;

    psMetadataItem *doPattern;

    doPattern = pmConfigRecipeValueByView(config, recipeName, recipeValue, chip->parent, view);
    
    if (!doPattern) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine whether row pattern matching should be applied.");
        return false;
    }
    if (doPattern->type == PS_DATA_BOOL) {
        *doit = doPattern->data.B;
        return true;
    }
    if (doPattern->type == PS_DATA_STRING) {
        // expect a string of the form "000110001001" with at least view->cell entries
        char *string = doPattern->data.str;
        if (strlen(string) < view->cell) {
            psError(PS_ERR_UNKNOWN, true, "error in PATTERN.ROW.SUBSET chip string (too few elements %d)", (int) strlen(string));
            return false;
        }
        switch (string[view->cell]) {
          case '0':
          case 'f':
          case 'F':
          case 'n':
          case 'N':
            *doit = false;
            return true;
          case '1':
          case 't':
          case 'T':
          case 'y':
          case 'Y':
            *doit = true;
            return true;
          default:
            psError(PS_ERR_UNKNOWN, true, "error in PATTERN.ROW.SUBSET chip string %s (unknown value %c))", string, string[view->cell]);
            return false;
        }
        psAbort("imposible to reach here");
    }
    psError(PS_ERR_UNKNOWN, true, "error in PATTERN.ROW.SUBSET : invalid data type");
    return false;
}

// thread safety : 
bool ppImageDetrendPatternApplyCell (pmConfig *config, pmFPA *fpa, pmChip *chip, pmCell *cell, pmFPAview *view, ppImageOptions *options) {
  
    // process each of the readouts
    pmReadout *readout;         // Readout from cell
    while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
	if (!readout->data_exists) {
	    continue;
	}

	// Perform pattern correction
	if (!pmPatternRow(readout, options->patternRowOrder, options->patternRowIter,
			  options->patternRowRej, options->patternRowThresh, options->patternRowMean,
			  options->patternRowStdev, options->maskValue, options->darkMask)) {
	    psFree (view);
	    return false;
	}
    }
    psFree (view); // supplied view must be freeable
    return true;
}

psVector *ppImageDetrendPatternCellFailures(pmFPAfile *input, const pmFPAview *inputView) {
    
    bool status = false;
    pmCell *cell = NULL;
    
    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    // extract the MEDIAN_CELL_BACKGROUND values to check for problems
    psVector *cellBackground = psVectorAllocEmpty (64, PS_DATA_F32);
    while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
	if (!cell->process || !cell->file_exists || !cell->data_exists) {
	    psVectorAppend (cellBackground, NAN);
	    continue;
	}
	
	// select the HDU for this cell
	pmHDU *hdu = pmHDUFromCell(cell);  // HDU of interest
	
	psF32 value = psMetadataLookupF32(&status, hdu->header, "BACK_VAL");
	psVectorAppend (cellBackground, value);
    }
    return cellBackground;
}

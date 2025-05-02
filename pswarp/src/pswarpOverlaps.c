# include "pswarp.h"

static void pswarpBoundsFree (pswarpBounds *bounds) {
    psFree (bounds->Pmin);
    psFree (bounds->Pmax);
    psFree (bounds->Qmin);
    psFree (bounds->Qmax);
}

pswarpBounds *pswarpBoundsAlloc() {

    pswarpBounds *bounds = psAlloc(sizeof(pswarpBounds));
    psMemSetDeallocator(bounds, (psFreeFunc)pswarpBoundsFree);

    bounds->Pmin = psVectorAllocEmpty(32, PS_DATA_F32);
    bounds->Pmax = psVectorAllocEmpty(32, PS_DATA_F32);
    bounds->Qmin = psVectorAllocEmpty(32, PS_DATA_F32);
    bounds->Qmax = psVectorAllocEmpty(32, PS_DATA_F32);
    return bounds;
}

bool pswarpBoundsAppend(pswarpBounds *bounds, float Pmin, float Pmax, float Qmin, float Qmax) {
    psVectorAppend (bounds->Pmin, Pmin);
    psVectorAppend (bounds->Pmax, Pmax);
    psVectorAppend (bounds->Qmin, Qmin);
    psVectorAppend (bounds->Qmax, Qmax);
    return true;
}

pswarpBounds *pswarpMakeBounds (pmFPA *fpa, psProjection *frame) {

    pswarpBounds *bounds = pswarpBoundsAlloc ();
    
    psPlane *CH = psPlaneAlloc();
    psPlane *FP  = psPlaneAlloc();
    psPlane *TP  = psPlaneAlloc();
    psPlane *FR  = psPlaneAlloc();
    psSphere *sky = psSphereAlloc();

    pmFPAview *view = pmFPAviewAlloc (0);

    pmChip *chip = NULL;
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
	psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	if (!chip->process || !chip->file_exists) { 
	    // this ensures one entry per chip (regardless of existence)
	    pswarpBoundsAppend (bounds, NAN, NAN, NAN, NAN);
	    continue; 
	}

	// we've got the output astrom header
	pmHDU *hdu = pmHDUFromChip(chip); ///< HDU for source
	assert (hdu);
	assert (hdu->header);
	int numCols = psMetadataLookupS32(NULL, hdu->header, "NAXIS1"); ///< Number of columns
	int numRows = psMetadataLookupS32(NULL, hdu->header, "NAXIS2"); ///< Number of rows
	if ((numCols == 0) || (numRows == 0)) {
	    psError(PSWARP_ERR_DATA, false, "astrom has invalid dimensions %d x %d", numCols, numRows);
	    psFree(view);
	    return false;
	}

	float Pmin = +1e9;
	float Pmax = -1e9;
	float Qmin = +1e9;
	float Qmax = -1e9;

	// find P,Q at corners and center of edges (and center just for ease of code)
	for (float ix = 0.0; ix <= 1.0; ix += 0.5) {
	    for (float iy = 0.0; iy <= 1.0; iy += 0.5) {

		CH->x = ix * numCols;
		CH->y = iy * numRows;
		psPlaneTransformApply(FP, chip->toFPA, CH); 
		psPlaneTransformApply(TP, fpa->toTPA, FP); 
		psDeproject (sky, TP, fpa->toSky); 
		psProject (FR, sky, frame); 

# if (0)
		fprintf (stderr, "%10.3f %10.3f | %10.3f %10.3f | %10.3f %10.3f | %10.6f %10.6f | %10.3f %10.3f\n", 
			 CH->x, CH->y, 
			 FP->x, FP->y, 
			 TP->x, TP->y, 
			 PS_DEG_RAD*sky->r, PS_DEG_RAD*sky->d, 
			 FR->x, FR->y);
# endif

		Pmin = PS_MIN(Pmin, FR->x);
		Pmax = PS_MAX(Pmax, FR->x);
		Qmin = PS_MIN(Qmin, FR->y);
		Qmax = PS_MAX(Qmax, FR->y);
	    }
	}
	pswarpBoundsAppend (bounds, Pmin, Pmax, Qmin, Qmax);
    }
    psFree(view);
    psFree(CH);
    psFree(FP);
    psFree(TP);
    psFree(FR);
    psFree(sky);

    return bounds;
}

// find R,D for the center of the output pmFPA
psProjection *pswarpLocalFrame (pmFPA *fpa) {

    // find R,D for each corner of each chip and take the average
    psVector *Rvec = psVectorAllocEmpty (256, PS_DATA_F32);
    psVector *Dvec = psVectorAllocEmpty (256, PS_DATA_F32);
    
    psPlane *CH = psPlaneAlloc();
    psPlane *FP  = psPlaneAlloc();
    psPlane *TP  = psPlaneAlloc();
    psSphere *sky = psSphereAlloc();

    pmFPAview *view = pmFPAviewAlloc (0);

    // we are looping over the chips of the OUTPUT fpa.  chip->file_exists is false for all chips. 
    // at this point, I cannot exclude any of these chips (I do not know which output chips will not be created).
    // NOTE : I either have to use SKYCELL fpa and ensure SKYCELL->chip->toFPA exists or grab hdu from SKYCELL

    pmChip *chip = NULL;
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
	psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	// if (!chip->file_exists) { continue; } // this fpa refers to the output file, so no chip exist
	if (!chip->toFPA) { continue; } // this fpa refers to the output file, so no chip exist
	assert (chip->toFPA);
	chip->process = true; // start with process true, we will de-activate later (pswarpFindOverlaps)

	// we've got the output astrom header
	pmHDU *hdu = pmHDUFromChip(chip); ///< HDU for source
	assert (hdu);
	assert (hdu->header);
	int numCols = psMetadataLookupS32(NULL, hdu->header, "NAXIS1"); ///< Number of columns
	int numRows = psMetadataLookupS32(NULL, hdu->header, "NAXIS2"); ///< Number of rows
	if ((numCols == 0) || (numRows == 0)) {
	    psError(PSWARP_ERR_DATA, false, "astrom has invalid dimensions %d x %d", numCols, numRows);
	    psFree(view);
	    return false;
	}

	// get R,D for all corners
	for (float ix = 0.0; ix <= 1.0; ix += 1.0) {
	    for (float iy = 0.0; iy <= 1.0; iy += 1.0) {
		CH->x = ix * numCols;
		CH->y = iy * numRows;
		psPlaneTransformApply(FP, chip->toFPA, CH); 
		psPlaneTransformApply(TP, fpa->toTPA, FP); 
		psDeproject (sky, TP, fpa->toSky); 
		psVectorAppend (Rvec, sky->r);
		psVectorAppend (Dvec, sky->d);
	    }
	}
    }
    psFree(view);
    psFree(CH);
    psFree(FP);
    psFree(TP);
    psFree(sky);

    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psVectorStats (stats, Rvec, NULL, NULL, 0);
    double Rmid = stats->sampleMean;

    psStatsInit (stats);
    psVectorStats (stats, Dvec, NULL, NULL, 0);
    double Dmid = stats->sampleMean;

    psFree (Rvec);
    psFree (Dvec);
    psFree (stats);

    psProjection *frame = psProjectionAlloc (Rmid, Dmid, PS_RAD_DEG/3600.0, PS_RAD_DEG/3600.0, PS_PROJ_TAN);
    return frame;
}

bool pswarpFindOverlap (pmFPA *input, pmFPA *output, pswarpBounds *src, pswarpBounds *tgt) {

    bool status = false;

    // we have the source and target bounds.  loop over all output elements and check if any of the source
    // elements overlap it.

    // also add the list if input (src) cells which land on each output (tgt) chip we do this
    // by only identifying the input and output chips and adding all input chip cells to each
    // output cell

    assert (output->chips->n == tgt->Pmin->n);
    assert (input->chips->n == src->Pmin->n);

    for (int j = 0; j < output->chips->n; j++) {
	pmChip *chip = output->chips->data[j];

	if (!isfinite(tgt->Pmin->data.F32[j])) continue;
	if (!isfinite(tgt->Pmax->data.F32[j])) continue;
	if (!isfinite(tgt->Qmin->data.F32[j])) continue;
	if (!isfinite(tgt->Qmax->data.F32[j])) continue;

	// we have src bounds
	float Pmin = tgt->Pmin->data.F32[j];
	float Pmax = tgt->Pmax->data.F32[j];
	float Qmin = tgt->Qmin->data.F32[j];
	float Qmax = tgt->Qmax->data.F32[j];

	psArray *inputChips = psArrayAllocEmpty(8);

	bool hasOverlap = false;
	for (int i = 0; i < src->Pmin->n; i++) {
	    // overlaps in P?
	    if (!isfinite(src->Pmin->data.F32[i])) continue;
	    if (!isfinite(src->Pmax->data.F32[i])) continue;
	    if (!isfinite(src->Qmin->data.F32[i])) continue;
	    if (!isfinite(src->Qmax->data.F32[i])) continue;

	    if (Pmin > src->Pmax->data.F32[i]) continue;
	    if (Pmax < src->Pmin->data.F32[i]) continue;

	    if (Qmin > src->Qmax->data.F32[i]) continue;
	    if (Qmax < src->Qmin->data.F32[i]) continue;

	    hasOverlap = true;
	    psArrayAdd (inputChips, 1, input->chips->data[i]);
	}
	chip->process |= hasOverlap;
	if (hasOverlap) {
	    pmChipSelectCells (chip);
	    // get the complete list of input cells for this set of input chips
	    // pmConceptsAverageCells needs a psList (not a psArray)
	    psArray *inputCells = psArrayAllocEmpty(8);
	    for (int nChip = 0; nChip < inputChips->n; nChip++) {
		pmChip *inputChip = inputChips->data[nChip];
		for (int nCell = 0; nCell < inputChip->cells->n; nCell++) {
		    psArrayAdd (inputCells, 8, inputChip->cells->data[nCell]);
		}
	    }
	    
	    // place the inputCells on each of the output cells 
	    for (int nCell = 0; nCell < chip->cells->n; nCell++) {
		pmCell *outputCell = chip->cells->data[nCell];
		psList *outList = psMetadataLookupPtr(&status, outputCell->analysis, "INPUT.CELLS");
		if (!outList) {
		    outList = psListAlloc(NULL);
		    psMetadataAddPtr(outputCell->analysis, PS_LIST_TAIL, "INPUT.CELLS", PS_DATA_LIST , "input cells touching this output cell", outList);
		    psFree (outList);
		}
		for (int nCell = 0; nCell < inputCells->n; nCell ++) {
		    psListAdd (outList, PS_LIST_TAIL, inputCells->data[nCell]);
		}
	    }
	    psFree (inputCells);
	}
	psFree (inputChips);
    }
    return true;
}

/** @file pswarpLoop.c
 *
 *  @brief main processing loop for pswarp
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

// Loop over the inputs, warp them to the output skycell and then update metadata
bool pswarpLoop(pmConfig *config, psMetadata *stats)
{
    // load the recipe
    bool status = false;
    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    // select the input data sources
    pmFPAfile *output = psMetadataLookupPtr(&status, config->files, "PSWARP.OUTPUT");
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
        return false;
    }

    // load in the input pixel data (ex. background model)
    pmFPAfileActivate(config->files, false, NULL);
    pmFPAfileActivate(config->files, true, "PSWARP.INPUT");
    pmFPAfileActivate(config->files, true, "PSWARP.MASK");
    pmFPAfileActivate(config->files, true, "PSWARP.VARIANCE");

    // We re-activate the CMF load so we can transform the sources as well as the pixels.
    // We only need to read in these if the astrometry source is CMF.
    if (psMetadataLookupBool(&status, recipe, "SOURCES")) {
	pmFPAfileActivate(config->files, true, "PSWARP.ASTROM");
    }

    psString refcat = NULL;
    // loop over this section once per input group
    for (int i = 0; i < nInputs; i++) {
	// select the input data sources
	pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PSWARP.INPUT", i);
	if (!input) {
	    psError(PSWARP_ERR_CONFIG, true, "Can't find input data!\n");
	    return false;
	}

	// select the input data sources
	pmFPAfile *astrom = pmFPAfileSelectSingle(config->files, "PSWARP.ASTROM", i);
	if (!astrom) {
	    astrom = input;
	}

	pmFPAviewReset (view);

	// files associated with the science image
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
	    psError(psErrorCodeLast(), false, "Unable to read files.");
	    goto FAIL;
	}

	// *** main transformation block 
	// *** this section loops over the input chips/cells and reads them one at a time
	// *** the output chips/cells are filled where appropriate, but not yet written to disk 
	pmChip *chip;
	while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
	    psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	    if (!chip->process || !chip->file_exists) { continue; }
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
		psError(psErrorCodeLast(), false, "Unable to read files.");
		goto FAIL;
	    }

	    pmCell *cell;
	    while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
		psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
		if (!cell->process || !cell->file_exists) { continue; }
		if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
		    psError(psErrorCodeLast(), false, "Unable to read files.");
		    goto FAIL;
		}

		// process each of the readouts
		pmReadout *readout;
		while ((readout = pmFPAviewNextReadout(view, input->fpa, 1)) != NULL) {
		    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
			psError(psErrorCodeLast(), false, "Unable to read files.");
			goto FAIL;
		    }
		    if (!readout->data_exists) {
			continue;
		    }

		    // Copy the detections from the astrometry carrier to the input, so they can be accessed by
		    // pswarpTransformReadout
		    if (astrom != input) {
			pmReadout *astromRO = pmFPAviewThisReadout(view, astrom->fpa); // Readout for astrometry
			pmDetections *detections = psMetadataLookupPtr(&status, astromRO->analysis, "PSPHOT.DETECTIONS"); // Sources from astrometry
			if (detections) {
			    psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_ARRAY, "Sources from input astrometry", detections);
			}

			// Determine the reference catalog used for the astrometry
			if (!refcat && astromRO) {

			    // find the HDU for this readout
			    pmHDU *hdu = pmHDUFromReadout (astromRO);
			    if (! hdu) {
				psWarning ("unable to find HDU for astrometry source, cannot save PSREFCAT");
			    } 
			    if (!output->fpa->analysis) {
				psWarning ("analysis metadata not defined for output, cannot save PSREFCAT");
			    }

			    psMetadataItem *refItem = psMetadataLookup(hdu->header, "PSREFCAT");
			    if (!refItem) {
				psWarning ("PSREFCAT not found in astrometry HDU, cannot save PSREFCAT");
			    }

			    if (refItem && hdu && output->fpa->analysis) {
				refcat = psMetadataLookupStr(NULL, hdu->header, "PSREFCAT");
				psMetadataAddStr(output->fpa->analysis, PS_LIST_TAIL, "REFERENCE_CATALOG", PS_META_REPLACE,
						 "Reference catalog used for calibration.", refcat);
			    }
			}
		    }

		    pswarpTransformToTarget(output->fpa, readout, config, false);
		
		    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
			psError(psErrorCodeLast(), false, "Unable to write files.");
			goto FAIL;
		    }
		}
		if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
		    psError(psErrorCodeLast(), false, "Unable to write files.");
		    goto FAIL;
		}
	    }
	    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
		psError(psErrorCodeLast(), false, "Unable to write files.");
		goto FAIL;
	    }
	}
	if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	    psError(psErrorCodeLast(), false, "Unable to write files.");
	    goto FAIL;
	}

	if (!pswarpUpdateStatistics (output->fpa, stats, input->fpa, astrom->fpa, config)) {
	    psError(psErrorCodeLast(), false, "problem generating statistics.");
	    goto FAIL;
	}
	if (!pswarpUpdateMetadata (output->fpa, skycell->fpa, input->fpa, astrom->fpa, config, true)) {
	    psError(psErrorCodeLast(), false, "problem generating statistics.");
	    goto FAIL;
	}
    }

    if (!pswarpMakePSF (config, output, stats)) {
	psError(psErrorCodeLast(), false, "problem generating PSF.");
	goto FAIL;
    }

    psFree(view);
    return true;

FAIL:
    psFree (view);
    return false;
}

bool pswarpGetBackTransform (pmReadout *tgt, pmReadout *src);

// once the output fpa elements have been built, loop over the fpa and generate stats
// for each readout
bool pswarpTransformToTarget (pmFPA *output, pmReadout *input, pmConfig *config, bool backgroundWarp)  {

    pmFPAview *view = pmFPAviewAlloc(0);
    
    pmChip *chip;
    while ((chip = pmFPAviewNextChip (view, output, 1)) != NULL) {
        psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        pmCell *cell;
        while ((cell = pmFPAviewNextCell (view, output, 1)) != NULL) {
            psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            pmReadout *readout;
            while ((readout = pmFPAviewNextReadout(view, output, 1)) != NULL) {
		pswarpTransformReadout (readout, input, config, backgroundWarp);

		// determine a (rough) linear WCS from readout to input
		pswarpGetBackTransform (readout, input);
	    }
	}
    }
    psFree (view);
    return true;
}

bool pswarpGetBackTransform (pmReadout *tgt, pmReadout *src) {

    bool status;

    // tgt is the pswarp output target image, src is the input source image
    // here we are generating a linear transformation from tgt -> src

    // the map is defined for coordinates in the image parent frame.
    psRegion imageRegion = psRegionSet (0,0,0,0);
    imageRegion = psRegionForImage (src->image, imageRegion);
    psString imageRegionString = psRegionToString (imageRegion);
  
    // generate a set of points (X,Y)_tgt = (X,Y)_src and do the 2D fit?
    // save a 2D polynomial, not WCS?

    // use a map with only linear terms in x,y
    psPlaneTransform *map = psPlaneTransformAlloc(1, 1, PS_POLYNOMIAL_ORD);
    map->x->coeffMask[1][1] = PS_POLY_MASK_BOTH;
    map->y->coeffMask[1][1] = PS_POLY_MASK_BOTH;

    pmCell *cell = NULL;

    cell = src->parent;
    pmChip *chipSrc = cell->parent;
    pmFPA *fpaSrc = chipSrc->parent;

    cell = tgt->parent;
    pmChip *chipTgt = cell->parent;
    pmFPA *fpaTgt = chipTgt->parent;

    psPlane *srcPos = psPlaneAlloc();
    psPlane *tgtPos = psPlaneAlloc();

    psPlane *FP = psPlaneAlloc();
    psPlane *TP = psPlaneAlloc();
    psSphere *sky = psSphereAlloc();

    psVector *tgtX = psVectorAllocEmpty (121, PS_TYPE_F32);
    psVector *tgtY = psVectorAllocEmpty (121, PS_TYPE_F32);
    psVector *srcX = psVectorAllocEmpty (121, PS_TYPE_F32);
    psVector *srcY = psVectorAllocEmpty (121, PS_TYPE_F32);

    int dX = src->image->numCols / 10.0;
    int dY = src->image->numRows / 10.0;
    for (int ix = 0; ix < src->image->numCols; ix += dX) {
	for (int iy = 0; iy < src->image->numRows; iy += dY) {

	    srcPos->x = ix;
	    srcPos->y = iy;

	    psPlaneTransformApply(FP, chipSrc->toFPA, srcPos);
	    psPlaneTransformApply (TP, fpaSrc->toTPA, FP);
	    psDeproject (sky, TP, fpaSrc->toSky);

	    psProject (TP, sky, fpaTgt->toSky);
	    psPlaneTransformApply (FP, fpaTgt->fromTPA, TP);
	    psPlaneTransformApply (tgtPos, chipTgt->fromFPA, FP);

	    psVectorAppend (srcX, srcPos->x);
	    psVectorAppend (srcY, srcPos->y);
	    psVectorAppend (tgtX, tgtPos->x);
	    psVectorAppend (tgtY, tgtPos->y);
	}
    }

    psVectorFitPolynomial2D(map->x, NULL, 0, srcX, NULL, tgtX, tgtY);
    psVectorFitPolynomial2D(map->y, NULL, 0, srcY, NULL, tgtX, tgtY);

    // for each output image, we want to add headers corresponding to all input images
    // save an array of the transformations on the tgt analysis and an array of the input chip names
    psArray *backmaps = psMetadataLookupPtr (&status, tgt->analysis, PSWARP_ANALYSIS_BACKMAPS);
    if (!backmaps) {
	backmaps = psArrayAllocEmpty (4);
	psMetadataAddArray (tgt->analysis, PS_LIST_TAIL, PSWARP_ANALYSIS_BACKMAPS, 0, "backwards maps", backmaps);
	psFree (backmaps); // can free here since we put a copy on tgt->analysis
    }
    psArrayAdd (backmaps, 4, map);
    
    // we also need to save the corresponding chip name for this map
    char *chipname = psMetadataLookupStr (&status, chipSrc->concepts, "CHIP.NAME");
    psArray *chipnames = psMetadataLookupPtr (&status, tgt->analysis, PSWARP_ANALYSIS_CHIPNAMES);
    if (!chipnames) {
	chipnames = psArrayAllocEmpty (4);
	psMetadataAddArray (tgt->analysis, PS_LIST_TAIL, PSWARP_ANALYSIS_CHIPNAMES, 0, "source images", chipnames);
	psFree (chipnames); // can free here since we put a copy on tgt->analysis
    }
    psArrayAdd (chipnames, 4, chipname);

    // we also need to save the corresponding chip name for this map
    psArray *chipRegions = psMetadataLookupPtr (&status, tgt->analysis, PSWARP_ANALYSIS_CHIPREGIONS);
    if (!chipRegions) {
	chipRegions = psArrayAllocEmpty (4);
	psMetadataAddArray (tgt->analysis, PS_LIST_TAIL, PSWARP_ANALYSIS_CHIPREGIONS, 0, "source images", chipRegions);
	psFree (chipRegions); // can free here since we put a copy on tgt->analysis
    }
    psArrayAdd (chipRegions, 4, imageRegionString);

    fprintf (stderr, "warp to %s X = %f + %f x + %f y\n", chipname, map->x->coeff[0][0], map->x->coeff[1][0], map->x->coeff[0][1]);
    fprintf (stderr, "warp to %s Y = %f + %f x + %f y\n", chipname, map->y->coeff[0][0], map->y->coeff[1][0], map->y->coeff[0][1]);
    
    psFree (map);
    psFree (tgtX);
    psFree (tgtY);
    psFree (srcX);
    psFree (srcY);

    psFree (srcPos);
    psFree (tgtPos);
    psFree (FP);
    psFree (TP);
    psFree (sky);

    psFree (imageRegionString);

    return true;
}

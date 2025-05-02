/** @file ppMergeMask.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppMerge.h"

static bool mergeMask(pmConfig *config, ///< Configuration
                      const pmFPAview *view, ///< View to chip
                      bool writeOut,     ///< Write output?
                      pmHDU **lastHDU,   ///< HDU last updated
                      psRandom *rng,    ///< Random number generator
                      psMetadata *stats ///< Statistics output
    )
{
    assert(config);
    assert(view);
    assert(view->chip != -1 && view->cell == -1 && view->readout == -1);

    bool mdok;                          ///< Status of MD lookup
    int numFiles = psMetadataLookupS32(NULL, config->arguments, "INPUTS.NUM"); ///< Number of input files
    psStatsOptions meanStat = psMetadataLookupS32(NULL, config->arguments, "MEAN"); ///< Statistic for mean
    psStatsOptions stdevStat = psMetadataLookupS32(NULL, config->arguments, "STDEV"); ///< Statistic for stdev
    int sample = psMetadataLookupS32(NULL, config->arguments, "SAMPLE"); ///< Size of sample for statistics
    bool chipStats = psMetadataLookupBool(&mdok, config->arguments, "MASK.CHIPSTATS"); ///< Statistics on chip?

    char *maskSuspectMode  = psMetadataLookupStr(NULL, config->arguments, "MASK.SUSPECT.MODE"); ///< Threshold for suspect pixels
    if (strcasecmp(maskSuspectMode, "SIGMA") && strcasecmp(maskSuspectMode, "VALUE")) {
        psError (PS_ERR_UNKNOWN, true, "Invalid choice for MASK.SUSPECT.MODE: %s\n", maskSuspectMode);
        return false;
    }
    float maskSuspectSigma = psMetadataLookupF32(NULL, config->arguments, "MASK.SUSPECT.SIGMA"); ///< Threshold for suspect pixels
    float maskSuspectMin   = psMetadataLookupF32(NULL, config->arguments, "MASK.SUSPECT.MIN"); ///< Threshold for suspect pixels
    float maskSuspectMax   = psMetadataLookupF32(NULL, config->arguments, "MASK.SUSPECT.MAX"); ///< Threshold for suspect pixels

    float maskBad = psMetadataLookupF32(NULL, config->arguments, "MASK.BAD"); ///< Threshold for bad pixels
    pmMaskIdentifyMode maskMode = psMetadataLookupS32(NULL, config->arguments, "MASK.MODE"); ///< Mode for identifying bad pixels
    int maskGrow = psMetadataLookupS32(NULL, config->arguments, "MASK.GROW"); ///< Radius to grow mask

    bool smoothSuspect = psMetadataLookupBool(&mdok, config->arguments, "MASK.SMOOTH.SUSPECT"); ///< Radius to grow mask
    float smoothScale = psMetadataLookupF32(&mdok, config->arguments, "MASK.SMOOTH.SCALE"); ///< Radius to grow mask

    psImageMaskType markVal, maskValRaw;
    if (!pmConfigMaskSetBits(&maskValRaw, &markVal, config)) {
        psError(PS_ERR_UNKNOWN, true, "Unable to define the mask bit values");
        return false;
    }

    char *maskOutName = psMetadataLookupStr (&mdok, config->arguments, "MASK.SET.VALUE");
    psImageMaskType maskValOut = pmConfigMaskGet (maskOutName, config);
    if (!maskValOut) {
        psError (PS_ERR_UNKNOWN, true, "Undefined output mask bit value");
        return false;
    }

    psStats *statistics = psStatsAlloc(meanStat | stdevStat); // Statistics for background

    psString outName = ppMergeOutputFile(config); ///< Name of output file
    pmChip *outChip = pmFPAfileThisChip(config->files, view, outName); ///< Output chip
    psFree(outName);

    int numCells = 1;
    if (chipStats) {
        // count the number of active cells for this chip:
        numCells = 0;
        for (int i = 0; i < outChip->cells->n; i++) {
            pmCell *cell = outChip->cells->data[i];
            if (!cell->process) continue;
            numCells ++;
        }
    }

    // For each input file, get the statistics, which can be calculated at the chip or cell levels
    psVector *values = psVectorAlloc(sample, PS_TYPE_F32); ///< Pixel values for statistics
    pmFPAview *inView = pmFPAviewAlloc(0); ///< View for input
    for (int i = 0; i < numFiles; i++) {
        pmFPAfileActivate(config->files, false, NULL);
        psArray *files = ppMergeFileActivateSingle(config, PPMERGE_FILES_INPUT, true, i); ///< Input files
        pmFPAfile *input = files->data[0]; ///< Input file
        psFree(files);
        pmFPA *inFPA = input->fpa;  ///< Input FPA
        *inView = *view;

        int valueIndex = 0;             ///< Index for vector of pixel values

        pmCell *inCell;                 ///< Input cell
        while ((inCell = pmFPAviewNextCell(inView, inFPA, 1))) {

            // the output FPA structure carries the information about which cells to process
            pmCell *outCell = pmFPAfileThisCell(config->files, inView, "PPMERGE.OUTPUT.MASK"); // Output cell
            if (!outCell->process) continue;

            pmHDU *hdu = pmHDUFromCell(inCell); ///< HDU for cell
            if (!hdu || hdu->blankPHU) {
                // No data here
                continue;
            }
            psTrace("ppMerge", 1, "Getting suspect pixels for file %d chip %d cell %d",
                    i, inView->chip, inView->cell);

            // Update the header
            {
                pmHDU *hdu = pmHDUGetHighest(outCell->parent->parent, outCell->parent, outCell); // File HDU
                if (hdu && hdu != *lastHDU) {
                    if (!hdu->header) {
                        hdu->header = psMetadataAlloc();
                    }
                    ppMergeVersionHeader(hdu->header);
                    *lastHDU = hdu;
                }
            }

            if (!pmFPAfileIOChecks(config, inView, PM_FPA_BEFORE)) {
                psFree(inView);
                goto MERGE_MASK_ERROR;
            }

            if (!ppMergeFileOpenInput(config, inView, i)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to open file %d", i);
                psFree(inView);
                goto MERGE_MASK_ERROR;
            }

            if (inCell->readouts->n > 1) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "File %d chip %d cell %d contains more than one readout (%ld)",
                        i, inView->chip, inView->cell, inCell->readouts->n);
                psFree(inView);
                goto MERGE_MASK_ERROR;
            }

            pmReadout *readout;
            if (inCell->readouts && inCell->readouts->n == 1) {
                readout = psMemIncrRefCounter(inCell->readouts->data[0]); ///< Input readout
            } else {
                readout = pmReadoutAlloc(inCell);
            }

            if (!ppMergeFileReadInput(config, readout, i, 0)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to read readout %d", i);
                psFree(inView);
                psFree(readout);
                goto MERGE_MASK_ERROR;
            }

            pmReadout *outRO = NULL;    ///< Output readout
            if (outCell->readouts && outCell->readouts->n == 1) {
                outRO = psMemIncrRefCounter(outCell->readouts->data[0]);
            } else {
                outRO = pmReadoutAlloc(outCell);
            }
            psImage *outMask = outRO->mask;    ///< Output mask image (for iterative generation of mask)

            psImage *image = readout->image, *mask = readout->mask; ///< Image and mask
            int numCols = readout->image->numCols, numRows = readout->image->numRows; ///< Image size
            int numPix = numCols * numRows; ///< Number of pixels
            int num = PS_MIN(numPix, sample / numCells); ///< Number of values to add
            if (!chipStats) {
                valueIndex = 0;
            }

	    if (!strcasecmp(maskSuspectMode, "VALUE")) {
		// this function increments the count for each suspect pixel in each input plane
		// maskValRaw is used to test for valid input pixels
		if (!pmMaskFlagSuspectPixelsByValue(outRO, readout, maskSuspectMin, maskSuspectMax, maskValRaw)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to find suspect values in file %d", i);
		    psFree(inView);
		    psFree(readout);
		    goto MERGE_MASK_ERROR;
		}
		pmCellFreeData(inCell);

		if (!pmFPAfileIOChecks(config, inView, PM_FPA_AFTER)) {
		    psFree(inView);
		    psFree(readout);
		    goto MERGE_MASK_ERROR;
		}
	    } else {
		// extract a subset of pixels for stats measurement -- don't use pixels which are masked for this calculation
		for (int i = 0; i < num; i++) {
		    int pixel = numPix * psRandomUniform(rng);
		    int x = pixel % numCols;
		    int y = pixel / numCols;
		    if (mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskValRaw)) continue;
		    if (outMask && (outMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskValOut)) continue;
		    if (!isfinite(image->data.F32[y][x])) continue;

		    values->data.F32[valueIndex] = image->data.F32[y][x];
		    valueIndex++;
		}

		// for per-readout stats, measure the stats and find the suspect pixels
		if (!chipStats) {
		    values->n = valueIndex;
		    if (!psVectorStats(statistics, values, NULL, NULL, 0)) {
			psError(PS_ERR_UNKNOWN, false, "Unable to do statistics on readout.");
			psFree(inView);
			psFree(readout);
			goto MERGE_MASK_ERROR;
		    }
		    float mean = psStatsGetValue(statistics, meanStat);
		    float stdev = psStatsGetValue(statistics, stdevStat);

		    // this function increments the count for each suspect pixel in each input plane
		    // maskValRaw is used to test for valid input pixels
		    if (!pmMaskFlagSuspectPixelsBySigma(outRO, readout, mean, stdev, maskSuspectSigma, maskValRaw)) {
			psError(PS_ERR_UNKNOWN, false, "Unable to find suspect values in file %d", i);
			psFree(inView);
			psFree(readout);
			goto MERGE_MASK_ERROR;
		    }
		    pmCellFreeData(inCell);

		    if (!pmFPAfileIOChecks(config, inView, PM_FPA_AFTER)) {
			psFree(inView);
			psFree(readout);
			goto MERGE_MASK_ERROR;
		    }
		}
	    }
            psFree(readout);
            psFree(outRO);
        }

        // Additional run through cells if we want chip-level statistics
	// only used for MASK.SUSPECT.MODE == SIGMA
        if (!strcasecmp(maskSuspectMode, "SIGMA") && chipStats && valueIndex > 0) {
            values->n = valueIndex;
            if (!psVectorStats(statistics, values, NULL, NULL, 0)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to do statistics on chip.");
                goto MERGE_MASK_ERROR;
            }
            inView->cell = -1;
            while ((inCell = pmFPAviewNextCell(inView, inFPA, 1))) {
                // the output FPA structure carries the information about which cells to process
                pmCell *outCell = pmFPAfileThisCell(config->files, inView, "PPMERGE.OUTPUT.MASK"); // Output cell
                if (!outCell->process) continue;

                pmHDU *hdu = pmHDUFromCell(inCell); ///< HDU for cell
                if (!hdu || hdu->blankPHU) continue;

                pmReadout *readout = inCell->readouts->data[0]; ///< Readout of interest

                inView->readout = 0;
                pmReadout *outRO = pmFPAfileThisReadout(config->files, inView, "PPMERGE.OUTPUT.MASK");

                float mean = psStatsGetValue(statistics, meanStat);
                float stdev = psStatsGetValue(statistics, stdevStat);

                if (!pmMaskFlagSuspectPixelsBySigma(outRO, readout, mean, stdev, maskSuspectSigma, maskValRaw)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to find suspect values in file %d", i);
                    goto MERGE_MASK_ERROR;
                }
                pmCellFreeData(inCell);

                inView->readout = -1;
                if (!pmFPAfileIOChecks(config, inView, PM_FPA_AFTER)) {
                    psFree(inView);
                    goto MERGE_MASK_ERROR;
                }
            }
        }
    }
    psFree(inView);
    psFree(statistics); statistics = NULL;
    psFree(values); values = NULL;


    // Another run through the chip to threshold on the suspects
    pmFPAfileActivate(config->files, false, NULL);
    if (writeOut) {
        ppMergeFileActivate(config, PPMERGE_FILES_OUTPUT, true);
    }
    pmFPA *outFPA = outChip->parent;    ///< Output FPA
    pmCell *outCell;                    ///< Output cell
    pmFPAview *outView = pmFPAviewAlloc(0); ///< View into output FPA
    *outView = *view;
    while ((outCell = pmFPAviewNextCell(outView, outFPA, 1))) {

        // skip inactive cells
        if (!outCell->process) continue;

        // pmHDU *hdu = pmHDUFromCell(outCell); ///< HDU for cell
	pmHDU *hdu = pmHDUGetLowest(outFPA, outChip, outCell); ///< HDU for cell
        if (!hdu || hdu->blankPHU) continue;

        psTrace("ppMerge", 1, "Getting bad pixels for chip %d cell %d", outView->chip, outView->cell);

        assert(outCell->readouts && outCell->readouts->n == 1);
        pmReadout *outRO = outCell->readouts->data[0]; ///< Output readout

        if (smoothSuspect) {
            // XXX test output of suspect pixel image
            psImage *suspects = psMetadataLookupPtr(NULL, outRO->analysis, PM_MASK_ANALYSIS_SUSPECT); // Suspect img
            assert (suspects);
            psImageSmooth (suspects, smoothScale, 3); // extend smoothing region to 3-sigma
        }

        // set the bad pixels to the value 'maskVal'
        if (!pmMaskIdentifyBadPixels(outRO, maskValOut, maskBad, maskMode)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to mask bad pixels");
            goto MERGE_MASK_ERROR;
        }

        // Supplementary outputs
	pmCell *countsCell = NULL;
	pmCell *sigmaCell = NULL;
        {
            // The counts image is fairly useless, but it preserves the model
            countsCell = pmFPAfileThisCell(config->files, outView, "PPMERGE.OUTPUT.COUNT");
            pmReadout *countsRO = pmReadoutAlloc(countsCell); ///< Readout with count of inputs per pixel
            countsRO->image = psImageAlloc(outRO->mask->numCols, outRO->mask->numRows, PS_TYPE_F32);
            psImageInit(countsRO->image, numFiles);
            countsRO->data_exists = countsCell->data_exists = countsCell->parent->data_exists = true;
            psFree(countsRO);

            sigmaCell = pmFPAfileThisCell(config->files, outView, "PPMERGE.OUTPUT.SIGMA");
            pmReadout *sigmaRO = pmReadoutAlloc(sigmaCell); ///< Readout with suspect image
            psImage *suspect = psMetadataLookupPtr(NULL, outRO->analysis, PM_MASK_ANALYSIS_SUSPECT);
            sigmaRO->image = psImageCopy(sigmaRO->image, suspect, PS_TYPE_F32);
            psMetadataRemoveKey(outRO->analysis, PM_MASK_ANALYSIS_SUSPECT);
            sigmaRO->data_exists = sigmaCell->data_exists = sigmaCell->parent->data_exists = true;
            psFree(sigmaRO);

        }

        if (maskGrow > 0) {
            psImage *grown = psImageGrowMask(NULL, outRO->mask, maskValOut, maskGrow, maskValOut); ///< Grown mask
            psFree(outRO->mask);
            outRO->mask = grown;
        }

        if (writeOut) {
            if (!pmFPAfileIOChecks(config, outView, PM_FPA_BEFORE)) {
                psFree(outView);
                goto MERGE_MASK_ERROR;
            }

            outRO->data_exists = outCell->data_exists = outChip->data_exists = true;

            // Average concepts
            psList *cells = psListAlloc(NULL); ///< List of cells, for concept averaging
            for (int i = 0; i < numFiles; i++) {
                pmFPAfile *inFile = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i); ///< Input file
                pmCell *inCell = pmFPAviewThisCell(outView, inFile->fpa); ///< Input cell
                psListAdd(cells, PS_LIST_TAIL, inCell);
            }
            if (!pmConceptsAverageCells(outCell, cells, NULL, NULL, true)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to average cell concepts.");
                psFree(cells);
                psFree(outRO);
                psFree(outView);
                return false;
            }
            psFree(cells);

	    // XXX EAM 2016.03.29 : sigmaCell and countsCell need to have their concepts copied from outCell.
	    // This was causing segfaults for VYSOS5; Why did this ever work for SIMTEST?
	    if (!pmConceptsCopyCell(countsCell, outCell)) {
	      psError(PS_ERR_UNKNOWN, false, "Unable to copy cell concepts.");
	      goto MERGE_MASK_ERROR;
	    }

	    if (!pmConceptsCopyCell(sigmaCell, outCell)) {
	      psError(PS_ERR_UNKNOWN, false, "Unable to copy cell concepts.");
	      goto MERGE_MASK_ERROR;
	    }

            // Statistics on the merged cell using a fake image
            outRO->image = psImageAlloc(outRO->mask->numCols, outRO->mask->numRows, PS_TYPE_F32);
            psImageInit(outRO->image, 1.0);
            if (!ppStatsFPA(stats, outRO->parent->parent->parent, outView, maskValOut, config)) {
                psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to generate stats for image.");
                psFree(outRO);
                psFree(outView);
                return false;
            }
            psFree(outRO->image);
            outRO->image = NULL;

            // Write
            if (!pmFPAfileIOChecks(config, outView, PM_FPA_AFTER)) {
                psFree(outView);
                goto MERGE_MASK_ERROR;
            }
        }
    }
    psFree(outView);

    ppMergeFileActivate(config, PPMERGE_FILES_ALL, true);

    return true;


MERGE_MASK_ERROR:
    psFree(statistics);
    psFree(values);
    return false;
}

bool ppMergeMask(pmConfig *config)
{
    assert(config);

    bool mdok;                          // Status of MD lookup
    int numFiles = psMetadataLookupS32(NULL, config->arguments, "INPUTS.NUM"); // Number of inputs
    bool haveMasks = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS"); // Do we have masks?
    bool haveVariances = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.VARIANCES"); // Got variances?
    int iter = psMetadataLookupS32(NULL, config->arguments, "ITER"); // Number of rejection iterations

    PS_ASSERT_INT_POSITIVE(iter, false);

    pmFPAview *view = pmFPAviewAlloc(0); ///< View to component of interest
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); ///< Random number generator

    psMetadata *stats = NULL;           ///< Statistics for output
    if (psMetadataLookup(config->arguments, "STATS.NAME")) {
        stats = psMetadataAlloc();
        psMetadataAddMetadata(config->arguments, PS_LIST_TAIL, "STATS.DATA", 0, "Statistics output", stats);
    }

    psArray *inputs = ppMergeFileDataLevel(config, "PPMERGE.INPUT"); ///< Input images
    psFree(inputs);
    if (haveMasks) {
        psArray *masks = ppMergeFileDataLevel(config, "PPMERGE.INPUT.MASK");
        psFree(masks);
    }
    if (haveVariances) {
        psArray *variances = ppMergeFileDataLevel(config, "PPMERGE.INPUT.VARIANCE");
        psFree(variances);
    }

    if (!ppMergeFileActivate(config, PPMERGE_FILES_INPUT, true)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to activate files.");
        goto PPMERGE_MASK_ERROR;
    }
    if (!ppMergeFileActivate(config, PPMERGE_FILES_OUTPUT, true)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to activate files.");
        goto PPMERGE_MASK_ERROR;
    }

    // XXX this function should use pmConfigMaskReadHeader () to get the named values defined
    // for the input masks.

    psString outName = ppMergeOutputFile(config); ///< Name of output file
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, outName); ///< Output file
    psFree(outName);
    assert(output && output->fpa);
    pmFPA *outFPA = output->fpa;        ///< Output FPA
    pmHDU *lastHDU = NULL;              // Last HDU updated

    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        goto PPMERGE_MASK_ERROR;
    }
    pmChip *outChip;                    ///< Chip of interest
    while ((outChip = pmFPAviewNextChip(view, outFPA, 1))) {

        if (!outChip->process) continue;

        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            goto PPMERGE_MASK_ERROR;
        }

        for (int i = 0; i < iter; i++) {
            if (!mergeMask(config, view, (i == iter - 1), &lastHDU, rng, stats)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to merge chip %d", view->chip);
                goto PPMERGE_MASK_ERROR;
            }
        }

        if (outChip->data_exists) {
            psList *inChips = psListAlloc(NULL);
            for (int i=0; i < numFiles; i++) {
                pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i); // Input file
                pmChip *chip = pmFPAviewThisChip(view, file->fpa);
                psListAdd(inChips, PS_LIST_TAIL, chip);
            }

            // XXX I need to call pmConfigMaskWriteHeader for the PHU somewhere, after it is created!

            if (!pmConceptsAverageChips(outChip, inChips, true)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to average Chip concepts.");
                psFree(inChips);
                goto PPMERGE_MASK_ERROR;
            }
            psFree(inChips);
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            goto PPMERGE_MASK_ERROR;
        }
    }

    psList *fpaList = psListAlloc(NULL);///< List of FPAs for concept averaging
    for (int i = 0; i < numFiles; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i); ///< Input file
        psListAdd(fpaList, PS_LIST_TAIL, file->fpa);
    }
    if (!pmConceptsAverageFPAs(outFPA, fpaList)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to average FPA concepts.");
        psFree(fpaList);
        goto PPMERGE_MASK_ERROR;
    }
    psFree(fpaList);

    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        goto PPMERGE_MASK_ERROR;
    }

    psFree(stats);
    psFree(view);
    psFree(rng);

    return true;

PPMERGE_MASK_ERROR:
    psFree(stats);
    psFree(view);
    psFree(rng);
    return false;
}


/** @file ppMergeLoop_Threaded.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

#include "ppMerge.h"

// XXX this function is now sufficiently different for the major types, it would make sense to just
// split it into three: BASIC, SHUTTER, DARK

bool ppMergeLoop(pmConfig *config)
{
    assert(config);

    psMetadata *arguments = config->arguments; ///< Arguments
    ppMergeType type = psMetadataLookupS32(NULL, config->arguments, "TYPE"); ///< Type of frame
    int numFiles = psMetadataLookupS32(NULL, arguments, "INPUTS.NUM"); ///< Number of input files
    bool mdok;                          ///< Status of MD lookup
    bool haveMasks = psMetadataLookupBool(&mdok, arguments, "INPUTS.MASKS"); // Do we have masks?
    bool useMasks = psMetadataLookupBool(&mdok, arguments, "INPUTS.MASKS.USE"); // Do we have masks?
    bool haveVariances = psMetadataLookupBool(&mdok, arguments, "INPUTS.VARIANCES"); // Do we have variances?

    psArray *inputs    = ppMergeFileDataLevel(config, "PPMERGE.INPUT"); // Input images
    psArray *masks     = haveMasks ? ppMergeFileDataLevel(config, "PPMERGE.INPUT.MASK") : NULL; // Input masks 
    psArray *variances = haveVariances ? ppMergeFileDataLevel(config, "PPMERGE.INPUT.VARIANCE") : NULL; // Input variances

    int nThreads = psMetadataLookupS32 (&mdok, arguments, "NTHREADS");
    if (!mdok) nThreads = 0;

    // General combination parameters
    int iter = psMetadataLookupS32(NULL, arguments, "ITER"); // Number of rejection iterations
    float rej = psMetadataLookupF32(NULL, arguments, "REJ"); // Rejection level
    float fraclow = psMetadataLookupF32(NULL, arguments, "FRACLOW"); // Reject fraction of low pixels
    float frachigh = psMetadataLookupF32(NULL, arguments, "FRACHIGH"); // Reject fraction of hi pixels
    int nKeep = psMetadataLookupS32(NULL, arguments, "NKEEP"); // Minimum number of values to keep
    psStatsOptions combineStat = psMetadataLookupS32(NULL, arguments, "COMBINE"); // Combination statistic
    bool useVariances = psMetadataLookupBool(NULL, arguments, "VARIANCES"); // Use variances?

    // Fringe parameters
    int fringeNum = psMetadataLookupS32(NULL, arguments, "FRINGE.NUM"); // Number of fringe points
    int fringeSize = psMetadataLookupS32(NULL, arguments, "FRINGE.SIZE"); // Size of fringe regions
    int fringeSmoothX = psMetadataLookupS32(NULL, arguments, "FRINGE.XSMOOTH"); // Smoothing regions in x
    int fringeSmoothY = psMetadataLookupS32(NULL, arguments, "FRINGE.YSMOOTH"); // Smoothing regions in y
    bool fringeSmooth = psMetadataLookupBool(NULL, arguments, "FRINGE.SMOOTH"); // Smooth the output image?
    float fringeSmoothSigma = psMetadataLookupF32(NULL, arguments, "FRINGE.SMOOTH.SIGMA"); // Smooth the output image?

    // set the mask and mark bit values based on the named masks
    psImageMaskType maskVal, markVal;
    if (!pmConfigMaskSetBits(&maskVal, &markVal, config)) {
        psError (PS_ERR_UNKNOWN, true, "Unable to define the mask bit values");
        return false;
    }

    pmCombineParams *combination = pmCombineParamsAlloc(combineStat); ///< Combination parameters
    combination->maskVal = useMasks ? maskVal : 0;
    combination->blank = pmConfigMaskGet("BLANK", config);
    combination->nKeep = nKeep;
    combination->fracHigh = frachigh;
    combination->fracLow = fraclow;
    combination->iter = iter;
    combination->rej = rej;
    combination->variances = useVariances;

    psMetadata *stats = NULL;           ///< Statistics for output
    if (psMetadataLookup(config->arguments, "STATS.NAME")) {
        stats = psMetadataAlloc();
        psMetadataAddMetadata(config->arguments, PS_LIST_TAIL, "STATS.DATA", 0, "Statistics output", stats);
    }

    pmFPAview *view = pmFPAviewAlloc(0); // View to component of interest

    // Retrieve data placed on analysis
    psVector *scales = NULL, *zeros = NULL; ///< Scale and zeroes for combination
    psArray *shutters = NULL;           ///< Shutter correction data
    psImage *zeroSet = NULL;
    switch (type) {
      case PPMERGE_TYPE_FRINGE:
        zeroSet = psMetadataLookupPtr(NULL, arguments, "ZEROS");
        if (!zeroSet) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find ZEROS");
            goto ERROR;
        }
        // the zeros vector is passed to pmReadoutCombine for each set of inputs per cell
        zeros = psVectorAlloc(zeroSet->numRows, PS_TYPE_F32);
        // Flow through
      case PPMERGE_TYPE_FLAT:
        scales = psMetadataLookupPtr(NULL, arguments, "SCALES");
        if (!scales) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find SCALES");
            goto ERROR;
        }
        break;
      case PPMERGE_TYPE_SHUTTER:
        shutters = psMetadataLookupPtr(NULL, arguments, "SHUTTER");
        if (!shutters) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find SHUTTER");
            goto ERROR;
        }
        break;
      case PPMERGE_TYPE_MASK:
      case PPMERGE_TYPE_BIAS:
      case PPMERGE_TYPE_DARK:
      case PPMERGE_TYPE_CTEMASK:
      case PPMERGE_TYPE_NOISEMAP:
        break;
      default:
        psAbort("Should never get here.");
    }

    // Dark parameters
    psArray *darkOrdinates = psMetadataLookupPtr(NULL, arguments, "DARK.ORDINATES"); ///< Dark info
    psString darkNorm = psMetadataLookupStr(&mdok, arguments, "DARK.NORM"); ///< Dark normalisation

    if (!ppMergeFileActivate(config, PPMERGE_FILES_ALL, true)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to activate files.");
        goto ERROR;
    }
    psString outName = ppMergeOutputFile(config); ///< Name of output file
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, outName); ///< Output file
    psFree(outName);
    assert(output && output->fpa);
    pmFPA *outFPA = output->fpa;        ///< Output FPA
    pmHDU *lastHDU = NULL;              // Last HDU that was updated

    int cellNum = 0;                    ///< Index of cell
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        goto ERROR;
    }

    // Average concepts across inputs
    {
        psList *inFPAs = psListAlloc(NULL); ///< List of FPAs
        for (int i = 0; i < numFiles; i++) {
            pmFPAfile *input = inputs->data[i]; ///< Input file
            psListAdd(inFPAs, PS_LIST_TAIL, input->fpa);
        }
        if (!pmConceptsAverageFPAs(output->fpa, inFPAs)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to average FPA concepts.");
            psFree(inFPAs);
            goto ERROR;
        }
        psFree(inFPAs);
    }

    pmChip *outChip;                    ///< Chip of interest
    while ((outChip = pmFPAviewNextChip(view, outFPA, 1))) {
        if (!outChip->process || !outChip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            goto ERROR;
        }

        // Average concepts across inputs
        {
            psList *inChips = psListAlloc(NULL);
            for (int i = 0; i < numFiles; i++) {
                pmChip *chip = pmFPAviewThisChip(view, ((pmFPAfile *)inputs->data[i])->fpa);
                psListAdd(inChips, PS_LIST_TAIL, chip);
            }
            if (!pmConceptsAverageChips(outChip, inChips, true)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to average Chip concepts.");
                psFree(inChips);
                goto ERROR;
            }
            psFree(inChips);
        }

        pmCell *outCell;                ///< Cell of interest

        while ((outCell = pmFPAviewNextCell(view, outFPA, 1))) {
            if (!outCell->process || !outCell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                goto ERROR;
            }

            pmHDU *hdu = pmHDUGetLowest(outFPA, outChip, outCell); ///< HDU for cell
            if (!hdu || hdu->blankPHU) {
                // No data here
                continue;
            }

            // Update the header
            {
                pmHDU *hdu = pmHDUGetHighest(outFPA, outChip, outCell); // HDU for file
                if (hdu && hdu != lastHDU) {
                    if (!hdu->header) {
                        hdu->header = psMetadataAlloc();
                    }
                    ppMergeVersionHeader(hdu->header);
                    lastHDU = hdu;
                }
            }

            float shutterRef = NAN;     ///< Reference shutter correction
            pmReadout *pattern = NULL;
            if (type == PPMERGE_TYPE_SHUTTER) {
                shutterRef = pmShutterCorrectionReference(shutters->data[cellNum]);
                pattern = pmReadoutAlloc(NULL);
            }

            pmReadout *outRO = pmReadoutAlloc(outCell);

            // open the input files (we need to do the work ourselves)
            for (int i = 0; i < numFiles; i++) {
                if (!ppMergeFileOpenInput(config, view, i)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to open file %d", i);
                    goto ERROR;
                }
            }

            if (zeroSet) {
              for (int i = 0; i < zeroSet->numRows; i++) {
                zeros->data.F32[i] = zeroSet->data.F32[i][cellNum];
              }
            }

            int rows = psMetadataLookupS32(NULL, config->arguments, "ROWS"); // Number of rows to read per chunk
            if (!rows && nThreads) {
              psError(PS_ERR_UNKNOWN, false, "Invalid combination of threads > 0 and ROWS == 0 (ie, multiple threads working on the full array...)");
              goto ERROR;
            }

            ppMergeFileGroup *fileGroup = NULL;
            psArray *fileGroups = psArrayAlloc(nThreads + 1);

            // Generate readouts for each input file in each file group
            for (int i = 0; i < fileGroups->n; i++) {
                psArray *readouts = psArrayAlloc(numFiles); ///< Input readouts
                for (int j = 0; j < numFiles; j++) {
                    pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", j);
                    pmCell *inCell = pmFPAviewThisCell(view, input->fpa); ///< Input cell
                    pmReadout *readout = pmReadoutAlloc(inCell);
                    readout->process = true; // until proven otherwise, attempt to process this readout
                    readouts->data[j] = readout;
                }

                fileGroup = ppMergeFileGroupAlloc();
                fileGroup->readouts = readouts;
                fileGroup->read = false;
                fileGroup->busy = false;
                fileGroup->lastScan = 0;
                fileGroup->firstScan = 0;
                fileGroups->data[i] = fileGroup;
            }

            // call the init functions
            switch (type) {
              case PPMERGE_TYPE_CTEMASK:
              case PPMERGE_TYPE_BIAS:
              case PPMERGE_TYPE_FLAT:
              case PPMERGE_TYPE_FRINGE:
  	      case PPMERGE_TYPE_NOISEMAP:
                psAssert (fileGroups->n > 0, "no valid file groups defined");
                fileGroup = fileGroups->data[0];
                if (!pmReadoutCombinePrepare(outRO, fileGroup->readouts, combination)) {
                    goto ERROR;
                }
                break;
              case PPMERGE_TYPE_DARK:
                psAssert (fileGroups->n > 0, "no valid file groups defined");
                fileGroup = fileGroups->data[0];

                if (!pmDarkCombinePrepare(outCell, fileGroup->readouts, darkOrdinates, darkNorm)) {
                    goto ERROR;
                }
                break;
              case PPMERGE_TYPE_SHUTTER:
                psAssert (fileGroups->n > 0, "no valid file groups defined");
                fileGroup = fileGroups->data[0];
                if (!pmShutterCorrectionGeneratePrepare(outRO, pattern, fileGroup->readouts, combination->maskVal)) {
                    goto ERROR;
                }
                break;
              default:
                psAbort("Should never get here.");
            }
	    fprintf (stderr, "after pmDarkCombinePrepare / pmReadoutCombinePrepare:\n");
	    psMemStats(true, NULL, NULL);

            // Read input data by chunks
            psTimerStart("ppMergeLoop");
            for (int numChunk = 0; true; numChunk++) {

                bool status = false;
                fileGroup = ppMergeReadChunk(&status, fileGroups, config, numChunk);
                if (!status) {
                    // Something went wrong
                    goto ERROR;
                }
                if (!fileGroup) {
                    // Nothing more to read
                    break;
                }

                // Start a job
                switch (type) {
                  case PPMERGE_TYPE_CTEMASK:
                  case PPMERGE_TYPE_BIAS:
                  case PPMERGE_TYPE_FLAT:
  		  case PPMERGE_TYPE_NOISEMAP:
                  case PPMERGE_TYPE_FRINGE: {
                      psThreadJob *job = psThreadJobAlloc("PPMERGE_READOUT_COMBINE"); ///< Job to start

                      // Construct the arguments for this job
                      psArrayAdd(job->args, 1, outRO);
                      psArrayAdd(job->args, 1, fileGroup);
                      psArrayAdd(job->args, 1, zeros);
                      psArrayAdd(job->args, 1, scales);
                      psArrayAdd(job->args, 1, combination);

                      // call: pmReadoutCombine(outRO, fileGroup->readouts, zeros, scales, combination);
                      if (!psThreadJobAddPending(job)) {
                          goto ERROR;
                      }
                      break;
                  }
                  case PPMERGE_TYPE_DARK: {
                      psThreadJob *job = psThreadJobAlloc ("PPMERGE_DARK_COMBINE"); ///< Job to start

                      // construct the arguments for this job
                      psArrayAdd(job->args, 1, outCell);
                      psArrayAdd(job->args, 1, fileGroup);
		      
		      // need to free the allocated scalars:
		      { psScalar *tmpVal = psScalarAlloc(iter,    PS_TYPE_S32);        psArrayAdd(job->args, 1, tmpVal); psFree (tmpVal); }
		      { psScalar *tmpVal = psScalarAlloc(rej,     PS_TYPE_F32);        psArrayAdd(job->args, 1, tmpVal); psFree (tmpVal); }
		      { psScalar *tmpVal = psScalarAlloc(maskVal, PS_TYPE_IMAGE_MASK); psArrayAdd(job->args, 1, tmpVal); psFree (tmpVal); }

                      // call: pmDarkCombine(outCell, fileGroup->readouts, iter, rej, maskVal);
                      if (!psThreadJobAddPending(job)) {
			// CZW commented this out with a note that this was needed to allow ppMerge to work even if some inputs are invalid.
			// EAM thinks pmDarkCombine will only return FALSE if there is a programming or setup error (see pmDark.c:pmDarkCombine)
			// if so, we should raise an error here.
			// goto ERROR;
                      }
                      break;
                  }
                  case PPMERGE_TYPE_SHUTTER: {
                      psThreadJob *job = psThreadJobAlloc ("PPMERGE_SHUTTER_CORRECTION");

                      // construct the arguments for this job
                      psArrayAdd(job->args, 1, outRO);
                      psArrayAdd(job->args, 1, pattern);
                      psArrayAdd(job->args, 1, fileGroup);
                      psArrayAdd(job->args, 1, psScalarAlloc(shutterRef, PS_TYPE_F32));
                      psArrayAdd(job->args, 1, shutters->data[cellNum]);
                      psArrayAdd(job->args, 1, psScalarAlloc(iter, PS_TYPE_S32));
                      psArrayAdd(job->args, 1, psScalarAlloc(rej, PS_TYPE_F32));
                      psArrayAdd(job->args, 1, psScalarAlloc(maskVal, PS_TYPE_IMAGE_MASK));

                      // call: pmShutterCorrectionGenerate(outRO, pattern, fileGroup->readouts, shutterRef,
                      //                                   shutters->data[cellNum], iter, rej, maskVal);
                      if (!psThreadJobAddPending (job)) {
                          goto ERROR;
                      }
                      break;
                  }
                  default:
                    psAbort("Should never get here.");
                }
            }

            // Wait for the threads to finish and manage results
            if (!psThreadPoolWait(false, true)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to combine images.");
                return false;
            }

            // we don't care about the results, just dump the done queue jobs
            psThreadJob *job = NULL;    // Job to dump
            while ((job = psThreadJobGetDone())) {
                psFree (job);
            }

            psFree(fileGroups);

            // XXX eventually need to keep both the shutter and the pattern, as we do with dark
            psFree(pattern);

            // Get list of cells for concepts averaging
            psList *inCells = psListAlloc(NULL); // List of cells
            for (int i = 0; i < numFiles; i++) {
                pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i);
                pmCell *inCell = pmFPAviewThisCell(view, input->fpa); ///< Input cell
                psListAdd(inCells, PS_LIST_TAIL, inCell);
            }
            if (!pmConceptsAverageCells(outCell, inCells, NULL, NULL, true)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to average cell concepts.");
                psFree(inCells);
                psFree(outRO);
                goto ERROR;
            }
            psFree(inCells);

            // Plug supplementary images into their own FPAs
            {
                pmCell *countsCell = pmFPAfileThisCell(config->files, view, "PPMERGE.OUTPUT.COUNT");
                pmReadout *countsRO = pmReadoutAlloc(countsCell); ///< Readout with count of inputs per pixel
                psImage *counts = psMetadataLookupPtr(NULL, outRO->analysis, PM_READOUT_STACK_ANALYSIS_COUNT);
                countsRO->image = psImageCopy(countsRO->image, counts, PS_TYPE_F32);
                psMetadataRemoveKey(outRO->analysis, PM_READOUT_STACK_ANALYSIS_COUNT);
                countsRO->data_exists = countsCell->data_exists = countsCell->parent->data_exists = true;
                psFree(countsRO);

                // XXX EAM 2009.01.18 : sigmaCell and countsCell need to have their concepts copied from outCell.
                // This was causing segfaults for VYSOS5; Why did this ever work for SIMTEST?
                if (!pmConceptsCopyCell(countsCell, outCell)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to copy cell concepts.");
                    psFree(outRO);
                    goto ERROR;
                }

                pmCell *sigmaCell = pmFPAfileThisCell(config->files, view, "PPMERGE.OUTPUT.SIGMA");
                pmReadout *sigmaRO = pmReadoutAlloc(sigmaCell); ///< Readout with stdev per pixel
                psImage *sigma = psMetadataLookupPtr(NULL, outRO->analysis, PM_READOUT_STACK_ANALYSIS_SIGMA);
                sigmaRO->image = psImageCopy(sigmaRO->image, sigma, PS_TYPE_F32);
                psMetadataRemoveKey(outRO->analysis, PM_READOUT_STACK_ANALYSIS_SIGMA);
                sigmaRO->data_exists = sigmaCell->data_exists = sigmaCell->parent->data_exists = true;
                psFree(sigmaRO);

                if (!pmConceptsCopyCell(sigmaCell, outCell)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to copy cell concepts.");
                    psFree(outRO);
                    goto ERROR;
                }
            }

            // Measure the fringes for this cell
            //
            // XXX Need to deal with multiple components: we will do this by building up the components
            // Read the existing fringe measurements
            // Use existing regions to measure fringe statistics
            // Add the new fringe measurements to the existing fringe measurements.
            // Write the appended fringe measurements.
            // Read in the "output" file to get the existing components.
            // Put the new readout into the cell after the existing readouts.
            if (type == PPMERGE_TYPE_FRINGE && outRO) {
                if (fringeSmooth) {
                    if (outRO->mask) {
                        psImage *smoothed = psImageSmoothMask (NULL, outRO->image, outRO->mask, maskVal, fringeSmoothSigma, 3, 0.2);
                        psFree (outRO->image);
                        outRO->image = smoothed;
                    } else {
                        psImageSmooth (outRO->image, fringeSmoothSigma, 3);
                    }
                }

                pmFringeRegions *regions = pmFringeRegionsAlloc(fringeNum, fringeSize, fringeSize, fringeSmoothX, fringeSmoothY);
                pmFringeStats *fringe = pmFringeStatsMeasure(regions, outRO, maskVal);
                psFree(regions);
                if (!fringe) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to measure fringe statistics.\n");
                    psFree(outRO);
                    goto ERROR;
                }

                psArray *fringes = psArrayAlloc(1); ///< Array of fringes
                fringes->data[0] = fringe;

                // XXX replaced this : pmFringesFormat(outCell, NULL, fringes);
                if (!psMetadataAdd(outCell->analysis, PS_LIST_TAIL, "FRINGE.MEASUREMENTS", PS_DATA_ARRAY, "Fringes", fringes)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to add fringe to analysis metadata\n");
                    goto ERROR;
                }
                psFree(fringes);        // Drop reference
            }

            if (stats && !ppStatsFPA(stats, outFPA, view, maskVal, config)) {
                psError(PS_ERR_UNKNOWN, true, "Unable to generate stats for image.");
                goto ERROR;
            }

            // calculate CTEMASK after stats so stats reflect median image
            if (type == PPMERGE_TYPE_CTEMASK && outRO) {
                // need to apply range cuts on the output image
                psAssert (outRO->mask, "mask is not defined");
                psAssert (outRO->mask->numCols == outRO->image->numCols, "mismatch between image and mask");
                psAssert (outRO->mask->numRows == outRO->image->numRows, "mismatch between image and mask");

                // CTEMASK parameters
                float cteMin = psMetadataLookupF32(NULL, arguments, "CTE.MIN"); // Number of fringe points

                char *cteMaskName = psMetadataLookupStr (&mdok, config->arguments, "MASK.SET.VALUE");
                psImageMaskType cteMaskValue = pmConfigMaskGet(cteMaskName, config);

                if (0) {
                  psFits *fits = NULL;
                  fits = psFitsOpen ("combine.fits", "w");
                  psFitsWriteImage (fits, NULL, outRO->image, 0, NULL);
                  psFitsClose (fits);

                  fits = psFitsOpen ("inmask.fits", "w");
                  psFitsWriteImage (fits, NULL, outRO->mask, 0, NULL);
                  psFitsClose (fits);
                }

                psF32 **outputImage = outRO->image->data.F32;
                psImageMaskType **outputMask = outRO->mask->data.PS_TYPE_IMAGE_MASK_DATA;
                for (int iy = 0; iy < outRO->image->numRows; iy++) {
                    for (int ix = 0; ix < outRO->image->numCols; ix++) {
                        if (outputImage[iy][ix] < cteMin) {
                            outputMask[iy][ix] |= cteMaskValue;
                        }
                    }
                }

                if (0) {
                  psFits *fits = NULL;
                  fits = psFitsOpen ("otmask.fits", "w");
                  psFitsWriteImage (fits, NULL, outRO->mask, 0, NULL);
                  psFitsClose (fits);
                }

            }

            // free the data used by the input files (we need to do the work ourselves)
            for (int i = 0; i < numFiles; i++) {
                if (!ppMergeFileFreeInput(config, view, i)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to open file %d", i);
                    goto ERROR;
                }
            }

            psFree(outRO);
            cellNum++;

            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                goto ERROR;
            }

	    // pmFPAfileIOList (config);
        }

        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            goto ERROR;
        }
    }

    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        goto ERROR;
    }

    psFree(view);
    psFree(combination);
    psFree(inputs);
    psFree(masks);
    psFree(variances);
    psFree(stats);
    psFree(zeros);
    return true;

ERROR:
    psFree(view);
    psFree(combination);
    psFree(inputs);
    psFree(masks);
    psFree(variances);
    psFree(stats);
    psFree(zeros);
    return false;
}


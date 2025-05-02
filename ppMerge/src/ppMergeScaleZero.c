/** @file ppMergeScaleZero.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.31 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppMerge.h"

/**
 * Get the scale and zero for each chip of each input
 */
bool ppMergeScaleZero(pmConfig *config)
{
    assert(config);

    ppMergeType type = psMetadataLookupS32(NULL, config->arguments, "TYPE"); ///< Type of frame
    int numInputs = psMetadataLookupS32(NULL, config->arguments, "INPUTS.NUM"); ///< Number of inputs
    int numCells = psMetadataLookupS32(NULL, config->arguments, "INPUTS.CELLS"); ///< Number of cells
    psStatsOptions meanStat = psMetadataLookupS32(NULL, config->arguments, "MEAN"); ///< Statistic for mean
    psStatsOptions stdevStat = psMetadataLookupS32(NULL, config->arguments, "STDEV"); ///< Statistic for stdev
    int shutterSize = psMetadataLookupS32(NULL, config->arguments, "SHUTTER.SIZE"); ///< Size of shutter region

    psVector *gains = NULL;             ///< Gains for each cell
    psArray *shutters = NULL;           ///< Shutter data for each cell
    psStats *stats = NULL;              ///< Statistics for background
    psImage *background = NULL;         ///< Background measurements per cell per file

    switch (type) {
      case PPMERGE_TYPE_BIAS:
      case PPMERGE_TYPE_DARK:
      case PPMERGE_TYPE_CTEMASK:
      case PPMERGE_TYPE_NOISEMAP:
        // Nothing to measure
        return true;
      case PPMERGE_TYPE_FLAT:
      case PPMERGE_TYPE_FRINGE:
        gains = psVectorAlloc(numCells, PS_TYPE_F32);
        background = psImageAlloc(numCells, numInputs, PS_TYPE_F32);
        psImageInit(background, NAN);
        stats = psStatsAlloc(meanStat);
        break;
      case PPMERGE_TYPE_SHUTTER:
        shutters = psArrayAlloc(numCells);
        break;
      case PPMERGE_TYPE_MASK:
      default:
        break;
    }
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); ///< Random number generator
    pmFPAview *view = NULL;             ///< View into FPA

    for (int i = 0; i < numInputs; i++) {
        pmFPAfileActivate(config->files, false, NULL);
        psArray *files = ppMergeFileActivateSingle(config, PPMERGE_FILES_INPUT, true, i); // Activated files
        pmFPAfile *input = files->data[0]; // Representative file; should be the image (not mask or variance)
        pmFPA *fpa = input->fpa;        // FPA of interest
        view = pmFPAviewAlloc(0);       // View to component of interest
        int cellNum = 0;                // Index for cell
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            goto ERROR;
        }
        pmChip *chip;                   ///< Chip of interest
        while ((chip = pmFPAviewNextChip(view, fpa, 1))) {
            if (!chip->process || !chip->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                goto ERROR;
            }

            pmCell *cell;               ///< Cell of interest
            while ((cell = pmFPAviewNextCell(view, fpa, 1))) {
                if (!cell->process || !cell->file_exists) {
                    continue;
                }
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    goto ERROR;
                }

                if (!cell->data_exists) {
                    continue;
                }

                // skip cells with video data
                if (cell->readouts->n > 1) {
                    // psError(PS_ERR_BAD_PARAMETER_VALUE, true, "File %d chip %d cell %d contains more than one readout (%ld)", i, view->chip, view->cell, cell->readouts->n);
                    // goto ERROR;
                  psWarning("File %d chip %d cell %d contains more than one readout (%ld), skipping",
                            i, view->chip, view->cell, cell->readouts->n);
                  continue;
                }
                pmReadout *readout = cell->readouts->data[0]; ///< Readout of interest

                psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); ///< Value to mask

                switch (type) {
                  case PPMERGE_TYPE_FLAT:
                  case PPMERGE_TYPE_FRINGE: {
                      // Extract the gain
                      float gain = psMetadataLookupF32(NULL, cell->concepts, "CELL.GAIN"); ///< Cell gain
                      if (!isfinite(gain)) {
                          psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes,
                                                                        PPMERGE_RECIPE); // Recipe
                          psAssert(recipe, "Should be there!");
                          bool override = psMetadataLookupBool(NULL, recipe,
                                                               "GAIN.OVERRIDE"); // Override the bad gain?
                          if (override) {
                              psWarning("CELL.GAIN is not set for readout (%d,%d,%d) on file %d "
                                        "--- setting to unity.",
                                        view->chip, view->cell, view->readout, i);
                              psMetadataItem *item = psMetadataLookup(cell->concepts, "CELL.GAIN"); // Item with gain
                              psAssert(item, "Should be there!");
                              item->data.F32 = 1.0;
			      
			      // for unity gain, there is no modification for the readnoise, note that it has (effectively) been updated
			      psMetadataRemoveKey(cell->concepts, "CELL.READNOISE.UPDATE");
                          } else {
                              // psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                              // "CELL.GAIN for file %d chip %d cell %d is not set.",
                              // i, view->chip, view->cell);
                              // goto ERROR;
                              psWarning("CELL.GAIN for file %d chip %d cell %d is NaN",
                                        i, view->chip, view->cell);
                          }
                      }
                      gains->data.F32[cellNum] = gain;

                      // Measure the background
                      if (!psImageBackground(stats, NULL, readout->image, readout->mask, maskVal, rng)) {
                        // psError(PS_ERR_UNKNOWN, false,
                        // "Unable to get statistics for file %d chip %d cell %d",
                        // i, view->chip, view->cell);
                        // goto ERROR;
                        psWarning ("Unable to get statistics for file %d chip %d cell %d",
                                   i, view->chip, view->cell);
                        background->data.F32[i][cellNum] = NAN;
                      } else {
                        background->data.F32[i][cellNum] = psStatsGetValue(stats, meanStat);
                      }
                      break;
                  }
                  case PPMERGE_TYPE_SHUTTER: {
                      pmShutterCorrectionData *shutter = shutters->data[cellNum]; ///< Shutter correction data
                      if (!shutter) {
                          shutter = pmShutterCorrectionDataAlloc(readout->image->numCols,
                                                                 readout->image->numRows,
                                                                 shutterSize);
                          shutters->data[cellNum] = shutter;
                      }
                      if (!pmShutterCorrectionAddReadout(shutter, readout, meanStat, stdevStat,
                                                         maskVal, rng)) {
                          psError(PS_ERR_UNKNOWN, false,
                                  "Can't add file %d chip %d cell %d to shutter correction.",
                                  i, view->chip, view->cell);
                          goto ERROR;
                      }
                      break;
                  }
                  case PPMERGE_TYPE_MASK:
                  default:
                    psAbort("Should never get here.");
                }

                cellNum++;

                if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                    goto ERROR;
                }
            }

            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                goto ERROR;
            }
        }

        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            goto ERROR;
        }

        psFree(view);

#if 0
        // Reset files for reading again
        for (int i = 0; i < files->n; i++) {
            pmFPAfile *file = files->data[i]; // File of interest
        }
#endif
        psFree(files);
    }

    psFree(rng); rng = NULL;
    psFree(stats); stats = NULL;

    // Store results
    switch (type) {
      case PPMERGE_TYPE_FRINGE:
        psMetadataAddImage(config->arguments, PS_LIST_TAIL, "ZEROS", 0,
                           "Zero to subtract from each input cell", background);
        // Flow through
      case PPMERGE_TYPE_FLAT: {
          // Need to normalize over the focal plane
          if (psTraceGetLevel("ppMerge") > 9) {
              for (int i = 0; i < gains->n; i++) {
                  psTrace("ppMerge", 10, "Gain for cell %d is %f\n", i, gains->data.F32[i]);
              }
          }
          psVector *fluxes = NULL;        ///< Solution to fluxes
          if (!pmFlatNormalize(&fluxes, &gains, background)) {
              psError(PS_ERR_UNKNOWN, false, "Normalisation failed to converge --- continuing anyway.");
              psFree(fluxes);
              goto ERROR;
          }

          psMetadataAddVector(config->arguments, PS_LIST_TAIL, "SCALES", 0,
                              "Scale to divide into each input file", fluxes);
          psFree(fluxes);               // Drop reference
          break;
      }
      case PPMERGE_TYPE_SHUTTER:
        psMetadataAddArray(config->arguments, PS_LIST_TAIL, "SHUTTER", 0,
                           "Shutter data", shutters);
        break;
      case PPMERGE_TYPE_MASK:
      default:
        psAbort("Should never get here.");
    }

    psFree(gains);
    psFree(background);
    psFree(shutters);
    return true;

ERROR:
    // Common path for errors
    psFree(gains);
    psFree(background);
    psFree(shutters);
    psFree(rng);
    psFree(stats);
    psFree(view);
    return false;
}


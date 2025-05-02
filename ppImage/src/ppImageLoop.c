#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

#define ESCAPE(MESSAGE) { \
  psError(PS_ERR_UNKNOWN, false, MESSAGE); \
  pmFPAfileIOChecks(config, view, PM_FPA_AFTER); \
  psFree(view); \
  psFree(stats); \
  pmFPAfileFreeSetStrict(false); \
  return false; \
}

bool ppImageLoop(pmConfig *config, ppImageOptions *options)
{
    psMetadata *stats = NULL;           // Statistics to output
    float timeDetrend = 0;              // Amount of time spent in detrend
    float timePhot = 0;                 // Amount of time spent in photometry

    if (options->doStats) {
        stats = psMetadataAlloc();
        psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", 0, "No problems", 0);
    }

    bool status;                        // Status of MD lookup
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPIMAGE.INPUT");
    if (!status) {
        psErrorStackPrint(stderr, "Can't find input data!\n");
        ppImageCleanup(config, options);
        exit(PS_EXIT_PROG_ERROR);
    }

    pmConfigCamerasCull(config, NULL);
    pmConfigRecipesCull(config, "PPIMAGE,PPSTATS,PSPHOT,MASKS,PSASTRO,JPEG");

    pmFPAview *view = pmFPAviewAlloc(0);// View for level of interest
    pmHDU *lastHDU = NULL;              // Last HDU that was updated

    // files associated with the science image
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        ESCAPE("load failure for FPA");
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1)) != NULL) {
        psLogMsg ("ppImageLoop", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            ESCAPE("load failure for Chip");
        }

        // crosstalk measurement needs to be done on the entire chip at once, and before
        // signal levels are modified by the detrending.  If crosstalk measurement is
        // requested, the read-level for the images is set to CHIP.
        if (!ppImageMeasureCrosstalk(config, options, view)) {
          ESCAPE("Unable to perform crosstalk correction");
        }

        // crosstalk correction needs to be done on the entire chip at once, and before
        // signal levels are modified by the detrending.  If crosstalk correction is
        // requested, the read-level for the images is set to CHIP.
        if (!ppImageCorrectCrosstalk(config, options, view)) {
          ESCAPE("Unable to perform crosstalk correction");
        }

        psTimerStart(TIMER_DETREND);
        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
            psLogMsg ("ppImageLoop", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                ESCAPE("load failure for Cell");
            }

            // Put version information into the header
            pmHDU *hdu = pmHDUGetHighest(input->fpa, chip, cell);
            if (hdu && hdu != lastHDU) {
                ppImageVersionHeader(hdu->header);
                lastHDU = hdu;
            }

            // XXX for now, skip the video cells (cell->readouts->n > 1)
            if (cell->readouts->n > 1) {
              psWarning ("Skipping Video Cell for ppImageDetrendReadout");
              continue;
            }

            // process each of the readouts
            pmReadout *readout;         // Readout from cell
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    ESCAPE("load failure for Readout");
                }
                if (!readout->data_exists) {
                    continue;
                }

                // flip the image to match the native detector orientation (to match bias, flat, etc)
                if (!ppImageParityFlip(config, options, view, true)) {
                    ESCAPE("Unable to detrend readout");
                }

                // XXX set the options->*Mask values here (after the mask images have been loaded
                // and before any of the value are used)
                if (!ppImageSetMaskBits(config, options)) {
                    ESCAPE("Unable to set bit masks");
                }

                // perform the detrend analysis
                if (!ppImageDetrendReadout(config, options, view)) {
                    ESCAPE("Unable to detrend readout");
                }

                // free detrend images potentially in use: MASK, BIAS, DARK, SHUTTER, FLAT
                if (!ppImageDetrendFree (config, view)) {
                    ESCAPE("Unable to free detrend images");
                }

                // optionally measure CTE by examining the variance in a box
                if (!ppImageCheckCTE (config, options, view)) {
                    ESCAPE("Unable to measure CTE");
                }

		if (!ppImageCheckNoise (config, options, view)) {
		    ESCAPE("Unable to generate noisemap");
		}
		
		// optionally degrade a MD image to 3pi exposure times
		if (!ppImageAddNoise(config, options, view, input->fpa)){
                    ESCAPE("Unable to degrade MD image to 3pi");
		}


		if (!ppImageSquashNANs(config, options, view)) {
                    ESCAPE("Unable to squash NAN pixels");
		}

            }

            if (cell->data_exists) {
                ppImageDetrendRecord(cell, config, options, view);
            }

            // process each of the readouts
            // XXX reset the view to the first readout?
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                if (!readout->data_exists) {
                    continue;
                }
                // flip the image to match the raw readout orientation
                if (!ppImageParityFlip(config, options, view, false)) {
                    ESCAPE("Unable to detrend readout");
                }
            }

            // free detrend images potentially in use: MASK, BIAS, DARK, SHUTTER, FLAT
            if (!ppImageDetrendFree (config, view)) {
                ESCAPE("Unable to free detrend images");
            }

        }
        // free detrend images potentially in use: MASK, BIAS, DARK, SHUTTER, FLAT
        if (!ppImageDetrendFree (config, view)) {
            ESCAPE("Unable to free detrend images");
        }

        // Apply the fringe correction
	if (!ppImageDetrendFringeApply(config, chip, view, options)) {
	    ESCAPE("Unable to defringe");
        }

        // free detrend images potentially in use: MASK, BIAS, DARK, SHUTTER, FLAT
        if (!ppImageFringeFree (config, view)) {
            ESCAPE("Unable to free fringe images");
        }

        // Apply the pattern correction (only done if requested)
	if (!ppImageDetrendPatternApply(config,chip,view,options)) {
            ESCAPE("Problem applying pattern corrections");
        }

        // measure various pixel-based statistics for this image
        if (!ppImagePixelStats(config, stats, options, view)) {
            ESCAPE("Unable to measures pixel stats for image");
        }

        if (!ppImageMosaicChip(config, options, view, "PPIMAGE.CHIP", "PPIMAGE.OUTPUT")) {
            ESCAPE("Unable to mosaic chip");
        }

        if (!ppImageAuxiliaryMask(config, view, options, stats)) {
            ESCAPE("Unable to apply auxiliary mask");
        }

        timeDetrend += psTimerClear(TIMER_DETREND);

        // we perform photometry on the readouts of this chip in the output

        psTimerStart(TIMER_PHOT);
        if (options->doPhotom) {
            if (!ppImagePhotom(stats, config, view)) {
                ESCAPE("error running photometry.");
            }
        }
        timePhot += psTimerClear(TIMER_PHOT);

        // replace the masked pixels with the background level
        if (options->doBG) {
            if (!ppImageSubtractBackground(config, view, options)) {
                ESCAPE("Unable to subtract background");
            }
        }

	if (options->doMaskStats) {
	  //if (!ppImageMaskStats(config, view, options)) {
	  if (!ppImageMaskStats(config, view, stats)) {
	    ESCAPE("Unable to do Mask stats");
	  }
	}

        // these may be used by ppImageSubtractBackground.
        // if these are defined as internal files, drop them here
        status = true;
        status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL");
        status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL.STDEV");
        status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKGND");
        if (!status) {
            psError(PSPHOT_ERR_PROG, false, "trouble dropping internal files");
            psFree (view);
            return false;
        }

        // binning (used for display) must take place after the background is replaced, if desired
	if ((options->Bin1FITS)||(options->Bin1JPEG)||(options->FPA1FITS)) {
	  if (!ppImageRebinChip(config, view, options, "PPIMAGE.BIN1")) {
            ESCAPE("Unable to bin chip (level 1).");
	  }
	}
	if ((options->Bin2FITS)||(options->Bin2JPEG)||(options->FPA2FITS)) {
	  if (!ppImageRebinChip(config, view, options, "PPIMAGE.BIN2")) {
            ESCAPE("Unable to bin chip (level 2).");
	  }
	}

	if (options->doBackgroundContinuity) {
	  pmFPAfile *out = psMetadataLookupPtr(NULL, config->files, "PPIMAGE.BACKMDL");
	  pmFPAfileCopyView(out->fpa,out->src,view);
	}

        // Close cells (XXX shouldn't pmFPAfileClose iterate down as needed?)
        view->cell = -1;
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                ESCAPE("save failure for Cell");
            }
        }

        // Close chip
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            ESCAPE("save failure for Chip");
        }
    }

    // Do background model continuity updates
    if (!ppImageMosaicBackground(config, options )) {
      ESCAPE("failure in Background Mosaic");
    }

    // generate the full-scale FPA mosaic
    if ((options->FPA1FITS)||(options->Bin1JPEG)) {
      if (!ppImageMosaicFPA(config, options, "PPIMAGE.OUTPUT.FPA1", "PPIMAGE.BIN1")) {
        ESCAPE("failure in FPA Mosaic (level 1)");
      }
    }
    if ((options->FPA2FITS)||(options->Bin2JPEG)) {
      if (!ppImageMosaicFPA(config, options, "PPIMAGE.OUTPUT.FPA2", "PPIMAGE.BIN2")) {
        ESCAPE("failure in FPA Mosaic (level 2)");
      }
    }

    // we perform astrometry on all chips after sources have been detected
    // this also performs the psastro file IO
    if (options->doAstromChip || options->doAstromMosaic) {
        if (!ppImageAstrom(config, stats)) {
            ESCAPE("error running astrometry.");
        }
    }

    if (psTraceGetLevel("ppImage") >= 3) {
        ppImageFileCheck(config);
    }

    // Calculate summary statistics from FPA Metadata
    if (!ppImageMetadataStats(config, stats, options)) {
        ESCAPE("Unable to determine FPA-level metadata statistics.");
    }

    // Output and Close FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        ESCAPE("save failure for FPA");
    }
    psFree(view);

    // Dump configuration
    psString dump_file = psMetadataLookupStr(&status, config->arguments, "DUMP_CONFIG");
    if (dump_file) {
        if (!pmConfigDump(config, dump_file)) {
            ESCAPE("Unable to dump configuration.");
        }
    }

    // Write out summary statistics
    if (options->doStats) {
        psMetadataAddF32(stats, PS_LIST_TAIL, "DT_DET", 0, "Time spent detrending (sec)", timeDetrend);
        psMetadataAddF32(stats, PS_LIST_TAIL, "DT_PHOT", 0, "Time spent photometering (sec)", timePhot);
        psMetadataAddF32(stats, PS_LIST_TAIL, "DT_TOTAL", 0, "Total time (sec)", psTimerMark(TIMER_TOTAL));
        if (!ppImageStatsOutput(config, stats, options)) {
            ESCAPE("Unable to write statistics file.");
        }
    }
    psFree (stats);

    return true;
}

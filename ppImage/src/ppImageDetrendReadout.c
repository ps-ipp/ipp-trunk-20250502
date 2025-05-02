#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

bool ppImageDetrendReadout(pmConfig *config, ppImageOptions *options, pmFPAview *view)
{
  // psTimerStart("detrend.readout");

    // construct a view for the detrend images (which have only one readout)
    pmFPAview *detview = pmFPAviewAlloc(0);
    *detview = *view;
    detview->readout = 0;

    // find the currently selected readout
    pmReadout *input = pmFPAfileThisReadout(config->files, view, "PPIMAGE.INPUT");

    // Check that the gain is set (this is used by both pmReadoutGenerateMask and pmReadoutGenerateVariance)
    { 
      float gain = psMetadataLookupF32(NULL, input->parent->concepts, "CELL.GAIN"); // Gain for cell
      if (!isfinite(gain)) {
	psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, RECIPE_NAME); // Recipe
	psAssert(recipe, "Should be there!");
	bool override = psMetadataLookupBool(NULL, recipe, "GAIN.OVERRIDE"); // Override the bad gain?
	if (override) {
	  psWarning("CELL.GAIN is not set for readout (%d,%d,%d) --- setting to unity.", view->chip, view->cell, view->readout);
	  psMetadataItem *item = psMetadataLookup(input->parent->concepts, "CELL.GAIN"); // Gain item
	  psAssert(item, "Should be there!");
	  item->data.F32 = 1.0;

	  // for unity gain, there is no modification for the readnoise, note that it has (effectively) been updated
	  psMetadataRemoveKey(input->parent->concepts, "CELL.READNOISE.UPDATE");
	} else {
	  psWarning("CELL.GAIN is NAN for readout (%d,%d,%d), image will be masked.", view->chip, view->cell, view->readout);
	}
      }
    }
    // psLogMsg ("ppImage", 6, "check gain: %f sec\n", psTimerMark ("detrend.readout"));

    // Check to see if we're in a chip that contains video
    // NOTE: this is an esoteric test for a GPC1 feature : this is irrelevant & will not work for other cameras
    bool hasVideo = false;
    {
      // XXX test for GPC1? or CAN_HAVE_VIDEO in camera config?
      bool status = false;
      if (!input) goto done_video_check;
      if (!input->parent) goto done_video_check;
      if (!input->parent->parent) goto done_video_check;
      if (!input->parent->parent->hdu) goto done_video_check;
      if (!input->parent->parent->hdu->header) goto done_video_check;
      char *ptr = psMetadataLookupStr(&status,input->parent->parent->hdu->header,"CELLMODE");
      if (status) {
	psLogMsg ("ppImage.detrend", PS_LOG_DETAIL, "VIDEO: %d %d %d\n",(int) options->hasVideo,(int) options->useVideoDark, (int) options->useVideoMask);
	char *Vptr = strchr(ptr,'V');
	if (Vptr) {
	  hasVideo = options->hasVideo = true;
	  psLogMsg ("ppImage.detrend", PS_LOG_INFO, "VIDEO: %d %d %d\n",(int) options->hasVideo,(int) options->useVideoDark, (int) options->useVideoMask);
	}
      }
    }
    done_video_check:
    // psLogMsg ("ppImage", 6, "check video: %f sec\n", psTimerMark ("detrend.readout"));

    // XXX for GPC2, there are some bizzare cell levels that need to be filtered out
    // Here I measure the median background before overscan subtraction, but an alternative
    // is to use the overscan-subtracted image.
    // XX if (0) {
    // XX 	psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN);
    // XX 	psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    // XX 	psImageBackground (stats, NULL, input->image, NULL, 0xffff, rng);
    // XX 	readoutBackground = stats->robustMedian;
    // XX }

    // Masking on the basis of pixel value needs to be done before anything else, so the values are pristine.
    if (options->doMaskBuild) {
        psImageMaskType satMask = options->doMaskSat ? options->satMask : 0;
        psImageMaskType lowMask = options->doMaskLow ? options->lowMask : 0;
        if (!pmReadoutGenerateMask(input, satMask, lowMask)) {
	  psError(PS_ERR_UNKNOWN, false, "Unable to generate a mask.");
	  psFree(detview);
	  return false;
	}
	// psLogMsg ("ppImage", 6, "generate mask: %f sec\n", psTimerMark ("detrend.readout"));
    }
    // apply the externally supplied mask to the input->mask pixels
    if (options->doMask) {
      pmReadout *mask;
      if ((options->useVideoMask)&&(hasVideo)) {
        mask = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.VIDEOMASK");
      }
      else {
	mask = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.MASK");
      }
      pmMaskBadPixels(input, mask, options->maskValue);
      // psLogMsg ("ppImage", 6, "apply mask: %f sec\n", psTimerMark ("detrend.readout"));
    }

    if (options->doApplyBurntool)
    {
        // extern bool ppImageBurntoolApply(pmConfig *, ppImageOptions *, pmFPAview *, pmReadout *);
        ppImageBurntoolApply(config, options, view, input);
	// psLogMsg ("ppImage", 6, "apply burntool: %f sec\n", psTimerMark ("detrend.readout"));
    }
    
    if (options->doMaskBurntool) {
        if (options->doApplyBurntool) {
            // build burntool mask from data input burntool table
            ppImageBurntoolMaskFromTable(config,options,view,input);
        } else {
            // build burntool mask from data in the input image's fits extension
            ppImageBurntoolMask(config,options,view,input);
        }
	// psLogMsg ("ppImage", 6, "apply burntool mask: %f sec\n", psTimerMark ("detrend.readout"));
    }

    // Subtract the overscan
    if (options->doOverscan) {
      if (!pmOverscanSubtract (input, options->overscan)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to subtract overscan.");
	psFree(detview);
	return false;
      }
      // psLogMsg ("ppImage", 6, "subtract overscan: %f sec\n", psTimerMark ("detrend.readout"));
    }

    // measure the overscan-subtracted readoutBackground here (or subtract the overscan value?)
    // XXX this is the measurements and should be independent of the pattern masking
    if (options->doPatternDeadCells) {
	psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
	psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
	psImageBackground (stats, NULL, input->image, NULL, 0xffff, rng);

	// save this value somewhere
	pmHDU *hdu = pmHDUFromReadout(input);  // HDU of interest
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, "BACK_VAL", PS_META_REPLACE, "Median cell background", stats->robustMedian);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, "BACK_ERR", PS_META_REPLACE, "Stdev of cell background", stats->robustStdev);
	psFree (stats);
	psFree (rng);
    }

    // Non-linearity correction
    if (options->doNonLin) {
      if (!ppImageDetrendNonLinear(input,detview,config)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to correct NonLinearity");
	psFree(detview);
	return(false);
      }
      // psLogMsg ("ppImage", 6, "nonlinear correction: %f sec\n", psTimerMark ("detrend.readout"));
    }
    // New Non-linearity correction (exclusive of the above)
    if (options->doNewNonLin) {
      if (!ppImageDetrendNewNonLinear(input, detview, config)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to correct Non-Linearity with new version (2023)");
	psFree(detview);
	return(false);
      }
      // psLogMsg ("ppImage", 6, "nonlinear correction: %f sec\n", psTimerMark ("detrend.readout"));
    }

    // set up the dark and bias
    pmCell *dark = NULL;                // Multi-dark
    pmReadout *oldDark = NULL;          // Old-fashioned dark
    pmReadout  *bias = NULL;
    if (options->doBias) {
        bias = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.BIAS");
    }
    if (options->doDark) {
        bool mdok;                      // Status of MD lookup
        psMetadata *recipe = psMetadataLookupPtr (&mdok, config->recipes, RECIPE_NAME);
        assert(mdok && recipe);

	if ((options->useVideoDark)&&(hasVideo)) {
	  if (psMetadataLookupBool(&mdok, recipe, "OLDDARK")) {
            oldDark = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.VIDEODARK");
	  } else {
            dark = pmFPAfileThisCell(config->files, detview, "PPIMAGE.VIDEODARK");
	  }
	}
	else {
	  if (psMetadataLookupBool(&mdok, recipe, "OLDDARK")) {
            oldDark = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.DARK");
	  } else {
            dark = pmFPAfileThisCell(config->files, detview, "PPIMAGE.DARK");
	  }
	}
    }

    // Bias and temperature-independent-dark subtraction are merged.
    if (options->doBias) {
        if (!pmBiasSubtract(input, bias, oldDark, view)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to subtract bias.");
            psFree(detview);
            return false;
        }
	// psLogMsg ("ppImage", 6, "apply bias: %f sec\n", psTimerMark ("detrend.readout"));
    }
    
    // Weight on the basis of pixel value needs to be done after the overscan has been subtracted
    if (options->doVarianceBuild) {
        // create the target mask and variance images
        psImage *noiseImage = NULL;
        if (options->doNoiseMap) {
            // XXX convert the noiseMap image to a binned image
            pmReadout *noiseMap = NULL;
            noiseMap = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.NOISEMAP");
            noiseImage = psImageCopy (NULL, input->image, PS_TYPE_F32);
            psImageInit (noiseImage, 0.0);

            // XXX this works, but is not really quite right: the model shoud include the
            // offset information, we are not really getting exactly the right mapping from the
            // original file.
	    // CZW 2012-03-21: I do not believe this is true anymore.  In any case, this seems to be what
	    //                 ppImageReplaceBackground does to do sky subtraction.
            psImageBinning *binning = psImageBinningAlloc();
            binning->nXruff = noiseMap->image->numCols;
            binning->nYruff = noiseMap->image->numRows;
            binning->nXfine = input->image->numCols;
            binning->nYfine = input->image->numRows;
            psImageBinningSetScale(binning, PS_IMAGE_BINNING_LEFT);

            psImageUnbin (noiseImage, noiseMap->image, binning);
            psFree (binning);
        }

        if (!pmReadoutGenerateVariance(input, noiseImage, true)) {
	    psError(PS_ERR_UNKNOWN, false, "Unable to generate a variance image.");
	    psFree(detview);
	    return false;
	}
        psFree (noiseImage);
	// psLogMsg ("ppImage", 6, "generate variance: %f sec\n", psTimerMark ("detrend.readout"));
    }

    if (options->doDark && dark) {
        if (!pmDarkApply(input, dark, options->darkMask)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to subtract dark.");
            psFree(detview);
            return false;
        }
	// psLogMsg ("ppImage", 6, "apply dark: %f sec\n", psTimerMark ("detrend.readout"));
    }

    if (options->doRemnance) {
        if (!pmRemnance(input, options->maskValue, options->lowMask,
                        options->remnanceSize, options->remnanceThresh)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to mask remnance.");
            psFree(detview);
            return false;
        }
	// psLogMsg ("ppImage", 6, "mask remnance: %f sec\n", psTimerMark ("detrend.readout"));
    }

    // Shutter correction
    if (options->doShutter) {
        pmReadout *shutter = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.SHUTTER");
        if (!pmShutterCorrectionApply(input, shutter, pmConfigMaskGet("FLAT", config))) {
            psFree(detview);
            return false;
        }
	// psLogMsg ("ppImage", 6, "shutter correction: %f sec\n", psTimerMark ("detrend.readout"));
    }

    // Flat-field correction (no options used?)
    if (options->doFlat) {
        pmReadout *flat = pmFPAfileThisReadout(config->files, detview, "PPIMAGE.FLAT");
        if (!pmFlatField(input, flat, options->flatMask)) {
            psFree(detview);
            return false;
        }
	// psLogMsg ("ppImage", 6, "apply flat: %f sec\n", psTimerMark ("detrend.readout"));
    }

/*     // Pattern noise correction */
/*     if (options->doPattern) { */
/*         if (!pmPatternRow(input, options->patternOrder, options->patternIter, options->patternRej, */
/*                           options->patternThresh, options->patternMean, options->patternStdev, */
/*                           options->maskValue, options->darkMask)) { */
/*             psFree(detview); */
/*             return false; */
/*         } */
/*     } */

    // Normalization by a single (known) constant
    bool mdok;                          // Status of MD lookup
    float norm = psMetadataLookupF32(&mdok, config->arguments, "NORMALIZATION");
    if (mdok && isfinite(norm) && norm != 1.0) {
        pmHDU *hdu = pmHDUFromReadout(input); // HDU of interest
        psString comment = NULL;        // Comment to add
        psStringAppend(&comment, "Normalization: %f", norm);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
        psFree(comment);

        psBinaryOp(input->image, input->image, "*", psScalarAlloc(norm, PS_TYPE_F32));
	// psLogMsg ("ppImage", 6, "renormalize: %f sec\n", psTimerMark ("detrend.readout"));
    }

# if (1)
    // Normalization by per-class values
    psMetadata *normlist = psMetadataLookupMetadata(&mdok, config->arguments, "NORMALIZATION.TABLE");
    if (normlist) {
        pmFPAfile *inputFile = psMetadataLookupPtr(&mdok, config->files, "PPIMAGE.INPUT");

        // get the menu of class IDs
        psMetadata *menu = psMetadataLookupMetadata(&mdok, inputFile->camera, "CLASSID");
        if (!menu) {
            psError(PS_ERR_IO, false, "Unable to find CLASSID metadata in camera configuration");
            psFree(detview);
            return false;
        }
        // get the rule for class_id for the desired class
        const char *rule = psMetadataLookupStr(&mdok, menu, options->normClass);
        if (!rule) {
            psError(PS_ERR_IO, false, "Unable to find NORM.CLASS value %s in CLASSID in camera configuration", options->normClass);
            psFree(detview);
            return false;
        }
        // get the class_id from the rule
        char *classID = pmFPAfileNameFromRule(rule, inputFile, view);
        if (!classID) {
            psError(PS_ERR_IO, false, "error converting CLASSID rule %s to name\n", rule);
            psFree(detview);
            return false;
        }

        // get normalization from the class_id
        float norm = psMetadataLookupF32 (&mdok, normlist, classID);
        if (!mdok) {
            psError(PS_ERR_IO, false, "failed to find class ID %s in normalization table\n", classID);
            psFree(detview);
            return false;
        }

        pmHDU *hdu = pmHDUFromReadout(input); // HDU of interest
        psString comment = NULL;        // Comment to add
        psStringAppend(&comment, "Normalization: %f", norm);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
        psFree(comment);

        // apply the normalization
        psBinaryOp(input->image, input->image, "*", psScalarAlloc(norm, PS_TYPE_F32));

        psFree (classID);

	// psLogMsg ("ppImage", 6, "renormalize by class: %f sec\n", psTimerMark ("detrend.readout"));
    }
# endif

    if (options->doFringe) {
        pmCell *fringe = pmFPAfileThisCell(config->files, detview, "PPIMAGE.FRINGE");
        if (!ppImageDetrendFringeMeasure(input, fringe, false, options)) {
            psFree(detview);
            return false;
        }
	// psLogMsg ("ppImage", 6, "measure fringe: %f sec\n", psTimerMark ("detrend.readout"));
    }

    ppImageMemoryDump("detrend");

    // psLogMsg ("ppImage", 5, "detrend readout: %f sec\n", psTimerMark ("detrend.readout"));
    psFree(detview);
    return true;
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

ppImageOptions *ppImageParseCamera(pmConfig *config)
{
    bool status = false;

    if (!ppImageDefineFile(config, NULL, "PPIMAGE.INPUT", "INPUT", PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_NONE)) {
        psError(PS_ERR_IO, false, "Can't find an input image source");
        return NULL;
    }
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PPIMAGE.INPUT"); // Input file
    psAssert(input, "We just put it there!");

    // add recipe options supplied on command line
    psMetadata *recipe  = psMetadataLookupPtr(&status, config->recipes, RECIPE_NAME);

    // parse the options from the metadata format to the ppImageOptions structure
    ppImageOptions *options = ppImageOptionsParse(config);

    // the following are defined from the argument list, if given,
    // otherwise they revert to the config information
    // not all input or output images are used in a given recipe
    if (options->doNoiseMap) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.NOISEMAP", "NOISEMAP",
                               PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_NOISEMAP)) {
            psError(PS_ERR_IO, false, "Can't find a noise map image source");
            psFree(options);
            return NULL;
        }
    }
    if (options->doBias) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.BIAS", "BIAS",
                               PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_BIAS)) {
            psError(PS_ERR_IO, false, "Can't find a bias image source");
            psFree(options);
            return NULL;
        }
    }
    if (options->doDark) {
      // Always load the regular Dark
      if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.DARK", "DARK",
			     PM_FPA_FILE_DARK, PM_DETREND_TYPE_DARK)) {
	psError(PS_ERR_IO, false, "Can't find a dark image source");
	psFree(options);
	return NULL;
      }
      // Sometimes load the video dark if we need it.
      if (options->useVideoDark) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.VIDEODARK", "DARK",
                               PM_FPA_FILE_DARK, PM_DETREND_TYPE_VIDEODARK)) {
            psError(PS_ERR_IO, false, "Can't find a dark image source");
            psFree(options);
            return NULL;
        }
      }
    }
    if (options->doMask) {
      // Always load the regular mask
      if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.MASK", "MASK",
			     PM_FPA_FILE_MASK, PM_DETREND_TYPE_MASK)) {
	psError(PS_ERR_IO, false, "Can't find a mask image source");
	psFree(options);
	return NULL;
      }

      if (options->useVideoMask) {
	if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.VIDEOMASK", "MASK",
                               PM_FPA_FILE_MASK, PM_DETREND_TYPE_VIDEOMASK)) {
            psError(PS_ERR_IO, false, "Can't find a mask image source");
            psFree(options);
            return NULL;
        }
      }
      if (options->doAuxMask) {
	if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.AUXMASK", "MASK",
                               PM_FPA_FILE_MASK, PM_DETREND_TYPE_AUXMASK)) {
            psError(PS_ERR_IO, false, "Can't find a auxillary mask image source");
            psFree(options);
            return NULL;
        }
      }
    }
    if (options->doShutter) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.SHUTTER", "SHUTTER",
                               PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_SHUTTER)) {
            psError(PS_ERR_IO, false, "Can't find a shutter image source");
            psFree(options);
            return NULL;
        }
    }

    if (options->doFlat) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.FLAT", "FLAT",
                               PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_FLAT)) {
            psError(PS_ERR_IO, false, "Can't find a flat image source");
            psFree(options);
            return NULL;
        }
    }

    if (options->doNonLin) {
	if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.LINEARITY", "LINEARITY",
			       PM_FPA_FILE_LINEARITY, PM_DETREND_TYPE_LINEARITY)) {
	    psError(PS_ERR_IO, false, "Can't find a non-linearity correction source");
	    psFree(options);
	    return NULL;
	}
    }
    if (options->doNewNonLin) {
	// if the file has been specified on the command-line (-newnonlin file), then the file
	// is identified by the NEWNONLIN entry in config->arguments.  otherwise, load from the
	// detrend system as type NEWNONLIN
	if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.NEWNONLIN", "NEWNONLIN",
			       PM_FPA_FILE_NEWNONLIN, PM_DETREND_TYPE_NEWNONLIN)) {
	    psError(PS_ERR_IO, false, "Can't find a new non-linearity correction source");
	    psFree(options);
	    return NULL;
	}
    }

    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS");
    if (nThreads > 0) {
        int nScanRows = psMetadataLookupS32(&status, recipe, "SCAN.ROWS");
        pmDetrendSetThreadTasks(nScanRows);
    }
    ppImageSetThreads (); // even if threading is off, we need to identify the ppImageDetrendPattern thread function

    if (options->doPatternRow || options->doPatternCell) {
        pmFPAfile *outPattern = pmFPAfileDefineOutput(config, input->fpa, "PPIMAGE.PATTERN");
        if (!outPattern) {
            psError(PS_ERR_IO, false, _("Unable to generate output file from PPIMAGE.PATTERN"));
            psFree(options);
            return NULL;
        }
        outPattern->save = true;
    }

    // fringe frame are only applied for a subset of the filters.
    // if the filter is one of those identified by a FRINGE.FILTERS metadata entry
    // in ppImage.config, apply the fringe frame
    if (options->doFringe) {
        // determine filter from the concepts
        const char *filter = psMetadataLookupStr(&status, input->fpa->concepts, "FPA.FILTER");
        if (!status || !filter || strlen(filter) == 0) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find FPA.FILTER.\n");
            psFree(options);
            return NULL;
        }

        // select FRINGE.FILTERS from the recipe and test if the filter matches
        // is the mdi saved with psList or string?
        psMetadataItem *mdi = psMetadataLookup (recipe, "FRINGE.FILTERS");
        if (mdi == NULL) {
            // no valid filters for fringe data for this camera
            options->doFringe = false;
            goto skip_fringe;
        }
        // place entry on a list regardless of type
        psList *filters = NULL;
        if (mdi->type == PS_DATA_STRING) {
            filters = psListAlloc(NULL);
            psListAdd (filters, PS_LIST_HEAD, mdi);
        } else if (mdi->type == PS_DATA_METADATA_MULTI) {
            filters = psMemIncrRefCounter(mdi->data.list);
        } else {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    "FRINGE.FILTERS in recipe %s is not of type METADATA", RECIPE_NAME);
            psFree(options);
            return NULL;
        }

        // search through list to find the current filter
        psListIterator *iter = psListIteratorAlloc (filters, PS_LIST_HEAD, FALSE);
        options->doFringe = false;
        for (int i = 0; !options->doFringe && (i < filters->n); i++) {
            psMetadataItem *item = psListGetAndIncrement (iter);
            char *validFilter = item->data.V;
            if (strcmp (validFilter, filter)) continue;
            options->doFringe = true;
        }
        psFree(iter);
        psFree(filters);
    }
skip_fringe:
    if (options->doFringe) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.FRINGE", "FRINGE",
                               PM_FPA_FILE_FRINGE, PM_DETREND_TYPE_FRINGE)) {
            psError (PS_ERR_IO, false, "Can't find a fringe image source");
            return NULL;
        }
    }

    if (options->doPatternRow) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.PATTERN.ROW.AMP", "PATTERN.ROW.AMP",
			       PM_FPA_FILE_PATTERN_ROW_AMP, PM_DETREND_TYPE_PATTERN_ROW_AMP)) {
            psWarning ("Can't find a pattern row amplitude source, will apply to all cells or defined subset");
	    // an empty or invalid file may have been generated -- we want to skip, not raise an error
	    pmFPAfile *PRAfile = psMetadataLookupPtr (NULL, config->files, "PPIMAGE.PATTERN.ROW.AMP");
	    if (PRAfile) {
	      PRAfile->state |= PM_FPA_STATE_INACTIVE;
	    }
        }
    }
    if (options->doPatternDeadCells) {
        if (!ppImageDefineFile(config, input->fpa, "PPIMAGE.PATTERN.DEAD.CELLS", "PATTERN.DEAD.CELLS",
			       PM_FPA_FILE_PATTERN_DEAD_CELLS, PM_DETREND_TYPE_PATTERN_DEAD_CELLS)) {
            psWarning ("Can't find a pattern dead cells source");
	    // an empty or invalid file may have been generated -- we want to skip, not raise an error
	    pmFPAfile *PRAfile = psMetadataLookupPtr (NULL, config->files, "PPIMAGE.PATTERN.DEAD.CELLS");
	    if (PRAfile) {
	      PRAfile->state |= PM_FPA_STATE_INACTIVE;
	    }
        }
    }

    if (options->checkCTE && false) {
        int DX = psMetadataLookupS32 (&status, recipe, "CTE.XBIN");
        int DY = psMetadataLookupS32 (&status, recipe, "CTE.YBIN");
        pmFPAfile *outCTE = pmFPAfileDefineFromFile (config, input, DX, DY, "PPIMAGE.CTEMAP");
        if (!outCTE) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PPIMAGE.CTEMAP");
            return false;
        }
        outCTE->save = true;
    }
    if (options->doApplyBurntool) {
        // If no burntool file was supplied do not fail. (camera_exp.pl does not supply it
        // for example.
        if (!psMetadataLookupStr(NULL, config->arguments, "BURNTOOL.TABLE")) {
            psWarning("BURNTOOL.TABLE not supplied setting doApplyBurntool to false");
            options->doApplyBurntool = false;
        }
    }

    // the following files are output targets
    pmFPAfile *outImage = pmFPAfileDefineOutput(config, input->fpa, "PPIMAGE.OUTPUT");
    if (!outImage) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPIMAGE.OUTPUT"));
        psFree(options);
        return NULL;
    }
    if (outImage->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT is not of type IMAGE");
        psFree(options);
        return NULL;
    }
    pmFPAfile *outMask = pmFPAfileDefineOutput(config, input->fpa, "PPIMAGE.OUTPUT.MASK");
    if (!outMask) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPIMAGE.OUTPUT.MASK"));
        psFree(options);
        return NULL;
    }
    if (outMask->type != PM_FPA_FILE_MASK) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.MASK is not of type MASK");
        psFree(options);
        return NULL;
    }
    pmFPAfile *outVariance = pmFPAfileDefineOutput(config, input->fpa, "PPIMAGE.OUTPUT.VARIANCE");
    if (!outVariance) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPIMAGE.OUTPUT.VARIANCE"));
        psFree(options);
        return NULL;
    }
    if (outVariance->type != PM_FPA_FILE_VARIANCE) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.VARIANCE is not of type VARIANCE");
        psFree(options);
        return NULL;
    }

    // XXX should these be bound explicitly to the outImage->fpa rather than the input->fpa?
    pmFPAfile *chipImage = pmFPAfileDefineChipMosaic(config, input->fpa, "PPIMAGE.CHIP");
    if (!chipImage) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.CHIP"));
        psFree(options);
        return NULL;
    }
    if (chipImage->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPIMAGE.CHIP is not of type IMAGE");
        psFree(options);
        return NULL;
    }
    pmFPAfile *chipMask = pmFPAfileDefineOutput(config, chipImage->fpa, "PPIMAGE.CHIP.MASK");
    if (!chipMask) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPIMAGE.CHIP.MASK"));
        psFree(options);
        return NULL;
    }
    if (chipMask->type != PM_FPA_FILE_MASK) {
        psError(PS_ERR_IO, true, "PPIMAGE.CHIP.MASK is not of type MASK");
        psFree(options);
        return NULL;
    }
    pmFPAfile *chipVariance = pmFPAfileDefineOutput(config, chipImage->fpa, "PPIMAGE.CHIP.VARIANCE");
    if (!chipVariance) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPIMAGE.CHIP.VARIANCE"));
        psFree(options);
        return NULL;
    }
    if (chipVariance->type != PM_FPA_FILE_VARIANCE) {
        psError(PS_ERR_IO, true, "PPIMAGE.CHIP.VARIANCE is not of type VARIANCE");
        psFree(options);
        return NULL;
    }

    pmFPAfile *byFPA1 = pmFPAfileDefineFPAMosaic(config, input->fpa, "PPIMAGE.OUTPUT.FPA1");
    if (!byFPA1) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.OUTPUT.FPA1"));
        psFree(options);
        return NULL;
    }
    if (byFPA1->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.FPA1 is not of type IMAGE");
        psFree(options);
        return NULL;
    }
    pmFPAfile *byFPA2 = pmFPAfileDefineFPAMosaic(config, input->fpa, "PPIMAGE.OUTPUT.FPA2");
    if (!byFPA2) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.OUTPUT.FPA2"));
        psFree(options);
        return NULL;
    }
    if (byFPA2->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.FPA2 is not of type IMAGE");
        psFree(options);
        return NULL;
    }


    // chipImage    -> psphotInput  (pmFPAfileDefineFromFile)       : fpa is constructed
    // psphotInput  -> psphotOutput (pmFPAfileDefineOutputFromFile) : fpa is equated
    // psphotOutput -> psastroInput (pmFPAfileDefineInput)          : fpa is ref-copied
    // psastroInput -> psastroModel (pmFPAfileDefineFromArgs        : fpa is ref-copied
    // psastroInput -> psastroModel (pmFPAfileDefineFromConf        : fpa is constructed
    // psastroInput -> psastroModel (pmFPAfileDefineFromDetDB       : fpa is constructed (pmDetrendSelect uses input concepts )

    // For photometry, we operate on the chip-mosaicked image
    // we create a copy of the mosaicked image for psphot so we can write out a clean image
    if (options->doPhotom || options->doBG) {

        // this file is just used as a carrier; output files (eg, PSPHOT.RESID) are defined by
        // psphotDefineFiles
        pmFPAfile *psphotInput = pmFPAfileDefineFromFile (config, chipImage, 1, 1, "PSPHOT.INPUT");
        PS_ASSERT (psphotInput, false);

        // specify the number of psphot input images
        psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.INPUT.NUM", PS_META_REPLACE, "number of inputs", 1);

        // define associated psphot input/output files
        if (!psphotDefineFiles (config, psphotInput)) {
            psError(PSPHOT_ERR_CONFIG, false,
                    "Trouble defining the additional input/output files for psphot");
            return false;
        }
    }

    // For photometry, we operate on the chip-mosaicked image
    if (options->doAstromChip || options->doAstromMosaic) {
        if (!options->doPhotom) {
            psError(PSASTRO_ERR_CONFIG, false,
                    "Photometry mode is not selected; it is required for astrometry");
            return false;
        }

        pmFPAfile *psphotOutput = psMetadataLookupPtr(&status, config->files, "PSPHOT.OUTPUT");
        PS_ASSERT(psphotOutput, false);

        pmFPAfile *psastroInput = pmFPAfileDefineInput(config, psphotOutput->fpa, NULL, "PSASTRO.INPUT");
        PS_ASSERT(psastroInput, false);
        psastroInput->mode = PM_FPA_MODE_REFERENCE;

        // define associated psphot input/output files
        if (!psastroDefineFiles(config, psastroInput)) {
            psError(PSPHOT_ERR_CONFIG, false,
                    "Trouble defining the additional input/output files for psastro");
            return false;
        }

        // deactivate the psastro files, reactive when needed
        pmFPAfileActivate(config->files, false, "PSASTRO.OUTPUT");
    }

    // save any of these files?
    outImage->save   = options->BaseFITS;
    outMask->save    = options->BaseMaskFITS;
    outVariance->save  = options->BaseVarianceFITS;

    chipImage->save  = options->ChipFITS;
    chipMask->save   = options->ChipMaskFITS;
    chipVariance->save = options->ChipVarianceFITS;

    byFPA1->save     = options->FPA1FITS;
    byFPA2->save     = options->FPA2FITS;

    // outImage is used as a carrier: input to chipImage -> require the data to remain at the CHIP level
    outImage->freeLevel = PS_MIN(outImage->freeLevel, PM_FPA_LEVEL_CHIP);
    outImage->dataLevel = outImage->freeLevel;
    outImage->fileLevel = PS_MIN(outImage->fileLevel, outImage->dataLevel);

    // outMask and outVariance must be freed at the same level as outImage (all freed by pmFPAFreeData)
    outMask->freeLevel   = outImage->freeLevel;
    outVariance->freeLevel = outImage->freeLevel;
    outMask->dataLevel   = outImage->dataLevel;
    outVariance->dataLevel = outImage->dataLevel;

    // Ditto for the chip-mosaicked version
    chipMask->freeLevel   = chipImage->freeLevel;
    chipVariance->freeLevel = chipImage->freeLevel;
    chipMask->dataLevel   = chipImage->dataLevel;
    chipVariance->dataLevel = chipImage->dataLevel;

    // the input data is the same as the outImage data : force the free levels to match
    input->freeLevel = PS_MIN(outImage->freeLevel, input->freeLevel);

    // define the continuity corrected background model files
    if (options->doBackgroundContinuity) {
      pmFPAfile *bkgMosaicModel = pmFPAfileDefineFromFPA(config,input->fpa, 1, 1,  "PPIMAGE.BACKMDL");
      if (!bkgMosaicModel) {
	psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.BACKMDL"));
	psFree(options);
	return NULL;
      }
      if (bkgMosaicModel->type != PM_FPA_FILE_IMAGE) {
	psError(PS_ERR_IO, true, "PPIMAGE.BACKMDL is not of type IMAGE");
	psFree(options);
	return NULL;

      }
      bkgMosaicModel->save = options->doBackgroundContinuity;
      //      bkgMosaicModel->freeLevel = PS_MIN(bkgMosaicModel->freeLevel, PM_FPA_LEVEL_FPA);
      bkgMosaicModel->freeLevel = PM_FPA_LEVEL_FPA;
      bkgMosaicModel->dataLevel = bkgMosaicModel->dataLevel;
      bkgMosaicModel->fileLevel = PS_MIN(bkgMosaicModel->fileLevel, bkgMosaicModel->dataLevel);
    }

    // define the binned target files (which may just be carriers for some camera configurations)
    pmFPAfile *bin1 = pmFPAfileDefineFromFPA(config, chipImage->fpa, options->xBin1, options->yBin1,
                                             "PPIMAGE.BIN1");
    if (!bin1) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.BIN1"));
        psFree(options);
        return NULL;
    }
    if (bin1->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.BIN1 is not of type IMAGE");
        psFree(options);
        return NULL;
    }

    pmFPAfile *bin2 = pmFPAfileDefineFromFPA(config, chipImage->fpa, options->xBin2, options->yBin2,
                                             "PPIMAGE.BIN2");
    if (!bin2) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.BIN2"));
        psFree(options);
        return NULL;
    }
    if (bin2->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.BIN2 is not of type IMAGE");
        psFree(options);
        return NULL;
    }

    // bin1 and bin2 are used as carriers: input for byFPA1, byFPA2
    bin1->freeLevel = PM_FPA_LEVEL_FPA;
    bin2->freeLevel = PM_FPA_LEVEL_FPA;

    pmFPAfile *jpg1 = pmFPAfileDefineOutput(config, byFPA1->fpa, "PPIMAGE.JPEG1");
    if (!jpg1) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.JPEG1"));
        psFree(options);
        return NULL;
    }
    if (jpg1->type != PM_FPA_FILE_JPEG) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.JPEG1 is not of type JPEG");
        psFree(options);
        return NULL;
    }
    pmFPAfile *jpg2 = pmFPAfileDefineOutput(config, byFPA2->fpa, "PPIMAGE.JPEG2");
    if (!jpg2) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPIMAGE.JPEG2"));
        psFree(options);
        return NULL;
    }
    if (jpg2->type != PM_FPA_FILE_JPEG) {
        psError(PS_ERR_IO, true, "PPIMAGE.OUTPUT.JPEG2 is not of type JPEG");
        psFree(options);
        return NULL;
    }

    // XXX we could potentially not define these pmFPAfiles if no output is requested...
    bin1->save = options->Bin1FITS;
    bin2->save = options->Bin2FITS;
    jpg1->save = options->Bin1JPEG;
    jpg2->save = options->Bin2JPEG;

    // Chip selection: turn on only the chips specified (pass status to suppress missing-key log msg)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray(chipLine, ",", false);
    if (chips->n > 0) {
        pmFPASelectChip (input->fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(input->fpa, chipNum, false)) {
                psError(PS_ERR_IO, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                psFree(options);
                return false;
            }
        }
    }
    psFree (chips);

    if (psMetadataLookupBool(NULL, config->arguments, "INPUT_IS_FRINGE")) {
        // It's a fringe file, so change the file type
        input->type = PM_FPA_FILE_FRINGE;
        outImage->type = PM_FPA_FILE_FRINGE;
    }
    if (psMetadataLookupBool(NULL, config->arguments, "INPUT_IS_DARK")) {
        // It's a dark file, so change the file type
        input->type = PM_FPA_FILE_DARK;
        outImage->type = PM_FPA_FILE_DARK;
        // Turn off compression --- there are just too many nasties that can happen
        psFree(outImage->compression);
        outImage->compression = NULL;
        psFree(outImage->options);
        outImage->options = NULL;
    }

    // Turn off mask and variance output if we're not doing anything interesting
    if (!options->doMaskBuild && outMask->save) {
        psWarning("output mask image (BASE.MASK.FITS) requested, but not generated: skipping.\n");
        outMask->save = false;
    }
    if (!options->doVarianceBuild && outVariance->save) {
        psWarning("output variance image (BASE.VARIANCE.FITS) requested, but not generated: skipping.\n");
        outVariance->save = false;
    }
    if (!options->doMaskBuild && chipMask->save) {
        psWarning("output mask image (CHIP.MASK.FITS) requested, but not generated: skipping.\n");
        chipMask->save = false;
    }
    if (!options->doVarianceBuild && chipVariance->save) {
        psWarning("output variance image (CHIP.VARIANCE.FITS) requested, but not generated: skipping.\n");
        chipVariance->save = false;
    }
    
    if (psTraceGetLevel("ppImage.config") > 0) {
        // Get a look inside all the files.
        psMetadataIterator *filesIter = psMetadataIteratorAlloc(config->files, PS_LIST_HEAD, NULL);
        psMetadataItem *item;               // Item from iteration
        fprintf(stderr, "Files:\n");
        while ((item = psMetadataGetAndIncrement(filesIter))) {
            pmFPAfile *file = item->data.V; // File of interest
            fprintf(stderr, "%s: %p %p %p (%p) %p\n", file->name,
                    file->src, file->fpa,
                    file->camera, file->fpa->camera, file->format);
        }
        psFree(filesIter);
    }

    // Change the input dark type between the old (IMAGE) and new (multi-DARK).
    // Hopefully this is a temporary change until we all move over to using the new dark types
    bool mdok;                          // Status of MD lookup
    if (options->doDark && psMetadataLookupBool(&mdok, recipe, "OLDDARK")) {
        pmFPAfile *dark = psMetadataLookupPtr(NULL, config->files, "PPIMAGE.DARK");
        assert(dark);
        dark->type = PM_FPA_FILE_IMAGE;
    }

    return (options);
}

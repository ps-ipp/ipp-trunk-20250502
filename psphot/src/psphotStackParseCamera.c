# include "psphotStandAlone.h"

static pmFPAfile *defineFile(pmConfig *config, pmFPAfile *bind, const char *name, const char *filename, pmFPAfileType type);

// define the needed / desired I/O files
bool psphotStackParseCamera (pmConfig *config) {

    bool status = false;

    // the input images are defined as a set of metadatas in the INPUTS metadata folder
    psMetadata *inputs = psMetadataLookupMetadata(&status, config->arguments, "INPUTS"); // The inputs info
    if (!inputs) {
	psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find inputs.");
	return false;
    }

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    bool savePSF = psMetadataLookupBool(&status, recipe, "SAVE.PSF");
    // XXX: bills: I don't think psphotStack will work if SAVE.BACKMDL is false
    bool saveBackgroundModel = psMetadataLookupBool(&status, recipe, "SAVE.BACKMDL");
    bool saveBackground = psMetadataLookupBool(&status, recipe, "SAVE.BACKGND");
    bool saveBackSub = psMetadataLookupBool(&status, recipe, "SAVE.BACKSUB");
    bool saveResid = psMetadataLookupBool(&status, recipe, "SAVE.RESID");

    bool saveCnv = psMetadataLookupBool(&status, recipe, "SAVE.CNV");
    bool saveChisq = psMetadataLookupBool(&status, recipe, "SAVE.CHISQ");
    bool useRaw = psMetadataLookupBool(&status, recipe, "PSPHOT.STACK.USE.RAW");

    bool updateMode = psMetadataLookupBool(&status, config->arguments, "PSPHOT.STACK.UPDATEMODE");
    if (updateMode) {
        // Tell the sources reader to save a copy of the header from the input sources file on
        // readout->analysis so that we can copy some metadata from measurements that are skipped
        // in update mode from there to the analysis structure.
        psMetadataAddBool(recipe, PS_LIST_TAIL, "SAVE.INPUT.SOURCES.HEADER", PS_META_REPLACE, "", true);
    }


    int nRaw = 0;
    int nCnv = 0;
    int nInputs = inputs->list->n;
    for (int i = 0; i < nInputs; i++) {
	psMetadataItem *item = psMetadataGet(inputs, i);
	if (item->type != PS_DATA_METADATA) {
	    psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Component %s of the input metadata is not of type METADATA", item->name);
	    return false;
	}

	psMetadata *input = item->data.md; // The input metadata of interest

	pmFPAfile *rawInputFile = NULL;
	pmFPAfile *cnvInputFile = NULL;

	// RAW (unconvolved) input data (RAW:IMAGE, RAW:MASK, RAW:VARIANCE, RAW:PSF)
	psString rawImage = psMetadataLookupStr(&status, input, "RAW:IMAGE"); // Name of image
	if (rawImage && strlen(rawImage) > 0) {
	    rawInputFile = defineFile(config, NULL, "PSPHOT.STACK.INPUT.RAW", rawImage, PM_FPA_FILE_IMAGE); // File for image
	    if (!rawInputFile) {
		psError(PS_ERR_UNKNOWN, false, "Unable to define file from image %d (%s)", i, rawImage);
		return false;
	    }
	    psString mask = psMetadataLookupStr(&status, input, "RAW:MASK"); // Name of mask
	    if (mask && strlen(mask) > 0) {
		if (!defineFile(config, rawInputFile, "PSPHOT.STACK.MASK.RAW", mask, PM_FPA_FILE_MASK)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from mask %d (%s)", i, mask);
		    return false;
		}
	    }
	    psString variance = psMetadataLookupStr(&status, input, "RAW:VARIANCE"); // Name of variance map
	    if (variance && strlen(variance) > 0) {
		if (!defineFile(config, rawInputFile, "PSPHOT.STACK.VARIANCE.RAW", variance, PM_FPA_FILE_VARIANCE)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from variance %d (%s)", i, variance);
		    return false;
		}
	    }
	    psString psf = psMetadataLookupStr(&status, input, "RAW:PSF"); // Name of psf
	    if (psf && strlen(psf) > 0) {
		// if (!defineFile(config, rawInputFile, "PSPHOT.STACK.PSF.RAW", psf, PM_FPA_FILE_PSF)) {
		if (!defineFile(config, rawInputFile, "PSPHOT.PSF.LOAD", psf, PM_FPA_FILE_PSF)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from psf %d (%s)", i, psf);
		    return false;
		}
	    }
	    psString backmdl = psMetadataLookupStr(&status, input, "RAW:BACKMDL"); // Name of background model
	    if (backmdl && strlen(backmdl) > 0) {
		if (!defineFile(config, NULL, "PSPHOT.STACK.BACKMDL.RAW", backmdl, PM_FPA_FILE_IMAGE)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from backmdl %d (%s)", i, backmdl);
		    return false;
		}
	    }
	    psString expnum = psMetadataLookupStr(&status, input, "RAW:EXPNUM"); // Name of expnum image
	    if (expnum && strlen(expnum) > 0) {
		if (!defineFile(config, rawInputFile, "PSPHOT.STACK.EXPNUM.RAW", expnum, PM_FPA_FILE_EXPNUM)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from expnum %d (%s)", i, expnum);
		    return false;
		}
	    }
	    nRaw ++;
	}

	// CNV (convolved) input data (CNV:IMAGE, CNV:MASK, CNV:VARIANCE, CNV:PSF)
	psString cnvImage = psMetadataLookupStr(&status, input, "CNV:IMAGE"); // Name of image
	if (cnvImage && strlen(cnvImage) > 0) {
	    cnvInputFile = defineFile(config, NULL, "PSPHOT.STACK.INPUT.CNV", cnvImage, PM_FPA_FILE_IMAGE); // File for image
	    if (!cnvInputFile) {
		psError(PS_ERR_UNKNOWN, false, "Unable to define file from image %d (%s)", i, cnvImage);
		return false;
	    }
	    psString mask = psMetadataLookupStr(&status, input, "CNV:MASK"); // Name of mask
	    if (mask && strlen(mask) > 0) {
		if (!defineFile(config, cnvInputFile, "PSPHOT.STACK.MASK.CNV", mask, PM_FPA_FILE_MASK)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from mask %d (%s)", i, mask);
		    return false;
		}
	    }
	    psString variance = psMetadataLookupStr(&status, input, "CNV:VARIANCE"); // Name of variance map
	    if (variance && strlen(variance) > 0) {
		if (!defineFile(config, cnvInputFile, "PSPHOT.STACK.VARIANCE.CNV", variance, PM_FPA_FILE_VARIANCE)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from variance %d (%s)", i, variance);
		    return false;
		}
	    }
	    psString expnum = psMetadataLookupStr(&status, input, "CNV:EXPNUM"); // Name of EXPNUM image
	    if (expnum && strlen(expnum) > 0) {
		if (!defineFile(config, cnvInputFile, "PSPHOT.STACK.EXPNUM.CNV", expnum, PM_FPA_FILE_EXPNUM)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from expnum %d (%s)", i, expnum);
		    return false;
		}
	    }
	    psString psf = psMetadataLookupStr(&status, input, "CNV:PSF"); // Name of mask
	    if (psf && strlen(psf) > 0) {
		if (!defineFile(config, cnvInputFile, "PSPHOT.STACK.PSF.CNV", psf, PM_FPA_FILE_PSF)) {
		    psError(PS_ERR_UNKNOWN, false, "Unable to define file from psf %d (%s)", i, psf);
		    return false;
		}
	    }
	    nCnv ++;
	}

	if (!rawInputFile && !cnvInputFile) {
	    psError(PSPHOT_ERR_CONFIG, true, "Component %s (%d) lacks both RAW:IMAGE and CNV:IMAGE of type STR", item->name, i);
	    return false;
	}

	// XXX what if they do not match in length
	if (nCnv && nRaw) {
	    if (nCnv != nRaw) {
		psError (PSPHOT_ERR_CONFIG, true, "if both RAW and CNV images are supplied, the number must match");
		return false;
	    }
	}
        pmFPAfile *inputTemplate;
        if (useRaw) {
            inputTemplate = rawInputFile;
        } else {
            inputTemplate = cnvInputFile;
        }
        if (!inputTemplate) {
            psError(PS_ERR_UNKNOWN, true, "cannot determinte inputTemplate: USE.RAW: %d\n", useRaw);
            return false;
        }

        psString sources = psMetadataLookupStr(&status, input, "SOURCES"); // Name of sources
        if (sources && strlen(sources) > 0) {
            // input sources are not bound to fpa. 
            // XXX: bills: I believe that they are only required in -updatemode now.
            if (!defineFile(config, NULL, "PSPHOT.STACK.SOURCES", sources, PM_FPA_FILE_CMF)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to define file from sources %d (%s)", i, sources);
                return false;
            }
        }

        psS64 stack_id = psMetadataLookupS64(&status, input, "STACK_ID");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find STACK_ID from sources %d", i);
            return false;
        }
	// generate an pmFPAimage for the output convolved image
	{
	    // pmFPAfile *srcInputFile = rawInputFile ? rawInputFile : cnvInputFile;
	    pmFPAfile *outputImage = pmFPAfileDefineOutput(config, NULL, "PSPHOT.STACK.OUTPUT.IMAGE");
	    if (!outputImage) {
		psError(PSPHOT_ERR_CONFIG, false, "Trouble defining PSPHOT.STACK.OUTPUT.IMAGE");
		return false;
	    }
	    outputImage->save = saveCnv;
	    outputImage->fileID = stack_id;		// this is used to generate output names

	    pmFPAfile *outputMask = pmFPAfileDefineOutput(config, outputImage->fpa, "PSPHOT.STACK.OUTPUT.MASK");
	    if (!outputMask) {
		psError(PS_ERR_IO, false, _("Unable to generate output file from PSPHOT.STACK.OUTPUT.MASK"));
		return NULL;
	    }
	    if (outputMask->type != PM_FPA_FILE_MASK) {
		psError(PS_ERR_IO, true, "PSPHOT.STACK.OUTPUT.MASK is not of type MASK");
		return NULL;
	    }
	    outputMask->save = saveCnv;
	    outputMask->fileID = stack_id;		// this is used to generate output names

	    pmFPAfile *outputVariance = pmFPAfileDefineOutput(config, outputImage->fpa, "PSPHOT.STACK.OUTPUT.VARIANCE");
	    if (!outputVariance) {
		psError(PS_ERR_IO, false, _("Unable to generate output file from PSPHOT.STACK.OUTPUT.VARIANCE"));
		return NULL;
	    }
	    if (outputVariance->type != PM_FPA_FILE_VARIANCE) {
		psError(PS_ERR_IO, true, "PSPHOT.STACK.OUTPUT.VARIANCE is not of type VARIANCE");
		return NULL;
	    }
	    outputVariance->save = saveCnv;
	    outputVariance->fileID = stack_id;		// this is used to generate output names

	    // the output sources are carried on the outputImage->fpa structures
	    pmFPAfile *outsources = pmFPAfileDefineOutputFromFile (config, outputImage, "PSPHOT.STACK.OUTPUT");
	    if (!outsources) {
		psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.STACK.OUTPUT");
		return false;
	    }
	    outsources->save = true;
	    outsources->fileID = stack_id;		// this is used to generate output names

            if (savePSF) {
                pmFPAfile *output = pmFPAfileDefineOutputFromFile (config, inputTemplate, "PSPHOT.STACK.PSF.SAVE");
                if (!output) {
                    psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.STACK.PSF.SAVE");
                    return false;
                }
                // don't save psf in update mode?
                output->save = !updateMode;
                output->fileID = stack_id;
            }
            if (saveBackgroundModel) {
                int DX = psMetadataLookupS32 (&status, recipe, "BACKGROUND.XBIN");
                int DY = psMetadataLookupS32 (&status, recipe, "BACKGROUND.YBIN");
                pmFPAfile *output = pmFPAfileDefineFromFile (config, inputTemplate, DX, DY, "PSPHOT.STACK.BACKMDL");
                if (!output) {
                    psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.STACK.BACKMDL");
                    return false;
                }

                // do not save the background model file when in update mode. We need to create the file above
                // but we don't need save it.
                output->save = !updateMode;
                output->fileID = stack_id;
            }
            if (saveBackground) {
                pmFPAfile *output = pmFPAfileDefineFromFile (config, inputTemplate, 1, 1, "PSPHOT.STACK.BACKGND");
                if (!output) {
                    psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.STACK.STACK.BACKGND");
                    return false;
                }
                output->save = true;
                output->fileID = stack_id;
            }
            if (saveBackSub) {
                pmFPAfile *output = pmFPAfileDefineFromFile (config, inputTemplate, 1, 1, "PSPHOT.STACK.BACKSUB");
                if (!output) {
                    psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.STACK.STACK.BACKSUB");
                    return false;
                }
                output->save = true;
                output->fileID = stack_id;
            }
            if (saveResid) {
                pmFPAfile *output = pmFPAfileDefineOutputFromFile (config, inputTemplate, "PSPHOT.STACK.RESID");
                if (!output) {
                    psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.STACK.RESID");
                    return false;
                }
                output->save = true;
                output->fileID = stack_id;
            }
	}
    }
    psMetadataRemoveKey(config->arguments, "FILENAMES");
    psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.STACK.INPUT.RAW.NUM", PS_META_REPLACE, "number of inputs", nRaw);
    psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.STACK.INPUT.CNV.NUM", PS_META_REPLACE, "number of inputs", nCnv);
    psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.STACK.OUTPUT.IMAGE.NUM", PS_META_REPLACE, "number of inputs", nInputs);
    psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.INPUT.NUM", PS_META_REPLACE, "number of inputs", nInputs);

    if (!psphotSetMaskBits (config)) {
        psError (PS_ERR_UNKNOWN, false, "failed to set mask bit values");
        return NULL;
    }

#ifdef MAKE_CHISQ_IMAGE
    // generate an pmFPAimage for the chisqImage
    {
	pmFPAfile *chisqImage = pmFPAfileDefineOutput(config, NULL, "PSPHOT.CHISQ.IMAGE");
	if (!chisqImage) {
	    psError(PSPHOT_ERR_CONFIG, false, "Trouble defining PSPHOT.CHISQ.IMAGE");
	    return false;
	}
	chisqImage->save = saveChisq;

	pmFPAfile *chisqMask = pmFPAfileDefineOutput(config, chisqImage->fpa, "PSPHOT.CHISQ.MASK");
	if (!chisqMask) {
	    psError(PS_ERR_IO, false, _("Unable to generate output file from PSPHOT.CHISQ.MASK"));
	    return NULL;
	}
	if (chisqMask->type != PM_FPA_FILE_MASK) {
	    psError(PS_ERR_IO, true, "PSPHOT.CHISQ.MASK is not of type MASK");
	    return NULL;
	}
	chisqMask->save = saveChisq;

	pmFPAfile *chisqVariance = pmFPAfileDefineOutput(config, chisqImage->fpa, "PSPHOT.CHISQ.VARIANCE");
	if (!chisqVariance) {
	    psError(PS_ERR_IO, false, _("Unable to generate output file from PSPHOT.CHISQ.VARIANCE"));
	    return NULL;
	}
	if (chisqVariance->type != PM_FPA_FILE_VARIANCE) {
	    psError(PS_ERR_IO, true, "PSPHOT.CHISQ.VARIANCE is not of type VARIANCE");
	    return NULL;
	}
	chisqVariance->save = saveChisq;
    }
#else
    (void) saveChisq;
#endif   // MAKE_CHISQ_IMAGE

    psTrace("psphot", 1, "Done with psphotStackParseCamera...\n");

    psErrorClear();                     // some metadata lookup may have failed
    return true;
}

// Define a file
static pmFPAfile *defineFile(pmConfig *config, // Configuration
                             pmFPAfile *bind, // File to which to bind
                             const char *name, // Name of file rule
                             const char *filename, // Name of file
                             pmFPAfileType type // Type of file
                             )
{

    psArray *dummy = psArrayAlloc(1);   // Dummy array of filenames for this FPA
    dummy->data[0] = psStringCopy(filename);
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, "FILENAMES", PS_META_REPLACE,
                       "Filenames for file rule definition", dummy);
    psFree(dummy);

    bool found = false;             // Found the file?
    pmFPAfile *file = bind ? pmFPAfileBindFromArgs(&found, bind, config, name, "FILENAMES") :
        pmFPAfileDefineFromArgs(&found, config, name, "FILENAMES");
    if (!file || !found) {
        psError(PS_ERR_UNKNOWN, false, "Unable to define file %s from %s", name, filename);
        return NULL;
    }
    if (file->type != type) {
        psError(PS_ERR_IO, true, "%s is not of type %s", name, pmFPAfileStringFromType(type));
        return NULL;
    }

    return file;
}


/***
 *
 *  psphotStack :

 *    * inputs:
 *      * unconvolved images
 *      * raw convolved images
 *      * psfs (unconvolved or convolved?)
 *      * sources
 
 * optionally convolve the unconvolved or the raw inputs
 * optionally perform no convolutions
 * optionally save the psf-matched images

 */

    
# if (0)    
    // define the additional input/output files associated with psphot
    // XXX figure out which files are needed by psphotStack
    if (false && !psphotDefineFiles (config, input)) {
        psError(PSPHOT_ERR_CONFIG, false, "Trouble defining the additional input/output files");
        return false;
    }
# endif


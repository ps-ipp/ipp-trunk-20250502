/** @file pswarpParseCamera.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.24 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 03:10:36 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

static bool foundAstrom = false;
static bool foundVariance = false;
static bool foundBackground = false;

bool pswarpParseCamera(pmConfig *config)
{
    psAssert(config, "Require configuration");
    bool status;                          // Status of MD lookup

    // *** parse the input information (either from -file or from -input)

    // if INPUTS exists, we have a metadata file to provide input, variance, mask, etc
    psMetadata *inputFile = psMetadataLookupPtr(&status, config->arguments, "INPUTS");
    if (inputFile) {
	if (!pswarpParseMultiInput (config, inputFile)) {
	    psError(PSWARP_ERR_CONFIG, false, "failed to parse multi-input file");
	    return false;
	}
    } else {
	if (!pswarpParseSingleInput (config)) {
	    psError(PSWARP_ERR_CONFIG, false, "failed to parse multi-input file");
	    return false;
	}
    }

    // once we have read the input mask headers (if any), we can use that to set the mask bits
    if (!pswarpSetMaskBits(config)) {
        psError(psErrorCodeLast(), false, "failed to set mask bits");
        return false;
    }

    // *** parse the output information (output target, output astrometry [skycell])

    // The input skycell is a required argument: it defines the output image
    pmConfig *skyConfig = NULL;
    pmFPAfile *skycell = NULL;

    skycell = pmFPAfileDefineNewConfig(&status, &skyConfig, config, "PSWARP.SKYCELL", "SKYCELL");
    if (!status || !skycell) {
      psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.SKYCELL");
      return false;
      }
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "SKYCELL.CAMERA", 0, "Name of camera for skycell", skyConfig->cameraName);
    psFree(skyConfig);

    psMetadata *recipe = psMetadataLookupPtr (&status, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, false, "missing recipe %s", PSWARP_RECIPE);
        return false;
    }

    // use the skycell camera or the input camera?
    bool useInputCamera = !strcmp (skycell->cameraName, "SIMPLE");

    // The output skycell
    pmFPAfile *output = useInputCamera ? 
	pmFPAfileDefineSkycell(config, NULL, "PSWARP.OUTPUT") :
	pmFPAfileDefineOutputForFormat(config, NULL, "PSWARP.OUTPUT", skycell->cameraName, skycell->formatName);
    if (!output) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.OUTPUT");
        return false;
    }
    output->save = true;
    pmFPAAddSourceFromFormat(output->fpa, output->format); // ** builds the HDUs, is this OK?

    pmFPAfile *outMask = useInputCamera ?
	pmFPAfileDefineSkycell(config, output->fpa, "PSWARP.OUTPUT.MASK"):
	pmFPAfileDefineOutputForFormat(config, output->fpa, "PSWARP.OUTPUT.MASK", skycell->cameraName, skycell->formatName);
    if (!outMask) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.OUTPUT.MASK");
        return false;
    }
    outMask->save = true;

    // only create an output variance in we supply an input variance
    if (foundVariance) {
	pmFPAfile *outVariance = useInputCamera ? 
	    pmFPAfileDefineSkycell(config, output->fpa, "PSWARP.OUTPUT.VARIANCE"):
	    pmFPAfileDefineOutputForFormat(config, output->fpa, "PSWARP.OUTPUT.VARIANCE", skycell->cameraName, skycell->formatName);
        if (!outVariance) {
            psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.OUTPUT.VARIANCE");
            return false;
        }
        outVariance->save = true;
    }

    // XXX we assume input sources come from the input astrom description, but this need not be true (we could define an input sources file)
    if (foundAstrom && psMetadataLookupBool(&status, recipe, "SOURCES")) {
	pmFPAfile *outSources = useInputCamera ? 
	    pmFPAfileDefineSkycell(config, output->fpa, "PSWARP.OUTPUT.SOURCES"):
	    pmFPAfileDefineOutputForFormat(config, output->fpa, "PSWARP.OUTPUT.SOURCES", skycell->cameraName, skycell->formatName);
        if (!outSources) {
            psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.OUTPUT.SOURCES");
            return false;
        }
        outSources->save = true;
    }

    // only create an output background if an input background is supplied
    if (foundBackground && psMetadataLookupBool(&status, recipe, "BACKGROUND.MODEL")) {
	// pmFPAfile *outBackground = pmFPAfileDefineSkycell(config, NULL, "PSWARP.OUTPUT.BKGMODEL");
	pmFPAfile *outBackground = useInputCamera ? 
	    pmFPAfileDefineSkycell(config, NULL, "PSWARP.OUTPUT.BKGMODEL"):
	    pmFPAfileDefineOutputForFormat(config, NULL, "PSWARP.OUTPUT.BKGMODEL", skycell->cameraName, skycell->formatName);
	if (!outBackground) {
	    psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.OUTPUT.BKGMODEL");
	    return false;
	}
	outBackground->xBin = psMetadataLookupS32(&status, recipe, "BKG.XGRID");
	outBackground->yBin = psMetadataLookupS32(&status, recipe, "BKG.YGRID");
	outBackground->save = true;
	pmFPAAddSourceFromFormat(outBackground->fpa, outBackground->format); // ** builds the HDUs, is this OK?
    }

    if (psMetadataLookupBool(&status, recipe, "PSF")) {
        // This file, PSPHOT.INPUT, is just used as a carrier; output files (eg, PSPHOT.RESID) are defined by psphotDefineFiles
	pmFPAfile *psphotInput = useInputCamera ? 
	    pmFPAfileDefineSkycell(config, NULL, "PSPHOT.INPUT"):
	    pmFPAfileDefineOutputForFormat(config, NULL, "PSPHOT.INPUT", skycell->cameraName, skycell->formatName);
        if (!psphotInput) {
            psError(psErrorCodeLast(), false, _("Unable to generate output file from PSPHOT.INPUT"));
            return false;
        }
        psphotInput->src = psMemIncrRefCounter(output->fpa);
	pmFPAAddSourceFromFormat(psphotInput->fpa, psphotInput->format); // ** builds the HDUs, is this OK?

        // specify the number of psphot input images (psphotReadout loops over all input images)
        psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.INPUT.NUM", PS_META_REPLACE, "number of inputs", 1);

	// the input sources (read from the input astrometry file) are transformed (in pswarpLoop) to the readout->analysis
	// entries of the output file PSWARP.OUTPUT.SOURCES, associated with the main output pmFPAfile PSWARP.OUTPUT

	// the PSPHOT.INPUT.CMF is used to supply those sources to psphot (specifically psphotLoadPSFSources).  

        // pmFPAfile *psphotInSources = pmFPAfileDefineSkycell(config, output->fpa, "PSPHOT.INPUT.CMF");
	pmFPAfile *psphotInSources = useInputCamera ? 
	    pmFPAfileDefineSkycell(config, output->fpa, "PSPHOT.INPUT.CMF"):
	    pmFPAfileDefineOutputForFormat(config, output->fpa, "PSPHOT.INPUT.CMF", skycell->cameraName, skycell->formatName);
        if (!psphotInSources) {
            psError(psErrorCodeLast(), false, _("Unable to generate output file from PSPHOT.INPUT.CMF"));
            return false;
        }

        // Ensure psphot defines an output file for the PSF.
        psMetadata *psphotRecipe = psMetadataLookupPtr(NULL, config->recipes, PSPHOT_RECIPE); // Recipe
        if (!psphotRecipe) {
            psError(PSWARP_ERR_CONFIG, false, "Unable to find %s recipe.", PSPHOT_RECIPE);
            return false;
        }
        psMetadataAddBool(psphotRecipe, PS_LIST_TAIL, "SAVE.PSF", PS_META_REPLACE, "Save PSF?", true);

        // Define associated psphot input/output files
        if (!psphotDefineFiles(config, psphotInput)) {
            psError(PSWARP_ERR_CONFIG, false,
                    "Unable to define the additional input/output files for psphot");
            return false;
        }

        // Turn off writing the sources we find as part of deriving the PSF --- they might clobber the ones we
        // get from transforming the input coordinates.
        pmFPAfile *psphotSources = psMetadataLookupPtr(NULL, config->files, "PSPHOT.OUTPUT");
        assert(psphotSources);
        psphotSources->save = false;
    }
 
    // only keep the necessary (output) camera configs and relevant recipes
    const char *skyCamera = psMetadataLookupStr(NULL, config->arguments, "SKYCELL.CAMERA");
    pmConfigCamerasCull(config, skyCamera);
    pmConfigRecipesCull(config, "PSWARP,PPSTATS,PSPHOT,PSASTRO,MASKS,JPEG");

    psTrace("pswarp", 1, "Done with pswarpParseCamera...\n");
    return true;
}

bool pswarpParseSingleInput (pmConfig *config) {

    // The input image(s) is required: it defines the camera
    pmFPAfile *input = pswarpDefineInputFile(config, NULL, "PSWARP.INPUT", "INPUT", PM_FPA_FILE_IMAGE);
    if (!input) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.INPUT");
        return false;
    }

    // Define the input astrometry file(s) (may be either WCS or CMF)
    bool status = false;
    pmFPAfile *astrom = pmFPAfileDefineFromArgs(&status, config, "PSWARP.ASTROM", "ASTROM");
    if (!astrom) {
        // look for the file on the RUN metadata
        astrom = pmFPAfileDefineFromRun(&status, NULL, config, "PSWARP.ASTROM"); // File to return
        if (!status) {
            psLogMsg("pswarp", PS_LOG_INFO, "Failed to load file definition for %s", "PSWARP.ASTROM");
        }
    }
    if (astrom) {
	if ((astrom->type != PM_FPA_FILE_CMF) && (astrom->type != PM_FPA_FILE_WCS)) {
	    psLogMsg("pswarp", PS_LOG_INFO, "%s is neither CMF nor WCS", "PSWARP.ASTROM");
	    // XXX raise an error here??
	} else {
	    foundAstrom = true;
	}
    }
    psLogMsg("pswarp", PS_LOG_INFO, "Astrometry source: %s", astrom ? "supplied" : "header");

    // Define the input mask file(s)
    pmFPAfile *inMask = pswarpDefineInputFile(config, input, "PSWARP.MASK", "MASK", PM_FPA_FILE_MASK);
    if (!inMask) {
        psLogMsg("pswarp", PS_LOG_INFO, "No mask supplied");
    }

    // Define the input variance file(s)
    pmFPAfile *inVariance = pswarpDefineInputFile(config, input, "PSWARP.VARIANCE", "VARIANCE", PM_FPA_FILE_VARIANCE);
    if (inVariance) {
	foundVariance = true;
    } else {
        psLogMsg("pswarp", PS_LOG_INFO, "No variance supplied");
    }

    pmFPAfile *inBackground = pswarpDefineInputFile(config, NULL, "PSWARP.BKGMODEL", "BACKGROUND", PM_FPA_FILE_IMAGE);
    if (inBackground) {
	foundBackground = true;
    } else {
      // cannot do the background model if an input is not supplied.
      psMetadataAddBool (config->arguments, PS_LIST_TAIL, "BACKGROUND.MODEL", PS_META_REPLACE, "no input background supplied", false);
      psLogMsg("pswarp", PS_LOG_INFO, "No background models supplied");
    }

    // Chip selection: turn on only the chips specified
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    if (status) {
        psArray *chips = psStringSplitArray (chipLine, ",", false);
        if (chips->n > 0) {
            pmFPASelectChip (input->fpa, -1, true); // deselect all chips
            for (int i = 0; i < chips->n; i++) {
                int chipNum = atoi(chips->data[i]);
                if (! pmFPASelectChip(input->fpa, chipNum, false)) {
                    psError(PSWARP_ERR_ARGUMENTS, true, "Chip number %d doesn't exist in camera.\n", chipNum);
                    return false;
                }
            }
        }
        psFree(chips);
    }

    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "NUM_INPUTS", PS_META_REPLACE, "input file sets", 1);
    return true;
}

// read input file information from a metadata file
// XXX for now, in the multi-input format, we disable PSF, BACKGROUND, and CHIP_SELECTIONS
bool pswarpParseMultiInput (pmConfig *config, psMetadata *fileListMD) {

    // the multi-input file consists of a set of metadata blocks, each with entries 
    // for INPUT and (possibly) ASTROM, MASK, VARIANCE, ???

    bool status = false;

    for (int i = 0; i < fileListMD->list->n; i++) {
	psMetadataItem *item = psMetadataGet(fileListMD, i);
	if (item->type != PS_DATA_METADATA) {
	    psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Component %s of the input metadata is not of type METADATA", item->name);
	    return false;
	}

	// next input metadata block of interest
	psMetadata *fileBlockMD = item->data.md;

	psString inputName = psMetadataLookupStr(&status, fileBlockMD, "INPUT"); // Name of image
	if (!inputName || !strlen(inputName)) {
	    psError(PS_ERR_UNKNOWN, false, "input file not found in metadata block %d (%s)", i, item->name);
	    return false;
	}
	AddStringAsArray (config->arguments, inputName, "FILENAMES");

	pmFPAfile *input = pswarpDefineInputFile (config, NULL, "PSWARP.INPUT", "FILENAMES", PM_FPA_FILE_IMAGE);
	if (!input) {
	    psError(psErrorCodeLast(), false, "Failed to build FPA from PSWARP.INPUT");
	    return false;
	}

	psString astromName = psMetadataLookupStr(&status, fileBlockMD, "ASTROM"); // Name of astrom file
	if (astromName && strlen(astromName)) {
	    AddStringAsArray (config->arguments, astromName, "FILENAMES");

	    pmFPAfile *astrom = pmFPAfileDefineFromArgs (&status, config, "PSWARP.ASTROM", "FILENAMES");
	    if (!status) {
		psLogMsg("pswarp", PS_LOG_INFO, "Failed to load file definition for %s", "PSWARP.ASTROM");
	    }
	    if (astrom) {
		if ((astrom->type != PM_FPA_FILE_CMF) && (astrom->type != PM_FPA_FILE_WCS)) {
		    psLogMsg("pswarp", PS_LOG_INFO, "%s is neither CMF nor WCS", "PSWARP.ASTROM");
		} else {
		    foundAstrom = true;
		}
	    }
	    psLogMsg("pswarp", PS_LOG_INFO, "Astrometry source: %s", astrom ? "supplied" : "header");
	}

	pmFPAfile *mask = NULL;
	psString maskName = psMetadataLookupStr(&status, fileBlockMD, "MASK"); // Name of mask
	if (maskName && strlen(maskName)) {
	    AddStringAsArray (config->arguments, maskName, "FILENAMES");

	    mask = pswarpDefineInputFile (config, input, "PSWARP.MASK", "FILENAMES", PM_FPA_FILE_MASK);
	}
	if (!mask) {
	    psLogMsg("pswarp", PS_LOG_INFO, "No mask supplied");
	}

	pmFPAfile *variance = NULL;
	psString varianceName = psMetadataLookupStr(&status, fileBlockMD, "VARIANCE"); // Name of variance
	if (varianceName && strlen(varianceName)) {
	    AddStringAsArray (config->arguments, varianceName, "FILENAMES");

	    variance = pswarpDefineInputFile (config, input, "PSWARP.VARIANCE", "FILENAMES", PM_FPA_FILE_VARIANCE);
	}
	if (variance) {
	    foundVariance = true;
	} else {
	    psLogMsg("pswarp", PS_LOG_INFO, "No variance supplied");
	}

	// XXX for now we do not have an option to supply a background model set in multi input mode
	psMetadataAddBool (config->arguments, PS_LIST_TAIL, "BACKGROUND.MODEL", PS_META_REPLACE, "no input background supplied", false);
    }

    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "NUM_INPUTS", PS_META_REPLACE, "input file sets", fileListMD->list->n);
    return true;
}

// Define an input file based on name in config->arguments or config RUN block
pmFPAfile *pswarpDefineInputFile(pmConfig *config,// Configuration
				 pmFPAfile *bind,    // File to which to bind, or NULL
				 char *filerule,     // Name of file rule
				 char *argname,      // Argument name
				 pmFPAfileType fileType // Type of file
    )
{
    bool status;

    pmFPAfile *file = NULL;
    // look for the file on the argument list
    if (bind) {
        file = pmFPAfileBindFromArgs(&status, bind, config, filerule, argname);
    } else {
        file = pmFPAfileDefineFromArgs(&status, config, filerule, argname);
    }
    if (!status) {
        psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
        return false;
    }
    if (!file) {
        // look for the file on the RUN metadata
        file = pmFPAfileDefineFromRun(&status, bind, config, filerule); // File to return
        if (!status) {
            psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
            return NULL;
        }
    }

    if (!file) {
        return NULL;
    }

    if (file->type != fileType) {
        psError(PSWARP_ERR_CONFIG, true, "%s is not of type %s", filerule, pmFPAfileStringFromType(fileType));
        return NULL;
    }

    return file;
}

bool AddStringAsArray (psMetadata *md, char *string, char *name) {

    psArray *dummy = psArrayAlloc(1);   // Dummy array of filenames for this FPA
    dummy->data[0] = psStringCopy(string);

    psMetadataAddArray(md, PS_LIST_TAIL, name, PS_META_REPLACE, "string added as array", dummy);
    psFree(dummy);
    return true;
}


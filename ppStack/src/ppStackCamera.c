#include "ppStack.h"

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
        psError(psErrorCodeLast(), false, "Unable to define file %s from %s", name, filename);
        return NULL;
    }
    if (file->type != type) {
        psError(PS_ERR_IO, PPSTACK_ERR_CONFIG, "%s is not of type %s", name, pmFPAfileStringFromType(type));
        return NULL;
    }

    return file;
}

pmConfig *pmConfigMakeTemp (pmConfig *config) {
    pmConfig *altconfig = pmConfigAlloc();

    // these are NULL on pmConfigAlloc
    altconfig->user   = psMemIncrRefCounter(config->user);   // inherit from primary camera
    altconfig->site   = psMemIncrRefCounter(config->site);   // inherit from primary camera
    altconfig->system = psMemIncrRefCounter(config->system); // inherit from primary camera

    psFree (altconfig->files);
    altconfig->files  = psMemIncrRefCounter(config->files); // inherit from primary camera

    psFree (altconfig->arguments);
    altconfig->arguments = psMemIncrRefCounter(config->arguments); // inherit from primary camera

    psFree (altconfig->recipes);
    altconfig->recipes = psMetadataCopy(NULL, config->recipes); // container for camera-specific recipe values (to be dropped)

    return (altconfig);
}

bool ppStackCamera(pmConfig *config)
{
    int num = 0;                        // Number of inputs
    bool haveVariances = false;         // Do we have variance maps?
    bool havePSFs = false;              // Do we have PSFs?

    bool status = false;                // Status of file definition

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // Recipe for ppSim
    if (!recipe) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find recipe %s", PPSTACK_RECIPE);
        return false;
    }
    bool convolve = psMetadataLookupBool(NULL, recipe, "CONVOLVE"); // Convolve images before stack?

    psArray *runImages = pmFPAfileDefineMultipleFromRun(&status, NULL, config, "PPSTACK.INPUT"); // Input images from previous run
    if (runImages) {
        // Defining files from the RUN metadata
        num = runImages->n;

        psArray *runMasks = pmFPAfileDefineMultipleFromRun(&status, runImages, config, "PPSTACK.INPUT.MASK"); // Input masks
        if (!status) {
            psError(psErrorCodeLast(), false, "Unable to define input masks from RUN metadata.");
            psFree(runImages);
            return false;
        }
        psFree(runMasks);

        psArray *runVars = pmFPAfileDefineMultipleFromRun(&status, runImages, config, "PPSTACK.INPUT.VARIANCE"); // Input variances
        if (!status) {
            psError(psErrorCodeLast(), false, "Unable to define input variances from RUN metadata.");
            psFree(runImages);
            return false;
        }
        if (runVars) {
            haveVariances = true;
        }
        psFree(runVars);

        psArray *runSrc = pmFPAfileDefineMultipleFromRun(&status, runImages, config, "PPSTACK.INPUT.SOURCES"); // Input sources
        if (!status) {
            psError(psErrorCodeLast(), false, "Unable to define input sources from RUN metadata.");
            psFree(runImages);
            return false;
        }
        if (!runSrc) {
            psError(PPSTACK_ERR_CONFIG, true, "Unable to define input sources from RUN metadata.");
            psFree(runImages);
            return false;
        }
        psFree(runSrc);

        if (convolve) {
            {
                psArray *runPSF = pmFPAfileDefineMultipleFromRun(&status, runImages, config, "PPSTACK.INPUT.PSF"); // Input PSFs
                if (!status) {
                    psError(psErrorCodeLast(), false, "Unable to define input PSFs from RUN metadata.");
                    psFree(runImages);
                    return false;
                }
                if (runPSF) {
                    havePSFs = true;
                }
                psFree(runPSF);
            }
            {

                psArray *runKernel = pmFPAfileDefineMultipleFromRun(&status, runImages, config, "PPSTACK.CONV.KERNEL"); // Conv'n kernels
                if (!status) {
                    psError(psErrorCodeLast(), false,
                            "Unable to define convolution kernels from RUN metadata.");
                    psFree(runImages);
                    return false;
                }
                if (!runKernel) {
                    psError(PPSTACK_ERR_CONFIG, true,
                            "Unable to define convolution kernels from RUN metadata.");
                    psFree(runImages);
                    return false;
                }
                psFree(runKernel);
            }
        }

        psFree(runImages);
    } else {
        // Defining files from the input metadata
        psMetadata *inputs = psMetadataLookupMetadata(NULL, config->arguments, "INPUTS"); // The inputs info
        if (!inputs) {
            psError(PPSTACK_ERR_ARGUMENTS, false, "Unable to find inputs.");
            return false;
        }
        psMetadataIterator *iter = psMetadataIteratorAlloc(inputs, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *item;               // Item from iteration
        int i = 0;                          // Counter
        while ((item = psMetadataGetAndIncrement(iter))) {
            if (item->type != PS_DATA_METADATA) {
                psError(PPSTACK_ERR_ARGUMENTS, true,
                        "Component %s of the input metadata is not of type METADATA", item->name);
                psFree(iter);
                return false;
            }

            psMetadata *input = item->data.md; // The input metadata of interest

            psString image = psMetadataLookupStr(NULL, input, "IMAGE"); // Name of image
            if (!image || strlen(image) == 0) {
                psError(PPSTACK_ERR_ARGUMENTS, true, "Component %s lacks IMAGE of type STR", item->name);
                psFree(iter);
                return false;
            }

            bool mdok;
            psString mask = psMetadataLookupStr(&mdok, input, "MASK"); // Name of mask
            psString variance = psMetadataLookupStr(&mdok, input, "VARIANCE"); // Name of variance map
            psString psf = psMetadataLookupStr(&mdok, input, "PSF"); // Name of PSF
            psString sources = psMetadataLookupStr(&mdok, input, "SOURCES"); // Name of sources
	    psString bkgmodel = psMetadataLookupStr(&mdok, input, "BKGMODEL"); // Name of warped background model
	    
	    // Use a temporary config for all but the first image, keeping the main user,
	    // site, system, files, arguments entries loaded for the first image.  This
	    // allows the images to be from different cameras than the first input image.
	    // NOTE: there is no check that these images match in terms of size, pixel
	    // scale, etc. That is up to the user.
	    
	    pmConfig *tempConfig = (i == 0) ? psMemIncrRefCounter(config) : pmConfigMakeTemp(config);

            pmFPAfile *imageFile = defineFile(tempConfig, NULL, "PPSTACK.INPUT",
                                              image, PM_FPA_FILE_IMAGE); // File for image
            if (!imageFile) {
                psError(psErrorCodeLast(), false, "Unable to define file from image %d (%s)", i, image);
                return false;
            }

            if (mask && strlen(mask) > 0 &&
                !defineFile(tempConfig, imageFile, "PPSTACK.INPUT.MASK", mask, PM_FPA_FILE_MASK)) {
                psError(psErrorCodeLast(), false, "Unable to define file from mask %d (%s)", i, mask);
                return false;
            }

            if (variance && strlen(variance) > 0) {
                haveVariances = true;
                if (!defineFile(tempConfig, imageFile, "PPSTACK.INPUT.VARIANCE", variance,
                                PM_FPA_FILE_VARIANCE)) {
                    psError(psErrorCodeLast(), false,
                            "Unable to define file from variance %d (%s)", i, variance);
                    return false;
                }
            }

            if (psf && strlen(psf) > 0) {
                if (i != 0 && !havePSFs) {
                    psWarning("PSF not provided for all inputs --- ignoring.");
                } else {
                    havePSFs = true;
                    if (!defineFile(tempConfig, imageFile, "PPSTACK.INPUT.PSF", psf, PM_FPA_FILE_PSF)) {
                        psError(psErrorCodeLast(), false, "Unable to define file from psf %d (%s)", i, psf);
                        return false;
                    }
                }
            } else if (havePSFs) {
                psError(PPSTACK_ERR_CONFIG, true, "Unable to find PSF %d", i);
                return false;
            }

            if (!sources || strlen(sources) == 0) {
                psError(PPSTACK_ERR_CONFIG, true, "SOURCES not provided for file %d", i);
                return false;
            }
            if (!defineFile(tempConfig, imageFile, "PPSTACK.INPUT.SOURCES", sources, PM_FPA_FILE_CMF)) {
                psError(psErrorCodeLast(), false, "Unable to define file from sources %d (%s)",
                        i, sources);
                return false;
            }

            if (convolve) {
                pmFPAfile *kernel = pmFPAfileDefineOutput(tempConfig, imageFile->fpa, "PPSTACK.CONV.KERNEL");
                if (!kernel) {
                    psError(psErrorCodeLast(), false,
                            "Unable to generate output file from PPSTACK.CONV.KERNEL");
                    return false;
                }
                kernel->save = true;
            }

	    // Grab bkgmodel information here
	    if ((!bkgmodel) || (strlen(bkgmodel) == 0)) {
	      // We have no background models.
	      psMetadataAddBool(recipe, PS_LIST_TAIL, "BACKGROUND.MODEL", PS_META_REPLACE, "Do photometry on stacked image?", false);
	    } else {
	      pmFPAfile *inputBKG = defineFile(tempConfig,NULL,"PPSTACK.INPUT.BKGMODEL",bkgmodel, PM_FPA_FILE_IMAGE);
	      if (!inputBKG) {
		psMetadataAddBool(recipe, PS_LIST_TAIL, "BACKGROUND.MODEL", PS_META_REPLACE, "Do photometry on stacked image?", false);

#if (0)
		psError(psErrorCodeLast(), false,
			"Unable to define file from bkgmodel %d (%s)",i,bkgmodel);
		return(false);
#endif
	      }
	    } // End bkgmodel
	    psFree(tempConfig);
            i++;
        }
        psFree(iter);
        psMetadataRemoveKey(config->arguments, "FILENAMES");
        num = i;
    }
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INPUTS.NUM", 0, "Number of input files", num);
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "HAVE.PSF", 0, "Have PSFs available?", havePSFs);

    // Output image
    pmFPA *outFPA = pmFPAConstruct(config->camera, config->cameraName); // FPA to contain the output
    if (!outFPA) {
        psError(psErrorCodeLast(), false, "Unable to construct an FPA from camera configuration.");
        return false;
    }
    pmFPAfile *output = pmFPAfileDefineOutput(config, outFPA, "PPSTACK.OUTPUT");
    psFree(outFPA);                        // Drop reference
    if (!output) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT"));
        return false;
    }
    if (output->type != PM_FPA_FILE_IMAGE) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT is not of type IMAGE");
        return false;
    }
    output->save = true;

    if (!pmFPAAddSourceFromFormat(outFPA, output->format)) {
        psError(psErrorCodeLast(), false, "Unable to generate output FPA.");
        return false;
    }

    // Output mask
    pmFPAfile *outMask = pmFPAfileDefineOutput(config, output->fpa, "PPSTACK.OUTPUT.MASK");
    if (!outMask) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.MASK"));
        return false;
    }
    if (outMask->type != PM_FPA_FILE_MASK) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.MASK is not of type MASK");
        return false;
    }
    outMask->save = true;

    // Output variance
    if (haveVariances) {
        pmFPAfile *outVariance = pmFPAfileDefineOutput(config, output->fpa, "PPSTACK.OUTPUT.VARIANCE");
        if (!outVariance) {
            psError(psErrorCodeLast(), false, "Unable to generate output file from PPSTACK.OUTPUT.VARIANCE");
            return false;
        }
        if (outVariance->type != PM_FPA_FILE_VARIANCE) {
            psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.VARIANCE is not of type VARIANCE");
            return false;
        }
        outVariance->save = true;
    }


    // Exposure image
    pmFPA *expFPA = pmFPAConstruct(config->camera, config->cameraName); // FPA to contain the output
    if (!expFPA) {
        psError(psErrorCodeLast(), false, "Unable to construct an FPA from camera configuration.");
        return false;
    }
    pmFPAfile *exp = pmFPAfileDefineOutput(config, expFPA, "PPSTACK.OUTPUT.EXP");
    psFree(expFPA);                        // Drop reference
    if (!exp) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.EXP"));
        return false;
    }
    if (exp->type != PM_FPA_FILE_IMAGE) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.EXP is not of type IMAGE");
        return false;
    }
    exp->save = true;

    if (!pmFPAAddSourceFromFormat(expFPA, exp->format)) {
        psError(psErrorCodeLast(), false, "Unable to generate output FPA.");
        return false;
    }

    // Exposure numbers
    pmFPAfile *expNum = pmFPAfileDefineOutput(config, exp->fpa, "PPSTACK.OUTPUT.EXPNUM");
    if (!expNum) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.EXPNUM"));
        return false;
    }
    if (expNum->type != PM_FPA_FILE_MASK) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.EXPNUM is not of type MASK");
        return false;
    }
    expNum->save = true;

    // Weighted exposure
    pmFPAfile *expWt = pmFPAfileDefineOutput(config, exp->fpa, "PPSTACK.OUTPUT.EXPWT");
    if (!expWt) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.EXPWT"));
        return false;
    }
    if (expWt->type != PM_FPA_FILE_VARIANCE) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.EXPWT is not of type VARIANCE");
        return false;
    }
    expWt->save = true;


    if (havePSFs) {
        pmFPA *psfFPA = pmFPAConstruct(config->camera, config->cameraName); // FPA to contain PSF
        if (!psfFPA) {
            psError(psErrorCodeLast(), false, "Unable to construct an FPA from camera configuration.");
            return false;
        }
        pmFPAfile *targetPSF = pmFPAfileDefineOutput(config, psfFPA, "PPSTACK.TARGET.PSF");
        if (!targetPSF) {
            psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.TARGET.PSF"));
            return false;
        }
        psFree(psfFPA);
        if (targetPSF->type != PM_FPA_FILE_PSF) {
            psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.TARGET.PSF is not of type PSF");
            return false;
        }
        targetPSF->save = true;
    }

    // Unconvolved stack
    pmFPA *unconvFPA = pmFPAConstruct(config->camera, config->cameraName); // FPA to contain unconvolved output
    if (!unconvFPA) {
        psError(psErrorCodeLast(), false, "Unable to construct an FPA from camera configuration.");
        return false;
    }
    pmFPAfile *unConv = pmFPAfileDefineOutput(config, unconvFPA, "PPSTACK.UNCONV");
    psFree(unconvFPA);                  // Drop reference
    if (!unConv) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.UNCONV"));
        return false;
    }
    if (unConv->type != PM_FPA_FILE_IMAGE) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.UNCONV is not of type IMAGE");
        return false;
    }
    unConv->save = true;

    if (!pmFPAAddSourceFromFormat(unconvFPA, unConv->format)) {
        psError(psErrorCodeLast(), false, "Unable to generate output FPA.");
        return false;
    }

    // Unconvolved mask
    pmFPAfile *unconvMask = pmFPAfileDefineOutput(config, unconvFPA, "PPSTACK.UNCONV.MASK");
    if (!unconvMask) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.UNCONV.MASK"));
        return false;
    }
    if (unconvMask->type != PM_FPA_FILE_MASK) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.UNCONV.MASK is not of type MASK");
        return false;
    }
    unconvMask->save = true;

    // Unconvolved variance
    if (haveVariances) {
        pmFPAfile *unconvVariance = pmFPAfileDefineOutput(config, unconvFPA, "PPSTACK.UNCONV.VARIANCE");
        if (!unconvVariance) {
            psError(psErrorCodeLast(), false, "Unable to generate output file from PPSTACK.UNCONV.VARIANCE");
            return false;
        }
        if (unconvVariance->type != PM_FPA_FILE_VARIANCE) {
            psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.UNCONV.VARIANCE is not of type VARIANCE");
            return false;
        }
        unconvVariance->save = true;
    }


    // Exposure image
    pmFPA *unconvExpFPA = pmFPAConstruct(config->camera, config->cameraName); // FPA to contain the output
    if (!unconvExpFPA) {
        psError(psErrorCodeLast(), false, "Unable to construct an FPA from camera configuration.");
        return false;
    }
    pmFPAfile *unconvExp = pmFPAfileDefineOutput(config, unconvExpFPA, "PPSTACK.UNCONV.EXP");
    psFree(unconvExpFPA);               // Drop reference
    if (!unconvExp) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.UNCONV.EXP"));
        return false;
    }
    if (unconvExp->type != PM_FPA_FILE_IMAGE) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.UNCONV.EXP is not of type IMAGE");
        return false;
    }
    unconvExp->save = true;

    if (!pmFPAAddSourceFromFormat(unconvExpFPA, unconvExp->format)) {
        psError(psErrorCodeLast(), false, "Unable to generate output FPA.");
        return false;
    }

    // Exposure numbers
    pmFPAfile *unconvExpNum = pmFPAfileDefineOutput(config, unconvExp->fpa, "PPSTACK.UNCONV.EXPNUM");
    if (!unconvExpNum) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.UNCONV.MASK"));
        return false;
    }
    if (unconvExpNum->type != PM_FPA_FILE_MASK) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.UNCONV.EXPNUM is not of type MASK");
        return false;
    }
    unconvExpNum->save = true;

    // Weighted exposure
    pmFPAfile *unconvExpWt = pmFPAfileDefineOutput(config, unconvExp->fpa, "PPSTACK.UNCONV.EXPWT");
    if (!unconvExpWt) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.UNCONV.EXPWT"));
        return false;
    }
    if (unconvExpWt->type != PM_FPA_FILE_VARIANCE) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.UNCONV.EXPWT is not of type VARIANCE");
        return false;
    }
    unconvExpWt->save = true;

    // Output JPEGs
    pmFPAfile *jpeg1 = pmFPAfileDefineOutput(config, NULL, "PPSTACK.OUTPUT.JPEG1");
    if (!jpeg1) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.JPEG1"));
        return false;
    }
    if (jpeg1->type != PM_FPA_FILE_JPEG) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.JPEG1 is not of type JPEG");
        return false;
    }
    jpeg1->save = true;
    pmFPAfile *jpeg2 = pmFPAfileDefineOutput(config, NULL, "PPSTACK.OUTPUT.JPEG2");
    if (!jpeg2) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.JPEG2"));
        return false;
    }
    if (jpeg2->type != PM_FPA_FILE_JPEG) {
        psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.JPEG2 is not of type JPEG");
        return false;
    }
    jpeg2->save = true;

    // Output background
    pmFPAfile *outBkg = pmFPAfileDefineOutput(config,NULL,"PPSTACK.OUTPUT.BKGMODEL");
    if (!outBkg) {
      psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.BKGMODEL"));
      return(false);
    }
    if (outBkg->type != PM_FPA_FILE_IMAGE) {
      psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.BKGMODEL is not of type IMAGE");
      return(false);
    }
    outBkg->save = true;
    pmFPAfile *outBkgRest = pmFPAfileDefineOutput(config,NULL,"PPSTACK.OUTPUT.BKGREST");
    if (!outBkgRest) {
      psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSTACK.OUTPUT.BKGREST"));
      return(false);
    }
    if (outBkgRest->type != PM_FPA_FILE_IMAGE) {
      psError(PPSTACK_ERR_CONFIG, true, "PPSTACK.OUTPUT.BKGREST is not of type IMAGE");
      return(false);
    }
    
    // For photometry, we operate on the chip-mosaicked image
    // we create a copy of the mosaicked image for psphot so we can write out a clean image
    bool mdok = false;
    bool doPhotom = psMetadataLookupBool(&mdok, recipe, "PHOTOMETRY") ||
        psMetadataLookupBool(&mdok, config->arguments, "-photometry"); // perform photometry
    if (doPhotom) {
        // This pmFPAfile, PSPHOT.INPUT, is just used as a carrier; output files (eg,
        // PSPHOT.RESID) are defined by psphotDefineFiles
        pmFPAfile *psphotInput = pmFPAfileDefineFromFPA(config, output->fpa, 1, 1, "PSPHOT.INPUT");
        if (!psphotInput) {
            psError(psErrorCodeLast(), false, _("Unable to generate output file from PSPHOT.INPUT"));
            return false;
        }
        // specify the number of psphot input images
        psMetadataAddS32 (config->arguments, PS_LIST_TAIL, "PSPHOT.INPUT.NUM", PS_META_REPLACE, "number of inputs", 1);

        // Define associated psphot input/output files
        if (!psphotDefineFiles(config, psphotInput)) {
            psError(psErrorCodeLast(), false,
                    "Trouble defining the additional input/output files for psphot");
            return false;
        }
    } else {
        // Output PSF --- only required if photometry is not being performed
        pmFPAfile *outPSF = pmFPAfileDefineOutputFromFile(config, output, "PSPHOT.PSF.SAVE");
        if (!outPSF) {
            psError(psErrorCodeLast(), false, _("Unable to generate output file from PSPHOT.PSF.SAVE"));
            return false;
        }
        if (outPSF->type != PM_FPA_FILE_PSF) {
            psError(PPSTACK_ERR_CONFIG, true, "PSPHOT.PSF.SAVE is not of type PSF");
            return false;
        }
        outPSF->save = true;
    }

    // Define output file here.
    
    return true;
}

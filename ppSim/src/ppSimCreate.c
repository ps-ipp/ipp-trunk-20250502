# include "ppSim.h"

pmFPAfile *ppSimCreate(pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    bool status;
    pmFPA *fpa = NULL;

    // the input image defines the camera.  if it is not supplied, the user must have
    // supplied a camera and other metadata on the command line
    pmFPAfile *input = pmFPAfileDefineFromArgs (&status, config, "PPSIM.INPUT", "INPUT");
    if (!input) {
        // if we have not specified the camera already, we need to interpolate the recipes associated with this camera, and read other command-line recipes
        if (!pmConfigReadRecipes(config, PM_RECIPE_SOURCE_CL)) {
            psError(PS_ERR_IO, false, "Error merging recipes from camera config for %s", config->cameraName);
            return NULL;
        }
    } else {
        // If an image is supplied, we still generate a fake image and merge them together downstream
        // (otherwise, we get the variance wrong).
        if (input->type != PM_FPA_FILE_IMAGE) {
            psError(PS_ERR_IO, true, "PPSIM.INPUT is not of type IMAGE");
            return NULL;
        }
    }

    assert (config->camera);

    int nx = psMetadataLookupS32 (&status, config->arguments, "NX.CELL");
    int ny = psMetadataLookupS32 (&status, config->arguments, "NY.CELL");

    if (nx || ny) {
      // need to find format(s)
      psMetadata *formats = psMetadataLookupPtr(&status, config->camera, "FORMATS");
      psAssert (formats, "FORMATS missing from camera config");
      
      psMetadataIterator *iter = psMetadataIteratorAlloc(formats, PS_LIST_HEAD, NULL); // Iterator
      psMetadataItem *format;               // Item from iteration
      while ((format = psMetadataGetAndIncrement(iter))) {
	psAssert (format->type == PS_DATA_METADATA, "unexpected format block");
	
	if (nx) {
	  psMetadata *defaults = psMetadataLookupPtr(&status, format->data.md, "DEFAULTS");
	  psMetadataLookupS32 (&status, defaults, "CELL.XSIZE");
	  psAssert (status, "CELL.XSIZE should be in DEFAULTS");
	  psMetadataAddF32(defaults, PS_LIST_TAIL, "CELL.XSIZE", PS_META_REPLACE, "", nx);
	}
	if (ny) {
	  psMetadata *defaults = psMetadataLookupPtr(&status, format->data.md, "DEFAULTS");
	  psMetadataLookupS32 (&status, defaults, "CELL.YSIZE");
	  psAssert (status, "CELL.YSIZE should be in DEFAULTS");
	  psMetadataAddF32(defaults, PS_LIST_TAIL, "CELL.YSIZE", PS_META_REPLACE, "", ny);
	}
      }
      psFree(iter);
    }

    // generate the fpa structure used by the output camera (determined from INPUT or specified)
    fpa = pmFPAConstruct(config->camera, config->cameraName); // FPA to contain the observation
    if (!fpa) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to construct an FPA from camera configuration.");
        return NULL;
    }

    // define the output image file -- this is the basis for the ppSimLoop
    pmFPAfile *output = pmFPAfileDefineOutput(config, fpa, "PPSIM.OUTPUT");
    if (!output) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to create output file from PPSIM.OUTPUT. Did you forget to specify the format?");
        return NULL;
    }
    if (output->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "PPSIM.OUTPUT type is not IMAGE");
        psFree(fpa);
        return NULL;
    }
    // XXX we should not require the output image to be written
    output->save = true;

    config->format = psMemIncrRefCounter (output->format);
    config->formatName = psStringCopy (output->formatName);

    // the recipe is now fully realized for the desired camera
    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

    char *typeStr = psMetadataLookupStr(NULL, recipe, "IMAGE.TYPE"); // Type of image to simulate
    ppSimType type = ppSimTypeFromString (typeStr); // Type of image to simulate

    if (type == PPSIM_TYPE_OBJECT) {
        // adjust the seeing by the scale
        float seeing = psMetadataLookupF32(&status, recipe, "SEEING");
        if (isnan(seeing)) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "seeing is not defined");
            psFree(fpa);
            return NULL;
        }
        float scale = psMetadataLookupF32(&status, recipe, "PIXEL.SCALE");
        if (isnan(scale)) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "pixel scale is not defined");
            psFree(fpa);
            return NULL;
        }
        psMetadataAddF32(recipe, PS_LIST_TAIL, "SEEING", PS_META_REPLACE, "Seeing SIGMA (pixels)", seeing / 2.0 / sqrt(2.0 * log(2.0)) / scale);

        float seeingMax = psMetadataLookupF32(&status, recipe, "SEEING.MAX");
        psMetadataAddF32(recipe, PS_LIST_TAIL, "SEEING.MAX", PS_META_REPLACE, "Seeing SIGMA (pixels)", seeingMax / 2.0 / sqrt(2.0 * log(2.0)) / scale);

        // if we have been supplied an input image, but no ra & dec, use the input image values
        if (input) {
            float ra = psMetadataLookupF32(&status, recipe, "RA");
            if (isnan(ra)) {
                ra = psMetadataLookupF64(&status, input->fpa->concepts, "FPA.RA");
                psMetadataAddF32(recipe, PS_LIST_TAIL, "RA", PS_META_REPLACE, "ra (radians)", ra);
            }
            float dec = psMetadataLookupF32(&status, recipe, "DEC");
            if (isnan(dec)) {
                dec = psMetadataLookupF64(&status, input->fpa->concepts, "FPA.DEC");
                psMetadataAddF32(recipe, PS_LIST_TAIL, "DEC", PS_META_REPLACE, "dec (radians)", dec);
            }
        }
    }

    if ((type == PPSIM_TYPE_OBJECT) || (type == PPSIM_TYPE_FLAT)) {
        // determine the zeropoint from the filter
        float zp = psMetadataLookupF32(&status, recipe, "ZEROPOINT");
        if (isnan(zp)) {
            char *filter = psMetadataLookupStr(&status, recipe, "FILTER");
            float zp = ppSimGetZeroPoint (recipe, filter);
            psMetadataAddF32(recipe, PS_LIST_TAIL, "ZEROPOINT", PS_META_REPLACE, "Photometric zeropoint", zp);
        }
    }

    // For photometry, we operate on the chip-mosaicked image.  we create a copy of the mosaicked
    // image for psphot so we can write out a clean image
    bool doPhotom = psMetadataLookupBool(&status, recipe, "PHOTOM"); // Density of fakes
    if (doPhotom) {
	psError(PS_ERR_UNKNOWN, false, "in-line photometry in ppSim had been deprecated");
	return NULL;
    }

    // have we supplied a psf model?  this happens in ppSimPhotomFiles if we request a photometry
    // analysis.  however, even if we do not, a psf model may be used to generate the fake
    // sources.
    if (psMetadataLookupPtr(NULL, config->arguments, "PSPHOT.PSF")) {
	// tie the psf file to the chipMosaic
	pmFPAfileBindFromArgs(&status, output, config, "PSPHOT.PSF.LOAD", "PSPHOT.PSF");
	if (!status) {
	    psError(PS_ERR_UNKNOWN, false, "Failed to find/build PSPHOT.PSF.LOAD");
	    psFree(fpa);
	    return NULL;
	}
    }

    // PPSIM.SOURCES carries the constructed, fake sources with their true parameters
    // XXX only invoke this code for OBJECT types of images?
    pmFPAfile *simSources = pmFPAfileDefineOutput (config, output->fpa, "PPSIM.SOURCES");
    if (!simSources) {
        psError(PS_ERR_UNKNOWN, false, "Cannot find a rule for PPSIM.SOURCES");
        return false;
    }

    // XXXX TEST this is causing trouble for unknown reasons -- output step is blowing up.
    simSources->save = true;
    // simSources->save = false;

    // if we have loaded an input image, we derive certain values from the image, if possible
    if (input) {
        // we need to extract certain metadata from the image and populate the recipe.
        // or else we need to set the fpa concepts based on the recipe options...

        psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

        ppSimArgToRecipeF32(&status, recipe, "EXPTIME", input->fpa->concepts, "FPA.EXPOSURE");
        char *filter = ppSimArgToRecipeStr(&status, recipe, "FILTER", input->fpa->concepts, "FPA.FILTERID");

        float zp = ppSimGetZeroPoint(recipe, filter);
        if (!isfinite(zp)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find zero point for filter %s", filter);
            psFree(fpa);
            return NULL;
        }
        psMetadataAddF32(recipe, PS_LIST_TAIL, "ZEROPOINT", PS_META_REPLACE, "Photometric zeropoint", zp);
    }

    pmFPALevel phuLevel = pmFPAPHULevel(output->format); // Level at which PHU goes

    pmFPAview *view = pmFPAviewAlloc(0);// View for current level

    if (phuLevel == PM_FPA_LEVEL_FPA) {
        if (!pmFPAAddSourceFromView(fpa, view, output->format)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to FPA.");
            psFree(fpa);
            psFree(view);
            return NULL;
        }
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, fpa, 1))) {
        if (phuLevel == PM_FPA_LEVEL_CHIP) {
            if (!pmFPAAddSourceFromView(fpa, view, output->format)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to FPA.");
                psFree(fpa);
                psFree(view);
                return NULL;
            }
        }

        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, fpa, 1))) {
            if (phuLevel == PM_FPA_LEVEL_CELL) {
                if (!pmFPAAddSourceFromView(fpa, view, output->format)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to FPA.");
                    psFree(fpa);
                    psFree(view);
                    return NULL;
                }
            }
	    // XXX this is a hack, but I don't have a better way at the moment: assumes a single cell per chip
	    if (nx) {
		psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.XSIZE", PS_META_REPLACE, "", nx);
	    } else {
		nx = psMetadataLookupF32(&status, cell->concepts, "CELL.XSIZE");
		psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.XSIZE", PS_META_REPLACE, "", nx);
	    }
	    if (ny) {
		psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.YSIZE", PS_META_REPLACE, "", ny);
	    } else {
		ny = psMetadataLookupF32(&status, cell->concepts, "CELL.YSIZE");
		psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.YSIZE", PS_META_REPLACE, "", ny);
	    }
        }
    }

    psFree(fpa);
    psFree(view);

    return output;
}

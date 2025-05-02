#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// In this function, we perform the psphot analysis routine for the chip-mosaicked images
bool ppImageSubtractBackground(pmConfig *config, const pmFPAview *view, const ppImageOptions *options)
{
    psAssert(config, "Need configuration");
    psAssert(view, "Need view to chip");
    psAssert(options, "Need options");

    if (!options->doBG) {
        return true;
    }

    bool status;                        // Status of MD lookup
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPIMAGE.CHIP"); // File to correct
    if (!status) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "PPIMAGE.CHIP file is not defined");
        return false;
    }

    psMetadata *ppImageRecipe = psMetadataLookupPtr(NULL, config->recipes, RECIPE_NAME);
    psAssert(ppImageRecipe, "Need PPIMAGE recipe");
    psMetadata *psphotRecipe = psMetadataLookupPtr(NULL, config->recipes, PSPHOT_RECIPE);
    psAssert(psphotRecipe, "Need PSPHOT recipe");

    // XXX Should this be options->maskValue or options->maskValue & ~options->satMask?
    //     The latter will leave saturated pixels high
    psImageMaskType maskVal = options->maskValue;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psMetadataAddImageMask(psphotRecipe, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "user-defined mask", maskVal);

    // Since we are working on a chip-mosaicked image, there should only be a single cell and readout
    pmChip *chip = pmFPAviewThisChip(view, input->fpa); // Chip of interest
    if (!chip) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find chip");
        return false;
    }
    if (chip->cells->n == 0) {
        psWarning("Chip has no cells");
        return true;
    }
    if (chip->cells->n > 1) {
        psWarning("Chip has %ld cells; only the first will be processed", chip->cells->n);
    }
    pmCell *cell = chip->cells->data[0]; // Cell of interest
    if (!cell || !cell->process || !cell->file_exists) {
        // Nothing to process
        return true;
    }
    if (cell->readouts->n == 0) {
        psWarning("Cell has no readouts");
        return true;
    }
    if (cell->readouts->n > 1) {
        psWarning("Cell has %ld readouts; only the first will be processed", cell->readouts->n);
    }
    pmReadout *ro = cell->readouts->data[0]; // Readout of interest
    if (!ro || !ro->data_exists) {
        // Nothing to process
        return true;
    }
    psImage *image = ro->image, *mask = ro->mask; // Image and mask of interest

    // View corresponding to this readout
    pmFPAview roView = *view;
    roView.cell = roView.readout = 0;

    // If the background model file has not been defined, psphotModelBackground will generate it
    pmReadout *modelRO = NULL;
    pmFPAfile *modelFile = psMetadataLookupPtr(&status, config->files, "PSPHOT.BACKMDL"); // Background model
    if (modelFile && modelFile->fpa) {
        modelRO = pmFPAviewThisReadout(&roView, modelFile->fpa); // Background model
    }

    // the background model has not been defined, or at least not generated
    if (!modelFile || !modelRO) {
        if (!psphotModelBackgroundReadoutFileIndex(config, &roView, "PPIMAGE.CHIP", 0)) {
            int lastError = psErrorCodeLast();
	    if (lastError == PSPHOT_ERR_DATA) {
	      // a data error in psphotModelBackground* means an empty or bad image: skip background subtraction
	      psErrorStackPrint(stderr, "Unable to model background");
	      psErrorClear();
	      return true;
	    } else {
	      psError(PS_ERR_UNKNOWN, false, "Unable to model background");
	      return false;
	    }
        }
        // the model file should now at least be defined
        modelFile = psMetadataLookupPtr(&status, config->files, "PSPHOT.BACKMDL"); // Background model
        if (!modelFile) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to define background model I/O file");
            return false;
        }
        // now grab the readout from the correct location:
        if (modelFile->mode == PM_FPA_MODE_INTERNAL) {
            modelRO = modelFile->readout;
        } else {
            modelRO = pmFPAviewThisReadout(&roView, modelFile->fpa);
        }
        if (!modelRO) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find background model");
            return false;
        }
    }
    psImageBinning *binning = psMetadataLookupPtr(&status, modelRO->analysis,
                                                  "PSPHOT.BACKGROUND.BINNING"); // Binning for model
    if (!binning) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find background binning");
        return false;
    }

# define USE_UNBIN 1
# if (USE_UNBIN)
    // select background pixels, from output background file, or create
    pmReadout *background = NULL;
    pmFPAfile *backfile = psMetadataLookupPtr (&status, config->files, "PSPHOT.BACKGND");
    if (backfile) {
        // we are using PSPHOT.BACKGND as an I/O file: select readout or create
        if (backfile->mode == PM_FPA_MODE_INTERNAL) {
            background = backfile->readout;
        } else {
            background = pmFPAviewThisReadout (&roView, backfile->fpa);
        }
        if (background == NULL) {
            // readout does not yet exist: create from input
            pmFPAfileCopyStructureView (backfile->fpa, input->fpa, 1, 1, &roView);
            background = pmFPAviewThisReadout (&roView, backfile->fpa);
            if ((image->numCols != background->image->numCols) || (image->numRows != background->image->numRows)) {
                psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for background dimensions");
                return false;
            }
        }
    } else {
        background = pmFPAfileDefineInternal (config->files, "PSPHOT.BACKGND", image->numCols, image->numRows, PS_TYPE_F32);
    }
    psF32 **backData = background->image->data.F32;

    // linear interpolation to full-scale
    if (!psImageUnbin (background->image, modelRO->image, binning)) {
        psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for unbinning");
        return false;
    }

    // Do the background subtraction
    int numCols = image->numCols, numRows = image->numRows; // Size of image
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if(options->doApplyPixelZero) {
              if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                  image->data.F32[y][x] = 0.0;
              } else {
		float value = backData[y][x];
		if (!isfinite(value)) {
		    image->data.F32[y][x] = NAN;
		    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= options->lowMask;
		} else {
		    image->data.F32[y][x] -= value;
		}
  	      }
            } else {
		float value = backData[y][x];
		if (!isfinite(value)) {
		    image->data.F32[y][x] = NAN;
		    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= options->lowMask;
		} else {
		    image->data.F32[y][x] -= value;
		}
            }
        }
    }
# else
    // Do the background subtraction
    int numCols = image->numCols, numRows = image->numRows; // Size of image
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if(options->doApplyPixelZero) {
              if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                  image->data.F32[y][x] = 0.0;
              } else {
                float value = psImageUnbinPixel(x + 0.5, y + 0.5, modelRO->image, binning); // Background value
                if (!isfinite(value)) {
                    image->data.F32[y][x] = NAN;
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= options->lowMask;
                } else {
                    image->data.F32[y][x] -= value;
                }
              }
            } else {
                float value = psImageUnbinPixel(x + 0.5, y + 0.5, modelRO->image, binning); // Background value
                if (!isfinite(value)) {
                    image->data.F32[y][x] = NAN;
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= options->lowMask;
                } else {
                    image->data.F32[y][x] -= value;
                }
            }
        }
    }
# endif

    // XXX should these really be here?? (probably not...)
    // pmFPAfileDropInternal(config->files, "PSPHOT.BACKMDL");
    // pmFPAfileDropInternal(config->files, "PSPHOT.BACKMDL.STDEV");

    return true;
}

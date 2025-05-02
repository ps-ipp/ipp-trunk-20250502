#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"

// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  pmConfig *config      // Configuration
    )
{
    fprintf(stderr, "\nPan-STARRS image convolution\n\n");
    fprintf(stderr, "Usage: %s OUTPUT_ROOT\n\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
}

// Put filename in it's own list
static void fileList(const char *file, // The symbolic name for the file
                     const char *name, // The name of the file
                     const char *comment, // Description of the file
                     pmConfig *config // Configuration
    )
{
    if (!name) {
        return;
    }
    psArray *files = psArrayAlloc(1); // Array with file names
    files->data[0] = psStringCopy(name);
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, file, 0, comment, files);
    psFree(files);
    return;
}

void ppSubSetThreads (void) {
    // ppSub does not have any of its own thread handlers
    return;
}

int main(int argc, char *argv[])
{
    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy

    pmConfig *config = pmConfigRead(&argc, argv, PPSUB_RECIPE); // Configuration
    if (!config) {
        psError(psErrorCodeLast(), false, "Unable to read configuration");
        goto die;
    }

    // generic arguments (version, dumpconfig)
    PS_ARGUMENTS_GENERIC( ppSub, config, argc, argv );

    // thread arguments
    PS_ARGUMENTS_THREADS( ppSub, config, argc, argv )

    bool reference = false;             // Input is actually the reference image?
    int threads = 0;                    // Number of threads
    {
        psMetadata *arguments = config->arguments; // Command-line arguments
        psMetadataAddStr(arguments, PS_LIST_TAIL, "-image", 0, "Input image", NULL);
        psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask", 0, "Input mask", NULL);
        psMetadataAddStr(arguments, PS_LIST_TAIL, "-variance", 0, "Input variance", NULL);
        psMetadataAddStr(arguments, PS_LIST_TAIL, "-kernel", 0, "Convolution kernel", NULL);
        psMetadataAddBool(arguments, PS_LIST_TAIL, "-reference", 0, "Input is actually reference?", false);
        psMetadataAddBool(arguments, PS_LIST_TAIL, "-save-all", 0, "Save all outputs?", false);

        if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 2) {
            usage(argv[0], arguments, config);
        }

        const char *inImage = psMetadataLookupStr(NULL, arguments, "-image"); // Input image
        const char *inMask = psMetadataLookupStr(NULL, arguments, "-mask"); // Input mask
        const char *inVariance = psMetadataLookupStr(NULL, arguments, "-variance"); // Input variance
        const char *inKernel = psMetadataLookupStr(NULL, arguments, "-kernel"); // Input kernel
        if (!inImage || !inKernel) {
            usage(argv[0], arguments, config);
        }

        reference = psMetadataLookupBool(NULL, arguments, "-reference");
        threads = psMetadataLookupS32(NULL, arguments, "NTHREADS");
        fileList("PPSUB.INPUT", inImage, "Input image", config);
        fileList("PPSUB.INPUT.MASK", inMask, "Input mask", config);
        fileList("PPSUB.INPUT.VARIANCE", inVariance, "Input variance", config);
        fileList("PPSUB.INPUT.KERNEL", inKernel, "Input kernel", config);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);

        bool status;                    // Status of file definition
        pmFPAfile *image = pmFPAfileDefineFromArgs(&status, config, "PPSUB.INPUT", "PPSUB.INPUT");
        if (!image || !status || image->type != PM_FPA_FILE_IMAGE) {
            psError(PPSUB_ERR_CONFIG, false, "Unable to define input image file.");
            goto die;
        }
        if (inMask) {
            pmFPAfile *mask = pmFPAfileBindFromArgs(&status, image, config, "PPSUB.INPUT.MASK",
                                                    "PPSUB.INPUT.MASK");
            if (!mask || !status || mask->type != PM_FPA_FILE_MASK) {
                psError(PPSUB_ERR_CONFIG, false, "Unable to define input mask file.");
                goto die;
            }
        }
        if (inVariance) {
            pmFPAfile *variance = pmFPAfileBindFromArgs(&status, image, config, "PPSUB.INPUT.VARIANCE",
                                                        "PPSUB.INPUT.VARIANCE");
            if (!variance || !status || variance->type != PM_FPA_FILE_VARIANCE) {
                psError(PPSUB_ERR_CONFIG, false, "Unable to define input variance file.");
                goto die;
            }
        }

        pmFPAfile *kernel = pmFPAfileBindFromArgs(&status, image, config, "PPSUB.INPUT.KERNEL",
                                                  "PPSUB.INPUT.KERNEL");
        if (!kernel || !status || kernel->type != PM_FPA_FILE_SUBKERNEL) {
            psError(PPSUB_ERR_CONFIG, false, "Unable to define kernel file.");
            goto die;
        }

        pmFPAfile *output = pmFPAfileDefineFromFile(config, image, 1, 1, "PPSUB.INPUT.CONV");
        if (!output || output->type != PM_FPA_FILE_IMAGE) {
            psError(PPSUB_ERR_CONFIG, false, "Unable to define output file.");
            goto die;
        }
        output->save = true;

        if (psMetadataLookupBool(NULL, arguments, "-save-all")) {
            pmFPAfile *outMask = pmFPAfileDefineOutput(config, output->fpa, "PPSUB.INPUT.CONV.MASK");
            if (!outMask || outMask->type != PM_FPA_FILE_MASK) {
                psError(PPSUB_ERR_CONFIG, false, "Unable to define output mask file.");
                goto die;
            }
            outMask->save = true;

            pmFPAfile *outVar = pmFPAfileDefineOutput(config, output->fpa, "PPSUB.INPUT.CONV.VARIANCE");
            if (!outVar || outVar->type != PM_FPA_FILE_VARIANCE) {
                psError(PPSUB_ERR_CONFIG, false, "Unable to define output variance file.");
                goto die;
            }
            outVar->save = true;
        }
    }

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    if (!recipe) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to find recipe %s", PPSUB_RECIPE);
        goto die;
    }

    // Read everything
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(psErrorCodeLast(), false, "File checks failed.");
        goto die;
    }
    view->chip = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(psErrorCodeLast(), false, "File checks failed.");
        goto die;
    }
    view->cell = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(psErrorCodeLast(), false, "File checks failed.");
        goto die;
    }
    view->readout = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(psErrorCodeLast(), false, "File checks failed.");
        goto die;
    }


    pmReadout *inRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT"); // Input readout
    pmSubtractionKernels *kernels = psMetadataLookupPtr(NULL, inRO->analysis,
                                                        PM_SUBTRACTION_ANALYSIS_KERNEL); // Convolution kernel
    pmCell *outCell = pmFPAfileThisCell(config->files, view, "PPSUB.INPUT.CONV"); // Output cell
    pmReadout *outRO = pmReadoutAlloc(outCell);                                   // Output readout
    pmConceptsCopyFPA(outRO->parent->parent->parent, inRO->parent->parent->parent, true, true);
    pmHDU *inHDU = pmHDUFromCell(inRO->parent); // Input header
    pmHDU *outHDU = pmHDUFromCell(outCell);     // Output header
    outHDU->header = psMetadataCopy(outHDU->header, inHDU->header);
    ppSubVersionHeader(outHDU->header);

    pmReadout *input, *ref, *inConv, *refConv; // Input and output readouts
    if (reference) {
        ref = inRO;
        refConv = outRO;
        input = NULL;
        inConv = NULL;
    } else {
        input = inRO;
        inConv = outRO;
        ref = NULL;
        refConv = NULL;
    }

    bool convolve = false;              // Do we need to convolve?
    float norm = 1.0;                   // Normalisation to apply
    switch (kernels->mode) {
      case PM_SUBTRACTION_MODE_1:
        if (!reference) {
            convolve = true;
        }
        norm = 1.0 / kernels->solution1->data.F64[PM_SUBTRACTION_INDEX_NORM(kernels)];
        break;
      case PM_SUBTRACTION_MODE_2:
        if (reference) {
            convolve = true;
        }
        // No normalisation; should already be normalised appropriately
        break;
      case PM_SUBTRACTION_MODE_DUAL: {
          // We only have a single input, so hack together a single-mode kernel
          int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index of normalisation term
          norm = 1.0 / kernels->solution1->data.F64[normIndex];
          convolve = true;
          if (reference) {
              for (int i = 0; i < kernels->solution2->n; i++) {
                  kernels->solution1->data.F64[i] = -kernels->solution2->data.F64[i];
              }
              kernels->solution1->data.F64[normIndex] = 1.0;
              kernels->solution1->data.F64[PM_SUBTRACTION_INDEX_BG(kernels)] = 0.0;
              kernels->mode = PM_SUBTRACTION_MODE_2;
          } else {
              kernels->mode = PM_SUBTRACTION_MODE_1;
          }
          psFree(kernels->solution2);
          kernels->solution2 = NULL;
          break;
      }
      default:
        psAbort("Unrecognised kernel mode: %x", kernels->mode);
    }

    if (convolve) {
        int stride = psMetadataLookupS32(NULL, recipe, "STRIDE"); // Size of convolution patches
        float kernelErr = psMetadataLookupF32(NULL, recipe, "KERNEL.ERR"); // Relative sys error in kernel
        float covarFrac = psMetadataLookupF32(NULL, recipe, "COVAR.FRAC"); // Fraction for covar calculation
        float badFrac = psMetadataLookupF32(NULL, recipe, "BADFRAC"); // Maximum bad fraction
        float poorFrac = psMetadataLookupF32(NULL, recipe, "POOR.FRACTION"); // Fraction for "poor"

        psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Bits to mask in inputs
        psImageMaskType maskPoor = pmConfigMaskGet("CONV.POOR", config); // Bits to mask for poor pixels
        psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels

        if (threads > 0) {
            pmSubtractionThreadsInit();
        }

	// Mask the NAN values (USE BLANK instead of SAT?)
	if (!pmReadoutMaskInvalid(input, maskVal, maskBad)) {
	  psError(PPSUB_ERR_DATA, false, "Unable to mask non-finite pixels in input.");
	  return false;
	}
	if (!pmReadoutMaskInvalid(ref, maskVal, maskBad)) {
	  psError(PPSUB_ERR_DATA, false, "Unable to mask non-finite pixels in reference.");
	  return false;
	}

        if (!pmSubtractionMatchPrecalc(inConv, refConv, input, ref, inRO->analysis,
                                       stride, kernelErr, covarFrac, maskVal, maskBad, maskPoor,
                                       poorFrac, badFrac)) {
            psError(psErrorCodeLast(), false, "Unable to convolve images.");
            if (threads > 0) {
                pmSubtractionThreadsFinalize();
            }
            goto die;
        }
        if (threads > 0) {
            pmSubtractionThreadsFinalize();
        }
    } else {
        // Subtract background, since that's what ppSub will do with it
        psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Bits to mask in inputs
        psStats *bg = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics for background
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        psStatsInit(bg);
        if (!psImageBackground(bg, NULL, inRO->image, inRO->mask, maskVal, rng)) {
            psError(PPSUB_ERR_DATA, false, "Unable to measure background statistics.");
            psFree(bg);
            psFree(rng);
            goto die;
        }
        psFree(rng);
        outRO->image = (psImage*)psBinaryOp(outRO->image, inRO->image, "-",
                                            psScalarAlloc((float)bg->robustMedian, PS_TYPE_F32));
        psFree(bg);
        outRO->data_exists = outRO->parent->data_exists = outRO->parent->parent->data_exists = true;
    }

    if (norm != 1.0) {
        psBinaryOp(outRO->image, outRO->image, "*", psScalarAlloc(norm, PS_TYPE_F32));
    }

    psFree(outRO);

 die:
    {
        psExit exitValue = ppSubExitCode(PS_EXIT_SUCCESS); // Exit value

        // Write everything
        view->chip = view->cell = view->readout = 0;
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "File checks failed.");
            exitValue = ppSubExitCode(exitValue);
            pmFPAfileFreeSetStrict(false);
        }
        view->readout = -1;
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "File checks failed.");
            exitValue = ppSubExitCode(exitValue);
            pmFPAfileFreeSetStrict(false);
        }
        view->cell = -1;
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "File checks failed.");
            exitValue = ppSubExitCode(exitValue);
            pmFPAfileFreeSetStrict(false);
        }
        view->chip = -1;
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "File checks failed.");
            exitValue = ppSubExitCode(exitValue);
            pmFPAfileFreeSetStrict(false);
        }
        psFree(view);

        psFree(config);
        pmConceptsDone();
        pmConfigDone();
        psLibFinalize();
        exit(exitValue);
    }

    psAbort("Should never reach here.");
}

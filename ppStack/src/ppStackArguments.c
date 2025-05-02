#include "ppStack.h"

// XXX add in the version info as in ppImage

// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  pmConfig *config      // Configuration
    )
{
    fprintf(stderr, "\nPan-STARRS Image combination\n\n");
    fprintf(stderr,
            "Usage: %s -input INPUTS.mdc OUTPUT_ROOT [-sources STAMPS.cmf | -stamps STAMPS.dat]\n"
            "where INPUTS.mdc contains various METADATAs, each with:\n"
            "\tIMAGE(STR):     Image filename\n"
            "\tMASK(STR):      Mask filename\n"
            "\tVARIANCE(STR):  Variance map filename\n"
            "\tPSF(STR):       PSF filename\n"
            "\tSOURCES(STR):   Sources filename\n",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
}

// Get a float-point value from the command-line or recipe, and add it to the arguments
#define VALUE_ARG_RECIPE_FLOAT(ARGNAME, RECIPENAME, TYPE) { \
    ps##TYPE value = psMetadataLookup##TYPE(NULL, config->arguments, ARGNAME); \
    if (isnan(value)) { \
        bool mdok; \
        value = psMetadataLookup##TYPE(&mdok, recipe, RECIPENAME); \
        if (!mdok) { \
            psError(PPSTACK_ERR_CONFIG, true, "Unable to find %s in recipe %s", \
                RECIPENAME, PPSTACK_RECIPE); \
            goto ERROR; \
        } \
    } \
    psMetadataAdd##TYPE(recipe, PS_LIST_TAIL, RECIPENAME, PS_META_REPLACE, NULL, value); \
}

// Get an integer value from the command-line or recipe, and add it to the arguments
#define VALUE_ARG_RECIPE_INT(ARGNAME, RECIPENAME, TYPE, UNSET) { \
    ps##TYPE value = psMetadataLookup##TYPE(NULL, config->arguments, ARGNAME); \
    if (value == UNSET) { \
        bool mdok; \
        value = psMetadataLookup##TYPE(&mdok, recipe, RECIPENAME); \
        if (!mdok) { \
            psError(PPSTACK_ERR_CONFIG, true, "Unable to find %s in recipe %s", \
                RECIPENAME, PPSTACK_RECIPE); \
            goto ERROR; \
        } \
    } \
    psMetadataAdd##TYPE(recipe, PS_LIST_TAIL, RECIPENAME, PS_META_REPLACE, NULL, value); \
}

// Get a mask value from the command-line or recipe, and add it to the arguments
#define VALUE_ARG_RECIPE_MASK(ARGNAME, RECIPENAME) { \
    bool mdok; \
    const char *name = psMetadataLookupStr(&mdok, config->arguments, ARGNAME); \
    if (!mdok || !name || strlen(name) == 0) { \
        name = psMetadataLookupStr(NULL, recipe, RECIPENAME); \
        if (!name) { \
            psError(PPSTACK_ERR_CONFIG, true, "Unable to find %s in recipe %s", \
                    RECIPENAME, PPSTACK_RECIPE);                        \
            goto ERROR; \
        } \
    } \
    psImageMaskType value = pmConfigMaskGet(name, config); \
    psMetadataAddImageMask(recipe, PS_LIST_TAIL, RECIPENAME, PS_META_REPLACE, NULL, value); \
}

// Get a string value from the command-line and add it to the target
static bool valueArgStr(psMetadata *arguments, // Command-line arguments
                        const char *argName, // Argument name in the command-line arguments
                        const char *mdName, // Name for value in the metadata
                        psMetadata *target // Target metadata to which to add value
                        )
{
    psString value = psMetadataLookupStr(NULL, arguments, argName); // Value of interest
    if (value && strlen(value) > 0) {
        return psMetadataAddStr(target, PS_LIST_TAIL, mdName, PS_META_REPLACE, NULL, value);
    }
    return false;
}

// Get a string value from the command-line or recipe and add it to the target
static bool valueArgRecipeStr(psMetadata *arguments, // Command-line arguments
                              psMetadata *recipe, // Recipe
                              const char *argName, // Argument name in the command-line arguments
                              const char *mdName, // Name for value in the metadata and recipe
                              psMetadata *target // Target metadata to which to add value
                              )
{
    psString value = psMetadataLookupStr(NULL, arguments, argName); // Value of interest
    if (!value) {
        value = psMetadataLookupStr(NULL, recipe, mdName);
        if (!value) {
            psError(PPSTACK_ERR_CONFIG, true, "Unable to find %s in recipe %s",
                    mdName, PPSTACK_RECIPE);
            return false;
        }
    }
    return psMetadataAddStr(target, PS_LIST_TAIL, mdName, PS_META_REPLACE, NULL, value);
}

bool ppStackArgumentsSetup(int argc, char *argv[], pmConfig *config)
{
    assert(config);

    // generic arguments (version, dumpconfig)
    PS_ARGUMENTS_GENERIC( ppStack, config, argc, argv );

    // thread arguments
    PS_ARGUMENTS_THREADS( ppStack, config, argc, argv )

    {
        int argNum = psArgumentGet(argc, argv, "-debug"); // Debugging argument number
        if (argNum) {
            psArgumentRemove(argNum, &argc, argv);
            pmSubtractionRegions(true);
        }
    }

    // This capability makes things much faster when debugging
    bool debugStack = false;            // Read old convolutions to debug the stacking?
    int argNum = psArgumentGet(argc, argv, "-debug-stack"); // Argument number
    if (argNum > 0) {
        debugStack = true;
        psArgumentRemove(argNum, &argc, argv);
    }

    pmConfigFileSetsMD(config->arguments, &argc, argv, "PPSTACK.SOURCES", "-sources", NULL);

    if ((argNum = psArgumentGet(argc, argv, "-dumpconfig"))) {
        psArgumentRemove(argNum, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "DUMP_CONFIG", PS_META_REPLACE,
                         "Filename for configuration dump", argv[argNum]);
        psArgumentRemove(argNum, &argc, argv);
    }

    psMetadata *arguments = config->arguments; // Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stamps", 0, "Stamps file with x,y,flux per line", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stats", 0, "Statistics file", NULL);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-combine-iter", 0, "Number of rejection iterations per input", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-combine-rej", 0,
                     "Combination rejection thresold (sigma)", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-combine-sys", 0,
                     "Relative systematic error in combination", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-combine-discard", 0,
                     "Discard fraction for Olympic weighted mean", NAN);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask-val", 0, "Mask value of input bad pixels", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask-bad", 0, "Mask value to give bad pixels", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask-poor", 0, "Mask value to give poor pixels", NULL);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-threshold-mask", 0, "Threshold for mask deconvolution", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-poor-frac", 0, "Fraction of variance for poor pixels", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-deconv-limit", 0, "Maximum deconvolution fraction limit", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-image-rej", 0,
                     "Pixel rejection fraction threshold for rejecting entire image", NAN);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-rows", 0, "Rows to read at once", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-photometry", 0, "Do photometry on stacked image?", false);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-psf-instances", 0,
                     "Number of instances for PSF generation", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-psf-radius", 0, "Radius for PSF generation", NAN);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-psf-model", 0, "Model name for PSF generation", NULL);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-psf-order", 0, "Spatial order for PSF generation", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-variance", 0, "Use variance for rejection?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-safe", 0,
                      "Play safe with small numbers of pixels to combine?", false);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-radius", 0, "Radius (pixels) for matching sources", NAN);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-zp-iter-1", 0, "Maximum iterations for zero point; pass 1", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-zp-iter-2", 0, "Maximum iterations for zero point; pass 2", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-tol", 0, "Tolerance for zero point iterations", NAN);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-zp-trans-iter", 0, "Iterations for transparency determination", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-trans-rej", 0, "Rejection threshold for transparency determination", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-trans-thresh", 0, "Threshold for transparency determination", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-star-rej-1", 0, "Rejection threshold for stars; pass 1", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-star-rej-2", 0, "Rejection threshold for stars; pass 2", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-star-limit", 0, "Limit on star rejection fraction for successful iteration", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-star-sys-1", 0, "Estimated systematic error; pass 1", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-zp-star-sys-2", 0, "Estimated systematic error; pass 2", NAN);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-temp-dir", 0, "Directory for temporary images", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-temp-image", 0, "Suffix for temporary images", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-temp-mask", 0, "Suffix for temporary masks", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-temp-variance", 0, "Suffix for temporary variance maps", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-temp-delete", 0, "Delete temporary files on completion?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-visual", 0, "visualisation", false);

    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stack_id",   0, "stack ID",        NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID",      NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-tess_id",    0, "tessellation ID", NULL);

    if (argc == 1) {
        usage(argv[0], arguments, config);
    }

    // stack-type : used to define the stack for PSPS (added as metadata to header key STK_TYPE)
    if ((argNum = psArgumentGet (argc, argv, "-stack-type"))) {
	if (argc <= argNum+1) {
	    psErrorStackPrint(stderr, "Expected to see an argument for -stack-type");
	    exit(PS_EXIT_CONFIG_ERROR);
	}
        psArgumentRemove (argNum, &argc, argv);
	if (strcasecmp(argv[argNum], "NIGHTLY_STACK") && strcasecmp(argv[argNum], "DEEP_STACK") && strcasecmp(argv[argNum], "IQ_STACK")) {
	    psErrorStackPrint(stderr, "Invalid option for -stack-type %s (must be one of NIGHTLY_STACK, DEEP_STACK, IQ_STACK)", argv[argNum]);
	    exit(PS_EXIT_CONFIG_ERROR);
	}
	psMetadataAddStr (arguments, PS_LIST_TAIL, "STACK_TYPE", PS_META_REPLACE, "Stack Type", argv[argNum]);
        psArgumentRemove (argNum, &argc, argv);
    } else { 
	psMetadataAddStr (arguments, PS_LIST_TAIL, "STACK_TYPE", PS_META_REPLACE, "Stack Type", "DEEP_STACK");
    }

    if ((argNum = psArgumentGet(argc, argv, "-input"))) {
        psArgumentRemove(argNum, &argc, argv);
        if (argNum >= argc) {
            usage(argv[0], arguments, config);
        }

        unsigned int numBad = 0;                     // Number of bad lines
        psMetadata *inputs = psMetadataConfigRead(NULL, &numBad, argv[argNum], false); // Input file info
        if (!inputs || numBad > 0) {
            psError(PPSTACK_ERR_ARGUMENTS, false, "Unable to cleanly read MDC file with inputs.");
            return false;
        }
        psMetadataAddMetadata(arguments, PS_LIST_TAIL, "INPUTS", 0, "Metadata with input details", inputs);
        psFree(inputs);

        psArgumentRemove(argNum, &argc, argv);
    }

    if (!psArgumentParse(arguments, &argc, argv) || argc != 2) {
        usage(argv[0], arguments, config);
    }

    psMetadataAddStr(arguments, PS_LIST_TAIL, "OUTPUT", 0, "Root name of the output image list", argv[1]);

    const char *stampsName = psMetadataLookupStr(NULL, arguments, "-stamps"); // Name of stamps file
    psMetadataAddStr(arguments, PS_LIST_TAIL, "STAMPS", 0, "Stamps file", stampsName);

    valueArgStr(arguments, "-stats", "STATS", arguments);

    psMetadataAddBool(arguments, PS_LIST_TAIL, "PPSTACK.DEBUG.STACK", 0,
                      "Read old convolved images to debug stack?", debugStack);

    return true;
}

bool ppStackArgumentsParse(pmConfig *config)
{
    assert(config);

    psMetadata *arguments = config->arguments; // Command-line arguments

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // Recipe
    if (!recipe) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find recipe %s", PPSTACK_RECIPE);
        goto ERROR;
    }

    VALUE_ARG_RECIPE_FLOAT("-combine-iter",    "COMBINE.ITER",    F32);
    VALUE_ARG_RECIPE_FLOAT("-combine-rej",     "COMBINE.REJ",     F32);
    VALUE_ARG_RECIPE_FLOAT("-combine-sys",     "COMBINE.SYS",     F32);
    VALUE_ARG_RECIPE_FLOAT("-combine-discard", "COMBINE.DISCARD", F32);
    VALUE_ARG_RECIPE_FLOAT("-threshold-mask",  "THRESHOLD.MASK",  F32);
    VALUE_ARG_RECIPE_FLOAT("-image-rej",       "IMAGE.REJ",       F32);
    VALUE_ARG_RECIPE_FLOAT("-deconv-limit",    "DECONV.LIMIT",    F32);
    VALUE_ARG_RECIPE_INT("-rows",              "ROWS",            S32, 0);
    VALUE_ARG_RECIPE_FLOAT("-poor-frac",       "POOR.FRACTION",   F32);

    valueArgRecipeStr(arguments, recipe, "-mask-val",  "MASK.VAL",   recipe);
    valueArgRecipeStr(arguments, recipe, "-mask-bad",  "MASK.BAD",  recipe);
    valueArgRecipeStr(arguments, recipe, "-mask-poor", "MASK.POOR", recipe);

    VALUE_ARG_RECIPE_FLOAT("-zp-radius",       "ZP.RADIUS",       F32);
    VALUE_ARG_RECIPE_INT("-zp-iter-1",         "ZP.ITER.1",       S32, 0);
    VALUE_ARG_RECIPE_INT("-zp-iter-2",         "ZP.ITER.2",       S32, 0);
    VALUE_ARG_RECIPE_FLOAT("-zp-tol",          "ZP.TOL",          F32);
    VALUE_ARG_RECIPE_INT("-zp-trans-iter",     "ZP.TRANS.ITER",   S32, 0);
    VALUE_ARG_RECIPE_FLOAT("-zp-trans-rej",    "ZP.TRANS.REJ",    F32);
    VALUE_ARG_RECIPE_FLOAT("-zp-trans-thresh", "ZP.TRANS.THRESH", F32);
    VALUE_ARG_RECIPE_FLOAT("-zp-star-rej-1",   "ZP.STAR.REJ.1",   F32);
    VALUE_ARG_RECIPE_FLOAT("-zp-star-rej-2",   "ZP.STAR.REJ.2",   F32);
    VALUE_ARG_RECIPE_FLOAT("-zp-star-limit",   "ZP.STAR.LIMIT",   F32);
    VALUE_ARG_RECIPE_FLOAT("-zp-star-sys-1",   "ZP.STAR.SYS.1",   F32);
    VALUE_ARG_RECIPE_FLOAT("-zp-star-sys-2",   "ZP.STAR.SYS.2",   F32);

    VALUE_ARG_RECIPE_INT("-psf-instances", "PSF.INSTANCES", S32, 0);
    VALUE_ARG_RECIPE_FLOAT("-psf-radius",  "PSF.RADIUS",    F32);
    VALUE_ARG_RECIPE_INT("-psf-order",     "PSF.ORDER",     S32, 0);
    valueArgRecipeStr(arguments, recipe, "-psf-model", "PSF.MODEL", recipe);

    if (psMetadataLookupBool(NULL, arguments, "-visual")) {
        pmVisualSetVisual(true);
    }

    if (psMetadataLookupBool(NULL, arguments, "-photometry") ||
        psMetadataLookupBool(NULL, recipe, "PHOTOMETRY")) {
        psMetadataAddBool(recipe, PS_LIST_TAIL, "PHOTOMETRY", PS_META_REPLACE,
                          "Do photometry on stacked image?", true);
    }

    if (psMetadataLookupBool(NULL, arguments, "-variance") ||
        psMetadataLookupBool(NULL, recipe, "VARIANCE")) {
        psMetadataAddBool(arguments, PS_LIST_TAIL, "VARIANCE", 0, "Use variance for rejection?", true);
    }

    if (psMetadataLookupBool(NULL, arguments, "-safe") ||
        psMetadataLookupBool(NULL, recipe, "SAFE")) {
        psMetadataAddBool(arguments, PS_LIST_TAIL, "SAFE", 0,
                          "Play safe with small number of pixels to combine?", true);
    }

    valueArgRecipeStr(arguments, recipe, "-temp-image",    "TEMP.IMAGE",  recipe);
    valueArgRecipeStr(arguments, recipe, "-temp-mask",     "TEMP.MASK",   recipe);
    valueArgRecipeStr(arguments, recipe, "-temp-variance", "TEMP.VARIANCE", recipe);

    if (psMetadataLookupBool(NULL, arguments, "-temp-delete") ||
        psMetadataLookupBool(NULL, recipe, "TEMP.DELETE")) {
        psMetadataAddBool(arguments, PS_LIST_TAIL, "TEMP.DELETE", 0,
                          "Delete temporary files on completion?", true);
    }

    
    psTrace("ppStack", 1, "Done parsing arguments\n");

    pmConfigCamerasCull(config, NULL);
    pmConfigRecipesCull(config, "PPSTACK,PPSUB,PPSTATS,PSPHOT,PSASTRO,MASKS,JPEG");

    return true;

ERROR:
    return false;
}

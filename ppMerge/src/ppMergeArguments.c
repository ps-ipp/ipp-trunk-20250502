/** @file ppMergeArguments.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.17 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppMerge.h"

/**
 * Print usage information and die
 */
static void usage(const char *program,  ///< Name of the program
                  psMetadata *arguments ///< Command-line arguments
    )
{
    fprintf(stderr, "\nPan-STARRS Detrend Merging\n\n");
    fprintf(stderr, "Usage: %s INPUT.mdc OUTPUT_ROOT\n"
            "where INPUTS.mdc contains various METADATAs, each with:\n"
            "\tIMAGE(STR):     Image filename\n"
            "\tMASK(STR):      Mask filename\n"
            "\tVARIANCE(STR)   Variance map filename\n"
            "where MASK and VARIANCE are optional.",
            program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
}

/**
 * Get a float-point value from the command-line or recipe, and add it to the arguments
 */
#define VALUE_ARG_RECIPE_FLOAT(ARGNAME, RECIPENAME, TYPE) { \
    ps##TYPE value = psMetadataLookup##TYPE(NULL, arguments, ARGNAME); \
    if (isnan(value)) { \
        bool mdok; \
        value = psMetadataLookup##TYPE(&mdok, recipe, RECIPENAME); \
        if (!mdok) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to find %s in recipe %s", \
                RECIPENAME, PPMERGE_RECIPE); \
            goto ERROR; \
        } \
    } \
    psMetadataAdd##TYPE(config->arguments, PS_LIST_TAIL, RECIPENAME, 0, NULL, value); \
}

/**
 * Get an integer value from the command-line or recipe, and add it to the arguments
 */
#define VALUE_ARG_RECIPE_INT(ARGNAME, RECIPENAME, TYPE, UNSET) { \
    ps##TYPE value = psMetadataLookup##TYPE(NULL, arguments, ARGNAME); \
    if (value == UNSET) { \
        bool mdok; \
        value = psMetadataLookup##TYPE(&mdok, recipe, RECIPENAME); \
        if (!mdok) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to find %s in recipe %s", \
                RECIPENAME, PPMERGE_RECIPE); \
            goto ERROR; \
        } \
    } \
    psMetadataAdd##TYPE(config->arguments, PS_LIST_TAIL, RECIPENAME, 0, NULL, value); \
}

/**
 * Get a boolean from the command-line or recipe, and add it to the arguments if either is set
 */
#define VALUE_ARG_RECIPE_BOOL(ARGNAME, RECIPENAME) { \
    bool value = (psMetadataLookupBool(NULL, arguments, ARGNAME) || \
                  psMetadataLookupBool(NULL, recipe, RECIPENAME)); \
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, RECIPENAME, 0, NULL, value); \
}

/**
 * Get a statistic name from the command-line or recipe, and add the enum to the arguments
 */
#define VALUE_ARG_RECIPE_STAT(ARGNAME, RECIPENAME) { \
    const char *stat = psMetadataLookupStr(NULL, arguments, ARGNAME); \
    if (!stat) { \
        stat = psMetadataLookupStr(NULL, recipe, RECIPENAME); \
        if (!stat) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find %s in recipe %s", \
                    RECIPENAME, PPMERGE_RECIPE); \
            goto ERROR; \
        } \
    } \
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, RECIPENAME, 0, NULL, psStatsOptionFromString(stat)); \
}

/**
 * Get a string from the command-line or recipe, and add to the arguments
 */
#define VALUE_ARG_RECIPE_STR(ARGNAME, RECIPENAME) { \
    const char *str = psMetadataLookupStr(NULL, arguments, ARGNAME); \
    if (!str) { \
        str = psMetadataLookupStr(NULL, recipe, RECIPENAME); \
        if (!str) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find %s in recipe %s", \
                    RECIPENAME, PPMERGE_RECIPE); \
            goto ERROR; \
        } \
    } \
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, RECIPENAME, 0, NULL, str); \
}

/**
 * Get a string from the command-line or recipe, and add the appropriate mask value to the arguments
 */
#define VALUE_ARG_RECIPE_MASK(ARGNAME, RECIPENAME) { \
    const char *str = psMetadataLookupStr(NULL, arguments, ARGNAME); \
    if (!str) { \
        str = psMetadataLookupStr(NULL, recipe, RECIPENAME); \
        if (!str) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find %s in recipe %s", \
                    RECIPENAME, PPMERGE_RECIPE); \
            goto ERROR; \
        } \
    } \
    psImageMaskType mask = pmConfigMask(str, config); \
    psMetadataAddImageMask(config->arguments, PS_LIST_TAIL, RECIPENAME, 0, NULL, mask); \
}

/**
 * Get a string value from the command-line and add it to the target
 */
static bool valueArgStr(psMetadata *arguments, ///< Command-line arguments
                        const char *argName, ///< Argument name in the command-line arguments
                        const char *mdName, ///< Name for value in the metadata
                        psMetadata *target ///< Target metadata to which to add value
                        )
{
    psString value = psMetadataLookupStr(NULL, arguments, argName); ///< Value of interest
    if (value && strlen(value) > 0) {
        return psMetadataAddStr(target, PS_LIST_TAIL, mdName, 0, NULL, value);
    }
    return false;
}

bool ppMergeArguments(int argc, char *argv[], pmConfig *config)
{
    assert(config);

    psMetadata *arguments = psMetadataAlloc(); ///< Command-line arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-type", 0, "Type of calibration frame", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stats", 0, "MDC file to hold statistics ", NULL);
    // Standard combination parameters
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-rows",     0, "Rows to read per scan", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-sample",   0, "Sampling factor for background", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-iter",     0, "Number of rejection iterations", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-rej",      0, "Rejection threshold (sigma)", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-fraclow",  0, "Fraction of low pixels to discard", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-frachigh", 0, "Fraction of low pixels to discard", NAN);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-nkeep",    0, "Minimum number of pixels in stack to keep", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-variances", 0, "Use image variances in combination?", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-use-masks", 0, "Use image masks in combination?", false);

    // XXX EAM : not clear this should be allowed on the command line.
    // psMetadataAddStr(arguments, PS_LIST_TAIL, "-maskval",  0, "Mask value for input data", NULL);

    psMetadataAddStr(arguments, PS_LIST_TAIL, "-combine",  0, "Statistic to use for combination", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mean",     0, "Statistic to use to measure the mean", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-stdev",    0, "Statistic to use to measure the stdev", NULL);

    /** Fringe construction parameters */
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-fringe-num",     0, "Number of fringe regions", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-fringe-size",    0, "Half-size of fringe regions", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-fringe-xsmooth", 0, "Number of smoothing regions in x", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-fringe-ysmooth", 0, "Number of smoothing regions in y", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-fringe-smooth", 0, "Smooth output image", false);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-fringe-smooth-sigma", 0, "Size of smoothing Gaussian", NAN);

    /** CTEMASK construction parameters */
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-cte-min", 0, "min allowed value for good CTE", NAN);

    /** Shutter construction parameters */
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-shutter-size", 0, "Size for shutter measurement regions", 0);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-shutter-iter", 0, "Number of iterations for shutter", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-shutter-rej",  0, "Rejection limit for shutter", NAN);

    /** Mask construction parameters */
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-mask-suspect-sigma",  0, "Threshold for suspect pixels (sigma)", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-mask-suspect-min",  0, "Threshold for suspect pixels (sigma)", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-mask-suspect-max",  0, "Threshold for suspect pixels (sigma)", NAN);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-mask-bad",      0, "Threshold for bad pixels (sigma)", NAN);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask-mode",     0, "Mode to identify bad pixels", NULL);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-mask-grow",     0, "Number of pixels to grow final mask", 0);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-mask-set-value",0, "Value to set for output mask pixels", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-mask-chip",    0, "Measure mask statistics by chip?", false);

    psMetadataAddBool(arguments, PS_LIST_TAIL, "-mask-smooth-suspect",  0, "smooth suspect pixels?", false);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-mask-smooth-scale",  0, "smoothing sigma for suspect pixels", NAN);

    // chip & cell selections are used to limit chips to be processed
    int argnum;
    if ((argnum = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (argnum, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING, "", argv[argnum]);
        psArgumentRemove (argnum, &argc, argv);
    }
    if ((argnum = psArgumentGet (argc, argv, "-cell"))) {
        psArgumentRemove (argnum, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CELL_SELECTIONS", PS_DATA_STRING, "", argv[argnum]);
        psArgumentRemove (argnum, &argc, argv);
    }

    if ((argnum = psArgumentGet (argc, argv, "-visual"))) {
        psArgumentRemove (argnum, &argc, argv);
	pmVisualSetVisual(true);
    }

    // Number of threads
    if ((argnum = psArgumentGet(argc, argv, "-threads"))) {
        psArgumentRemove(argnum, &argc, argv);
        int nThreads = atoi(argv[argnum]);
        psMetadataAddS32(config->arguments, PS_LIST_TAIL, "NTHREADS", 0, "number of warp threads", nThreads);
        psArgumentRemove(argnum, &argc, argv);

        // create the thread pool with number of desired threads, supplying our thread launcher function
        // XXX need to determine the number of threads from the config data
        psThreadPoolInit (nThreads);
    }
    ppMergeSetThreads();

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv) || argc != 3) {
        usage(argv[0], arguments);
        goto ERROR;
    }

    unsigned int numBad = 0;                     ///< Number of bad lines
    psMetadata *inputs = psMetadataConfigRead(NULL, &numBad, argv[1], false); ///< Information about inputs
    if (!inputs || numBad > 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to cleanly read MDC file with inputs.");
        goto ERROR;
    }
    psMetadataAddMetadata(config->arguments, PS_LIST_TAIL, "INPUTS", 0,
                          "Metadata with input details", inputs);
    psFree(inputs);
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0,
                     "Root name of the output image list", argv[2]);

    valueArgStr(arguments, "-stats", "STATS.NAME", config->arguments);


    // Set the type of calibration frame
    const char *typeStr = psMetadataLookupStr(NULL, arguments, "-type"); ///< Type of calibration
    if (!typeStr || strlen(typeStr) <= 0) {
      psError(PS_ERR_UNKNOWN, false, "No -type specified.");
      goto ERROR;
    }
    ppMergeType type = PPMERGE_TYPE_UNKNOWN; ///< Enumerated type for frame type
    if (strcasecmp(typeStr, "BIAS") == 0) {
      type = PPMERGE_TYPE_BIAS;
      goto VALID;
    }
    if (strcasecmp(typeStr, "DARK") == 0 ||
        strcasecmp(typeStr, "DARKTEST") == 0 ||
        strcasecmp(typeStr, "DARK_PREMASK") == 0) {
      type = PPMERGE_TYPE_DARK;
      goto VALID;
    }
    if (!strcasecmp(typeStr, "FLAT") ||
        !strcasecmp(typeStr, "FLATTEST") ||
        !strcasecmp(typeStr, "DOMEFLAT") ||
        !strcasecmp(typeStr, "SKYFLAT") ||
        !strcasecmp(typeStr, "FLAT_RAW") ||
        !strcasecmp(typeStr, "SKYFLAT_RAW") ||
	!strcasecmp(typeStr, "SKYFLATTEST_RAW") ||
        !strcasecmp(typeStr, "DOMEFLAT_RAW") ||
        !strcasecmp(typeStr, "FLAT_PREMASK") ||
        !strcasecmp(typeStr, "SKYFLAT_PREMASK") ||
        !strcasecmp(typeStr, "DOMEFLAT_PREMASK")) {
      type = PPMERGE_TYPE_FLAT;
      goto VALID;
    }
    if (strcasecmp(typeStr, "FRINGE") == 0) {
      type = PPMERGE_TYPE_FRINGE;
      goto VALID;
    }
    if (strcasecmp(typeStr, "SHUTTER") == 0) {
      type = PPMERGE_TYPE_SHUTTER;
      goto VALID;
    }
    if (strcasecmp(typeStr, "CTEMASK") == 0) {
      type = PPMERGE_TYPE_CTEMASK;
      goto VALID;
    }
    if (strcasecmp(typeStr, "MASK") == 0 ||
        strcasecmp(typeStr, "DARKMASK") == 0 ||
        strcasecmp(typeStr, "FLATMASK") == 0) {
      type = PPMERGE_TYPE_MASK;
      goto VALID;
    }
    if (strcasecmp(typeStr, "NOISEMAP") == 0) {
      type = PPMERGE_TYPE_NOISEMAP;
      goto VALID;
    }
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised image type: %s", typeStr);
    goto ERROR;

 VALID:
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "TYPE", 0, "Type of calibration frame", type);

    // Need to set the camera before the recipes can be read
    if (!ppMergeCamera(config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to setup cameras.");
        goto ERROR;
    }

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPMERGE_RECIPE); ///< Recipe for ppSim
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPMERGE_RECIPE);
        goto ERROR;
    }

    /** Standard combination parameters */
    VALUE_ARG_RECIPE_INT("-rows",       "ROWS",     S32, 0);
    VALUE_ARG_RECIPE_INT("-sample",     "SAMPLE",   S32, 0);
    VALUE_ARG_RECIPE_INT("-iter",       "ITER",     S32, 0);
    VALUE_ARG_RECIPE_FLOAT("-rej",      "REJ",      F32);
    VALUE_ARG_RECIPE_FLOAT("-fraclow",  "FRACLOW",  F32);
    VALUE_ARG_RECIPE_FLOAT("-frachigh", "FRACHIGH", F32);
    VALUE_ARG_RECIPE_INT("-nkeep",      "NKEEP",    S32, 0);
    VALUE_ARG_RECIPE_BOOL("-variances", "VARIANCES");
    VALUE_ARG_RECIPE_BOOL("-use-masks", "INPUTS.MASKS.USE");

    // XXX we do not supply this on the command line
    // VALUE_ARG_RECIPE_MASK("-maskval",   "MASKVAL");

    VALUE_ARG_RECIPE_STAT("-combine",   "COMBINE");
    VALUE_ARG_RECIPE_STAT("-mean",      "MEAN");
    VALUE_ARG_RECIPE_STAT("-stdev",     "STDEV");

    /** Fringe construction parameters */
    VALUE_ARG_RECIPE_INT("-fringe-num",     "FRINGE.NUM",     S32, 0);
    VALUE_ARG_RECIPE_INT("-fringe-size",    "FRINGE.SIZE",    S32, 0);
    VALUE_ARG_RECIPE_INT("-fringe-xsmooth", "FRINGE.XSMOOTH", S32, 0);
    VALUE_ARG_RECIPE_INT("-fringe-ysmooth", "FRINGE.YSMOOTH", S32, 0);
    VALUE_ARG_RECIPE_BOOL("-fringe-smooth", "FRINGE.SMOOTH");
    VALUE_ARG_RECIPE_FLOAT("-fringe-smooth-sigma", "FRINGE.SMOOTH.SIGMA", F32);

    /** CTEMASK construction parameters */
    VALUE_ARG_RECIPE_FLOAT("-cte-min",      "CTE.MIN",        F32);

    /** Shutter construction parameters */
    VALUE_ARG_RECIPE_INT("-shutter-size",  "SHUTTER.SIZE", S32, 0);

    /** Mask construction parameters */
    VALUE_ARG_RECIPE_FLOAT("-mask-suspect-sigma", "MASK.SUSPECT.SIGMA", F32);
    VALUE_ARG_RECIPE_FLOAT("-mask-suspect-min",   "MASK.SUSPECT.MIN", F32);
    VALUE_ARG_RECIPE_FLOAT("-mask-suspect-max",   "MASK.SUSPECT.MAX", F32);
    VALUE_ARG_RECIPE_STR("-mask-mode", "MASK.SUSPECT.MODE");
    VALUE_ARG_RECIPE_FLOAT("-mask-bad",     "MASK.BAD",     F32);
    VALUE_ARG_RECIPE_INT("-mask-grow",      "MASK.GROW",    S32, 0);
    VALUE_ARG_RECIPE_STR("-mask-set-value", "MASK.SET.VALUE");
    VALUE_ARG_RECIPE_BOOL("-mask-chip",     "MASK.CHIPSTATS");

    VALUE_ARG_RECIPE_BOOL("-mask-smooth-suspect", "MASK.SMOOTH.SUSPECT");
    VALUE_ARG_RECIPE_FLOAT("-mask-smooth-scale", "MASK.SMOOTH.SCALE", F32);

    const char *maskModeStr = psMetadataLookupStr(NULL, arguments, "-mask-mode"); ///< Mode to identify bad pix
    if (!maskModeStr) {
        maskModeStr = psMetadataLookupStr(NULL, recipe, "MASK.MODE");
        if (!maskModeStr) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "No mask mode specified in recipe.");
            goto ERROR;
        }
    }
    pmMaskIdentifyMode maskMode = pmMaskIdentifyModeFromString(maskModeStr);
    if (maskMode == PM_MASK_ID_NONE) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Invalid mask mode %s", maskModeStr);
        goto ERROR;
    }
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "MASK.MODE", 0, "Mode for mask identification",
                     maskMode);


    if (type == PPMERGE_TYPE_DARK) {
        psMetadata *ordinates = psMetadataLookupMetadata(NULL, recipe, "DARK.ORDINATES"); ///< Ordinates info
        psArray *translated = psArrayAllocEmpty(psListLength(ordinates->list)); ///< Translated version

        psMetadataIterator *iter = psMetadataIteratorAlloc(ordinates, PS_LIST_HEAD, NULL); ///< Iterator
        psMetadataItem *item;           ///< Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            int order = 0;              ///< Polynomial order
            bool scale = false;         ///< Scale values?
            float min = NAN, max = NAN; ///< Minimum and maximum values for scaling
	    char *rule = NULL;
            switch (item->type) {
              case PS_TYPE_S32:
                order = item->data.S32;
                break;
              case PS_DATA_METADATA:
                order = psMetadataLookupS32(NULL, item->data.md, "ORDER");
                bool mdok;                  ///< Status of MD lookup
                scale = psMetadataLookupBool(&mdok, item->data.md, "SCALE");
                min = psMetadataLookupF32(&mdok, item->data.md, "MIN");
                max = psMetadataLookupF32(&mdok, item->data.md, "MAX");
                rule = psMetadataLookupStr(&mdok, item->data.md, "RULE");
                break;
              default:
                psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                        "Type of DARK.ORDINATES entry %s (%x) is not METADATA or S32",
                        item->name, item->type);
                return false;
            }
            if (order <= 0) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "ORDER not positive (%d) for DARK.ORDINATES %s",
                        order, item->name);
                return false;
            }

            pmDarkOrdinate *ord = pmDarkOrdinateAlloc(item->name, order);
            ord->scale = scale;
            ord->min = min;
            ord->max = max;
	    ord->rule = psStringCopy(rule);
            psArrayAdd(translated, translated->n, ord);
            psFree(ord);
        }
        psFree(iter);

        psMetadataAddArray(config->arguments, PS_LIST_TAIL, "DARK.ORDINATES", 0,
                           "Ordinates to fit for dark", translated);
        psFree(translated);             ///< Drop reference

        psString darkNorm = psMetadataLookupStr(NULL, recipe, "DARK.NORM"); ///<Normalisation concept
        if (darkNorm && strcmp(darkNorm, "NONE") != 0) {
            psMetadataAddStr(config->arguments, PS_LIST_TAIL, "DARK.NORM", 0,
                             "Normalisation concept for dark", darkNorm);
        }
    }


#if 0
    // Add concepts for scale and zero
    // XXX These have never been used
    psMetadataItem *scaleItem = psMetadataItemAllocF32("PPMERGE.SCALE", "Scaling for ppMerge", NAN);
    psMetadataItem *zeroItem = psMetadataItemAllocF32("PPMERGE.ZERO", "Zero offset for ppMerge", NAN);
    pmConceptRegister(scaleItem, NULL, NULL, false, PM_FPA_LEVEL_CELL);
    pmConceptRegister(zeroItem, NULL, NULL, false, PM_FPA_LEVEL_CELL);
    psFree(scaleItem);
    psFree(zeroItem);
#endif

    psFree(arguments);
    return true;

ERROR:
    psFree(arguments);
    return false;
}


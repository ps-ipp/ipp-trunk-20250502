#include "ppStatsInternal.h"

// This file is for setting up the required inputs from the command-line

// Print usage information and die
static void usageAndDie(char *argv[],   // Command-line arguments: only need the first which is always present
                        pmConfig *config // Configuration
    )
{
    printf("Return headers, concepts and/or image statistics.\n\n"
           "Usage:\n"
           "\t%s INPUT.fits [OUTPUT_NAME]\n"
           "\n", argv[0]);
    psArgumentHelp(config->arguments);
    psFree(config);
    psLibFinalize();
    pmConceptsDone();
    pmConfigDone();
    exit(EXIT_FAILURE);
}

// Generate a list from the arguments
static void listFromArguments(psMetadata *arguments, // Arguments to parse
                              const char *name, // Name of the item, for error message
                              const char *flag, // The flag for the argument of interest
                              psList *target // The target list
    )
{
    psString regex = NULL;              // Regular expression for the flag
    psStringAppend(&regex, "^%s$", flag);
    psMetadataIterator *iterator = psMetadataIteratorAlloc(arguments, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iterator))) {
        if (item->type != PS_DATA_STRING) {
            psLogMsg(__func__, PS_LOG_WARN, "%s name is not of type STRING (%x) --- ignored.\n",
                     name, item->type);
            continue;
        }
        if (item->data.V && strlen(item->data.V) > 0) {
            psListAdd(target, PS_LIST_TAIL, item->data.V);
        }
    }
    psFree(iterator);

    return;
}

// Set the statistics option; for arguments
static inline void statsOptionArguments(psMetadata *arguments, // Arguments to parse
                                        const char *name, // Name for option
                                        ppStatsData *data, // Configuration data
                                        psStatsOptions option // Option to check for
    )
{
    if (psMetadataLookupBool(NULL, arguments, name)) {
        data->stats->options |= option;
        data->doStats = true;
    }
    return;
}

// Print out what we're going to do, from the list
static void checkList(psList *list,     // List
                      const char *name  // Name of list
    )
{
    psString value;
    psListIterator *iterator = psListIteratorAlloc(list, PS_LIST_HEAD, false);
    while ((value = psListGetAndIncrement(iterator))) {
        printf("%s: %s\n", name, value);
    }
    psFree(iterator);
    return;
}


ppStatsData *ppStatsSetupFromArgs(int *argc, char *argv[], // Command-line arguments
                                  pmConfig *config // Configuration
    )
{
    // Setup and parse command-line arguments
    psMetadata *arguments = config->arguments; // Arguments
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-chip", PS_META_DUPLICATE_OK, "Chip to inspect", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-cell", PS_META_DUPLICATE_OK, "Cell to inspect", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-header", PS_META_DUPLICATE_OK, "Header to look up", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-concept", PS_META_DUPLICATE_OK, "Concept to look up", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-summary", PS_META_DUPLICATE_OK, "Summary statistic to calculate", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-mean", 0, "Calculate sample mean", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-stdev", 0, "Calculate sample standard deviation", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-median", 0, "Calculate sample median", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-quartile", 0, "Calculate sample quartiles", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-skewness", 0, "Calculate sample skewness", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-kurtosis", 0, "Calculate sample kurtosis", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-robust-median", 0, "Calculate robust median", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-robust-stdev", 0, "Calculate robust standard deviation", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-robust-quartile", 0, "Calculate robust quartile range", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-fitted-mean", 0, "Calculate fitted mean", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-fitted-stdev", 0, "Calculate fitted standard deviation", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-clipped-mean", 0, "Calculate clipped median", false);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-clipped-stdev", 0, "Calculate clipped standard deviation", false);
    psMetadataAddS32(arguments, PS_LIST_TAIL, "-iter", 0, "Clipping iterations", 0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-rej", 0, "Clipping level", 0.0);
    psMetadataAddF32(arguments, PS_LIST_TAIL, "-sample", 0, "Sampling fraction", 0.0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-level", 0, "File level", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-format", 0, "Show File format", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-show-camera", 0, "Show Camera name", 0);

    if (*argc == 1) {
        // No command-line arguments: print the help
        usageAndDie(argv, config);
    }
    if (!psArgumentParse(arguments, argc, argv) ||
        (*argc != 2 && *argc != 3)) {
        printf("Unable to parse command-line arguments.\n\n");
        usageAndDie(argv, config);
    }

    // Parse the command-line options
    ppStatsData *data = ppStatsDataAlloc(); // The data
    const char *inName = argv[1]; // Input file name
    psArgumentRemove(1, argc, argv);

    listFromArguments(arguments, "Chip", "-chip", data->chips);
    listFromArguments(arguments, "Cell", "-cell", data->cells);
    listFromArguments(arguments, "Header", "-header", data->headers);
    listFromArguments(arguments, "Concept", "-concept", data->concepts);
    listFromArguments(arguments, "Summary", "-summary", data->summary);

    // Set the statistics options
    statsOptionArguments(arguments, "-mean", data,     PS_STAT_SAMPLE_MEAN);
    statsOptionArguments(arguments, "-stdev", data,    PS_STAT_SAMPLE_STDEV);
    statsOptionArguments(arguments, "-median", data,   PS_STAT_SAMPLE_MEDIAN);
    statsOptionArguments(arguments, "-quartile", data, PS_STAT_SAMPLE_QUARTILE);
    statsOptionArguments(arguments, "-skewness", data, PS_STAT_SAMPLE_SKEWNESS);
    statsOptionArguments(arguments, "-kurtosis", data, PS_STAT_SAMPLE_KURTOSIS);
    statsOptionArguments(arguments, "-robust-median", data,   PS_STAT_ROBUST_MEDIAN);
    statsOptionArguments(arguments, "-robust-stdev", data,    PS_STAT_ROBUST_STDEV);
    statsOptionArguments(arguments, "-robust-quartile", data, PS_STAT_ROBUST_QUARTILE);
    statsOptionArguments(arguments, "-fitted-mean", data,   PS_STAT_FITTED_MEAN);
    statsOptionArguments(arguments, "-fitted-stdev", data,  PS_STAT_FITTED_STDEV);
    statsOptionArguments(arguments, "-clipped-mean", data,  PS_STAT_CLIPPED_MEAN);
    statsOptionArguments(arguments, "-clipped-stdev", data, PS_STAT_CLIPPED_STDEV);
    data->stats->clipSigma = psMetadataLookupF32(NULL, arguments, "-rej");
    data->stats->clipIter = psMetadataLookupS32(NULL, arguments, "-iter");
    data->sample = psMetadataLookupF32(NULL, arguments, "-sample");
    data->fileLevel = psMetadataLookupBool(NULL, arguments, "-level");
    data->showFormat = psMetadataLookupBool(NULL, arguments, "-format");
    data->showCamera = psMetadataLookupBool(NULL, arguments, "-show-camera");
    data->fileView = NULL;

    // Open the input file, determine the camera
    int result = PS_EXIT_UNKNOWN_ERROR;
    {
        psString resolved = pmConfigConvertFilename(inName, config, false, false); // Resolved filename
        data->fits = psFitsOpen(resolved, "r");
        if (!data->fits) {
            psError(PS_ERR_IO, false, "Unable to open input file %s\n", resolved);
            psFree(resolved);
            result = PS_EXIT_DATA_ERROR;
            goto die;
        }
        psFree(resolved);

        psMetadata *header = psFitsReadHeader(NULL, data->fits); // The FITS (primary) header
        psMetadata *format = pmConfigCameraFormatFromHeader(NULL, NULL, NULL, config, header, true);
        if (!format) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine camera format for %s\n", inName);
            psFree(header);
            result = PS_EXIT_DATA_ERROR;
            goto die;
        }

        // need to handle optional alternate EXTWORD value
        psMetadata *fileMenu = psMetadataLookupMetadata (NULL, format, "FILE");
        if (!fileMenu) {
            psError (PS_ERR_IO, true, "FILE METADATA missing from camera format\n");
            return false;
        }
        bool status = false;
        char *extword = psMetadataLookupStr (&status, fileMenu, "EXTWORD");
        if (status) {
            psFitsSetExtnameWord (data->fits, extword);
        }

        data->fpa = pmFPAConstruct(config->camera, config->cameraName);
        if (!data->fpa) {
            psError(PS_ERR_UNKNOWN, false, "Unable to construct FPA for %s\n", resolved);
            psFree(header);
            psFree(format);
            result = PS_EXIT_CONFIG_ERROR;
            goto die;
        }
        data->fileView = pmFPAAddSourceFromHeader(data->fpa, header, format);
        psFree(header);
        psFree(format);
        if (!data->fileView) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add input file %s to FPA.\n", inName);
            result = PS_EXIT_CONFIG_ERROR;
            goto die;
        }
    }

    // Get the rest from the recipe
    if (!ppStatsSetupFromRecipe(data, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read ppStats options from recipe.");
        psFree(data);
        return NULL;
    }

    // Print out what we're going to do
    if (psTraceGetLevel("ppStats") > 9) {
        checkList(data->chips, "CHIP");
        checkList(data->cells, "CELL");
        checkList(data->headers, "HEADER");
        checkList(data->concepts, "CONCEPT");
    }

    return data;

    // Common path for error conditions: clean up and exit.
die:
    psErrorStackPrint(stderr, "Unable to set ppStats parameters from command-line arguments");
    psFree(config);
    psFree(data);
    pmConceptsDone();
    pmConfigDone();
    psLibFinalize();
    exit(result);
}

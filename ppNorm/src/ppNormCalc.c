#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

void helpAndDie(const char *programName)
{
    printf("Calculate normalisation for flat fields.\n\n"
           "Usage: %s [IN.mdc [OUT.mdc]]\n\n"
           "where IN.mdc is a metadata config file containing the background\n"
           "      value for each component of each exposure;\n"
           "and   OUT.mdc is a metadata config file containing the output\n"
           "      normalisation factors.\n"
           "\n", programName);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    psLibInit(NULL);
    pmConfig *config = pmConfigRead(&argc, argv, NULL);
    psFree(config);

    if (argc > 3 ||
        psArgumentGet(argc, argv, "-h") ||
        psArgumentGet(argc, argv, "-help") ||
        psArgumentGet(argc, argv, "--help") ||
        psArgumentGet(argc, argv, "-?")) {
        helpAndDie(argv[0]);
    }

    FILE *inFile = stdin;               // Input file stream
    FILE *outFile = stdout;             // Output file stream

    if (argc >= 2) {
        psString resolved = pmConfigConvertFilename(argv[1], config, false, false); // Resolved filename
        inFile = fopen(resolved, "r");
        if (!inFile) {
            psError(PS_ERR_IO, true, "Unable to open input file: %s\n\n", resolved);
            psFree(resolved);
            helpAndDie(argv[0]);
        }
        psFree(resolved);
    }
    if (argc == 3) {
        psString resolved = pmConfigConvertFilename(argv[2], config, true, true); // Resolved filename
        outFile = fopen(resolved, "w");
        if (!outFile) {
            psError(PS_ERR_IO, true, "Unable to open output file: %s\n\n", resolved);
            psFree(resolved);
            helpAndDie(argv[0]);
        }
        psFree(resolved);
    }

    psString inputMDC = psSlurpFile(inFile); // Input metadata config stuff
    if (argc >= 2) {
        fclose(inFile);
    }

    psU32 badLines = 0;                   // Number of bad lines
    psMetadata *exposures = psMetadataConfigParse(NULL, &badLines, inputMDC, false); // Exposure statistics
    if (badLines > 0) {
        psWarning("%d bad lines found when reading input\n", badLines);
    }

    // Get a list of all the components
    psMetadata *components = psMetadataAlloc(); // Components of the exposures
    psMetadataIterator *expIter = psMetadataIteratorAlloc(exposures, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *expItem;            // Item from iteration
    while ((expItem = psMetadataGetAndIncrement(expIter))) {
        if (expItem->type != PS_DATA_METADATA) {
            psLogMsg("ppNormCalc", PS_LOG_WARN,
                     "Metadata item %s is not of type METADATA --- ignored.\n",
                     expItem->name);
            continue;
        }

        // Inspect each component for this exposure; add it if we don't know about it
        psMetadata *comps = expItem->data.V; // The components
        psMetadataIterator *compsIter = psMetadataIteratorAlloc(comps, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *compsItem;      // Item from iteration
        while ((compsItem = psMetadataGetAndIncrement(compsIter))) {
            if (compsItem->type != PS_DATA_F32) {
                psLogMsg("ppNormCalc", PS_LOG_WARN,
                         "Component %s within exposure %s is not of type F32 --- ignored.\n",
                         compsItem->name, expItem->name);
                continue;
            }
            if (!psMetadataLookup(components, compsItem->name)) {
                psMetadataAddBool(components, PS_LIST_TAIL, compsItem->name, 0, NULL, true);
            }
        }
        psFree(compsIter);
    }

    // Convert to a matrix
    int numComps = psListLength(components->list); // Number of components
    int numExps = psListLength(exposures->list); // Number of exposures
    psImage *matrix = psImageAlloc(numComps, numExps, PS_TYPE_F32); // Matrix of backgrounds
    psMetadataIteratorSet(expIter, PS_LIST_HEAD);
    for (int expNum = 0; (expItem = psMetadataGetAndIncrement(expIter)); expNum++) {
        if (expItem->type != PS_DATA_METADATA) {
            continue;
        }
        psMetadata *comps = expItem->data.V; // The components
        psMetadataIterator *compsIter = psMetadataIteratorAlloc(components, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *compsItem;      // Item from iteration
        for (int compNum = 0; (compsItem = psMetadataGetAndIncrement(compsIter)); compNum++) {
            matrix->data.F32[expNum][compNum] = psMetadataLookupF32(NULL, comps, compsItem->name);
        }
        psFree(compsIter);
    }
    psFree(expIter);

    // Do the normalisation
    psVector *gains = psVectorAlloc(numComps, PS_TYPE_F32); // Vector of gains for each component
    psVectorInit(gains, 100.0);
    if (!pmFlatNormalize(NULL, &gains, matrix)) {
        psLogMsg("ppNormCalc", PS_LOG_ERROR, "Normalisation didn't converge.\n");
        exit(EXIT_FAILURE);
    }
    psFree(matrix);


    // Output a MDC format to stdout
    psMetadata *outputMD = psMetadataAlloc(); // Output metadata
    psMetadataIterator *compsIter = psMetadataIteratorAlloc(components, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *compsItem;          // Item from iteration
    for (int compNum = 0; (compsItem = psMetadataGetAndIncrement(compsIter)); compNum++) {
        psMetadataAddF32(outputMD, PS_LIST_TAIL, compsItem->name, 0, NULL, gains->data.F32[compNum]);
    }
    psFree(compsIter);
    psString outputString = psMetadataConfigFormat(outputMD);
    fprintf(outFile, "%s", outputString);
    psFree(outputString);
    psFree(outputMD);
    if (argc == 3) {
        fclose(outFile);
    }

    // Clean up
    psFree(gains);
    psFree(components);
    psFree(exposures);

    pmConfigDone();
    psLibFinalize();

    return EXIT_SUCCESS;
}

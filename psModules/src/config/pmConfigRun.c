#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmFPAfile.h"

#include "pmConfigRun.h"

// Get the nominated element of the configuration
static psMetadata *configElement(psMetadata *md, // Metadata with element
                                 const char *name, // Name of element
                                 const char *comment // Comment for element
                             )
{
    psAssert(md, "Require metadata");
    psAssert(name, "Require name of element");

    bool mdok;                          // Status of MD lookup
    psMetadata *elem = psMetadataLookupMetadata(&mdok, md, name); // Element of interest
    if (!elem) {
        elem = psMetadataAlloc();
        psMetadataAddMetadata(md, PS_LIST_HEAD, name, 0, comment, elem);
        psFree(elem);                   // Drop reference
    }
    return elem;
}

// Get the RUN information from the configuration
static psMetadata *configRun(pmConfig *config // Configuration
                             )
{
    psAssert(config, "Require configuration");
    return configElement(config->user, "RUN", "Run-time information");
}

// Add a file to a nominated metadata in the RUN information
static bool configRunFileAdd(pmConfig *config, // Configuration
                             const char *description, // Description of file
                             const char *name, // Name of file
                             const char *target // Name of metadata to which to add
                             )
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(description, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);
    PS_ASSERT_STRING_NON_EMPTY(target, false);

    psMetadata *run = configRun(config);// RUN information
    psAssert(run, "Require run-time information");
    psMetadata *files = configElement(run, target, "Filerules used during execution");
    psAssert(files, "Require list of files");

    psString regex = NULL;              // Regular expression for iteration
    psStringAppend(&regex, "^%s$", description);
    psMetadataIterator *iter = psMetadataIteratorAlloc(files, PS_LIST_HEAD, regex);
    psFree(regex);
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        psAssert(item->type == PS_DATA_STRING, "We only put STRING types here.");
        if (strcmp(item->data.str, name) == 0) {
            // It's already present
            psFree(iter);
            return true;
        }
    }
    psFree(iter);

    return psMetadataAddStr(files, PS_LIST_TAIL, description, PS_META_DUPLICATE_OK, NULL, name);
}

bool pmConfigRunFileAddRead(pmConfig *config, const pmFPAfile *file)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    return configRunFileAdd(config, file->name, file->origname ? file->origname : file->filename,
                            "FILES.INPUT");
}

bool pmConfigRunFilenameAddRead(pmConfig *config, const char *description, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(description, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    return configRunFileAdd(config, description, name, "FILES.INPUT");
}

bool pmConfigRunFileAddWrite(pmConfig *config, const pmFPAfile *file)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    return configRunFileAdd(config, file->name, file->origname ? file->origname : file->filename,
                            "FILES.OUTPUT");
}

bool pmConfigRunFilenameAddWrite(pmConfig *config, const char *description, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(description, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    return configRunFileAdd(config, description, name, "FILES.OUTPUT");
}

// Get an array of filenames from the nominated RUN information
static psArray *configRunFileGet(pmConfig *config, // Configuration
                                 const char *name, // Name of file
                                 const char *source // Source metadata for file
                                 )
{
    psMetadata *run = configRun(config);// RUN information
    psAssert(run, "Require run-time information");
    psMetadata *files = configElement(run, source, "Filerules used during execution");
    psAssert(files, "Require list of files");

    if (psListLength(files->list) == 0) {
        // Can't find anything
        return NULL;
    }

    psList *list = psListAlloc(NULL);   // List of file names

    psString regex = NULL;              // Regular expression for iteration
    psStringAppend(&regex, "^%s$", name);
    psMetadataIterator *iter = psMetadataIteratorAlloc(files, PS_LIST_HEAD, regex);
    psFree(regex);
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        psAssert(item->type == PS_DATA_STRING, "We only put STRING types here.");
        psListAdd(list, PS_LIST_TAIL, item->data.str);
    }
    psFree(iter);

    if (psListLength(list) == 0) {
        // Didn't find anything
        psFree(list);
        return NULL;
    }

    psArray *array = psListToArray(list); // Array of file names, to return
    psFree(list);

    return array;
}


psArray *pmConfigRunFileGet(pmConfig *config, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    // Try the input and output, in turn
    psArray *files = configRunFileGet(config, name, "FILES.INPUT"); // Files from RUN metadata
    if (!files) {
        files = configRunFileGet(config, name, "FILES.OUTPUT");
    }

    return files;
}


bool pmConfigRunCommand(pmConfig *config, int argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *run = configRun(config);// Run-time information
    psAssert(run, "Require run-time information");

    psString command = NULL;
    for (int i = 0; i < argc; i++) {
        psStringAppend(&command, "%s ", argv[i]);
    }

    psMetadataAddStr(run, PS_LIST_TAIL, "COMMAND", PS_META_REPLACE, "Command line", command);
    psFree(command);

    return true;
}


bool pmConfigRunSeed(pmConfig *config, psU64 seed)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *run = configRun(config);// Run-time information
    psAssert(run, "Require run-time information");

    if (!seed) {
        bool mdok;                          // Status of MD lookup
        seed = psMetadataLookupU64(&mdok, run, "SEED");
    }
    if (!seed) {
        bool mdok;                          // Status of MD lookup
        seed = psMetadataLookupU64(&mdok, config->user, "SEED");
    }

    // seed may still be zero by this point
    seed = psRandomSeed(seed);
    return psMetadataAddU64(run, PS_LIST_TAIL, "SEED", PS_META_REPLACE, "Random number generator seed", seed);
}


# include "ppStatsInternal.h"

void p_ppStatsGetMetadata(psMetadata *target, // Target for metadata
                          psMetadata *source, // Source for metadata
                          psList *list    // List containing keywords
    )
{
    assert(target);
    assert(list);
    if (!source) {
        // Nothing to get from!
        return;
    }

    psListIterator *iterator = psListIteratorAlloc(list, PS_LIST_HEAD, false); // Iterator
    psString name;                      // Name from iteration
    while ((name = psListGetAndIncrement(iterator))) {
        psMetadataItem *item = psMetadataLookup(source, name); // Item of interest, or NULL
        if (item) {
            psMetadataAddItem(target, item, PS_LIST_TAIL, PS_META_REPLACE);
        }
    }
    psFree(iterator);
    return;
}

void p_ppStatsGetAnalysis(psMetadata *target, // Output Target for metadata
                          psList *headers,    // List containing desired keywords
                          psMetadata *source, // Input Source for metadata
                          psList *list        // List containing analysis blocks
    )
{
    bool status;

    psListIterator *iterator = psListIteratorAlloc(list, PS_LIST_HEAD, false); // Iterator
    psString name;                      // Name from iteration
    while ((name = psListGetAndIncrement(iterator))) {
        psMetadata *folder = psMetadataLookupMetadata(&status, source, name); // Item of interest, or NULL
        if (folder) {
            p_ppStatsGetMetadata (target, folder, headers);
        }
    }
    psFree(iterator);
    return;
}

bool p_ppStatsDoThis(psList *toDoList,    // List of things to do
                     const char *this     // The name of "this"
    )
{
    if (psListLength(toDoList) == 0) {
        // No list --- do everything
        return true;
    }

    psListIterator *iterator = psListIteratorAlloc(toDoList, PS_LIST_HEAD, false); // Iterator
    psString test;                      // Test string, from iteration
    while ((test = psListGetAndIncrement(iterator))) {
        if (strcmp(this, test) == 0) {
            // It's in the list --- do it
            psFree(iterator);
            return true;
        }
    }
    psFree(iterator);
    // Couldn't find it --- don't do it
    return false;
}

void p_ppStatsAddToHierarchy(psMetadata *source, // Source to add
                             psMetadata *target, // Target to which to add
                             const char *name, // Name of source
                             const char *comment // Comment for source
    )
{
    if (psListLength(source->list) > 0 && !psMetadataLookup(target, name)) {
        psMetadataAdd(target, PS_LIST_TAIL, name, PS_DATA_METADATA | PS_META_REPLACE,
                      comment, source);
    }
    return;
}



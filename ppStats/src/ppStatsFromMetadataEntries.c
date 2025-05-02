# include "ppStatsInternal.h"

psArray *ppStatsFromMetadataEntries (psMetadata *recipe) {

    bool status = false;

    psMetadataItem *item = NULL;

    psArray *entries = psArrayAllocEmpty(16);

    // the recipe file should include a collection of psMetadata items with the name ENTRY
    
    // psMetadataPrint (stderr, recipe, 1);

    // loop over the items, selecting those with the name "ENTRY"
    psMetadataIterator *iter = psMetadataIteratorAlloc(recipe, PS_LIST_HEAD, NULL); // Iterator
    while ((item = psMetadataGetAndIncrement(iter))) {
	if (strcmp (item->name, "ENTRY")) continue;
	
	if (item->type != PS_DATA_METADATA) {
	    psError(PS_ERR_UNKNOWN, false, "ppStatsFromMetadata ENTRY not of type METADATA\n");
	    return NULL;
	}

	psMetadata *entryMD = item->data.md;

	ppStatsEntry *entry = ppStatsEntryAlloc ();

	entry->keyword = psMetadataLookupStr (&status, entryMD, "KEYWORD");
	if (!entry->keyword) {
	    psError(PS_ERR_UNKNOWN, true, "ppStatsFromMetadata recipe ENTRY missing KEYWORD value\n");
	    return NULL;
	}
	
	entry->statistic = psMetadataLookupStr (&status, entryMD, "STATISTIC");
	if (!entry->statistic) {
	    psError(PS_ERR_UNKNOWN, true, "ppStatsFromMetadata recipe ENTRY %s missing STATISTIC\n", entry->keyword);
	    return NULL;
	}

	entry->flag = psMetadataLookupStr (&status, entryMD, "FLAG");
	if (!entry->flag) {
	    psError(PS_ERR_UNKNOWN, true, "ppStatsFromMetadata recipe ENTRY %s missing FLAG\n", entry->keyword);
	    return NULL;
	}

	char *typename = psMetadataLookupStr (&status, entryMD, "TYPE");
	if (!typename) {
	    psError(PS_ERR_UNKNOWN, true, "ppStatsFromMetadata recipe ENTRY %s missing TYPE\n", entry->keyword);
	    return NULL;
	}

	entry->type = psDataTypeFromString (typename);
	if (!entry->type) {
	    psError(PS_ERR_UNKNOWN, true, "ppStatsFromMetadata recipe ENTRY %s has invalid TYPE %s\n", entry->keyword, typename);
	    return NULL;
	}
	
	// fprintf (stderr, "adding %s to array of %ld\n", entry->keyword, entries->n);
	psArrayAdd (entries, 16, entry);
	psFree (entry);
    }
    
    return entries;
}

void ppStatsEntryFree (ppStatsEntry *entry) {

    if (!entry) return;

    psFree (entry->keyword);
    psFree (entry->statistic);
    psFree (entry->flag);
    psFree (entry->value);
    psFree (entry->vector);
    return;
}

ppStatsEntry *ppStatsEntryAlloc () {

    ppStatsEntry *entry = (ppStatsEntry *) psAlloc (sizeof(ppStatsEntry));
    psMemSetDeallocator(entry, (psFreeFunc) ppStatsEntryFree);
    
    entry->keyword = NULL;
    entry->statistic = NULL;
    entry->flag = NULL;
    entry->value = NULL;
    entry->vector = NULL;
    return entry;
}

psDataType psDataTypeFromString (char *typename) {

    if (!strcmp (typename, "U8"))  return PS_DATA_U8;
    if (!strcmp (typename, "U16")) return PS_DATA_U16;
    if (!strcmp (typename, "U32")) return PS_DATA_U32;
    if (!strcmp (typename, "U64")) return PS_DATA_U64;

    if (!strcmp (typename, "S8"))  return PS_DATA_S8;
    if (!strcmp (typename, "S16")) return PS_DATA_S16;
    if (!strcmp (typename, "S32")) return PS_DATA_S32;
    if (!strcmp (typename, "S64")) return PS_DATA_S64;

    if (!strcmp (typename, "F32")) return PS_DATA_F32;
    if (!strcmp (typename, "F64")) return PS_DATA_F64;

    if (!strcmp (typename, "BOOL")) return PS_DATA_BOOL;
    if (!strcmp (typename, "STR")) return PS_DATA_STRING;
    if (!strcmp (typename, "TIME")) return PS_DATA_TIME;
    if (!strcmp (typename, "UTC")) return PS_DATA_TIME;
    if (!strcmp (typename, "TAI")) return PS_DATA_TIME;

    return 0;
}

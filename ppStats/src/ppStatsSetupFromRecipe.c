#include "ppStatsInternal.h"

// Strings in a recipe may be defined multiply (with MULTI) or listed on a single line
static void listFromRecipe(psList *target, // The target list
                           const psMetadata *recipe, // Recipe to search
                           const char *name // Name for item within recipe
    )
{
    // If the list already has entries, don't read anything else
    if (psListLength(target) > 0) {
        return;
    }

    // First check that at least one item of interest exists
    psMetadataItem *checkItem = psMetadataLookup(recipe, name);
    if (!checkItem) {
        // Nothing to see here
        return;
    }

    psString regex = NULL;              // Regular expression for the flag
    psStringAppend(&regex, "^%s$", name);
    psMetadataIterator *iterator = psMetadataIteratorAlloc(recipe, PS_LIST_HEAD, regex);
    psFree(regex);
    psMetadataItem *item;
    int numItem = 0; // Occurrence of the item in the recipe; to help the user in case of trouble
    while ((item = psMetadataGetAndIncrement(iterator))) {
        numItem++;
        if (item->type != PS_DATA_STRING) {
            psLogMsg(__func__, PS_LOG_WARN, "Occurrence %d of %s in the recipe is "
                     "not of type STRING (%x) --- ignored.\n", numItem, name, item->type);
            continue;
        }
        // Parse into a list of independent values
        psList *values = psStringSplit(item->data.V, " ,;", false);
        // Copy into the target
        psListIterator *valuesIter = psListIteratorAlloc(values, PS_LIST_HEAD, false);
        psString valueString;
        while ((valueString = psListGetAndIncrement(valuesIter))) {
            // Remove the default _UNDEF entries in the recipe, which come from the limitation of the metadata
            // config language to have an empty MULTI, combined with the constraint that higher-level recipes
            // aren't allowed to add new values but only change what already exists.
            if (strcmp(valueString, "_UNDEF") != 0) {
                psListAdd(target, PS_LIST_TAIL, valueString);
            }
        }
        psFree(valuesIter);
        psFree(values);
    }
    psFree(iterator);

    return;
}


bool ppStatsSetupFromRecipe(ppStatsData *data, // Data for running ppStats
                            pmConfig *config // Configuration
    )
{
    PS_ASSERT_PTR_NON_NULL(data, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    // Determine recipe parameters
    bool mdok;                          // Status of MD lookup
    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSTATS_RECIPE);
    if (!mdok || !recipe) {
        psLogMsg(__func__, PS_LOG_WARN, "Unable to find recipe %s.\n", PPSTATS_RECIPE);
        return data;
    }

    listFromRecipe(data->chips,    recipe, "CHIP");
    listFromRecipe(data->cells,    recipe, "CELL");
    listFromRecipe(data->headers,  recipe, "HEADER");
    listFromRecipe(data->concepts, recipe, "CONCEPT");
    listFromRecipe(data->analysis, recipe, "ANALYSIS");

    // Parse SUMMARY statistics information
    listFromRecipe(data->summary, recipe, "SUMMARY");

    // Parse the statistics options
    psList *recipeStats = psListAlloc(NULL); // List of statistics options
    listFromRecipe(recipeStats, recipe, "STAT");

    // validate STATs choices
    if (psListLength(recipeStats) > 0) {
        psListIterator *iterator = psListIteratorAlloc(recipeStats, PS_LIST_HEAD, false);
        psString statString;            // Statistic string, from iteration
        while ((statString = psListGetAndIncrement(iterator))) {
            psStatsOptions stat = psStatsOptionFromString(statString);
            if (stat == 0) {
                psLogMsg(__func__, PS_LOG_WARN, "Can't interpret STATS entry in recipe: "
                         "%s --- ignored.\n", statString);
                continue;
            }
            data->stats->options |= stat;
            data->doStats = true;
        }
    }
    psFree(recipeStats);

    // Clipping options
    if (data->stats->clipIter == 0 && isnan(data->stats->clipSigma)) {
        int iter = psMetadataLookupS32(&mdok, recipe, "ITER"); // Number of clipping iterations
        if (mdok && iter > 0) {
            data->stats->clipIter = iter;
        } else {
            psLogMsg(__func__, PS_LOG_WARN, "ITER in recipe is not of type S32 and positive --- "
                     "retaining default.\n");
        }
        float rej = psMetadataLookupF32(&mdok, recipe, "REJ"); // Clipping level
        if (mdok && rej > 0) {
            data->stats->clipSigma = rej;
        } else {
            psLogMsg(__func__, PS_LOG_WARN, "REJ in recipe is not of type F32 and positive --- "
                     "retaining default.\n");
        }
    }

    if (data->sample == 0) {
        float sample = psMetadataLookupF32(&mdok, recipe, "SAMPLE"); // Sample fraction
        if (mdok && sample > 0) {
            data->sample = sample;
        } else {
            psLogMsg(__func__, PS_LOG_WARN, "SAMPLE in recipe is not of type F32 and positive --- "
                     "retaining default.\n");
        }
    }

    bool doFirst = psMetadataLookupBool(&mdok, recipe, "DO.FIRST.READOUT.3D"); // Sample fraction
    if (mdok) {
	data->doFirstReadout3D = doFirst;
    } 

    // set the mask value used for stand-alone analyses.
    if (data->maskVal == 0) {
        const char *names = psMetadataLookupStr(&mdok, recipe, "MASKVAL"); // Names for mask value
        if (mdok) {
            data->maskVal = pmConfigMaskGet(names, config);
        } else {
            psWarning("MASKVAL in recipe is not of type STR --- retaining default.\n");
        }
    }

    return data;
}

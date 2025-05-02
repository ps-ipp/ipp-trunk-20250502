#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <assert.h>
#include <pslib.h>
#include "pmConfig.h"
#include "pmConfigRecipes.h"

static bool loadRecipeSystem(bool *status, pmConfig *config);
static bool loadRecipeCamera(bool *status, pmConfig *config, psMetadata *source);
static bool loadRecipeSymbols(bool *status, pmConfig *config, pmRecipeSource source);
static bool loadRecipeFromArguments(bool *status, pmConfig *config);
static bool loadRecipeOptions(bool *status, pmConfig *config);
static bool mergeRecipeCamera(bool *status, pmConfig *config);

// use this function to select the options structure for the specified recipe
// add additional command-line options to this metadata (before parsing the camera)
psMetadata *pmConfigRecipeOptions (pmConfig *config, char *recipeName)
{
    bool success;

    // select or create the OPTIONS folder
    psMetadata *options = psMetadataLookupMetadata(&success, config->arguments, "OPTIONS");
    if (!options) {
        options = psMetadataAlloc ();
        success = psMetadataAddPtr (config->arguments, PS_LIST_TAIL, "OPTIONS",  PS_DATA_METADATA, "",
                                    options);
        assert (success); // type mismatch : OPTIONS already defined but wrong type
        psFree (options); // drop extra reference
    }

    // look for the recipe defined in recipes
    // if the recipe is already defined in config->arguments:OPTIONS, supplement
    // save the recipe options onto config->arguments:RECIPES
    psMetadata *recipe = psMetadataLookupMetadata(&success, options, recipeName);
    if (!recipe) {
        recipe = psMetadataAlloc();
        success = psMetadataAddPtr(options, PS_LIST_TAIL, recipeName,  PS_DATA_METADATA, "", recipe);
        assert (success); // type mismatch : OPTIONS already defined but wrong type
        psFree (recipe);  // drop extra reference
    }
    return recipe;
}

// this function may be called several times.  it attempts to load the recipe data from one of three
// locations: config->system, config->camera, and argv.  We cannot read the recipes from
// config->camera until a camera has been read BUT, the argv recipes must override the camera and
// system recipes.  This command strips the argv elements it uses from the argv list.
bool pmConfigReadRecipes(pmConfig *config, pmRecipeSource source)
{
    bool status;
    PS_ASSERT_PTR_NON_NULL(config, false);

    // Read the recipe file names from the system configuration and camera configuration
    // It is an error for config->system:recipes not to exist.  all programs install their
    // master recipe files in the system:recipe location when they are built.
    psAssert(config->system, "base config data defined");
    if (!config->recipes || (source & PM_RECIPE_SOURCE_SYSTEM)) {
        if (!loadRecipeSystem(&status, config)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from system config");
            return false;
        }
        psTrace ("psModules.config", 3, "read recipes from system config");
    }

    // camera-specific recipes are not required : note the absence with a message
    // camera-specific recipes may be read for a specified camera (in pmConfigRead) or
    // for an identified camera (in pmConfigCameraFormatFromHeader).  the second
    // set should not override the first set
    if (config->camera && (source & PM_RECIPE_SOURCE_CAMERA) &&
        !(config->recipesRead & PM_RECIPE_SOURCE_CAMERA)) {
        if (!loadRecipeCamera(&status, config, config->camera)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from camera config");
            return false;
        }
        if (status) {
            psTrace ("psModules.config", 3, "read recipes from camera config");
        } else {
            psTrace ("psModules.config", PS_LOG_DETAIL, "no recipe supplied by camera config");
        }
    }

    // merge camera and sytem recipes, apply recipes loaded into config->arguments based on command-line
    // arguments
    if (config->arguments && (source & PM_RECIPE_SOURCE_CL)) {

        // update the system-level recipes with the symbolically-defined recipes
        if (!loadRecipeSymbols(&status, config, PM_RECIPE_SOURCE_SYSTEM)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from symbolic references");
            return false;
        }
        if (status) {
            psTrace ("psModules.config", 3, "read recipes from symbolic references");
        } else {
            psTrace ("psModules.config", PS_LOG_DETAIL, "no recipe supplied by symbolic reference");
        }

        // merge the SYSTEM and CAMERA recipes
        if (!mergeRecipeCamera(&status, config)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from symbolic references");
            return false;
        }
        psTrace ("psModules.config", PS_LOG_DETAIL, "merged camera recipes with system recipes");

        // load recipe-files specified on the command line
        if (!loadRecipeFromArguments(&status, config)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from command-line arguments");
            return false;
        }
        if (status) {
            psTrace ("psModules.config", 3, "read recipes from command-line arguments");
        } else {
            psTrace ("psModules.config", PS_LOG_DETAIL, "no recipe supplied on command-line arguments");
        }

        // update the system-level recipes with the symbolically-defined recipes
        if (!loadRecipeSymbols(&status, config, PM_RECIPE_SOURCE_CAMERA)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from symbolic references");
            return false;
        }
        if (status) {
            psTrace ("psModules.config", 3, "read recipes from symbolic references");
        } else {
            psTrace ("psModules.config", PS_LOG_DETAIL, "no recipe supplied by symbolic reference");
        }

        // override any specific values with values from the command line
        if (!loadRecipeOptions(&status, config)) {
            psError(PS_ERR_IO, false, "Failed to read recipes from symbolic references");
            return false;
        }
        if (status) {
            psTrace ("psModules.config", 3, "read recipes from command-line arguments");
        } else {
            psTrace ("psModules.config", PS_LOG_DETAIL, "no recipe supplied on command-line arguments");
        }
    }
    return true;
}

// search for options of the form -D KEY VALUE or -D RECIPE:KEY VALUE
bool pmConfigLoadRecipeOptions (int *argc, char **argv, pmConfig *config, char *flag)
{
    bool success;
    int argNum;

    // save the recipe options onto config->arguments:OPTIONS
    // increment so we can free below (is a NOP if 'options' is NULL)
    psMetadata *options = psMetadataLookupMetadata(&success, config->arguments, "OPTIONS");
    if (!options) {
        options = psMetadataAlloc();
        success = psMetadataAddPtr(config->arguments, PS_LIST_TAIL, "OPTIONS",  PS_DATA_METADATA,
                                   "Command-line options specified with -D", options);
        assert (success); // type mismatch : OPTIONS already defined but wrong type
        psFree (options); // drop extra reference
    }

    // -D key value (all added as string)
    while ((argNum = psArgumentGet (*argc, argv, flag))) {
        psArgumentRemove (argNum, argc, argv);

        // do we have enough arguments?
        if (argNum + 1 >= *argc) {
            psError(PS_ERR_IO, true, "insufficient parameters for command-line argument -D");
            return false;
        }

        // is a target recipe specified?
        const char *recipeName = NULL;
        char *key;
        psArray *words = psStringSplitArray(argv[argNum], ":", false);
        switch (words->n) {
        case 1:
            recipeName = config->defaultRecipe;
            if (!config->defaultRecipe) {
                psError(PS_ERR_IO, true,
                        "syntax error in parameter: no default recipe available; must specify recipe");
                return false;
            }
            key = words->data[0];
            break;
        case 2:
            recipeName = words->data[0];
            key = words->data[1];
            break;
        default:
            psError(PS_ERR_IO, true, "syntax error in parameter");
            return false;
        }

        // if this recipe is already defined in options, supplement
        psMetadata *recipe = psMetadataLookupMetadata(&success, options, recipeName);
        if (!recipe) {
            recipe = psMetadataAlloc();
            success = psMetadataAddPtr(options, PS_LIST_TAIL, recipeName,  PS_DATA_METADATA, "", recipe);
            assert (success); // type mismatch : recipe already defined but wrong type
            psFree (recipe); // drop extra reference
        }

        bool valid = false;
        if (!strcmp (flag, "-D")) {
            psMetadataAddStr (recipe, PS_LIST_TAIL, key, PS_META_REPLACE, "", argv[argNum+1]);
            valid = true;
        }
        if (!strcmp (flag, "-Di")) {
            psMetadataAddS32 (recipe, PS_LIST_TAIL, key, PS_META_REPLACE, "", atoi(argv[argNum+1]));
            valid = true;
        }
        if (!strcmp (flag, "-Df")) {
            psMetadataAddF32 (recipe, PS_LIST_TAIL, key, PS_META_REPLACE, "", atof(argv[argNum+1]));
            valid = true;
        }
        if (!strcmp (flag, "-Db")) {
            if (!strcasecmp(argv[argNum+1], "true") || !strcasecmp(argv[argNum+1], "t") ||
                !strcasecmp(argv[argNum+1], "1")) {
                psMetadataAddBool (recipe, PS_LIST_TAIL, key, PS_META_REPLACE, "", true);
            } else if (!strcasecmp(argv[argNum+1], "false") || !strcasecmp(argv[argNum+1], "f") ||
                       !strcasecmp(argv[argNum+1], "0")) {
                psMetadataAddBool (recipe, PS_LIST_TAIL, key, PS_META_REPLACE, "", false);
            } else {
                psWarning("Couldn't interpret option for %s (%s) --- ignored", argv[argNum], argv[argNum+1]);
            }
            valid = true;
        }
        psFree (words);
        psAssert (valid, "invalid flag: may be: -D, -Df, -Di, -Db\n");

        psArgumentRemove (argNum, argc, argv);
        psArgumentRemove (argNum, argc, argv);
    }
    return true;
}

// examine command-line arguments for -recipe RECIPE SYMBOLIC-NAME or -recipe-file RECIPE FILENAME
// in the first case, the symbolic lookup is saved on config->recipeSymbols
//   for later interpolation (pmConfigReadRecipes with option CL)
// in the second case, read as metadata and save on config->arguments with name = KEY
bool pmConfigLoadRecipeArguments (int *argc, char **argv, pmConfig *config)
{
    bool success;

    psMetadata *recipes = psMetadataLookupMetadata(&success, config->arguments, "RECIPES");
    if (!recipes) {
        recipes = psMetadataAlloc();
        success = psMetadataAddPtr (config->arguments, PS_LIST_TAIL, "RECIPES",  PS_DATA_METADATA, "",
                                    recipes);
        assert (success);
        psFree (recipes);
    }

    // Go through the command-line arguments
    int argNum;                         // Argument number

    // -recipe-file: read recipe from file
    while ((argNum = psArgumentGet(*argc, argv, "-recipe-file")) > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum + 1 >= *argc) {
            psError(PS_ERR_IO, false,
                    "-recipe command-line switch provided without the required recipe and filename\n");
            return false;
        }

        psString recipeName = psStringCopy(argv[argNum]); // Name of the recipe
        psArgumentRemove(argNum, argc, argv);
        psString filename = psStringCopy(argv[argNum]); // Filename for the recipe
        psArgumentRemove(argNum, argc, argv);

        psMetadata *recipe = NULL;      // Recipe from file
        if (!pmConfigFileRead(&recipe, filename, "recipe")) {
            psError(PS_ERR_IO, false, "Error reading config file %s\n", filename);
            psFree(recipeName);
            psFree(filename);
            return false;
        }

        psString comment = NULL;
        psStringAppend(&comment, "Recipe added at command line from file %s", filename);
        psMetadataAdd(recipes, PS_LIST_TAIL, recipeName, PS_DATA_METADATA | PS_META_REPLACE,
                      comment, recipe);
        psFree(comment);
        psFree(recipe);                 // Drop reference
        psFree(recipeName);
        psFree(filename);
    }

    // -recipe: read recipe from symbolic link
    while ((argNum = psArgumentGet(*argc, argv, "-recipe")) > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum + 1 >= *argc) {
            psError(PS_ERR_IO, false,
                    "-recipe command-line switch provided without the required recipe and source\n");
            return false;
        }

        char *recipeName = psStringCopy(argv[argNum]); // Name of the recipe
        psArgumentRemove(argNum, argc, argv);
        char *recipeSource = psStringCopy(argv[argNum]); // Source of the recipe
        psArgumentRemove(argNum, argc, argv);

        // Assume it's a symbolic reference to something that's not yet read in.
        // it will be loaded later by pmConfigReadRecipes with option CL
        psMetadataAddStr(config->recipeSymbols, PS_LIST_TAIL, recipeName, PS_META_REPLACE, NULL,
                         recipeSource);

        psTrace ("psModules.config", 3, "read recipe %s from %s", recipeName, recipeSource);
        psFree(recipeName);
        psFree(recipeSource);
    }

    return true;
}

// Load the recipe files for SYSTEM : REQUIRED
static bool loadRecipeSystem(bool *status,
                           pmConfig *config // The configuration into which to read the recipes
    )
{
    assert(status);
    assert(config);
    *status = false;

    if (!config->system) {
        psError(PS_ERR_IO, true,
                "The system configuration has not been read --- cannot read recipes from this location.\n");
        config->recipesRead &= ~PM_RECIPE_SOURCE_SYSTEM;
        return false;
    }

    if (!config->recipes) {
        config->recipes = psMemIncrRefCounter(psMetadataLookupMetadata(NULL, config->system, "RECIPES")); // The list of recipes
        if (!config->recipes) {
            psError(PS_ERR_IO, false, "RECIPES not found in the system configuration\n");
            return false;
        }
    }

    // Read in the component recipes
    psMetadataIterator *recipesIter = psMetadataIteratorAlloc(config->recipes, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL;        // MD item containing the filename, from recipe iteration
    while ((item = psMetadataGetAndIncrement(recipesIter))) {
        if (!pmConfigFileIngest(item, "recipe")) {
            psError(PS_ERR_IO, false, "Failed to read recipe %s listed in system configuration",
                    item->name);
            psFree(recipesIter);
            return false;
        }
    }
    psFree(recipesIter);
    config->recipesRead |= PM_RECIPE_SOURCE_SYSTEM;

    *status = true;
    return true;
}

// Load the recipe files for a specific CAMERA.  these are saved on recipesCamera
static bool loadRecipeCamera(bool *status, // status variable
                             pmConfig *config, // The configuration into which to read the recipes
                             psMetadata *source // The source configuration, from which to read the filenames
    )
{
    bool success;

    assert(status);
    assert(config);
    *status = false;

    if (!source) {
        psError(PS_ERR_IO, true,
                "The camera configuration has not been read --- cannot read recipes from this location.\n");
        config->recipesRead &= ~PM_RECIPE_SOURCE_CAMERA;
        return false;
    }

    // it is not necessary to define any local recipes in the camera config; it this entry is missing,
    // just return true
    psMetadata *recipes = psMetadataLookupMetadata(&success, source, "RECIPES"); // The list of recipes
    if (!recipes) {
        psTrace ("psModules.config", 3, "RECIPES not found in the camera configuration\n");
        return true;
    }

    // Copy contents of the filenames to config->recipes from the "RECIPES" metadata in the source.
    // We could use psMetadataCopy for this, but it's better to check that everything's of the correct type.
    // If it's not of the correct type, we can tell the user which file it's in, so they can find it easier.
    psMetadataIterator *recipesIter = psMetadataIteratorAlloc(recipes, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL;    // MD item containing the filename, from recipe iteration
    while ((item = psMetadataGetAndIncrement(recipesIter))) {
        if (!pmConfigFileIngest(item, "recipe")) {
            psError(PS_ERR_IO, false, "Failed to read recipe %s listed in camera configuration",
                    item->name);
            return false;
        }
        const char *recipeName = item->name; // Name of the recipe
        psMetadata *recipe = item->data.md; // The recipe

        // the named recipe must exist at the system level
        psMetadata *current = psMetadataLookupMetadata(NULL, config->recipes, recipeName);
        if (!current) {
            psError(PS_ERR_IO, false, "Failed to find recipe for %s in master recipe list", recipeName);
            return false;
        }

        // add the contents of this recipe file to config->recipesCamera
        psMetadataAdd(config->recipesCamera, PS_LIST_TAIL, recipeName, PS_DATA_METADATA | PS_META_REPLACE,
                      item->comment, recipe);
    }
    psFree(recipesIter);
    config->recipesRead |= PM_RECIPE_SOURCE_CAMERA;
    *status = true;
    return true;
}

// Merge the CAMERA recipes into the SYSTEM recipes
static bool mergeRecipeCamera(bool *status, // status variable
                             pmConfig *config // The configuration into which to read the recipes
    )
{
    assert(status);
    assert(config);
    *status = false;

    // Copy contents of config->recipesCamera to config->recipes
    // We could use psMetadataCopy for this, but it's better to check that everything's of the correct type.
    // If it's not of the correct type, we can tell the user which file it's in, so they can find it easier.
    psMetadataIterator *recipesIter = psMetadataIteratorAlloc(config->recipesCamera, PS_LIST_HEAD, NULL);
    psMetadataItem *folderItem = NULL;    // MD item containing the filename, from recipe iteration
    while ((folderItem = psMetadataGetAndIncrement(recipesIter))) {
        char *recipeName = folderItem->name;
        psMetadata *recipe = folderItem->data.md;

        psTrace("psModules.config", 3, "merging %s from camera with system recipes.\n", recipeName);

        // type mismatch is a serious error
        if (folderItem->type != PS_DATA_METADATA) {
            psAbort("%s not of type METADATA", recipeName);
        }

        // the select the named recipe from the system level
        psMetadata *current = psMetadataLookupMetadata(NULL, config->recipes, recipeName);
        psAssert (current, "Failed to find recipe for %s in system recipe list", recipeName);

        // update the contents of this recipe from the one on config->recipesCamera
        if (!psMetadataUpdate(current, recipe)) {
            psError(PS_ERR_IO, false, "Failed to update recipe for %s from camera recipe", recipeName);
            return false;
        }
    }
    psFree(recipesIter);
    *status = true;
    return true;
}

// Load the recipes from config->arguments (CL)
// Load the recipes: each time we load a specific recipe, it overrides the metadata
// entries for an existing recipe metadata
static bool loadRecipeFromArguments(bool *status,
                                    pmConfig *config // The configuration into which to read the recipes
    )
{
    assert(status);
    assert(config);
    *status = false;

    if (!config->arguments) {
        psTrace("psModules.config", 4, "no config->arguments metadata, nothing to read here");
        return true;
    }

    psMetadata *recipes = psMetadataLookupMetadata(NULL, config->arguments, "RECIPES"); // The list of recipes
    if (!recipes) {
        psTrace("psModules.config", 4, "no RECIPES in config->arguments, nothing to read here");
        return true;
    }

    // Copy the recipes from config->arguments:RECIPES to config->recipes.  supplement existing recipes.
    psMetadataIterator *recipesIter = psMetadataIteratorAlloc(recipes, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL;    // MD item containing the filename, from recipe iteration
    while ((item = psMetadataGetAndIncrement(recipesIter))) {
        // type mismatch is a serious error
        if (item->type != PS_DATA_METADATA) {
            psAbort("%s in config arguments RECIPES is not of type METADATA", item->name);
        }
        // increment the ref counter to protect the data

        psMetadata *recipe = item->data.md; // Recipe of interest

        // if this named recipe exists, supplement it
        psMetadata *current = psMetadataLookupMetadata(NULL, config->recipes, item->name);
        if (!current) {
            psError(PS_ERR_IO, false, "Failed to find recipe for %s in master recipe list", item->name);
            psFree(recipe);  // Drop reference
            return false;
        }
        psTrace("psModules.config", 3, "Supplementing %s from arguments.\n", item->name);

        if (!psMetadataUpdate (current, recipe)) {
            psError(PS_ERR_IO, false, "Failed to update recipe for %s from camera recipe", item->name);
            return false;
        }
    }
    psFree(recipesIter);
    *status = true;
    return true;
}

// Load the recipes: each time we load a specific recipe, it overrides the metadata
// entries for an existing recipe metadata
// The configuration into which to read the recipes
static bool loadRecipeSymbols(bool *status, pmConfig *config, pmRecipeSource source) {

    bool found = false;

    assert(status);
    assert(config);
    *status = false;

    // check to see if any symbolic names need to be resolved
    // each entry in recipeSymbols are of the form TARGET=SOURCE where TARGET is an existing
    // recipe MD and REF is a MD to load over that recipe
    psMetadataIterator *iter = psMetadataIteratorAlloc(config->recipeSymbols, PS_LIST_HEAD, NULL);
    psMetadataItem *item = NULL;  // Item containing source, from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        assert(item->type == PS_DATA_STRING); // It should be this type: we put it in ourselves
        const char *sourceName = item->data.str; // The name of the symbolic reference
        const char *targetName = item->name;
        psTrace("psModules.config", 3, "Supplementing %s from %s.\n", targetName, sourceName);

        // the target recipe must exist; select it
        psMetadata *targetMD = psMetadataLookupMetadata(&found, config->recipes, targetName);
        if (!targetMD) {
            psError(PS_ERR_IO, true, "Failed to find recipe for %s in master recipe list", targetName);
            return false;
        }

        // search for sourceName in config->recipes (folder name is targetName)
        psMetadata *folder = psMetadataLookupMetadata(&found, config->recipes, targetName);
        psMetadata *sourceMD = psMetadataLookupMetadata(&found, folder, sourceName);

        // if we find the desired symbolic name at this level, set the item comment to say "FOUND"
        if (sourceMD) {
          if (!psMetadataUpdate(targetMD, sourceMD)) {
            psError(PS_ERR_IO, false, "Failed to update recipe for %s from camera recipe", targetName);
            return false;
          }
          psStringAppend (&item->comment, "(FOUND)");
        }

        // if we have not found it by the camera level, we have a problem
        if (source == PM_RECIPE_SOURCE_CAMERA) {
          if (strstr (item->comment, "(FOUND)") == NULL) {
            psError(PS_ERR_IO, false, "Selected symbolic name %s does not exist in recipes. Use syntax '-recipe NAME VALUE'", sourceName);
            return false;
          }
        }
    }
    psFree(iter);
    *status = true;
    return true;
}

// Load the recipe options
// Load the recipes: each time we load a specific recipe, it overrides the metadata
// entries for an existing recipe metadata
static bool loadRecipeOptions(bool *status,
                              pmConfig *config // The configuration into which to read the recipes
    )
{
    bool found;
    assert(status);
    assert(config);
    *status = false;

    if (!config->arguments) {
        psTrace("psModules.config", 4, "no config->arguments metadata, nothing to read here");
        return true;
    }

    psMetadata *recipes = psMetadataLookupMetadata(&found, config->arguments, "OPTIONS"); // List of recipes
    if (!recipes) {
        psTrace("psModules.config", 4, "no OPTIONS in config->arguments, nothing to read here");
        return true;
    }

    // Copy the recipes from config->arguments:OPTIONS to config->recipes.  supplement existing recipes.
    psMetadataIterator *recipesIter = psMetadataIteratorAlloc(recipes, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL;    // MD item containing the filename, from recipe iteration
    while ((item = psMetadataGetAndIncrement(recipesIter))) {
        char *recipeName = item->name;
        psMetadata *recipe = item->data.md;

        // type mismatch is a serious error
        if (item->type != PS_DATA_METADATA) {
            psAbort("%s in config arguments OPTIONS is not of type METADATA", recipeName);
        }

        // if this named recipe exists, supplement it
        psMetadata *current = psMetadataLookupMetadata(NULL, config->recipes, recipeName);
        if (!current) {
            psError(PS_ERR_IO, false, "Selected recipe %s is not found in camera recipe", recipeName);
            return false;
        }
        if (!psMetadataUpdate (current, recipe)) {
            psError(PS_ERR_IO, false, "Failed to update recipe for %s from camera recipe", recipeName);
            return false;
        }
    }
    psFree(recipesIter);
    *status = true;
    return true;
}

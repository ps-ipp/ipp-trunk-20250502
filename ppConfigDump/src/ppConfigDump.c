#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

pmConfig *config = NULL;                // Configuration; declared globally for convenience
psMetadata *arguments = NULL;           // Command-line arguments; declared globally for convenience

// Clean up and die
void die(int code                       // Exit code
         )
{
    psFree(arguments);
    psFree(config);
    pmConceptsDone();
    pmConfigDone();
    psLibFinalize();

    exit(code);
}

void dump(const char *filename,     // Filename to which to dump
          const char *description,  // Description of what's being dumped
          psMetadata *md,           // Metadata to dump
	  const char *compressMode
    )
{
    if (!filename || strlen(filename) == 0) {
        return;
    }
    if (strcmp(filename, "-") == 0) {
        psString string = psMetadataConfigFormat(md); // String to dump
        if (!string) {
            psErrorStackPrint(stderr, "Can't write %s to STDOUT\n", description);
            die(PS_EXIT_SYS_ERROR);
        }

	if (compressMode) {
	    if (strlen(compressMode) > 2) {
		psErrorStackPrint(stderr, "invalid compression options %s!\n", compressMode);
		die(PS_EXIT_CONFIG_ERROR);
	    }
	    char modeString[4];
	    snprintf (modeString, 4, "w%s", compressMode);

	    gzFile file = gzdopen (STDOUT_FILENO, modeString);
	    if (file == Z_NULL) {
		psErrorStackPrint(stderr, "Failed to open file\n");
		die(PS_EXIT_SYS_ERROR);
	    }
	    int nbytes = gzwrite (file, string, strlen(string));
	    if (nbytes != strlen(string)) {
		psErrorStackPrint(stderr, "Failed to write contents of configuration file %s", filename);
		psFree(string);
		gzclose(file);
		die(PS_EXIT_SYS_ERROR);
	    }
	    psFree(string);
	    if (gzclose(file) != Z_OK) {
		psErrorStackPrint(stderr, "Failed to close file, %s\n", filename);
		die(PS_EXIT_SYS_ERROR);
	    }
	} else {
	    fprintf(stdout, "%s", string);
	}
        psFree(string);
    } else {
	if (!psMetadataConfigWrite(md, filename, compressMode)) {
	    psErrorStackPrint(stderr, "Can't write %s to %s\n", description, filename);
	    die(PS_EXIT_SYS_ERROR);
	}
    }
}

int main(int argc, char *argv[])
{
    psLibInit(NULL);
    (void) psTraceSetLevel("err", 10);

    // load the site-wide configuration information
    config = pmConfigRead(&argc, argv, NULL);
    if (!config) {
        psErrorStackPrint(stderr, "Can't find site configuration!\n");
        die(PS_EXIT_CONFIG_ERROR);
    }

    //    psMetadata *options = pmConfigRecipeOptions(config, RECIPE_NAME);

    arguments = psMetadataAlloc();
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-file", 0, "FITS file to use for camera determination", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-compress", 0, "output compression mode", NULL);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "-z", 0, "default compression", false);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dump-user", 0, "Filename for user configuration", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dump-site", 0, "Filename for site configuration", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dump-system", 0, "Filename for system configuration", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dump-camera", 0, "Filename for camera configuration", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dump-format", 0, "Filename for camera format", NULL);
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-dump-recipes", 0, "Filename for recipes", NULL);

    psMetadataAddStr(arguments, PS_LIST_TAIL, "-get-key", PS_META_DUPLICATE_OK, "Key to return", NULL);

    psMetadata *recipeArgs = psMetadataAlloc(); // Options for dumping a single recipe
    psMetadataAddStr(recipeArgs, PS_LIST_TAIL, "recipe", 0, "Name of recipe", NULL);
    psMetadataAddStr(recipeArgs, PS_LIST_TAIL, "filename", 0, "Filename", NULL);
    psMetadataAddMetadata(arguments, PS_LIST_TAIL, "-dump-recipe", 0, "Dump a single recipe", recipeArgs);
    psFree(recipeArgs);

    if (argc == 1 || !psArgumentParse(arguments, &argc, argv)) {
        fprintf(stderr, "\nPan-STARRS IPP configuration dumper\n\n");
        fprintf(stderr, "Usage: %s [-file INPUT.fits] [-dump-site FILE.mdc]\n", argv[0]);
        fprintf(stderr, "       [-dump-camera FILE.mdc] [-dump-format FILE.mdc] [-dump-recipes FILE.mdc]\n");
        fprintf(stderr, "       [-dump-recipe RECIPE FILE.mdc]\n");
        fprintf(stderr, "       [-get-key KEY]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "       FILE.mdc may be \"-\", in which case the file is written to stdout.\n");
        fprintf(stderr, "\n");
        psArgumentHelp(arguments);
        die(PS_EXIT_CONFIG_ERROR);
    }

    const char *defaultCompressMode = "7f";
    const char *compressMode = psMetadataLookupStr(NULL, arguments, "-compress"); // compression
    bool defaultCompression = psMetadataLookupBool(NULL, arguments, "-z"); // compression?
    if (!compressMode && defaultCompression) {
	compressMode = defaultCompressMode;
    }

    const char *inName = psMetadataLookupStr(NULL, arguments, "-file"); // Name of input FITS file
    if (inName) {
        psString resolved = pmConfigConvertFilename(inName, config, false, false); // Resolved filename
        psFits *inFile = psFitsOpen(resolved, "r"); // File handle for FITS file
        if (!inFile) {
            psErrorStackPrint(stderr, "Can't open input image: %s\n", resolved);
            die(PS_EXIT_DATA_ERROR);
        }
        psFree(resolved);

        psMetadata *phu = psFitsReadHeader(NULL, inFile); // FITS primary header
        psFitsClose(inFile);
        if (!phu) {
            psErrorStackPrint(stderr, "Can't read PHU of input image: %s\n", inName);
            die(PS_EXIT_DATA_ERROR);
        }

        psMetadata *format = pmConfigCameraFormatFromHeader(NULL, NULL, NULL, config, phu, true); // Camera format
        if (!format || !config->camera) {
            psErrorStackPrint(stderr, "Can't find suitable camera configuration!\n");
            psFree(format);
            psFree(phu);
            die(PS_EXIT_SYS_ERROR);
        }
        psFree(format);
        psFree(phu);
    } else {
        // Now we have the camera, we can read the recipes
        if (!pmConfigReadRecipes(config, PM_RECIPE_SOURCE_CAMERA | PM_RECIPE_SOURCE_CL)) {
            psError(PS_ERR_IO, false, "Error reading recipes from camera config for %s", config->cameraName);
            psErrorStackPrint(stderr, "problem read recipes!\n");
            die(PS_EXIT_SYS_ERROR);
        }
    }

    const char *userName = psMetadataLookupStr(NULL, arguments, "-dump-user"); // User filename
    dump(userName, "user configuration", config->user, compressMode);

    const char *siteName = psMetadataLookupStr(NULL, arguments, "-dump-site"); // Site filename
    dump(siteName, "site configuration", config->site, compressMode);

    const char *systemName = psMetadataLookupStr(NULL, arguments, "-dump-system"); // System filename
    dump(systemName, "system configuration", config->system, compressMode);

    const char *camName = psMetadataLookupStr(NULL, arguments, "-dump-camera"); // Camera filename
    dump(camName, "camera configuration", config->camera, compressMode);

    const char *formatName = psMetadataLookupStr(NULL, arguments, "-dump-format"); // Format filename
    dump(formatName, "camera format", config->format, compressMode);

    const char *recipesName = psMetadataLookupStr(NULL, arguments, "-dump-recipes"); // Recipes filename
    dump(recipesName, "recipes", config->recipes, compressMode);

    recipeArgs = psMetadataLookupMetadata(NULL, arguments, "-dump-recipe");
    const char *recipeName = psMetadataLookupStr(NULL, recipeArgs, "recipe"); // Name of recipe
    if (recipeName) {
        psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, recipeName); // Recipe desired
        if (!recipe) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to find recipe %s", recipeName);
            die(PS_EXIT_CONFIG_ERROR);
        }
        const char *recipeFile = psMetadataLookupStr(NULL, recipeArgs, "filename"); // Filename for recipe
        dump(recipeFile, "recipe", recipe, compressMode);
    }

    // parse the -get-key stuff:
    psMetadataItem *item = psMetadataLookup(arguments, "-get-key");
    psAssert (item->type == PS_DATA_METADATA_MULTI, "created above with this type");
    psListIterator *iter = psListIteratorAlloc(item->data.list, 0, false);
    psMetadataItem *mItem = NULL;
    while ((mItem = psListGetAndIncrement(iter))) {
	char *getKey = mItem->data.str;
	if (!getKey) continue;
	psMetadataItem *keyItem = psMetadataLookup (config->camera, getKey);
	if (!keyItem) continue;
	psString str = psMetadataItemFormat(keyItem);
	fprintf (stdout, "%s", str);
	psFree (str);
    }
    psFree(iter);
	
    psFree(arguments);
    psFree(config);

    pmConceptsDone();
    pmConfigDone();
    psLibFinalize();
    if (0) fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stdout, false), "psphot");

    die(PS_EXIT_SUCCESS);
}

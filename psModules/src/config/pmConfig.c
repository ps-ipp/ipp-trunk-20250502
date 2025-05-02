/** @file  pmConfig.h
 *
 *  @author PAP (IfA)
 *  @author EAM (IfA)
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <unistd.h>
#include <libgen.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmFPALevel.h"
#include "pmConfigRecipes.h"
#include "pmConfigCamera.h"
#include "pmConfigRun.h"

#include "pmConfig.h"
#include "pmVisualUtils.h"

#ifdef HAVE_NEBCLIENT
#include <nebclient.h>
#endif // ifdef HAVE_NEBCLIENT

#define IPPRC_ENV "IPPRC"        // Name of the environment variable containing the top-level config file
#define IPPRC_FILE ".ipprc"      // Default top-level config file

#define DEFAULT_LOG STDERR_FILENO       // Default file descriptor for log messages
#define DEFAULT_TRACE STDERR_FILENO     // Default file descriptor for trace messages

#define CHECK_FILE_RETRY 5              // Number of retries when checking a file
#define CHECK_FILE_WAIT 250000          // Wait between retries (usec) when checking a file

static bool readCameraConfig = true;    // Read the camera config on startup (with pmConfigRead)?
static psArray *configPath = NULL;      // Search path for configuration files

static bool checkPath(const char *filename, bool create, bool trunc);
static psString resolveConfigFile(const char *name);

bool pmConfigReadParamsSet(bool newReadCameraConfig)
{
    bool oldReadCameraConfig = readCameraConfig;
    readCameraConfig = newReadCameraConfig;
    return oldReadCameraConfig;
}

static void configFree(pmConfig *config)
{
    psFree(config->user);
    psFree(config->site);
    psFree(config->system);
    psFree(config->files);
    psFree(config->camera);
    psFree(config->cameraName);
    psFree(config->format);
    psFree(config->formatName);
    psFree(config->recipes);
    psFree(config->recipesCamera);
    psFree(config->recipeSymbols);
    psFree(config->arguments);
    psFree(config->database);
    psFree(config->program);

    // Close log and trace files
    if (config->logFD != STDOUT_FILENO && config->logFD != STDERR_FILENO) {
        close(config->logFD);
    }
    if (config->traceFD != STDOUT_FILENO && config->traceFD != STDERR_FILENO) {
        close(config->traceFD);
    }

    return;
}

// Check the end of a string for a word; return the length of the string without the ending word
// Used to identify the camera base name (e.g., "MEGACAM" out of "_MEGACAM-CHIP")
int checkEndForWord(const char *line,   // String to check for ending word
                    const char *word    // Ending word to check for
                    )
{
    int wlen = strlen(word);            // Length of word
    int nlen = strlen(line);            // Length of line
    if (nlen < wlen) {
        return 0;
    }

    char *ptr = (char *)line + nlen - wlen; // Expected position of ending word
    if (strcasecmp(ptr, word)) {
        return 0;
    }

    return (nlen - wlen);
}

// the camera name is of the form: BASE, BASE_CHIP, or BASE_FPA.  pull out BASE:
char *cameraBaseName(const char *name   // Name of meta-camera
                     )
{
    char *answer;

    int N = checkEndForWord (name, "-CHIP");
    if (N && name[0] == '_') {
        psString answer = psStringNCopy(name + 1, N - 1);
        return answer;
    }

    N = checkEndForWord(name, "-FPA");
    if (N && name[0] == '_') {
        psString answer = psStringNCopy(name + 1, N - 1);
        return answer;
    }

    N = checkEndForWord(name, "-SKYCELL");
    if (N && name[0] == '_') {
        psString answer = psStringNCopy(name + 1, N - 1);
        return answer;
    }

    answer = psStringCopy(name);
    return answer;
}


pmConfig *pmConfigAlloc()
{
    pmConfig *config = psAlloc(sizeof(pmConfig));
    (void)psMemSetDeallocator(config, (psFreeFunc)configFree);

    // Initialise
    config->user = NULL;
    config->site = NULL;
    config->system = NULL;
    config->camera = NULL;
    config->cameraName = NULL;
    config->format = NULL;
    config->formatName = NULL;
    config->recipes = NULL;
    config->recipesRead = PM_RECIPE_SOURCE_NONE;
    config->recipesCamera = psMetadataAlloc();
    config->recipeSymbols = psMetadataAlloc();
    config->arguments = psMetadataAlloc();
    config->database = NULL;
    config->defaultRecipe = NULL;
    config->program = NULL;

    config->traceFD = DEFAULT_TRACE;
    config->logFD = DEFAULT_LOG;

    // the file structure is used to carry pmFPAfiles
    config->files = psMetadataAlloc ();

    config->sourceId = 0;
    config->imageId = 0;
    return config;
}

// Resolve environment variables within a directory name; returns the resolved directory string.
// The returned string is likely a new pointer; the old pointer should be freed by psStringSubstitute.
static psString resolveEnvVar(psString dir // Directory to check for environment variables
                             )
{
    char *envStart;                     // Start of any environment variable
    while ((envStart = strchr(dir, '$'))) {
        char *envName = envStart + 1;   // Start of the environment variable name
        if (envName[0] == '\0') {
            psError(PM_ERR_CONFIG, true, "Path %s contains a bad environment variable.\n", dir);
            return NULL;
        }
        if (envName[0] == '{') {
            envName++;
            if (envName[0] == '\0') {
                psError(PM_ERR_CONFIG, true,
                        "Path %s contains a bad environment variable.\n", dir);
                return NULL;
            }
        }
        char *envStop = strpbrk(envStart, "}/"); // End of the environment variable
        ssize_t nameLength = envStop ? envStop - envName : strlen(envName); // Length of the name
        psString name = psStringNCopy(envName, nameLength); // The environment variable name
        char *value = getenv(name);     // Value of the environment variable
        psFree(name);
        psString valueSlash = NULL;    // Value with appended slash
        psStringAppend(&valueSlash, "%s/", value);

        ssize_t envvarLength = envStop ? envStop - envStart : strlen(envStart); // Length, w/o '}'
        psString envvar = psStringNCopy(envStart, envvarLength + 1);  // Environment variable, with $, {, }

        psTrace("psModules.config", 7, "Replacing %s with %s in directory %s\n", envvar, valueSlash, dir);
        psStringSubstitute(&dir, valueSlash, envvar);
        psFree(envvar);
        psFree(valueSlash);
    }

    return dir;
}


void pmConfigSet(const char *path)
{
    PS_ASSERT_STRING_NON_EMPTY(path,);

    assert (configPath == NULL);
    // XXX why was this being called?  pmConfigSet should only be called once...
    // pmConfigDone();

    psList *list = psStringSplit(path, ":", false);
    configPath = psListToArray(list);
    // Resolve environment variables
    for (long i = 0; i < configPath->n; i++) {
        configPath->data[i] = resolveEnvVar(configPath->data[i]);
        psTrace("psModules.config", 4, "Path %ld: %s\n", i, (char*)configPath->data[i]);
    }
    psFree(list);
}

void pmConfigDone(void)
{
    if (configPath) {
        psFree(configPath);
    }
    configPath = NULL;

    return;
}

bool pmConfigFileRead(psMetadata **config, const char *name, const char *description)
{
    assert(config);
    assert(name);
    assert(description);

    char *realName = NULL;
    unsigned int numBadLines = 0;
    struct stat filestat;

    pmErrorRegister();

    psTrace("psModules.config", 3, "Loading %s configuration from file %s\n",
            description, name);

    uid_t uid = getuid();
    gid_t gid = getgid();

    // we try: name, path[0]/name, path[1]/name, ...
    // find the first existing entry in the path (starting with the bare name)
    realName = psStringCopy (name);
    psTrace ("psModules.config", 8, "trying %s\n", realName);

    int status = stat (realName, &filestat);
    if (status == 0) {
        if ((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR)) {
            goto found;
        }
        if ((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP)) {
            goto found;
        }
        if (filestat.st_mode & S_IROTH) {
            goto found;
        }
    }
    psFree (realName);

    if (configPath == NULL) {
        psError(PM_ERR_CONFIG, true, "Cannot find %s configuration file (%s) in path\n", description, name);
        return false;
    }

    for (int i = 0; i < configPath->n; i++) {
        realName = psStringCopy (configPath->data[i]);
        psStringAppend (&realName, "/%s", name);
        psTrace ("psModules.config", 8, "trying %s\n", realName);

        status = stat (realName, &filestat);
        if (status == 0) {
            if ((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR)) {
                goto found;
            }
            if ((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP)) {
                goto found;
            }
            if (filestat.st_mode & S_IROTH) {
                goto found;
            }
        }
        psFree (realName);
    }

    psError(PM_ERR_CONFIG, true, "Cannot find %s configuration file %s in path\n", description, name);
    return false;

found:
    *config = psMetadataConfigRead(NULL, &numBadLines, realName, true);
    if (numBadLines > 0) {
        psError(PM_ERR_CONFIG, true, "%d bad lines in %s configuration file (%s)",
                numBadLines, description, realName);
        psFree (realName);

        return false;
    }
    if (!*config) {
        psError(PM_ERR_CONFIG, true, "Unable to read %s configuration from %s",
                description, realName);
        psFree (realName);
        return false;
    }

    psFree (realName);
    return true;
}

bool pmConfigFileIngest(psMetadataItem *item, const char *description)
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, false);
    PS_ASSERT_STRING_NON_EMPTY(description, false);

    if (item->type == PS_DATA_METADATA) {
        return true;                    // We've already read it
    }
    if (item->type != PS_DATA_STRING) {
        psError(PM_ERR_CONFIG, true, "Element %s in %s metadata is not of type STR.\n",
                item->name, description);
        return false;
    }

    psTrace("config", 2, "Reading %s %s: %s\n", description, item->name, item->data.str);
    psMetadata *new = NULL;         // New metadata
    if (!pmConfigFileRead(&new, item->data.str, item->name)) {
        psError(psErrorCodeLast(), false, "Trouble reading reading %s %s.\n",
                description, item->name);
        psFree(new);
        return false;
    }

    // Muck around under the hood to replace the filename with the metadata; don't try this at home, kids
    item->type = PS_DATA_METADATA;
    psFree(item->data.str);
    item->data.md = new;

    return true;
}

// Read metadata config files in a metadata
// The metadata contains file names, which will be replaced with the metadata that are in the files.
static bool metadataReadFiles(psMetadata *source, // Source metadata
                              const char *description // Description, for error messages
                             )
{
    assert(source);
    psMetadataIterator *iter = psMetadataIteratorAlloc(source, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (!pmConfigFileIngest(item, description)) {
            psError(psErrorCodeLast(), false, "Unable to read %s %s.", description, item->name);
            psFree(iter);
            return false;
        }
    }
    psFree(iter);

    return true;
}

// Read the formats for a camera
static bool cameraReadFormats(psMetadata *camera, // Camera for which to read the formats
                              const char *name // Name of the camera, for error messages
                             )
{
    assert(camera);
    assert(name);

    bool mdok;                          // Status of MD lookup
    psMetadata *formats = psMetadataLookupMetadata(&mdok, camera, "FORMATS"); // Formats
    if (!mdok || !formats) {
        psError(PM_ERR_CONFIG, true, "Unable to find FORMATS in camera configuration %s.\n", name);
        return false;
    }
    if (!metadataReadFiles(formats, "camera format")) {
        psError(psErrorCodeLast(), false, "Unable to read formats within camera configuration %s.\n", name);
        return false;
    }

    return true;
}

// Read the calibrations for a camera
static bool cameraReadCalibrations(psMetadata *camera, // Camera for which to read the formats
                                   const char *cameraName // Name of the camera, for error messages
    )
{
    assert(camera);
    assert(cameraName);

    psMetadataItem *darkNorm = psMetadataLookup(camera, "DARK.NORM"); // The dark normalisation calibration
    if (darkNorm) {
        if (!pmConfigFileIngest(darkNorm, "dark normalisation")) {
            psWarning("Unable to ingest DARK.NORM in camera %s", cameraName);
        }
    } else {
        // Add a dummy entry
        psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1); // Dummy polynomial
        poly->coeff[0] = 0.0;
        poly->coeff[1] = 1.0;
        psMetadata *polyMD = psMetadataAlloc(); // Container for the polynomial
        (void)psPolynomial1DtoMetadata(polyMD, poly, "_DEFAULT"); // Metadata to insert
        psFree(poly);
        psMetadataAddMetadata(camera, PS_LIST_TAIL, "DARK.NORM", 0, "Dark normalisation polynomial",
                              polyMD);
        psMetadataAddStr(camera, PS_LIST_TAIL, "DARK.NORM.KEY", 0, "Key for dark normalisation", "_DEFAULT");
        psFree(polyMD);
    }

    return true;
}

pmConfig *pmConfigRead(int *argc, char **argv, const char *defaultRecipe)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_INT_POSITIVE(*argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    pmConfig *config = pmConfigAlloc(); // The configuration, containing site, camera and recipes
    config->program = psStringCopy(argv[0]);
    config->defaultRecipe = defaultRecipe;

    // The following section of code attempts to determine which file to use as the
    // top-level the configuration file.  At the end of this code block, the configFile
    // variable will contain the name of the configuration file.

    char *configFile = NULL;            // Name of configuration file

    // First, try command line
    psS32 argNum = psArgumentGet(*argc, argv, "-ipprc");
    if (argNum != 0) {
        // remove the "-ipprc" argument from argv, check and remove filename
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-ipprc command-line switch provided without the required filename --- ignored.\n");
        } else {
            configFile = resolveConfigFile(argv[argNum]);
            psArgumentRemove(argNum, argc, argv);
        }
    }

    // Next, try environment variable
    if (!configFile) {
        configFile = getenv(IPPRC_ENV);
        if (configFile) {
            configFile = psStringCopy(configFile);
        }
    }

    // Last chance is ~/.ipprc
    if (!configFile) {
        char *home = getenv("HOME");
        configFile = psStringCopy(home);
        psStringAppend(&configFile, "/%s", IPPRC_FILE);
    }

    // Read and parse the config file and store in struct user.
    // XXX move this section to pmConfigReadUser.c ?
    if (!pmConfigFileRead(&config->user, configFile, "user")) {
        psFree(config);
        psFree(configFile);
        return NULL;
    }
    psFree(configFile);

    pmConfigRunCommand(config, *argc, argv);

    // define the config-file search path (configPath).
    psAssert(configPath == NULL, "Configuration path is already defined.");
    psString path = psMetadataLookupStr(NULL, config->user, "PATH");
    pmConfigSet(path);

    // read the SITE file
    psMetadataItem *siteItem = psMetadataLookup(config->user, "SITE");
    if (!siteItem) {
        psError(PM_ERR_CONFIG, true, "Unable to find SITE in user configuration.");
        psFree(config);
        return NULL;
    }
    if (!pmConfigFileIngest(siteItem, "site configuration")) {
        psError(psErrorCodeLast(), false, "Unable to read site configuration");
        psFree(config);
        return NULL;
    }
    config->site = psMemIncrRefCounter(siteItem->data.md);

    // load the SYSTEM file
    psMetadataItem *systemItem = psMetadataLookup(config->user, "SYSTEM");
    if (!systemItem) {
        psError(PM_ERR_CONFIG, true, "Unable to find SYSTEM in user configuration.");
        psFree(config);
        return NULL;
    }
    if (!pmConfigFileIngest(systemItem, "system configuration")) {
        psError(psErrorCodeLast(), false, "Unable to read system configuration");
        psFree(config);
        return NULL;
    }
    config->system = psMemIncrRefCounter(systemItem->data.md);

    // Set LOG and TRACE options based on the user configuration.  These must be set AFTER
    // the SITE and SYSTEM config files are read so path:// entries here can be resolved.
    {
        bool mdok = true;   // Status of MD lookup result

        // Set logging level
        int logLevel = psMetadataLookupS32(&mdok, config->user, "LOGLEVEL");
        if (mdok && logLevel >= 0)
        {
            psTrace("psModules.config", 7, "Setting log level to %d\n", logLevel);
            psLogSetLevel(logLevel);
        }

        // Set logging format
        psString logFormat = psMetadataLookupStr(&mdok, config->user, "LOGFORMAT");
        if (mdok && logFormat)
        {
            psTrace("psModules.config", 7, "Setting log format to %s\n", logFormat);
            psLogSetFormat(logFormat);
        }

        // Set logging destination first from command line, second from user configuration
        psString logDest = NULL;        // Logging destination
        argNum = psArgumentGet(*argc, argv, "-log");
        if (argNum > 0) {
            psArgumentRemove(argNum, argc, argv);
            if (argNum >= *argc) {
                psWarning("-log command-line switch provided without the required log destination "
                          "--- ignored.");
            } else {
                logDest = psStringCopy(argv[argNum]);
                psArgumentRemove(argNum, argc, argv);
            }
        }
        if (!logDest) {
            logDest = psMemIncrRefCounter(psMetadataLookupStr(&mdok, config->user, "LOGDEST"));
        }
        if (logDest) {
            psString resolved = pmConfigConvertFilename(logDest, config, true, false); // Resolved filename
            if (!resolved || strlen(resolved) == 0) {
                psError(psErrorCodeLast(), false, "Unable to resolve log destination: %s", logDest);
                psFree(logDest);
                return NULL;
            }
            pmConfigRunFilenameAddWrite(config, "LOG", logDest);
            config->logFD = psMessageDestination(resolved);
            psFree(resolved);
            psFree(logDest);
        }
        if (!psLogSetDestination(config->logFD)) {
            psError(PS_ERR_IO, false, "Unable to set log destination to file number %d --- ignored",
                    config->logFD);
            psFree(config);
            return NULL;
        }

        // Set trace levels
        psMetadata *trace = psMetadataLookupMetadata(&mdok, config->user, "TRACE");
        if (mdok && trace) {
            psMetadataIterator *traceIter = psMetadataIteratorAlloc(trace, PS_LIST_HEAD, NULL); // Iterator
            psMetadataItem *traceItem = NULL; // Item from MD iteration
            while ((traceItem = psMetadataGetAndIncrement(traceIter))) {
                if (traceItem->type != PS_DATA_S32) {
                    psWarning("The level for trace component %s is not of type S32 (%x)\n",
                             traceItem->name, traceItem->type);
                    continue;
                }
                psTrace("psModules.config", 7, "Setting trace level for %s to %d\n",
                        traceItem->name, traceItem->data.S32);
                (void)psTraceSetLevel(traceItem->name, traceItem->data.S32);
            }
            psFree(traceIter);
        }

        // Set trace formats
        psString traceFormat = psMetadataLookupStr(&mdok, config->user, "TRACEFORMAT");
        if (mdok && traceFormat) {
            psTrace("psModules.config", 7, "Setting trace format to %s\n", traceFormat);
            (void)psTraceSetFormat(traceFormat);
        }

        // Set trace destinations
        psString traceDest = NULL;      // Trace destination
        argNum = psArgumentGet(*argc, argv, "-tracedest");
        if (argNum > 0) {
            psArgumentRemove(argNum, argc, argv);
            if (argNum >= *argc) {
                psWarning("-tracedest command-line switch provided without the required trace destination "
                          "--- ignored.\n");
            } else {
                traceDest = psStringCopy(argv[argNum]);
                psArgumentRemove(argNum, argc, argv);
            }
        }
        if (!traceDest) {
            traceDest = psMemIncrRefCounter(psMetadataLookupStr(&mdok, config->user, "TRACEDEST"));
        }
        if (traceDest) {
            psString resolved = pmConfigConvertFilename(traceDest, config, true, false); // Resolved filename
            if (!resolved || strlen(resolved) == 0) {
                psError(psErrorCodeLast(), false, "Unable to resolve trace destination: %s", traceDest);
                psFree(traceDest);
                return NULL;
            }
            pmConfigRunFilenameAddWrite(config, "TRACE", traceDest);
            config->traceFD = psMessageDestination(resolved);
            psFree(resolved);
            psFree(traceDest);
        }
        if (!psTraceSetDestination(config->traceFD)) {
            psError(PS_ERR_IO, false, "Unable to set trace destination to file number %d --- ignored",
                    config->traceFD);
            psFree(config);
            return NULL;
        }

        // Allow command line options to override defaults for logging.
        // XXX: Is it appropriate to use the ArgVerbosity function for this?
        //   A: it removes the options from the command line.
        //   B: will the pmConfigRead function always be called on initialization.
        //
        psArgumentVerbosity(argc, argv);
        // XXX: substitute the string for the default log level for "2".
    }

    // Set the visualization levels
    // argument format is: -visual (facil) (level)
    while ((argNum = psArgumentGet(*argc, argv, "-visual"))) {
        if ( (*argc < argNum + 3) ) {
            psError(PS_ERR_IO, true, "-visual switch specified without facility and level.");
            return NULL;
        }
        psArgumentRemove(argNum, argc, argv);
        pmVisualSetLevel(argv[argNum], atoi(argv[argNum+1]));
        psArgumentRemove(argNum, argc, argv);
        psArgumentRemove(argNum, argc, argv);
    }
    if ((argNum = psArgumentGet(*argc, argv, "-visual-all"))) {
        pmVisualSetLevel(".", 10);
        psArgumentRemove(argNum, argc, argv);
    }
    if ((argNum = psArgumentGet(*argc, argv, "-visual-levels"))) {
        pmVisualPrintLevels(stdout);
        psArgumentRemove(argNum, argc, argv);
    }

    // XXX read TIME from SITE (or system?)
    {
        bool mdok = true;

        // Initialise the psLib time handling
        // XXX is this still needed / desired?
        psString timeName = psMetadataLookupStr(&mdok, config->system, "TIME");
        if (mdok && timeName) {
            psTrace("psModules.config", 7, "Initialising psTime with file %s\n", timeName);
            psTimeInit(timeName);
        }
    }

    // Set the random number generator seed
    {
        psU64 seed = 0;                 // RNG seed
        int argNum = psArgumentGet(*argc, argv, "-seed"); // Argument number
        if (argNum > 0) {
            psArgumentRemove(argNum, argc, argv);
            if (argNum >= *argc) {
                psWarning("-seed command-line switch provided without the required seed value --- ignored.");
            } else {
                char *end = NULL;       // Pointer to end of consumed string
                seed = strtoull(argv[argNum], &end, 0);
                if (strlen(end) > 0) {
                    psError(PM_ERR_CONFIG, true, "Unable to read random number generator seed: %s",
                            argv[argNum]);
                    psFree(config);
                    return NULL;
                }
                psArgumentRemove(argNum, argc, argv);
            }
        }
        pmConfigRunSeed(config, seed);
    }

    // Next, we do a similar thing for the camera configuration file.  The
    // file is read and parsed into psMetadata struct "camera".
    argNum = psArgumentGet(*argc, argv, "-camera");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-camera command-line switch provided without the required camera or filename --- "
                      "ignored.\n");
        } else {
            bool mdok = true;           // Status of MD lookup
            char *cameraName = argv[argNum]; // symbolic name of the camera

            // look for the CAMERAS list in config->system
            psMetadata *cameras = psMetadataLookupMetadata(&mdok, config->system, "CAMERAS");
            if (!cameras) {
                psError(PM_ERR_CONFIG, false, "Unable to find CAMERAS in site configuration.\n");
                psFree(config);
                return NULL;
            }

            // look for the symbolic camera name in the CAMERAS metadata
            char *cameraFile = psMetadataLookupStr(&mdok, cameras, cameraName); // The filename
            if (!cameraFile) {
                psError(PM_ERR_CONFIG, false, "%s is not listed in the site CAMERAS list\n", cameraName);
                psFree(config);
                return NULL;
            }

            // load this camera's configuration informatoin
            if (!pmConfigFileRead(&config->camera, cameraFile, "camera")) {
                psError(psErrorCodeLast(), false, "Problem reading %s", cameraName);
                psFree(config);
                return NULL;
            }
            // save the name for future uses
            config->cameraName = psStringCopy (cameraName);

            psArgumentRemove(argNum, argc, argv);

            // Read in the formats
            if (!cameraReadFormats(config->camera, cameraFile)) {
                psError(psErrorCodeLast(), false, "Unable to read formats within camera configuration %s.\n",
                        cameraFile);
                psFree(config);
                return NULL;
            }

            // Read in any camera-specific calibrations
            if (!cameraReadCalibrations(config->camera, cameraName)) {
                psError(psErrorCodeLast(), false,
                        "Unable to read calibrations within camera configuration %s.\n",
                        cameraName);
                psFree(config);
                return NULL;
            }

            psMetadataAddMetadata(cameras, PS_LIST_HEAD, cameraName, PS_META_REPLACE,
                                  "Camera specified on command line", config->camera);

            if (!pmConfigCameraSkycellVersion(config->system, cameraName)) {
                psError(psErrorCodeLast(), false,
                        "Unable to generate skycell versions of specified camera %s.\n",
                        cameraName);
                psFree(config);
                return NULL;
            }

            if (!pmConfigCameraMosaickedVersions(config->system, cameraName)) {
                psError(psErrorCodeLast(), false,
                        "Unable to generate mosaicked versions of specified camera %s.\n",
                        cameraName);
                psFree(config);
                return NULL;
            }
        }
    }

    // Read the camera configurations, if not already defined, and not turned off
    if (!config->camera && readCameraConfig) {
        bool mdok;                      // Status of MD lookup
        psMetadata *cameras = psMetadataLookupMetadata(&mdok, config->system, "CAMERAS"); // List of cameras
        if (!mdok || !cameras) {
            psError(PM_ERR_CONFIG, true, "Unable to find CAMERAS in the system configuration.\n");
            return false;
        }

        if (!metadataReadFiles(cameras, "camera configuration")) {
            psError(psErrorCodeLast(), false, "Unable to read cameras within system configuration.\n");
            psFree(config);
            return NULL;
        }

        // Now fill in the formats and calibrations
        psMetadataIterator *iter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *item;           // Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            assert(item->type == PS_DATA_METADATA);
            if (!cameraReadFormats(item->data.md, item->name)) {
                psWarning("Unable to read formats for camera %s: removed.\n", item->name);
                psErrorStackPrint(stderr, "errors from read failure\n");
                psErrorClear();
                psMetadataRemoveKey(cameras, item->name);
                continue;
            }
            if (!cameraReadCalibrations(item->data.md, item->name)) {
                psWarning("Unable to read calibrations for camera %s: removed.\n", item->name);
                psErrorStackPrint(stderr, "errors from read failure\n");
                psErrorClear();
                psMetadataRemoveKey(cameras, item->name);
                continue;
            }
        }
        psFree(iter);

        if (!pmConfigCameraSkycellVersionsAll(config->system)) {
            psError(psErrorCodeLast(), false, "Unable to generate skycell versions of cameras.\n");
            psFree(config);
            return NULL;
        }
        if (!pmConfigCameraMosaickedVersionsAll(config->system)) {
            psError(psErrorCodeLast(), false, "Unable to generate mosaicked versions of cameras.\n");
            psFree(config);
            return NULL;
        }
    }

    // Load the recipes from the camera file, if appropriate
    if(!pmConfigReadRecipes(config, PM_RECIPE_SOURCE_SYSTEM | PM_RECIPE_SOURCE_CAMERA)) {
        psError(psErrorCodeLast(), false, "Failed to read recipes from camera file");
        psFree(config);
        return NULL;
    }

    // load command-line options of the form -recipe NAME RECIPE
    pmConfigLoadRecipeArguments(argc, argv, config);

    // read in command-line options to specific recipe values
    pmConfigLoadRecipeOptions(argc, argv, config, "-D");
    pmConfigLoadRecipeOptions(argc, argv, config, "-Di");
    pmConfigLoadRecipeOptions(argc, argv, config, "-Df");
    pmConfigLoadRecipeOptions(argc, argv, config, "-Db");

    if (!pmConfigReadRecipes(config, PM_RECIPE_SOURCE_CL)) {
        psError(psErrorCodeLast(), false, "Failed to read recipes from command-line");
        psFree(config);
        return NULL;
    }

    // Look for command-line options for files to replace
    while ((argNum = psArgumentGet(*argc, argv, "-F")) > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum + 1 >= *argc) {
            psError(PM_ERR_CONFIG, true,
                    "Filerule switch (-F) provided without old and new filerule.");
            psFree(config);
            return NULL;
        }

        const char *old = argv[argNum]; // The old file, to be replaced
        psArgumentRemove(argNum, argc, argv);
        const char *new = argv[argNum]; // The new file, the replacement
        psArgumentRemove(argNum, argc, argv);

        psMetadata *cameras = psMetadataLookupMetadata(NULL, config->system, "CAMERAS"); // List of cameras
        if (!cameras) {
            psError(PM_ERR_CONFIG, false, "Unable to find CAMERAS in the site configuration.\n");
            return false;
        }

        psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *cameraItem;     // Item from iteration
        while ((cameraItem = psMetadataGetAndIncrement(camerasIter))) {
            // Silently ignore problems --- they will be caught later, because if the user wants the nominated
            // file and it's not available for that camera, then they will know.

            if (cameraItem->type != PS_DATA_METADATA) {
                psTrace("psModules.config", 2,
                        "Entry %s in CAMERAS is not of type METADATA --- ignored.", cameraItem->name);
                continue;
            }
            psMetadata *camera = cameraItem->data.md; // Camera configuration

            psMetadata *newRule = pmConfigFileRule(config, camera, new); // The rule of interest
            if (!newRule) {
                psTrace("psModules.config", 2,
                        "Unable to find filerule %s in camera %s --- ignored.", new, cameraItem->name);
                continue;
            }

            // By calling pmConfigFileRule, we've assured that the FILERULES is now a metadata
            psMetadata *filerules = psMetadataLookupMetadata(NULL, camera, "FILERULES"); // File rules
            if (!filerules) {
                psTrace("psModules.config", 2,
                        "Can't find FILERULES of type METADATA in camera %s --- ignored.", cameraItem->name);
                continue;
            }

            psMetadataAddMetadata(filerules, PS_LIST_TAIL, old, PS_META_REPLACE,
                                  "Original replaced by -F option", newRule);
        }
        psFree(camerasIter);
    }

    // Look for command-line options for files to replace
    while ((argNum = psArgumentGet(*argc, argv, "-R")) > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum + 2 >= *argc) {
            psError(PM_ERR_CONFIG, true,
                    "Filerule element switch (-R) provided without filerule element and value.");
            psFree(config);
            return NULL;
        }

        const char *rulename = argv[argNum]; // The filerule, to be modified
        psArgumentRemove(argNum, argc, argv);
        const char *element  = argv[argNum]; // The element, to be modified
        psArgumentRemove(argNum, argc, argv);
	const char *value    = argv[argNum]; // The value, to be set
	psArgumentRemove(argNum, argc, argv);

        psMetadata *cameras = psMetadataLookupMetadata(NULL, config->system, "CAMERAS"); // List of cameras
        if (!cameras) {
            psError(PM_ERR_CONFIG, false, "Unable to find CAMERAS in the site configuration.\n");
            return false;
        }

        psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *cameraItem;     // Item from iteration
        while ((cameraItem = psMetadataGetAndIncrement(camerasIter))) {
            // Silently ignore problems --- they will be caught later, because if the user wants the nominated
            // file and it's not available for that camera, then they will know.

            if (cameraItem->type != PS_DATA_METADATA) {
                psTrace("psModules.config", 2,
                        "Entry %s in CAMERAS is not of type METADATA --- ignored.", cameraItem->name);
                continue;
            }
            psMetadata *camera = cameraItem->data.md; // Camera configuration

            psMetadata *newRule = pmConfigFileRule(config, camera, rulename); // The rule of interest
            if (!newRule) {
                psTrace("psModules.config", 2,
                        "Unable to find filerule %s in camera %s --- ignored.", rulename, cameraItem->name);
                continue;
            }

            // By calling pmConfigFileRule, we've assured that the FILERULES is now a metadata
            psMetadata *filerules = psMetadataLookupMetadata(NULL, camera, "FILERULES"); // File rules
            if (!filerules) {
                psTrace("psModules.config", 2,
                        "Can't find FILERULES of type METADATA in camera %s --- ignored.", cameraItem->name);
                continue;
            }

	    // Convert newRule to have the element value requested.
	    if (!psMetadataLookupStr(NULL,newRule,element)) {
	      psTrace("psModules.config", 2,
		      "Unable to find filerule element %s in filerule %s in camera %s --- ignored.",
		      element,rulename,cameraItem->name);
	      continue;
	    }
	    psMetadataAddStr(newRule, PS_LIST_TAIL, element, PS_META_REPLACE,
			     "Original replaced by -R option", value);
	    
            psMetadataAddMetadata(filerules, PS_LIST_TAIL, rulename, PS_META_REPLACE,
                                  "Original replaced by -R option", newRule);
        }
        psFree(camerasIter);
    }

    // Look for command-line options for files to replace
    while ((argNum = psArgumentGet(*argc, argv, "-photcode-rule")) > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psError(PM_ERR_CONFIG, true,
                    "-photcode-rule provided without new rule.");
            psFree(config);
            return NULL;
        }

        psString newrule = psStringCopy(argv[argNum]); // The filerule, to be modified
        psArgumentRemove(argNum, argc, argv);

        psMetadata *cameras = psMetadataLookupMetadata(NULL, config->system, "CAMERAS"); // List of cameras
        if (!cameras) {
            psError(PM_ERR_CONFIG, false, "Unable to find CAMERAS in the site configuration.\n");
            return false;
        }

        psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *cameraItem;     // Item from iteration
        while ((cameraItem = psMetadataGetAndIncrement(camerasIter))) {
            // Silently ignore problems --- they will be caught later, because if the user wants the nominated
            // file and it's not available for that camera, then they will know.

            if (cameraItem->type != PS_DATA_METADATA) {
                psTrace("psModules.config", 2,
                        "Entry %s in CAMERAS is not of type METADATA --- ignored.", cameraItem->name);
                continue;
            }
            psMetadata *camera = cameraItem->data.md; // Camera configuration

	    psMetadataAddStr (camera, PS_LIST_TAIL, "PHOTCODE.RULE", PS_META_REPLACE, "original replaced by -photcode-rule option", newrule);
        }
	psFree(newrule);
        psFree(camerasIter);
    }

    // check for values that override DB* keywords
    argNum = psArgumentGet(*argc, argv, "-dbserver");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-dbserver command-line switch provided without the required server name --- ");
        } else {
            char *dbserver = argv[argNum]; // The camera configuration file to read
            if (!psMetadataAddStr(config->user, PS_LIST_TAIL, "DBSERVER", PS_META_REPLACE,
                                  NULL, dbserver)) {
                psWarning("Failed to overwrite .ipprc DBSERVER value");
            }

            psArgumentRemove(argNum, argc, argv);
        }
    }

    argNum = psArgumentGet(*argc, argv, "-dbname");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-dbname command-line switch provided without the required database name");
        } else {
            char *dbname = argv[argNum]; // The camera configuration file to read
            if (!psMetadataAddStr(config->user, PS_LIST_TAIL, "DBNAME", PS_META_REPLACE, NULL, dbname)) {
                psWarning("Failed to overwrite .ipprc DBNAME value");
            }

            psArgumentRemove(argNum, argc, argv);
        }
    }

    argNum = psArgumentGet(*argc, argv, "-dbuser");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-dbuser command-line switch provided without the required database name");
        } else {
            char *dbuser = argv[argNum]; // The camera configuration file to read
            if (!psMetadataAddStr(config->user, PS_LIST_TAIL, "DBUSER", PS_META_REPLACE, NULL, dbuser)) {
                psWarning("Failed to overwrite .ipprc DBUSER value");
            }

            psArgumentRemove(argNum, argc, argv);
        }
    }

    argNum = psArgumentGet(*argc, argv, "-dbpassword");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-dbpassword command-line switch provided without the required password");
        } else {
            char *dbpassword = argv[argNum]; // The camera configuration file to read
            if (!psMetadataAddStr(config->user, PS_LIST_TAIL, "DBPASSWORD", PS_META_REPLACE,
                                  NULL, dbpassword)) {
                psWarning("Failed to overwrite .ipprc DBPASSWORD value");
            }

            psArgumentRemove(argNum, argc, argv);
        }
    }

    argNum = psArgumentGet(*argc, argv, "-dbport");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-dbpport command-line switch provided without the required port number");
        } else {
            char *dbport = argv[argNum]; // The camera configuration file to read
            if (!psMetadataAddS32(config->user, PS_LIST_TAIL, "DBPORT", PS_META_REPLACE, NULL,
                                  (psS32)atoi(dbport))) {
                psWarning("Failed to overwrite .ipprc DBPORT value");
            }

            psArgumentRemove(argNum, argc, argv);
        }
    }

    argNum = psArgumentGet(*argc, argv, "-image_id");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-image_id command-line switch provided without the required id number");
        } else {
            if (!sscanf(argv[argNum], "%" PRId64, &config->imageId)) {
                psWarning("Failed to parse image_id value %s", argv[argNum]);
            }
            psArgumentRemove(argNum, argc, argv);
        }
    }

    argNum = psArgumentGet(*argc, argv, "-source_id");
    if (argNum > 0) {
        psArgumentRemove(argNum, argc, argv);
        if (argNum >= *argc) {
            psWarning("-source_id command-line switch provided without the required id number");
        } else {
            if (!sscanf(argv[argNum], "%" PRId64, &config->sourceId)) {
                psWarning("Failed to parse image_id value %s", argv[argNum]);
            }
            psArgumentRemove(argNum, argc, argv);
        }
    }

    psErrorClear();   // we may have failed to find some items in the metadata

    return config;
}


// does this header match the specified camera format?  answer is supplied to 'valid' the
// return value defines the error condition. error only on config errors
bool pmConfigValidateCameraFormat(bool *valid, const psMetadata *cameraFormat, const psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(cameraFormat, false);
    PS_ASSERT_PTR_NON_NULL(header, false);

    // Read the rule for that camera format
    bool mdStatus = true;

    psMetadata *rule = psMetadataLookupMetadata(&mdStatus, cameraFormat, "RULE");
    if (! mdStatus || ! rule) {
        psError(PM_ERR_CONFIG, false, "Unable to read rule for camera.");
        *valid = false;
        return false;
    }

    // grab the metadata items in sequence by key so we get the MULTI entry
    psList *keyList = psMetadataKeys (rule);
    psArray *keys = psListToArray (keyList);
    if (! keys) {
        psError(PM_ERR_CONFIG, false, "Unable to read rule for camera.");
        *valid = false;
        return false;
    }

    *valid = true;
    for (int i = 0; *valid && (i < keys->n); i++) {

        // get the ruleItem for this key
        psMetadataItem *ruleItem = psMetadataLookup(rule, keys->data[i]);

        // Check for the existence of the rule in the header
        psMetadataItem *headerItem = psMetadataLookup(header, ruleItem->name);
        if (! headerItem) {
            // rule item not found in header
            psTrace("psModules.config.format", 5, "Can't find %s", ruleItem->name);
            *valid = false;
            continue;
        }

        // if the RULE type is a primitive type (int, float, etc) or string compare directly
        if (PS_DATA_IS_PRIMITIVE (ruleItem->type) || (ruleItem->type == PS_DATA_STRING)) {
            // Check to see if the rule works
            if (!psMetadataItemCompare(headerItem, ruleItem)) {
                psTrace("psModules.config.format", 5, "%s doesn't match.", ruleItem->name);
                *valid = false;
            }
            continue;
        }

        // for MULTI, try each one & succeed if any match (valid = true is default state)
        if (ruleItem->type == PS_DATA_METADATA_MULTI) {
            bool found = false;
            for (int j = 0; j < ruleItem->data.list->n; j++) {
                psMetadataItem *entry = psListGet (ruleItem->data.list, j);
                assert (entry);
                if (psMetadataItemCompare(headerItem, entry)) {
                    found = true;
                    psTrace("psModules.config.format", 5, "%s in multi list matches.", ruleItem->name);
                    break;
                }
            }
            if (!found) {
                *valid = false;
                psTrace("psModules.config.format", 5, "%s doesn't match.", ruleItem->name);
            }
            continue;
        }

        psError(PM_ERR_CONFIG, false, "Invalid type for RULE %s.", ruleItem->name);
        *valid = false;
        psFree (keyList);
        psFree (keys);
        return false;
    }

    psFree (keyList);
    psFree (keys);
    return true;
}

// Given a camera and a header, see if any of the camera formats match the header
// if so, return the winning format and the name of the winning format (both allocated here)
static bool formatFromHeader(bool *status,
                             psMetadata **format, // Format to return
                             psString *name, // Name to return
                             psMetadata *camera, // Camera configuration
                             const psMetadata *header, // FITS header
                             const char *cameraName // Name of camera
                            )
{
    assert(format);
    assert(camera);
    assert(header);
    assert(cameraName);
    assert(*cameraName);

    *status = true;                     // error status
    bool result = false;                // Did we find the first match?

    // Read the list of formats
    bool mdok = true;                   // Status of MD lookup
    psMetadata *formats = psMetadataLookupMetadata(&mdok, camera, "FORMATS"); // List of formats
    if (!mdok || !formats) {
        psError(PM_ERR_CONFIG, false, "Unable to find list of FORMATS in camera %s", cameraName);
        *status = false;
        return false;
    }

    if (!metadataReadFiles(formats, "camera format")) {
        psError(psErrorCodeLast(), false, "Unable to read cameras formats within camera configuration.\n");
        *status = false;
        return false;
    }

    // Iterate over the formats
    psMetadataIterator *formatsIter = psMetadataIteratorAlloc(formats, PS_LIST_HEAD, NULL);
    psMetadataItem *formatsItem = NULL; // Item from formats
    while ((formatsItem = psMetadataGetAndIncrement(formatsIter))) {
        assert(formatsItem->type == PS_DATA_METADATA); // Since we have just read it in or deleted it
        psMetadata *testFormat = formatsItem->data.md; // Format to test against

        psTrace("psModules.config.format", 5, "trying format %s", formatsItem->name);

        bool valid = false;
        if (!pmConfigValidateCameraFormat(&valid, testFormat, header)) {
            psError (psErrorCodeLast(), false, "Error in config scripts for camera %s, format %s\n",
                     cameraName, formatsItem->name);
            *status = false;
            return false;
        }
        if (valid) {
            if (!*format) {
                psLogMsg("psModules.config.format", PS_LOG_INFO, "Camera %s, format %s matches header.\n",
                         cameraName, formatsItem->name);
                *format = psMemIncrRefCounter(testFormat);
                *name = psStringCopy(formatsItem->name);
                result = true;
            } else {
                psWarning("Camera %s, format %s also matches header --- ignored.\n",
                         cameraName, formatsItem->name);
            }
        }
    }
    psFree(formatsIter);
    *status = true;
    return result;
}

// determine the camera format based on the keywords in the header.  If we have already chosen
// a camera, then we select the formats only from that camera, and the meta-cameras for that
// camera.  If we are discovering the camera (config->camera == NULL), then we also load the
// recipe files for the camera.
psMetadata *pmConfigCameraFormatFromHeader(psMetadata **camera, psString *cameraName, psString *formatName,
                                           pmConfig *config, const psMetadata *header, bool readRecipes)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(header, NULL);

    bool status = false;                // error status
    psMetadata *format = NULL;          // The winning format
    psString testFormatName = NULL;         // Name of the winning format

    // If we don't know what sort of camera we have, we try all that we know
    if (! config->camera) {
        psAssert (!config->cameraName, "programming error: cameraName should be NULL if camera is undefined");
        psAssert (!config->format,     "programming error: format should be NULL if camera is undefined");
        psAssert (!config->formatName, "programming error: formatName should be NULL if camera is undefined");

        bool mdok;                      // Metadata lookup status
        psMetadata *cameras = psMetadataLookupMetadata(&mdok, config->system, "CAMERAS");
        if (! mdok || !cameras) {
            psError(PM_ERR_CONFIG, true, "Unable to find CAMERAS in the configuration.");
            return NULL;
        }

        if (!metadataReadFiles(cameras, "camera configuration")) {
            psError(psErrorCodeLast(), false, "Unable to read cameras within site configuration.\n");
            return NULL;
        }

        // Iterate over the cameras
        psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL);
        psMetadataItem *camerasItem = NULL; // Item from the metadata
        while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
            // Open the camera information
            psTrace("psModules.config.format", 3, "Inspecting camera %s (%s)\n", camerasItem->name, camerasItem->comment);
            assert(camerasItem->type == PS_DATA_METADATA); // It should be because we've read it in or deleted
            psMetadata *testCamera = camerasItem->data.md; // Camera to test against what we've got:
            if (formatFromHeader(&status, &format, &testFormatName, testCamera, header, camerasItem->name)) {
                config->camera = psMemIncrRefCounter(testCamera);
                config->cameraName = psStringCopy(camerasItem->name);
                config->formatName = testFormatName;
                config->format = format;
                if (camera) {
                    *camera = psMemIncrRefCounter(testCamera);  // view on value saved on config
                }
                if (formatName) {
                    *formatName = psMemIncrRefCounter(testFormatName);    // view on value saved on config
                }
                if (cameraName) {
                    *cameraName = psMemIncrRefCounter(config->cameraName);    // view on value saved on config
                }
            } else {
                if (!status) {
                    psError(psErrorCodeLast(), false, "Error reading camera config data for %s",
                            camerasItem->name);
                    return NULL;
                }
            }
        }
        psFree(camerasIter);

        // Done looking at all cameras
        if (!config->camera) {
            psError(PM_ERR_CONFIG, true, "Unable to find a camera that matches input FITS header!");
            return NULL;
        }

        // Now we have the camera, we can read the recipes
        if (readRecipes && !pmConfigReadRecipes(config, PM_RECIPE_SOURCE_CAMERA | PM_RECIPE_SOURCE_CL)) {
            psError(psErrorCodeLast(), false, "Error reading recipes from camera config for %s",
                    config->cameraName);
            return NULL;
        }
        return psMemIncrRefCounter(format); // a second copy, since the first copy sits on config->format
    }

    // we have a config with a specified camera.  However, the supplied header may not
    // correspond to this mosaic level for the camera.  We need to try the CHIP and FPA mosaic
    // versions as well as the base version

    psAssert (config->cameraName, "programming error: cameraName should not be NULL if camera is defined");
    psAssert (config->format,     "programming error: format should not be NULL if camera is defined");
    psAssert (config->formatName, "programming error: formatName should not be NULL if camera is defined");

    // the camera name is of the form: BASE, BASE_CHIP, or BASE_FPA.  pull out BASE:
    char *baseName = cameraBaseName (config->cameraName);

    bool found = false;

    psMetadata *testCamera = NULL;
    char *testCameraName = NULL;

    psMetadata *cameras = psMetadataLookupMetadata (NULL, config->system, "CAMERAS");
    psAssert (cameras, "missing CAMERAS in complete metadata");

    // try the FPA metaCamera
    if (!found) {
        testCameraName = NULL;
        psStringAppend (&testCameraName, "_%s-FPA", baseName);

        testCamera = psMetadataLookupMetadata (NULL, cameras, testCameraName);
        psAssert (testCamera, "missing %s in CAMERAS in complete metadata", testCameraName);

        bool status;
        found = formatFromHeader(&status, &format, &testFormatName, testCamera, header, testCameraName);
        if (!found) psFree (testCameraName);
    }

    // try the CHIP metaCamera
    if (!found) {
        testCameraName = NULL;
        psStringAppend (&testCameraName, "_%s-CHIP", baseName);

        testCamera = psMetadataLookupMetadata (NULL, cameras, testCameraName);
        psAssert (testCamera, "missing %s in CAMERAS in complete metadata", testCameraName);

        bool status;
        found = formatFromHeader(&status, &format, &testFormatName, testCamera, header, testCameraName);
        if (!found) psFree (testCameraName);
    }

    // try the SKYCELL metaCamera
    if (!found) {
        testCameraName = NULL;
        psStringAppend (&testCameraName, "_%s-SKYCELL", baseName);

        testCamera = psMetadataLookupMetadata (NULL, cameras, testCameraName);
        psAssert (testCamera, "missing %s in CAMERAS in complete metadata", testCameraName);

        bool status;
        found = formatFromHeader(&status, &format, &testFormatName, testCamera, header, testCameraName);
        if (!found) psFree (testCameraName);
    }

    // try the base name
    if (!found) {
        testCameraName = psMemIncrRefCounter (baseName);

        testCamera = psMetadataLookupMetadata (NULL, cameras, testCameraName);
        psAssert (testCamera, "missing %s in CAMERAS in complete metadata", testCameraName);

        bool status;
        found = formatFromHeader(&status, &format, &testFormatName, testCamera, header, testCameraName);
    }

    if (!found) {
        psError(PM_ERR_CONFIG, true,
                "Unable to find a format with the specified camera (%s) that matches the given header.",
                baseName);
        psFree (baseName);
        return NULL;
    }

    psFree (baseName);
    if (formatName) {
        *formatName = testFormatName;
    } else {
        psFree (testFormatName);
    }
    if (cameraName) {
        *cameraName = testCameraName;
    } else {
        psFree (testCameraName);
    }

    if (camera) {
        *camera = psMemIncrRefCounter(testCamera);
    }
    return format; // we do NOT need to incr ref counter: this is the only copy
}

// Return the requested camera configuration
psMetadata *pmConfigCameraByName(pmConfig *config, const char *cameraName)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(cameraName, NULL);

    psMetadata *cameras = psMetadataLookupMetadata(NULL, config->system, "CAMERAS");
    if (!cameras) {
        psError(PM_ERR_CONFIG, true, "Unable to find CAMERAS in the configuration.");
        return NULL;
    }

    psMetadataItem *item = psMetadataLookup(cameras, cameraName); // Item with camera of interest
    if (!pmConfigFileIngest(item, "camera configuration")) {
        psError(psErrorCodeLast(), false, "Unable to ingest camera configuration.");
        return NULL;
    }

    return psMemIncrRefCounter(item->data.md);
}

psMetadataItem *pmConfigUserSite(const pmConfig *config, const char *name, psDataType type)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    psMetadataItem *item = psMetadataLookup(config->user, name);
    if (!item) {
        item = psMetadataLookup(config->site, name);
        if (!item) {
            psError(PM_ERR_CONFIG, true,
                    "Unable to find %s in user or site configuration", name);
            return NULL;
        }
    }
    if (item->type != type) {
        psError(PM_ERR_CONFIG, true,
                "Type of %s (%x) in user/site configuration does not match expected (%x)",
                name, item->type, type);
        return NULL;
    }

    return item;
}


psDB *pmConfigDB(pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(config->user, NULL);

#ifndef HAVE_PSDB

    psError(PM_ERR_PROG, false,
            "Cannot configure database: psModules was compiled without database support.");
    return NULL;

#else

    if (config->database) {
        return config->database;
    }

    // Connection details
    psMetadataItem *server = pmConfigUserSite(config, "DBSERVER",   PS_DATA_STRING);
    psMetadataItem *user   = pmConfigUserSite(config, "DBUSER",     PS_DATA_STRING);
    psMetadataItem *pass   = pmConfigUserSite(config, "DBPASSWORD", PS_DATA_STRING);
    psMetadataItem *name   = pmConfigUserSite(config, "DBNAME",     PS_DATA_STRING);
    psMetadataItem *port   = pmConfigUserSite(config, "DBPORT",     PS_TYPE_S32);

    if (!server || !user || !pass || !name) {
        psWarning("Cannot find DBSERVER/DBUSER/DBPASSWORD/DBNAME in user or site configuration: "
                  "unable to connect to database.");
        psErrorClear();
        return NULL;
    }
    if (!port) {
        psTrace("psModules.config", 1, "Database port defaulting to 0");
        psErrorClear();
    }

    if (strcasecmp(name->data.str, "XXX") == 0 || strcasecmp(name->data.str, "NONE") == 0) {
        psTrace("psModules.config", 1, "Database initialisation skipped: database is %s.", name->data.str);
        return NULL;
    }

    config->database = psDBInit(server->data.str, user->data.str, pass->data.str, name->data.str,
                                port ? port->data.S32 : 0);
    return config->database;

#endif
}


bool pmConfigConformHeader(psMetadata *header, const psMetadata *format)
{
    PS_ASSERT_PTR_NON_NULL(header, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    bool mdok = true;                   // Status of MD lookup
    psMetadata *rules = psMetadataLookupMetadata(&mdok, format, "RULE"); // How to identify this format
    if (!mdok || !rules) {
        psError(PM_ERR_CONFIG, true, "Unable to find RULE in camera format.\n");
        return false;
    }

    psMetadataIterator *rulesIter = psMetadataIteratorAlloc(rules, PS_LIST_HEAD, NULL); // Iterator for rules
    psMetadataItem *rulesItem = NULL;   // Item from iteration
    while ((rulesItem = psMetadataGetAndIncrement(rulesIter))) {
        if (!PS_DATA_IS_PRIMITIVE(rulesItem->type) && rulesItem->type != PS_DATA_STRING) {
            psError(PM_ERR_CONFIG, false, "Invalid type for RULE %s.", rulesItem->name);
            return false;
        }

        psMetadataItem *hdrItem = psMetadataLookup(header, rulesItem->name); // Item from header
        if (hdrItem && psMetadataItemCompare(hdrItem, rulesItem)) {
            // It's already there and matches
            continue;
        }

        // Look for an operation
        psMetadataItemCompareOp op = psMetadataItemCompareOperation(rulesItem);
        switch (op) {
          case PS_METADATA_ITEM_COMPARE_OP_NONE:
          case PS_METADATA_ITEM_COMPARE_OP_EQ:
          case PS_METADATA_ITEM_COMPARE_OP_LE:
          case PS_METADATA_ITEM_COMPARE_OP_GE: {
              // A comparison involving equality: add the value
              psMetadataItem *newItem = psMetadataItemCopy(rulesItem); // Copy of item
              if (op == PS_METADATA_ITEM_COMPARE_OP_LE || op == PS_METADATA_ITEM_COMPARE_OP_GE) {
                  // Not clear what the value is supposed to be, so add a warning as well
                  psFree(newItem->comment);
                  newItem->comment = psStringCopy("MAY BE WRONG: added to match format rule");
              }
              psMetadataAddItem(header, newItem, PS_LIST_TAIL, PS_META_REPLACE);
              psFree(newItem);                // Drop reference
              break;
          }
          case PS_METADATA_ITEM_COMPARE_OP_LT:
          case PS_METADATA_ITEM_COMPARE_OP_GT:
          case PS_METADATA_ITEM_COMPARE_OP_NE:
            // It's not at all obvious what the value should be, so return an error.
            psError(PM_ERR_CONFIG, true,
                    "RULE %s (defined by an OPeration) is not present or not consistent in output header",
                    rulesItem->name);
            return false;
          default:
            psAbort("Unknown operation: %x", op);
        }
    }
    psFree(rulesIter);

    return true;
}

psArray *pmConfigFileSets(int *argc, char **argv, const char *file, const char *list)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_INT_NONNEGATIVE(*argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    int Narg;                           // Argument number

    // we load all input files onto a psArray, to be parsed later
    psArray *input = psArrayAllocEmpty(16);

    // load the list of filenames the supplied file
    // maybe a comma-separated list of words
    // each word may be a glob: "file*.fits"
    if (file && strlen(file) > 0 && (Narg = psArgumentGet (*argc, argv, file))) {

        // select the word after 'file' and split by comma
        psArgumentRemove (Narg, argc, argv);
        psArray *words = psStringSplitArray (argv[Narg], ",", true);
        psArgumentRemove (Narg, argc, argv);

        // parse the word as a glob
        glob_t globList;
        for (int i = 0; i < words->n; i++) {
            globList.gl_offs = 0;
            glob (words->data[i], 0, NULL, &globList);

            // if the glob does not match, save the literal word:
            // otherwise save all glob matches
            if (globList.gl_pathc == 0) {
                psArrayAdd (input, 16, words->data[i]);
            } else {
                for (int j = 0; j < globList.gl_pathc; j++) {
                    char *filename = psStringCopy (globList.gl_pathv[j]);
                    psArrayAdd (input, 16, filename);
                    psFree (filename);
                }
            }
            globfree(&globList);
        }
        psFree (words);
    }

    // load the list from the supplied text file
    if (list && strlen(list) > 0 && (Narg = psArgumentGet(*argc, argv, list))) {
        psArgumentRemove (Narg, argc, argv);
        FILE *f = fopen(argv[Narg], "r");
        if (!f) {
            psError(PS_ERR_IO, true, "Unable to open specified list file");
            psFree(input);
            return NULL;
        }

        // XXX Reading the list should be reimplemented using psSlurp

        char line[1024]; // XXX limits the list lines to 1024 chars
        while (fgets(line, 1024, f) != NULL) {
            char word[1024];
            int nItems = sscanf(line, "%s", word);
            switch (nItems) {
              case 0:
                break;
              case 1: {
                  psString filename = psStringCopy(word);
                  psArrayAdd(input, 16, filename);
                  psFree(filename);
                  break;
              }
              default:
                // rigid format, no comments allowed?
                psError(PM_ERR_CONFIG, true, "Unable to parse file list: spaces detected.");
                psFree(input);
                fclose(f);
                return NULL;
            }
        }
        psArgumentRemove(Narg, argc, argv);
        fclose(f);
    }

    return input;
}

// XXX this is a prime example of the failing of our error-handling system.  this function has
// three possible outcomes: the argument was found, it was not found, or we raised an error.
// returning only the bool does not distinguish failure to find the argument from a deeper
// error.  requiring the calling function to both test the bool AND trap the error stack is
// fragile: the error stack may not have been cleared, or they may not do both.  in some
// places, we solve this by returning two types of boolean status values.  a better option
// might be to return a psErrorCode value (as RHL proposed), which would be 0 on success and
// any of several options on failure.

bool pmConfigFileSetsMD(psMetadata *metadata, int *argc, char **argv, const char *name,
                        const char *file, const char *list)
{
    PS_ASSERT_PTR_NON_NULL(metadata, false);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    psErrorClear();   // pmConfigFileSets may or may not call psError, so
    // if files->n == 0 we'll want to call psError(..., false, ...)
    psArray *files = pmConfigFileSets(argc, argv, file, list);
    if (!files) {
        psAbort("error parsing argument list");
        psError(psErrorCodeLast(), false, "error parsing argument list");
        psFree (files);
        return false;
    }

    // no files found: this is not really an error
    if (files->n == 0) {
        psFree (files);
        return false;
    }

    psMetadataAddPtr(metadata, PS_LIST_TAIL, name,  PS_DATA_ARRAY, "", files);
    psFree (files);
    return true;
}

// convert the supplied name, create a new output psString
psString pmConfigConvertFilename(const char *filename, const pmConfig *config, bool create, bool trunc)
{
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // strip file:// from front of name
    if (!strncasecmp(filename, "file:", strlen("file:"))) {
        psString newName = psStringCopy(filename);

        char *point = newName + strlen("file:");
        while (*point == '/') {
            point ++;
        }
        char *tmpName = NULL;
        psStringAppend (&tmpName, "/%s", point);
        psFree (newName);
        newName = tmpName;

        if (!checkPath(newName, create, trunc)) {
            // let checkPath()'s psError() call float up
            psError(psErrorCodeLast(), false, "error from checkPath for file:// (%s)", newName);
            psFree (newName);
            return NULL;
        }

        return newName;
    }

    // replace path://PATH with matched datapath
    if (!strncasecmp(filename, "path://", strlen("path://"))) {
        PS_ASSERT_METADATA_NON_NULL(config->site, NULL);

        psString newName = psStringCopy(filename);

        // filename should be of the form: path://PATH/rest/of/file
        // replace PATH with matching name from config->site:DATAPATH
        psMetadata *datapath = psMetadataLookupPtr (NULL, config->site, "DATAPATH");
        if (datapath == NULL) {
            psError(PM_ERR_CONFIG, true, "DATAPATH is not defined in config.site");
            psFree (newName);
            return NULL;
        }

        char *point = newName + strlen("path://");
        char *mark = strchr (point, '/');
        if (mark == NULL) {
            psError(PM_ERR_CONFIG, true, "syntax error in PATH-style name %s", newName);
            psFree (newName);
            return false;
        }

        psString path = psStringNCopy (point, mark - point);
        char *realpath = psMetadataLookupStr (NULL, datapath, path);
        if (realpath == NULL) {
            psError(PM_ERR_CONFIG, true,
                    "path (%s) not defined in config.site:DATAPATH for PATH-style name %s",
                    path, newName);
            psFree(newName);
            psFree(path);
            return false;
        }
        psFree(path);

        char *tmpName = NULL;
        psStringAppend(&tmpName, "%s/%s", realpath, mark + 1);
        psFree(newName);
        newName = tmpName;

        if (!checkPath(newName, create, trunc)) {
            // let checkPath()'s psError() call float up
            psError(psErrorCodeLast(), false, "error from checkPath for path:// (%s)", newName);
            psFree (newName);
            return NULL;
        }

        return newName;
    }

    // substitute neb://name with matched nebulous name
    if (!strncasecmp(filename, "neb://", strlen("neb://"))) {
        #ifdef HAVE_NEBCLIENT

        bool status = false;
        psString neb_server = NULL;

        // check the env first
        neb_server = getenv("NEB_SERVER");

        // if env isn't set, check the config system
        if (!neb_server) {
            neb_server = psMetadataLookupStr(&status, config->site, "NEB_SERVER");
            if (!status) {
                psError(PM_ERR_CONFIG, true, "failed to lookup config value for NEB_SERVER.");
                return NULL;
            }
        }

        if (!neb_server) {
            psError(PM_ERR_CONFIG, true, "Could not determine nebulous server URI.");
            return NULL;
        }

        nebServer *server = nebServerAlloc(neb_server);
        if (!server) {
            psError(PM_ERR_SYS, true, "failed to create a nebServer object.");
            return NULL;
        }

        char *nebfile = NULL;
        if (!(nebfile = nebFind(server, filename))) {
            // object does not exist
            if (create) {
                nebfile = nebCreate(server, filename, NULL, NULL);
                if (!nebfile) {
                    psError(PM_ERR_SYS, true, "failed to create a new nebulous key: %s", nebErr(server));
                    nebServerFree(server);
                    return NULL;
                }
            } else {
                // if the object does not exist and create isn't set, then we
                // should puke
                psError(PM_ERR_SYS, true, "Unable to access file %s: %s", filename, nebErr(server));
                nebServerFree(server);
                return NULL;
            }
        }

        // convert nebfile into a psString
        psString path = psStringCopy(nebfile);
        nebFree(nebfile);
        nebServerFree(server);

        // Check to ensure it's there.  Will create the file if Nebulous failed to do so.
        if (!checkPath(path, create, trunc)) {
            psError(psErrorCodeLast(), false, "Cannot find file %s", path);
            psFree(path);
            return NULL;
        }

        return path;

        #else // ifdef HAVE_NEBCLIENT

        psError(PM_ERR_PROG, true, "psModules was compiled without nebulous support.");
        return NULL;
        #endif // ifdef HAVE_NEBCLIENT

    }

    // if we go this far, do nothing
    return psStringCopy(filename);
}

psMetadata *pmConfigFileRule(const pmConfig *config, const psMetadata *camera, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_METADATA_NON_NULL(camera, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    psMetadataItem *item = psMetadataLookup(camera, "FILERULES"); // Item with the file rule of interest
    if (!item) {
        psError(PM_ERR_CONFIG, true, "Unable to find FILERULES in the camera configuration.");
        return NULL;
    }

    if (!pmConfigFileIngest(item, "file rules ")) {
        psError(PM_ERR_CONFIG, false, "Unable to read file rules for camera.");
        return NULL;
    }

    assert(item->type == PS_DATA_METADATA);
    psMetadata *filerules = item->data.md; // File rules from the camera configuration

    // select the name from the FILERULES
    // check for alias name (type == STR, name is aliased name)
    bool mdok;                          // Status of MD lookup
    const char *realname = psMetadataLookupStr(&mdok, filerules, name); // Name of file rule to look up
    if (!realname || strlen(realname) == 0) {
        realname = name;
    }

    return psMetadataLookupMetadata(&mdok, filerules, realname);
}

psMetadata *pmConfigFitsType (const pmConfig *config, const psMetadata *camera, const char *fitsType)
{
    bool mdok;

    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_METADATA_NON_NULL(camera, NULL);
    PS_ASSERT_STRING_NON_EMPTY(fitsType, NULL);

    psMetadataItem *item = psMetadataLookup(camera, "FITSTYPES"); // Item with the file rule of interest
    if (!item) {
        psError(PM_ERR_CONFIG, false, "Unable to find FITSTYPES in the camera configuration.");
        return NULL;
    }

    if (!pmConfigFileIngest(item, "FITS Types")) {
        psError(PM_ERR_CONFIG, false, "Unable to read fits types for camera.");
        return NULL;
    }

    assert(item->type == PS_DATA_METADATA);
    psMetadata *fitstypes = item->data.md; // FITS Types from the camera configuration

    // select the name from the FITSTYPES
    psMetadata *scheme = psMetadataLookupMetadata(&mdok, fitstypes, fitsType);
    if (!scheme) {
        psWarning("Unable to find specified FITS Type %s in camera configuration.", fitsType);
        return NULL;
    }

    return scheme;
}

static bool checkPath(const char *filename, bool create, bool trunc)
{
    PS_ASSERT_PTR_NON_NULL(filename, false);

    // re-try access up to 5 times (1.25sec) to reduce NFS lurches
    for (int i = 0; i < CHECK_FILE_RETRY; i++) {
        if (access(filename, R_OK) == 0) {
            // file already exists
            if (trunc) {
                if(truncate(filename, 0) != 0) {
                    psError(PS_ERR_IO, true, "Failed to truncate file, %s\n", filename);
                    return false;
                }
            }
            return true;
        }

        // file does not exist
        if (create) {
            int fd = open(filename, O_WRONLY|O_CREAT, 0666);
            if (fd == 0) {
                psError(PS_ERR_IO, true, "Failed to open & create file, %s\n", filename);
                return false;
            }
            if (close(fd) != 0) {
                psError(PS_ERR_IO, true, "Failed to close file, %s\n", filename);
                return false;
            }
            return true;
        }
        usleep(CHECK_FILE_WAIT);
    }

    // We've tried 5 times to access the file; give up and report a problem.  If the file does
    // not exist and create isn't set, then we should puke
    psError(PS_ERR_IO, true, "Unable to access file %s", filename);
    return false;
}

static psString resolveConfigFile(const char *nameArg)
{
    // if config file name is nebulous path resolve it
    // otherwise just return a copy of the argument
    if (strncasecmp(nameArg, "neb://", strlen("neb://"))) {
        return psStringCopy(nameArg);
    }

#ifdef HAVE_NEBCLIENT
    char *neb_server = getenv("NEB_SERVER");

    // if env isn't set, check the config system
    if (!neb_server) {
        psError(PM_ERR_CONFIG, true, "NEB_SERVER environment variable must be set in order to resolve config file.");
            return NULL;
    }

    nebServer *server = nebServerAlloc(neb_server);
    if (!server) {
        psError(PM_ERR_SYS, true, "failed to create a nebServer object.");
        return NULL;
    }

    char *nebfile = nebFind(server, nameArg);
    nebServerFree(server);
    if (!nebfile) {
        // object does not exist
        psError(PM_ERR_SYS, true, "failed to resolve nebulous path: %s.", nameArg);
        return NULL;
    }
    // XXX: do I need to free nebfile?

    return psStringCopy(nebfile);
#else
    psError(PM_ERR_PROG, true, "psModules was compiled without nebulous support.");
    return NULL;
#endif // ifdef HAVE_NEBCLIENT
}

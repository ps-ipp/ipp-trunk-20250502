#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmVersion.h"
#include "pmConcepts.h"
#include "pmConfigCamera.h"

#define TABLE_OF_CONTENTS "CONTENTS"    // Name for camera format metadata containing the contents
#define CHIP_TYPES "CHIPS"              // Name for camera format metadata containing the chip types
#define CELL_TYPES "CELLS"              // Name for camera format metadata containing the cell types

// local helper functions defined below
static void removeCellConceptsSources(psMetadata *source);
static void removeChipConceptsSources(psMetadata *source);

psString pmConfigCameraRootName(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    if (name[0] != '_') {
        // It's an original
        return psStringCopy(name);
    }

    psString root = psStringCopy(name + 1); // Camera name
    int length = strlen(name);                     // Length of camera name
    if (strcmp(root + length - 9, "-SKYCELL") == 0) {
        length -= 9;
    } else if (strcmp(root + length - 6, "-CHIP") == 0) {
        length -= 6;
    } else if (strcmp(root + length - 5, "-FPA") == 0) {
        length -= 5;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised derivative camera: %s", name);
        psFree(root);
        return NULL;
    }

    // Truncate the string
    root[length] = '\0';

    return root;
}

psString pmConfigCameraSkycellName(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    psString root = pmConfigCameraRootName(name); // Root name of camera
    if (!root) {
        return NULL;
    }

    psStringAppend(&root, "-SKYCELL");
    psStringPrepend(&root, "_");

    return root;
}

psString pmConfigCameraChipName(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    psString root = pmConfigCameraRootName(name); // Root name of camera
    if (!root) {
        return NULL;
    }

    psStringAppend(&root, "-CHIP");
    psStringPrepend(&root, "_");

    return root;
}

psString pmConfigCameraFPAName(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    psString root = pmConfigCameraRootName(name); // Root name of camera
    if (!root) {
        return NULL;
    }

    psStringAppend(&root, "-FPA");
    psStringPrepend(&root, "_");

    return root;
}

// Generate the skycell version of a named camera configuration
bool pmConfigCameraSkycellVersion(psMetadata *system, // The system configuration
                                  const char *name // Name of the un-mosaicked camera
                                  )
{
    PS_ASSERT_METADATA_NON_NULL(system, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    bool mdok;                          // Status of MD lookup
    psMetadata *cameras = psMetadataLookupMetadata(&mdok, system, "CAMERAS"); // List of cameras
    if (!mdok || !cameras) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find CAMERAS in the system configuration.\n");
        return false;
    }
    if (!pmConfigGenerateSkycellVersion(cameras, cameras, name, system)) {
        psError(PS_ERR_UNKNOWN, true, "Failed to build skycell camera description for %s\n", name);
        return false;
    }
    return true;
}


bool pmConfigCameraSkycellVersionsAll(psMetadata *system)
{
    PS_ASSERT_METADATA_NON_NULL(system, false);

    bool mdok;                          // Status of MD lookup
    psMetadata *cameras = psMetadataLookupMetadata(&mdok, system, "CAMERAS"); // List of cameras
    if (!mdok || !cameras) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find CAMERAS in the system configuration.\n");
        return false;
    }

    psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *camerasItem = NULL; // Item from iteration
    psMetadata *new = psMetadataAlloc();// New cameras to add
    while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
        assert(camerasItem->type == PS_DATA_METADATA); // Only metadata are allowed here!
        if (!pmConfigGenerateSkycellVersion(cameras, new, camerasItem->name, system)) {
            psError(PS_ERR_UNKNOWN, true, "Failed to build skycell camera description for %s\n",
                    camerasItem->name);
            return false;
        }
    }
    psFree(camerasIter);

    // Now put the new cameras at the top of the list of cameras, so they get recognised first
    // Note: going from the top, and putting everything to the top as we get there, so that the last one on
    // goes to the top.  This preserves the original order of the cameras, putting the skycell versions
    // before the originals.
    camerasIter = psMetadataIteratorAlloc(new, PS_LIST_HEAD, NULL); // Iterator
    while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
        psMetadataAddItem(cameras, camerasItem, PS_LIST_HEAD, PS_META_REPLACE);
    }
    psFree(camerasIter);
    psFree(new);

    return true;
}

// Don't update these skycell concepts; last one MUST be 0 (i.e., NULL).
const static char *skycellConceptsCell[] = { "CELL.BIASSEC", "CELL.TRIMSEC", "CELL.READDIR", "CELL.XPARITY",
                                             "CELL.YPARITY", "CELL.X0", "CELL.Y0", "CELL.TIMESYS", 0 };
const static char *skycellConceptsChip[] = { "CHIP.XPARITY", "CHIP.YPARITY", 0 };
const static char *skycellConceptsFPA[] = { "FPA.TIMESYS" };

// What do we call the skycell concept in the FITS header?
static const char *skycellConceptName(const char *name, // Name of concept
                                      const char **concepts, // List of concepts NOT to update
                                      const psMetadata *system // System configuration
                                      )
{
    for (int i = 0; concepts[i]; i++) {
        if (strcmp(name, concepts[i]) == 0) {
            return NULL;
        }
    }

    if (!system) {
        return name;
    }
    bool mdok;                          // Status of MD lookup
    psMetadata *skycells = psMetadataLookupMetadata(&mdok, system, "SKYCELLS"); // Skycell concept headers
    if (!skycells) {
        return name;
    }
    const char *keyword = psMetadataLookupStr(&mdok, skycells, name); // Keyword to use for this concept
    if (!mdok || !keyword || strlen(keyword) == 0) {
        return name;
    }
    return keyword;
}


// Generate a skycell version of a camera configuration
bool pmConfigGenerateSkycellVersion(psMetadata *oldCameras, // Old list of camera configurations
                                    psMetadata *newCameras, // New list of camera configurations
                                    const char *name, // Name of original camera configuration
                                    const psMetadata *system // System configuration
                                    )
{
    assert(oldCameras);
    assert(newCameras);
    assert(name);

    // See if the old one is there
    psMetadata *camera = psMetadataLookupMetadata(NULL, oldCameras, name); // The camera configuration
    if (!camera) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Can't find camera to be skycelled in camera list.");
        return false;
    }

    // See if the new one is already there
    psString newName = pmConfigCameraSkycellName(name); // Name of skycelled camera
    bool mdok;                       // Status of MD lookup
    psMetadata *oldCam = psMetadataLookupMetadata(&mdok, oldCameras, newName); // Existing camera configuration
    if (mdok && oldCam) {
        // Ensure new camera goes to the head of the metadata, so that it will be recognised first
        // The old camera doesn't contain the PSMOSAIC header, so it will match anything!
        psTrace("psModules.config", 6, "Camera configuration for %s exists, so moving to the front.", newName);
        psMetadataAddMetadata(newCameras, PS_LIST_HEAD, newName, PS_META_REPLACE, NULL, oldCam);
        psFree(newName);
        return true;
    }

    psMetadata *new = psMetadataCopy(NULL, camera); // Copy of the camera description

    // Fix the FPA description to contain a single chip with single cell
    {
        psMetadata *fpa = psMetadataAlloc();// The FPA description
        psMetadataAddStr(fpa, PS_LIST_HEAD, "SkyChip", 0, "Single chip with single cell", "SkyCell");
        psMetadataAddMetadata(new, PS_LIST_TAIL, "FPA", PS_META_REPLACE, "Description of FPA hierarchy", fpa);
        psFree(fpa);
    }

    // Clear out the formats, replace them with the One True Format
    psMetadata *formats = psMetadataLookupMetadata(NULL, new, "FORMATS"); // The list of formats
    if (!formats) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Can't find FORMATS within camera configuration.");
        psFree(new);
        return false;
    }
    while (psListLength(formats->list) > 0) {
        psMetadataRemoveIndex(formats, PS_LIST_HEAD);
    }
    psMetadata *format = psMetadataAlloc(); // The One True Format

    {
        psMetadata *rule = psMetadataAlloc(); // The RULE --- how to recognise the camera
        psMetadataAddStr(rule, PS_LIST_TAIL, "PSCAMERA", 0, "Camera name", name);
        psMetadataAddStr(rule, PS_LIST_TAIL, "PSFORMAT", 0, "Camera format", "SKYCELL");
        psMetadataAddMetadata(format, PS_LIST_TAIL, "RULE", 0, "How to recognise this type of file", rule);
        psFree(rule);
    }

    {
        psMetadata *file = psMetadataAlloc(); // The FILE --- how to read the data
        psMetadataAddStr(file, PS_LIST_TAIL, "PHU", 0, "What level the FITS file represents", "FPA");
        psMetadataAddStr(file, PS_LIST_TAIL, "EXTENSIONS", 0, "What level the extensions represent", "NONE");
        psMetadataAddStr(file, PS_LIST_TAIL, "FPA.OBS", 0, "PHU keyword for unique identifier", "FPA.OBS");
        psMetadataAddMetadata(format, PS_LIST_TAIL, "FILE", 0, "How to read this type of file", file);
        psFree(file);
    }

    psMetadataAddStr(format, PS_LIST_TAIL, "CONTENTS", 0, "What's in this type of file",
                     "SkyChip:SkyCell:_skycell");

    {
        psMetadata *cells = psMetadataAlloc(); // The CELLS --- how to read the cells
        psMetadata *skycell = psMetadataAlloc(); // How to read the skycell
        psMetadataAddStr(skycell, PS_LIST_TAIL, "CELL.TRIMSEC", 0, "Trim section", "CELL.TRIMSEC");
        psMetadataAddStr(skycell, PS_LIST_TAIL, "CELL.BIASSEC", 0, "Bias section", "CELL.BIASSEC");
        psMetadataAddStr(skycell, PS_LIST_TAIL, "CELL.TRIMSEC.SOURCE", 0, "Source for trim section",
                         "HEADER");
        psMetadataAddStr(skycell, PS_LIST_TAIL, "CELL.BIASSEC.SOURCE", 0, "Source for bias section",
                         "HEADER");
        psMetadataAddMetadata(cells, PS_LIST_TAIL, "_skycell", 0, "Skycell specification", skycell);
        psFree(skycell);
        psMetadataAddMetadata(format, PS_LIST_TAIL, "CELLS", 0, "How to read the cells", cells);
        psFree(cells);
    }

    // Stuffing all concepts into the header, by their PS concept name (e.g., "FPA.AIRMASS").
    // (HIERARCH will take care of the long names, implemented in psLib.)
    // Some people may not like this, but it's quick and easy and will do for now.
    // An alternative may be provided later.
    {
        psMetadata *translation = psMetadataAlloc(); // The TRANSLATION --- how to read the FITS headers

        psMetadata *concepts;           // List of concepts for each level
        psMetadataIterator *iter;       // Iterator for concepts
        psMetadataItem *item;           // Concept specification item, from iteration

        concepts = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // FPA-level concepts
        iter = psMetadataIteratorAlloc(concepts, PS_LIST_HEAD, NULL);
        while ((item = psMetadataGetAndIncrement(iter))) {
            const char *new = skycellConceptName(item->name, skycellConceptsFPA, system); // Name for skycell
            if (new) {
                psMetadataAddStr(translation, PS_LIST_TAIL, item->name, 0, NULL, new);
            }
        }
        psFree(iter);

        concepts = pmConceptsSpecs(PM_FPA_LEVEL_CHIP);
        iter = psMetadataIteratorAlloc(concepts, PS_LIST_HEAD, NULL);
        while ((item = psMetadataGetAndIncrement(iter))) {
            const char *new = skycellConceptName(item->name, skycellConceptsChip, system); // Name for skycell
            if (new) {
                psMetadataAddStr(translation, PS_LIST_TAIL, item->name, 0, NULL, new);
            }
        }
        psFree(iter);

        concepts = pmConceptsSpecs(PM_FPA_LEVEL_CELL);
        iter = psMetadataIteratorAlloc(concepts, PS_LIST_HEAD, false);
        while ((item = psMetadataGetAndIncrement(iter))) {
            const char *new = skycellConceptName(item->name, skycellConceptsCell, system); // Name for skycell
            if (new) {
                psMetadataAddStr(translation, PS_LIST_TAIL, item->name, 0, NULL, new);
            }
        }
        psFree(iter);

        psMetadataAddMetadata(format, PS_LIST_TAIL, "TRANSLATION", 0, "How to translate the FITS headers",
                              translation);
        psFree(translation);
    }

    {
        psMetadata *defaults = psMetadataAlloc(); // Default values for concepts

        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.XPARITY", 0, NULL, 1);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.YPARITY", 0, NULL, 1);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.XPARITY", 0, NULL, 1);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.YPARITY", 0, NULL, 1);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.Y0", 0, NULL, 0);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.X0", 0, NULL, 0);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.Y0", 0, NULL, 0);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.X0", 0, NULL, 0);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.READDIR", 0, "Read direction (rows)", 1);
        psMetadataAddStr(defaults, PS_LIST_TAIL, "CELL.TIMESYS", 0, "Time system", "TAI");
        psMetadataAddStr(defaults, PS_LIST_TAIL, "FPA.TIMESYS", 0, "Time system", "TAI");

        psMetadataAddMetadata(format, PS_LIST_TAIL, "DEFAULTS", 0, "Default values for concepts", defaults);
        psFree(defaults);

    }

    {
        psMetadata *database = psMetadataAlloc(); // Database values for concepts
        psMetadataAddMetadata(format, PS_LIST_TAIL, "DATABASE", 0, "Database values for concepts", database);
        psFree(database);
    }

    {
        psMetadata *conceptFormats = psMetadataAlloc(); // Format peculiarities for various concepts
        // These are the only essential formats
        psMetadataAddStr(conceptFormats, PS_LIST_TAIL, "FPA.RA", 0, "Units for RA", "HOURS");
        psMetadataAddStr(conceptFormats, PS_LIST_TAIL, "FPA.DEC", 0, "Units for RA", "DEGREES");
        psMetadataAddStr(conceptFormats, PS_LIST_TAIL, "FPA.TIME", 0, "Format for time", "MJD");
        psMetadataAddStr(conceptFormats, PS_LIST_TAIL, "CELL.TIME", 0, "Format for time", "MJD");
        psMetadataAddStr(conceptFormats, PS_LIST_TAIL, "FPA.LONGITUDE", 0, "Units for longitude", "HOURS");
        psMetadataAddStr(conceptFormats, PS_LIST_TAIL, "FPA.LATITUDE", 0, "Units for latitude", "DEGREES");

        psMetadataAddMetadata(format, PS_LIST_TAIL, "FORMATS", 0, "Formats for various concepts",
                              conceptFormats);
        psFree(conceptFormats);
    }

    psMetadataAddMetadata(formats, PS_LIST_TAIL, "SKYCELL", 0, "The One True Format for skycells", format);
    psFree(format);

    // New camera MUST go to the head of the metadata, so that it will be recognised first
    // The old camera doesn't contain the PSCAMERA and PSFORMAT headers, so it will match anything!
    psTrace("psModules.config", 6, "Generated new camera configuration for %s.", newName);
    psMetadataAddMetadata(newCameras, PS_LIST_HEAD, newName, PS_META_REPLACE,
                          "Automatically generated", new);
    psFree(newName);
    psFree(new);

    return true;
}

// Generate the Chip and FPA mosaicked version of a named camera configuration
bool pmConfigCameraMosaickedVersions(psMetadata *system, // The system configuration
                                     const char *name // Name of the un-mosaicked camera
                                    )
{
    PS_ASSERT_METADATA_NON_NULL(system, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    bool mdok;                          // Status of MD lookup
    psMetadata *cameras = psMetadataLookupMetadata(&mdok, system, "CAMERAS"); // List of cameras
    if (!mdok || !cameras) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find CAMERAS in the system configuration.\n");
        return false;
    }
    if (!pmConfigGenerateMosaickedVersion(cameras, cameras, name, PM_FPA_LEVEL_CHIP)) {
        psError(PS_ERR_UNKNOWN, true, "Failed to build Chip mosaic camera description for %s\n", name);
        return false;
    }
    if (!pmConfigGenerateMosaickedVersion(cameras, cameras, name, PM_FPA_LEVEL_FPA)) {
        psError(PS_ERR_UNKNOWN, true, "Failed to build FPA mosaic camera description for %s\n", name);
        return false;
    }
    return true;
}

// the operation putting the new entries first is now implemented in pmConfigGenerateMosaickedVersion
// Generate the Chip and FPA mosaicked version of a named camera configuration
bool pmConfigCameraMosaickedVersionsAll(psMetadata *system)
{
    PS_ASSERT_METADATA_NON_NULL(system, false);

    bool mdok;                          // Status of MD lookup
    psMetadata *cameras = psMetadataLookupMetadata(&mdok, system, "CAMERAS"); // List of cameras
    if (!mdok || !cameras) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find CAMERAS in the system configuration.\n");
        return false;
    }

    psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *camerasItem = NULL; // Item from iteration
    psMetadata *new = psMetadataAlloc();// New cameras to add
    while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
        assert(camerasItem->type == PS_DATA_METADATA); // Only metadata are allowed here!
        if (!pmConfigGenerateMosaickedVersion(cameras, new, camerasItem->name, PM_FPA_LEVEL_CHIP)) {
            psError(PS_ERR_UNKNOWN, true, "Failed to build Chip mosaic camera description for %s\n",
                    camerasItem->name);
            return false;
        }
        if (!pmConfigGenerateMosaickedVersion(cameras, new, camerasItem->name, PM_FPA_LEVEL_FPA)) {
            psError(PS_ERR_UNKNOWN, true, "Failed to build FPA mosaic camera description for %s\n",
                    camerasItem->name);
            return false;
        }
    }
    psFree(camerasIter);

    // Now put the new cameras at the top of the list of cameras, so they get recognised first
    // Note: going from the top, and putting everything to the top as we get there, so that the last one on
    // goes to the top.  This preserves the original order of the cameras, putting the mosaicked versions
    // before the originals.
    camerasIter = psMetadataIteratorAlloc(new, PS_LIST_HEAD, NULL); // Iterator
    while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
        psMetadataAddItem(cameras, camerasItem, PS_LIST_HEAD, PS_META_REPLACE);
    }
    psFree(camerasIter);
    psFree(new);

    return true;
}

// Generate a mosaicked version of a camera configuration
bool pmConfigGenerateMosaickedVersion(psMetadata *oldCameras, // Old list of camera configurations
                                      psMetadata *newCameras, // New list of camera configurations
                                      const char *name, // Name of original camera configuration
                                      pmFPALevel mosaicLevel // Level to which we are mosaicking
    )
{
    assert(oldCameras);
    assert(newCameras);
    assert(name);
    assert(mosaicLevel == PM_FPA_LEVEL_CHIP || mosaicLevel == PM_FPA_LEVEL_FPA);

    if (name[0] == '_') {
        // It's already a mosaicked version of some sort
        return true;
    }

    // See if the old one is there
    psMetadata *camera = psMetadataLookupMetadata(NULL, oldCameras, name); // The camera configuration
    if (!camera) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Can't find camera to be mosaicked in camera list.");
        return false;
    }

    // See if the new one is already there
    psString newName = mosaicLevel == PM_FPA_LEVEL_CHIP ? pmConfigCameraChipName(name) :
        pmConfigCameraFPAName(name); // Name of mosaicked camera
    bool mdok;                       // Status of MD lookup
    psMetadata *oldCam = psMetadataLookupMetadata(&mdok, oldCameras, newName); // Existing camera configuration
    if (mdok && oldCam) {
        // Ensure new camera goes to the head of the metadata, so that it will be recognised first
        // The old camera doesn't contain the PSMOSAIC header, so it will match anything!
        psTrace("psModules.config", 6, "Camera configuration for %s exists, so moving to the front.", newName);
        psMetadataAddMetadata(newCameras, PS_LIST_HEAD, newName, PS_META_REPLACE, NULL, oldCam);
        psFree(newName);
        return true;
    }

    psMetadata *new = psMetadataCopy(NULL, camera); // Copy of the camera description

    // ** Fix up the contents of the FPA description to match the mosaicked camera **
    // select the FPA description
    psMetadata *fpa = psMetadataLookupMetadata(NULL, new, "FPA"); // FPA in the camera configuration
    if (!fpa) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Can't find FPA within camera configuration.");
        psFree(new);
        return false;
    }
    switch (mosaicLevel) {
        // For CHIP mosaic, replace the contents of each chip with a single cell
      case PM_FPA_LEVEL_CHIP: {
          psMetadataIterator *fpaIter = psMetadataIteratorAlloc(fpa, PS_LIST_HEAD, NULL); // Iterator
          psMetadataItem *fpaItem = NULL;     // Item from iteration
          while ((fpaItem = psMetadataGetAndIncrement(fpaIter))) {
              if (fpaItem->type != PS_DATA_STRING) {
                  psError(PS_ERR_UNKNOWN, true,
                          "Element %s within FPA in camera configuration is not of type STR.",
                          fpaItem->name);
                  psFree(new);
                  return false;
              }

              psFree(fpaItem->data.str);
              fpaItem->data.str = psStringCopy("MosaickedCell");
              psFree(fpaItem->comment);
              fpaItem->comment = psStringCopy("Mosaicked cell; automatically generated");
          }
          psFree(fpaIter);
          break;
      }
        // For FPA mosaic, replace the contents of the FPA with a single chip containing a single cell
      case PM_FPA_LEVEL_FPA: {
          while (psListLength(fpa->list) > 0) {
              psMetadataRemoveIndex(fpa, PS_LIST_TAIL);
          }

          psMetadataAddStr(fpa, PS_LIST_HEAD, "MosaickedChip", 0,
                           "Mosaicked chip with mosaicked cell; automatically generated",
                           "MosaickedCell");
          break;
      }
    default:
        psAbort("Should never get here.\n");
    }

    // ** Update the camera formats : add a new (mosaicked) format for each existing camera format **
    // select the list of all camera formats
    psMetadata *formats = psMetadataLookupMetadata(NULL, new, "FORMATS"); // FORMATS in the configuration
    assert(formats);            // It had better be there --- we've already read them in
    // loop over each of the formats
    psMetadataIterator *formatsIter = psMetadataIteratorAlloc(formats, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *formatsItem = NULL; // Item from iteration
    while ((formatsItem = psMetadataGetAndIncrement(formatsIter))) {
        assert(formatsItem->type == PS_DATA_METADATA); // We should have read it by now!
        psMetadata *format = formatsItem->data.md; // The camera format

        // Add a new RULE which uniquely describes the mosaicked format.  this is needed so
        // that when a mosaic is written to a FITS file, it can be recognised again when read.
        psMetadata *rule = psMetadataLookupMetadata(NULL, format, "RULE"); // Way to identify format from PHU
        if (!rule) {
            // a camera format without a rule is not allowed.
            psError(PS_ERR_UNKNOWN, false, "Camera format %s has no RULE", formatsItem->name);
            return false;
        }

        // the new rule is supplemented by the mosaicLevel
        switch (mosaicLevel) {
        case PM_FPA_LEVEL_CHIP:
            psMetadataAddStr(rule, PS_LIST_TAIL, "PSMOSAIC", 0, "Mosaicked level", "CHIP");
            break;
        case PM_FPA_LEVEL_FPA:
            psMetadataAddStr(rule, PS_LIST_TAIL, "PSMOSAIC", 0, "Mosaicked level", "FPA");
            break;
        default:
            psAbort("Should never get here.\n");
        }

        // Fix the FILE information: need to fix the levels for the PHU and EXTENSIONS.
        // both of these elements are required in the format; we raise an error if they are not found
        // If EXTENSIONS is NONE, then we need to change the CONTENT specifier to point to the chip name.
        psMetadata *file = psMetadataLookupMetadata(NULL, format, "FILE"); // File information
        if (!file) {
            psError(PS_ERR_UNKNOWN, false, "Camera format %s has no FILE", formatsItem->name);
            return false;
        }
        psMetadataItem *phuItem = psMetadataLookup(file, "PHU"); // PHU level
        if (!phuItem || phuItem->type != PS_DATA_STRING) {
            psError(PS_ERR_UNKNOWN, false, "Camera format %s is missing PHU in the FILE information", formatsItem->name);
            return false;
        }
        psMetadataItem *extensionsItem = psMetadataLookup(file, "EXTENSIONS"); // Extensions level
        if (!extensionsItem || extensionsItem->type != PS_DATA_STRING) {
            psError(PS_ERR_UNKNOWN, false, "Camera format %s is missing EXTENSIONS in the FILE information", formatsItem->name);
            return false;
        }

        // mosaicLevel == CHIP:
        // Case    PHU     EXTENSIONS     Modifications
        // ====    ===     ==========     ===========
        // 1.      FPA     CHIP           NONE
        // 2.      FPA     CELL           EXT->CHIP
        // 3.      FPA     NONE           NONE
        // 4.      CHIP    CELL           EXT->NONE
        // 5.      CHIP    NONE           NONE
        // 6.      CELL    NONE           PHU->CHIP
        // possible outcomes:
        //         FPA     CHIP
        //         FPA     NONE
        //         CHIP    NONE

        // mosaicLevel == FPA:
        // Case    PHU     EXTENSIONS     Modifications
        // ====    ===     ==========     ===========
        // 1.      FPA     CHIP           EXT->NONE
        // 2.      FPA     CELL           EXT->NONE
        // 3.      FPA     NONE           NONE
        // 4.      CHIP    CELL           PHU->FPA, EXT->NONE
        // 5.      CHIP    NONE           PHU->FPA
        // 6.      CELL    NONE           PHU->FPA
        // possible outcomes:
        //         FPA     NONE

        // modify the values of phuItem and extensionsItem
        switch (mosaicLevel) {
          case PM_FPA_LEVEL_CHIP:
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "CHIP")) {
                break;
            }
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "CELL")) {
                psFree(extensionsItem->data.str);
                extensionsItem->data.str = psStringCopy("CHIP");
                break;
            }
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CHIP") && !strcasecmp(extensionsItem->data.str, "CELL")) {
                psFree(extensionsItem->data.str);
                extensionsItem->data.str = psStringCopy("NONE");
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CHIP") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CELL") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                psFree(phuItem->data.str);
                phuItem->data.str = psStringCopy("CHIP");
                break;
            }
            psAbort ("should not reach here");

          case PM_FPA_LEVEL_FPA:
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "CHIP")) {
                psFree(extensionsItem->data.str);
                extensionsItem->data.str = psStringCopy("NONE");
                break;
            }
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "CELL")) {
                psFree(extensionsItem->data.str);
                extensionsItem->data.str = psStringCopy("NONE");
                break;
            }
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CHIP") && !strcasecmp(extensionsItem->data.str, "CELL")) {
                psFree(phuItem->data.str);
                phuItem->data.str = psStringCopy("FPA");
                psFree(extensionsItem->data.str);
                extensionsItem->data.str = psStringCopy("NONE");
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CHIP") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                psFree(phuItem->data.str);
                phuItem->data.str = psStringCopy("FPA");
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CELL") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                psFree(phuItem->data.str);
                phuItem->data.str = psStringCopy("FPA");
                break;
            }
            psAbort ("should not reach here");

          default:
            psAbort("Should never get here.\n");
        }

        // Fix up the CONTENTS to contain only the mosaicked cell for each chip
        switch (mosaicLevel) {
          case PM_FPA_LEVEL_FPA:
            psMetadataAddStr(format, PS_LIST_TAIL, TABLE_OF_CONTENTS, PS_META_REPLACE, NULL,
                             "MosaickedChip:MosaickedCell:_mosaic");
            break;
          case PM_FPA_LEVEL_CHIP:
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "CHIP")) {
                // ensure the value of CONTENT in the FILE section has the right value
                psMetadataAddStr(file, PS_LIST_TAIL, "CONTENT", PS_META_REPLACE, "Key to CONTENTS menu",
                                 "PS_CNTNT");
                psMetadataAddStr(file, PS_LIST_TAIL, "CONTENT.RULE", PS_META_REPLACE,
                                 "Rule to generate CONTENTS", "{CHIP.NAME}");

                // List the chipName:chipType for each chip.
                psMetadata *contents = psMetadataAlloc(); // List of contents, with chipName:chipType

                // XXX this is using the fpaItem->name not the chipName
                psMetadataIterator *fpaIter = psMetadataIteratorAlloc(fpa, PS_LIST_HEAD, NULL); // Iteratr
                psMetadataItem *fpaItem;    // Item from iteration
                while ((fpaItem = psMetadataGetAndIncrement(fpaIter))) {
                    assert (fpaItem->type == PS_DATA_STRING);
                    psString content = NULL; // Content to add
                    psStringAppend(&content, "%s:_mosaicChip ", fpaItem->name);
                    psMetadataAddStr(contents, PS_LIST_TAIL, fpaItem->name, 0, NULL, content);
                    psFree(content);
                }
                psFree(fpaIter);
                psMetadataAddMetadata(format, PS_LIST_TAIL, TABLE_OF_CONTENTS, PS_META_REPLACE,
                                      "List of contents", contents);
                psFree(contents);

                psMetadata *chips = psMetadataAlloc(); // List of chip types, with cellName:cellType
                psMetadataAddStr(chips, PS_LIST_TAIL, "_mosaicChip", 0, NULL,
                                 "MosaickedCell:_mosaic");
                psMetadataAddMetadata(format, PS_LIST_TAIL, CHIP_TYPES, PS_META_REPLACE,
                                      "List of chip types", chips);
                psFree(chips);
                break;
            }
            if (!strcasecmp(phuItem->data.str, "FPA") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                // List the contents on a single line
                psString contentsLine = NULL; // Contents of the PHU
                psMetadataIterator *fpaIter = psMetadataIteratorAlloc(fpa, PS_LIST_HEAD, NULL); // Iteratr
                psMetadataItem *fpaItem;    // Item from iteration
                while ((fpaItem = psMetadataGetAndIncrement(fpaIter))) {
                    assert (fpaItem->type == PS_DATA_STRING);
                    psStringAppend(&contentsLine, "%s:MosaickedCell:_mosaic ", fpaItem->name);
                }
                psFree(fpaIter);
                psMetadataAddStr(format, PS_LIST_TAIL, TABLE_OF_CONTENTS, PS_META_REPLACE,
                                 NULL, contentsLine);
                psFree(contentsLine);
                break;
            }
            if (!strcasecmp(phuItem->data.str, "CHIP") && !strcasecmp(extensionsItem->data.str, "NONE")) {
                // XXX recode this to match the structure above (FPA/CHIP)?
                // select and remove the old contents
                psMetadata *contents = psMetadataLookupMetadata(NULL, format, TABLE_OF_CONTENTS); // File contents
                if (!contents) {
                    psError(PS_ERR_UNKNOWN, false, "Couldn't find %s in the camera format %s.\n",
                            TABLE_OF_CONTENTS, formatsItem->name);
                    return false;
                }

                // replace chip type with _mosaicChip
                psMetadataIterator *contentIter = psMetadataIteratorAlloc(contents, PS_LIST_HEAD, NULL); // Iterator
                psMetadataItem *contentItem;    // Item from iteration
                while ((contentItem = psMetadataGetAndIncrement(contentIter))) {
                    assert (contentItem->type == PS_DATA_STRING);
                    char *ptr = strchr (contentItem->data.str, ':');
                    assert (ptr);
                    psString content = psStringNCopy (contentItem->data.str, ptr - contentItem->data.str);
                    psStringAppend(&content, ":_mosaicChip ");
                    psFree (contentItem->data.str);
                    contentItem->data.str = content;
                }
                psFree(contentIter);

                # if (0)
                while (psListLength(contents->list) > 0) {
                    psMetadataRemoveIndex(contents, PS_LIST_TAIL);
                }

                // update with the new contents
                psMetadataIterator *fpaIter = psMetadataIteratorAlloc(fpa, PS_LIST_HEAD, NULL); // Iterator
                psMetadataItem *fpaItem;    // Item from iteration
                while ((fpaItem = psMetadataGetAndIncrement(fpaIter))) {
                    assert (fpaItem->type == PS_DATA_STRING);
                    psString content = NULL; // Content to add
                    psStringAppend(&content, "%s:_mosaicChip ", fpaItem->name);
                    psMetadataAddStr(contents, PS_LIST_TAIL, fpaItem->name, 0, NULL, content);
                    psFree(content);
                }
                psFree(fpaIter);
                # endif

                psMetadata *chips = psMetadataAlloc(); // List of chip types, with cellName:cellType
                psMetadataAddStr(chips, PS_LIST_TAIL, "_mosaicChip", 0, NULL,
                                 "MosaickedCell:_mosaic");
                psMetadataAddMetadata(format, PS_LIST_TAIL, CHIP_TYPES, PS_META_REPLACE,
                                      "List of chip types", chips);
                psFree(chips);
                break;
            }
        default:
            psAbort("Should never get here.\n");
        }

        // Fix the cell type
        psMetadata *cells = psMetadataLookupMetadata(NULL, format, CELL_TYPES); // CELLS information
        if (!cells) {
            psError(PS_ERR_UNKNOWN, false, "Couldn't find CELLS of type METADATA in the camera format %s.\n", formatsItem->name);
            return false;
        }
        psMetadata *cell = psMetadataAlloc(); // Cell information
        psMetadataAddStr(cell, PS_LIST_TAIL, "CELL.TRIMSEC", 0, "Trim section", "TRIMSEC");
        psMetadataAddStr(cell, PS_LIST_TAIL, "CELL.BIASSEC", 0, "Bias section", "BIASSEC");
        psMetadataAddStr(cell, PS_LIST_TAIL, "CELL.TRIMSEC.SOURCE", 0, "Trim section source", "HEADER");
        psMetadataAddStr(cell, PS_LIST_TAIL, "CELL.BIASSEC.SOURCE", 0, "Bias section source", "HEADER");
        psMetadataAddMetadata(cells, PS_LIST_HEAD, "_mosaic", PS_META_REPLACE, "Mosaic cell information", cell);
        psFree(cell);                   // Drop reference

        // Update the concepts, so that they are all stored in the FITS headers, under headers of the same
        // name as the concept
        psMetadata *database = psMetadataLookupMetadata(&mdok, format, "DATABASE"); // DATABASE concepts
        psMetadata *defaults = psMetadataLookupMetadata(&mdok, format, "DEFAULTS"); // DEFAULTS concepts
        if (!mdok || !defaults) {
            psWarning("Couldn't find DEFAULTS of type METADATA in the camera format %s.\n",
                      formatsItem->name);
            continue;
        }
        psMetadata *translation = psMetadataLookupMetadata(&mdok, format, "TRANSLATION"); // TRANSLATION info
        if (!mdok || !translation) {
            psWarning("Couldn't find TRANSLATION of type METADATA in the camera format %s.\n",
                      formatsItem->name);
            continue;
        }

        removeCellConceptsSources(translation);
        removeCellConceptsSources(database);
        removeCellConceptsSources(defaults);

        psMetadata *conceptFormats = psMetadataLookupMetadata(&mdok, format, "FORMATS"); // Concepts formats

        // Add in the positioning concepts
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.XPARITY", 0, NULL, 1);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.YPARITY", 0, NULL, 1);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.X0",      0, NULL, 0);
        psMetadataAddS32(defaults, PS_LIST_TAIL, "CELL.Y0",      0, NULL, 0);
        if (conceptFormats) {
            if (psMetadataLookup(conceptFormats, "CELL.X0")) {
                psMetadataRemoveKey(conceptFormats, "CELL.X0");
            }
            if (psMetadataLookup(conceptFormats, "CELL.Y0")) {
                psMetadataRemoveKey(conceptFormats, "CELL.Y0");
            }
        }

        if (mosaicLevel == PM_FPA_LEVEL_FPA) {
            removeChipConceptsSources(translation);
            removeChipConceptsSources(database);
            removeChipConceptsSources(defaults);
            psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.XPARITY", 0, NULL, 1);
            psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.YPARITY", 0, NULL, 1);
            psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.X0", 0, NULL, 0);
            psMetadataAddS32(defaults, PS_LIST_TAIL, "CHIP.Y0", 0, NULL, 0);
            if (conceptFormats) {
                if (psMetadataLookup(conceptFormats, "CHIP.X0")) {
                    psMetadataRemoveKey(conceptFormats, "CHIP.X0");
                }
                if (psMetadataLookup(conceptFormats, "CHIP.Y0")) {
                    psMetadataRemoveKey(conceptFormats, "CHIP.Y0");
                }
            }
        }

    }
    psFree(formatsIter);

    // New camera MUST go to the head of the metadata, so that it will be recognised first
    // The old camera doesn't contain the PSMOSAIC header, so it will match anything!
    psTrace("psModules.config", 6, "Generated new camera configuration for %s.", newName);
    psMetadataAddMetadata(newCameras, PS_LIST_HEAD, newName, PS_META_REPLACE,
                          "Automatically generated", new);
    psFree(newName);
    psFree(new);

    return true;
}

/*** Helper Functions ***/

// Remove a concept from the list of sources.  Need to check to see if it exists first, to avoid a warning.
static void removeConcept(psMetadata *source, // Source from which to remove concept
                          const char *concept // Concept name to remove
                         )
{
    assert(source);
    assert(concept && strlen(concept) > 0);

    if (psMetadataLookup(source, concept)) {
        psMetadataRemoveKey(source, concept);
    }

    return;
}

// Remove certain concepts from the list of sources.  These concepts are important in the mosaicking process,
// and are added explicitly to the defaults (elsewhere) so that the user can't get them wrong.
static void removeCellConceptsSources(psMetadata *source // Source for concepts
    )
{
    if (!source) {
        return;
    }

    removeConcept(source, "CELL.BIASSEC");
    removeConcept(source, "CELL.TRIMSEC");
    removeConcept(source, "CELL.XPARITY");
    removeConcept(source, "CELL.YPARITY");
    removeConcept(source, "CELL.X0");
    removeConcept(source, "CELL.Y0");

    // For the sake of the defaults, include the .DEPEND
    removeConcept(source, "CELL.XPARITY.DEPEND");
    removeConcept(source, "CELL.YPARITY.DEPEND");
    removeConcept(source, "CELL.X0.DEPEND");
    removeConcept(source, "CELL.Y0.DEPEND");

    return;
}

// Remove certain concepts from the list of sources.  These concepts are important in the mosaicking process,
// and are added explicitly to the defaults (elsewhere) so that the user can't get them wrong.
static void removeChipConceptsSources(psMetadata *source // Source for concepts
    )
{
    if (!source) {
        return;
    }

    removeConcept(source, "CHIP.XPARITY");
    removeConcept(source, "CHIP.YPARITY");
    removeConcept(source, "CHIP.X0");
    removeConcept(source, "CHIP.Y0");

    // For the sake of the defaults, include the .DEPEND
    removeConcept(source, "CHIP.XPARITY.DEPEND");
    removeConcept(source, "CHIP.YPARITY.DEPEND");
    removeConcept(source, "CHIP.X0.DEPEND");
    removeConcept(source, "CHIP.Y0.DEPEND");

    return;
}

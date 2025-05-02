#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmConfig.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmHDUUtils.h"
#include "pmConcepts.h"
#include "pmConceptsUpdate.h"

#include "pmConceptsRead.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This function gets called for the really boring concepts --- where all you have to do is parse from a
// header or database and you don't need to muck around with conversions.  There is no similar "formatPlain",
// since the type is already known.
static psMetadataItem *parsePlain(psMetadataItem *concept, // The concept to parse
                                  const psMetadataItem *pattern // The concept pattern
                                 )
{
    assert(concept);
    assert(pattern);

    switch (pattern->type) {
    case PS_DATA_STRING: {
            psString string = psMetadataItemParseString(concept); // Get the string, so I can free it after it
            // goes on the MetadataItem
            psMetadataItem *item = psMetadataItemAllocStr(pattern->name, pattern->comment, string);
            psFree(string);
            return item;
        }
    case PS_DATA_S32:
        return psMetadataItemAllocS32(pattern->name, pattern->comment, psMetadataItemParseS32(concept));
    case PS_DATA_F32:
        return psMetadataItemAllocF32(pattern->name, pattern->comment, psMetadataItemParseF32(concept));
    case PS_DATA_F64:
        return psMetadataItemAllocF64(pattern->name, pattern->comment, psMetadataItemParseF64(concept));
    case PS_DATA_BOOL:
      return psMetadataItemAllocBool(pattern->name, pattern->comment, psMetadataItemParseBool(concept));
    default:
        psWarning("Concept %s (%s) is not of a standard type (%x)\n",
                 pattern->name, pattern->comment, pattern->type);
        return NULL;
    }
}

// Parse a single concept
static bool conceptParse(pmConceptSpec *spec, // The concept specification
                         psMetadataItem *concept, // The concept to parse
                         pmConceptSource source, // The concept source
                         psMetadata *cameraFormat, // The camera format
                         psMetadata *target, // The target
                         const pmFPA *fpa, // The FPA
                         const pmChip *chip, // The chip
                         const pmCell *cell // The cell
                        )
{
    assert(spec);
    assert(cameraFormat);
    assert(target);

    if (!concept) {
        psError(PS_ERR_UNKNOWN, true, "Concept is NULL");
        return false;
    }
    psTrace ("psModules.concepts", 3, "parsing concept: %s\n", spec->blank->name);
    if (!strcmp (spec->blank->name, "CELL.XPARITY")) {
        psTrace ("psModules.concepts", 3, "parsing CELL.XPARITY: %s\n", spec->blank->name);
    }

    psMetadataItem *parsed = NULL;  // The parsed concept
    if (spec->parse) {
        parsed = spec->parse(concept, spec->blank, source, cameraFormat, fpa, chip, cell);
    } else {
        parsed = parsePlain(concept, spec->blank);
    }
    if (!parsed) {
        if (spec->required) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s\n", spec->blank->name);
            return false;
        } else {
            psWarning("Unable to parse concept %s, but concept not marked as required.\n", spec->blank->name);
            psErrorClear();
            return true; // XXX return?
        }
    }

    // Plug the parsed concept into a new psMetadataItem, so each "concept" has its own version that can
    // be altered without affecting the others.  Also, so that we maintain the template name and comment.
    psMetadataItem *cleaned = NULL;     // Item that's been cleaned up --- correct name and comment
    switch (spec->blank->type) {
    case PS_DATA_STRING:
        cleaned = psMetadataItemAllocStr(spec->blank->name, spec->blank->comment, parsed->data.V);
        break;
    case PS_DATA_S32:
        cleaned = psMetadataItemAllocS32(spec->blank->name, spec->blank->comment, parsed->data.S32);
        break;
    case PS_DATA_F32:
        cleaned = psMetadataItemAllocF32(spec->blank->name, spec->blank->comment, parsed->data.F32);
        break;
    case PS_DATA_F64:
        cleaned = psMetadataItemAllocF64(spec->blank->name, spec->blank->comment, parsed->data.F64);
        break;
    default:
        cleaned = psMetadataItemAlloc(spec->blank->name, parsed->type, spec->blank->comment,
                                      parsed->data.V);
    }
    psFree(parsed);
    psMetadataAddItem(target, cleaned, PS_LIST_TAIL, PS_META_REPLACE);
    psFree(cleaned);                 // Drop reference
    return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool p_pmConceptsReadFromCells(psMetadata *target, const psMetadata *specs, const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(specs, false);
    PS_ASSERT_PTR_NON_NULL(target, false);
    if (!cell) {
        psError(PS_ERR_UNKNOWN, true, "cell is NULL");
        return false;
    }

    pmHDU *hdu = pmHDUGetLowest(NULL, NULL, cell); // The HDU at the lowest level
    if (!hdu) {
        psError(PS_ERR_UNKNOWN, true, "Can't find HDU for cell");
        return false;
    }
    psMetadata *cameraFormat = hdu->format; // The camera format
    psMetadata *cellConfig = cell->config; // The camera configuration for this cell
    psMetadataIterator *specsIter = psMetadataIteratorAlloc(specs, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *specItem = NULL;    // Item from the specs metadata
    bool status = true;                 // Status of reading concepts
    while ((specItem = psMetadataGetAndIncrement(specsIter))) {
        pmConceptSpec *spec = specItem->data.V; // The specification
        psString name = specItem->name; // The concept name
        psMetadataItem *conceptItem = psMetadataLookup(cellConfig, name); // The concept, or NULL
        psTrace("psModules.concepts", 10, "%s: %p\n", name, conceptItem);
        if (conceptItem) {
            if (conceptItem->type == PS_DATA_STRING) {
                // Check the SOURCE
                psString nameSource = NULL; // String with the concept name and ".SOURCE" added
                psStringAppend(&nameSource, "%s.SOURCE", name);
                bool mdok = true;       // Status of MD lookup
                psString source = psMetadataLookupStr(&mdok, cell->config, nameSource); // The source
                psFree(nameSource);
                if (mdok && source && strlen(source) > 0 && strcasecmp(source, "VALUE") == 0) {
                    if (!conceptParse(spec, conceptItem, PM_CONCEPT_SOURCE_CELLS,
                                      cameraFormat, target, NULL, NULL, cell)) {
                        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s from camera "
                                "configuration\n", name);
                        status = false;
                    }
                } else if (source && (strlen(source) == 0 || strcasecmp(source, "HEADER") != 0)) {
                    // We leave "HEADER" to pmConceptsReadFromHeader
                    psError(PS_ERR_IO, true, "%s isn't HEADER or VALUE --- can't read %s\n", source,
                            name);
                    continue;
                }
            } else {
                // Another type --- should be OK
                if (!conceptParse(spec, conceptItem, PM_CONCEPT_SOURCE_CELLS,
                                  cameraFormat, target, NULL, NULL, cell)) {
                    psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s from camera "
                            "configuration.\n", name);
                    status = false;
                }
            }
        }
    }
    psFree(specsIter);
    return status;
}

psMetadataItem *p_pmConceptsReadSingleFromDefaults(const char *name, const psMetadata *defaults,
                                                  const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);
    PS_ASSERT_METADATA_NON_NULL(defaults, NULL);

    psMetadataItem *item = psMetadataLookup(defaults, name); // The concept, or NULL
    psTrace("psModules.concepts", 10, "%s: %p\n", name, item);
    if (item && item->type == PS_DATA_METADATA) {
        // This is a menu
        psTrace("psModules.concepts", 5, "%s is of type METADATA.\n", name);
        item = p_pmConceptsDepend(name, item->data.md, defaults, fpa, chip, cell);
    }
    return item;
}

bool p_pmConceptsReadFromDefaults(psMetadata *target, const psMetadata *specs,
                                  const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(specs, false);
    PS_ASSERT_PTR_NON_NULL(target, false);

    psTrace("psModules.concepts", 3, "Reading concepts from defaults...\n");

    pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // The HDU at the lowest level
    if (!hdu) {
        // We read the defaults for all the HDUs we could find
        return true;
    }
    psMetadata *cameraFormat = hdu->format; // The camera format
    bool mdok = true;                   // Status of MD lookup
    psMetadata *defaults = psMetadataLookupMetadata(&mdok, cameraFormat, "DEFAULTS"); // The DEFAULTS spec
    if (!mdok || !defaults) {
        psError(PS_ERR_IO, true, "Failed to find \"DEFAULTS\"");
        return false;
    }
    psMetadataIterator *specsIter = psMetadataIteratorAlloc(specs, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *specItem = NULL;    // Item from the specs metadata
    bool status = true;                 // Status of reading concepts
    psErrorClear();   // we're going to declare all errors "old" => won't clear stack
    while ((specItem = psMetadataGetAndIncrement(specsIter))) {
        pmConceptSpec *spec = specItem->data.V; // The specification
        psString name = specItem->name; // The concept name
        psMetadataItem *conceptItem = p_pmConceptsReadSingleFromDefaults(name, defaults, fpa, chip, cell);
        if (conceptItem && !conceptParse(spec, conceptItem, PM_CONCEPT_SOURCE_DEFAULTS,
                                         cameraFormat, target, fpa, chip, cell)) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s from DEFAULTS.\n", name);
            status = false;
        }
    }
    psFree(specsIter);
    return status;
}


bool p_pmConceptsReadFromHeader(psMetadata *target, const psMetadata *specs,
                                const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(specs, false);
    PS_ASSERT_PTR_NON_NULL(target, false);

    pmHDU *hduLow = pmHDUGetLowest(fpa, chip, cell); // The HDU at the lowest level
    if (!hduLow) {
        // We read the defaults for all the HDUs we could find
        return true;
    }
    pmHDU *hduHigh = pmHDUGetHighest(fpa, chip, cell); // The HDU at the highest level
    if (!hduHigh) {
        psError(PS_ERR_UNKNOWN, true, "Can't find HDU at the highest level");
        return false;
    }
    assert(hduLow->format == hduHigh->format); // Just in case....
    psMetadata *cameraFormat = hduLow->format; // The camera format
    bool mdok = true;                   // Status of MD lookup
    psMetadata *transSpec = psMetadataLookupMetadata(&mdok, cameraFormat, "TRANSLATION"); // TRANSLATION spec
    if (!mdok || !transSpec) {
        psError(PS_ERR_IO, true, "Failed to find \"TRANSLATION\"");
        return false;
    }

    psMetadataIterator *specsIter = psMetadataIteratorAlloc(specs, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *specItem = NULL;    // Item from the specs metadata
    bool status = true;                 // Status of reading concepts
    while ((specItem = psMetadataGetAndIncrement(specsIter))) {
        pmConceptSpec *spec = specItem->data.V; // The specification
        psString name = specItem->name; // The concept name
        psMetadataItem *headerItem = NULL; // The value of the concept from the header


        psTrace ("psModules.concepts", 3, "reading concept: %s\n", name);
        if (!strcmp (name, "CELL.XPARITY")) {
            psTrace ("psModules.concepts", 3, "parsing CELL.XPARITY: %s\n", name);
        }
        if (!strcmp (name, "CELL.TRIMSEC")) {
            psTrace ("psModules.concepts", 3, "parsing CELL.TRIMSEC: %s\n", name);
        }

        // First check the cell configuration
        if (cell && cell->config) {
            psMetadataItem *conceptItem = psMetadataLookup(cell->config, name); // The concept, or NULL
            if (conceptItem) {
                // Check the SOURCE
                psString nameSource = NULL; // String with the concept name and ".SOURCE" added
                psStringAppend(&nameSource, "%s.SOURCE", name);
                psString source = psMetadataLookupStr(&mdok, cell->config, nameSource); // The source
                psFree(nameSource);
                if (mdok && strlen(source) && strcasecmp(source, "HEADER") == 0) {
                    if (hduLow->header) {
                        headerItem = psMetadataLookup(hduLow->header, conceptItem->data.V);
                    }
                    if (!headerItem && hduHigh != hduLow && hduHigh->header) {
                        headerItem = psMetadataLookup(hduHigh->header, conceptItem->data.V);
                    }
                    // if (!headerItem) {
                    // psWarning("Unable to find concept %s claimed to be in header as %s", name, conceptItem->data.str);
                    // }
                    psMemIncrRefCounter(headerItem);
                }
                // Leave the error handling to pmConceptsFromCamera, which should already have been called
            }
        }
        if (!headerItem) {
            psMetadataItem *formatItem = psMetadataLookup(transSpec, name); // Item with keyword
            if (!formatItem) {
                continue;
            }
            if (formatItem->type == PS_DATA_METADATA) {
                // This is a menu
                psTrace("psModules.concepts", 5, "%s is of type METADATA.\n", name);
                formatItem = p_pmConceptsDepend(name, formatItem->data.md, transSpec, fpa, chip, cell);
                if (!formatItem) {
                    continue;
                }
            }
            if (formatItem->type != PS_DATA_STRING) {
                psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for concept %s in TRANSLATION is not STR",
                        name);
                psFree(specsIter);
                return false;
            }
            psString keywords = formatItem->data.str; // The FITS keywords
            // In case there are multiple headers
            psList *keys = psStringSplit(keywords, " ,;", true); // List of keywords
            if (keys->n == 1) {
                // Only one key --- proceed as usual
                if (hduLow->header) {
                    headerItem = psMetadataLookup(hduLow->header, keywords);
                }
                if (!headerItem && hduHigh != hduLow && hduHigh->header) {
                    headerItem = psMetadataLookup(hduHigh->header, keywords);
                }
                psMemIncrRefCounter(headerItem);
            } else {
                psListIterator *keysIter = psListIteratorAlloc(keys, PS_LIST_HEAD, false); // Iterator
                psString key = NULL; // Item from iteration
                psList *values = psListAlloc(NULL); // List containing the values
                while ((key = psListGetAndIncrement(keysIter))) {
                    psMetadataItem *value = NULL;
                    if (hduLow->header) {
                        value = psMetadataLookup(hduLow->header, key);
                    }
                    if (!value && hduHigh != hduLow && hduHigh->header) {
                        value = psMetadataLookup(hduHigh->header, key);
                    }
                    if (value) {
                        psListAdd(values, PS_LIST_TAIL, value);
                    } else {
                        psWarning("Unable to find header %s --- assuming value is NULL", key);
                    }
                }
                psFree(keysIter);
                headerItem = psMetadataItemAlloc(name, PS_DATA_LIST, specItem->comment, values);
                psFree(values);
            }
            psFree(keys);
        }

        // This will also clean up the name
        if (headerItem) {
            if (!conceptParse(spec, headerItem, PM_CONCEPT_SOURCE_HEADER,
                              cameraFormat, target, fpa, chip, cell)) {
                psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s from header.\n", name);
                status = false;
            }
        }
        psTrace("psModules.concepts", 10, "%s: %p\n", name, headerItem);
        psFree(headerItem);
    }
    psFree(specsIter);
    return status;
}

psMetadataItem *p_pmConceptsReadSingleFromDatabase(const char *name, const psMetadata *database,
                                                   pmConfig *config, const pmFPA *fpa, const pmChip *chip,
                                                   const pmCell *cell)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadataItem *item = psMetadataLookup(database, name); // Item to return
    if (item && item->type == PS_DATA_METADATA) {
        // This is a menu
        psTrace("psModules.concepts", 5, "%s is of type METADATA.\n", name);
        item = p_pmConceptsDepend(name, item->data.md, database, fpa, chip, cell);
    }
    if (!item) {
        return NULL;
    }
    if (item->type != PS_DATA_STRING) {
        psWarning("%s in DATABASE in camera format is not of type STR --- ignored.", name);
        return NULL;
    }

#ifndef HAVE_PSDB
    psError(PS_ERR_UNKNOWN, false,
            "Cannot read concept: psModules was compiled without database support.");
    return NULL;
#else

    psDB *db = pmConfigDB(config);      // Database handle
    if (!db) {
        psErrorClear();
        psWarning("Unable to initialise database to write concepts.");
        return NULL;
    }

    psString sql = pmConceptsInterpolate(item->data.str, fpa, chip, cell);
    if (!p_psDBRunQuery(config->database, sql)) {
        psWarning("Unable to query database for concept %s --- ignored.", name);
        psFree(sql);
        return NULL;
    }
    psFree(sql);

    psArray *rows = p_psDBFetchResult(config->database); // Rows returned from the query
    if (rows->n == 0) {
        psWarning("No rows returned from database query for concept %s --- ignored.", name);
	psFree(rows);
        return NULL;
    }
    if (rows->n > 1) {
        psWarning("Multiple rows returned from database query for concept %s --- using the first", name);
    }
    psMetadata *row = rows->data[0]; // First (and only) row
    if (row->list->n > 1) {
        psWarning("Multiple columns returned from database query for concept %s --- using the first", name);
    }

    psMetadataItem *concept = psMetadataGet(row, PS_LIST_HEAD); // Item of interest

    psFree(rows);

    return concept;
#endif // HAVE_PSDB
}

bool p_pmConceptsReadFromDatabase(psMetadata *target, const psMetadata *specs,
                                  const pmFPA *fpa, const pmChip *chip, const pmCell *cell, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(specs, false);
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

#ifndef HAVE_PSDB
    return true;
#else
    pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // The HDU at the lowest level
    if (!hdu) {
        // We read the database for all the HDUs we could find
        return true;
    }
    psMetadata *cameraFormat = hdu->format; // The camera format
    bool mdok = true;                   // Status of MD lookup
    psMetadata *dbSpec = psMetadataLookupMetadata(&mdok, cameraFormat, "DATABASE"); // The DATABASE spec
    if (!mdok || !dbSpec) {
        return true;
    }

    psMetadataIterator *specsIter = psMetadataIteratorAlloc(specs, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *specItem = NULL;    // Item from the specs metadata
    bool status = true;                 // Status of reading concepts
    while ((specItem = psMetadataGetAndIncrement(specsIter))) {
        pmConceptSpec *spec = specItem->data.V; // The specification
        psString name = specItem->name; // The concept name
        psMetadataItem *conceptItem = p_pmConceptsReadSingleFromDatabase(name, dbSpec, config,
                                                                         fpa, chip, cell);
        if (conceptItem && !conceptParse(spec, conceptItem, PM_CONCEPT_SOURCE_DATABASE,
                                         cameraFormat, target, fpa, chip, cell)) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s from database.\n", name);
            status = false;
        }
    } // Iterating through the concept specifications
    psFree(specsIter);

    return status;
#endif
}



// Read all registered concepts for the specified level
static bool conceptsRead(psMetadata **specs, // One of the concepts specifications
                         pmFPA *fpa,    // The FPA
                         pmChip *chip,  // The chip
                         pmCell *cell, // The cell
                         unsigned int *read,     // What's already been read
                         pmConceptSource source, // The source of the concepts to read
                         pmConfig *config, // Configuration
                         psMetadata *target // Place into which to read the concepts
                        )
{
    assert(specs);
    assert(read);
    assert(target);

    pmConceptsInit();

    // At least one HDU is required for the reading functions
    pmHDU *hduLow = pmHDUGetLowest(fpa, chip, cell); // Lowest HDU.
    if (!hduLow) {
        // Can't do anything --- don't record any success, but don't return an error either
        return true;
    }
    pmHDU *hduHigh = pmHDUGetHighest(fpa, chip, cell); // Highest HDU

    if (cell && (cell->conceptsRead == PM_CONCEPT_SOURCE_NONE)) {
        pmConceptsBlankCell(cell);
        cell->conceptsRead = PM_CONCEPT_SOURCE_BLANK;
    }
    if (chip && (chip->conceptsRead == PM_CONCEPT_SOURCE_NONE)) {
        pmConceptsBlankChip(chip);
        chip->conceptsRead = PM_CONCEPT_SOURCE_BLANK;
    }
    if (fpa && (fpa->conceptsRead == PM_CONCEPT_SOURCE_NONE)) {
        pmConceptsBlankFPA(fpa);
        fpa->conceptsRead = PM_CONCEPT_SOURCE_BLANK;
    }

    bool success = true;                // Success in reading concepts?
    if (source & PM_CONCEPT_SOURCE_CELLS && !(*read & PM_CONCEPT_SOURCE_CELLS) && cell) {
        if (p_pmConceptsReadFromCells(target, *specs, cell)) {
            *read |= PM_CONCEPT_SOURCE_CELLS;
        } else {
            psError(PS_ERR_UNKNOWN, false, "Error reading concepts from camera configuration.\n");
            success = false;
        }
    }

    if (source & PM_CONCEPT_SOURCE_DEFAULTS && !(*read & PM_CONCEPT_SOURCE_DEFAULTS)) {
        if (p_pmConceptsReadFromDefaults(target, *specs, fpa, chip, cell)) {
            *read |= PM_CONCEPT_SOURCE_DEFAULTS;
        } else {
            psError(PS_ERR_UNKNOWN, false, "Error reading concepts from defaults.\n");
            success = false;
        }
    }

    if (source & PM_CONCEPT_SOURCE_PHU && !(*read & PM_CONCEPT_SOURCE_PHU) && hduHigh->header) {
        if (p_pmConceptsReadFromHeader(target, *specs, fpa, chip, cell)) {
            *read |= PM_CONCEPT_SOURCE_PHU;
        } else {
            psError(PS_ERR_UNKNOWN, false, "Error reading concepts from PHU.\n");
            success = false;
        }
    }

    // If there are multiple HDUs, then it may be that one of them hasn't been read yet (hdu->header not set)
    if (source & PM_CONCEPT_SOURCE_HEADER && !(*read & PM_CONCEPT_SOURCE_HEADER) &&
        hduLow != hduHigh && hduLow->header) {
        if (p_pmConceptsReadFromHeader(target, *specs, fpa, chip, cell)) {
            *read |= PM_CONCEPT_SOURCE_HEADER;
        } else {
            psError(PS_ERR_UNKNOWN, false, "Error reading concepts from header.\n");
            success = false;
        }
    }

#ifdef HAVE_PSDB
    if (source & PM_CONCEPT_SOURCE_DATABASE && !(*read & PM_CONCEPT_SOURCE_DATABASE)) {
        if (p_pmConceptsReadFromDatabase(target, *specs, fpa, chip, cell, config)) {
            *read |= PM_CONCEPT_SOURCE_DATABASE;
        } else {
            psError(PS_ERR_UNKNOWN, false, "Error reading concepts from database.\n");
            success = false;
        }
    }
#endif

    pmConceptsUpdate(fpa, chip, cell);

    return success;
}




bool pmConceptsRead(pmFPA *fpa, pmChip *chip, pmCell *cell, pmConceptSource source, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications
    bool success = conceptsRead(&conceptsFPA, fpa, chip, cell, &fpa->conceptsRead, source,
                                config, fpa->concepts);
    if (chip) {
        psMetadata *conceptsChip = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
        success &= conceptsRead(&conceptsChip, fpa, chip, cell, &chip->conceptsRead, source,
                                config, chip->concepts);
    }
    if (cell) {
        psMetadata *conceptsCell = pmConceptsSpecs(PM_FPA_LEVEL_CELL); // Concept specifications
        success &= conceptsRead(&conceptsCell, fpa, chip, cell, &cell->conceptsRead, source,
                                config, cell->concepts);
    }

    return success;
}



bool pmConceptsReadFPA(pmFPA *fpa, pmConceptSource source, bool propagateDown, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications
    psTrace("psModules.concepts", 5, "Reading FPA concepts: %p %p\n", conceptsFPA, fpa->concepts);
    bool success = conceptsRead(&conceptsFPA, fpa, NULL, NULL, &fpa->conceptsRead, source,
                                config, fpa->concepts);
    if (propagateDown) {
        psArray *chips = fpa->chips;    // Array of chips
        for (long i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i]; // Chip of interest
            if (chip) {
                success &= pmConceptsReadChip(chip, source, false, true, config);
            }
        }
    }

    return success;
}



bool pmConceptsReadChip(pmChip *chip, pmConceptSource source, bool propagateUp,
                        bool propagateDown, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    psMetadata *conceptsChip = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
    psTrace("psModules.concepts", 5, "Reading chip concepts: %p %p\n", conceptsChip, chip->concepts);
    pmFPA *fpa = chip->parent;          // FPA to which the chip belongs
    bool success = conceptsRead(&conceptsChip, fpa, chip, NULL, &chip->conceptsRead, source, config,
                                chip->concepts);
    if (propagateUp) {
        psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications
        success &= conceptsRead(&conceptsFPA, fpa, chip, NULL, &fpa->conceptsRead, source,
                                config, fpa->concepts);
    }
    if (propagateDown) {
        psArray *cells = chip->cells;        // Array of cells
        for (long i = 0; i < cells->n; i++) {
            pmCell *cell = cells->data[i];  // Cell of interest
            if (cell) {
                success &= pmConceptsReadCell(cell, source, false, config);
            }
        }
    }
    return success;
}


bool pmConceptsReadCell(pmCell *cell, pmConceptSource source, bool propagateUp, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    psMetadata *conceptsCell = pmConceptsSpecs(PM_FPA_LEVEL_CELL); // Concept specifications
    psTrace("psModules.concepts", 5, "Reading cell concepts: %p %p\n", conceptsCell, cell->concepts);
    pmChip *chip = cell->parent;        // Chip to which the cell belongs
    pmFPA *fpa = chip->parent;          // FPA to which the chip belongs

    bool success = conceptsRead(&conceptsCell, fpa, chip, cell, &cell->conceptsRead, source, config,
                                cell->concepts);
    if (propagateUp) {
        psMetadata *conceptsChip = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
        success &= conceptsRead(&conceptsChip, fpa, chip, cell, &chip->conceptsRead, source, config,
                                chip->concepts);
        psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications

        success &= conceptsRead(&conceptsFPA, fpa, chip, cell, &fpa->conceptsRead, source, config,
                                fpa->concepts);
    }

    return success;
}

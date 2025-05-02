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
#include "pmConceptsRead.h"

#include "pmConceptsWrite.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool compareConcepts(const psMetadataItem *compare, // Item to compare
                            const psMetadataItem *standard // Standard for comparison
                           )
{
    // First order checks
    if (! compare || ! standard) {
        return false;
    }
    if (strcasecmp(compare->name, standard->name) != 0) {
        return false;
    }

    // Special case: list
    if (compare->type == PS_DATA_LIST) {
        // "compare" contains a list of psMetadataItems
        // "standard" likely contains just a string (but it might possibly be a list of strings)
        psList *cList = compare->data.V; // The list from comparison item
        psList *sList = NULL;         // The list from standard item
        switch (standard->type) {
        case PS_DATA_STRING:
            sList = psStringSplit(standard->data.V, " ;", true);
            break;
        case PS_DATA_LIST:
            sList = psMemIncrRefCounter(standard->data.V);
            break;
        default:
            return false;
        }
        if (cList->n != sList->n) {
            psFree(sList);
            return false;
        }
        psVector *match = psVectorAlloc(cList->n, PS_TYPE_U8); // Array indicating which values match
        psVectorInit(match, 0);
        psListIterator *cIter = psListIteratorAlloc(cList, PS_LIST_HEAD, false); // compare iterator
        psListIterator *sIter = psListIteratorAlloc(sList, PS_LIST_HEAD, false); // standard iterator
        psMetadataItem *cItem = NULL; // Item from compare list
        while ((cItem = psListGetAndIncrement(cIter))) {
            if (cItem->type != PS_DATA_STRING) {
                psWarning("psMetadataItem from list is of type %x instead of %x (PS_DATA_STRING) --- can't interpret.\n", cItem->type, PS_DATA_STRING);
                psFree(cIter);
                psFree(sIter);
                psFree(match);
                psFree(sList);
                return false;
            }
            psString cString = cItem->data.V; // String from compare list
            psListIteratorSet(sIter, PS_LIST_HEAD);
            int index = 0;            // Index for list
            bool found = false;       // Found a match?
            for (psString sString = NULL; (sString = psListGetAndIncrement(sIter)) && !found; index++) {
                if (strcasecmp(cString, sString) == 0) {
                    match->data.U8[index]++;
                    found = true;
                }
            }
            if (! found) {
                // Can give up immediately
                psFree(cIter);
                psFree(sIter);
                psFree(match);
                psFree(sList);
                return false;
            }
        }
        // Make sure we got 100% matches in both directions
        bool allMatch = true;         // Did all of them match?
        for (int i = 0; i < match->n && allMatch; i++) {
            if (!match->data.U8[i]) {
                allMatch = false;
            }
        }
        psFree(cIter);
        psFree(sIter);
        psFree(sList);
        psFree(match);
        return allMatch;
    }

    return psMetadataItemCompare(compare, standard);

}


// Format a single concept
static psMetadataItem *conceptFormat(const pmConceptSpec *spec, // The concept specification
                                     const psMetadataItem *concept, // The concept to parse
                                     pmConceptSource source, // The concept source
                                     const psMetadata *cameraFormat, // The camera format
                                     const pmFPA *fpa, // The FPA
                                     const pmChip *chip, // The chip
                                     const pmCell *cell // The cell
                                    )
{
    assert(spec);
    assert(cameraFormat);

    if (concept) {
        psMetadataItem *formatted = NULL;  // The formatted concept
        if (spec->format) {
            formatted = spec->format(concept, source, cameraFormat, fpa, chip, cell);
        } else if (strcmp(concept->name, spec->blank->name) != 0) {
            // Adjust so that the name is correct
            formatted = psMetadataItemCopy(concept);
            psFree(formatted->name);
            formatted->name = psStringCopy(spec->blank->name);
        } else {
            // Can get away with merely incrementing the reference counter
            formatted = psMemIncrRefCounter((const psPtr)concept);
        }

        return formatted;
    }
    return NULL;
}

// Write a single value to a header
static bool writeSingleHeader(pmHDU *hdu, // HDU for which to add to the header
                              const char *keyword, // Keyword to add
                              const psMetadataItem *item // Item to add to the header; may be NULL
                             )
{
    assert(hdu);
    assert(keyword && strlen(keyword) > 0);

    if (!hdu->header) {
        hdu->header = psMetadataAlloc();
    }
    if (!item) {
        psTrace("psModules.concepts", 9, "Writing header %s: <<<BLANK>>>\n", keyword);
        // Assume it's a NULL string: it's most easily parsed.
        return psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, NULL, NULL);
    }
    switch (item->type) {
      case PS_DATA_BOOL:
        psTrace("psModules.concepts", 9, "Writing header %s: %d\n", keyword, item->data.B);
        return psMetadataAddBool(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, item->comment,
                                 item->data.B);
      case PS_DATA_STRING:
        psTrace("psModules.concepts", 9, "Writing header %s: %s\n", keyword, item->data.str);
        return psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, item->comment,
                                item->data.V);
      case PS_DATA_S32:
        psTrace("psModules.concepts", 9, "Writing header %s: %d\n", keyword, item->data.S32);
        return psMetadataAddS32(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, item->comment,
                                item->data.S32);
      case PS_DATA_F32:
        psTrace("psModules.concepts", 9, "Writing header %s: %f\n", keyword, item->data.F32);
        return psMetadataAddF32(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, item->comment,
                                item->data.F32);
      case PS_DATA_F64:
        psTrace("psModules.concepts", 9, "Writing header %s: %f\n", keyword, item->data.F64);
        return psMetadataAddF64(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, item->comment,
                                item->data.F64);
      case PS_DATA_REGION: {
          psString region = psRegionToString(*(psRegion*)item->data.V);
          psTrace("psModules.concepts", 9, "Writing header %s: %s\n", keyword, region);
          bool result = psMetadataAddStr(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, item->comment,
                                         region);
          psFree(region);
          return result;
      }
      default:
        psWarning("Type of %s is not suitable for a FITS header --- not added.\n",
                  item->name);
        return false;
    }
}


// Write potentially multiple values to a header
static bool writeHeader(pmHDU *hdu,     // HDU for which to add to the header
                        const char *keywords, // Keywords to add
                        const psMetadataItem *item // Item to add to the header
                       )
{
    assert(hdu);
    assert(keywords);
    assert(item);

    bool status = true;                 // Status of writing headers, to be returned
    if (item->type == PS_DATA_LIST) {
        psList *values = item->data.V;  // List of outputs
        psList *keys = psStringSplit(keywords, " ,;", true); // List of keywords
        if (keys->n != values->n && values->n != 0) {
            psError(PS_ERR_UNKNOWN, true, "Number of keywords (%ld) does not match number of "
                    "values (%ld).\n", keys->n, values->n);
            psFree(keys);
            return false;
        }
        psListIterator *keysIter = psListIteratorAlloc(keys, PS_LIST_HEAD, false); // Iterator for keywords
        psListIterator *valuesIter = psListIteratorAlloc(values, PS_LIST_HEAD, false); // Iterator for values
        psString key = NULL;            // Keyword from iteration
        while ((key = psListGetAndIncrement(keysIter))) {
            psMetadataItem *value = psListGetAndIncrement(valuesIter); // Value from iteration; may be NULL
            status &= writeSingleHeader(hdu, key, value);
        }
        psFree(keysIter);
        psFree(valuesIter);
        psFree(keys);
    } else {
        status = writeSingleHeader(hdu, keywords, item);
    }
    return status;
}

// Return the camera format appropriate for a focal plane hierarchy
static psMetadata *conceptsCameraFormat(const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // The HDU at the lowest level
    if (!hdu) {
        return NULL;
    }
    return hdu->format;
}

// Return the DATABASE metadata from the format
static psMetadata *conceptsDatabase(const psMetadata *format)
{
    bool mdok;                          // Status of MD lookup
    return psMetadataLookupMetadata(&mdok, format, "DEFAULTS");
}

// Return the TRANSLATION metadata from the format
static psMetadata *conceptsTranslation(const psMetadata *format)
{
    bool mdok;                          // Status of MD lookup
    return psMetadataLookupMetadata(&mdok, format, "TRANSLATION");
}

// Return the DEFAULTS metadata from the format
static psMetadata *conceptsDefaults(const psMetadata *format)
{
    bool mdok;                          // Status of MD lookup
    return psMetadataLookupMetadata(&mdok, format, "DEFAULTS");
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool p_pmConceptWriteToCells(const pmCell *cell, const pmConceptSpec *spec,
                             const psMetadataItem *conceptItem, const psMetadata *format)
{
    if (!cell) {
        return false;
    }
    if (!cell->config) {
        return false;
    }

    if (!format) {
        format = conceptsCameraFormat(NULL, NULL, cell);
        if (!format) {
            return false;
        }
    }

    psMetadataItem *cameraItem = psMetadataLookup(cell->config, conceptItem->name); // Version in the config
    if (!cameraItem) {
        return false;
    }

    psString nameSource = NULL; // String with the concept name and ".SOURCE" added
    psStringAppend(&nameSource, "%s.SOURCE", conceptItem->name);
    bool mdok = true;       // Status of MD lookup
    psString source = psMetadataLookupStr(&mdok, cell->config, nameSource); // The source
    if (mdok && strlen(source) > 0) {
        psTrace("psModules.concepts", 8, "%s is %s\n", nameSource, source);
        if (strcasecmp(source, "HEADER") == 0) {
            if (cameraItem->type != PS_DATA_STRING) {
                psWarning("Concept %s is specified by header, but is not of type STR --- ignored.",
                          conceptItem->name);
                psFree(nameSource);
                return false;
            }

            // Formatted version
            psMetadataItem *formatted = conceptFormat(spec, conceptItem, PM_CONCEPT_SOURCE_HEADER,
                                                      format, NULL, NULL, cell);
            if (!formatted) {
                psFree(nameSource);
                return true;
            }

            psTrace("psModules.concepts", 8, "Writing %s to header %s\n",
                    conceptItem->name, cameraItem->data.str);
            pmHDU *hdu = pmHDUGetLowest(NULL, NULL, cell); // Header data unit
            if (!hdu) {
                psError(PS_ERR_UNEXPECTED_NULL, false,
                        "Unable to find HDU to write concept %s", conceptItem->name);
                return false;
            }
            writeHeader(hdu, cameraItem->data.V, formatted);
            psFree(formatted);
        } else if (strcasecmp(source, "VALUE") == 0) {
            // Formatted version
            psMetadataItem *formatted = conceptFormat(spec, conceptItem, PM_CONCEPT_SOURCE_CELLS,
                                                      format, NULL, NULL, cell);
            if (!formatted) {
                psFree(nameSource);
                return true;
            }

            psTrace("psModules.concepts", 8, "Checking %s against camera format.\n", conceptItem->name);
            if (!compareConcepts(formatted, cameraItem)) {
                psWarning("Concept %s is specified by value in the camera format, but the values don't match",
                          conceptItem->name);
            }
            psFree(formatted);
        } else {
            psWarning("Concept source %s isn't HEADER or VALUE --- can't write", nameSource);
        }
    } else {
        // Assume it's specified by value
        psMetadataItem *formatted = conceptFormat(spec, conceptItem, PM_CONCEPT_SOURCE_CELLS,
                                                  format, NULL, NULL, cell);
        if (!formatted) {
            psFree(nameSource);
            return true;
        }

        if (!compareConcepts(formatted, cameraItem)) {
            psWarning("Concept %s is specified by value in the camera format, but the values don't match.",
                      conceptItem->name);
        }
        psFree(formatted);
    }
    psFree(nameSource);

    return true;
}

bool p_pmConceptWriteToDefaults(const pmFPA *fpa, const pmChip *chip, const pmCell *cell,
                                const pmConceptSpec *spec, const psMetadataItem *conceptItem,
                                const psMetadata *format, const psMetadata *defaults)
{
    if (!format) {
        format = conceptsCameraFormat(fpa, chip, cell);
        if (!format) {
            return false;
        }
    }
    if (!defaults) {
        defaults = conceptsDefaults(format);
        if (!defaults) {
            return false;
        }
    }

    psMetadataItem *defaultItem = p_pmConceptsReadSingleFromDefaults(conceptItem->name, defaults, fpa, chip, cell);
    if (!defaultItem) {
        return false;
    }
    psMetadataItem *formatted = conceptFormat(spec, conceptItem, PM_CONCEPT_SOURCE_DEFAULTS, format, fpa, chip, cell);
    if (!formatted) {
        return true;
    }

    if (strcmp(defaultItem->name, conceptItem->name) != 0) {
        // Correct the name to match the concept name
        defaultItem = psMetadataItemCopy(defaultItem);
        psFree(defaultItem->name);
        defaultItem->name = psStringCopy(conceptItem->name);
    } else {
        psMemIncrRefCounter(defaultItem);
    }

    if (!compareConcepts(formatted, defaultItem)) {
        psWarning("Concept %s is specified by the DEFAULTS in the camera format, but the values don't match.",
                  conceptItem->name);
    }
    psFree(defaultItem);
    psFree(formatted);

    return true;
}


bool p_pmConceptWriteToHeader(const pmFPA *fpa, const pmChip *chip, const pmCell *cell,
                              const pmConceptSpec *spec, const psMetadataItem *conceptItem,
                              const psMetadata *format, const psMetadata *translation)
{
    if (!format) {
        format = conceptsCameraFormat(fpa, chip, cell);
        if (!format) {
            return false;
        }
    }
    if (!translation) {
        translation = conceptsTranslation(format);
        if (!translation) {
            return false;
        }
    }

    psMetadataItem *headerItem = psMetadataLookup(translation, conceptItem->name); // How to format for header
    if (!headerItem) {
        return false;
    }
    if (headerItem->type == PS_DATA_METADATA) {
        // This is a menu
        psTrace("psModules.concepts", 5, "%s is of type METADATA.\n", conceptItem->name);
        headerItem = p_pmConceptsDepend(conceptItem->name, headerItem->data.md, translation, fpa, chip, cell);
        if (!headerItem) {
            return false;
        }
    }
    if (headerItem->type != PS_DATA_STRING) {
        psWarning("TRANSLATION keyword for concept %s isn't of type STR --- ignored.", conceptItem->name);
        return false;
    }
    psTrace("psModules.concepts", 3, "Writing %s to header %s\n", conceptItem->name, headerItem->data.str);
    psMetadataItem *formatted = conceptFormat(spec, conceptItem, PM_CONCEPT_SOURCE_HEADER,
                                              format, fpa, chip, cell);
    if (!formatted) {
        // Found it, but it doesn't need to be written
        return true;
    }

    pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // HDU to which to write
    if (!hdu) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find HDU to write concept %s", conceptItem->name);
        return false;
    }
    writeHeader(hdu, headerItem->data.V, formatted);
    psFree(formatted);

    return true;
}

bool p_pmConceptWriteToDatabase(const pmFPA *fpa, const pmChip *chip, const pmCell *cell,
                                pmConfig *config, const pmConceptSpec *spec,
                                const psMetadataItem *conceptItem, const psMetadata *format,
                                const psMetadata *database)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

#ifndef HAVE_PSDB
    return false;
#else

    if (!format) {
        format = conceptsCameraFormat(fpa, chip, cell);
        if (!format) {
            return false;
        }
    }
    if (!database) {
        database = conceptsDatabase(format);
        if (!database) {
            return false;
        }
    }

    psMetadataItem *dbItem = p_pmConceptsReadSingleFromDatabase(conceptItem->name, database, config,
                                                                fpa, chip, cell); // Database version
    if (!dbItem) {
        return false;
    }

    psMetadataItem *formatted = conceptFormat(spec, conceptItem, PM_CONCEPT_SOURCE_DATABASE,
                                              format, fpa, chip, cell);
    if (!formatted) {
        return false;
    }

    if (strcmp(dbItem->name, conceptItem->name) != 0) {
        // Correct the name to match the concept name
        dbItem = psMetadataItemCopy(dbItem);
        psFree(dbItem->name);
        dbItem->name = psStringCopy(conceptItem->name);
    } else {
        psMemIncrRefCounter(dbItem);
    }

    if (!compareConcepts(formatted, dbItem)) {
        psWarning("Concept %s is specified by the DATABASE in the camera "
                  "format, but the values don't match.\n", conceptItem->name);
    }
    psFree(dbItem);
    psFree(formatted);

    return true;
#endif
}


bool pmConceptWriteSingle(const pmFPA *fpa, const pmChip *chip, const pmCell *cell,
                          pmConfig *config, const psMetadataItem *conceptItem)
{
    pmConceptsInit();

    psMetadata *format = conceptsCameraFormat(fpa, chip, cell); // Camera format
    if (!format) {
        psError(PS_ERR_UNKNOWN, false, "Unable to retrieve camera format.");
        return false;
    }

    const char *name = conceptItem->name; // Name of concept

    psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications for FPA
    bool mdok;                          // Status of MD lookup
    pmConceptSpec *spec = psMetadataLookupPtr(&mdok, conceptsFPA, name); // Concept specification of interest
    if (!spec) {
        psMetadata *conceptsChip = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications for Chip
        spec = psMetadataLookupPtr(&mdok, conceptsChip, name);
        if (!spec) {
            psMetadata *conceptsCell = pmConceptsSpecs(PM_FPA_LEVEL_CELL); // Concept specifications for Cell
            spec = psMetadataLookupPtr(&mdok, conceptsCell, name);
            if (!spec) {
                psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find specification for concept %s", name);
                return false;
            }
        }
    }

    if (!p_pmConceptWriteToCells(cell, spec, conceptItem, format) &&
        !p_pmConceptWriteToDefaults(fpa, chip, cell, spec, conceptItem, format, NULL) &&
        !p_pmConceptWriteToHeader(fpa, chip, cell, spec, conceptItem, format, NULL) &&
        !p_pmConceptWriteToDatabase(fpa, chip, cell, config, spec, conceptItem, format, NULL)) {
        return false;
    }
    return true;
}


// Write all registered concepts for the specified level
static bool conceptsWrite(psMetadata **specs, // One of the concepts specifications
                          const pmFPA *fpa,   // The FPA
                          const pmChip *chip, // The chip
                          const pmCell *cell, // The cell
                          pmConfig *config, // Configuration
                          psMetadata *concepts // The concepts to write out
                         )
{
    assert(specs);
    assert(concepts);

    pmConceptsInit();

    psTrace("psModules.concepts", 3, "Writing concepts (%p %p %p)\n", fpa, chip, cell);

    psMetadata *format = conceptsCameraFormat(fpa, chip, cell); // Camera format
    if (!format) {
        psError(PS_ERR_UNKNOWN, false, "Unable to retrieve camera format.");
        return false;
    }

    psMetadata *defaults = conceptsDefaults(format); // DEFAULTS configuration
    psMetadata *translation = conceptsTranslation(format); // TRANSLATION configuration
    psMetadata *database = conceptsDatabase(format); // DATABASE configuration

    psMetadataIterator *iter = psMetadataIteratorAlloc(*specs, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL;    // Item from the specs metadata
    while ((item = psMetadataGetAndIncrement(iter))) {
        pmConceptSpec *spec = item->data.V; // The specification
        psString name = item->name; // The concept name

        psMetadataItem *concept = psMetadataLookup(concepts, name); // Concept to write

        if (!p_pmConceptWriteToCells(cell, spec, concept, format) &&
            !p_pmConceptWriteToDefaults(fpa, chip, cell, spec, concept, format, defaults) &&
            !p_pmConceptWriteToHeader(fpa, chip, cell, spec, concept, format, translation) &&
            !p_pmConceptWriteToDatabase(fpa, chip, cell, config, spec, concept, format, database)) {
            psTrace("psModules.concepts", 1, "Unable to write concept %s to any output", name);
        }
    }
    psFree(iter);

    return true;
}


bool pmConceptsWriteFPA(const pmFPA *fpa, bool propagateDown, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications
    psTrace("psModules.concepts", 5, "Writing FPA concepts: %p %p\n", conceptsFPA, fpa->concepts);
    bool success = conceptsWrite(&conceptsFPA, fpa, NULL, NULL, config, fpa->concepts);
    if (propagateDown) {
        psArray *chips = fpa->chips;        // Array of chips
        for (long i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i];  // Chip of interest
            if (chip) {
                success &= pmConceptsWriteChip(chip, false, true, config);
            }
        }
    }
    return success;
}


bool pmConceptsWriteChip(const pmChip *chip, bool propagateUp, bool propagateDown, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    psMetadata *conceptsChip = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
    psTrace("psModules.concepts", 5, "Writing chip concepts: %p %p\n", conceptsChip, chip->concepts);
    pmFPA *fpa = chip->parent;          // FPA to which the chip belongs
    bool success = conceptsWrite(&conceptsChip, fpa, chip, NULL, config, chip->concepts);
    if (propagateUp) {
        psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications
        success &= conceptsWrite(&conceptsFPA, fpa, chip, NULL, config, fpa->concepts);
    }
    if (propagateDown) {
        psArray *cells = chip->cells;        // Array of cells
        for (long i = 0; i < cells->n; i++) {
            pmCell *cell = cells->data[i];  // Cell of interest
            if (cell) {
                success &= pmConceptsWriteCell(cell, false, config);
            }
        }
    }
    return success;
}


bool pmConceptsWriteCell(const pmCell *cell, bool propagateUp, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    psMetadata *conceptsCell = pmConceptsSpecs(PM_FPA_LEVEL_CELL); // Concept specifications
    psTrace("psModules.concepts", 5, "Writing cell concepts: %p %p\n", conceptsCell, cell->concepts);
    pmChip *chip = cell->parent;        // Chip to which the cell belongs
    pmFPA *fpa = chip->parent;          // FPA to which the chip belongs

    bool success = conceptsWrite(&conceptsCell, fpa, chip, cell, config, cell->concepts);
    if (propagateUp) {
        psMetadata *conceptsChip = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
        success &= conceptsWrite(&conceptsChip, fpa, chip, cell, config, chip->concepts);
        psMetadata *conceptsFPA = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications
        success &= conceptsWrite(&conceptsFPA, fpa, chip, cell, config, fpa->concepts);
    }

    return success;
}

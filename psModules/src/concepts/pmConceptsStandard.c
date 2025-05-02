#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>			// for tolower()
#include <string.h>
#include <strings.h>			// for strn?casecmp 
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmConcepts.h"
#include "pmConceptsRead.h"
#include "pmConceptsWrite.h"
#include "pmConceptsStandard.h"

// XXX why are these functions not supporting all types (S64, U64 often missing)?

// The functions in this file are intended to be called solely within the psModules concepts code.  For this
// reason, they use "assert" instead of the PS_ASSERT_WHATEVER functions --- if there's a problem, then
// there's a BIG problem that affects all of the code.

#define COMPARE_REGIONS(a,b) (((a)->x0 == (b)->x0 && \
                               (a)->x1 == (b)->x1 && \
                               (a)->y0 == (b)->y0 && \
                               (a)->y1 == (b)->y1) ? true : false)

#define TYPE_CASE(assign, item, TYPE) \
case PS_TYPE_##TYPE: \
assign = item->data.TYPE; \
break;


// Format type for time
typedef enum {
  TIME_FORMAT_YYYYMMDD,                 // Date stored in YYYY-MM-DD order (ISO-standard)
  TIME_FORMAT_DDMMYYYY,                 // Date stored in DD-MM-YYYY order
  TIME_FORMAT_MMDDYYYY,                 // Date stored in MM-DD-YYYY order
  TIME_FORMAT_JD,                       // Date stored as JD
  TIME_FORMAT_MJD,                      // Date stored as MJD
} conceptTimeFormatType;

// Format for time
typedef struct {
    conceptTimeFormatType format;       // Format type for time
    bool separate;                      // Date and time stored separately?
    bool pre2000;                       // Year is pre-2000 (two digits only)?
} conceptTimeFormat;


static double defaultCoordScaling(const psMetadataItem *pattern)
{
    if (strcmp(pattern->name, "FPA.RA") == 0 || strcmp(pattern->name, "FPA.LATITUDE") == 0) {
        psWarning("Assuming format for %s is HOURS.\n", pattern->name);
        return M_PI / 12.0;
    }
    if (strcmp(pattern->name, "FPA.DEC") == 0 || strcmp(pattern->name, "FPA.LONGITUDE") == 0) {
        psWarning("Assuming format for %s is DEGREES.\n", pattern->name);
        return M_PI / 180.0;
    }
    psAbort("Should never ever get here.\n");
    return NAN;
}


psMetadataItem *p_pmConceptParse_CELL_READNOISE(const psMetadataItem *concept,
                                                const psMetadataItem *pattern,
                                                pmConceptSource source,
                                                const psMetadata *cameraFormat,
                                                const pmFPA *fpa,
                                                const pmChip *chip,
                                                const pmCell *cell)
{
    assert(concept);
    assert(pattern);
    assert(cameraFormat);
    assert(cell);

    float rn = psMetadataItemParseF32(concept); // Read noise
    if (isfinite(rn)) {
        bool mdok;                      // Status of MD lookup
        psMetadata *formats = psMetadataLookupMetadata(&mdok, cameraFormat, "FORMATS");
        if (mdok && formats) {
            psString format = psMetadataLookupStr(&mdok, formats, pattern->name);
            if (mdok && strlen(format) > 0) {
                if (strcasecmp(format, "ADU") == 0) {
                    float gain = psMetadataLookupF32(NULL, cell->concepts, "CELL.GAIN"); // Gain (e/ADU)
                    if (!isfinite(gain)) {
                        // Will need to update the readnoise once the gain is available
                        psMetadataAddBool(cell->concepts, PS_LIST_TAIL, "CELL.READNOISE.UPDATE",
                                          PS_META_REPLACE,
                                          "Need to update CELL.READNOISE when the gain is available.", true);
                    } else {
                        rn *= gain;
                    }
                } else {
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised format for CELL.READNOISE: %s",
                            format);
                    return NULL;
                }
            }
        }
    }

    return psMetadataItemAllocF32(pattern->name, pattern->comment, rn);
}

psMetadataItem *p_pmConceptFormat_CELL_READNOISE(const psMetadataItem *concept,
                                                 pmConceptSource source,
                                                 const psMetadata *cameraFormat,
                                                 const pmFPA *fpa,
                                                 const pmChip *chip,
                                                 const pmCell *cell)
{
    assert(concept);
    assert(cell);
    assert(concept->type == PS_DATA_F32);

    float rn = concept->data.F32;       // Read noise
    if (isfinite(rn)) {
        bool mdok;                      // Status of MD lookup
        psMetadata *formats = psMetadataLookupMetadata(&mdok, cameraFormat, "FORMATS");
        if (mdok && formats) {
            psString format = psMetadataLookupStr(&mdok, formats, concept->name);
            if (mdok && strlen(format) > 0) {
                if (strcasecmp(format, "ADU") == 0) {
                    if (!psMetadataLookup(cell->concepts, "CELL.READNOISE.UPDATE")) {
                        // Gain correction has been applied, so we need to reverse it to format
                        float gain = psMetadataLookupF32(NULL, cell->concepts, "CELL.GAIN"); // Gain (e/ADU)
                        if (!isfinite(gain)) {
                            psWarning("CELL.READNOISE is supposed to be in ADU, but CELL.GAIN isn't set -- forcing to 1.0");
                            gain = 1.0;
                        }
                        rn /= gain;
                    }
                } else {
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised format for CELL.READNOISE: %s",
                            format);
                    return NULL;
                }
            }
        }
    }

    return psMetadataItemAllocF32(concept->name, concept->comment, rn);
}


// TELTEMPS : parse a list of the form 'X1 X2 X3 X4 X5 ...' : for now use median
psMetadataItem *p_pmConceptParse_TELTEMPS(const psMetadataItem *concept,
                                          const psMetadataItem *pattern,
                                          pmConceptSource source,
                                          const psMetadata *cameraFormat,
                                          const pmFPA *fpa,
                                          const pmChip *chip,
                                          const pmCell *cell)
{
    assert(concept);
    assert(pattern);
    double value = NAN;
    switch (concept->type) {
      case PS_TYPE_F32:
        value = concept->data.F32;
        break;
      case PS_TYPE_F64:
        value = concept->data.F64;
        break;
      case PS_DATA_STRING: {
          // parse the list of values into an array of substrings
          psArray *strValues = psStringSplitArray (concept->data.V, " ,;", false);
          assert (strValues);

          // convert the substrings into a vector
          psVector *fltValues = psVectorAlloc (strValues->n, PS_DATA_F32);
          for (int i = 0; i < strValues->n; i++) {
              fltValues->data.F32[i] = atof(strValues->data[i]);
          }

          // take the (for now) MEDIAN of the data
          psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

          if (!psVectorStats (stats, fltValues, NULL, NULL, 0)) {
              psAbort ("how can this stats function fail?");
          }

          value = stats->sampleMedian;
          psFree (stats);
          psFree (fltValues);
          psFree (strValues);
          break;
      }

      default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Invalid type for %s (%x)\n", pattern->name, concept->type);
        return NULL;
    }

    psMetadataItem *item = psMetadataItemAllocF32(pattern->name, pattern->comment, value);
    return (item);
}

// FPA.FILTER
psMetadataItem *p_pmConceptParse_FPA_FILTER(const psMetadataItem *concept,
                                            const psMetadataItem *pattern,
                                            pmConceptSource source,
                                            const psMetadata *cameraFormat,
                                            const pmFPA *fpa,
                                            const pmChip *chip,
                                            const pmCell *cell)
{
    assert(concept);
    assert(pattern);
    assert(fpa);
    assert(fpa->camera);

    if (concept->type != PS_DATA_STRING) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for %s (%x) is not STR\n",
                pattern->name, concept->type);
        return NULL;
    }
    if (!concept->data.str || strlen(concept->data.str) == 0) {
        return psMetadataItemAllocStr(pattern->name, pattern->comment, "");
    }

    bool mdok;                          // Status of MD lookup
    psMetadata *filters = psMetadataLookupMetadata(&mdok, fpa->camera, "FILTER.ID");
    if (!mdok || !filters) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find FILTER.ID in camera configuration.\n");
        return NULL;
    }

    // the metadata is in the format (internal) STR (external)
    // do a reverse lookup to get the internal name
    psMetadataIterator *iter = psMetadataIteratorAlloc(filters, PS_LIST_HEAD, NULL); // Iterator for filters
    psMetadataItem *item;               // Item from iteration
    char *name = NULL;                  // The winning name
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_STRING) {
            psWarning("Type for %s (%x) in FILTER.ID in camera configuration is not STR\n",
                      item->name, item->type);
            continue;
        }
        if (strcmp(item->data.str, concept->data.str) == 0) {
            name = item->name;
            break;
        }
    }
    psFree(iter);
    if (!name) {
        psError(PS_ERR_UNEXPECTED_NULL, false,
                "Unable to find any filter matching %s in FILTER.ID in camera configuration.\n",
                concept->data.str);
        return NULL;
    }

    return psMetadataItemAllocStr(pattern->name, pattern->comment, name);
}

psMetadataItem *p_pmConceptFormat_FPA_FILTER(const psMetadataItem *concept,
                                             pmConceptSource source,
                                             const psMetadata *cameraFormat,
                                             const pmFPA *fpa,
                                             const pmChip *chip,
                                             const pmCell *cell)
{
    assert(concept);
    assert(fpa);
    assert(fpa->camera);

    if (concept->type != PS_DATA_STRING) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for %s (%x) is not STR\n",
                concept->name, concept->type);
        return NULL;
    }
    if (!concept->data.str || strlen(concept->data.str) == 0) {
        return psMetadataItemAllocStr(concept->name, concept->comment, "");
    }

    bool mdok;                          // Status of MD lookup
    psMetadata *filters = psMetadataLookupMetadata(&mdok, fpa->camera, "FILTER.ID");
    if (!mdok || !filters) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find FILTER.ID in camera configuration.\n");
        return NULL;
    }

    const char *key = concept->data.str;        // The name to look up
    if (!key || strlen(key) == 0) {
        return psMetadataItemAllocStr(concept->name, concept->comment, NULL);
    }

    // the metadata is in the format (internal) STR (external)
    // find the first internal name that matches
    psMetadataItem *item = psMetadataLookup (filters, key);
    if (!item) {
        psError(PS_ERR_UNEXPECTED_NULL, true,
                "Unable to find %s in FILTER.ID in camera configuration.\n", key);
        return NULL;
    }

    if (item->type == PS_DATA_STRING) {
        return psMetadataItemAllocStr(concept->name, concept->comment, item->data.V);
    }

    if (item->type == PS_DATA_METADATA_MULTI) {
        psMetadataItem *entry = psListGet (item->data.list, PS_LIST_HEAD);
        if (!entry) {
            psError(PS_ERR_UNEXPECTED_NULL, true,
                    "List for %s in FILTER.ID in camera configuration is empty.\n", key);
            return NULL;
        }
        return psMetadataItemAllocStr(concept->name, concept->comment, entry->data.V);
    }

    psError(PS_ERR_UNEXPECTED_NULL, true,
            "Unable to find %s in FILTER.ID in camera configuration.\n", key);
    return NULL;
}

// FPA.OBSTYPE
// convert concept->data.str to new value 
psMetadataItem *p_pmConceptParse_FPA_OBSTYPE(const psMetadataItem *concept,
                                            const psMetadataItem *pattern,
                                            pmConceptSource source,
                                            const psMetadata *cameraFormat,
                                            const pmFPA *fpa,
                                            const pmChip *chip,
                                            const pmCell *cell)
{
    assert(concept);
    assert(pattern);
    assert(fpa);
    assert(fpa->camera);

    if (concept->type != PS_DATA_STRING) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for %s (%x) is not STR\n",
                pattern->name, concept->type);
        return NULL;
    }
    if (!concept->data.str || strlen(concept->data.str) == 0) {
        return psMetadataItemAllocStr(pattern->name, pattern->comment, "");
    }

    bool mdok;                          // Status of MD lookup
    psMetadata *table = psMetadataLookupMetadata(&mdok, fpa->camera, "OBSTYPE.TABLE");
    if (!mdok || !table) {
	// if the table is not defined, pass the supplied value unmodified
        return psMetadataItemAllocStr(pattern->name, pattern->comment, concept->data.str);
    }

    // the metadata is in the format (external) STR (internal) 
    // do a lookup to get the internal name
    char *extname = psStringCopy (concept->data.str);
    for (int i = 0; i < strlen(extname); i++) {
	extname[i] = tolower(extname[i]);
    }
    char *name = psMetadataLookupStr (&mdok, table, extname);
    psFree(extname);
    if (!name) {
	// if the entry is not defined, pass the supplied value unmodified
        return psMetadataItemAllocStr(pattern->name, pattern->comment, concept->data.str);
    }

    return psMetadataItemAllocStr(pattern->name, pattern->comment, name);
}

// convert concept->data.str to new value 
psMetadataItem *p_pmConceptFormat_FPA_OBSTYPE(const psMetadataItem *concept,
                                             pmConceptSource source,
                                             const psMetadata *cameraFormat,
                                             const pmFPA *fpa,
                                             const pmChip *chip,
                                             const pmCell *cell)
{
    assert(concept);
    assert(fpa);
    assert(fpa->camera);

    if (concept->type != PS_DATA_STRING) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for %s (%x) is not STR\n",
                concept->name, concept->type);
        return NULL;
    }
    if (!concept->data.str || strlen(concept->data.str) == 0) {
        return psMetadataItemAllocStr(concept->name, concept->comment, "");
    }

    bool mdok;                          // Status of MD lookup
    psMetadata *table = psMetadataLookupMetadata(&mdok, fpa->camera, "OBSTYPE.TABLE");
    if (!mdok || !table) {
	// if the table is not defined, pass the supplied value unmodified
        return psMetadataItemAllocStr(concept->name, concept->comment, concept->data.str);
    }

    const char *key = concept->data.str;        // The name to look up
    if (!key || strlen(key) == 0) {
        return psMetadataItemAllocStr(concept->name, concept->comment, NULL);
    }

    // the metadata is in the format (internal) STR (external)
    // do a reverse lookup to get the internal name
    psMetadataIterator *iter = psMetadataIteratorAlloc(table, PS_LIST_HEAD, NULL); // Iterator for filters
    psMetadataItem *item;               // Item from iteration
    char *name = NULL;                  // The winning name
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_STRING) {
            psWarning("Type for %s (%x) in OBSTYPE.TABLE in camera configuration is not STR\n", item->name, item->type);
            continue;
        }
        if (strcmp(item->data.str, key) == 0) {
            name = item->name;
            break;
        }
    }
    psFree(iter);

    if (!name) {
	return psMetadataItemAllocStr(concept->name, concept->comment, key);
    }
    return psMetadataItemAllocStr(concept->name, concept->comment, name);
}

// FPA.RA and FPA.DEC
psMetadataItem *p_pmConceptParse_FPA_Coords(const psMetadataItem *concept,
                                            const psMetadataItem *pattern,
                                            pmConceptSource source,
                                            const psMetadata *cameraFormat,
                                            const pmFPA *fpa,
                                            const pmChip *chip,
                                            const pmCell *cell)
{
    assert(concept);
    assert(pattern);
    assert(cameraFormat);

    double coords = NAN;                // The coordinates
    switch (concept->type) {
      case PS_TYPE_F32:
        coords = concept->data.F32;
        break;
      case PS_TYPE_F64:
        coords = concept->data.F64;
        break;
      case PS_DATA_STRING:
        // Sexagesimal format
        {
            int big, medium;
            float small;
            bool negative =  *((char *)concept->data.V) == '-'; // check for sign explicitly since -0 is not less than 0

            // XXX: Upgrade path is to allow dd:mm.mmm
            if (sscanf(concept->data.V, "%d:%d:%f", &big, &medium, &small) != 3 &&
                sscanf(concept->data.V, "%d %d %f", &big, &medium, &small) != 3)
            {
                psError(PS_ERR_UNKNOWN, true, "Cannot interpret %s: %s\n", pattern->name, concept->data.str);
                break;
            }
            coords = abs(big) + (float)medium/60.0 + small/3600.0;
            if (negative)
            {
                coords *= -1.0;
            }
        }
        break;
      default:
        psError(PS_ERR_UNKNOWN, true, "%s concept is of an unexpected type: %x\n",
                pattern->name, concept->type);
        return NULL;
    }

    // How to interpret the coordinates
    bool mdok = true;           // Status of MD lookup
    psMetadata *formats = psMetadataLookupMetadata(&mdok, cameraFormat, "FORMATS");
    if (mdok && formats) {
        psString format = psMetadataLookupStr(&mdok, formats, pattern->name);
        if (mdok && strlen(format) > 0) {
            if (strcasecmp(format, "HOURS") == 0) {
                coords *= M_PI / 12.0;
            } else if (strcasecmp(format, "DEGREES") == 0) {
                coords *= M_PI / 180.0;
            } else if (strcasecmp(format, "RADIANS") == 0) {
                // No action required
            } else {
                coords *= defaultCoordScaling(pattern);
            }
        } else {
            coords *= defaultCoordScaling(pattern);
        }
    } else {
        coords *= defaultCoordScaling(pattern);
    }

    return psMetadataItemAllocF64(pattern->name, pattern->comment, coords);
}


// FPA.RA and FPA.DEC
psMetadataItem *p_pmConceptFormat_FPA_Coords(const psMetadataItem *concept,
                                             pmConceptSource source,
                                             const psMetadata *cameraFormat,
                                             const pmFPA *fpa,
                                             const pmChip *chip,
                                             const pmCell *cell)
{
    assert(concept);
    assert(cameraFormat);

    double coords = concept->data.F64;  // The coordinates

    if (!isfinite(coords)) {
        return psMetadataItemAllocF32(concept->name, concept->comment, NAN);
    }

    // How to interpret the coordinates
    bool mdok = true;                   // Status of MD lookup
    psMetadata *formats = psMetadataLookupMetadata(&mdok, cameraFormat, "FORMATS");
    bool sexagesimal = false;           // Write sexagesimal format?
    if (mdok && formats) {
        psString format = psMetadataLookupStr(&mdok, formats, concept->name);
        if (mdok && strlen(format) > 0) {
            if (strcasecmp(format, "HOURS") == 0) {
                coords /= M_PI / 12.0;
            } else if (strcasecmp(format, "DEGREES") == 0) {
                coords /= M_PI / 180.0;
            } else if (strcasecmp(format, "RADIANS") == 0) {
                // No action required
            } else {
                coords /= defaultCoordScaling(concept);
            }
        } else {
            coords /= defaultCoordScaling(concept);
        }

        if (strcmp(concept->name, "FPA.RA") == 0 || strcmp(concept->name, "FPA.DEC") == 0) {
            psString ra = psMetadataLookupStr(&mdok, formats, "FPA.RA"); // Format for RA
            psString dec = psMetadataLookupStr(&mdok, formats, "FPA.DEC"); // Format for Dec
            if (ra && strcasecmp(ra, "HOURS") == 0 && dec && strcasecmp(dec, "DEGREES") == 0) {
                sexagesimal = true;
            }
        }
    } else {
        coords /= defaultCoordScaling(concept);
    }

    psMetadataItem *coordItem = NULL;   // Item with coordinates, to return
    if (sexagesimal) {
        int big, medium;                    // Degrees and minutes
        float small;                        // Seconds
        bool negative = (coords < 0);       // Are we working below zero?
        coords = fabs(coords);
        big = (int)abs(coords);
        coords -= big;
        medium = 60.0 * coords;
        coords -= medium / 60.0;
        small = 3600.0 * coords;
        small = (float)((int)(small * 1000.0)) / 1000.0;
        psString coordString = NULL;        // String with the coordinates in sexagesimal format
        psStringAppend(&coordString, "%s%02d:%02d:%06.3f",
                       negative ? "-" : (strcmp(concept->name, "FPA.DEC") == 0 ? "+" : ""),
                       big, medium, small);
        coordItem = psMetadataItemAllocStr(concept->name, concept->comment, coordString);
        psFree(coordString);
    } else {
        coordItem = psMetadataItemAllocF64(concept->name, concept->comment, coords);
    }

    return coordItem;
}


psMetadataItem *p_pmConceptParse_CELL_TRIMSEC(const psMetadataItem *concept,
                                              const psMetadataItem *pattern,
                                              pmConceptSource source,
                                              const psMetadata *cameraFormat,
                                              const pmFPA *fpa,
                                              const pmChip *chip,
                                              const pmCell *cell)
{
    assert(concept);
    assert(cell);
    assert(pattern);

    int xParity = 0;
    int yParity = 0;
    psRegion *trimsec = psRegionAlloc(0, 0, 0, 0);

    if (concept->type != PS_DATA_STRING) {
        psError(PS_ERR_UNKNOWN, true, "CELL.TRIMSEC after read is not of type STR (%x)\n", concept->type);
        psFree(trimsec);
        return NULL;
    } else {
        // allow for x and y flips in regions
        *trimsec = psRegionAndParityFromString(&xParity, &yParity, concept->data.V);
    }

    // Need to correct for binning when CELL.TRIMSEC are specified immutably in the CELLS
    if (source == PM_CONCEPT_SOURCE_CELLS) {
        psMetadataAddBool(cell->concepts, PS_LIST_TAIL, "CELL.TRIMSEC.UPDATE", PS_META_REPLACE,
                          "Need to update CELL.TRIMSEC when the binning is available.",
                          true);
    }

    psMetadataItem *item = psMetadataItemAllocPtr(pattern->name, PS_DATA_REGION, pattern->comment, trimsec);
    psFree(trimsec);
    return item;
}


psList *p_pmConceptParseRegions(const char *region)
{
    psList *list = psListAlloc(NULL);   // List of regions
    if (!region || strlen(region) == 0) {
        // Empty list
        return list;
    }

    // a single BIASSEC is of the form [AAAA]
    // we may have multiple BIASSEC entries separated by space, commas, or semicolons
    int xParity = 0, yParity = 0;       // Parity of region
    char *p = strchr (region, '[');
    while (p) {
        char *q = strchr (p, ']');
        if (!q) {
            break;
        }
        char *regionString = psStringAlloc(q - p + 2);
        strncpy (regionString, p, q - p + 1);
        regionString[q - p + 1] = 0;

        psRegion *region = psAlloc(sizeof(psRegion)); // The region
        *region = psRegionAndParityFromString(&xParity, &yParity, regionString);
        psListAdd(list, PS_LIST_TAIL, region);
        psFree(region);           // Drop reference
        psFree(regionString);     // Drop reference

        p = strchr (q, '[');
    }

    return list;
}

psMetadataItem *p_pmConceptParse_CELL_BIASSEC(const psMetadataItem *concept,
                                              const psMetadataItem *pattern,
                                              pmConceptSource source,
                                              const psMetadata *cameraFormat,
                                              const pmFPA *fpa,
                                              const pmChip *chip,
                                              const pmCell *cell)
{
    assert(concept);
    assert(cell);
    assert(pattern);

    psList *biassecs = NULL; // List of bias sections

    switch (concept->type) {
      case PS_DATA_STRING: {
          biassecs = p_pmConceptParseRegions(concept->data.V);
          break;
      }
      case PS_DATA_LIST: {
          biassecs = psListAlloc(NULL);
          psList *regions = concept->data.V; // The list of regions
          psListIterator *regionsIter = psListIteratorAlloc(regions, PS_LIST_HEAD, false); // Iterator
          psMetadataItem *regionItem = NULL; // Item from list iteration
          while ((regionItem = psListGetAndIncrement(regionsIter))) {
              if (regionItem->type != PS_DATA_STRING) {
                  psWarning("CELL.BIASSEC member is not of type STR --- ignored.\n");
                  continue;
              }
              int xParity = 0;
              int yParity = 0;
              psRegion *region = psAlloc(sizeof(psRegion)); // The region
              *region = psRegionAndParityFromString(&xParity, &yParity, regionItem->data.V);
              psListAdd(biassecs, PS_LIST_TAIL, region);
              psFree(region);           // Drop reference
          }
          psFree(regionsIter);
          break;
      }
      default:
        psError(PS_ERR_UNKNOWN, true, "CELL.BIASSEC after read is not of type STRING or LIST --- assuming "
                "blank.\n");
    }

    // Need to correct for binning when CELL.BIASSEC are specified immutably in the CELLS
    if (source == PM_CONCEPT_SOURCE_CELLS) {
        psMetadataAddBool(cell->concepts, PS_LIST_TAIL, "CELL.BIASSEC.UPDATE", 0,
                          "Need to update CELL.BIASSEC when the binning is available.",
                          true);
    }

    psMetadataItem *item = psMetadataItemAllocPtr(pattern->name, PS_DATA_LIST, pattern->comment, biassecs);
    psFree(biassecs);               // Drop reference
    return item;
}

// CELL.XBIN and CELL.YBIN
psMetadataItem *p_pmConceptParse_CELL_Binning(const psMetadataItem *concept,
                                              const psMetadataItem *pattern,
                                              pmConceptSource source,
                                              const psMetadata *cameraFormat,
                                              const pmFPA *fpa,
                                              const pmChip *chip,
                                              const pmCell *cell)
{
    assert(concept);
    assert(pattern);

    int binning = 1;                    // Binning factor in x
    switch (concept->type) {
      case PS_DATA_STRING: {
          psString binString = concept->data.V; // The string containing the binning
          if ((strcmp(pattern->name, "CELL.XBIN") == 0 && sscanf(binString, "%d %*d", &binning) != 1 &&
               sscanf(binString, "%d,%*d", &binning) != 1) ||
              (strcmp(pattern->name, "CELL.YBIN") == 0 && sscanf(binString, "%*d %d", &binning) != 1 &&
               sscanf(binString, "%*d,%d", &binning) != 1)) {
              psError(PS_ERR_UNKNOWN, true, "Unable to parse string to get %s: %s\n", pattern->name, binString);
          }
          break;
      }
        TYPE_CASE(binning, concept, U8);
        TYPE_CASE(binning, concept, U16);
        TYPE_CASE(binning, concept, U32);
        // TYPE_CASE(binning, concept, U64);
        TYPE_CASE(binning, concept, S8);
        TYPE_CASE(binning, concept, S16);
        TYPE_CASE(binning, concept, S32);
        // TYPE_CASE(binning, concept, S64);
      default:
        psError(PS_ERR_UNKNOWN, true, "Note sure how to parse %s of type %x --- assuming 1.\n", pattern->name,
                concept->type);
    }

    return psMetadataItemAllocS32(pattern->name, pattern->comment, binning);
}

// VIDEOCELLS
psMetadataItem *p_pmConceptParse_VideoCell(const psMetadataItem *concept,
					  const psMetadataItem *pattern,
					  pmConceptSource source,
					  const psMetadata *cameraFormat,
					  const pmFPA *fpa,
					  const pmChip *chip,
					  const pmCell *cell)
{
  assert(concept);
  assert(pattern);
  bool has_video_cell = false;

  if (concept->type == PS_DATA_BOOL) {
    has_video_cell = concept->data.B;
  } else { 
    if (concept->type != PS_DATA_STRING) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for %s (%x) is not string\n",
                concept->name, concept->type);
        return NULL;
      }

      char *Vptr = strchr(concept->data.V,'V');
      if (Vptr) {
        has_video_cell = true;
      }
  }

  return psMetadataItemAllocBool(pattern->name, pattern->comment, has_video_cell);
}
  
   

// BTOOLAPP
psMetadataItem *p_pmConceptParse_BTOOLAPP(const psMetadataItem *concept,
					  const psMetadataItem *pattern,
					  pmConceptSource source,
					  const psMetadata *cameraFormat,
					  const pmFPA *fpa,
					  const pmChip *chip,
					  const pmCell *cell)
{
  assert(concept);
  assert(pattern);

  int bt_status = 0;

  if (concept->type != PS_DATA_BOOL) {
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type for %s (%x) is not BOOL\n",
	    concept->name, concept->type);
    if (concept->type != PS_DATA_S32) {
      psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Wasn't the type I'd guessed either.\n");
      return NULL;
    }
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Looks like an S32 value? (%d)\n",
	    concept->data.S32);

    if (concept->data.S32 == 0) {
      bt_status = 1;
    }
    else if (concept->data.S32 == 1) {
      bt_status = -2;
    }
  }

  if (concept->data.B == true) {
    bt_status = -2;
  }
  else if (concept->data.B == false) {
    bt_status = 1 ;
  }
  
  return psMetadataItemAllocS32(concept->name, concept->comment, bt_status);
}  
psMetadataItem *p_pmConceptFormat_BTOOLAPP(const psMetadataItem *concept,
					   pmConceptSource source,
					   const psMetadata *cameraFormat,
					   const pmFPA *fpa,
					   const pmChip *chip,
					   const pmCell *cell)
{
  assert(concept);

  if (concept->type != PS_DATA_S32) {
    return NULL;
  }

  if (concept->data.S32 == 0) {
    return NULL;
  }
  else if (concept->data.S32 == -2) {
    return psMetadataItemAllocBool(concept->name,concept->comment,true);
  }
  else if (concept->data.S32 == 1) {
    return psMetadataItemAllocBool(concept->name,concept->comment,false);
  }
  else {
    return NULL;
  }

}  


// Get the current value of a concept
static psMetadataItem *conceptGet(const pmFPA *fpa, // FPA of interest
                                  const pmChip *chip, // Chip of interest, or NULL
                                  const pmCell *cell, // Cell of interest, or NULL
                                  const char *name // Concept name
    )
{
    psMetadataItem *item = NULL;        // Item with time system
    if (cell) {
        item = psMetadataLookup(cell->concepts, name);
    }
    if (!item && chip) {
        item = psMetadataLookup(chip->concepts, name);
    }
    if (!item && fpa) {
        item = psMetadataLookup(fpa->concepts, name);
    }
    if (!item) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find %s in concepts", name);
        return NULL;
    }
    return item;
}

static conceptTimeFormat conceptGetTimeFormat(const char *name, // Concept name ("CELL.TIME" or "FPA.TIME")
                                              const psMetadata *cameraFormat // Camera format
    )
{
    conceptTimeFormat time;               // Time format, to return
    time.format = TIME_FORMAT_YYYYMMDD;
    time.separate = false;
    time.pre2000 = false;

    bool mdok = true;                   // Status of MD lookup
    psMetadata *formats = psMetadataLookupMetadata(&mdok, cameraFormat, "FORMATS"); // The formats
    if (mdok && formats) {
        psString format = psMetadataLookupStr(&mdok, formats, name); // The formats for eg, CELL.TIME
        if (mdok && format && strlen(format) > 0) {
            psList *formatList = psStringSplit(format, " ,;", false); // List of formats specified
            psListIterator *formatListIter = psListIteratorAlloc(formatList, PS_LIST_HEAD, false); // Iterator
            while ((format = psListGetAndIncrement(formatListIter))) {
                if (strcasecmp(format, "SEPARATE") == 0) {
                    time.separate = true;
                } else if (strcasecmp(format, "YYYYMMDD") == 0) {
                    time.format = TIME_FORMAT_YYYYMMDD;
                } else if (strcasecmp(format, "MMDDYYYY") == 0) {
                    time.format = TIME_FORMAT_MMDDYYYY;
                } else if (strcasecmp(format, "DDMMYYYY") == 0) {
                    time.format = TIME_FORMAT_DDMMYYYY;
                } else if (strcasecmp(format, "ISO") == 0) {
                    time.format = TIME_FORMAT_YYYYMMDD;
                } else if (strcasecmp(format, "YEAR.FIRST") == 0) {
                    time.format = TIME_FORMAT_YYYYMMDD;
                } else if (strcasecmp(format, "USA") == 0) {
                    time.format = TIME_FORMAT_MMDDYYYY;
                } else if (strcasecmp(format, "BACKWARDS") == 0) {
                    time.format = TIME_FORMAT_DDMMYYYY;
                } else if (strcasecmp(format, "PRE2000") == 0) {
                    time.pre2000 = true;
                } else if (strcasecmp(format, "MJD") == 0) {
                    time.format = TIME_FORMAT_MJD;
                } else if (strcasecmp(format, "JD") == 0) {
                    time.format = TIME_FORMAT_JD;
                } else {
                    psWarning("Unrecognised FORMATS option for %s: %s --- ignored.", name, format);
                }
            }
            psFree(formatListIter);
            psFree(formatList);
        }
    }

    return time;
}

// Determine the corresponding TIMESYS for one of the TIME concepts
static psTimeType conceptGetTimesysForTime(const char *name, // Concept name ("CELL.TIME" or "FPA.TIME")
                                           const pmFPA *fpa, // FPA of interest
                                           const pmChip *chip, // Chip of interest, or NULL
                                           const pmCell *cell // Cell of interest, or NULL
                                           )
{
    assert(name);

    psString timesysName = psStringCopy(name); // e.g., "CELL.TIME" --> "CELL.TIMESYS"
    psStringSubstitute(&timesysName, "TIMESYS", "TIME");
    psMetadataItem *item = conceptGet(fpa, chip, cell, timesysName); // Time system
    psFree(timesysName);

    if (!item || item->type != PS_DATA_S32) {
        psWarning("Unable to find %s in format file --- assuming UTC", timesysName);
        return PS_TIME_UTC;
    }
    return item->data.S32;
}

// Set the corresponding TIMESYS for one of the TIME concepts
static bool conceptSetTimesysForTime(const char *name, // Concept name ("CELL.TIME" or "FPA.TIME")
                                     const pmFPA *fpa, // FPA of interest
                                     const pmChip *chip, // Chip of interest, or NULL
                                     const pmCell *cell, // Cell of interest, or NULL
                                     psTimeType timeSys // The time system value
                                     )
{
    assert(name);

    psString timesysName = psStringCopy(name); // e.g., "CELL.TIME" --> "CELL.TIMESYS"
    psStringSubstitute(&timesysName, "TIMESYS", "TIME");
    psMetadataItem *item = conceptGet(fpa, chip, cell, timesysName); // Time system
    psFree(timesysName);

    if (!item) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find %s in concepts when setting %s\n",
                timesysName, name);
        return false;
    }

    if (item->data.S32 != -1 && item->data.S32 != timeSys) {
        psWarning("Time system is set to %x; but should be %x", item->data.S32, timeSys);
    }

    item->data.S32 = timeSys;

    return true;
}

psMetadataItem *p_pmConceptParse_TIMESYS(const psMetadataItem *concept,
                                         const psMetadataItem *pattern,
                                         pmConceptSource source,
                                         const psMetadata *cameraFormat,
                                         const pmFPA *fpa,
                                         const pmChip *chip,
                                         const pmCell *cell)
{
    assert(concept);
    assert(pattern);

    psTimeType timeSys = PS_TIME_UTC;   // The time system
    psString sys = concept->data.V;     // The time system string
    if (concept->type != PS_DATA_STRING || strlen(sys) <= 0) {
        // XXX is this too low verbosity?
        psWarning("Can't interpret %s --- assuming UTC (other options: TAI, UT1, TT).", pattern->name);
    } else if (strcasecmp(sys, "TAI") == 0) {
        timeSys = PS_TIME_TAI;
    } else if (strcasecmp(sys, "UTC") == 0) {
        timeSys = PS_TIME_UTC;
    } else if (strcasecmp(sys, "UT1") == 0) {
        timeSys = PS_TIME_UT1;
    } else if (strcasecmp(sys, "TT") == 0) {
        timeSys = PS_TIME_TT;
    } else {
        // XXX is this too low verbosity?
        psWarning("Can't interpret %s --- assuming UTC (other options: TAI, UT1, TT).", pattern->name);
    }

    psMetadataItem *old = conceptGet(fpa, chip, cell, pattern->name); // Old value
    if (old && old->data.S32 != -1 && old->data.S32 != timeSys) {
        psWarning("%s is already set (%x) and not consistent with new value (%x)",
                  pattern->name, old->data.S32, timeSys);
    }

    return psMetadataItemAllocS32(pattern->name, pattern->comment, timeSys);
}



psMetadataItem *p_pmConceptParse_TIME(const psMetadataItem *concept,
                                      const psMetadataItem *pattern,
                                      pmConceptSource source,
                                      const psMetadata *cameraFormat,
                                      const pmFPA *fpa,
                                      const pmChip *chip,
                                      const pmCell *cell)
{
    assert(concept);
    assert(cameraFormat);

    psTimeType timeSys = conceptGetTimesysForTime(pattern->name, fpa, chip, cell); // Time system

    conceptTimeFormat timeFormat = conceptGetTimeFormat(pattern->name, cameraFormat); // Format for time

    psTime *time = NULL;                // The time
    switch (concept->type) {
      case PS_DATA_LIST: {
          if (!timeFormat.separate) {
              psWarning ("DATE and TIME stored separately, but not specified in format\n");
          }
          // The date and time are stored separately
          // Assume the date is first and the time second
          psList *dateTime = concept->data.V; // The list containing items for date and time
          if (psListLength(dateTime) != 2) {
              psError(PS_ERR_BAD_PARAMETER_SIZE, false,
                      "Unable to parse %s: date and time are not both available.", pattern->name);
              return NULL;
          }
          psMetadataItem *dateItem = psListGet(dateTime, PS_LIST_HEAD); // Item containing the date
          if (!dateItem) {
              psError(PS_ERR_UNKNOWN, true, "Date is not found.\n");
              return NULL;
          }
          if (dateItem->type != PS_DATA_STRING) {
              psError(PS_ERR_UNKNOWN, true, "Date is not of type STR.\n");
              return NULL;
          }
          psString dateString = dateItem->data.V; // The string with the date
          int day = 0, month = 0, year = 0;
          if (sscanf(dateString, "%d-%d-%d", &year, &month, &day) != 3 &&
              sscanf(dateString, "%d/%d/%d", &year, &month, &day) != 3) {
              psError(PS_ERR_UNKNOWN, true, "Unable to read date: %s\n", dateString);
              return NULL;
          }
          switch (timeFormat.format) {
            case TIME_FORMAT_DDMMYYYY: {
                // Need to switch days and years
                int temp = day;
                day = year;
                year = temp;
                break;
            }
            case TIME_FORMAT_MMDDYYYY: {
                // Need to switch everything around.... Yanks!
                int temp = day;
                day = month;
                month = year;
                year = temp;
                break;
            }
            default:
              break;
          }
          if (year < 100) {
              if (timeFormat.pre2000) {
                  year += 1900;
              } else {
                  year += 2000;
              }
          }
          sprintf(dateString,"%04d-%02d-%02d", year, month, day);

          psMetadataItem *timeItem = psListGet(dateTime, PS_LIST_HEAD + 1); // Item containing the time
          if (!timeItem) {
              psError(PS_ERR_UNKNOWN, true, "Time is not found.\n");
              return NULL;
          }
          psString timeString = NULL; // The string with the time
          if (timeItem->type == PS_DATA_STRING) {
              timeString = timeItem->data.V;
          } else {
              // Assume that time is specified in Second of Day (!)
              double seconds = NAN;
              switch (timeItem->type) {
                  TYPE_CASE(seconds, timeItem, U8);
                  TYPE_CASE(seconds, timeItem, U16);
                  TYPE_CASE(seconds, timeItem, U32);
                  TYPE_CASE(seconds, timeItem, S8);
                  TYPE_CASE(seconds, timeItem, S16);
                  TYPE_CASE(seconds, timeItem, S32);
                  TYPE_CASE(seconds, timeItem, F32);
                  TYPE_CASE(seconds, timeItem, F64);
                default:
                  psError(PS_ERR_UNKNOWN, true, "Time is not of an expected type: %x\n", timeItem->type);
                  return NULL;
              }
              // Now print to timeString as "hh:mm:ss.ss"
              int hours = seconds / 3600;
              seconds -= (double)hours * 3600.0;
              int minutes = seconds / 60;
              seconds -= (double)minutes * 60.0;
              psStringAppend(&timeString, "%02d:%02d:%02f", hours, minutes, seconds);
          }
          psString dateTimeString = NULL;
          psStringAppend(&dateTimeString, "%sT%s", dateString, timeString);
          time = psTimeFromISO(dateTimeString, timeSys);
          psFree(dateTimeString);
          break;
      }
      case PS_DATA_STRING: {
          psString timeString = concept->data.V;   // String with the time
          switch (timeFormat.format) {
            case TIME_FORMAT_JD: {
                double timeValue = strtod (timeString, NULL);
                time = psTimeFromJD(timeValue);
                break;
            }
            case TIME_FORMAT_MJD: {
                double timeValue = strtod (timeString, NULL);
                time = psTimeFromMJD(timeValue);
                break;
            }
            case TIME_FORMAT_YYYYMMDD: {
                // this is ISO-standard
                time = psTimeFromISO(timeString, timeSys);
                break;
            }
            default:
              psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to interpret time string: %s", timeString);
              return NULL;
          }
          break;
      }
      case PS_TYPE_F32: {
          double timeValue = (double)concept->data.F32;
          switch (timeFormat.format) {
            case TIME_FORMAT_JD:
              time = psTimeFromJD(timeValue);
              break;
            case TIME_FORMAT_MJD:
              time = psTimeFromMJD(timeValue);
              break;
            default:
              psError(PS_ERR_UNKNOWN, true, "Unable to interpret time %s (%f)", pattern->name, timeValue);
              return NULL;
          }
          break;
      }
      case PS_TYPE_F64: {
          double timeValue = (double)concept->data.F64;
          switch (timeFormat.format) {
            case TIME_FORMAT_JD:
              time = psTimeFromJD(timeValue);
              break;
            case TIME_FORMAT_MJD:
              time = psTimeFromMJD(timeValue);
              break;
            default:
              psError(PS_ERR_UNKNOWN, true, "Unable to interpret time %s (%f)", pattern->name, timeValue);
              return NULL;
          }
          break;
      }
      default:
        psError(PS_ERR_UNKNOWN, true, "Unable to parse %s.\n", pattern->name);
        return NULL;
    }

    if (!time) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, false, "Unable to parse time for %s", pattern->name);
        return NULL;
    }

    // Set the time system appropriately
    switch (timeFormat.format) {
      case TIME_FORMAT_JD:
      case TIME_FORMAT_MJD:
        conceptSetTimesysForTime(pattern->name, fpa, chip, cell, PS_TIME_TAI);
        break;
      default:
        time->type = timeSys;
        break;
    }

    psMetadataItem *item = psMetadataItemAllocPtr(pattern->name, PS_DATA_TIME, pattern->comment, time);
    psFree(time);                       // Drop reference
    return item;
}

// Correct a position --- in case the user wants FORTRAN indexing (like the FITS standard...)
static int fortranCorr(const psMetadata *cameraFormat, // The camera format description
                       const char *name // Name of concept to check for FORTRAN indexing
    )
{
    bool mdok = false;                  // Result of MD lookup
    psMetadata *formats = psMetadataLookupMetadata(&mdok, cameraFormat, "FORMATS");
    if (mdok && formats) {
        psString format = psMetadataLookupStr(&mdok, formats, name);
        if (mdok && strlen(format) > 0 && strcasecmp(format, "FORTRAN") == 0) {
            return 1;
        }
    }
    return 0;
}

psMetadataItem *p_pmConceptParse_Positions(const psMetadataItem *concept,
                                           const psMetadataItem *pattern,
                                           pmConceptSource source,
                                           const psMetadata *cameraFormat,
                                           const pmFPA *fpa,
                                           const pmChip *chip,
                                           const pmCell *cell)
{
    assert(concept);
    assert(cameraFormat);

    int offset = 0;                     // Offset of component (0,0) corner from the parent (0,0) corner

    switch (concept->type) {
        TYPE_CASE(offset, concept, U8);
        TYPE_CASE(offset, concept, U16);
        TYPE_CASE(offset, concept, U32);
        TYPE_CASE(offset, concept, S8);
        TYPE_CASE(offset, concept, S16);
        TYPE_CASE(offset, concept, S32);
#if 0

      case PS_DATA_STRING: {
          // Interpret as a region specifier [x0:x1,y0:y1]
          int xParity = 0;
          int yParity = 0;
          psRegion region = psRegionAndParityFromString(&xParity, &yParity, concept->data.V);
          if (strstr(pattern->name, ".X0")) {
              offset = region.x0;
          } else if (strstr(pattern->name, ".Y0")) {
              offset = region.y0;
          } else if (strstr(pattern->name, ".X1")) {
              offset = region.x1;
          } else if (strstr(pattern->name, ".Y1")) {
              offset = region.y1;
          } else {
              psError(PS_ERR_UNKNOWN, true,
                      "Unable to interpret %s because unable to determine if concept is X or Y.\n",
                      pattern->name);
              return NULL;
          }
          break;
      }
#endif
      default:
        if (concept->type == PS_DATA_F32 && concept->data.F32 - (int)concept->data.F32 == 0) {
            offset = concept->data.F32;
        } else if (concept->type == PS_DATA_F64 && concept->data.F64 - (int)concept->data.F64 == 0) {
            offset = concept->data.F64;
        } else {
            psError(PS_ERR_UNKNOWN, true, "Concept %s is not of integer type, as expected.\n", pattern->name);
            return NULL;
        }
    }
    offset -= fortranCorr(cameraFormat, pattern->name);
    return psMetadataItemAllocS32(pattern->name, pattern->comment, offset);
}


psMetadataItem *p_pmConceptFormat_CELL_TRIMSEC(const psMetadataItem *concept,
                                               pmConceptSource source,
                                               const psMetadata *cameraFormat,
                                               const pmFPA *fpa,
                                               const pmChip *chip,
                                               const pmCell *cell)
{
    assert(concept);

    psRegion *trimsec = psMemIncrRefCounter(concept->data.V); // The trimsec region
    if (!trimsec) {
        return psMetadataItemAllocStr(concept->name, concept->comment, NULL);
    }

    // Correct trim section for binning if it's specified explicitly (i.e., immutably) in the CELLS.
    if (source == PM_CONCEPT_SOURCE_CELLS) {
        bool xStatus, yStatus;          // Status of MD lookups
        int xBin = psMetadataLookupS32(&xStatus, cell->concepts, "CELL.XBIN");
        int yBin = psMetadataLookupS32(&yStatus, cell->concepts, "CELL.YBIN");
        if (!xStatus || !yStatus || xBin == 0 || yBin == 0) {
            psWarning("Unable to find CELL.XBIN and CELL.YBIN to correct CELL.TRIMSEC.\n");
            psFree(trimsec);            // Drop reference
            return psMemIncrRefCounter((psPtr)concept); // Casting away "const" to increment
        }
        psRegion *newTrimsec = psRegionAlloc(trimsec->x0 * xBin, trimsec->x1 * xBin,
                                             trimsec->y0 * yBin, trimsec->y1 * yBin); // Adjusted for binning
        psFree(trimsec);
        trimsec = newTrimsec;
    }

    psString trimsecString = psRegionToString(*trimsec);
    psFree(trimsec);
    psMetadataItem *formatted = psMetadataItemAllocStr(concept->name, concept->comment,
                                                       trimsecString);
    psFree(trimsecString);
    return formatted;
}

psMetadataItem *p_pmConceptFormat_CELL_BIASSEC(const psMetadataItem *concept,
                                               pmConceptSource source,
                                               const psMetadata *cameraFormat,
                                               const pmFPA *fpa,
                                               const pmChip *chip,
                                               const pmCell *cell)
{
    // Return a metadata item containing a list of metadata items of region strings
    psList *biassecs = concept->data.V; // The biassecs region list
    psList *new = psListAlloc(NULL);    // New list containing metadatas
    if (biassecs) {
        psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, false); // Iterator
        psRegion *region = NULL;            // Region from iteration
        while ((region = psListGetAndIncrement(biassecsIter))) {
            // Correct bias section for binning if it's specified explicitly (i.e., immutably) in the CELLS.
            if (source == PM_CONCEPT_SOURCE_CELLS) {
                bool xStatus, yStatus;          // Status of MD lookups
                int xBin = psMetadataLookupS32(&xStatus, cell->concepts, "CELL.XBIN");
                int yBin = psMetadataLookupS32(&yStatus, cell->concepts, "CELL.YBIN");
                if (!xStatus || !yStatus || xBin == 0 || yBin == 0) {
                    psWarning("Unable to find CELL.XBIN and CELL.YBIN to correct CELL.BIASSEC.\n");
                } else {
                    psRegion *newTrimsec = psRegionAlloc(region->x0 * xBin, region->x1 * xBin,
                                                         region->y0 * yBin, region->y1 * yBin);
                    region = newTrimsec;
                }
            } else {
                psMemIncrRefCounter(region);
            }

            psString regionString = psRegionToString(*region); // The string region "[x0:x1,y0:y1]"
            psFree(region);
            psMetadataItem *item = psMetadataItemAllocStr(concept->name, concept->comment, regionString);
            psFree(regionString);           // Drop reference
            psListAdd(new, PS_LIST_TAIL, item);
            psFree(item);                   // Drop reference
        }
        psFree(biassecsIter);
    }

    psMetadataItem *formatted = psMetadataItemAllocPtr(concept->name, PS_DATA_LIST, concept->comment, new);
    psFree(new);                        // Drop reference
    return formatted;
}

// This function actually does both CELL.XBIN and CELL.YBIN if CELL.XBIN and CELL.YBIN are specified by the
// same header.
psMetadataItem *p_pmConceptFormat_CELL_XBIN(const psMetadataItem *concept,
                                            pmConceptSource source,
                                            const psMetadata *cameraFormat,
                                            const pmFPA *fpa,
                                            const pmChip *chip,
                                            const pmCell *cell)
{
    assert(concept);

    psMetadata *translation = psMetadataLookupMetadata(NULL, cameraFormat, "TRANSLATION");
    bool xBinOK = true, yBinOK = true;  // Status of MD lookups
    psString xKeyword = psMetadataLookupStr(&xBinOK, translation, "CELL.XBIN");
    psString yKeyword = psMetadataLookupStr(&yBinOK, translation, "CELL.YBIN");
    if (xBinOK && yBinOK && strlen(xKeyword) > 0 && strlen(yKeyword) > 0 &&
        strcasecmp(xKeyword, yKeyword) == 0) {
        psMetadataItem *yBinItem = psMetadataLookup(cell->concepts, "CELL.YBIN"); // Binning factor in y
        psString binString = NULL;
        psStringAppend(&binString, "%d %d", concept->data.S32, yBinItem->data.S32);
        psMetadataItem *binItem = psMetadataItemAllocStr(concept->name, concept->comment, binString);
        psFree(binString);
        return binItem;
    }

    // Otherwise, there's no formatting required
    return psMetadataItemCopy(concept);
}

// Only need to format if both if CELL.XBIN and CELL.YBIN are not specified by the same header.
psMetadataItem *p_pmConceptFormat_CELL_YBIN(const psMetadataItem *concept,
                                            pmConceptSource source,
                                            const psMetadata *cameraFormat,
                                            const pmFPA *fpa,
                                            const pmChip *chip,
                                            const pmCell *cell)
{
    assert(concept);

    psMetadata *translation = psMetadataLookupMetadata(NULL, cameraFormat, "TRANSLATION");
    bool xBinOK = true, yBinOK = true;  // Status of MD lookups
    psString xKeyword = psMetadataLookupStr(&xBinOK, translation, "CELL.XBIN");
    psString yKeyword = psMetadataLookupStr(&yBinOK, translation, "CELL.YBIN");
    if (xBinOK && yBinOK && strlen(xKeyword) > 0 && strlen(yKeyword) > 0 &&
        strcasecmp(xKeyword, yKeyword) == 0) {
        // Censor this --- it's already done (though no harm if it's done twice
        return NULL;
    }

    // No formatting required
    return psMetadataItemCopy(concept);
}


psMetadataItem *p_pmConceptFormat_TIMESYS(const psMetadataItem *concept,
                                          pmConceptSource source,
                                          const psMetadata *cameraFormat,
                                          const pmFPA *fpa,
                                          const pmChip *chip,
                                          const pmCell *cell)
{
    psString timeName = psStringCopy(concept->name); // Name of corresponding TIME concept
    psStringSubstitute(&timeName, "TIME", "TIMESYS");

    conceptTimeFormat timeFormat = conceptGetTimeFormat(timeName, cameraFormat); // Format for time
    psFree(timeName);

    psTimeType timesys = concept->data.S32; // Time system

    // JD and MJD are converted to TAI before writing
    switch (timeFormat.format) {
      case TIME_FORMAT_JD:
      case TIME_FORMAT_MJD:
        timesys = PS_TIME_TAI;
        break;
      default:
        break;
    }

    psString sys = NULL;            // String to store
    switch (timesys) {
      case PS_TIME_TAI:
        sys = psStringCopy("TAI");
        break;
      case PS_TIME_UTC:
        sys = psStringCopy("UTC");
        break;
      case PS_TIME_UT1:
        sys = psStringCopy("UT1");
        break;
      case PS_TIME_TT:
        sys = psStringCopy("TT");
        break;
      default:
        sys = psStringCopy("Unknown");
    }
    psMetadataItem *newItem = psMetadataItemAllocStr(concept->name, concept->comment, sys);
    psFree(sys);

    return newItem;
}

psMetadataItem *p_pmConceptFormat_TIME(const psMetadataItem *concept,
                                       pmConceptSource source,
                                       const psMetadata *cameraFormat,
                                       const pmFPA *fpa,
                                       const pmChip *chip,
                                       const pmCell *cell)
{
    psAssert (concept->type == PS_DATA_TIME, "programming error: concept not supplied with psTime pointer\n");
    psTime *time = concept->data.V;     // The time

    psTimeType timeSys = conceptGetTimesysForTime(concept->name, fpa, chip, cell); // Time system
    psTimeConvert(time, timeSys);

    conceptTimeFormat timeFormat = conceptGetTimeFormat(concept->name, cameraFormat); // Format for time

    if (timeFormat.separate) {
        // We're working with two separate headers --- construct a list with the date and time separately
        psString dateTimeString = psTimeToISO(time); // String representation
        psList *dateTime = psStringSplit(dateTimeString, "T", true);
        psFree(dateTimeString);
        psString dateString = psListGet(dateTime, PS_LIST_HEAD); // The date string
        psString timeString = psListGet(dateTime, PS_LIST_TAIL); // The time string

        // Need to format the strings....
        // XXX: Couldn't be bothered doing these right now
        if (timeFormat.pre2000) {
            psError(PS_ERR_UNKNOWN, true, "Don't you realise it's the twenty-first century?\n");
            return NULL;
        }

        switch (timeFormat.format) {
          case TIME_FORMAT_DDMMYYYY: {
              int day, month, year;
              psTrace ("psModules.concepts", 5, "ISO time has year first, convert to DD-MM-YYYY");
              sscanf (dateString, "%d-%d-%d", &year, &month, &day);
              sprintf (dateString, "%02d-%02d-%04d", day, month, year);
              // XXX fix this for str length
              break;
          }
          case TIME_FORMAT_MMDDYYYY: {
              int day, month, year;
              psTrace ("psModules.concepts", 5, "ISO time has year first, convert to MM-DD-YYYY");
              sscanf (dateString, "%d-%d-%d", &year, &month, &day);
              sprintf (dateString, "%02d-%02d-%04d", month, day, year);
              // XXX fix this for str length
              break;
          }
          default:
            break;
        }

        psMetadataItem *dateItem = psMetadataItemAllocStr(concept->name, "The date of observation",
                                                          dateString);
        psMetadataItem *timeItem = psMetadataItemAllocStr(concept->name, "The time of observation",
                                                          timeString);

        psListRemove(dateTime, PS_LIST_HEAD);
        psListRemove(dateTime, PS_LIST_HEAD);

        psListAdd(dateTime, PS_LIST_HEAD, dateItem);
        psListAdd(dateTime, PS_LIST_TAIL, timeItem);

        psMetadataItem *item = psMetadataItemAllocPtr(concept->name, PS_DATA_LIST,
                                                      concept->comment, dateTime);
        psFree(dateItem);
        psFree(timeItem);
        psFree(dateTime);
        return item;
    }

    switch (timeFormat.format) {
      case TIME_FORMAT_JD: {
          double jd = psTimeToMJD(time);
          return psMetadataItemAllocF64(concept->name, concept->comment, jd);
      }
      case TIME_FORMAT_MJD: {
          double mjd = psTimeToMJD(time);
          return psMetadataItemAllocF64(concept->name, concept->comment, mjd);
      }
      default: {
          // If we've gotten this far, it's straight ISO.
          psString dateTimeString = psTimeToISO(time); // String representation
          psMetadataItem *item = psMetadataItemAllocStr(concept->name, concept->comment, dateTimeString);
          psFree(dateTimeString);
          return item;
      }
    }

    return NULL;
}


psMetadataItem *p_pmConceptFormat_Positions(const psMetadataItem *concept,
                                            pmConceptSource source,
                                            const psMetadata *cameraFormat,
                                            const pmFPA *fpa,
                                            const pmChip *chip,
                                            const pmCell *cell)
{
    assert(concept);
    assert(cameraFormat);

    if (concept->type != PS_DATA_S32) {
        psError(PS_ERR_UNKNOWN, true, "Concept %s is not of type S32, as expected.\n", concept->name);
        return NULL;
    }

#if 0
    // If both the X0 and Y0 positions are specified by the same header keyword, write both together, as part
    // of the call for the X0 position.
    // This is a bit of a kludge --- we're going to write it out as [x0:0,y0:0].  This means that we will be
    // able to read it back in, but we've destroyed the x1 and y1 if it was present.
    // We *could* attempt to read the header, parse the region, and only update the ones that we're trying
    // to update.  Consider this an upgrade option later.
    // Alternatively, we could add X1 and Y1 concepts, and write the whole lot out together.
    // But until we care about X1 and Y1, it doesn't really matter --- if you want X1 and Y1, look at X0 and
    // Y0 and add NXAIS1 and NAXIS2, respectively....
    if (strstr(concept->name, ".X0")) {
        psString companion = psStringCopy(concept->name); // Companion entry: ".Y" where this one has ".X"
        psStringSubstitute(&companion, ".Y0", ".X0");

        // Look both up in the camera format config
        psMetadata *translation = psMetadataLookupMetadata(NULL, cameraFormat, "TRANSLATION");
        bool xFound = true, yFound = true;  // Status of MD lookups
        psString xKeyword = psMetadataLookupStr(&xFound, translation, concept->name);
        psString yKeyword = psMetadataLookupStr(&yFound, translation, companion);
        if (xFound && yFound && strlen(xKeyword) > 0 && strlen(yKeyword) > 0 &&
            strcasecmp(xKeyword, yKeyword) == 0) {
            psMetadataItem *yItem = psMetadataLookup(cell->concepts, companion); // Corresponding y value

            int x = concept->data.S32 + fortranCorr(cameraFormat, concept->name); // x value
            int y = yItem->data.S32 + fortranCorr(cameraFormat, companion); // y value

            psRegion region = psRegionSet(x, x, y, y);
            psString string = psRegionToString(region);
            psMetadataItem *binItem = psMetadataItemAllocStr(concept->name, concept->comment, string);
            psFree(string);
            psFree(companion);
            return binItem;
        }
        psFree(companion);
    } else if (strstr(concept->name, ".Y0")) {
        psString companion = psStringCopy(concept->name); // Companion entry: ".Y" where this one has ".X"
        psStringSubstitute(&companion, ".X0", ".Y0");

        // Look both up in the camera format config
        psMetadata *translation = psMetadataLookupMetadata(NULL, cameraFormat, "TRANSLATION");
        bool xFound = true, yFound = true;  // Status of MD lookups
        psString xKeyword = psMetadataLookupStr(&xFound, translation, concept->name);
        psString yKeyword = psMetadataLookupStr(&yFound, translation, companion);
        psFree(companion);
        if (xFound && yFound && strlen(xKeyword) > 0 && strlen(yKeyword) > 0 &&
            strcasecmp(xKeyword, yKeyword) == 0) {
            return NULL;                // We did it with the X; don't do anything.
        }
    }
#endif

    int offset = concept->data.S32;
    offset += fortranCorr(cameraFormat, concept->name);
    return psMetadataItemAllocS32(concept->name, concept->comment, offset);
}


psMetadataItem *p_pmConceptCopy_TIMESYS(const psMetadataItem *target,
                                        const psMetadataItem *source,
                                        const psMetadata *cameraFormat,
                                        const pmFPA *fpa,
                                        const pmChip *chip,
                                        const pmCell *cell)
{
    if (!target || target->data.S32 == -1) {
        // Replace
        return psMetadataItemCopy(source);
    }
    // Keep what we've got --- it's been mandated by use of the DEFAULTS
    return psMemIncrRefCounter((psMetadataItem*)target);
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAFlags.h"
#include "pmConceptsRead.h"
#include "pmFPAConstruct.h"
#include "pmFPAUtils.h"
#include "pmHDUUtils.h"

#define TABLE_OF_CONTENTS "CONTENTS"    // Name for camera format metadata containing the contents
#define CHIP_TYPES "CHIPS"              // Name for camera format metadata containing the chip types
#define CELL_TYPES "CELLS"              // Name for camera format metadata containing the cell types


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Read data for a particular cell from the camera format description
static psMetadata *getCellData(const psMetadata *format, // The camera format description
                               const char *cellName // The name of the cell
                              )
{
    assert(format);
    assert(cellName && strlen(cellName) > 0);

    bool status = true;                 // Result of MD lookup
    psMetadata *cells = psMetadataLookupMetadata(&status, format, CELL_TYPES); // The CELLS
    if (!status || !cells) {
        psError(PS_ERR_IO, true, "Unable to find %s in camera format.\n", CELL_TYPES);
        return NULL;
    }

    psMetadata *cellData = psMetadataLookupMetadata(&status, cells, cellName); // The data for the particular cell
    if (!status || !cellData) {
        psWarning("Unable to find specs for cell %s: ignored\n", cellName);
    }

    return cellData;
}

// Parse a list of first:second:third pairs in a string
// EAM : this function takes an input 'string' and attempts to parse out
// groups of the form FIRST:SECOND:THIRD into the arrays (first, second, third).

// The input string may have multiple entries of this form separated by spaces, commas, or
// semi-colons.  The number of arrays which are supplied must match the string format or
// an error will be raised.

// e.g., the string could be CHIP:CELL:TYPE in which case all three arrays must exist
// or, the string could be CHIP:CELL only, in which case the array 'third' must be NULL
 
static int parseContent(psArray **first, // Array of the first values
                         psArray **second, // Array of the second values
                         psArray **third, // Array of the third values
                         const char *string // The string to parse
                        )
{
    assert(string && strlen(string) > 0);
    // Must populate 'first', 'second', 'third' in order.
    assert(!second || first);
    assert(!third || second);

    int numArrays = third ? 3 : (second ? 2 : 1); // Number of arrays

    psList *values = psStringSplit(string, " ,;", true); // List of the parts
    int num = values->n; // number of parsed content elements

    // extend the arrays if they exist, create new ones if they don't
    if (first && !*first) {
        *first = psArrayAllocEmpty(values->n);
    }
    if (second && !*second) {
        *second = psArrayAllocEmpty(values->n);
    }
    if (third && !*third) {
        *third = psArrayAllocEmpty(values->n);
    }

    psListIterator *valuesIter = psListIteratorAlloc(values, PS_LIST_HEAD, false); // Iterator for values
    psString value = NULL;               // "first:second:third" string
    while ((value = psListGetAndIncrement(valuesIter))) {
        psArray *fst = psStringSplitArray(value, ":", true); // First, second, third
	if (fst->n != numArrays) {
	  psError(PS_ERR_BAD_PARAMETER_VALUE, false, "string %s does not match expected format (%ld colon-separated items supplied, %d expected)", value, fst->n, numArrays);
	  return 0;
	}
        switch (numArrays) {
          case 3:
            psArrayAdd(*third, 8, fst->data[2]);
          case 2:
            psArrayAdd(*second, 8, fst->data[1]);
          case 1:
            psArrayAdd(*first, 8, fst->data[0]);
            break;
        default:
          psAbort("Should never get here.");
        }
        psFree(fst);
    }
    psFree(valuesIter);
    psFree(values);

    return num;
}

// Add an HDU to the FPA
static bool addHDUtoFPA(pmFPA *fpa,     // FPA to which to add
                        pmHDU *hdu      // HDU to be added
                       )
{
    assert(fpa);
    assert(hdu);

    // XXXX here is the issue : we need to avoid raising an error here
    if (fpa->hdu) {
        // Something's already here
        if (fpa->hdu != hdu) {
# define TEST1 1
# if (TEST1)
	    psError(PS_ERR_IO, true, "Unable to add HDU since FPA already has one.\n");
# else
	    psWarning ("Unable to add HDU since FPA already has one.\n");
# endif
        }
# if (TEST1)
        return false;
# else
        return true;
# endif
    }
    fpa->hdu = psMemIncrRefCounter(hdu);
    pmFPASetFileStatus(fpa, true);

    return true;
}

// Add an HDU to the chip
static bool addHDUtoChip(pmChip *chip,  // Chip to which to add
                         pmHDU *hdu     // HDU to be added
                        )
{
    assert(chip);
    assert(hdu);

    if (chip->hdu) {
        // Something's already here
        if (chip->hdu != hdu) {
            psError(PS_ERR_IO, true, "Unable to add HDU since chip already has one.\n");
        }
        return false;
    }
    chip->hdu = psMemIncrRefCounter(hdu);
    pmChipSetFileStatus(chip, true);

    return true;
}

// Add an HDU to the cell
static bool addHDUtoCell(pmCell *cell,  // Cell to which to add
                         pmHDU *hdu     // HDU to be added
                        )
{
    assert(cell);
    assert(hdu);

    if (cell->hdu) {
        // Something's already here
        if (cell->hdu != hdu) {
            psError(PS_ERR_IO, true, "Unable to add HDU since cell already has one.\n");
        }
        return false;
    }
    cell->hdu = psMemIncrRefCounter(hdu);
    pmCellSetFileStatus(cell, true);

    return true;
}


// Looks up the particular content, based on the chip and cell
static const char *getContent(const psMetadata *fileInfo, // The FILE from the camera format configuration
                              const psMetadata *header, // The FITS header
                              const psMetadata *contents // The CONTENTS from the camera format configuration
                             )
{
    assert(fileInfo);
    assert(contents);
    assert(header);

    const char *contentHeader = psMetadataLookupStr(NULL, fileInfo, "CONTENT"); // Keyword to get contents
    if (!contentHeader || strlen(contentHeader) == 0) {
        psError(PS_ERR_UNEXPECTED_NULL, false,
                "Unable to find CONTENT in FILE within camera format configuration.\n");
        return NULL;
    }

    psMetadataItem *contentKey = psMetadataLookup(header, contentHeader); // Key to CONTENTS menu
    if (!contentKey) {
        psError(PS_ERR_UNEXPECTED_NULL, false,
                "Unable to find %s in header to determine file content.", contentHeader);
        return NULL;
    }

    psString contentKeyStr = psMetadataItemParseString(contentKey); // Key, as a string

    psTrace("psModules.camera", 5, "Looking up %s in the CONTENTS.\n", contentKeyStr);
    const char *content = psMetadataLookupStr(NULL, contents, contentKeyStr);
    if (!content || strlen(content) == 0) {
        psError(PS_ERR_IO, false, "Unable to find %s in the CONTENTS.\n", contentKeyStr);
        return NULL;
    }

    psFree(contentKeyStr);

    return content;
}


// Given a list of contents, put the HDU in the correct place and plug in the cell configuration information
static bool processContents(pmFPA *fpa,  // The FPA
                            pmChip *chip, // The chip
                            pmHDU *hdu,  // The HDU to be added
                            pmFPALevel level, // The level at which to add the HDU
                            psArray *chipNames, // The chip names
                            psArray *cellNames, // The cell names
                            psArray *cellTypes, // The cell types
                            const psMetadata *format // Camera format configuration
                            )
{
    assert(fpa);
    assert(cellTypes);
    long num = cellTypes->n;            // Number of entries to add
    assert(chip || (chipNames && chipNames->n == num));
    assert(cellNames && cellNames->n == num);
    assert(format);

    if (hdu && level == PM_FPA_LEVEL_FPA) {
        if (!addHDUtoFPA(fpa, hdu)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add HDU to FPA");
            return false;
        }
    }
    // Load fpa-related concepts
    if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_DEFAULTS, false, NULL)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read concepts from camera and defaults for fpa\n");
        return false;
    }

    for (int i = 0; i < num; i++) {
        psString cellType = cellTypes->data[i]; // The type of the cell

        // Find the chip
        pmChip *newChip;                // Chip of interest
        if (chip) {
            newChip = chip;
        } else {
            psString chipName = chipNames->data[i]; // The name of the chip
            int chipNum = pmFPAFindChip(fpa, chipName); // The chip we're looking for
            if (chipNum == -1) {
                psError(PS_ERR_LOCATION_INVALID, false,
                        "Unable to find chip %s in fpa --- ignored.\n", chipName);
                return false;
            }
            newChip = fpa->chips->data[chipNum];
        }

        // Put in the HDU
        if (hdu && level == PM_FPA_LEVEL_CHIP) {
            addHDUtoChip(newChip, hdu);
        }
        // Load chip-related concepts
        if (!pmConceptsReadChip(newChip, PM_CONCEPT_SOURCE_DEFAULTS, false, false, NULL)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read concepts from camera and defaults for chip\n");
            return false;
        }

        // Find the cell
        pmCell *newCell;                // Cell of interest
        psString cellName = cellNames->data[i]; // The name of the cell
        int cellNum = pmChipFindCell(newChip, cellName); // The cell we're looking for
        if (cellNum == -1) {
            psError(PS_ERR_LOCATION_INVALID, false, "Unable to find cell %s in chip --- ignored.\n",
                    cellName);
            return false;
        }
        newCell = newChip->cells->data[cellNum];

        psMetadata *cellData = getCellData(format, cellType); // Data for this cell

        if (hdu && level == PM_FPA_LEVEL_CELL) {
            addHDUtoCell(newCell, hdu);
        }

        // Put in the cell data
        if (newCell->config) {
            psWarning("Overwriting cell data in chip\n");
            psFree(newCell->config); // Make way!
        }
        newCell->config = psMemIncrRefCounter(cellData);
        if (!pmConceptsReadCell(newCell, PM_CONCEPT_SOURCE_CELLS | PM_CONCEPT_SOURCE_DEFAULTS,
                                false, NULL)) {
            psError(PS_ERR_UNKNOWN, false,
                    "Unable to read concepts from camera and defaults for cell type %s", cellType);
            return false;
        }
    }

    return true;
}

#if 0
// Return the level at which EXTENSIONS go, from the FILE metadata within the camera format
static pmFPALevel hduLevel(const psMetadata *format // The camera format configuration
                          )
{
    assert(format);

    bool mdok = true;                   // Status of MD lookup
    psMetadata *file = psMetadataLookupMetadata(&mdok, format, "FILE"); // File information
    if (!mdok || !file) {
        psError(PS_ERR_IO, true, "Unable to find FILE information in camera format configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }
    const char *extType = psMetadataLookupStr(&mdok, file, "EXTENSIONS");
    if (!mdok || !extType || strlen(extType) == 0) {
        psError(PS_ERR_IO, true, "Unable to find EXTENSIONS in the FILE information in the camera format"
                " configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }

    // Where do we stick in the HDUs?
    pmFPALevel level = PM_FPA_LEVEL_NONE; // Level for HDU insertion
    if (strcasecmp(extType, "CHIP") == 0) {
        level = PM_FPA_LEVEL_CHIP;
    } else if (strcasecmp(extType, "CELL") == 0) {
        level = PM_FPA_LEVEL_CELL;
    } else if (strcasecmp(extType, "NONE") != 0) {
        psError(PS_ERR_IO, true, "EXTENSIONS is not CHIP or CELL or NONE.\n");
    }

    return level;
}
#endif

// Find the chip of interest within the FPA
static bool whichChip(int *chipNum, // Chip number, modified
                      psString *chipType, // Type of chip, modified
                      const pmFPA *fpa, // FPA holding chip of interest
                      const char *content // Content consisting of chipName:chipType
                      )
{
    assert(chipType);
    assert(fpa);
    assert(content);

    psArray *chipNames = NULL;
    psArray *chipTypes = NULL;
    if (parseContent(&chipNames, &chipTypes, NULL, content) != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Unable to parse chipName:chipType in %s in camera format",
                TABLE_OF_CONTENTS);
        return false;
    }

    psString chipName = psMemIncrRefCounter(chipNames->data[0]); // Name of chip
    *chipType = psMemIncrRefCounter(chipTypes->data[0]); // Type of chip
    psFree(chipNames);
    psFree(chipTypes);

    psTrace("psModules.camera", 5, "This is chip %s\n", chipName);

    // Get the chip
    *chipNum = pmFPAFindChip(fpa, chipName); // Chip number
    if (*chipNum == -1) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find chip %s in FPA.\n", chipName);
        psFree(chipName);
        return false;
    }
    psFree(chipName);

    return true;
}


// Process a chip, using the cellName:cellType pair
static bool processChip(const psMetadata *format, // Camera format
                        const psMetadataItem *chipContents, // Contents of chip, cellName:cellType pairs (either in a string or a metadata)
                        pmFPA *fpa, // FPA of interest
                        pmChip *chip, // Chip of interest
                        pmFPALevel level, // Level for HDU to go
                        pmHDU *hdu      // HDU to add
    )
{
    assert(format);
    assert(chipContents);
    assert(fpa);

    psMetadata *chips = psMetadataLookupMetadata(NULL, format, CHIP_TYPES); // The chip types
    if (!chips) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", CHIP_TYPES);
        return false;
    }

    psArray *cellNames = NULL;      // Cell names
    psArray *cellTypes = NULL;      // Cell types

    int nParsed = 0;

    switch (chipContents->type) {
      case PS_DATA_STRING: {
          nParsed = parseContent(&cellNames, &cellTypes, NULL, chipContents->data.str);
          break;
      }
      case PS_DATA_METADATA: {
          psMetadataIterator *iter = psMetadataIteratorAlloc(chipContents->data.md, PS_LIST_HEAD, NULL); // Iterator
          psMetadataItem *item;           // Item from iteration
          while ((item = psMetadataGetAndIncrement(iter))) {
              if (item->type != PS_DATA_STRING) {
                  psWarning ("Item %s in camera format chip table is not of type STR.", item->name);
                  continue;
              }
              nParsed += parseContent(&cellNames, &cellTypes, NULL, item->data.str);
          }
          psFree (iter);
          break;
      }
      default:
        psWarning ("Item %s in camera format chip table is not of type STR.", chipContents->name);
        break;
    }

    if (nParsed == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Unable to parse chip contents (within %s in camera format) as cellName:cellType",
                CHIP_TYPES);
        psFree(cellNames);
        psFree(cellTypes);
        return false;
    }

    if (!processContents(fpa, chip, hdu, level, NULL, cellNames, cellTypes, format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to set contents for chip from camera format.");
        psFree(cellNames);
        psFree(cellTypes);
        return false;
    }

    psFree(cellNames);
    psFree(cellTypes);

    return true;
}

// Given a chip, find the corresponding type by searching through the contents, looking for a match to its
// name
psString findChipType(const pmChip *chip, // Chip of interest
                      psMetadata *contents // Contents, from camera format
                      )
{
    assert(chip);
    assert(contents);

    const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip
    assert(chipName);

    psString chipType = NULL;           // Type of chip
    psMetadataIterator *iter = psMetadataIteratorAlloc(contents, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;           // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_STRING) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Item %s within %s in camera format is not of type STR.", item->name, TABLE_OF_CONTENTS);
            psFree(iter);
            psFree(chipType);
            return NULL;
        }

        psArray *chipNames = NULL;  // Chip names
        psArray *chipTypes = NULL;  // Chip types
        if (parseContent(&chipNames, &chipTypes, NULL, item->data.str) != 1) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                    "Unable to parse contents (within %s in camera format) as chipName:chipType",
                    TABLE_OF_CONTENTS);
            psFree(chipNames);
            psFree(chipTypes);
            psFree(iter);
            psFree(chipType);
            return NULL;
        }

        if (strcmp(chipName, chipNames->data[0]) == 0) {
            if (chipType) {
                if (strcmp(chipType, chipTypes->data[0]) != 0) {
                    psError(PS_ERR_UNKNOWN, true,
                            "Multiple instances of chip %s in contents, with differing chipType "
                            "(%s vs %s)", chipName, chipType, (char*)chipTypes->data[0]);
                    psFree(chipNames);
                    psFree(chipTypes);
                    psFree(iter);
                    psFree(chipType);
                    return NULL;
                }
            } else {
                chipType = psMemIncrRefCounter(chipTypes->data[0]);
            }
        }
        psFree(chipNames);
        psFree(chipTypes);
    }
    psFree(iter);

    if (!chipType) {
        psError(PS_ERR_UNKNOWN, true, "Unable to identify chip type for chip %s", chipName);
        return NULL;
    }

    return chipType;
}

// PHU=FPA and EXTENSIONS=CHIP:
// TABLE_OF_CONTENTS(METADATA) has a list of extensions, each with a chipName:chipType.
// CHIP_TYPES(METADATA) has a list of chip types, each with cellName:cellType
static bool addSource_FPA_CHIP(pmFPA *fpa, // FPA to which to add
                               const psMetadata *format // The camera format
                               )
{
    assert(fpa);
    assert(format);

    psMetadata *contents = psMetadataLookupMetadata(NULL, format, TABLE_OF_CONTENTS); // The contents
    if (!contents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", TABLE_OF_CONTENTS);
        return false;
    }

    psMetadata *chips = psMetadataLookupMetadata(NULL, format, CHIP_TYPES); // The chip types
    if (!chips) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", CHIP_TYPES);
        return false;
    }

    // Iterate over all extensions
    psMetadataIterator *contentsIter = psMetadataIteratorAlloc(contents, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(contentsIter))) {
        if (item->type != PS_DATA_STRING) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    "Type for %s (%x) in %s METADATA in camera format is not STR",
                    item->name, item->type, TABLE_OF_CONTENTS);
            psFree(contentsIter);
            return false;
        }

        const char *extname = item->name; // Extension name
        pmHDU *hdu = pmHDUAlloc(extname); // HDU for this extension
        // Casting to avoid "warning: passing arg 1 of `p_psMemIncrRefCounter' discards qualifiers from
        // pointer target type"
        hdu->format = psMemIncrRefCounter((const psPtr)format);

        // What's in the extension?  It's specified by chipName:chipType
        // Assume that an extension contains only a single chip, instead of multiple chips
        psString chipType = NULL;       // Type of chip
        int chipNum = -1;               // Chip number
        if (!whichChip(&chipNum, &chipType, fpa, item->data.str)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine chip from contents");
            return false;
        }
        pmChip *chip = fpa->chips->data[chipNum]; // Chip of interest

        const psMetadataItem *chipContents = psMetadataLookup(chips, chipType); // Contents of chip
        psFree(chipType);
        if (!chipContents) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find chip type %s in %s.",
                    chipType, CHIP_TYPES);
            psFree(hdu);
            psFree(contentsIter);
            return false;
        }

        if (!processChip(format, chipContents, fpa, chip, PM_FPA_LEVEL_CHIP, hdu)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to process chip %d\n", chipNum);
            psFree(hdu);
            psFree(contentsIter);
            return false;
        }

        psFree(hdu);                    // Drop reference
    }
    psFree(contentsIter);

    return true;
}

// PHU=FPA and EXTENSIONS=CELL:
// TABLE_OF_CONTENTS(METADATA) has a list of extensions, each with a chipName:cellName:cellType.
static bool addSource_FPA_CELL(pmFPA *fpa, // FPA to which to add
                               const psMetadata *format // The camera format
                               )
{
    assert(fpa);
    assert(format);

    psMetadata *contents = psMetadataLookupMetadata(NULL, format, TABLE_OF_CONTENTS); // The contents
    if (!contents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", TABLE_OF_CONTENTS);
        return false;
    }

    // Iterate over all extensions
    psMetadataIterator *contentsIter = psMetadataIteratorAlloc(contents, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(contentsIter))) {
        if (item->type != PS_DATA_STRING) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    "Type for %s (%x) in %s METADATA in camera format is not STR",
                    item->name, item->type, TABLE_OF_CONTENTS);
            psFree(contentsIter);
            return false;
        }

        const char *extname = item->name; // Extension name
        pmHDU *hdu = pmHDUAlloc(extname); // HDU for this extension
        // Casting to avoid "warning: passing arg 1 of `p_psMemIncrRefCounter' discards qualifiers from
        // pointer target type"
        hdu->format = psMemIncrRefCounter((const psPtr)format);

        // What's in the extension?  It's specified by (possibly multiple) chipName:cellName:cellType

        psArray *chipNames = NULL;      // Chip names
        psArray *cellNames = NULL;      // Cell names
        psArray *cellTypes = NULL;      // Cell types
        if (parseContent(&chipNames, &cellNames, &cellTypes, item->data.str) == 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                    "Unable to parse extension contents (within %s->%s in camera format) as "
                    "chipName:cellName:cellType", TABLE_OF_CONTENTS, extname);
            psFree(chipNames);
            psFree(cellNames);
            psFree(cellTypes);
            psFree(hdu);
            psFree(contentsIter);
            return false;
        }

        if (!processContents(fpa, NULL, hdu, PM_FPA_LEVEL_CELL, chipNames, cellNames, cellTypes,
                             format)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to set contents from camera format.");
            psFree(chipNames);
            psFree(cellNames);
            psFree(cellTypes);
            psFree(hdu);
            psFree(contentsIter);
            return false;
        }

        psFree(chipNames);
        psFree(cellNames);
        psFree(cellTypes);

        psFree(hdu);                    // Drop reference
    }
    psFree(contentsIter);

    return true;
}

// PHU=FPA and EXTENSIONS=NONE:
// TABLE_OF_CONTENTS(STR) has a list of chipName:cellName:cellType.
static bool addSource_FPA_NONE(pmFPA *fpa, // FPA to which to add
                               const psMetadata *format // The camera format
                               )
{
    assert(fpa);
    assert(format);

    psString contents = psMetadataLookupStr(NULL, format, TABLE_OF_CONTENTS); // The contents
    if (!contents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", TABLE_OF_CONTENTS);
        return false;
    }

    // What's in the file?  It's specified by (possibly multiple) chipName:cellName:cellType

    psArray *chipNames = NULL;          // Chip names
    psArray *cellNames = NULL;          // Cell names
    psArray *cellTypes = NULL;          // Cell types
    if (parseContent(&chipNames, &cellNames, &cellTypes, contents) == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Unable to parse contents (within %s in camera format) as chipName:cellName:cellType",
                TABLE_OF_CONTENTS);
        psFree(chipNames);
        psFree(cellNames);
        psFree(cellTypes);
        return false;
    }

    if (!processContents(fpa, NULL, NULL, PM_FPA_LEVEL_NONE, chipNames, cellNames, cellTypes,
                         format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to set contents from camera format.");
        psFree(chipNames);
        psFree(cellNames);
        psFree(cellTypes);
        return false;
    }

    psFree(chipNames);
    psFree(cellNames);
    psFree(cellTypes);

    return true;
}


// PHU=CHIP and EXTENSIONS=CELL:
// TABLE_OF_CONTENTS(METADATA) has a menu of contents, each with a chipName:chipType.
// CHIP_TYPES(METADATA) has a list of chip types(METADATA), each with extension(STR) with cellName:cellType
static bool addSource_CHIP_CELL(pmFPAview *view, // View for PHU, modified
                                pmFPA *fpa, // FPA to which to add
                                pmChip *chip, // Known chip to which to add, or NULL
                                const psMetadata *format, // The camera format
                                pmHDU *phdu, // The Primary HDU
                                bool install // Install the HDUs?
                                )
{
    assert(view);
    assert(fpa);
    assert(format);
    assert(phdu);

    psMetadata *contents = psMetadataLookupMetadata(NULL, format, TABLE_OF_CONTENTS); // The contents
    if (!contents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", TABLE_OF_CONTENTS);
        return false;
    }

    psMetadata *chips = psMetadataLookupMetadata(NULL, format, CHIP_TYPES); // The chip types
    if (!chips) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", CHIP_TYPES);
        return false;
    }

    psMetadata *fileInfo = psMetadataLookupMetadata(NULL, format, "FILE"); // The file information
    if (!fileInfo) {
        psError(PS_ERR_IO, false, "Unable to find FILE in the camera format configuration.\n");
        return false;
    }


    psString chipType = NULL;           // Type of chip
    if (chip) {
        // We're given the chip (adding source from view)
        // Need to identify the chip type, which we will do by traversing the contents
        chipType = findChipType(chip, contents);
    } else {
        // We're given a header, from which to identify what chip we've got, and its type
        const char *content = getContent(fileInfo, phdu->header, contents); // The contents of this chip
        if (!content) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine content of file.");
            return false;
        }

        int chipNum = -1;               // Chip number
        if (!whichChip(&chipNum, &chipType, fpa, content)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine chip from contents");
            return false;
        }
        chip = fpa->chips->data[chipNum]; // Chip of interest
        view->chip = chipNum;
    }

    if (!install) {
        // Everything below is about installing the HDUs
        psFree(chipType);
        return true;
    }

    if (!addHDUtoChip(chip, phdu)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add HDU to chip\n");
        psFree(chipType);
        return false;
    }

    psMetadata *chipContents = psMetadataLookupMetadata(NULL, chips, chipType); // Contents of chip
    psFree(chipType);
    if (!chipContents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find chip type %s in %s.",
                chipType, CHIP_TYPES);
        return false;
    }

    psMetadataIterator *contentsIter = psMetadataIteratorAlloc(chipContents, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *contentItem;        // Content, from iteration
    while ((contentItem = psMetadataGetAndIncrement(contentsIter))) {
        pmHDU *hdu = pmHDUAlloc(contentItem->name); // HDU for this extension
        // Casting to avoid "warning: passing arg 1 of `p_psMemIncrRefCounter' discards qualifiers from
        // pointer target type"
        hdu->format = psMemIncrRefCounter((const psPtr)format);

        if (!processChip(format, contentItem, fpa, chip, PM_FPA_LEVEL_CELL, hdu)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to process chip\n");
            psFree(hdu);
            psFree(contentsIter);
            return false;
        }

        psFree(hdu);                    // Drop reference
    }
    psFree(contentsIter);

    if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_PHU, true, true, NULL)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read concepts for chip.");
        return false;
    }

    return true;
}

// PHU=CHIP and EXTENSIONS=NONE:
// TABLE_OF_CONTENTS(METADATA) has a menu of contents, each with a chipName:chipType.
// CHIP_TYPES(METADATA) has a list of chip types, each with cellName:cellType
static bool addSource_CHIP_NONE(pmFPAview *view, // View for PHU, modified
                                pmFPA *fpa, // FPA to which to add
                                pmChip *chip, // Known chip to which to add, or NULL
                                const psMetadata *format, // The camera format
                                pmHDU *phdu, // Primary HDU
                                bool install // Install the HDUs?
                                )
{
    assert(fpa);
    assert(format);
    assert(phdu);

    psMetadata *contents = psMetadataLookupMetadata(NULL, format, TABLE_OF_CONTENTS); // The contents
    if (!contents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", TABLE_OF_CONTENTS);
        return false;
    }

    psMetadata *chips = psMetadataLookupMetadata(NULL, format, CHIP_TYPES); // The chip types
    if (!chips) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", CHIP_TYPES);
        return false;
    }

    psMetadata *fileInfo = psMetadataLookupMetadata(NULL, format, "FILE"); // The file information
    if (!fileInfo) {
        psError(PS_ERR_IO, false, "Unable to find FILE in the camera format configuration.\n");
        return false;
    }

    psString chipType = NULL;           // Type of chip
    if (chip) {
        // We're given the chip (adding source from view)
        // Need to identify the chip type, which we will do by traversing the contents
        chipType = findChipType(chip, contents);
    } else {
        const char *content = getContent(fileInfo, phdu->header, contents); // The chip type
        if (!content) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find CONTENT entry in header");
            return false;
        }

        int chipNum = -1;               // Chip number
        if (!whichChip(&chipNum, &chipType, fpa, content)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine chip from contents");
            return false;
        }
        chip = fpa->chips->data[chipNum]; // Chip of interest
        view->chip = chipNum;
    }

    if (!install) {
        // Everything below is about installing the HDU
        psFree(chipType);
        return true;
    }

    if (!addHDUtoChip(chip, phdu)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add HDU to chip\n");
        psFree(chipType);
        return false;
    }

    // What's in the chip?
    psMetadataItem *chipContents = psMetadataLookup(chips, chipType); // Contents of the chip
    if (!chipContents) {
        psError(PS_ERR_UNEXPECTED_NULL, false,
                "Unable to find chip type %s in %s of camera format", chipType, CHIP_TYPES);
        return false;
    }
    psFree(chipType);

    if (!processChip(format, chipContents, fpa, chip, PM_FPA_LEVEL_NONE, NULL)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to process chip\n");
        return false;
    }

    if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_PHU, true, true, NULL)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read concepts for chip.");
        return false;
    }

    return true;
}

// PHU=CELL and EXTENSIONS=NONE:
// TABLE_OF_CONTENTS(METADATA) has a menu of contents, each with a chipName:cellName:cellType
static bool addSource_CELL_NONE(pmFPAview *view, // View for PHU, modified
                                pmFPA *fpa, // FPA to which to add
                                pmCell *cell, // Known cell to which to add, or NULL
                                const psMetadata *format, // The camera format
                                pmHDU *phdu, // The Primary HDU
                                bool install // Install the HDUs?
                                )
{
    assert(fpa);
    assert(format);
    assert(phdu);

    psMetadata *contents = psMetadataLookupMetadata(NULL, format, TABLE_OF_CONTENTS); // The contents
    if (!contents) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find %s in camera format.", TABLE_OF_CONTENTS);
        return false;
    }

    psMetadata *fileInfo = psMetadataLookupMetadata(NULL, format, "FILE"); // The file information
    if (!fileInfo) {
        psError(PS_ERR_IO, false, "Unable to find FILE in the camera format configuration.\n");
        return false;
    }

    psArray *chipNames = NULL;          // Chip names
    psArray *cellNames = NULL;          // Cell names
    psArray *cellTypes = NULL;          // Cell types
    pmChip *chip = NULL;                // Chip of interest
    if (cell) {
        // We're given the chip and cell (adding source from view)
        // Need to identify the cell type, which we will do by traversing the contents

        chip = cell->parent;            // The chip of interest
        psString cellType = NULL;       // Type of cell

        // The below is very similar to findChipType(), but with modifications for finding the cellType
        const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip
        assert(chipName);
        const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
        assert(cellName);

        psMetadataIterator *iter = psMetadataIteratorAlloc(contents, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *item;           // Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            if (item->type != PS_DATA_STRING) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        "Item %s within %s in camera format is not of type STR.",
                        item->name, TABLE_OF_CONTENTS);
                psFree(chipNames);
                psFree(cellNames);
                psFree(cellTypes);
                psFree(iter);
                psFree(cellType);
                return false;
            }

            psArray *testChipNames = NULL; // Chip names
            psArray *testCellNames = NULL; // Cell names
            psArray *testCellTypes = NULL; // Cell types
            if (parseContent(&testChipNames, &testCellTypes, &testCellTypes, item->data.str) != 1) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                        "Unable to parse contents (within %s in camera format) as chipName:cellName:cellType",
                        TABLE_OF_CONTENTS);
                psFree(chipNames);
                psFree(cellNames);
                psFree(cellTypes);
                psFree(testChipNames);
                psFree(testCellNames);
                psFree(testCellTypes);
                psFree(iter);
                psFree(cellType);
                return false;
            }

            if (strcmp(chipName, chipNames->data[0]) == 0 && strcmp(cellName, cellNames->data[0]) == 0) {
                if (cellType) {
                    if (strcmp(cellType, cellTypes->data[0]) != 0) {
                        psError(PS_ERR_UNKNOWN, true,
                                "Multiple instances of chip %s cell %s in contents, with differing cellType "
                                "(%s vs %s)", chipName, cellName, cellType, (char*)cellTypes->data[0]);
                        psFree(chipNames);
                        psFree(cellNames);
                        psFree(cellTypes);
                        psFree(testChipNames);
                        psFree(testCellNames);
                        psFree(testCellTypes);
                        psFree(iter);
                        psFree(cellType);
                        return false;
                    }
                } else {
                    cellType = psMemIncrRefCounter(cellTypes->data[0]);
                    chipNames = psMemIncrRefCounter(testChipNames);
                    cellNames = psMemIncrRefCounter(testCellNames);
                    cellTypes = psMemIncrRefCounter(testCellTypes);
                }
            }
            psFree(testChipNames);
            psFree(testCellNames);
            psFree(testCellTypes);
        }
        psFree(iter);

        if (!cellType) {
            psError(PS_ERR_UNKNOWN, true, "Unable to identify cell type for chip %s cell %s",
                    chipName, cellName);
            psFree(chipNames);
            psFree(cellNames);
            psFree(cellTypes);
            return false;
        }

        // We don't really care about the cell type here --- it's taken care of by processContents
        psFree(cellType);

    } else {
        const char *content = getContent(fileInfo, phdu->header, contents); // Content of cell

        if (parseContent(&chipNames, &cellNames, &cellTypes, content) != 1) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                    "Unable to parse cell contents (%s) as cellName:cellType", content);
            psFree(chipNames);
            psFree(cellNames);
            psFree(cellTypes);
            return false;
        }

        int chipNum = pmFPAFindChip(fpa, chipNames->data[0]); // Chip number
        if (chipNum == -1) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find chip %s referred to in contents",
                    (char*)chipNames->data[0]);
            psFree(chipNames);
            psFree(cellNames);
            psFree(cellTypes);
            return false;
        }
        chip = fpa->chips->data[chipNum];

        int cellNum = pmChipFindCell(chip, cellNames->data[0]); // Cell number
        if (cellNum == -1) {
            psError(PS_ERR_UNKNOWN, false, "Unable to find cell %s referred to in contents",
                    (char*)cellNames->data[0]);
            psFree(chipNames);
            psFree(cellNames);
            psFree(cellTypes);
            return false;
        }
        cell = chip->cells->data[cellNum];

        view->chip = chipNum;
        view->cell = cellNum;

        psFree(chipNames);
        psFree(cellNames);
        psFree(cellTypes);
    }

    if (!install) {
        // Everything below is about installing the HDU
        psFree(chipNames);
        psFree(cellNames);
        psFree(cellTypes);
        return true;
    }

    if (!processContents(fpa, NULL, phdu, PM_FPA_LEVEL_NONE, chipNames, cellNames, cellTypes, format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to set contents for cell from camera format.");
        psFree(chipNames);
        psFree(cellNames);
        psFree(cellTypes);
        return false;
    }

    psFree(chipNames);
    psFree(cellNames);
    psFree(cellTypes);

    if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_PHU, true, NULL)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read concepts for cell.");
        return false;
    }

    return true;
}


// This is the engine for the pmFPAAddSourceFrom{Header,View} functions.
// It uses the camera format configuration information to determine where HDUs go in the FPA.
// It returns a view corresponding to the PHU
static pmFPAview *addSource(pmFPA *fpa,       // The FPA
                            const pmFPAview *phuView, // The view corresponding to the PHU, or NULL
                            const psMetadata *header, // The PHU header, or NULL
                            const psMetadata *format, // Format of file
                            bool install // Install the provided header in the location that we find?
                           )
{
    assert(fpa);
    assert(phuView || header);
    assert(format);

    bool mdok;                          // Status of MD lookup

    psMetadata *fileInfo = psMetadataLookupMetadata(&mdok, format, "FILE"); // The file information
    if (!mdok || !fileInfo) {
        psError(PS_ERR_IO, false, "Unable to find FILE in the camera format configuration.\n");
        return NULL;
    }

    // At what level does the PHU go?
    const char *phuType = psMetadataLookupStr(&mdok, fileInfo, "PHU"); // What is the PHU?
    if (!mdok || strlen(phuType) == 0) {
        psError(PS_ERR_IO, false, "Unable to find PHU in the format specification.\n");
        return NULL;
    }

    // Prepare the PHU to be placed in the camera hierarchy
    pmHDU *phdu = pmHDUAlloc(NULL);     // The primary header data unit
    // Casting to psPtr to avoide "warning: passing arg 1 of `p_psMemIncrRefCounter' discards qualifiers from
    // pointer target type"
    phdu->header = psMemIncrRefCounter((const psPtr)header);
    phdu->format = psMemIncrRefCounter((const psPtr)format);
    pmFPAview *view = pmFPAviewAlloc(0); // View, to be returned
    if (phuView) {
        // Copy the view values, for the case where we're given a view.
        *view = *phuView;
    }

    // And at what level do the individual extensions go?
    const char *extType = psMetadataLookupStr(&mdok, fileInfo, "EXTENSIONS"); // What's in the extns?
    if (!mdok || strlen(extType) == 0) {
        psError(PS_ERR_IO, false, "Unable to find EXTENSIONS in the format specification.\n");
        psFree(view);
        return NULL;
    }

    // Now, there are a few cases:

    // Case    PHU     EXTENSIONS     Description
    // ====    ===     ==========     ===========
    // 1.      FPA     CHIP           CONTENTS(METADATA) has a list of extensions, each with chipName:chipType
    //                                CHIPS(METADATA) has a list of chip types, each with cell:type
    // 2.      FPA     CELL           CONTENTS(METADATA) has a list of extensions, each with chip:cell:type
    //                                No need for CHIPS.
    // 3.      FPA     NONE           CONTENTS(STRING) has a list of extensions, chip:cell:type
    //                                No need for CHIPS
    // 4.      CHIP    CELL           CONTENTS(METADATA) is a menu, each with a chipName:chipType
    //                                CHIPS(METADATA) has a list of chip types(METADATA), containg a list of
    //                                extensions.
    // 5.      CHIP    NONE           CONTENTS(METADATA) is a menu, each with a chipName:chipType
    //                                CHIPS(METADATA) has a list of chip types(STRING) with cell:type
    // 6.      CELL    NONE           CONTENTS(METADATA) is a menu, each with a chipName:cellName:cellType.
    //                                No need for CHIPS.


    pmFPALevel phuLevel = pmFPALevelFromName(phuType); // Level for PHU
    pmFPALevel extLevel = pmFPALevelFromName(extType); // Level for extensions

    switch (phuLevel) {
      case PM_FPA_LEVEL_FPA: {
          // We don't have to work out where the PHU is --- there's only one FPA.
          // 'view' already points to the FPA.
          switch (extLevel) {
            case PM_FPA_LEVEL_CHIP:
              phdu->blankPHU = true;
              if (install) {
		  if (!addHDUtoFPA(fpa, phdu)) {
		      psError(PS_ERR_UNKNOWN, false, "Unable to add HDU to FPA.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
		  }		  
                  if (!addSource_FPA_CHIP(fpa, format)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to add source.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
                  if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_PHU, true, NULL)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to read concepts.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
	      }
              psFree(phdu);
              return view;
            case PM_FPA_LEVEL_CELL:
	      phdu->blankPHU = true;
              if (install) {
                  if (!addHDUtoFPA(fpa, phdu)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to add HDU to FPA.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
                  if (!addSource_FPA_CELL(fpa, format)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to add source.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
                  if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_PHU, true, NULL)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to read concepts.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
              }
              psFree(phdu);
              return view;
            case PM_FPA_LEVEL_NONE:
	      phdu->blankPHU = false;
              if (install) {
                  if (!addHDUtoFPA(fpa, phdu)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to add HDU to FPA.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
                  if (!addSource_FPA_NONE(fpa, format)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to add source.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
                  if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_PHU, true, NULL)) {
                      psError(PS_ERR_UNKNOWN, false, "Unable to read concepts.");
                      psFree(phdu);
                      psFree(view);
                      return NULL;
                  }
              }
              psFree(phdu);
             return view;
            default:
              psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                      "EXTENSIONS level (%s) incompatible with PHU level (FPA)", extType);
              psFree(phdu);
              psFree(view);
              return NULL;
          }
          break;
      }
      case PM_FPA_LEVEL_CHIP: {
          pmChip *chip = NULL;          // Appropriate chip, if the view is specified
          if (phuView) {
              chip = fpa->chips->data[phuView->chip];
          }
          switch (extLevel) {
            case PM_FPA_LEVEL_CELL:
              phdu->blankPHU = true;
              if (!addSource_CHIP_CELL(view, fpa, chip, format, phdu, install)) {
                  psError(PS_ERR_UNKNOWN, false, "Unable to add source.");
                  psFree(phdu);
                  psFree(view);
                  return NULL;
              }
              psFree(phdu);
              return view;
            case PM_FPA_LEVEL_NONE:
              phdu->blankPHU = false;
              if (!addSource_CHIP_NONE(view, fpa, chip, format, phdu, install)) {
                  psError(PS_ERR_UNKNOWN, false, "Unable to add source.");
                  psFree(phdu);
                  psFree(view);
                  return NULL;
              }
              psFree(phdu);
              return view;
            default:
              psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                      "EXTENSIONS level (%s) incompatible with PHU level (CHIP)", extType);
              return NULL;
          }
          break;
      }
      case PM_FPA_LEVEL_CELL: {
          if (extLevel != PM_FPA_LEVEL_NONE) {
              psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                      "EXTENSIONS level (%s) incompatible with PHU level (CELL)", extType);
              return NULL;
          }
          pmChip *chip = NULL;          // Appropriate chip, if the view is specified
          pmCell *cell = NULL;          // Appropriate cell, if the view is specified
          if (phuView) {
              chip = fpa->chips->data[phuView->chip];
              cell = chip->cells->data[phuView->cell];
          }
          phdu->blankPHU = false;
          if (!addSource_CELL_NONE(view, fpa, cell, format, phdu, install)) {
              psError(PS_ERR_UNKNOWN, false, "Unable to add source.");
              psFree(phdu);
              psFree(view);
              return NULL;
          }
          psFree(phdu);
          return view;
          break;
      }
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Bad PHU level: %s", phuType);
        return NULL;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

pmFPA *pmFPAConstruct(const psMetadata *camera, const char *cameraName)
{
    PS_ASSERT_PTR_NON_NULL(camera, NULL);

    pmFPA *fpa = pmFPAAlloc(camera, cameraName);    // The FPA to fill out

    bool mdok = true;                   // Status from MD lookups
    psMetadata *components = psMetadataLookupMetadata(&mdok, camera, "FPA"); // FPA components
    if (!mdok || !components) {
        psError(PS_ERR_IO, true, "Failed to lookup \"FPA\"");
        psFree(fpa);
        return NULL;
    }
    psMetadataIterator *componentsIter = psMetadataIteratorAlloc(components, PS_LIST_HEAD, NULL);
    psMetadataItem *componentsItem = NULL; // Item from components
    while ((componentsItem = psMetadataGetAndIncrement(componentsIter))) {
        const char *chipName = componentsItem->name; // Name of the chip
        if (componentsItem->type != PS_DATA_STRING) {
            psWarning("Element %s in FPA within the camera configuration is not of "
                     "type STR (type=%x) --- ignored.\n", chipName, componentsItem->type);
            continue;
        }

        pmChip *chip = pmChipAlloc(fpa, chipName); // The chip
        psList *cellNames = psStringSplit(componentsItem->data.V, " ,;", true); // List of cell names
        psListIterator *cellNamesIter = psListIteratorAlloc(cellNames, PS_LIST_HEAD, false); // Iterator

        psString cellName = NULL;       // Name of cell
        while ((cellName = psListGetAndIncrement(cellNamesIter))) {
            pmCell *cell = pmCellAlloc(chip, cellName); // New cell
            psFree(cell);               // Drop reference
        }
        psFree(chip);                   // Drop reference
        psFree(cellNamesIter);
        psFree(cellNames);
    }
    psFree(componentsIter);

    return fpa;
}

bool pmFPAAddSourceFromFormat(pmFPA *fpa, const psMetadata *format)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_METADATA_NON_NULL(format, false);

    // Generate the correct structure
    pmFPALevel phuLevel = pmFPAPHULevel(format); // Level at which PHU goes
    pmFPAview *view = pmFPAviewAlloc(0);// View for current level
    if (phuLevel == PM_FPA_LEVEL_FPA) {
        if (!pmFPAAddSourceFromView(fpa, view, format)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to FPA.");
            psFree(view);
            return false;
        }
    } else {
        pmChip *chip;                       // Chip from FPA
        while ((chip = pmFPAviewNextChip(view, fpa, 1))) {
            if (phuLevel == PM_FPA_LEVEL_CHIP) {
                if (!pmFPAAddSourceFromView(fpa, view, format)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to FPA.");
                    psFree(view);
                    return false;
                }
            } else {
                pmCell *cell;                   // Cell from chip
                while ((cell = pmFPAviewNextCell(view, fpa, 1))) {
                    if (phuLevel == PM_FPA_LEVEL_CELL) {
                        if (!pmFPAAddSourceFromView(fpa, view, format)) {
                            psError(PS_ERR_UNKNOWN, false, "Unable to add PHU to FPA.");
                            psFree(view);
                            return false;
                        }
                    }
                }
            }
        }
    }
    psFree(view);

    return true;
}

bool pmFPAAddSourceFromView(pmFPA *fpa, const pmFPAview *phuView,
                            const psMetadata *format)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(phuView, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    pmFPAview *view = addSource(fpa, phuView, NULL, format, true);
    bool status = (view != NULL);
    psFree(view);
    return status;
}

pmFPAview *pmFPAAddSourceFromHeader(pmFPA *fpa, psMetadata *phu, const psMetadata *format)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(phu, NULL);
    PS_ASSERT_PTR_NON_NULL(format, NULL);

    bool mdok = true;                   // Status from metadata lookups
    psMetadata *fileInfo = psMetadataLookupMetadata(&mdok, format, "FILE"); // The file information
    if (!mdok || !fileInfo) {
        psError(PS_ERR_IO, false, "Unable to find FILE in the camera format configuration.\n");
        return NULL;
    }

    pmFPAview *view = addSource(fpa, NULL, phu, format, true); // View of PHU, to return

    return view;
}


pmFPAview *pmFPAIdentifySourceFromHeader(pmFPA *fpa, psMetadata *phu, const psMetadata *format)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(phu, NULL);
    PS_ASSERT_PTR_NON_NULL(format, NULL);

    return addSource(fpa, NULL, phu, format, false);
}


// Print spaces to indent
#define INDENT(FILE, LEVEL) \
{ \
    for (int i = 0; i < (LEVEL); i++) { \
        fprintf(FILE, " "); \
    } \
}

void pmFPAPrint(FILE *fd, const pmFPA *fpa, bool header, bool concepts)
{
    PS_ASSERT_PTR_NON_NULL(fpa,);

    INDENT(fd, 1);
    fprintf(fd, "FPA:\n");
    if (fpa->hdu) {
        pmHDUPrint(fd, fpa->hdu, 2, header);
    }
    if (concepts) {
        psMetadataPrint(fd, fpa->concepts, 2);
    }

    psArray *chips = fpa->chips;        // Array of chips
    // Iterate over the FPA
    for (int i = 0; i < chips->n; i++) {
        INDENT(fd, 3);
        fprintf(fd, "Chip: %d\n", i);
        pmChip *chip = chips->data[i]; // The chip
        if (chip->hdu) {
            pmHDUPrint(fd, chip->hdu, 4, header);
        }
        if (concepts) {
            psMetadataPrint(fd, chip->concepts, 4);
        }

        // Iterate over the chip
        psArray *cells = chip->cells;   // Array of cells
        for (int j = 0; j < cells->n; j++) {
            INDENT(fd, 5);
            fprintf(fd, "Cell: %d\n", j);
            pmCell *cell = cells->data[j]; // The cell
            if (cell->hdu) {
                pmHDUPrint(fd, cell->hdu, 6, header);
            }
            if (concepts) {
                psMetadataPrint(fd, cell->concepts, 6);
            }

            psArray *readouts = cell->readouts; // Array of readouts
            for (int k = 0; k < readouts->n; k++) {
                pmReadout *readout = readouts->data[k]; // The readout
                INDENT(fd, 6);
                fprintf(fd, "Readout %d:\n", k);
                INDENT(fd, 7);
                fprintf(fd, "col0: %d\n", readout->col0);
                INDENT(fd, 7);
                fprintf(fd, "row0: %d\n", readout->row0);
                psImage *image = readout->image; // The image
                psImage *mask = readout->mask; // The mask
                psImage *variance = readout->variance; // The variance
                psList *bias = readout->bias; // The list of bias images
                if (image) {
                    INDENT(fd, 7);
                    fprintf(fd, "Image: [%d:%d,%d:%d] (%dx%d)\n",
                            image->col0, image->col0 + image->numCols,
                            image->row0, image->row0 + image->numRows,
                            image->numCols, image->numRows);
                }
                if (bias) {
                    psListIterator *biasIter = psListIteratorAlloc(bias, PS_LIST_HEAD, false); // Iterator
                    psImage *biasImage = NULL; // Bias image from iteration
                    while ((biasImage = psListGetAndIncrement(biasIter))) {
                        INDENT(fd, 7);
                        fprintf(fd, "Bias:  [%d:%d,%d:%d] (%dx%d)\n",
                                biasImage->col0, biasImage->col0 + biasImage->numCols,
                                biasImage->row0, biasImage->row0 + biasImage->numRows,
                                biasImage->numCols, biasImage->numRows);
                    }
                    psFree(biasIter);
                }
                if (mask) {
                    INDENT(fd, 7);
                    fprintf(fd, "Mask: [%d:%d,%d:%d] (%dx%d)\n",
                            mask->col0, mask->col0 + mask->numCols,
                            mask->row0, mask->row0 + mask->numRows,
                            mask->numCols, mask->numRows);
                }
                if (variance) {
                    INDENT(fd, 7);
                    fprintf(fd, "Variance: [%d:%d,%d:%d] (%dx%d)\n",
                            variance->col0, variance->col0 + variance->numCols,
                            variance->row0, variance->row0 + variance->numRows,
                            variance->numCols, variance->numRows);
                }
            } // Iterating over cell
        } // Iterating over chip
    } // Iterating over FPA

}


pmFPALevel pmFPAPHULevel(const psMetadata *format)
{
    PS_ASSERT_METADATA_NON_NULL(format, PM_FPA_LEVEL_NONE);

    bool mdok;                          // Status of MD lookup
    psMetadata *fileInfo = psMetadataLookupMetadata(&mdok, format, "FILE"); // Contents of FILE metadata
    if (!mdok || !fileInfo) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find FILE in camera format configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }
    const char *phu = psMetadataLookupStr(&mdok, fileInfo, "PHU"); // PHU level
    if (!mdok || !phu || strlen(phu) == 0) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find PHU in FILE in camera format configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }

    return pmFPALevelFromName(phu);
}

pmFPALevel pmFPAExtensionsLevel(const psMetadata *format)
{
    PS_ASSERT_METADATA_NON_NULL(format, PM_FPA_LEVEL_NONE);

    bool mdok;                          // Status of MD lookup
    psMetadata *fileInfo = psMetadataLookupMetadata(&mdok, format, "FILE"); // Contents of FILE metadata
    if (!mdok || !fileInfo) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find FILE in camera format configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }

    const char *extensions = psMetadataLookupStr(&mdok, fileInfo, "EXTENSIONS"); // EXTENSIONS level
    if (!mdok || !extensions || strlen(extensions) == 0) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find EXTENSIONS in FILE in camera format configuration.\n");
        return PM_FPA_LEVEL_NONE;
    }

    return pmFPALevelFromName(extensions);
}


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmHDUUtils.h"
#include "pmFPA.h"
#include "pmConcepts.h"

#include "pmConceptsCopy.h"



// List of concepts not to copy, for each level.
// Must be NULL-terminated
static const char *dontCopyConceptsFPA[] = { "FPA.OBS", "FPA.NAME", "FPA.CAMERA", 0 };
static const char *dontCopyConceptsChip[] = { "CHIP.NAME", 0 };
static const char *dontCopyConceptsCell[] = { "CELL.NAME", 0 };

// Copy concepts from a source container to a target container, avoiding certain entries
static bool copyConcepts(psMetadata *target, // Target metadata container
                         psMetadata *source, // Source metadata container
                         psMetadata *specs, // Concept specifications
                         psMetadata *cameraFormat, // Camera format configuration
                         const pmFPA *fpa,    // FPA of interest
                         const pmChip *chip,  // Chip of interest, or NULL
                         const pmCell *cell,  // Cell of interest, or NULL
                         const char *dontCopyConcepts[] // Don't copy these concepts
                         )
{
    assert(target);
    assert(source);
    assert(specs);
    assert(dontCopyConcepts);

    psMetadataIterator *iter = psMetadataIteratorAlloc(source, PS_LIST_HEAD, NULL);
    psMetadataItem *sourceItem;         // Item from iteration
    while ((sourceItem = psMetadataGetAndIncrement(iter))) {
        const char *name = sourceItem->name;  // Name of concept
        bool copyOK = true;            // OK to copy
        for (int i = 0; dontCopyConcepts[i] && copyOK; i++) {
            if (!strcmp(name, dontCopyConcepts[i])) {
                copyOK = false;
            }
        }
        if (!copyOK) {
            continue;
        }

        bool mdok;                      // Status of MD lookup
        pmConceptSpec *spec = psMetadataLookupPtr(&mdok, specs, name); // Specification for concept
        psMetadataItem *copy = NULL;    // Copy of source item
        if (mdok && spec && spec->copy) {
            psMetadataItem *targetItem = psMetadataLookup(target, name); // Corresponding item from target
            copy = spec->copy(targetItem, sourceItem, cameraFormat, fpa, chip, cell);
            if (!copy) {
                psError(PS_ERR_UNKNOWN, false, "Unable to copy concept %s", name);
                return false;
            }
        } else {
            copy = psMetadataItemCopy(sourceItem);
        }
        psMetadataAddItem(target, copy, PS_LIST_TAIL, PS_META_REPLACE);
        psFree(copy);                    // Drop reference
    }
    psFree(iter);

    return true;
}


bool pmConceptsCopyFPA(pmFPA *target, const pmFPA *source, bool chips, bool cells)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);

    psMetadata *specs = pmConceptsSpecs(PM_FPA_LEVEL_FPA); // Concept specifications

    pmHDU *hdu = target->hdu;           // Header data unit
    psMetadata *format = hdu ? hdu->format : NULL; // Camera format

    // Copy FPA concepts
    if (!copyConcepts(target->concepts, source->concepts, specs, format,
                      source, NULL, NULL, dontCopyConceptsFPA)) {
        return false;
    }

    // Copy chip concepts
    bool status = true;                 // Status of chips
    if (chips) {
        psArray *targetChips = target->chips; // Chips in target
        psArray *sourceChips = source->chips; // Chips in source
        if (targetChips->n != sourceChips->n) {
            psError(PS_ERR_IO, true,
                    "Number of chips in target (%ld) and source (%ld) differ --- unable to copy concepts.",
                    targetChips->n, sourceChips->n);
            return false;
        }
        for (int i = 0; i < targetChips->n; i++) {
            pmChip *targetChip = targetChips->data[i]; // Target chip of interest
            pmChip *sourceChip = sourceChips->data[i]; // Source chip of interest
            if (!targetChip || !sourceChip) {
                continue;
            }

            status &= pmConceptsCopyChip(targetChip, sourceChip, cells);
        }
    }

    return status;
}

bool pmConceptsCopyChip(pmChip *target, const pmChip *source, bool cells)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);

    psMetadata *specs = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
    pmHDU *hdu = pmHDUFromChip(target); // Header data unit
    psMetadata *format = hdu ? hdu->format : NULL; // Camera format

    // Copy chip concepts
    if (!copyConcepts(target->concepts, source->concepts, specs, format,
                      source->parent, source, NULL, dontCopyConceptsChip)) {
        return false;
    }

    // Copy cell concepts
    bool status = true;                 // Status of cells
    if (cells) {
        psArray *targetCells = target->cells; // Cells in target
        psArray *sourceCells = source->cells; // Cells in source
        if (targetCells->n != sourceCells->n) {
            psError(PS_ERR_IO, true,
                    "Number of cells in target (%ld) and source (%ld) differ --- unable to copy concepts.",
                    targetCells->n, sourceCells->n);
            return false;
        }
        for (int j = 0; j < targetCells->n; j++) {
            pmCell *targetCell = targetCells->data[j]; // Target chip of interest
            pmCell *sourceCell = sourceCells->data[j]; // Source chip of interest
            if (! targetCell || ! sourceCell) {
                continue;
            }

            status &= pmConceptsCopyCell(targetCell, sourceCell);
        }
    }

    return status;
}


bool pmConceptsCopyCell(pmCell *target, const pmCell *source)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(source, false);

    psMetadata *specs = pmConceptsSpecs(PM_FPA_LEVEL_CHIP); // Concept specifications
    pmHDU *hdu = pmHDUFromCell(target); // Header data unit
    psMetadata *format = hdu ? hdu->format : NULL; // Camera format

    return copyConcepts(target->concepts, source->concepts, specs, format,
                        source->parent->parent, source->parent, source, dontCopyConceptsCell);
}

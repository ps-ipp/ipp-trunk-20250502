#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmConceptsUpdate.h"

bool pmConceptsUpdate(const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    if (fpa) {
        // Check for FPA concepts updates
    }

    if (chip) {
        // Check for chip concepts updates
    }

    if (cell) {
        // Check for cell concepts updates

        // CELL.READNOISE needs to be updated if specified in ADU
        if (psMetadataLookup(cell->concepts, "CELL.READNOISE.UPDATE")) {
            float gain = psMetadataLookupF32(NULL, cell->concepts, "CELL.GAIN"); // Gain for cell
            if (isfinite(gain)) {
                psMetadataItem *rn = psMetadataLookup(cell->concepts, "CELL.READNOISE"); // Read noise
                psAssert(rn && rn->type == PS_DATA_F32, "Should be of the correct type");
                rn->data.F32 *= gain;
                psMetadataRemoveKey(cell->concepts, "CELL.READNOISE.UPDATE");
            }
        }

        bool xStatus, yStatus; // Status of MD lookups
        psImageBinning *binning = psImageBinningAlloc();
        binning->nXbin = psMetadataLookupS32(&xStatus, cell->concepts, "CELL.XBIN");
        binning->nYbin = psMetadataLookupS32(&yStatus, cell->concepts, "CELL.YBIN");
        if (!xStatus || !yStatus) {
            // XXX should this be an error condition?
            psFree (binning);
            return true;
        }
        if (!binning->nXbin || !binning->nXbin) {
            // XXX should this be an error condition?
            psFree (binning);
            return true;
        }

        // CELL.TRIMSEC needs to be updated for the binning
        if (psMetadataLookup(cell->concepts, "CELL.TRIMSEC.UPDATE")) {
            psRegion *trimsec = psMetadataLookupPtr(NULL, cell->concepts, "CELL.TRIMSEC"); // Trim section
            *trimsec = psImageBinningSetRuffRegion (binning, *trimsec);
            // force integer pixels : truncate x0, roundup x1:
            trimsec->x0 = (int)trimsec->x0;
            if (trimsec->x1 > (int)trimsec->x1) {
                trimsec->x1 = (int)trimsec->x1 + 1;
            } else {
                trimsec->x1 = (int)trimsec->x1;
            }
            trimsec->y0 = (int)trimsec->y0;
            if (trimsec->y1 > (int)trimsec->y1) {
                trimsec->y1 = (int)trimsec->y1 + 1;
            } else {
                trimsec->y1 = (int)trimsec->y1;
            }
            psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC.UPDATE");
        }

        // CELL.BIASSEC needs to be updated for the binning
        if (psMetadataLookup(cell->concepts, "CELL.BIASSEC.UPDATE")) {
            psList *biassecs = psMetadataLookupPtr(NULL, cell->concepts, "CELL.BIASSEC"); // Bias sections
            psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, true); // Iterator
            psRegion *biassec; // Bias region, from iteration
            while ((biassec = psListGetAndIncrement(biassecsIter))) {
                *biassec = psImageBinningSetRuffRegion (binning, *biassec);
                // force integer pixels : truncate x0, roundup x1:
                biassec->x0 = (int)biassec->x0;
                if (biassec->x1 > (int)biassec->x1) {
                    biassec->x1 = (int)biassec->x1 + 1;
                } else {
                    biassec->x1 = (int)biassec->x1;
                }
                biassec->y0 = (int)biassec->y0;
                if (biassec->y1 > (int)biassec->y1) {
                    biassec->y1 = (int)biassec->y1 + 1;
                } else {
                    biassec->y1 = (int)biassec->y1;
                }
            }
            psFree(biassecsIter);
            psMetadataRemoveKey(cell->concepts, "CELL.BIASSEC.UPDATE");
        }
        psFree (binning);
    }

    return true;
}

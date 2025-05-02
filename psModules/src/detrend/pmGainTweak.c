#include <stdio.h>
#include <pslib.h>

#include "pmFPA.h"

#include "pmGainTweak.h"

bool pmGainTweak(const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    pmChip *chip = cell->parent;
    PS_ASSERT_PTR_NON_NULL(chip, false);

    int numCells = chip->cells;         // Number of cells
    psVector *gains = psVectorAlloc(numCells, PS_TYPE_F32); // Gain for each cell
    psVector *mask = psVectorAlloc(numCells, PS_TYPE_VECTOR_MASK); // Mask for gains
    psVectorInit(mask, 0);
    int numGood = 0;                    // Number of good gains

   for (int i = 0; i < numCells; i++) {
        pmCell *otherCell = chip->cells->data[i]; // A different cell
        if (otherCell == cell) {
            mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xFF;
            continue;
        }
        float gain = psMetadataLookupF32(NULL, otherCell->concepts, "CELL.GAIN"); // Gain for cell
        gains->data.F32[i] = gain;
        if (!isfinite(gains->data.F32[i])) {
            mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xFF;
        } else {
            numGood++;
        }
   }

   if (numGood == 0) {
       psError(PM_ERR_DETREND, true, "No other cell gains available with which to tweak gain.");
       psFree(gains);
       psFree(mask);
       return false;
   }

   psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN); // Statistics
   if (!psVectorStats(stats, gains, NULL, mask, 0xFF)) {
       psError(psErrorCodeLast(), false, "Unable to determine target gain");
       psFree(stats);
       psFree(gains);
       psFree(mask);
       return false;
   }
   float target = stats->sampleMedian;  // Target gain
   psFree(stats);
   psFree(gains);
   psFree(mask);

   psMetadataItem *item = psMetadataLookup(cell->concepts, "CELL.GAIN");
   item->data.F32 = target;

   return true;
}

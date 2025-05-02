#include "ppStatsInternal.h"

bool ppStatsFringe(psMetadata *stats, const pmChip *chip, const char *root, const char *fringeName)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_STRING_NON_EMPTY(fringeName, false);

    bool mdok;                          // Status of MD lookup
    const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip

    psMetadata *chipStats = psMetadataLookupMetadata(&mdok, stats, chipName); // Chip statistics
    if (!mdok || !chipStats) {
        chipStats = psMetadataAlloc();
        psMetadataAddMetadata(stats, PS_LIST_TAIL, chipName, 0, NULL, chipStats);
    } else {
        psMemIncrRefCounter(chipStats);
    }

    pmFringeScale *fringes = psMetadataLookupPtr(NULL, chip->analysis, fringeName); // Solution for fringes
    for (int i = 0; i < fringes->nFringeFrames; i++) {
        psString name = NULL; // Name of statistic
        psStringAppend(&name, "%s_%d", root, i);
        psMetadataAddF32(chipStats, PS_LIST_TAIL, name, 0, "Fringe amplitude",
                         fringes->coeff->data.F32[i + 1]);
        psFree(name);
        name = NULL;
        psStringAppend(&name, "%s_ERR_%d", root, i);
        psMetadataAddF32(chipStats, PS_LIST_TAIL, name, 0, "Fringe amplitude error",
                         fringes->coeffErr->data.F32[i + 1]);
        psFree(name);
    }

    psFree(chipStats);

    return true;
}


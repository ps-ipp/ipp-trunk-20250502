#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <pslib.h>

#include "pmFPALevel.h"

static const char *nameNONE = "NONE";   ///< Name for PM_FPA_LEVEL_NONE
static const char *nameFPA = "FPA";     ///< Name for PM_FPA_LEVEL_FPA
static const char *nameCHIP = "CHIP";   ///< Name for PM_FPA_LEVEL_CHIP
static const char *nameCELL = "CELL";   ///< Name for PM_FPA_LEVEL_CELL
static const char *nameREADOUT = "READOUT"; ///< Name for PM_FPA_LEVEL_READOUT
static const char *nameCHUNK = "CHUNK"; ///< Name for PM_FPA_LEVEL_CHUNK

const char *pmFPALevelToName(pmFPALevel level)
{
    switch (level) {
    case PM_FPA_LEVEL_NONE:
        return nameNONE;
    case PM_FPA_LEVEL_FPA:
        return nameFPA;
    case PM_FPA_LEVEL_CHIP:
        return nameCHIP;
    case PM_FPA_LEVEL_CELL:
        return nameCELL;
    case PM_FPA_LEVEL_READOUT:
        return nameREADOUT;
    case PM_FPA_LEVEL_CHUNK:
        return nameCHUNK;
    default:
        psAbort("You can't get here; level = %d", level);
    }
    return NULL;
}

pmFPALevel pmFPALevelFromName(const char *name)
{
    if (name == NULL) {
        return PM_FPA_LEVEL_NONE;
    }
    if (!strcasecmp(name, nameFPA)) {
        return PM_FPA_LEVEL_FPA;
    }
    if (!strcasecmp(name, nameCHIP)) {
        return PM_FPA_LEVEL_CHIP;
    }
    if (!strcasecmp(name, nameCELL)) {
        return PM_FPA_LEVEL_CELL;
    }
    if (!strcasecmp(name, nameREADOUT)) {
        return PM_FPA_LEVEL_READOUT;
    }
    if (!strcasecmp(name, nameCHUNK)) {
        return PM_FPA_LEVEL_CHUNK;
    }
    if (!strcasecmp(name, nameNONE)) {
        return PM_FPA_LEVEL_NONE;
    }

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised FPA level name: %s", name);
    return PM_FPA_LEVEL_NONE;
}



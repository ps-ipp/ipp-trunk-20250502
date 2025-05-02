#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmConceptsRead.h"
#include "pmFPAHeader.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmCellReadHeader(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    if (!cell->hdu) {
        return pmChipReadHeader(cell->parent, fits, config);
    }
    if (!pmHDUReadHeader(cell->hdu, fits)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell.\n");
        return false;
    }

    return pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_DATABASE, false, config);
}


bool pmChipReadHeader(pmChip *chip, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    if (!chip->hdu) {
        return pmFPAReadHeader(chip->parent, fits, config);
    }
    if (!pmHDUReadHeader(chip->hdu, fits)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell.\n");
        return false;
    }

    if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_DATABASE, true, true, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read concepts for chip.\n");
        return false;
    }

    return true;
}


bool pmFPAReadHeader(pmFPA *fpa, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    if (!fpa->hdu) {
        return false;
    }
    if (!pmHDUReadHeader(fpa->hdu, fits)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell.\n");
        return false;
    }

    if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_DATABASE, true, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read concepts for FPA.\n");
        return false;
    }

    return true;
}

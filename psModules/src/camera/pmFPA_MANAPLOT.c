/** @file  pmFPA_MANAPLOT.c
 *
 * This file contains functions to write MANAPLOT images.
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-06-10 20:58:28 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAfile.h"
#include "pmFPAview.h"
#include "pmFPA_MANAPLOT.h"


bool pmFPAviewWriteMANAPLOT(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        pmFPAWriteMANAPLOT (fpa, view, file, config);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipWriteMANAPLOT (chip, view, file, config);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellWriteMANAPLOT (cell, view, file, config);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    pmReadoutWriteMANAPLOT (readout, view, file, config);
    return true;
}

// read in all chip-level MANAPLOT files for this FPA
bool pmFPAWriteMANAPLOT (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    for (int i = 0; i < fpa->chips->n; i++) {

        pmChip *chip = fpa->chips->data[i];
        pmChipWriteMANAPLOT (chip, view, file, config);
    }
    return true;
}

// read in all cell-level MANAPLOT files for this chip
bool pmChipWriteMANAPLOT (pmChip *chip, const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    for (int i = 0; i < chip->cells->n; i++) {

        pmCell *cell = chip->cells->data[i];
        pmCellWriteMANAPLOT (cell, view, file, config);
    }
    return true;
}

// read in all readout-level MANAPLOT files for this cell
bool pmCellWriteMANAPLOT (pmCell *cell, const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    for (int i = 0; i < cell->readouts->n; i++) {

        pmReadout *readout = cell->readouts->data[i];
        pmReadoutWriteMANAPLOT (readout, view, file, config);
    }
    return true;
}

// read in all readout-level Objects files for this cell
bool pmReadoutWriteMANAPLOT (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    if (file->type != PM_FPA_FILE_MANAPLOT) {
        psError(PS_ERR_UNKNOWN, true, "warning: type mismatch");
        return false;
    }

    // XXX does not have to be a mana script...
    // this function will call a mana script of the form:
    // scriptname (input) (output)
    // scriptname : extname
    // input : filextra
    // output : filerule

    bool create = file->mode == PM_FPA_MODE_WRITE ? true : false;

    psString tmpName;
    tmpName = pmFPAfileNameFromRule (file->filextra, file, view);
    psString input = pmConfigConvertFilename (tmpName, config, create, false);
    psFree (tmpName);

    tmpName = pmFPAfileNameFromRule (file->filerule, file, view);
    psString output = pmConfigConvertFilename (tmpName, config, create, false);
    psFree (tmpName);

    tmpName = pmFPAfileNameFromRule (file->extname, file, view);
    psString script = pmConfigConvertFilename (tmpName, config, create, false);
    psFree (tmpName);

    psString line = NULL;
    psStringAppend (&line, "%s %s %s", script, input, output);

    // capture the stdout and stderr?
    // XXX use psPipe instead?
    int status = system (line);
    if (status == -1) {
        psError(PS_ERR_UNKNOWN, true, "fork failure: %s", script);
        return false;
    }
    if (WEXITSTATUS(status) != 0) {
        psError(PS_ERR_UNKNOWN, true, "error running: %s", script);
        return false;
    }

    psFree(line);
    psFree(input);
    psFree(output);
    psFree(script);

    return true;
}

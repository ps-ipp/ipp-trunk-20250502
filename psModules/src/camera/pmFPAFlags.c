#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAFlags.h"


/** functions to turn on/off the file_exists flag **/
bool pmFPASetFileStatus(pmFPA *fpa, bool status)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        pmChipSetFileStatus (chip, status);
    }
    return true;
}

bool pmChipSetFileStatus(pmChip *chip, bool status)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);

    chip->file_exists = status;
    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        pmCellSetFileStatus (cell, status);
    }
    return true;
}

bool pmCellSetFileStatus(pmCell *cell, bool status)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);

    cell->file_exists = status;
    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        readout->file_exists = status;
    }
    return true;
}

bool pmReadoutSetFileStatus(pmReadout *readout, bool status)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    readout->file_exists = status;
    return true;
}

bool pmFPAviewSetFileStatus (pmFPA *fpa, const pmFPAview *view, bool status) {

    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (view->chip == -1) {
	bool set = pmFPASetFileStatus (fpa, status);
        return set;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        bool set = pmChipSetFileStatus (chip, status);
        return set;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        bool set = pmCellSetFileStatus (cell, status);
        return set;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_IO, true, "Requested readout == %d >= cell->readouds->n == %ld", view->readout, cell->readouts->n);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    bool set = pmReadoutSetFileStatus (readout, status);
    return set;
}

bool pmFPACheckFileStatus(const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!pmChipCheckFileStatus(chip)) {
            return false;
        }
    }
    return true;
}

bool pmChipCheckFileStatus(const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    if (!chip->file_exists) {
        return false;
    }

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        if (!pmCellCheckFileStatus(cell)) {
            return false;
        }
    }
    return true;
}

bool pmCellCheckFileStatus(const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    if (!cell->file_exists) {
        return false;
    }

    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        if (!readout->file_exists) {
            return false;
        }
    }
    return true;
}

/** functions to turn on/off the data_exists flag **/
bool pmFPASetDataStatus(pmFPA *fpa, bool status)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        pmChipSetDataStatus (chip, status);
    }
    return true;
}

bool pmChipSetDataStatus(pmChip *chip, bool status)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);

    chip->data_exists = status;
    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        pmCellSetDataStatus (cell, status);
    }
    return true;
}

bool pmCellSetDataStatus (pmCell *cell, bool status)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);

    cell->data_exists = status;
    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        pmReadoutSetDataStatus(readout, status);
    }
    return true;
}

bool pmReadoutSetDataStatus (pmReadout *readout, bool status)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    readout->data_exists = status;
    return true;
}

bool pmFPAviewSetDataStatus (pmFPA *fpa, const pmFPAview *view, bool status) {

    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (view->chip == -1) {
	bool set = pmFPASetDataStatus (fpa, status);
        return set;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        bool set = pmChipSetDataStatus (chip, status);
        return set;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        bool set = pmCellSetDataStatus (cell, status);
        return set;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_IO, true, "Requested readout == %d >= cell->readouds->n == %ld", view->readout, cell->readouts->n);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    bool set = pmReadoutSetDataStatus (readout, status);
    return set;
}

// pmFPA does not have its own data_exists flag; check its children
bool pmFPACheckDataStatus (const pmFPA *fpa) {

    PS_ASSERT_PTR_NON_NULL(fpa, false);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (chip == NULL) continue;
        if (chip->data_exists) return true;
    }
    return false;
}

bool pmChipCheckDataStatus (const pmChip *chip) {

    PS_ASSERT_PTR_NON_NULL(chip, false);

    return (chip->data_exists);
}

bool pmCellCheckDataStatus (const pmCell *cell) {

    PS_ASSERT_PTR_NON_NULL(cell, false);

    return (cell->data_exists);
}

bool pmReadoutCheckDataStatus (const pmReadout *readout) {

    PS_ASSERT_PTR_NON_NULL(readout, false);

    return (readout->data_exists);
}

bool pmFPAviewCheckDataStatus (const pmFPA *fpa, const pmFPAview *view) {

    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (view->chip == -1) {
        bool exists = pmFPACheckDataStatus (fpa);
        return exists;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        bool exists = pmChipCheckDataStatus (chip);
        return exists;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        bool exists = pmCellCheckDataStatus (cell);
        return exists;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_IO, true, "Requested readout == %d >= cell->readouds->n == %ld", view->readout, cell->readouts->n);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    bool exists = pmReadoutCheckDataStatus (readout);
    return exists;
}

// Set cells within a chip to be processed or not
static bool setCellsProcess(const pmChip *chip, // Chip of interest
                            bool process  // Process this chip?
                           )
{
    PS_ASSERT_PTR_NON_NULL(chip, false);

    psArray *cells = chip->cells;       // Component cells
    if (! cells) {
        return false;
    }
    for (int i = 0; i < cells->n; i++) {
        pmCell *tmpCell = cells->data[i]; // Cell of interest
        if (tmpCell) {
            tmpCell->process = process;
        }
    }

    return true;
}


bool pmFPASelectChip(pmFPA *fpa, int chipNum, bool exclusive)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    psArray *chips = fpa->chips;        // Component chips
    if ((chips == NULL) || (chipNum >= chips->n)) {
        return(false);
    }

    for (int i = 0 ; i < chips->n ; i++) {
        pmChip *tmpChip = (pmChip *) chips->data[i];
        if (tmpChip == NULL) {
            continue;
        }
        if (i == chipNum) {
            tmpChip->process = true;
            setCellsProcess(tmpChip, true);
        } else {
            if (exclusive) {
                tmpChip->process = false;
                setCellsProcess(tmpChip, false);
            }
        }

    }

    return true;
}

// XXX this function should probably be re-defined to merge with 'setCellsProcess'
bool pmChipSelectCell(pmChip *chip, int cellNum, bool exclusive)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);

    psArray *cells = chip->cells;       // Component cells
    if (!cells || cellNum > cells->n) {
        return false;
    }

    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];
        if (!cell) {
            continue;
        }
        if (i == cellNum) {
            cell->process = true;
        } else {
            if (exclusive) {
                cell->process = false;
            }
        }
    }
    return true;
}


// XXX this function should probably be re-defined to merge with 'setCellsProcess'
bool pmChipSelectCells(pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);

    psArray *cells = chip->cells;       // Component cells
    if (!cells) {
        return false;
    }

    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];
        if (!cell) {
            continue;
        }
	cell->process = true;
    }
    return true;
}


int pmFPAExcludeChip(pmFPA *fpa, int chipNum)
{
    PS_ASSERT_PTR_NON_NULL(fpa, -1);

    psArray *chips = fpa->chips;        // Component chips
    if (chips == NULL) {
        psWarning("WARNING: fpa->chips == NULL\n");
        return(0);
    }
    if ((chipNum >= chips->n) || (NULL == (pmChip *) chips->data[chipNum])) {
        psWarning("WARNING: the specified chip (%d) does not exist.\n", chipNum);
        return(0);
    }

    int numChips = 0;                   // Number of chips to be processed
    for (int i = 0 ; i < chips->n ; i++) {
        pmChip *tmpChip = (pmChip *) chips->data[i]; // Chip of interest
        if (tmpChip != NULL) {
            if (i == chipNum) {
                tmpChip->process = false;
                setCellsProcess(tmpChip, false); // Wipe out the cell as well
            } else if (tmpChip->process) {
                numChips++;
            }
        }
    }

    return(numChips);
}


// turn off all chips
bool pmFPAExcludeChips(pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    psArray *chips = fpa->chips;        // Component chips
    if (chips == NULL) {
        psWarning("WARNING: fpa->chips == NULL\n");
        return false;
    }

    for (int i = 0 ; i < chips->n ; i++) {
        pmChip *tmpChip = (pmChip *) chips->data[i]; // Chip of interest
        if (tmpChip != NULL) {
	  tmpChip->process = false;
	  setCellsProcess(tmpChip, false); // Wipe out the cell as well
        }
    }

    return true;
}

int pmChipExcludeCell(pmChip *chip, int cellNum)
{
    PS_ASSERT_PTR_NON_NULL(chip, -1);

    psArray *cells = chip->cells;       // The component cells
    if (!cells || cellNum > cells->n) {
        return 0;
    }

    int numCells = 0;                   // Number of cells to be processed
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];
        if (!cell) {
            continue;
        }
        if (i == cellNum) {
            cell->process = false;
        } else {
            numCells++;
        }
    }

    return numCells;
}


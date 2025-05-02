/** @file  pmFPAview.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-09-04 03:09:21 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmHDUUtils.h"
#include "pmFPAview.h"

static void pmFPAviewFree(pmFPAview *view)
{
    // No reason to keep this function, apart from the fact that it allows us to type the memBlock
    return;
}

pmFPAview *pmFPAviewAlloc(int nRows)
{
    pmFPAview *view = psAlloc(sizeof(pmFPAview));
    psMemSetDeallocator(view, (psFreeFunc) pmFPAviewFree);

    view->nRows = nRows;
    pmFPAviewReset(view);
    return view;
}

bool psMemCheckFPAview(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmFPAviewFree);
}


bool pmFPAviewReset(pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(view, false);

    view->chip    = -1;
    view->cell    = -1;
    view->readout = -1;
    view->iRows   =  0;
    return true;
}

// return a view restricted to the level (must be >= the input level)
pmFPAview *pmFPAviewForLevel(pmFPALevel level, const pmFPAview *input)
{
    PS_ASSERT_PTR_NON_NULL(input, NULL);

    pmFPAview *output = pmFPAviewAlloc (input->nRows);
    *output = *input;

    switch (level) {
      case PM_FPA_LEVEL_FPA:
        output->chip = -1;
      case PM_FPA_LEVEL_CHIP:
        output->cell = -1;
      case PM_FPA_LEVEL_CELL:
        output->readout = -1;
        break;
      default:
        break;
    }
    return output;
}

pmFPALevel pmFPAviewLevel(const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(view, PM_FPA_LEVEL_NONE);

    if (view->chip < 0) {
        return PM_FPA_LEVEL_FPA;
    }
    if (view->cell < 0) {
        return PM_FPA_LEVEL_CHIP;
    }
    if (view->readout < 0) {
        return PM_FPA_LEVEL_CELL;
    }
    return PM_FPA_LEVEL_READOUT;
}

pmChip *pmFPAviewThisChip(const pmFPAview *view, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, NULL);

    if (view->chip < 0) {
        return NULL;
    }

    if (view->chip >= fpa->chips->n) {
        return NULL;
    }

    pmChip *chip = fpa->chips->data[view->chip];
    return chip;
}

pmChip *pmFPAviewNextChip(pmFPAview *view, const pmFPA *fpa, int nStep)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    view->cell = -1;
    view->readout = -1;
    view->iRows = 0;

    // if there are no available chips, return NULL
    if (fpa->chips->n <= 0) {
        view->chip = -1;
        return NULL;
    }

    // clean up < -1 values
    if (view->chip < -1) {
        view->chip = -1;
    }

    // increment to the next chip
    view->chip += nStep;

    // if we are at the end of the stack, return NULL
    if (view->chip >= fpa->chips->n) {
        view->chip = -1;
        return NULL;
    }

    // get the correct chip pointer
    pmChip *chip = fpa->chips->data[view->chip];
    return (chip);
}

pmCell *pmFPAviewThisCell(const pmFPAview *view, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    if (view->cell < 0) {
        return NULL;
    }
    PS_ASSERT_PTR_NON_NULL(fpa->chips, NULL);

    pmChip *chip = pmFPAviewThisChip (view, fpa);
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    if (view->cell >= chip->cells->n) {
        return NULL;
    }

    pmCell *cell = chip->cells->data[view->cell];
    return cell;
}

pmCell *pmFPAviewNextCell (pmFPAview *view, const pmFPA *fpa, int nStep)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    pmChip *chip = pmFPAviewThisChip (view, fpa);
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    view->readout = -1;
    view->iRows = 0;

    // if there are no available cells, return NULL
    if (chip->cells->n <= 0) {
        view->cell = -1;
        return NULL;
    }

    // clean up < -1 values
    if (view->cell < -1) {
        view->cell = -1;
    }

    // increment to the next cell
    view->cell += nStep;

    // if we are at the end of the stack, return NULL
    if (view->cell >= chip->cells->n) {
        view->cell = -1;
        return NULL;
    }

    // get the correct cell pointer
    pmCell *cell = chip->cells->data[view->cell];
    return (cell);
}

pmReadout *pmFPAviewThisReadout (const pmFPAview *view, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    if (view->readout < 0) {
        return NULL;
    }

    pmCell *cell = pmFPAviewThisCell (view, fpa);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, NULL);

    if (view->readout >= cell->readouts->n) {
        return NULL;
    }

    pmReadout *readout = cell->readouts->data[view->readout];
    return readout;
}

pmReadout *pmFPAviewNextReadout (pmFPAview *view, const pmFPA *fpa, int nStep)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    pmCell *cell = pmFPAviewThisCell (view, fpa);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    view->iRows = 0;

    // if there are no available cells, return NULL
    if (cell->readouts->n <= 0) {
        view->readout = -1;
        return NULL;
    }

    // clean up < -1 values
    if (view->readout < -1) {
        view->readout = -1;
    }

    // increment to the next cell
    view->readout += nStep;

    // if we are at the end of the stack, return NULL
    if (view->readout >= cell->readouts->n) {
        view->readout = -1;
        return NULL;
    }

    // get the correct cell pointer
    pmReadout *readout = cell->readouts->data[view->readout];
    return (readout);
}

pmHDU *pmFPAviewThisHDU(const pmFPAview *view, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    // the HDU is attached to a cell, chip or fpa
    // if this view has a -1 for the level which contains the hdu,
    // there is no unambiguous HDU

    if (view->chip < 0) {
        return pmHDUFromFPA (fpa);
    }
    if (view->cell < 0) {
        return pmHDUFromChip (pmFPAviewThisChip (view, fpa));
    }
    if (view->readout < 0) {
        return pmHDUFromCell (pmFPAviewThisCell (view, fpa));
    }
    return pmHDUFromReadout (pmFPAviewThisReadout (view, fpa));
}

pmHDU *pmFPAviewThisPHU(const pmFPAview *view, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    // select the HDU which corresponds to the PHU containing this view

    pmHDU *hdu;
    pmFPAview new;
    pmChip *chip;
    pmCell *cell;

    new = *view;

    if (view->chip < 0) {
        hdu = pmHDUFromFPA (fpa);
        if (!hdu)
            return NULL;
        if (hdu->blankPHU)
            return hdu;
        return NULL;
    }
    if (view->cell < 0) {
        chip = pmFPAviewThisChip (view, fpa);
        hdu  = pmHDUFromChip (chip);
        if (!hdu)
            return NULL;
        if (hdu->blankPHU)
            return hdu;
        new.chip = -1;
        hdu = pmFPAviewThisPHU (&new, fpa);
        return hdu;
    }
    if (view->readout < 0) {
        cell = pmFPAviewThisCell (view, fpa);
        hdu  = pmHDUFromCell (cell);
        if (!hdu) {
            psAbort("a split readout is not covered by the current paradigm");
        }
        if (hdu->blankPHU)
            return hdu;
        new.cell = -1;
        hdu = pmFPAviewThisPHU (&new, fpa);
        return hdu;
    }
    return NULL;
}

pmFPAview *pmFPAviewGenerate(const pmFPA *fpa, const pmChip *chip, const pmCell *cell,
                             const pmReadout *readout)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    pmFPAview *view = pmFPAviewAlloc(0);// View to return

    if (!chip) {
        return view;
    }

    for (view->chip = 0; view->chip < fpa->chips->n && fpa->chips->data[view->chip] != chip; view->chip++);
    if (view->chip == fpa->chips->n) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find chip %p in fpa.", chip);
        psFree(view);
        return NULL;
    }

    if (!cell) {
        return view;
    }

    for (view->cell = 0; view->cell < chip->cells->n && chip->cells->data[view->cell] != cell; view->cell++);
    if (view->cell == chip->cells->n) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find cell %p in chip.", cell);
        psFree(view);
        return NULL;
    }

    if (!readout) {
        return view;
    }

    for (view->readout = 0;
         view->readout < cell->readouts->n && cell->readouts->data[view->readout] != readout;
         view->readout++);
    if (view->readout == cell->readouts->n) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find readout %p in cell.", readout);
        psFree(view);
        return NULL;
    }

    return view;
}

pmFPAview *pmFPAviewTop(const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    pmFPAview *view = pmFPAviewAlloc(0);// View to top

    view->chip = -1;
    view->cell = -1;
    if (!fpa->hdu) {
        int numChips = 0;                   // Number of active chips
        psArray *chips = fpa->chips;        // Chips of interest
        for (int i = 0; i < chips->n; i++) {
            pmChip *chip = chips->data[i];  // Chip of interest
            if (!chip) {
                continue;
            }
            if (chip->hdu) {
                numChips++;
                view->chip = i;
            } else {
                int numCells = 0;               // Number of active cells
                psArray *cells = chip->cells;   // Cells of interest
                for (int j = 0; j < cells->n; j++) {
                    pmCell *cell = cells->data[j]; // Cell of interest
                    if (!cell) {
                        continue;
                    }
                    if (cell->hdu) {
                        numCells++;
                        view->cell = j;
                    }
                }

                if (numCells > 1) {
                    if (numCells != cells->n) {
                        psWarning("More than one, but not all cells are active.");
                    }
                    view->cell = -1;
                }
            }
        }

        if (numChips > 1) {
            if (numChips != chips->n) {
                psWarning("More than one, but not all chips are active.");
            }
            view->chip = -1;
            view->cell = -1;
        }
    }

    return view;
}


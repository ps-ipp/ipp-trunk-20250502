/** @file  pmSourcePlot.c
 *
 * This file contains functions to write source plots
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-02 20:52:01 $
 *
 *  Copyright 2006 IfA, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmSourcePlots.h"

// this function is called for the specific plotting program at the fileLevel
// fileLevel must be >= chip
bool pmFPAviewWriteSourcePlot(const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    if ((view->readout != -1) || (view->cell != -1)) {
        psError(PS_ERR_UNKNOWN, true, "source plots must have fileLevel >= chip");
        return false;
    }

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        pmFPAWriteSourcePlot (fpa, view, file, config, NULL);
        return true;
    }

    pmChipWriteSourcePlot (fpa, view, file, config, NULL);
    return true;
}

// read in all chip-level SourcePlot files for this FPA
bool pmFPAWriteSourcePlot (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    // if the layout is not defined, this level is the top level being plotted
    // determine the plot layout
    if (!layout) {
        layout = pmSourcePlotLayoutAlloc ();
        // XXX temporary hardwired values for essence?
        layout->nX = 1;
        layout->nY = 1;
        layout->aspectRatio = 1.0;
        // count the number of chips and their layout (xrange, yrange, axis ratio)
        for (int i = 0; i < fpa->chips->n; i++) {
            pmChip *chip = fpa->chips->data[i];
            if (chip->data_exists) {
                layout->nTotal ++;
            }
        }
    }

    pmFPAview *chipView = pmFPAviewAlloc(0);

    while (pmFPAviewNextChip (chipView, fpa, 1) != NULL) {
        pmChipWriteSourcePlot (fpa, chipView, file, config, layout);
        layout->i ++;
        layout->iX ++;
        if (layout->iX == layout->nX) {
            layout->iY++;
            layout->iX = 0;
        }
    }
    psFree (chipView);
    psFree (layout);
    return true;
}

// read in all cell-level SourcePlot files for this chip
bool pmChipWriteSourcePlot (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config, pmSourcePlotLayout *layout)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    // if the layout is not defined, this level is the top level being plotted
    // determine the plot layout (always single chip)
    if (!layout) {
        layout = pmSourcePlotLayoutAlloc ();
        layout->nTotal = 1;
        layout->nX = 1;
        layout->nY = 1;
        layout->aspectRatio = 1.0;
    } else {
        psMemIncrRefCounter (layout);
    }

    pmFPAview *newView = pmFPAviewAlloc(0);
    newView->chip = view->chip;

    while (pmFPAviewNextCell (newView, fpa, 1) != NULL) {
        while (pmFPAviewNextReadout (newView, fpa, 1) != NULL) {

            if (!strcmp (file->name, "SOURCE.PLOT.PSFMODEL")) {
                pmSourcePlotPSFModel (newView, file, config, layout);
            }
            if (!strcmp (file->name, "SOURCE.PLOT.MOMENTS")) {
                pmSourcePlotMoments (newView, file, config, layout);
            }
            if (!strcmp (file->name, "SOURCE.PLOT.APRESID")) {
                pmSourcePlotApResid (newView, file, config, layout);
            }
        }
    }
    psFree (newView);
    psFree (layout);
    return true;
}

static void pmSourcePlotLayoutFree(pmSourcePlotLayout *layout)
{
    return;
}

pmSourcePlotLayout *pmSourcePlotLayoutAlloc(void)
{
    pmSourcePlotLayout *layout = (pmSourcePlotLayout *)psAlloc(sizeof(pmSourcePlotLayout));
    psMemSetDeallocator(layout, (psFreeFunc)pmSourcePlotLayoutFree );

    layout->nX = layout->nY = layout->nTotal = 0;
    layout->iX = layout->iY = layout->i  =  0;
    layout->aspectRatio = 1.0;
    return (layout);
}

bool psMemCheckSourcePlotLayout(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourcePlotLayoutFree);
}


/* we have three types of plots to produce (maybe more?)
 * for each plot, we need to supply the following information:
 *     - what are overall dimensions?
 *     - what is the relative position of this element?
 *     - what are the dimensions of this element?
 *     - create a new page for this element?

 * the answers to these questions come from slightly different locations
 * for the different types of plots:

 *  1) focal-plane layout based plots:
 *     - dimensions depend on fileLevel

 *  2) multi-row plots:
 *     - dimensions depend on total number of rows

 *  2) multi-page plots:
 *     - dimensions depend on total number of rows
 */


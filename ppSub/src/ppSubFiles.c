#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"


// Input files
static const char *inputFiles[] = { "PPSUB.INPUT", "PPSUB.INPUT.MASK", "PPSUB.INPUT.VARIANCE",
                                    "PPSUB.INPUT.SOURCES", "PPSUB.REF", "PPSUB.REF.MASK",
                                    "PPSUB.REF.VARIANCE", "PPSUB.REF.SOURCES", NULL };

// Convolved files
static const char *convFiles[] = { "PPSUB.INPUT.CONV", "PPSUB.INPUT.CONV.MASK", "PPSUB.INPUT.CONV.VARIANCE",
                                   "PPSUB.REF.CONV", "PPSUB.REF.CONV.MASK", "PPSUB.REF.CONV.VARIANCE",
                                   NULL };

// Subtraction files
static const char *subFiles[] = { "PPSUB.OUTPUT", "PPSUB.OUTPUT.MASK", "PPSUB.OUTPUT.VARIANCE",
                                  "PPSUB.OUTPUT.JPEG1", "PPSUB.OUTPUT.JPEG2", "PPSUB.OUTPUT.RESID.JPEG",
                                  NULL };

// Subtraction photometry
static const char *subPhotFiles[] = { "PPSUB.OUTPUT.SOURCES", NULL };

// Inverse subtraction files
static const char *invFiles[] = { "PPSUB.INVERSE", "PPSUB.INVERSE.MASK", "PPSUB.INVERSE.VARIANCE", NULL };

// Inverse subtraction photometry
static const char *invPhotFiles[] = { "PPSUB.INVERSE.SOURCES", NULL };

// PSF files
static const char *psfFiles[] = { "PSPHOT.PSF.SAVE", NULL };

// Calculation (may be either input or output) files
static const char *calcFiles[] = { "PPSUB.OUTPUT.KERNELS", NULL };


// Activate/deactivate a list of files
static void filesActivate(pmConfig *config, // Configuration
                          const char **files, // List of files
                          bool state    // Activation status to set
    )
{
    for (int i = 0; files[i]; i++) {
        pmFPAfileActivate(config->files, state, files[i]);
    }
    return;
}

// Activate/deactivate a list of files depending on their 'save' boolean.
//  This is so we can activate/deactivate the 'calculation' files, which may be either input or output, which
// is indicated by their 'save' boolean.
static void filesActivateSave(pmConfig *config, // Configuration
                              const char **files, // List of files
                              bool save, // Activate when this save state is set
                              bool state // Activation status to set
    )
{
    for (int i = 0; files[i]; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, files[i], 0);
        if (file && file->save == save) {
            pmFPAfileActivate(config->files, state, files[i]);
        }
    }
    return;
}

void ppSubFilesActivate(pmConfig *config, ppSubFiles files, bool state)
{
    psAssert(config, "Require configuration");

    if (files & PPSUB_FILES_INPUT) {
        filesActivate(config, inputFiles, state);
        filesActivateSave(config, calcFiles, false, state);
    }
    if (files & PPSUB_FILES_CONV) {
        filesActivate(config, convFiles, state);
    }
    if (files & PPSUB_FILES_SUB) {
        filesActivate(config, subFiles, state);
        filesActivateSave(config, calcFiles, true, state);
    }
    if (files & PPSUB_FILES_INV) {
        filesActivate(config, invFiles, state);
    }
    if (files & PPSUB_FILES_PHOT_SUB) {
        filesActivate(config, subPhotFiles, state);
    }
    if (files & PPSUB_FILES_PHOT_INV) {
        filesActivate(config, invPhotFiles, state);
    }
    if (files & PPSUB_FILES_PSF) {
        filesActivate(config, psfFiles, state);
    }
    if (files & PPSUB_FILES_PHOT) {
        psphotFilesActivate(config, state);
    }

    return;
}


pmFPAview *ppSubViewReadout(void)
{
    pmFPAview *view = pmFPAviewAlloc(0);
    view->chip = view->cell = view->readout = 0;
    return view;
}

bool ppSubFilesIterateDown(pmConfig *config, ppSubFiles files)
{
    psAssert(config, "Require configuration");

    ppSubFilesActivate(config, PPSUB_FILES_ALL, false);
    ppSubFilesActivate(config, files, true);

    pmFPAview *view = pmFPAviewAlloc(0);// View to FPA top

    // FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psFree (view);
        return false;
    }

    // Chip
    view->chip = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psFree (view);
        return false;
    }

    // Cell
    view->cell = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psFree (view);
        return false;
    }

    // Readout
    view->readout = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psFree (view);
        return false;
    }

    ppSubFilesActivate(config, PPSUB_FILES_ALL, false);

    psFree (view);
    return true;
}

bool ppSubFilesIterateUp(pmConfig *config, ppSubFiles files)
{
    psAssert(config, "Require configuration");

    ppSubFilesActivate(config, PPSUB_FILES_ALL, false);
    ppSubFilesActivate(config, files, true);

    pmFPAview *view = ppSubViewReadout(); // View to readout

    // Readout
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psFree (view);
        return false;
    }

    // Cell
    view->readout = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psFree (view);
        return false;
    }

    // Chip
    view->cell = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psFree (view);
        return false;
    }

    // FPA
    view->chip = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psFree (view);
        return false;
    }

    psFree(view);

    // Disable writing of data now that we've written things out
    if (files & PPSUB_FILES_CONV) {
        pmFPAview *view = ppSubViewReadout(); // View to readout
        {
            pmReadout *ro = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV");
            if (ro) {
                ro->data_exists = ro->parent->data_exists = ro->parent->parent->data_exists = false;
            }
        }
        {
            pmReadout *ro = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV");
            if (ro) {
                ro->data_exists = ro->parent->data_exists = ro->parent->parent->data_exists = false;
            }
        }
        psFree(view);
    }
    if (files & PPSUB_FILES_SUB) {
        pmFPAview *view = ppSubViewReadout(); // View to readout
        pmReadout *ro = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT");
        if (ro) {
            ro->data_exists = ro->parent->data_exists = ro->parent->parent->data_exists = false;
        }
        psFree(view);
    }
    // Disable writing of data now that we've written things out
    if (files & PPSUB_FILES_INV) {
        pmFPAview *view = ppSubViewReadout(); // View to readout
        pmReadout *ro = pmFPAfileThisReadout(config->files, view, "PPSUB.INVERSE");
        if (ro) {
            ro->data_exists = ro->parent->data_exists = ro->parent->parent->data_exists = false;
        }
        psFree(view);
    }

    ppSubFilesActivate(config, PPSUB_FILES_ALL, false);

    psFree (view);
    return true;
}

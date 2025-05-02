/** @file pswarpFiles.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

void pswarpFileActivation(pmConfig *config, char **files, bool state)
{
    for (int i = 0; files[i] != NULL; i++) {
        pmFPAfileActivate(config->files, state, files[i]);
    }
    return;
}

bool pswarpIOChecksBefore(pmConfig *config)
{
    pmFPAview *view = pmFPAviewAlloc(0); // View for checking
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psFree(view);
        return false;
    }
    view->chip = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psFree(view);
        return false;
    }
    view->cell = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psFree(view);
        return false;
    }
    psFree(view);
    return true;
}

bool pswarpIOChecksAfter(pmConfig *config)
{
    pmFPAview *view = pmFPAviewAlloc(0); // View for checking
    view->chip = view->cell = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psFree(view);
        return false;
    }
    view->cell = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psFree(view);
        return false;
    }
    view->chip = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psFree(view);
        return false;
    }
    psFree(view);
    return true;
}


/** @file  pmSourceSatstar.c
 *
 *  Functions to manage saturated star profiles
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:31:25 $
 *
 *  Copyright 2012 University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "pmSourceSatstar.h"

// pmSourceSatstar defines the profile
static void pmSourceSatstarFree(pmSourceSatstar *satstar)
{
    if (!satstar) return;
    psFree(satstar->logRmodel);
    psFree(satstar->logFmodel);
}

pmSourceSatstar *pmSourceSatstarAlloc()
{
    pmSourceSatstar *satstar = (pmSourceSatstar *)psAlloc(sizeof(pmSourceSatstar));
    psMemSetDeallocator(satstar, (psFreeFunc) pmSourceSatstarFree);

    satstar->logRmodel = NULL;
    satstar->logFmodel = NULL;

    satstar->Xo = NAN;
    satstar->Yo = NAN;
    satstar->Rmax = NAN;

    return satstar;
}

bool psMemCheckSourceSatstar(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceSatstarFree);
}

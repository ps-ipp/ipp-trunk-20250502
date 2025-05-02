/** @file  pmMoments.c
 *
 *  Functions defining the pmMoments structure
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-03 20:59:16 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include "pmMoments.h"

/******************************************************************************
pmMomentsAlloc(): Allocate the pmMoments structure and initialize the members
to zero.
*****************************************************************************/
pmMoments *pmMomentsAlloc(void)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    pmMoments *tmp = (pmMoments *) psAlloc(sizeof(pmMoments));

    tmp->Mrf = NAN;
    tmp->Mrh = NAN;

    tmp->KronCore = NAN;
    tmp->KronCoreErr = NAN;

    tmp->KronFlux = NAN;
    tmp->KronFluxErr = NAN;

    tmp->KronFinner = NAN;
    tmp->KronFouter = NAN;

    tmp->KronFluxPSF = NAN;
    tmp->KronFluxPSFErr = NAN;
    tmp->KronRadiusPSF = NAN;

    tmp->Mx = NAN;
    tmp->My = NAN;

    tmp->Mxx = NAN;
    tmp->Mxy = NAN;
    tmp->Myy = NAN;

    tmp->Mxxx = NAN;
    tmp->Mxxy = NAN;
    tmp->Mxyy = NAN;
    tmp->Myyy = NAN;

    tmp->Mxxxx = NAN;
    tmp->Mxxxy = NAN;
    tmp->Mxxyy = NAN;
    tmp->Mxyyy = NAN;
    tmp->Myyyy = NAN;

    tmp->Sum = NAN;
    tmp->Peak = NAN;
    tmp->Sky = NAN;
    tmp->nPixels = 0;
    tmp->SN = 0;

    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmp);
}

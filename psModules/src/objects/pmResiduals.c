/** @file pmResiduals.c
 *
 * Functions to manipulate the residual tables (data - model).
 *
 * @author EAM, IfA
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:25 $
 * Copyright 2004 IfA, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "pmResiduals.h"

static void pmResidualsFree (pmResiduals *resid) {

    if (resid == NULL) return;

    psFree (resid->Ro);
    psFree (resid->Rx);
    psFree (resid->Ry);
    psFree (resid->variance);
    psFree (resid->mask);
    return;
}

pmResiduals *pmResidualsAlloc (int xSize, int ySize, int xBin, int yBin) {

    pmResiduals *resid = (pmResiduals *) psAlloc(sizeof(pmResiduals));
    psMemSetDeallocator(resid, (psFreeFunc) pmResidualsFree);

    int nX = xSize * xBin;
    int nY = ySize * yBin;

    nX = (nX % 2) ? nX : nX + 1;
    nY = (nY % 2) ? nY : nY + 1;

    resid->Ro  = psImageAlloc (nX, nY, PS_TYPE_F32);
    resid->Rx  = psImageAlloc (nX, nY, PS_TYPE_F32);
    resid->Ry  = psImageAlloc (nX, nY, PS_TYPE_F32);
    resid->variance = psImageAlloc (nX, nY, PS_TYPE_F32);
    resid->mask   = psImageAlloc (nX, nY, PM_TYPE_RESID_MASK);

    // NOTE : the residual mask is internal only : 1 byte is sufficient

    resid->xBin = xBin;
    resid->yBin = yBin;
    resid->xCenter = 0.5*(nX - 1);
    resid->yCenter = 0.5*(nY - 1);
    return resid;
}

bool psMemCheckResiduals(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmResidualsFree);
}


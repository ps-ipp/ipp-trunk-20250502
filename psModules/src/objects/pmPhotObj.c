/** @file  pmPhotObj.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-09-15 09:49:01 $
 * Copyright 2009 Institute for Astronomy, University of Hawaii
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

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"

#include "pmPhotObj.h"


static void pmPhotObjFree (pmPhotObj *tmp)
{
    if (!tmp) return;
    psFree(tmp->sources);
}


pmPhotObj *pmPhotObjAlloc() 
{
    static int id = 1;
    pmPhotObj *obj = (pmPhotObj *) psAlloc(sizeof(pmPhotObj));
    psMemSetDeallocator(obj, (psFreeFunc) pmPhotObjFree);

    *(int *)&obj->id = id; // cast away the const to set the id.
    id++;

    obj->sources = NULL;
    return obj;
}

bool pmPhotObjAddSource(pmPhotObj *object, pmSource *source) {

    psAssert (source, "programming error: NULL source");
    if (!source->peak) {
	psError(PS_ERR_UNKNOWN, true, "source missing peak");
	return false; 
    }
    if (!isfinite(source->peak->xf)) {
	psError(PS_ERR_UNKNOWN, true, "NAN peak coordinate");
	return false; 
    }
    if (!isfinite(source->peak->yf)) {
	psError(PS_ERR_UNKNOWN, true, "NAN peak coordinate");
	return false; 
    }

    // XXX we should probably use the fitted position if it exists
    if (!object->sources) {
	object->sources = psArrayAllocEmpty(1);
	object->x  = source->peak->xf;
	object->y  = source->peak->yf;
	object->flux = source->peak->rawFlux;
    } else {
	object->flux = PS_MAX(object->flux, source->peak->rawFlux);
    }
    psArrayAdd (object->sources, 1, source);
    return true;
}

// sort by flux (descending)
int pmPhotObjSortByFlux (const void **a, const void **b)
{
    pmPhotObj *objA = *(pmPhotObj **)a;
    pmPhotObj *objB = *(pmPhotObj **)b;

    psF32 fA = objA->flux;
    psF32 fB = objB->flux;

    psF32 diff = fA - fB;
    if (diff > FLT_EPSILON) return (-1);
    if (diff < FLT_EPSILON) return (+1);
    return (0);
}

// sort by X (ascending)
int pmPhotObjSortByX (const void **a, const void **b)
{
    pmPhotObj *objA = *(pmPhotObj **)a;
    pmPhotObj *objB = *(pmPhotObj **)b;

    psF32 fA = objA->x;
    psF32 fB = objB->x;

    psF32 diff = fA - fB;
    if (diff > FLT_EPSILON) return (+1);
    if (diff < FLT_EPSILON) return (-1);
    return (0);
}

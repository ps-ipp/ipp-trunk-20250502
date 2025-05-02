#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <pslib.h>
#include "ppMops.h"

static void mopsDetectionsFree(ppMopsDetections *det)
{
    psFree(det->component);
    psFree(det->raBoresight);
    psFree(det->decBoresight);
    psFree(det->filter);
    psFree(det->table);
    psFree(det->x);
    psFree(det->y);
    psFree(det->ra);
    psFree(det->dec);
    psFree(det->raErr);
    psFree(det->decErr);
    psFree(det->mask);
    psFree(det->fpashutoutc);
    psFree(det->fpashutcutc);
    psFree(det->fpashmdoutc);
    psFree(det->fpashmdcutc);
    return;
}

ppMopsDetections *ppMopsDetectionsAlloc(void)
{
    ppMopsDetections *det = psAlloc(sizeof(ppMopsDetections)); // Detections, to return
    psMemSetDeallocator(det, (psFreeFunc)mopsDetectionsFree);
    det->component = NULL;
    det->raBoresight = NULL;
    det->decBoresight = NULL;
    det->filter = NULL;
    det->airmass = NAN;
    det->exptime = NAN;
    det->posangle = NAN;
    det->alt = NAN;
    det->az = NAN;
    det->mjd = NAN;
    det->seeing = NAN;
    det->num = 0;
    det->table = NULL;
    det->x = NULL;
    det->y = NULL;
    det->ra = NULL;
    det->dec = NULL;
    det->raErr = NULL;
    det->decErr = NULL;
    det->mask = NULL;
    det->diffSkyfileId = 0;
    det->fpashutoutc = NULL;
    det->fpashutcutc = NULL;
    det->fpashmdoutc = NULL;
    det->fpashmdcutc = NULL;
    return det;
}

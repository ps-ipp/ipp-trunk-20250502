/* @file  pmDetections.c
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-01 00:00:17 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>
#include "pmDetections.h"

void pmDetectionsFree (pmDetections *detections) {

  if (!detections) return;

  psFree (detections->footprints);
  psFree (detections->peaks);
  psFree (detections->oldPeaks);
  psFree (detections->oldFootprints);

  psFree (detections->newSources);
  psFree (detections->allSources);
  return;
}

// generate a pmDetections container with empty (allocated) footprints and peaks containers
pmDetections *pmDetectionsAlloc(void) {

    pmDetections *detections = (pmDetections *)psAlloc(sizeof(pmDetections));
    psMemSetDeallocator(detections, (psFreeFunc) pmDetectionsFree);

    detections->footprints    = NULL;
    detections->peaks         = NULL;
    detections->oldPeaks      = NULL;
    detections->oldFootprints = NULL;
    detections->newSources    = NULL;
    detections->allSources    = NULL;
    detections->last          = 0;

    return (detections);
}


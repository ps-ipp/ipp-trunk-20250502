/* @file  pmFootprint.c
 * low-level pmFootprint functions
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-08 02:51:14 $
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
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"

static void footprintFree(pmFootprint *tmp)
{
   if (!tmp) {
        return;
   }

   psTrace("psModules.objects", 10, "---- begin ----\n");

   psFree(tmp->spans);
   psFree(tmp->peaks);

   psTrace("psModules.objects", 10, "---- end ----\n");
}

/*
 * pmFootprintAlloc(): Allocate the pmFootprint structure to NULL.
 */
pmFootprint *pmFootprintAlloc(int nspan, // number of spans expected in footprint
			      const psImage *image) // region footprint lives in
{
    psTrace("psModules.objects", 10, "---- begin ----\n");

    static int id = 1;
    pmFootprint *footprint = (pmFootprint *)psAlloc(sizeof(pmFootprint));
    *(int *)&footprint->id = id++;
    psMemSetDeallocator(footprint, (psFreeFunc) footprintFree);

    footprint->normalized = false;

    assert(nspan >= 0);
    footprint->npix = 0;
    footprint->nspans = 0; // we may allocate more spans than we set -- this is the number of active spans
    footprint->spans = psArrayAllocEmpty(nspan);
    footprint->peaks = psArrayAlloc(0);

    footprint->bbox.x0 = footprint->bbox.y0 = 0;
    footprint->bbox.x1 = footprint->bbox.y1 = -1;

    if (image == NULL) {
	footprint->region.x0 = footprint->region.y0 = 0;
	footprint->region.x1 = footprint->region.y1 = -1;
    } else {
	footprint->region.x0 = image->col0;
	footprint->region.x1 = image->col0 + image->numCols - 1;
	footprint->region.y0 = image->row0;
	footprint->region.y1 = image->row0 + image->numRows - 1;
    }

    psTrace("psModules.objects", 10, "---- end ----\n");
    return(footprint);
}

bool pmFootprintAllocEmptySpans (pmFootprint *footprint, int nSpans) {

    psArrayRealloc (footprint->spans, nSpans);
    for (int i = 0; i < nSpans; i++) {
	footprint->spans->data[i] = pmSpanAlloc(-1, -1, -1);
    }
    footprint->spans->n = nSpans;
    return true;
}

// reset the footprint containers
bool pmFootprintInit(pmFootprint *footprint) {

    footprint->bbox.x0 = footprint->bbox.y0 = 0;
    footprint->bbox.x1 = footprint->bbox.y1 = -1;
    footprint->nspans = 0;
    return true;
}

bool pmFootprintTest(const psPtr ptr) {
    return (psMemGetDeallocator(ptr) == (psFreeFunc)footprintFree);
}

// XXX not actually used anywhere
pmFootprint *pmFootprintNormalize(pmFootprint *fp) {
    if (fp != NULL && !fp->normalized) {
	if (PM_PEAKS_CULL_WITH_SMOOTHED_IMAGE) {
	    fp->peaks = psArraySort(fp->peaks, pmPeaksSortBySmoothFluxDescend);
	} else {
	    fp->peaks = psArraySort(fp->peaks, pmPeaksSortByRawFluxDescend);
	}
	fp->normalized = true;
    }

    return fp;
}

//
// Add a span to a footprint, returning the new span
//
pmSpan *pmFootprintAddSpan(pmFootprint *fp,	// the footprint to add to
			   const int y, // row to add
			   int x0,      // range of
			   int x1) {    //          columns

    if (x1 < x0) {
	int tmp = x0;
	x0 = x1;
	x1 = tmp;
    }

    pmSpan *sp = pmSpanAlloc(y, x0, x1);
    psArrayAdd(fp->spans, 1, sp);
    psFree(sp);

    fp->nspans ++;

    fp->npix += x1 - x0 + 1;

    if (fp->nspans == 1) {
	fp->bbox.x0 = x0;
	fp->bbox.x1 = x1;
	fp->bbox.y0 = y;
	fp->bbox.y1 = y;
    } else {
	if (x0 < fp->bbox.x0) fp->bbox.x0 = x0;
	if (x1 > fp->bbox.x1) fp->bbox.x1 = x1;
	if (y < fp->bbox.y0) fp->bbox.y0 = y;
	if (y > fp->bbox.y1) fp->bbox.y1 = y;
    }

    return sp;
}


// Set the next available elements of the nSpan entry in footprint->spans
pmSpan *pmFootprintSetSpan(pmFootprint *fp,	// the footprint to add to
			   const int y,		// row to add
			   int x0,		// range of
			   int x1) {		// columns

    if (x1 < x0) {
	int tmp = x0;
	x0 = x1;
	x1 = tmp;
    }

    int N = fp->nspans;
    if (N == fp->spans->n) {
	// if we need more space, extend fp->spans as needed
	int Nalloc = fp->spans->n + 100;
	psArrayRealloc(fp->spans, Nalloc);
	fp->spans->n = Nalloc;
	for (int i = N; i < fp->spans->n; i++) {
	    fp->spans->data[i] = pmSpanAlloc(-1, -1, -1);
	}
    }

    pmSpan *span = fp->spans->data[N];
    span->y = y;
    span->x0 = x0;
    span->x1 = x1;

    fp->nspans ++;

    fp->npix += x1 - x0 + 1;

    if (fp->nspans == 1) {
	fp->bbox.x0 = x0;
	fp->bbox.x1 = x1;
	fp->bbox.y0 = y;
	fp->bbox.y1 = y;
    } else {
	if (x0 < fp->bbox.x0) fp->bbox.x0 = x0;
	if (x1 > fp->bbox.x1) fp->bbox.x1 = x1;
	if (y < fp->bbox.y0) fp->bbox.y0 = y;
	if (y > fp->bbox.y1) fp->bbox.y1 = y;
    }

    return span;
}

void pmFootprintSetBBox(pmFootprint *fp) {
    assert (fp != NULL);
    if (fp->nspans == 0) {
	return;
    }
    pmSpan *sp = fp->spans->data[0];
    int x0 = sp->x0;
    int x1 = sp->x1;
    int y0 = sp->y;
    int y1 = sp->y;

    for (int i = 1; i < fp->nspans; i++) {
	sp = fp->spans->data[i];
	
	if (sp->x0 < x0) x0 = sp->x0;
	if (sp->x1 > x1) x1 = sp->x1;
	if (sp->y < y0) y0 = sp->y;
	if (sp->y > y1) y1 = sp->y;
    }

    fp->bbox.x0 = x0;
    fp->bbox.x1 = x1;
    fp->bbox.y0 = y0;
    fp->bbox.y1 = y1;
}

int pmFootprintSetNpix(pmFootprint *fp) {
   assert (fp != NULL);
   int npix = 0;
   for (int i = 0; i < fp->nspans; i++) {
       pmSpan *span = fp->spans->data[i];
       npix += span->x1 - span->x0 + 1;
   }
   fp->npix = npix;

   return npix;
}

/*
 * Extract the peaks in a psArray of pmFootprints, returning a psArray of pmPeaks
 */
psArray *pmFootprintArrayToPeaks(const psArray *footprints) {
   assert(footprints != NULL);
   assert(footprints->n == 0 || pmFootprintTest(footprints->data[0]));

   int npeak = 0;
   for (int i = 0; i < footprints->n; i++) {
      const pmFootprint *fp = footprints->data[i];
      npeak += fp->peaks->n;
   }

   psArray *peaks = psArrayAllocEmpty(npeak);
   
   for (int i = 0; i < footprints->n; i++) {
      const pmFootprint *fp = footprints->data[i];
      for (int j = 0; j < fp->peaks->n; j++) {
	 psArrayAdd(peaks, 1, fp->peaks->data[j]);
      }
   }

   return peaks;
}

// create a new footprint with the same span set as the input footprint
pmFootprint *pmFootprintCopyData(pmFootprint *inFoot, psImage *image) {

    pmFootprint *outFoot = pmFootprintAlloc(inFoot->nspans, image);
    for (int i = 0; i < inFoot->nspans; i++) {
	pmSpan *span = inFoot->spans->data[i];
	pmFootprintAddSpan(outFoot, span->y, span->x0, span->x1);
    }
    return outFoot;
}

/************************************************************************************************************/

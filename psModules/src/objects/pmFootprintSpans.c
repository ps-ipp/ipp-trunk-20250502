/* @file  pmFootprintFindAtPoint.c
 * find footprints in a small image relative to a reference point
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
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

static void pmStartSpanFree(pmStartSpan *sspan) {
    return;
}

// Allocate an un-assigned pmStartSpan
pmStartSpan *pmStartSpanAlloc() {
    pmStartSpan *sspan = psAlloc(sizeof(pmStartSpan));
    psMemSetDeallocator(sspan, (psFreeFunc)pmStartSpanFree);
    
    sspan->span = NULL;
    sspan->direction = PM_STARTSPAN_NONE;
    sspan->stop = false;
    
    return sspan;
}

// Assign a pmSpan to this pmStartSpan
bool pmStartSpanSet(pmStartSpan *sspan,
		    pmSpan *span,      // The span in question
		    psImage *mask,           // Pixels that we've already detected
		    const PM_STARTSPAN_DIR dir   // Should we continue searching towards the top of the image?
    ) {
    sspan->span = span; // view on the span (we do not own this memory)
    sspan->direction = dir;
    sspan->stop = false;
    
    if (mask) {                 // remember that we've detected these pixels
        psImageMaskType *mpix = &mask->data.PS_TYPE_IMAGE_MASK_DATA[span->y - mask->row0][span->x0 - mask->col0];

        for (int i = 0; i <= span->x1 - span->x0; i++) {
            mpix[i] |= PM_STARTSPAN_DETECTED;
            if (mpix[i] & PM_STARTSPAN_STOP) {
                sspan->stop = true;
            }
        }
    }

    return true;
}

// remove a span assignment
bool pmStartSpanUnset(pmStartSpan *sspan) {
    if (!sspan) return false;
    sspan->span = NULL;
    sspan->direction = PM_STARTSPAN_NONE;
    sspan->stop = false;
    return true;
}

static void pmFootprintSpansFree(pmFootprintSpans *fp) {
    psFree(fp->startspans);
    return;
}

// Allocate a pmFootprintSpans structure with unassigned spans
pmFootprintSpans *pmFootprintSpansAlloc(int nSpans) {
    pmFootprintSpans *fpSpans = psAlloc(sizeof(pmFootprintSpans));
    psMemSetDeallocator(fpSpans, (psFreeFunc)pmFootprintSpansFree);
    
    // create an footprint with allocated pmStartSpans, but none yet assigned
    fpSpans->nStartSpans = 0;
    fpSpans->startspans = psArrayAlloc(nSpans);

    for (int i = 0; i < nSpans; i++) {
	fpSpans->startspans->data[i] = pmStartSpanAlloc();
    }
    return fpSpans;
}

// Allocate a pmFootprintSpans structure with unassigned spans
bool pmFootprintSpansInit(pmFootprintSpans *fpSpans) {
    fpSpans->nStartSpans = 0;
    for (int i = 0; i < fpSpans->startspans->n; i++) {
	pmStartSpanUnset(fpSpans->startspans->data[i]);
    }
    return true;
}

// Add a new pmSpan to a pmFootprintSpans.  Iff we see a stop bit, return true
bool pmFootprintSpansSet(pmFootprintSpans *fpSpans, // the saved pmStartSpans
			 pmSpan *sp, // the span in question
			 psImage *mask, // mask of detected/stop pixels
			 const PM_STARTSPAN_DIR dir) { // the desired direction to search
    if (dir == PM_STARTSPAN_RESTART) {
        if (pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_UP)) return true;
	if (pmFootprintSpansSet(fpSpans, sp, NULL, PM_STARTSPAN_DOWN)) return true;
    } else {
	int N = fpSpans->nStartSpans;
	if (N == fpSpans->startspans->n) {
	    // if we need more space, extend fpSpans->startspans as needed
	    int Nalloc = fpSpans->startspans->n + 100;
	    psArrayRealloc(fpSpans->startspans, Nalloc);
	    fpSpans->startspans->n = Nalloc;
	    for (int i = N; i < fpSpans->startspans->n; i++) {
		fpSpans->startspans->data[i] = pmStartSpanAlloc();
	    }
	}
	pmStartSpan *startspan = fpSpans->startspans->data[N];
	
	pmStartSpanSet (startspan, sp, mask, dir);

        if (startspan->stop) {              // we detected a stop bit
            pmStartSpanUnset(startspan);    // don't allocate new span
            return true;
        } else {
	    fpSpans->nStartSpans ++;
	    return false;
        }
    }
    return false;
}


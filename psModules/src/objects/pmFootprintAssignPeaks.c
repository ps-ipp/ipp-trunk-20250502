/* @file  pmFootprintAssignPeaks.c
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

/*
 * Given a psArray of pmFootprints and another of pmPeaks, assign the peaks to the
 * footprints in which they fall; if they _don't_ fall in a footprint, add a suitable
 * one to the list.
 */
psErrorCode
pmFootprintsAssignPeaks(psArray *footprints,	// the pmFootprints
			const psArray *peaks) { // the pmPeaks
    assert (footprints != NULL);
    assert (footprints->n == 0 || pmFootprintTest(footprints->data[0]));
    assert (peaks != NULL);
    assert (peaks->n == 0 || psMemCheckPeak(peaks->data[0]));
    
    if ((footprints->n == 0) && (peaks->n == 0)) {
	return PS_ERR_NONE;
    }

    /*
     * Create an image filled with the object IDs, and use it to assign pmPeaks to the
     * objects
     */
    psImage *ids = pmSetFootprintArrayIDs(footprints, true);
    if (ids) { assert (ids->type.type == PS_TYPE_S32); }
    
    const int row0 = ids ? ids->row0 : 0;
    const int col0 = ids ? ids->col0 : 0;
    const int numRows = ids ? ids->numRows : -1;
    const int numCols = ids ? ids->numCols : -1;

    for (int i = 0; i < peaks->n; i++) {
	pmPeak *peak = peaks->data[i];
	const int x = peak->x - col0;
	const int y = peak->y - row0;
	
	if (ids) { psAssert (x >= 0 && x < numCols && y >= 0 && y < numRows, "out of range");}
	int id = ids ? ids->data.S32[y][x] : 0;

	if (id == 0) {			// peak isn't in a footprint, so make one for it
	    pmFootprint *nfp = pmFootprintAlloc(1, ids);
	    pmFootprintAddSpan(nfp, y, x, x);
	    psArrayAdd(footprints, 1, nfp);
	    psFree(nfp);
	    id = footprints->n;
	}

	assert (id >= 1 && id <= footprints->n);
	pmFootprint *fp = footprints->data[id - 1];
	psArrayAdd(fp->peaks, 5, peak);
	peak->footprint = fp; // reference to containing footprint
    }
    
    psFree(ids);

    // Make sure that peaks within each footprint are sorted and unique
    for (int i = 0; i < footprints->n; i++) {
	
	pmFootprint *fp = footprints->data[i];

	// XXX are we allowed to have peak-less footprints??
	if (fp->peaks->n == 0) continue;
	if (fp->peaks->n == 1) continue;

	// make sure the peaks are sorted in a way consistent with our cull process
	if (PM_PEAKS_CULL_WITH_SMOOTHED_IMAGE) {
	    fp->peaks = psArraySort(fp->peaks, pmPeaksSortBySmoothFluxDescend);
	} else {
	    fp->peaks = psArraySort(fp->peaks, pmPeaksSortByRawFluxDescend);
	}

	// XXX check for an assert on duplicates (I don't think they can happen, but
	// let's double check for now)

	for (int j = 1; j < fp->peaks->n; j++) {
	    psAssert (fp->peaks->data[j] != fp->peaks->data[j-1], "duplicate peak!");
	}
	continue;

	// XXX WHY am I culling duplicates?  how can there be duplicates?
	// XXX EAM : the algorithm below should be much faster than using psArrayRemove if
	// the number of peaks in the footprint is large, or if there are no duplicates.
	// if we have a lot of small-number peak arrays with duplicates, this may be
	// slower.

	// track the number of good peaks in the footprint
	int lastGood = 0;

	// check for duplicates
	// on first pass, we set the index to NULL if peak is a duplicate
	// XXX EAM : this can leave behind duplicates of the same S/N
	// (if sorted list has A, B, A, B ...)
	for (int j = 1; j < fp->peaks->n; j++) { 
	    if (fp->peaks->data[j] == fp->peaks->data[lastGood]) {
		// everything on the array has its own mem reference; free and drop this one
		psFree (fp->peaks->data[j]);
		fp->peaks->data[j] = NULL;
	    } else {
		lastGood ++;
	    }
	}
	int nGood = lastGood + 1;

	// no deleted peaks, go to next footprint
	if (nGood == fp->peaks->n) continue;

	int nKeep = 0;

	psArray *goodPeaks = psArrayAlloc (nGood);
	// on second pass, save the good peaks
	for (int j = 0; j < fp->peaks->n; j++) { // check for duplicates
	    if (fp->peaks->data[j] == NULL) continue;
	    // transfer the data (NULL to avoid double free)
	    // this is only slightly sleazy
	    goodPeaks->data[nKeep] = fp->peaks->data[j];
	    fp->peaks->data[j] = NULL;
	    nKeep ++;
	}
	psAssert (nGood == nKeep, "mis-counted nKeep or nGood");

	// free the old (now NULL-filled) array
	psFree (fp->peaks);
	fp->peaks = goodPeaks;
    }

    // (void)psArrayRemoveIndex(fp->peaks, j);


    return PS_ERR_NONE;
}


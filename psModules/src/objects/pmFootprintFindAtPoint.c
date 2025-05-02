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

/************************************************************************************************************/
/*
 * Search the image for pixels above threshold, starting at a single pmStartSpan.
 * We search the array looking for one to process; it'd be better to move the
 * ones that we're done with to the end, but it probably isn't worth it for
 * the anticipated uses of this routine.
 *
 * This is the guts of pmFootprintsFindAtPoint
 * 
 * This function is/was ill-defined if pixel values are NAN.  we should either treat NAN as >
 * threshold or < threshold, but the current (r29004) code is ambiguous.
 * EAM : change code so NAN is always > threshold
 */
bool pmFootprintSpansBuild(pmFootprint *fp, // the footprint that we're building
			   pmFootprintSpans *fpSpans,
			   const psImage *img, // the psImage we're working on
			   psImage *mask, // the associated masks
			   const float threshold // Threshold
    ) {
    bool F32 = false;                   // is this an F32 image?
    if (img->type.type == PS_TYPE_F32) {
        F32 = true;
    } else if (img->type.type == PS_TYPE_S32) {
        F32 = false;
    } else {                            // N.b. You can't trivially add more cases here; F32 is just a bool
        psError(PS_ERR_UNKNOWN, true, "Unsupported psImage type: %d", img->type.type);
        return NULL;
    }

    psF32 *imgRowF32 = NULL;            // row pointer if F32
    psS32 *imgRowS32 = NULL;            //  "   "   "  "  !F32
    psImageMaskType *maskRow = NULL;            //  masks's row pointer

    const int row0 = img->row0;
    const int col0 = img->col0;
    const int numRows = img->numRows;
    const int numCols = img->numCols;

    /********************************************************************************************************/

    pmStartSpan *startspan = NULL;
    for (int i = 0; i < fpSpans->nStartSpans; i++) {
        startspan = fpSpans->startspans->data[i];
        if (startspan->direction != PM_STARTSPAN_DONE) {
            break;
        }
        if (startspan->stop) {
            break;
        }
    }
    if (startspan == NULL || startspan->direction == PM_STARTSPAN_DONE) { // no more pmStartSpans to process
        return false;
    }
    if (startspan->stop) {                  // they don't want any more spans processed
        return false;
    }

    /*
     * Work
     */
    const PM_STARTSPAN_DIR dir = startspan->direction;
    /*
     * Set initial span to the startspan
     */
    int x0 = startspan->span->x0 - col0, x1 = startspan->span->x1 - col0;
    /*
     * Go through image identifying objects
     */
    int nx0, nx1 = -1;                  // new values of x0, x1
    const int di = (dir == PM_STARTSPAN_UP) ? 1 : -1; // how much i changes to get to the next row
    bool stop = false;                  // should I stop searching for spans?

    for (int i = startspan->span->y - row0 + di; i < numRows && i >= 0; i += di) {
        imgRowF32 = img->data.F32[i];   // only one of
        imgRowS32 = img->data.S32[i];   //      these is valid!
        maskRow = mask->data.PS_TYPE_IMAGE_MASK_DATA[i];
        //
        // Search left from the pixel diagonally to the left of (i - di, x0). If there's
        // a connected span there it may need to grow up and/or down, so push it onto
        // the stack for later consideration
        //
        nx0 = -1;
        for (int j = x0 - 1; j >= -1; j--) {
            double pixVal = (j < 0) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
	    bool belowThreshold = (pixVal < threshold) && isfinite(pixVal);
            if ((maskRow[j] & PM_STARTSPAN_DETECTED) || belowThreshold)  {
                if (j < x0 - 1) {       // we found some pixels above threshold
                    nx0 = j + 1;
                }
                break;
            }
        }

        if (nx0 < 0) {                  // no span to the left
            nx1 = x0 - 1;               // we're going to resume searching at nx1 + 1
        } else {
            //
            // Search right in leftmost span
            //
            for (int j = nx0 + 1; j <= numCols; j++) {
                double pixVal = (j >= numCols) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
		bool belowThreshold = (pixVal < threshold) && isfinite(pixVal);
                if ((maskRow[j] & PM_STARTSPAN_DETECTED) || belowThreshold) {
                    nx1 = j - 1;
                    break;
                }
            }

	    pmSpan *sp = pmFootprintSetSpan(fp, i + row0, nx0 + col0, nx1 + col0);
	    bool status = pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_RESTART);
	    // fprintf (stderr, "set 1: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);
            if (status) {
                stop = true;
                break;
            }
        }
        //
        // Now look for spans connected to the old span.  The first of these we'll
        // simply process, but others will have to be deferred for later consideration.
        //
        // In fact, if the span overhangs to the right we'll have to defer the overhang
        // until later too, as it too can grow in both directions
        //
        // Note that column numCols exists virtually, and always ends the last span; this
        // is why we claim below that sx1 is always set
        //
        bool first = false;             // is this the first new span detected?
        for (int j = nx1 + 1; j <= x1 + 1; j++) {
            double pixVal = (j >= numCols) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
	    bool aboveThreshold = (pixVal >= threshold) || !isfinite(pixVal);
            if (!(maskRow[j] & PM_STARTSPAN_DETECTED) && aboveThreshold) {
                int sx0 = j++;          // span that we're working on is sx0:sx1
                int sx1 = -1;           // We know that if we got here, we'll also set sx1
                for (; j <= numCols; j++) {
                    double pixVal = (j >= numCols) ? threshold - 100 : (F32 ? imgRowF32[j] : imgRowS32[j]);
		    bool belowThreshold = (pixVal < threshold) && isfinite(pixVal);
                    if ((maskRow[j] & PM_STARTSPAN_DETECTED) || belowThreshold) { // end of span
                        sx1 = j;
                        break;
                    }
                }
                assert (sx1 >= 0);

                pmSpan *sp;
                if (first) {
                    if (sx1 <= x1) {
                        sp = pmFootprintSetSpan(fp, i + row0, sx0 + col0, sx1 + col0 - 1);
                        bool status = pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_DONE);
			// fprintf (stderr, "set 2: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);
                        if (status) {
                            stop = true;
                            break;
                        }
                    } else {            // overhangs to right
                        sp = pmFootprintSetSpan(fp, i + row0, sx0 + col0, x1 + col0);
                        bool status = pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_DONE);
			// fprintf (stderr, "set 3: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);
			if (status) {
                            stop = true;
                            break;
                        }
                        sp = pmFootprintSetSpan(fp, i + row0, x1 + 1 + col0, sx1 + col0 - 1);
                        status = pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_RESTART);
			// fprintf (stderr, "set 4: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);
			if (status) {
                            stop = true;
                            break;
                        }
                    }
                    first = false;
                } else {
                    sp = pmFootprintSetSpan(fp, i + row0, sx0 + col0, sx1 + col0 - 1);
                    bool status = pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_RESTART);
		    // fprintf (stderr, "set 5: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);
		    if (status) {
                        stop = true;
                        break;
                    }
                }
            }
        }

        if (stop || first == false) {   // we're done
            break;
        }

        x0 = nx0; x1 = nx1;
    }
    /*
     * Cleanup
     */

    startspan->direction = PM_STARTSPAN_DONE;
    return stop ? false : true;
}

/*
 * Go through an image, starting at (row, col) and assembling all the pixels
 * that are connected to that point (in a chess kings-move sort of way) into
 * a pmFootprint.
 *
 * This is much slower than pmFootprintsFind if you want to find lots of
 * footprints, but if you only want a small region about a given point it
 * can be much faster
 *
 * N.b. The returned pmFootprint is not in "normal form"; that is the pmSpans
 * are not sorted by increasing y, x0, x1.  If this matters to you, call
 * pmFootprintNormalize()
 * 
 * The calling function must supply a footprint allocated with a reasonable amount of space
 *
 */

bool pmFootprintsFindAtPoint(pmFootprint *fp,
			     pmFootprintSpans *fpSpans,
			     const psImage *img,     // image to search
			     psImage *mask,
			     const float threshold,   // Threshold
			     const psArray *peaks, // array of peaks; finding one terminates search for footprint
			     int row, int col) { // starting position (in img's parent's coordinate system)
    psAssert(img, "image must be supplied");
    psAssert(fp, "footprint must be supplied");
    psAssert(fpSpans, "footprint spans must be supplied");

    bool F32 = false;                    // is this an F32 image?
    if (img->type.type == PS_TYPE_F32) {
	F32 = true;
    } else if (img->type.type == PS_TYPE_S32) {
	F32 = false;
    } else {                             // N.b. You can't trivially add more cases here; F32 is just a bool
	psError(PS_ERR_UNKNOWN, true, "Unsupported psImage type: %d", img->type.type);
	return false;
    }
    psF32 *imgRowF32 = NULL;             // row pointer if F32
    psS32 *imgRowS32 = NULL;             //  "   "   "  "  !F32

    const int row0 = img->row0;
    const int col0 = img->col0;
    const int numRows = img->numRows;
    const int numCols = img->numCols;

    /*
     * Is point in image, and above threshold?
     */
    row -= row0; col -= col0;
    if (row < 0 || row >= numRows ||
	col < 0 || col >= numCols) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "row/col == (%d, %d) are out of bounds [%d--%d, %d--%d]",
                row + row0, col + col0, row0, row0 + numRows - 1, col0, col0 + numCols - 1);
	return false;
    }

    double pixVal = F32 ? img->data.F32[row][col] : img->data.S32[row][col];
    if (pixVal < threshold) {
	return true;
    }

    /*
     * We need a mask for two purposes; to indicate which pixels are already detected,
     * and to store the "stop" pixels --- those that, once reached, should stop us
     * looking for the rest of the pmFootprint.  These are generally set from peaks.
     */

    pmFootprintInit(fp);
    pmFootprintSpansInit(fpSpans);
    psImageInit(mask, PM_STARTSPAN_INITIAL);

    // fprintf (stderr, "init: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);

    //
    // Set stop bits from peaks list
    //
    assert (peaks == NULL || peaks->n == 0 || psMemCheckPeak(peaks->data[0]));
    if (peaks != NULL) {
	for (int i = 0; i < peaks->n; i++) {
	    pmPeak *peak = peaks->data[i];
	    mask->data.PS_TYPE_IMAGE_MASK_DATA[peak->y - mask->row0][peak->x - mask->col0] |= PM_STARTSPAN_STOP;
	}
    }

    /*
     * Find starting span passing through (row, col)
     */
    imgRowF32 = img->data.F32[row];      // only one of
    imgRowS32 = img->data.S32[row];      //      these is valid!
    psImageMaskType *maskRow = mask->data.PS_TYPE_IMAGE_MASK_DATA[row];
    {
	int i;
	for (i = col; i >= 0; i--) {
	    pixVal = F32 ? imgRowF32[i] : imgRowS32[i];
	    bool belowThreshold = (pixVal < threshold) && isfinite(pixVal);
	    if ((maskRow[i] & PM_STARTSPAN_DETECTED) || belowThreshold) {
		break;
	    }
	}
	int i0 = i;
	for (i = col; i < numCols; i++) {
	    pixVal = F32 ? imgRowF32[i] : imgRowS32[i];
	    bool belowThreshold = (pixVal < threshold) && isfinite(pixVal);
	    if ((maskRow[i] & PM_STARTSPAN_DETECTED) || belowThreshold) {
		break;
	    }
	}
	int i1 = i;
	pmSpan *sp = pmFootprintSetSpan(fp, row + row0, i0 + col0 + 1, i1 + col0 - 1);
	pmFootprintSpansSet(fpSpans, sp, mask, PM_STARTSPAN_RESTART);
	// fprintf (stderr, "first: %d vs %d\n", fp->nspans, fpSpans->nStartSpans);
    }
    /*
     * Now workout from those pmStartSpans, searching for pixels above threshold
     */
    while (pmFootprintSpansBuild(fp, fpSpans, img, mask, threshold)) continue;
    /*
     * Cleanup
     */
    // psFree(mask);
    // psFree(startspans);                  // restores the image pixel

    return fp;                           // pmFootprint really
}

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

bool dumpfootprints (pmFootprint *fp, pmFootprintSpans *fpSp);

/*
 * Examine the peaks in a pmFootprint, and throw away the ones that are not sufficiently
 * isolated.  More precisely, for each peak find the highest coll that you'd have to traverse
 * to reach a still higher peak --- and if that coll's more (less?) than nsigma DN below your
 * starting point, discard the peak.
 */

# define IN_PEAK 1
psErrorCode pmFootprintCullPeaks(const psImage *img, // the image wherein lives the footprint
                                 const psImage *weight, // corresponding variance image
                                 pmFootprint *fp, // Footprint containing mortal peaks
                                 const float nsigma_delta, // how many sigma above local background a peak needs to be to survive
                                 const float fPadding, // fractional padding added to stdev since bright peaks have unreasonably high significance
                                 const float min_threshold, // minimum permitted coll height
                                 const float max_threshold,
				 const bool useSmoothedImage
    ) { // maximum permitted coll height
    assert (img != NULL); assert (img->type.type == PS_TYPE_F32);
    assert (weight != NULL); assert (weight->type.type == PS_TYPE_F32);
    assert (img->row0 == weight->row0 && img->col0 == weight->col0);
    assert (fp != NULL);

    if (fp->peaks == NULL || fp->peaks->n < 2) { // nothing to do
	return PS_ERR_NONE;
    }

    psRegion subRegion;                 // desired subregion; 1 larger than bounding box (grr)
    subRegion.x0 = fp->bbox.x0; subRegion.x1 = fp->bbox.x1 + 1;
    subRegion.y0 = fp->bbox.y0; subRegion.y1 = fp->bbox.y1 + 1;

    psImage *subImg = psImageSubset((psImage *)img, subRegion);
    psAssert (subImg, "trouble making local subimage");

    psImage *idImg = psImageAlloc(subImg->numCols, subImg->numRows, PS_TYPE_S32);

    // We need a psArray of peaks brighter than the current peak.
    // We reject peaks which either:
    // 1) are below the local threshold
    // 2) have a brighter peak within their threshold

    // allocate the full-sized array.  if the final array is much smaller, we can realloc
    // at that point.
    psArray *brightPeaks = psArrayAllocEmpty(fp->peaks->n);
    psArrayAdd (brightPeaks, 128, fp->peaks->data[0]);

    // XXX test point
    // pmPeak *myPeak = fp->peaks->data[0];
    // if ((fabs(myPeak->x - 205) < 100) && (fabs(myPeak->y - 806) < 100)) {
    // 	fprintf (stderr, "test peak\n");
    // }

    // allocate the peakFootprint and peakFPSpans containers -- these are re-used by pmFootprintsFindAtPoint to minimize allocs in this function
    pmFootprint *peakFootprint = pmFootprintAlloc(fp->nspans, subImg);
    pmFootprintSpans *peakFPSpans = pmFootprintSpansAlloc(2*fp->nspans);

    // allocate empty spans for the footprints
    pmFootprintAllocEmptySpans(peakFootprint, fp->nspans);

    psImage *subMask = psImageCopy(NULL, subImg, PS_TYPE_IMAGE_MASK);

    // if we have many peaks in a large footprint, we waste a lot of time generating nearly identical footprints
    // here we attempt to cull peaks drawing a single footprint for all peaks in some threshold range
    // fprintf (stderr, "footprint: %d x %d : %d pix, %d peaks\n", subImg->numCols, subImg->numRows, fp->npix, (int) fp->peaks->n);
    if ((fp->npix > 30000) && (fp->peaks->n > 10)) {

	// max flux is above threshold for brightest peak
	pmPeak *maxPeak = NULL;
	for (int i = 0; i < fp->peaks->n; i++) {
	    pmPeak *testPeak = fp->peaks->data[i];
	    float this_peak = useSmoothedImage ? testPeak->smoothFlux : testPeak->rawFlux;
	
	    if (isfinite(this_peak)) {
		maxPeak = fp->peaks->data[i];
		break;
	    }
	}
	psAssert(maxPeak,"maxPeak was not set in these peaks");
	//      = fp->peaks->data[0];
	float maxFlux = useSmoothedImage ? maxPeak->smoothFlux : maxPeak->rawFlux;

	// we have a relationship between the bin and the threshold of:
	// threshold = 0.25 beta^2 bin^2 + minThreshold
	// thus, the max bin is: sqrt(4.0*(maxThreshold - minThreshold)/ALPHA^2)

# define ALPHA 0.1
	
	float beta = nsigma_delta * ALPHA;
	float beta2 = PS_SQR(beta);
	float value = 4.0*(maxFlux - min_threshold)/beta2;
	int nBins = (value > 1.0) ? sqrt(value) + 10 : 10; // let's be extra generous

	// create a vector to store the threshold bins used for each peak
	psVector *threshbins = psVectorAlloc(fp->peaks->n, PS_TYPE_S32);
	threshbins->data.S32[0] = -1; // we skip the first peak

	/// create a vector to track if a peak has been tried or not:
	psVector *peaktried = psVectorAlloc(fp->peaks->n, PS_TYPE_U8);
	psVectorInit(peaktried, 0);
	peaktried->data.U8[0] = true; // we skip the first peak

	psVector *threshbounds = psVectorAlloc(nBins, PS_TYPE_F32);
	for (int i = 0; i < nBins; i++) {
	    threshbounds->data.F32[i] = 0.25*beta2*PS_SQR(i) + min_threshold;	    
	}
#if (0)
	if (threshbounds->data.F32[threshbounds->n-1] > maxFlux) {
	    psWarning ("upper limit: %f does not include max flux: %f",
		       threshbounds->data.F32[threshbounds->n-1], maxFlux);
	}
#endif
	psHistogram *threshist = psHistogramAllocGeneric(threshbounds);

	// assign the peaks to the histogram bins based on their nominal thresholsd
	for (int i = 1; i < fp->peaks->n; i++) {
	    const pmPeak *peak = fp->peaks->data[i];
	    float flux = useSmoothedImage ? peak->smoothFlux : peak->rawFlux;
	    float stdev = useSmoothedImage ? peak->smoothFluxStdev : peak->rawFluxStdev;

	    // if flux is negative, careful with fStdev
	    const float fStdev = fabs(stdev/flux);
	    const float stdev_pad = fabs(flux * hypot(fStdev, fPadding));

	    float threshold = flux - nsigma_delta*stdev_pad;
	    psAssert(!isnan(threshold), "impossible");

	    if (threshold <= min_threshold) {
		threshist->nums->data.F32[0] += 1.0;
		threshbins->data.S32[i] = 0;
		continue;
	    }
	    int bin = sqrt(4.0*(threshold - min_threshold)/beta2);
	    psAssert(bin >= 0, "impossible bin");

	    bin = PS_MIN(bin, threshist->nums->n - 1);
	    threshist->nums->data.F32[bin] += 1.0;
	    
	    // record the bin selected for this peak
	    threshbins->data.S32[i] = bin;
	}

	// XXX TEST: did we assign correctly
# if (0)
	int nPeaks = 1; // we don't put the brightest in the histogram
	for (int i = 0; i < threshist->nums->n; i++) {
	    if (threshist->nums->data.F32[i] > 0) {
		fprintf (stderr, "%f : %f : %d\n", threshist->bounds->data.F32[i], threshist->bounds->data.F32[i+1], (int)threshist->nums->data.F32[i]);
		nPeaks += threshist->nums->data.F32[i];
	    }
	}
	fprintf (stderr, "%d peaks vs %d in histogram\n", (int) fp->peaks->n, nPeaks);
# endif

	// XXX for the moment, we will use the alternate cull for all peaks -- it might be
	// faster to use the standard process for the peaks for which the threshold bin
	// contains < N sources (N ~ 5-10?)

	// loop over the threshold bins from brightest to faintest
	for (int i = threshist->nums->n - 1; i >= 0; i--) {
	    if (threshist->nums->data.F32[i] == 0) continue;

	    // we are going to examine the footprints at this threshold
	    float threshold = 0.5*(threshist->bounds->data.F32[i] + threshist->bounds->data.F32[i+1]);
		    
	    // generate all footprints corresponding to this threshold
	    psArray *myFP = pmFootprintsFind(subImg, threshold, 5);
	    if (!myFP) {
		psWarning ("missing footprint? threshold: %.f", threshold);
		continue;
	    }
	    if (!myFP->n) {
		psWarning ("empty footprint? threshold: %.f", threshold);
		psFree (myFP);
		continue;
	    }

	    // an array to track if the footprint has a brighter peak or not
	    psVector *found = psVectorAlloc(myFP->n + 1, PS_TYPE_U8);
	    psVectorInit(found, 0);
	    int nFound = 0;

	    // set IDs to distinguish the footprints
	    psImageInit(idImg, 0);
	    pmSetFootprintArrayIDsForImage(idImg, myFP, true);

	    // check which footprints contain already-accepted (brighter) peaks 
	    // (we can give up if/when we found a peak for all footprints)
	    for (int j = 0; (j < brightPeaks->n) && (nFound < found->n); j++) {
		const pmPeak *peak = brightPeaks->data[j];
		int x = peak->x - subImg->col0;
		int y = peak->y - subImg->row0;
		int myID = idImg->data.S32[y][x];
		psAssert(myID >= 0, "impossible");
		psAssert(myID < found->n, "impossible");

		if (myID == 0) continue; // bright peak is not in a footprint
		if (found->data.U8[myID]) continue; // we already know this footprint contains a peak
		found->data.U8[myID] = true;
		nFound ++;
	    }
	
	    // check the peaks from this threshold bin: if they land in a footprint which has
	    // been found, we should drop that peak.  otherwise, keep it
	    for (int j = 0; j < fp->peaks->n; j++) {
		pmPeak *peak = fp->peaks->data[j];
		if (peak->assigned) continue; // peak is already claimed by a source -- don't cull

		// skip peaks if we've already tried them
		if (peaktried->data.U8[j]) continue;

		// is this peak in the threshold bin of interest?
		if (threshbins->data.S32[j] != i) continue;
		
		// do not try this peak again
		peaktried->data.U8[j] = true;

		int x = peak->x - subImg->col0;
		int y = peak->y - subImg->row0;
		int myID = idImg->data.S32[y][x];
		psAssert(myID < found->n, "impossible");

		// a peak in this threshold bin without a valid footprint comes from a region
		// with only a handful of pixels (1 or more from the peak itself).  It probably
		// cannot be joined to a neighbor
		if (myID == 0) {
		    psArrayAdd (brightPeaks, 128, peak);
		    continue;
		}

		// keep this peak if found->data.U8[myID] == false (no brighter peak in the footprint)
		if (!found->data.U8[myID]) {
		    // fprintf (stderr, "keeping %d: %d,%d\n", j, peak->x, peak->y);
		    psArrayAdd (brightPeaks, 128, peak);
		    continue;
		}
		// fprintf (stderr, "skipping %d: %d,%d\n", j, peak->x, peak->y);
	    }
	    psFree (myFP);
	    psFree (found);
	}
	psFree (threshist);
	psFree (threshbounds);
	psFree (threshbins);
	psFree (peaktried);

    } else {

	// The brightest peak is always safe; go through other peaks trying to cull them
	for (int i = 1; i < fp->peaks->n; i++) { // n.b. fp->peaks->n can change within the loop
	    const pmPeak *peak = fp->peaks->data[i];

	    float flux  = useSmoothedImage ? peak->smoothFlux      : peak->rawFlux;
	    float stdev = useSmoothedImage ? peak->smoothFluxStdev : peak->rawFluxStdev;

	    // if flux is negative, careful with fStdev
	    const float fStdev = fabs(stdev/flux);
	    const float stdev_pad = fabs(flux * hypot(fStdev, fPadding));

	    float threshold = flux - nsigma_delta*stdev_pad;

	    if (isnan(threshold)) {
		// min_threshold is assumed to be below the detection threshold,
		// so all the peaks are pmFootprint, and this isn't the brightest
		continue;
	    }

	    // just in case, force the threshold below the peak source flux
	    if (threshold > flux) {
		threshold = flux - 10*FLT_EPSILON;
	    }

	    if (threshold < min_threshold) {
		threshold = min_threshold;
	    }
	    if (threshold > max_threshold) {
		threshold = max_threshold;
	    }

	    pmFootprintsFindAtPoint(peakFootprint, peakFPSpans, subImg, subMask, threshold, brightPeaks, peak->y, peak->x);
	    // if (peakFPSpans->nStartSpans > 2000) {
	    // 	// dumpfootprints(peakFootprint, peakFPSpans);
	    // 	// fprintf (stderr, "big footprint %d : %d\n", peakFootprint->nspans, peakFPSpans->nStartSpans);
	    // 	// fprintf (stderr, "big test footprint: %f %f to %f %f (%d pix)\n", peakFootprint->bbox.x0, peakFootprint->bbox.y0, peakFootprint->bbox.x1, peakFootprint->bbox.y1, peakFootprint->npix);
	    // }

	    // at this point brightPeaks only has the peaks brighter than the current

	    // we set the IDs to either 1 (in peak) or 0 (not in peak)
	    pmSetFootprintID (idImg, peakFootprint, IN_PEAK);

	    // If this peak has not already been assigned to a source, then we can look for any
	    // brighter peaks within its footprint. Check if any of the previous (brighter) peaks
	    // are within the footprint of this peak If so, the current peak is bogus; drop it.
	    bool keep = true;
	    for (int j = 0; keep && !peak->assigned && (j < brightPeaks->n); j++) {
		// const pmPeak *peak2 = fp->peaks->data[j]; XXX isn't this an error?  we only care about the kept brighter peak, right?
		const pmPeak *peak2 = brightPeaks->data[j];
		int x2 = peak2->x - subImg->col0;
		int y2 = peak2->y - subImg->row0;
		if (idImg->data.S32[y2][x2] == IN_PEAK) {
		    // There's a brighter peak within the footprint above threshold; so cull our initial peak
		    keep = false;
		}
	    }
	    if (!keep) {
		psAssert (!peak->assigned, "logic error: trying to drop a previously-assigned peak");  // we should not drop any already assigned peaks.
		continue;
	    }
	    psArrayAdd (brightPeaks, 128, fp->peaks->data[i]);
	}
    }

    psFree (fp->peaks);
    fp->peaks = brightPeaks;

    psFree(idImg);
    psFree(subImg);
    psFree(subMask);
    psFree(peakFootprint);
    psFree(peakFPSpans);

    return PS_ERR_NONE;
}


bool dumpfootprints (pmFootprint *fp, pmFootprintSpans *fpSp) {

    FILE *f1 = fopen ("fp.dat", "w");
    if (!f1) return false;

    for (int i = 0; i < fp->nspans; i++) {
	pmSpan *sp = fp->spans->data[i];
	fprintf (f1, "%d - %d : %d\n", sp->x0, sp->x1, sp->y);
    }
    fclose (f1);

    FILE *f2 = fopen ("fpSp.dat", "w");
    if (!f2) return false;

    for (int i = 0; i < fpSp->nStartSpans; i++) {
	pmStartSpan *sp = fpSp->startspans->data[i];
	if (!sp->span) continue;
	fprintf (f2, "%d - %d : %d\n", sp->span->x0, sp->span->x1, sp->span->y);
    }
    fclose (f2);
    return true;
}

/* @file  pmFootprintSpans.h
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-09 21:16:09 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FOOTPRINT_SPANS_H
#define PM_FOOTPRINT_SPANS_H

/* We define two helper structures used in building the pmFootprints:
 *
 * pmStartSpan      : a smart span which knows how to (re-)start pixel scanning
 * pmFootprintSpans : a collection of pmStartSpans which can define a footprint
 *
 * pmFootprintSpans allows us to allocate the memory for the spans before actually defining them
 *
 */

/* pmStartSpan
 *
 * A data structure to hold the starting point for a search for pixels above threshold,
 * used by pmFootprintsFindAtPoint
 *
 * We don't want to find this span again --- it's already part of the footprint ---
 * so we set appropriate mask bits
 *
 */

//
// An enum for what we should do with a pmStartSpan
//
typedef enum {PM_STARTSPAN_NONE = 0,	// span is not defined
	      PM_STARTSPAN_DOWN,	// scan down from this span
              PM_STARTSPAN_UP,		// scan up from this span
              PM_STARTSPAN_RESTART,	// restart scanning from this span
              PM_STARTSPAN_DONE		// this span is processed
} PM_STARTSPAN_DIR;			// How to continue searching
//
// An enum for mask's pixel values.  We're looking for pixels that are above threshold, and
// we keep extra book-keeping information in the PM_STARTSPAN_STOP plane.  It's simpler to be
// able to check for
//
enum {
    PM_STARTSPAN_INITIAL = 0x0,             // initial state of pixels.
    PM_STARTSPAN_DETECTED = 0x1,            // we've seen this pixel
    PM_STARTSPAN_STOP = 0x2                 // you may stop searching when you see this pixel
};
//
// The struct that remembers how to [re-]start scanning the image for pixels
//
typedef struct {
    pmSpan *span;			// view on the real span (on a pmFootprint->spans array)
    PM_STARTSPAN_DIR direction;		// How to continue searching
    bool stop;                          // should we stop searching?
} pmStartSpan;

typedef struct {
    psArray *startspans;
    int nStartSpans;
} pmFootprintSpans;

pmStartSpan *pmStartSpanAlloc();

bool pmStartSpanSet(pmStartSpan *sspan,
		    pmSpan *span,      // The span in question
		    psImage *mask,           // Pixels that we've already detected
		    const PM_STARTSPAN_DIR dir   // Should we continue searching towards the top of the image?
    );

pmFootprintSpans *pmFootprintSpansAlloc(int nSpans);

bool pmFootprintSpansInit(pmFootprintSpans *fpSpans);

bool pmFootprintSpansSet(pmFootprintSpans *fpSpans, // the saved pmStartSpans
			 pmSpan *sp, // the span in question
			 psImage *mask, // mask of detected/stop pixels
			 const PM_STARTSPAN_DIR dir); // the desired direction to search

/// @}
# endif /* PM_FOOTPRINT_SPANS_H */

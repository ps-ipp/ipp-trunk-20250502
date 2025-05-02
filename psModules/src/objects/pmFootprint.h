/* @file  pmFootprint.h
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-09 21:16:09 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FOOTPRINT_H
#define PM_FOOTPRINT_H

// We need to choose up front if the culling algorithm uses the raw or smoothed image.
// depending on which we choose, we should produce sorted peaks based on peak->rawFlux or
// peak->smoothFlux

# define PM_PEAKS_CULL_WITH_SMOOTHED_IMAGE 1

typedef struct {
    const int id;                       //!< unique ID
    int npix;                           //!< number of pixels in this pmFootprint
    int nspans;
    psArray *spans;                     //!< the allocated pmSpans
    psRegion bbox;                      //!< the pmFootprint's bounding box
    psArray *peaks;                     //!< the peaks lying in this footprint
    psRegion region;   //!< A region describing the psImage the footprints live in
    bool normalized;                    //!< Are the spans sorted?
} pmFootprint;

pmFootprint *pmFootprintAlloc(int nspan, const psImage *img);
bool pmFootprintInit(pmFootprint *footprint);
bool pmFootprintTest(const psPtr ptr);

bool pmFootprintAllocEmptySpans (pmFootprint *footprint, int nSpans);

pmFootprint *pmFootprintCopyData(pmFootprint *inFoot, psImage *image);

pmFootprint *pmFootprintNormalize(pmFootprint *fp);
int pmFootprintSetNpix(pmFootprint *fp);
void pmFootprintSetBBox(pmFootprint *fp);

pmSpan *pmFootprintAddSpan(pmFootprint *fp,     // the footprint to add to
                           const int y, // row to add
                           int x0,      // range of
                           int x1);    //          columns

pmSpan *pmFootprintSetSpan(pmFootprint *fp,	// the footprint to add to
			   const int y,		// row to add
			   int x0,		// range of
			   int x1); 		// columns

psArray *pmFootprintsFind(const psImage *img, const float threshold, const int npixMin);

bool pmFootprintsFindAtPoint(pmFootprint *fp,
			     pmFootprintSpans *fpSpans,
			     const psImage *img,     // image to search
			     psImage *mask,
			     const float threshold,   // Threshold
			     const psArray *peaks, // array of peaks; finding one terminates search for footprint
			     int row, int col);

// pmFootprint *pmFootprintsFindAtPoint(const psImage *img,
//                                     const float threshold,
//                                     const psArray *peaks,
//                                     int row, int col);

bool pmFootprintSpansBuild(pmFootprint *fp, // the footprint that we're building
			   pmFootprintSpans *fpSpans,
			   const psImage *img, // the psImage we're working on
			   psImage *mask, // the associated masks
			   const float threshold // Threshold
    );

psArray *pmFootprintArrayGrow(const psArray *footprints, int r);
psArray *pmFootprintArraysMerge(const psArray *footprints1, const psArray *footprints2,
                                const int includePeaks);

psImage *pmSetFootprintArrayIDs(const psArray *footprints, const bool relativeIDs);
psImage *pmSetFootprintID(psImage *idImage, const pmFootprint *fp, const int id);
void pmSetFootprintArrayIDsForImage(psImage *idImage,
                                    const psArray *footprints, // the footprints to insert
                                    const bool relativeIDs // show IDs starting at 0, not pmFootprint->id
    );

psErrorCode pmFootprintsAssignPeaks(psArray *footprints, const psArray *peaks);

psErrorCode pmFootprintCullPeaks(const psImage *img,       // the image wherein lives the footprint
				 const psImage *weight,	   // corresponding variance image
				 pmFootprint *fp,	   // Footprint containing mortal peaks
				 const float nsigma_delta, // how many sigma above local background a peak needs to be to survive
				 const float fPadding, // fractional padding added to stdev since bright peaks have unreasonably high significance
				 const float min_threshold, // minimum permitted coll height
				 const float max_threshold,// maximum permitted coll height
				 const bool isWeightVar // the input weight may be variance (sigma^2) or S/N (1/sigma)
    );

psArray *pmFootprintArrayToPeaks(const psArray *footprints);

/// @}
# endif /* PM_FOOTPRINT_H */

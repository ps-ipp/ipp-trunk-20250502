/* @file  pmPeaks.h
 *
 * The process of finding, measuring, and classifying astronomical sources on
 * images is one of the critical tasks of the IPP or any astronomical software
 * system. This file will define structures and functions related to the task
 * of source detection and measurement. The elements defined in this section
 * are generally low-level components which can be connected together to
 * construct a complete object measurement suite.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-09 21:16:09 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_PEAKS_H
# define PM_PEAKS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmPeakType
 *
 *  A peak pixel may have several features which may be determined when the
 *  peak is found or measured. These are specified by the pmPeakType enum.
 *  PM_PEAK_LONE represents a single pixel which is higher than its 8 immediate
 *  neighbors.  The PM_PEAK_EDGE represents a peak pixel which touching the image
 *  edge. The PM_PEAK_FLAT represents a peak pixel which has more than a specific
 *  number of neighbors at the same value, within some tolarence:
 *
 */
typedef enum {
    PM_PEAK_LONE,                       ///< Isolated peak.
    PM_PEAK_EDGE,                       ///< Peak on edge.
    PM_PEAK_FLAT,                       ///< Peak has equal-value neighbors.
    PM_PEAK_SUSPECT_SATURATION,         ///< Peak is probably saturated
    PM_PEAK_UNDEF                       ///< Undefined.
} pmPeakType;


/** pmPeak data structure
 *
 *  A source has the capacity for several types of measurements. The
 *  simplest measurement of a source is the location and flux of the peak pixel
 *  associated with the source:
 *
 *  There are 3 values which define the amplitude of the peak and which may be used to sort the
 *  peaks: 
 *  * detValue - the peak in the detection image (nominally, the S/N)
 *  * rawFlux - the peak in the unsmoothed image
 *  * smoothFlux - the peak in the smoothed image
 * 
 *  For a given image, peaks do necesarily not have the same sequence for these three values.
 *  Depending on the analysis, it may make sense to sort by one or the other of these values
 */
typedef struct
{
    const int id;   ///< Unique ID for object
    int x;                              ///< X-coordinate of peak pixel.
    int y;                              ///< Y-coordinate of peak pixel.
    float xf;                           ///< bicube fit to peak coord (x)
    float yf;                           ///< bicube fit to peak coord (y)
    float dx;                           ///< bicube fit error on peak coord (x)
    float dy;                           ///< bicube fit error on peak coord (y)
    float detValue;                     ///< peak flux in detection image (= S/N)
    float rawFlux;                      ///< peak flux in unsmoothed signal image
    float rawFluxStdev;                 ///< peak stdev in unsmoothed signal image
    float smoothFlux;                   ///< peak flux in smoothed signal image
    float smoothFluxStdev;              ///< peak stdev in smoothed signal image
    bool assigned;                      ///< is peak assigned to a source?
    pmPeakType type;                    ///< Description of peak.
    pmFootprint *footprint;		///< reference to containing footprint (just a view, not a memcopy)
    psArray *saddlePoints;		///< set of saddle points between this peak and near neighbors
}
pmPeak;


/** pmPeakAlloc()
 *
 *  @return pmPeak*    newly allocated pmPeak with all internal pointers set to NULL
 */
pmPeak *pmPeakAlloc(
    int x,    ///< Row-coordinate in image space
    int y,    ///< Col-coordinate in image space
    float counts,   ///< The value of the peak pixel
    pmPeakType type   ///< The type of peak pixel
);

bool psMemCheckPeak(psPtr ptr);

bool pmPeakCopy(pmPeak *out, pmPeak *in);

/** pmPeaksInVector()
 *
 * Find all local peaks in the given vector above the given threshold. A peak
 * is defined as any element with a value greater than its two neighbors and with
 * a value above the threshold. Two types of special cases must be addressed.
 * Equal value elements: If an element has the same value as the following
 * element, it is not considered a peak. If an element has the same value as the
 * preceding element (but not the following), then it is considered a peak. Note
 * that this rule (arbitrarily) identifies flat regions by their trailing edge.
 * Edge cases: At start of the vector, the element must be higher than its
 * neighbor. At the end of the vector, the element must be higher or equal to its
 * neighbor. These two rules again places the peak associated with a flat region
 * which touches the image edge at the image edge. The result of this function is
 * a vector containing the coordinates (element number) of the detected peaks
 * (type psU32).
 *
 */
psVector *pmPeaksInVector(
    const psVector *vector,  ///< The input vector (float)
    float threshold   ///< Threshold above which to find a peak
);


/** pmPeaksInImage()
 *
 * Find all local peaks in the given image above the given threshold. This
 * function should find all row peaks using pmFindVectorPeaks, then test each row
 * peak and exclude peaks which are not local peaks. A peak is a local peak if it
 * has a higher value than all 8 neighbors. If the peak has the same value as its
 * +y neighbor or +x neighbor, it is NOT a local peak. If any other neighbors
 * have an equal value, the peak is considered a valid peak. Note two points:
 * first, the +x neighbor condition is already enforced by pmFindVectorPeaks.
 * Second, these rules have the effect of making flat-topped regions have single
 * peaks at the (+x,+y) corner. When selecting the peaks, their type must also be
 * set. The result of this function is an array of pmPeak entries.
 *
 */
psArray *pmPeaksInImage(
    const psImage *image,  ///< The input image where peaks will be found (float)
    float threshold   ///< Threshold above which to find a peak
);


/** pmPeaksSubset()
 *
 * Create a new peaks array, removing certain types of peaks from the input
 * array of peaks based on the given criteria. Peaks should be eliminated if they
 * have a peak value above the given maximum value limit or if the fall outside
 * the valid region.  The result of the function is a new array with a reduced
 * number of peaks.
 *
 */
psArray *pmPeaksSubset(
    psArray *peaks,                     ///< Add comment.
    float maxvalue,                     ///< Add comment.
    const psRegion valid                ///< Add comment.
);

int pmPeaksSortByDetValueAscend (const void **a, const void **b);
int pmPeaksSortByDetValueDescend (const void **a, const void **b);
int pmPeaksSortByRawFluxAscend (const void **a, const void **b);
int pmPeaksSortByRawFluxDescend (const void **a, const void **b);
int pmPeaksSortBySmoothFluxAscend (const void **a, const void **b);
int pmPeaksSortBySmoothFluxDescend (const void **a, const void **b);

/// @}
# endif /* PM_PEAKS_H */

/* @file  pmSourceDiffStats.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.29 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2004 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_SOURCE_DIFF_STATS_H
# define PM_SOURCE_DIFF_STATS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/* The elements of this structure are used to characterize diff image detections.
 * The values in the structure are derived from the following measurements:
 * fGood = \sum(flux) for pixels with S/N > +SN_LIMIT
 * fBad  = \sum(flux) for pixels with S/N < -SN_LIMIT
 * nGood = \sum(pixels) with S/N > +SN_LIMIT
 * nBad  = \sum(pixels) with S/N < -SN_LIMIT
 * nMask = \sum(pixels) masked
 */

typedef struct {
    float fRatio;			// = fGood / (fGood + fBad)
    float nRatioBad;			// = nGood / (nGood + nBad)
    float nRatioMask;			// = nGood / (nGood + nMask)
    float nRatioAll;			// = nGood / (nGood + nMask + nBad)
    int   nGood;			// nGood as defined above
    float SNp;				// S/N of matched source in positive image
    float SNm;				// S/N of matched source in negative image
    float Rp;				// radius of matched source in positive image
    float Rm;				// radius of matched source in negative image
} pmSourceDiffStats;


/** pmSourceDiffStatsAlloc()
 */
pmSourceDiffStats *pmSourceDiffStatsAlloc(void);

/** pmSourceDiffStatsInit()
 * function to initialize the values in the structure
 */
void pmSourceDiffStatsInit(pmSourceDiffStats *diffStats);

/// @}
# endif

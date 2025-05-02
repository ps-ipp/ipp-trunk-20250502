/* @file  psStats.h
 * @brief basic statistical operations
 *
 * This file will hold the definition of the histogram and stats data
 * structures.  It also contains prototypes for procedures which operate
 * on those data structures.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.66 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_STATS_H
#define PS_STATS_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/******************************************************************************
    Statistical functions and data structures.
 *****************************************************************************/

/** enumeration of statistical calculation options
 *
 *  @see psStats, psVectorStats, psImageStats
 */
typedef enum {
    PS_STAT_NONE            = 0x000000, ///< Empty set
    PS_STAT_MIN             = 0x000001, ///< Maximum
    PS_STAT_MAX             = 0x000002, ///< Minumum
    PS_STAT_SAMPLE_MEAN     = 0x000004, ///< Sample Mean
    PS_STAT_SAMPLE_MEDIAN   = 0x000008, ///< Sample Median
    PS_STAT_SAMPLE_STDEV    = 0x000010, ///< Sample Standard Deviation
    PS_STAT_SAMPLE_QUARTILE = 0x000020, ///< Sample Quartile
    PS_STAT_SAMPLE_SKEWNESS = 0x000040, ///< Sample Skewness (third moment)
    PS_STAT_SAMPLE_KURTOSIS = 0x000080, ///< Sample Kurtosis (fourth moment)
    PS_STAT_ROBUST_MEDIAN   = 0x000100, ///< Robust Median
    PS_STAT_ROBUST_STDEV    = 0x000200, ///< Robust Standarad Deviation
    PS_STAT_ROBUST_QUARTILE = 0x000400, ///< Robust Quartile
    PS_STAT_ROBUST_SPARE1   = 0x000800, ///< Spare 1
    PS_STAT_FITTED_MEAN     = 0x001000, ///< Fitted Mean
    PS_STAT_FITTED_STDEV    = 0x002000, ///< Fitted Standard Deviation
    PS_STAT_CLIPPED_MEAN    = 0x040000, ///< Clipped Mean
    PS_STAT_CLIPPED_STDEV   = 0x080000, ///< Clipped Standard Deviation
    PS_STAT_USE_RANGE       = 0x100000, ///< Range
    PS_STAT_USE_BINSIZE     = 0x200000, ///< Binsize
} psStatsOptions;

/** This is the generic statistics structure.  It contails the data members
    for the various statistic values.  It also contains the options member to
    specifiy which statistics should be calculated. */
typedef struct
{
    double sampleMean;                 ///< formal mean of sample
    double sampleMedian;               ///< formal median of sample
    double sampleStdev;                ///< standard deviation of sample
    double sampleUQ;                   ///< upper quartile of sample
    double sampleLQ;                   ///< lower quartile of sample
    double sampleSkewness;             ///< skewness (third moment) of sample
    double sampleKurtosis;             ///< kurtosis (fourth moment) of sample
    double robustMedian;               ///< robust median of array
    double robustStdev;                ///< robust standard deviation of array
    double robustUQ;                   ///< robust upper quartile
    double robustLQ;                   ///< robust lower quartile
    long robustN50;                    ///< Number of points in Gaussian fit; XXX: This is currently unused.
    double fittedMean;                 ///< robust mean of data
    double fittedStdev;                ///< robust standard deviation of data
    long fittedNfit;                   ///< Number of points in Gaussian fit; XXX: This is currently unused
    double clippedMean;                ///< Nsigma clipped mean
    double clippedStdev;               ///< standard deviation after clipping
    long clippedNvalues;               ///< Number of data points used for clipped mean.
    double clipSigma;                  ///< Nsigma used for clipping; user input
    int clipIter;                      ///< Number of clipping iterations; user input
    double min;                        ///< minimum data value in array
    double max;                        ///< maximum data value in array
    double binsize;                    ///< binsize for robust fit (input/ouput)
    long nSubsample;                   ///< maxinum number of measurements (input)
    psStatsOptions options;            ///< bitmask of values requested
    psStatsOptions results;            ///< bitmask of values calculated
    psVector *tmpData;		       ///< temporary vector so repeated calls do not have to realloc
    psVector *tmpMask;		       ///< temporary vector so repeated calls do not have to realloc
}
psStats;

/** Performs statistical calculations on a vector.
 *
 *  @return psStats*    the statistical results as specified by stats->options
 */
bool psVectorStats(
    psStats* stats,	       ///< stats structure defines stats to be calculated and how
    const psVector* in,			///< Vector to be analysed.
    const psVector* errors,		///< Errors.
    const psVector* mask, ///< Ignore elements where (maskVector & maskVal) != 0: must be INT or NULL
    psVectorMaskType maskVal ///< Only mask elements with one of these bits set in maskVector
);

/** Allocator of the psStats structure.
 *
 *  @return psStats*    A new psStats struct with the options member set to the
 *                      value given.
 */
#ifdef DOXYGEN
psStats* psStatsAlloc(
    psStatsOptions options              ///< Statistics to calculate
);
#else // ifdef DOXYGEN
psStats* p_psStatsAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psStatsOptions options              ///< Statistics to calculate
) PS_ATTR_MALLOC;
#define psStatsAlloc(options) \
      p_psStatsAlloc(__FILE__, __LINE__, __func__, options)
#endif // ifdef DOXYGEN

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psStats structure, false otherwise.
 */
bool psMemCheckStats(
    psPtr ptr                          ///< the pointer whose type to check
);

// reset the values which are output, and which may be used from one psStats stage to the next
void psStatsInit(psStats *stats);

// Get the statistics option from a string
psStatsOptions psStatsOptionFromString(const char *string);
// Write a string from the statistics options
psString psStatsOptionToString(psStatsOptions option);
// Generate a psStats from a string of statistics options
psStats *psStatsFromString(const char *string);
// Generate a string of statistics options from a psStats
psString psStatsToString(const psStats *stats);
// Is only a single statistics option set?
psStatsOptions psStatsSingleOption(psStatsOptions option);
// Return a particular stats value
double psStatsGetValue(const psStats *stats, psStatsOptions option);
// Return the statistics option(s) for the mean/median
psStatsOptions psStatsMeanOption(psStatsOptions options);
// Return the statistics option(s) for the stdev
psStatsOptions psStatsStdevOption(psStatsOptions options);

/// @}
#endif // #ifndef PS_STATS_H

/*
 * private stats functions used in psStats.c:
 *
 * vectorSampleMean
   (none)
 * vectorMinMax
   (none)
 * vectorSampleMedian (also yields SAMPLE_QUARTILE)
   (none)
 * vectorSampleStdev
   (vectorSampleMean)
 * vectorClippedStats
   (vectorSampleMedian)
   (vectorSampleMean (*also subset))
   (vectorSampleStdev (*also subset))
 * vectorRobustStats
   (vectorMinMax (*only subset))
 * vectorFittedStats
   (vectorRobustStats)

 * private stats functions called by other private stats functions are automatically called by
 * those functions.  since they set the stats->results flags, they are not called multiple
 * times.

 * the private stats functions do not test for their corresponding stats flags: it is not
 * necessary to request them if they are called within this function.

*/

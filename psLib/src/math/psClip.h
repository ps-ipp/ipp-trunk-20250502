/* @file  psClip.h
 * @brief vector clipping functions
 *
 * @author Paul Price, IfA.
 *
 * $Revision: 1.6 $ $Name: not supported by cvs2svn $
 * $Date: 2009-01-27 06:39:38 $
 * Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_CLIP_H
#define PS_CLIP_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/// Parameters for clipping
typedef struct
{
    psStatsOptions meanStat;            ///< Stats option to use for mean
    psStatsOptions stdevStat;           ///< Stats option to use for standard deviation
    float fracHigh;                     ///< Fraction of high values to clip
    float fracLow;                      ///< Fraction of low values to clip
    int numKeep;                        ///< Minimum number of values to keep from clipping
    int iter;                           ///< Number of rejection iterations; unused by psClip functions
    float rej;                          ///< Rejection limit (standard deviations)
    psVectorMaskType masked;                  ///< Mask value for entries already masked
    psVectorMaskType clipped;                 ///< Mask value to give to clipped entries
    double mean;                        ///< Resultant mean
    double stdev;                       ///< Resultant stdev
}
psClipParams;

/// Allocator
psClipParams *psClipParamsAlloc(psStatsOptions meanStat, ///< Stats option to use for mean
                                psStatsOptions stdevStat, ///< Stats option to use for standard deviation
                                psVectorMaskType masked, ///< Mask value for entries already masked
                                psVectorMaskType clipped ///< Mask value to give to clipped entries
    ) PS_ATTR_MALLOC;


/// Apply min-max clipping to a list of values
///
/// The specified fraction of high and low values are identified as clipped in the mask.  Errors are not used
/// in this step.
long psClipMinMax(const psClipParams *params, ///< Clip parameters
                  const psVector *values, ///< Values to inspect and clip
                  psVector *mask        ///< Mask for values
    );



/// Apply a rejection iteration to a list of values
///
/// The specified rejection limit is applied to the values and errors; discrepant values are identified as
/// clipped in the mask.  This function only applies a single rejection iteration.
long psClipReject(psClipParams *params, ///< Clip parameters
                  const psVector *values, ///< Values to inspect and clip
                  psVector *mask,       ///< Mask for values
                  const psVector *errors ///< Errors for values, or NULL
    );

/// @}
#endif

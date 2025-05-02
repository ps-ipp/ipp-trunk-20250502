/* @file  pmReadoutCombine.h
 * @brief Combine multiple readouts
 *
 * @author George Gusciora, MHPCC
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:25 $
 * Copyright 2004-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_READOUT_COMBINE_H
#define PM_READOUT_COMBINE_H

/// @addtogroup imcombine Image Combinations
/// @{

/// Combination parameters for pmReadoutCombine.
///
/// These values define how the combination is performed, and should not vary by detector, so that it can be
/// re-used for multiple combinations.
typedef struct {
    psStatsOptions combine;             ///< Statistic to use when performing the combination
    psImageMaskType maskVal;            ///< Mask value
    psImageMaskType blank;            ///< Mask value to give blank (i.e., no data) pixels
    int nKeep;                          ///< Mimimum number of pixels to keep
    float fracHigh;                     ///< Fraction of high pixels to immediately throw
    float fracLow;                      ///< Fraction of low pixels to immediately throw
    int iter;                           ///< Number of iterations for clipping (for CLIPPED_MEAN only)
    float rej;                          ///< Rejection threshould for clipping (for CLIPPED_MEAN only)
    bool variances;                     ///< Use the supplied variances (instead of calculated stdev)?
} pmCombineParams;

// Allocator for pmCombineParams
pmCombineParams *pmCombineParamsAlloc(psStatsOptions statsOptions ///< Statistic to use for combination
                                     );

// check the input parameters and set up the output images
bool pmReadoutCombinePrepare(pmReadout *output, const psArray *inputs, const pmCombineParams *params);

/// Combine multiple readouts, applying zero and scale, with optional minmax clipping
bool pmReadoutCombine(pmReadout *output,///< Output readout; altered and returned
                      const psArray *inputs,  ///< Array of input readouts
                      const psVector *zero, ///< Zero corrections to subtract from input, or NULL
                      const psVector *scale, ///< Scale corrections to divide into input, or NULL
                      const pmCombineParams *params ///< Combination parameters
                     );

bool pmReadoutCombineVisualInit(void);
bool pmReadoutCombineVisualPixels(psVector *pixels, psVector *mask, float mean);
bool pmReadoutCombineVisualCleanup(void);

/// @}
#endif

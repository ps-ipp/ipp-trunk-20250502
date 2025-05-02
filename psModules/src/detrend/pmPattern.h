/* @file pmPattern.h
 * @brief Fit and remove pattern noise
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.16 $
 * @date $Date: 2009-02-12 19:25:52 $
 * Copyright 2004-2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_PATTERN_H
#define PM_PATTERN_H

#include <pslib.h>
#include <pmFPA.h>
#include <pmFPAview.h>
#include <pmFPAfile.h>
#include <pmFPAfileFitsIO.h>
#include <pmFPAHeader.h>
#include <pmHDU.h>
#include <pmHDUUtils.h>

/// @addtogroup detrend Detrend Creation and Application
/// @{

#define PM_PATTERN_ROW_CORRECTION "PATTERN.ROW.CORRECTION" // Pattern row correction on analysis metadata
#define PM_PATTERN_CELL_CORRECTION "PATTERN.CELL.CORRECTION" // Pattern cell correction on analysis metadata

/// Fit and remove pattern noise over rows
bool pmPatternRow(
    pmReadout *ro,                      ///< Readout to correct
    int order,                          ///< Polynomial order
    int iter,                           ///< Number of clipping iterations
    float rej,                          ///< Rejection threshold for clipping
    float thresh,                       ///< Threshold for ignoring pixels
    psStatsOptions clipMean,            ///< Statistic to use for mean
    psStatsOptions clipStdev,           ///< Statistic to use for standard deviation
    psImageMaskType maskVal,            ///< Mask value to use
    psImageMaskType maskBad             ///< Mask value to give bad pixels
    );

/// Apply previously measured row pattern correction
bool pmPatternRowApply(pmReadout *ro,   ///< Readout to correct
                       psImageMaskType maskBad ///< Mask value to give bad pixels
                       );

/// Fix the background on cells known to be troublesome
bool pmPatternCell(
    pmChip *chip,                       ///< Chip to correct
    const psVector *tweak,              ///< U8 vector indicating whether to tweak the corresponding cell
    psStatsOptions bgStat,              ///< Statistic to use for background measurement
    psStatsOptions cellStat,            ///< Statistic to use for combination of cell background measurements
    psImageMaskType maskVal,            ///< Mask value to use
    psImageMaskType maskBad             ///< Mask value to give bad pixels
    );

/// Apply previously measured cell pattern correction
bool pmPatternCellApply(pmReadout *ro,          ///< Readout to correct
                        psImageMaskType maskBad ///< Mask value to give bad pixels
                        );

/// Fix the background on cells known to be troublesome
bool pmPatternContinuity(
    pmChip *chip,                       ///< Chip to correct
    const psVector *tweak,              ///< U8 vector indicating whether to tweak the corresponding cell
    psStatsOptions bgStat,              ///< Statistic to use for background measurement
    psStatsOptions cellStat,            ///< Statistic to use for combination of cell background measurements
    psImageMaskType maskVal,            ///< Mask value to use
    psImageMaskType maskBad,            ///< Mask value to give bad pixels
    int edgeWidth                       ///< Size of box to use
    );

bool pmPatternContinuityBackground(pmFPAfile *in,
				   pmFPAfile *out,
				   psStatsOptions bgStat,
				   psStatsOptions cellStat,
				   psImageMaskType maskVal,
				   psImageMaskType maskBad,
				   int edgeWidth);

/// Apply previously measured cell pattern correction
bool pmPatternContinuityApply(pmReadout *ro,          ///< Readout to correct
                        psImageMaskType maskBad ///< Mask value to give bad pixels
                        );



bool pmPatternDeadCells(pmChip *chip, const psVector *backs, psImageMaskType maskVal);

/// @}
#endif




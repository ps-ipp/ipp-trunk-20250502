/** @file ppMergeScaleZero.h
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PP_MERGE_SCALE_ZERO_H
#define PP_MERGE_SCALE_ZERO_H

#include <pslib.h>
#include <psmodules.h>

#include "ppMergeData.h"
#include "ppMergeOptions.h"

/**
 * Get the scale and zero for each chip of each input
 */
bool ppMergeScaleZero(psImage **scales, ///< The scales for each integration/cell
                      psImage **zeros, ///< The zeroes for each integration/cell
                      psArray **shutter,///< The shutter correction data for each cell
                      ppMergeData *data,///< The data
                      const ppMergeOptions *options, ///< The options
                      const pmConfig *config ///< The configuration
    );

#endif

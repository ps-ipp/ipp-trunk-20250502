#ifndef PM_REMNANCE_H
#define PM_REMNANCE_H

#include <pslib.h>
#include "pmFPA.h"

// Analysis metadata names
#define PM_REMNANCE_ANALYSIS_NUM "DETREND.REMNANCE.NUM" // Number of masked remnance pixels

// Mask remnance pixels
//
// By "remnance", we mean pixels left over from previous exposures.
// GPC1 leaves remnance that flows down from where the annoyed pixels are.
bool pmRemnance(pmReadout *ro,           ///< Readout with input image
                psImageMaskType maskVal,      ///< Value of mask
                psImageMaskType maskRem,       ///< Value to give remance
                int size,               ///< Size of accumulation patches
                float threshold         ///< Threshold for masking
    );

#endif

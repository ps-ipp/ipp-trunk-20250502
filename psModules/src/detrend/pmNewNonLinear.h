/* @file pmNewNonLinear.h
 * @brief Perform new (2023) non-linear correction using spline fits
 *
 * @author Eugene Magnier, IfA
 * Copyright 2023 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_NEW_NON_LINEAR_H
#define PM_NEW_NON_LINEAR_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// Correct non-linearity using spline
bool pmNewNonLinearityApply(pmReadout *inputReadout, psArray *table);

/// @}
#endif

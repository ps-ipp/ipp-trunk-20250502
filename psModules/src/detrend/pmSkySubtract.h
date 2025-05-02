/* @file  pmSubtractSky.h
 *
 * This file will contain a module which will create a model of the
 * background sky and subtract that from the input image.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PM_SUBTRACT_SKY_H
#define PM_SUBTRACT_SKY_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

// XXX: this is pmFit in pmSubtractBias.c, named psFit here.
typedef enum {
    PM_FIT_NONE,                              ///< No fit
    PM_FIT_POLYNOMIAL,                        ///< Fit polynomial
    PM_FIT_SPLINE                             ///< Fit cubic splines
} psFit;

pmReadout *pmSubtractSky(pmReadout *in,
                         void *fitSpec,
                         psFit fit,
                         int binFactor,
                         psStats *stats,
                         float clipSD);

/// @}
#endif

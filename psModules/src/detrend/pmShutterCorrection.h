/* @file pmShutterCorrection.h
 * @brief Functions to build and apply a shutter exposure-time correction.
 *
 * @author Eugene Magnier, IfA
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.24 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:27:49 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_SHUTTER_CORRECTION_H
#define PM_SHUTTER_CORRECTION_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/*  A mechanical shutter may not yield uniform exposure times as a function of position on the
 *  detector.  The typical error consists of a constant exposure-time offset relative to the
 *  requested value, ie exposure time is T_o + dT(x,y).  The exposure error, dT, may be
 *  measured with the following scheme.  Obtain a set of exposures with different exposures
 *  times taken of the same flat-field source; the source must be spatially stable between the
 *  exposures, but need not have a stable amplitude.  For an illuminating flux of intensity
 *  F(x,y) = F_o f(x,y), the signal recorded by any pixel in the detector is given by: S(t,x,y)
 *  = F_o(t) f(x,y) (T_o + dT(x,y)) where F_o(t) is the (variable) overall intensity of the
 *  illuminating source and f(x,y) is the spatial illumination pattern times the flat-field
 *  response.  Choose a reference location in the image (eg, the detector center) and divide by
 *  the value of that region (ie, mean or median):
 *
 *  s(t,x,y) = S(t,x,y) / S(t,0,0)
 *  s(t,x,y) = F_o(t) f(x,y) (T_o + dT(x,y)) / F_o(t) f(0,0) (T_o + dT(0,0))
 *  s(t,x,y) = f(x,y) (T_o + dT(x,y)) / f(0,0) (T_o + dT(0,0))
 *
 *  we can absorb the term f(0,0) into f(x,y) as we have no motivation for the scale of f(x,y)
 *  -- a normalization for the flat-field is not specified here.  For any single pixel, over
 *  the set of exposures, we thus need to solve for dT(x,y), dT(0,0), and f'(x,y) in the
 *  equation: s(t,x,y) = f'(x,y) (T_o + dT(x,y)) / (T_o + dT(0,0))
 *
 *  we avoid directly fitting these values as the process would be a non-linear
 *  least-squares problem for every pixel in the image, and thus very time
 *  consuming.  There are linear options which may be used instead.
 *  First, as T_o goes to a large value, s() approaches the value of f'(x,y).
 *  Next, as T_o goes to a very small value, s() approaches the value of
 *  f'(x,y)*dT(x,y)/dT(0,0).  Finally, when s() has the value of
 *  f'(x,y)*(1 + dT(x,y)/dT(0,0))/2, T_o has the value of dT(0,0).  with data
 *  points covering a reasonable dynamic range, we can solve for these three
 *  values by interpolation and/or extrapolation.
 *
 *  To take the strategy one step further, we could use the above recipe to
 *  obtain a guess for the three parameters and then apply non-linear fitting to
 *  solve more accurately for the parameters.  If we limit this operation to a
 *  handful of positions in the image (user defined, but the obvious choice would
 *  be positions near the center, edges, and corners), then we may determine a
 *  good value for dT(0,0).  Since there is only one dT(0,0) for the image, we
 *  can apply the resulting measurement to the rest of the pixels in the image.
 *  If dT(0,0) is not a free parameter, then the fitting process is linear in
 *  terms of dT(x,y) and f'(x,y)
 */

/// Shutter correction parameters, applicable for a single pixel
typedef struct {
    double scale;                       ///< The normalisation for an exposure, A(k) or f'(x,y)
    double offset;                      ///< The time offset, dTk
    double offref;                      ///< The reference time offset, dTo
    int num;                            ///< Number of points used
    float stdev;                        ///< Standard deviation
    bool valid;                         // is the fitted shutter correction valid (produce a significant improvement?)
} pmShutterCorrection;

/// Allocator for shutter correction parameters
pmShutterCorrection *pmShutterCorrectionAlloc(void);

/// Guess a shutter correction, based on plot of counts vs exposure time
///
/// This function is used before doing the full non-linear fit, to get parameters close to the true.  Assumes
/// exptime vector is sorted (ascending order; longest is last) prior to input.
pmShutterCorrection *pmShutterCorrectionGuess(
    const psVector *exptime,            ///< Exposure times for each exposure
    const psVector *counts              ///< Counts for each exposure
    );

/// Generate shutter correction based on a linear fit
///
/// Performs a linear fit to counts as a function of exposure time, with the reference time offset fixed (so
/// that the system is linear).  Performs iterative clipping, if nIter > 1.
pmShutterCorrection *pmShutterCorrectionLinFit(
    const psVector *exptime,            ///< Exposure times for each exposure
    const psVector *counts,             ///< Counts for each exposure
    const psVector *cntError,           ///< Error in the counts
    const psVector *mask,               ///< Mask for each exposure
    float offref,                       ///< Reference time offset
    int nIter,                          ///< Number of iterations
    float rej                           ///< Rejection threshold (sigma)
    );

/// Generate shutter correction based on a full non-linear fit
///
/// Performs a full non-linear fit to counts as a function of exposure time.  The main purpose is to solve for
/// the reference time offset, so that future fits may be performed using linear fitting with the reference
/// time offset fixed.
pmShutterCorrection *pmShutterCorrectionFullFit(
    const psVector *exptime,            ///< Exposure times for each exposure
    const psVector *counts,             ///< Counts for each exposure
    const psVector *cntError,           ///< Error in the counts
    const pmShutterCorrection *guess    ///< Initial guess
    );

/// Measure a shutter correction image from an array of images
///
/// Given an array of readouts (with known exposure times from the cell concepts), this function measures the
/// shutter correction (our principal concern is for the time offset, rather than the normalisation) by
/// measuring the reference time offset using the full non-linear fit for a small number of representative
/// regions (middle and corners), and then using that to perform a linear fit to each pixel.
bool pmShutterCorrectionMeasure(
    pmReadout *output,                  ///< Output readout
    const psArray *readouts,            ///< Array of readouts
    int size,                           ///< Size of samples for statistics for non-linear fit
    psStatsOptions meanStat,            ///< Statistic to use for mean
    psStatsOptions stdevStat,           ///< Statistic to use for stdev
    int nIter,                          ///< Number of iterations
    float rej,                          ///< Rejection threshold (sigma)
    psImageMaskType maskVal                  ///< Mask value
    );

/// Thread entry point for applying a shutter correction
bool pmShutterCorrectionApplyScan_Threaded(
    psThreadJob *job                    ///< Job to execute
    );

/// Apply the shutter correction to a scan
bool pmShutterCorrectionApplyScan(
    psImage *image,                     ///< Input image to correct
    psImage *mask,                      ///< Input mask image
    psImage *var,                       ///< Input variance image
    const psImage *shutterImage,        ///< Shutter correction image
    float exptime,                      ///< Exposure time to which to correct
    psImageMaskType blank,                   ///< Mask value to give blank pixels
    int rowStart, int rowStop           ///< Range of scan
    );

/// Apply a shutter correction
///
/// Given a shutter correction (with dT for each pixel), applies this correction to an input image.
bool pmShutterCorrectionApply(
    pmReadout *readout,                 ///< Readout to which to apply shutter correction
    const pmReadout *shutter,           ///< Shutter correction readout, with dT for each pixel
    psImageMaskType blank                    ///< Value to give blank pixels
    );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Functions for doing the shutter correction piece-meal (don't have to read entire image stack into memory at
// once).  A single read run through the stack is required, calling pmShutterCorrectionAddReadout on each.
// Then pmShutterCorrectionReference provides the required reference shutter time, so that
// pmShutterCorrectionGenerate can generate a shutter correction piece by piece as overlapping pixels from
// each input are read in.


/// Data for measuring the shutter correction
typedef struct {
    int num;                            ///< Number of images
    int numCols, numRows;               ///< Size of images
    psArray *regions;                   ///< Regions at which to measure statistics
    psArray *mean;                      ///< Vector of means at each region
    psArray *stdev;                     ///< Vector of standard deviations at each region
    psVector *exptimes;                 ///< Exposure times for each image
    psVector *refs;                     ///< Reference fluxes
} pmShutterCorrectionData;

/// Allocator for pmShutterCorrectionData
pmShutterCorrectionData *pmShutterCorrectionDataAlloc(int numCols, int numRows, ///< Size of images
                                                      int size ///< Size of regions
    );

/// Add a readout to the correction data
///
/// Performs statistics on the readout, recording the data
bool pmShutterCorrectionAddReadout(
    pmShutterCorrectionData *data,      ///< Correction data
    const pmReadout *readout,           ///< Readout to add
    psStatsOptions meanStat,            ///< Statistic to use for mean
    psStatsOptions stdevStat,           ///< Statistic to use for stdev
    psImageMaskType maskVal,                 ///< Mask value
    psRandom *rng                       ///< Random number generator
    );

/// Calculate the reference shutter time from the correction data
float pmShutterCorrectionReference(
    pmShutterCorrectionData *data ///< Correction data
    );

/// Generate a shutter correction
///
/// Performs the linear fit to each pixel in the stack.
bool pmShutterCorrectionGenerate(
    pmReadout *shutter,                 ///< Shutter correction
    pmReadout *pattern,                 ///< Background pattern (or NULL)
    const psArray *inputs,              ///< Stack of input pmReadouts
    float reference,                    ///< Reference shutter time (from pmShutterCorrectionRef)
    const pmShutterCorrectionData *data, ///< Correction data
    int nIter,                          ///< Number of iterations
    float rej,                          ///< Rejection threshold (sigma)
    psImageMaskType maskVal                  ///< Mask value
    );

// prepare outputs for shutter correction
bool pmShutterCorrectionGeneratePrepare(pmReadout *shutter, pmReadout *pattern, const psArray *inputs,
                                        psImageMaskType maskVal);

/// @}
#endif

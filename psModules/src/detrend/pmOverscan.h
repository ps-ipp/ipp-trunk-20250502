/* @file pmOverscan.h
 * @brief Functions to subtract the overscan, used by pmBiasSubtract
 *
 * @author George Gusciora, MHPCC
 * @author Paul Price, IfA
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-08-15 20:21:18 $
 * Copyright 2004--2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_OVERSCAN_H
#define PM_OVERSCAN_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// Type of fit to perform
typedef enum {
    PM_FIT_NONE,                        ///< No fit
    PM_FIT_POLY_ORD,                    ///< Fit ordinary polynomial
    PM_FIT_POLY_CHEBY,                  ///< Fit Chebyshev polynomial
    PM_FIT_SPLINE                       ///< Fit cubic splines
} pmFit;

/// Options for overscan subtraction
///
/// The overscan subtraction may be performed by reducing all overscan regions to a single value (e.g., if
/// there is no structure); or the overscan may be fit perpendicular to the read direction (usually the
/// columns) with a particular functional form; or a single value may be subtracted for each read/scan without
/// fitting (if the structure defies characterisation).  In any case, statistics are required to reduce
/// multiple values to a single value (either for the scan, or for the entire overscan regions).
typedef struct
{
    // Inputs
    bool single;                        ///< Reduce all overscan regions to a single value?
    bool constant;			///< use a supplied constant value (do not measure region)
    float value;			///< supplied value if needed (per above)
    pmFit fitType;                      ///< Type of fit to overscan
    unsigned int order;                 ///< Order of polynomial, or number of spline pieces
    psStats *stat;                      ///< Statistic to use when reducing the minor direction
    int boxcar;                         ///< Boxcar smoothing radius
    float gauss;                        ///< Gaussian smoothing sigma
    float minValid;			///< if overscan is too low, readout is dead : mask
    float maxValid;			///< if overscan is too high, readout is dead : mask
    psImageMaskType maskVal;            ///< Mask value to give dead readouts

    // Outputs
    psPolynomial1D *poly;               ///< Result of polynomial fit
    psSpline1D *spline;                 ///< Result of spline fit
}
pmOverscanOptions;

/// Allocator for overscan options
pmOverscanOptions *pmOverscanOptionsAlloc(bool single, ///< Reduce all overscan regions to a single value?
                                          pmFit fitType, ///< Type of fit to overscan
                                          unsigned int order, ///< Order of polynomial, or number of splines
                                          psStats *stat, ///< Statistic to use
                                          int boxcar, ///< Boxcar smoothing radius
                                          float gauss ///< Gaussian smoothing sigma
                                         );

psVector *pmOverscanVector(float *chi2, // chi^2 from fit
			   pmOverscanOptions *overscanOpts, // Overscan options
			   const psArray *pixels, // Array of vectors containing the pixel values
			   psStats *myStats // Statistic to use in reducing the overscan
    );

bool pmOverscanUpdateHeader (pmHDU *hdu, pmOverscanOptions *overscanOpts, float chi2);

bool pmOverscanSubtract (pmReadout *input, pmOverscanOptions *overscanOpts);

/// @}
#endif


/* @file  pmMoments.h
 * @brief Definitions of the moments structure
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-10-03 20:59:16 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_MOMENTS_H
# define PM_MOMENTS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmMoments data structure
 *
 * One of the simplest measurements which can be made quickly for an object
 * are the object moments. We specify a structure to carry the moment information
 * for a specific source:
 *
 */
typedef struct
{
    float Mrf;    ///< radial first moment
    float Mrh;    ///< radial half moment

    float Mx;     ///< X-coord of centroid.
    float My;     ///< Y-coord of centroid.
    float Mxx;    ///< x-second moment = sigma_x^2 = = (FWHM_x/2.355)^2
    float Mxy;    ///< xy cross moment = sigma_xy
    float Myy;    ///< y-second moment = sigma_y^2 = = (FWHM_y/2.355)^2

    float Mxxx;    ///< third moment
    float Mxxy;    ///< third moment
    float Mxyy;    ///< third moment
    float Myyy;    ///< third moment

    float Mxxxx;   ///< fourth moment
    float Mxxxy;   ///< fourth moment
    float Mxxyy;   ///< fourth moment
    float Mxyyy;   ///< fourth moment
    float Myyyy;   ///< fourth moment

  // float wSum;    ///< window-weighted sum (NOT needed by lensing)

    float Sum;    ///< Pixel sum above sky (background).
    float Peak;   ///< Peak counts above sky.
    float Sky;    ///< Sky level (background).
    float dSky;   ///< local Sky variance
    float SN;     ///< approx signal-to-noise
    int nPixels;  ///< Number of pixels used.

    float KronFluxPSF; ///< Kron Flux using PSF-optimized window
    float KronFluxPSFErr; ///< Kron Flux Error using PSF-optimized window
    float KronRadiusPSF; ///< Kron Radius using PSF-optimized window (Flux in 2.5 Radius)

    float KronCore;    ///< flux in r < 1.0*Mrf
    float KronCoreErr;    ///< error on flux in r < 1.0*Mrf

    float KronFlux;    ///< Kron flux (flux in r < 2.5*Mrf)
    float KronFluxErr; ///< Kron flux error

    float KronFinner;    ///< Kron flux (flux in 1.0*Mrf < r < 2.5*Mrf)
    float KronFouter;    ///< Kron flux (flux in 2.5*Mrf < r < 4.0*Mrf)
}
pmMoments;

/** pmMomentsAlloc()
 *
 */
pmMoments *pmMomentsAlloc(void);

/// @}
# endif

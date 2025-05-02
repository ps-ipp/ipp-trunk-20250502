/** @file  psEarthOrientation.h
*
*  @brief earth orientation calculations and transformation
*
*  @author Josh Hoblitt, IfA
*  @author Ed Pier, IfA
*  @author Eugene Magnier, IfA
*  @author Robert Daniel DeSonia, MHPCC
*
*  @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-08-09 01:40:07 $
*
*  Copyright 2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifndef PS_EARTH_ORIENTATION
#define PS_EARTH_ORIENTATION

/// @addtogroup Astro Astronomy
/// @{

#include "psCoord.h"
#include "psTime.h"
#include "psSphereOps.h"

/** Structure for respresenting the Earth's pole at any moment or determine velocity
 *  at the pole.  This structure carries the polar coordinate information.       */
typedef struct
{
    double x;                          ///< X component of the earth's pole
    double y;                          ///< Y component of the earth's pole
    double s;                          ///< s component of the earth's pole
}
psEarthPole;

/** Method for spherical precession used to specify the level of detail used in
 *  calculation.
 *
 *  @see psSpherePrecess
 *
 */
typedef enum {
    PS_PRECESS_ROUGH,                  ///< roughest, lowest level of detail
    PS_PRECESS_COMPLETE_A,             ///< complete level of detail using IERS A
    PS_PRECESS_COMPLETE_B,             ///< complete level of detail using IERS B
    PS_PRECESS_IAU2000A                ///< highest level of detail
}
psPrecessMethod;

/** Initialize EOC.
 *
 *  Parses IERS table data and creates the polynomials used by certain EOC functions.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool p_psEOCInit(void);

/** Finalize EOC after using functions that make calls to eocInit for time table data
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool p_psEOCFinalize(void);

/** Allocates a new psEarthPole structure.  */
psEarthPole *psEarthPoleAlloc(void) PS_ATTR_MALLOC;

/** Calculates the apparent position of a star, given its actual position and the
 *  velocity vector of the observer.
 *
 *  The actual and apparent positions are represented as psSphere entries, as is the
 *  direction of motion.  The speed in that direction is given in units of the speed
 *  of light.  If the value of apparent is NULL, a new psSphere is allocated, otherwise
 *  the point to apparent is used for the result.
 *
 *  @return psSphere*:      the actual position of a star.
 */
psSphere *psAberration(
    psSphere *apparent,                ///< apparent position of star
    const psSphere *actual,            ///< actual position of star
    const psSphere *direction,         ///< direction of motion of observer
    double speed                       ///< speed of motion of observer
);

/** Calculates the apparent position of a star, given its actual position and the
 *  position of the sun.
 *
 *  The actual and apparent positions are represented as psSphere entries, as is
 *  position of the sun.  If the value of apparent is NULL, a new psSphere is allocated,
 *  otherwise the point to apparent is used for the result.
 *
 *  @return psSphere*:      the apparent position of a star.
 */
psSphere *psGravityDeflection(
    psSphere *apparent,                ///< apparent position of star
    psSphere *actual,                  ///< actual position of star
    psSphere *sunPos                      ///< position of the sun
);

/** Calculates the components of the rotation between the CEO and GCRS frames, X, Y,
 *  and s, using the IAU2000A precession & nutation model.
 *
 *  The input time may be represented in any format other than UT1.  This routine must
 *  give results identical to the IERS XYS2000A subroutine.
 *
 *  @return psEarthPole*:       the calculated components of the rotation.
 */
psEarthPole *psEOC_PrecessionModel(
    const psTime *time                 ///< specified time
);

/** Provides interpolated corrections to the X and Y components of the polar
 *  coordinates from the tables provided by the IERS, just as it does for UT1 and
 *  polar motion.
 *
 *  @return psEarthPole*:       interpolated corrections to the precession components.
 */
psEarthPole *psEOC_PrecessionCorr(
    const psTime *time,                ///< specified time
    psTimeBulletin bulletin            ///< IERS tables for polar coordinate components.
);

/** Constructs the spherical rotation for transforming from CEO to GCRS coordinates.
 *
 *  The resulting psSphereRot may be used to determine the rotation from CIP/CEO to
 *  GCRS.  This function must give results identical to the IERS BPN2000.
 *
 *  @return psSphereRot*:       spherical rotation for CEO to GCRS transformation.
 */
psSphereRot *psSphereRot_CEOtoGCRS(
    const psEarthPole *pole            ///< input coordinates to transform
);

/** Calculates the rotation of the Earth about the CIP.
 *
 *  If tidalCorr is non-NULL, use the S-component to provide tidal corrections to the
 *  UT1 time.  If tidalCorr is NULL, no corrections are made & this step is skipped.
 *
 *  @return psSphereRot*:       spherical rotation of the Earth about the CIP.
 */
psSphereRot *psSphereRot_TEOtoCEO(
    const psTime *time,                ///< specified time
    psEarthPole *tidalCorr             ///< UT1 polar tide correction or NULL
);

/** Provides interpolated values of the polar motion components extracted from the
 *  IERS tables.
 *
 *  @return psEarthPole*:       interpolated polar motion components.
 */
psEarthPole *psEOC_GetPolarMotion(
    const psTime *time,                ///< specified time
    psTimeBulletin bulletin            ///< IERS tables for polar coordinate components.
);

/** Provides tidal corrections to the polar motion components using the Ray model
 *  of Simon et al.
 *
 *  @return psEarthPole*:       corrected polar motion components.
 */
psEarthPole *psEOC_PolarTideCorr(
    const psTime *time                 ///< specified time
);

/** Provides the additional corrections due to nutation terms with periods less than
 *  or equal to two days, as well as the correction to the s-prime component of polar
 *  motion.
 *
 *  @return psEarthPole*:       corrected polar motion components.
 */
psEarthPole *psEOC_NutationCorr(
    psTime *time                       ///< specified time
);

/** Converts the polar motion corrections to a spherical rotation.
 *
 *  This function should give identical results to the IERS POM2000 subroutine.
 *
 *  @return psSphereRot*:       ITRS to TEO sphere rotation.
 */
psSphereRot *psSphereRot_ITRStoTEO(
    const psEarthPole *motion          ///< corrected polar motion components
);

/** Generates the complete spherical rotation to account for precession
 *  between two times.  The equinoxes shall be Julian equinoxes.
 *
 *  If NULL is provided for either time, it is assumed to have the reference
 *  equinox value of J2000.  The mode argument is used to specify the level of
 *  detail used in the calculation.
 *
 *  @return psSphere*:       the resulting spherical rotation
 */
psSphereRot* psSpherePrecess(
    const psTime *fromTime,            ///< equinox of coords input
    const psTime *toTime,              ///< equinox of coords output
    psPrecessMethod mode               ///< level of detail to use
);


/// @}
#endif // #ifndef PS_EARTH_ORIENTATION

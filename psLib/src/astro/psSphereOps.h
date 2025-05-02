/** @file  psSphereOps.h
 *
 *  @brief Contains spherical rotation and offset operations
 *
 *  @author Robert DeSonia, MHPCC
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-09 01:40:07 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_SPHERE_H
#define PS_SPHERE_H

/// @addtogroup Astro Astronomy
/// @{

# include "psTime.h"

/** Spherical Rotation Definition
 *
 *  We need to be able to convert between ICRS, Galactic and Ecliptic
 *  coordinates, and potentially between arbitrary spherical coordinate
 *  systems. All of these basic spherical transformations represent rotations
 *  of the spherical coordinate reference.
 *
 */
typedef struct
{
    double q0;
    double q1;
    double q2;
    double q3;
}
psSphereRot;

/** Mode for Offset calculation between two sky positions
 *
 *  @see  psSphereGetOffset, psSphereSetOffset
 *
 */
typedef enum {
    PS_SPHERICAL,                      ///< offset corresponds to an angular offset
    PS_LINEAR                          ///< offset corresponds to a linear offset
} psSphereOffsetMode;

/** The units of the offset
 *
 *  @see  psSphereGetOffset, psSphereSetOffset
 *
 */
typedef enum {
    PS_ARCSEC,                         ///< Arcseconds
    PS_ARCMIN,                         ///< Arcminutes
    PS_DEGREE,                         ///< Degrees
    PS_RADIAN                          ///< Radians
} psSphereOffsetUnit;


/** Allocator for psSphereRot
 *
 *  @return psSphereRot*         newly allocated psSphereRot
 */

psSphereRot* psSphereRotAlloc(
    double alphaP,                     ///< north pole latitude
    double deltaP,                     ///< north pole longitude
    double phiP                        ///< defines the longitude in the input system of the equatorial intersection between the two systems (e.g, the first point of Ares).
) PS_ATTR_MALLOC;

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psSphereRot structure, false otherwise.
 */
bool psMemCheckSphereRot(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Allocator for psSphereRot given quaternions.
 *
 *  Normalizes a given quaternion and returns the cooresponding newly allocated
 *  psSphereRot object.
 *
 *  @return psSphereRot*         newly allocated psSphereRot
 */
psSphereRot* psSphereRotQuat(
    double q0,                          ///< q0
    double q1,                          ///< q1
    double q2,                          ///< q2
    double q3                           ///< q3
);

/** Applies the psSphereTransform transform for a specified coordinate
 *
 *  @return psSphere*      resulting coordinate based on transform
 */
psSphere* psSphereRotApply(
    psSphere* out,                     ///< a psSphere to recycle.  If NULL, a new one is generated.
    const psSphereRot* transform,      ///< the transform to apply
    const psSphere* coord              ///< the coordinate to apply the transform above.x
);

/** Combines two rotations.
 *
 *  Combines two rotations to produce a single rotation which is the
 *  equivalent of applying the ï¬rst rotation and then the second. The output
 *  rotation may be supplied, or will be allocated if NULL.
 *
 *  @return psSphereRot*    New psSphereRot that is the combination of the input psSphereRots
 */
psSphereRot* psSphereRotCombine(
    psSphereRot* out,                  ///< a psSphereRot to recycle or NULL
    const psSphereRot* rot1,           ///< first rotation to combine
    const psSphereRot* rot2            ///< second rotation to combine
);

/** Returns the conjugate of a specified rotation.
 *
 *  Stores the conjugate of a specified rotation in an existing psSphereRot* or creates a
 *  new psSphereRot* if NULL.  The conjugate of a rotation given in quaternions is -q0,
 *  -q1, -q2, q3.
 *
 *  @return psSphereRot*    the Conjugate of the specified psSphereRot, in.
 */
psSphereRot* psSphereRotConjugate(
    psSphereRot *out,                  ///< a psSphereRot to recycle or NULL
    const psSphereRot *in              ///< the psSphereRot from which to obtain the conjugate
);

/** Inverts the rotation.
 *
 *  A given rotation is inverted by creating a psSphereRot using -phiP, -deltaP, -alphaP.
 *
 *  @return psSphereRot*    Inverted input psSphereRot
 */
psSphereRot* psSphereRotInvert(
    double alphaP,                      ///< north pole latitude
    double deltaP,                      ///< north pole longitude
    double phiP                         ///< defines the longitude in the input system of the equatorial intersection between the two systems (e.g, the first point of Ares).
);

/** Determines the offset (RA,Dec) on the sky between two positions.
 *
 *  Both an offset mode and an offset unit may be defined. The mode may be
 *  either PS_SPHERICAL, in which case the specified offset corresponds to an
 *  offset in angles, or it may be PS_LINEAR, in which case the offset
 *  corresponds to a linear offset in a local projection. The offset unit may
 *  be in one of PS_ARCSEC, PS_ARCMIN, PS_DEGREE, and PS_RADIAN, which
 *  specifies the units of the offset only.
 *
 *  @return psSphere*        the offset between position1 and position2
 */
psSphere* psSphereGetOffset(
    const psSphere* position1,         ///< first position for calculating offset
    const psSphere* position2,         ///< second position for calculating offset
    psSphereOffsetMode mode,           ///< type of offset can be PS_SPHERICAL or PS_LINEAR
    psSphereOffsetUnit unit            ///< specifies the units of offset only
);

/** Applies the given offset to a coordinate.
 *
 *  Both an offset mode and an offset unit may be defined. The mode may be
 *  either PS_SPHERICAL, in which case the specified offset corresponds to an
 *  offset in angles, or it may be PS_LINEAR, in which case the offset
 *  corresponds to a linear offset in a local projection. The offset unit may
 *  be in one of PS_ARCSEC, PS_ARCMIN, PS_DEGREE, and PS_RADIAN, which
 *  specifies the units of the offset only.
 *
 *  @return psSphere*              the original position with the given offset applied.
 */
psSphere* psSphereSetOffset(
    const psSphere* position,          ///< coordinate of origin
    const psSphere* offset,            ///< coordinate of offset to apply
    psSphereOffsetMode mode,           ///< corresponds to an offset in angles or local projection
    psSphereOffsetUnit unit            ///< specifies the units of offset only
);

/** Creates the appropriate transform for converting from ICRS to Ecliptic
 *  coordinate systems.
 *
 *  @return psSphereRot*     transform for ICRS->Ecliptic coordinate systems
 */
psSphereRot* psSphereRotICRSToEcliptic(
    const psTime *time                 ///< the time for which the resulting transform will be valid
);

/** Creates the appropriate transform for converting from Ecliptic to ICRS
 *  coordinate systems.
 *
 *  @return psSphereRot*     transform for Ecliptic->ICRS coordinate systems
 */
psSphereRot* psSphereRotEclipticToICRS(
    const psTime *time                 ///< the time for which the resulting transform will be valid
);

/** Creates the appropriate transform for converting from ICRS to Galactic
 *  coordinate systems.
 *
 *  @return psSphereRot*        new sphere rotation for ICRS to Galactic transformations.
 */
psSphereRot* psSphereRotICRSToGalactic(void);

/** Creates the appropriate transform for converting from Galactic to ICRS
 *  coordinate systems.
 *
 *  @return psSphereRot*        new sphere rotation for Galactic to ICRS transformations.
 */
psSphereRot* psSphereRotGalacticToICRS(void);

/// @}

#endif // #ifndef PS_SPHERE_H

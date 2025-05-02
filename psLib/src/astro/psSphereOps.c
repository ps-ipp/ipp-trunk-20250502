/** @file  psSphereOps.c
 *
 *  @brief Contains spherical rotation and offset operations
 *
 *  @ingroup CoordinateTransform
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Dave Robbins, MHPCC
 *
 *  @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-01-09 22:38:52 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <float.h>

#include "psSphereOps.h"
#include "psType.h"
#include "psCoord.h"
#include "psMemory.h"
#include "psTime.h"
#include "psAssert.h"
#include "psError.h"
#include "psLogMsg.h"



// Modified Julian Day 01/01/1900 00:00:00
#define MJD_1900 15021.0

// Days in Julian century
#define JULIAN_CENTURY 36525.0

static void sphereRotFree(psSphereRot *result)
{
    // There are non dynamic allocated items
}

static psSphereRot* sphereRotAlloc(void)
{
    psSphereRot *sphereRot = (psSphereRot* ) psAlloc(sizeof(psSphereRot));
    psMemSetDeallocator(sphereRot, (psFreeFunc)sphereRotFree);
    return (sphereRot);
}

psSphereRot* psSphereRotAlloc(double alphaP,
                              double deltaP,
                              double phiP)
{
    if (isnan(alphaP) || isnan(deltaP) || isnan(phiP) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Input angles cannot be NaN in psSphereRotAlloc.");
        return NULL;
    }
    psSphereRot *r = sphereRotAlloc();
    psSphereRot *s = sphereRotAlloc();
    psSphereRot *t = sphereRotAlloc();

    //The following represents: r =a rotation about the z-axis by alphaP, s =a rotation about
    // the y-axis by deltaP, and t =a rotation about the z-axis by phiP.
    r->q0=0;
    r->q1=0;
    r->q2=sin(alphaP/2.0);
    r->q3=cos(alphaP/2.0);

    s->q0=0;
    s->q1=sin(deltaP/2.0);
    s->q2=0;
    s->q3=cos(deltaP/2.0);

    t->q0=0;
    t->q1=0;
    t->q2=sin(phiP/2.0);
    t->q3=cos(phiP/2.0);

    // calculate t*s*r.
    psSphereRot* temp = psSphereRotCombine(NULL,t,s);
    psSphereRot* result = psSphereRotCombine(NULL, temp, r);
    psFree(temp);
    psFree(r);
    psFree(s);
    psFree(t);
    return result;
}

bool psMemCheckSphereRot(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    //See if the ptr corresponds to a psSphereRot*
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)sphereRotFree );
}

//This function is really a second allocate function for psSphereRot's that uses quaternion
// component inputs instead of angle inputs as in psSphereRotAlloc.
psSphereRot* psSphereRotQuat(double q0,
                             double q1,
                             double q2,
                             double q3)
{
    if (isnan(q0) || isnan(q1) || isnan(q2) || isnan(q3) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Input quaternions cannot be NaN in psSphereRotQuat.");
        return NULL;
    }
    //allocate space for a new sphere rotation and set deallocator
    psSphereRot* rot = sphereRotAlloc();

    //The magnitude of a rotation quaternion should = 1 so we normalize here in case the
    // inputs have not been.
    double len = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    rot->q0 = q0 / len;
    rot->q1 = q1 / len;
    rot->q2 = q2 / len;
    rot->q3 = q3 / len;

    return rot;
}

psSphereRot* psSphereRotConjugate(psSphereRot *out,
                                  const psSphereRot *in)
{
    //if input sphere rotation is NULL, return NULL
    if (in == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psSphereRot input cannot be NULL.\n");
        return NULL;
    }
    //if output sphere rotation is NULL, allocate a new sphere rotation to return
    if (out == NULL) {
        out = sphereRotAlloc();
    }
    //Since q3 is the magnitude and q0,q1,q2 are direction, the conjugate rotation
    // is formed by -q0,-q1,-q2,+q3.  Note that this is the same as q0,q1,q2,-q3.
    out->q0 = -in->q0;
    out->q1 = -in->q1;
    out->q2 = -in->q2;
    out->q3 = in->q3;

    return out;
}

psSphere* psSphereRotApply(psSphere* out,
                           const psSphereRot* transform,
                           const psSphere* coord)
{
    //Make sure that input coordinates and rotation are not NULL
    PS_ASSERT_PTR_NON_NULL(transform, NULL);
    PS_ASSERT_PTR_NON_NULL(coord, NULL);
    //If output coordinates is NULL allocate a new psSphere
    if (out == NULL) {
        out = psSphereAlloc();
    }
    //apply the transform by creating a new psSphereRot from the input coord
    // and combining it with the input transform (see ADD)
    double cosD = cos(coord->d);
    psSphereRot* coordQuat = psSphereRotQuat(
                                 cosD*cos(coord->r),
                                 cosD*sin(coord->r),
                                 sin(coord->d),
                                 0.0);
    //Get the conjugate of the input rotation.  Need to calculate r*p*R to apply rotation
    // where R = r-conjugate.
    psSphereRot *conjugate = psSphereRotConjugate(NULL, transform);
    psSphereRot *temp = psSphereRotCombine(NULL, transform, coordQuat);
    psSphereRot *result = psSphereRotCombine(NULL, temp, conjugate);
    //From ADD we can find the new sphere coordinates,
    // r is calculated as tan^-1 (q1/q0), d is sin^-1 (q2)
    out->r = atan2(result->q1, result->q0);
    out->d = asin(result->q2);
    out->rErr = 0.0;
    out->dErr = 0.0;
    //Simply for convention, we make sure all output r-parameters are positive in the range
    // of 0 to 2pi
    if (out->r < -0.000001) {
        out->r += 2.0 * M_PI;
    }

    psFree(conjugate);
    psFree(temp);
    psFree(result);
    psFree(coordQuat);
    return out;
}

psSphereRot* psSphereRotCombine(psSphereRot* out,
                                const psSphereRot* rot1,
                                const psSphereRot* rot2)
{
    //Make sure that input rotations are not NULL
    PS_ASSERT_PTR_NON_NULL(rot1, NULL);
    PS_ASSERT_PTR_NON_NULL(rot2, NULL);
    //If output rotation is NULL, allocate a new psSphereRot to return
    if (out == NULL) {
        out = sphereRotAlloc();
    }

    double a0 = rot1->q0;
    double a1 = rot1->q1;
    double a2 = rot1->q2;
    double a3 = rot1->q3;
    double b0 = rot2->q0;
    double b1 = rot2->q1;
    double b2 = rot2->q2;
    double b3 = rot2->q3;

    //Combine rot1 & rot2.  Formulas here came from ADD.
    out->q0 = a3*b0 + a0*b3 + a1*b2 - a2*b1;
    out->q1 = a3*b1 - a0*b2 + a1*b3 + a2*b0;
    out->q2 = a3*b2 + a0*b1 - a1*b0 + a2*b3;
    out->q3 = b3*a3 - b2*a2 - b1*a1 - b0*a0;

    return out;
}

psSphereRot *psSphereRotInvert(double alphaP,
                               double deltaP,
                               double phiP)
{
    //This function should produce identical results to psSphereRotConjugate in most
    // or possibly all situations.  Creates the inverse rotation from the input angles.
    return (psSphereRotAlloc(-phiP, -deltaP, -alphaP));
}

psSphereRot* psSphereRotEclipticToICRS(const psTime *time)
{
    psF64 T;
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    // Convert psTime to MJD
    psF64 MJD = psTimeToMJD(time);
    // Check the specified MJD is greater than 1900
    if ( MJD < MJD_1900 ) {
        psError(PS_ERR_BAD_PARAMETER_TYPE,true,_("Specified time is less than 1900."));
        return NULL;
    }
    // Calculate number of Julian centuries since 1900
    T = ( MJD - MJD_1900 ) / JULIAN_CENTURY;
    //Formulas for phiP, deltaP, alphaP came from ADD.
    psF64 phiP = - DEG_TO_RAD(270.0);
    psF64 deltaP = - (DEG_TO_RAD(23.0) +
                      MIN_TO_RAD(27.0) +
                      SEC_TO_RAD(8.26) -
                      (SEC_TO_RAD(46.845) * T) -
                      (SEC_TO_RAD(0.0059) * T * T) +
                      (SEC_TO_RAD(0.00181) * T * T * T));
    psF64 alphaP = - DEG_TO_RAD(90.0);

    return (psSphereRotAlloc(alphaP, deltaP, phiP));
}

psSphereRot* psSphereRotICRSToEcliptic(const psTime *time)
{
    psF64 T;
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    // Convert psTime to MJD
    psF64 MJD = psTimeToMJD(time);
    // Check the specified MJD is greater than 1900
    if ( MJD < MJD_1900 ) {
        psError(PS_ERR_BAD_PARAMETER_TYPE,true,_("Specified time is less than 1900."));
        return NULL;
    }
    // Calculate number of Julian centuries since 1900
    T = ( MJD - MJD_1900 ) / JULIAN_CENTURY;

    //Formulas for phiP, deltaP, alphaP came from ADD.  Notice that the formulas are the
    // same as for EclipticToICRS except that alpha=-phiP, deltaP=-deltaP, phiP=-alphaP to
    // produce the inverse rotation as in psSphereRotInverse.
    psF64 alphaP = DEG_TO_RAD(270.0);
    psF64 deltaP = DEG_TO_RAD(23.0) +
                   MIN_TO_RAD(27.0) +
                   SEC_TO_RAD(8.26) -
                   (SEC_TO_RAD(46.845) * T) -
                   (SEC_TO_RAD(0.0059) * T * T) +
                   (SEC_TO_RAD(0.00181) * T * T * T);
    psF64 phiP = DEG_TO_RAD(90.0);

    return (psSphereRotAlloc(alphaP, deltaP, phiP));
}

psSphereRot* psSphereRotGalacticToICRS(void)
{
    //Formulas for alphaP, deltaP, phiP came from the ADD for ICRSToGalactic.  Notice that
    // this is the reason the inverse gets allocated and returned here.
    psF64 alphaP = DEG_TO_RAD(180.0-192.85948);
    psF64 deltaP = DEG_TO_RAD(90.0 - 27.12825);
    psF64 phiP = DEG_TO_RAD(90.0+32.93192);
    return (psSphereRotAlloc(-phiP,-deltaP,-alphaP));
}

psSphereRot* psSphereRotICRSToGalactic(void)
{
    //Formulas for alphaP, deltaP, phiP came from ADD.
    psF64 alphaP = DEG_TO_RAD(180.0-192.85948);
    psF64 deltaP = DEG_TO_RAD(90.0 - 27.12825);
    psF64 phiP = DEG_TO_RAD(90.0+32.93192);

    return (psSphereRotAlloc(alphaP, deltaP, phiP));
}

//Calculates the difference between coordinates in position1 & 2 and returns this offset.
psSphere* psSphereGetOffset(const psSphere* position1,
                            const psSphere* position2,
                            psSphereOffsetMode mode,
                            psSphereOffsetUnit unit)
{
    //Make sure that input coordinates are not NULL & that mode & unit are valid
    PS_ASSERT_PTR_NON_NULL(position1, NULL);
    PS_ASSERT_PTR_NON_NULL(position2, NULL);
    // Check positions near 90 degree and issue warnings if necessary
    if (position1->d >= DEG_TO_RAD(90.0)) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: psDeproject(): position1->d is larger than 90 degrees.  Returning NULL.");
        return NULL;
    }
    if (position2->d >= DEG_TO_RAD(90.0)) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: psDeproject(): position2->d is larger than 90 degrees.  Returning NULL.");
        return NULL;
    }
    // Allocate return structure
    psSphere* tmp = psSphereAlloc();

    // Mode is LINEAR - Use first position as projection center and project second point
    // onto tangent plane, set point projected into psSphere structure x->r y->d
    if (mode == PS_LINEAR) {
        //The basic idea is to project both positions onto the linear plane, with position1
        // at the center, then calculate the linear offset between those projections.
        psProjection* proj = psProjectionAlloc(position1->r,
                                               position1->d,
                                               1.0,
                                               1.0,
                                               PS_PROJ_TAN);
        // Perform projection onto tangent plane
        psPlane* lin = psProject(NULL, position2, proj);
        // Set return values
        tmp->r = lin->x;
        tmp->d = lin->y;
        // Free data structures allocated
        psFree(proj);
        psFree(lin);

        // Mode is SPHERICAL - Get difference between positiion 1 and position 2 and convert
        // offset value from radians to desired units and return
    } else if (mode == PS_SPHERICAL) {
        tmp->r = position2->r - position1->r;
        tmp->d = position2->d - position1->d;

        // Wrap these to an acceptable range.  This assumes that all
        // angles are in radians.
        tmp->r = fmod(tmp->r, 2*M_PI);
        tmp->d = fmod(tmp->d, 2*M_PI);
        tmp->rErr = 0.0;
        tmp->dErr = 0.0;

        // Convert output to desired units
        if (unit == PS_ARCSEC) {
            tmp->r = RAD_TO_SEC(tmp->r);
            tmp->d = RAD_TO_SEC(tmp->d);
        } else if (unit == PS_ARCMIN) {
            tmp->r = RAD_TO_MIN(tmp->r);
            tmp->d = RAD_TO_MIN(tmp->d);
        } else if (unit == PS_DEGREE) {
            tmp->r = RAD_TO_DEG(tmp->r);
            tmp->d = RAD_TO_DEG(tmp->d);
        } else if (unit == PS_RADIAN) {}
        else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified units, 0x%x, is not supported."),
                    unit);
            psFree(tmp);
            return NULL;
        }

        // Invalid mode
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified offset mode, 0x%x, is not supported."),
                mode);
        psFree(tmp);
        return NULL;
    }

    // Return value
    return tmp;
}

//Applies the specified offset to the input position coordinates and returns the new position.
psSphere* psSphereSetOffset(const psSphere* position,
                            const psSphere* offset,
                            psSphereOffsetMode mode,
                            psSphereOffsetUnit unit)
{
    //Make sure that input coordinates are not NULL & that mode & unit are valid
    PS_ASSERT_PTR_NON_NULL(position, NULL);
    PS_ASSERT_PTR_NON_NULL(offset, NULL);

    psSphere* tmp;
    psF64 tmpR = 0.0;
    psF64 tmpD = 0.0;

    // If mode is linear then set position to projection center
    // and offset to linear coordinate then deproject to obtain
    // new sphere coordinate
    if (mode == PS_LINEAR) {
        // Allocate plane coordinate and set coordinate
        psPlane*  lin = psPlaneAlloc();
        lin->x = offset->r;
        lin->y = offset->d;
        // Allocate and set projection structure
        psProjection* proj = psProjectionAlloc(position->r,
                                               position->d,
                                               1.0,
                                               1.0,
                                               PS_PROJ_TAN);
        // Project tangent plane coord to spherical coord
        tmp = psDeproject(NULL, lin, proj);
        // Free data structures used
        psFree(proj);
        psFree(lin);

        // If mode is spherical then convert offset to radians, add the offset
        // to the position and wrap to 0 to 2pi
    } else if (mode == PS_SPHERICAL) {
        // Convert offset unit to radians
        if (unit == PS_ARCSEC) {
            tmpR = SEC_TO_RAD(offset->r);
            tmpD = SEC_TO_RAD(offset->d);
        } else if (unit == PS_ARCMIN) {
            tmpR = MIN_TO_RAD(offset->r);
            tmpD = MIN_TO_RAD(offset->d);
        } else if (unit == PS_DEGREE) {
            tmpR = DEG_TO_RAD(offset->r);
            tmpD = DEG_TO_RAD(offset->d);
        } else if (unit == PS_RADIAN) {
            tmpR = offset->r;
            tmpD = offset->d;
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified units, 0x%x, is not supported."),
                    unit);
            return NULL;
        }

        // Allocate sphere structure to return
        tmp = psSphereAlloc();

        // Add offset and wrap to 0 to 2PI if necessary
        tmp->r = position->r + tmpR;
        tmp->r = fmod(tmp->r, 2.0*M_PI);
        tmp->d = position->d + tmpD;
        tmp->d = fmod(tmp->d, 2.0*M_PI);
        tmp->rErr = 0.0;
        tmp->dErr = 0.0;

        // Invalid mode report error
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified offset mode, 0x%x, is not supported."),
                mode);
        return NULL;
    }

    return tmp;
}



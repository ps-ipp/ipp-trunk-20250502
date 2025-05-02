/**
 *  C Implementation: tap_psSphereOps_all
 *
 * Description:  Tests for psSphereRotAlloc, psMemCheckSphereRot, psSphereRotQuat,
 *                         psSphereRotApply, psSphereRotCombine, psSphereRotConjugate,
 *                         psSphereRotInvert, psSphereGetOffset, psSphereSetOffset,
 *                         psSphereRotICRSToEcliptic, psSphereRotEclipticToICRS,
 *                         psSphereRotICRSToGalactic, psSphereRotGalacticToICRS
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"

#define DEG_INC   30.0
#define MJD_1900  15021.0        // Modified Julian Day 1/1/1900 00:00:00
#define MJD_2000  51544.0        // Modified Julian Day 1/1/2000 00:00:00
#define MJD_2100  88069.0        // Modified Julian Day 1/1/2100 00:00:00
#define ERROR_TOL   0.0001
#define ALPHA_P 4*M_PI/3
#define DELTA_P M_PI/4
#define PHI_P M_PI/3

static void testSphereRotCreate(void);
static void testSphereRotConvert(void);
static void testSphereOffsets(void);

int main(void)
{
    psLogSetFormat("HLNM");
    plan_tests(86);
    testSphereRotCreate();
    testSphereRotConvert();
    testSphereOffsets();
}

void testSphereRotCreate(void)
{
    // ----------------------------------------------------------------
    //Tests for psSphereRotAlloc
    //Return NULL for NAN input alpha
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotAlloc(NAN, DELTA_P, PHI_P);
        ok( s1 == NULL, "psSphereRotAlloc(): return NULL for NAN input alpha");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NAN input delta
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotAlloc(ALPHA_P, NAN, PHI_P);
        ok( s1 == NULL, "psSphereRotAlloc(): return NULL for NAN input delta");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NAN input phi
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotAlloc(ALPHA_P, DELTA_P, NAN);
        ok( s1 == NULL, "psSphereRotAlloc(): return NULL for NAN input phi");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    double a0 = (ALPHA_P - PHI_P)/2.0;
    double a1 = (ALPHA_P - PHI_P)/2.0;
    double a2 = (ALPHA_P + PHI_P)/2.0;
    double a3 = (ALPHA_P + PHI_P)/2.0;
    //From Mathworld, this is another way to calculate the quaternions of a rotation
    double q0 = sin(a0)*sin(DELTA_P/2);
    double q1 = cos(a1)*sin(DELTA_P/2);
    double q2 = sin(a2)*cos(DELTA_P/2);
    double q3 = cos(a3)*cos(DELTA_P/2);
    // Test psSphereRot() for valid inputs
    {
        psMemId id = psMemGetId();
        psSphereRot* myST = NULL;
        myST = psSphereRotAlloc(ALPHA_P, DELTA_P, PHI_P);
        ok(myST != NULL && psMemCheckSphereRot(myST),
            "psSphereRotAlloc:  return allocated SphereRot for valid inputs.");
        is_double(q0, myST->q0, "psSphereRotAlloc: return correct q0 value.");
        is_double(q1, myST->q1, "psSphereRotAlloc: return correct q1 value.");
        is_double(q2, myST->q2, "psSphereRotAlloc: return correct q2 value.");
        is_double(q3, myST->q3, "psSphereRotAlloc: return correct q3 value.");
        psFree(myST);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------
    //Tests for psMemCheckSphereRot
    //Make sure psMemCheckSphereRot works correctly - return false
    //for non-psSphereRot input
    {
        psMemId id = psMemGetId();
        int j = 2;
        ok( !psMemCheckSphereRot(&j), "psMemCheckSphereRot: return false for non-SphereRot input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------
    //Tests for psSphereRotQuat
    //Return NULL for NAN input q0
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotQuat(NAN, q1, q2, q3);
        ok( s1 == NULL, "psSphereRotQuat: return NULL for NAN input q0");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NAN input q1
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotQuat(q0, NAN, q2, q3);
        ok( s1 == NULL, "psSphereRotQuat: return NULL for NAN input q1");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NAN input q2
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotQuat(q0, q1, NAN, q3);
        ok( s1 == NULL, "psSphereRotQuat: return NULL for NAN input q2");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NAN input q3
    {
        psMemId id = psMemGetId();
        psSphereRot *s1 = psSphereRotQuat(q0, q1, q2, NAN);
        ok( s1 == NULL, "psSphereRotQuat: return NULL for NAN input q3");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid psSphereRot (allocated) for valid inputs
    psSphereRot* myST = psSphereRotQuat(q0*2.0, q1*2.0, q2*2.0, q3*2.0);
    {
        psMemId id = psMemGetId();
        ok( myST != NULL && psMemCheckSphereRot(myST),
            "psSphereRotQuat: return allocated SphereRot for valid inputs.");
        is_double(q0, myST->q0, "psSphereRotQuat: return correct q0 value.");
        is_double(q1, myST->q1, "psSphereRotQuat: return correct q1 value.");
        is_double(q2, myST->q2, "psSphereRotQuat: return correct q2 value.");
        is_double(q3, myST->q3, "psSphereRotQuat: return correct q3 value.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


void testSphereRotConvert(void)
{
    // Allocate data structure
    psSphereRot* myST = NULL;
    psSphereRot* s1 = NULL;
    psSphereRot *s2 = NULL;
    myST = psSphereRotAlloc(ALPHA_P, DELTA_P, PHI_P);
    psSphere *sphere = psSphereAlloc();
    sphere->r = 0.0;
    sphere->d = 0.0;
    psSphere *out = NULL;
    psSphere *out2 = NULL;
    psTime *pre1900 = psTimeFromMJD(MJD_1900 - 100.0);
    psTime *mjd2k = psTimeFromMJD(MJD_2000);
    psSphere* icrs = psSphereAlloc();
    psSphere* galactic = NULL;
    icrs->r = DEG_TO_RAD(0.0);
    icrs->d = DEG_TO_RAD(90.0);


    // ---------------------------------------------------------
    //Tests for psSphereRotCombine
    //Return NULL for NULL sphere input
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotCombine(s1, myST, NULL);
        ok( s1 == NULL, "psSphereRotCombine: return NULL for NULL input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NULL transform input
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotCombine(s1, NULL, myST);
        ok( s1 == NULL, "psSphereRotCombine: return NULL for NULL input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // XXX: Must test with legitimate input data.


    // ---------------------------------------------------------
    //Tests for psSphereRotApply
    //Return NULL for NULL sphere input
    {
        psMemId id = psMemGetId();
        out = psSphereRotApply(out, myST, NULL);
        ok( out == NULL, "psSphereRotApply: return NULL for NULL sphere input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for NULL transform input
    {
        psMemId id = psMemGetId();
        out = psSphereRotApply(out, NULL, sphere);
        ok( out == NULL, "psSphereRotApply: return NULL for NULL transform input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ---------------------------------------------------------
    //Tests for psSphereRotConjugate
    //Return NULL for NULL inputs
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotConjugate(s1, NULL);
        ok(s1 == NULL, "psSphereRotConjugate: return NULL SphereRot for NULL inputs");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    //Return correct conjugate for valid inputs
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotConjugate(s1, myST);
        skip_start(s1 == NULL, 1, "Skipping 1 tests because psSphereRot is NULL!");
        out = psSphereRotApply(out, myST, sphere);
        skip_start(out == NULL || out->r == 0, 2,
                  "Skipping 1 tests because psSphereRotApply failed!");
        out2 = psSphereRotApply(out2, s1, out);
        is_double( out2->r, 0.0, "psSphereRotConjugate: return correct SphereRot values.");
        is_double( out2->d, 0.0, "psSphereRotConjugate: return correct SphereRot values.");
        skip_end();
        psFree(s1);
        psFree(out);
        psFree(out2);
        s1 = NULL;
        out = NULL;
        out2 = NULL;
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ---------------------------------------------------------
    //Tests for psSphereRotInvert
    //Return correct inversion for valid inputs
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotInvert(-PHI_P, -DELTA_P, -ALPHA_P);
        is_double(s1->q0, myST->q0, "psSphereRotInvert: return correct q0 value.");
        is_double(s1->q1, myST->q1, "psSphereRotInvert: return correct q1 value.");
        is_double(s1->q2, myST->q2, "psSphereRotInvert: return correct q2 value.");
        is_double(s1->q3, myST->q3, "psSphereRotInvert: return correct q3 value.");
        psFree(s1);
        s1 = NULL;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ---------------------------------------------------------
    //Tests for psSphereRotICRSToEcliptic
    //Return NULL for NULL input
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotICRSToEcliptic(NULL);
        ok( s1 == NULL, "psSphereRotICRSToEcliptic: return NULL SphereRot for NULL inputs.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for Pre-MJD_1900 Date
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotICRSToEcliptic(pre1900);
        ok( s1 == NULL, "psSphereRotICRSToEcliptic: return NULL SphereRot for pre-1900 MJD date.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct SphereRot for MJD_2000
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotICRSToEcliptic(mjd2k);
        skip_start(  s1 == NULL, 2, "Skipping 2 tests because psSphereRot is NULL!");
        icrs->r = DEG_TO_RAD(0.0);
        icrs->d = DEG_TO_RAD(90.0);
        psSphere* ecliptic = psSphereRotApply(NULL, s1, icrs);
        is_double_tol(DEG_TO_RAD(90.0), ecliptic->r, 0.00001,
                      "psSphereRotICRSToEcliptic: return correct SphereRot for MJD_2000 date.");
        is_double_tol(DEG_TO_RAD(66.560719), ecliptic->d, 0.00001,
                      "psSphereRotICRSToEcliptic: return correct SphereRot for MJD_2000 date.");
        psFree(ecliptic);
        skip_end();
        psFree(s1);
        s1 = NULL;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------
    //Tests for psSphereRotEclipticToICRS
    //Return NULL for NULL input
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotEclipticToICRS(NULL);
        ok( s1 == NULL, "psSphereRotEclipticToICRS: return NULL SphereRot for NULL inputs.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for Pre-MJD_1900 Date
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotEclipticToICRS(pre1900);
        ok( s1 == NULL, "psSphereRotEclipticToICRS: return NULL SphereRot for pre-1900 MJD date.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct SphereRot for MJD_2000
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotEclipticToICRS(mjd2k);
        skip_start(  s1 == NULL, 2, "Skipping 2 tests because psSphereRot is NULL!");
        psSphere* ecliptic = psSphereRotApply(NULL, s1, icrs);
        s2 = psSphereRotICRSToEcliptic(mjd2k);
        psSphere* fromEcliptic = psSphereRotApply(NULL, s2, ecliptic);
        //XXX:  Pretty sure the following can be 180 degrees OR 0 degrees. (= the poles)
        is_double_tol(DEG_TO_RAD(180.0), fromEcliptic->r, 0.00001,
                      "psSphereRotEclipticToICRS: return correct SphereRot for MJD_2000 date.");
        is_double_tol(DEG_TO_RAD(90.0), fromEcliptic->d, 0.00001,
                      "psSphereRotEclipticToICRS: return correct SphereRot for MJD_2000 date.");
        psFree(ecliptic);
        psFree(fromEcliptic);
        skip_end();
        psFree(s1);
        s1 = NULL;
        psFree(s2);
        s2 = NULL;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------
    //Tests for psSphereRotICRSToGalactic
    //Return correct transformation
    {
        psMemId id = psMemGetId();
        s2 = psSphereRotICRSToGalactic();
        galactic = psSphereRotApply(NULL, s2, icrs);
        is_double_tol(DEG_TO_RAD(122.93192), galactic->r, 0.00001,
                      "psSphereRotICRSToGalactic: return correct SphereRot.");
        is_double_tol(DEG_TO_RAD(27.12825), galactic->d, 0.00001,
                      "psSphereRotICRSToGalactic: return correct SphereRot.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------
    //Tests for psSphereRotGalacticToICRS
    //Return correct transformation
    {
        psMemId id = psMemGetId();
        s1 = psSphereRotGalacticToICRS();
        psSphere *test = psSphereRotApply(NULL, s1, galactic);
        //XXX: Pretty sure the following can be 180 degrees OR 0 degrees. (= the poles)
        is_double_tol(DEG_TO_RAD(180.0), test->r, 0.00001,
                      "psSphereRotGalacticToICRS: return correct SphereRot.");
        is_double_tol(DEG_TO_RAD(90.0), test->d, 0.00001,
                      "psSphereRotGalacticToICRS: return correct SphereRot.");
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    {
        psFree(galactic);
        psFree(s2);
        psFree(s1);
        psFree(mjd2k);
        psFree(pre1900);
        psFree(icrs);
        psFree(sphere);
        psFree(myST);
    }

}


void testSphereOffsets(void)
{
    psSphereRot* myST = NULL;
    myST = psSphereRotAlloc(ALPHA_P, DELTA_P, PHI_P);

    psSphere *origin = psSphereAlloc();
    psSphere *offset = psSphereAlloc();
    psSphere *empty = NULL;
    psSphere *output = NULL;

    // -----------------------------------------------------------
    //Tests for psSphereGetOffset
    //Test Get for NULL position1
    {
        output = psSphereGetOffset(empty, origin, PS_LINEAR, PS_RADIAN);
        ok( output == NULL, "psSphereGetOffset: return NULL for NULL position1 input.");
    }


    //Test Get for NULL position2
    {
        output = psSphereGetOffset(origin, empty, PS_LINEAR, PS_RADIAN);
        ok( output == NULL, "psSphereGetOffset: return NULL for NULL position2 input.");
    }


    //Return NULL for position1 declination > 90 degrees
    {
        origin->d = 100.0;
        output = psSphereGetOffset(origin, offset, PS_LINEAR, PS_RADIAN);
        ok( output == NULL, "psSphereGetOffset: return NULL for position1->d > 90 degrees.");
    }


    //Return NULL for position1 declination > 90 degrees
    {
        origin->d = 0.0;
        offset->d = 100.0;
        output = psSphereGetOffset(origin, offset, PS_LINEAR, PS_RADIAN);
        ok( output == NULL, "psSphereGetOffset: return NULL for position2->d > 90 degrees.");
    }


    //Return NULL for invalid mode specification
    {
        offset->d = 0.0;
        output = psSphereGetOffset(origin, offset, -1, PS_RADIAN);
        ok( output == NULL, "psSphereGetOffset: return NULL for invalid mode input.");
    }


    //Return NULL for invalid unit specification
    {
        output = psSphereGetOffset(origin, offset, PS_SPHERICAL, -1);
        ok( output == NULL, "psSphereGetOffset: return NULL for invalid unit input.");
    }
    //Return matching sphere offsets regardless of units
    {
        offset->r = (M_PI / 4.0);     //45 deg
        offset->d = (M_PI / 6.0);     //30 deg
        output = psSphereGetOffset(origin, offset, PS_SPHERICAL, PS_ARCMIN);
        empty = psSphereGetOffset(origin, offset, PS_SPHERICAL, PS_DEGREE);
        is_double_tol(DEG_TO_RAD(empty->r), MIN_TO_RAD(output->r), 0.0001,
                      "psSphereGetOffset: return correct offset for differing units.");
        is_double_tol(DEG_TO_RAD(empty->d), MIN_TO_RAD(output->d), 0.0001,
                      "psSphereGetOffset: return correct offset for differing units.");
        psFree(output);
        psFree(empty);
    }

    //Tests for psSphereSetOffset
    output = NULL;
    empty = NULL;
    //Return NULL for NULL position
    {
        output = psSphereSetOffset(empty, offset, PS_SPHERICAL, PS_DEGREE);
        ok( output == NULL, "psSphereSetOffset: return NULL for NULL position input.");
    }
    //Return NULL for NULL offset
    {
        output = psSphereSetOffset(offset, empty, PS_SPHERICAL, PS_DEGREE);
        ok( output == NULL, "psSphereSetOffset: return NULL for NULL offset input.");
    }
    //Return NULL for invalid mode specification
    {
        output = psSphereSetOffset(origin, offset, -1, PS_RADIAN);
        ok( output == NULL, "psSphereSetOffset: return NULL for for invalid mode input.");
    }
    //Return NULL for invalid unit specification
    {
        output = psSphereSetOffset(origin, offset, PS_SPHERICAL, PS_RADIAN+1);
        ok( output == NULL, "psSphereSetOffset: return NULL for for invalid unit input.");
    }
    //Test Set using Spherical mode, Degree units
    origin->r = 0.0;
    origin->d = 0.0;
    offset->r = 45.0;
    offset->d = 30.0;
    {
        output = psSphereSetOffset(origin, offset, PS_SPHERICAL, PS_DEGREE);
        skip_start(output == NULL, 2, "Skipping 2 tests because Sphere output is NULL!");
        is_double(output->r, M_PI/4.0, "psSphereSetOffset: return correct spherical offset.");
        is_double(output->d, M_PI/6.0, "psSphereSetOffset: return correct spherical offset.");
        skip_end();
        psFree(output);
    }
    //Test Set and Get using Linear mode, Radian units
    origin->r = 0.0;
    origin->d = 0.0;
    offset->r = 1.0;
    offset->d = 1.0;
    {
        output = psSphereSetOffset(origin, offset, PS_LINEAR, PS_RADIAN);
        empty = psSphereGetOffset(origin, output, PS_LINEAR, PS_RADIAN);
        skip_start(empty == NULL, 2, "Skipping 2 tests because Sphere output is NULL!");
        is_double_tol(empty->r, offset->r, 0.00001,
                      "psSphereGetOffset: return correct spherical offset.");
        is_double_tol(empty->d, offset->d, 0.00001,
                      "psSphereGetOffset: return correct spherical offset.");
        skip_end();
        psFree(output);
        psFree(empty);
    }
    //Test Set using SPHERICAL mode, Arcmin units
    origin->r = 0.0;
    origin->d = 0.0;
    offset->r = RAD_TO_MIN(M_PI / 4.0);     //45 deg
    offset->d = RAD_TO_MIN(M_PI / 6.0);     //30 deg
    {
        output = psSphereSetOffset(origin, offset, PS_SPHERICAL, PS_ARCMIN);
        //Test Get using Spherical mode, Arcsec units
        empty = psSphereGetOffset(origin, output, PS_SPHERICAL, PS_ARCSEC);
        empty->r = SEC_TO_RAD(empty->r);
        empty->d = SEC_TO_RAD(empty->d);
        skip_start(empty == NULL, 2, "Skipping 2 tests because Sphere output is NULL!");
        is_double_tol(empty->r, (M_PI / 4.0), 0.0001,
                      "psSphereGetOffset: return correct spherical offset.");
        is_double_tol(empty->d, (M_PI / 6.0), 0.0001,
                      "psSphereGetOffset: return correct spherical offset.");
        skip_end();
        psFree(output);
        psFree(empty);
    }
    //Return matching sphere offsets regardless of units
    {
        offset->r = RAD_TO_SEC(M_PI / 4.0);     //45 deg
        offset->d = RAD_TO_SEC(M_PI / 6.0);     //30 deg
        output = psSphereSetOffset(origin, offset, PS_SPHERICAL, PS_ARCSEC);
        offset->r = (M_PI / 4.0);     //45 deg
        offset->d = (M_PI / 6.0);     //30 deg
        empty = psSphereSetOffset(origin, offset, PS_SPHERICAL, PS_RADIAN);
        is_double_tol(empty->r, offset->r, 0.0001,
                      "psSphereSetOffset: return correct offset for differing units.");
        is_double_tol(empty->d, offset->d, 0.0001,
                      "psSphereSetOffset: return correct offset for differing units.");
        psFree(output);
        psFree(empty);
    }

    //Check for Memory leaks
    {
        psFree(origin);
        psFree(offset);
        psFree(myST);
        checkMem();
    }
}

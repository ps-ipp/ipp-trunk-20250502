/**
 *  C Implementation: tap_psEarthOrientation_motion
 *
 * Description:  Tests for psEarthPoleAlloc, psEOC_PrecessionModel, psSpherePrecess
 *                         psSphereRot_CEOtoGCRS, psSphereRot_TEOtoCEO,
 *                         psEOC_GetPolarMotion, psSphereRot_ITRStoTEO
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

static void testPrecessionModel(void);
static void testPolarMotion(void);
static void testSphereRots(void);
static void testSpherePrecess(void);
#define MJD_1900  15021.0        // Modified Julian Day 1/1/1900 00:00:00
#define MJD_2100  88069.0        // Modified Julian Day 1/1/2100 00:00:00

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(78);

    // Tests for psEarthOrientation Motion Functions
    // Initialize library internal structures
    psLibInit("pslib.config");

    testPrecessionModel();
    testPolarMotion();
    testSphereRots();
    testSpherePrecess();

    psLibFinalize();
    done();
}

void testPrecessionModel(void)
{
    // psEOC_PrecessionModel

    psEarthPole *ep = NULL;


    //Test for psEarthPoleAlloc
    //Return properly allocated psEarthPole
    {
        psMemId id = psMemGetId();
        ep = psEarthPoleAlloc();
        ok( ep != NULL, "psEarthPoleAlloc: return properly allocated psEarthPole.");
        psFree(ep);
        ep = NULL;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Tests for psEOC_PrecessionModel
    //Return NULL for NULL time input.
    {
        psMemId id = psMemGetId();
        ep = psEOC_PrecessionModel(NULL);
        ok( ep == NULL, "psEOC_PrecessionModel: return NULL for NULL time input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for UT1 time input.
    {
        psMemId id = psMemGetId();
        psTime *testTime = psTimeAlloc(PS_TIME_UT1);
        ep = psEOC_PrecessionModel(testTime);
        ok( ep == NULL, "psEOC_PrecessionModel: return NULL for UT1 time input.");
        psFree(testTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid time input.
    {
        psMemId id = psMemGetId();
        psTime *testTime = psTimeAlloc(PS_TIME_UTC);
        testTime->nsec = 2e9;
        ep = psEOC_PrecessionModel(testTime);
        ok( ep == NULL,
            "psEOC_PrecessionModel:          return NULL for invalid time input.");
        testTime->nsec = 0;
        psFree(testTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for failed eoc init - missing file
    //printf("\n filename = '%s' \n", p_psGetConfigFileName());
    //XXX: Use the PS_CONFIG_FILE_DEFAULT macro, or PS_CONFIG_FILE environment variable
    //to define where the real pslib config is.
    {
        psMemId id = psMemGetId();
        psTime *testTime = psTimeAlloc(PS_TIME_UTC);
        testTime->sec = 1049160600;
        testTime->nsec = 0;
        testTime->leapsecond = false;
        skip_start(rename("../../etc/pslib/pslib.config", "../../etc/pslib/pslib_config.bak"),
                   1, "Skipping 1 tests because file rename failed!");
        ep = psEOC_PrecessionModel(testTime);
        ok( ep == NULL,
            "psEOC_PrecessionModel:          return NULL for failed eoc init.");
        rename("../../etc/pslib/pslib_config.bak", "../../etc/pslib/pslib.config");
        skip_end();
        psFree(testTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid EarthPole for valid time input
    {
        psMemId id = psMemGetId();
        psTime *testTime = psTimeAlloc(PS_TIME_UTC);
        double x, y, s;
        x = 2.857175590089105e-4;
        y = 2.3968739377734732e-5;
        s = -1.3970066457904322e-8;
        ep = psEOC_PrecessionModel(testTime);
        skip_start(  ep == NULL, 3,
                     "Skipping 3 tests because psEarthPole is NULL!");
        is_double_tol(ep->x, x, 0.1,
                      "psEOC_PrecessionModel:          return valid EarthPole for valid inputs (x).");
        is_double_tol(ep->y, y, 0.1,
                      "psEOC_PrecessionModel:          return valid EarthPole for valid inputs (y).");
        is_double_tol(ep->s, s, 0.1,
                      "psEOC_PrecessionModel:          return valid EarthPole for valid inputs (s).");
        skip_end();
        psFree(testTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    /*
    //Tests for psMemCheckSphereRot
    //Make sure psMemCheckSphereRot works correctly - return false
    {
    int j = 2;
    ok( !psMemCheckSphereRot(&j),
    "psMemCheckSphereRot:            return false for non-SphereRot input.");
    skip_start(  s1 == NULL, 1,
    "Skipping 1 tests because psSphereRot is NULL!");
    skip_end();
    }
    */
    //Check for Memory leaks
    {
        psFree(ep);
        checkMem();
    }

}

void testPolarMotion(void)
{
    // psEOC_GetPolarMotion()

    psTime *in = psTimeAlloc(PS_TIME_UTC);
    in->sec = 1049160600;
    in->nsec = 0;
    in->leapsecond = false;
    psEarthPole *polarMotion = NULL;


    //Tests for psEOC_GetPolarMotion
    //Return NULL for NULL time input.
    {
        psMemId id = psMemGetId();
        polarMotion = psEOC_GetPolarMotion(NULL, PS_IERS_B);
        ok( polarMotion == NULL, "psEOC_GetPolarMotion: return NULL for NULL time input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid IERS table input
    {
        psMemId id = psMemGetId();
        polarMotion = psEOC_GetPolarMotion(NULL, PS_IERS_B+1);
        ok( polarMotion == NULL, "psEOC_GetPolarMotion: return NULL for invalid IERS table input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid time input.
    {
        psMemId id = psMemGetId();
        in->nsec = 2e9;
        polarMotion = psEOC_GetPolarMotion(in, PS_IERS_B);
        ok( polarMotion == NULL, "psEOC_GetPolarMotion: return NULL for invalid time input.");
        in->nsec = 0;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for failed eoc init - missing file
    //XXX: Use the PS_CONFIG_FILE_DEFAULT macro, or PS_CONFIG_FILE environment variable
    //to define where the real pslib config is.
    {
        psMemId id = psMemGetId();
        p_psEOCFinalize();
        skip_start(rename("../../etc/pslib/pslib.config", "../../etc/pslib/pslib_config.bak"),
                   1, "Skipping 1 tests because file rename failed!");
        polarMotion = psEOC_GetPolarMotion(in, PS_IERS_B);
        ok( polarMotion == NULL, "psEOC_GetPolarMotion: return NULL for failed eoc init.");
        rename("../../etc/pslib/pslib_config.bak", "../../etc/pslib/pslib.config");
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid EarthPole for valid time input - IERS B
    {
        psMemId id = psMemGetId();
        double x, y, s;
        x = -6.454389659777e-07;
        y = 2.112606414597e-06;
        s = 0.0;
        polarMotion = psEOC_GetPolarMotion(in, PS_IERS_B);
        skip_start(  polarMotion == NULL, 3,
                     "Skipping 3 tests because psEarthPole is NULL!");
        is_double_tol(polarMotion->x, x, 0.1,
                      "psEOC_GetPolarMotion: return valid EarthPole for valid inputs "
                      "(x) - IERS B.");
        is_double_tol(polarMotion->y, y, 0.1,
                      "psEOC_GetPolarMotion: return valid EarthPole for valid inputs "
                      "(y) - IERS B.");
        is_double_tol(polarMotion->s, s, 0.1,
                      "psEOC_GetPolarMotion: return valid EarthPole for valid inputs "
                      "(s) - IERS B.");
        skip_end();
        psFree(polarMotion);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid EarthPole for valid time input - IERS A
    {
        psMemId id = psMemGetId();
        double x, y, s;
        x = -6.45381397904e-07;
        y = 2.112819726698e-06;
        s = 0.0;
        polarMotion = psEOC_GetPolarMotion(in, PS_IERS_A);
        skip_start(  polarMotion == NULL, 3,
                     "Skipping 3 tests because psEarthPole is NULL!");
        is_double_tol(polarMotion->x, x, 0.1,
                      "psEOC_GetPolarMotion: return valid EarthPole for valid inputs "
                      "(x) - IERS A.");
        is_double_tol(polarMotion->y, y, 0.1,
                      "psEOC_GetPolarMotion: return valid EarthPole for valid inputs "
                      "(y) - IERS A.");
        is_double_tol(polarMotion->s, s, 0.1,
                      "psEOC_GetPolarMotion: return valid EarthPole for valid inputs "
                      "(s) - IERS A.");
        skip_end();
        psFree(polarMotion);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid EarthPole for valid time input - IERS A
    {
        psMemId id = psMemGetId();
        psTime *firstTime = psTimeFromMJD(41684.50);
        polarMotion = psEOC_GetPolarMotion(firstTime, PS_IERS_B);
        ok( polarMotion != NULL,
            "psEOC_GetPolarMotion: return valid EarthPole for valid inputs.");
        psFree(firstTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Check for Memory leaks
    {
        psFree(in);
        psFree(polarMotion);
        checkMem();
    }

}

static psSphere *objSetup(void)
{
    psSphere *obj = NULL;
    psCube *tempCube = psCubeAlloc();
    tempCube->x = -0.3598480726985338;
    tempCube->y = 0.5555012823608123;
    tempCube->z = 0.7496183628158023;
    obj = psCubeToSphere(tempCube);
    psFree(tempCube);
    return obj;
}

void testSphereRots(void)
{
    // psSphereRot Functions
    psSphereRot *out = NULL;
    psEarthPole *in = NULL;
    double q0,q1,q2,q3;
    q0 = -1.1984522406756289e-5;
    q1 = 1.4285893358610674e-4;
    q2 = 1.2191193518914336e-10;
    q3 = -0.9999999897238481;

    //Tests for psSphereRot_CEOtoGCRS
    //Return NULL for NULL earthPole input
    {
        psMemId id = psMemGetId();
        out = psSphereRot_CEOtoGCRS(in);
        ok( out == NULL, "psSphereRot_CEOtoGCRS: return NULL for NULL earthPole input.");
        in = psEarthPoleAlloc();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    in->x = 2.857175590089105e-4;
    in->y = 2.3968739377734732e-5;
    in->s = -1.3970066457904322e-8;
    //Return correct psSphereRot for valid input
    {
        psMemId id = psMemGetId();
        out = psSphereRot_CEOtoGCRS(in);
        skip_start(  out == NULL, 4,
                     "Skipping 4 tests because psSphereRot output is NULL!");
        is_double_tol( out->q0, q0, 0.0001,
                       "psSphereRot_CEOtoGCRS: return correct psSphereRot for valid input (q0).");
        is_double_tol( out->q1, q1, 0.0001,
                       "psSphereRot_CEOtoGCRS: return correct psSphereRot for valid input (q1).");
        is_double_tol( out->q2, q2, 0.0001,
                       "psSphereRot_CEOtoGCRS: return correct psSphereRot for valid input (q2).");
        is_double_tol( out->q3, -q3, 0.0001,
                       "psSphereRot_CEOtoGCRS: return correct psSphereRot for valid input (q3).");
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Tests for psSphereRot_TEOtoCEO
    psFree(out);
    psTime *time = psTimeAlloc(PS_TIME_UT1);
    psTime *time2 = psTimeAlloc(PS_TIME_UTC);
    time->sec = 1049160600-1;
    time->nsec = 657017200;
    time->leapsecond = false;
    psEarthPole *temp = psEarthPoleAlloc();
    temp->s = -2.0;
    //Return NULL for NULL time input
    {
        psMemId id = psMemGetId();
        out = psSphereRot_TEOtoCEO(NULL, NULL);
        ok( out == NULL, "psSphereRot_TEOtoCEO: return NULL for NULL time input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid time input - large nsec, UTC time type
    {
        psMemId id = psMemGetId();
        time2->nsec = 3e9;
        out = psSphereRot_TEOtoCEO(time2, NULL);
        ok( out == NULL, "psSphereRot_TEOtoCEO: return NULL for invalid time input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid time input - UT1 time type, large nsec
    {
        psMemId id = psMemGetId();
        psFree(time2);
        time2 = psTimeAlloc(PS_TIME_UT1);
        time2->nsec = 1e9;
        temp->s = 1.0;
        out = psSphereRot_TEOtoCEO(time2, temp);
        ok( out == NULL, "psSphereRot_TEOtoCEO: return NULL for invalid time input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid earthPole input
    {
        psMemId id = psMemGetId();
        time2->nsec = 0;
        psEarthPole *temp = psEarthPoleAlloc();
        temp->s = -2.0;
        out = psSphereRot_TEOtoCEO(time2, temp);
        ok( out == NULL, "psSphereRot_TEOtoCEO: return NULL for invalid earthPole input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs
    double x, y, z;
    x = 0.01698625430807123;
    y = -0.6616523084626379;
    z = 0.7496183628158023;
    {
        psMemId id = psMemGetId();
        psEarthPole *polarTideCorr = psEOC_PolarTideCorr(time);
        out = psSphereRot_TEOtoCEO(time, polarTideCorr);
        skip_start(  out == NULL, 3,
                     "Skipping 3 tests because psSphereRot output is NULL!");
        psSphere *obj = objSetup();
        psSphereRot *earthRot = psSphereRotConjugate(NULL, out);
        psSphere *result = psSphereRotApply(NULL, earthRot, obj);
        psCube *cube = psSphereToCube(result);
        is_double_tol( cube->x, x, 0.0001,
                       "psSphereRot_TEOtoCEO: return NULL for NULL time input. (x)");
        is_double_tol( cube->y, y, 0.0001,
                       "psSphereRot_TEOtoCEO: return NULL for NULL time input. (y)");
        is_double_tol( cube->z, z, 0.0001, "psSphereRot_TEOtoCEO: return NULL for NULL time input. (z)");
        psFree(earthRot);
        psFree(result);
        psFree(cube);
        psFree(obj);
        skip_end();
        psFree(polarTideCorr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Tests for psSphereRot_ITRStoTEO
    //Return NULL for NULL earthPole input
    {
        psMemId id = psMemGetId();
        psFree(out);
        out = psSphereRot_ITRStoTEO(NULL);
        ok( out == NULL,
            "psSphereRot_ITRStoTEO:         return NULL for NULL earthPole input.");
        in = psEarthPoleAlloc();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid input
    {
        psMemId id = psMemGetId();
        q0 = -1.0567571848664005e-6;
        q1 = 3.218036562931509e-7;
        q2 = -3.3580195807204483e-12;
        q3 = -0.9999999999993899;
        in->x = SEC_TO_RAD(-0.13275353774074533);
        in->y = SEC_TO_RAD(0.4359436319739848);
        in->s = SEC_TO_RAD(-4.2376965863576153e-10);
        out = psSphereRot_ITRStoTEO(in);
        skip_start(  out == NULL, 4,
                     "Skipping 4 tests because psSphereRot output is NULL!");
        is_double_tol( out->q0, q0, 0.0001,
                       "psSphereRot_ITRStoTEO: return correct psSphereRot for valid input (q0).");
        is_double_tol( out->q1, q1, 0.0001,
                       "psSphereRot_ITRStoTEO: return correct psSphereRot for valid input (q1).");
        is_double_tol( out->q2, q2, 0.0001,
                       "psSphereRot_ITRStoTEO: return correct psSphereRot for valid input (q2).");
        is_double_tol( out->q3, q3, 0.0001,
                       "psSphereRot_ITRStoTEO: return correct psSphereRot for valid input (q3).");
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    //Check for Memory leaks
    {
        psFree(temp);
        psFree(time2);
        psFree(time);
        psFree(out);
        psFree(in);
        checkMem();
    }
}

#define SPHERE_PRECESS_TP1_R            0.0               //    0.0       degrees
#define SPHERE_PRECESS_TP1_D            0.0               //    0.0       degrees
#define SPHERE_PRECESS_TP1_EXPECT_R     6.238453          //  357.437     degrees
#define SPHERE_PRECESS_TP1_EXPECT_D    -0.019426          //   -1.113     degrees
#define SPHERE_PRECESS_TP2_R            0.0               //    0.0       degrees
#define SPHERE_PRECESS_TP2_D            1.570796          //   90.0       degrees
#define SPHERE_PRECESS_TP2_EXPECT_R     6.260828          //  358.719     degrees
#define SPHERE_PRECESS_TP2_EXPECT_D     1.551353          //   88.886     degrees
#define SPHERE_PRECESS_TP3_R            3.141593          //  180.0       degrees
#define SPHERE_PRECESS_TP3_D            0.523599          //   30.0       degrees
#define SPHERE_PRECESS_TP3_EXPECT_R     3.096616          //  177.423     degrees
#define SPHERE_PRECESS_TP3_EXPECT_D     0.543024          //   31.113     degrees
#define ERROR_TOL   0.0001

void testSpherePrecess(void)
{
    // psSpherePrecess
    psSphereRot *rot = NULL;
    psTime *fromTime = NULL;
    psTime *toTime = NULL;
    psSphere* inputCoord  = psSphereAlloc();
    psSphere* outputCoord = NULL;


    //Return NULL for NULL time inputs
    {
        psMemId id = psMemGetId();
        rot = psSpherePrecess(fromTime, toTime, PS_PRECESS_ROUGH);
        ok( rot == NULL, "psSpherePrecess: return NULL for NULL time inputs.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for invalid mode input
    {
        psMemId id = psMemGetId();
        fromTime    = psTimeFromMJD(MJD_2100);
        toTime      = psTimeFromMJD(MJD_1900);
        rot = psSpherePrecess(fromTime, toTime, -1);
        ok( rot == NULL, "psSpherePrecess: return NULL for invalid mode input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs, mode = ROUGH
    {
        psMemId id = psMemGetId();
        inputCoord->r = SPHERE_PRECESS_TP1_R;
        inputCoord->d = SPHERE_PRECESS_TP1_D;
        inputCoord->rErr = 0.0;
        inputCoord->dErr = 0.0;

        rot = psSpherePrecess(fromTime, toTime, PS_PRECESS_ROUGH);
        outputCoord = psSphereRotApply(NULL, rot, inputCoord);
        if (outputCoord->r < -0.0001)
            outputCoord->r += 2.0 * M_PI;
        skip_start( rot == NULL || outputCoord == NULL, 2,
                    "Skipping 2 tests because psSphereRot output is NULL!");
        is_double_tol( outputCoord->r, SPHERE_PRECESS_TP1_EXPECT_R, ERROR_TOL,
                       "psSpherePrecess: return correct psSphereRot for valid"
                       " inputs and PS_PRECESS_ROUGH mode. (r)");
        is_double_tol( outputCoord->d, SPHERE_PRECESS_TP1_EXPECT_D, ERROR_TOL,
                       "psSpherePrecess: return correct psSphereRot for valid"
                       " inputs and PS_PRECESS_ROUGH mode. (d)");
        skip_end();
        psFree(outputCoord);
        psFree(rot);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs, mode = ROUGH
    {
        psMemId id = psMemGetId();
        inputCoord->r = SPHERE_PRECESS_TP2_R;
        inputCoord->d = SPHERE_PRECESS_TP2_D;
        inputCoord->rErr = 0.0;
        inputCoord->dErr = 0.0;

        rot = psSpherePrecess(fromTime, toTime, PS_PRECESS_ROUGH);
        outputCoord = psSphereRotApply(NULL, rot, inputCoord);
        if (outputCoord->r < -0.0001)
            outputCoord->r += 2.0 * M_PI;
        skip_start( rot == NULL || outputCoord == NULL, 2,
                    "Skipping 2 tests because psSphereRot output is NULL!");
        is_double_tol( outputCoord->r, SPHERE_PRECESS_TP2_EXPECT_R, ERROR_TOL,
                       "psSpherePrecess: return correct psSphereRot for valid"
                       " inputs and PS_PRECESS_ROUGH mode. (r)");
        is_double_tol( outputCoord->d, SPHERE_PRECESS_TP2_EXPECT_D, ERROR_TOL,
                       "psSpherePrecess: return correct psSphereRot for valid"
                       " inputs and PS_PRECESS_ROUGH mode. (d)");
        skip_end();
        psFree(outputCoord);
        psFree(rot);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs, mode = ROUGH
    {
        psMemId id = psMemGetId();
        inputCoord->r = SPHERE_PRECESS_TP3_R;
        inputCoord->d = SPHERE_PRECESS_TP3_D;
        inputCoord->rErr = 0.0;
        inputCoord->dErr = 0.0;

        rot = psSpherePrecess(fromTime, toTime, PS_PRECESS_ROUGH);
        outputCoord = psSphereRotApply(NULL, rot, inputCoord);
        if (outputCoord->r < -0.0001)
            outputCoord->r += 2.0 * M_PI;
        skip_start( rot == NULL || outputCoord == NULL, 2,
                    "Skipping 2 tests because psSphereRot output is NULL!");
        is_double_tol( outputCoord->r, SPHERE_PRECESS_TP3_EXPECT_R, ERROR_TOL,
                       "psSpherePrecess: return correct psSphereRot for valid"
                       " inputs and PS_PRECESS_ROUGH mode. (r)");
        is_double_tol( outputCoord->d, SPHERE_PRECESS_TP3_EXPECT_D, ERROR_TOL,
                       "psSpherePrecess: return correct psSphereRot for valid"
                       " inputs and PS_PRECESS_ROUGH mode. (d)");
        skip_end();
        psFree(outputCoord);
        psFree(rot);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs, mode = COMPLETE_A
    {
        psMemId id = psMemGetId();
        rot = psSpherePrecess(NULL, toTime, PS_PRECESS_COMPLETE_A);
        outputCoord = psSphereRotApply(NULL, rot, inputCoord);
        ok( outputCoord != NULL && rot != NULL,
            "psSpherePrecess: return correct psSphereRot for valid"
            " inputs and PS_PRECESS_COMPLETE_A mode.");
        psFree(outputCoord);
        psFree(rot);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs, mode = COMPLETE_B
    {
        psMemId id = psMemGetId();
        rot = psSpherePrecess(NULL, toTime, PS_PRECESS_COMPLETE_B);
        outputCoord = psSphereRotApply(NULL, rot, inputCoord);
        ok( outputCoord != NULL && rot != NULL,
            "psSpherePrecess: return correct psSphereRot for valid"
            " inputs and PS_PRECESS_COMPLETE_B mode.");
        psFree(outputCoord);
        psFree(rot);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return correct psSphereRot for valid inputs, mode = IAU2000A
    {
        psMemId id = psMemGetId();
        rot = psSpherePrecess(NULL, toTime, PS_PRECESS_IAU2000A);
        outputCoord = psSphereRotApply(NULL, rot, inputCoord);
        ok( outputCoord != NULL && rot != NULL,
            "psSpherePrecess: return correct psSphereRot for valid"
            " inputs and PS_PRECESS_IAU2000A mode.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    //Check for Memory leaks
    {
        psFree(fromTime);
        psFree(toTime);
        psFree(outputCoord);
        psFree(rot);
        checkMem();
    }

}



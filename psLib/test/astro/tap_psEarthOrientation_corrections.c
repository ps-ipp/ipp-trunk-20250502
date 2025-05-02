/**
 *  C Implementation: tap_psEarthOrientation_corrections
 *
 * Description:  Tests for p_psEOCInit, p_psEOCFinalize, psAberration,
 *                         psGravityDeflection, psEOC_PrecessionCorr,
 *                         psEOC_PolarTideCorr, psEOC_NutationCorr,
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

static void testEOCInit(void);
static void testAberration(void);
static void testGravDef(void);
static void testEOC_Corrs(void);

#define MJD_1900  15021.0        // Modified Julian Day 1/1/1900 00:00:00
#define MJD_2100  88069.0        // Modified Julian Day 1/1/2100 00:00:00

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(8);

    // Initialize library internal structures
    psLibInit("pslib.config");

    testEOCInit();
    testAberration();
    testGravDef();
    testEOC_Corrs();
    //    testSphereOffsets();

    // Cleanup library
    psLibFinalize();

    done();
}

void testEOCInit(void)
{
//    diag("  >>>Test 1:  p_psEOCInit ");
    //Test for psEarthPoleAlloc
    //Return properly allocated psEarthPole
    /*    {
            skip_start(  ep == NULL, 3,
                         "Skipping 3 tests because psEarthPole is NULL!");
            ep = psEarthPoleAlloc();
            ok( ep != NULL,
                "psEarthPoleAlloc:               return properly allocated psEarthPole.");
            psFree(ep);
            ep = NULL;
        }
    */


    //Check for Memory leaks
    {
        checkMem();
    }

}

void testAberration(void)
{
    // psAberration()
    psSphere *apparent = NULL;
    psSphere *empty = NULL;
    psCube *actualCube = psCubeAlloc();
    //values from After gravity deflection//
    actualCube->x = -0.35961949760293604;
    actualCube->y = 0.5555613950298085;
    actualCube->z = 0.7496835020836093;
    psSphere *actual = psCubeToSphere(actualCube);
    psCube *cubeDir = psCubeAlloc();
    cubeDir->x = 5148.713262821658;
    cubeDir->y = -26945.04752348012;
    cubeDir->z = -11682.787302030947;
    cubeDir->x += -357.6031690489248;
    cubeDir->y += 248.46429758174693;
    cubeDir->z += 0.09694774143797581;
    psSphere *direction = psCubeToSphere(cubeDir);
    double speed = sqrt(cubeDir->x*cubeDir->x + cubeDir->y*cubeDir->y + cubeDir->z*cubeDir->z);
    double c = 299792458.0; // Speed of light in vacuum (src:NIST) m/s
    speed /= c;

    //Tests for psAberration
    //Return NULL for NULL actual sphere input
    {
        empty = psAberration(apparent, NULL, direction, speed);
        ok( empty == NULL,
            "psAberration:                   return NULL for NULL actual sphere input.");
    }
    //Return NULL for NULL direction sphere input
    {
        empty = psAberration(apparent, actual, NULL, speed);
        ok( empty == NULL,
            "psAberration:                   return NULL for NULL direction sphere input.");
    }
    //Return NULL for speed = 0.0
    {
        empty = psAberration(apparent, actual, direction, 0.0);
        ok( empty == NULL,
            "psAberration:                   return NULL for zero speed input.");
    }
    //Return NULL for speed > 1.0
    {
        empty = psAberration(apparent, actual, direction, 1.1);
        ok( empty == NULL,
            "psAberration:                   return NULL for impossible speed input (> c).");
    }

    //Return correct values for valid inputs
    {
        double x, y, z;
        x = -0.35963388069046304;
        y = 0.5555192509816625;
        z = 0.7497078321908413;
        apparent = psAberration(apparent, actual, direction, speed);
        skip_start(  apparent == NULL, 3,
                     "Skipping 3 tests because psEarthPole is NULL!");
        psCube *outCube = psSphereToCube(apparent);
        is_double_tol( outCube->x, x, 0.001,
                       "psAberration:                   return correct sphere for valid inputs.");
        is_double_tol( outCube->y, y, 0.001,
                       "psAberration:                   return correct sphere for valid inputs.");
        is_double_tol( outCube->z, z, 0.001,
                       "psAberration:                   return correct sphere for valid inputs.");
        psFree(outCube);
        skip_end();
    }

    //Check for Memory leaks
    {
        psFree(actualCube);
        psFree(cubeDir);
        psFree(apparent);
        psFree(actual);
        psFree(direction);
        checkMem();
    }

}

void testGravDef(void)
{
    // psGravityDeflection
    // Test for psGravityDeflection
    // Return properly allocated psEarthPole
    /* XXX: Fix this
        psSphere *apparent = NULL;
        psSphere *empty = NULL;
        psCube *actualCube = psCubeAlloc();
        actualCube->x = -0.3596195125758298;
        actualCube->y = 0.5555613903455866;
        actualCube->z = 0.7496834983724809;
        psSphere *actual = psCubeToSphere(actualCube);
        psCube *sunCube = psCubeAlloc();
        sunCube->x = 1.467797790127511e11;
        sunCube->y = 2.5880956908748722e10;
        sunCube->z = 1.1220046291457653e10;
        double sunLength = sqrt(sunCube->x*sunCube->x + sunCube->y*sunCube->y + sunCube->z*sunCube->z);
        sunCube->x /= sunLength;
        sunCube->y /= sunLength;
        sunCube->z /= sunLength;
        if (VERBOSE) {
        printf("sunCube = x,y,z = %.13g, %.13g, %.13g\n",
        sunCube->x, sunCube->y, sunCube->z);
    }
        psSphere *sun = psCubeToSphere(sunCube);
        if (VERBOSE) {
        printf("sunSphere  = r, d = %.13g, %.13g\n", sun->r, sun->d);
    }
        psCube *outCube = psCubeAlloc();
     
        empty = psGravityDeflection(apparent, empty, sun);
        if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
        "psGravityDeflection Failed to return NULL for NULL actual input sphere.\n");
        return 1;
    }
        empty = psGravityDeflection(apparent, actual, empty);
        if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
        "psGravityDeflection Failed to return NULL for NULL sun input sphere.\n");
        return 2;
    }
     
        apparent = psGravityDeflection(NULL, actual, sun);
        //    apparent->r *= -1.0;
        //    apparent->d *= -1.0;
        psSphere *result2;
        //    psSphere *result2 = psSphereSetOffset(actual, apparent, PS_SPHERICAL, PS_RADIAN);
        //    psSphere *result = psSphereSetOffset(actual, apparent, PS_SPHERICAL, PS_RADIAN);
        //    psSphere *result = psSphereGetOffset(apparent, actual, PS_SPHERICAL, PS_RADIAN);
        if (VERBOSE) {
        printf(" -- actualCube = x,y,z = %.13g, %.13g, %.13g  -- \n",
        actualCube->x, actualCube->y, actualCube->z);
    }
        //    psCube *outCube = psSphereToCube(result);
        //    printf(" -- resultCube = x,y,z = %.13g, %.13g, %.13g  -- \n",
        //           outCube->x, outCube->y, outCube->z);
        psCube *outCube2 = psSphereToCube(apparent);
        if (VERBOSE) {
        printf(" -- resultCube2= x,y,z = %.13g, %.13g, %.13g  -- \n",
        outCube2->x, outCube2->y, outCube2->z);
    }
        double x, y, z;
        x = -0.35961949760293604;
        y = 0.5555613950298085;
        z = 0.7496835020836093;
     
        if (VERBOSE) {
        printf(" -- expectCube = x,y,z = %.13g, %.13g, %.13g  -- \n\n", x, y, z);
     
            //    if ( fabs(x - outCube->x) > DBL_EPSILON || fabs(y - outCube->y) > DBL_EPSILON ||
            //            fabs(z - outCube->z) > DBL_EPSILON ) {
            //        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
            //                "psGravityDeflection returned incorrect values.\n");
        printf("expect-actual=  x,y,z  = %.13g, %.13g, %.13g \n",
        (x - actualCube->x), (y - actualCube->y), (z - actualCube->z) );
        printf("expect-result=  x,y,z  = %.13g, %.13g, %.13g \n",
        (x - outCube2->x), (y - outCube2->y), (z - outCube2->z) );
        printf("result-actual=  x,y,z  = %.13g, %.13g, %.13g \n",
        (outCube2->x - actualCube->x), (outCube2->y - actualCube->y),
        (outCube2->z - actualCube->z) );
    }
        //        return 1;
        //    }
        //    psFree(result2);
        outCube2->x = x;
        outCube2->y = y;
        outCube2->z = z;
        result2 = psCubeToSphere(outCube2);
        //    psFree(result);
        psSphere *result = psSphereGetOffset(actual, result2, PS_SPHERICAL, PS_RADIAN);
        psFree(result2);
        result2 = psSphereGetOffset(actual, apparent, PS_SPHERICAL, PS_RADIAN);
        if (VERBOSE) {
        printf("The apparent output sphere = r,d = %.13g, %.13g\n", result2->r, result2->d);
        printf("The expected output sphere = r,d = %.13g, %.13g\n\n", result->r, result->d);
    }
        psFree(result2);
     
        psFree(outCube);
        psFree(outCube2);
        psFree(sunCube);
        psFree(actualCube);
        psFree(result);
        psFree(actual);
        psFree(apparent);
        psFree(sun);
     
     
     
        {
            skip_start(  ep == NULL, 3,
                         "Skipping 3 tests because psEarthPole is NULL!");
            ep = psEarthPoleAlloc();
            ok( ep != NULL,
                "psEarthPoleAlloc:               return properly allocated psEarthPole.");
            psFree(ep);
            ep = NULL;
        }
    */


    //Check for Memory leaks
    {
        checkMem();
    }

}

void testEOC_Corrs(void)
{
    // psEOC Correction Functions");
    //Tests for psEOC_PrecessionCorr
    /* XXX: Fix this
        psTime *empty = NULL;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = timesec;
        time2->nsec = 0;
        time2->leapsecond = false;
        //    time2 = psTimeConvert(time2, PS_TIME_TAI);
     
        //Tests for Precession Correction function//
        //Return NULL for NULL time input
        psEarthPole *pcorr = NULL;
        // Following should generate error message
        pcorr = psEOC_PrecessionCorr(empty, PS_IERS_A);
        if (pcorr != NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                    "psEOC_PrecessionCorr failed to return NULL for NULL time input.\n");
            return 5;
        }
     
        //Return NULL for Invalid IERS table
        // Following should generate error message
        pcorr = psEOC_PrecessionCorr(time2, 3);
        if (pcorr != NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                    "psEOC_PrecessionCorr failed to return NULL for incorrect IERS table.\n");
            return 6;
        }
        psFree(pcorr);
     
        //Check values from IERS table A
        pcorr = psEOC_PrecessionCorr(time2, PS_IERS_A);
        if ( pcorr == NULL ) {
            psError(PS_ERR_BAD_PARAMETER_NULL, false,
                    "psEOC_PrecessionCorr returned NULL for valid inputs.\n");
            return 7;
        } else {
            if (VERBOSE) {
                printf("\nPrecessionCorr output (IERSA) = x,y,s = %.13g,  %.13g, %.13g\n",
                       pcorr->x, pcorr->y, pcorr->s);
            }
        }
        psFree(pcorr);
     
        //Check values from IERS table B
        pcorr = psEOC_PrecessionCorr(time2, PS_IERS_B);
        if ( pcorr == NULL ) {
            psError(PS_ERR_BAD_PARAMETER_NULL, false,
                    "psEOC_PrecessionCorr returned NULL for valid inputs.\n");
            return 9;
        } else {
            double xx, yy, ss;
            xx = 0.06295703125;
            yy = -0.0287618408203125;
            ss = 0.0;
            xx = SEC_TO_RAD(xx) * 1e-3;
            yy = SEC_TO_RAD(yy) * 1e-3;
            //        if ( fabs(pcorr->x - xx) > DBL_EPSILON || fabs(pcorr->y - yy) > DBL_EPSILON
            //                || fabs(pcorr->s - ss) > DBL_EPSILON) {
            //            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
            //                    "   psEOC_PrecessionCorr return incorrect values.\n");
            if (VERBOSE) {
                printf("PrecessionCorr output (IERSB) = x,y,s = %.13g, %.13g, %.13g\n",
                       pcorr->x, pcorr->y, pcorr->s);
                printf("Expected output               = x,y,s = %.13g, %.13g, %.13g\n", xx, yy, ss);
                printf("          A difference of:              %.13g,  %.13g, %.13g\n\n",
                       (pcorr->x - xx), (pcorr->y - yy), (pcorr->s - ss) );
            }
            //            return 10;
            //        }
        }
        //precess is the *actual* output from PrecessionModel + PrecessionCorr
        psEarthPole *precess = psEOC_PrecessionModel(time2);
        precess->x += pcorr->x;
        precess->y += pcorr->y;
        double xCorr, yCorr;
        xCorr = 3.05224300720406e-10;
        yCorr = -1.39441339235822e-10;
        //pcorr is the *expected* output from PrecessionModel//
        pcorr->x = 2.857175590089105e-4;
        pcorr->y = 2.3968739377734732e-5;
        pcorr->s = -1.3970066457904322e-8;
        pcorr->x += xCorr;
        pcorr->y += yCorr;
        psSphereRot *precessNutInv = psSphereRot_CEOtoGCRS(precess);
        psSphereRot *precessNut = psSphereRotConjugate(NULL, precessNutInv);
        double q0, q1, q2;
        q0 = -1.1984522406756289e-5;
        q1 = 1.4285893358610674e-4;
        q2 = 1.2191193518914336e-10;
        psSphereRot *pni = psSphereRot_CEOtoGCRS(pcorr);
        if (fabs(pni->q0-q0) > FLT_EPSILON || fabs(pni->q1-q1) > FLT_EPSILON ||
                fabs(pni->q2-q2) > FLT_EPSILON ) {
            printf("\n Error at CEOtoGCRS, output psSphereRot doesn't match expected.\n");
        }
        //    printf("  Output from CEOtoGCRS only  = %.13g,%.13g,%.13g,%.13g\n",
        //           pni->q0, pni->q1, pni->q2, pni->q3);
        if (VERBOSE) {
            printf("  Expected sphere rotation    = %.13g, %.13g, %.13g\n", q0,q1,q2);
            printf("  Result sphere rotation      = %.13g, %.13g, %.13g\n",
                   precessNutInv->q0, precessNutInv->q1, precessNutInv->q2);
            printf("     Difference         =        %.13g, %.13g, %.13g\n\n",
                   precessNutInv->q0-q0, precessNutInv->q1-q1, precessNutInv->q2-q2);
        }
        psCube *objC = psCubeAlloc();
        //    objC->x = -3.5963388069046304;
        //    objC->y = 0.5555192509816625;
        //    objC->z = 0.7497078321908413;
        //    objSetup();
     
        //This is the sphere rotation for the *expected* precession output//
        psSphereRot *pn = psSphereRotConjugate(NULL, pni);
     
        //    psSphere *sphere = psSphereAlloc();
        //    *sphere = *obj;
        //    psFree(obj);
     
        //create a psSphere for (from) the start position given in eoc_testing//
        objC->x = -0.35963388069046304;
        objC->y = 0.5555192509816625;
        objC->z = 0.7497078321908413;
        psSphere *sphere = psCubeToSphere(objC);
     
        psSphere *expect = psSphereRotApply(NULL, pn, sphere);
        //expected results below - stored in:  sphere  //
        double x,y,z;
        x = -0.3598480726985338;
        y = 0.5555012823608123;
        z = 0.7496183628158023;
        if (VERBOSE) {
            printf("\n<<Expected out       = x,y,z = %.13g, %.13g, %.13g\n", x, y, z);
        }
        //    psFree(objC);
        //    objC = psSphereToCube(expect);
        //printf("<<Expected out (CEO)  = x,y,z = %.13g, %.13g, %.13g\n", objC->x, objC->y, objC->z);
        //printf("     Difference     =           %.13g, %.13g, %.13g\n", objC->x-x, objC->y-y, objC->z-z);
        //    x = objC->x;
        //    y = objC->y;
        //    z = objC->z;
        psSphere *result = psSphereRotApply(NULL, precessNut, sphere);
        psFree(objC);
        objC = psSphereToCube(result);
        double xx = greatCircle(result, expect);
        if (VERBOSE) {
            printf("<<Resulting out      = x,y,z = %.13g, %.13g, %.13g\n", objC->x, objC->y, objC->z);
            printf("     Difference         =      %.13g, %.13g, %.13g\n\n",
                   objC->x-x, objC->y-y, objC->z-z);
            printf("GREAT CIRCLE DIFFERENCE = %.13g \n", xx);
        }
     
        psFree(precess);
        psFree(precessNut);
        psFree(precessNutInv);
        psFree(expect);
        psFree(objC);
     
        psFree(sphere);
        psFree(result);
        psFree(pn);
        psFree(pni);
        psFree(pcorr);
        psFree(time2);
        if (!p_psEOCFinalize() ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "EOC failed finalization!\n");
            return 12;
        }
     
    */

    //Tests for psEOC_PolarTideCorr
    /*
        psTime *in = psTimeAlloc(PS_TIME_UTC);
        in->sec = timesec;
        in->nsec = 0;
        in->leapsecond = false;
        psTime *empty = NULL;
        psEarthPole *eop = NULL;
     
        //Return NULL for NULL input time
        // Following should generate error message
        eop = psEOC_PolarTideCorr(empty);
        if (eop != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
        "psEOC_PolarTideCorr failed to return NULL for NULL input time.\n");
        return 1;
    }
     
        eop = psEOC_PolarTideCorr(in);
        if (eop == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
        "psEOC_PolarTideCorr returned NULL for valid input time.\n");
        return 2;
    } else {
        if (VERBOSE) {
        printf("\nPolarTideCorr output = x,y,s = %.13g, %.13g, %.13g\n",
        eop->x, eop->y, eop->s);
    }
    }
     
        psFree(in);
        psFree(eop);
     
    */



    //Tests for psEOC_NutationCorr
    /*
        psTime *in = psTimeAlloc(PS_TIME_UTC);
        in->sec = timesec;
        in->nsec = 0;
        in->leapsecond = false;
        psTime *empty = NULL;
        psEarthPole *nute = NULL;
     
        //Return NULL for NULL input time.
        // Following should generate error message
        nute = psEOC_NutationCorr(empty);
        if (nute != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
        "psEOC_NutationCorr failed to return NULL for NULL input time.\n");
        return 1;
    }
        //Return NULL for UT1 time input
        *//*    psTime *UT1time = psTimeAlloc(PS_TIME_UT1);
        nute = psEOC_NutationCorr(UT1time);
        if (nute != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
        "psEOC_NutationCorr failed to return NULL for UT1 input time.\n");
        return 2;
    }
        psFree(UT1time);
        *//*
        //Check return values from valid nutation time input
        nute = psEOC_NutationCorr(in);
        if ( nute == NULL ) {
            psError(PS_ERR_BAD_PARAMETER_NULL, false,
                    "psEOC_NutationCorr returned NULL for valid input.\n");
            return 3;
        } else {
            if (VERBOSE) {
                printf("Nutation Correction output = x,y,s = %.13g, %.13g, %.13g\n\n",
                       nute->x, nute->y, nute->s);
            }
        }
        psFree(nute);
        psFree(in);
     
        if (!p_psEOCFinalize() ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "EOC failed finalization!\n");
            return 12;
        }
     
    */
    //Return properly allocated psEarthPole
    /*
     
     
        {
            skip_start(  ep == NULL, 3,
                         "Skipping 3 tests because psEarthPole is NULL!");
            ep = psEarthPoleAlloc();
            ok( ep != NULL,
                "psEarthPoleAlloc:               return properly allocated psEarthPole.");
            psFree(ep);
            ep = NULL;
        }
    */


    //Check for Memory leaks
    {
        checkMem();
    }

}



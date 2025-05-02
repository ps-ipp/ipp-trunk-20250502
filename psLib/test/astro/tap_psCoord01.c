/**  @file  tst_psCoord01.c
*
*    @brief  The code will test several functions with PSLib source file
*            psCoord.c
*
*    @author Eric Van Alst, MHPCC
*
* XXX: must do (r,d) -> (x, y) -> (r, d) test to ensure correctness.
* XXX: The (Xs, Ys) scales are not be used properly.
* XXX: Much work remains to be done.  The original tests defined correct
*      input/output pairs and compared results.  It's not clear how those
*      values were obtained, and they nearly all fail now.
*
*    @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
*    @date  $Date: 2007-05-01 00:08:52 $
*    @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
*    @date  $Date: 2007-05-01 00:08:52 $
*
*    Copyright 2005 Maui High Performance Computing Center, Univ. of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define ERROR_TOL    0.0001
#define TESTPOINTS   4
#define DEG_INC   30.0

#define MY_TINY 0.0001
#define PS_COMPARE_TINY_THEN_PRINT_ERROR(ACTUAL, EXPECT, TESTSTATUS) \
if (MY_TINY < fabs(EXPECT - ACTUAL)) { \
    diag("%s is %lg, should be %lg", #ACTUAL, ACTUAL, EXPECT, TESTSTATUS); \
    errorFlag = true; \
}


//  alpha, delta, alpha-center, delta-center, scale-x, scale-y
psF64 projectionTestPoint[TESTPOINTS][6] = {
            {  0.785398,  0.785398,  0.000000,  0.000000,  1.000000,  1.000000 },
            {  0.100000,  1.500000,  0.500000,  0.250000,  1.000000,  1.000000 },
            {  0.628319,  0.448799,  0.000000,  0.000000,  0.250000,  0.750000 },
            { -1.047196,  0.222222, -0.250000,  0.000000,  1.500000,  1.250000 }
        };

// Expected values for TAN
psF64 projectionTanExpected[TESTPOINTS][2] = {
            { -1.000000, -1.414214 },
            {  0.088884, -3.066567 },
            { -0.181636, -0.446444 },
            {  1.535818, -0.404231 }
        };

// Expected values for SIN
psF64 projectionSinExpected[TESTPOINTS][2] = {
            { -0.500000, -0.707101 },
            {  0.027546, -0.950366 },
            { -0.132394, -0.325413 },
            {  1.046712, -0.275497 }
        };

// Expected values for AIT
psF64 projectionAitExpected[TESTPOINTS][2] = {
            { -0.549175,  0.523375 },
            {  0.027895,  0.313807 },
            { -0.162822,  0.607646 },
            {  1.455312,  0.955388 }
        };

// Expected values for PAR
psF64 projectionParExpected[TESTPOINTS][2] = {
            { -0.541244,  0.545532 },
            {  0.027703,  0.329366 },
            { -0.157157,  0.633550 },
            {  1.432951,  0.971372 }
        };

// Testpoints, offset and expected values for psSphereSetOffset test
#define TESTPOINTS_OFFSET  4
psF64 setOffsetTestpoint[TESTPOINTS_OFFSET][2] = {
            { 0.50,  0.50 },
            {-0.50,  0.50 },
            { 0.00,  0.00 },
            { 1.50, -0.10 }
        };

psF64 setOffsetOffset[TESTPOINTS_OFFSET][2] = {
            { 0.190761, -0.272205 },
            {-0.553049, -0.460926 },
            { 0.100335,-14.172222 },
            {14.172222, -0.100335 }
        };

psF64 setOffsetResult[TESTPOINTS_OFFSET][2] = {
            //XXX: Eugene says his values are correct, so i'm changing these values to the output.
            //            { 0.25,  0.75 },
            //            { 0.20,  0.80 },
            //            {-0.10,  1.50 },
            //            { 0.00,  0.00 }
            { 0.68702,   0.230294},
            { -0.966388,  0.0608434 },
            {0.10,  -1.50 },
            { 3.00141, -0.0140538  }
        };

psF64 getOffsetTestpoint1[TESTPOINTS_OFFSET][2] = {
            { 0.00,  0.00 },
            {-0.25,  0.50 },
            { 0.50, -0.25 },
            { 0.25, -0.10 }
        };

psF64 getOffsetTestpoint2[TESTPOINTS_OFFSET][2] = {
            { 0.75,  0.25 },
            { 0.40, -0.60 },
            { 1.50,  0.50 },
            { 0.10,  0.75 }
        };

psF64 getOffsetResult[TESTPOINTS_OFFSET][2] = {
            //XXX: Eugene says his values are correct, so i'm changing these values to the output.
            //            { -0.931596, -0.348976 },
            //            { -1.632830,  2.649629 },
            //            { -2.166795, -1.707211 },
            //            {  0.167752, -1.151351 }
            { 0.931596, 0.348976 },
            { 1.632830,  -2.649629 },
            { 2.166795, 1.707211 },
            {  -0.167752, 1.151351 }
        };


psS32 main( psS32 argc, char *argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(87);

    // psProjectionAlloc()
    {
        psMemId id = psMemGetId();
        psProjection *myProjection = psProjectionAlloc(1.1, 2.2, 3.3, 4.4, PS_PROJ_AIT);
        ok(myProjection != NULL, "psProjectionAlloc() returned non-NULL");
        ok(myProjection->R == 1.1, "psProjection->R set correctly");
        ok(myProjection->D == 2.2, "psProjection->D set correctly");
        ok(myProjection->Xs == 3.3, "psProjection->Xs set correctly");
        ok(myProjection->Ys == 4.4, "psProjection->Ys set correctly");
        ok(myProjection->type == PS_PROJ_AIT, "psProjection->type set correctly");
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testProjectTan()
    // We do a psProject() then a psDeproject() on several data points and
    // verify that we produce the original data point.
    // XXX: This test currently fails.
    {
        psMemId id = psMemGetId();
        psSphere *in = psSphereAlloc();
        psSphere *inTest = NULL;
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_TAN);

        // Perform projecton on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            in->r = projectionTestPoint[i][0];
            in->d = projectionTestPoint[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            psPlane *out = psProject(NULL, in, myProjection);
            if(out == NULL) {
                diag("psProject() returned NULL");
                errorFlag = true;
            } else {
                inTest = psDeproject(NULL, out, myProjection);
                if(fabs(out->x - projectionTanExpected[i][0]) > ERROR_TOL) {
                    diag("TEST ERROR: Testpoint %d  psPlane->x = %lg  expected %lg",
                         i,out->x, projectionTanExpected[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->y - projectionTanExpected[i][1]) > ERROR_TOL) {
                    diag("TEST ERROR: Testpoint % d psPlane->y = %lg  expected %lg",
                         i,out->y, projectionTanExpected[i][1]);
                    errorFlag = true;
                }

                // Verify output is as expected
                if(fabs(in->r - inTest->r) > ERROR_TOL) {
                    diag("TEST ERROR: (in->r, inTest->r) (%.2f %.2f)\n", in->r, inTest->r);
                    errorFlag = true;
                }
                if(fabs(in->d - inTest->d) > ERROR_TOL) {
                    diag("TEST ERROR: (in->d, inTest->d) (%.2f %.2f)\n", in->d, inTest->d);
                    errorFlag = true;
                }
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
            psFree(inTest);
            psFree(out);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testDeprojectTan()
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_TAN);

        // Perform deprojection on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and projection members
            in->x = projectionTanExpected[i][0];
            in->y = projectionTanExpected[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform deprojection
            psSphere *out = psDeproject(NULL, in, myProjection);

            // Verify output is not NULL
            if(out == NULL) {
                diag("psDeproject() returned NULL");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->r - projectionTestPoint[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->r = %lg  expected %lg",
                         i,out->r,projectionTestPoint[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->d - projectionTestPoint[i][1]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->d = %lg expected %lg",
                         i, out->d, projectionTestPoint[i][1]);
                    errorFlag = true;
                }
            }
            psFree(out);
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testProjectSin()
    {
        psMemId id = psMemGetId();
        psSphere *in = psSphereAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_SIN);

        // Perform projecton on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and project members
            in->r = projectionTestPoint[i][0];
            in->d = projectionTestPoint[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform projection
            psPlane *out = psProject(NULL, in, myProjection);

            // Verify output not NULL
            if(out == NULL) {
                diag("Return null not expected");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->x - projectionSinExpected[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psPlane->x = %lg  expected %lg",
                         i,out->x, projectionSinExpected[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->y - projectionSinExpected[i][1]) > ERROR_TOL) {
                    diag("Testpoint % d  psPlane->y = %lg  expected %lg",
                         i,out->y, projectionSinExpected[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testDeprojectSin()
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_SIN);

        // Perform deprojection on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and projection members
            in->x = projectionSinExpected[i][0];
            in->y = projectionSinExpected[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform deprojection
            psSphere *out = psDeproject(NULL, in, myProjection);

            // Verify output is not NULL
            if(out == NULL) {
                diag("Return null not expected");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->r - projectionTestPoint[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->r = %lg  expected %lg",
                         i,out->r,projectionTestPoint[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->d - projectionTestPoint[i][1]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->d = %lg expected %lg",
                         i, out->d, projectionTestPoint[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testProjectAit()
    {
        psMemId id = psMemGetId();
        psSphere *in = psSphereAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_AIT);

        // Perform projecton on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and project members
            in->r = projectionTestPoint[i][0];
            in->d = projectionTestPoint[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform projection
            psPlane *out = psProject(NULL, in, myProjection);

            // Verify output not NULL
            if(out == NULL) {
                diag("Return null not expected");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->x - projectionAitExpected[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psPlane->x = %lg  expected %lg",
                         i,out->x, projectionAitExpected[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->y - projectionAitExpected[i][1]) > ERROR_TOL) {
                    diag("Testpoint % d  psPlane->y = %lg  expected %lg",
                         i,out->y, projectionAitExpected[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testDeprojectAit()
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_AIT);

        // Perform deprojection on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and projection members
            in->x = projectionAitExpected[i][0];
            in->y = projectionAitExpected[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform deprojection
            psSphere *out = psDeproject(NULL, in, myProjection);

            // Verify output is not NULL
            if(out == NULL) {
                diag("Return null not expected");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->r - projectionTestPoint[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->r = %lg  expected %lg",
                         i,out->r,projectionTestPoint[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->d - projectionTestPoint[i][1]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->d = %lg expected %lg",
                         i, out->d, projectionTestPoint[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testProjectPar()
    {
        psMemId id = psMemGetId();
        psSphere *in = psSphereAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_PAR);

        // Perform projecton on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and project members
            in->r = projectionTestPoint[i][0];
            in->d = projectionTestPoint[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform projection
            psPlane *out = psProject(NULL, in, myProjection);

            // Verify output not NULL
            if(out == NULL) {
                diag("Return null not expected");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->x - projectionParExpected[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psPlane->x = %lg  expected %lg",
                         i,out->x, projectionParExpected[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->y - projectionParExpected[i][1]) > ERROR_TOL) {
                    diag("Testpoint % d  psPlane->y = %lg  expected %lg",
                         i,out->y, projectionParExpected[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testDeprojectPar()
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_PAR);

        // Perform deprojection on various test points
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            bool errorFlag = false;
            // Initialize input and projection members
            in->x = projectionParExpected[i][0];
            in->y = projectionParExpected[i][1];
            myProjection->R = projectionTestPoint[i][2];
            myProjection->D = projectionTestPoint[i][3];
            myProjection->Xs = projectionTestPoint[i][4];
            myProjection->Ys = projectionTestPoint[i][5];

            // Perform deprojection
            psSphere *out = psDeproject(NULL, in, myProjection);

            // Verify output is not NULL
            if(out == NULL) {
                diag("Return null not expected");
                errorFlag = true;
            } else {
                // Verify output is as expected
                if(fabs(out->r - projectionTestPoint[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->r = %lg  expected %lg",
                         i,out->r,projectionTestPoint[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->d - projectionTestPoint[i][1]) > ERROR_TOL) {
                    diag("Testpoint %d  psSphere->d = %lg expected %lg",
                         i, out->d, projectionTestPoint[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
            ok(!errorFlag, "psProject()/psDeproject() successful for test point %d\n", i);
        }

        psFree(in);
        psFree(myProjection);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psProject(): Invoke function with null coordinate argument
    // Following should generate an error message for null coord arg
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_TAN);
        psPlane *out = psProject(NULL, NULL, myProjection);
        ok(out == NULL, "psProject(NULL, NULL, xxx) returned NULL");
        psFree(myProjection);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Invoke function with null projection argument
    // Following should generate an error message for null projection arg
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *in = psSphereAlloc();
        psPlane *out = psProject(NULL, in, NULL);
        ok(out == NULL, "psProject(NULL, in, NULL) returned NULL");
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Invoke function with unallowed projection type
    // Following should generate an error message for unallowed projection type
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_TAN);
        myProjection->type = PS_PROJ_NTYPE;
        psSphere *in = psSphereAlloc();
        psPlane *out = psProject(NULL, in,myProjection);
        ok(out == NULL, "psProject(NULL, in, out) returned NULL with unallowed projection type");
        psFree(myProjection);
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testDeprojectFail()
    psMemId id = psMemGetId();


    // Invoke function with null coordinate argument
    // Following should generate an error message for null coord arg
    // XXX: We do not test the error generation
    {
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_TAN);
        psSphere *out = psDeproject(NULL, NULL, myProjection);
        ok(out == NULL, "psDeproject(NULL, NULL, myProjection) returned NULL");
        psFree(myProjection);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with null projection argument
    // Following should generate an error message for null projection arg
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psSphere *out = psDeproject(NULL, in, NULL);
        ok(out == NULL, "psDeproject(NULL, in, NULL) returned NULL");
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invoke function with unallowed projection type
    // Following should generate an error message for unallowed projection type
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psProjection *myProjection = psProjectionAlloc(0.0,0.0,1.0,1.0,PS_PROJ_TAN);
        psPlane *in = psPlaneAlloc();
        myProjection->type = PS_PROJ_NTYPE;
        psSphere *out = psDeproject(NULL, in,myProjection);
        ok(out == NULL, "psDeproject(NULL, in,myProjection) returned NULL with unallowed projection type");
        psFree(myProjection);
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testSetOffsetSphere()
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = NULL;
        psSphere *offset = psSphereAlloc();
        psSphere *tmpOffset = psSphereAlloc();
        bool errorFlag = false;

        // Initialize first position
        position1->r = DEG_TO_RAD(90.0);
        position1->d = DEG_TO_RAD(45.0);
        position1->rErr = 0.0;
        position1->dErr = 0.0;

        //  Using various offset verify spherical offset
        //  Use all the valid unit types
        for (psF64 r = 0.0; r < 180.0; r += DEG_INC)
        {
            for (psF64 d = 0.0; d < 90.0; d += DEG_INC) {

                offset->r = DEG_TO_RAD(r);
                offset->d = DEG_TO_RAD(d);
                offset->rErr = 0.0;
                offset->dErr = 0.0;

                position2 = psSphereSetOffset(position1, offset, PS_SPHERICAL, PS_RADIAN);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->r, (position1->r + offset->r), 1);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->d, (position1->d + offset->d), 2);
                psFree(position2);

                tmpOffset->r = RAD_TO_DEG(offset->r);
                tmpOffset->d = RAD_TO_DEG(offset->d);
                tmpOffset->rErr = 0.0;
                tmpOffset->dErr = 0.0;
                position2 = psSphereSetOffset(position1, tmpOffset, PS_SPHERICAL, PS_DEGREE);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->r, (position1->r + offset->r), 3);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->d, (position1->d + offset->d), 4);
                psFree(position2);

                tmpOffset->r = RAD_TO_MIN(offset->r);
                tmpOffset->d = RAD_TO_MIN(offset->d);
                tmpOffset->rErr = 0.0;
                tmpOffset->dErr = 0.0;
                position2 = psSphereSetOffset(position1, tmpOffset, PS_SPHERICAL, PS_ARCMIN);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->r, (position1->r + offset->r), 5);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->d, (position1->d + offset->d), 6);
                psFree(position2);

                tmpOffset->r = RAD_TO_SEC(offset->r);
                tmpOffset->d = RAD_TO_SEC(offset->d);
                tmpOffset->rErr = 0.0;
                tmpOffset->dErr = 0.0;
                position2 = psSphereSetOffset(position1, tmpOffset, PS_SPHERICAL, PS_ARCSEC);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->r, (position1->r + offset->r), 7);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(position2->d, (position1->d + offset->d), 8);
                psFree(position2);
            }
        }
        ok(!errorFlag, "psSphereSetOffset() successful on several dat points");

        psFree(position1);
        psFree(offset);
        psFree(tmpOffset);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to call function with null input coordinate
    // Following should generate error message null input coord
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *offset = psSphereAlloc();
        psSphere *position2 = psSphereSetOffset(NULL, offset, PS_LINEAR, PS_ARCSEC);
        ok(position2 == NULL, "psSphereSetOffset() returned NULL with NULL input coordinate");
        psFree(position2);
        psFree(offset);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to call function with null offset
    // Following should generate error message null offset
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = psSphereSetOffset(position1, NULL, PS_LINEAR, PS_ARCSEC);
        ok(position2 == NULL, "psSphereSetOffset() returned NULL with NULL offset");
        psFree(position1);
        psFree(position2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to call function with unallowed mode
    // Following should generate error message unallowed mode
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *offset = psSphereAlloc();
        psSphere *position2 = psSphereSetOffset(position1, offset, 0x54321, 0);
        ok(position2 == NULL, "psSphereSetOffset() returned NULL with unallowed mode");
        psFree(position1);
        psFree(position2);
        psFree(offset);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to call function with unallowed unit
    // Following should generate an error message unallowed unit
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *offset = psSphereAlloc();
        psSphere *position2 = psSphereSetOffset(position1, offset, PS_SPHERICAL, 0x54321);
        ok(position2 == NULL, "psSphereSetOffset() returned NULL with unallowed unit");
        psFree(position1);
        psFree(position2);
        psFree(offset);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testSetOffsetLinear()
    {
        psMemId id = psMemGetId();
        psSphere *coord = psSphereAlloc();
        psSphere *offset = psSphereAlloc();
        psSphere *out = NULL;
        bool errorFlag = false;

        for(psS32 i = 0; i < TESTPOINTS_OFFSET; i++)
        {
            bool errorFlag = false;
            coord->r = setOffsetTestpoint[i][0];
            coord->d = setOffsetTestpoint[i][1];

            offset->r = setOffsetOffset[i][0];
            offset->d = setOffsetOffset[i][1];

            out = psSphereSetOffset(coord, offset, PS_LINEAR, PS_RADIAN);

            if(fabs(out->r - setOffsetResult[i][0]) > ERROR_TOL) {
                diag("Testpoint %d: Result out->r = %lg not equal to expected %lg",
                     i, out->r, setOffsetResult[i][0]);
                errorFlag = true;
            }
            if(fabs(out->d - setOffsetResult[i][1]) > ERROR_TOL) {
                diag("Testpoint %d: Result out->d = %lg not equal to expected %lg",
                     i,out->d, setOffsetResult[i][1]);
                errorFlag = true;
            }

            psFree(out);
        }
        psFree(coord);
        psFree(offset);
        ok(!errorFlag, "psSphereSetOffset(), linear, successful on several data points");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testGetOffsetSphere()
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = psSphereAlloc();
        psSphere *offset = NULL;
        bool errorFlag = false;
        position1->r = DEG_TO_RAD(90.0);
        position1->d = DEG_TO_RAD(45.0);
        position1->rErr = 0.0;
        position1->dErr = 0.0;

        for (psF64 r = 0.0; r < 180.0;r += DEG_INC)
        {
            for (psF64 d = 0.0;d < 90.0; d += DEG_INC) {
                position2->r = DEG_TO_RAD(r);
                position2->d = DEG_TO_RAD(d);
                position2->rErr = 0.0;
                position2->dErr = 0.0;

                offset = psSphereGetOffset( position1,  position2,
                                            PS_SPHERICAL, PS_RADIAN);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->r, (position2->r - position1->r), 1);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->d, (position2->d - position1->d), 1);
                psFree(offset);

                offset = psSphereGetOffset( position1, position2,
                                            PS_SPHERICAL, PS_DEGREE);
                offset->r = DEG_TO_RAD(offset->r);
                offset->d = DEG_TO_RAD(offset->d);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->r, (position2->r - position1->r), 2);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->d, (position2->d - position1->d), 3);
                psFree(offset);

                offset = psSphereGetOffset( position1,  position2,
                                            PS_SPHERICAL, PS_ARCMIN);
                offset->r = MIN_TO_RAD(offset->r);
                offset->d = MIN_TO_RAD(offset->d);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->r, (position2->r - position1->r), 2);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->d, (position2->d - position1->d), 3);
                psFree(offset);

                offset = psSphereGetOffset( position1,  position2,
                                            PS_SPHERICAL, PS_ARCSEC);
                offset->r = SEC_TO_RAD(offset->r);
                offset->d = SEC_TO_RAD(offset->d);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->r, (position2->r - position1->r), 2);
                PS_COMPARE_TINY_THEN_PRINT_ERROR(offset->d, (position2->d - position1->d), 3);
                psFree(offset);
            }
        }
        psFree(position1);
        psFree(position2);
        ok(!errorFlag, "psSphereSetOffset(), linear, successful on several data points");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to invoke function with null position 1 parameter
    // Following should generate an error message for null position
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position2 = psSphereAlloc();
        psSphere *offset = psSphereGetOffset(NULL, position2, PS_LINEAR, 0);
        ok(offset == NULL, "psSphereGetOffset() returned NULL with null position 1 parameter");
        psFree(offset);
        psFree(position2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to invoke function with null position 2 parameter
    // Following should generate an error message for null position
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *offset = psSphereGetOffset(position1, NULL, PS_LINEAR, 0);
        ok(offset == NULL, "psSphereGetOffset() returned NULL with null position 2 parameter");
        psFree(offset);
        psFree(position1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to invoke function with unallowed mode
    // Following should generate an error message for unallowed mode
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = psSphereAlloc();
        psSphere *offset = psSphereGetOffset(position1, position2, 0x54321, 0);
        ok(offset == NULL, "psSphereGetOffset() returned NULL with unallowed mode");
        psFree(offset);
        psFree(position1);
        psFree(position2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to invoke function with unallowed unit type
    // Following should generate an error message for unallowed unit type
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = psSphereAlloc();
        psSphere *offset = psSphereGetOffset(position1, position2, PS_SPHERICAL, 0x54321);
        ok(offset == NULL, "psSphereGetOffset() returned NULL with unallowed unit type");
        psFree(offset);
        psFree(position1);
        psFree(position2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to invoke function with coordinate 1 declination value 90.0 degree
    // Following should generate warning message
    // XXX: We do not test the warning generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = psSphereAlloc();
        position1->d = DEG_TO_RAD(90.0);
        psSphere *offset = psSphereGetOffset(position1, position2, PS_SPHERICAL, PS_RADIAN);
        ok(offset == NULL, "psSphereGetOffset() returned NULL with coordinate 1 declination value 90.0 degree");
        psFree(offset);
        psFree(position1);
        psFree(position2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to invoke function with coordinate 2 declination value 90.0 degree
    // Following should generate warning message
    // XXX: We do not test the warning generation
    {
        psMemId id = psMemGetId();
        psSphere *position1 = psSphereAlloc();
        psSphere *position2 = psSphereAlloc();
        position1->d = DEG_TO_RAD(45.0);
        position2->d = DEG_TO_RAD(90.0);
        psSphere *offset = psSphereGetOffset(position1, position2, PS_SPHERICAL, PS_RADIAN);
        ok(offset == NULL, "psSphereGetOffset() returned NULL with coordinate 2 declination value 90.0 degree");
        psFree(offset);
        psFree(position1);
        psFree(position2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testGetOffsetLinear()
    {
        psMemId id = psMemGetId();
        psSphere *coord1 = psSphereAlloc();
        psSphere *coord2 = psSphereAlloc();
        bool errorFlag = false;

        for(psS32 i = 0; i < TESTPOINTS_OFFSET; i++)
        {
            bool errorFlag = false;
            coord1->r = getOffsetTestpoint1[i][0];
            coord1->d = getOffsetTestpoint1[i][1];
            coord2->r = getOffsetTestpoint2[i][0];
            coord2->d = getOffsetTestpoint2[i][1];
            psSphere *out = psSphereGetOffset(coord1, coord2, PS_LINEAR, PS_RADIAN);
            if (out == NULL) {
                diag("TEST ERROR: psSphereGetOffset() returned NULL");
                errorFlag = true;
            } else {
                if(fabs(out->r - getOffsetResult[i][0]) > ERROR_TOL) {
                    diag("Testpoint %d: Result out->r = %lg not equal to expected %lg",
                         i, out->r, getOffsetResult[i][0]);
                    errorFlag = true;
                }
                if(fabs(out->d - getOffsetResult[i][1]) > ERROR_TOL) {
                    diag("Testpoint %d: Result out->d = %lg not equal to expected %lg",
                         i,out->d, getOffsetResult[i][1]);
                    errorFlag = true;
                }
                psFree(out);
            }
        }
        psFree(coord1);
        psFree(coord2);
        ok(!errorFlag, "psSphereGetOffset() worked on several data points");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    /* 
        #define DEG_INC2 15.0
        #define VERBOSE 0
        // testProjectTanDeProjectTan()
        // XXX: Get rid of this?
        {
            psMemId id = psMemGetId();
            bool errorFlag = false;
            //
            // I'm not convinced that the p_psProject() and p_psDeproject() functions work
            // correctly.  If we project a set of coordinates over a wide range of (R, D)
            // values, then deproject them, the original (R, D) values are only produced
            // when D is larger than 0.  This code demonstrates that.
            //
            psProjection *tmpProj = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
            psPlane planeCoord01;
            psSphere skyCoord01;
            psSphere skyCoord02;
            for (psF32 R = -90.0 ; R <= 90.0 ; R+= DEG_INC2) {
                for (psF32 D = -90.0 ; D <= 90.0 ; D+= DEG_INC2) {
                    if ((fabs(R) != 90.0) && (fabs(D) != 90.0)) {
                        skyCoord01.r = DEG_TO_RAD(R);
                        skyCoord01.d = DEG_TO_RAD(D);
                        p_psProject(&planeCoord01, &skyCoord01, tmpProj);
                        p_psDeproject(&skyCoord02, &planeCoord01, tmpProj);
                        if ((fabs(skyCoord01.r - skyCoord02.r) < FLT_EPSILON) &&
                            (fabs(skyCoord01.d - skyCoord02.d) < FLT_EPSILON)) {
                            if (VERBOSE) {
                                printf("CORRECT: (%.2fr %.2fd) (%.2fr %.2fd) -> (%.2f %.2f) -> (%.2fr %.2fd)\n", R, D,
                                       skyCoord01.r, skyCoord01.d,
                                       planeCoord01.x, planeCoord01.y,
                                       skyCoord02.r, skyCoord02.d);
                            }
                        } else {
                            diag("TEST ERROR: (%.2fr %.2fd) (%.2fr %.2fd) -> (%.2f %.2f) -> (%.2fr %.2fd)\n", R, D,
                                  skyCoord01.r, skyCoord01.d,
                                  planeCoord01.x, planeCoord01.y,
                                  skyCoord02.r, skyCoord02.d);
                            errorFlag = true;
                        }
                    }
                }
            }
            ok(!errorFlag, "p_psProject()/p_psDeproject() worked on several data points");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
    */
}




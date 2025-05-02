/* @file  tap_psImageManip.c
*
*  @brief This file contains tests for all of the public psLib functions
*         that implement 1-D spline functionality:
* psSpline1DAlloc()
* psSpline1DEval()
* psSpline1DEvalVector()
* psSpline1DFitVector()
*
*         This file is composed of the tests formerly in tst_psFunc02.c,
*         tst_psFunc03.c, tst_psFunc04.c, tst_psFunc05.c, and tst_psFunc07.c.
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-05-02 04:20:06 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define ERROR_TOLERANCE_PERCENT    1.00
#define VERBOSE 0
/*********************************************************************************
CheckErrorF32(psF32 actual, psF32 expect): this routine returns FALSE if the
actual and expect arguments are with ERROR_TOLERANCE_PERCENT of each other,
otherwise it returns TRUE.
 ********************************************************************************/
psBool CheckErrorF32(
    psF32 actual,
    psF32 expect)
{
    // NOTE: this returns NAN if 'expect' == 0.0
    // NOTE 2: this is not testing a percent, but fractional error
    if ((fabs(actual - expect) / fabs(expect)) > ERROR_TOLERANCE_PERCENT) {
        return(true);
    } else {
        return(false);
    }
}
/*********************************************************************************
CheckErrorF64(psF64 actual, psF64 expect): this routine returns FALSE if the
actual and expect arguments are with ERROR_TOLERANCE_PERCENT of each other,
otherwise it returns TRUE.
 ********************************************************************************/
psBool CheckErrorF64(
    psF64 actual,
    psF64 expect)
{
    if ((fabs(actual - expect) / fabs(expect)) > ERROR_TOLERANCE_PERCENT) {
        return(true);
    } else {
        return(false);
    }
}

psF32 myFunc00(psF32 x)
{
    return(x);
}

psF64 myFunc00_F64(psF64 x)
{
    return(x);
}

#define A 4.0
#define B -3.0
#define C 0.2
#define D 0.1
psF32 myFunc01(psF32 x)
{
    return(A + x * (B + x * (C + x * (D))));
}
psF64 myFunc01_F64(psF64 x)
{
    return(A + x * (B + x * (C + x * (D))));
}

typedef psF32 (*mappingFuncF32)(psF32 x);
typedef psF64 (*mappingFuncF64)(psF64 x);

/* EAM 2023.01.22 : these tests are not well considered.  They generate a set of N+1 points 
   to use as knots at integer values from 0 to N, then evaluate the spline half-way between
   the knots.   the function above is a cubic, so a cubic spline should fit it perfectly, which 
   is fine.  Perhaps this test suite should use a pre-defined collection of data points?
 */

bool genericF32Test(psS32 NumSplines, mappingFuncF32 func, bool xNull)
{
    // We test the psSpline1DFitVector, psSpline1DEval() functions.  F32 version.
    bool testStatus = true;
    {
	// Generate the vector data
        psMemId id = psMemGetId();
        psVector *xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
        psVector *yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
        for (psS32 i=0;i<NumSplines+1;i++) {
            xF32->data.F32[i] = (psF32) i;
            yF32->data.F32[i] = (psF32) func((psF32) i);
        }

        psSpline1D *tmpSpline = NULL;
        if (!xNull) {
            tmpSpline = psSpline1DFitVector(xF32, yF32);
        } else {
            tmpSpline = psSpline1DFitVector(NULL, yF32);
        }

        if(tmpSpline == NULL) {
            diag("psSpline1DFitVector() returned NULL");
            testStatus = false;
        } else {
            if (tmpSpline->n != NumSplines) {
                diag("psSpline1DFitVector() did not properly set the psSpline1D->n member");
                testStatus = false;
            }
            if(tmpSpline->spline == NULL) {
                diag("psSpline1DFitVector() returned a NULL psSpline1D->spline member.");
                testStatus = false;
            }
            for (psS32 i = 0 ; i < NumSplines ; i++) {
                if (tmpSpline->spline[i] == NULL) {
                    diag("psSpline1DFitVector() returned a NULL psSpline1D->spline[%d] member.", i);
                    testStatus = false;
                }
            }
            if (tmpSpline->knots == NULL) {
                diag("psSpline1DFitVector() returned a NULL psSpline1D->knots member");
                testStatus = false;
            }
            if (tmpSpline->p_psDeriv2 == NULL) {
                diag("psSpline1DFitVector()returned a NULL psSpline1D->p_psDeriv2 member");
                testStatus = false;
            }

            // Test psSpline1DEval()
            for (psS32 i=0;i<NumSplines;i++) {
                psF32 x = 0.5 + (float) i;
                psF32 y = psSpline1DEval(tmpSpline, x);
                if (CheckErrorF32(y, func(x))) {
                    testStatus = false;
                    diag("TEST ERROR: f(%f) is %f, should be %f", x, y, myFunc00(x));
		    // XXX EAM : the truth value above should be 'func' not myFunc00
                }
            }

            // Test psSpline1DEvalVector()
            psVector *yF32Test = psSpline1DEvalVector(tmpSpline, xF32);
            if (yF32Test == NULL) {
                testStatus = false;
                diag("TEST ERROR: psSpline1DEvalVector() returned NULL");
            } else {
                for (psS32 i=0;i<NumSplines;i++) {
                    if (CheckErrorF32(yF32Test->data.F32[i], func(xF32->data.F32[i]))) {
                        testStatus = false;
                        diag("TEST ERROR: f(%f) is %f, should be %f", xF32->data.F32[i],
                             yF32Test->data.F32[i], myFunc00(xF32->data.F32[i]));
		    // XXX EAM : the truth value above should be 'func' not myFunc00
                    }
                }
                psFree(yF32Test);
            }
        }

        psFree(tmpSpline);
        psFree(xF32);
        psFree(yF32);
        if (psMemCheckLeaks (id, NULL, NULL, false)) {
            diag("TEST ERROR: memory leaks");
            testStatus = false;
        }
    }
    return(testStatus);
}


// This is a duplicate cut-n-paste of the F32 function
bool genericF64Test(psS32 NumSplines, mappingFuncF64 func, bool xNull)
{
    // We test the psSpline1DFitVector, psSpline1DEval() functions.  F64 version.
    bool testStatus = true;
    {
        psMemId id = psMemGetId();
        psVector *xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
        psVector *yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
        for (psS32 i=0;i<NumSplines+1;i++) {
            xF64->data.F64[i] = (psF64) i;
            yF64->data.F64[i] = (psF64) func((psF64) i);
        }

        psSpline1D *tmpSpline = NULL;
        if (!xNull) {
            tmpSpline = psSpline1DFitVector(xF64, yF64);
        } else {
            tmpSpline = psSpline1DFitVector(NULL, yF64);
        }
        if(tmpSpline == NULL) {
            diag("psSpline1DFitVector() returned NULL");
            testStatus = false;
        } else {
            if (tmpSpline->n != NumSplines) {
                diag("psSpline1DFitVector() did not properly set the psSpline1D->n member");
                testStatus = false;
            }
            if(tmpSpline->spline == NULL) {
                diag("psSpline1DFitVector() returned a NULL psSpline1D->spline member.");
                testStatus = false;
            }
            for (psS32 i = 0 ; i < NumSplines ; i++) {
                if (tmpSpline->spline[i] == NULL) {
                    diag("psSpline1DFitVector() returned a NULL psSpline1D->spline[%d] member.", i);
                    testStatus = false;
                }
            }
            if (tmpSpline->knots == NULL) {
                diag("psSpline1DFitVector() returned a NULL psSpline1D->knots member");
                testStatus = false;
            }
            if (tmpSpline->p_psDeriv2 == NULL) {
                diag("psSpline1DFitVector()returned a NULL psSpline1D->p_psDeriv2 member");
                testStatus = false;
            }

            // Test psSpline1DEval()
            for (psS32 i=0;i<NumSplines;i++) {
                psF64 x = 0.5 + (float) i;
                psF64 y = psSpline1DEval(tmpSpline, x);
                if (CheckErrorF64(y, func(x))) {
                    testStatus = false;
                    diag("TEST ERROR psSpline1DEval(): f(%f) is %f, should be %f", x, y, myFunc00(x));
                }
            }

            // Test psSpline1DEvalVector()
            psVector *yF64Test = psSpline1DEvalVector(tmpSpline, xF64);
            if (yF64Test == NULL) {
                testStatus = false;
                diag("TEST ERROR: psSpline1DEvalVector() returned NULL");
            } else {
                for (psS32 i=0;i<NumSplines;i++) {
                    if (CheckErrorF64((psF64)yF64Test->data.F32[i], func(xF64->data.F64[i]))) {
                        testStatus = false;
                        diag("TEST ERROR psSpline1DEvalVector(): f(%f) is %f, should be %f", xF64->data.F64[i],
                             yF64Test->data.F32[i], myFunc00(xF64->data.F64[i]));
                    }
                }
                psFree(yF64Test);
            }
        }

        psFree(tmpSpline);
        psFree(xF64);
        psFree(yF64);
        if (psMemCheckLeaks (id, NULL, NULL, false)) {
            diag("TEST ERROR: memory leaks");
            testStatus = false;
        }
    }
    return(testStatus);
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel( PS_LOG_INFO );
    plan_tests(28);

    // psSplineAllocTest()
    {
        psMemId id = psMemGetId();
        psSpline1D *tmpSpline = psSpline1DAlloc();
        ok(tmpSpline != NULL, "psSpline1DAlloc() returned non-NULL");
        skip_start(tmpSpline == NULL, 4, "Skipping tests because psSpline1DAlloc() failed");
        ok(tmpSpline->n == 0, "psSpline1DAlloc() properly set the psSpline1D->n member");
        ok(tmpSpline->spline == NULL, "psSpline1DAlloc() properly set the psSpline1D->spline member");
        ok(tmpSpline->knots == NULL, "psSpline1DAlloc() properly set the psSpline1D->knots member");
        ok(tmpSpline->p_psDeriv2 == NULL, "psSpline1DAlloc() properly set the psSpline1D->p_psDeriv2 member");
        psFree(tmpSpline);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psSplineEvalTest_sub(): Call psSpline1DFitVector with NULL arguments.
    {
        psMemId id = psMemGetId();
        psSpline1D *tmpSpline = psSpline1DFitVector(NULL, NULL);
        ok(tmpSpline == NULL, "psSpline1DFitVector() returns NULL with NULL arguments");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Verify psSpline1DEval() for NULL input
    // Verify with spline->n==0, and spline->knots PS_TYPE_F64
    {
        psMemId id = psMemGetId();
        float y = psSpline1DEval(NULL, 0.0);
        ok(isnan(y), "psSpline1DEval() returned NAN with NULL input spline");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify psSpline1DEvalVector() for NULL input spline
    {
        psMemId id = psMemGetId();
        psVector *x = psVectorAlloc(10, PS_TYPE_F32);
        psVector *y = psSpline1DEvalVector(NULL, x);
        ok(y == NULL, "psSpline1DEvalVector() returned NULL with NULL input spline");
        psFree(x);
        psFree(y);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify psSpline1DEvalVector() for NULL input vector
    // XXX: Move this where we have a good spline.  It currently fails because
    // were calling it with an un-fully-allocated one.
    if (0) {
        psMemId id = psMemGetId();
        psSpline1D *tmpSpline = psSpline1DAlloc();
        psVector *y = psSpline1DEvalVector(tmpSpline, NULL);
        ok(y == NULL, "psSpline1DEvalVector() returned NAN will NULL input vector");
        psFree(tmpSpline);
        psFree(y);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    ok(genericF32Test(1, myFunc00, false), "Generic, simple mapping, F32 test. 1 spline");
    ok(genericF32Test(95, myFunc00, false), "Generic, simple mapping, F32 test. 95 splines");
    ok(genericF32Test(1, myFunc01, false), "Generic, simple mapping, F32 test. 1 spline");
    ok(genericF32Test(95, myFunc01, false), "Generic, simple mapping, F32 test. 95 splines");
    ok(genericF32Test(1, myFunc00, true), "Generic, simple mapping, F32 test. 1 spline");
    ok(genericF32Test(95, myFunc00, true), "Generic, simple mapping, F32 test. 95 splines");
    ok(genericF32Test(1, myFunc01, true), "Generic, simple mapping, F32 test. 1 spline");
    ok(genericF32Test(95, myFunc01, true), "Generic, simple mapping, F32 test. 95 splines");
    ok(genericF64Test(1, myFunc00_F64, false), "Generic, simple mapping, F64 test. 1 spline");
    ok(genericF64Test(95, myFunc00_F64, false), "Generic, simple mapping, F64 test. 95 splines");
    ok(genericF64Test(1, myFunc01_F64, false), "Generic, simple mapping, F64 test. 1 spline");
    ok(genericF64Test(95, myFunc01_F64, false), "Generic, simple mapping, F64 test. 95 splines");
    ok(genericF64Test(1, myFunc00_F64, true), "Generic, simple mapping, F64 test. 1 spline");
    ok(genericF64Test(95, myFunc00_F64, true), "Generic, simple mapping, F64 test. 95 splines");
    ok(genericF64Test(1, myFunc01_F64, true), "Generic, simple mapping, F64 test. 1 spline");
    ok(genericF64Test(95, myFunc01_F64, true), "Generic, simple mapping, F64 test. 95 splines");
}

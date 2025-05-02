/* @file  tst_psImageManip.c
*
*  @brief This file will contain tests for all of the public psLib functions
*         that implement 1-D spline functionality:
* psSpline1DAlloc()
* psSpline1DEval()
* psSpline1DEvalVector()
* psVectorFitSpline1D()
*
*         This file is composed of the tests formerly in tst_psFunc02.c,
*         tst_psFunc03.c, tst_psFunc04.c, tst_psFunc05.c, and tst_psFunc07.c.
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-02-07 23:52:54 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#include "psTest.h"
#include "pslib_strict.h"

static psS32 psSplineAllocTest( void );
static psS32 psSplineEvalTest( void );
static psS32 psSplineEvalVectorTest( void );

// {testFunction, testpointNumber, description, expected rc, boolean-ignore this test}
testDescription tests[] = {
                              {psSplineAllocTest, 665, "(TEST A) psSplineAllocTest",true, false},
                              {psSplineEvalTest, 666, "(TEST B) psSplineEvalTest",true, false},
                              {psSplineEvalVectorTest,667,"(TEST C) psSplineEvalVectorTest",true,false},
                              {NULL}
                          };

#define  ERROR_TOLERANCE_PERCENT    1.00
/*********************************************************************************
CheckErrorF32(psF32 actual, psF32 expect): this routine returns FALSE if the
actual and expect arguments are with ERROR_TOLERANCE_PERCENT of each other,
otherwise it returns TRUE.
 ********************************************************************************/
psBool CheckErrorF32(
    psF32 actual,
    psF32 expect)
{
    if ((fabs(actual - expect) / fabs(expect)) > ERROR_TOLERANCE_PERCENT) {
        return(true);
    } else {
        return(false);
    }
}

psS32 main(
    psS32 argc,
    char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel( PS_LOG_INFO );
    //
    // Set the following trace levels to track what is happening in the
    // various functions in psSpline.c
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("spline1DFree", 0);
    psTraceSetLevel("calculateSecondDerivs", 0);
    psTraceSetLevel("vectorBinDisectF32", 0);
    psTraceSetLevel("vectorBinDisectF64", 0);
    psTraceSetLevel("p_psVectorBinDisect", 0);
    psTraceSetLevel("psSpline1DAlloc", 0);
    psTraceSetLevel("psVectorFitSpline1D", 0);
    psTraceSetLevel("psSpline1DEval", 0);
    psTraceSetLevel("psSpline1DEvalVector", 0);

    return ( ! runTestSuite( stderr, "psSpline1D", tests, argc, argv ) );
}

psS32 psSplineAllocTest()
{
    psTraceSetLevel(".", 0);
    psTraceSetLevel("spline1DFree", 0);
    psTraceSetLevel("calculateSecondDerivs", 0);
    psTraceSetLevel("vectorBinDisectF32", 0);
    psTraceSetLevel("vectorBinDisectF64", 0);
    psTraceSetLevel("p_psVectorBinDisect", 0);
    psTraceSetLevel("psSpline1DAlloc", 0);
    psTraceSetLevel("psVectorFitSpline1D", 0);
    psTraceSetLevel("psSpline1DEval", 0);
    psTraceSetLevel("psSpline1DEvalVector", 0);

    psS32 testStatus = true;
    psS32  currentId = psMemGetId();

    psSpline1D *tmpSpline = psSpline1DAlloc();
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != 0) {
        printf("TEST ERROR: psSpline1DAlloc() did not properly set the psSpline1D->n member.\n");
        testStatus = false;
    }

    if (tmpSpline->spline != NULL) {
        printf("TEST ERROR: psSpline1DAlloc() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    }

    if (tmpSpline->knots != NULL) {
        printf("TEST ERROR: psSpline1DAlloc() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 != NULL) {
        printf("TEST ERROR: psSpline1DAlloc() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }
    /*
     XXX: Must allocate these members.
        tmpSline->spline = (psPolynomial1D **) psAlloc(numSplines * sizeof(psPolynomial1D *));
        for (psS32 i=0;i<numSplines;i++) {
            spline->spline[i] = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 3);
        }
    */

    psFree(tmpSpline);
    psMemCheckCorruption(1);
    psS32 memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    return(testStatus);
}


/*********************************************************************************
psSplineEvalTest_sub(): We psVectorFitSpline1D with NULL arguments. psSpline1DEval()
 ********************************************************************************/
psS32 psSplineEvalTest_sub(psS32 NumSplines)
{
    psS32 testStatus = true;
    psS32 memLeaks=0;
    psS32  currentId = psMemGetId();

    printf("\n");
    printf("Calling psVectorFitSpline1D() with NULL y-vector.  Should generate error.\n");
    psSpline1D *tmpSpline = psVectorFitSpline1D(NULL, NULL);
    if (tmpSpline != NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not return NULL.\n");
        testStatus = false;
        psFree(tmpSpline);
    }

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    return (testStatus);
}


/*********************************************************************************
psSplineEvalTest_sub00(): We test the psVectorFitSpline1D, psSpline1DEval()
functions with a very simple x->y mapping (defined by myFunc00() below).  We
do this for both F32 and F64 versions of the input x and y vectors.
 ********************************************************************************/
psF32 myFunc00(psF32 x)
{
    return(x);
}

psS32 psSplineEvalTest_sub00(psS32 NumSplines)
{
    psS32 testStatus = true;
    psS32 memLeaks=0;
    psF32 x;
    psF32 y;
    psS32  currentId = psMemGetId();
    psVector *xF32 = NULL;
    psVector *xF64 = NULL;
    psVector *yF32 = NULL;
    psVector *yF64 = NULL;
    psSpline1D *tmpSpline = NULL;

    printf("\n");
    printf("psSplineEvalTest_sub00(): We test the psVectorFitSpline1D, psSpline1DEval() functions with a very\n");
    printf("simple x->y mapping (defined by myFunc00()).  We do this for both F32\n");
    printf("and F64 versions of the input x and y vectors.\n");
    printf("    Number of splines: %d\n", NumSplines);
    /****************************************************************************/
    /*    PS_TYPE_F32, PS_TYPE_F32 test          */
    /****************************************************************************/
    printf("Performing the F32 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    xF64->n = NumSplines+1;
    yF32->n = NumSplines+1;
    yF64->n = NumSplines+1;

    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        xF64->data.F64[i] = (psF64) i;
        yF32->data.F32[i] = (psF32) myFunc00(xF32->data.F32[i]);
        yF64->data.F64[i] = (psF64) myFunc00(xF32->data.F32[i]);
    }

    tmpSpline = psVectorFitSpline1D(xF32, yF32);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure (1)\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            x = 0.5 + (float) i;
            y = psSpline1DEval(tmpSpline, x);
            if (CheckErrorF32(y, myFunc00(x))) {
                printf("TEST ERROR: f(%f) is %f, should be %f\n", x, y, myFunc00(x));
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(xF64);
    psFree(yF32);
    psFree(yF64);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    /****************************************************************************/
    /*    PS_TYPE_F64, PS_TYPE_F64 test          */
    /****************************************************************************/
    printf("Performing the F64 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    xF64->n = NumSplines+1;
    yF32->n = NumSplines+1;
    yF64->n = NumSplines+1;

    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        xF64->data.F64[i] = (psF64) i;
        yF32->data.F32[i] = (psF32) myFunc00(xF32->data.F32[i]);
        yF64->data.F64[i] = (psF64) myFunc00(xF32->data.F32[i]);
    }

    tmpSpline = psVectorFitSpline1D(xF64, yF64);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            x = 0.5 + (float) i;
            y = psSpline1DEval(tmpSpline, x);
            if (CheckErrorF32(y, myFunc00(x))) {
                printf("TEST ERROR: f(%f) is %f, should be %f\n", x, y, myFunc00(x));
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(xF64);
    psFree(yF32);
    psFree(yF64);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    return (testStatus);
}

/*********************************************************************************
psSplineEvalTest_sub00b(): We test the psVectorFitSpline1D, psSpline1DEval()
functions with a more complicated x->y mapping (defined by myFunc00b() below).
We do this for both F32 and F64 versions of the input x and y vectors.
 ********************************************************************************/
#define A 4.0
#define B -3.0
#define C 0.2
#define D 0.1
psF32 myFunc00b(psF32 x)
{
    return(A + x * (B + x * (C + x * (D))));
}

psS32 psSplineEvalTest_sub00b(psS32 NumSplines)
{
    printf("\n");
    printf("psSplineEvalTest_sub00b(): We test the psVectorFitSpline1D, psSpline1DEval() functions with a\n");
    printf("more complicated x->y mapping (defined by myFunc00b()).  We do this for\n");
    printf("both F32 and F64 versions of the input x and y vectors.\n");
    printf("    Number of splines: %d\n", NumSplines);
    psS32 testStatus = true;
    psS32 memLeaks=0;
    psF32 x;
    psF32 y;
    psS32  currentId = psMemGetId();
    psVector *xF32 = NULL;
    psVector *xF64 = NULL;
    psVector *yF32 = NULL;
    psVector *yF64 = NULL;
    psSpline1D *tmpSpline = NULL;

    /****************************************************************************/
    /*    PS_TYPE_F32, PS_TYPE_F32 test          */
    /****************************************************************************/
    printf("Performing the F32 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    xF64->n = NumSplines+1;
    yF32->n = NumSplines+1;
    yF64->n = NumSplines+1;

    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        xF64->data.F64[i] = (psF64) i;
        yF32->data.F32[i] = (psF32) myFunc00b(xF32->data.F32[i]);
        yF64->data.F64[i] = (psF64) myFunc00b(xF32->data.F32[i]);
    }

    tmpSpline = psVectorFitSpline1D(xF32, yF32);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            x = 0.5 + (float) i;
            y = psSpline1DEval(tmpSpline, x);
            if (CheckErrorF32(y, myFunc00b(x))) {
                printf("TEST ERROR: f(%f) is %f, should be %f\n", x, y, myFunc00b(x));
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(xF64);
    psFree(yF32);
    psFree(yF64);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    /****************************************************************************/
    /*    PS_TYPE_F64, PS_TYPE_F64 test          */
    /****************************************************************************/
    printf("Performing the F64 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    xF64->n = NumSplines+1;
    yF32->n = NumSplines+1;
    yF64->n = NumSplines+1;

    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        xF64->data.F64[i] = (psF64) i;
        yF32->data.F32[i] = (psF32) myFunc00b(xF32->data.F32[i]);
        yF64->data.F64[i] = (psF64) myFunc00b(xF32->data.F32[i]);
    }

    tmpSpline = psVectorFitSpline1D(xF64, yF64);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            x = 0.5 + (float) i;
            y = psSpline1DEval(tmpSpline, x);
            if (CheckErrorF32(y, myFunc00b(x))) {
                printf("TEST ERROR: f(%f) is %f, should be %f\n", x, y, myFunc00b(x));
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(xF64);
    psFree(yF32);
    psFree(yF64);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    return (testStatus);
}


/*********************************************************************************
psSplineEvalTest_sub00b():  This is similar to psSplineEvalTest_sub00b()
except that the x vector is NULL.
 ********************************************************************************/
psS32 psSplineEvalTest_sub00c(psS32 NumSplines)
{
    printf("\n");
    printf("psSplineEvalTest_sub00c(): This is similar to psSplineEvalTest_sub00b()\n");
    printf("except that the x vector is NULL.\n");
    printf("    Number of splines: %d\n", NumSplines);
    psS32 testStatus = true;
    psS32 memLeaks=0;
    psF32 x;
    psF32 y;
    psS32  currentId = psMemGetId();
    psVector *xF32 = NULL;
    psVector *xF64 = NULL;
    psVector *yF32 = NULL;
    psVector *yF64 = NULL;
    psSpline1D *tmpSpline = NULL;

    /****************************************************************************/
    /*    PS_TYPE_F32, PS_TYPE_F32 test          */
    /****************************************************************************/
    printf("Performing the F32 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    xF64->n = NumSplines+1;
    yF32->n = NumSplines+1;
    yF64->n = NumSplines+1;

    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        xF64->data.F64[i] = (psF64) i;
        yF32->data.F32[i] = (psF32) myFunc00b(xF32->data.F32[i]);
        yF64->data.F64[i] = (psF64) myFunc00b(xF32->data.F32[i]);
    }

    tmpSpline = psVectorFitSpline1D(NULL, yF32);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            x = 0.5 + (float) i;
            y = psSpline1DEval(tmpSpline, x);
            if (CheckErrorF32(y, myFunc00b(x))) {
                printf("TEST ERROR: f(%f) is %f, should be %f\n", x, y, myFunc00b(x));
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(xF64);
    psFree(yF32);
    psFree(yF64);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    /****************************************************************************/
    /*    PS_TYPE_F64, PS_TYPE_F64 test          */
    /****************************************************************************/
    printf("Performing the F64 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    xF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF64 = psVectorAlloc(NumSplines+1, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    xF64->n = NumSplines+1;
    yF32->n = NumSplines+1;
    yF64->n = NumSplines+1;

    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        xF64->data.F64[i] = (psF64) i;
        yF32->data.F32[i] = (psF32) myFunc00b(xF32->data.F32[i]);
        yF64->data.F64[i] = (psF64) myFunc00b(xF32->data.F32[i]);
    }

    tmpSpline = psVectorFitSpline1D(NULL, yF64);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            x = 0.5 + (float) i;
            y = psSpline1DEval(tmpSpline, x);
            if (CheckErrorF32(y, myFunc00b(x))) {
                printf("TEST ERROR: f(%f) is %f, should be %f\n", x, y, myFunc00b(x));
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(xF64);
    psFree(yF32);
    psFree(yF64);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    return (testStatus);
}



psS32 psSplineEvalTest()
{
    psTraceSetLevel(".", 0);
    psTraceSetLevel("spline1DFree", 0);
    psTraceSetLevel("calculateSecondDerivs", 0);
    psTraceSetLevel("vectorBinDisectF32", 0);
    psTraceSetLevel("vectorBinDisectF64", 0);
    psTraceSetLevel("p_psVectorBinDisect", 0);
    psTraceSetLevel("psSpline1DAlloc", 0);
    psTraceSetLevel("psVectorFitSpline1D", 0);
    psTraceSetLevel("psSpline1DEval", 0);
    psTraceSetLevel("psSpline1DEvalVector", 0);

    psS32 testStatus = psSplineEvalTest_sub(10);

    // HEY: XXX: Test with empty psVectors.
    //    testStatus |= psSplineEvalTest_sub00(0);
    //    testStatus |= psSplineEvalTest_sub00b(0);
    //    testStatus |= psSplineEvalTest_sub00c(0);

    testStatus |= psSplineEvalTest_sub00(1);
    testStatus |= psSplineEvalTest_sub00b(1);
    testStatus |= psSplineEvalTest_sub00c(1);

    testStatus |= psSplineEvalTest_sub00(95);
    testStatus |= psSplineEvalTest_sub00b(95);
    testStatus |= psSplineEvalTest_sub00c(95);

    return(testStatus);
}



psS32 psSplineEvalVectorTest_sub00(psS32 NumSplines)
{
    psS32 testStatus = true;
    psS32 memLeaks=0;

    psS32  currentId = psMemGetId();
    psVector *xF32 = NULL;
    psVector *yF32 = NULL;
    psSpline1D *tmpSpline = NULL;
    /****************************************************************************/
    /*    PS_TYPE_F32, PS_TYPE_F32 test          */
    /****************************************************************************/
    printf("Performing the F32 test....\n");
    xF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    yF32 = psVectorAlloc(NumSplines+1, PS_TYPE_F32);
    psVector *xF32Test = psVectorAlloc(NumSplines, PS_TYPE_F32);
    psVector *yF32Test = NULL;
    psVector *xF64Test = psVectorAlloc(NumSplines, PS_TYPE_F64);
    xF32->n = NumSplines+1;
    yF32->n = NumSplines+1;
    xF32Test->n = NumSplines;
    xF64Test->n = NumSplines;


    for (psS32 i=0;i<NumSplines+1;i++) {
        xF32->data.F32[i] = (psF32) i;
        yF32->data.F32[i] = (psF32) i;
    }

    tmpSpline = psVectorFitSpline1D(xF32, yF32);
    if (tmpSpline == NULL) {
        printf("TEST ERROR: Could not allocate psSpline1D data structure\n");
        testStatus = false;
    }

    if (tmpSpline->n != NumSplines) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->n member.\n");
        printf("            psSpline1D->NumSplines was %d, should be %d.\n", tmpSpline->n, NumSplines);
        testStatus = false;
    }

    if (tmpSpline->spline == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline member.\n");
        testStatus = false;
    } else {
        for (psS32 i = 0 ; i < NumSplines ; i++) {
            if (tmpSpline->spline[i] == NULL) {
                printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->spline[%d] member.\n", i);
                testStatus = false;
            } else {
                if (psTraceGetLevel(__func__) >= 6) {
                    PS_POLY_PRINT_1D(tmpSpline->spline[i]);
                }
            }
        }
    }

    if (tmpSpline->knots == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->knots member.\n");
        testStatus = false;
    }

    if (tmpSpline->p_psDeriv2 == NULL) {
        printf("TEST ERROR: psVectorFitSpline1D() did not properly set the psSpline1D->p_psDeriv2 member.\n");
        testStatus = false;
    }

    printf("Testing psSpline1DEvalVector() with an F32 vector.\n");
    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            xF32Test->data.F32[i] = 0.5 + (psF32) i;
        }
        yF32Test = psSpline1DEvalVector(tmpSpline, xF32Test);

        for (psS32 i=0;i<NumSplines;i++) {
            if (CheckErrorF32(yF32Test->data.F32[i], xF32Test->data.F32[i])) {
                printf("TEST ERROR: f(%f) is %f\n", xF32Test->data.F32[i], yF32Test->data.F32[i]);
                testStatus = false;
            }
        }
        psFree(yF32Test);
    }

    printf("Testing psSpline1DEvalVector() with an F64 vector.\n");
    if (testStatus == true) {
        for (psS32 i=0;i<NumSplines;i++) {
            xF64Test->data.F64[i] = 0.5 + (psF64) i;
        }
        yF32Test = psSpline1DEvalVector(tmpSpline, xF64Test);

        for (psS32 i=0;i<NumSplines;i++) {
            if (CheckErrorF32(yF32Test->data.F32[i], (psF32) xF64Test->data.F64[i])) {
                printf("TEST ERROR: f(%f) is %f\n", (psF32) xF64Test->data.F64[i], yF32Test->data.F32[i]);
                testStatus = false;
            }
        }
    }

    psFree(tmpSpline);
    psFree(xF32);
    psFree(yF32);
    psFree(xF32Test);
    psFree(yF32Test);
    psFree(xF64Test);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    return (testStatus);
}

psS32 psSplineEvalVectorTest()
{
    psTraceSetLevel(".", 0);
    psTraceSetLevel("spline1DFree", 0);
    psTraceSetLevel("calculateSecondDerivs", 0);
    psTraceSetLevel("vectorBinDisectF32", 0);
    psTraceSetLevel("vectorBinDisectF64", 0);
    psTraceSetLevel("p_psVectorBinDisect", 0);
    psTraceSetLevel("psSpline1DAlloc", 0);
    psTraceSetLevel("psVectorFitSpline1D", 0);
    psTraceSetLevel("psSpline1DEval", 0);
    psTraceSetLevel("psSpline1DEvalVector", 0);

    return(psSplineEvalVectorTest_sub00(10));
}


/*****************************************************************************
    This routine must ensure that psMinimizePowell() works correctly.
 
    We test with a NULL and non-NULL paramMask.
 
    XXX: Must verify the stderr for the NULL parameter tests.
    XXX: psMinimizeChi2Powell() is untested.
 *****************************************************************************/
#include <stdio.h>
#include <math.h>
#include "pslib.h"
#include "psTest.h"
#define N 5
#define MIN_VALUE 20.0
#define NUM_PARAMS 10
#define ERROR_TOLERANCE 0.10
float expectedParm[NUM_PARAMS];
psS32 testStatus = true;

/*****************************************************************************
myFunc(): This routine subtracts the associate value in expectedParm[] from
each parameter and then squares it, then sums that for all parameters, then
adds MIN_VALUE to it.  The minimum for this function will be MIN_VALUE, and
will occur when each parameter equals the associated value in expectedParm[].
 
This procedure ignores the coordinates, other than to ensure that they were
passed correctly from psMinimizePowell().
 *****************************************************************************/
float myFunc(psVector *myParams,
             psArray *myCoords)
{
    float sum = 0.0;
    float coordData = 0.0;
    float expData = 0.0;
    psS32 i;

    //
    // Simply ensure that the coordinate data was passed correctly.
    //
    for (i=0;i<N;i++) {
        coordData = ((psVector *) (myCoords->data[i]))->data.F32[0];
        expData = (float) (i+10);
        if (fabs(coordData - expData) > FLT_EPSILON) {
            printf("ERROR(1): coordinate data was incorrectly passed to myFunc()\n");
            printf("ERROR(1): was (%f) should be (%f)\n", coordData, expData);
            testStatus = false;
        }
        coordData = ((psVector *) (myCoords->data[i]))->data.F32[1];
        expData = (float) (i+3);
        if (fabs(coordData - expData) > FLT_EPSILON) {
            printf("ERROR(2): coordinate data was incorrectly passed to myFunc()\n");
            printf("ERROR(2): was (%f) should be (%f)\n", coordData, expData);
            testStatus = false;
        }
    }


    sum = 0.0;
    for (i=0;i<NUM_PARAMS;i++) {
        sum+= (myParams->data.F32[i] - expectedParm[i]) * (myParams->data.F32[i] - expectedParm[i]);
    }
    sum = MIN_VALUE + (sum * sum);

    return(sum);
}


psS32 t00()
{
    psS32 currentId = psMemGetId();
    psS32 memLeaks = 0;
    psS32 i = 0;
    psArray *myCoords;
    psVector *myParams;
    psVector *myParamMask;
    psMinimization *min;

    psTraceSetLevel(".psLib", 0);
    /**************************************************************************
     *************************************************************************/
    myParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
    myParamMask = psVectorAlloc(NUM_PARAMS, PS_TYPE_U8);
    myParams->n = NUM_PARAMS;
    myParamMask->n = NUM_PARAMS;
    min = psMinimizationAlloc(100, 0.01);

    myCoords = psArrayAlloc(N);
    myCoords->n = N;
    for (i=0;i<N;i++) {
        myCoords->data[i] = (psPtr *) psVectorAlloc(2, PS_TYPE_F32);
        ((psVector *) (myCoords->data[i]))->data.F32[0] = (float) (i+10);
        ((psVector *) (myCoords->data[i]))->data.F32[1] = (float) (i+3);
    }
    for (i=0;i<NUM_PARAMS;i++) {
        expectedParm[i] = 2.32 + (float) (2 * i);
        myParams->data.F32[i] = 0.0;
        myParams->data.F32[i] = (float) i;
        myParamMask->data.U8[i] = 0;
    }

    printPositiveTestHeader(stdout,
                            "psMinimize functions",
                            "psMinimizePowell()");

    psMinimizePowell(min,
                     myParams,
                     myParamMask,
                     myCoords,
                     (psMinimizePowellFunc) myFunc);

    printf("\nThe minimum is %f (expected: %f)\n", min->value, MIN_VALUE);

    for (i=0;i<NUM_PARAMS;i++) {
        printf("Parameter %d at the minimum is %.1f (expected: %.1f)\n", i,
               myParams->data.F32[i], expectedParm[i]);

        if (fabs(myParams->data.F32[i] - expectedParm[i]) > fabs(ERROR_TOLERANCE * expectedParm[i])) {
            printf("ERROR: Parameter %d: (%.1f), expected was (%.1f)\n",
                   i, myParams->data.F32[i], expectedParm[i]);
            testStatus = false;
        } else {
            printf("Parameter %d: (%.1f), expected was (%.1f)\n",
                   i, myParams->data.F32[i], expectedParm[i]);
        }
    }


    psFree(myCoords);
    psFree(myParams);
    psFree(myParamMask);
    psFree(min);

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    printFooter(stdout,
                "psMinimize functions",
                "psMinimizePowell()",
                testStatus);


    return (!testStatus);
}

psS32 t01()
{
    psS32 currentId = psMemGetId();
    psS32 memLeaks = 0;
    psS32 i = 0;
    psArray *myCoords;
    psVector *myParams;
    psMinimization *min;

    psTraceSetLevel(".psLib", 0);
    /**************************************************************************
     *************************************************************************/
    myParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
    min = psMinimizationAlloc(100, 0.01);

    myCoords = psArrayAlloc(N);
    myCoords->n = N;
    for (i=0;i<N;i++) {
        myCoords->data[i] = (psPtr *) psVectorAlloc(2, PS_TYPE_F32);
        ((psVector *) (myCoords->data[i]))->data.F32[0] = (float) (i+10);
        ((psVector *) (myCoords->data[i]))->data.F32[1] = (float) (i+3);
        ((psVector *) (myCoords->data[i]))->n = 2;
    }
    for (i=0;i<NUM_PARAMS;i++) {
        expectedParm[i] = 2.32 + (float) (2 * i);
        myParams->data.F32[i] = 0.0;
        myParams->data.F32[i] = (float) i;
        myParams->n++;
    }

    printPositiveTestHeader(stdout,
                            "psMinimize functions",
                            "psMinimizePowell()");

    psMinimizePowell(min,
                     myParams,
                     NULL,
                     myCoords,
                     (psMinimizePowellFunc) myFunc);

    printf("\nThe minimum is %f (expected: %f)\n", min->value, MIN_VALUE);

    for (i=0;i<NUM_PARAMS;i++) {
        printf("Parameter %d at the minimum is %.1f (expected: %.1f)\n", i,
               myParams->data.F32[i], expectedParm[i]);

        if (fabs(myParams->data.F32[i] - expectedParm[i]) > fabs(ERROR_TOLERANCE * expectedParm[i])) {
            printf("ERROR: Parameter %d: (%.1f), expected was (%.1f)\n",
                   i, myParams->data.F32[i], expectedParm[i]);
            testStatus = false;
        } else {
            printf("Parameter %d: (%.1f), expected was (%.1f)\n",
                   i, myParams->data.F32[i], expectedParm[i]);
        }
    }


    psFree(myCoords);
    psFree(myParams);
    psFree(min);

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    printFooter(stdout,
                "psMinimize functions",
                "psMinimizePowell()",
                testStatus);


    return (!testStatus);
}

psS32 t02()
{
    psS32 currentId = psMemGetId();
    psS32 memLeaks = 0;
    psS32 i = 0;
    psArray *myCoords;
    psVector *myParams;
    psVector *myParamMask;
    psMinimization *min;

    psTraceSetLevel(".psLib", 0);
    /**************************************************************************
     *************************************************************************/
    myParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
    myParamMask = psVectorAlloc(NUM_PARAMS, PS_TYPE_U8);
    min = psMinimizationAlloc(100, 0.01);

    myCoords = psArrayAlloc(N);
    myCoords->n = N;
    for (i=0;i<N;i++) {
        myCoords->data[i] = (psPtr *) psVectorAlloc(2, PS_TYPE_F32);
        ((psVector *) (myCoords->data[i]))->data.F32[0] = (float) (i+10);
        ((psVector *) (myCoords->data[i]))->data.F32[1] = (float) (i+3);
        ((psVector *) (myCoords->data[i]))->n++;
    }
    for (i=0;i<NUM_PARAMS;i++) {
        expectedParm[i] = 2.32 + (float) (2 * i);
        myParams->data.F32[i] = 0.0;
        myParams->data.F32[i] = (float) i;
        myParamMask->data.U8[i] = 0;
        myParams->n++;
        myParamMask->n++;
    }

    printPositiveTestHeader(stdout,
                            "psMinimize functions",
                            "psMinimizePowell(): various NULL inputs");

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error for null minimize.");
    psMinimizePowell(NULL, myParams, myParamMask, myCoords, (psMinimizePowellFunc) myFunc);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error for null parameter vector");
    psMinimizePowell(min, NULL, myParamMask, myCoords,(psMinimizePowellFunc) myFunc);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error for null coords.");
    psMinimizePowell(min, myParams, myParamMask, NULL, (psMinimizePowellFunc) myFunc);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error for null function");
    psMinimizePowell(min, myParams, myParamMask, myCoords, NULL);

    psFree(myCoords);
    psFree(myParams);
    psFree(myParamMask);
    psFree(min);

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
    printFooter(stdout,
                "psMinimize functions",
                "psMinimizePowell(): various NULL inputs",
                testStatus);


    return (!testStatus);
}

psS32 main()
{
    psLogSetFormat("HLNM");
    t00();
    t01();
    t02();
}

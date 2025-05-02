/*****************************************************************************
    This routine must ensure that psMinimizePowell() works correctly.
 
    XXX: This needs extensive work
    XXX: We test with a NULL and non-NULL paramMask, however, the mask
         has all zero values.
    XXX: The tests that should generate errors are if'ed out.
    XXX: psMinimizeChi2Powell() is untested.
    XXX: Also, the test currently fails because of memory corruption in
         psMinimizePowell().
    XXX: The unallowed input parameter tests could be more extensive
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

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


psS32 main()
{
    psLogSetFormat("HLNM");
    plan_tests(8);

    // Check for various errors on unallowed input parameters
    {
        psVector *myParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *myParamMask = psVectorAlloc(NUM_PARAMS, PS_TYPE_U8);
        psMinimization *min = psMinimizationAlloc(100, 0.01);
        psArray *myCoords = psArrayAlloc(N);

        // Following should generate error for NULL minimize
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizePowell(NULL, myParams, myParamMask, myCoords,
                                           (psMinimizePowellFunc) myFunc);
            ok(!tmpBool, "psMinimizePowell() returned FALSE with NULL psMinimize param");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Following should generate error for NULL parameter vector
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizePowell(min, NULL, myParamMask, myCoords,(psMinimizePowellFunc) myFunc);
            ok(!tmpBool, "psMinimizePowell() returned FALSE with NULL parameter param");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Following should generate error for NULL coords
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizePowell(min, myParams, myParamMask, NULL, (psMinimizePowellFunc) myFunc);
            ok(!tmpBool, "psMinimizePowell() returned FALSE with NULL coords param");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Following should generate error for NULL function
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizePowell(min, myParams, myParamMask, myCoords, NULL);
            ok(!tmpBool, "psMinimizePowell() returned FALSE with NULL function param");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(myParams);
        psFree(myParamMask);
        psFree(min);
        psFree(myCoords);
    }


    // Powell minimize with parameter mask
    // XXX: This function aborts with a memory corruption error at around line
    // 60 of psMinimizePowell.c, at the psFree(v):
    //    if (fabs(baseFuncVal - currFuncVal) <= min->tol) {
    //        psFree(v);
    //        psFree(pQP);
    if (0) {
        psMemId id = psMemGetId();
        psVector *myParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psMinimization *min = psMinimizationAlloc(100, 0.01);
        psArray *myCoords = psArrayAlloc(N);
        psVector *myParamMask = psVectorAlloc(NUM_PARAMS, PS_TYPE_U8);

        for (psS32 i=0;i<N;i++)
        {
            myCoords->data[i] = (psPtr *) psVectorAlloc(2, PS_TYPE_F32);
            ((psVector *) (myCoords->data[i]))->data.F32[0] = (float) (i+10);
            ((psVector *) (myCoords->data[i]))->data.F32[1] = (float) (i+3);
        }
        for (psS32 i=0;i<NUM_PARAMS;i++)
        {
            expectedParm[i] = 2.32 + (float) (2 * i);
            myParams->data.F32[i] = 0.0;
            myParams->data.F32[i] = (float) i;
            myParamMask->data.U8[i] = 0;
        }

        bool tmpBool = psMinimizePowell(min, myParams, NULL, myCoords,
                                        (psMinimizePowellFunc) myFunc);
        ok(tmpBool, "psMinimizePowell() returned sucessfully");
        skip_start(!tmpBool, 0, "Skipping tests because psMinimizePowell() failed");

        printf("\nThe minimum is %f (expected: %f)\n", min->value, MIN_VALUE);
        for (psS32 i=0;i<NUM_PARAMS;i++)
        {
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
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // XXX: Add tests with active parameter mask
}

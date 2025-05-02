/** @file  psMinimize.c
 *  \brief basic minimization functions
 *  @ingroup Math
 *
 *  This file will contain functions to minimize an arbitrary function at
 *  a data point, fit an arbitrary function to a set of data points, and
 *  fit a 1-D polynomial to a set of data points.
 *
 *  @author GLG, MHPCC
 *
 *  NOTE: XXX: The SDR is silent about data types.  F32 is implemented here.
 *
 *  @version $Revision: 1.17 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "psMinimizePowell.h"
#include "psAssert.h"
#include "psStats.h"
#include "psImage.h"
#include "psImageStructManip.h"
#include "psLogMsg.h"
/*****************************************************************************/
/* DEFINE STATEMENTS                                                         */
/*****************************************************************************/

// This macro takes as input the vector BASE and adds a multiple of the vector
// LINE to it.  We assume BASEMASK is non-null.
#define PS_VECTOR_ADD_MULTIPLE(BASE, BASEMASK, LINE, OUT, MUL) \
for (psS32 i=0;i<BASE->n;i++) { \
    if (BASEMASK->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) { \
        OUT->data.F32[i] = BASE->data.F32[i] + (MUL * LINE->data.F32[i]); \
    } else { \
        OUT->data.F32[i] = BASE->data.F32[i]; \
    } \
} \

#define PS_VECTOR_F32_CHECK_ZERO_VECTOR(IN, BOOL_VAR) \
BOOL_VAR = true; \
for (psS32 i=0;i<IN->n;i++) { \
    if (fabs(IN->data.F32[i]) >= FLT_EPSILON) { \
        BOOL_VAR = false; \
        break; \
    } \
} \

#define PS_VECTOR_WITH_MASK_F32_CHECK_ZERO_VECTOR(IN, INMASK, BOOL_VAR) \
BOOL_VAR = true; \
for (psS32 i=0;i<IN->n;i++) { \
    if ((INMASK->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) && (fabs(IN->data.F32[i]) >= FLT_EPSILON)) { \
        BOOL_VAR = false; \
        break; \
    } \
} \

/*****************************************************************************/
/* TYPE DEFINITIONS                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - LOCAL                                           */
/*****************************************************************************/

/******************************************************************************
p_psDetermineBracket():  This routine takes as input an arbitrary function,
and the parameter to vary, and the line along which it must vary.  This
function produces as output a bracket [a, b, c] such that
f(param + b * line) < f(param + a * line)
f(param + b * line) < f(param + c * line)
a < b < c

Algorithm:

XXX completely ad hoc:
start with the user-supplied starting parameter and
call that b.  Calculate a/c as a fractional amount smaller/larger than b.
Repeat this process until a local minimum is found.

XXX: new algorithm:
start at x=0, expand in one direction until the function
decreases.  Then you have two points in the bracket.  Keep going until it
increases, or x is too large.  If thst does not work, expand in the other
direction.

XXX: output bracket vector should be an input as well.
*****************************************************************************/
psVector *p_psDetermineBracket(
    psVector *params,
    psVector *line,
    const psVector *paramMask,
    const psArray *coords,
    psMinimizePowellFunc func)
{
    psF32 a = 0.0;
    psF32 b = 0.0;
    psF32 c = 0.0;
    psF32 fa = 0.0;
    psF32 fb = 0.0;
    psF32 fc = 0.0;
    psS32 iter = 100;
    psF32 aDir = 0.0;
    psF32 cDir = 0.0;
    psF32 new_aDir = 0.0;
    psF32 new_cDir = 0.0;
    psVector *bracket = psVectorAlloc(3, PS_TYPE_F32);
    psF32 stepSize = PS_DETERMINE_BRACKET_STEP_SIZE;
    psVector *tmp = NULL;
    bool boolLineIsNull = true;

    psTrace("psLib.math", 4, "---- p_psDetermineBracket() begin ----\n");

    // If the line vector is zero, then return NULL.
    PS_VECTOR_WITH_MASK_F32_CHECK_ZERO_VECTOR(params, paramMask, boolLineIsNull);
    if (boolLineIsNull == true) {
        psTrace("psLib.math", 2, "p_psDetermineBracket() called with zero line vector.\n");
        psTrace("psLib.math", 4, "---- p_psDetermineBracket() end (NULL) ----\n");
        psFree(bracket);
        return(NULL);
    }

    tmp = psVectorAlloc(params->n, PS_TYPE_F32);

    b = 0;
    a = -stepSize;
    c = stepSize;

    PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, a);
    fa = func(tmp, coords);

    PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, b);
    fb = func(tmp, coords);

    PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, c);
    fc = func(tmp, coords);

    if (fa < fb) {
        aDir = -1;
    } else {
        aDir = 1;
    }

    if (fc < fb) {
        cDir = -1;
    } else {
        cDir = 1;
    }

    psTrace("psLib.math", 6, "(a, b, c) is (%f %f %f) (fa, fb, fc) is (%f %f %f)\n", a, b, c, fa, fb, fc);

    while (iter > 0) {
        psTrace("psLib.math", 6, "psDetermineBracket(): iteration %d\n", iter);
        if ((fb < fa) && (fb < fc)) {
            bracket->data.F32[0] = a;
            bracket->data.F32[1] = b;
            bracket->data.F32[2] = c;
            psFree(tmp);
            psTrace("psLib.math", 6, "---- p_psDetermineBracket() end ----\n");
            return(bracket);
        }
        stepSize*= (1.0 + stepSize);
        a =- stepSize;
        c =+ stepSize;

        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, a);
        fa = func(tmp, coords);

        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, c);
        fc = func(tmp, coords);

        psTrace("psLib.math", 6, "Iter(%d): (a, b, c) is (%f %f %f) (fa, fb, fc) is (%f %f %f)\n", iter, a, b, c, fa, fb, fc);

        if (fa < fb) {
            new_aDir = -1;
        } else {
            new_aDir = 1;
        }

        if (fc < fb) {
            new_cDir = -1;
        } else {
            new_cDir = 1;
        }
        if ((new_aDir == 1) && (aDir == -1)) {
            bracket->data.F32[0] = a;
            bracket->data.F32[1] = b;
            bracket->data.F32[2] = c;
            psFree(tmp);
            psTrace("psLib.math", 4, "---- p_psDetermineBracket() end ----\n");
            return(bracket);
        }

        if ((new_cDir == 1) && (cDir == -1)) {
            bracket->data.F32[0] = a;
            bracket->data.F32[1] = b;
            bracket->data.F32[2] = c;
            psFree(tmp);
            psTrace("psLib.math", 4, "---- p_psDetermineBracket() end ----\n");
            return(bracket);
        }
        aDir = new_aDir;
        cDir = new_cDir;
        iter--;
    }
    psFree(tmp);
    psFree(bracket);
    psTrace("psLib.math", 4, "---- p_psDetermineBracket() end (NULL) ----\n");
    return(NULL);
}


#define RETURN_FINAL_BRACKET(d) \
if (a < c) { \
    bracket->data.F32[0] = a; \
    bracket->data.F32[1] = b; \
    bracket->data.F32[2] = c; \
} else { \
    bracket->data.F32[0] = c; \
    bracket->data.F32[1] = b; \
    bracket->data.F32[2] = a; \
} \
psTrace("psLib.math", 4, "Final bracket (a, b, c) is (%f %f %f) (fa, fb, fc) is (%f %f %f)\n", a, b, c, fa, fb, fc); \
psTrace("psLib.math", 4, "---- p_psDetermineBracket() end ----\n"); \
psFree(tmp); \
return(bracket); \

#define PS_DETERMINE_BRACKET_MAX_ITERATIONS 100
psVector *p_psDetermineBracket2(
    psVector *params,
    psVector *line,
    const psVector *paramMask,
    const psArray *coords,
    psMinimizePowellFunc func)
{
    psF32 a = 0.0;
    psF32 b = 0.0;
    psF32 c = 0.0;
    psF32 fa = 0.0;
    psF32 fb = 0.0;
    psF32 fc = 0.0;
    psS32 iter = 0;
    psVector *tmp = psVectorAlloc(params->n, PS_TYPE_F32);
    bool boolLineIsNull = true;
    psF32 prevMin = 0.0;
    psS32 countMin = 0;

    psTrace("psLib.math", 4, "---- p_psDetermineBracket() begin ----\n");

    // If the line vector is zero, then return NULL.
    PS_VECTOR_WITH_MASK_F32_CHECK_ZERO_VECTOR(params, paramMask, boolLineIsNull);
    if (boolLineIsNull == true) {
        psTrace("psLib.math", 2, "p_psDetermineBracket() called with zero line vector.\n");
        psTrace("psLib.math", 4, "---- p_psDetermineBracket() end (NULL) ----\n");
        psFree(tmp);
        return(NULL);
    }

    // We determine in what x-direction does the function decrease.
    a = 0.0;
    fa = func(params, coords);
    b = 0.5;
    iter = 0;
    do {
        b*= (1.0 + PS_DETERMINE_BRACKET_STEP_SIZE);
        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, b);
        fb = func(tmp, coords);
    } while ((fabs(fb - fa) < FLT_EPSILON) && (iter++ < 100));

    if (fb > fa) {
        a = b;
        fa = fb;
        b = 0.0;
        fb = func(params, coords);
    }
    c = b;

    // At this point we have (a, b) and we know that (fa >= fb).  Initially, c=b;
    // We keep stretching b out further from "a" until (fc > previous fc).  If
    // that happens, then we have our bracket.
    psVector *bracket = psVectorAlloc(3, PS_TYPE_F32);
    iter = 0;
    while (iter < PS_DETERMINE_BRACKET_MAX_ITERATIONS) {
        psTrace("psLib.math", 6, "psDetermineBracket(): iterationA %d\n", iter);
        c+= (1.0 + PS_DETERMINE_BRACKET_STEP_SIZE) * (c - a);

        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmp, c);
        fc = func(tmp, coords);

        psTrace("psLib.math", 6, "Iteration(%d) (bracket): (a, b, c) is (%f %f %f) (fa, fb, fc) is (%f %f %f)\n", iter, a, b, c, fa, fb, fc);

        if ((fb < fa) && (fb < fc)) {
            RETURN_FINAL_BRACKET();
        } else {
            b = c;
            fb = fc;
        }

        // This code maintains a count of how many times the minimum fc has
        // stayed the same.  If it gets too high, we exit this loop.
        if (fc == prevMin) {
            countMin++;
        } else {
            countMin = 0;
        }
        prevMin = fc;
        if (countMin == 10) {
            RETURN_FINAL_BRACKET();
        }

        iter++;
    }

    psFree(bracket);
    psTrace("psLib.math", 4, "---- p_psDetermineBracket() end (NULL) (BAD) ----\n");
    return(NULL);
}

/******************************************************************************
This routine takes as input a possibly multi-dimensional function, along
with an initial guess at the parameters of that function and vector "line"
of the same size as the parameter vector.  It will minimize the function
along that vector and returns the offset along that vector at which the
minimum is determined.

XXX: This routine is not very efficient in terms of total evaluations of the
function.
XXX: Since this is an internal function, many of the parameter checks are
     redundant.
 *****************************************************************************/
#define PS_LINEMIN_MAX_ITERATIONS 30
static psF32 LineMin(
    psMinimization *min,
    psVector *params,
    psVector *line,
    const psVector *paramMask,
    const psArray *coords,
    psMinimizePowellFunc func)
{
    PS_ASSERT_PTR_NON_NULL(min, NAN);
    PS_ASSERT_VECTOR_NON_NULL(params, NAN);
    PS_ASSERT_VECTOR_NON_EMPTY(params, NAN);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, NAN);
    PS_ASSERT_VECTOR_NON_NULL(line, NAN);
    PS_ASSERT_VECTOR_NON_EMPTY(line, NAN);
    PS_ASSERT_VECTOR_TYPE(line, PS_TYPE_F32, NAN);
    PS_ASSERT_VECTOR_NON_NULL(paramMask, NAN);
    PS_ASSERT_VECTOR_NON_EMPTY(paramMask, NAN);
    PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, NAN);
    PS_ASSERT_PTR_NON_NULL(coords, NAN);
    PS_ASSERT_PTR_NON_NULL(func, NAN);
    psVector *bracket;
    psF32 a = 0.0;
    psF32 b = 0.0;
    psF32 c = 0.0;
    psF32 n = 0.0;
    psF32 fn = 0.0;
    psF32 mul = 0.0;
    psS32 i = 0;
    psS32 boolLineIsNull = true;
    psS32 numIterations = 0;

    psTrace("psLib.math", 4, "---- LineMin() begin ----\n");
    PS_VECTOR_F32_CHECK_ZERO_VECTOR(line, boolLineIsNull);

    if (boolLineIsNull == true) {
        min->value = func(params, coords);
        psTrace("psLib.math", 2, "LineMin() called with zero line vector.  Return 0.0.  Function value is %f\n", min->value);
        return(0.0);
    }

    if (6 <= psTraceGetLevel("psLib.math")) {
        for (i=0;i<params->n;i++) {
            psTrace("psLib.math", 6, "(params, paramMask, line)[%d] is (%f %d %f)\n", i,
                    params->data.F32[i], paramMask->data.PS_TYPE_VECTOR_MASK_DATA[i], line->data.F32[i]);
        }
    }

    bracket = p_psDetermineBracket2(params, line, paramMask, coords, func);
    if (bracket == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "Could not bracket minimum.  Returning NAN.\n");
        return(NAN);
    }
    numIterations = 0;

    psVector *tmpa = psVectorAlloc(params->n, PS_TYPE_F32);
    psVector *tmpb = psVectorAlloc(params->n, PS_TYPE_F32);
    psVector *tmpc = psVectorAlloc(params->n, PS_TYPE_F32);
    psVector *tmpn = psVectorAlloc(params->n, PS_TYPE_F32);

    while (numIterations < PS_LINEMIN_MAX_ITERATIONS) {
        numIterations++;
        psTrace("psLib.math", 6, "LineMin(): iteration %d\n", numIterations);

        a = bracket->data.F32[0];
        b = bracket->data.F32[1];
        c = bracket->data.F32[2];
        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmpa, a);
        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmpb, b);
        PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, tmpc, c);
        psF32 fb = func(tmpb, coords);
# if (PS_TRACE_ON)
        psF32 fa = func(tmpa, coords);
        psF32 fc = func(tmpc, coords);
        psTrace("psLib.math", 6, "LineMin: f(%f %f %f) is (%f %f %f)\n", a, b, c, fa, fb, fc);
# endif

        // We determine which is the biggest segment in [a,b,c] then split
        // that with the point n.
        if ((b-a) > (c-b)) {
            // This is the golden section formula
            n = a + (0.69 * (b-a));
            for (i=0;i<params->n;i++) {
                tmpn->data.F32[i] = params->data.F32[i] + (n * line->data.F32[i]);
            }
            fn = func(tmpn, coords);

            if (fn > fb) {
                // a = n, b = b, c = c
                bracket->data.F32[0] = n;
            } else {
                // a = a, b = n, c = b
                bracket->data.F32[1] = n;
                bracket->data.F32[2] = b;
            }
        } else {
            n = b + (0.69 * (c-b));
            for (i=0;i<params->n;i++) {
                tmpn->data.F32[i] = params->data.F32[i] + (n * line->data.F32[i]);
            }
            fn = func(tmpn, coords);

            if (fn > fb) {
                // a = a, b = b, c = n
                bracket->data.F32[2] = n;
            } else {
                // a = b, b = n, c = c
                bracket->data.F32[0] = b;
                bracket->data.F32[1] = n;
            }
        }
        psTrace("psLib.math", 6, "LineMin: new bracket is (%f %f %f)\n", bracket->data.F32[0], bracket->data.F32[1], bracket->data.F32[2]);

        mul = bracket->data.F32[1];
        if ((fabs(a-b) < min->minTol) && (fabs(b-c) < min->minTol)) {
            PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, params, mul);
            min->value = func(params, coords);
            psFree(bracket);
            psTrace("psLib.math", 4, "---- LineMin() end.a (%f) (%f) ----\n", mul, min->value);
            psFree(tmpa);
            psFree(tmpb);
            psFree(tmpc);
            psFree(tmpn);
            return(mul);
        }
    }

    mul = bracket->data.F32[1];
    PS_VECTOR_ADD_MULTIPLE(params, paramMask, line, params, mul);
    min->value = func(params, coords);
    psTrace("psLib.math", 4, "---- LineMin() end.b (%f) %f ----\n", mul, min->value);

    psFree(bracket);
    psFree(tmpa);
    psFree(tmpb);
    psFree(tmpc);
    psFree(tmpn);
    return(mul);
}


/******************************************************************************
This routine must minimize a possibly multi-dimensional function.  The
function to be minimized "func" is:
    psF32 func(psVector *params, psArray *coords)
The "params" are the parameters of the function which are varied.  The data
points at which the function is varied are in the argument "coords" which is
a psArray of psVectors: each vector represents a different coordinate.

XXX: We do not use Brent's method.
 *****************************************************************************/
#define PS_MINIMIZE_POWELL_LINEMIN_MAX_ITERATIONS 20
#define PS_MINIMIZE_POWELL_LINEMIN_ERROR_TOLERANCE 0.01

bool psMinimizePowell(
    psMinimization *min,
    psVector *params,
    const psVector *paramMask,
    const psArray *coords,
    psMinimizePowellFunc func)
{
    PS_ASSERT_PTR_NON_NULL(min, NULL);
    PS_ASSERT_VECTOR_NON_NULL(params, NULL);
    PS_ASSERT_VECTOR_NON_EMPTY(params, NULL);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, NULL);
    PS_ASSERT_PTR_NON_NULL(coords, NULL);
    PS_ASSERT_PTR_NON_NULL(func, NULL);
    psS32 numDims = params->n;
    psS32 i = 0;
    psS32 j = 0;
    psVector *myParamMask = NULL;
    psMinimization dummyMin;
    psF32 mul = 0.0;
    psF32 baseFuncVal = 0.0;
    psF32 currFuncVal = 0.0;
    psS32 biggestIter = 0;
    psF32 biggestDiff = 0.0;
    psS32 iterationNumber = 0;

    psTrace("psLib.math", 4, "---- psMinimizePowell() begin ----\n");
    psTrace("psLib.math", 6, "min->maxIter is %d\n", min->maxIter);
    psTrace("psLib.math", 6, "min->minTol is %f\n", min->minTol);

    if (paramMask == NULL) {
        myParamMask = psVectorAlloc(params->n, PS_TYPE_VECTOR_MASK);
        psVectorInit(myParamMask, 0);
    } else {
        myParamMask = (psVector *) paramMask;
    }
    PS_ASSERT_VECTORS_SIZE_EQUAL(params, myParamMask, NULL);


    psVector *pQP = psVectorAlloc(numDims, PS_TYPE_F32);
    psVector *u   = psVectorAlloc(numDims, PS_TYPE_F32);
    psVector *Q   = psVectorAlloc(numDims, PS_TYPE_F32);

    // 1: Set v[i] to be the unit vectors for each dimension in params
    psArray *v = psArrayAlloc(numDims);
    for (i=0;i<numDims;i++) {
        (v->data[i]) = (psVector *) psVectorAlloc(numDims, PS_TYPE_F32);
        for (j=0;j<numDims;j++) {
            if (i == j) {
                ((psVector *) (v->data[i]))->data.F32[j] = 1.0;
            } else {
                ((psVector *) (v->data[i]))->data.F32[j] = 0.0;
            }
        }
    }

    // 2: Set Q to be the initial params (P in the ADD)
    for (i=0;i<numDims;i++) {
        Q->data.F32[i] = params->data.F32[i];
        Q->n++;
    }

    while (iterationNumber < min->maxIter) {
        iterationNumber++;
        psTrace("psLib.math", 6, "psMinimizePowell() iteration %d\n", iterationNumber);

        // 3: For each dimension in params, move Q only in the vector v[i] to
        //    minimize the function.

        baseFuncVal = func(Q, coords);
        currFuncVal = baseFuncVal;
        psTrace("psLib.math", 6, "Current function value is %f\n", currFuncVal);

        biggestDiff = 0;
        biggestIter = 0;
        for (i=0;i<numDims;i++) {
            if (myParamMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {

                P_PSMINIMIZATION_SET_MAXITER((&dummyMin),PS_MINIMIZE_POWELL_LINEMIN_MAX_ITERATIONS);
		P_PSMINIMIZATION_SET_MIN_TOL((&dummyMin),PS_MINIMIZE_POWELL_LINEMIN_ERROR_TOLERANCE);

                mul = LineMin(&dummyMin, Q, ((psVector *) v->data[i]),
                              myParamMask, coords, func);
                if (isnan(mul)) {
                    psError(PS_ERR_UNKNOWN, false,
                            "Could not perform line minimization.  Returning FALSE.\n");
                    psFree(v);
                    psFree(pQP);
                    psFree(u);
                    psFree(Q);
                    psFree(myParamMask);
                    return(false);
                }
                psTrace("psLib.math", 6, "LineMin along dimension %d has multiple %f\n", i, mul);

                if (fabs(dummyMin.value - currFuncVal) > biggestDiff) {
                    biggestDiff = fabs(dummyMin.value - currFuncVal);
                    biggestIter = i;
                }
                currFuncVal = dummyMin.value;
            }
            // XXX: how can it be that we are not saving mul anywhere?
        }
        psTrace("psLib.math", 6, "New function value is %f\n", currFuncVal);
        // XXX: There must be a bug here.  How can currFuncVal be the current function value?
        // It is simply the minimum along one of the parameter dimensions.

        // 4: Set the vector u = Q - P
        for (i=0;i<numDims;i++) {
            if (myParamMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {
                u->data.F32[i] = Q->data.F32[i] - params->data.F32[i];
                u->n++;

                psTrace("psLib.math", 6, "u[i]=Q[i]-P[i] (%f = %f - %f)\n", u->data.F32[i],
                        Q->data.F32[i],
                        params->data.F32[i]);

            } else {
                u->data.F32[i] = 0.0;
                u->n++;
            }
        }

        // 5: Move Q only in the direction u, and minimize the function.
        for (i=0;i<numDims;i++) {
            psTrace("psLib.math", 6, "u[i] is %f\n", u->data.F32[i]);
        }

        mul = LineMin(&dummyMin, params, u, myParamMask, coords, func);
        if (isnan(mul)) {
            psError(PS_ERR_UNKNOWN, false,
                    "Could not perform line minimization.  Returning FALSE.\n");
            psFree(v);
            psFree(pQP);
            psFree(u);
            psFree(Q);
            psFree(myParamMask);
            return(false);
        }

        // 6:
        if (dummyMin.value > currFuncVal) {
            psFree(v);
            psFree(pQP);
            psFree(u);
            psFree(Q);
            min->iter = iterationNumber;
            min->value = currFuncVal;
            min->lastDelta = 0.0;
            psTrace("psLib.math", 4, "---- psMinimizePowell() end (1)(true) ----\n");
            psFree(myParamMask);
            return(true);
        }

        for (i=0;i<numDims;i++) {
            if (myParamMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {
                pQP->data.F32[i] = (2 * Q->data.F32[i]) - params->data.F32[i];
            } else {
                pQP->data.F32[i] = params->data.F32[i];
            }
        }
        psF32 fqp = func(pQP, coords);
        psF32 term1 = (baseFuncVal - currFuncVal) - biggestDiff;
        term1*= term1;
        term1*= 2.0 * (baseFuncVal - (2.0 * currFuncVal) + fqp);
        psF32 term2 = baseFuncVal - fqp;
        term2*= term2 * biggestDiff;
        if (term1 < term2) {
            for (i=0;i<numDims;i++) {
                if (myParamMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {
                    ((psVector *) v->data[biggestIter])->data.F32[i] = u->data.F32[i];
                }
            }
        }

        // 7: Set P to Q
        for (i=0;i<numDims;i++) {
            if (myParamMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {
                params->data.F32[i] = Q->data.F32[i];
            }
        }

        // 8: Go to step 3 until the change is less than some tolerance.
        if (fabs(baseFuncVal - currFuncVal) <= min->minTol) {
            psFree(v);
            psFree(pQP);
            psFree(u);
            psFree(Q);
            // XXX: Ensure that currFuncVal is the correct value to use here.
            min->value = currFuncVal;
            min->iter = iterationNumber;
            min->lastDelta = currFuncVal - baseFuncVal;
            psTrace("psLib.math", 4, "---- psMinimizePowell() end (2) (true) ----\n");
            psFree(myParamMask);
            return(true);
        }
    }

    psFree(v);
    psFree(pQP);
    psFree(u);
    psFree(Q);
    min->iter = iterationNumber;
    psTrace("psLib.math", 4, "---- psMinimizePowell() end (0) (false) ----\n");

    psFree(myParamMask);
    return(false);
}


/******************************************************************************
This routine is to be used with the psMinimizeChi2Powell() function below.
and the psMinimizePowell() function above.

The basic idea is calculate chi-squared for a set of params/coords/errors.
This functions uses global variables to receive the function pointer, the
data values, and the data errors.
 *****************************************************************************/
static psF32 myPowellChi2Func(
    const psVector *params,
    const psArray *coords)
{
    psTrace("psLib.math", 4, "---- myPowellChi2Func() begin ----\n");
    PS_ASSERT_VECTOR_NON_NULL(params, NAN);
    PS_ASSERT_VECTOR_NON_EMPTY(params, NAN);
    PS_ASSERT_PTR_NON_NULL(coords, NAN);

    psF32 chi2 = 0.0;
    psF32 d;
    psS32 i;
    psVector *tmp;

    psVector *values = coords->data[coords->n];
    psVector *errors = coords->data[coords->n + 1];
    psMinimizeChi2PowellFunc *func = coords->data[coords->n + 2];

    PS_ASSERT_VECTOR_NON_NULL(values, NAN);
    PS_ASSERT_VECTOR_NON_EMPTY(values, NAN);
    PS_ASSERT_VECTOR_TYPE(values, PS_TYPE_F32, NAN);
    if (errors) {
        PS_ASSERT_VECTOR_NON_NULL(errors, NAN);
        PS_ASSERT_VECTOR_NON_EMPTY(errors, NAN);
        PS_ASSERT_VECTOR_TYPE(errors, PS_TYPE_F32, NAN);
        PS_ASSERT_VECTORS_SIZE_EQUAL(values, errors, NAN);
    }

    tmp = (*func)(params, coords);

    if (errors == NULL) {
        for (i=0;i<coords->n;i++) {
            d = (tmp->data.F32[i] - values->data.F32[i]);
            chi2+= d * d;
        }
    } else {
        for (i=0;i<coords->n;i++) {
            d = (tmp->data.F32[i] - values->data.F32[i]) / errors->data.F32[i];
            chi2+= d * d;
        }
    }
    psFree(tmp);
    psTrace("psLib.math", 4, "---- myPowellChi2Func() end (chi2 is %f) ----\n", chi2);
    return(chi2);
}


/******************************************************************************
This routine must minimize the chi-squared match of a set of data points and
values for a possibly multi-dimensional function.

The basic idea is to use the psMinimizePowell() function defined above.  In
order to do so, we defined above a function myPowellChi2Func() which takes
the "func" function and returns chi-squared over the params/coords/values.
We then use that function myPowellChi2Func() in the call to
psMinimizePowell().
 *****************************************************************************/
bool psMinimizeChi2Powell(
    psMinimization *min,
    psVector *params,
    psMinConstraint *constraint,
    const psArray *coords,
    const psVector *value,
    const psVector *error,
    psMinimizeChi2PowellFunc model)
{
    PS_ASSERT_VECTOR_NON_NULL(params, false);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    PS_ASSERT_ARRAY_NON_NULL(coords, false);
    PS_ASSERT_VECTOR_NON_NULL(value, false);
    PS_ASSERT_VECTOR_TYPE(value, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(coords, value, false);

    // Generate extended version of coords array, so we can pass in extra data to the chi^2 function
    psArray *newCoords = psArrayAlloc(coords->n + 3);
    for (long i = 0; i < coords->n; i++) {
        newCoords->data[i] = psMemIncrRefCounter(coords->data[i]);
    }
    newCoords->n = coords->n;           // We deceive everyone else as to the length
    // Casting away const: I'm not going to hurt you, just want to increment your reference counter is all
    newCoords->data[coords->n] = psMemIncrRefCounter((psVector*)value);
    newCoords->data[coords->n + 1] = psMemIncrRefCounter((psVector*)error);
    newCoords->data[coords->n + 2] = &model;

    bool success = psMinimizePowell(min, params, constraint ? constraint->paramMask : NULL,
                                    newCoords, myPowellChi2Func);

    newCoords->data[coords->n - 1] = NULL; // We can't free the array with a function pointer on it
    psFree(newCoords);
    return success;
}


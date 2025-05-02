/** @file  tst_psVectorSort_02.c
 *
 *  @brief Test driver for psVectorSort functions
 *
 *  This test driver contains the following tests for psVectorSort test point 2:
 *    Attempt to sort with null input vector
 *    Attempt to sort input vector with unallowed type
 *    Sort input vector with zero elements
 *    Sort input vector with one element
 *    Sort vectors by index for all types: first with a NULL output vector, then not.
 *    Sort with output vector which needs to be resized
 *    Sort with output vector with different type
 *
 *  @author  Ross Harman, GLG, MHPCC
 *
 *  @version $Revision: 1.3 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-11-29 02:51:08 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define VERBOSE 0

#define tstVectorSortIndexByType(datatype, boolOutNull, value) \
{ \
    psMemId id = psMemGetId(); \
    psVector *out = NULL; \
    psVector *in = psVectorAlloc(5, PS_TYPE_##datatype); \
    in->data.datatype[0] = 7+value; \
    in->data.datatype[1] = 9+value; \
    in->data.datatype[2] = 5+value; \
    in->data.datatype[3] = 1+value; \
    in->data.datatype[4] = 5+value; \
    if (boolOutNull) { \
        out = NULL; \
    } else { \
        out = psVectorAlloc(5, PS_TYPE_##datatype); \
    } \
    psVector *tempVec = out; \
    out = psVectorSortIndex(out, in); \
    if (!boolOutNull) { \
        ok(tempVec == out, "Return value equal to orignal output argument passed to function"); \
    } \
    skip_start(out == NULL, 6, "Skipping tests because psVectorSortIndex() returned NULL"); \
    ok(out->type.type == PS_TYPE_S32, "Output vector is of type PS_TYPE_S32"); \
    ok(out->n == 5, "psVectorSortIndex() returned correct size vector"); \
    ok(out->data.U32[0] == 3, "index sort out[0] = %ld",out->data.U32[0]); \
    ok(out->data.U32[1] == 2, "index sort out[1] = %ld",out->data.U32[1]); \
    ok(out->data.U32[2] == 4, "index sort out[2] = %ld",out->data.U32[2]); \
    ok(out->data.U32[3] == 0, "index sort out[3] = %ld",out->data.U32[3]); \
    ok(out->data.U32[4] == 1, "index sort out[4] = %ld",out->data.U32[4]); \
    skip_end(); \
    psFree(in); \
    psFree(out); \
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
}\


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(196);


    // Attempt to sort with null input vector
    {
        psMemId id = psMemGetId();
        psVector *out = psVectorSortIndex(NULL, NULL);
        ok(out == NULL, "psVectorSortIndex() return NULL with NULL input specified");
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to sort input vector with unallowed type
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(5, PS_TYPE_BOOL);
        psVector *out = psVectorSortIndex(NULL, in);
        ok(out == NULL, "psVectorSortIndex() returned NULL with unallowed input vector type");
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort input vector with zero elements
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(0, PS_TYPE_U8);
        psVector *out = psVectorSortIndex(NULL, in);
        ok(out != NULL, "psVectorSortIndex() returned non-NULL with size 0 input vector");
        ok(out->n == 0, "psVectorSortIndex() returned correct size vector with size 0 input vector");
        psFree(out);
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort input vector with one element
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(1, PS_TYPE_U8);
        psVector *out = psVectorSortIndex(NULL, in);
        ok(out != NULL, "psVectorSortIndex() returned non-NULL with size 0 input vector");
        ok(out->n == 1, "psVectorSortIndex() returned correct size vector with size 0 input vector");
        psFree(out);
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort vectors by index for all types: first with a NULL output vector, then not.
    {
        tstVectorSortIndexByType(S8, true, 0)
        tstVectorSortIndexByType(U8, true, 1)
        tstVectorSortIndexByType(S16, true, 2)
        tstVectorSortIndexByType(U16, true, 3)
        tstVectorSortIndexByType(S32, true, 4)
        tstVectorSortIndexByType(U32, true, 5)
        tstVectorSortIndexByType(S64, true, 6)
        tstVectorSortIndexByType(U64, true, 7)
        tstVectorSortIndexByType(F32, true, 8)
        tstVectorSortIndexByType(F64, true, 9)
        tstVectorSortIndexByType(S8, false, 0)
        tstVectorSortIndexByType(U8, false, 1)
        tstVectorSortIndexByType(S16, false, 2)
        tstVectorSortIndexByType(U16, false, 3)
        tstVectorSortIndexByType(S32, false, 4)
        tstVectorSortIndexByType(U32, false, 5)
        tstVectorSortIndexByType(S64, false, 6)
        tstVectorSortIndexByType(U64, false, 7)
        tstVectorSortIndexByType(F32, false, 8)
        tstVectorSortIndexByType(F64, false, 9)
    }


    // Sort with output vector which needs to be resized
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(5, PS_TYPE_U8);
        for(psS32 m=0; m<5; m++) {
            in->data.U8[m]= 20-m;
        }
        psVector *out = psVectorAlloc(3,PS_TYPE_U32);
        out = psVectorSortIndex(out,in);
        ok(out->n == 5, "Did properly resize output vector...out->n=%d, in->n=%d", out->n, in->n);
        ok(out->data.U32[0] == 4, "Did properly sort index out[0] = %d",out->data.U32[0]);
        ok(out->data.U32[1] == 3, "Did properly sort index out[1] = %d",out->data.U32[1]);
        ok(out->data.U32[2] == 2, "Did properly sort index out[2] = %d",out->data.U32[2]);
        ok(out->data.U32[3] == 1, "Did properly sort index out[3] = %d",out->data.U32[3]);
        ok(out->data.U32[4] == 0, "Did properly sort index out[4] = %d",out->data.U32[4]);
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort with output vector with different type
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(5, PS_TYPE_U8);
        for(psS32 m=0; m<5; m++) {
            in->data.U8[m]= 20-m;
        }
        psVector *out = psVectorAlloc(3,PS_TYPE_F64);
        out = psVectorSortIndex(out,in);
        ok(out->n == 5, "Did properly resize output vector...out->n=%d, in->n=%d", out->n, in->n);
        ok(out->data.U32[0] == 4, "Did properly sort index out[0] = %d",out->data.U32[0]);
        ok(out->data.U32[1] == 3, "Did properly sort index out[1] = %d",out->data.U32[1]);
        ok(out->data.U32[2] == 2, "Did properly sort index out[2] = %d",out->data.U32[2]);
        ok(out->data.U32[3] == 1, "Did properly sort index out[3] = %d",out->data.U32[3]);
        ok(out->data.U32[4] == 0, "Did properly sort index out[4] = %d",out->data.U32[4]);
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort large input vector
    {
        #define N 1000000
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(N, PS_TYPE_S32);
        for (int i = 0 ; i < N ; i++) {
            in->data.S32[N-i-1] = i;
        }
        psVector *out = psVectorSortIndex(NULL, in);
        skip_start(out == NULL,  7, "Skipping tests because psVectorSort() returned NULL.");
        bool errorFlag = false;
        for (int i = 0 ; i < N ; i++) {
            if (out->data.U32[i] != N-i-1) {
                if (VERBOSE) {
                    diag("Test error: out[%d] is %d, should be %d\n", i, out->data.U32[i], i);
                }
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psVectorSortIndex() correctly sorted a large input vector");
        skip_end();
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

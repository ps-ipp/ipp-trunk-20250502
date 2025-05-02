/** @file  tst_psVectorSort_01.c
 *
 *  @brief Test driver for psVectorSort functions
 *
 *  This file is amalgamated from: tst_psVectorSort_01.c, tst_psVectorSort_03.c
 *  tst_psVectorSort_04.c
 *
 *  Attempt to sort NULL input vector
 *  Attempt to sort vector with unallowed type (i.e. PS_TYPE_BOOL)
 *  Sort input vector with zero items
 *  Sort vector with one element
 *  Verify the sort for all supported types.
 *  Sort input vector into itself
 *  Output vector is smaller than input vector
 *  Output vector is different type from input vector
 *
 *  @author  Ross Harman, GLG, MHPCC
 *
 *  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-05-08 06:21:16 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define VERBOSE 0

#define tstVectorSortByType(datatype, boolOutNull, value) \
{ \
    psMemId id = psMemGetId(); \
    psVector *out = NULL; \
    psVector *in = psVectorAlloc(7, PS_TYPE_##datatype); \
    if (boolOutNull) { \
        out = NULL; \
    } else { \
        out = psVectorAlloc(7,PS_TYPE_##datatype); \
    } \
    in->data.datatype[0] = 7+value; \
    in->data.datatype[1] = 9+value; \
    in->data.datatype[2] = 3+value; \
    in->data.datatype[3] = 1+value; \
    in->data.datatype[4] = 5+value; \
    in->data.datatype[5] = 5+value; \
    in->data.datatype[6] = 0+value; \
    psVector *tempVec = out; \
    out = psVectorSort(out, in); \
    if (!boolOutNull) { \
        ok(tempVec == out, "Return value equal to orignal output argument passed to function"); \
    } \
    ok(out->type.type == PS_TYPE_##datatype, "psVectorSort() returned vector with correct type"); \
    ok(out->n == 7, "psVectorSort() returned correct size vector"); \
    skip_start(out == NULL, 7,"Skipping tests because psVectorSort() returned NULL."); \
    ok(out->data.datatype[0] == in->data.datatype[6], "sort out[0] type %s",#datatype); \
    ok(out->data.datatype[1] == in->data.datatype[3], "sort out[1] type %s",#datatype); \
    ok(out->data.datatype[2] == in->data.datatype[2], "sort out[2] type %s",#datatype); \
    ok(out->data.datatype[3] == in->data.datatype[4], "sort out[3] type %s",#datatype); \
    ok(out->data.datatype[4] == in->data.datatype[5], "sort out[4] type %s",#datatype); \
    ok(out->data.datatype[5] == in->data.datatype[0], "sort out[5] type %s",#datatype); \
    ok(out->data.datatype[6] == in->data.datatype[1], "sort out[6] type %s",#datatype); \
    skip_end(); \
    psFree(in); \
    psFree(out); \
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
}


psS32 main(psS32 argc,
           char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(246);


    // Attempt to sort NULL input vector
    {
        psMemId id = psMemGetId();
        psVector *out = psVectorSort(NULL, NULL);
        ok(out == NULL, "psVectorSort returned a NULL for NULL input vector" );
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to sort vector with unallowed type
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(7, PS_TYPE_BOOL);
        psVector *out = psVectorSort(NULL, in);
        ok(out == NULL, "Did return NULL on error");
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort vector with zero elements
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(0, PS_TYPE_F32);
        psVector *out = psVectorSort(NULL, in);
        ok(out != NULL, "psVectorSort returned a non-NULL vector" );
        skip_start(out == NULL, 1, "psVectorSort returned a NULL vector" );
        ok(out->n == 0, "psVectorSort correctly return a 0-length vector");
        skip_end();
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Sort vector with one element
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(1, PS_TYPE_F32);
        psVector *out = psVectorSort(NULL, in);
        ok(out != NULL, "psVectorSort returned a non-NULL vector" );
        skip_start(out == NULL, 1, "psVectorSort returned a NULL vector" );
        ok(out->n == 1, "psVectorSort correctly return a 1-length vector");
        skip_end();
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify the sort for all supported types.
    {
        // At first we test with a NULL out vector.
        tstVectorSortByType(S8, true, 0);
        tstVectorSortByType(S16, true, 1);
        tstVectorSortByType(S32, true, 2);
        tstVectorSortByType(S64, true, 3);
        tstVectorSortByType(U8, true, 4);
        tstVectorSortByType(U16, true, 5);
        tstVectorSortByType(U32, true, 6);
        tstVectorSortByType(U64, true, 7);
        tstVectorSortByType(F32, true, 8);
        tstVectorSortByType(F64, true, 9);
        // We test with a non-NULL out vector.
        tstVectorSortByType(S8, false, 0);
        tstVectorSortByType(S16, false, 1);
        tstVectorSortByType(S32, false, 2);
        tstVectorSortByType(S64, false, 3);
        tstVectorSortByType(U8, false, 4);
        tstVectorSortByType(U16, false, 5);
        tstVectorSortByType(U32, false, 6);
        tstVectorSortByType(U64, false, 7);
        tstVectorSortByType(F32, false, 8);
        tstVectorSortByType(F64, false, 9);
    }


    // Sort input vector into itself
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(7, PS_TYPE_F32);
        in->data.F32[0] = 0.7f;
        in->data.F32[1] = 0.9f;
        in->data.F32[2] = 0.3f;
        in->data.F32[3] = 0.1f;
        in->data.F32[4] = 0.5f;
        in->data.F32[5] = 0.5f;
        in->data.F32[6] = -2.0f;
        in = psVectorSort(in, in);
        skip_start(in == NULL,  7, "Skipping tests because psVectorSort() returned NULL.");
        ok(in->data.F32[0] == -2.0f,"self sort in[0]");
        ok(in->data.F32[1] == 0.1f,"self sort in[1]");
        ok(in->data.F32[2] == 0.3f, "self sort in[2]");
        ok(in->data.F32[3] == 0.5f, "self sort in[3]");
        ok(in->data.F32[4] == 0.5f, "self sort in[4]");
        ok(in->data.F32[5] == 0.7f, "self sort in[5]");
        ok(in->data.F32[6] == 0.9f, "self sort in[6]");
        skip_end();
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Output vector is smaller than input vector
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(7, PS_TYPE_F32);
        psVector *out = psVectorAlloc(3, PS_TYPE_F32);
        in->data.F32[0] = 0.7f;
        in->data.F32[1] = 0.9f;
        in->data.F32[2] = 0.3f;
        in->data.F32[3] = 0.1f;
        in->data.F32[4] = 0.5f;
        in->data.F32[5] = 0.5f;
        in->data.F32[6] = -2.0f;
        out = psVectorSort(out, in);
        skip_start(out == NULL,  7, "Skipping tests because psVectorSort() returned NULL.");
        ok(out->data.F32[0] == -2.0f,"Smaller out vector: in[0]");
        ok(out->data.F32[1] == 0.1f,"Smaller out vector: in[1]");
        ok(out->data.F32[2] == 0.3f, "Smaller out vector: in[2]");
        ok(out->data.F32[3] == 0.5f, "Smaller out vector: in[3]");
        ok(out->data.F32[4] == 0.5f, "Smaller out vector: in[4]");
        ok(out->data.F32[5] == 0.7f, "Smaller out vector: in[5]");
        ok(out->data.F32[6] == 0.9f, "Smaller out vector: in[6]");
        skip_end();
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Output vector is different type from input vector
    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(7, PS_TYPE_F32);
        psVector *out = psVectorAlloc(7, PS_TYPE_F64);
        in->data.F32[0] = 0.7f;
        in->data.F32[1] = 0.9f;
        in->data.F32[2] = 0.3f;
        in->data.F32[3] = 0.1f;
        in->data.F32[4] = 0.5f;
        in->data.F32[5] = 0.5f;
        in->data.F32[6] = -2.0f;
        out = psVectorSort(out, in);
        skip_start(out == NULL,  7, "Skipping tests because psVectorSort() returned NULL.");
        ok(out->data.F32[0] == -2.0f,"Smaller out vector: in[0]");
        ok(out->data.F32[1] == 0.1f,"Smaller out vector: in[1]");
        ok(out->data.F32[2] == 0.3f, "Smaller out vector: in[2]");
        ok(out->data.F32[3] == 0.5f, "Smaller out vector: in[3]");
        ok(out->data.F32[4] == 0.5f, "Smaller out vector: in[4]");
        ok(out->data.F32[5] == 0.7f, "Smaller out vector: in[5]");
        ok(out->data.F32[6] == 0.9f, "Smaller out vector: in[6]");
        skip_end();
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
        psVector *out = psVectorSort(NULL, in);
        skip_start(out == NULL,  7, "Skipping tests because psVectorSort() returned NULL.");
        bool errorFlag = false;
        for (int i = 0 ; i < N ; i++) {
            if (out->data.S32[i] != i) {
                if (VERBOSE) {
                    diag("Test error: out[%d] is %d, should be %d\n", i, out->data.S32[i], i);
                }
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psVectorSort() correctly sorted a large input vector");
        skip_end();
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

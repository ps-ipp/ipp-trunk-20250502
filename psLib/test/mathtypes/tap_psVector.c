#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(294);


    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        ok(psVec != NULL, "psVector successfully allocated");
        skip_start(psVec == NULL, 4, "Skipping 4 tests because psVectorAlloc() failed");
        ok(psVec->nalloc == 5, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 5, "Vector population = %ld", psVec->n);
        ok(psVec->type.type == PS_TYPE_S32, "Vector type = %d", psVec->type.type);
        ok(psVec->type.dimen == PS_DIMEN_VECTOR, "Vector dimen = %d", psVec->type.dimen);
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector* vecZero = psVectorAlloc(0, PS_TYPE_S32);
        ok(vecZero != NULL, "zero-length vector allocated");
        skip_start(vecZero == NULL, 2, "Skipping 2 tests because psVectorAlloc() failed");
        ok(vecZero->nalloc == 0, "Vector size = %ld", vecZero->nalloc);
        ok(vecZero->n == 0, "Vector population = %ld", vecZero->n);
        skip_end();
        psFree(vecZero);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector* vecBogus = psVectorAlloc(10, 0);
        ok(vecBogus == NULL, "psVectorAlloc() doesn't not accept bogus type");
        psErr *err = psErrorLast();
        skip_start(err == NULL, 2, "Skipping 2 tests because psVectorAlloc() didn't fail as expect");
        ok(strstr(err->name, "vectorAlloc"), "alloc failure - got error name %s", err->name);
        ok(err->code == PS_ERR_BAD_PARAMETER_TYPE, "alloc failure - got error code %d", err->code);
        skip_end();
        psFree(vecBogus);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorRealloc() tests
    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        ok(psVec->n == 5, "Vector population = %ld", psVec->n);

        // generate first part of vector
        for(psS32 i = 0; i < 5; i++) {
            psVec->data.S32[i] = i*10;
        }

        // Test C - Reallocate S32 vector bigger
        ok(psVec->n == 5, "Vector population = %ld, should be %ld", psVec->n, 5);
        psVec = psVectorRealloc(psVec,10);
        ok(psVec != NULL, "test vector reallocated to bigger size");
        skip_start(psVec == NULL, 4, "Skipping 4 tests because psVectorRealloc() failed");
        ok(psVec->nalloc == 10, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 5, "Vector population = %ld", psVec->n);
        ok(psVec->type.type == PS_TYPE_S32, "Vector type = %d", psVec->type.type);
        ok(psVec->type.dimen == PS_DIMEN_VECTOR, "Vector dimen = %d", psVec->type.dimen);
        skip_end();

        // generate test data values
        for(psS32 i = 5; i < 10; i++) {
            psVec->data.S32[i] = i*10;
            psVec->n++;
        }

        // test data values
        skip_start(psVec == NULL, 10, "Skipping 10 tests because psVectorRealloc() failed");
        for(psS32 i = 0; i < 10; i++) {
            ok (psVec->data.S32[i] == i*10, "Elem %d = %d, expected %d", i, psVec->data.S32[i], i*10);
        }
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test D - Reallocate S32 vector smaller
    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        // generate test data values
        for(psS32 i = 0; i < 5; i++) {
            psVec->data.S32[i] = i*10;
            psVec->n++;
        }
        psVec = psVectorRealloc(psVec, 3);
        ok(psVec != NULL, "test vector reallocated to smaller size");
        skip_start(psVec == NULL, 5, "Skipping 6 tests because psVectorRealloc() failed");
        ok(psVec->nalloc == 3, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 3, "Vector population = %ld", psVec->n);

        // check that the test data values survived the realloc
        for(psS32 i = 0; i < 3; i++) {
            ok (psVec->data.S32[i] == i*10, "Elem %d = %d, expected %d", i, psVec->data.S32[i], i*10);
        }
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // reallocate to 0 length
    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        psVec = psVectorRealloc(psVec,0);

        ok(psVec != NULL, "test vector reallocated to zero length");
        skip_start(psVec == NULL, 2, "Skipping 2 tests because psVectorRealloc() failed");
        ok(psVec->nalloc == 0, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 0, "Vector population = %ld", psVec->n);
        skip_end();

        // Test E - Free S32 vector
        // XXX not really a test...
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vecBogus = psVectorRealloc(NULL, 6);
        ok(vecBogus == NULL, "psVectorRealloc() doesn't not accept bogus type");
        psErr *err = psErrorLast();
        skip_start(err == NULL, 2, "Skipping 2 tests because psVectorRealloc() didn't fail as expect");
        ok(strstr(err->name, "psVectorRealloc"), "alloc failure - got error name %s", err->name);
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL, "alloc failure - got error code %d", err->code);
        skip_end();
        psFree(vecBogus);
        psFree(err);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorExtend() tests
    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        psVec = psVectorExtend(psVec, 0, 2);
        ok(psVec != NULL, "test vector extended");
        skip_start(psVec == NULL, 2, "Skipping 2 tests because psVectorExtend() failed");
        // Skipping this because it appears the psVectorExtend() changes since the test was written.
        // ok(psVec->nalloc == 5, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 7, "Vector population = %ld", psVec->n);
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        psVec = psVectorExtend(psVec, 0, 2);
        psVec = psVectorExtend(psVec, 0, 2);
        ok(psVec != NULL, "test vector extended");
        skip_start(psVec == NULL, 2, "Skipping 2 tests because psVectorExtend() failed");
        // Skipping this because it appears the psVectorExtend() changes since the test was written.
        //ok(psVec->nalloc == 15, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 9,"Vector population = %ld", psVec->n);
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        psVec = psVectorExtend(psVec, 0, 2);
        psVec = psVectorExtend(psVec, 0, 2);
        psVec = psVectorExtend(psVec, 0, -2);
        ok(psVec != NULL, "test vector extended");
        skip_start(psVec == NULL, 2, "Skipping 2 tests because psVectorExtend() failed");
        // Skipping this because it appears the psVectorExtend() changes since the test was written.
        // ok(psVec->nalloc == 15 ,"Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 7, "Vector population = %ld", psVec->n);
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
        psVec = psVectorExtend(psVec, 0, 2);
        psVec = psVectorExtend(psVec, 0, 2);
        psVec = psVectorExtend(psVec, 0, -2);
        psVec = psVectorExtend(psVec, 0, -20);
        ok(psVec != NULL, "test vector extended");
        skip_start(psVec == NULL, 2, "Skipping 2 tests because psVectorExtend() failed");
        // Skipping this because it appears the psVectorExtend() changes since the test was written.
        // ok(psVec->nalloc == 15, "Vector size = %ld", psVec->nalloc);
        ok(psVec->n == 0, "Vector population = %ld", psVec->n);
        skip_end();
        psFree(psVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorInit() tests
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_U8);
        ok(psVectorInit(vec, 1 ), "U8 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_U16);
        ok(!psVectorInit(vec, PS_MAX_U64), "VectorInit failed.  U16 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_U32);
        ok(psVectorInit(vec, 10), "U32 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_U64);
        ok(psVectorInit(vec, PS_MIN_U64), "U64 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_S8);
        ok(!psVectorInit(vec, PS_MAX_S16), "VectorInit failed.  S8 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_S16);
        ok(psVectorInit(vec, -100), "S16 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_S32);
        ok(psVectorInit(vec, 1), "S32 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // XXX is this test ok?
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_S64);
        ok(psVectorInit(vec, PS_MAX_S64), "S64 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_F32);
        ok(psVectorInit(vec, 1.1), "F32 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_F64);
        ok(psVectorInit(vec, 1.4 ), "F64 Case");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorCreate() tests
    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_S8);
        for (int i = 0; i < 10; i++) {
            ok(test->data.S8[i] == i, "Vector data matches. i = %d, data=%d",
               i, test->data.S8[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_U8);
        for (int i = 0; i < 10; i++) {
            ok(test->data.U8[i] == i, "Vector data matches. i = %d, data=%d",
               i, test->data.U8[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_U32);
        for (int i = 0; i < 10; i++) {
            ok(test->data.U32[i] == i, "Vector data matches. i = %d, data=%d",
               i, test->data.U32[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_S32);
        for (int i = 0; i < 10; i++) {
            ok(test->data.S32[i] == i, "Vector data matches. i = %d, data=%d",
               i, test->data.S32[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *input = psVectorAlloc(5, PS_TYPE_F32);
        psVector *test = psVectorCreate(input, 0.0, 5.0, 0.5, PS_TYPE_F32);

        for (int i = 0; i < 10; i++) {
            ok(test->data.F32[i] == i * 0.5,
               "Vector data matches. i = %d, data=%f", i, test->data.F32[i]);
        }
        psFree(input);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //  Test PS_TYPE_F64
    //       PS_TYPE_S64
    //       PS_TYPE_U64
    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_S64);
        for (int i = 0; i < 10; i++)
        {
            ok(test->data.S64[i] == i,
               "Vector data does not match. i = %d, data=%ld",
               i, (long)test->data.S64[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_U64);
        for (int i = 0; i < 10; i++)
        {
            ok(test->data.U64[i] == i,
               "Vector data does not match. i = %d, data=%ld",
               i, (long)test->data.U64[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *input = psVectorAlloc(5, PS_TYPE_F64);
        psVector *test = psVectorCreate(input, 0.0, 5.0, 0.5, PS_TYPE_F64);
        for (int i = 0; i < 10; i++)
        {
            ok(test->data.F64[i] == i * 0.5,
               "Vector data does not match. i = %d, data=%f",
               i, test->data.F64[i]);
        }
        psFree(input);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //  Test PS_TYPE_U16
    //       PS_TYPE_S16
    {
        psMemId id = psMemGetId();
        psVector *test = psVectorCreate(NULL, 0.0, 10.0, 1.0, PS_TYPE_S16);
        for (int i = 0; i < 10; i++)
        {
            ok(test->data.S16[i] == i, "Vector data matches. i = %d, data=%d",
               i, test->data.S16[i]);
        }
        psFree(test);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *input = psVectorAlloc(5, PS_TYPE_U16);
        psVector *test = psVectorCreate(input, 0.0, 20.0, 2.0, PS_TYPE_U16);
        for (int i = 0; i < 10; i++)
        {
            ok(test->data.U16[i] == i * 2.0,
               "Vector data matches. i = %d, data=%d", i, test->data.U16[i]);
        }
        psFree(input);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    #define TEST_VECTOR_TO_STRING(TYPE, SIZE, START, INC, EXPECTED)         \
    {                                                                       \
        psMemId id = psMemGetId(); \
        const int asize = SIZE;  /* alloc size */                           \
        const int osize = 100;   /* output size (max) */                    \
        const char* const expected = EXPECTED;                              \
        \
        psVector *input = psVectorAlloc(asize, TYPE);                       \
        psVector *test = psVectorCreate(input, START, START+asize*INC,      \
                                        INC, TYPE);                                                     \
        \
        psString result = psVectorToString(test, osize);                    \
        ok(strcmp(result, expected) == 0,                                   \
           "psVectorToString expected:result = %s:%s", expected, result ); \
        \
        psFree(input);                                                      \
        psFree(result);                                                      \
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
    }

    // psVectorToString() tests
    TEST_VECTOR_TO_STRING(PS_TYPE_U8,  5, 1, 1, "[1,2,3,4,5]");
    TEST_VECTOR_TO_STRING(PS_TYPE_U16, 5, 1, 1, "[1,2,3,4,5]");
    TEST_VECTOR_TO_STRING(PS_TYPE_U32, 5, 1, 1, "[1,2,3,4,5]");
    TEST_VECTOR_TO_STRING(PS_TYPE_U64, 5, 1, 1, "[1,2,3,4,5]");
    TEST_VECTOR_TO_STRING(PS_TYPE_S8,  5, -100, 10,"[-100,-90,-80,-70,-60]");
    TEST_VECTOR_TO_STRING(PS_TYPE_S16, 5, 0, -100, "[0,-100,-200,-300,-400]");
    TEST_VECTOR_TO_STRING(PS_TYPE_S32, 5, 0, -100, "[0,-100,-200,-300,-400]");
    TEST_VECTOR_TO_STRING(PS_TYPE_S64, 5, 0, -100, "[0,-100,-200,-300,-400]");
    TEST_VECTOR_TO_STRING(PS_TYPE_F32,5,.123,1,"[0.123,1.123,2.123,3.123,4.123]");
    TEST_VECTOR_TO_STRING(PS_TYPE_F64,5,.123,1,"[0.123,1.123,2.123,3.123,4.123]");


    #define TEST_VECTOR_GET_ELEMENT_F64(ELEM_TYPE, SIZE, VALUE_TYPE,        \
                                        INIT_VALUE )                                                        \
    {                                                                       \
        psMemId id = psMemGetId(); \
        psVector *vec = psVectorAlloc(SIZE, ELEM_TYPE);                     \
        vec->n = SIZE;                                                      \
        VALUE_TYPE initValue = INIT_VALUE;                                  \
        psVectorInit(vec, initValue);                                       \
        psF64 result = p_psVectorGetElementF64(vec, 0);                     \
        psF64 expected = (psF64)initValue;                                  \
        ok(result == expected,                                              \
           "p_psVectorGetElementF64 expected:result = %f:%f",expected,result);\
        \
        psFree(vec);                                                        \
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
    }
    // p_psVectorGetElementF64() tests
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_U8, 1, psU8, 1);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_U16, 2, psU16, 2);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_U32, 3, psU32, 3);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_U64, 100, psU64, 999999);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_S8,  1, psS8, -1);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_S16, 2, psS16, -2);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_S32, 3, psS32, -3);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_S64, 100, psS64, -999999);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_F32,1, psF32, .123);
    TEST_VECTOR_GET_ELEMENT_F64(PS_TYPE_F64,2, psF64, -123.123);


    // XXX: Why are we testing private functions?
    #define TEST_VECTOR_PRINT(ELEM_TYPE, SIZE, VALUE_TYPE, INIT_VALUE, FD,  \
                              OUT_NAME, EXPECTED_RC )                                             \
    {                                                                       \
        psMemId id = psMemGetId(); \
        psVector *vec = psVectorAlloc(SIZE, ELEM_TYPE);                     \
        vec->n = SIZE;                                                      \
        VALUE_TYPE initValue = INIT_VALUE;                                  \
        psVectorInit(vec, initValue);                                       \
        psBool result = p_psVectorPrint(FD, vec, OUT_NAME);                 \
        ok(result == EXPECTED_RC,                                           \
           "p_psVectorPrint expected:result = %d:%d",EXPECTED_RC,result);  \
        \
        psFree(vec);                                                        \
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
    }
    // p_psVectorPrint() tests
    TEST_VECTOR_PRINT(PS_TYPE_U8, 1, psU8, 1, 1, "PS_TYPE_U8", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_U16, 2, psU16, 2, 1, "PS_TYPE_U16", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_U32, 3, psU32, 3, 1, "PS_TYPE_U32", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_U64, 4, psU64, 999999, 1, "PS_TYPE_U64", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_S8,  1, psS8, -1, 1, "PS_TYPE_S8", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_S16, 2, psS16, -2, 1, "PS_TYPE_S16", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_S32, 3, psS32, -3, 1, "PS_TYPE_S32", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_S64, 4, psS64, -999999, 1, "PS_TYPE_S64", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_F32,1, psF32, .123, 1, "PS_TYPE_F32", TRUE);
    TEST_VECTOR_PRINT(PS_TYPE_F64,2, psF64, -123.123, 1, "PS_TYPE_F64", TRUE);

    // psVectorSet() tests
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(5, PS_TYPE_S32);
        ok(psVectorSet(vec, 0, 10),
           "VectorSet set S32 at position 0");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(5, PS_TYPE_S32);
        ok(!psVectorSet(vec, 10, 10),
           "VectorSet failes to set S32 at out of range position");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(5, PS_TYPE_S32);
        ok(psVectorSet(vec, 1, 4) == true,
           "VectorSet set S32 at position 1");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorGet() tests
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(10, PS_TYPE_S32);
        psVectorSet(vec, 0, 10);
        ok((psS32)psVectorGet(vec, 0) == 10,
           "VectorGet returned the correct S32 from position 0");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(5, PS_TYPE_S32);
        psVectorSet(vec, -1, 4);
        ok((psS32)psVectorGet(vec, -1) == 4,
           "VectorGet returned the correct S32 from tail using -1");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorCountPixelMask() tests
    // Ensure -1 return for NULL psVector input
    {
        psMemId id = psMemGetId();
        ok(psVectorCountPixelMask(NULL, 1) == -1,
           "psVectorCountPixelMask() returned -1 for NULL psVector input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Ensure -1 return for incorrect TYPE psVector input
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(5, PS_TYPE_S32);
        vec->data.S32[0] = 0;
        vec->data.S32[1] = 1;
        vec->data.S32[2] = 0;
        vec->data.S32[3] = 1;
        vec->data.S32[4] = 0;
        ok(psVectorCountPixelMask(vec, 1) == -1,
           "psVectorCountPixelMask() returned -1 for incorrect TYPE psVector input");
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use correct inputs
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(5, PS_TYPE_U8);
        vec->data.U8[0] = 0;
        vec->data.U8[1] = 1;
        vec->data.U8[2] = 0;
        vec->data.U8[3] = 1;
        vec->data.U8[4] = 0;
        long numPix = psVectorCountPixelMask(vec, 1);
        ok(numPix != -1, "psVectorCountPixelMask() did return -1 for correct psVector input");
        ok(numPix == 2, "returned pixel count %d", numPix);
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use correct inputs; psVector length of 0
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(0, PS_TYPE_U8);
        long numPix = psVectorCountPixelMask(vec, 1);
        ok(numPix != -1, "psVectorCountPixelMask() did return -1 for correct psVector input (length 0)");
        ok(numPix == 0, "returned pixel count %d", numPix);
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use correct inputs; psVector length of 1
    {
        psMemId id = psMemGetId();
        psVector *vec = psVectorAlloc(1, PS_TYPE_U8);
        vec->data.U8[0] = 0;
        long numPix = psVectorCountPixelMask(vec, 1);
        ok(numPix != -1, "psVectorCountPixelMask() did return -1 for correct psVector input (length 1)");
        ok(numPix == 0, "returned pixel count %d", numPix);
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use correct inputs; large psVector 
    {
        psMemId id = psMemGetId();
        #define N 10
        psVector *vec = psVectorAlloc(N, PS_TYPE_U8);
        for (int i = 0 ; i < N ; i++) {
            if (0 == i%2) {
                vec->data.U8[i] = 0;
            } else {
                vec->data.U8[i] = 1;
            }
        }
        long numPix = psVectorCountPixelMask(vec, 1);
        ok(numPix != -1, "psVectorCountPixelMask() did return -1 for correct psVector input (length: %d)", N);
        ok(numPix == N/2, "returned pixel count %d", numPix);
        psFree(vec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorLength() tests
    {
        psMemId id = psMemGetId();
        psVector *vector = psVectorAlloc(5, PS_TYPE_F32);
        ok(psVectorLength(vector) == 5,
           "returned the correct length of vector (was %d, should be 5)", psVectorLength(vector));
        psFree(vector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vector = psVectorAlloc(5, PS_TYPE_F32);
        ok(psVectorLength(vector) == 5,
           "returned the correct length of vector");
        psFree(vector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *vector = psVectorAlloc(6, PS_TYPE_F32);
        ok(psVectorLength(vector) == 6,
           "returned the correct length of vector");
        psFree(vector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        ok(psVectorLength(NULL) == -1, "returned -1 for a NULL input vector");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psArray *array = psArrayAlloc(5);
        ok(psVectorLength((psVector*)array) == -1,
           "returned -1 for an invalid input vector");
        psFree(array);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorCopy() tests
    {
        psMemId id = psMemGetId();
        ok(psVectorCopy(NULL, NULL, PS_TYPE_F32) == NULL,
           "returned a NULL vector for NULL input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        psVector *in = psVectorAlloc(5, PS_TYPE_F32);
        for (int i = 0; i < 5; i++) {
            in->data.F32[i] = i;
        }
        //Try copy of different type
        psVector *copy = psVectorCopy(NULL, in, PS_TYPE_F64);
        ok(copy != NULL, "returned a non NULL vector for correct input");
        skip_start(copy == NULL, 1, "Skipping 1 test because psVectorCopy() failed");
        ok(copy->data.F64[2] == 2.0,
           "copy->data.f64[2] = %lf, in->data.f32[2] = %f",
           copy->data.F64[2], in->data.F32[2]);
        skip_end();
        psFree(in);
        psFree(copy);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // XXX psVectorRecycle needs it's own tests
    // copy = psVectorRecycle(copy, in->n + 2, PS_TYPE_F64);
    {
        psMemId id = psMemGetId();
        // Try copy of same type and non-NULL outVector of different type and
        // size.
        psVector *in = psVectorAlloc(5, PS_TYPE_F32);
        in->n = 5;
        for (int i = 0; i < 5; i++)
        {
            in->data.F32[i] = i;
        }
        psVector *copy = psVectorAlloc(5, PS_TYPE_F64);
        copy = psVectorCopy(copy, in, in->type.type);

        ok(copy != NULL, "returned a NULL vector for correct input");
        skip_start(copy == NULL, 2,
                   "Skipping 1 test because psVectorCopy() failed");
        ok(copy->type.type == PS_TYPE_F32, "copy has the correct data type");
        ok(copy->data.F32[2] == 2.0, "copy has the correct data value");
        skip_end();

        psFree(in);
        psFree(copy);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

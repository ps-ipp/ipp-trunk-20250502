/** @file  tst_psVector.c
 *
 *  @brief Test driver for psVector integer functions
 *
 *  This test driver contains the following tests for psVector test point 1:
 *     A)  Create S32 vector
 *     B)  Add data to S32 vector
 *     C)  Reallocate S32 vector bigger
 *     D)  Reallocate S32 vector smaller
 *     E)  Free S32 vector
 *     F)  Attempt to create a S32 vector with zero size
 *     G)  Attempt to realloc a null S32 vector
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.12 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2006-05-02 22:35:52 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "psTest.h"
#include "pslib_strict.h"

static psS32 testVectorAlloc(void);
static psS32 testVectorRealloc(void);
static psS32 testVectorExtend(void);
static psS32 testVectorInit(void);
static psS32 testVectorCreate(void);
static psS32 testVectorCreateA(void);
static psS32 testVectorCreateB(void);
static psS32 testVectorGetSet(void);
static psS32 testVectorCountPixelMask(void);
static psS32 testVectorLength(void);
static psS32 testVectorCopy(void);

testDescription tests[] = {
                              {testVectorAlloc,-1,"psVectorAlloc",0,false},
                              {testVectorRealloc,-2,"psVectorRealloc",0,false},
                              {testVectorExtend,-3,"psVectorExtend",0,false},
                              {testVectorInit,-4,"psVectorInit",0,false},
                              {testVectorCreate,-5,"psVectorCreate",0,false},
                              {testVectorCreateA,-6,"psVectorCreateA",0,false},
                              {testVectorCreateB,-7,"psVectorCreateB",0,false},
                              {testVectorGetSet,-8,"psVectorGet/Set",0,false},
                              {testVectorCountPixelMask,-9,"psVectorCountPixelMask",0,false},
                              {testVectorLength,666,"psVectorLength",0,false},
                              {testVectorCopy,667,"psVectorCopy",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);
    psLogSetFormat("HLNM");

    if ( ! runTestSuite(stderr,"psVector",tests,argc,argv) ) {
        return 1;
    }
    return 0;
}

psS32 testVectorAlloc(void)
{
    psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);

    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 1;
    }
    if (psVec->nalloc != 5) {
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 1;
    }
    if (psVec->n != 0) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 2;
    }

    if (psVec->type.type != PS_TYPE_S32) {
        fprintf(stderr,"Vector type = %d\n", psVec->type.type);
        return 3;
    }
    if (psVec->type.dimen != PS_DIMEN_VECTOR) {
        fprintf(stderr,"Vector dimen = %d\n", psVec->type.dimen);
        return 4;
    }

    // Test B - Add data to integer vector
    for(psS32 i = 0; i < 5; i++) {
        psVec->data.S32[i] = i*10;
    }

    psVector* vecZero = psVectorAlloc(0, PS_TYPE_S32);
    if(vecZero == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 5;
    }
    if (vecZero->nalloc != 0) {
        fprintf(stderr,"Vector size = %ld\n", vecZero->nalloc);
        return 6;
    }
    if (vecZero->n != vecZero->nalloc) {
        fprintf(stderr,"Vector population = %ld\n", vecZero->n);
        return 7;
    }

    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    psVector* vecBogus = psVectorAlloc(10, 0);
    if (vecBogus != NULL) {
        fprintf(stderr,"Huh!  bogus type generated a psVector?");
        return 20;
    }

    psFree(psVec);
    psFree(vecZero);

    return 0;
}

psS32 testVectorRealloc(void)
{
    // create new psVector
    psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 30;
    }
    for(psS32 i = 0; i < 5; i++) {
        psVec->data.S32[i] = i*10;
        psVec->n++;
    }

    // Test C - Reallocate S32 vector bigger
    psVec = psVectorRealloc(psVec,10);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 1;
    }
    if (psVec->nalloc != 10) {
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 1;
    }
    if (psVec->n != 5) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 2;
    }

    if (psVec->type.type != PS_TYPE_S32) {
        fprintf(stderr,"Vector type = %d\n", psVec->type.type);
        return 3;
    }
    if (psVec->type.dimen != PS_DIMEN_VECTOR) {
        fprintf(stderr,"Vector dimen = %d\n", psVec->type.dimen);
        return 4;
    }

    for(psS32 i = 5; i < 10; i++) {
        psVec->data.S32[i] = i*10;
        psVec->n++;
    }

    for(psS32 i = 0; i < 10; i++) {
        if (psVec->data.S32[i] != i*10) {
            fprintf(stderr,"Elem %d = %d, expected %d\n", i,
                    psVec->data.S32[i], i*10);
            return 5;
        }
    }

    // Test D - Reallocate S32 vector smaller
    psVec = psVectorRealloc(psVec,3);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 9;
    }
    if (psVec->nalloc != 3) {
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 10;
    }
    if (psVec->n != 3) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 11;
    }

    for(psS32 i = 0; i < 3; i++) {
        if (psVec->data.S32[i] != i*10) {
            fprintf(stderr,"Elem %d = %d, expected %d\n", i,
                    psVec->data.S32[i], i*10);
            return 12;
        }
    }

    psVec = psVectorRealloc(psVec,0);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 20;
    }
    if (psVec->nalloc != 0) {
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 21;
    }
    if (psVec->n != 0) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 22;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    psVector* vecBogus = psVectorRealloc(NULL, 6);
    if (vecBogus != NULL) {
        fprintf(stderr,"Huh!  bogus type generated a psVector?");
        return 25;
    }

    // Test E - Free S32 vector
    psFree(psVec);

    return 0;
}

psS32 testVectorExtend(void)
{
    // create new psVector
    psVector *psVec = psVectorAlloc(5, PS_TYPE_S32);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 1;
    }

    psVec->n = 0;

    psVec = psVectorExtend(psVec, 0, 2);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 2;
    }
    if (psVec->nalloc != 5) { // no growth should occur
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 3;
    }
    if (psVec->n != 2) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 4;
    }

    psVec = psVectorExtend(psVec, 0, 2);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 10;
    }
    if (psVec->nalloc != 15) { // growth should occur
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 11;
    }
    if (psVec->n != 4) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 12;
    }

    psVec = psVectorExtend(psVec, 0, -2);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 20;
    }
    if (psVec->nalloc != 15) { // no growth
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 21;
    }
    if (psVec->n != 2) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 22;
    }

    psVec = psVectorExtend(psVec, 0, -20);
    if (psVec == NULL) {
        fprintf(stderr,"ERROR: Return is NULL\n");
        return 30;
    }
    if (psVec->nalloc != 15) { // no growth
        fprintf(stderr,"Vector size = %ld\n", psVec->nalloc);
        return 31;
    }
    if (psVec->n != 0) {
        fprintf(stderr,"Vector population = %ld\n", psVec->n);
        return 32;
    }

    psFree(psVec);

    return 0;
}

psS32 testVectorInit(void)
{
    psVector *in1 = NULL;
    psVector *in2 = NULL;
    psVector *in3 = NULL;
    psVector *in4 = NULL;
    psVector *in5 = NULL;
    psVector *in6 = NULL;
    psVector *in7 = NULL;
    psVector *in8 = NULL;
    psVector *in9 = NULL;
    psVector *in10 = NULL;
    psVector *in11 = NULL;
    psVector *in12 = NULL;
    int nalloc = 1;
    in1 = psVectorAlloc(nalloc, PS_TYPE_U8);
    in2 = psVectorAlloc(nalloc, PS_TYPE_U16);
    in3 = psVectorAlloc(nalloc, PS_TYPE_U32);
    in4 = psVectorAlloc(nalloc, PS_TYPE_U64);
    in5 = psVectorAlloc(nalloc, PS_TYPE_S8);
    in6 = psVectorAlloc(nalloc, PS_TYPE_S16);
    in7 = psVectorAlloc(nalloc, PS_TYPE_S32);
    in8 = psVectorAlloc(nalloc, PS_TYPE_S64);
    in9 = psVectorAlloc(nalloc, PS_TYPE_F32);
    in10 = psVectorAlloc(nalloc, PS_TYPE_F64);
    in11 = psVectorAlloc(nalloc, PS_TYPE_C32);
    in12 = psVectorAlloc(nalloc, PS_TYPE_C64);
    psC32 vC32;
    psC64 vC64;

    if ( !psVectorInit(in1, 1 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  U8 Case \n");
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psVectorInit(in2, PS_MAX_U64) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  U16 Case \n");
    }
    if ( !psVectorInit(in3, 10) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  U32 Case \n");
    }
    if ( !psVectorInit(in4, -1) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  U64 Case \n");
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psVectorInit(in5, PS_MAX_S16 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  S8 Case \n");
    }
    if ( !psVectorInit(in6, -100 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  S16 Case \n");
    }
    if ( !psVectorInit(in7, 1 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  S32 Case \n");
    }
    if ( !psVectorInit(in8, PS_MAX_U64 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  S64 Case \n");
    }
    if ( !psVectorInit(in9, 1.1 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  F32 Case \n");
    }
    if ( !psVectorInit(in10, 1.4 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  F64 Case \n");
    }

    vC32 = 1.23 + 1.19I;
    vC64 = 2.13 + 2.31I;
    if ( !psVectorInit(in11, vC32 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  C32 Case \n");
    }
    if ( !psVectorInit(in12, vC64 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "VectorInit failed.  C64 Case \n");
    }

    psFree(in1);
    psFree(in2);
    psFree(in3);
    psFree(in4);
    psFree(in5);
    psFree(in6);
    psFree(in7);
    psFree(in8);
    psFree(in9);
    psFree(in10);
    psFree(in11);
    psFree(in12);
    return 0;
}

psS32 testVectorCreate(void)
{
    psVector *test = NULL;
    psVector *test2 = NULL;
    psVector *input = NULL;
    psVector *input2 = psVectorAlloc(5, PS_TYPE_F32);

    test = psVectorCreate(input, 0.0, 10.0, 1.0, PS_TYPE_S32);
    test2 = psVectorCreate(input2, 0.0, 5.0, 0.5, PS_TYPE_F32);

    for (int i = 0; i < 10; i++) {
        if (test->data.S32[i] != i) {
            fprintf(stderr, "Vector data does not match. i = %d, data=%d\n",
                    i, test->data.S32[i]);
            return 1;
        }
        if (test2->data.F32[i] != i*0.5) {
            fprintf(stderr, "Vector data does not match. i = %d, data=%f\n",
                    i, test->data.F32[i]);
            return 1;
        }
    }
    psFree(test);
    psFree(test2);
    return 0;
}
psS32 testVectorCreateA(void)
{
    //
    //  Test PS_TYPE_F64
    //       PS_TYPE_S64
    //
    psVector *test = NULL;
    psVector *test2 = NULL;
    psVector *input = NULL;
    psVector *input2 = psVectorAlloc(5, PS_TYPE_F64);

    test = psVectorCreate(input, 0.0, 10.0, 1.0, PS_TYPE_S64);
    test2 = psVectorCreate(input2, 0.0, 5.0, 0.5, PS_TYPE_F64);

    for (int i = 0; i < 10; i++) {
        if (test->data.S64[i] != i) {
            fprintf(stderr, "Vector data does not match. i = %d, data=%ld\n",
                    i, (long)test->data.S64[i]);
            return 1;
        }
        if (test2->data.F64[i] != i*0.5) {
            fprintf(stderr, "Vector data does not match. i = %d, data=%f\n",
                    i, test->data.F64[i]);
            return 1;
        }
    }
    psFree(test);
    psFree(test2);
    return 0;
}
psS32 testVectorCreateB(void)
{
    //
    //  Test PS_TYPE_U16
    //       PS_TYPE_S16
    //
    psVector *test = NULL;
    psVector *test2 = NULL;
    psVector *input = NULL;
    psVector *input2 = psVectorAlloc(5, PS_TYPE_U16);

    test = psVectorCreate(input, 0.0, 10.0, 1.0, PS_TYPE_S16);
    test2 = psVectorCreate(input2, 0.0, 20.0, 2.0, PS_TYPE_U16);

    for (int i = 0; i < 10; i++) {
        if (test->data.S16[i] != i) {
            fprintf(stderr, "Vector data does not match. i = %d, data=%d\n",
                    i, test->data.S16[i]);
            return 1;
        }
        if (test2->data.U16[i] != i*2.0) {
            fprintf(stderr, "Vector data does not match. i = %d, data=%d\n",
                    i, test->data.U16[i]);
            return 1;
        }
    }
    psFree(test);
    psFree(test2);
    return 0;
}
psS32 testVectorGetSet(void)
{
    psVector *vec = NULL;
    vec = psVectorAlloc(5, PS_TYPE_S32);
    vec->n = 0;

    if ( !psVectorSet(vec, 0, 10) ) {
        fprintf(stderr, "VectorSet failed to set S32 at position 0\n");
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psVectorSet(vec, 10, 10) ) {
        fprintf(stderr, "VectorSet Improperly set S32 at out of range position\n");
    }
    if ( !psVectorSet(vec, 1, 4) ) {
        fprintf(stderr, "VectorSet Failed to set S32 at position 1\n");
    }
    if ( (psS32)psVectorGet(vec, 0) != 10 ) {
        fprintf(stderr,
                "VectorGet Failed to return the correct S32 from position 0\n");
    }
    if ( (psS32)psVectorGet(vec, -1) != 4 ) {
        fprintf(stderr,
                "VectorGet Failed to return the correct S32 from tail using -1\n");
    }

    psFree(vec);
    return 0;
}

psS32 testVectorCountPixelMask(void)
{
    long numPix = 0;
    long numPix2 = 0;
    psVector *vec = NULL;

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    numPix = psVectorCountPixelMask(vec, 1);
    if (numPix != -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorCountPixelMask failed to return -1 for NULL Vector input.\n");
        return 1;
    }

    numPix = 0;
    vec = psVectorAlloc(5, PS_TYPE_S32);
    psVector *vec2 = NULL;
    vec2 = psVectorAlloc(5, PS_TYPE_U8);
    vec->data.S32[0] = 0;
    vec->data.S32[1] = 1;
    vec->data.S32[2] = 0;
    vec->data.S32[3] = 1;
    vec->data.S32[4] = 0;
    vec2->data.U8[0] = 0;
    vec2->data.U8[1] = 1;
    vec2->data.U8[2] = 0;
    vec2->data.U8[3] = 1;
    vec2->data.U8[4] = 0;
    vec->n = 5;
    vec2->n = 5;

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    numPix = psVectorCountPixelMask(vec, 1);
    numPix2 = psVectorCountPixelMask(vec2, 1);

    if (numPix != -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorCountPixelMask failed to return -1 for wrong type of Vector input.\n");
        return 2;
    }
    if (numPix2 == -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorCountPixelMask returned -1 for correct Vector input\n");
        return 3;
    }
    if (numPix2 != 2) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorCountPixelMask returned incorrect pixel count %d\n", numPix2);
        return 4;
    }

    psFree(vec);
    psFree(vec2);
    return 0;
}

psS32 testVectorLength( void )
{
    psVector *vector = psVectorAlloc(5, PS_TYPE_F32);

    if (psVectorLength(vector) != 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorLength failed to return the correct length of vector.\n");
        return 1;
    }
    vector->n = 5;
    if (psVectorLength(vector) != 5) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorLength failed to return the correct length of vector.\n");
        return 2;
    }
    vector->n++;
    if (psVectorLength(vector) != 6) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorLength failed to return the correct length of vector.\n");
        return 3;
    }
    psFree(vector);

    psVector *emptyVector = NULL;
    psArray *array = psArrayAlloc(5);

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if (psVectorLength(emptyVector) != -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorLength failed to return -1 for a NULL input vector.\n");
        return 4;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if (psVectorLength((psVector*)array) != -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psVectorLength failed to return -1 for an invalid input vector.\n");
        return 5;
    }
    psFree(array);

    return 0;
}

psS32 testVectorCopy(void)
{

    psVector *in = NULL;
    psVector *copy = NULL;

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    copy = psVectorCopy(copy, in, PS_TYPE_F32);
    if (copy != NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psVectorCopy failed to return a NULL vector for NULL input.\n");
        return 1;
    }

    in = psVectorAlloc(5, PS_TYPE_F32);
    in->n = 5;
    for (int i = 0; i < 5; i++) {
        in->data.F32[i] = i;
    }
    //Try copy of different type
    copy = psVectorCopy(copy, in, PS_TYPE_F64);
    if (copy == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psVectorCopy returned a NULL vector for correct input.\n");
        return 2;
    } else if (copy->data.F64[2] != 2.0) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psVectorCopy failed to return the correct data values.\n");
        printf("\n copy->data.f64[2] = %lf, in->data.f32[2] = %f\n", copy->data.F64[2],
               in->data.F32[2]);
        return 3;
    }

    copy = psVectorRecycle(copy, in->n + 2, PS_TYPE_F64);
    //Try copy of same type and non-NULL outVector of different type and size.
    copy = psVectorCopy(copy, in, in->type.type);
    if (copy == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psVectorCopy returned a NULL vector for correct input.\n");
        return 4;
    } else if (copy->type.type != PS_TYPE_F32 || copy->data.F32[2] != 2.0) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psVectorCopy failed to return the correct data values.\n");
        return 5;
    }


    psFree(in);
    psFree(copy);
    return 0;
}


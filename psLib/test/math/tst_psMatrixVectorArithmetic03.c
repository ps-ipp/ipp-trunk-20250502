/** @file  tst_psMatrixVectorArithmetic03.c
 *
 *  @brief Test driver for psMatrixVector arithmetic functions
 *
 *  This test driver contains negative tests for psBinaryOp and psUanryOp:
 *     Check for NULL arguments
 *     Inconsistent element types
 *     Inconsistent element count
 *     Inconsistent dimensionality
 *     Division by zero
 *     Attempt to use min with complex numbers
 *     Attempt to use max with complex numbers
 *     Invalid operation
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2005-08-24 01:24:24 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "pslib_strict.h"
#include "psTest.h"

#define PRINT_VECTOR(VECTOR,TYPE) \
for(psS32 i=0; i<VECTOR->n; i++) { \
    if(PS_IS_PSELEMTYPE_COMPLEX(VECTOR->type.type)) { \
        printf("%f+%fi ", creal(VECTOR->data.TYPE[i]), cimag(VECTOR->data.TYPE[i])); \
    } else if(PS_IS_PSELEMTYPE_INT(VECTOR->type.type)) { \
        printf("%d ", (psS32)VECTOR->data.TYPE[i]); \
    } else { \
        printf("%f ", (double)VECTOR->data.TYPE[i]); \
    } \
} \
printf("\n\n");

#define PRINT_MATRIX(IMAGE,TYPE) \
for(psS32 i=IMAGE->numRows-1; i>-1; i--) { \
    for(psS32 j=0; j<IMAGE->numCols; j++) { \
        if(PS_IS_PSELEMTYPE_COMPLEX(IMAGE->type.type)) { \
            printf("%f+%fi ", creal(IMAGE->data.TYPE[i][j]), cimag(IMAGE->data.TYPE[i][j])); \
        } else if(PS_IS_PSELEMTYPE_INT(IMAGE->type.type)) { \
            printf("%d ", (psS32)IMAGE->data.TYPE[i][j]); \
        } else { \
            printf("%f ", (double)IMAGE->data.TYPE[i][j]); \
        } \
    } \
    printf("\n"); \
} \
printf("\n");


#define CREATE_AND_SET_VECTOR(NAME,TYPE,VALUE,SIZE) \
psVector *NAME = (psVector*)psVectorAlloc(SIZE, PS_TYPE_##TYPE); \
for(psS32 i=0; i<SIZE; i++) { \
    NAME->data.TYPE[i] = VALUE; \
} \
NAME->n = SIZE;


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS) \
psImage *NAME = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE); \
for(psS32 i=0; i<NAME->numRows; i++) { \
    for(psS32 j=0; j<NAME->numCols; j++) { \
        NAME->data.TYPE[i][j] = VALUE; \
    } \
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    CREATE_AND_SET_IMAGE(image1,F64,0,3,3);
    CREATE_AND_SET_IMAGE(image2,F64,0,3,3);
    CREATE_AND_SET_IMAGE(image3,F32,0,3,3);
    CREATE_AND_SET_IMAGE(image4,F64,0,2,2);
    CREATE_AND_SET_IMAGE(image5,C32,1+1i,3,3);
    CREATE_AND_SET_VECTOR(vector1,F64,0,2);
    CREATE_AND_SET_VECTOR(vector2,F64,0,3);

    // Check for NULL output argument
    printPositiveTestHeader(stdout,"psBinaryOp", "Check for output generated");
    psImage* image6 = (psImage*)psBinaryOp(NULL, image1, "+", image2);
    if (image6 == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp failed to make an image given no output to recycle.");
        return 1;
    }
    printFooter(stdout,"psBinaryOp","Check for output generated",true);

    // Check for NULL input argument #1
    printPositiveTestHeader(stdout,"psBinaryOp","Check for null input arg 1");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, NULL, "+", image2);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given a NULL first operand.");
        return 2;
    }
    printFooter(stdout,"psBinaryOp","Check for null input arg 1",true);

    // Check for NULL input argument #2
    printPositiveTestHeader(stdout,"psBinaryOp","Check for null input arg 2");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, image1, "+", NULL);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given a NULL second operand.");
        return 3;
    }
    printFooter(stdout,"psBinaryOp","Check for null input arg 2",true);

    // Check for NULL operand
    printPositiveTestHeader(stdout,"psBinaryOp","Check for null operand");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, image1, NULL, image2);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given a NULL operator.");
        return 4;
    }
    printFooter(stdout,"psBinaryOp","Check for null operand",true);

    // Check for null output
    printPositiveTestHeader(stdout,"psUnaryOp","Check for null output");
    image6 = (psImage*)psUnaryOp(NULL, image1, "sin");
    if (image6 == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp failed to make an image given no output to recycle.");
        return 5;
    }
    printFooter(stdout,"psUnaryOp","Check for null output",true);

    // Check for NULL input arg
    printPositiveTestHeader(stdout,"psUnaryOp","Check for null input");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psUnaryOp(image6, NULL, "sin");
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp returned a result given a NULL operand.");
        return 6;
    }
    printFooter(stdout,"psUnaryOp","Check for null input",true);

    // Check for NULL operand
    printPositiveTestHeader(stdout,"psUnaryOp","Check for null operator");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psUnaryOp(image6, image1, NULL);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp returned a result given a NULL operator.");
        return 7;
    }
    printFooter(stdout,"psUnaryOp","Check for null operator",true);

    // Inconsistent element types
    printPositiveTestHeader(stdout,"psBinaryOp", "Inconsistent element types");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, image3, "+", image2);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given operands of different types.");
        return 8;
    }
    printFooter(stdout,"psBinaryOp","Inconsistent element types",true);

    // Check unary op to convert to correct type
    printPositiveTestHeader(stdout,"psUnaryOp","Check output type conversion");
    image6 = psImageCopy(image6,image2,PS_TYPE_F64);
    image6 = (psImage*)psUnaryOp(image6, image3, "sin");
    if (image6 == NULL || image6->type.type != PS_TYPE_F32) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp failed to convert the type of the output.");
        return 9;
    }
    printFooter(stdout,"psUnaryOp","Check output type conversion",true);

    // Inconsistent element count
    printPositiveTestHeader(stdout,"psBinaryOp","Check for inconsistent elements");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, image4, "+", image2);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given operands of different sizes.");
        return 10;
    }
    printFooter(stdout,"psBinaryOp","Check for inconsistent elements",true);

    // Inconsistent element in input and output
    printPositiveTestHeader(stdout,"psUnaryOp","Check inconsistent elements in input and output");
    image6 = psImageCopy(image6,image2,PS_TYPE_F64);
    image6 = (psImage*)psUnaryOp(image6, image4, "sin");
    if (image6 == NULL ||
            image6->numCols != image6->numCols ||
            image6->numRows != image6->numRows) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp failed to resize the output.");
        return 11;
    }
    printFooter(stdout,"psUnaryOp","Check inconsistent elements in input and output",true);

    // Inconsistent size of input 1 and input 2
    printPositiveTestHeader(stdout,"psBinaryOp","Check inconsistent size");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, vector1, "+", image2);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given operands of different sizes.");
        return 12;
    }
    printFooter(stdout,"psBinaryOp","Check inconsistent size",true);

    // Inconsistent size of input 1 and input 2
    printPositiveTestHeader(stdout,"psBinaryOp","Check inconsistent size");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    vector1->type.dimen = PS_DIMEN_TRANSV;
    image6 = (psImage*)psBinaryOp(image6, vector1, "+", image2);
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned a result given operands of different sizes.");
        return 13;
    }
    printFooter(stdout, "psBinaryOp", "Check inconsistent size", true);


    // Inconsistent dimensionality
    printPositiveTestHeader(stdout,"psUnaryOp","Check inconsistent dimensionality");
    image6 = psImageCopy(image6,image2,PS_TYPE_F64);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an two error messages");
    image6 = (psImage*)psUnaryOp(image6, vector2, "sin");
    if (image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp returned result given wrong type out parameter.");
        return 14;
    }
    printFooter(stdout,"psUnaryOp","Check inconsistent dimensionality",true);

    // Attempt to use min with complex numbers
    printPositiveTestHeader(stdout,"psBinaryOp","Attempt to use min with complex numbers");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    image6 = (psImage*)psBinaryOp(image6, image5, "min", image5);
    if(image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp returned result with min of complex numbers");
        return 15;
    }
    printFooter(stdout, "psBinaryOp", "Attempt to use  min with complex numbers", true);


    // Attempt to use max with complex numbers
    printPositiveTestHeader(stdout,"psBinaryOp","Attempt to use max with complex numbers");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psBinaryOp(image6, image5, "max", image5);
    if(image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp returned result with max of complex numbers");
        return 16;
    }
    printFooter(stdout, "psBinaryOp", "Attempt to use max with complex numbers", true);


    // Invalid operation
    printPositiveTestHeader(stdout,"psBinary","Attempt to use invalid operator");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error messgae");
    image6 = (psImage*)psBinaryOp(image6, image1, "yarg", image2);
    if(image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psBinaryOp returned result with invalid operator");
        return 17;
    }
    printFooter(stdout,"psBinaryOp","Attempt to use invalid operator",true);

    printPositiveTestHeader(stdout,"psUnaryOp","Attempt to use invalid operator");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    image6 = (psImage*)psUnaryOp(image6, image1, "yarg");
    if(image6 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"psUnaryOp returned result with invalid operator");
        return 18;
    }
    printFooter(stdout, "psUnaryOp", "Attempt to use invalid operator", true);

    CREATE_AND_SET_VECTOR(vector4,F64,0,3);
    CREATE_AND_SET_VECTOR(vector5,F64,0,3);

    // Input parameter with dimension of PS_DIMEN_OTHER
    printPositiveTestHeader(stdout,"psBinaryOp","Attempt to use input with PS_DIMEN_OTHER");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    vector4->type.dimen = PS_DIMEN_OTHER;
    if ( psBinaryOp(NULL,vector4,"+",vector5) != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psBinaryOp should return null when input dimen PS_DIMEN_OTHER.");
        return 19;
    }
    vector4->type.dimen = PS_DIMEN_VECTOR;
    printFooter(stdout,"psBinaryOp","Attempt to use input with PS_DIMEN_OTHER",true);

    // Input parameter with dimension of PS_DIMEN_OTHER
    printPositiveTestHeader(stdout,"psUnaryOp","Attempt to use input with PS_DIMEN_OTHER");
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    vector4->type.dimen = PS_DIMEN_OTHER;
    if ( psUnaryOp(NULL,vector4,"sin") != NULL ) {
        psError(PS_ERR_UNKNOWN,true,"psUnaryOp should return null when input dimen PS_DIMEN_OTHER");
        return 20;
    }
    vector4->type.dimen = PS_DIMEN_VECTOR;
    printFooter(stdout,"psUnaryOp","Attempt to use input with PS_DIMEN_OTHER",true);

    psFree(vector4);
    psFree(vector5);
    psFree(image1);
    psFree(image2);
    psFree(image3);
    psFree(image4);
    psFree(image5);
    psFree(vector1);
    psFree(vector2);

    psS32 nLeaks = psMemCheckLeaks(0,NULL,stdout,false);
    if(nLeaks != 0) {
        psError(PS_ERR_UNKNOWN,true,"Memory leaks detected");
        return 50;
    }
    psS32 nBad = psMemCheckCorruption(0);
    if(nBad) {
        psError(PS_ERR_UNKNOWN,true,"Memory corruption detected");
        return 51;
    }

    return 0;
}


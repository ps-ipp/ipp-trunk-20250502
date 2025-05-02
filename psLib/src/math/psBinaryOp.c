/** @file  psBinaryOp.c
 *
 *  @brief Provides binary functions for simple matrix and vector element operations. Functions
 *  include:
 *
 *      Addition (+)
 *      Subtraction (-)
 *      Multiplication (*)
 *      Division (/)
 *      Power (^)
 *      Minimum (min)
 *      Maximum (max)
 *      Absolute value (abs)
 *      Exponent (exp)
 *      Natural Log (ln)
 *      Power of 10 (ten)
 *      Log (log)
 *      Sine (sin or dsin)
 *      Cosine (cos or dcos)
 *      Tangent (tan or dtan)
 *      Arcsine (asin or dasin)
 *      Arccosine (acos or dacos)
 *      Arctan (atan or datan)
 *
 *  Currently only vector-vector and image-image binary operations are supported.
 *
 *  @ingroup MatrixArithmetic
 *
 *  @author Ross Harman, MHPCC
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-03-19 00:52:35 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/******************************************************************************
 *  INCLUDE FILES                                                             *
 ******************************************************************************/
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdint.h>

#include "psMemory.h"
#include "psError.h"
#include "psImage.h"
#include "psVector.h"
#include "psScalar.h"
#include "psLogMsg.h"
#include "psAssert.h"


/*****************************************************************************
 *  FUNCTION IMPLEMENTATION - LOCAL                                          *
 *****************************************************************************/

// Binary SCALAR_XXXX operations
#define SCALAR_SCALAR(OUT,IN1,OP,IN2,TYPE) \
{ \
    ps##TYPE *o  = &((psScalar*)OUT)->data.TYPE; \
    ps##TYPE *i1 = &((psScalar*)IN1)->data.TYPE; \
    ps##TYPE *i2 = &((psScalar*)IN2)->data.TYPE; \
    *o = OP; \
}

#define SCALAR_VECTOR(OUT,IN1,OP,IN2,TYPE) \
{ \
    long npt = ((psVector*)IN2)->n; \
    ps##TYPE *o  = ((psVector*)OUT)->data.TYPE; \
    ps##TYPE *i1 = &((psScalar*)IN1)->data.TYPE; \
    ps##TYPE *i2 = ((psVector*)IN2)->data.TYPE; \
    for (long i = 0; i < npt; i++, o++, i2++) { \
        *o = OP; \
    } \
}

#define SCALAR_IMAGE(OUT,IN1,OP,IN2,TYPE) \
{ \
    long numRows = ((psImage*)IN2)->numRows; \
    long numCols = ((psImage*)IN2)->numCols; \
    for (long j = 0; j < numCols; j++) { \
        ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
        ps##TYPE *i1 = &((psScalar*)IN1)->data.TYPE; \
        ps##TYPE *i2 = ((psImage*)IN2)->data.TYPE[j]; \
        for (long i = 0; i < numRows; i++, o++, i2++) { \
            *o = OP; \
        } \
    } \
}

// Binary VECTOR_XXXX operations
#define VECTOR_SCALAR(OUT,IN1,OP,IN2,TYPE) \
{ \
    long n1  = ((psVector*)IN1)->n; \
    ps##TYPE *o  = ((psVector*)OUT)->data.TYPE; \
    ps##TYPE *i1 = ((psVector*)IN1)->data.TYPE; \
    ps##TYPE *i2 = &((psScalar*)IN2)->data.TYPE; \
    for (long i = 0; i < n1; i++, o++, i1++) { \
        *o = OP; \
    } \
}

#define VECTOR_VECTOR(OUT,IN1,OP,IN2,TYPE) \
{ \
    long n1 = ((psVector*)IN1)->n; \
    long n2 = ((psVector*)IN2)->n; \
    if (n1 != n2) { \
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Number of elements inconsistent, %ld vs %ld.  Number of elements must match."), n1, n2); \
        if (OUT != IN1 && OUT != IN2) { \
            psFree(OUT); \
        } \
        return NULL; \
    } \
    ps##TYPE *o  = ((psVector*)OUT)->data.TYPE; \
    ps##TYPE *i1 = ((psVector*)IN1)->data.TYPE; \
    ps##TYPE *i2 = ((psVector*)IN2)->data.TYPE; \
    for (long i = 0; i < n1; i++, o++, i1++, i2++) { \
        *o = OP; \
    } \
}

#define VECTOR_IMAGE(OUT,IN1,OP,IN2,TYPE) \
{ \
    long n1 = ((psVector*)IN1)->n; \
    long numRows2 = ((psImage*)IN2)->numRows; \
    long numCols2 = ((psImage*)IN2)->numCols; \
    \
    if (((psVector*)IN1)->type.dimen == PS_DIMEN_VECTOR) { /* Regular vectors */ \
        if (n1 != numRows2) { \
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Number of elements inconsistent, %ld vs %ld.  Number of elements must match."), n1, numRows2); \
            if (OUT != IN1 && OUT != IN2) { \
                psFree(OUT); \
            } \
            return NULL; \
        } \
        \
        ps##TYPE *i1 = ((psVector*)IN1)->data.TYPE; \
        for (long j = 0; j < numRows2; j++, i1++) { \
            ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
            ps##TYPE *i2 = ((psImage*)IN2)->data.TYPE[j]; \
            for (long i = 0; i < numCols2; i++, o++, i2++) { \
                *o = OP; \
            } \
        } \
    } else {  /* Transposed vectors */ \
        if (n1 != numCols2) { \
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Number of elements inconsistent, %ld vs %ld.  Number of elements must match."), n1, numCols2); \
            if (OUT != IN1 && OUT != IN2) { \
                psFree(OUT); \
            } \
            return NULL; \
        } \
        \
        for (long j = 0; j < numRows2; j++) { \
            ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
            ps##TYPE *i1 = ((psVector*)IN1)->data.TYPE; \
            ps##TYPE *i2 = ((psImage*)IN2)->data.TYPE[j]; \
            for (long i = 0; i < numCols2; i++, o++, i1++, i2++) { \
                *o = OP; \
            } \
        } \
    } \
}

// Binary IMAGE_XXXX operations
#define IMAGE_SCALAR(OUT,IN1,OP,IN2,TYPE) \
{ \
    long numRows = ((psImage*)IN1)->numRows; \
    long numCols = ((psImage*)IN1)->numCols; \
    for (long j = 0; j < numRows; j++) { \
        ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
        ps##TYPE *i1 = ((psImage*)IN1)->data.TYPE[j]; \
        ps##TYPE *i2 = &((psScalar*)IN2)->data.TYPE; \
        for (long i = 0; i < numCols; i++, o++, i1++) { \
            *o = OP; \
        } \
    } \
}

#define IMAGE_VECTOR(OUT,IN1,OP,IN2,TYPE) \
{ \
    long n2 = ((psVector*)IN2)->n; \
    long numRows1 = ((psImage*)IN1)->numRows; \
    long numCols1 = ((psImage*)IN1)->numCols; \
    \
    if (((psVector*)IN2)->type.dimen == PS_DIMEN_VECTOR) { /* Regular vectors */ \
        if (n2 != numRows1) { \
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Number of elements inconsistent, %ld vs %ld.  Number of elements must match."), n2, numRows1); \
            if (OUT != IN1 && OUT != IN2) { \
                psFree(OUT); \
            } \
            return NULL; \
        } \
        \
        ps##TYPE *i2 = ((psVector* )IN2)->data.TYPE; \
        for (long j = 0; j < numRows1; j++, i2++) { \
            ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
            ps##TYPE *i1 = ((psImage*)IN1)->data.TYPE[j]; \
            for (long i = 0; i < numCols1; i++, o++, i1++) { \
                *o = OP; \
            } \
        } \
    } else {  /* Transposed vectors */ \
        if (n2 != numCols1) { \
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Number of elements inconsistent, %ld vs %ld.  Number of elements must match."), n2, numCols1); \
            if (OUT != IN1) { \
                psFree(OUT); \
            } \
            return NULL; \
        } \
        \
        for (long j = 0; j < numRows1; j++) { \
            ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
            ps##TYPE *i1 = ((psVector*)IN2)->data.TYPE; \
            ps##TYPE *i2 = ((psImage*)IN1)->data.TYPE[j]; \
            for (long i = 0; i < numCols1; i++, o++, i2++, i1++) { \
                *o = OP; \
            } \
        } \
    } \
}

#define IMAGE_IMAGE(OUT,IN1,OP,IN2,TYPE) \
{ \
    long numRows1 = ((psImage*)IN1)->numRows; \
    long numCols1 = ((psImage*)IN1)->numCols; \
    long numRows2 = ((psImage*)IN2)->numRows; \
    long numCols2 = ((psImage*)IN2)->numCols; \
    if (numRows1 != numRows2 || numCols1 != numCols2) { \
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Specified psImage dimensions differed, %ldx%ld vs %ldx%ld."), \
                numCols1, numRows1, numCols2, numRows2); \
        if (OUT != IN1 && OUT != IN2) { \
            psFree(OUT); \
        } \
        return NULL; \
    } \
    for (long j = 0; j < numRows1; j++) { \
        ps##TYPE *o  = ((psImage*)OUT)->data.TYPE[j]; \
        ps##TYPE *i1 = ((psImage*)IN1)->data.TYPE[j]; \
        ps##TYPE *i2 = ((psImage*)IN2)->data.TYPE[j]; \
        for (long i = 0; i < numCols1; i++, o++, i1++, i2++) { \
            *o = OP; \
        } \
    } \
}


// Preprocessor macro function to create arithmetic function based on input type --- for integers only
#define BINARY_TYPE_INTEGER(DIM1,DIM2,OUT,IN1,OP,IN2)                                                        \
switch (IN1->type) {                                                                                         \
case PS_TYPE_U8:                                                                                             \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U8);                                                                        \
    break;                                                                                                   \
case PS_TYPE_U16:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U16);                                                                       \
    break;                                                                                                   \
case PS_TYPE_U32:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U32);                                                                       \
    break;                                                                                                   \
case PS_TYPE_U64:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U64);                                                                       \
    break;                                                                                                   \
case PS_TYPE_S8:                                                                                             \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S8);                                                                        \
    break;                                                                                                   \
case PS_TYPE_S16:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S16);                                                                       \
    break;                                                                                                   \
case PS_TYPE_S32:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S32);                                                                       \
    break;                                                                                                   \
case PS_TYPE_S64:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S64);                                                                       \
    break;                                                                                                   \
default:                                                                                                     \
    /* char* strType;                                                                                        \
    PS_TYPE_NAME(strType,IN1->type);                                                                         \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true,                                                                 \
            _("Specified data type, %s, is not supported."),                                                             \
            strType);  */                                                                                    \
    if (OUT != IN1 && OUT != IN2) {                                                                          \
        psFree(OUT);                                                                                         \
    }                                                                                                        \
    return NULL;                                                                                             \
}

// Preprocessor macro function to create arithmetic function based on input type
#define BINARY_TYPE(DIM1,DIM2,OUT,IN1,OP,IN2)                                                                \
switch (IN1->type) {                                                                                         \
case PS_TYPE_U8:                                                                                             \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U8);                                                                        \
    break;                                                                                                   \
case PS_TYPE_U16:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U16);                                                                       \
    break;                                                                                                   \
case PS_TYPE_U32:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U32);                                                                       \
    break;                                                                                                   \
case PS_TYPE_U64:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,U64);                                                                       \
    break;                                                                                                   \
case PS_TYPE_S8:                                                                                             \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S8);                                                                        \
    break;                                                                                                   \
case PS_TYPE_S16:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S16);                                                                       \
    break;                                                                                                   \
case PS_TYPE_S32:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S32);                                                                       \
    break;                                                                                                   \
case PS_TYPE_S64:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,S64);                                                                       \
    break;                                                                                                   \
case PS_TYPE_F32:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,F32);                                                                       \
    break;                                                                                                   \
case PS_TYPE_F64:                                                                                            \
    DIM1##_##DIM2(OUT,IN1,OP,IN2,F64);                                                                       \
    break;                                                                                                   \
default: {                                                                                                   \
        char* strType;                                                                                       \
        PS_TYPE_NAME(strType,IN1->type);                                                                     \
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,                                                             \
                _("Specified data type, %s, is not supported."),                                             \
                strType);                                                                                    \
        if (OUT != IN1 && OUT != IN2) {                                                                      \
            psFree(OUT);                                                                                     \
        }                                                                                                    \
        return NULL;                                                                                         \
    }                                                                                                        \
}

// Preprocessor macro function to create arithmetic function operation name
#define BINARY_OP(DIM1,DIM2,OUT,IN1,OP,IN2)                                                                  \
if (!strncmp(OP, "+", 1)) {                                                                                  \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,*i1 + *i2,IN2);                                                            \
} else if (!strncmp(OP, "-", 1)) {                                                                           \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,*i1 - *i2,IN2);                                                            \
} else if (!strncmp(OP, "*", 1)) {                                                                           \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,*i1 * *i2,IN2);                                                            \
} else if (!strncmp(OP, "/", 1)) {                                                                           \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,*i1 / *i2,IN2);                                                            \
} else if (!strncmp(OP, "&", 1)) {                                                                           \
    if (PS_IS_PSELEMTYPE_INT(IN1->type) && PS_IS_PSELEMTYPE_INT(IN2->type)) {                                \
        BINARY_TYPE_INTEGER(DIM1,DIM2,OUT,IN1,(*i1) & (*i2),IN2);                                            \
    } else {                                                                                                 \
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,                                                            \
                "Types (%x,%x) are not appropriate for logical AND.\n", IN1->type, IN2->type);               \
        return NULL;                                                                                         \
    }                                                                                                        \
} else if (!strncmp(OP, "|", 1)) {                                                                           \
    if (PS_IS_PSELEMTYPE_INT(IN1->type) && PS_IS_PSELEMTYPE_INT(IN2->type)) {                                \
        BINARY_TYPE_INTEGER(DIM1,DIM2,OUT,IN1,(*i1) | (*i2),IN2);                                            \
    } else {                                                                                                 \
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,                                                            \
                "Types (%x,%x) are not appropriate for logical OR.\n", IN1->type, IN2->type);                \
        return NULL;                                                                                         \
    }                                                                                                        \
} else if (!strncmp(OP, "^", 1)) {                                                                           \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,pow(*i1,*i2),IN2);                                                         \
} else if (!strncasecmp(OP, "min", 3)) {                                                                     \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,fmin(*i1,*i2),IN2);                                                        \
} else if (!strncasecmp(OP, "max", 3)) {                                                                     \
    BINARY_TYPE(DIM1,DIM2,OUT,IN1,fmax(*i1,*i2),IN2);                                                        \
} else {                                                                                                     \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true,                                                                \
            _("Specified operation, %s, is not supported."),                                                 \
            OP);                                                                                             \
    if (OUT != IN1 && OUT != IN2) {                                                                          \
        psFree(OUT);                                                                                         \
    }                                                                                                        \
    return NULL;                                                                                             \
}

psMathType* psBinaryOp(psPtr out, psPtr in1, const char *op, psPtr in2)
{

    psVector* input1 = (psVector* ) in1;
    psVector* input2 = (psVector* ) in2;

    #define psBinaryOp_EXIT { \
                              if (out != in1 && out != in2) { \
                              psFree(out); \
                              } \
                              return NULL; \
                            }

    PS_ASSERT_GENERAL_PTR_NON_NULL(input1, psBinaryOp_EXIT);
    PS_ASSERT_GENERAL_PTR_NON_NULL(input2, psBinaryOp_EXIT);
    PS_ASSERT_GENERAL_PTR_NON_NULL(op, psBinaryOp_EXIT);

    PS_ASSERT_GENERAL_PTR_TYPE_EQUAL(input1,input2, psBinaryOp_EXIT);

    PS_ASSERT_PTR_DIMEN_GENERAL_NOT(input1, PS_DIMEN_OTHER, psBinaryOp_EXIT);
    PS_ASSERT_PTR_DIMEN_GENERAL_NOT(input2, PS_DIMEN_OTHER, psBinaryOp_EXIT);

    psMathType* psType1 = (psMathType*)in1;
    psMathType* psType2 = (psMathType*)in2;
    psDimen dim1 = psType1->dimen;
    psDimen dim2 = psType2->dimen;
    psElemType elType1 = psType1->type;
    psElemType elType2 = psType2->type;

    if (dim1 == PS_DIMEN_VECTOR || dim1 == PS_DIMEN_TRANSV) {
        if (((psVector* ) in1)->n == 0) {
            psLogMsg(__func__, PS_LOG_WARN, "Vector contains zero elements");
        }
    } else if (dim1 == PS_DIMEN_IMAGE) {
        if (((psImage* ) in1)->numCols == 0 || ((psImage* ) in1)->numRows == 0) {
            psLogMsg(__func__, PS_LOG_WARN, "Image contains zero length row or cols");
        }
    }

    if (dim2 == PS_DIMEN_VECTOR || dim2 == PS_DIMEN_TRANSV) {
        if (((psVector* ) in2)->n == 0) {
            psLogMsg(__func__, PS_LOG_WARN, "Vector contains zero elements");
        }
    } else if (dim2 == PS_DIMEN_IMAGE) {
        if (((psImage* ) in2)->numCols == 0 || ((psImage* ) in2)->numRows == 0) {
            psLogMsg(__func__, PS_LOG_WARN, "Image contains zero length row or cols");
        }
    }

    if (dim1 == PS_DIMEN_SCALAR) {
        if ( out != NULL && ((psMathType*)out)->dimen != dim2) {
            if (out != in1 && out != in2) {
                psFree(out);
            }
            out = NULL;
        }
        if (dim2 == PS_DIMEN_SCALAR) {
            if (out == NULL || ((psScalar*)out)->type.type != elType1) {
                if (out != in1 && out != in2) {
                    psFree(out);
                }
                out = psScalarAlloc(0.0,elType1);
            }
            BINARY_OP(SCALAR, SCALAR, out, psType1, op, psType2);       // scalar op scalar
        } else if (dim2 == PS_DIMEN_VECTOR || dim2 == PS_DIMEN_TRANSV) {
            out = psVectorRecycle(out,((psVector*)in2)->n,elType1);
            if (out == NULL) {
                psError(PS_ERR_UNKNOWN, false,
                        _("Couldn't create a proper output psVector."));
                return NULL;
            }
            ((psVector*)out)->n = ((psVector*)in2)->n;
            BINARY_OP(SCALAR, VECTOR, out, psType1, op, psType2);       // scalar op vector
        } else if (dim2 == PS_DIMEN_IMAGE) {
            out = psImageRecycle(out, ((psImage* ) in2)->numCols, ((psImage* ) in2)->numRows,elType1);
            if (out == NULL) {
                psError(PS_ERR_UNKNOWN, false,
                        _("Couldn't create a proper output psImage."));
                return NULL;
            }
            BINARY_OP(SCALAR, IMAGE, out, psType1, op, psType2);        // scalar op image
        } else {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified parameter, %s, has invalid dimensionality, %d."),
                    "in2",dim2);
            psBinaryOp_EXIT;
        }
    } else if (dim1 == PS_DIMEN_VECTOR || dim1 == PS_DIMEN_TRANSV) {
        if (dim2 == PS_DIMEN_SCALAR) {
            out = psVectorRecycle(out,((psVector*)in1)->n,elType1);
            if (out == NULL) {
                psError(PS_ERR_UNKNOWN, false,
                        _("Couldn't create a proper output psVector."));
                return NULL;
            }
            ((psVector*)out)->n = ((psVector*)in1)->n;
            BINARY_OP(VECTOR, SCALAR, out, psType1, op, psType2);       // vector op scalar
        } else if (dim2 == PS_DIMEN_VECTOR || dim2 == PS_DIMEN_TRANSV) {
            out = psVectorRecycle(out,((psVector*)in2)->n,elType2);
            if (out == NULL) {
                psError(PS_ERR_UNKNOWN, false,
                        _("Couldn't create a proper output psVector."));
                return NULL;
            }
            ((psVector*)out)->n = ((psVector*)in2)->n;
            BINARY_OP(VECTOR, VECTOR, out, psType1, op, psType2);       // vector op vector
        } else if (dim2 == PS_DIMEN_IMAGE) {
            out = psImageRecycle(out, ((psImage* ) in2)->numCols, ((psImage* ) in2)->numRows, elType2);
            if (out == NULL) {
                psError(PS_ERR_UNKNOWN, false,
                        _("Couldn't create a proper output psImage."));
                return NULL;
            }
            BINARY_OP(VECTOR, IMAGE, out, psType1, op, psType2);        // vector op image
        } else {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified parameter, %s, has invalid dimensionality, %d."),
                    "in2",dim2);
            psBinaryOp_EXIT;
        }
    } else if (dim1 == PS_DIMEN_IMAGE) {
        out = psImageRecycle(out, ((psImage*)in1)->numCols, ((psImage*)in1)->numRows, elType1);
        if (out == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    _("Couldn't create a proper output psImage."));
            return NULL;
        }
        if (dim2 == PS_DIMEN_SCALAR) {
            BINARY_OP(IMAGE, SCALAR, out, psType1, op, psType2);        // image op scalar
        } else if (dim2 == PS_DIMEN_VECTOR || dim2 == PS_DIMEN_TRANSV) {
            BINARY_OP(IMAGE, VECTOR, out, psType1, op, psType2);        // image op vector
        } else if (dim2 == PS_DIMEN_IMAGE) {
            BINARY_OP(IMAGE, IMAGE, out, psType1, op, psType2); // image op image
        } else {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified parameter, %s, has invalid dimensionality, %d."),
                    "in2",dim2);
            psBinaryOp_EXIT;
        }
    } else {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified parameter, %s, has invalid dimensionality, %d."),
                "in1",dim1);
        psBinaryOp_EXIT;
    }

    // Automtically free psScalar types, since they are usually allocated in the argument list when this
    // function is called, provided that the input is not the output.
    if (psType1->dimen==PS_DIMEN_SCALAR && in1!=out) {
        psFree(in1);
    }

    if (psType2->dimen==PS_DIMEN_SCALAR && in2!=out) {
        psFree(in2);
    }

    return out;
}

/** @file  psUnary.c
 *
 *  @brief Provides unary functions for simple matrix and vector element operations. Functions
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
 *      Square root (sqrt)
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

// Conversion for degrees to radians
// #define D2R 0.01745329252111111  /* PI/180 */
#define D2R    0.01745329251994329  /* PI/180 Corrected. Truncated digits: 576924 */
// Conversion for radians to degrees
// #define R2D 57.29577950924861   /* 180.0/PI */
#define R2D    57.29577951308232   /* 180.0/PI Correcte.  Truncated digits: 087679 */

// Unary SCALAR operations
#define SCALAR(OUT,IN,OP,TYPE) \
{ \
    ps##TYPE *o  = &((psScalar*)OUT)->data.TYPE; \
    ps##TYPE *i1 = &((psScalar*)IN)->data.TYPE; \
    *o = OP; \
}

// Unary IMAGE operations
#define VECTOR(OUT,IN,OP,TYPE) \
{ \
    long nIn = ((psVector*)IN)->n; \
    long nOut = ((psVector*)OUT)->n; \
    if (nIn != nOut) { \
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Number of elements inconsistent, %ld vs %ld.  Number of elements must match."), nIn, nOut); \
        if (OUT != IN) { \
            psFree(OUT); \
        } \
        return NULL; \
    } \
    ps##TYPE *o  = ((psVector*)OUT)->data.TYPE; \
    ps##TYPE *i1 = ((psVector*)IN)->data.TYPE; \
    for (long i = 0; i < nIn; i++, o++, i1++) { \
        *o = OP; \
    } \
}

// Unary IMAGE operations
#define IMAGE(OUT,IN,OP,TYPE) \
{ \
    long numRowsIn = ((psImage*)IN)->numRows; \
    long numColsIn = ((psImage*)IN)->numCols; \
    long numRowsOut = ((psImage*)OUT)->numRows; \
    long numColsOut = ((psImage*)OUT)->numCols; \
    if(numRowsIn!=numRowsOut || numColsIn!=numColsOut) { \
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Specified psImage dimensions differed, %ldx%ld vs %ldx%ld."), \
                numColsIn, numRowsIn, numColsOut, numRowsOut); \
        if (OUT != IN) { \
            psFree(OUT); \
        } \
        return NULL; \
    } \
    for (long j = 0; j < numRowsIn; j++) { \
        ps##TYPE *o  = ((psImage* )OUT)->data.TYPE[j]; \
        ps##TYPE *i1 = ((psImage* )IN)->data.TYPE[j]; \
        for (long i = 0; i < numColsIn; i++, o++, i1++) { \
            *o = OP; \
        } \
    }\
}

// Preprocessor macro function to create arithmetic function based on input type
#define UNARY_TYPE(DIM,OUT,IN,OP) \
switch (IN->type) { \
case PS_TYPE_U8: \
    DIM(OUT,IN,OP,U8); \
    break; \
case PS_TYPE_U16: \
    DIM(OUT,IN,OP,U16); \
    break; \
case PS_TYPE_U32: \
    DIM(OUT,IN,OP,U32); \
    break; \
case PS_TYPE_U64: \
    DIM(OUT,IN,OP,U64); \
    break; \
case PS_TYPE_S8: \
    DIM(OUT,IN,OP,S8); \
    break; \
case PS_TYPE_S16: \
    DIM(OUT,IN,OP,S16); \
    break; \
case PS_TYPE_S32: \
    DIM(OUT,IN,OP,S32); \
    break; \
case PS_TYPE_S64: \
    DIM(OUT,IN,OP,S64); \
    break; \
case PS_TYPE_F32: \
    DIM(OUT,IN,OP,F32); \
    break; \
case PS_TYPE_F64: \
    DIM(OUT,IN,OP,F64); \
    break; \
default: { \
        char* strType; \
        PS_TYPE_NAME(strType, IN->type); \
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, _("Specified data type, %s, is not supported."), strType); \
        if (OUT != IN) { \
            psFree(OUT); \
        } \
        return NULL; \
    } \
}

// Preprocessor macro function to create arithmetic function operation name. Functions below that add
// FLT_EPSILON are done so to align results with a 64 bit computing architecture
#define UNARY_OP(DIM,OUT,IN,OP) \
if(!strncasecmp(OP, "abs", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,fabs((double)*i1)); \
} else if(!strncasecmp(OP, "sqrt", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,sqrt((double)*i1)); \
} else if(!strncasecmp(OP, "exp", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,exp((double)*i1)); \
} else if(!strncasecmp(OP, "ln", 2)) { \
    UNARY_TYPE(DIM,OUT,IN,log((double)*i1)); \
} else if(!strncasecmp(OP, "ten", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,pow(10.0,(double)*i1)); \
} else if(!strncasecmp(OP, "log", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,log10((double)*i1)); \
} else if(!strncasecmp(OP, "sin", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,sin((double)*i1)); \
} else if(!strncasecmp(OP, "dsin", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,sin((double)*i1*D2R)); \
} else if(!strncasecmp(OP, "cos", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,cos((double)*i1)); \
} else if(!strncasecmp(OP, "dcos", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,cos((double)*i1*D2R)); \
} else if(!strncasecmp(OP, "tan", 3)) { \
    UNARY_TYPE(DIM,OUT,IN,tan((double)*i1)); \
} else if(!strncasecmp(OP, "dtan", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,tan((double)*i1*D2R)); \
} else if(!strncasecmp(OP, "asin", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,asin((double)*i1)); \
} else if(!strncasecmp(OP, "dasin", 5)) { \
    UNARY_TYPE(DIM,OUT,IN,(R2D*asin((double)*i1))); \
} else if(!strncasecmp(OP, "acos", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,acos((double)*i1)); \
} else if(!strncasecmp(OP, "dacos", 5)) { \
    UNARY_TYPE(DIM,OUT,IN,R2D*acos((double)*i1)); \
} else if(!strncasecmp(OP, "atan", 4)) { \
    UNARY_TYPE(DIM,OUT,IN,atan((double)*i1)); \
} else if(!strncasecmp(OP, "datan", 5)) { \
    UNARY_TYPE(DIM,OUT,IN,R2D*atan((double)*i1)); \
} else { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified operation, %s, is not supported."), OP); \
    if (OUT != IN) { \
        psFree(OUT); \
    } \
    return NULL; \
}

psMathType* psUnaryOp(psPtr out, psPtr in, const char *op)
{
    #define psUnaryOp_EXIT { \
                             if (out != in) { \
                             psFree(out); \
                             } \
                             return NULL; \
                           }

    psMathType* psTypeIn = (psMathType* ) in;

    PS_ASSERT_GENERAL_PTR_NON_NULL(in, psUnaryOp_EXIT);
    PS_ASSERT_GENERAL_PTR_NON_NULL(op, psUnaryOp_EXIT);

    switch (psTypeIn->dimen) {
    case PS_DIMEN_SCALAR:
        if (out == NULL ||
                ((psMathType*)out)->dimen != PS_DIMEN_SCALAR ||
                ((psScalar*)out)->type.type != psTypeIn->type) {
            psFree(out);
            out = psScalarAlloc(0.0, psTypeIn->type);
        }
        UNARY_OP(SCALAR, out, psTypeIn, op);    // scalar
        break;
    case PS_DIMEN_VECTOR:
    case PS_DIMEN_TRANSV:
        if (((psVector*)in)->n == 0) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Input psVector contains no elements.  No data to perform operation with."));
            psUnaryOp_EXIT;
        }

        out = psVectorRecycle(out, ((psVector*)in)->n, psTypeIn->type);
        if (out == NULL) {
            psError(PS_ERR_UNKNOWN, false, _("Couldn't create a proper output psVector."));
            psUnaryOp_EXIT;
        }
        ((psVector*)out)->n = ((psVector*)in)->n;

        UNARY_OP(VECTOR, out, psTypeIn, op);    // vector
        break;
    case PS_DIMEN_IMAGE:
        if (((psImage* ) in)->numCols == 0 || ((psImage* ) in)->numRows == 0) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Input psImage contains no pixels.  No data to perform operation with."));
            psUnaryOp_EXIT;
        }

        out = psImageRecycle(out, ((psImage*)in)->numCols, ((psImage*)in)->numRows, psTypeIn->type);
        if (out == NULL) {
            psError(PS_ERR_UNKNOWN, false, _("Couldn't create a proper output psImage."));
            psUnaryOp_EXIT;
        }

        UNARY_OP(IMAGE, out, psTypeIn, op);     // image
        break;
    default:
        if (out != in) {
            psFree(out);
        }
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("Specified parameter, %s, has invalid dimensionality, %d."), "in", psTypeIn->dimen);
        psUnaryOp_EXIT;
    }

    // Automtically free psScalar types, since they are usually allocated in the argument list when this
    // function is called, provided that the input is not the output.
    if (psTypeIn->dimen == PS_DIMEN_SCALAR && in!=out) {
        psFree(in);
    }

    return out;
}

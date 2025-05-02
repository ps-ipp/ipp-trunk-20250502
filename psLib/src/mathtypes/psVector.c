/** @file  psVector.c
*
*  @brief Contains support for basic vector types
*
*  This file defines the basic type for a vector struct and functions useful
*  in manupulating vectors.
*
*  @author Ross Harman, MHPCC
*  @author Robert DeSonia, MHPCC
*  @author Joshua Hoblitt, University of Hawaii
*
*  @version $Revision: 1.105 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-01-27 06:39:38 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>                        // for memcpy
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "psMemory.h"
#include "psAbort.h"
#include "psError.h"
#include "psVector.h"
#include "psLogMsg.h"
#include "psCompare.h"
#include "psAssert.h"
#include "psString.h"
#include "psSort.h"

static void vectorFree(psVector* psVec)
{
    if (psVec == NULL) {
        return;
    }

    psFree(psVec->data.U8);
}

// FUNCTION IMPLEMENTATION - PUBLIC
bool psMemCheckVector(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)vectorFree );
}


// Allocate a psVector; does not set vector->n (left to the caller)
static psVector *vectorAlloc(const char *file,
                             unsigned int lineno,
                             const char *func,
                             long nalloc, // Number of elements to allocate
                             psElemType type) // Type of elements
{
    int elementSize = PSELEMTYPE_SIZEOF(type); // Size, in bytes, of element
    if (elementSize < 1) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input psVector is an unsupported type (0x%x)."), type);
        return NULL;
    }

    // Create vector struct
    psVector *vector = (psVector*) p_psAlloc(file, lineno, func, sizeof(psVector));
    psMemSetDeallocator(vector, (psFreeFunc) vectorFree);

    vector->type.dimen = PS_DIMEN_VECTOR;
    vector->type.type = type;
    P_PSVECTOR_SET_NALLOC(vector, nalloc);

    // Create vector data array
    vector->data.U8 = psAlloc(nalloc * elementSize);
    memset (vector->data.U8, 0, nalloc * elementSize);

    return vector;
}

psVector* p_psVectorAlloc(const char *file,
                          unsigned int lineno,
                          const char *func,
                          long nalloc,
                          psElemType type)
{
    psVector *vector = vectorAlloc(file, lineno, func, nalloc, type);
    if (!vector) {
        return NULL;
    }
    vector->n = nalloc;
    return vector;
}

psVector* p_psVectorAllocEmpty(const char *file,
                               unsigned int lineno,
                               const char *func,
                               long nalloc,
                               psElemType type)
{
    psVector *vector = vectorAlloc(file, lineno, func, nalloc, type);
    if (!vector) {
        return NULL;
    }
    vector->n = 0;
    return vector;
}

psVector* psVectorRealloc(psVector* vector,
                          long nalloc)
{
    psS32 elementSize = 0;
    psElemType elemType;

    if (vector == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("psVectorRealloc must a given a non-NULL psVector to resize.  Desired datatype unknown."));
        return NULL;
    } 

    if (vector->nalloc == nalloc) {     
	// No need to realloc to same size
	return vector;
    }
     
    elemType = vector->type.type;
    elementSize = PSELEMTYPE_SIZEOF(elemType);

    long nallocOld = vector->nalloc;
    if (nalloc < vector->n) {
	vector->n = nalloc;
    }
    // Realloc after decrementation to avoid accessing freed array elements
    vector->data.U8 = psRealloc(vector->data.U8, nalloc * elementSize);
    P_PSVECTOR_SET_NALLOC(vector,nalloc);

    // fill newly allocated range with zeros: 
    if (nallocOld < nalloc) {
	long nNew = nalloc - nallocOld;
	memset (&vector->data.U8[nallocOld*elementSize], 0, nNew*elementSize);
    }

    return vector;
}

psVector* p_psVectorRecycle(const char *file,
                            unsigned int lineno,
                            const char *func,
                            psVector* vector,
                            long nalloc,
                            psElemType type)
{
    psS32 byteSize;

    if (vector == NULL) {
        return p_psVectorAlloc(file, lineno, func, nalloc, type);
    }

    if (vector->type.dimen !=  PS_DIMEN_VECTOR &&
            vector->type.dimen !=  PS_DIMEN_TRANSV) {
        psFree(vector);
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("The input psVector must have a vector dimension type."));
        return NULL;
    }

    byteSize = nalloc * PSELEMTYPE_SIZEOF(type);

    // need to increase data buffer?
    if (byteSize > vector->nalloc*PSELEMTYPE_SIZEOF(vector->type.type)) {
        vector->data.U8 = psRealloc(vector->data.U8, byteSize);
        P_PSVECTOR_SET_NALLOC(vector,nalloc);
    }

    vector->type.dimen = PS_DIMEN_VECTOR;
    vector->type.type = type;
    vector->n = nalloc;

    // zero the vector
    // memset (vector->data.U8, 0, byteSize);
    // XXX for reasons I don't understand, this breaks ppStack...

    return vector;
}

psVector *psVectorExtend(psVector *vector,
                         long delta,
                         long nExtend)
{
    // can't handle a NULL vector (don't know the data type to allocate)
    if (vector == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("The input psVector can not be NULL."));
        return NULL;
    }

    // requirement on delta; if < 1, set to 10.
    if (delta < 1) {
        delta = 10;
    }

    // adjust the allocated size, if needed. (if nExtend < 1, this is will never happen)
    if (nExtend > 0) {
        unsigned int minAlloc = vector->n + nExtend + nExtend;
        if (vector->nalloc < minAlloc) {
            unsigned int nAlloc = delta + vector->nalloc;
            // make sure the delta is large enough hold twice the extended length.
            if (nAlloc < minAlloc) {
                nAlloc = minAlloc;
            }
            vector = psVectorRealloc(vector, nAlloc);
        }
    } else if (nExtend < -vector->n) {
        // For the case of a negative nExtend, need to check that we are not decreasing
        // vector beyond its own length (i.e., creating a negative length).
        nExtend = -vector->n;
    }

    // increment the length by the value specified
    vector->n += nExtend;

    return vector;
}

bool psVectorAppend(psVector *vector,...) {

    va_list argPtr;
    
    int N = vector->n;
    if (vector->nalloc == vector->n) {
	psVectorRealloc (vector, vector->nalloc + 100);
    }

    // Get the variable list parameters to pass to allocation function
    va_start(argPtr, vector);

    switch(vector->type.type) {
    case PS_DATA_S8:
        vector->data.S8[N] = (psS8)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S16:
        vector->data.S16[N] = (psS16)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S32:
        vector->data.S32[N] = (psS32)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S64:
        vector->data.S64[N] = (psS64)va_arg(argPtr, psS64);
        break;
    case PS_DATA_U8:
        vector->data.U8[N] = (psU8)va_arg(argPtr, psU32);
        break;
    case PS_DATA_U16:
        vector->data.U16[N] = (psU16)va_arg(argPtr, psU32);
        break;
    case PS_DATA_U32:
        vector->data.U32[N] = (psU32)va_arg(argPtr, psU32);
        break;
    case PS_DATA_U64:
        vector->data.U64[N] = (psU64)va_arg(argPtr, psU64);
        break;
    case PS_DATA_F32:
        vector->data.F32[N] = (psF32)va_arg(argPtr, psF64);
        break;
    case PS_DATA_F64:
        vector->data.F64[N] = (psF64)va_arg(argPtr, psF64);
        break;
      default:
	psAbort ("invalid data type for vector");
    }

    vector->n ++;
    va_end(argPtr);

    return true;
}

psVector* p_psVectorCopy(const char *file,
                       unsigned int lineno,
                       const char *func,
                       psVector* output,
                       const psVector* input,
                       psElemType type)
{
    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("The input psVector can not be NULL."));
        psFree(output);
        return NULL;
    }

    psS32 nElements = input->n;

    output = p_psVectorRecycle(file, lineno, func, output, nElements, type);
    if (nElements == 0) {
        //        psWarning("Warning: psVector was copied with 0 elements!\n");
        return output;
    }
    output->n = nElements;

    if (input->type.type == type) {
        // Can simply copy the bytes if the types are the same

        #define PSVECTOR_COPY_SAME_CASE(NAME) \
    case PS_TYPE_##NAME: \
        output->data.NAME = memcpy(output->data.NAME, input->data.NAME, \
                                   input->n * PSELEMTYPE_SIZEOF(PS_TYPE_##NAME)); \
        break;

        switch (type) {
            PSVECTOR_COPY_SAME_CASE(U8);
            PSVECTOR_COPY_SAME_CASE(U16);
            PSVECTOR_COPY_SAME_CASE(U32);
            PSVECTOR_COPY_SAME_CASE(U64);
            PSVECTOR_COPY_SAME_CASE(S8);
            PSVECTOR_COPY_SAME_CASE(S16);
            PSVECTOR_COPY_SAME_CASE(S32);
            PSVECTOR_COPY_SAME_CASE(S64);
            PSVECTOR_COPY_SAME_CASE(F32);
            PSVECTOR_COPY_SAME_CASE(F64);
	  default: {
	      psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Input psVector is an unsupported type.\n");
	      psFree(output);
	      return NULL;
	  }
        }
        return output;
    }

    #define PSVECTOR_COPY(INTYPE,OUTTYPE) { \
        ps##INTYPE *inVec = input->data.INTYPE; \
        ps##OUTTYPE *outVec = output->data.OUTTYPE; \
        for (psS32 col=0;col<nElements;col++) { \
            *(outVec++) = *(inVec++); \
        } \
    }

    #define PSVECTOR_COPY_CASE(OUTTYPE) \
case PS_TYPE_##OUTTYPE: { \
        switch (input->type.type) { \
        case PS_TYPE_S8: \
            PSVECTOR_COPY(S8,OUTTYPE); \
            break; \
        case PS_TYPE_S16: \
            PSVECTOR_COPY(S16,OUTTYPE); \
            break; \
        case PS_TYPE_S32: \
            PSVECTOR_COPY(S32,OUTTYPE); \
            break; \
        case PS_TYPE_S64: \
            PSVECTOR_COPY(S64,OUTTYPE); \
            break; \
        case PS_TYPE_U8: \
            PSVECTOR_COPY(U8,OUTTYPE); \
            break; \
        case PS_TYPE_U16: \
            PSVECTOR_COPY(U16,OUTTYPE); \
            break; \
        case PS_TYPE_U32: \
            PSVECTOR_COPY(U32,OUTTYPE); \
            break; \
        case PS_TYPE_U64: \
            PSVECTOR_COPY(U64,OUTTYPE); \
            break; \
        case PS_TYPE_F32: \
            PSVECTOR_COPY(F32,OUTTYPE); \
            break; \
        case PS_TYPE_F64: \
            PSVECTOR_COPY(F64,OUTTYPE); \
            break; \
        default: { \
                char* typeStr; \
                PS_TYPE_NAME(typeStr,type); \
                psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
                        _("Input psVector is an unsupported type (0x%s)."), \
                        typeStr); \
                psFree(output); \
            } \
        } \
        break; \
    }

    switch (type) {
        PSVECTOR_COPY_CASE(S8);
        PSVECTOR_COPY_CASE(S16);
        PSVECTOR_COPY_CASE(S32);
        PSVECTOR_COPY_CASE(S64);
        PSVECTOR_COPY_CASE(U8);
        PSVECTOR_COPY_CASE(U16);
        PSVECTOR_COPY_CASE(U32);
        PSVECTOR_COPY_CASE(U64);
        PSVECTOR_COPY_CASE(F32);
        PSVECTOR_COPY_CASE(F64);
    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Input psVector is an unsupported type (0x%s)."),
                    typeStr);
            psFree(output);
            output = NULL;
            break;
        }
    }
    return output;


}


// Comparison and swap functions for sorting values directly
#define PSVECTOR_SORT_COMPARE_DIRECT(A,B) (value[A] < value[B])
#define PSVECTOR_SORT_SWAP_DIRECT(TYPE,A,B) { \
    if (A != B) { \
        ps##TYPE temp = value[A]; \
        value[A] = value[B]; \
        value[B] = temp; \
    } \
}

// Comparison and swap functions for sorting vector indices
#define PSVECTOR_SORT_COMPARE_INDEX(A,B) (value[index[A]] < value[index[B]])
#define PSVECTOR_SORT_SWAP_INDEX(TYPE,A,B) { \
    if (A != B) { \
        ps##TYPE temp = index[A]; \
        index[A] = index[B]; \
        index[B] = temp; \
    } \
}

#define PSVECTOR_SORT_CASE(ELEMTYPE, COMPAREEXPR, SWAPFUNC, SWAPTYPE) \
case PS_TYPE_##ELEMTYPE: { \
    ps##ELEMTYPE *value = vector->data.ELEMTYPE; \
    PSSORT(vector->n, COMPAREEXPR, SWAPFUNC, SWAPTYPE); \
    break; \
}

bool psVectorSortInPlace(const psVector *vector)
{
    PS_ASSERT_VECTOR_NON_NULL(vector, false);

    if (vector->n < 2) {
        // Already sorted!
        return true;
    }

    switch (vector->type.type) {
        PSVECTOR_SORT_CASE(U8 , PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U8 );
        PSVECTOR_SORT_CASE(U16, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U16);
        PSVECTOR_SORT_CASE(U32, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U32);
        PSVECTOR_SORT_CASE(U64, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U64);
        PSVECTOR_SORT_CASE(S8 , PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S8 );
        PSVECTOR_SORT_CASE(S16, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S16);
        PSVECTOR_SORT_CASE(S32, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S32);
        PSVECTOR_SORT_CASE(S64, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S64);
        PSVECTOR_SORT_CASE(F32, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, F32);
        PSVECTOR_SORT_CASE(F64, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input psVector is an unsupported type (0x%x)."),
                vector->type.type);
        return false;
    }
    return true;
}

psVector* psVectorSort(psVector* outVector,
                       const psVector* inVector)
{
    if (!inVector) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("psVectorSort can not sort a NULL psVector."));
        psFree(outVector);
        return NULL;
    }

    if (outVector != inVector) {
        outVector = psVectorCopy(outVector, inVector, inVector->type.type);
        if (!outVector) {
            psError(PS_ERR_BAD_PARAMETER_NULL, true,
                    "Error in psVectorSort:  psVectorCopy returned NULL vector!\n");
            return NULL;
        }
    }

    if (!psVectorSortInPlace(outVector)) {
        if (outVector != inVector) {
            // It was allocated here.
            psFree(outVector);
        }
        return NULL;
    }

    return outVector;
}

psVector* psVectorSortIndex(psVector *out,
                            const psVector *vector)
{
    PS_ASSERT_VECTOR_NON_NULL(vector, NULL);

    psVector *indexVector = psVectorCreate(out, 0, vector->n, 1, PS_TYPE_S32); // Array of indices
    psS32 *index = indexVector->data.S32; // Dereference for convenience

    if (vector->n < 2) {
        // Already sorted
        return indexVector;
    }
    switch (vector->type.type) {
        PSVECTOR_SORT_CASE(U8 , PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(U16, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(U32, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(U64, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(S8 , PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(S16, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(S32, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(S64, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(F32, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
        PSVECTOR_SORT_CASE(F64, PSVECTOR_SORT_COMPARE_INDEX, PSVECTOR_SORT_SWAP_INDEX, S32);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input psVector is an unsupported type (0x%x)."),
                vector->type.type);
        psFree(indexVector);
        return NULL;
    }

    return indexVector;
}

// Selection for different vector types
#define PSVECTOR_SELECT_CASE(TYPE, COMPAREFUNC, SWAPFUNC, SWAPTYPE) \
case PS_TYPE_##TYPE: { \
    ps##TYPE *value = vector->data.TYPE; \
    PSSELECT(vector->n, rank, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, SWAPTYPE); \
    break; \
}


bool psVectorSelectInPlace(psVector *vector, long rank)
{
    PS_ASSERT_VECTOR_NON_NULL(vector, false);
    PS_ASSERT_INT_NONNEGATIVE(rank, false);
    PS_ASSERT_INT_LESS_THAN(rank, vector->n, false);

    if (vector->n < 2) {
        // Already sorted!
        return true;
    }

    switch (vector->type.type) {
        PSVECTOR_SELECT_CASE(U8 , PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U8 );
        PSVECTOR_SELECT_CASE(U16, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U16);
        PSVECTOR_SELECT_CASE(U32, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U32);
        PSVECTOR_SELECT_CASE(U64, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, U64);
        PSVECTOR_SELECT_CASE(S8 , PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S8 );
        PSVECTOR_SELECT_CASE(S16, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S16);
        PSVECTOR_SELECT_CASE(S32, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S32);
        PSVECTOR_SELECT_CASE(S64, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, S64);
        PSVECTOR_SELECT_CASE(F32, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, F32);
        PSVECTOR_SELECT_CASE(F64, PSVECTOR_SORT_COMPARE_DIRECT, PSVECTOR_SORT_SWAP_DIRECT, F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input psVector is an unsupported type (0x%x)."),
                vector->type.type);
        return false;
    }
    return true;
}

psVector *psVectorSelect(psVector *out, const psVector *in, long rank)
{
    PS_ASSERT_VECTOR_NON_NULL(in, NULL);
    PS_ASSERT_INT_NONNEGATIVE(rank, false);
    PS_ASSERT_INT_LESS_THAN(rank, in->n, false);

    if (out != in) {
        out = psVectorCopy(out, in, in->type.type);
        if (!out) {
            psError(PS_ERR_UNKNOWN, false, "Unable to copy vector to select.");
            return NULL;
        }
    }

    if (!psVectorSelectInPlace(out, rank)) {
        if (out != in) {
            // It was allocated here.
            psFree(out);
        }
        return NULL;
    }

    return out;
}

psString psVectorToString(const psVector* vector,
                          int maxLength)
{

    if (maxLength < 5) {
        return NULL;
    }

    psString str = psStringAlloc(maxLength + 1);

    if (vector == NULL) {
        snprintf(str,maxLength, "NULL");
        return str;
    }

    int size = vector->n;

    if (size == 0) {
        snprintf(str,maxLength, "[]");
        return str;
    }

    psString tempStr = psStringAlloc(maxLength+1);
    *str = '\0';
    bool full = false;

    #define APPEND_ELEMENTS_CASE(TYPE, NATIVE_TYPE, FORMAT) \
case PS_TYPE_##TYPE: \
    for (lcv=0; lcv < size && ! full; lcv++) { \
        snprintf(tempStr, maxLength, "%s" FORMAT, prefix, (NATIVE_TYPE) (vector->data.TYPE[lcv])); \
        strncat(str,tempStr,maxLength); \
        full = (strlen(str) > maxLength-2); \
        prefix = ","; \
    } \
    break;

    int lcv;
    char* prefix = "[";
    switch(vector->type.type) {
        APPEND_ELEMENTS_CASE(S8,char,"%hd")
        APPEND_ELEMENTS_CASE(S16,short int,"%hd")
        APPEND_ELEMENTS_CASE(S32,int,"%d")
        APPEND_ELEMENTS_CASE(S64,long,"%ld")
        APPEND_ELEMENTS_CASE(U8,unsigned char,"%hu")
        APPEND_ELEMENTS_CASE(U16,unsigned short,"%hu")
        APPEND_ELEMENTS_CASE(U32,unsigned int, "%u")
        APPEND_ELEMENTS_CASE(U64,unsigned long,"%lu")
        APPEND_ELEMENTS_CASE(F32,double,"%g")
        APPEND_ELEMENTS_CASE(F64,double,"%g")
    default:
        snprintf(str,maxLength,"[...]");
        break;
    }

    if (full) {
        // couldn't all fit in given string length

        // remove elements until there is room for ",...]"
        while (strlen(str) > maxLength - 5) {
            char* lastComma = strrchr(str,',');
            if (lastComma == NULL) { // no comma, must be first number
                str[1] = '\0';
            } else {
                *lastComma = '\0';
            }
        }
        strncat(str,",...]",maxLength);
    } else {
        strncat(str,"]",maxLength);
    }

    psFree(tempStr);

    return str;
}

psF64 p_psVectorGetElementF64(const psVector* vector,
                              int position)
{
    if (vector == NULL) {
        return NAN;
    }
    if (position < 0 || position >= vector->n) {
        return NAN;
    }

    switch (vector->type.type) {
    case PS_TYPE_U8:
        return vector->data.U8[position];
        break;
    case PS_TYPE_U16:
        return vector->data.U16[position];
        break;
    case PS_TYPE_U32:
        return vector->data.U32[position];
        break;
    case PS_TYPE_U64:
        return vector->data.U64[position];
        break;
    case PS_TYPE_S8:
        return vector->data.S8[position];
        break;
    case PS_TYPE_S16:
        return vector->data.S16[position];
        break;
    case PS_TYPE_S32:
        return vector->data.S32[position];
        break;
    case PS_TYPE_S64:
        return vector->data.S64[position];
        break;
    case PS_TYPE_F32:
        return vector->data.F32[position];
        break;
    case PS_TYPE_F64:
        return vector->data.F64[position];
    default:
        return NAN;
    }
}


bool p_psVectorPrint (int fd,
                      const psVector *a,
                      char *name)
{
    char line[1024];

    sprintf (line, "# vector: %s\n", name);
    if (write(fd, line, strlen(line))) {;} //ignore return value

    for (int i = 0; i < a[0].n; i++) {
        sprintf (line, "%f\n", p_psVectorGetElementF64(a, i));
        if (write(fd, line, strlen(line))) {;} //ignore return value
    }
    sprintf (line, "\n");
    if (write(fd, line, strlen(line))) {;} //ignore return value
    return (true);
}

// Image initialisation for integer types
#define VECTORINIT_INTCASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            if (value == 0.0) { \
                memset(vector->data.TYPE, 0, vector->n * sizeof(ps##TYPE)); \
            } else { \
                if (value < (double)PS_MIN_##TYPE || value > (double)PS_MAX_##TYPE) { \
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Error: Value %f out of range for type %s.\n", \
                            value, #TYPE); \
                    return false; \
                } \
                ps##TYPE castValue = (ps##TYPE)value; \
                ps##TYPE *vectorData = vector->data.TYPE; \
                for (int i = 0; i < vector->n; i++) { \
                    vectorData[i] = castValue; \
                } \
            } \
            return true; \
        }

// Image initialisation for char-size integer types
#define VECTORINIT_CHARCASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            if (value < (double)PS_MIN_##TYPE || value > (double)PS_MAX_##TYPE) { \
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Error: Value %f out of range for type %s.\n", \
                        value, #TYPE); \
                return false; \
            } \
            memset(vector->data.TYPE, (ps##TYPE)value, vector->n * sizeof(ps##TYPE)); \
            return true; \
        }

// Image initialisation for floating point types
#define VECTORINIT_FLOATCASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            if (value == 0.0) { \
                memset(vector->data.TYPE, 0, vector->n * sizeof(ps##TYPE)); \
            } else { \
                ps##TYPE castValue = (ps##TYPE)value; \
                ps##TYPE *vectorData = vector->data.TYPE; \
                for (int i = 0; i < vector->n; i++) { \
                    vectorData[i] = castValue; \
                } \
            } \
            return true; \
        }



bool psVectorInit(psVector *vector, double value)
{
    PS_ASSERT_VECTOR_NON_NULL(vector, false);

    switch (vector->type.type) {
        VECTORINIT_CHARCASE(U8)
        VECTORINIT_INTCASE(U16)
        VECTORINIT_INTCASE(U32)
        VECTORINIT_CHARCASE(S8)
        VECTORINIT_INTCASE(S16)
        VECTORINIT_INTCASE(S32)
        VECTORINIT_INTCASE(U64)
        VECTORINIT_INTCASE(S64)
        VECTORINIT_FLOATCASE(F32)
        VECTORINIT_FLOATCASE(F64)
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type %x not supported", vector->type.type);
    }
    return (false);
}

#define FUNC_MACRO_VECTOR_CREATE(TYPE) \
static psVector *vectorCreate##TYPE(const char *file, unsigned int lineno, const char *func, psVector *input, double lower, double upper, double delta) \
{ \
    \
    int nBin = (upper - lower) / delta; \
    psVector *vec = p_psVectorRecycle(file, lineno, func, input, nBin, PS_TYPE_##TYPE); \
    vec->n = nBin; \
    for (int i = 0; i < nBin; i++) \
    { \
        vec->data.TYPE[i] = lower + (i * delta); \
    } \
    return vec; \
} \

FUNC_MACRO_VECTOR_CREATE(S8)
FUNC_MACRO_VECTOR_CREATE(S16)
FUNC_MACRO_VECTOR_CREATE(S32)
FUNC_MACRO_VECTOR_CREATE(S64)
FUNC_MACRO_VECTOR_CREATE(U8)
FUNC_MACRO_VECTOR_CREATE(U16)
FUNC_MACRO_VECTOR_CREATE(U32)
FUNC_MACRO_VECTOR_CREATE(U64)
FUNC_MACRO_VECTOR_CREATE(F32)
FUNC_MACRO_VECTOR_CREATE(F64)

psVector *p_psVectorCreate(const char *file,
                           unsigned int lineno,
                           const char *func,
                           psVector *input,
                           double lower,
                           double upper,
                           double delta,
                           psElemType type)
{
    psVector *out = NULL;
    switch (type) {
    case PS_TYPE_S8:
        out = vectorCreateS8(file, lineno, func, input, lower,  upper, delta);
        break;
    case PS_TYPE_S16:
        out = vectorCreateS16(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_S32:
        out = vectorCreateS32(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_S64:
        out = vectorCreateS64(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_U8:
        out = vectorCreateU8(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_U16:
        out = vectorCreateU16(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_U32:
        out = vectorCreateU32(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_U64:
        out = vectorCreateU64(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_F32:
        out = vectorCreateF32(file, lineno, func, input, lower, upper, delta);
        break;
    case PS_TYPE_F64:
        out = vectorCreateF64(file, lineno, func, input, lower, upper, delta);
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Invalid psType for Vector Create\n");
    }
    return (out);
}

bool psVectorSet(psVector *input,
                 long position,
                 double value)
{
    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true, _("The input psVector can not be NULL."));
        return false;
    }
    if (position > input->n) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Number too large\n");
        return false;
    }
    if (position < 0)
        position += input->n;
    if (position < 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Negative number too large\n");
        return false;
    }

    if (position == input->n) {
        if (position >= input->nalloc) {
            psError(PS_ERR_BAD_PARAMETER_NULL, true,
                    "Specified position, %ld, is greater than n+1 of the vector, %ld.",
                    position, input->nalloc);
            return false;
        }
        (*(psVector**)&input)->n++;
    }

    switch (input->type.type) {
    case PS_TYPE_U8:
        input->data.U8[position] = (psU8)value;
        break;
    case PS_TYPE_U16:
        input->data.U16[position] = (psU16)value;
        break;
    case PS_TYPE_U32:
        input->data.U32[position] = (psU32)value;
        break;
    case PS_TYPE_U64:
        input->data.U64[position] = (psU64)value;
        break;
    case PS_TYPE_S8:
        input->data.S8[position] = (psS8)value;
        break;
    case PS_TYPE_S16:
        input->data.S16[position] = (psS16)value;
        break;
    case PS_TYPE_S32:
        input->data.S32[position] = (psS32)value;
        break;
    case PS_TYPE_S64:
        input->data.S64[position] = (psS64)value;
        break;
    case PS_TYPE_F32:
        input->data.F32[position] = (psF32)value;
        break;
    case PS_TYPE_F64:
        input->data.F64[position] = (psF64)value;
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Invalid psVector Data Type\n");
        return false;
    }

    return true;
}

double psVectorGet(const psVector *input,
                           long position)
{
    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true, _("The input psVector can not be NULL."));
        return NAN;
    }
    if (position >= input->n) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Number too large\n");
        return NAN;
    }
    if(position < 0)
        position += input->n;
    if (position < 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Negative number too large\n");
        return NAN;
    }
    switch (input->type.type) {
    case PS_TYPE_U8:
        return input->data.U8[position];
        break;
    case PS_TYPE_U16:
        return input->data.U16[position];
        break;
    case PS_TYPE_U32:
        return input->data.U32[position];
        break;
    case PS_TYPE_U64:
        return input->data.U64[position];
        break;
    case PS_TYPE_S8:
        return input->data.S8[position];
        break;
    case PS_TYPE_S16:
        return input->data.S16[position];
        break;
    case PS_TYPE_S32:
        return input->data.S32[position];
        break;
    case PS_TYPE_S64:
        return input->data.S64[position];
        break;
    case PS_TYPE_F32:
        return input->data.F32[position];
        break;
    case PS_TYPE_F64:
        return input->data.F64[position];
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Invalid psVector Data Type\n");
        return NAN;
    }

}

// count number of pixels with given mask value.  the comparison is against a U64 type to
// allow any int-type of vector.  all signed and unsigned int types are allowed
long psVectorCountPixelMask (psVector *mask,
                             psU64 value)
{
    long Npixels = 0;
    if (mask == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true, _("The input psVector can not be NULL."));
        Npixels = -1;
        return Npixels;
    }

# define PS_VECTOR_COUNT_PIXEL_MASK(NAME,TYPE) \
    case PS_TYPE_##NAME: \
        for (long i = 0; i < mask->n; i++) { \
            if (mask->data.TYPE[i] & value) { \
                Npixels ++; \
            } \
        } \
        break;

    psElemType type = mask->type.type;
    switch (type) {
	PS_VECTOR_COUNT_PIXEL_MASK(U8, U8);
	PS_VECTOR_COUNT_PIXEL_MASK(U16,U16);
	PS_VECTOR_COUNT_PIXEL_MASK(U32,U32);
	PS_VECTOR_COUNT_PIXEL_MASK(U64,U64);
	PS_VECTOR_COUNT_PIXEL_MASK(S8, S8);
	PS_VECTOR_COUNT_PIXEL_MASK(S16,S16);
	PS_VECTOR_COUNT_PIXEL_MASK(S32,S32);
	PS_VECTOR_COUNT_PIXEL_MASK(S64,S64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input psVector is an unsupported type (0x%x)."), type);
        return -1;
    }
    return (Npixels);
}

long psVectorLength(const psVector *vector)
{
    if ( !psMemCheckVector((psVector*)vector) ) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Error:  Specified vector is not a valid psVector \n");
        return -1;
    }
    return (vector->n);
}



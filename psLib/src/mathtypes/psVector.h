/* @file  psVector.h
 *
 * @brief basic vector definitions and operations
 *
 * This file defines the basic type for a vector struct and functions useful
 * in manupulating vectors.
 *
 * @author Robert DeSonia, MHPCC
 * @author Ross Harman, MHPCC
 * @author Joshua Hoblitt, University of Hawaii
 *
 * @version $Revision: 1.74 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_VECTOR_H
#define PS_VECTOR_H

/// @addtogroup MathTypes Mathematical Structures
/// @{

#include <stdio.h>
#include "psType.h"
#include "psMutex.h"

/** An vector to support primitive types.
 *
 * Struct for maintaining an vector of frequently used primitive types.
 *
 */
typedef struct
{
    psMathType type;                   ///< Type of data.
    long n;                            ///< Number of elements in use.
    const long nalloc;                 ///< Total number of elements available.
    union {
        psS8* S8;                      ///< Signed 8-bit integer data.
        psS16* S16;                    ///< Signed 16-bit integer data.
        psS32* S32;                    ///< Signed 32-bit integer data.
        psS64* S64;                    ///< Signed 64-bit integer data.
        psU8* U8;                      ///< Unsigned 8-bit integer data.
        psU16* U16;                    ///< Unsigned 16-bit integer data.
        psU32* U32;                    ///< Unsigned 32-bit integer data.
        psU64* U64;                    ///< Unsigned 64-bit integer data.
        psF32* F32;                    ///< Single-precision float data.
        psF64* F64;                    ///< Double-precision float data.
    } data;
    psMutex lock;                       ///< Optional lock for thread safety.
}
psVector;

#define P_PSVECTOR_SET_NALLOC(vec,n) *(long*)&(vec->nalloc) = n

/*****************************************************************************/

/* FUNCTION PROTOTYPES                                                       */

/*****************************************************************************/

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psVector structure, false
 *  otherwise.
 */
bool psMemCheckVector(
    psPtr ptr                          ///< the pointer whose type to check
)
;

/** Allocate a vector, with length set to number allocated
 *
 *  Uses psLib memory allocation functions to create a vector collection of
 *  data as defined by the psType type.
 *
 * @return psVector*    Pointer to psVector.
 */
#ifdef DOXYGEN
psVector* psVectorAlloc(
    long nalloc,                       ///< Total number of elements to make available.
    psElemType type                    ///< Type of data to be held by vector.
);
#else // ifdef DOXYGEN
psVector* p_psVectorAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc,                        ///< Total number of elements to make available.
    psElemType type                    ///< Type of data to be held by vector.
) PS_ATTR_MALLOC;
#define psVectorAlloc(nalloc, type) \
      p_psVectorAlloc(__FILE__, __LINE__, __func__, nalloc, type)
#endif // ifdef DOXYGEN


/** Allocate a vector, with length set to zero
 *
 *  Uses psLib memory allocation functions to create a vector collection of
 *  data as defined by the psType type.
 *
 * @return psVector*    Pointer to psVector.
 */
#ifdef DOXYGEN
psVector* psVectorAllocEmpty(
    long nalloc,                       ///< Total number of elements to make available.
    psElemType type                    ///< Type of data to be held by vector.
);
#else // ifdef DOXYGEN
psVector* p_psVectorAllocEmpty(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc,                       ///< Total number of elements to make available.
    psElemType type                    ///< Type of data to be held by vector.
) PS_ATTR_MALLOC;
#define psVectorAllocEmpty(nalloc, type) \
      p_psVectorAllocEmpty(__FILE__, __LINE__, __func__, nalloc, type)
#endif // ifdef DOXYGEN


/** Reallocate a vector.
 *
 *  Uses psLib memory allocation functions to reallocate a vector collection
 *  of data. The vector is reallocated according to the psType type member
 *  contained within the vector.
 *
 *  @return psVector*      Pointer to psVector.
 *
 */
psVector* psVectorRealloc(
    psVector* vector,                  ///< Vector to reallocate.
    long nalloc                        ///< Total number of elements to make available.
);


/** Extend a vector's length.
 *
 *  Increments a vector's length, n, by the specified number of elements.
 *  If the allocated storage is less than the current vector's length plus
 *  twice the number of elements to be added, it is reallocated larger by
 *  a given amount.
 *
 *  @return psVector*      Pointer to the adjusted psVector
 */
psVector *psVectorExtend(
    psVector *vector,                  ///< Vector to extend
    long delta,                        ///< Amount to expand allocation, if necessary
    long nExtend                       ///< Number of elements to add to vector length
);


// add one more element of the vector type, extend as needed
bool psVectorAppend(psVector *vector,...);

/** Recycle a vector.
 *
 *  Uses psLib memory allocation functions to reallocate a vector collection
 *  of data. The vector is reallocated according to the psElemType type
 *  parameter.
 *
 * @return psVector*       Pointer to psVector.
 *
 */
#ifdef DOXYGEN
psVector* psVectorRecycle(
    psVector* vector,
    ///< Vector to recycle.  If NULL, a new vector is created.  No effort
    ///< taken to preserve the values.

    long nalloc,                        ///< Total number of elements to make available.
    psElemType type                     ///< the datatype of the returned vector
);
#else // ifdef DOXYGEN
psVector* p_psVectorRecycle(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psVector* vector,
    ///< Vector to recycle.  If NULL, a new vector is created.  No effort
    ///< taken to preserve the values.

    long nalloc,                        ///< Total number of elements to make available.
    psElemType type                     ///< the datatype of the returned vector
);
#define psVectorRecycle(vector, nalloc, type) \
      p_psVectorRecycle(__FILE__, __LINE__, __func__, vector, nalloc, type)
#endif // ifdef DOXYGEN


/** Copy a vector, converting types.
 *
 *  Performs a deep copy of the elements of one psVector to a new psVector,
 *  converting numeric types to a specified type.
 *
 * @return psVector*       Pointer to resulting psVector.
 *
 */
#ifdef DOXYGEN
psVector* psVectorCopy(
    psVector* output,                  ///< if non-NULL, a psVector to recycle
    const psVector* input,             ///< the vector to copy.
    psElemType type                    ///< the data type of the resulting psVector
);
#else // ifdef DOXYGEN
psVector* p_psVectorCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psVector* output,                  ///< if non-NULL, a psVector to recycle
    const psVector* input,             ///< the vector to copy.
    psElemType type                    ///< the data type of the resulting psVector
);
#define psVectorCopy(output, input, type) \
      p_psVectorCopy(__FILE__, __LINE__, __func__, output, input, type)
#endif // ifdef DOXYGEN

/** Sort a vector in-place
 *
 * Sorts in ascending order
 *
 * @return  bool           Success or failure
 */
bool psVectorSortInPlace(const psVector *vector ///< Vector to sort
    );

/** Sort an array of floats.
 *
 *  Sorts an array of floats in ascending order.
 *
 *  @return  psVector*     Pointer to sorted psVector.
 */
psVector* psVectorSort(
    psVector* outVector,               ///< the output vector to recycle, or NULL if new vector desired.
    const psVector* inVector           ///< the vector to sort.
);


/** Creates an array of indices based on sort ordered of array.
 *
 *  Sorts a vector and creates an integer array holding indices of
 *  sorted float values based on pre-sort index positions.
 *
 *  @return  psVector*     vector of the indices of sort.
 */
psVector* psVectorSortIndex(
    psVector* outVector,               ///< vector to recycle
    const psVector* inVector           ///< vector to sort
);


/// Select a ranked element within a vector; operations are performed in-place
///
/// Selection orders the vector so that the element of the nominated rank is in the correct place.  No
/// guarantee is made about other elements.
bool psVectorSelectInPlace(psVector *vector, ///< Vector from which to select
                           long rank    ///< Rank of interest
    );

/// Select a ranked element within a vector; operations are performed on a copy
///
/// Selection orders the vector so that the element of the nominated rank is in the correct place.  No
/// guarantee is made about other elements.
psVector *psVectorSelect(psVector *out, ///< Output vector, or NULL
                         const psVector *in, ///< Vector from which to select
                         long rank      ///< Rank of interest
    );


/** Creates a string from a psVector's values in the form "[x0,x1,x2]".
 *
 *  @return psPtr          a newly allocated string
 */
psString psVectorToString(
    const psVector* vector,             ///< vector to create a string from
    int maxLength                      ///< the maximum length of the resulting string
);


/** Returns an element in the vector as a psF64 value
 *
 *  @return psF64          the value at specified position, or NAN if position is invalid.
 */
psF64 p_psVectorGetElementF64(
    const psVector* vector,                  ///< vector to retrieve element
    int position                       ///< the vector position to get
);


/** Print a vector to a stream
 *
 *  @return bool          TRUE is successful, otherwise FALSE.
 */
bool p_psVectorPrint(
    int fd,                            ///< output file descriptor
    const psVector *a,                       ///< vector to print
    char *name                         ///< name of vector (for title)
);


/** Initializes the vector with the given value.
 *
 *  The input data is cast to match the vector datatype, allowing for integers
 *  to be preserved.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psVectorInit(
    psVector *vector,                  ///< the vector to be initialized
    double value                        ///< Value to which to initialise
);


/** Creates a new vector, or reallocates the provided vector if input is not NULL.
 *
 *  The created vector consists of the data range starting at lower, running to
 *  upper, in steps of delta.  The upper-end value is exclusive; the sequence
 *  is equivalent to for (x = lower; x <= upper - 1; x += delta).
 *
 *  @return psVector*:       the newly created psVector
 */
#ifdef DOXYGEN
psVector *psVectorCreate(
    psVector *input,                   ///< Input vector
    double lower,                      ///< lower bound
    double upper,                      ///< upper bound
    double delta,                      ///< size of increment
    psElemType type                    ///< type of vector to create
);
#else // ifdef DOXYGEN
psVector *p_psVectorCreate(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psVector *input,                   ///< Input vector
    double lower,                      ///< lower bound
    double upper,                      ///< upper bound
    double delta,                      ///< size of increment
    psElemType type                    ///< type of vector to create
);
#define psVectorCreate(input, lower, upper, delta, type) \
      p_psVectorCreate(__FILE__, __LINE__, __func__, input, lower, upper, delta, type)
#endif // ifdef DOXYGEN


/** Sets the value of the input vector at the specified position to value.
 *
 *  A negative position means index from the end.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psVectorSet(
    psVector *input,                    ///< Input vector to set
    long position,                      ///< vector position
    double value                        ///< value to set
);


/** Returns the value of the input vector at the specified position.
 *
 *  A negative position means index from the end.
 *
 *  @return double:        Value of the input vector at the specified position.
 */
double psVectorGet(
    const psVector *input,             ///< Input vector from which to get value
    long position                      ///< vector position
);


/** Returns the number of pixels in the vector which satisfy any of the mask bits.
 *
 *  An error (eg, invalid vector) results in a return value of -1.  The vector
 *  must be U8.
 *
 *  @return long:       the number of pixels counted
 */
long psVectorCountPixelMask(
    psVector *mask,			///< input vector to count
    psU64 value				///< the mask value to satisfy
);


/** Get the number of elements in use from a specified psVector. (vector.n)
 *
 *  @return long:       The number of elements in use.
 */
long psVectorLength(
    const psVector *vector             ///< input psVector
);


/*****************************************************************************
    PS_VECTOR macros:
 *****************************************************************************/

#define PS_ASSERT_VECTOR_NON_NULL(NAME, RVAL) PS_ASSERT_GENERAL_VECTOR_NON_NULL(NAME, return RVAL)
#define PS_ASSERT_GENERAL_VECTOR_NON_NULL(NAME, CLEANUP) \
if ((NAME) == NULL || (NAME)->data.U8 == NULL) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: psVector %s or its data is NULL.", \
            #NAME); \
    CLEANUP; \
} \

#define PS_ASSERT_VECTOR_NON_EMPTY(NAME, RVAL) PS_ASSERT_GENERAL_VECTOR_NON_EMPTY(NAME, return RVAL)
#define PS_ASSERT_GENERAL_VECTOR_NON_EMPTY(NAME, CLEANUP) \
if ((NAME)->n < 1) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "Unallowable operation: psVector %s has no elements.", \
            #NAME); \
    CLEANUP; \
} \

#define PS_ASSERT_VECTOR_TYPE_F32_OR_F64(NAME, RVAL) \
if (((NAME)->type.type != PS_TYPE_F32) && ((NAME)->type.type != PS_TYPE_F64)) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "psVector %s: bad type(%d)", \
            #NAME, (NAME)->type.type); \
    return(RVAL); \
} \

#define PS_ASSERT_VECTOR_TYPE_S16_S32_F32(NAME, RVAL) \
if (((NAME)->type.type != PS_TYPE_S16) && ((NAME)->type.type != PS_TYPE_S32) && ((NAME)->type.type != PS_TYPE_F32)) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "psVector %s: bad type(%d)", \
            #NAME, (NAME)->type.type); \
    return(RVAL); \
} \

#define PS_ASSERT_VECTOR_TYPE(NAME, TYPE, RVAL) \
if ((NAME)->type.type != TYPE) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: psVector %s has incorrect type.", \
            #NAME); \
    return(RVAL); \
}

#define PS_ASSERT_VECTORS_SIZE_EQUAL(VEC1, VEC2, RVAL) \
if ((VEC1)->n != (VEC2)->n) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "psVector %s has size %ld, psVector %s has size %ld.", \
            #VEC1, (VEC1)->n, #VEC2, (VEC2)->n); \
    return(RVAL); \
}

#define PS_ASSERT_VECTOR_SIZE(VEC, SIZE, RVAL) \
if ((VEC)->n != (SIZE)) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "psVector %s has size %ld, should be %ld.", \
            #VEC, (VEC)->n, (SIZE)); \
    return(RVAL); \
}

#define PS_ASSERT_VECTOR_TYPE_EQUAL(VEC1, VEC2, RVAL) \
if ((VEC1)->type.type != (VEC2)->type.type) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "psVector %s has size %d, psVector %s has size %d.", \
            #VEC1, (VEC1)->type.type, #VEC2, (VEC2)->type.type); \
    return(RVAL); \
}

#define PS_VECTOR_PRINT_F32(NAME) \
if ((NAME) != NULL) { \
    for (int i = 0; i < (NAME)->n; i++) { \
        printf("%s->data.F32[%d] is %f\n", #NAME, i, (NAME)->data.F32[i]); \
    } \
    printf("\n"); \
} else {\
    printf("MACRO WARNING: vector %s is NULL.\n", #NAME); \
}\

#define PS_VECTOR_PRINT_F64(NAME) \
if ((NAME) != NULL) { \
    for (int i = 0; i < (NAME)->n; i++) { \
        printf("%s->data.F64[%d] is %f\n", #NAME, i, (NAME)->data.F64[i]); \
    } \
    printf("\n"); \
} else {\
    printf("MACRO WARNING: vector %s is NULL.\n", #NAME); \
}\

/// @}
#endif // #ifndef PS_VECTOR_H

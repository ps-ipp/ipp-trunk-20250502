/** @file  psArray.h
 *
 *  @brief Contains basic array definitions and operations
 *
 *  This file defines the basic type for a array struct and functions useful
 *  in manupulating arrays.
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Ross Harman, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.54 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-14 03:18:41 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_ARRAY_H
#define PS_ARRAY_H

#include "psType.h"
#include "psCompare.h"
#include "psVector.h"
#include "psMutex.h"

/// @addtogroup DataContainer Data Containers
/// @{

/** An array to support primitive types.
 *
 * Struct for maintaining an array of frequently used primitive types.
 *
 */
typedef struct
{
    long n;                            ///< Number of elements in use.
    const long nalloc;                 ///< Total number of elements available.
    psPtr* data;                       ///< An Array of pointer elements
    psMutex lock;                       ///< Optional lock for thread safety
}
psArray;

#define P_PSARRAY_SET_NALLOC(vec,n) *(long*)&vec->nalloc = n

/*****************************************************************************/

/* FUNCTION PROTOTYPES                                                       */

/*****************************************************************************/

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psArray structure, false
 *  otherwise.
 */
bool psMemCheckArray(
    psPtr ptr                          ///< the pointer whose type to check
)
;


/** Allocate an array, set the length to the number of allocated elements
 *
 * Uses psLib memory allocation functions to create an array collection of
 * data
 *
 * @return psArray* : Pointer to psArray.
 *
 */
#ifdef DOXYGEN
psArray* psArrayAlloc(
    long nalloc                         ///< Total number of elements to make available.
);
#else // ifdef DOXYGEN
psArray* p_psArrayAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc                         ///< Total number of elements to make available.
) PS_ATTR_MALLOC;
#define psArrayAlloc(nalloc) \
      p_psArrayAlloc(__FILE__, __LINE__, __func__, nalloc)
#endif // ifdef DOXYGEN


/** Allocate an array, set the length to zero.
 *
 * Uses psLib memory allocation functions to create an array collection of
 * data
 *
 * @return psArray* : Pointer to psArray.
 *
 */
#ifdef DOXYGEN
psArray* psArrayAllocEmpty(
    long nalloc                         ///< Total number of elements to make available.
);
#else // ifdef DOXYGEN
psArray* p_psArrayAllocEmpty(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc                         ///< Total number of elements to make available.
);
#define psArrayAllocEmpty(nalloc) \
      p_psArrayAllocEmpty(__FILE__, __LINE__, __func__, nalloc)
#endif // ifdef DOXYGEN


/** Reallocate an array.
 *
 * Uses psLib memory allocation functions to reallocate an array collection
 * of data.
 *
 * @return psArray* : Pointer to psArray.
 *
 */
#ifdef DOXYGEN
psArray* psArrayRealloc(
    psArray* array,                    ///< array to reallocate.
    long nalloc                        ///< Total number of elements to make available.
);
#else // ifdef DOXYGEN
psArray* p_psArrayRealloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psArray* array,                    ///< array to reallocate.
    long nalloc                        ///< Total number of elements to make available.
);
#define psArrayRealloc(array, nalloc) \
      p_psArrayRealloc(__FILE__, __LINE__, __func__, array, nalloc)
#endif // ifdef DOXYGEN


/** Add an element to the end the array, expanding the array storage if
 *  necessary.
 *
 *  If delta < 1, then 10 is used.
 *
 *  @return psArray*        The array with the element added
 */
psArray* psArrayAdd(
    psArray* array,                    ///< array to operate on
    long delta,                        ///< the amount to expand array, if necessary.
    psPtr data                         ///< the data pointer to add to psArray
);
#ifdef DOXYGEN
psArray* psArrayAdd(
    psArray* array,                    ///< array to operate on
    long delta,                        ///< the amount to expand array, if necessary.
    psPtr data                         ///< the data pointer to add to psArray
);
#else // ifdef DOXYGEN
psArray* p_psArrayAdd(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psArray* array,                     ///< array to operate on
    long delta,                         ///< the amount to expand array, if necessary.
    psPtr data                          ///< the data pointer to add to psArray
) PS_ATTR_MALLOC;
#define psArrayAdd(array, delta, data) \
      p_psArrayAdd(__FILE__, __LINE__, __func__, array, delta, data)
#endif // ifdef DOXYGEN

/// Add a scalar value to an array using psScalar
#define PS_ARRAY_ADD_SCALAR(ARRAY, VALUE, TYPE) { \
      psScalar *scalar = psScalarAlloc(VALUE, TYPE); \
      psArrayAdd(ARRAY, 0, scalar); \
      psFree(scalar); \
}


/** Remove an element from the array by it's pointer
 *
 *  Finds and removes the specified data pointer from the list.
 *
 * @return bool:  TRUE if the specified data pointer was found and removed,
 *                otherwise FALSE.
 *
 */
bool psArrayRemoveData(
    psArray* array,                    ///< array to operate on
    psPtr data			       ///< the data pointer to remove from psArray
);


/** Remove an element from the array by it's pointer WITHOUT freeing
 *
 *  Finds and removes the specified data pointer from the list, but does not free it
 *
 * @return bool:  TRUE if the specified data pointer was found and removed,
 *                otherwise FALSE.
 *
 */
bool psArrayRemoveDataNoFree(
    psArray* array,                    ///< array to operate on
    const psPtr data                   ///< the data pointer to remove from psArray
);


/** Remove an element from the array
 *
 *  Finds and removes the elements as the specified position
 *
 * @return bool:  TRUE if the specified data pointer was found and removed,
 *                otherwise FALSE.
 *
 */
bool psArrayRemoveIndex(
    psArray* array,                    ///< array to operate on
    long index                      ///< the element to remove
);


/** Deallocate/Dereference elements of an array.
 *
 * Uses psLib memory allocation functions to deallocate/dereference elements
 * of a array of void pointers.  The array psArr is not freed, and its elements
 * will all be set to NULL.  Additionaly, the array size (n) is set to zero.
 *
 */
void psArrayElementsFree(
    psArray* array                     ///< Void pointer array to destroy.
);


/** Sort the array according to an external compare function.
 *
 *  Sorts an array via the specification of a comparison function
 *  to specify how the objects on the array should be sorted.
 *
 *  The comparison function must return an integer less than, equal to, or
 *  greater than zero if the first argument is considered to be respectively
 *  less than, equal to, or greater than the second.
 *
 *  If two members compare as equal, their order in the sorted array is
 *  undefined.
 *
 *  @return psArray* The sorted array.
 */
psArray* psArraySort(
    psArray* array,                       ///< input array to sort.
    psComparePtrFunc func                 ///< the compare function
);

// return the index which sorts the array
psVector *psArraySortIndex (psVector *outIndex, psArray *in, psCompareFunc func);

/** Set an element in the array.  If the current element is non-NULL, the old
 *  element is freed.
 *
 *  @return bool  TRUE if the element was set successfully, otherwise FALSE
 */
bool psArraySet(
    psArray* array,                    ///< input array to set element in
    long position,                     ///< the element position to set
    psPtr data                         ///< the value to set it to
);


/** Get an element from the array.
 *
 *  @return void*   the element at given position.
 */
psPtr psArrayGet(
    const psArray* array,              ///< input array to get element from
    long position                      ///< the element position to get
);


/** Get the number of elements in use from a specified psArray. (array.n)
 *
 *  @return long:       The number of elements in use.
 */
long psArrayLength(
    const psArray *array               ///< input psArray
);


// Some assertions

#define PS_ASSERT_ARRAY_NON_NULL(NAME, RETURNVAL) PS_ASSERT_GENERAL_ARRAY_NON_NULL(NAME, return RETURNVAL)
#define PS_ASSERT_GENERAL_ARRAY_NON_NULL(NAME, CLEANUP) \
if ((NAME) == NULL || (NAME)->data == NULL || (NAME)->n < 0) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: psArray %s or its data is NULL.", \
            #NAME); \
    CLEANUP; \
} \

#define PS_ASSERT_ARRAY_NON_EMPTY(NAME, RETURNVAL) PS_ASSERT_GENERAL_ARRAY_NON_EMPTY(NAME, return RETURNVAL)
#define PS_ASSERT_GENERAL_ARRAY_NON_EMPTY(NAME, CLEANUP) \
if ((NAME)->n < 1) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "Unallowable operation: psArray %s has no elements.", \
            #NAME); \
    CLEANUP; \
} \

#define PS_ASSERT_ARRAYS_SIZE_EQUAL(ARRAY1, ARRAY2, RVAL) \
if ((ARRAY1)->n != (ARRAY2)->n) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "psArray %s has size %ld, psArray %s has size %ld.", \
            #ARRAY1, (ARRAY1)->n, #ARRAY2, (ARRAY2)->n); \
    return(RVAL); \
}

#define PS_ASSERT_ARRAY_SIZE(ARRAY, SIZE, RVAL) \
if ((ARRAY)->n != (SIZE)) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "psArray %s has size %ld instead of expected size %ld.", \
            #ARRAY, (ARRAY)->n, SIZE); \
    return RVAL; \
}

/// @}
#endif // #ifndef PS_ARRAY_H

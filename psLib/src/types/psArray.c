
/** @file  psArray.c
 *
 *  @brief Contains support for basic vector types
 *
 *  This file defines the basic type for a vector struct and functions useful
 *  in manupulating vectors.
 *
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.67 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-11-29 21:48:53 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/******************************************************************************/

/*  INCLUDE FILES                                                             */

/******************************************************************************/
#include<stdlib.h>                         // for qsort, etc.
#include<string.h>

#include "psMemory.h"
#include "psError.h"
#include "psArray.h"
#include "psLogMsg.h"
#include "psAbort.h"
#include "psAssert.h"
#include "psSort.h"

#define DEFAULT_ARRAY_ADD 8            // Default number to add to an array when not specified

/*****************************************************************************
  FUNCTION IMPLEMENTATION - LOCAL
 *****************************************************************************/
static void arrayFree(psArray* psArr);

static void arrayFree(psArray* psArr)
{
    psArrayElementsFree(psArr);

    psFree(psArr->data);
}

bool psMemCheckArray(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)arrayFree );
}

// Allocate an array, deliberately leave size unset.
static psArray *arrayAlloc(const char *file,
                           unsigned int lineno,
                           const char *func,
                           long nalloc)
{
    if (nalloc < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't allocate a psArray of negative size.");
        return NULL;
    }

    // Create vector struct
    psArray *array = (psArray*)p_psAlloc(file, lineno, func, sizeof(psArray));
    psMemSetDeallocator(array, (psFreeFunc)arrayFree);

    P_PSARRAY_SET_NALLOC(array, nalloc);
    // Create vector data array
    array->data = psAlloc(nalloc * sizeof(psPtr));
    memset(array->data, 0, sizeof(void*) * nalloc);  //set the initial values of data to NULL

    return array;
}

/*****************************************************************************
  FUNCTION IMPLEMENTATION - PUBLIC
 *****************************************************************************/
psArray* p_psArrayAlloc(const char *file,
                        unsigned int lineno,
                        const char *func,
                        long nalloc)
{
    psArray *array = arrayAlloc(file, lineno, func, nalloc);
    if (!array) {
        return NULL;
    }
    array->n = nalloc;
    return array;
}

psArray* p_psArrayAllocEmpty(const char *file,
                             unsigned int lineno,
                             const char *func,
                             long nalloc)
{
    psArray *array = arrayAlloc(file, lineno, func, nalloc);
    if (!array) {
        return NULL;
    }
    array->n = 0;
    return array;
}

psArray* p_psArrayRealloc(const char *file,
                          unsigned int lineno,
                          const char *func,
                          psArray* in,
                          long nalloc)
{
    PS_ASSERT_ARRAY_NON_NULL(in, NULL);
    if (nalloc < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't reallocate a psArray to negative size.");
        return in;
    }

    if (in->nalloc != nalloc) {     // No need to realloc to same size
        if (nalloc < in->n) {
            for (long i = nalloc; i < in->n; i++) {      // For reduction in vector size
                psFree(in->data[i]);
            }
            in->n = nalloc;
        }
        // Realloc after decrementation to avoid accessing freed array elements
        long n = in->n;
        in->data = p_psRealloc(file, lineno, func, in->data, nalloc * sizeof(psPtr));
        P_PSARRAY_SET_NALLOC(in,nalloc);
        for (long m = n; m < nalloc; m++) { //if array is grown, set grown data to NULL
            in->data[m] = NULL;
        }
    }

    return in;
}

psArray* p_psArrayAdd(const char *file, unsigned int lineno, const char *func,
                      psArray* array, long delta, psPtr data)
{
    if (array == NULL) {
        long d = (delta > 0) ? delta : DEFAULT_ARRAY_ADD;
        array = p_psArrayAlloc(file, lineno, func, d);
        array->n = 0;
    }

    int n = array->n;

    if (n >= array->nalloc) {
        // array needs to be expanded to make room for more elements
        long d = (delta > 0) ? delta : DEFAULT_ARRAY_ADD;
        array = p_psArrayRealloc(file, lineno, func, array, n+d);
    }

    // add the element to the end of the array.
    array->data[n] = psMemIncrRefCounter(data);
    array->n = n+1;

    return array;
}

// drop an item from the array and free it
bool psArrayRemoveData(psArray* array,
                       psPtr data)
{
    PS_ASSERT_ARRAY_NON_NULL(array, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    bool success = false;
    long n = array->n;
    psPtr *arrayData = array->data;
    for (long i = n-1; i >= 0; i--) {
        if (arrayData[i] == data) {
            memmove(&arrayData[i],&arrayData[i+1],(n-i-1)*sizeof(psPtr));
            psFree(data); // Free the removed item
            n--;
            success = true;
        }
    }
    array->n = n; // reset the array size to indicate the removed item(s)

    return success;
}

// drop an item from the array and do not free it: this
// can be useful in the free function of a data type
// with a reference on another structure
bool psArrayRemoveDataNoFree(psArray* array,
                             const psPtr data)
{
    PS_ASSERT_ARRAY_NON_NULL(array, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    bool success = false;
    long n = array->n;
    psPtr *arrayData = array->data;
    for (long i = n-1; i >= 0; i--) {
        if (arrayData[i] == data) {
            memmove(&arrayData[i],&arrayData[i+1],(n-i-1)*sizeof(psPtr));
            n--;
            success = true;
        }
    }
    array->n = n; // reset the array size to indicate the removed item(s)

    return success;
}

bool psArrayRemoveIndex(psArray* array,
                        long index)
{
    PS_ASSERT_ARRAY_NON_NULL(array, false);
    if (index < 0 || index >= array->n) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified index outside the range of elements in the array."));
        return false;
    }

    long i = index;
    long n = array->n;
    psFree(array->data[i]);
    memmove(&array->data[i], &array->data[i + 1], (n - i - 1) * sizeof(psPtr));
    array->n--;    // reset the array size to indicate the removed item

    return true;
}

void psArrayElementsFree(psArray* array)
{
    if (array == NULL) {
        return;
    }

    for (long i = 0; i < array->n; i++) {
        psFree(array->data[i]);
        array->data[i] = NULL;
    }

    array->n = 0;
}

// Comparison and swap functions for sorting index into array
#define PSARRAY_SORT_COMPARE_INDEX(A,B) (func(array[index[A]], array[index[B]]) < 0)
#define PSARRAY_SORT_SWAP_INDEX(TYPE,A,B) { \
    if (A != B) { \
        ps##TYPE temp = index[A]; \
        index[A] = index[B]; \
        index[B] = temp; \
    } \
}

// Heap sort of the index array
psVector *psArraySortIndex (psVector *out, psArray *in, psCompareFunc func) {

    if (in == NULL) {
        return NULL;
    }

    out = psVectorCreate(out, 0, in->n, 1, PS_TYPE_S32);
    psS32 *index = out->data.S32;       // Dereference index vector
    psPtr *array = in->data;            // Dereference input array
    PSSORT(out->n, PSARRAY_SORT_COMPARE_INDEX, PSARRAY_SORT_SWAP_INDEX, S32);
    return out;
}

psArray* psArraySort(psArray* array,
                     psComparePtrFunc func)
{
    PS_ASSERT_ARRAY_NON_NULL(array, NULL);
    qsort(array->data, array->n, sizeof(psPtr), (int (*)(const void* , const void*))func);
    return array;
}

// Set an element in the array.
bool psArraySet(psArray* array,                      ///< input array to set element in
                long position,                      ///< the element position to set
                psPtr data)                        ///< the value to set it to
{
    PS_ASSERT_ARRAY_NON_NULL(array, false);

    if (position > array->n)
    {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Specified position, %ld, is greater than n+1 of the array, %ld.",
                position, array->n);
        return false;
    }

    if (position < 0) {
        position += array->n;
    }
    if (position < 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Negative number too large\n");
        return false;
    }

    if (position == array->n) {
        if (position >= array->nalloc) {
            psError(PS_ERR_BAD_PARAMETER_NULL, true,
                    _("Specified position, %ld, is greater than the allocated size of the array, %ld."),
                    position, array->nalloc);
            return false;
        }
        array->n++;
    }
    psFree(array->data[position]);
    array->data[position] = psMemIncrRefCounter(data);

    return true;
}

// Get an element in the array.
psPtr psArrayGet(const psArray* array,
                 long position )
{
    PS_ASSERT_ARRAY_NON_NULL(array, NULL);

    if (position >= array->n) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Specified position, %ld, is greater than n+1 of the array, %ld.",
                position, array->n);
        return NULL;
    }
    if (position < 0)
        position += array->n;
    if (position < 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Negative number too large\n");
        return NULL;
    }
    return (array->data[position]);
}

long psArrayLength(const psArray *array)
{
    PS_ASSERT_ARRAY_NON_NULL(array, -1);
    return (array->n);
}


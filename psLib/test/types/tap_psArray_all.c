/**
 *  C Implementation: tap_psArray_creating
 *
 * Description:  Tests for psArrayAlloc, psArrayRealloc, psMemCheckArray,
 *               psArrayElementsFree, psArrayLength
 *
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"


int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(31);

    // testArrayAllocs()
    {
        psMemId id = psMemGetId();
        psArray *a = NULL;

        //Tests for psArrayAlloc
        //Return NULL on attempting to allocate a negative size array
        {
            a = psArrayAlloc(-2);
            ok( a == NULL,
                "psArrayAlloc:           return NULL for negative array-size input.");
        }
        //Return NULL for negative array size - psArrayAllocEmpty
        {
            a = psArrayAllocEmpty(-2);
            ok( a == NULL,
                "psArrayAllocEmpty:      return NULL for negative array-size input.");
        }
        //Return properly allocated psArray
        {
            a = psArrayAlloc(0);
            ok( a != NULL && psMemCheckArray(a),
                "psArrayAlloc:           return properly allocated psArray.");
        }

        //Tests for psArrayRealloc
        //Now try to reallocate the psArray - bigger
        {
            a = psArrayRealloc(a, 2);
            ok ( psArrayLength(a) == 0 && a->nalloc == 2,
                 "psArrayRealloc:         return properly reallocated psArray.");
        }
        //Return NULL when trying to reallocate a NULL psArray
        {
            psArray *temp = NULL;
            temp = psArrayRealloc(temp, 1);
            ok( temp == NULL,
                "psArrayRealloc:         return NULL for NULL input psArray.");
        }
        //Attempt to reallocate the psArray - smaller
        {
            psS32 *s32 = (psS32*)psAlloc(sizeof(psS32));
            *s32 = 1;
            psS32 *s32_2 = (psS32*)psAlloc(sizeof(psS32));
            *s32_2 = 2;
            skip_start(  !psArraySet(a, 0, s32) || !psArraySet(a, 1, s32), 1,
                         "Skipping 1 tests because psArraySet failed");
            a = psArrayRealloc(a, 1);
            *s32_2 = *((psS32*)(a->data[0]));
            ok( a->n == 1 && a->nalloc == 1 && *s32_2 == 1,
                "psArrayRealloc:         return properly reallocated psArray.");
            skip_end();
            psFree(s32);
            psFree(s32_2);
        }
        //Attempt to reallocate the psArray to negative size
        {
            a = psArrayRealloc(a, -1);
            psS32 *s32 = (psS32*)psAlloc(sizeof(psS32));
            *s32 = 2;
            *s32 = *((psS32*)(a->data[0]));
            ok( a->n == 1 && a->nalloc == 1 && *s32 == 1,
                "psArrayRealloc:         return same psArray for negative-size input.");
            psFree(s32);
        }

        //Attempt to free a NULL psArray
        psArrayElementsFree(NULL);
        //Check the length of a NULL array
        {
            psArray *emptyArray = NULL;
            ok( psArrayLength(emptyArray) == -1,
                "psArrayLength:          return -1 for NULL input psArray.");
        }
        psFree(a);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testArrayAddRemove()
    {
        psMemId id = psMemGetId();
        psArray *a = psArrayAlloc(0);
        psS32 *s1 = (psS32*)psAlloc(sizeof(psS32));
        *s1 = 2;
        psS32 *s2 = (psS32*)psAlloc(sizeof(psS32));
        psS32 *s3 = (psS32*)psAlloc(sizeof(psS32));

        //Tests for psArrayAdd
        //Attempt to add element to NULL psArray.
        {
            psArray *temp = NULL;
            temp = psArrayAdd(temp, 1, (psPtr)s1);
            *s3 = *((psS32*)(temp->data[0]));
            ok( temp->n == 1 && temp->nalloc == 1 && *s3 == 2,
                "psArrayAdd:             return array with newly added data.");
            psFree(temp);
        }
        //Attempt to add an element to empty array.
        {
            a = psArrayAdd(a, 1, (psPtr)s1);
            *s2 = 666;
            *s3 = 212;
            a = psArrayAdd(a, 1, s2);
            a = psArrayAdd(a, 1, s3);
            *s2 = *((psS32*)(a->data[0]));
            ok ( a->n == 3 && a->nalloc == 3 && *s2 == 2,
                 "psArrayAdd:            return array with newly added data.");
        }

        //Tests for psArrayRemoveData
        //Setup array with 3 elements to test remove function
        //Return false for attempting to remove from a NULL array
        {
            psArray *temp = NULL;
            ok( !psArrayRemoveData(temp, s2),
                "psArrayRemoveData:     return false for NULL input psArray.");
        }
        //Return true for successful removal
        {
            ok( psArrayRemoveData(a, s2) && a->n == 2,
                "psArrayRemoveData:     return true for successful removal.");
        }

        //Tests for psArrayRemoveIndex
        //Return false for NULL array input
        {
            ok( !psArrayRemoveIndex(NULL, 0),
                "psArrayRemoveIndex:    return false for NULL input psArray.");
        }
        //Return false for out-of-range index
        {
            ok( !psArrayRemoveIndex(a, 5),
                "psArrayRemoveIndex:    return false for out-of-range location.");
        }
        //Return true for successful removal
        {
            ok( psArrayRemoveIndex(a, 1) && a->n == 1,
                "psArrayRemoveIndex:    return true for successful removal.");
        }

        psFree(s1);
        psFree(s2);
        psFree(s3);
        psFree(a);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testArraySetGet()
    {
        psMemId id = psMemGetId();
        psArray *a = NULL;
        psArray *b = NULL;
        psS32 *s1 = (psS32*)psAlloc(sizeof(psS32));
        *s1 = 2;
        psS32 *s2 = (psS32*)psAlloc(sizeof(psS32));
        *s2 = 1;
        psS32 *s3 = (psS32*)psAlloc(sizeof(psS32));
        *s3 = 3;

        //Tests for psArraySet
        //Return false for trying to set a NULL psArray
        {
            ok( !psArraySet(a, 0, (psPtr)s1),
                "psArraySet:            return false for NULL input psArray.");
        }
        //Return false for trying to set an invalid position
        a = psArrayAlloc(1);
        {
            ok( !psArraySet(a, 2, (psPtr)s1),
                "psArraySet:            return false for invalid input position.");
        }
        //Return false for trying to set nalloc position
        psArraySet(a, 0, (psPtr)s1);
        {
            ok( !psArraySet(a, a->nalloc, (psPtr)s2),
                "psArraySet:            return false for out-of-range position.");
        }
        //Return false for an invalid position (-2 = out-of-range neg. index)
        {
            ok( !psArraySet(a, -2, (psPtr)s2),
                "psArraySet:            return false for out-of-range negative position.");
        }
        //Return true for set to a->n position (< nalloc)
        a = psArrayRealloc(a, 10);
        {
            ok( psArraySet(a, a->n, (psPtr)s2)  && a->n == 2,
                "psArraySet:            return true for valid input position.");
        }
        //Return true for a negative index input
        {
            ok( psArraySet(a, -2, (psPtr)s3) && a->n == 2 &&
                *((psS32*)(a->data[0])) == 3,
                "psArraySet:            return true for valid negative input position.");
        }

        //Tests for psArrayGet
        //Return NULL for NULL array input
        psS32 *s4 = NULL;
        {
            s4 = (psS32*)psArrayGet(b, 0);
            ok( s4 == NULL,
                "psArrayGet:            return NULL for NULL input psArray.");
        }
        //Return NULL for an out-of-range index
        {
            s4 = (psS32*)psArrayGet(a, a->n);
            ok( s4 == NULL,
                "psArrayGet:            return NULL for out-of-range position.");
        }
        //Return NULL for an out-of-range negative index
        {
            s4 = (psS32*)psArrayGet(a, -1-a->n);
            ok( s4 == NULL,
                "psArrayGet:            return NULL for out-of-range negative position.");
        }
        //Return valid case
        {
            s4 = (psS32*)psArrayGet(a, 1);
            ok( *s4 == 1,
                "psArrayGet:            return correct value for valid position.");
        }

        psFree(a);
        psFree(s1);
        psFree(s2);
        psFree(s3);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testArraySort()
    {
        psMemId id = psMemGetId();
        psArray *a = NULL;
        a = psArrayAlloc(3);
        a->n = 0;
        psS32 *s1 = (psS32*)psAlloc(sizeof(psS32));
        *s1 = 2;
        psS32 *s2 = (psS32*)psAlloc(sizeof(psS32));
        *s2 = 1;
        psS32 *s3 = (psS32*)psAlloc(sizeof(psS32));
        *s3 = 3;
        psArraySet(a, a->n, (psPtr)s1);
        psArraySet(a, a->n, (psPtr)s2);
        psArraySet(a, a->n, (psPtr)s3);
        psS32 *s4 = (psS32*)psAlloc(sizeof(psS32));
        psS32 *s5 = (psS32*)psAlloc(sizeof(psS32));
        psS32 *s6 = (psS32*)psAlloc(sizeof(psS32));

        //Tests for psArraySort
        //Return NULL for attempt to sort NULL psArray input.
        {
            psArray *temp = NULL;
            temp = psArraySort(temp, (psComparePtrFunc)psCompareDescendingS32Ptr);
            ok( temp == NULL,
                "psArraySort:           return NULL for NULL input psArray.");
        }
        //Return properly sorted psArray with descending psS32 elements - 3,2,1
        {
            a = psArraySort(a, (psComparePtrFunc)psCompareDescendingS32Ptr);
            *s4 = *((psS32*)(a->data[0]));
            *s5 = *((psS32*)(a->data[1]));
            *s6 = *((psS32*)(a->data[2]));
            ok( *s4 == 3 && *s5 == 2 && *s6 == 1,
                "psArraySort:           return properly sorted psArray.");
        }

        psFree(s1);
        psFree(s2);
        psFree(s3);
        psFree(s4);
        psFree(s5);
        psFree(s6);
        psFree(a);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}


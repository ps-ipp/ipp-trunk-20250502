/**
 *  C Implementation: tap_psList_all
 *
 * Description:  Tests for psListAlloc, psListAdd, psMemCheckList, psListAddAfter,
 *               psListAddBefore, psListRemove, psListRemoveData, psListGet,
 *               psListIteratorAlloc, psListIteratorSet, psListGetAndIncrement,
 *               psListGetAndDecrement, psListToArray, psArrayToList, psListSort,
 *               psListLength
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(61);

    // Test 1:  psList Creation Fxns");
    // testListCreate(void)
    {
        psMemId id = psMemGetId();
        psList *noList = NULL;
        psListIterator *noIter = NULL;
        psList *list = psListAlloc(NULL);
        psMetadata *md = psMetadataAlloc();
        psListIterator *iter1 = NULL;
        psListIterator *iter2 = NULL;

        //Tests for psListAlloc & psMemCheckList
        //Return empty list for NULL data input.
        {
            noList = psListAlloc(NULL);
            ok( psListLength(noList) == 0 && psMemCheckList(noList),
                "psListAlloc:             return empty list for NULL data input.");
        }
        //Return allocated list for non-NULL data input.
        psFree(noList);
        {
            noList = psListAlloc(md);
            ok( psListLength(noList) == 1,
            "psListAlloc:             return correct list for non-NULL data input.");
        }
        //Make sure psMemCheckList works correctly - return false
        if (0) {
            int j = 2;
            ok( !psMemCheckList(&j),
                "psMemCheckList:          return false for non-List input.");
        }

        //Tests for ListIteratorAlloc
        //Return NULL for attempt to allocate iterator with NULL list input
        {
            noIter = psListIteratorAlloc(NULL, 0, true);
            ok( noIter == NULL,
                "psListIteratorAlloc:     return false for NULL data input.");
        }
        //Attempt to Allocate a psListIterator with out-of-range location
        {
            iter1 = psListIteratorAlloc(noList, 10, false);
            ok( iter1 == NULL,
                "psListIteratorAlloc:     return NULL for out-of-range location.");
        }
        //Return valid iterator for valid inputs
        {
            iter1 = psListIteratorAlloc(noList, 0, true);
            ok( iter1 != NULL,
                "psListIteratorAlloc:     return valid iterator for valid inputs.");
        }

        //Tests For ListAdd Fxns
        //Return false for NULL list input
        {
            ok( !psListAdd(NULL, 0, md),
                "psListAdd:               return false for NULL list input.");
        }
        //Return false for NULL data input
        {
            ok( !psListAdd(list, 0, NULL),
                "psListAdd:               return false for NULL data input.");
        }
        //Return false for list with NULL data
        {
            psFree(list->iterators->data[0]);
            list->iterators->data[0] = NULL;
            ok( !psListAdd(list, 0, md),
                "psListAdd:               return false for list with NULL iterators.");
            psFree(list);
            list = psListAlloc(md);
        }
        //Return true for valid inputs - tail location
        {
            ok( psListAdd(list, 2, md),
                "psListAdd:              return true for valid inputs - Tail Location.");
        }
        //Return true for valid inputs - head location
        {
            ok( psListAdd(list, 0, md),
                "psListAdd:              return true for valid inputs - head Location.");
        }

        //psListAddAfter Tests
        //psListAddAfter - Return false for NULL data input
        {
            ok( !psListAddAfter(iter1, NULL),
                "psListAddAfter:         return false for NULL data input.");
        }
        //psListAddAfter - Return false for NULL iterator input
        {
            ok( !psListAddAfter(NULL, md),
                "psListAddAfter:         return false for NULL iterator input.");
        }
        //psListAddAfter - Return false for non-mutable iterator input
        {
            iter1->mutable = false;
            ok( !psListAddAfter(iter1, md),
                "psListAddAfter:         return false for non-mutable iterator input.");
            iter1->mutable = true;
            psFree(iter1);
        }
        //psListAddAfter - Return false for iterator with NULL cursor
        psList *newList = psListAlloc(md);
        iter1 = psListIteratorAlloc(newList, 0, true);
        psListElem *cursor = iter1->cursor;
        {
            iter1->cursor = NULL;
            ok( !psListAddAfter(iter1, md),
                "psListAddAfter:         return false for iterator with headptr but no cursor.");
            iter1->cursor = cursor;
        }
        //Set the iterator to a middle element, then add after
        {
            iter2 = psListIteratorAlloc(list, 1, true);
            ok( psListAddAfter(iter2, md),
                "psListAddAfter:         return true for adding a list element to the middle.");
        }
        //Set the iterator to the head element, then add after a wrong index.  return true
        {
            if (!psListIteratorSet(iter2, PS_LIST_HEAD) )
                printf("\nerror in set\n\n");
            ((psListIterator*)(iter2->list->iterators->data[0]))->index = 10;
            ok( psListAddAfter(iter2, md),
                "psListAddAfter:         return true for adding a list element to the head.");
        }
        psFree(noList);

        //Return true for adding to an empty list
        noList = psListAlloc(NULL);
        psListIterator *iter3 = psListIteratorAlloc(noList, 0, true);
        {
            ok( psListAddAfter(iter3, md), "psListAddAfter: return true for adding to an empty list.");
        }

        //psListAddBefore Tests
        //psListAddBefore - Return false for NULL data input
        {
            ok( !psListAddBefore(iter1, NULL),
                "psListAddBefore:        return false for NULL data input.");
        }
        //psListAddBefore - Return false for NULL iterator input
        {
            ok( !psListAddBefore(NULL, md),
                "psListAddBefore:        return false for NULL iterator input.");
        }
        //psListAddBefore - Return false for non-mutable iterator input
        {
            iter1->mutable = false;
            ok( !psListAddBefore(iter1, md),
                "psListAddBefore:        return false for non-mutable iterator input.");
            iter1->mutable = true;
        }
        {
            cursor = iter1->cursor;
            iter1->cursor = NULL;
            ok( !psListAddBefore(iter1, md),
                "psListAddBefore:        return false for iterator with headptr but no cursor.");
            iter1->cursor = cursor;
        }
        //Add before a middle list element
        {
            psListIteratorSet(iter2, -2);
            ok( psListAddBefore(iter2, md),
                "psListAddBefore:        return true for adding a list element to the middle.");
        }
        //Set the iterator to the 2nd element, then add after a wrong index.  return true
        {
            skip_start(  !psListIteratorSet(iter2, 1), 1,
                         "Skipping 1 tests because psListIteratorSet failed");
            ((psListIterator*)(iter2->list->iterators->data[0]))->index = 10;
            ok( psListAddBefore(iter2, md),
                "psListAddBefore:        return true for adding a list element to the head.");
            skip_end();
        }
        //Return true for adding to an empty list
        {
            psFree(noList);
            noList = psListAlloc(NULL);
            iter3 = psListIteratorAlloc(noList, 0, true);
            psFree(iter3);
            ok( psListAddBefore(iter3, md),
                "psListAddBefore:        return true for adding to an empty list.");
        }
    
        //Check the length of a NULL list
        {
            psList *emptyList = NULL;
            ok( psListLength(emptyList) == -1,
                "psListLength:           return -1 for NULL input psList.");
        }
    
        psFree(iter2);
        psFree(list);
        psFree(iter1);
        psFree(newList);
        psFree(md);
        psFree(noList);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testListManip()
    // psList Manipulation Fxns");
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psList *emptyList = psListAlloc(NULL);
        psList *list = psListAlloc(md);
        psSphere *sphere = psSphereAlloc();
        psCube *cube = psCubeAlloc();
        psArray *array = psArrayAlloc(0);
        psListAdd(list, PS_LIST_TAIL, sphere);
        psListAdd(list, PS_LIST_TAIL, cube);
        psListAdd(list, PS_LIST_TAIL, array);
        psListIterator *emptyIter = psListIteratorAlloc(emptyList, 0, true);
        psListIterator *iter1 = psListIteratorAlloc(list, PS_LIST_TAIL, true);
    
        //Tests for psListGet
        //Return NULL for NULL list input
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGet(NULL, PS_LIST_HEAD);
            ok( out == NULL,
                "psListGet:              return NULL for NULL list input.");
        }
        //Return NULL for empty list
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGet(emptyList, PS_LIST_HEAD);
            ok( out == NULL,
                "psListGet:              return NULL for empty list.");
        }
        //Return NULL for out-of-range location
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGet(list, -10);
            ok( out == NULL,
                "psListGet:              return NULL for out-of-range location.");
        }
        //Return correct head item for valid inputs
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGet(list, PS_LIST_HEAD);
            ok( out != NULL && psMemCheckMetadata(out),
                "psListGet:              return correct head item for valid inputs.");
        }
    
        //Tests for psListGetAndIncrement
        //Return NULL for NULL iterator input
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGetAndIncrement(NULL);
            ok( out == NULL,
                "psListGetAndIncrement:  return NULL for NULL iterator input.");
        }
        //Return NULL for empty iterator with offend = false
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGetAndIncrement(emptyIter);
            ok( out == NULL,
                "psListGetAndIncrement:  return NULL for empty iterator input.");
        }
        //Return NULL for empty iterator with offend = true
        {
            emptyIter->offEnd = true;
            psMetadata *out = NULL;
            out = (psMetadata*)psListGetAndIncrement(emptyIter);
            ok( out == NULL,
                "psListGetAndIncrement:  return NULL for empty iterator input.");
            emptyIter->offEnd = false;
        }
        //Return correct tail item for valid inputs
        {
            psArray *out = NULL;
            out = (psArray*)psListGetAndIncrement(iter1);
            ok( out != NULL && psMemCheckArray(out),
                "psListGetAndIncrement:  return correct tail item for valid inputs.");
        }
    
        //Tests for psListGetAndDecrement
        //Return NULL for NULL iterator input
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGetAndDecrement(NULL);
            ok( out == NULL,
                "psListGetAndDecrement:  return NULL for NULL iterator input.");
        }
        //Return NULL for empty iterator with offend = false
        {
            psMetadata *out = NULL;
            out = (psMetadata*)psListGetAndDecrement(emptyIter);
            ok( out == NULL,
                "psListGetAndDecrement:  return NULL for empty iterator input.");
        }
        //Return NULL for empty iterator with offend = true
        {
            emptyIter->offEnd = true;
            psMetadata *out = NULL;
            out = (psMetadata*)psListGetAndDecrement(emptyIter);
            ok( out == NULL,
                "psListGetAndDecrement:  return NULL for empty iterator input.");
            emptyIter->offEnd = false;
        }
        //Return correct tail item for valid inputs
        {
            psArray *out = NULL;
            skip_start(  !psListIteratorSet(iter1, PS_LIST_TAIL), 1,
                         "Skipping 1 tests because psListIteratorSet failed");
            out = (psArray*)psListGetAndDecrement(iter1);
            ok( out != NULL && psMemCheckArray(out),
                "psListGetAndDecrement:  return correct tail item for valid inputs.");
            skip_end();
        }
    
        //Tests for psListRemove
        //Return false for NULL list input
        {
            ok( !psListRemove(NULL, PS_LIST_HEAD),
                "psListRemove:           return false for NULL list input.");
        }
        //Return false for invalid location
        {
            ok( !psListRemove(list, -10),
                "psListRemove:           return false for invalid location.");
        }
        //Return false for empty list
        {
            ok( !psListRemove(emptyList, PS_LIST_HEAD),
                "psListRemove:           return false for remove from empty list.");
        }
        //Return true for remove from middle of list
        {
            ok( psListRemove(list, 2) && list->n == 3,
                "psListRemove:           return true for remove from middle of list.");
        }
        //Return true for remove from head of list
        {
            ok( psListRemove(list, PS_LIST_HEAD) && list->n ==2,
                "psListRemove:           return true for remove from head of list.");
        }
        //Return true for remove from tail of list
        {
            ok( psListRemove(list, PS_LIST_TAIL) && list->n == 1,
                "psListRemove:           return true for remove from tail of list.");
        }
        //Tests for psListRemoveData
        //Return false for NULL list input
        {
            ok( !psListRemoveData(NULL, md),
                "psListRemoveData:       return false for NULL list input.");
        }
        //Return false for NULL data input
        {
            ok( !psListRemoveData(list, NULL),
                "psListRemoveData:       return false for NULL data input.");
        }
        //Return false for non-matching data
        {
            ok( !psListRemoveData(list, md),
                "psListRemoveData:       return false for non-matching data.");
        }
        //Return false for trying to remove from an empty list
        {
            ok( !psListRemoveData(emptyList, md),
                "psListRemoveData:       return false for trying to remove from an empty list.");
        }
        //Return true for remove of valid data
        psListAdd(list, PS_LIST_HEAD, cube);
        psListAdd(list, PS_LIST_HEAD, array);
        {
            ok( psListRemoveData(list, sphere),
                "psListRemoveData:       return true for remove of valid data.");
        }
    
        psFree(md);
        psFree(emptyList);
        psFree(list);
        psFree(sphere);
        psFree(cube);
        psFree(array);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testListConvertSort()
    // psList Conversion and Sorting Fxns");
    {
        psMemId id = psMemGetId();
        psList *emptyList = psListAlloc(NULL);
        psS32 *s1 = (psS32*)psAlloc(sizeof(psS32));
        *s1 = 2;
        psS32 *s2 = (psS32*)psAlloc(sizeof(psS32));
        *s2 = 3;
        psS32 *s3 = (psS32*)psAlloc(sizeof(psS32));
        *s3 = 1;
        psList *list = psListAlloc(s1);
        psListAdd(list, PS_LIST_TAIL, s2);
        psListAdd(list, PS_LIST_TAIL, s3);
        psArray *array = NULL;
        psArray *emptyArray = psArrayAlloc(0);
        psList *out = NULL;
    
        //Tests for psListToArray
        //Return NULL for NULL list input
        {
            array = psListToArray(NULL);
            ok( array == NULL,
                "psListToArray:          return NULL for NULL list input.");
        }
        //Return empty array for empty list input
        {
            array = psListToArray(emptyList);
            ok( array->n == 0 && psMemCheckArray(array),
                "psListToArray:          return empty array for empty list input.");
            psFree(array);
            array = NULL;
        }
        //Return correct array for valid list input
        {
            array = psListToArray(list);
            ok( array->n == 3 && *((psS32*)(array->data[0])) == 2,
                "psListToArray:          return correct array for valid list input.");
        }
    
        //Tests for psArrayToList
        //Return NULL for NULL array input
        {
            out = psArrayToList(NULL);
            ok( out == NULL,
                "psArrayToList:          return NULL for NULL array input.");
        }
        //Return empty list for empty array input
        {
            out = psArrayToList(emptyArray);
            ok( out->n == 0 && psMemCheckList(out),
                "psArrayToList:          return empty list for empty array input.");
            psFree(out);
            out = NULL;
        }
        //Return correct list for valid array input
        {
            out = psArrayToList(array);
            ok( out->n == 3 && *((psS32*)psListGet(out, PS_LIST_HEAD)) == 2,
                "psArrayToList:          return correct list for valid array input.");
        }
    
        //Tests for psListSort
        //Return NULL for NULL list input
        {
            psList *none = NULL;
            none = psListSort(none, (psComparePtrFunc)psCompareDescendingS32Ptr);
            ok( none == NULL,
                "psListSort:             return NULL for NULL list input.");
        }
        //Return empty list for empty list input
        {
            emptyList = psListSort(emptyList, (psComparePtrFunc)psCompareDescendingS32Ptr);
            ok( emptyList != NULL,
                "psListSort:             return empty list for empty list input.");
        }
        //Return properly sorted list for valid inputs
        {
            list = psListSort(list, (psComparePtrFunc)psCompareS32Ptr);
            ok( *((psS32*)psListGet(list, 0)) == 1 &&  *((psS32*)psListGet(list, 1)) == 2,
                "psListSort:             return properly sorted list for valid inputs.");
        }
        //Return unchanged list for NULL psComparePtrFunc
        {
            psList *tempList = NULL;
            tempList = psListSort(list, NULL);
            ok( tempList == NULL,
                "psListSort:             return NULL for NULL psComparePtrFunc.");
        }
    
        psFree(array);
        psFree(out);
        psFree(emptyArray);
        psFree(emptyList);
        psFree(list);
        psFree(s1);
        psFree(s2);
        psFree(s3);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

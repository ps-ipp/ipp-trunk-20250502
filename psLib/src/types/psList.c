/** @file psList.c
 *  @brief Support for doubly linked lists
 *  @ingroup LinkedList
 *
 *  @author Robert Lupton, Princeton University
 *  @author Robert Daniel DeSonia, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.71 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-08 18:06:27 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "psError.h"
#include "psAbort.h"
#include "psMemory.h"
#include "psList.h"
#include "psTrace.h"
#include "psLogMsg.h"
#include "psAssert.h"


#define ITER_INIT_HEAD ((psPtr )1)         // next iteration should return head
#define ITER_INIT_TAIL ((psPtr )2)         // next iteration should return tail

#define INITIAL_NUM_ITERATORS 16        // Initial number of iterators to create

// private functions.
static void listFree(psList* list);
static void listIteratorFree(psListIterator* iter);
static bool listIteratorRemove(psListIterator* iterator);

static void listFree(psList* list)
{
    if (list->iterators != NULL) {

        // remove the associated iterators -- any references are invalid once psList is freed.
        psArray* iterators = list->iterators;
        // ONLY orphan the iterators if the list iterators are about to be destroyed
        // a case where this is not the case is in psListSort.
        if (psMemGetRefCounter(iterators)  < 2) {
            for (int i = 0; i < iterators->n; i++) {
                // orphan iterators first to avoid any callbacks to dying list
                ((psListIterator*)iterators->data[i])->list = NULL;
            }
        }
        psFree(iterators);
    }

    for (psListElem* ptr = list->head; ptr != NULL;) {
        psListElem* next = ptr->next;

        psFree(ptr->data);
        psFree(ptr);

        ptr = next;
    }
}

static void listIteratorFree(psListIterator* iter)
{
    // remove this iterator from the parent list
    if (iter->list != NULL) {
        psArray* iters = iter->list->iterators;
        for (int lcv = 0; lcv < iters->n; lcv++) {
            if (iters->data[lcv] == iter) {
                // following is done to match SDRS.
                iters->data[lcv] = iters->data[iters->n-1];
                iters->n--;
                break;
            }
        }
    }
}

static bool listIteratorRemove(psListIterator* iterator)
{
    psAssert(iterator, "impossible");
    if (iterator->cursor == NULL) {
        return false;
    }

    psListElem* elem = iterator->cursor;
    psList* list = iterator->list;
    int index = iterator->index;

    if (elem == list->head) {        // head of list?
        list->head = elem->next;
    } else {
        elem->prev->next = elem->next;
    }

    if (elem == list->tail) {        // tail of list?
        list->tail = elem->prev;
    } else {
        elem->next->prev = elem->prev;
    }

    psArray* iterators = list->iterators;
    for (int i = 0; i < iterators->n; i++) {
        psListIterator* iter = (psListIterator*) iterators->data[i];
        if (iter->cursor == elem) {
            iter->cursor = NULL;
        } else if (iter->index > index && iter->index > 0) {
            iter->index--;
        }
    }

    list->n--;

    // OK, delete orphaned list element and its data
    psFree(elem->data);
    psFree(elem);

    return true;
}

psList* p_psListAlloc(const char *file,
                    unsigned int lineno,
                    const char *func,
                    psPtr data)
{
    psList* list = p_psAlloc(file, lineno, func, sizeof(psList));

    psMemSetDeallocator(list, (psFreeFunc) listFree);

    list->n = 0;
    list->head = list->tail = NULL;
    list->iterators = psArrayAllocEmpty(INITIAL_NUM_ITERATORS);

    // create a default iterator
    psListIteratorAlloc(list,PS_LIST_HEAD,true);

    if (data != NULL) {
        psListAdd(list, PS_LIST_TAIL, data);
    }

    return list;
}

bool psMemCheckList(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)listFree );
}

psListIterator* p_psListIteratorAlloc(const char *file,
                                    unsigned int lineno,
                                    const char *func,
                                    psList* list,
                                    long location,
                                    bool mutable)
{
    PS_ASSERT_LIST_NON_NULL(list, NULL);

    psListIterator* iter = p_psAlloc(file, lineno, func, sizeof(psListIterator));
    psMemSetDeallocator(iter, (psFreeFunc) listIteratorFree);

    // initialize the attributes
    iter->list = list;
    iter->cursor = NULL;
    iter->index = 0;
    iter->offEnd = false;
    iter->mutable = mutable;

    // associate the iterator with the list
    list->iterators = psArrayAdd(list->iterators,0,iter);
    // don't want the list's array of iterators to hold a true reference
    psMemDecrRefCounter(iter);

    if (!psListIteratorSet(iter,location)) {
        psFree(iter);
        return NULL;
    }

    return iter;
}

bool psListIteratorSet(psListIterator* iterator,
                       long location)
{
    PS_ASSERT_LIST_ITERATOR_NON_NULL(iterator, false);

    psList* list = iterator->list;

    if (location == PS_LIST_TAIL) {
        iterator->cursor = list->tail;
        iterator->index = list->n - 1;
        iterator->offEnd = false;
        return true;
    }

    if (location == PS_LIST_HEAD) {
        iterator->cursor = list->head;
        iterator->index = 0;
        iterator->offEnd = false;
        return true;
    }

    if (location < 0) {
        location = list->n + location;
    }

    // XXX remove this as an error
    if (location < 0 || location >= (int)list->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified location, %ld, is invalid."),
                location);
        return false;
    }

    psListElem* cursor = iterator->cursor;
    int index = iterator->index;
    if (cursor == NULL) {      // set the cursor to the head if it is NULL
        //XXX: if location can't be >= n, it def. can't be greater than n/2.
        /*        if (location > list->n/2) { // closer to tail or head?
                    cursor = list->tail;
                    index = list->n - 1;
                } else {
        */
        cursor = list->head;
        index = 0;
        //        }
    }

    if (location < index) {
        psS32 diff = index - location;

        for (psS32 count = 0; count < diff; count++) {
            cursor = cursor->prev; // shouldn't need to check for NULL
        }
    } else {
        psS32 diff = location - index;

        for (psS32 count = 0; count < diff; count++) {
            cursor = cursor->next; // shouldn't need to check for NULL
        }
    }
    iterator->cursor = cursor;
    iterator->index = location;
    iterator->offEnd = false;

    return true;
}

bool psListAdd(psList* list,
               long location,
               psPtr data)
{
    PS_ASSERT_LIST_NON_NULL(list, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    if (location != PS_LIST_HEAD && location >= list->n) {
        psLogMsg(__func__,PS_LOG_DETAIL,
                 "Specified location, %ld, is beyond the end of the list.  "
                 "Adding data item to tail.",
                 location);
        location = PS_LIST_TAIL;
    }

    // move ourselves to the given position
    if (! psListIteratorSet(list->iterators->data[0],location)) {
        return false;
    }

    if (location == PS_LIST_TAIL) {
        // insert the element at the end of the list
        return psListAddAfter(list->iterators->data[0],data);
    } else {
        return psListAddBefore(list->iterators->data[0],data);
    }
}

bool psListAddAfter(psListIterator* iterator,
                    void* data)
{
    PS_ASSERT_LIST_ITERATOR_NON_NULL(iterator, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    // Check if the list pointed by the iterator can be changed
    if (!iterator->mutable) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified iterator indicates list is non-mutable."));
        return false;
    }

    psListElem* cursor = iterator->cursor;
    psList* list = iterator->list;

    if (cursor == NULL && list->head != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified iterator is not valid."));
        return false;
    }

    psListElem* elem = psAlloc(sizeof(psListElem));

    // set the new list element's attributes
    if (cursor == NULL) { // must be an empty list
        elem->prev = NULL;
        elem->next = NULL;
        list->head = elem;
        list->tail = elem;
    } else {
        elem->prev = cursor;
        elem->next = cursor->next;
        cursor->next = elem;
        if (elem->next == NULL) {
            list->tail = elem;
        } else {
            elem->next->prev = elem;
        }
    }

    elem->data = psMemIncrRefCounter(data);

    list->n++;

    psArray* iterators = list->iterators;
    int index = iterator->index;
    for (int i = 0; i < iterators->n; i++) {
        psListIterator* iter = (psListIterator*) iterators->data[i];
        if (iter->index > index) {
            iter->index++;
        }
    }

    return true;
}

bool psListAddBefore(psListIterator* iterator,
                     void* data)
{
    PS_ASSERT_LIST_ITERATOR_NON_NULL(iterator, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    // Check if the list pointed by the iterator can be changed
    if (!iterator->mutable) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified iterator indicates list is non-mutable."));
        return false;
    }

    psListElem* cursor = iterator->cursor;
    psList* list = iterator->list;

    if (cursor == NULL && list->head != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified iterator is not valid."));
        return false;
    }

    psListElem* elem = psAlloc(sizeof(psListElem));

    // set the new list element's attributes
    if (cursor == NULL) { // empty list.
        elem->prev = NULL;
        elem->next = NULL;
        list->head = elem;
        list->tail = elem;
    } else {
        elem->prev = cursor->prev;
        elem->next = cursor;
        cursor->prev = elem;
        if (elem->prev == NULL) {
            list->head = elem;
        } else {
            elem->prev->next = elem;
        }
    }

    elem->data = psMemIncrRefCounter(data);

    list->n++;

    psArray* iterators = list->iterators;
    int index = iterator->index;
    for (int i = 0; i < iterators->n; i++) {
        psListIterator* iter = (psListIterator*) iterators->data[i];
        if (iter->index >= index) {
            iter->index++;
        }
    }

    return true;
}

bool psListRemove(psList* list,
                  long location)
{
    PS_ASSERT_LIST_NON_NULL(list, false);

    // move ourselves to the given position
    psListIterator* defaultIterator = list->iterators->data[0];
    if (! psListIteratorSet(defaultIterator,location)) {
        return false;
    }

    return listIteratorRemove(defaultIterator);
}

bool psListRemoveData(psList* list,
                      psPtr data)
{
    PS_ASSERT_LIST_NON_NULL(list, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    psListElem* elem = list->head;
    int index = 0;
    while (elem != NULL && elem->data != data) {
        elem = elem->next;
        index++;
    }
    if (elem == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified data item is not found in the psList."));
        return false;
    }

    psListIterator* iterator = (psListIterator*)list->iterators->data[0];
    iterator->index = index;
    iterator->cursor = elem;

    return listIteratorRemove(iterator);
}

psPtr psListGet(psList* list,
                long location)
{
    PS_ASSERT_LIST_NON_NULL(list, NULL);

    // XXX this should not be an error, right?
    if (list->head == NULL) { // list empty?
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified psList reference is empty."));
        return NULL;
    }

    psListIterator* iterator = list->iterators->data[0];

    // XXX remove this as an eror
    if (! psListIteratorSet(iterator,location)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified location, %ld, is invalid."),
                location);
        return NULL;
    }

    return iterator->cursor->data;
}

// simultaneous get and remove (ie, 'pop')
psPtr psListGetAndRemove(psList *list, long location) {

    PS_ASSERT_LIST_NON_NULL(list, NULL);

    // empty list :
    // XXX handle this explicitly since psListGet raises an error in this case
    if (list->head == NULL) {
        return NULL;
    }

    psPtr *item = psListGet (list, location); // Item of interest
    if (psMemIncrRefCounter(item)) {    // To prevent psListRemove from killing the item before it gets out
        psListRemove(list, location);
    }

    return item;
}

/*
 * and now return the previous/next element of the list
 */
psPtr psListGetAndIncrement(psListIterator* iterator)
{
    PS_ASSERT_LIST_ITERATOR_NON_NULL(iterator, NULL);

    if (( iterator->cursor == NULL) && (iterator->offEnd)) {
        return NULL;
    }
    if ( (iterator->cursor == NULL) && (!iterator->offEnd)) {
        iterator->cursor = iterator->list->head;
        iterator->index = 0;
        return NULL;
    }

    psPtr data = iterator->cursor->data;

    iterator->cursor = iterator->cursor->next;
    iterator->index++;
    if (iterator->cursor == NULL) {
        iterator->offEnd = true;
    }

    return data;
}

psPtr psListGetAndDecrement(psListIterator* iterator)
{
    PS_ASSERT_LIST_ITERATOR_NON_NULL(iterator, NULL);

    if ((iterator->cursor == NULL) && (!iterator->offEnd))  {
        return NULL;
    }
    if ( (iterator->cursor == NULL) && (iterator->offEnd) ) {
        iterator->cursor = iterator->list->tail;
        iterator->index = iterator->list->n-1;
        iterator->offEnd = false;
        return NULL;
    }

    psPtr data = iterator->cursor->data;

    iterator->cursor = iterator->cursor->prev;
    iterator->index--;

    return data;
}

/*
 * Convert a psList to/from a psVoidPtrArray
 */
psArray* psListToArray(const psList* list)
{
    PS_ASSERT_LIST_NON_NULL(list, NULL);

    long n = list->n;
    psArray *arr = psArrayAlloc(n);

    psListElem *ptr = list->head;
    for (long i = 0; i < n; i++) {
        arr->data[i] = psMemIncrRefCounter(ptr->data);
        ptr = ptr->next;
    }

    return arr;
}

psList* psArrayToList(const psArray* array)
{
    PS_ASSERT_ARRAY_NON_NULL(array, NULL);

    psList *list = psListAlloc(NULL);   // list of elements
    for (long i = 0; i < array->n; i++) {
        psListAdd(list, PS_LIST_TAIL, array->data[i]);
    }
    return list;
}

psList* psListSort(psList* list,
                   psComparePtrFunc func)
{
    PS_ASSERT_LIST_NON_NULL(list, NULL);
    PS_ASSERT_PTR_NON_NULL(func, NULL);

    // convert to indexable vector for use by qsort.
    psArray *arr = psListToArray(list);
    psArray *iterators = psMemIncrRefCounter(list->iterators);
    psFree(list);

    arr = psArraySort(arr, func);

    // convert back to linked list
    list = psArrayToList(arr);
    psFree(list->iterators);
    list->iterators = iterators;
    psFree(arr);

    // Invalidate all iterator positions.
    // Also need to point them to the new list.
    for (int i = 0; i < iterators->n; i++) {
        psListIterator *iterator = iterators->data[i]; // Iterator of interest
        iterator->list = list;
        iterator->cursor = NULL;
    }

    return list;
}

long psListLength(const psList *list)
{
    PS_ASSERT_LIST_NON_NULL(list, -1);
    return list->n;
}



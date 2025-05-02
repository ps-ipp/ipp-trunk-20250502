/** @file psList.h
 *  @brief Support for doubly linked lists
 *
 *  @author Robert Lupton, Princeton University
 *  @author Robert Daniel DeSonia, MHPCC
 *
 *  @version $Revision: 1.49 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-14 03:18:41 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_LIST_H
#define PS_LIST_H

#include "psCompare.h"
#include "psArray.h"
#include "psMutex.h"

/// @addtogroup DataContainer Data Containers
///  @{

/** Special values of index into list
 *
 *  This list of possible list position values should be contiguous
 *  non-positive values ending with PS_LIST_UNKNOWN.  Any value
 *  less-than-or-equal-to PS_LIST_UNKNOWN is considered a undefined position.
 *
 */
enum {
    PS_LIST_HEAD = 0,                  ///< at head
    PS_LIST_TAIL = -1,                 ///< at tail
};


/** Doubly-linked list element */
typedef struct psListElem {
    struct psListElem* prev;           ///< previous link in list
    struct psListElem* next;           ///< next link in list
    psPtr data;                        ///< real data item
} psListElem;


/** The psList Linked list structure.  User should not allocate this struct
 *  directly; rather the psListAlloc should be used.
 *
 *  @see psListAlloc
 */
typedef struct {
    long n;                            ///< number of elements on list
    psListElem* head;                  ///< first element on list (may be NULL)
    psListElem* tail;                  ///< last element on list (may be NULL)
    psArray* iterators;
    ///< array of all iterators associated with this list.  First iterator is
    ///< used internally to improve performance when using indexed access, all
    ///< others are user-level iterators created by psListIteratorAlloc.
    psMutex lock;                       ///< Optional lock for thread safety
} psList;


/** The psList iterator structure.  This should be allocated via
 *  psListIteratorAlloc and not directly.
 *
 *  The life span of a psListIterator object is ended by either a psFree
 *  of this structure OR psFree of the psList in which it operates on.
 *
 *  @see psListIteratorAlloc, psListIteratorSet, psListGetAndIncrement, psListGetAndDecrement
 */
typedef struct
{
    psList* list;                      ///< List iterator to works on
    psListElem* cursor;                ///< current cursor position
    bool offEnd;                       ///< Iterator off the end?
    long index;                         ///< the index number in the list
    bool mutable;                      ///< Is it permissible to modify the list?
}
psListIterator;


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psList structure, false otherwise.
 */
bool psMemCheckList(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Creates a psList linked list object.
 *
 *  @return psList* A new psList object.
 */
#ifdef DOXYGEN
psList* psListAlloc(
    psPtr data          ///< initial data item; may be NULL if an empty psList is desired
);
#else // ifdef DOXYGEN
psList* p_psListAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psPtr data                          ///< initial data item; may be NULL if an empty psList is desired
) PS_ATTR_MALLOC;
#define psListAlloc(data) \
      p_psListAlloc(__FILE__, __LINE__, __func__, data)
#endif // ifdef DOXYGEN


/** Creates a psListIterator object and associates it with a psList.
 *
 *  @return psListIterator* A new psListIterator object.
 */
#ifdef DOXYGEN
psListIterator* psListIteratorAlloc(
    psList* list,                      ///< the psList to iterate with
    long location,                     ///< the initial starting point.
    ///<  This can be a numeric index, PS_LIST_HEAD, or PS_LIST_TAIL.
    bool mutable                       ///< Is it permissible to modify list?
);
#else // ifdef DOXYGEN
psListIterator* p_psListIteratorAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psList* list,                      ///< the psList to iterate with
    long location,                     ///< the initial starting point.
    ///<  This can be a numeric index, PS_LIST_HEAD, or PS_LIST_TAIL.
    bool mutable                       ///< Is it permissible to modify list?
) PS_ATTR_MALLOC;
#define psListIteratorAlloc(list, location, mutable) \
      p_psListIteratorAlloc(__FILE__, __LINE__, __func__, list, location, mutable)
#endif // ifdef DOXYGEN


/** Set the iterator of the list to a given position.  If location is invalid the
 *  iterator position is not changed.
 *
 *  @return bool        TRUE if iterator successfully set, otherwise FALSE.
 */
bool psListIteratorSet(
    psListIterator* iterator,          ///< list iterator
    long location                      ///< index number, PS_LIST_HEAD, or PS_LIST_TAIL
);


/** Adds an element to a psList at position given.
 *
 *  @return bool        TRUE if item was successfully added, otherwise FALSE.
 */
bool psListAdd(
    psList* list,                      ///< list to add item to
    long location,                     ///< index, PS_LIST_HEAD, PS_LIST_TAIL, or numbered location.
    psPtr data                         ///< data item to add.  If NULL, list is not modified.
);


/** Adds an data item to a psList at position just after the list position given
 *
 *  @return bool        TRUE if item was successfully added, otherwise FALSE.
 */
bool psListAddAfter(
    psListIterator* iterator,          ///< list position to add item to
    psPtr data                         ///< data item to add.  If NULL, list is not modified.
);


/** Adds a data item to a psList at position just before the list position given
 *
 *  @return bool        TRUE if item was successfully added, otherwise FALSE.
 */
bool psListAddBefore(
    psListIterator* iterator,          ///< list position to add item to
    psPtr data                         ///< data item to add.  If NULL, list is not modified.
);

/** Remove an item at the specified location from a list.
 *
 *  @return bool        TRUE if element is successfully removed, otherwise FALSE.
 */
bool psListRemove(
    psList* list,                      ///< list to remove element from
    long location                      ///< index of item
);


/** Remove an item from a list.
 *
 *  @return bool        TRUE if element is successfully removed, otherwise FALSE.
 */
bool psListRemoveData(
    psList* list,                      ///< list to remove element from
    psPtr data                         ///< data item to find and remove
);


/** Retrieve an item from a list.
 *
 *  @return psPtr       the item corresponding to the location parameter.  If
 *                      location is invalid (e.g., a numbered index greater
 *                      than the list size or if the list is empty), a
 *                      NULL is returned.
 */
psPtr psListGet(
    psList* list,                      ///< list to retrieve element from
    long location                      ///< index number, PS_LIST_HEAD, or PS_LIST_TAIL
);

/** Retrieve an item from a list.
 *
 *  @return psPtr       the item corresponding to the location parameter.  If
 *                      location is invalid (e.g., a numbered index greater
 *                      than the list size or if the list is empty), a
 *                      NULL is returned.
 */
psPtr psListGetAndRemove(
    psList *list,                       ///< list from which to get and remove the element
    long location                       ///< index of item
);

/** Position the specified iterator to the next item in list.
 *
 *  @return psPtr       the data item at the original iterator position or NULL if the
 *                      iterator went past the end of the list.
 */
psPtr psListGetAndIncrement(
    psListIterator* iterator           ///< iterator to move
);


/** Position the specified iterator to the previous item in list.
 *
 *  @return psPtr       the data item at the original iterator position or NULL if the
 *                      iterator went past the beginning of the list.
 */
psPtr psListGetAndDecrement(
    psListIterator* iterator           ///< iterator to move
);


/** Convert a linked list to an array
 *
 *  @return psArray* A new psArray populated with elements from the list,
 *                      or NULL if the given dlist parameter is NULL.
 */
psArray* psListToArray(
    const psList* list                 ///< List to convert
);


/** Convert array to a doubly-linked list
 *
 *  @return psList* A new psList populated with elements formt the psArray,
 *                      or NULL is the given arr parameter is NULL.
 */
psList* psArrayToList(
    const psArray* array               ///< vector to convert
);


/** Sort a list via a comparison function.
 *
 *  The comparison function must return an integer less than, equal to, or
 *  greater than zero if the first argument is considered to be respectively
 *  less than, equal to, or greater than the second.
 *
 *  If two members compare as equal, their order in the sorted array is
 *  undefined.
 *
 *  @return psList*     Sorted list.
 */
psList* psListSort(
    psList* list,                      ///< the list to sort
    psComparePtrFunc func              ///< the comparison function
);


/** Get the number of elements in use from a specified psList. (list.n)
 *
 *  @return long:       The number of elements in use.
 */
long psListLength(
    const psList *list                 ///< input psList
);


#define PS_ASSERT_LIST_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->iterators || (NAME)->n < 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: List %s or one of its components is NULL.", \
            #NAME); \
    return RVAL; \
}

#define PS_ASSERT_LIST_ITERATOR_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->list || !(NAME)->list->iterators || (NAME)->list->n < 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: List iterator %s or one of its components is NULL.", \
            #NAME); \
    return RVAL; \
}


/// @} End of DataContainer Functions
#endif // #ifndef PS_LIST_H

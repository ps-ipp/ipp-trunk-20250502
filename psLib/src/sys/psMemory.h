/* @file  psMemory.h
 * @brief Contains the definitions for the memory management system
 *
 *  @brief Contains the definitions for the memory management system
 *
 *  This is the generic memory management system put inbetween the user's high
 *  level code and the OS-level memory allocation routines.  This system adds
 *  such features as callback routines for memory error events, tracing
 *  capabilities, and reference counting.
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Robert Lupton, Princeton University
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @ingroup MemoryManagement
 *
 *  @version $Revision: 1.73 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-09-28 00:36:08 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_MEMORY_H
#define PS_MEMORY_H

/// @addtogroup SysUtils System Utilities
/// @{

#include <stdio.h>                      // needed for FILE
#include <pthread.h>                    // mutexes
#include <stdint.h>                     // for uint32_t
#include <stdbool.h>

/** @addtogroup MemoryManagement
 *  @{
 */

/// typedef for memory identification numbers.  Guaranteed to be some variety of integer.
typedef unsigned long psMemId;

/// typedef for a memory block's reference count. Guaranteed to be some variety of integer.
typedef unsigned long psReferenceCount;

/// typedef for deallocator.
typedef void (*psFreeFunc) (void *ptr);

/** Book-keeping data for storage allocator.
 *  N.b. sizeof(psMemBlock) must be chosen such that if ptr is a pointer
 *  returned by malloc, then ((char *)ptr + sizeof(psMemBlock)) is properly
 *  aligned for all storage types.
 */

// The memory overhead of a psMemBlock + the trailing post can be checked in
// gdb with the following command:
//      p sizeof(psMemBlock) + sizeof(void*)
typedef struct psMemBlock
{
    const uint32_t startblock;          ///< initialised to p_psMEMMAGIC
    struct psMemBlock *previousBlock;   ///< previous block in allocation list
    struct psMemBlock *nextBlock;       ///< next block allocation list
    psFreeFunc freeFunc;                ///< deallocator.  If NULL, use generic deallocation.
    size_t userMemorySize;              ///< the size of the user-portion of the memory block
    const psMemId id;                   ///< a unique ID for this allocation
    const pthread_t tid;                ///< set from pthread_self();
    const char *file;                   ///< set from __FILE__ in e.g. p_psAlloc
    const unsigned int lineno;          ///< set from __LINE__ in e.g. p_psAlloc
    const char *func;                   ///< set from __func__

#if defined(PS_MEM_BACKTRACE) && defined(HAVE_BACKTRACE)
    const void **backtrace;             ///< set from backtrace()
    const size_t backtraceSize;         ///< set from bracktrace()
#endif // defined(PS_MEM_BACKTRACE) && defined(HAVE_BACKTRACE)

    psReferenceCount refCounter;        ///< how many times pointer is referenced
    bool persistent;                    ///< true if this is non-user persistent data like error stack, etc.
    bool inFlight;	                ///< true if a nearby block is being free'ed / realloc'ed (local lock)
    const uint32_t endblock;            ///< initialised to p_psMEMMAGIC
}
psMemBlock;

/** prototype of a basic callback used by memory functions
 *
 *  @see psMemAllocCallbackSet
 */
typedef psMemId(*psMemAllocCallback) (
    const psMemBlock *ptr              ///< the psMemBlock just allocated
);

/** prototype of memory free callback used by memory functions
 *
 *  @see psMemFreeCallbackSet
 */
typedef psMemId(*psMemFreeCallback) (
    const psMemBlock *ptr              ///< the psMemBlock being freed
);

/** prototype of a callback used in error conditions
 *
 *  This callback should not try to call psAlloc or psFree.
 *
 *  @see psMemProblemCallbackSet
 */
typedef void (*psMemProblemCallback) (
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psMemBlock *ptr                     ///< the pointer to the problematic memory block.
);

/** prototype of a callback function used when memory runs out
 *
 *  @return void * pointer to requested buffer of the size size_t, or NULL if
 *  memory could not be found.
 *
 *  @see psMemExhaustedCallbackSet
 */
typedef void *(*psMemExhaustedCallback) (
    size_t size                         ///< the size of buffer required
);

/** Memory allocation.  This operates much like malloc(), but is guaranteed to
 * return a non-NULL value.
 *
 *  @return void * pointer to the allocated buffer. This will not be NULL.
 *  @see psFree
 */
#ifdef DOXYGEN
void *psAlloc(
    size_t size                        ///< Size required
);
#else // ifdef DOXYGEN
void *p_psAlloc(
    const char *file,                  ///< File of caller
    unsigned int lineno,               ///< Line number of caller
    const char *func,                  ///< Function name of caller
    size_t size                        ///< Size required
#ifdef __GNUC__
) __attribute__((malloc));
# else // ifdef __GNUC__
);
#endif // ifdef __GNUC__
#ifndef SWIG
#define psAlloc(size) \
p_psAlloc(__FILE__, __LINE__, __func__, size)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN

// if we decide to use spinlocks for psMemory, it will be necessary to call this function at
// the start of every psLib-based program (spinlocks do not have a static initializer)
void psMemInit(void);

/** Set the deallocator routine
 *
 *  A deallocator routine can optionally be assigned to a memory block to
 *  ensure that associated memory blocks also get freed, e.g., memory buffers
 *  referenced within a struct.
 *
 */
#ifdef DOXYGEN
void psMemSetDeallocator(
    void *ptr,                         ///< the memory block to operate on
    psFreeFunc freeFunc                ///< the function to be executed at deallocation
);
#else // ifdef DOXYGEN
void p_psMemSetDeallocator(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    void *ptr,                          ///< the memory block to operate on
    psFreeFunc freeFunc                 ///< the function to be executed at deallocation
);
#ifndef SWIG
#define psMemSetDeallocator(ptr, freeFunc) \
      p_psMemSetDeallocator(__FILE__, __LINE__, __func__, ptr, freeFunc)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Get the deallocator routine
 *
 *  This function returns the deallocator for a memory block.  A deallocator
 *  routine can optionally be assigned to a memory block to ensure that
 *  associated memory blocks also get freed, e.g., memory buffers referenced
 *  within a struct.
 *
 *  @return psFreeFunc    the routine to be called at deallocation.
 */
#ifdef DOXYGEN
psFreeFunc psMemGetDeallocator(
    void *ptr                           ///< the memory block
);
#else // ifdef DOXYGEN
psFreeFunc p_psMemGetDeallocator(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    void *ptr                           ///< the memory block
);
#ifndef SWIG
#define psMemGetDeallocator(ptr) \
      p_psMemGetDeallocator(__FILE__, __LINE__, __func__, ptr)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Activate or Deactivate thread safety and mutex locking in the memory
 * management.
 *
 *  psMemThreadSafety shall turn on thread safety in the memory management
 *  functions if safe is true, and deactivate all mutex locking in the memory
 *  management functions if safe is false.  The function shall return the
 *  previous value of the thread safety.  Note that the default behaviour of
 *  the library shall be for the locking to be performed.
 *
 *  @return bool:       The previous value of the thread safety.
 */
bool psMemSetThreadSafety(
    bool safe                          ///< boolean for turning on/off thread safety
);


/** Get the current state of thread safety and mutex locking in the memory
 * management.
 *
 * psMemGetThreadSafety shall return the current state of thread safety in the
 * memory management system.
 *
 *  @return bool:       The current state of thread safety.
 */
bool psMemGetThreadSafety(void);


/** Set the memory as persistent so that it is ignored when detecting memory
 * leaks.
 *
 *  Used to mark a memory block as persistent data within the library,
 *  i.e., non user-level data used to hold psLib's state or cache data.  Such
 *  examples of this class of memory is psTrace's trace-levels and dynamic
 *  error codes.
 *
 *  Memory marked as persistent is excluded from memory leak checks.
 *
 */
#ifdef DOXYGEN
void psMemSetPersistent(
    void *ptr,                          ///< the memory block to operate on
    bool value,                         ///< true if memory is persistent, otherwise false
);
#else // ifdef DOXYGEN
void p_psMemSetPersistent(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    void *ptr,                          ///< the memory block to operate on
    bool value                          ///< true if memory is persistent, otherwise false
);
#ifndef SWIG
#define psMemSetPersistent(ptr, value) \
      p_psMemSetPersistent(__FILE__, __LINE__, __func__, ptr, value)
#endif // idndef SWIG
#endif // ifdef DOXYGEN


/** Set whether allocated memory is persistent
 *
 *  Set whether allocated memory is persistent. The defeault is false.
 *
 *  @return bool:       The previous value of whether all allocated memory is
 *  persistent
 */
bool p_psMemAllocatePersistent(bool is_persistent); ///< Should all memory allocated be persistent?


/** Get the memory's persistent flag.
 *
 *  Checks if a memory block has been marked as persistent by
 *  p_psMemSetPresistent.
 *
 *  Memory marked as persistent is excluded from memory leak checks.
 *
 *  @return bool    true if memory is marked persistent, otherwise false.
 */
#ifdef DOXYGEN
bool psMemGetPersistent(
    void *ptr,                          ///< the memory block to check.
);
#else // ifdef DOXYGEN
bool p_psMemGetPersistent(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    void *ptr                           ///< the memory block to check.
);
#ifndef SWIG
#define psMemGetPersistent(ptr) \
      p_psMemGetPersistent(__FILE__, __LINE__, __func__, ptr)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Memory re-allocation.  This operates much like realloc(), but is guaranteed
 * to return a non-NULL value.
 *
 *  @return void * pointer to resized buffer. This will not be NULL.
 *  @see psAlloc, psFree
 */
#ifdef DOXYGEN
void *psRealloc(
    void *ptr,                          ///< Pointer to re-allocate
    size_t size                         ///< Size required
);
#else // ifdef DOXYGEN
void *p_psRealloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    void *ptr,                          ///< Pointer to re-allocate
    size_t size                         ///< Size required
#ifdef __GNUC__
) __attribute__((malloc));
# else // ifdef __GNUC__
);
#endif // ifdef __GNUC__
#ifndef SWIG
#define psRealloc(ptr, size) \
      p_psRealloc(__FILE__, __LINE__, __func__, ptr, size)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Free memory.  This operates much like free().
 *  
 *  @see psAlloc, psRealloc
 *  note: we cast ptr to (void *) in case we are supplied a const pointer.
 */
#ifdef DOXYGEN
void psFree(
    void *ptr                           ///< Pointer to free, if NULL, function returns immediately.
);
#else // ifdef DOXYGEN
#ifndef SWIG
#define psFree(ptr) \
    ptr = psMemDecrRefCounter((void *)ptr);
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Check for memory leaks.  This scans for allocated memory buffers not freed
 * with an ID not less than id0.  This is used to check for memory leaks by: -#
 * before a block of code to be checked, store the current ID count via
 * psGetMemId -# after the block of code to be checked, call this function
 * using the ID stored above.  If all memory in the block that was allocated
 * has been freed, this call should output nothing and return 0.
 *
 *  If memory leaks are found, the Memory Problem callback will be called as
 *  well.
 *
 *  @return int  number of memory blocks found as 'leaks', i.e., the number of
 *  currently allocated memory blocks above id0 that have not been freed.  @see
 *  psAlloc, psFree, psgetMemId, psMemProblemCallbackSet
 */
#ifdef DOXYGEN
int psMemCheckLeaks(
    psMemId id0,                       ///< don't list blocks with id < id0
    psMemBlock ***array,               ///< pointer to array of pointers to leaked blocks, or NULL
    FILE * fd,                         ///< print list of leaks to fd (or NULL)
    bool persistence                   ///< make check across all object even persistent ones
);
int psMemCheckLeaks2(
    psMemId id0,                       ///< don't list blocks with id < id0
    psMemBlock ***array,               ///< pointer to array of pointers to leaked blocks, or NULL
    FILE * fd,                         ///< print list of leaks to fd (or NULL)
    bool persistence,                  ///< make check across all object even persistent ones
    int maxDisplayedLeaksCount         ///< List at most maxDisplayedLeaksCount (-1 for all)
);
#else // ifdef DOXYGEN
int p_psMemCheckLeaks(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psMemId id0,                        ///< don't list blocks with id < id0
    psMemBlock ***array,                ///< pointer to array of pointers to leaked blocks, or NULL
    FILE * fd,                          ///< print list of leaks to fd (or NULL)
    bool persistence,                   ///< make check across all object even persistent ones
    int maxDisplayedLeaksCount          ///< List at most maxDisplayedLeaksCount (-1 for all)
);
#ifndef SWIG
#define psMemCheckLeaks2(id0, array, fd, persistence, maxDisplayedLeaksCount) \
      p_psMemCheckLeaks(__FILE__, __LINE__, __func__, id0, array, fd, persistence, maxDisplayedLeaksCount)
#define psMemCheckLeaks(id0, array, fd, persistence) \
      p_psMemCheckLeaks(__FILE__, __LINE__, __func__, id0, array, fd, persistence, 500)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Check for memory corruption.  Scans all currently allocated memory buffers
 * and checks for corruptions, i.e., invalid markers that signify a buffer
 * under/overflow.
 *
 *  @return int
 *
 */
#ifdef DOXYGEN
int psMemCheckCorruption(
    FILE *output,                       ///< FILE to write corrupted blocks too
    bool abort_on_error                 ///< Abort on detecting corruption?
);
#else // ifdef DOXYGEN
int p_psMemCheckCorruption(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    FILE *output,                       ///< FILE to write corrupted blocks too
    bool abort_on_error                 ///< Abort on detecting corruption?
);
#ifndef SWIG
#define psMemCheckCorruption(output, abort_on_error) \
      p_psMemCheckCorruption(__FILE__, __LINE__, __func__, output, abort_on_error)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN

/** Checks to see if a pointer is to a region of memory that was allocated by psAlloc().
 *  @return bool
 *
 */
#ifdef DOXYGEN
bool psMemIsAlloced(
    const void *ptr                           ///< pointer to memory
);
#else // ifdef DOXYGEN
bool p_psMemIsAlloced(
    const char *file,
    unsigned int lineno,
    const char *func,
    const void *ptr
);
#ifndef SWIG
#define psMemIsAlloced(ptr) \
      p_psMemIsAlloced(__FILE__, __LINE__, __func__, ptr)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Return reference counter
 *
 *  @return psReferenceCount
 *
 */
#ifdef DOXYGEN
psReferenceCount psMemGetRefCounter(
    void *ptr                     ///< Pointer to get refCounter for
);

#else // ifdef DOXYGEN
psReferenceCount p_psMemGetRefCounter(
    const char *file,                   ///< File of call
    unsigned int lineno,                ///< Line number of call
    const char *func,                   ///< Function name of caller
    void *ptr                           ///< Pointer to get refCounter for
);
#ifndef SWIG
#define psMemGetRefCounter(ptr) \
      p_psMemGetRefCounter(__FILE__, __LINE__, __func__, ptr)
#endif // !SWIG
#endif // !DOXYGEN


/** Increment reference counter and return the pointer
 *
 *  @return void *
 *
 */
#ifdef DOXYGEN
void *psMemIncrRefCounter(
    void *ptr                           ///< Pointer to increment refCounter, and return
);
#else // ifdef DOXYGEN
void *p_psMemIncrRefCounter(
    const char *file,                   ///< File of call
    unsigned int lineno,                ///< Line number of call
    const char *func,                   ///< Function name of caller
    void *ptr                           ///< Pointer to increment refCounter, and return
);
#ifndef SWIG
#define psMemIncrRefCounter(ptr) \
      p_psMemIncrRefCounter(__FILE__, __LINE__, __func__, ptr)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Decrement reference counter and return the pointer
 *
 *
 *  @return void *    the pointer deremented in refCount, or NULL if pointer is
 *                   fully dereferenced.
 */
#ifdef DOXYGEN
void *psMemDecrRefCounter(
    void *ptr                           ///< Pointer to decrement refCounter, and return
);
#else // DOXYGEN
void *p_psMemDecrRefCounter(
    const char *file,                   ///< File of call
    unsigned int lineno,                ///< Line number of call
    const char *func,                   ///< Function name of caller
    void *ptr                           ///< Pointer to decrement refCounter, and return
);
#ifndef SWIG
#define psMemDecrRefCounter(ptr) \
      p_psMemDecrRefCounter(__FILE__, __LINE__, __func__, ptr)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


#if 0 // psMemSetRefCounter
/** Set reference counter and return the pointer
 *
 *  @return void *    the pointer with refCount set, or NULL if pointer is
 *                   fully dereferenced.
 */
#ifdef DOXYGEN
void * psMemSetRefCounter(
    void * ptr,                        ///< Pointer to decrement refCounter, and return
    psReferenceCount count            ///< New reference count
);
#else // DOXYGEN
void * p_psMemSetRefCounter(
    void * vptr,                        ///< Pointer to decrement refCounter, and return
    psReferenceCount count,            ///< New reference count
    const char *file,                  ///< File of call
    psS32 lineno                       ///< Line number of call
);

#ifndef SWIG
#define psMemSetRefCounter(vptr, count) p_psMemSetRefCounter(vptr, count, __FILE__, __LINE__)
#endif // !SWIG

#endif // !DOXYGEN
#endif // psMemSetRefCounter

/** Set callback for out-of-memory.
 *
 *  If not enough memory is available to satisfy a request by psAlloc or
 *  psRealloc, these functions attempt to find an alternative solution by
 *  calling the psMemExhaustedCallback, a function which may be set by the
 *  programmer in appropriate circumstances, rather than immediately fail.
 *  The typical use of such a feature may be when a program needs a large
 *  chunk of memory to do an operation, but the exact size is not critical.
 *  This feature gives the programmer the opportunity to make a smaller
 *  request and try again, limiting the size of the operating buffer.
 *
 *  @return psMemExhaustedCallback     old psMemExhaustedCallback function
 */
psMemExhaustedCallback psMemExhaustedCallbackSet(
    psMemExhaustedCallback func        ///< Function to run at memory exhaustion
);


/** Set call back for when a particular memory block is allocated
 *
 *  A private variable, p_psMemAllocID, can be used to trace the allocation
 *  and freeing of specific memory blocks. If p_psMemAllocID is set and a
 *  memory block with that ID is allocated, psMemAllocCallback is called
 *  just before memory is returned to the calling function.
 *
 *  @return psMemAllocCallback      old psMemAllocCallback function
 */
psMemAllocCallback psMemAllocCallbackSet(
    psMemAllocCallback func            ///< Function to run at memory allocation of specific mem block
);


/** Set call back for when a particular memory block is freed
 *
 *  A private variable, p_psMemFreeID, can be used to trace the freeing of
 *  specific memory blocks. If p_psMemFreeID is set and the memory block with
 *  the ID is about to be freed, the psMemFreeCallback callback is called just
 *  before the memory block is freed.
 *
 *  @return psMemFreeCallback          old psMemFreeCallback function
 */
psMemFreeCallback psMemFreeCallbackSet(
    psMemFreeCallback func             ///< Function to run at memory free of specific mem block
);


/** get next memory ID
 *
 *  @return psMemId                 the next memory ID to be used
 */
psMemId psMemGetId(void);


/** get the last memory ID used
 *
 *  @return psMemId                 the last memory ID used
 */
psMemId psMemGetLastId(void);


/** set p_psMemAllocID to specific id
 *
 *  A private variable, p_psMemAllocID, can be used to trace the allocation
 *  and freeing of specific memory blocks. If p_psMemAllocID is set and a
 *  memory block with that ID is allocated, psMemAllocCallback is called
 *  just before memory is returned to the calling function.
 *
 *  @return psMemId
 *
 *  @see psMemAllocCallbackSet
 */
psMemId psMemAllocCallbackSetID(
    psMemId id                         ///< ID to set
);


/** set p_psMemFreeID to id
 *
 *  A private variable, p_psMemFreeID, can be used to trace the freeing of
 *  specific memory blocks. If p_psMemFreeID is set and the memory block with
 *  the ID is about to be freed, the psMemFreeCallback callback is called just
 *  before the memory block is freed.
 *
 *  @return psMemId                 the old p_psMemFreeID
 *
 *  @see psMemFreeCallbackSet
 */
psMemId psMemFreeCallbackSetID(
    psMemId id                         ///< ID to set
);


/** return statistics on memory usage
 *
 * @return the total amount of memory owned by psLib; if non-NULL also provide
 * a breakdown into allocated and allocated-and-persistent
 */
size_t psMemStats(const bool print, ///< print details as they're found?
                  size_t *allocated, ///< memory that's currently allocated (but not persistent)
                  size_t *persistent); ///< persistent memory that's currently allocated

/** print detailed information about a psMemBlock
 *
 * This function prints a detailed description of a psMemBlock to output.
 *
 * @return the return status of fprintf()
 */
int psMemBlockPrint(
    FILE *output,                       ///< FILE to write information too
    const psMemBlock *memBlock          ///< psMemBlock to be examined
);

/** print detailed information about a pointer allocated by psAlloc
 *
 * This function prints a detailed description of a psMemBlock to output.
 *
 * @return the return status of fprintf()
 */
int psMemBlockPrintPtr(
    FILE *output,                       ///< FILE to write information too
    void *ptr ///< pointer to be examined
);

/** test for matching types (equal free functions)
 *
 * This function returns true if the two pointers have matching, non-NULL free functions.
 * Supplied pointers must have been allocated within the psLib memory system (ie, with a psAlloc) or the function will abort. (XXX just return false?)
 * Supplied pointers must have been provided with free function or the function returns false.
 */
bool psMemTypeEqual (void *ptr1, ///< pointer to first psMemory object
		     void *ptr2 ///< pointer to second psMemory object
  );

void psMemDumpSetState (bool state);
void psMemDump(const char *name);

// Ensure this is a psLib pointer
#define PS_ASSERT_PTR_HEAVY(PTR, RVAL) \
{ \
    if (PTR && (!psMemIsAlloced(PTR))) { \
        psError(PS_ERR_MEMORY_CORRUPTION, false, \
            "Error: Pointer %p is corrupted or not on the PS memory system.", \
            PTR); \
        return RVAL; \
    } \
}

/// @} end of SysUtils

#ifndef DOXYGEN

/*
 * Ensure that any program using malloc/realloc/free will fail to compile
 */
#ifndef PS_ALLOW_MALLOC
#ifdef __GNUC__
#pragma GCC poison malloc realloc calloc free
#else // ifdef __GNUC__
#define malloc(S)       _Pragma("error Use of malloc is not allowed.  Use psAlloc instead.")
#define realloc(P,S)    _Pragma("error Use of realloc is not allowed.  Use psRealloc instead.")
#define calloc(S)       _Pragma("error Use of calloc is not allowed.  Use psAlloc instead.")
#define free(P)         _Pragma("error Use of free is not allowed.  Use psFree instead.")
#endif // ifdef __GNUC__
#endif // ifndef PS_ALLOW_MALLOC

#endif // #ifndef DOXYGEN
#endif // #ifndef PS_MEMORY_H

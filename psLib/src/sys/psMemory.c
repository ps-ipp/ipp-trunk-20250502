/** @file  psMemory.c
*
*  @brief Contains the definitions for the memory management system
*
*  psMemory.h has additional information and documentation of the routines
*  found in this file.
*
*  @author Robert DeSonia, MHPCC
*  @author Robert Lupton, Princeton University
*  @author Joshua Hoblitt, University of Hawaii
*
*  @version $Revision: 1.101 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-02-06 01:05:30 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#define PS_ALLOW_MALLOC                    // we're allowed to call malloc()

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#if defined(PS_MEM_BACKTRACE) && defined(HAVE_BACKTRACE)
# include <execinfo.h>
#endif

#include "psError.h"    // for psErrorStackPrint() only
#include "psMemory.h"
#include "psAbort.h"

// Magic number in psMemBlock header
#define P_PS_MEMMAGIC (uint32_t)0xdeadbeef

// The psLib memory tools require locking to manipulate the linked list which holds the memory
// structure.  There are two major options which can be invoked, given below.  

// USE_SPINLOCK replaces the mutex in this code block with a spinlock.  This might be faster in
// some cases, but probably not enough to matter.  The disadavantages are: (1) programs would
// need to call the function 'psMemInit' (which they currently do not in general); (2)
// single-processor machines are poorly set up to use spinlocks; (3) not all compilers /
// machine combinations support spinlocks.  This option is NOT recommended

// USE_HARDLOCK replaces the per-memBlock locking that is the minimum required to protect the
// psRealloc function with pure mutex locking that holds the lock during the (possibly
// expensive) realloc call.  At this point, it does not seem clear that there is a substantial
// gain in processing speed from using the per-memBlock locking, but more testing may reveal
// cases where it matters.  

# define USE_SPINLOCK 0
# define USE_HARDLOCK 1

# if (USE_SPINLOCK) 
#define MUTEX_LOCK(mutexPtr) \
if (safeThreads) { \
    pthread_spin_lock(mutexPtr); \
}
#define MUTEX_UNLOCK(mutexPtr) \
if (safeThreads) { \
    pthread_spin_unlock(mutexPtr); \
}
# else // !USE_SPINLOCK
#define MUTEX_LOCK(mutexPtr) \
if (safeThreads) { \
    pthread_mutex_lock(mutexPtr); \
}
#define MUTEX_UNLOCK(mutexPtr) \
if (safeThreads) { \
    pthread_mutex_unlock(mutexPtr); \
}
# endif // USE_SPINLOCK

// psAbort() calls functions that call psAlloc() so it is *UNSAFE* to use it
// from within the memory subsystem.  Previous implementations tried to do
// this and would deadlock while trying to allocate memory.
//
// Note that psError() is also *UNSAFE* to use from within the memory
// subsystem.
#define PS_MEM_ABORT(name, ...) \
P_PS_MEM_ABORT(__FILE__, __LINE__, __func__, name, __VA_ARGS__)

// psErrorStackPrint() was specifically modified to be safe to call from inside
// psMemory.c.
#define P_PS_MEM_ABORT(filename, lineno, func, name, ...) \
fprintf(stderr, "%s (%s:%d) ", func, filename, lineno); \
fprintf(stderr, __VA_ARGS__);\
psErrorStackPrint(stderr, "\nAborting.  Error stack:\n"); \
fprintf(stderr, "\n");\
abort();

#define HANDLE_BAD_BLOCK(memBlock, file, lineo, func) \
if (isBadMemBlock(stderr, memBlock, file, lineo, func)) { \
    PS_MEM_ABORT(__func__, "Unsafe to Continue\n"); \
}

static bool checkingForCorruption = false;

static bool isBadMemBlock(FILE *output, const psMemBlock *memBlock, const char *file, unsigned int lineo, const char *func);

// memBlockListMutex protects access to:
//      safeThreads -- very rarely accessed
//      memory_is_persistent -- very rarely accessed
//      memAllocCallback -- very rarely accessed
//      memFreeCallback -- very rarely accessed
//      memExhaustedCallback -- very rarely accessed
//      memAllocID -- rarely accessed
//      memFreeID -- rarely accessed
//      lastMemBlockAllocated
//      memid -- rarely accessby
//      "the linked list of mem blocks"
//
// This is a fair amount of stuff to protect with a single mutex but most of
// these items are *VERY* low contention items.  The only item that should be
// performance issue is "the linked list of mem blocks".  If this does become a
// problem in production use of the list could be disabled as it is largely a
// debugging feature.
//
//

# if (USE_SPINLOCK)
static pthread_spinlock_t memBlockListMutex;
# else
static pthread_mutex_t memBlockListMutex = PTHREAD_MUTEX_INITIALIZER;
# endif

void psMemInit() {

# if (USE_SPINLOCK) 
    pthread_spin_init(&memBlockListMutex, 0);
# endif

}

// test function : only use in test mode:
void psMemDumpBigBlocks (psMemBlock *mb, char *mode);

/******** thread safety options management ********/

// private boolean for enabling/disabling thread safety.  Default = enabled.
static bool safeThreads = true;

// Set the thread-safety state of the memory system: default is true
bool psMemSetThreadSafety(bool safe)
{
    // this function is only called ~once per program, before threads are launched
    MUTEX_LOCK(&memBlockListMutex);

    bool oldState = safeThreads;
    safeThreads = safe;

    MUTEX_UNLOCK(&memBlockListMutex);

    return oldState;
}


// Get the thread-safety state of the memory system
bool psMemGetThreadSafety(void)
{
    // this function is only called ~once per program, probably before threads are launched
    MUTEX_LOCK(&memBlockListMutex);

    bool oldState = safeThreads;

    MUTEX_UNLOCK(&memBlockListMutex);

    return oldState;
}

/******** persistent memory options management ********/

// private boolean for deciding if allocated memory is persistent by default
static bool memory_is_persistent = false;

/* Set whether allocated memory is persistent
 */
bool p_psMemAllocatePersistent(bool is_persistent)
{
    // this function is only called ~once per program, before threads are launched
    MUTEX_LOCK(&memBlockListMutex);

    const bool old = memory_is_persistent;
    memory_is_persistent = is_persistent;

    MUTEX_UNLOCK(&memBlockListMutex);

    return old;
}

/*
 * And now the I-want-to-be-informed callbacks
 *
 * Call the callbacks when these IDs are allocated/freed
 */
/******** memory allocation callback management (memAllocCallback) ********/

// Default memFreeCallback function
static psMemId memAllocCallbackDefault(const psMemBlock *memBlock)
{
    static psMemId incr = 0; // "memAllocID += incr"

    return incr;
}

static psMemAllocCallback memAllocCallback = memAllocCallbackDefault;

psMemAllocCallback psMemAllocCallbackSet(psMemAllocCallback func)
{
    // this function is only called rarely per program
    MUTEX_LOCK(&memBlockListMutex);

    psMemAllocCallback old = memAllocCallback;

    if (func != NULL) {
        memAllocCallback = func;
    } else {
        memAllocCallback = memAllocCallbackDefault;
    }

    MUTEX_UNLOCK(&memBlockListMutex);

    return old;
}

// notify user when this block is allocated
static psMemId memAllocID = 0;

// the above callback is only called if a specific psMemId is set with this function
// this function is rarely called in a given program
psMemId psMemAllocCallbackSetID(psMemId id)
{
    // this function is only called rarely per program
    MUTEX_LOCK(&memBlockListMutex);

    psMemId old = memAllocID;

    memAllocID = id;

    MUTEX_UNLOCK(&memBlockListMutex);

    return old;
}

/******** memory free callback management (memFreeCallback) ********/

// Default memFreeCallback function
static psMemId memFreeCallbackDefault(const psMemBlock *memBlock)
{
    static psMemId incr = 0; // "memFreeID += incr"

    return incr;
}

static psMemFreeCallback memFreeCallback = memFreeCallbackDefault;

// this function is called only rarely in a program
psMemFreeCallback psMemFreeCallbackSet(psMemFreeCallback func)
{
    // this function is only called rarely per program
    MUTEX_LOCK(&memBlockListMutex);

    psMemFreeCallback old = memFreeCallback;

    if (func != NULL) {
        memFreeCallback = func;
    } else {
        memFreeCallback = memFreeCallbackDefault;
    }

    MUTEX_UNLOCK(&memBlockListMutex);

    return old;
}

// notify user when this block is freed
static psMemId memFreeID = 0;

// the above callback is only called if a specific psMemId is set with this function
psMemId psMemFreeCallbackSetID(psMemId id)
{
    // this function is rarely called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    psMemId old = memFreeID;

    memFreeID = id;

    MUTEX_UNLOCK(&memBlockListMutex);

    return old;
}

/******** memory exhausted callback management (memExhaustedCallback) ********/

// Default memExhaustedCallback function
static void *memExhaustedCallbackDefault(size_t size)
{
    return NULL;
}

static psMemExhaustedCallback memExhaustedCallback = memExhaustedCallbackDefault;

psMemExhaustedCallback psMemExhaustedCallbackSet(psMemExhaustedCallback func)
{
    // this function is rarely called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    psMemExhaustedCallback old = memExhaustedCallback;

    if (func != NULL) {
        memExhaustedCallback = func;
    } else {
        memExhaustedCallback = memExhaustedCallbackDefault;
    }

    MUTEX_UNLOCK(&memBlockListMutex);

    return old;
}

/* An example callback function to check the state of the memory system; may be registered
 * with psMem{Alloc,Free}CallbackSet
 */
psMemId memAllocCallbackCheckCorruption(const psMemBlock *memBlock)
{
    static psMemId incr = 10; // "memAllocID += incr"

    if (psMemCheckCorruption(stderr, false) > 0) {
        fprintf(stderr, "Detected memory corruption\n"); // somewhere to set a breakpoint
    }

    return incr;
}

/**** Unique ID for allocated blocks and associated accessor functions
 */
static volatile psMemId memid = 0;

/* Return memory ID counter for next block to be allocated
 */
psMemId psMemGetId(void)
{
    // this function is only occasionally called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    psMemId id = memid + 1;

    MUTEX_UNLOCK(&memBlockListMutex);

    return id;
}

/* Return memory ID counter for last block allocated
 */
psMemId psMemGetLastId(void)
{
    // this function is only occasionally called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    psMemId id = memid;

    MUTEX_UNLOCK(&memBlockListMutex);

    return id;
}

/** BLOCK_LOCK, BLOCK_UNLOCK, BLOCKSET_LOCK, BLOCKSET_UNLOCK are used to set per-memBlock locks
 **     to allow psRealloc to release the global lock before performing the expensive system
 **     call 'realloc'
 **/

# if (!USE_HARDLOCK)
static int setLock = 0;
static int clearLock = 0;
static int retryLock = 0;
# endif

# define BLOCK_SLEEP 100

# define MEM_ASSERT(COND,MSG) { if (!(COND)) { fprintf (stderr, MSG); abort(); } }

// pointer to the last mem block that was allocated.
// This is the root of the entire memory list
static volatile psMemBlock *lastMemBlockAllocated = NULL;

// set lock on lastMemBlockAllocated
void BLOCKLAST_LOCK () {

# if (USE_HARDLOCK) 
    MUTEX_LOCK(&memBlockListMutex);
# else

    // if memBlock is not defined, we are just asking for a m
    if (!lastMemBlockAllocated) {
	MUTEX_LOCK(&memBlockListMutex);
	return;
    }
	
    while (true) {
	// set a lock, then lock this memblock and its neighbors
	MUTEX_LOCK(&memBlockListMutex);
    
	// did we beat the race condition?  is the lock still clear?
	if (lastMemBlockAllocated->inFlight) {
	    // nope, we need to try again
	    MUTEX_UNLOCK(&memBlockListMutex);
	    retryLock ++;
	    usleep (BLOCK_SLEEP);
	    continue;
	}

	// the marker is ours!
	lastMemBlockAllocated->inFlight = true;
	setLock ++;
	return;
    }
# endif
}

// memBlock->inFlight is a 'soft' lock.  if true, the lock is set
void BLOCK_LOCK (psMemBlock *memBlock, bool keepMutex) {

# if (USE_HARDLOCK) 
    MUTEX_LOCK(&memBlockListMutex);
# else

    MEM_ASSERT (memBlock || keepMutex, "trying to set a soft lock on a non-existent memBlock\n");

    // if memBlock is not defined, we are just asking for a m
    if (!memBlock) {
	MUTEX_LOCK(&memBlockListMutex);
	return;
    }
	
    while (true) {
	// set a lock, then lock this memblock and its neighbors
	MUTEX_LOCK(&memBlockListMutex);
    
	// did we beat the race condition?  is the lock still clear?
	if (memBlock->inFlight) {
	    // nope, we need to try again
	    MUTEX_UNLOCK(&memBlockListMutex);
	    usleep (BLOCK_SLEEP);
	    retryLock ++;
	    continue;
	}

	// the marker is ours!
	memBlock->inFlight = true;
	setLock ++;
	if (keepMutex) return;

	MUTEX_UNLOCK(&memBlockListMutex);
	return;
    }
# endif
}

void BLOCK_UNLOCK (psMemBlock *memBlock, bool haveMutex) {
    
# if (USE_HARDLOCK)
    MUTEX_UNLOCK(&memBlockListMutex);
# else

    MEM_ASSERT (memBlock || haveMutex, "trying to clear a soft lock on a non-existent memBlock\n");

    // if memBlock is not defined, we are just asking for a regular lock
    if (!memBlock) {
	MUTEX_UNLOCK(&memBlockListMutex);
	return;
    }

    MEM_ASSERT (memBlock->inFlight, "trying to clear an unlocked memBlock\n");

    // need to lock before clearing the marker
    if (!haveMutex) {
	MUTEX_LOCK(&memBlockListMutex);
    }
    memBlock->inFlight = false;
    clearLock ++;
    MUTEX_UNLOCK(&memBlockListMutex);
    return;
# endif    
}

// attempt to grab the markers on the given memBlock and its two neighbors.  the only times the
// two neighbors do not exist is if we are at the beginning or end of the list (or the list is
// new).
void BLOCKSET_LOCK (psMemBlock *memBlock, bool keepMutex) {

# if (USE_HARDLOCK)
    MUTEX_LOCK(&memBlockListMutex);
# else

    MEM_ASSERT (memBlock, "trying to set a soft lock on a non-existent memBlock\n");

    while (true) {
	// we cannot set the lock on the marker while the lock is held
	// wait until all three markers are clear:
	// while (memBlock->inFlight) { usleep (BLOCK_SLEEP); }
	// while (memBlock->nextBlock && memBlock->nextBlock->inFlight) { usleep (BLOCK_SLEEP); }
	// while (memBlock->previousBlock && memBlock->previousBlock->inFlight) { usleep (BLOCK_SLEEP); }

	// set a lock, then lock this memblock and its neighbors
	MUTEX_LOCK(&memBlockListMutex);
    
	// did we beat the race condition?  are all three locks still clear?
	if (memBlock->inFlight) {
	    // nope, we need to try again
	    MUTEX_UNLOCK(&memBlockListMutex);
	    usleep (BLOCK_SLEEP);
	    retryLock ++;
	    continue;
	}
        if (memBlock->nextBlock && memBlock->nextBlock->inFlight) {
            // we reply on the value of 'inFlight'.  we should crash if this block is corrupted
            if (memBlock->nextBlock->startblock != P_PS_MEMMAGIC) {
                PS_MEM_ABORT(__func__, "Unsafe to Continue\n");
            }
            if (memBlock->nextBlock->endblock != P_PS_MEMMAGIC) {
                PS_MEM_ABORT(__func__, "Unsafe to Continue\n");
            }
            // nope, we need to try again
            MUTEX_UNLOCK(&memBlockListMutex);
            usleep (BLOCK_SLEEP);
            retryLock ++;
            continue;
        }
        if (memBlock->previousBlock && memBlock->previousBlock->inFlight) {
            // we reply on the value of 'inFlight'.  we should crash if this block is corrupted
            if (memBlock->previousBlock->startblock != P_PS_MEMMAGIC) {
                PS_MEM_ABORT(__func__, "Unsafe to Continue\n");
            }
            if (memBlock->previousBlock->endblock != P_PS_MEMMAGIC) {
                PS_MEM_ABORT(__func__, "Unsafe to Continue\n");
            }
            // nope, we need to try again
            MUTEX_UNLOCK(&memBlockListMutex);
            usleep (BLOCK_SLEEP);
            retryLock ++;
            continue;
        }

	// the markers are ours!
	memBlock->inFlight = true;
	setLock ++;
	if (memBlock->nextBlock) {
	    memBlock->nextBlock->inFlight = true;
	}
	if (memBlock->previousBlock) {
	    memBlock->previousBlock->inFlight = true;
	}
	if (keepMutex) return;

	MUTEX_UNLOCK(&memBlockListMutex);
	return;
    }
# endif
}

void BLOCKSET_UNLOCK (psMemBlock *memBlock, bool haveMutex) {
    
# if (USE_HARDLOCK) 
    MUTEX_UNLOCK(&memBlockListMutex);
# else

    // need to lock before clearing the marker
    if (!haveMutex) {
	MUTEX_LOCK(&memBlockListMutex);
    }
    MEM_ASSERT (memBlock->inFlight, "trying to clear an unlocked memBlock\n");
    memBlock->inFlight = false;
    clearLock ++;
    if (memBlock->nextBlock) {
	MEM_ASSERT (memBlock->nextBlock->inFlight, "trying to clear an unlocked memBlock\n");
	memBlock->nextBlock->inFlight = false;
    }
    if (memBlock->previousBlock) {
	MEM_ASSERT (memBlock->previousBlock->inFlight, "trying to clear an unlocked memBlock\n");
	memBlock->previousBlock->inFlight = false;
    }
    MUTEX_UNLOCK(&memBlockListMutex);
    return;
# endif
}

/* these are the measured values, but below we round the ends up and down
# define PS_BAD_MALLOC_RANGE_1_MIN 39637060
# define PS_BAD_MALLOC_RANGE_1_MAX 39649156

# define PS_BAD_MALLOC_RANGE_2_MIN 79288100
# define PS_BAD_MALLOC_RANGE_2_MAX 79294312
*/

# define PS_BAD_MALLOC_RANGE_1_MIN 39636000
# define PS_BAD_MALLOC_RANGE_1_MAX 39650000

# define PS_BAD_MALLOC_RANGE_2_MIN 79288000
# define PS_BAD_MALLOC_RANGE_2_MAX 79298000

/* Actually allocate memory
 */
void *p_psAlloc(const char *file,
                unsigned int lineno,
                const char *func,
                size_t size)
{
    size_t totalSize = sizeof(psMemBlock) + size + sizeof(void *);

    // for gcc 4.3.2, linux 3.7.6 (at least) there are bad malloc sizes.  if a request
    // is made for one of these bad ranges, actually allocate a larger amount 
    if ((totalSize > PS_BAD_MALLOC_RANGE_1_MIN) && (totalSize < PS_BAD_MALLOC_RANGE_1_MAX)) { totalSize = PS_BAD_MALLOC_RANGE_1_MAX; }
    if ((totalSize > PS_BAD_MALLOC_RANGE_2_MIN) && (totalSize < PS_BAD_MALLOC_RANGE_2_MAX)) { totalSize = PS_BAD_MALLOC_RANGE_2_MAX; }

    psMemBlock *memBlock = malloc(totalSize);
    if (memBlock == NULL) {
        // this lock is only occasionally called in a given program (rarely fail malloc)
        MUTEX_LOCK(&memBlockListMutex);
        memBlock = memExhaustedCallback(size);
        MUTEX_UNLOCK(&memBlockListMutex);
        if (memBlock == NULL) {
            PS_MEM_ABORT(__func__, "Failed to allocate %zd bytes at %s (%s:%d)", size, func, file, lineno);
        }
    }

    // posts
    *(psU32 *)&memBlock->startblock = P_PS_MEMMAGIC;
    *(psU32 *)&memBlock->endblock   = P_PS_MEMMAGIC;
    *(psU32 *)((char *) (memBlock + 1) + size) = P_PS_MEMMAGIC;

    // size of memory allocated
    memBlock->userMemorySize = size;

    // alloc request by:
    // thread
    *(pthread_t *)&memBlock->tid = pthread_self();
    // file
    memBlock->file = file;
    // line number
    *(unsigned int *)&memBlock->lineno = (unsigned int)lineno;
    // function
    memBlock->func = func;

    // per-block lock (off by default)
    memBlock->inFlight = false;

    #if defined(PS_MEM_BACKTRACE) && defined(HAVE_BACKTRACE)
    #define BACKTRACE_BUFFER_SIZE 32
    // psMemBlock.func is a 'const char *', so basically we're going to abuse
    // that and treat it as a void ** to carry around backtrace information.
    // psMemBlock is not ifdef'd to make sure that psMemBlock is always the
    // same size & layout reguardless of the pslib .so that's being linked
    // against
    void **bt = malloc(BACKTRACE_BUFFER_SIZE * sizeof(void *));
    if (bt == NULL) {
        PS_MEM_ABORT(__func__, "Failed to allocate memory for backtrace buffer: %zd bytes at %s (%s:%d)",
                     32 * sizeof(void *), func, file, lineno);
    }
    *(size_t *)&memBlock->backtraceSize = backtrace(bt, BACKTRACE_BUFFER_SIZE);
    *(void ***)&memBlock->backtrace = bt;
    #endif // ifdef HAVE_BACKTRACE

    // free function
    memBlock->freeFunc = NULL;

    // persistent memory flag
    memBlock->persistent = memory_is_persistent;

    // this block will be add as the last mem block in the list
    memBlock->previousBlock = NULL;

    // ref count
    memBlock->refCounter = 1;                   // one user so far

    // XXX these lines are test lines that potentially access invalid memory
    // XXX psMemBlock *last0 = lastMemBlockAllocated;
    // XXX int flight0 = (lastMemBlockAllocated) ? lastMemBlockAllocated->inFlight : 10;

    // need exclusive access of the memory block list now...
    // this lock is very frequently called in a given program
    // lastMemBlockAllocated is always true except the first allocation
    BLOCKLAST_LOCK ();

    // XXX psMemBlock *last1 = lastMemBlockAllocated;
    // XXX int flight1 = (lastMemBlockAllocated) ? lastMemBlockAllocated->inFlight : 10;

    // XXX if (false) {
    // XXX 	fprintf (stderr, "last 0 : %lld, last 1 : %lld, flight0: %d, flight1: %d\n", (long long int) last0, (long long int) last1, (int) flight0, (int) flight1);
    // XXX }

    // increment the memory id only after we've grabbed the memBlockListMutex this value is
    // returned by psMemGetID and psMemGetLastID, but only ever modified here
    *(psMemId* )&memBlock->id = ++memid;

    // insert the new block to the front of the memBlock linked-list
    if (lastMemBlockAllocated) {
        // exchange forward and backward references with the last allocated block
        lastMemBlockAllocated->previousBlock = memBlock;
    }
    memBlock->nextBlock = (psMemBlock *) lastMemBlockAllocated;
    lastMemBlockAllocated = memBlock;

    // Did the user ask to be informed about this allocation?
    if (memBlock->id == memAllocID) {
        // memAllocID can only be changed while the memBlockList mutex is held
        memAllocID += memAllocCallback(memBlock);
    }

    BLOCK_UNLOCK (memBlock->nextBlock, true);

    // And return the user the memory that they allocated
    return memBlock + 1;                        // user memory
}

/* internal routine to check the consistency of the allocated and/or free memory arena.
 * this is used by the user functions below (psMem{Set,Get}Deallocator,
 * N.b. If the block wasn't allocated by psAlloc, it will appear corrupted
 */
static bool isBadMemBlock(FILE *output, const psMemBlock *memBlock, const char *file, unsigned int lineno, const char *func)
{
    // n.b. since this is called by psMemCheckCorruption while the memblock
    // list is mutex locked, we shouldn't call such things as
    // p_psAlloc/p_psFree here.

    bool bad = false;
    bool blockPrinted = false;

    if (output && (memBlock == NULL)) {
        fprintf(output, _("NULL memory block.\n"
                          "Caught in %s at (%s:%d.).\n\n"), func, file, lineno);
        // return now as we can't do anything else
        return true;
    }

# if (0)
    // Currently psAlloc()/psRealloc() will blindly create memBlock's with a
    // size of 0.  This test is in here to check if this is really
    // happening/being use as a feature in the wild.
    if (memBlock->userMemorySize < 1) {
	bad = true;
	if (output) {
	    psMemBlockPrint(output, memBlock);
	    blockPrinted = true;
	    fprintf(output, _("\n\tMemory block has a size of less than 1.\n"));
	}
    }
# endif

    // XXX Looking at the reference counter is subject to a race condition because this function is generally
    // not locked.  Normally this is not a problem because though we may increment and decrement references
    // within a thread, we don't destroy the object completely (which is what we're checking for here).  It is
    // the user's responsibility to protect against the complete destruction of memory either by not doing it
    // or by locking on all reference changes for that memory.
    if (memBlock->refCounter < 1) {
        // using an unreferenced block of memory, are you?
        bad = true;
	if (output) {
	  psMemBlockPrint(output, memBlock);
	  blockPrinted = true;
	  fprintf(output, _("\n\tMemory block was freed but still being used.\n"));
	}
    }

    if (memBlock->startblock != P_PS_MEMMAGIC || memBlock->endblock != P_PS_MEMMAGIC) {
        bad = true;
        if (output) {
	  if (!blockPrinted) {
            psMemBlockPrint(output, memBlock);
            blockPrinted = true;
	  }
	  fprintf(output, _("\n\tMemory block is corrupted; buffer underflow detected.\n"));
	}
	if (!checkingForCorruption) {
	  p_psMemCheckCorruption(__FILE__, __LINE__, __func__, output, false);
	}
    }

    if (*(psU32 *)((char *)(memBlock + 1) + memBlock->userMemorySize) != P_PS_MEMMAGIC) {
        bad = true;
        if (output) {
	  if (!blockPrinted) {
            psMemBlockPrint(output, memBlock);
            blockPrinted = true;
	  }
	  fprintf(output, _("\n\tMemory block is corrupted; buffer overflow detected.\n"));
	}
	if (!checkingForCorruption) {
	  p_psMemCheckCorruption(__FILE__, __LINE__, __func__, output, false);
	}
    }

# if (0)
    //  XXX ->nextBlock, & -> prevousBlock really should not be looked at by
    //  this function as they may be changed out from underneath us by new
    //  memory allocation.  However, the memBlock itself shouldn't go away (end
    //  users responsiblity) so all we're really risking is a garbage value.
    //  XXX EAM : is this the error I'm catching in multithreaded processing?
    if (memBlock == memBlock->nextBlock) {
        bad = true;
	if (output) {
	  if (!blockPrinted) {
            psMemBlockPrint(output, memBlock);
            blockPrinted = true;
	  }
	  fprintf(output, _("\n\tMemory block's ->nextBlock pointer refers to itself.\n"));
	}
    }

    if (memBlock == memBlock->previousBlock) {
        bad = true;
	if (output) {
	  if (!blockPrinted) {
            psMemBlockPrint(output, memBlock);
            blockPrinted = true;
	  }
	  fprintf(output, _("\n\tMemory block's ->previousBlock pointer refers to itself.\n"));
	}
    }
# endif

    if (bad && output) {
        fprintf(output, _("\tCaught in %s at (%s:%d).\n\n"), func, file, lineno);
    }

    return bad;
}

void *p_psRealloc(const char *file,
                  unsigned int lineno,
                  const char *func,
                  void *ptr,
                  size_t size)
{
    if (ptr == NULL) {
        return p_psAlloc(file, lineno, func, size);
    }

    psMemBlock *memBlock = ((psMemBlock *)ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    if (size == memBlock->userMemorySize) {
        // Nothing to do
        return ptr;
    }

    // Reallocate the memory

    /* psRealloc and locks: 

       psRealloc calls 'realloc' to change the size of the memory block.  This call is likely
       to change the address of the memory block itself.  This is a problem because the address
       of the memory block is referred to by the neighbor blocks via their memBlock->nextBlock
       and memBlock->previousBlock elements.  If the current memBlock is the first one, then
       this issue also applies to 'lastMemBlockAllocated'.  The elements may also be modified
       by psFree and psAlloc.  Thus, we need to prevent multiple threads from calling psFree,
       psAlloc, or psRealloc on neighbor memory blocks at the same time.

       Unfortunately, unlike psAlloc and psFree, psRealloc cannot perform the (expensive)
       system call operation (realloc) first and adjust the pointers in separate step (since
       the value modified by realloc *is* one of those points.  It is thus necessary to include
       the realloc call within the locked segement, making psRealloc likely to serialize the
       thread operations.

       Alternatively, we can recognize that the lock only need be applied to operations which
       are performed on neighboring memBlocks.  We can thus reduce the contention by having a
       per-memBlock boolean (inFlight) which says the memBlock or a neighbor is being
       modified.  If psFree and psAlloc respect that boolean, they will not modify a memBlock
       which is already being realloced (or its neighbor).  In this way, the 'realloc' call can
       be performed outside of the locked region.

    */

    // set a lock, then lock this memblock and its neighbors.  at the end of this call,
    // the global mutex is released, but the memBlock and its neighbors are protected
    BLOCKSET_LOCK (memBlock, false);
    // MUTEX_LOCK(&memBlockListMutex);

    psMemBlock *nextBlock = memBlock->nextBlock;
    psMemBlock *previousBlock = memBlock->previousBlock;

    // Is this the last block we allocated?  If it is, we need to keep track of
    // this fact and update lastMemBlockAllocated *after* the realloc or
    // lastMemBlockAllocated will be left with a bogus value
    bool isBlockLast = (memBlock == lastMemBlockAllocated);

    size_t totalSize = sizeof(psMemBlock) + size + sizeof(void *);

    // for gcc 4.3.2, linux 3.7.6 (at least) there are bad malloc sizes.  if a request
    // is made for one of these bad ranges, actually allocate a larger amount 
    if ((totalSize > PS_BAD_MALLOC_RANGE_1_MIN) && (totalSize < PS_BAD_MALLOC_RANGE_1_MAX)) { totalSize = PS_BAD_MALLOC_RANGE_1_MAX; }
    if ((totalSize > PS_BAD_MALLOC_RANGE_2_MIN) && (totalSize < PS_BAD_MALLOC_RANGE_2_MAX)) { totalSize = PS_BAD_MALLOC_RANGE_2_MAX; }

    // do the expensive system call.  other threads can continue to modify other memBlocks
    // except this one and its two neighbors
    memBlock = (psMemBlock *)realloc(memBlock, totalSize);
    if (memBlock == NULL) {
        memBlock = memExhaustedCallback(size);
        if (memBlock == NULL) {
            psMemBlockPrint(stderr,  ((psMemBlock *)ptr) - 1);
            fprintf(stderr, "Problem reallocating block\n");
            PS_MEM_ABORT(__func__, "Failed to reallocate to %zd bytes at %s (%s:%d)", size, func, file, lineno);
        }
    }

    memBlock->userMemorySize = size;
    *(psU32 *)((char *)(memBlock + 1) + size) = P_PS_MEMMAGIC;

    // update the references on the list:

    // is we are modifying the last mem block, we need to update lastMemBlockAllocated
    if (isBlockLast) {
        lastMemBlockAllocated = memBlock;
    }

    // the block location may have changed, so fix the linked list addresses.
    if (nextBlock != NULL) {
        nextBlock->previousBlock = memBlock;
    }
    if (previousBlock != NULL) {
        previousBlock->nextBlock = memBlock;
    }

    // Did the user ask to be informed about this allocation?
    if (memBlock->id == memAllocID) {
        memAllocID += memAllocCallback(memBlock);
    }

    // all of the list modifications are done; set the lock and clear the per-memBlock locks
    BLOCKSET_UNLOCK(memBlock, false);
    // MUTEX_UNLOCK(&memBlockListMutex);

    // XXX these are not actually guaranteed : another thread may already grab them before we get here
    // psAssert (!memBlock->inFlight, "unreleased lock?");
    // psAssert (!nextBlock || !nextBlock->inFlight, "unreleased lock?");
    // psAssert (!previousBlock || !previousBlock->inFlight, "unreleased lock?");

    return memBlock + 1;                    // usr memory
}

/*
 * Check for memory leaks.
 */
int p_psMemCheckLeaks(const char *file,
                      unsigned int lineno,
                      const char *func,
                      psMemId id0,
                      psMemBlock ***array,
                      FILE * fd,
                      bool persistence,
		      int maxDisplayedLeaksCount          ///< List at most maxDisplayedLeaksCount (-1 for all)
		  )
{
    psS32 nleak = 0;
    psS32 j = 0;
    psMemBlock *topBlock = (psMemBlock *) lastMemBlockAllocated;

    // XXX move this elsewhere?
    if (fd != NULL) {
# if (USE_HARDLOCK)
# else
      fprintf (fd, "set %d locks, cleared %d locks, retry on %d locks (memID %ld)\n", setLock, clearLock, retryLock, memid);
# endif
    }

    // make sure that the memblock list is free of corruption before we crawl
    // the list
    p_psMemCheckCorruption(file, lineno, func, fd, true);

    // this lock is rarely called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    // if topBlock is NULL there's nothing to do
    if (!topBlock) {
        MUTEX_UNLOCK(&memBlockListMutex);
        return 0;
    }

    // find the very first memblock
    psMemBlock *memBlock = NULL;
    for (memBlock = topBlock; memBlock->nextBlock != NULL; memBlock = memBlock->nextBlock) { }

    int maxToDisplay = maxDisplayedLeaksCount;
    psMemBlock *memBlockBackup = memBlock;
    if (maxToDisplay == -1 ) {
      for (; memBlock != NULL; memBlock = memBlock->previousBlock) {
	if ( (memBlock->refCounter > 0) &&
	     ( (persistence) || (!persistence && !memBlock->persistent) ) &&
	     (memBlock->id >= id0)) {
	  nleak++;
	}
      }
      maxToDisplay=nleak;
    }
    if (fd != NULL) {
      fprintf(fd, "Number of leaks to display: %d\n", maxToDisplay);
    }
    memBlock = memBlockBackup;

    nleak=0;
    // iterate through the block list starting with the oldest block
    for (; memBlock != NULL; memBlock = memBlock->previousBlock) {
        if ( (memBlock->refCounter > 0) &&
                ( (persistence) || (!persistence && !memBlock->persistent) ) &&
                (memBlock->id >= id0)) {

            nleak++;

	    // only print a max of 500 leaks (make this an argument)
            if ( (nleak <= maxToDisplay) && (fd != NULL) ) {
                if (nleak == 1) {
                    fprintf(fd, "# func at (file:line)  ID: X  Ref: X\n");
                }

                fprintf(fd, "%s at (%s:%d)  ID: %lu  Ref: %lu", memBlock->func, memBlock->file, (int)memBlock->lineno, (unsigned long)memBlock->id, memBlock->refCounter);
                #if defined(PS_MEM_BACKTRACE) && defined(HAVE_BACKTRACE)

                size_t size = memBlock->backtraceSize;
                char **strings = backtrace_symbols((void *const *)memBlock->backtrace, size);

                fprintf(fd, "  Alloc Call Depth: %zd\n", size);

                for (int i = 0; i < size; i++) {
                    // always ident
                    int ident = 4;  // initial indent
                    ident += 2 * i; // nesting depth
                    fprintf(fd, "%*s", ident, "");

                    // if the caller was an anon function then strchr won't
                    // find a '(' in the string and will return NULL
                    char *caller = caller = strchr(strings[i], '(');
                    if (caller) {
                        // skip over the '('
                        caller++;
                        // find the end of the symbol name
                        size_t callerLength = abs(strchr(caller, '+') - caller);
                        // print just the symbol name
                        for (int i = 0; i < callerLength; i++) {
                            fputc(caller[i],fd);
                        }
                        fprintf(fd, "\n");
                    } else {
                        fprintf(fd, "(unknown)\n");
                    }
                }

                free (strings);
                #else // ifdef HAVE_BACKTRACE
                // \n after "Memory Block ID"
                fprintf(fd, "\n");
                #endif // ifdef HAVE_BACKTRACE

            }
        }
    }

    MUTEX_UNLOCK(&memBlockListMutex);

    if (nleak == 0 || array == NULL) {
        return nleak;
    }

    *array = psAlloc(nleak * sizeof(psMemBlock));

    // this lock is rarely called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    for (psMemBlock *memBlock = topBlock; memBlock != NULL; memBlock = memBlock ->nextBlock) {
        if ( (memBlock->refCounter > 0) &&
                ( (persistence) || (!persistence && !memBlock->persistent) ) &&
                (memBlock->id >= id0)) {

            (*array)[j++] = memBlock;
            if (j == nleak) {              // found them all
                break;
            }
        }
    }

    MUTEX_UNLOCK(&memBlockListMutex);

    return nleak;
}


/*
 * Reference counting APIs
 */
psReferenceCount p_psMemGetRefCounter(const char *file,
                                      unsigned int lineno,
                                      const char *func,
                                      void *ptr)
{
    if (ptr == NULL) {
        return 0;
    }

    psMemBlock *memBlock = ((psMemBlock *) ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    return memBlock->refCounter;
}


// increment and return refCounter
void *p_psMemIncrRefCounter(const char *file,
                            unsigned int lineno,
                            const char *func,
                            void *ptr)
{
    if (ptr == NULL) {
        return ptr;
    }

    psMemBlock* memBlock = ((psMemBlock *) ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    // we need a MUTEX_LOCK here: otherwise, two functions can race on recCounter++ and
    // refCounter--;
    MUTEX_LOCK(&memBlockListMutex);
    memBlock->refCounter++;
    MUTEX_UNLOCK(&memBlockListMutex);

    // Did the user ask to be informed about this allocation?
    // this lock is frequently called in a given program
    // this lock is probably not needed: if someone changes the callback ID in a different thread,
    // do we really care about really rarely getting this wrong?
    if (memBlock->id == memAllocID) {
        MUTEX_LOCK(&memBlockListMutex);
        memAllocID += memAllocCallback(memBlock);
        MUTEX_UNLOCK(&memBlockListMutex);
    }

    return ptr;
}


#if 0
void * p_psMemSetRefCounter(void * vptr,
                            psReferenceCount count,
                            const char *file,
                            psS32 lineno)
{
    psMemBlock* ptr;

    if (vptr == NULL) {
        return vptr;
    }

    if (count < 0) {
        count = 0;
    }

    ptr = ((psMemBlock* ) vptr) - 1;

    if (isBadMemBlock(ptr, __func__)) {
        (void)p_psMemDecrRefCounter(vptr, filename, lineno);
        memProblemCallback(ptr, file, lineno);
    }

    ptr->refCounter = count;

    if (count < 1) {
        vptr = p_psMemDecrRefCounter(vptr,file,lineno);
    }

    return vptr;
}
#endif


// decrement and return refCounter
void *p_psMemDecrRefCounter(const char *file,
                            unsigned int lineno,
                            const char *func,
                            void * ptr)
{
    if (ptr == NULL) {
        return NULL;
    }

    psMemBlock *memBlock = ((psMemBlock *) ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    // if we have multiple references, just decrement the count and return.
    // we need a MUTEX_LOCK here: otherwise, two functions can race on refCounter--;
    BLOCK_LOCK (memBlock, true);
    if (memBlock->refCounter > 1) {
        memBlock->refCounter--;

        // Did the user ask to be informed about this deallocation?
        if (memBlock->id == memFreeID) {
            memFreeID += memFreeCallback(memBlock);
        }
	BLOCK_UNLOCK(memBlock, true);
        return ptr;
    }
    BLOCK_UNLOCK(memBlock, true);

    // we can't invoke freeFunc() while we're holding memBlockListMutex as it
    // may invoke psFree() itself
    if (memBlock->freeFunc != NULL) {
        memBlock->freeFunc(ptr);
    }

    // this lock is frequently set in a given program
    BLOCKSET_LOCK (memBlock, true);

    // Did the user ask to be informed about this deallocation?
    if (memBlock->id == memFreeID) {
        memFreeID += memFreeCallback(ptr);
    }

    psMemBlock *nextBlock = memBlock->nextBlock;
    psMemBlock *previousBlock = memBlock->previousBlock;

    // cut the memBlock out of the memBlock list
    if (nextBlock != NULL) {
        nextBlock->previousBlock = previousBlock;
    }
    if (previousBlock != NULL) {
        previousBlock->nextBlock = nextBlock;
    }
    if (lastMemBlockAllocated == memBlock) {
        lastMemBlockAllocated = nextBlock;
    }

    BLOCKSET_UNLOCK (memBlock, true);

    // XXX keep this?
    psAssert (memBlock->nextBlock == nextBlock, "unexpected rearrangement");
    psAssert (memBlock->previousBlock == previousBlock, "unexpected rearrangement");

    // NULL out the refs so no one can get confused
    memBlock->nextBlock = NULL;
    memBlock->previousBlock = NULL;

    // XXX these are not actually guaranteed : another thread may already grab them before we get here
    // psAssert (!memBlock->inFlight, "unreleased lock?");
    // psAssert (!nextBlock || !nextBlock->inFlight, "unreleased lock?");
    // psAssert (!previousBlock || !previousBlock->inFlight, "unreleased lock?");

    // invoke free only after we've released the block list lock as free()
    // could take awhile.  We can get away with this as at this point the
    // memBlock is no longer part of the mem block list.
    #if defined(PS_MEM_BACKTRACE) && defined(HAVE_BACKTRACE)

    free(memBlock->backtrace);
    #endif

# if (0 && PS_TRACE_ON)    
    // before freeing a particular block of memory, fill the entire block with FF to poison it
    // this should trigger any bad reactions early.  this may be costly, so on do it for
    // unoptimzed builds
    size_t totalSize = sizeof(psMemBlock) + memBlock->userMemorySize + sizeof(void *);
    memset ((void *) memBlock, 0xff, totalSize);
# endif

    free(memBlock);

    // since we freed it, make sure we return NULL.
    return NULL;
}


/**** user functions to manage memory references ****/
void p_psMemSetDeallocator(const char *file,
                           unsigned int lineno,
                           const char *func,
                           void *ptr,
                           psFreeFunc freeFunc)
{
    if (ptr == NULL) {
        return;
    }

    psMemBlock* memBlock = ((psMemBlock *)ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    memBlock->freeFunc = freeFunc;
}


psFreeFunc p_psMemGetDeallocator(const char *file,
                                 unsigned int lineno,
                                 const char *func,
                                 void *ptr)
{
    if (ptr == NULL) {
        return NULL;
    }

    psMemBlock* memBlock = ((psMemBlock *)ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    return memBlock->freeFunc;
}


bool p_psMemGetPersistent(const char *file,
                          unsigned int lineno,
                          const char *func,
                          void *ptr)
{
    if (ptr == NULL) {
        return NULL;
    }

    psMemBlock* memBlock = ((psMemBlock *) ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    return memBlock->persistent;
}


void p_psMemSetPersistent(const char *file,
                          unsigned int lineno,
                          const char *func,
                          void *ptr,
                          bool value)
{
    if (ptr == NULL) {
        return;
    }

    psMemBlock* memBlock = ((psMemBlock *) ptr) - 1;

    HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

    memBlock->persistent = value;
}

int p_psMemCheckCorruption(const char *file,
                           unsigned int lineno,
                           const char *func,
                           FILE *output,
                           bool abort_on_error)
{
    // get exclusive access to the memBlock list to avoid it changing on us while we check it.

    // this lock is rarely called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    // this function calls 'isBadMemBlock', which may in turn call this function : avoid nested calls or we will be stuck forever
    checkingForCorruption = true;

    // int nPrint = 0;
    psS32 nbad = 0;               // number of bad blocks
    for (psMemBlock *memBlock = (psMemBlock *) lastMemBlockAllocated; memBlock != NULL; memBlock = memBlock->nextBlock) {
	// if (nPrint < 20) {
	//     psMemBlockPrint (stderr, memBlock);
	//     nPrint ++;
	// }
        if (isBadMemBlock(output, memBlock, __FILE__, __LINE__, __func__)) {
            nbad++;

            if (abort_on_error) {
                // release the lock on the memblock list
                MUTEX_UNLOCK(&memBlockListMutex);
                PS_MEM_ABORT(__func__, "Detected memory corruption");
            }
        }
    }

    // revert to false after calling isBadMemBlock above
    checkingForCorruption = false;

    // release the lock on the memblock list
    MUTEX_UNLOCK(&memBlockListMutex);

    return nbad;
}

bool p_psMemIsAlloced(const char *file,
                      unsigned int lineno,
                      const char *func,
                      const void *ptr)
{
    // if ptr is a psAlloc()'d memory, find the actual address of the memBlock
    psMemBlock *addr = ((psMemBlock *)ptr) - 1;

    // get exclusive access to the memBlock list to avoid it changing on us
    // this lock is only occasionally called in a given program
    MUTEX_LOCK(&memBlockListMutex);

    // loop through the linked list of memBlocks looking for a matching pointer
    for (psMemBlock *memBlock = (psMemBlock *) lastMemBlockAllocated; memBlock != NULL; memBlock = memBlock->nextBlock) {
        if (memBlock == addr) {
            // we found the memBlock
            HANDLE_BAD_BLOCK(memBlock, file, lineno, func);

            MUTEX_UNLOCK(&memBlockListMutex);
            return true;
        }
    }

    // release the lock on the memblock list
    MUTEX_UNLOCK(&memBlockListMutex);

    return false;
}


/**** memory usage statistics functions ****/

/*
 * Return the total amount of memory owned by psLib; if non-NULL also provide a
 * breakdown into recyclable, allocated, and allocated-and-persistent
 *
 * It would be simple enough to fix this code to return an array of structs to
 * describe the insides of the allocator rather than the printf used here.
 */
size_t psMemStats(const bool print, // print details as they're found?
                  size_t *allocated, // memory that's currently allocated (but not persistent)
                  size_t *persistent) // persistent memory that's currently allocated
{
    const size_t overhead = sizeof(psMemBlock) + sizeof(void *); // overhead on each allocation

    if (print) {
        printf("Type       %6s  %10s %8s\n", "size", "nByte", "nBlock");
    }

    // this lock is rarely called in a given program
    MUTEX_LOCK(&memBlockListMutex);
    /*
     * All memory that's currently allocated, whether persistent or not
     */
    size_t allocated_s, persistent_s;
    if (allocated == NULL) {
        allocated = &allocated_s;
    }
    if (persistent == NULL) {
        persistent = &persistent_s;
    }

    size_t alloc = 0, persist = 0;
    size_t nalloc = 0, npersist = 0;
    for (psMemBlock* ptr = (psMemBlock *) lastMemBlockAllocated; ptr != NULL; ptr = ptr->nextBlock) {
        assert (ptr->refCounter > 0);

        if (ptr->persistent) {
            npersist++;
            persist += ptr->userMemorySize + overhead;
        } else {
            nalloc++;
            alloc += ptr->userMemorySize + overhead;
        }
    }
    *allocated = alloc;
    *persistent = persist;

    if (print) {
        printf("Allocated  %6s  %10zd %8zd\n", "", alloc, nalloc);
        printf("Persistent %6s  %10zd %8zd\n", "", persist, npersist);
    }

    MUTEX_UNLOCK(&memBlockListMutex);

    return *allocated + *persistent;
}

int psMemBlockPrint(FILE *output, const psMemBlock *memBlock)
{
    return fprintf(output,
                   "Memory Block ID: %lu @ %p\n"
                   "\tPrevious Block: %p Next Block: %p\n"
                   "\tFree function: %p\n"
                   "\tSize: %zd Reference count: %lu Persistent: %s\n"
                   "\tPosts: %x %x %x\n"
                   "\tAllocated in %s at (%s:%d)\n"
                   "\t\tby Thread ID %lu\n",
                   memBlock->id, memBlock,
                   memBlock->previousBlock, memBlock->nextBlock,
                   memBlock->freeFunc,
                   memBlock->userMemorySize, memBlock->refCounter, (memBlock->persistent ? "Yes" : "No"),
                   memBlock->startblock, memBlock->endblock,
                   *(psU32 *)((char *) (memBlock + 1) + memBlock->userMemorySize),
                   memBlock->func, memBlock->file, memBlock->lineno, (unsigned long)memBlock->tid);
}

int psMemBlockPrintPtr(FILE *output, void *ptr)
{
    psMemBlock *mb = (psMemBlock *)ptr - 1;
    int status = psMemBlockPrint (stderr, mb);
    return status;
}

bool psMemTypeEqual (void *ptr1, void *ptr2) {

    // if ptr is a psAlloc()'d memory, find the actual address of the memBlock
    psMemBlock *memBlock1 = ((psMemBlock *)ptr1) - 1;
    HANDLE_BAD_BLOCK(memBlock1, __FILE__, __LINE__, __func__);
    if (!memBlock1->freeFunc) return false;

    // if ptr is a psAlloc()'d memory, find the actual address of the memBlock
    psMemBlock *memBlock2 = ((psMemBlock *)ptr2) - 1;
    HANDLE_BAD_BLOCK(memBlock2, __FILE__, __LINE__, __func__);
    if (!memBlock2->freeFunc) return false;

    return (memBlock1->freeFunc == memBlock2->freeFunc);
}

bool static dumpMemory = false;

void psMemDumpSetState (bool state) {
    dumpMemory = state;
}

void psMemDump(const char *name)
{
    if (!dumpMemory) return;

    char filename[1024];	  // don't make your sub-names too long!
    static int num = 0;		  // Counter, to make files unique and give an idea of sequence

    snprintf (filename, 1024, "memdump_%s_%03d.txt", name, num);
    FILE *memFile = fopen(filename, "w");

    psMemBlock **leaks = NULL;
    int numLeaks = psMemCheckLeaks(0, &leaks, NULL, true);
    fprintf(memFile, "# MemBlock Size Source\n");
    unsigned long total = 0;            // Total memory used
    for (int i = 0; i < numLeaks; i++) {
        psMemBlock *mb = leaks[i];
        fprintf(memFile, "%12lu\t%12zd\t%p\t%p\t%s:%d\n", mb->id, mb->userMemorySize, (mb + 1), (char *) (mb + 1) + mb->userMemorySize, mb->file, mb->lineno);
        total += mb->userMemorySize;
    }
    fclose(memFile);
    psFree(leaks);

    // fprintf(stderr, "Memdump %s %d: Memory use: %ld, sbrk: %p\n", name, num, total, (void *) sbrk(0));
    fprintf(stderr, "Memdump %s %d: Memory use: %ld\n", name, num, total);
    num++;
}

static FILE *memDumpFile = NULL;

void psMemDumpBigBlocks (psMemBlock *mb, char *mode) {

    if (!memDumpFile) {
	memDumpFile = fopen ("memdump.txt", "w");
	psAssert (memDumpFile, "failed to open memdump.txt");
    }

    if (mb->userMemorySize < 1) return;
    // fprintf(memDumpFile, "%12lu %12zd %16p %16p %8s %s:%d\n", mb->id, mb->userMemorySize, (mb + 1), (char *) (mb + 1) + mb->userMemorySize, mode, mb->file, mb->lineno);
    fprintf (memDumpFile, "--- %s ---\n", mode);
    psMemBlockPrint (memDumpFile, mb);
    fprintf (memDumpFile, "-----------\n");
    return;
}

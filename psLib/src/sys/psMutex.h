#ifndef PS_MUTEX_H
#define PS_MUTEX_H

#include <pthread.h>
#include <assert.h>

// We desire that these functions be fast (they may get called a lot), so we've defined them as macros.

//#define PS_MUTEX_CAREFUL                // Use error-checking mutexes, trace messages, check the results

typedef pthread_mutex_t psMutex;        /// Mutual Exclusion using pthreads

// Set type of mutex to use based on what sort of system we're using.
// GNU systems define a "fast" mutex that is non-portable (i.e., non-standard) that we'd like to use if we can
#ifdef __USE_GNU
#ifdef PS_MUTEX_CAREFUL

// These bells and whistles are only defined if PS_MUTEX_CAREFUL and __USE_GNU are both defined

// Error-checking mutex
#define PS_MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK_NP

#define PS_MUTEX_TRACE(LEVEL, STRING, PTR) psTrace("psLib.sys.mutex", LEVEL, STRING, #PTR, (unsigned long) PTR); // Trace message

// Trace message with thread and pointer
#define PS_MUTEX_TRACE_THREAD(LEVEL, STRING, PTR) \
    psTrace("psLib.sys.mutex", LEVEL, STRING, (void*)pthread_self(), #PTR, (unsigned long) PTR);

// Check for dead-locked mutex
#define PS_MUTEX_CHECK_DEADLOCK(PTR) \
    if (pml == EDEADLK) { \
        psAbort("Deadlocked mutex on %s (%lx)", #PTR, (unsigned long) PTR); \
    }

// Check for stolen mutex
#define PS_MUTEX_CHECK_STOLEN(PTR) \
    if (pmu == EPERM) { \
        psAbort("Unlocking stolen mutex on %s (%lx)", #PTR, (unsigned long) PTR);	\
    }

#else  // PS_MUTEX_CAREFUL

#define PS_MUTEX_TYPE PTHREAD_MUTEX_FAST_NP // Fast mutex
#define PS_MUTEX_TRACE(LEVEL, STRING, PTR) // No action
#define PS_MUTEX_TRACE_THREAD(LEVEL, STRING, PTR) // No action
#define PS_MUTEX_CHECK_DEADLOCK(PTR)    // No action
#define PS_MUTEX_CHECK_STOLEN(PTR)      // No action

#endif // PS_MUTEX_CAREFUL
#else  // __USE_GNU

#define PS_MUTEX_TYPE PTHREAD_MUTEX_DEFAULT // Default mutex type: portable
#define PS_MUTEX_TRACE(LEVEL, STRING, PTR) // No action
#define PS_MUTEX_TRACE_THREAD(LEVEL, STRING, PTR) // No action
#define PS_MUTEX_CHECK_DEADLOCK(PTR)    // No action
#define PS_MUTEX_CHECK_STOLEN(PTR)      // No action

#endif


/// Initialize a mutex
#define psMutexInit(PTR) { \
    psAssert(PTR, "Require non-NULL pointer to initialise mutex"); \
    PS_MUTEX_TRACE(3, "Initialising mutex on %s (%lx)...", PTR); \
    /* Gotta go the long way, since we can't use PTHREAD_MUTEX_INITIALIZER after we declare the variable */ \
    pthread_mutexattr_t attr; /* Mutex attributes */ \
    pthread_mutexattr_init(&attr); \
    pthread_mutexattr_settype(&attr, PS_MUTEX_TYPE); \
    pthread_mutex_init(&(PTR)->lock, &attr); \
    pthread_mutexattr_destroy(&attr); \
}

/// Lock a mutex
#define psMutexLock(PTR) { \
    psAssert(PTR, "Require non-NULL pointer to lock mutex"); \
    PS_MUTEX_TRACE_THREAD(1, "Thread %p waiting for mutex lock on %s (%lx)...", PTR); \
    int pml = pthread_mutex_lock(&(PTR)->lock); \
    PS_MUTEX_TRACE_THREAD(1, "Thread %p locked mutex on %s (%lx)", PTR); \
    if (pml == EINVAL) { \
        psAbort("Cannot lock mutex on %s (%lx): not initialised", #PTR, (unsigned long) PTR); \
    } \
    PS_MUTEX_CHECK_DEADLOCK(PTR); \
}

/// Unlock a mutex
#define psMutexUnlock(PTR) { \
    psAssert(PTR, "Require non-NULL pointer to unlock mutex"); \
    PS_MUTEX_TRACE_THREAD(1, "Thread %p unlocking mutex on %s (%lx)", PTR); \
    int pmu = pthread_mutex_unlock(&(PTR)->lock); \
    if (pmu == EINVAL) { \
        psAbort("Cannot unlock mutex on %s (%lx): not initialised", #PTR, (unsigned long) PTR); \
    } \
    PS_MUTEX_CHECK_STOLEN(PTR); \
}

/// Destroy a mutex
#define psMutexDestroy(PTR) { \
    psAssert(PTR, "Require non-NULL pointer to destroy mutex"); \
    if (pthread_mutex_destroy(&(PTR)->lock) == EBUSY) { \
        psAbort("Cannot destroy mutex on %s (%lx): is busy", #PTR, (unsigned long) PTR);	\
    } \
}

#endif

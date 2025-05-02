/** @file  psThread.h
 *
 *  @brief tools to manage a pool of threads
 *
 *  @author EAM, IFA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-30 02:23:09 $
 *
 *  Copyright 2004-2005 Insitute for Astronomy, University of Hawaii
 */

#ifndef PS_THREAD_H
#define PS_THREAD_H

#include <pthread.h>
#include "psString.h"
#include "psArray.h"

/// @addtogroup SysUtils System Utilities
/// @{

/// Job to be executed on a thread
///
/// This job is passed to the function that executes it
typedef struct {
    psString type;                      // Type of thread
    psArray *args;                      // Arguments to job
    psArray *results;                   // Results of job
} psThreadJob;

#define PS_ASSERT_THREAD_JOB_NON_NULL(JOB, RVAL) \
if (!(JOB) || !(JOB)->type || !(JOB)->args) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Thread job %s or one of its components is NULL.", #JOB); \
    return RVAL; \
}

/// A thread, which executes a job
///
/// Wraps pthread with a few extra conveniences
typedef struct {
    bool busy;                          // Is the thread busy?
    bool fault;                         // Has the thread faulted?
} psThread;

/// Function to execute a thread job
typedef bool (*psThreadTaskFunction)(psThreadJob *job);

/// Task that is executed on a thread
typedef struct {
    psString type;                      // Type of task
    int nArgs;                          // Number of arguments that function takes
    psThreadTaskFunction function;      // Function to execute
} psThreadTask;

#define PS_ASSERT_THREAD_TASK_NON_NULL(TASK, RVAL) \
if (!(TASK) || !(TASK)->type || (TASK)->nArgs < 0 || !(TASK)->function) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Thread task %s or one of its components is NULL.", #TASK); \
    return RVAL; \
}


/// Lock the thread mutex
void psThreadLock(void);

/// Unlock the thread mutex
void psThreadUnlock(void);

/// Allocate a thread
psThread *psThreadAlloc(void);

/// Allocate a thread job
psThreadJob *psThreadJobAlloc(const char *type);

/// Add a pending job to the queue
///
/// This function swallows the provided job, so that the user no longer owns it.  This is because freeing the
/// job is not thread-safe (its reference count is being changed within the threads) so we handle it ourselves
/// and absolve the user from all responsibility.  If the user stores the job, he should only access it while
/// threads are processing in code protected by psThreadLock/psThreadUnlock.
bool psThreadJobAddPending(psThreadJob *job);

/// Get a job off the queue of pending jobs
///
/// This function is not thread-safe.  Protect with psThreadLock/psThreadUnlock if threads are running.
psThreadJob *psThreadJobGetPending(void);

/// Get a job off the queue of done jobs
///
/// This function is not thread-safe.  Protect with psThreadLock/psThreadUnlock if threads are running.
psThreadJob *psThreadJobGetDone(void);

// utility function to access the pending queue
psList *psThreadGetPendingQueue ();

/// Allocate a thread task
psThreadTask *psThreadTaskAlloc(const char *type, // Type of task
                                int nArgs // Number of arguments
    );

/// Add a task to the list
bool psThreadTaskAdd(psThreadTask *task // Task to add
    );

/// Remove a task from the list
bool psThreadTaskRemove(const char *type // Task type to remove
    );

/// Launch jobs on a thread
void *psThreadLauncher(void *thread     // Thread (of type psThread)
    );

/// Initialise a pool of threads
bool psThreadPoolInit(int nThreads      // Number of threads
    );

/// Return size of thread pool
int psThreadPoolSize(void);

/// Wait for the thread pool to finish
///
/// This function blocks (waits in usleep) until all  threads are idle and no jobs
/// are left on the queue
/// returns success if all jobs return success, otherwise returns false
bool psThreadPoolWait(bool harvest,         // Harvest the jobs from the queue?
                      bool harvestOnFailure // If harvest is false, harvest the jobs if a failure is encountered
    );

/// Clean up the thread pool
bool psThreadPoolFinalize(void);

#if 0
/// Add thread-specific data
///
/// The provided pointer is added to a thread-specific hash under the provided name.
bool psThreadDataAdd(const char *name,  // Name of data
                     psPtr ptr          // Data to add
    );

/// Lookup thread-specific data
///
/// The thread-specific hash is interrogated using the provided name.
void *psThreadDataLookup(const char *name // Name of data
    );

/// Remove thread-specific data
///
/// The thread-specific hash has the data associated with the provided name deleted.
bool psThreadDataRemove(const char *name // Name of data
    );
#endif

/// @}
#endif /* PS_THREAD_H */

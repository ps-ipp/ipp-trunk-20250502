#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

// Backtrace to help nail down bugs
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#include <stdlib.h>
#define BACKTRACE_BUFFER_SIZE 256       // Maximum size of backtrace
static void **bt_buffer = NULL;         // Backtrace buffer
static int bt_size = 0;                 // Backtrace buffer size
#endif

#include "psAssert.h"
#include "psConstants.h"
#include "psAbort.h"
#include "psMemory.h"
#include "psList.h"
#include "psArray.h"
#include "psHash.h"
#include "psString.h"
#include "psThread.h"

#define THREAD_WAIT 10000               // Microseconds to wait for threads
#define TASK_BUCKETS 8                  // Number of hash buckets for task list

// Mutex covers:
// * pending queue
// * done queue
// * thread->busy states
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for locking threads

static volatile psList *pending = NULL; // queue of pending jobs
static volatile psList *done = NULL;    // queue of done jobs
static pthread_t *threads = NULL;       // array of the POSIX thread handles
static psArray *pool = NULL;            // array of defined threads
static psHash *tasks = NULL;            // List of defined tasks
static psArray *tsd = NULL;             // Thread-specific data

/***** basic thread functions *****/

void psThreadLock(void)
{
    pthread_mutex_lock(&mutex);
    return;
}

void psThreadUnlock(void)
{
    pthread_mutex_unlock(&mutex);
    return;
}

static void threadFree(psThread *thread)
{
    // Nothing to free; this function is merely provided for identification purposes
    return;
}

// allocate a psThread
psThread *psThreadAlloc(void)
{
    psThread *thread = (psThread*)psAlloc(sizeof(psThread));
    psMemSetDeallocator(thread, (psFreeFunc)threadFree);

    thread->busy  = false;
    thread->fault = false;
    return thread;
}

/***** thread job functions *****/

static void threadJobFree(psThreadJob *job)
{
    psFree(job->type);
    psFree(job->args);
    psFree(job->results);
    return;
}

// allocate a psThreadJob of the given type
psThreadJob *psThreadJobAlloc(const char *type)
{
    psThreadJob *job = (psThreadJob *)psAlloc(sizeof(psThreadJob));
    psMemSetDeallocator(job, (psFreeFunc)threadJobFree);

    job->type = psStringCopy(type);
    job->args = psArrayAllocEmpty(16);
    job->results = NULL;
    return job;
}

// add a job to the queue of pending jobs
bool psThreadJobAddPending(psThreadJob *job)
{
    if (!job) {
        // Asking for a no-op
        return true;
    }

    psThreadTask *task = psHashLookup(tasks, job->type); // Task to execute job
    psAssert(task, "Unable to find task %s", job->type);
    psAssert(job->args->n == task->nArgs, "invalid number of arguments to %s", task->type);

    // if we failed to call psThreadPoolInit, or we called it with nThreads == 0,
    // find the matching function and just run it.
    if (!pool || !pool->n) {

        // in non-threaded operation, the job is placed on the done list and immediately run
        if (!done) {
            done = psListAlloc(NULL);
        }
        psListAdd((psList *)done, PS_LIST_TAIL, job);
        psFree(job);
        return task->function(job);
    }

    psThreadLock();
    if (!pending) {
        pending = psListAlloc(NULL);
    }
    psListAdd((psList *)pending, PS_LIST_TAIL, job);
    psFree(job);
    psThreadUnlock();

    return true;
}

// this function is not locked -- see thread launder for example
psThreadJob *psThreadJobGetPending(void)
{
    if (!pending) {
        return NULL;
    }

    psThreadJob *job = psListGetAndRemove((psList *)pending, PS_LIST_HEAD);
    return job;
}

// this function is not locked -- see thread launcher for example
psThreadJob *psThreadJobGetDone(void)
{
    if (!done) {
        return NULL;
    }

    psThreadJob *job = psListGetAndRemove((psList *) done, PS_LIST_HEAD);
    return job;
}

psList *psThreadGetPendingQueue () {
  return (psList *) pending;
}

/***** thread task functions *****/

static void threadTaskFree(psThreadTask *task)
{
    psFree(task->type);
    return;
}

// allocate a psThreadTask with nArgs arguments
psThreadTask *psThreadTaskAlloc(const char *type, int nArgs)
{
    psThreadTask *task = (psThreadTask *)psAlloc(sizeof(psThreadTask));
    psMemSetDeallocator(task, (psFreeFunc)threadTaskFree);

    task->type = psStringCopy(type);
    task->nArgs = nArgs;
    task->function = NULL;
    return task;
}

// add a task to the collection of tasks
bool psThreadTaskAdd(psThreadTask *task)
{
    PS_ASSERT_THREAD_TASK_NON_NULL(task, false);

    // fprintf(stderr, "adding task %s\n", task->type);

    if (!tasks) {
        tasks = psHashAlloc(TASK_BUCKETS);
    }

    return psHashAdd(tasks, task->type, task);
}

bool psThreadTaskRemove(const char *type)
{
    PS_ASSERT_STRING_NON_EMPTY(type, false);
    // fprintf(stderr, "removing task %s\n", type);

    return psHashRemove(tasks, type);
}

// each thread runs this function to choose the task functions
void *psThreadLauncher(void *thread)
{
    psThread *self = thread;            // Thread that's running

    while (1) {
        // if we get an error, just wait until we are cleared or killed
        while (self->fault) {
            usleep(THREAD_WAIT);
        }

        // if no tasks are assigned, just wait until they are
        while (!tasks) {
            usleep(THREAD_WAIT);
        }

        // request a new job, if there are none available, sleep a bit
        // we have to lock here so the job queue cannot be empty yet no threads busy
        psThreadJob *job = NULL;        // Job to process
        psThreadLock();
        while ((job = psThreadJobGetPending()) == NULL) {
            // Unlock while sleeping, then lock to read the pending queue again
            psThreadUnlock();
            usleep(THREAD_WAIT);
            psThreadLock();
        }
        self->busy = true;
        psThreadUnlock();

        psThreadTask *task = psHashLookup(tasks, job->type); // Task to execute job
        // fprintf(stderr, "launching job %s\n", job->type);
#ifdef HAVE_BACKTRACE
        if (!task && bt_buffer) {
            psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace of waiter:\n");
            char **strings = backtrace_symbols((void *const *)bt_buffer, bt_size);
            psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace depth: %d", bt_size);
            for (int i = 0; i < bt_size; i++) {
                char *caller = strchr(strings[i], '(');
                if (caller) {
                    caller++;
                    size_t callerLength = abs(strchr(caller, '+') - caller);
                    psString name = psStringNCopy(caller, callerLength);
                    psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace %d: %s", i, name);
                    psFree(name);
                } else {
                    psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace %d: (unknown)", i);
                }
            }
        }
#endif
        psAssert(task, "Couldn't find thread task %s", job->type);
        psAssert(job->args->n == task->nArgs,
                 "invalid number of arguments to %s (%ld supplied, expected %d)",
                 task->type, job->args->n, task->nArgs);
        // fprintf(stderr, "    thread for %s %p launching on %p\n", job->type, task->function, self);

        // Run the job's function
        bool status = task->function(job); // Status of executing task

        // fprintf(stderr, "    thread for %s %p finished on %p with status %d\n", job->type, task->function, self, status);

        // Put the completed job on the 'done' queue
        psThreadLock();
        if (!done) {
            done = psListAlloc(NULL);
        }
        psListAdd((psList *)done, PS_LIST_TAIL, job);
        psFree(job);

        if (!status) {
            self->fault = true;
        }
        self->busy = false;
        psThreadUnlock();
    }
}

/***** thread pool functions *****/

// create a pool of Nthreads, each running the user's job-launcher function
bool psThreadPoolInit(int nThreads)
{
    if (pool || threads) {
        psAbort("psThreadsInit already called");
    }

    PS_ASSERT_INT_NONNEGATIVE(nThreads, false);
    if (nThreads == 0) {
        // No threading
        return true;
    }

    pool = psArrayAlloc(nThreads);
    threads = psAlloc(nThreads * sizeof(pthread_t));
    for (int i = 0; i < nThreads; i++) {
        psThread *thread = pool->data[i] = psThreadAlloc(); // Thread for pool
	// XXX these threads are not destroyed, creating a minor leak
        if (pthread_create(&threads[i], NULL, psThreadLauncher, thread)) {
            psAbort("Unable to create thread");
        }
    }
    return true;
}

int psThreadPoolSize(void)
{
    return pool ? pool->n : 0;
}

// Harvest jobs from the done list
static void psThreadJobHarvest(void)
{
    psThreadJob *job;           // Job from done queue
    while ((job = psThreadJobGetDone())) {
        psFree(job);
    }
    return;
}

// call this function after you have added jobs to the queue and
bool psThreadPoolWait(bool harvest, bool harvestOnFailure)
{
    // fprintf(stderr, "psThreadPoolWait called with harvest: %d\n", harvest);
    if (!pool || pool->n == 0) {
        // No threads initialised, so everything's done
        // Ensure everything is harvested, if requested
        if (harvest) {
            // No threads, no no need to lock
            psThreadJobHarvest();
        }
        return true;
    }

#ifdef HAVE_BACKTRACE
    if (bt_buffer) {
        psFree(bt_buffer);
    }
    bt_buffer = psAlloc(BACKTRACE_BUFFER_SIZE * sizeof(void *));
    bt_size = backtrace(bt_buffer, BACKTRACE_BUFFER_SIZE);
#endif

    // accumulate the number of faulted jobs that we encounter
    int numFaults = 0;
    while (1) {
        // check for an error
        for (int i = 0; i < pool->n; i++) {
            psThread *thread = pool->data[i];
            if (thread->fault) {
		// we had a fault on this thread -- clear the fault and keep going, but record
		// the fault.
                numFaults++;
		thread->fault = false;
            }
        }

        psThreadLock();

        // Harvest jobs in the background, if requested
        if (harvest) {
            psThreadJobHarvest();
        }

        // are all threads idle?
        for (int i = 0; i < pool->n; i++) {
            psThread *thread = pool->data[i];
            if (thread->busy) {
                // At least one thread is busy: sleep.
                goto SLEEP;
            }
        }

        if (!pending || !pending->head) {
            // Nothing in the queue and nothing more to add
            // Ensure everything is harvested, if requested
            if (harvest || (numFaults && harvestOnFailure)) {
                psThreadJobHarvest();
            }
            psThreadUnlock();
            return numFaults == 0;
        }

    SLEEP:
        psThreadUnlock();
        usleep(THREAD_WAIT);
    }

    return false;
}

bool psThreadPoolFinalize(void)
{
    psThreadLock();
    psFree(pending);
    pending = NULL;

    psFree(done);
    done = NULL;

    psFree(pool);
    pool = NULL;

    psFree(threads);
    threads = NULL;

    psFree(tasks);
    tasks = NULL;

    psFree(tsd);
    tsd = NULL;

#ifdef HAVE_BACKTRACE
    if (bt_buffer) {
        psFree(bt_buffer);
    }
#endif

    psThreadUnlock();

    return true;
}


#if 0
// This doesn't work like I thought it would: pthread_self can return anything.
bool psThreadDataAdd(const char *name, psPtr ptr)
{
    PS_ASSERT_STRING_NON_EMPTY(name, false);
    PS_ASSERT_PTR_NON_NULL(ptr, false);

    pthread_key_t *key = psAlloc(sizeof(pthread_key_t)); // Key for data
    pthread_key_create(key, psFree);

    if (!tsd) {
        tsd = psArrayAlloc(numThreads);
    }

    psHashAdd(

    pthread_t tid = pthread_self();     // Thread identifier
    int numThreads = psThreadPoolSize();// Number of threads
    psAssert(tid < numThreads, "Thread identifier (%d) exceeds number of threads (%d)", (int)tid, numThreads);

    if (!tsd) {
        tsd = psArrayAlloc(numThreads);
    }

    psHash *hash = tsd->data[tid];      // Thread-specific hash of data
    return psHashAdd(hash, name, ptr);
}

void *psThreadDataLookup(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    if (!tsd) {
        return NULL;
    }

    pthread_t tid = pthread_self();     // Thread identifier
    int numThreads = psThreadPoolSize();// Number of threads
    psAssert(tid < numThreads, "Thread identifier (%d) exceeds number of threads (%d)", (int)tid, numThreads);

    psHash *hash = tsd->data[tid];      // Thread-specific hash of data
    return psHashLookup(hash, name);
}

bool psThreadDataRemove(const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    if (!tsd) {
        return false;
    }

    pthread_t tid = pthread_self();     // Thread identifier
    int numThreads = psThreadPoolSize();// Number of threads
    psAssert(tid < numThreads, "Thread identifier (%d) exceeds number of threads (%d)", (int)tid, numThreads);

    psHash *hash = tsd->data[tid];      // Thread-specific hash of data
    return psHashRemove(hash, name);
}
#endif

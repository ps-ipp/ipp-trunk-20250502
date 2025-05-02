/** @file  psError.c
 *
 *  @brief Contains the definitions for the error reporting functions
 *
 *  Error reporting functions shall be used to create log entries in the
 *  event errors are detected.  The messages shall give enough information
 *  to allow the user to know where the error has occurred and the type
 *  of error detected.
 *
 *  @author Joshua Hoblitt, University of Hawaii
 *  @author Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.48 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-12 22:53:34 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>
#include <pthread.h>
#include <string.h>

#include "psLogMsg.h"
#include "psError.h"
#include "psString.h"
#include "psTrace.h"
#include "psAbort.h"
#include "psMemory.h"

#define MAX_STRING_LENGTH 2048
#define MAX_ERROR_STACK_SIZE 64

static pthread_mutex_t lockErrorStack = PTHREAD_MUTEX_INITIALIZER;
static bool errorStackKeyInitialized = false;
static pthread_key_t errorStack_key;

static void psFreeWrapper(void *ptr);
static void psErrorStackPush(psErr* err);
static psArray *psErrorStackGet(void);
static void psErrFree(psErr* err);

// needed for pthread_key_create() because p_psFree() does not match free()'s
// prototype
// XXX does something like this need to be global in psMmemory.h
static void psFreeWrapper(void *ptr)
{
    psFree(ptr);
}

static void psErrorStackPush(psErr* err)
{
    psArray *errorStack = psErrorStackGet();

    // push the item onto the stack and increment the error count
    if (psArrayLength(errorStack) < MAX_ERROR_STACK_SIZE) {
        // make the psErr persistent
        psMemSetPersistent(err, true);
        psMemSetPersistent(err->msg, true);
        psMemSetPersistent(err->name, true);

        psArrayAdd(errorStack, 0, err);
    } else {
        psAbort("attempt to exceed maximum error stack depth of %ld",
                (long)MAX_ERROR_STACK_SIZE);
    }
}

static psArray *psErrorStackGet(void)
{
    // check to see if the error stack key has been initialized
    pthread_mutex_lock(&lockErrorStack);
    if (errorStackKeyInitialized == false) {
        // note that each error stack will automatically be free'd when it's
        // thread exits
        if (pthread_key_create(&errorStack_key, psFreeWrapper)) {
            psAbort("pthread_key_create failed()");
        }
        errorStackKeyInitialized = true;
    }
    pthread_mutex_unlock(&lockErrorStack);

    // check to see if the error stack for this thread has been allocated
    psArray *errorStack = NULL;
    if ((errorStack = pthread_getspecific(errorStack_key)) == NULL) {
        // allocate the error stack
        errorStack = psArrayAllocEmpty(MAX_ERROR_STACK_SIZE);
        psMemSetPersistent(errorStack, true);
        psMemSetPersistent(errorStack->data, true);
        // store this threads error stack
        // note that pthread_setspecifc() does not take a pointer as the first
        // param
        if (pthread_setspecific(errorStack_key, errorStack)) {
            psAbort("pthread_setspecific() failed");
        }
    }

    return errorStack;
}

// This function serves the same purpose of psErrorStackGet() accept it does
// not alloc the error stack if it DOES NOT already exist.  Unlike
// psErrorStackGet(), this function is NOT guarenteed to return a valid pointer
// and it's return status must be checked.
static psArray *psErrorStackGetNoAlloc(void)
{
    // check to see if the error stack key has been initialized
    pthread_mutex_lock(&lockErrorStack);
    if (errorStackKeyInitialized == false) {
        pthread_mutex_unlock(&lockErrorStack);
        return NULL;
    }
    pthread_mutex_unlock(&lockErrorStack);

    // check to see if the error stack for this thread has been allocated
    psArray *errorStack = NULL;
    if ((errorStack = pthread_getspecific(errorStack_key)) == NULL) {
        return NULL;
    }

    return errorStack;
}

static void psErrFree(psErr* err)
{
    if (err != NULL) {
        psFree(err->msg);
        psFree(err->name);
    }
}

psErr* psErrAlloc(const char* name, psErrorCode code, const char* msg)
{
    psErr* err = psAlloc(sizeof(psErr));
    err->msg = psStringCopy(msg);
    err->name = psStringCopy(name);
    err->code = code;

    psMemSetDeallocator(err, (psFreeFunc)psErrFree);

    return err;
}

psErrorCode p_psErrorV(const char* filename,
                       unsigned int lineno,
                       const char* func,
                       psErrorCode code,
                       bool new,
                       const char* format,
                       va_list ap)
{
  char errMsg[MAX_STRING_LENGTH];
    char msgName[MAX_STRING_LENGTH];

    // if this the origin of a new error reset the error stack
    if (new) {
        psErrorClear();
    }

    snprintf(msgName, MAX_STRING_LENGTH, "%s (%s:%d)", func, filename, lineno);
    vsnprintf(errMsg, MAX_STRING_LENGTH, format, ap);

    // Remove a single trailing \n from message -- it interferes with
    // psErrorStackPrint
    size_t len = strlen(errMsg);
    if (len > 0 && errMsg[len - 1] == '\n') {
        errMsg[len - 1] = '\0';
    }

    psErr *err = psErrAlloc(msgName, code, errMsg);
    psErrorStackPush(err);

    #ifndef PS_NO_TRACE
    // Call tracing function with PS_LOG_ERROR level
    // p_psTrace() automatically appends the the function name to the facility
    // for us
    p_psTrace(__FILE__, __LINE__, func, "err", PS_LOG_ERROR, "%s : %s", err->name, err->msg);
    #endif

    psFree(err);

    return code;
}

psErrorCode p_psError(const char* filename,
                      unsigned int lineno,
                      const char* func,
                      psErrorCode code,
                      bool new,
                      const char* format,
                      ...)
{
    va_list ap;
    va_start(ap, format);
    p_psErrorV(filename, lineno, func, code, new, format, ap);
    va_end(ap);
    return code;
}

void p_psWarning(const char* file,
                 int lineno,
                 const char* func,
                 const char* format,
                 ...)
{
    char msgName[MAX_STRING_LENGTH];

    snprintf(msgName, MAX_STRING_LENGTH, "%s (%s:%d)", func, file, lineno);

    va_list ap;
    va_start(ap, format);

    psLogMsgV(msgName, PS_LOG_WARN, format, ap);

    va_end(ap);

    return;
}

psErr* psErrorGet(long which)
{
    psErr* result;

    psArray *errorStack = psErrorStackGet();

    // Check for negative reference and if found return PS_ERR_NONE
    if (which < 0 ) {
        result = psErrAlloc("", PS_ERR_NONE, "");
    } else {
        // the which input is from the end of errorStack
        which = psArrayLength(errorStack) - 1 - which;
        if (which < 0 || which >= psArrayLength(errorStack)) {
            // no error at the given location
            result = psErrAlloc("", PS_ERR_NONE, "");
        } else {
            // a new reference passed back
            result = psMemIncrRefCounter(errorStack->data[which]);
        }
    }

    return result;
}

long psErrorGetStackSize(void)
{
    psArray *errorStack = psErrorStackGet();

    return psArrayLength(errorStack);
}

psErr* psErrorLast(void)
{
    return psErrorGet(0);
}

psErrorCode psErrorCodeLast(void)
{
    psErr *err = psErrorGet(0);
    psErrorCode code = err->code;
    psFree(err);

    return code;
}

void psErrorClear(void)
{
    psArray *errorStack = psErrorStackGet();

    for (long i = 0; i < psArrayLength(errorStack); i++) {
        psErr *err = errorStack->data[i];

        psMemSetPersistent(err, false);
        psMemSetPersistent(err->msg, false);
        psMemSetPersistent(err->name, false);
    }

    psArrayElementsFree(errorStack);
}

void psErrorStackPrint(FILE *fd, const char *format, ...)
{
    if (fd == NULL) {
        fd = stdout;
    }

    va_list ap;             // variable list arguement pointer

    va_start(ap, format);

    psErrorStackPrintV(fd, format, ap);

    va_end(ap);
}

// This function does not allocate any memory so it is safe to call from inside
// of psMemory.c.  Do not allocate memory in function (or call any functions
// that do) without first removing it's use from psMemory.c.
void psErrorStackPrintV(FILE *fd, const char *format, va_list va)
{
    psArray *errorStack = psErrorStackGetNoAlloc();
    // do nothing if the error stack has not been allocated
    if (!errorStack) {
        return;
    }

    vfprintf(fd, format, va);
    fprintf (fd, "\n");

    for (long i = 0; i < psArrayLength(errorStack); i++) {
        psErr *err = errorStack->data[i];
        if(err->code >= PS_ERR_BASE) {
            fprintf(fd," -> %s: %s\n     %s\n",
                    err->name,
                    psErrorCodeString(err->code),
                    err->msg);
        } else {
            fprintf(fd," -> %s: %s\n     %s\n",
                    err->name,
                    strerror(err->code),
                    err->msg);
        }
    }
}


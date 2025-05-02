
/** @file  psAbort.c
 *
 *  @brief Contains the definition for abort function
 *
 *  The abort logging and handling shall be performed by psAbort function.
 *  This will allow for consistent handling of other software units
 *  needing to abort from program execution.
 *
 *  @author Eric Van Alst, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-04-13 08:18:27 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(HAVE_BACKTRACE)
#include <execinfo.h>
#define BACKTRACE_BUFFER_SIZE 256       // Maximum size of backtrace
#endif

#include "psAbort.h"
#include "psString.h"
#include "psMemory.h"
#include "psError.h"
#include "psLogMsg.h"

// Write backtrace to log
static inline void psBacktrace(void)
{
#ifdef HAVE_BACKTRACE
    void **bt = psAlloc(BACKTRACE_BUFFER_SIZE * sizeof(void *)); // Backtrace information
    if (!bt) {
        psLogMsg("psLib.sys", PS_LOG_ABORT, "Unable to allocate memory for backtrace");
        return;
    }
    int size = backtrace(bt, BACKTRACE_BUFFER_SIZE);             // Size of backtrace
    char **strings = backtrace_symbols((void *const *)bt, size);
    psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace depth: %d", size);
    for (int i = 0; i < size; i++) {
        // if the caller was an anon function then strchr won't
        // find a '(' in the string and will return NULL
        char *caller = strchr(strings[i], '(');
        if (caller) {
            // skip over the '('
            caller++;
            // find the end of the symbol name
            size_t callerLength = abs(strchr(caller, '+') - caller);
            psString name = psStringNCopy(caller, callerLength);
            psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace %d: %s", i, name);
            psFree(name);
        } else {
            psLogMsg("psLib.sys", PS_LOG_ABORT, "Backtrace %d: (unknown)", i);
        }
    }
#endif // ifdef HAVE_BACKTRACE
}

void p_psAbort(const char *file,
               unsigned int lineno,
               const char *func,
               const char *format,
               ...)
{
    psErrorStackPrint(stderr, "Aborting in function %s at %s:%d. Error stack:", func, file, lineno);

    va_list argPtr;             // variable list arguement pointer
    // Get the variable list parameters to pass to logging function
    va_start(argPtr, format);

    // Call logging function with PS_LOG_ABORT level
    psLogMsgV("psLib.sys", PS_LOG_ABORT, format, argPtr);

    // Clean up stack after variable arguement has been used
    va_end(argPtr);

    psBacktrace();

    // Call system abort function to terminate program execution
    fsync(psLogGetDestination());
    abort();
}

void p_psAssert(const char *file,
                unsigned int lineno,
                const char *func,
                const bool value,
                const char *format,
                ...)
{
    if (value) return;
    psErrorStackPrint(stderr, "Assertion failed in function %s at %s:%d. Error stack:", func, file, lineno);

    va_list argPtr;             // variable list arguement pointer
    // Get the variable list parameters to pass to logging function
    va_start(argPtr, format);

    // Call logging function with PS_LOG_ABORT level
    psLogMsgV("psLib.sys", PS_LOG_ABORT, format, argPtr);

    // Clean up stack after variable arguement has been used
    va_end(argPtr);

    psBacktrace();

    // Call system abort function to terminate program execution
    fsync(psLogGetDestination());
    abort();
}

/** @file psTrace.h
 *
 *  @brief basic run-time trace facilities
 *
 *  This file will hold the prototypes for defining procedures to insert
 *  trace messages into the code.
 *
 *  @author Robert Lupton, Princeton University
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.59 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-09 01:40:07 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_TRACE_H
#define PS_TRACE_H 1

/// @addtogroup SysUtils System Utilities
/// @{

#include <stdarg.h>
#include "psMetadata.h"

#define PS_UNKNOWN_TRACE_LEVEL -9999   // we don't know this name's level
#define PS_DEFAULT_TRACE_LEVEL -1
#define PS_THE_OTHER_DEFAULT_TRACE_LEVEL 0

enum {
    PS_TRACE_TO_NONE = 0,             ///< turn off all traces
    PS_TRACE_TO_STDOUT = 1,           ///< trace to system's stdout
    PS_TRACE_TO_STDERR = 2,           ///< trace to system's stderr
};

/** Functions **************************************************************/

//#define PS_NO_TRACE 1   ///< to turn off all tracing

// XXX EAM : the old 'empty' values of (void) 0 are dangerous
# if defined(PS_NO_TRACE)
#   define psTraceSetFormat(format)     true    /* success */
#   define psTrace(facil, level, ...)           /* do nothing */
#   define psTraceGetLevel(facil)       0       /* trace level is always 0 */
#   define psTraceV(facil, level, format, __VA_LIST)    /* do nothing */
#   define psTraceSetLevel(facil,level) 0       /* previous level was 0 */
#   define psTraceReset()                       /* do nothing */
#   define psTracePrintLevels()                 /* do nothing */
#   define psTraceSetDestination(fp)    true    /* success */
#   define psTraceGetDestination()      2       /* destination is stderr */
#   define psTraceLevels()              psMetadataAlloc() /* empty metadata */
#   define PS_TRACE_ON                  0

# else /* PS_NO_TRACE */
#   define PS_TRACE_ON 1

/** Basic structure for the component tree.  A component is a string of the
    form aaa.bbb.ccc, and may itself contain further subcomponents.  The
    Component structure doesn't in fact contain it's full name, but only the
    last part. */
typedef struct p_psComponent
{
    const char *name;                  ///< last part of name of component
    psS32 level;                       ///< trace level for this component
    bool p_psSpecified;                ///< whether the component is specified
    psS32 n;                           ///< number of subcomponents
    struct p_psComponent* *subcomp;    ///< next level of subcomponents
}
p_psComponent;


/** This procedure sets the trace format for future trace messages.  The argument
 *  must be a character string consistsing of the letters H (host), L
 *  (level), M (message), N (name), and T (time).  The default is "THLNM".
 *  Deleting a letter from the string will cause the associated information
 *  to not be logged.  This procedure does not alter the order in which
 *  the messages are displayed.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psTraceSetFormat(
    const char *format                 ///< Specifies the system trace format
);

/** Sends a trace message. */
#ifdef DOXYGEN
void psTrace(
    const char *facil,                 ///< facilty of interest
    int level,                         ///< desired trace level
    const char *format,                ///< printf-style format command
    ...                                ///< trace message arguments
);
#else // ifdef DOXYGEN
void p_psTrace(
    const char* file,                  ///< file name
    int lineno,                        ///< line number in file
    const char* func,                  ///< function name
    const char *facil,                 ///< facilty of interest
    psS32 level,                       ///< desired trace level
    const char *format,                ///< printf-style format command
    ...                                ///< trace message arguments
) PS_ATTR_FORMAT(printf, 6, 7);
#ifndef SWIG
#define psTrace(facil, level, ...) \
      p_psTrace(__FILE__,__LINE__,__func__,facil, level, __VA_ARGS__)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Get the trace level
 *
 *  @return int:    Trace Level
 */
#ifdef DOXYGEN
int psTraceGetLevel(
    const char *facil                  ///< facilty of interest
);
#else // ifdef DOXYGEN
int p_psTraceGetLevel(
    const char* file,                  ///< file name
    int lineno,                        ///< line number in file
    const char *func,                  ///< function name
    const char *facil                  ///< facilty of interest
);
#ifndef SWIG
#define psTraceGetLevel(facil) \
      p_psTraceGetLevel(__FILE__,__LINE__,__func__,facil)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN



/** Sends a trace message. */
void psTraceV(
    const char *facil,                 ///< facilty of interest
    int level,                         ///< desired trace level
    const char *format,                ///< printf-style format command
    va_list ap                         ///< varargs argument list
);


/** Set trace level
 *
 *  @return int:       The previous level.
 */
int psTraceSetLevel(
    const char *facil,                 ///< facilty of interest
    int level                          ///< desired trace level
);

/// Set all trace levels to zero (do not free nodes in the facility tree).
void psTraceReset(void);


/// print trace levels
void psTracePrintLevels(void);


/// Set the destination of future trace messages.
bool psTraceSetDestination(
    int fd                             ///< File descriptor
);


/** Get the current destination for trace messages.
 *
 *  @return FILE*:      File Destination
 */
int psTraceGetDestination(void);


/// Return a psMetadata summarising the trace levels
psMetadata *psTraceLevels(void);


/// @}
#endif /* PS_NO_TRACE */
#endif /* PS_TRACE_H */

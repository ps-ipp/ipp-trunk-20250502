/** @file  psError.h
 *
 *  @brief error reporting functions
 *
 *  Error reporting functions shall be used to create log entries in the
 *  event errors are detected.  The messages shall give enough information
 *  to allow the user to know where the error has occurred and the type
 *  of error detected.
 *
 *  @author RHL, Princeton
 *  @author Eric Van Alst, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.37 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-12 22:53:34 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_ERROR_H
#define PS_ERROR_H

/// @addtogroup SysUtils System Utilities
/// @{

#include<stdio.h>
#include<stdbool.h>
#include<stdarg.h>

#include "psType.h"
#include "psErrorCodes.h"

#define _(string) string

/** Error message object */
typedef struct
{
    char* name;                        ///< category of code that caused the error
    psErrorCode code;                  ///< class of error
    char* msg;                         ///< the message associated with the error
}
psErr;


/** Get a error from the error stack
 *
 *  Previous errors on the stack are returned by psErrorGet (a value of 0
 *  passed to psErrorGet is equivalent to a call to psErrorLast).
 *
 *  if no error is at the which position, a non-NULL psErr is returned with
 *  code PS_ERR_NONE.
 *
 *  @return    Error message object at 'which'
 */
psErr* psErrorGet(
    long which                          ///< position in the error stack. 0 is last error on stack.
);


/** Get last error put on the error stack
 *
 *  The last error reported is available from psErrorLast; if no errors are
 *  current, a non-NULL psErr is returned with code PS_ERR_NONE.
 *
 *  @return psErr*     Reference to last error message on error stack
 */
psErr* psErrorLast(void);


/** Get errorCode of last error put on the error stack
 *
 *  @return psErrorCode     last error code, or PS_ERR_NONE
 */
psErrorCode psErrorCodeLast(void);


/** Clears the error stack.
 *
 *  The error stack may be completely cleared with psErrorClear.
 *
 */
void psErrorClear(void);


/** Get the error stack depth
 *
 *  @return int The number of items on the error stack
 */
long psErrorGetStackSize(void);


/** Prints error stack to specified open file descriptor
 *
 *  The entire error stack may be printed to an open file descriptor by
 *  calling psErrorStackPrint; if and only if there are current errors, the
 *  printf-style string format is first printed to the file descriptor fd. In
 *  this printout, error codes are replaced by their string equivalents.
 *
 */
void psErrorStackPrint(
    FILE* fd,                          ///< destination file descriptor
    const char* format,                ///< printf-style format of header line
    ...                                ///< any parameters required in format
) PS_ATTR_FORMAT(printf, 2, 3);

#ifndef SWIG
/** Prints error stack to specified open file descriptor
 *
 *  The entire error stack may be printed to an open file descriptor by
 *  calling psErrorStackPrintV; if and only if there are current errors, the
 *  vprintf-style string format is first printed to the file descriptor fd. In
 *  this printout, error codes are replaced by their string equivalents.
 *
 */
void psErrorStackPrintV(
    FILE* fd,                          ///< destination file descriptor
    const char* format,                   ///< printf-style format of header line
    va_list va                         ///< any parameters required in format
);
#endif // ifndef SWIG


/** Reports an error message to the logging facility
 *
 *  This function will invoke the psLogMsg function with a level of
 *  PS_LOG_ERROR and pass the parameters name and format to generate a proper
 *  log message.
 *
 *  This function modifies the error stack.
 *
 *  @return psErrorCode    the given error code
 */
#ifdef DOXYGEN
psErrorCode psError(
    psErrorCode code,                  ///< Error class code
    bool new,                        ///< true if error originates at this location
    const char* format,                ///< printf-style format of header line
    ...                                ///< any parameters required in format
);
psErrorCode psErrorV(
    psErrorCode code,                  ///< Error class code
    bool new,                          ///< true if error originates at this location
    const char* format,                ///< printf-style format of header line
    va_list ap                         ///< any parameters required in format
);
#else // ifdef DOXYGEN
psErrorCode p_psError(
    const char* filename,              ///< file name
    unsigned int lineno,               ///< line number in file
    const char* func,                  ///< function name
    psErrorCode code,                  ///< Error class code
    bool new,                          ///< true if error originates at this location
    const char* format,                ///< printf-style format of header line
    ...                                ///< any parameters required in format
) PS_ATTR_FORMAT(printf, 6, 7);
psErrorCode p_psErrorV(
    const char* filename,              ///< file name
    unsigned int lineno,               ///< line number in file
    const char* func,                  ///< function name
    psErrorCode code,                  ///< Error class code
    bool new,                          ///< true if error originates at this location
    const char* format,                ///< printf-style format of header line
    va_list ap                         ///< any parameters required in format
  );
#ifndef SWIG
#define psError(code,new,...) p_psError(__FILE__,__LINE__,__func__,code,new,__VA_ARGS__)
#define psErrorV(code,new,format,ap) p_psErrorV(__FILE__,__LINE__,__func__,code,new,format,ap)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Logs a warning message.
 *
 *  This procedure logs a message to the destination set by a prior
 *  call to psLogSetDestination(), This is equivalent to calling
 *  psLogMsg with a level of PS_LOG_WARN.
 *
 */
#ifdef DOXYGEN
void psWarning(
    const char* format,                ///< printf-style format of header line
    ...                                ///< any parameters required in format
);
#else // #ifdef DOXYGEN
void p_psWarning(
    const char* file,                  ///< file name
    int lineno,                        ///< line number in file
    const char* func,                  ///< function name
    const char* format,                ///< printf-style format of header line
    ...                                ///< any parameters required in format
) PS_ATTR_FORMAT(printf, 4, 5);
#ifndef SWIG
#define psWarning(...) \
      p_psWarning(__FILE__,__LINE__,__func__,__VA_ARGS__)
#endif // ifndef SWIG
#endif // ifdef DOXYGEN


/** Create a new psErr struct
 *
 *  Creates a new psErr struct, making a copy of the parameters.
 *
 *  @return psErr*     new psErr object
 */
psErr* psErrAlloc(
    const char* name,                  ///< Name of error in the form aaa.bbb.ccc
    psErrorCode code,                  ///< Error class code
    const char* msg                    ///< Error message
) PS_ATTR_MALLOC;

/// @}
#endif

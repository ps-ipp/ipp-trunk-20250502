/** @file  psAbort.h
 *
 *  @brief Contains the declarations for the abort function
 *
 *  The abort logging and handling shall be performed by psAbort function.
 *  This will allow for consistent handling of other software units
 *  needing to abort from program execution.
 *
 *  @author Eric Van Alst, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  $Revision: 1.16 $ $Name: not supported by cvs2svn $
 *  $Date: 2008-04-13 08:18:27 $
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_ABORT_H
#define PS_ABORT_H

/// @addtogroup SysUtils System Utilities
/// @{

#include <stdarg.h>

#include "psType.h"

/** Reports an abort message to logging facility
 *
 *  This function will invoke the psLogMsg function with a level of
 *  PS_LOG_ABORT and pass the parameters name and fmt to generate a proper
 *  log message.  After logging, this function will call system abort
 *  function to abnormally terminate the program.
 *
 *  @return  void No return value
 *
 */
#ifdef DOXYGEN
void psAbort(
    const char *format,                 ///< A printf style formatting statement
    ...
);
#else // ifdef DOXYGEN
void p_psAbort(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *format,                 ///< A printf style formatting statement
    ...
) PS_ATTR_FORMAT(printf, 4, 5) PS_ATTR_NORETURN;
#ifndef SWIG
#define psAbort(...) \
      p_psAbort(__FILE__, __LINE__, __func__, __VA_ARGS__)
#endif // iddef SWIG
#endif // ifdef DOXYGEN

/** Reports an abort message to logging facility
 *
 *  This function will invoke the psLogMsg function with a level of
 *  PS_LOG_ABORT and pass the parameters name and fmt to generate a proper
 *  log message.  After logging, this function will call system abort
 *  function to abnormally terminate the program.
 *
 *  @return  void No return value
 *
 */
#ifdef DOXYGEN
void psAssert(
    const bool value,
    const char *format,                 ///< A printf style formatting statement
    ...
);
#else // ifdef DOXYGEN
void p_psAssert(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const bool value,
    const char *format,                 ///< A printf style formatting statement
    ...
) PS_ATTR_FORMAT(printf, 5, 6);
#ifndef SWIG
#define psAssert(VALUE, ...) \
      p_psAssert(__FILE__, __LINE__, __func__, (VALUE), __VA_ARGS__)
#endif // iddef SWIG
#endif // ifdef DOXYGEN

/// @}
#endif // #ifndef PS_ABORT_H

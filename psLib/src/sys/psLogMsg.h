/** @file  psLogMsg.h
 *
 *  @brief Procedures for logging messages.
 *
 *  This file will hold the prototypes for defining procedure which set
 *  message log levels, messahe log formats, message log destinations, and
 *  for generating the messages themselves.
 *
 *  @author Robert Lupton, Princeton University
 *  @author GLG, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.40 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-09 01:40:07 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_LOG_MSG_H
#define PS_LOG_MSG_H

/// @addtogroup SysUtils System Utilities
/// @{

#include <stdarg.h>
#include "psType.h"


///< Status codes for log messages
enum {
    PS_LOG_ABORT = 0,                  ///< log message is a critical error, perform an abort after printing
    PS_LOG_ERROR,                      ///< log message is an error, but don't abort
    PS_LOG_WARN,                       ///< log message is a warning
    PS_LOG_INFO,                       ///< log message is informational only
    PS_LOG_DETAIL,                     ///< log message provides details of minor interest
    PS_LOG_MINUTIA,                    ///< log message provides very detailed information
};


///< Destinations for log messages
enum {
    PS_LOG_TO_NONE = 0,                ///< turn off logging
    PS_LOG_TO_STDERR = 1,              ///< log to system's stderr
    PS_LOG_TO_STDOUT = 2               ///< log to system's stdout
};

/** This procedure sets the destination for future log messages.
 *
 *  This procedure will take an integer as an argument
 *  which can specify general log destinations.
 *
 *  @return bool     true if set successfully, otherwise false.
 */
bool psLogSetDestination(
    int fd                             ///< Specifies where to send messages.
);


/** This procedure returns the current log destination file descriptor.  If the
 * destination has not been defined by the use, the descriptor for stdout is
 * returned.
 *
 *  @return int:        The current file descriptor.
 */
int psLogGetDestination(void);


/** This procedure sets the message level for future log messages.  Subsequent
 *  log messages, with a log level of "mylevel", will only be logged if
 *  "mylevel" is less than the current log level set by this procedure.
 *  Ie. higher values set by this procedure will cause more log messages to
 *  be displayed.  The old log level will be returned.
 *
 *  @return int:        old logging level.
 */
int psLogSetLevel(
    int level                          ///< Specifies the system log level
);


/** This procedures returns the current log message level.
 *
 *  @return int:        The current logging level.
 */
int psLogGetLevel(void);

/** This procedure sets the log format for future log messages.  The argument
 *  must be a character string consistsing of the letters H (host), L
 *  (level), M (message), N (name), and T (time).  The default is "THLNM".
 *  Deleting a letter from the string will cause the associated information
 *  to not be logged.  This procedure does not alter the order in which
 *  the messages are displayed.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psLogSetFormat(
    const char *format                 ///< Specifies the system log format
);


/** This procedures uses a string to set the destination for which to send
 *  the corresponding log messages.
 *
 *  @return int:        The file descriptor location of the message.
 */
int psMessageDestination(
    const char *dest                   ///< Specifies where to send the message
);


/** This procedure logs a message to the destination set by a prior
 *  call to psLogSetDestination(), if myLevel is less than the level
 *  specified by a prior call to psLogSetLevel().  The message is specified
 *  with a printf-type string and arguments.
 *
 */
void psLogMsg(
    const char *name,                  ///< name of the log source
    int level,                         ///< severity level of this log message
    const char *format,                ///< printf-style format command
    ...
) PS_ATTR_FORMAT(printf, 3, 4);


/** This procedure is functionally equivalent to psLogMsg(), except that
 *  it takes a va_list as the message parameter, not a printf-style string.
 *
 */
#ifndef SWIG
void psLogMsgV(
    const char *name,                  ///< name of the log source
    int level,                         ///< severity level of this log message
    const char *format,                ///< printf-style format command
    va_list ap                         ///< varargs argument list
);
#endif // #ifndef SWIG


/// @}
#endif // #ifndef PS_LOG_MSG_H

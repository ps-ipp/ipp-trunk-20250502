/** @file  psLogMsg.c
 *  @brief Procedures for logging messages.
 *  \ingroup LogTrace
 *
 *  This file contains code for setting message log levels, message log
 *  formats, message log destinations, and for generating the messages
 *  themselves.
 *  @ingroup LogTrace
 *
 *  @author Robert Lupton, Princeton University
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.72 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-11-05 10:58:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "psLogMsg.h"
#include "psError.h"
#include "psTrace.h"
#include "psList.h"
#include "psString.h"
#include "psMemory.h"



#define MIN_LOG_LEVEL 0
#define MAX_LOG_LEVEL 9

#define MAX_LOG_LINE_LENGTH 256

static int logFD = STDERR_FILENO;       // Log file descriptor
static psS32 globalLogLevel = PS_LOG_INFO; // log all messages at this or above
static bool logTime = true;     // Flag to include time info
static bool logHost = true;     // Flag to include host info
static bool logLevel = true;    // Flag to include level info
static bool logName = true;     // Flag to include name info
static bool logMsg = true;      // Flag to include message info

/*****************************************************************************
psLogSetLevel(): Set the current log level and return old level.
Input:
 level (psS32): the new log level.
Output:
 The old log level.
 *****************************************************************************/
int psLogSetLevel(int level)
{
    // Save old global log level for changing it.
    psS32 oldLevel = globalLogLevel;

    if ((level < MIN_LOG_LEVEL) || (level > MAX_LOG_LEVEL)) {
        psLogMsg("logmsg", PS_LOG_WARN, "Attempt to set invalid logMsg level: %d", level);
        level = (level < MIN_LOG_LEVEL) ? MIN_LOG_LEVEL : MAX_LOG_LEVEL;
    }
    // Set new global log level
    globalLogLevel = level;

    // Return old global log level
    return oldLevel;
}

int psLogGetLevel(void)
{
    return globalLogLevel;
}

/*****************************************************************************
psLogSetDestination(): sets the log message destination.
Input:
 dest (psS32): the new log destination
Return:
 An bool: TRUE if successful.
 *****************************************************************************/
bool psLogSetDestination(int fd)
{
    if (fd < 0) {
        return false;
    }

    // Close the current FD if it's not stdout, stderr.
    if (logFD > STDERR_FILENO) {
        close(logFD);
    }
    logFD = fd;

    return true;
}

int psLogGetDestination(void)
{
    return logFD;
}


/*****************************************************************************
psLogSetFormat(): Set the format of psLogMsg output.  More precisely,
    provide a string consisting of the letters {H (host), L (level), M
    (message), N (name), T (time)}.  The default is "HLMNT".  This string
    determines whether or not they associated type of information will be
    included in message logs.  It does not determine the order in which that
    information will appear (that order is fixed).

Input:
    fmt: a string specifying the format.
Return:
    NULL.
 *****************************************************************************/
bool psLogSetFormat(const char *format)
{
    // assume none.
    logHost = false;
    logLevel = false;
    logMsg = false;
    logName = false;
    logTime = false;

    // if fmt is NULL, no logging is desired.
    if (format == NULL) {
        return false;
    }

    // XXX: What is the purpose of this conditional.
    if (strlen(format) == 0) {
        format = "THLNM";
    }
    // Step through each character in the format string.  For each letter
    // in that string, set the appropriate logging.

    for (const char *ptr = format; *ptr != '\0'; ptr++) {
        switch (*ptr) {
        case 'H':
        case 'h':
            logHost = true;
            break;
        case 'L':
        case 'l':
            logLevel = true;
            break;
        case 'M':
        case 'm':
            logMsg = true;
            break;
        case 'N':
        case 'n':
            logName = true;
            break;
        case 'T':
        case 't':
            logTime = true;
            break;
        default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Unknown logging keyword %c."), *ptr);
            return false;
        }
    }

    // XXX: If one must at least log error messages, why don't we set logMsg = true here?
    if (!logMsg) {
        psTrace("psLib.sys", 1, "You must at least log error messages (You chose \"%s\")", format);

    }
    return true;
}

int psMessageDestination(const char *dest)
{
    // No destination
    if (dest == NULL || strcasecmp(dest, "none") == 0) {
        return 0;
    }

    // Special destinations: stdout, stderr
    if (strcasecmp(dest, "stdout") == 0) {
        return STDOUT_FILENO;
    }
    if (strcasecmp(dest, "stderr") == 0) {
        return STDERR_FILENO;
    }

    int fileD = open(dest, O_WRONLY | O_CREAT, 0666);
    if (fileD == 0) {
        psError(PS_ERR_IO, true, _("Could not open file '%s' for output."), dest);
        return -1;
    }

    if (lseek(fileD, 0, SEEK_END) == -1) {
        psError(PS_ERR_IO, true, "Could not seek to end of file %s", dest);
        return -1;
    }

    return fileD;
}

#ifndef HOST_NAME_MAX                // should be in limits.h
#define HOST_NAME_MAX 256
#endif // #ifndef HOST_NAME_MAX

/*****************************************************************************
psLogMsgV(): This routine sends the message, which is a printf style string
specified in the "..." argument, to the current message log destination with
the severity specified by the "level" argument.
 Input:
   name
   level
   fmt
   ap
 *****************************************************************************/
void psLogMsgV(const char *name, int level, const char *format, va_list ap)
{
    static psS32 first = 1;       // Flag for calling gethostname()
    static char hostname[HOST_NAME_MAX + 1];

    // Buffer for hostname.
    char clevel = 0;            // letter-name for level
    char head[MAX_LOG_LINE_LENGTH + 2]; // the added two are for the ending | and \0
    char *head_ptr = head;      // where we've got to in head
    psS32 maxLength = MAX_LOG_LINE_LENGTH;
    time_t clock = time(NULL);  // The current time.
    struct tm *utc = gmtime(&clock);    // The current gm time.

    // If it's an abort, we always want to see the message
    if (logFD == 0 && level == PS_LOG_ABORT) {
        logFD = STDERR_FILENO;
    }
    // If logging is off, or if the level is too high, return immediately.
    if ((level > globalLogLevel) || (logFD == 0)) {
        return;
    }
    // If I have not been here yet, determine my hostname and save it.
    if (first) {
        first = 0;
        gethostname(hostname, HOST_NAME_MAX);
    }

    switch (level) {
    case PS_LOG_ABORT:
        clevel = 'A';
        break;

    case PS_LOG_ERROR:
        clevel = 'E';
        break;

    case PS_LOG_WARN:
        clevel = 'W';
        break;

    case PS_LOG_INFO:
        clevel = 'I';
        break;

    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        clevel = level + '0';
        break;

    default:
        psTrace("psLib.sys", 2, "Invalid logMsg level: %d (%s)\n", level, format);
        level = (level < 0) ? 0 : 9;
        clevel = level + '0';
        break;
    }

    // Create the various log fields...
    if (logTime) {
        maxLength -= snprintf(head_ptr, maxLength, "%4d-%02d-%02d %02d:%02d:%02dZ",
                              utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday,
                              utc->tm_hour, utc->tm_min, utc->tm_sec) - 1;
        head_ptr += strlen(head_ptr);
    }

    // Hostname should be 10 characters.
    if (logHost) {
        if (head_ptr > head) {
            *head_ptr++ = '|';
        }
        maxLength -= snprintf(head_ptr, maxLength, " %s ", hostname);
        head_ptr += strlen(head_ptr);
    }
    if (logLevel) {
        if (head_ptr > head) {
            *head_ptr++ = '|';
        }
        maxLength -= snprintf(head_ptr, maxLength, "%c", clevel);
        head_ptr += strlen(head_ptr);
    }
    if (logName) {
        if (head_ptr > head) {
            *head_ptr++ = '|';
        }
        maxLength -= snprintf(head_ptr, maxLength, "%s|\n", name);

        head_ptr += strlen(head_ptr);
    } else {
      if (head_ptr > head) {
	*head_ptr++ = '|';
      }
    }

    // rather than putting in a return for the message, let's only put in the return if we asked for the function name
    if ((head_ptr == head) && !logMsg) { // no output desired
        return;
    }
    *head_ptr = '\0';

    if (write(logFD, head, strlen(head))) {;} // ignore return value

    if (logMsg) {
        psString msg = NULL;            // Message to print
        psStringAppendV(&msg, format, ap);

        // detect multiple lines in message and indent each line by 4 spaces.
        char *msgPtr = NULL;
        char *line = strtok_r(msg, "\n", &msgPtr);
        while (line) {
            if(write(logFD, "          ", PS_MIN(2*level, 10))) {;} // ignore return value
            if(write(logFD, line, strlen(line))) {;} // ignore return value
            if(write(logFD, "\n", 1)) {;} // ignore return value
            line = strtok_r(NULL, "\n", &msgPtr);
        }
        psFree(msg);
    } else {
        if(write(logFD, "\n", 1)) {;} // ignore return value
    }

    if (level == PS_LOG_ABORT) {
        switch (logFD) {
        case STDOUT_FILENO:
            fflush(stdout);
            break;
        case STDERR_FILENO:
            fflush(stderr);
            break;
            // For the others, write() should send it unbuffered...?
        }

    }
}

/*****************************************************************************
psLogMsg(): This routine sends the message, which is a printf style string
specified in the "..." argument, to the current message log destination with
the severity specified by the "level" argument.

Input:
  name: Indicates the source of this log message.
  level: The severity of this log message.
  fmt: The printf-stype formatted string, followed by the arguments
        to that string.
  ... The arguments to the above printf-style string.

Return:
   NULL
 *****************************************************************************/
void psLogMsg(const char *name, int level, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    psLogMsgV(name, level, format, ap);
    va_end(ap);
}

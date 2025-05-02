/** @file  psTime.h
 *
 *  @brief Definitions for time, time utilities, and conversion functions for use
 *  with psLib astronomy functions.
 *
 *  A collection of functions are required by psLib to manipulate time data. These
 *  functions primarily consist of conversions between specific time formats.  They
 *  use the UNIX timeval time system as the base upon which International Atomic
 *  Time (TAI) and Universal Time Coordinated (UTC) are calculated.
 *
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.58 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-03-17 23:53:59 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PSTIME_H
#define PSTIME_H

/// @addtogroup Astro Astronomy
/// @{

#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

#include "psType.h"
#include "psImage.h"
#include "psLookupTable.h"
#include "psCoord.h"

#if defined(__APPLE__)   // incorrectly missing in time.h
struct tm *gmtime_r(const time_t *, struct tm *);
#endif

struct psSphere;

/** Time type.
 *
 * Enumeration for psTime types, TAI or UTC time.
 */
typedef enum {
    PS_TIME_TAI,                       ///< Temps Atomique International (TAI) time (time with leapseconds)
    PS_TIME_UTC,                       ///< Universal Time Coordinated (UTC) time (time without leapseconds)
    PS_TIME_UT1,                       ///< Universal Time corrected for polar motion
    PS_TIME_TT,                        ///< Terrestrial Time
} psTimeType;

/** Time Bulletin type
 *
 * Enumeration for psTimeBulletin type, A or B.
 */
typedef enum {
    PS_IERS_A,                         ///< IERS Bulletin A
    PS_IERS_B,                         ///< IERS Bulletin B
} psTimeBulletin;

/** Definition of psTime.
 *
 *  The psTime struct is used by psLib to represent time values critical to
 *  astronomical calculations.  This structure represents a time which is
 *  equivalent to TAI (International Atomic Time) and is measured in both
 *  seconds and microseconds.
 */
typedef struct
{
    psS64 sec;                         ///< Seconds since epoch, Jan 1, 1970.
    psU32 nsec;                        ///< Nanoseconds since last second.
    bool leapsecond;                   ///< if time falls on UTC leapsecond
    psTimeType type;                   ///< Type of time.
}
psTime;


/** Initialize time data.
 *
 *  Sets the configuration file and sets up the appropriate psTimeTables and predictions.
 */
bool psTimeInit(
    const char *filename                ///< psTime configuration file
);


/** Frees memory that was allocated by psTime functions.
 *
 *  Allows a subsequent search for leaked memory.
 *
 * @return true on sucess.
*/
bool p_psTimeFinalize(void);

/** Frees memory that was allocated by psTime functions.
 *
 *  Allows a subsequent search for leaked memory.
 */
void psTimeFinalize(void);

/** Allocate time struct.
 *
 * Allocates an empty time struct. User must specify the psTimeType
 * (PS_TIME_TAI or PS_TIME_UTC) in the argument. The seconds and microseconds members
 * of the struct are set to zero.
 *
 * @return psTime*:     Struct with empty time.
 */
psTime* psTimeAlloc(
    psTimeType type                    ///< Type of time to create (UTC or TAI).
) PS_ATTR_MALLOC;


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psTime structure, false otherwise.
 */
bool psMemCheckTime(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Get current time.
 *
 * Gets current time from the system clock. User must specify the psTimeType
 * (PS_TIME_TAI or PS_TIME_UTC) in the argument.
 *
 *  @return psTime*:    Struct with current time.
 */
psTime* psTimeGetNow(
    psTimeType type                    ///< Type of time to get (UTC or TAI).
);

/** Convert psTime to UTC, TAI, UT1, or TT time.
 *
 *  Converts psTime in-place to UTC, TAI, UT1, or TT time based on the psTimeType argument.
 *
 *  @return bool: Successful conversion?
 */
bool psTimeConvert(
    psTime *time,                      ///< Time to be converted.
    psTimeType type                    ///< Type to be converted to.
);

/** Convert psTime to Local Mean Sidereal Time (LMST).
 *
 *  Converts psTime at the given longitude to LMST time. If the input time is not
 *  in UTC format, then it is converted.
 *
 *  @return double:     LST Time.
 */
double psTimeToLMST(
    psTime *time,                      ///< psTime to be converted.
    double longitude                   ///< Longitude.
);

/** Determine UT1 - UTC from table lookup.
 *
 *  This function is necessary to for various SLALIB functions.
 *
 *  @return double:     Time difference.
 */
double psTimeGetUT1Delta(
    const psTime *time,                ///< psTime to be looked up.
    psTimeBulletin bulletin            ///< IERS bulletin to use
);

/** Provides tidal corrections to UT1-UTC.
 *
 *  Uses the Ray model of Simon et al.
 *
 *  @return psTime*:    The corrected time in UT1.
 */
psTime *psTime_TideUT1Corr(
    const psTime *time                 ///< psTime to be corrected.
);

/** Determine TAI - UTC from table lookup.
 *
 *  This function is necessary to for various psTime functions.
 *
 *  @return psF64:      Time difference.
 */
psF64 p_psTimeGetTAIDelta(
    const psTime *time                 ///< psTime to be looked up.
);

/** Determine polar coordinates at a given time.
 *
 *  Determines the orientation of the polar axis at the given time.
 *
 *  @return psSphere*:      Spherical coordinates of Earth's polar axias.
 */
psSphere* p_psTimeGetPoleCoords(
    const psTime *time      ///< psTime determine polar orientation.
);

/** Calculate the number of leapseconds between two times.
 *
 *  Calculates the number of leapseconds between two times.
 *
 *  @return long:   leapseconds added between given times
 */
long psTimeLeapSecondDelta(
    const psTime* time1,               ///< First input time.
    const psTime* time2                ///< Second input time.
);

/** Determine if UTC time is a leapsecond.
 *
 *  Determines if the specified UTC time is a valid leapsecond.
 *
 *  @return bool:   valid leap second
 */
bool psTimeIsLeapSecond(
    const psTime* utc                  ///< UTC to verify if leap second
);

/** Convert psTime to Julian date time.
 *
 *  Converts psTime to Julian date (JD) time. This function does not add or
 *  subtract leapseconds.
 *
 *  @return double:     Julian Date (JD) time.
 */
double psTimeToJD(
    const psTime* time                 ///< Input time to be converted.
);
/** Convert psTime to modified Julian date time.
 *
 *  Converts psTime to modified Julian date (MJD) time. This function does not
 *  add or subtract leapseconds.
 *
 *  @return double:     Modified Julian Days (MJD) time.
 */
double psTimeToMJD(
    const psTime* time                  ///< Input time to be converted.
);

/** Convert psTime to ISO8601 formatted string.
 *
 *  Converts psTime to a null terminated string in the form of YYYY-MM-DDThh:mm:ss.ssssss
 *  This function does not add or subtract leapseconds.
 *
 *  @return psString:     Pointer null terminated array of chars in ISO time.
 */
psString psTimeToISO(
    const psTime* time                  ///< Input time to be converted.
);

/** Set number of decimals printed by psTimeToISO
 *
 * @return int: Previous setting
 */
int psTimeSetISODecimals(
    int num                             ///< Number of decimals to print
    );

/** Get number of decimals printed by psTimeToISO
 *
 * @return int: Current setting
 */
int psTimeGetISODecimals(void);

/** Convert psTime to ISO8601 formatted string with limited decimal places
 *
 *  Converts psTime to a null terminated string in the form of YYYY-MM-DDThh:mm:ss.s, with as
 *  many decimal places as specified.
 *  This function does not add or subtract leapseconds.
 *
 *  @return psString:     Pointer null terminated array of chars in ISO time.
 */
psString psTimeToString(
    const psTime *time,                 ///< Input time to be converted
    int decimals                        ///< Number of decimals to use
    );


/** Convert psTime to struct tm time.
 *
 *  Converts psTime to struct tm time.  This function should handle
 *  UTC leapseconds correctly.
 *
 *  @return tm*:   tm struct.
 */
struct tm *psTimeToTM(
                const psTime* time     ///< Input time to be converted.
            );

/** Convert psTime to timeval time.
 *
 *  Converts psTime to timeval time. This function does not add or subtract leapseconds.
 *
 *  @return timeval*:   timeval struct time.
 */
struct timeval* psTimeToTimeval(
                const psTime* time     ///< Input time to be converted.
            );

/*
 * Convert psTime to tm time.
 *
 * Converts psTime to tm time. This function is based on a Perl algorithm availble
 * in the Pan-STARRS Image processing Algorithm Design Description (ADD). This function
 * does not add or subtract leapseconds.
 *
 *  @return  tm: tm struct time.
 *
struct tm* p_psTimeToTM(
                const psTime *time     ///< Input time to be converted.
            );
*/
/** Convert JD to psTime.
 *
 *  Converts JD time to psTime. This function does not add or subtract leapseconds.
 *
 *  @return  psTime: time.
 */
psTime* psTimeFromJD(
    double jd                          ///< Input time to be converted.
);

/** Convert MJD to psTime.
 *
 *  Converts MJD time to psTime. This function does not add or subtract leapseconds.
 *
 *  @return  psTime: time.
 */
psTime* psTimeFromMJD(
    double mjd                         ///< Input time to be converted.
);

/** Convert ISO to psTime.
 *
 *  Converts ISO time to psTime. This function does not add or subtract leapseconds.
 *
 *  @return  psTime*: time
 */
psTime* psTimeFromISO(
    const char* input,                 ///< Input time to be converted.
    psTimeType type                    ///< Time type.
);

/** Convert various human-readable formats to psTime.
 *
 *  Converts human readable time formats to psTime. This function does not add or subtract leapseconds.
 *
 *  @return  psTime*: time
 */
psTime* psTimeFromString(
    const char* input,                 ///< Input time to be converted.
    psTimeType type                    ///< Time type.
);

/** Convert timeval to psTime.
 *
 *  Converts timeval time to psTime. This function does not add or subtract leapseconds.
 *
 *  @return  psTime*: time.
 */
psTime* psTimeFromTimeval(
    const struct timeval *input        ///< Input time to be converted.
);

/** Convert Terrestrial Time to psTime
 *
 *  Converts Terrestial Time to psTime.  This function assumes resultant time is of type TT.
 *
 *  @return psTime*: time (TT)
 */
psTime* psTimeFromTT(
    psS64 sec,                         ///< Input terrestrial time in seconds
    psU32 nsec                         ///< Input terrestrial time fraction of seconds (nanoseconds)
);

/** Convert UTC time to psTime
 *
 *  Converts UTC time to psTime.  It will verify if time specified is a leapsecond.
 *
 *  @return psTime*: time (UTC)time
 */
psTime* psTimeFromUTC(
    psS64  sec,                        ///< Input time in seconds
    psU32  nsec,                       ///< Input time fraction of seconds (nanoseconds)
    bool leapsecond                    ///< Input time is a leapsecond
);

/** Convert tm time to psTime.
 *
 *  Converts tm time to psTime. This function is based on a Perl algorithm availble
 *  in the Pan-STARRS Image processing Algorithm Design Description (ADD). This function
 *  does not add or subtract leapseconds.
 *
 *  @return  psTime*: time.
 */
psTime* psTimeFromTM(
    const struct tm *time              ///< Input time to be converted.
);

/** Convert an arbitrary string into a psTime.
 *
 *  Converts a string, using a strptime(3) format, into a psTime.  See
 *  strptime(3) for documentation on this format.
 *
 *  @return  psTime*: time.
 */

psTime* psTimeStrptime(
    const char *s,                      ///< string to be converted
    const char *format                  ///< strptime(3) format
);

/** Convert a psTime into a formated string.
 *
 *  Converts a psTime, using a strftime(3) format, into a formatted string.
 *  See strftime(3) for documentation on this format.
 *
 *  @return  psString: string.
 */

psString psTimeStrftime(
    const psTime *time,                 ///< Time to be formatted.
    const char *format                  ///< strftime(3) format
);

/** Adds delta to time. Result is in TAI time.
 *
 *  Adds delta to time. Input time is converted to TAI format if necessary.
 *
 *  @return  psTime*: time.
 */
psTime* psTimeMath(
    const psTime *time,                ///< Time.
    double delta                       ///< Time delta.
);

/** Determine difference between two times. Result is in TAI time.
 *
 *  Determine difference between two times. Input times are converted to TAI format if necessary.
 *
 *  @return double: Time difference.
 */
double psTimeDelta(
    const psTime *time1,               ///< First time.
    const psTime *time2                ///< Second time.
);

/** Searches the IERS time tables for a specified entry location.
 *
 *  Returns the interpolated double precision (arcsec) value at the specified entry
 *  location.  Inputs to specify are the time index in mjd, the column number
 *  corresponding to Xp, Yp, or Sp (UT1-UTC) in IERS A or B, the time table names,
 *  and the number of time tables.
 *
 *  @return psF64:          Resulting table entry for specified parameters.
 */
psF64 p_psTimeSearchTables(
    psF64 index,                       ///< time index for which to search
    psU64 column,                      ///< column number of specified index
    char *metadataTableNames[],        ///< names of IERS tables to search
    psU32 nTables,                     ///< number of IERS tables to search
    psLookupStatusType* status         ///< status of table search
);

/** Stores the current time in a psHash of timers, under the supplied name.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psTimerStart(
    char *name                         ///< timer name to start
);

/** Resets the named timer.
 *
 *  @return psF64:      The time elapsed since start.
 */
psF64 psTimerClear(
    char *name                         ///< timer name to clear
);

/** Returns the elapsed time, in seconds, for the timer specified by name.
 *
 *  @return psF64:      The elapsed time in seconds since timer start.
 */
psF64 psTimerMark(
    char *name                        ///< timer name to mark
);

/** Frees all memory associated with all timers and returns the expended time.
 *
 *  @return psF64:      The maximum time expended.
 */
bool psTimerStop(void);

/** Copy a psTime.
 *
 *  @return psTime*:        New copy of existing psTime.
 */
psTime *psTimeCopy(
    const psTime *inTime               ///< input time to copy.
);

/// Return the name of the configuration file used for time
///
/// The name is derived first from the environment variable PS_CONFIG_FILE if set; next from a previously used
/// configuration file if available; finally from the default which is set at install time.
const char *p_psTimeConfigFilename(const char *filename // Filename to use, or NULL
    );

/// @}

#endif // #ifndef PSTIME_H

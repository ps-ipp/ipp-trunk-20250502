/** @file  psTime.c
 *
 *  @brief Definitions for time, time utilities, and conversion functions for use with psLib astronomy
 *  functions.
 *
 *  A collection of functions are required by psLib to manipulate time data. These functions primarily consist
 *  of conversions between specific time formats.  They use the UNIX timeval time system as the
 *  base upon which International Atomic Time (TAI) and Universal Time Coordinated (UTC) are calculated.
 *
 *  @author Ross Harman, MHPCC
 *  @author Paul Price, IfA
 *
 *  Copyright 2004-2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "psTime.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psTrace.h"
#include "psMemory.h"
#include "psAbort.h"
#include "psImage.h"
#include "psString.h"
#include "psMetadata.h"
#include "psMetadataConfig.h"
#include "psLookupTable.h"
#include "psAssert.h"


#define S2R (M_PI / (180.0 * 240.0))    /// Sidereal conversion: GMST in seconds to radians
#define R2DEG = (180.0/M_PI)            /// Conversion from radians to degrees
#define MAX_STRING_LENGTH 256           /// Maximum length of string
#define MAX_TIME_STRING_LENGTH 30       /// Maximum length of time string: 1234-67-90T23:56:89.123456789
#define SEC_PER_MINUTE 60.0             /// Seconds per minute
#define SEC_PER_HOUR (60.0*SEC_PER_MINUTE) /// Seconds per hour
#define SEC_PER_DAY (24.0*SEC_PER_HOUR) /// Seconds per day
#define SEC_PER_YEAR (365.0*SEC_PER_DAY) /// Seconds per year
#define NSEC_PER_DAY (SEC_PER_DAY * 1000000000.0) /// Nanoseconds per day

#define MJD_EPOCH_OFFSET 40587.0        // Offset from converting to MJD
#define JD_EPOCH_OFFSET 2440587.5       // Offset for converting to JD
#define YEAR_0000_SEC -62125920000      // Offset of year 0000 from epoch
#define YEAR_9999_SEC 253202544000      // Offset of year 9999 from epoch

// Offset to convert terrestrial time (TT) to international atomic time (TAI)
#define TAI_TT_OFFSET_SECONDS        32
#define TAI_TT_OFFSET_NANOSECONDS    184000000


// Static global variables
static char *timeConfig = NULL;         // Time config file path
static psMetadata *timeMetadata = NULL; // Time metadata read from config file
static int isoDecimals = 6;             // Number of decimals to use in a string by psTimeToISO


/** Static function prototypes */
static char *cleanString(char *inString, int sLen);
static char* getToken(char **inString, char *delimiter, psParseErrorType *status);
static bool convertTimeTAIUTC(psTime* time);
static bool convertTimeUTCTAI(psTime* time);
static bool convertTimeTAITT(psTime* time);
static bool convertTimeTTTAI(psTime* time);
static bool convertTimeUTCUT1(psTime* time);

static bool timeInit(const char *fileName);

/** Removes leading and trailing whitespace and # characters from a string. The cleaned string is a new null
 *  terminated copy of the original input string. */
static char *cleanString(char *inString,// Input string
                         int sLen       // Length of string
                         )
{
    char *ptrB = inString;              // Pointer to start

    // Skip over leading # or whitespace
    while (isspace(*ptrB) || *ptrB == '#') {
        ptrB++;
    }

    // Skip over trailing whitespace, null terminators, and # characters
    char *ptrE = inString + sLen - 1;   // Pointer to end
    while (isspace(*ptrE) || *ptrE == '\0' || *ptrE == '#') {
        ptrE--;
    }

    // Length, sLen, does not include '\0'
    sLen = ptrE - ptrB + 1;

    // Adds '\0' to end of string and +1 to sLen
    return psStringNCopy(ptrB, sLen);
}

/** Returns cleaned token based on delimiter, but not including delimiter. Also changes the pointer location
 * the beginning of the string. Tokens are newly allocated null terminated strings. */
static char* getToken(char **inString,  // Input string
                      char *delimiter,  // Delimiter
                      psParseErrorType *status // Parsing status, returned
                      )
{
    // Skip over leading whitespace
    while (isspace(**inString)) {
        (*inString)++;
    }

    int sLen = strcspn(*inString, delimiter); // Length of token, not including delimiter
    char *cleanToken = NULL;            // Token, cleaned of delimiters
    if (sLen) {
        // Create new, cleaned, and null terminated token
        cleanToken = cleanString(*inString, sLen);

        // Move to end of token
        (*inString) += sLen;
        if (**inString != '\0' ) {
            (*inString)++;
        }
    } else if (**inString != '\0' && sLen == 0) {
        *status = PS_PARSE_ERROR_GENERAL;
    }

    return cleanToken;
}

// get the pslib.config filename by checking environment variable first, then
// the possiblity set config file name, then the original installation area.
const char *p_psTimeConfigFilename(const char *filename)
{
    // if filename is provided, set timeConfig to this value
    if (filename) {
        psFree(timeConfig);
        timeConfig = psStringCopy(filename);
        psMemSetPersistent(timeConfig, true);
        return timeConfig;
    }

    // check the env var first
    const char *PS_CONFIG_FILE = getenv("PS_CONFIG_FILE");
    if (PS_CONFIG_FILE) {
        return PS_CONFIG_FILE;
    }

    // check timeConfig var 2nd
    if (timeConfig) {
        return timeConfig;
    }

    // fall back to the default, this should come from configure.ac
    return PS_CONFIG_FILE_DEFAULT;
}


// Searches time tables in priority order and performs interpolation if input index value is within a table.
// If the index value is out of range, the status is set accordingly.
psF64 p_psTimeSearchTables(psF64 index,
                           psU64 column,
                           char *metadataTableNames[],
                           psU32 nTables,
                           psLookupStatusType* status)
{
    char*            tableName          = NULL;
    psF64            result             = NAN;
    psLookupTable*   table              = NULL;
    psMetadataItem*  tableMetadataItem  = NULL;

    // Check if psTime tables are already loaded
    if (!timeInit(p_psTimeConfigFilename(NULL))) {
        *status = PS_LOOKUP_ERROR;
        return NAN;
    }

    // Search each table in priority order: daily, eopc,finals
    for(psS32 i = 0; i < nTables; i++) {

        // Get table name from list of tables to search
        tableName = metadataTableNames[i];

        // Lookup table name in time metadata
        tableMetadataItem = psMetadataLookup(timeMetadata, tableName);

        // Check if table not a metadata item
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                    tableName);
            *status = PS_LOOKUP_ERROR;
            return NAN;
        }

        // Get table from metadata
        table = (psLookupTable*)tableMetadataItem->data.V;

        // Check that table is not NULL
        PS_ASSERT_PTR_NON_NULL(table,NAN);

        // Check if index within to/from range
        if (index >= table->validFrom ) {
            if (index <= table->validTo) {
                // Attempt to interpolate table
                result = psLookupTableInterpolate(table, index, column);
                *status = PS_LOOKUP_SUCCESS;
                if (!isnan(result)) {
                    break;
                }
            } else {
                *status = PS_LOOKUP_PAST_BOTTOM;
            }
        } else {
            *status = PS_LOOKUP_PAST_TOP;
        }
    }

    return result;
}

static bool timeInit(const char *fileName)
{
    bool foundTable = false;
    char *tableDir = NULL;
    char *tableNames = NULL;
    char *tableFormats = NULL;
    char *namesPtr = NULL;
    char *formatPtr = NULL;
    char *metadataNamesPtr = NULL;
    char *tableName = NULL;
    char *tableFormat = NULL;
    psS32 numTables = 0;
    psU32 nFail = 0;
    psVector *tablesFrom = NULL;
    psVector *tablesTo = NULL;
    psVector *tablesIndex = NULL;
    psMetadataItem *metadataItem = NULL;
    psLookupTable *table = NULL;
    psParseErrorType status = PS_PARSE_SUCCESS;
    char metadataTableNames[4][MAX_STRING_LENGTH] = {"daily", "eopc",  "finals", "tai"};

    // Check if the timeInit has already been run
    if (timeMetadata) {
        return true;
    }

    // All memory allocated below is "persistent"
    // XXX this is not thread safe as the persistence setting is global
    const bool initialPersistence = p_psMemAllocatePersistent(true); // Initial setting of persistence

    // Read config file
    timeMetadata = psMetadataConfigRead(timeMetadata, &nFail, fileName, true);
    if (!timeMetadata) {
        psError(PS_ERR_IO, false, "Unable to read time configuration file %s", fileName);
        return false;
    } else if (nFail != 0) {
        psError(PS_ERR_IO, false, "Failed to parse %d lines reading time configuration file %s",
                nFail, fileName);
        return false;
    }

    // Get number of tables
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.n");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.time.tables.n");
        return false;
    }
    numTables = (psS32)metadataItem->data.S32;

    // Get lower range of tables
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.from");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.time.tables.from");
        return false;
    }
    tablesFrom = psVectorCopy(tablesFrom, metadataItem->data.V, PS_TYPE_F64);
    if (tablesFrom->n != numTables) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Incorrect vector size. Size: %ld, Expected %d."), tablesFrom->n, numTables);
        psFree(tablesFrom);
        return false;
    }

    // Get upper range of tables
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.to");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.time.tables.to");
        psFree(tablesFrom);
        return false;
    }
    tablesTo = psVectorCopy(tablesTo, metadataItem->data.V, PS_TYPE_F64);
    if (tablesTo->n != numTables) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Incorrect vector size. Size: %ld, Expected %d."), tablesTo->n, numTables);
        psFree(tablesFrom);
        psFree(tablesTo);
        return false;
    }

    // Get index columns for the tables
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.index");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,_("Failed find '%s' in time metadata."),
                "psLib.time.tables.index");
        psFree(tablesFrom);
        psFree(tablesTo);
        return false;
    }
    tablesIndex = psVectorCopy(tablesIndex, metadataItem->data.V, PS_TYPE_S32);
    if (tablesIndex->n != numTables) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,_("Incorrect vector size. Size: %ld, Expected %d."),tablesIndex->n,numTables);
        psFree(tablesFrom);
        psFree(tablesTo);
        psFree(tablesIndex);
        return false;
    }

    // Get path to time data files
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.dir");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.time.tables.dir");
        psFree(tablesFrom);
        psFree(tablesTo);
        psFree(tablesIndex);
        return false;
    }
    tableDir = psStringCopy(metadataItem->data.V);

    // Table file names
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.files");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.time.tables.files");

        psFree(tablesFrom);
        psFree(tablesTo);
        psFree(tablesIndex);
        psFree(tableDir);
        return false;
    }
    tableNames = psStringCopy(metadataItem->data.V);

    // Get table format strings
    metadataItem = psMetadataLookup(timeMetadata, "psLib.time.tables.format");
    if (metadataItem == NULL) {
        p_psMemAllocatePersistent(initialPersistence);
        psError(PS_ERR_BAD_PARAMETER_VALUE,true, _("Failed find '%s' in time metadata."),
                "psLib.time.tables.format");
        psFree(tablesFrom);
        psFree(tablesTo);
        psFree(tablesIndex);
        psFree(tableDir);
        psFree(tableNames);
        return false;
    }
    tableFormats = psStringCopy(metadataItem->data.V);
    formatPtr = tableFormats;

    // Read time tables
    bool no_problem = true;  // True if we've detected no errors
    namesPtr = tableNames;
    int i;                              // Iterator
    for (i = 0; (tableName = getToken(&namesPtr, " ", &status)) != NULL; i++) {
        psString fullTableName = NULL;  // Full path for table
        psStringAppend(&fullTableName, "%s/%s", tableDir, tableName);

        // Get table format
        tableFormat = getToken(&formatPtr, ",", &status);
        if (!tableFormat) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, no_problem, _("Failed find '%s' in time metadata."),
                    "psLib.time.tables.format");
            no_problem = false;
        }

        // Create and read table
        if (i < numTables) {
            table = psLookupTableAlloc(fullTableName, (const char*)tableFormat, tablesIndex->data.S32[i]);
            psLookupTableRead(table);
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, no_problem,
                    _("Incorrect number of table files entered. Found: %d. Expected: %d."), i + 1, numTables);
            no_problem = false;
        }

        // Place tables into metadata slightly altered names as keys to create consistent naming conventions
        foundTable = false;
        for (int j = 0; j < numTables; j++) {
            metadataNamesPtr = strstr(tableName, metadataTableNames[j]);
            if (metadataNamesPtr != NULL) {
                psMetadataAdd(timeMetadata, PS_LIST_TAIL, strcat(metadataTableNames[j], "Table"),
                              PS_DATA_LOOKUPTABLE, NULL, table);
                foundTable = true;
            } else if (foundTable == false && j == numTables - 1) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, no_problem,
                        _("Incorrect number of table files entered. Found: %d. Expected: %d."), j, numTables);
                no_problem = false;
            }
        }

        psFree(fullTableName);
        psFree(tableName);
        psFree(tableFormat);
        psFree(table);
    }

    p_psMemAllocatePersistent(initialPersistence);

    if (numTables != i) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, no_problem,
                _("Incorrect number of table files entered. Found: %d. Expected: %d."),
                i, numTables);
    }

    psFree(tableDir);
    psFree(tableNames);
    psFree(tablesFrom);
    psFree(tablesTo);
    psFree(tablesIndex);
    psFree(tableFormats);

    return no_problem;
}

bool p_psTimeFinalize(void)
{
    if (timeMetadata != NULL) {
        psFree(timeMetadata);
        timeMetadata = NULL;
    }

    return true;
}

static void timeFree(psTime *outTime)
{
    // There are non dynamic allocated items
}

psTime* psTimeAlloc(psTimeType type)
{
    // Error checks
    switch (type) {
      case PS_TIME_TAI:
      case PS_TIME_UTC:
      case PS_TIME_UT1:
      case PS_TIME_TT:
        // No action
        break;
      default:
        // Since we can never return NULL from an allocator
        psAbort("Specified type, %d, is not supported.", type);
    }

    // Allocate memory for structure
    psTime *outTime = psAlloc(sizeof(psTime));
    psMemSetDeallocator(outTime, (psFreeFunc)timeFree);

    // Initialize members
    outTime->sec = 0;
    outTime->nsec = 0;
    outTime->type = type;
    outTime->leapsecond = false;

    return outTime;
}


bool psMemCheckTime(psPtr ptr)
{
    return (psMemGetDeallocator(ptr) == (psFreeFunc)timeFree);
}


psTime* psTimeGetNow(psTimeType type)
{
    // Get the system time
    struct timeval now;                 // Current time
    if (gettimeofday(&now, 0) == -1) {
        psError(PS_ERR_OS_CALL_FAILED, true,
                _("Failed to determine the current time from gettimeofday function."));
        return NULL;
    }

    // Allocate psTime struct
    psTime *time = psTimeAlloc(type);   // Container for current time

    // Convert timeval time to psTime
    time->sec = now.tv_sec;
    time->nsec = now.tv_usec * 1000;

    // Add most leapseconds to UTC time to get TAI time if necessary
    if (type == PS_TIME_TAI) {
        time->sec += p_psTimeGetTAIDelta(time);
    }

    return time;
}

static bool convertTimeTAIUTC(psTime* time)
{
    psF64  deltaTAI     = 0.0;
    psS64  deltaSec     = 0;
    psU32  deltaNsec    = 0;
    psF64  deltaUTC     = 0.0;

    // Determine delta to convert between UTC and TAI
    deltaTAI = p_psTimeGetTAIDelta(time);
    deltaSec = (psS64)(deltaTAI);
    deltaNsec = (psU32)((deltaTAI - (psF64)deltaSec) * 1e9);

    // Determine seconds
    time->sec -= deltaSec;

    // Check for underflow in nsec
    if (deltaNsec > time->nsec) {
        // Borrow second
        time->nsec += 1e9;
        time->sec--;
    }

    // Determine nsec
    time->nsec -= deltaNsec;

    // Check for overflow in nsec
    if (time->nsec >= 1e9) {
        time->nsec -= 1e9;
        time->sec++;
    }

    // Set new type
    time->type = PS_TIME_UTC;

    // Check if leapsecond present in delta
    deltaUTC = p_psTimeGetTAIDelta(time);
    if (fabs(deltaTAI-deltaUTC) >= 1.0) {
        time->sec++;
    }

    return true;
}

static bool convertTimeUTCTAI(psTime* time)
{
    psF64  delta     = 0.0;
    psS64  deltaSec  = 0;
    psU32  deltaNsec = 0;

    // Determine delta to convert between UTC and TAI
    delta = p_psTimeGetTAIDelta(time);

    deltaSec = (psS64)(delta);
    deltaNsec = (psU32)((delta - (psF64)deltaSec) * 1e9);

    // Determine seconds
    time->sec += deltaSec;

    // Determine nsec
    time->nsec += deltaNsec;

    // Check for overflow in nsec
    if (time->nsec >= 1e9) {
        time->nsec -= 1e9;
        time->sec++;
    }

    // Set new type
    time->type = PS_TIME_TAI;

    return true;
}

static bool convertTimeTAITT(psTime* time)
{
    // Add TT offset
    time->sec += TAI_TT_OFFSET_SECONDS;
    time->nsec += TAI_TT_OFFSET_NANOSECONDS;

    // Check for overflow in nsec
    if (time->nsec >= 1e9) {
        time->nsec -= 1e9;
        time->sec++;
    }

    // Set new type
    time->type = PS_TIME_TT;

    return true;
}

static bool convertTimeTTTAI(psTime* time)
{
    // Subtract TT offset
    time->sec -= TAI_TT_OFFSET_SECONDS;

    // Check for nsec underflow
    if (TAI_TT_OFFSET_NANOSECONDS > time->nsec) {
        // Borrow second
        time->sec--;
        time->nsec += 1e9;
    }
    time->nsec -= TAI_TT_OFFSET_NANOSECONDS;

    // Check for overflow in nsec
    if (time->nsec >= 1e9) {
        time->nsec -= 1e9;
        time->sec++;
    }

    // Set new type
    time->type = PS_TIME_TAI;

    return true;
}

static bool convertTimeUTCUT1(psTime* time)
{
    psS64   ut1utc  = 0;

    // Get UT1-UTC value
    ut1utc = (psS64)(psTimeGetUT1Delta(time,PS_IERS_A) * 1e9);

    // Since UTC is within 0.9 sec of UT1 then nsec member is the member affected
    if ((ut1utc < 0) && (abs(ut1utc) > time->nsec)) {
        // Borrow from sec
        time->sec--;
        if (time->leapsecond) {
            time->leapsecond = false;
        } else {
            time->leapsecond = psTimeIsLeapSecond(time);
        }
        // Add to nsec
        time->nsec += 1e9;
    }
    time->nsec += ut1utc;

    // Check for overflow in nsec
    if (time->nsec >= 1e9) {
        time->nsec -= 1e9;
        time->sec++;
        if (time->leapsecond) {
            time->leapsecond = false;
            time->sec--;
        } else {
            time->leapsecond = psTimeIsLeapSecond(time);
        }
    }

    // Set new type
    time->type = PS_TIME_UT1;

    return true;
}

bool psTimeConvert(psTime *time, psTimeType type)
{
    // Error checks
    PS_ASSERT_PTR_NON_NULL(time, false);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec, 0, (psU32)((1e9)-1), false);

    if (time->type == PS_TIME_UT1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot convert from UT1 time type");
        return false;
    }
    if (time->type == type) {
        // No action required
        return true;
    }

    switch (time->type) {
      case PS_TIME_TAI:
        switch (type) {
          case PS_TIME_UTC:
            return convertTimeTAIUTC(time);
          case PS_TIME_TT:
            return convertTimeTAITT(time);
          case PS_TIME_UT1:
            convertTimeTAIUTC(time);
            return convertTimeUTCUT1(time);
          default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified type, %d, is not supported."), type);
            return false;
        }
        break;
      case PS_TIME_TT:
        switch (type) {
          case PS_TIME_UTC:
            convertTimeTTTAI(time);
            return convertTimeTAIUTC(time);
          case PS_TIME_TAI:
            return convertTimeTTTAI(time);
          case PS_TIME_UT1:
            convertTimeTTTAI(time);
            convertTimeTAIUTC(time);
            return convertTimeUTCUT1(time);
          default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified type, %d, is not supported."), type);
            return NULL;
        }
        break;
      case PS_TIME_UTC:
        switch (type) {
          case PS_TIME_TAI:
            return convertTimeUTCTAI(time);
          case PS_TIME_TT:
            convertTimeUTCTAI(time);
            return convertTimeTAITT(time);
          case PS_TIME_UT1:
            return convertTimeUTCUT1(time);
          default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified type, %d, is not supported."), type);
            return NULL;
        }
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified type, %d, is not supported."), time->type);
        return NULL;
    }

    psAbort("Should never reach here.");
    return false;
}

double psTimeToLMST(psTime *time,
                    double longitude)
{
    psF64  jdTdtDays    =  0.0;
    psF64  jdUt1Days    =  0.0;
    psF64  mjdUt1Days   =  0.0;
    psF64  lmstRad      =  0.0;
    psF64  fracDays     =  0.0;
    psF64  gmstRad      =  0.0;
    psF64  t            =  0.0;
    psF64  tu           =  0.0;
    psF64  const1       =  24110.5493771;
    psF64  const2       =  8639877.3173760;
    psF64  const3       =  307.4771600;
    psF64  const4       =  0.0931118;
    psF64  const5       = -0.0000062;
    psF64  const6       =  0.0000013;
    psTime *tdtTime     = NULL;
    psTime *ut1Time     = NULL;

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NAN);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NAN);

    // Verify input time is not in UT1 seconds
    if (time->type == PS_TIME_UT1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,_("Specified type, %d, is incorrect."),time->type);
        return NAN;
    }

    // Determine time reference to TT
    tdtTime = psTimeAlloc(time->type);
    tdtTime->sec = time->sec;
    tdtTime->nsec = time->nsec;
    tdtTime->leapsecond = time->leapsecond;
    psTimeConvert(tdtTime,PS_TIME_TT);

    // Determine time reference to UT1
    ut1Time = psTimeAlloc(time->type);
    ut1Time->sec = time->sec;
    ut1Time->nsec = time->nsec;
    ut1Time->leapsecond = time->leapsecond;
    psTimeConvert(ut1Time,PS_TIME_UT1);

    // Calculate UT1 as Julian Centuries since J2000.0
    jdUt1Days = psTimeToJD(ut1Time);
    mjdUt1Days = psTimeToMJD(ut1Time);
    t = (jdUt1Days - 2451545.0)/36525.0;

    // Calculate TDT as Julian centuries since J2000.0
    jdTdtDays = psTimeToJD(tdtTime);
    tu = (jdTdtDays - 2451545.0)/36525.0;

    // Calculate fractional part of MJD
    fracDays = fmod(mjdUt1Days, 1.0);

    // Calculate Greenwich Mean Sidereal Time (GMST) in radians.
    // Equation set up to minimize multiplications.
    gmstRad = fracDays * 2 * M_PI +
        (const1 + const2 * tu + t * (const3 + t * (const4 + t * (const5 + const6 * t)))) * S2R;

    // Place GMST between 0 and 2*pi
    gmstRad = fmod(gmstRad, 2 * M_PI);

    // Calculate Local Mean Sidereal Time (LMST) in radians
    lmstRad = gmstRad + longitude;

    // Free temporary structs
    psFree(ut1Time);
    psFree(tdtTime);

    return lmstRad;
}

double psTimeGetUT1Delta(const psTime *time,
                         psTimeBulletin bulletin)
{
    psU32              nTables               = 2;
    psF64              mjd                   = 0.0;
    psF64              result                = 0.0;
    psU64              tableColumn           = 0;
    psF64              dut2ut1               = 0.0;
    psF64              t                     = 0.0;
    psVector*          dut                   = NULL;
    psMetadataItem*    tableMetadataItem     = NULL;
    psLookupStatusType status                = PS_LOOKUP_SUCCESS;
    char*              metadataTableNames[2] = {"dailyTable",  "finalsTable"};

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NAN);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NAN);

    // Check for invalid bulletin specified
    if ((bulletin != PS_IERS_A) && (bulletin != PS_IERS_B)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,"Invalid bulletin specified %d",bulletin);
        return NAN;
    }

    // Check if psTime tables are already loaded
    if (!timeInit(p_psTimeConfigFilename(NULL))) {
        psError(PS_ERR_UNKNOWN, true, "failed to init time tables.");
        return NAN;
    }

    // Set lookup table column based on Bullentin
    if (bulletin == PS_IERS_A) {
        tableColumn = 3;
    } else {
        tableColumn = 6;
    }

    // Attempt to find value through table lookup and interpolation
    mjd = psTimeToMJD(time);
    result = p_psTimeSearchTables(mjd,tableColumn,metadataTableNames,nTables,&status);

    // Value could not be found through table lookup and interpolation
    if (status == PS_LOOKUP_PAST_TOP) {

        // Date too early for tables. Get default time delta value from metadata, and issue warning.
        psLogMsg("psLib.astro", PS_LOG_WARN,_("Specified psTime predates (%g) all tables of %s information."),mjd,"UT1-UTC");

        // Lookup value from time metadata loaded from pslib.config
        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.before.dut");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                    "psLib.time.before.dut");
            return NAN;
        }
        result = tableMetadataItem->data.F64;

    } else if (status == PS_LOOKUP_PAST_BOTTOM) {
        /* Date too late for tables. Issue warning and use following formulae for predicting
           ahead of the most recent available table entry.
             ut1-utc = [0] + [1]*(MJD - [2]) - (ut2-ut1)
             [0, 1, 2] = @psLib.time.predict.dut
             ut2-ut1 = 0.022 sin(2*pi*t) - 0.012 cos(2*pi*t) - 0.006 sin(4*pi*t) + 0.007 cos(4*pi*t)
             t = 2000.0 + (MJD - 51544.03)/365.2422
        */
        // Generate warning of postdate information
        psLogMsg("psLib.astro", PS_LOG_WARN,_("Specified psTime postdates (%g) all tables of %s information."), mjd, "UT1-UTC");

        // Lookup values to calculate prediction
        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.predict.dut");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                    "psLib.time.predict.dut");
            return NAN;
        }
        dut = (psVector*)tableMetadataItem->data.V;
        PS_ASSERT_PTR_NON_NULL(dut,NAN);

        // Calculate predication of future UT1-UTC
        t = 2000.0 + (mjd - 51544.03) / 365.2422;
        dut2ut1 = 0.022 * sin(2 * M_PI * t) - 0.012 * cos(2 * M_PI * t) -
            0.006 * sin(4.0 * M_PI * t) + 0.007 * cos(4.0 * M_PI * t);
        result = dut->data.F64[0] + dut->data.F64[1] * (mjd - dut->data.F64[2]) - dut2ut1;

    } else if (status != PS_LOOKUP_SUCCESS) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed time table interpolation."));
        return NAN;
    }

    return result;
}

static double DMOD(double x, double y)
{
    double value = x - y * trunc(x/y);
    return value;
}

psTime *psTime_TideUT1Corr(const psTime *time)
{
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    psTime *out = NULL;
    //Also see psEOC_PolarTideCorr for more info.

    // Convert psTime to MJD
    double MJD = psTimeToMJD(time);
    if (isnan(MJD)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to MJD failed.  Invalid input time.\n");
        return NULL;
    }

    // Calculate number of Julian centuries since 2000
    double RJD = MJD;

    //Formula comes from fortran reference
    //DMOD in fortran ref. = double remainder -> x - y * trunc(x/y)
    double T, L, LPRIME, CAPF, CAPD, OMEGA, THETA, CORZ;
    double ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8;
    double T2, T3, T4;
    T = (RJD - 51544.5) / 36525.0;
    T2 = T*T;
    T3 = T*T*T;
    T4 = T*T*T*T;
    L = -0.0002447 * T4 + 0.051635 * T3 + 31.8792 * T2 + 1717915923.2178 * T + 485868.249036;
    L = DMOD(L, 1296000.0);
    LPRIME = -0.00001149 * T4 - 0.000136 * T3 - 0.5532 * T2 + 129596581.0481 * T + 1287104.79305;
    LPRIME = DMOD(LPRIME, 1296000.0);
    CAPF = 0.00000417 * T4 - 0.001037 * T3 - 12.7512 * T2 + 1739527262.8478 * T + 335779.526232;
    CAPF = DMOD(CAPF, 1296000.0);
    CAPD = -0.00003169 * T4 + 0.006593 * T3 - 6.3706 * T2 + 1602961601.209 * T + 1072260.70369;
    CAPD = DMOD(CAPD, 1296000.0);
    OMEGA = -0.00005939 * T4 + 0.007702 * T3 + 7.4722 * T2 - 6962890.2665 * T + 450160.398036;
    OMEGA = DMOD(OMEGA, 1296000.0);
    THETA = (67310.54841 + (876600.0 * 3600.0 + 8640184.812866) * T + 0.093104 * T2 -
             6.2e-6 * T3) * 15.0 + 648000.0;
    ARG7 = DMOD((-L - 2.0 * CAPF - 2.0 * OMEGA + THETA) * M_PI / 648000.0, 2.0 * M_PI)
           - M_PI / 2.0;
    ARG1 = DMOD((-2.0 * CAPF - 2.0 * OMEGA + THETA) * M_PI / 648000.0, 2.0 * M_PI) - M_PI / 2.0;
    ARG2 = DMOD((-2.0 * CAPF + 2.0 * CAPD - 2.0 * OMEGA + THETA) * M_PI / 648000.0, 2.0 * M_PI)
           - M_PI / 2.0;
    ARG3 = DMOD(THETA * M_PI / 648000.0, 2.0 * M_PI) - M_PI / 2.0;
    ARG4 = DMOD((-L - 2.0 * CAPF - 2.0 * OMEGA + 2.0 * THETA) * M_PI / 648000.0, 2.0 * M_PI);
    ARG5 = DMOD((-2.0 * CAPF - 2.0 * OMEGA + 2.0 * THETA) * M_PI / 648000.0, 2.0 * M_PI);
    ARG6 = DMOD((-2.0 * CAPF + 2.0 * CAPD - 2.0 * OMEGA + 2.0 * THETA) * M_PI / 648000.0,
                2.0 * M_PI);
    ARG8 = DMOD((2.0 * THETA) * M_PI / 648000.0, 2.0 * M_PI);
    CORZ =  0.0245 * sin(ARG7) + 0.0503 * cos(ARG7)
            +0.1210 * sin(ARG1) + 0.1605 * cos(ARG1)
            +0.0286 * sin(ARG2) + 0.0516 * cos(ARG2)
            +0.0864 * sin(ARG3) + 0.1771 * cos(ARG3)
            -0.0380 * sin(ARG4) - 0.0154 * cos(ARG4)
            -0.1617 * sin(ARG5) - 0.0720 * cos(ARG5)
            -0.0759 * sin(ARG6) - 0.0004 * cos(ARG6)
            -0.0196 * sin(ARG8) - 0.0038 * cos(ARG8);
    CORZ = CORZ * 0.1e-3;

    double timeCheck = (double)(time->sec) + (double)(1e-9*time->nsec);
    if ( (timeCheck + CORZ) < 0.0 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Invalid time for Tide Correction.\n");
        return NULL;
    }
    out = psTimeAlloc(time->type);
    *out = *time;
    if (out->type != PS_TIME_UT1) {
        psTimeConvert(out, PS_TIME_UT1);
    }
    //see if corrections include seconds or just nano-seconds
    //nano-seconds must be converted (scaled) to integer values & total cannot be negative
    if (fabs(CORZ) > 1.0) {
        int sec = (int)CORZ;
        out->sec += sec;
        int nsec = (int)((CORZ - sec)*1e9);
        out->nsec += nsec;
    } else {
        int nsec = out->nsec + (int)(CORZ * 1e9);
        if (nsec < 0) {
            out->sec += -1;
            out->nsec = (int)(1e9) + nsec;
        } else {
            out->nsec = nsec;
        }
    }
    return out;
}

psSphere* p_psTimeGetPoleCoords(const psTime* time)
{
    psU32 nTables = 3;
    psF64 x = 0.0;
    psF64 y = 0.0;
    psF64 mjd = 0.0;
    psF64 a = 0.0;
    psF64 c = 0.0;
    psF64 mjdPred = 0.0;
    psSphere* output = NULL;
    psLookupStatusType xStatus = PS_LOOKUP_SUCCESS;
    psLookupStatusType yStatus = PS_LOOKUP_SUCCESS;
    psMetadataItem *tableMetadataItem = NULL;
    char *metadataTableNames[3] = {"dailyTable", "eopcTable",  "finalsTable"};
    psVector *xp = NULL;
    psVector *yp = NULL;

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NULL);

    if (time->type != PS_TIME_TAI) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified type, %d, is incorrect."), time->type);
        return NULL;
    }

    // Check if psTime tables are already loaded
    if (!timeInit(p_psTimeConfigFilename(NULL))) {
        psError(PS_ERR_UNKNOWN, true, "failed to init time tables.");
        return NULL;
    }

    // Attempt to find value through table lookup and interpolation
    mjd = psTimeToMJD(time);
    //    x = p_psTimeSearchTables(mjd, 0, &xStatus, metadataTableNames, nTables);
    x = p_psTimeSearchTables(mjd, 0, metadataTableNames, nTables,&xStatus);
    //    y = p_psTimeSearchTables(mjd, 0, &yStatus, metadataTableNames, nTables);
    y = p_psTimeSearchTables(mjd, 0, metadataTableNames, nTables,&yStatus);

    // Value could not be found through table lookup and interpolation
    if (xStatus==PS_LOOKUP_PAST_TOP && yStatus==PS_LOOKUP_PAST_TOP) {

        // Date too earlier for tables. Get default polar coodinate values from metadata, and issue warning.
        #if 0
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified psTime predates (%g) all tables of %s information."), mjd, "polar motion");
        return NULL;
        #else

        psLogMsg("psLib.astro", PS_LOG_ERROR, _("Specified psTime predates (%g) all tables of %s information."), mjd, "polar motion");
        #endif

        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.before.xp");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed find '%s' in time metadata."), "psLib.time.before.xp");
            return NULL;
        }
        x = tableMetadataItem->data.F64;

        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.before.yp");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."), "psLib.time.before.yp");
            return NULL;
        }
        y = tableMetadataItem->data.F64;

    } else if (xStatus==PS_LOOKUP_PAST_BOTTOM && yStatus==PS_LOOKUP_PAST_BOTTOM) {

        /* Date too late for tables. Issue warning and use following formulae for predicting
           ahead of the most recent available table entry.
              x = [0] + [1]*cos a + [2]*sin a + [3]*cos c + [4]*sin c
              [0], [1], [2], [3] = @psLib.time.predict.xp
              y = [0] + [1]*cos a + [2]*sin a + [3]*cos c + [4]*sin c
              [0], [1], [2], [3] = @psLib.time.predict.yp
              a = 2*pi*(mjd - pslib.time.predict.mjd)/365.25
              c = 2*pi*(mjd - pslib.time.predict.mjd)/435.0
        */
        #if 0
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified psTime postdates (%g) all tables of %s information."), mjd, "polar motion");
        return NULL;
        #else

        psLogMsg("psLib.astro", PS_LOG_ERROR,
                 _("Specified psTime postdates (%g) all tables of %s information."), mjd, "polar motion");
        #endif

        // Get predicted MJD
        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.predict.mjd");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                    "psLib.time.predict.mjd");
            return NULL;
        }
        mjdPred = tableMetadataItem->data.F64;

        // Get xp
        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.predict.xp");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."), "psLib.time.predict.xp");
            return NULL;
        }
        xp = (psVector*)tableMetadataItem->data.V;
        PS_ASSERT_PTR_NON_NULL(xp,NULL);

        // Get yp
        tableMetadataItem = psMetadataLookup(timeMetadata, "psLib.time.predict.yp");
        if (tableMetadataItem == NULL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."), "psLib.time.predict.yp");
            return NULL;
        }
        yp = (psVector*)tableMetadataItem->data.V;
        PS_ASSERT_PTR_NON_NULL(yp,NULL);

        // Calculate "a" and "c" constants
        a = 2 * M_PI * (mjd - mjdPred) / 365.25;
        c = 2 * M_PI * (mjd - mjdPred) / 435.0;

        // Calculate x and y polar coordinates
        x = xp->data.F64[0] +
            xp->data.F64[1]*cos(a) +
            xp->data.F64[2]*sin(a) +
            xp->data.F64[3]*cos(c) +
            xp->data.F64[4]*sin(c);

        y = yp->data.F64[0] +
            yp->data.F64[1]*cos(a) +
            yp->data.F64[2]*sin(a) +
            yp->data.F64[3]*cos(c) +
            yp->data.F64[4]*sin(c);

    } else if (xStatus!=PS_LOOKUP_SUCCESS || yStatus!=PS_LOOKUP_SUCCESS) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed time table interpolation."));
        return NULL;
    }

    // Create output sphere and convert arcsec to radians (i.e. x/60/60*PS_PI/180)
    output = psAlloc(sizeof(psSphere));
    output->r = x * M_PI / 648000.0;
    output->d = y * M_PI / 648000.0;

    return output;
}

psF64 p_psTimeGetTAIDelta(const psTime *time)
{
    psF64 jd = 0.0;
    psF64 mjd = 0.0;
    psF64 out = 0.0;
    psF64 const1 = 0.0;
    psF64 const2 = 0.0;
    psF64 const3 = 0.0;
    psLookupTable* table = NULL;
    psMetadataItem *tableMetadataItem = NULL;
    psVector *results = NULL;

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NAN);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NAN);

    // Check if psTime tables are loaded/loadable
    if (!timeInit(p_psTimeConfigFilename(NULL))) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed to open file %s."),
                p_psTimeConfigFilename(NULL));
        return NAN;
    }

    // Get table from metadata
    tableMetadataItem = psMetadataLookup(timeMetadata, "taiTable");
    if (tableMetadataItem == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."), "taiTable");
        return NAN;
    }
    table = (psLookupTable*)tableMetadataItem->data.V;
    PS_ASSERT_PTR_NON_NULL(table,0);

    // Determine Julian and modified Julian dates used in table lookup and time delta calculation
    if (time->sec < 0) {
        // psTime earlier than epoch
        jd = time->sec / SEC_PER_DAY - time->nsec / NSEC_PER_DAY + JD_EPOCH_OFFSET;
        mjd = time->sec / SEC_PER_DAY - time->nsec / NSEC_PER_DAY + MJD_EPOCH_OFFSET;
    } else {
        // psTime greater than epoch
        jd = time->sec / SEC_PER_DAY + time->nsec / NSEC_PER_DAY + JD_EPOCH_OFFSET;
        mjd = time->sec / SEC_PER_DAY + time->nsec / NSEC_PER_DAY + MJD_EPOCH_OFFSET;
    }

    // Set ceiling of the julian date to the last entry in the lookup table
    if (table->validTo < jd) {
        jd = table->validTo;
    }

    // Interpolation of look up table
    results = psLookupTableInterpolateAll(table, jd);

    // Check for successful interpolation
    if (results == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed time table interpolation."));
        return NAN;
    }

    // Set constants from table
    const1 = results->data.F64[1];
    const2 = results->data.F64[2];
    const3 = results->data.F64[3];

    // If const3 not equal to zero solve for difference else floor of const1
    if (fabs(const3-0.0) > FLT_EPSILON) {
        out = const1 + (mjd - const2) * const3;
    } else {
        out = floor(const1);
    }

    psFree(results);

    return out;
}

long psTimeLeapSecondDelta(const psTime *time1,
                           const psTime *time2)
{
    psS64 diff = 0;

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time1,0);
    PS_ASSERT_PTR_NON_NULL(time2,0);
    PS_ASSERT_INT_WITHIN_RANGE(time1->nsec,0,(psU32)((1e9)-1),0);
    PS_ASSERT_INT_WITHIN_RANGE(time2->nsec,0,(psU32)((1e9)-1),0);
    diff = abs((psS64)p_psTimeGetTAIDelta((psTime*)time1)-(psS64)p_psTimeGetTAIDelta((psTime*)time2));
    return diff;
}

bool psTimeIsLeapSecond(const psTime* utc)
{
    psTime*    prevUtc     = NULL;
    bool     returnValue = false;

    // Check for valid time
    PS_ASSERT_PTR_NON_NULL(utc,false);

    // Verify time is UTC type
    if (utc->type != PS_TIME_UTC) {
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,_("Specified type, %d, is incorrect."),utc->type);
        return false;
    }

    // Allocate time to hold utc - 1 second
    prevUtc = psTimeAlloc(PS_TIME_UTC);
    prevUtc->sec = utc->sec - 1;

    // Check the absolute difference between the two times for leapsecond
    if (psTimeLeapSecondDelta(utc,prevUtc) >= 1.0) {
        returnValue = true;
    } else {
        returnValue = false;
    }

    // Free prevUtc
    psFree(prevUtc);

    return returnValue;
}

double psTimeToJD(const psTime *time)
{
    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NAN);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NAN);

    // ADD says that this formula works only for PS_TIME_TAI, so adding the following:
    psTime *time2 = psTimeCopy(time);
    psTimeConvert(time2, PS_TIME_TAI);

    double jd;                          // Julian date, to return
    if (time2->sec < 0) {
        // psTime earlier than epoch
        jd = time2->sec / SEC_PER_DAY - time2->nsec / NSEC_PER_DAY + JD_EPOCH_OFFSET;
    } else {
        // psTime greater than epoch
        jd = time2->sec / SEC_PER_DAY + time2->nsec / NSEC_PER_DAY + JD_EPOCH_OFFSET;
    }
    psFree(time2);

    return jd;
}

double psTimeToMJD(const psTime *time)
{
    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NAN);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NAN);

    // ADD says that this formula works only for PS_TIME_TAI, so adding the following:
    psTime *time2 = psTimeCopy(time);
    psTimeConvert(time2, PS_TIME_TAI);

    double mjd;                         // Modified Julian Date, to return
    if (time2->sec < 0) {
        // psTime earlier than epoch
        mjd = time2->sec / SEC_PER_DAY - time2->nsec / NSEC_PER_DAY + MJD_EPOCH_OFFSET;
    } else {
        // psTime greater than epoch
        mjd = time2->sec / SEC_PER_DAY + time2->nsec / NSEC_PER_DAY + MJD_EPOCH_OFFSET;
    }
    psFree(time2);

    return mjd;
}


// Format a time as a string, allowing for a specified number of decimals for the seconds field
static psString timeToString(const psTime *time, // Time to format as string
                             int decimals // Number of decimals to permit for seconds
                             )
{
    // Error checks
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec, 0, (psU32)((1e9)-1), NULL);
    if (time->sec < YEAR_0000_SEC || time->sec > YEAR_9999_SEC) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Time (%" PRId64 " sec) is not between year 0000 or 9999.\n", time->sec);
        return NULL;
    }
    PS_ASSERT_INT_WITHIN_RANGE(decimals, 0, 9, NULL);

    struct tm *tmTime = psTimeToTM(time); // Unix time structure

    // Convert psTime to YYYY-MM-DDThh:mm:ss in string form
    psString string = psStringAlloc(MAX_TIME_STRING_LENGTH); // Time string
    if (!strftime(string, MAX_TIME_STRING_LENGTH, "%Y-%m-%dT%H:%M:%S", tmTime)) {
        psError(PS_ERR_OS_CALL_FAILED, true, _("Failed to convert a time via strftime function."));
        return NULL;
    }
    psFree(tmTime);

    // Check if time is UTC and leapsecond
    if ((time->type == PS_TIME_UTC || time->type == PS_TIME_UT1) && time->leapsecond) {
        // Modify second to be 60
        string[17] = '6';
        string[18] = '0';
    }

    // Add in the nanoseconds
    if (decimals > 0) {
        int partial = round((double)time->nsec / pow(10.0, 9 - decimals)); // Partial part
        psString format = NULL;             // Format for printing partial part
        psStringAppend(&format, ".%%0%dd", decimals);
        psStringAppend(&string, format, partial);
        psFree(format);
    }

    return string;
}

psString psTimeToString(const psTime *time, int decimals)
{
    return timeToString(time, decimals);
}


int psTimeSetISODecimals(int num)
{
    int temp = isoDecimals;          // Current value, to return
    isoDecimals = num;
    return temp;
}

int psTimeGetISODecimals(void)
{
    return isoDecimals;
}

psString psTimeToISO(const psTime *time)
{
    return timeToString(time, isoDecimals);
}

struct tm *psTimeToTM(const psTime *time)
{
    PS_ASSERT_PTR_NON_NULL(time,NULL);

    // XXX is it safe to assume that time_t is always an integer value?
    time_t sec = time->sec;

    // if this is NOT a UTC time then we want to make sure tm.tm_sec does not
    // end up being set to 60
    if (!(time->type == PS_TIME_UTC))
    {
        // If leapsecond use previous day
        if (time->leapsecond) {
            sec--;
        }
    }

    // struct tm can handle leapseconds
    struct tm *tmTime = psAlloc(sizeof(struct tm));
    gmtime_r(&sec, tmTime);

    return tmTime;
}

struct timeval* psTimeToTimeval(const psTime *time)
{
    struct timeval  *timevalTime = NULL;

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(time->sec,0,INT32_MAX,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec,0,(psU32)((1e9)-1),NULL);

    // Allocate structure timeval
    timevalTime = (struct timeval*)psAlloc(sizeof(struct timeval));

    // Set structure members
    timevalTime->tv_sec = time->sec;
    timevalTime->tv_usec = time->nsec / 1000;

    return timevalTime;
}

psTime* psTimeFromJD(double jd)
{
    psF64 days = 0.0;
    psF64 seconds = 0.0;
    psTime *outTime = NULL;

    // Allocate psTime struct
    outTime = psTimeAlloc(PS_TIME_TAI);

    // Julian date conversion courtesy of Eugene Magnier
    days = jd - 2440587.5;
    seconds = days * SEC_PER_DAY;
    if (seconds < 0.0) {
        outTime->nsec = (seconds - (psS64)seconds) * -1000000000.0;  // psTime earlier than epoch
    } else {
        outTime->nsec = (seconds - (psS64)seconds) * 1000000000.0;   // psTime greater than epoch
    }
    outTime->sec = seconds;

    // Error check
    PS_ASSERT_INT_WITHIN_RANGE(outTime->nsec,0,(psU32)((1e9)-1),outTime);

    return outTime;
}

psTime* psTimeFromMJD(double mjd)
{
    psF64 days = 0.0;
    psF64 seconds = 0.0;
    psTime *outTime = NULL;

    // Allocate psTime struct
    outTime = psTimeAlloc(PS_TIME_TAI);

    // Modified Julian date conversion courtesy of Eugene Magnier
    days = mjd - 40587.0;
    seconds = days * SEC_PER_DAY;

    if (seconds < 0.0) {
        outTime->nsec = (seconds - (psS64)seconds) * -1000000000.0;  // psTime earlier than epoch
    } else {
        outTime->nsec = (seconds - (psS64)seconds) * 1000000000.0;   // psTime greater than epoch
    }
    outTime->sec = seconds;

    // Error check
    PS_ASSERT_INT_WITHIN_RANGE(outTime->nsec,0,(psU32)((1e9)-1),NULL);

    return outTime;
}

psTime* psTimeFromISO(const char *input,
                      psTimeType type)
{

    // Check for NULL string
    PS_ASSERT_PTR_NON_NULL(input,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(type, PS_TIME_TAI, PS_TIME_TT, NULL);

    // Convert YYYY-MM-DDThh:mm:ss.sss[Z] in string form to tm time
    psTime *outTime = psTimeStrptime(input, "%Y-%m-%dT%H:%M:%S");
    if (!outTime) {
        // Try without the middle 'T'
        outTime = psTimeStrptime(input, "%Y-%m-%d %H:%M:%S");
    }
    if (!outTime) {
        // try for date with assumed 00:00:00 time
        outTime = psTimeStrptime(input, "%Y-%m-%d");
    }
    if (!outTime) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified ISO Time string, '%s', is malformed.  "
                  "Must be in 'YYYY-MM-DDThh:mm:ss.sss' format."), input);
        return NULL;
    }

    outTime->type = type;

    return outTime;
}

// accepts a range of human readable times:
// ISO (YYYY-MM-MMTHH:MM:SS[.sss][Z])
// YYYY/MM/DDTHH:MM:DD
// YYYY-MM-DD,HH:MM:DD
// YYYY/MM/DD,HH:MM:DD
// YYYY-MM-DD@HH:MM:DD
// YYYY/MM/DD@HH:MM:DD
psTime* psTimeFromString(const char *input,
                      psTimeType type)
{

    // Check for NULL string
    PS_ASSERT_PTR_NON_NULL(input,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(type, PS_TIME_TAI, PS_TIME_TT, NULL);

    psTime *outTime = NULL;

    // Convert YYYY-MM-DDThh:mm:ss.sss[Z] in string form to tm time
    outTime = psTimeStrptime(input, "%Y-%m-%dT%H:%M:%S");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // Convert YYYY/MM/DDThh:mm:ss.sss[Z] in string form to tm time
    outTime = psTimeStrptime(input, "%Y/%m/%dT%H:%M:%S");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // Convert YYYY-MM-DD,hh:mm:ss.sss[Z] in string form to tm time
    outTime = psTimeStrptime(input, "%Y-%m-%d,%H:%M:%S");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // Convert YYYY/MM/DD,hh:mm:ss.sss[Z] in string form to tm time
    outTime = psTimeStrptime(input, "%Y/%m/%d,%H:%M:%S");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // Convert YYYY-MM-DD@hh:mm:ss.sss[Z] in string form to tm time
    outTime = psTimeStrptime(input, "%Y-%m-%d@%H:%M:%S");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // Convert YYYY/MM/DD@hh:mm:ss.sss[Z] in string form to tm time
    outTime = psTimeStrptime(input, "%Y/%m/%d@%H:%M:%S");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // try for date with assumed 00:00:00 time
    outTime = psTimeStrptime(input, "%Y-%m-%d");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    // try for date with assumed 00:00:00 time
    outTime = psTimeStrptime(input, "%Y/%m/%d");
    if (outTime) {
      outTime->type = type;
      return outTime;
    }
    psErrorClear ();

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified Time string, '%s', is malformed.  Must be in 'YYYY-MM-DDThh:mm:ss.sss' format."), input);
    return NULL;
}

psTime* psTimeFromTT(psS64 sec,
                     psU32 nsec)
{
    psTime*      outTime  = NULL;

    // Verify nsec within range
    PS_ASSERT_INT_WITHIN_RANGE(nsec,0,(psU32)((1e9)-1),NULL);

    // Allocate psTime data
    outTime = psTimeAlloc(PS_TIME_TT);

    // Set data members
    outTime->sec = sec;
    outTime->nsec = nsec;

    // Return data structure
    return outTime;
}

psTime* psTimeFromUTC(psS64 sec,
                      psU32 nsec,
                      bool leapsecond)
{
    psTime*   outTime   = NULL;

    // Verify nsec within range
    PS_ASSERT_INT_WITHIN_RANGE(nsec,0,(psU32)((1e9)-1),NULL);

    // Allocate psTime data
    outTime = psTimeAlloc(PS_TIME_UTC);

    // Set data members
    outTime->sec = sec;
    outTime->nsec = nsec;

    // Set leapsecond flag if necessary
    outTime->leapsecond = psTimeIsLeapSecond(outTime);

    return outTime;
}

psTime* psTimeFromTimeval(const struct timeval *input)
{
    psTime *outTime = NULL;


    // Error check
    PS_ASSERT_PTR_NON_NULL(input,NULL);

    // Allocate psTime struct
    outTime = psTimeAlloc(PS_TIME_TAI);

    // Convert to psTime
    outTime->sec = input->tv_sec;
    outTime->nsec = input->tv_usec * 1000;

    // Error check
    PS_ASSERT_INT_WITHIN_RANGE(outTime->nsec,0,(psU32)((1e9)-1),outTime);

    return outTime;
}

psTime* psTimeFromTM(const struct tm* time)
{
    psS64 year;
    psS64 month;
    psS64 day;
    psS64 hour;
    psS64 minute;
    psS64 seconds;
    psS64 temp;
    psTime *outTime = NULL;

    // Error check
    PS_ASSERT_PTR_NON_NULL(time,NULL);

    // Allocate psTime struct
    outTime = psTimeAlloc(PS_TIME_TAI);

    // Extract data from TM struct
    year = time->tm_year + 1900;
    month = time->tm_mon + 1;
    day = time->tm_mday;
    hour = time->tm_hour;
    minute = time->tm_min;
    seconds = time->tm_sec;

    // Make month in range 3..14 (treat Jan & Feb as months 13..14 of prev year)
    if ( month <= 2 )
    {
        temp = (14 - month) / 12;
        //        year -= (temp = (14 - month) / 12);
        year -= temp;
        month += 12 * temp;
    } else if (month > 14)
    {
        temp = (month - 3) / 12;
        //        year += (temp = (month - 3) / 12);
        year += temp;
        month -= 12 * temp;
    }

    // Make year positive
    if (year < 0 )
    {
        day -= 146097 * (temp = (399 - year) / 400);
        year += 400 * temp;
    }

    // Add day of month, days of previous 0-11 month period that began w/March, days of previous 0-399 year
    // period that began w/March of a 400-multiple year), days of any 400-year periods before that, and 306
    // days to adjust from Mar 1, year 0-relative to Jan 1, year 1-relative. Add hours, minutes, and seconds.
    day += (month * 367 - 1094) / 12 + year % 100 * 1461 / 4 + (year/100 * 36524 + year/400) - 306;

    outTime->sec = ((day - 1) * SEC_PER_DAY - 62135596800)
                   + (hour * SEC_PER_HOUR)
                   + (minute * SEC_PER_MINUTE)
                   + seconds;

    // C's TM does not define a microsecond field. Microseconds must be manipulated by calling function.
    outTime->nsec = 0;

    // Error check
    PS_ASSERT_INT_WITHIN_RANGE(outTime->nsec,0,(psU32)((1e9)-1),outTime);

    return outTime;
}

psTime* psTimeStrptime(const char *s, const char *format)
{
    PS_ASSERT_PTR_NON_NULL(s, NULL);
    PS_ASSERT_PTR_NON_NULL(format, NULL);

    double fractionalSeconds = 0.0;

    struct tm tmTime;
    tmTime.tm_year = tmTime.tm_mon = tmTime.tm_mday = 0;
    tmTime.tm_hour = tmTime.tm_min = tmTime.tm_sec = 0;
    char *lastChar = strptime(s, format, &tmTime);
    if (!lastChar) {
        // No error because this function is called repeatedly as a test.
        // Returning NULL without an error is fine, since we're just saying that it's not readable.
        return NULL;
    }

    // strptime cannot handle fractional seconds, so we do that ourselves
    if (*lastChar != '\0') {
        if (*lastChar == '.') {
            char *reallyLast;           // The real last part of the string
            fractionalSeconds = strtod(lastChar, &reallyLast);
            if (!reallyLast) {
                psWarning("Time string was not completely consumed");
            }
        }
    }

    psTime *time = psTimeFromTM(&tmTime);
    if (!time) {
        psError(PS_ERR_UNKNOWN, false, "failed to generate a psTime");
        return NULL;
    }

    time->nsec += fractionalSeconds * 1.0e9L;
    return time;
}

psString psTimeStrftime(const psTime *time, const char *format)
{
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    PS_ASSERT_PTR_NON_NULL(format, NULL);

    struct tm *tmTime = psTimeToTM(time);

    psString s = psAlloc(MAX_TIME_STRING_LENGTH);
    size_t size = strftime(s, MAX_TIME_STRING_LENGTH, format, tmTime);
    psFree(tmTime);
    // it's worth nothing that strftime() can zero without an error having
    // occured don't believe it's worth supporting that edge case.  See
    // strftime(3) for further details.
    if (size == 0) {
        psError(PS_ERR_UNKNOWN, true, "failed to stringify a psTime");
        return NULL;
    }

    return s;
}

psTime* psTimeMath(const psTime *time,
                   double delta)
{
    psF64 sec = 0.0;
    psTime *outTime = NULL;
    psTime *tempTime = NULL;

    // Error checks
    PS_ASSERT_PTR_NON_NULL(time,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(time->nsec, (psU32)0, (psU32)((1e9)-1), NULL);

    // Convert time to TAI if necessary, but without changing input arguments
    if (time->type == PS_TIME_UTC) {
        tempTime = psTimeAlloc(PS_TIME_UTC);
        tempTime->sec = time->sec;
        tempTime->nsec = time->nsec;
        psTimeConvert(tempTime, PS_TIME_TAI);
        outTime = psTimeAlloc(PS_TIME_TAI);
    } else {
        tempTime = psMemIncrRefCounter((psTime*)time);
        outTime = psTimeAlloc(time->type);
    }

    // Create output time
    sec = delta + (psF64)tempTime->sec + (psF64)tempTime->nsec/1e9;
    PS_ASSERT_S64_WITHIN_RANGE((psS64)sec, (psS64)0, PS_MAX_S64, NULL);
    outTime->sec = (psS64)sec;
    outTime->nsec = (psU32)((sec - (psF64)outTime->sec)*1e9);

    // Error check
    PS_ASSERT_INT_WITHIN_RANGE(outTime->nsec,0,(psU32)((1e9)-1), NULL);

    // Convert result to same time type as input
    if (time->type == PS_TIME_UTC) {
        psTimeConvert(outTime, PS_TIME_UTC);
    }

    psFree(tempTime);

    return outTime;
}

double psTimeDelta(const psTime *time1,
                   const psTime *time2)
{
    psF64 uSec1 = 0.0;
    psF64 uSec2 = 0.0;
    psTime *tempTime1 = NULL;
    psTime *tempTime2 = NULL;

    // Error checks
    // XXX nsec is U32, use an integer for comparison
    PS_ASSERT_PTR_NON_NULL(time1,0.0);
    PS_ASSERT_INT_WITHIN_RANGE(time1->nsec,0,(psU32)(999999999),0.0);
    PS_ASSERT_PTR_NON_NULL(time2,0.0);
    PS_ASSERT_INT_WITHIN_RANGE(time2->nsec,0,(psU32)(999999999),0.0);

    // Verify both times of the same type
    if (time1->type != time2->type) {
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,_("Specified type, %d, is incorrect."),time1->type);
        return NAN;
    }

    // Convert time to TAI if necessary, but without changing input arguments
    if (time1->type == PS_TIME_UTC) {
        tempTime1 = psTimeAlloc(PS_TIME_UTC);
        tempTime1->sec = time1->sec;
        tempTime1->nsec = time1->nsec;
        psTimeConvert(tempTime1, PS_TIME_TAI);
    } else {
        tempTime1 = psMemIncrRefCounter((psTime*)time1);
    }
    if (time2->type == PS_TIME_UTC) {
        tempTime2 = psTimeAlloc(PS_TIME_UTC);
        tempTime2->sec = time2->sec;
        tempTime2->nsec = time2->nsec;
        psTimeConvert(tempTime2, PS_TIME_TAI);
    } else {
        tempTime2 = psMemIncrRefCounter((psTime*)time2);
    }

    uSec1 = tempTime1->sec >= 0 ? 1.0 : -1.0;
    uSec1 = uSec1*tempTime1->nsec/1e9;
    uSec2 = tempTime2->sec >= 0 ? 1.0 : -1.0;
    uSec2 = uSec2*tempTime2->nsec/1e9;
    psF64 out = (tempTime1->sec-tempTime2->sec) + (uSec1-uSec2);

    psFree(tempTime1);
    psFree(tempTime2);

    return out;
}

// delay actual init until a timefunction is used
bool psTimeInit(const char *filename)
{
    // at present, this function can not fail
    p_psTimeConfigFilename(filename);

    return true;
}

void psTimeFinalize(void)
{
    p_psTimeFinalize();
}

psTime *psTimeCopy(const psTime *inTime)
{
    // Pass through NULL values!
    if (!inTime) {
        psTrace("psLib.astro", 6, "passing through NULL value");
        return NULL;
    }

    psTime *outTime = psTimeAlloc(inTime->type);
    if (outTime == NULL) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Invalid type specified in psTimeCopy.  %x", inTime->type);
        return NULL;
    }
    //    *outTime = *inTime;
    outTime->sec = inTime->sec;
    outTime->nsec = inTime->nsec;
    outTime->leapsecond = inTime->leapsecond;
    return outTime;
}

// XXX EAM : I've changed the timers to report TAI
// this makes more sense because it has monotonically increasing seconds
// would be even better if we could get a TAI value without doing a big lookup...

static psHash *timers = NULL;

// free all timers
static void psTimerFree ()
{
    p_psTimeFinalize();
    psFree(timers);
    timers = NULL;
    return;
}

// start/restart a named timer
bool psTimerStart (char *name)
{
    if (name == NULL)
        return false;
    if (timers == NULL) {
        timers = psHashAlloc (16);
    }
    psTime *start = psTimeGetNow (PS_TIME_TAI);
    psHashAdd (timers, name, start);
    psFree (start);
    return true;
}

// clear the timer, return elapsed time to date, or NAN if not previously defined
psF64 psTimerClear (char *name)
{
    if (name == NULL)
        return NAN;
    psF64 delta = psTimerMark(name);

    psTime *start = psTimeGetNow (PS_TIME_TAI);
    psHashAdd (timers, name, start);
    psFree (start);
    return delta;
}

// get current elapsed time on named timer (NAN if not defined)
psF64 psTimerMark (char *name)
{
    if (timers == NULL)
        return (NAN);
    psTime *start = psHashLookup (timers, name);
    if (start == NULL)
        return (NAN);

    psTime *mark = psTimeGetNow (PS_TIME_TAI);
    psF64  delta = psTimeDelta (mark, start);
    psFree (mark);
    return delta;
}

bool psTimerStop(void)
{
    psTimerFree();
    return true;
}


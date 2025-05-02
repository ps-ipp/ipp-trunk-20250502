/** @file  tst_psTime_01.c
 *
 *  @brief Test driver for psTime functions
 *
 *  This test driver contains the following tests for psTime:
 *     1) Allocate psTime structure
 *     2) Get current time
 *     3) Get UT1 UTC delta
 *     4) Convert psTime to MJD
 *     5) Convert psTime to JD
 *     6) Convert psTime to ISO
 *     7) Convert psTime to timeval
 *     8) Create psTime from MJD
 *     9) Create psTime from JD
 *    10) Create psTime from ISO
 *    11) Create psTime from timeval
 *    12) Create psTime from TM
 *    13) Convert time between different types
 *
 *     O) Convert psTime time to LMST
 *
 *  @author  Ross Harman, MHPCC
 *  @author  Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.7 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-06-05 01:10:22 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define ERROR_TOL    0.0001

// Test Time 1 : July 21, 2004  18:22:24.3
//               MJD = 53207.765559
//               JD = 2453208.265559
// UTC Test Time 1
const psS64 testTime1SecondsUTC     = 1090434144;
const psU32 testTime1NanosecondsUTC = 272044000;
// TAI Test Time 1
const psS64 testTime1SecondsTAI     = 1090434176;
const psU32 testTime1NanosecondsTAI = 272044000;
const psF64 testTime1MJDTAI         = 53207.76592937;
const psF64 testTime1JDTAI          = 2453208.26592937;

// TT Test Time 1
const psS64 testTime1SecondsTT      = 1090434208;
const psU32 testTime1NanosecondsTT  = 456044000;
// Expected UT1-UTC IERS A & B
const psF64 testTime1UT1DeltaBullA  = -0.457233186;
const psF64 testTime1UT1DeltaBullB  = -0.457227;
// UT1 Test Time 1
const psS64 testTime1SecondsUT1     = 1090434143;
//const psU32 testTime1NanosecondsUT1 = 814810814;
const psU32 testTime1NanosecondsUT1 = 814810861;
// Expected MJD & JD
const psF64 testTime1MJD            = 53207.765559;
const psF64 testTime1JD             = 2453208.265559;
// Expected ISO string
const char* testTime1Str     = "2004-07-21T18:22:24.2Z";
const char* testTime1StrLeap = "2004-07-21T18:22:60.2Z";
// Expected timeval values
const psS32 testTime1TimevalSec = 1090434144;
const psS32 testTime1TimevalUsec = 272044;

// Test Time 2 : Jan. 1, 1973 00:00:00.0000
//               MJD = 41683.0000
//               JD = 2441683.5000
const psS64 testTime2SecondsUTC     = 94694400;
const psU32 testTime2NanosecondsUTC = 0;

// Expected UT1-UTC IERS A & B
const psF64 testTime2UT1DeltaBullA  = 0.000000;
const psF64 testTime2UT1DeltaBullB  = 0.000000;

// Test Time 3 : Sept. 21, 2006 00:00:00.0000
//               MJD = 53999
//               JD = 2453999.5
const psS64 testTime3SecondsUTC     = 1158796800;
const psU32 testTime3NanosecondsUTC = 0;
// Expected UT1-UTC IERS A & B
const psF64 testTime3UT1DeltaBullA  = -0.63574;
const psF64 testTime3UT1DeltaBullB  = -0.63574;

// Test Time 4 : Jan. 1, 1969 00:00:00.0000
//               MJD = 40222
//               JD = 2440222.5
const psS64 testTime4SecondsUTC     = -31536000;
const psU32 testTime4NanosecondsUTC = 0;
// Expected MJD and JD
const psF64 testTime4MJD            = 40222.0;
const psF64 testTime4JD             = 2440222.5;

// Test Time 5 : Dec 31, 0001 BC 23:59:59
//               MJD = -1397755
//               JD = 1002245.4999
const psS64 testTime5SecondsUTC     = -62125920001;
const psU32 testTime5NanosecondsUTC  = 0;

// Test Time 6 : Jan. 1, 10000 AD 00:00:00
const psS64 testTime6SecondsUTC      = 253202544001;
const psU32 testTime6NanosecondsUTC  = 0;

// Test Time 7 : Jan. 1, 2004 00:00:00,0
const psS64 testTime7Seconds         = 1072915200;
const psU32 testTime7Nanoseconds     = 0;
const psS32 testTime7TmYear          = 104;
const psS32 testTime7TmMon           = 0;
const psS32 testTime7TmDay           = 1;
const psS32 testTime7TmHour          = 0;
const psS32 testTime7TmMin           = 0;
const psS32 testTime7TmSec           = 0;

// Test Time 8 : Dec. 31, 2003 00:00:00,0
const psS64 testTime8Seconds         = 1072828800;
const psU32 testTime8Nanoseconds     = 0;
const psS32 testTime8TmYear          = 103;
const psS32 testTime8TmMon           = 11;
const psS32 testTime8TmDay           = 31;
const psS32 testTime8TmHour          = 0;
const psS32 testTime8TmMin           = 0;
const psS32 testTime8TmSec           = 0;

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(198);
    // Initialize library internal structures
    psLibInit("pslib.config");


    // Allocate new psTime with unallowed time type
    // Following should generate error message
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(-100);
        ok(time == NULL, "psTimeAlloc(-100) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeAlloc(TAI)
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        ok(time != NULL, "psTimeAlloc() did not return NULL");
        skip_start(time == NULL, 4, "Skipping tests because psTimeAlloc() failed");
        ok(time->type == PS_TIME_TAI, "psTimeAlloc() correctly set psTime->type");
        ok(time->sec == 0, "psTimeAlloc() correctly set psTime->sec");
        ok(time->nsec == 0, "psTimeAlloc() correctly set psTime->nsec");
        ok(time->leapsecond == false, "psTimeAlloc() correctly set psTime->leapsecond");
        psFree(time);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeAlloc(UTC)
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        ok(time != NULL, "psTimeAlloc() did not return NULL");
        skip_start(time == NULL, 4, "Skipping tests because psTimeAlloc() failed");
        ok(time->type == PS_TIME_UTC, "psTimeAlloc() correctly set psTime->type");
        ok(time->sec == 0, "psTimeAlloc() correctly set psTime->sec");
        ok(time->nsec == 0, "psTimeAlloc() correctly set psTime->nsec");
        ok(time->leapsecond == false, "psTimeAlloc() correctly set psTime->leapsecond");
        psFree(time);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeAlloc(UT1)
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UT1);
        ok(time != NULL, "psTimeAlloc() did not return NULL");
        skip_start(time == NULL, 4, "Skipping tests because psTimeAlloc() failed");
        ok(time->type == PS_TIME_UT1, "psTimeAlloc() correctly set psTime->type");
        ok(time->sec == 0, "psTimeAlloc() correctly set psTime->sec");
        ok(time->nsec == 0, "psTimeAlloc() correctly set psTime->nsec");
        ok(time->leapsecond == false, "psTimeAlloc() correctly set psTime->leapsecond");
        psFree(time);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeAlloc(TT)
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TT);
        ok(time != NULL, "psTimeAlloc() did not return NULL");
        skip_start(time == NULL, 4, "Skipping tests because psTimeAlloc() failed");
        ok(time->type == PS_TIME_TT, "psTimeAlloc() correctly set psTime->type");
        ok(time->sec == 0, "psTimeAlloc() correctly set psTime->sec");
        ok(time->nsec == 0, "psTimeAlloc() correctly set psTime->nsec");
        ok(time->leapsecond == false, "psTimeAlloc() correctly set psTime->leapsecond");
        psFree(time);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeGetNow()
    // Attempt to get time with unallowed type
    // Following should generate an error message for unallowed time type
    {
        psMemId id = psMemGetId();
        psTime *timeNow = psTimeGetNow(-100);
        ok(timeNow == NULL, "psTimeGetNow(-100) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeGetNow(TAI)
    {
        psMemId id = psMemGetId();
        psTime *timeNow = psTimeGetNow(PS_TIME_TAI);
        ok(timeNow != NULL, "psTimeGetNow() returned NULL");
        skip_start(time == NULL, 4, "Skipping tests because psTimeGetNow() failed");
        ok(timeNow->type == PS_TIME_TAI, "psTimeGetNow() correctly set psTime->type");
        ok(timeNow->sec != 0, "psTimeAlloc() set psTime->sec to something");
        ok(timeNow->nsec != 0, "psTimeAlloc() set psTime->nsec to something");
        psFree(timeNow);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeGetUT1Delta()
    // Attempt to convert NULL time
    // psTimeGetUT1Delta() should generate an error message for NULL time
    {
        psMemId id = psMemGetId();
        psF64 ut1Delta = psTimeGetUT1Delta(NULL, PS_IERS_B);
        is_double(ut1Delta, NAN, "psTimeGetUT1Delta(NULL, PS_IERS_B) returned NAN");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to convert unallowed time
    // Following should generate an error message for incorrect time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = 1;
        time->nsec = 2e9;
        time->leapsecond = false;
        psF64 ut1Delta = psTimeGetUT1Delta(time, PS_IERS_B);
        is_double(ut1Delta, NAN, "psTimeGetUT1Delta() returned NAN for incorrect time");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to convert time with unallowed bulletin
    // Following should generate an error message for incorrect bulletin
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = 1;
        time->nsec = 2;
        time->leapsecond = false;
        psF64 ut1Delta = psTimeGetUT1Delta(time, -100);
        is_double(ut1Delta, NAN, "psTimeGetUT1Delta(time, -100) returned NAN (incorrect bulletin)");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to get delta with valid time and bulletin A
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec  = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->type = PS_TIME_UTC;
        time->leapsecond = false;
        psF64 ut1Delta = psTimeGetUT1Delta(time, PS_IERS_A);

        is_double_tol(ut1Delta, testTime1UT1DeltaBullA, ERROR_TOL, "psTimeGetUT1Delta() produced the correct result: bulletin A");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to get delta with valid time and bulletin B
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec  = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->type = PS_TIME_UTC;
        psF64 ut1Delta = psTimeGetUT1Delta(time, PS_IERS_B);

        is_double_tol(ut1Delta, testTime1UT1DeltaBullB, ERROR_TOL, "psTimeGetUT1Delta() produced the correct result: bulletin B");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to get delta with valid time and bulletin A
    // Following should generate a warning message predating table
    // XXX: We don't test whether the warning message is generated
    if (1) {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = testTime2SecondsUTC;
        time->nsec = testTime2NanosecondsUTC;
        time->type = PS_TIME_UTC;
        psF64 ut1Delta = psTimeGetUT1Delta(time,PS_IERS_A);

        is_double_tol(ut1Delta, testTime2UT1DeltaBullA, ERROR_TOL, "psTimeGetUT1Delta() produced the correct result: bulletin B");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to get delta with valid time and bulletin B
    // Following should generate a warning message predating table
    // XXX: We don't test whether the warning message is generated
    if (1) {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = testTime2SecondsUTC;
        time->nsec = testTime2NanosecondsUTC;
        time->type = PS_TIME_UTC;
        psF64 ut1Delta = psTimeGetUT1Delta(time,PS_IERS_B);

        is_double_tol(ut1Delta, testTime2UT1DeltaBullB, ERROR_TOL, "psTimeGetUT1Delta() produced the correct result: bulletin B");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to get delta with valid time and bulletin A
    // Following should generate a warning message postdating table
    // XXX: We don't test whether the warning message is generated
    if (1) {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec  = testTime3SecondsUTC;
        time->nsec = testTime3NanosecondsUTC;
        time->type = PS_TIME_UTC;
        psF64 ut1Delta = psTimeGetUT1Delta(time,PS_IERS_A);

        is_double_tol(ut1Delta, testTime3UT1DeltaBullA, ERROR_TOL, "psTimeGetUT1Delta() produced the correct result: bulletin B");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to get delta with valid time and bulletin B
    // Following should generate a warning message postdating table
    // XXX: We don't test whether the warning message is generated
    if (1) {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec  = testTime3SecondsUTC;
        time->nsec = testTime3NanosecondsUTC;
        time->type = PS_TIME_UTC;
        psF64 ut1Delta = psTimeGetUT1Delta(time,PS_IERS_B);

        is_double_tol(ut1Delta, testTime3UT1DeltaBullB, ERROR_TOL, "psTimeGetUT1Delta() produced the correct result: bulletin B");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToMJD()
    // Attempt to convert with time NULL
    // Following should generate an error message for NULL time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psF64 mjd = psTimeToMJD(NULL);
        is_double(mjd, NAN, "psTimeToMJD(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToMJD()
    // Attempt to convert incorrect time
    // Following should generate an error message for incorrect time
    // Following should generate an error message for NULL time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = 1;
        time->nsec = 2e9;
        time->leapsecond = false;
        psF64 mjd = psTimeToMJD(time);
        is_double(mjd, NAN, "psTimeToMJD() returned NAN for incorrect time");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToMJD()
    // Check valid time conversion to MJD after 1/1/1970 epoch
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        psF64 mjd = psTimeToMJD(time);
        is_double_tol(mjd, 53207.765929, ERROR_TOL, "psTimeToMJD() returned correct time after 1/1/1970 epoch");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToMJD()
    // Check valid time conversion to MJD before 1/1/1970 epoch
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime4SecondsUTC;
        time->nsec = testTime4NanosecondsUTC;
        psF64 mjd = psTimeToMJD(time);
        is_double_tol(mjd, testTime4MJD, ERROR_TOL, "psTimeToMJD() returned correct time before 1/1/1970 epoch");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToJD()
    // Attempt to convert with time NULL
    // Following should generate an error message for NULL time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psF64 jd = psTimeToJD(NULL);
        is_double(jd, NAN, "psTimeToJD(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToJD()
    // Attempt to convert incorrect time
    // Following should generate an error message for incorrect time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = 1;
        time->nsec = 2e9;
        time->leapsecond = false;
        psF64 jd = psTimeToJD(time);
        is_double(jd, NAN, "psTimeToJD() returned NAN for incorrect time");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToJD()
    // Check valid time conversion to MJD after 1/1/1970 epoch
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        psF64 jd = psTimeToJD(time);
        is_double_tol(jd, 2453208.265929, ERROR_TOL, "psTimeToJD() returned the correct time after 1/1/1970 epoch");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToJD()
    // Check valid time conversion to MJD before 1/1/1970 epoch
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime4SecondsUTC;
        time->nsec = testTime4NanosecondsUTC;
        psF64 jd = psTimeToJD(time);
        is_double_tol(jd, testTime4JD, ERROR_TOL, "psTimeToJD() returned the correct time before 1/1/1970 epoch");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToISO()
    // Attempt to convert with NULL time
    // Following should generate error message for NULL time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        char *timeStr = psTimeToISO(NULL);
        ok(timeStr == NULL, "psTimeToISO(NULL) returned NULL");
        psFree(timeStr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToISO()
    // Attempt to convert incorrect time
    // Following should generate an error message for incorrect time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->leapsecond = false;
        time->sec = 1;
        time->nsec = 2e9;
        char *timeStr = psTimeToISO(time);
        ok(timeStr == NULL, "psTimeToISO(time) returned NULL for incorrect time");
        psFree(time);
        psFree(timeStr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToISO()
    // Verify return NULL for time prior to year 0000
    // Following should generate error message for time prior year 0000
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime5SecondsUTC;
        time->nsec = testTime5NanosecondsUTC;
        char *timeStr = psTimeToISO(time);
        ok(timeStr == NULL, "psTimeToISO(time) returned NULL for time prior to year 0000");
        psFree(time);
        psFree(timeStr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToISO()
    // Verify return NULL for time after to year 9999
    // Following should generate error message for time after year 9999
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->leapsecond = false;
        time->sec = testTime6SecondsUTC;
        time->nsec = testTime6NanosecondsUTC;
        char *timeStr = psTimeToISO(time);
        ok(timeStr == NULL, "psTimeToISO(time) returned NULL for time after year 9999");
        psFree(time);
        psFree(timeStr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToISO()
    // Verify return string with valid time
    // XXX: These tests fail.  They used to succeed in early 2006, I think
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->leapsecond = false;
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        char *timeStr = psTimeToISO(time);
        is_str(timeStr, testTime1Str, "psTimeToISO(time) returned correct time (no leapsecond)");
        psFree(timeStr);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToISO()
    // Verify return string with valid time and leap second set
    // XXX: These tests fail.  They used to succeed in early 2006, I think
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->leapsecond = true;
        char *timeStr = psTimeToISO(time);
        is_str(timeStr, testTime1StrLeap, "psTimeToISO(time) returned correct time (with leapsecond)");
        psFree(timeStr);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToTimeval()
    // Attempt to convert with NULL time
    // Following should generate error message for NULL time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        struct timeval *timevalTime = psTimeToTimeval(NULL);

        ok(timevalTime == NULL, "psTimeToTimeval(NULL) returned NULL");

        psFree(timevalTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToTimeval()
    // Attempt to convert incorrect time
    // Following should generate an error message for incorrect time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->leapsecond = false;
        time->sec = 1;
        time->nsec = 2e9;
        struct timeval *timevalTime = psTimeToTimeval(time);
        ok(timevalTime == NULL, "psTimeToTimeval() returned NULL for incorrect time");
        psFree(time);
        psFree(timevalTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToTimeval()
    // Attempt to convert incorrect time
    // Following should generate an error message for incorrect time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->leapsecond = false;
        time->sec = -1;
        time->nsec = 0;
        struct timeval *timevalTime = psTimeToTimeval(time);
        ok(timevalTime == NULL, "psTimeToTimeval() returned NULL for incorrect time");
        psFree(time);
        psFree(timevalTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeToTimeval()
    // Verify convert to timeval with valid time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->leapsecond = false;
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        struct timeval *timevalTime = psTimeToTimeval(time);
        is_long(timevalTime->tv_sec, testTime1TimevalSec, "psTimeToTimeval()->tv_sec");
        is_long(timevalTime->tv_usec, testTime1TimevalUsec, "psTimeToTimeval()->tv_usec");
        psFree(time);
        psFree(timevalTime);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromMJD()
    // Attempt to convert valid time to psTime
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromMJD(testTime1MJDTAI);
        ok(time->type == PS_TIME_TAI, "psTimeFromMJD() returned the correct type");
        is_long(time->sec, testTime1SecondsTAI, "psTimeFromMJD()->sec");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromMJD()
    // Attempt to convert valid time before 1970 epoch
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromMJD(testTime4MJD);
        ok(time->type == PS_TIME_TAI, "psTimeFromMJD() returned the correct type");
        is_long(time->sec, testTime4SecondsUTC, "psTimeFromMJD()->sec");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeFromJD()
    // Attempt to convert valid time to psTime
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromJD(testTime1JDTAI);
        ok(time->type == PS_TIME_TAI, "psTimeFromJD() returned the correct type");
        is_long(time->sec, testTime1SecondsTAI, "psTimeFromJD()->sec");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeFromJD()
    // Attempt to convert valid time before 1970 epoch
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromJD(testTime4JD);

        ok(time->type == PS_TIME_TAI, "psTimeFromJD() returned the correct type");
        is_long(time->sec, testTime4SecondsUTC, "psTimeFromJD() returned the correct ->sec");

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromISO()
    // Convert valid ISO string
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromISO(testTime1Str, PS_TIME_TAI);
        ok(time->type == PS_TIME_TAI, "psTimeFromISO() returned the correct type");
        is_long(time->sec, testTime1SecondsUTC, "psTimeFromISO()->sec");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromISO()
    // Attempt to convert NULL string
    // Following should generate error message for NULL ISO string");
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromISO(NULL, PS_TIME_TAI);
        ok(time == NULL, "psTimeFromISO(NULL, PS_TIME_TAI) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromISO()
    // Attempt to convert incorrect ISO string
    // Following should generate an error for incorrect ISO string");
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromISO("Here I am", PS_TIME_TAI);
        ok(time == NULL, "psTimeFromISO() returned NULL for incorrect ISO string");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeFromTimeval()
    // Attempt to create psTime from NULL timeval ptr
    // Following should generate error message for NULL timeval
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromTimeval(NULL);
        ok(time == NULL, "psTimeFromTimeval(NULL) returned NULL");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psTimeFromTimeval()
    // Convert valid timeval structure
    {
        psMemId id = psMemGetId();
        struct timeval *timevalTime = (struct timeval*)psAlloc(sizeof(struct timeval));
        timevalTime->tv_sec = testTime1SecondsTAI;
        timevalTime->tv_usec = testTime1NanosecondsTAI / 1000;
        psTime *time = psTimeFromTimeval(timevalTime);
        ok(time != NULL, "psTimeFromTimeval() returned non-NULL for correct timeval structure");
        skip_start(time == NULL, 3, "Skipping tests because psTimeFromTimeval() returned NULL");
        ok(time->type == PS_TIME_TAI, "psTimeFromTimeval() returned the correct type");
        is_long(time->sec, testTime1SecondsTAI, "psTimeFromTimeval() returned the correct ->sec");
        is_long(time->nsec, testTime1NanosecondsTAI, "psTimeFromTimeval() returned the correct ->nsec");
        skip_end();
        psFree(timevalTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromTM()
    // Attempt to convert from NULL tm structure ptr
    // Following should generate error message for NULL tm ptr
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromTM(NULL);
        ok(time == NULL, "psTimeFromTM(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromTM()
    // Verify convert for valid tm structure
    {
        psMemId id = psMemGetId();
        struct tm *tmTime = (struct tm*)psAlloc(sizeof(struct tm));
        tmTime->tm_year = testTime7TmYear;
        tmTime->tm_mon  = testTime7TmMon;
        tmTime->tm_mday = testTime7TmDay;
        tmTime->tm_hour = testTime7TmHour;
        tmTime->tm_min  = testTime7TmMin;
        tmTime->tm_sec  = testTime7TmSec;
        psTime *time = psTimeFromTM(tmTime);
        ok(time != NULL, "psTimeFromTM(NULL) returned non-NULL");
        skip_start(time == NULL, 2, "Skipping tests because psTimeFromTM() returned NULL");
        is_long(time->sec, testTime7Seconds, "psTimeFromTM() returned the correct ->sec");
        is_long(time->nsec, testTime7Nanoseconds, "psTimeFromTM() returned the correct ->nsec");
        skip_end();
        psFree(tmTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromTM()
    // Verify convert for valid tm structure
    {
        psMemId id = psMemGetId();
        struct tm *tmTime = (struct tm*)psAlloc(sizeof(struct tm));
        tmTime->tm_year = testTime8TmYear;
        tmTime->tm_mon  = testTime8TmMon;
        tmTime->tm_mday = testTime8TmDay;
        tmTime->tm_hour = testTime8TmHour;
        tmTime->tm_min  = testTime8TmMin;
        tmTime->tm_sec  = testTime8TmSec;
        psTime *time = psTimeFromTM(tmTime);
        // XXX should test all fields here
        ok(time != NULL, "psTimeFromTM(NULL) returned non-NULL");
        skip_start(time == NULL, 2, "Skipping tests because psTimeFromTM() returned NULL");
        is_long(time->sec, testTime8Seconds, "psTimeFromTM() returned the correct ->sec");
        is_long(time->nsec, testTime8Nanoseconds, "psTimeFromTM() returned the correct ->nsec");
        skip_end();
        psFree(tmTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert NULL time
    // Following should generate an error message for NULL time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        bool status= psTimeConvert(NULL, PS_TIME_TAI);

        ok(status== false, "psTimeConvert(NULL, PS_TIME_TAI) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Following should generate an error message for incorrect type output
    // Input psTime struct is PS_TIME_TAI
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        bool status = psTimeConvert(time1,-100);
        ok(status == false, "psTimeConvert(time1, -100) returned false");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Following should generate an error message for incorrect type output
    // Input psTime struct is PS_TIME_UTC
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        bool status = psTimeConvert(time1,-100);
        ok(status == false, "psTimeConvert(time1, -100) returned false");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Following should generate an error message for incorrect type
    // Input psTime struct is PS_TIME_TT
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TT);
        bool status = psTimeConvert(time1,-100);
        ok(status == false, "psTimeConvert(time1, -100) returned false");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Following should generate an error message for incorrect type input
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        time1->type = -100;
        bool status = psTimeConvert(time1,PS_TIME_TAI);
        ok(status == false, "psTimeConvert(time1, PS_TIME_TAI) returned false");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert with incorrect time nsec > 1e9
    // Following should generate an error message for incorrect time
    // XXX: We don't test whether the error message is generated
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        time1->nsec = 2e9;
        bool status = psTimeConvert(time1, PS_TIME_TAI);
        ok(status == false, "psTimeConvert(time1, PS_TIME_TAI) returns NULL for incorrect psTime object");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    //Attempt to convert a time to the same type
    //Should return true because time->type == type
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        time1->sec = 1;
        time1->nsec = 2;
        time1->type = PS_TIME_TAI;
        time1->leapsecond = false;
        bool status = psTimeConvert(time1, PS_TIME_TAI);
        ok(status == true, "psTimeConvert(time, ...) returns true for conversion to same type");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a UTC time to a TAI
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_TAI);
        ok(status == true, "psTimeConvert(time, ...) returns true after conversion to a different type");
        is_long(time1->sec, testTime1SecondsTAI, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsTAI, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_TAI, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a UTC time to a TT
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_TT);
        ok(status == true, "psTimeConvert(time, ...) returns true after conversion to a different type");
        is_long(time1->sec, testTime1SecondsTT, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsTT, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_TT, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a UTC time to UT1
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_UT1);
        ok(status == true, "psTimeConvert(time, ...) returns true after conversion to a different type");
        is_long(time1->sec, testTime1SecondsUT1, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsUT1, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_UT1, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psTimeConvert()
    // Attempt to convert a TAI time to UTC
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        time1->sec = testTime1SecondsTAI;
        time1->nsec = testTime1NanosecondsTAI;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_UTC);
        ok(status == true, "psTimeConvert(time, ...) returns true after conversion to a different type");
        is_long(time1->sec, testTime1SecondsUTC, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsUTC, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_UTC, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a TAI time to TT
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        time1->sec = testTime1SecondsTAI;
        time1->nsec = testTime1NanosecondsTAI;
        time1->type = PS_TIME_TAI;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_TT);
        ok(status == true, "psTimeConvert() returned true for conversion to same type");
        is_long(time1->sec, testTime1SecondsTT, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsTT, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_TT, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a TAI time to UT1
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TAI);
        time1->sec = testTime1SecondsTAI;
        time1->nsec = testTime1NanosecondsTAI;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_UT1);
        ok(status == true, "psTimeConvert() returned true for conversion to same type");
        is_long(time1->sec, testTime1SecondsUT1, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsUT1, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_UT1, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a TT time to UTC
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TT);
        time1->sec = testTime1SecondsTT;
        time1->nsec = testTime1NanosecondsTT;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_UTC);
        ok(status == true, "psTimeConvert() returned true for conversion to same type");
        is_long(time1->sec, testTime1SecondsUTC, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsUTC, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_UTC, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a TT time to TAI
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TT);
        time1->sec = testTime1SecondsTT;
        time1->nsec = testTime1NanosecondsTT;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_TAI);
        ok(status == true, "psTimeConvert() returned true for conversion to same type");
        is_long(time1->sec, testTime1SecondsTAI, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsTAI, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_TAI, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert a TT time to UT1
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_TT);
        time1->sec = testTime1SecondsTT;
        time1->nsec = testTime1NanosecondsTT;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1,PS_TIME_UT1);
        ok(status == true, "psTimeConvert() returned true for conversion to same type");
        is_long(time1->sec, testTime1SecondsUT1, "psTimeConvert() returned the correct ->sec");
        is_long(time1->nsec, testTime1NanosecondsUT1, "psTimeConvert() returned the correct ->nsec");
        ok(time1->type == PS_TIME_UT1, "psTimeConvert() returned the correct type");
        is_bool(time1->leapsecond, false, "time->leapsecond");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // Attempt to convert from UT1 to TAI, UTC, TT
    // Following should generate an error message converting from UT1");
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UT1);
        time1->sec = 1;
        time1->nsec = 2;
        time1->leapsecond = false;

        bool status = psTimeConvert(time1, PS_TIME_UTC);
        ok(status == false, "psTimeConvert() returned false for conversion from UT1");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

/** @file  tst_psTime_02.c
 *
 *  @brief Test driver for psTime functions
 *
 *  This test driver contains the following tests for psTime:
 *     1) Convert psTime to Local Mean Sidereal Time (LMST)
 *     2) Calculate leap second delta between times
 *     3) Creation of psTime of type TT
 *     4) Creation of psTime of type UTC
 *
 *  @author  Ross Harman, MHPCC
 *  @author  Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.6 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-06-05 01:10:22 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

#define ERROR_TOL  0.001
// Test Time 1 : May 9, 2005  00:00:00,0
//               MJD = 53499.000
//               JD = 2453499.5
// UTC Test Time 1
const psS64 testTime1SecondsUTC     = 1115596900;
const psU32 testTime1NanosecondsUTC = 0;
// Expected LMST  15:09:18
const psF64 testTime1LMST0          = 3.967604;

// Test Time 2 : May 9, 1995 00:00:00,0
//               MJD = 49846.00
//               JD = 2449846.5
// UTC Test Time 2
const psS64 testTime2SecondsUTC     = 799977600;
const psU32 testTime2NanosecondsUTC = 0;
// Expected leap second delta
const psS64 testTimeLeapSecondDelta1 = 3;

// Test Time 3: Jan 1, 1999 00:00:00,0
//              MJD = 51179
//              JD = 2451179.5
const psS64 testTime3SecondsUTC     = 915148800;
const psU32 testTime3NanosecondsUTC = 0;

int main(int argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);
    psLogSetFormat("HLNM");
    plan_tests(54);
    psLibInit("pslib.config");

    // psTimeToLMST()
    // Attempt to get LMST with NULL time
    // Following should generate an error message for NULL time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psF64 lmst = psTimeToLMST(NULL, 0);
        is_double(lmst, NAN, "psTimeToLMST(NULL, 0) returned NAN");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToLMST()
    // Attempt to get LMST with valid test time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->leapsecond = false;
        psF64 lmst = psTimeToLMST(time, 0.0);
        is_double_tol(lmst, testTime1LMST0, ERROR_TOL, "psTimeToLMST() returned the correct time");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeToLMST()
    // Attempt to get LMST with unallowed input time UT1
    // Following should generate error message for incorrect type
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->leapsecond = false;
        time->type = PS_TIME_UT1;
        psF64 lmst = psTimeToLMST(time,0.0);
        is_double(lmst, NAN, "psTimeToLMST() generated a NAN for incorrect type");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Attempt to get delta with NULL time1 argument
    // Following should generate an error message for NULL time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psS64 delta = psTimeLeapSecondDelta(NULL, NULL);
        is_long(delta, 0, "psTimeLeapSecondDelta(NULL, NULL) returned 0");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Set test time 1
    // Attempt to get delta with NULL time2 argument
    // Following should generate an error message for NULL time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;
        psS64 delta = psTimeLeapSecondDelta(time1, NULL);
        is_long(delta, 0, "psTimeLeapSecondDelta(time1, NULL) returned 0");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Set test time 2 with unallowed time
    // Attempt to get delta with unallowed time2
    // Following should generate an error message for unallowed time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = 0;
        time2->nsec = 2e9;
        time2->leapsecond = false;
        psS64 delta = psTimeLeapSecondDelta(time1, time2);
        is_long(delta, 0, "psTimeLeapSecondDelta(time1, time2) returned 0 with incorrect time2");
        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Set test time 2 valid
    // Attempt to get delta with unallowed time1
    // Following should generate an error message for unallowed time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->leapsecond = false;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->leapsecond = false;
        time2->sec = testTime2SecondsUTC;
        time2->nsec = testTime2NanosecondsUTC;
        // Set test time 1 incorrect
        time1->sec = 0;
        time1->nsec = 2e9;
        psS64 delta = psTimeLeapSecondDelta(time1,time2);
        is_long(delta, 0, "psTimeLeapSecondDelta(time1, time2) returned 0 with incorrect time1");
        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Set test time 1 to greater time
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->leapsecond = false;
        time2->sec = testTime2SecondsUTC;
        time2->nsec = testTime2NanosecondsUTC;
        psS64 delta = psTimeLeapSecondDelta(time1,time2);
        is_long(delta, testTimeLeapSecondDelta1, "psTimeLeapSecondDelta() produced the correct result");
        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Set test time 1 to lesser time
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = testTime2SecondsUTC;
        time2->nsec = testTime2NanosecondsUTC;
        psS64 delta = psTimeLeapSecondDelta(time2, time1);
        is_long(delta, testTimeLeapSecondDelta1, "psTimeLeapSecondDelta() produced the correct result");
        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeLeapSecondDelta()
    // Attempt to get delta with times equal
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        time1->leapsecond = false;
        psS64 delta = psTimeLeapSecondDelta(time1,time1);
        is_long(delta, 0, "psTimeLeapSecondDelta() produced the correct result");
        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeIsLeapSecond()
    // Attempt to determine if leap second with NULL time
    // Following should generate an error message for NULL time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        bool leapsecond = psTimeIsLeapSecond(NULL);
        is_bool(leapsecond, false, "psTimeIsLeapSecond(NULL) returned correct value");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeIsLeapSecond()
    // Attempt to determine if leap second with non-UTC time
    // Following should generate an error message for unallowed type
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->leapsecond = false;
        bool leapsecond = psTimeIsLeapSecond(time);
        is_bool(leapsecond, false, "psTimeIsLeapSecond() returned false with incorrect type");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeIsLeapSecond()
    // Set time to UTC
    // Attempt to determine if leap second with valid time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        time->leapsecond = false;
        time->type = PS_TIME_UTC;
        bool leapsecond = psTimeIsLeapSecond(time);
        is_bool(leapsecond, false, "psTimeIsLeapSecond() returned false");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeIsLeapSecond()
    // Set time to UTC with leap second
    // Note: leapseconds are only relevent for UTC
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime3SecondsUTC;
        time->nsec = testTime3NanosecondsUTC;
        bool leapsecond = psTimeIsLeapSecond(time);
        is_bool(leapsecond, true, "psTimeIsLeapSecond() returned true");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeIsLeapSecond()
    // Set time to 1 second before a known leap second
    // Note: leapseconds are only relevent for UTC
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime3SecondsUTC;
        time->nsec = testTime3NanosecondsUTC;
        time->sec--;
        bool leapsecond = psTimeIsLeapSecond(time);
        is_bool(leapsecond, false, "psTimeIsLeapSecond() returned false");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeIsLeapSecond()
    // Set time to 1 second after a known leap second
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = testTime3SecondsUTC;
        time->nsec = testTime3NanosecondsUTC;
        time->sec++;
        bool leapsecond = psTimeIsLeapSecond(time);
        is_bool(leapsecond, false, "psTimeIsLeapSecond() returned false");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromTT()
    // Attempt to create psTime with unallowed time
    // Following should generate an error message for unallowed time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromTT(0,2e9);
        ok(time == NULL, "psTimeFromTT() returned NULL with incorrect time");
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromTT()
    // Attempt to create psTime with valid time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromTT(testTime1SecondsUTC,testTime1NanosecondsUTC);
        ok(time != NULL, "psTimeFromTT() returned non-NULL with valid time");
        skip_start(time == NULL, 2, "Skipping tests because psTimeFromTT() returned NULL");
        ok(time->type == PS_TIME_TT, "psTimeFromTT() returned correct type");
        is_long(time->sec, testTime1SecondsUTC, "psTimeFromTT() returned correct ->sec"); 
        is_long(time->nsec, testTime1NanosecondsUTC, "psTimeFromTT() returned correct ->nsec");
        is_bool(time->leapsecond, false, "psTimeFromTT() returned the correct leapsecond flag");
        skip_end();
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromUTC()
    // Attempt to create psTime with unallowed time
    // Following should generate an error message for unallowed time
    // XXX: We do not test the error generation
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromUTC(0, 2e9, true);
        ok(time == NULL, "psTimeFromUTC() returned NULL with incorrect time input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromUTC()
    // Attempt to create psTime with valid leapsecond time but leapsecond
    // flag false
    {
        psMemId id = psMemGetId();

        psTime *time = psTimeFromUTC(testTime3SecondsUTC, testTime3NanosecondsUTC, false);
        ok(time != NULL, "psTimeFromUTC() returned non-NULL with correct input");
        skip_start(time == NULL, 5, "Skipping tests because psTimeFromUTC() returned NULL");
        ok(time->type == PS_TIME_UTC, "psTimeFromUTC() returned the correct type");
        is_long(time->sec, testTime3SecondsUTC, "psTimeFromUTC() returned the correct ->sec");
        is_long(time->nsec, testTime3NanosecondsUTC, "psTimeFromUTC() returned the correct ->nsec");
        is_bool(time->leapsecond, true, "psTimeFromUTC() returned the correct leapsecond flag");
        psFree(time);
        skip_end();

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeFromUTC()
    // Attempt to create psTime with valid non-leapsecond time and
    // leapsecond flag true
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeFromUTC(testTime1SecondsUTC, testTime1NanosecondsUTC, true);
        ok(time != NULL, "psTimeFromUTC() returned non-NULL with correct input");
        skip_start(time == NULL, 3, "Skipping tests because psTimeFromUTC() returned NULL");
        ok(time->type == PS_TIME_UTC, "psTimeFromUTC() returned the correct type");
        is_long(time->sec, testTime1SecondsUTC, "psTimeFromUTC() returned the correct ->sec and ->nsec");
        is_long(time->nsec, testTime1NanosecondsUTC, "psTimeFromUTC() returned the correct ->sec and ->nsec");
        is_bool(time->leapsecond, false, "psTimeFromUTC() returned the correct leapsecond flag");
        skip_end();

        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

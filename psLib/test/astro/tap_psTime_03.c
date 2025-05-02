/** @file  tst_psTime_03.c
 *
 *  @brief Test driver for psTime functions
 *
 *  This test driver contains the following tests for psTime:
 *   1) psTimeMath invalid times
 *   2) psTimeMath valid time of different types
 *   3) psTimeDelta valid times with different types
 *
 *  @author  Ross Harman, MHPCC
 *  @author  Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.7 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-06-21 23:53:33 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define ERROR_TOL    0.001

// Test Time 1 : May 9, 2005 00:00:00,0
//               MJD = 53499.00
//               JD = 2453499.5
// UTC Test Time 1
const psS64 testTime1SecondsUTC         = 1115596900;
const psU32 testTime1NanosecondsUTC     = 0;
// TAI Test Time 1
const psS64 testTime1SecondsTAI         = 1115596932;
const psU32 testTime1NanosecondsTAI     = 0;
// TT Test Time 1
const psS64 testTime1SecondsTT          = 1115596964;
const psU32 testTime1NanosecondsTT      = 184000000;
// UT1 Test Time 1
const psS64 testTime1SecondsUT1         = 1115596900;
const psU32 testTime1NanosecondsUT1     = 184000000;
// Delta 1
const psF64 deltaTime1                  = -15.5;
// Expected UTC Time 1
const psS64 newTestTime1SecondsUTC      = 1115596884;
const psU32 newTestTime1NanosecondsUTC  = 500000000;
// Expected TAI Time 1
const psS64 newTestTime1SecondsTAI      = 1115596916;
const psU32 newTestTime1NanosecondsTAI  = 500000000;
// Delta 2
const psF64 deltaTime2                  = 123.066;
// Expected TT Time 1 w/ delta 2
const psS64 newTestTime1SecondsTT       = 1115597087;
const psU32 newTestTime1NanosecondsTT   = 250000000;
// Expected UT1 Time 1 w/ delta 2
const psS64 newTestTime1SecondsUT1      = 1115597023;
const psU32 newTestTime1NanosecondsUT1  = 250000000;

// Test Time 2 : Dec. 31 1998 23:59:45,0
//               MJD = 51178.99983
//               JD = 2451179.49983
// UTC Test Time 1
const psS64 testTime2SecondsUTC         = 915148785;
const psU32 testTime2NanosecondsUTC     = 0;
// Delta 3
const psF64 deltaTime3                    = 30.0;
// Expected UTC Time
const psS64 newTestTime2SecondsUTC      = 915148814;
const psU32 newTestTime2NanosecondsUTC  = 0;

// Appendix B time conversion tests
//
#define APPB_TESTS    8
const psS64 testTimeBSeconds[APPB_TESTS] =
    {
        915148829,
        915148829,
        915148830,
        915148830,
        915148831,
        915148831,
        915148832,
        915148832
    };
const psU32 testTimeBNanoseconds[APPB_TESTS] =
    {
        0,
        500000000,
        0,
        500000000,
        0,
        500000000,
        0,
        500000000
    };
const psBool testTimeBLeapsecond[APPB_TESTS] =
    {
        false,
        false,
        false,
        false,
        true,
        true,
        false,
        false
    };
// Expected results
const char* testTimeBStrUTC[APPB_TESTS] =
    {
        "1998-12-31T23:59:58.0Z",
        "1998-12-31T23:59:58.5Z",
        "1998-12-31T23:59:59.0Z",
        "1998-12-31T23:59:59.5Z",
        "1998-12-31T23:59:60.0Z",
        "1998-12-31T23:59:60.5Z",
        "1999-01-01T00:00:00.0Z",
        "1999-01-01T00:00:00.5Z"
    };
const char* testTimeBStrTAI[APPB_TESTS] =
    {
        "1999-01-01T00:00:29.0Z",
        "1999-01-01T00:00:29.5Z",
        "1999-01-01T00:00:30.0Z",
        "1999-01-01T00:00:30.5Z",
        "1999-01-01T00:00:31.0Z",
        "1999-01-01T00:00:31.5Z",
        "1999-01-01T00:00:32.0Z",
        "1999-01-01T00:00:32.5Z"
    };
const char* testTimeBStrTT[APPB_TESTS] =
    {
        "1999-01-01T00:01:01.1Z",
        "1999-01-01T00:01:01.6Z",
        "1999-01-01T00:01:02.1Z",
        "1999-01-01T00:01:02.6Z",
        "1999-01-01T00:01:03.1Z",
        "1999-01-01T00:01:03.6Z",
        "1999-01-01T00:01:04.1Z",
        "1999-01-01T00:01:04.6Z"
    };
const char* testTimeBStrUT1[APPB_TESTS] =
    {
        "1998-12-31T23:59:58.7Z",
        "1998-12-31T23:59:59.2Z",
        "1998-12-31T23:59:59.7Z",
        "1998-12-31T23:59:60.2Z",
        "1998-12-31T23:59:60.7Z",
        "1999-01-01T00:00:00.2Z",
        "1999-01-01T00:00:00.7Z",
        "1999-01-01T00:00:01.2Z"
    };

// Test Time B1 : Dec 31, 1998 23:59:58,0
//                MJD = 51178.99998
//                JD = 2451179.49998
//const psS64 testTimeB1SecondsUTC        = 915148798;
//const psU32 testTimeB1NanosecondsUTC    = 0;
// Expected ISO times
//const char testTimeB1StrUTC[] = "1998-12-31T23:59:58,0Z";
//const char testTimeB1StrTAI[] = "1999-01-01T00:00:29,0Z";
//const char testTimeB1StrTT[]  = "1999-01-01T00:01:01,1Z";
//const char testTimeB1StrUT1[] = "1998-12-31T23:59:57,7Z";
//
// Test Time B2 : Dec 31, 1998 23:59:58,5
//
//
//const psS64 testTimeB2SecondsUTC       = 915148798;
//const psU32 testTimeB2NanosecondsUTC   = 500000000;
// Expected ISO times
//const char testTimeB2StrUTC[] = "1998-12-31T23:59:58,5Z";
//const char testTimeB2StrTAI[] = "1991-01-01T00:00:29,5Z";
//const char testTimeB2StrTT[]  = "1999-01-01T00:01:01,6Z";
//const char testTimeB2StrUT1[] = "1998-12-31T23:59:58,2Z";

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(77);
    psLibInit("pslib.config");


    // psTimeMath()
    // Attempt to perform math operation on NULL time
    // Following should generate error message for NULL time
    // XXX: We do not test error generation here
    {
        psMemId id = psMemGetId();
        ok(psTimeMath(NULL, -1.1) == NULL, "psTimeMath(NULL, -1.1) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeMath()
    // Set up input time with invalid nanoseconds
    // Following should generate error message for invalid time
    // XXX: We do not test error generation here
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = 0;
        time->nsec = 2e9;
        ok(psTimeMath(time, -1.1) == NULL, "psTimeMath() returns NULL for unallowable time input");
        psFree(time);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeMath()
    // Set up input time with valid time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime1SecondsUTC;
        time->nsec = testTime1NanosecondsUTC;
        psTime *newTime = psTimeMath(time, deltaTime1);
        ok(newTime != NULL, "psTimeMath() returns non-NULL for allowable time input (PS_TIME_UTC)");
        skip_start(newTime == NULL, 2, "Skipping tests because psTimeMath() returned NULL");
        ok(newTime->type == PS_TIME_UTC, "psTimeMath() returns the correct type (PS_TIME_UTC)");
        is_long(newTime->sec, newTestTime1SecondsUTC, "psTimeMath() returns the correct ->sec");
        is_long(newTime->nsec, newTestTime1NanosecondsUTC, "psTimeMath() returns the correct ->nsec");
        skip_end();
        psFree(newTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeMath()
    // Set up input time with valid TAI time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TAI);
        time->sec = testTime1SecondsTAI;
        time->nsec = testTime1NanosecondsTAI;

        psTime *newTime = psTimeMath(time, deltaTime1);
        ok(newTime != NULL, "psTimeMath() returns non-NULL for allowable time input (PS_TIME_TAI)");
        skip_start(newTime == NULL, 2, "Skipping tests because psTimeMath() returned NULL");
        ok(newTime->type == PS_TIME_TAI, "psTimeMath() returns the correct type (PS_TIME_TAI)");
        is_long(newTime->sec, newTestTime1SecondsTAI, "psTimeMath() returns the correct ->sec");
        is_long(newTime->nsec, newTestTime1NanosecondsTAI, "psTimeMath() returns the correct ->nsec");
        skip_end();

        psFree(newTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeMath()
    // Set up input time with valid TT time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_TT);
        time->sec = testTime1SecondsTT;
        time->nsec = testTime1NanosecondsTT;

        psTime *newTime = psTimeMath(time,deltaTime2);
        ok(newTime != NULL, "psTimeMath() returns non-NULL for allowable time input (PS_TIME_TT)");
        skip_start(newTime == NULL, 2, "Skipping tests because psTimeMath() returned NULL");
        ok(newTime->type == PS_TIME_TT, "psTimeMath() returns the correct type (PS_TIME_TT)");
        is_long(newTime->sec, newTestTime1SecondsTT, "psTimeMath() returns the correct ->sec (PS_TIME_TT)");
        is_long(newTime->nsec, newTestTime1NanosecondsTT, "psTimeMath() returns the correct ->nsec (PS_TIME_TT)");
        skip_end();

        psFree(newTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeMath()
    // Set up input time with valid TT time
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UT1);
        time->sec = testTime1SecondsUT1;
        time->nsec = testTime1NanosecondsUT1;

        psTime *newTime = psTimeMath(time,deltaTime2);
        ok(newTime != NULL, "psTimeMath() returns non-NULL for allowable time input (PS_TIME_UT1)");
        skip_start(newTime == NULL, 2, "Skipping tests because psTimeMath() returned NULL");
        ok(newTime->type == PS_TIME_UT1, "psTimeMath() returns the correct type (PS_TIME_UT1)");
        is_long(newTime->sec, newTestTime1SecondsUT1, "psTimeMath() returns the correct ->sec (PS_TIME_UT1)");
        is_long(newTime->nsec, newTestTime1NanosecondsUT1, "psTimeMath() returns the correct ->nsec (PS_TIME_UT1)");
        skip_end();

        psFree(newTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeMath()
    // Set up input time with valid UTC time and delt which crosses leap second
    {
        psMemId id = psMemGetId();
        psTime *time = psTimeAlloc(PS_TIME_UTC);
        time->sec = testTime2SecondsUTC;
        time->nsec = testTime2NanosecondsUTC;

        psTime *newTime = psTimeMath(time, deltaTime3);
        ok(newTime != NULL, "psTimeMath() returns non-NULL for allowable time input (PS_TIME_UTC)");
        skip_start(newTime == NULL, 2, "Skipping tests because psTimeMath() returned NULL");
        ok(newTime->type == PS_TIME_UTC, "psTimeMath() returns the correct type (PS_TIME_UTC)");
        is_long(newTime->sec, newTestTime2SecondsUTC, "psTimeMath() returns the correct ->sec (PS_TIME_UTC)");
        is_long(newTime->nsec, newTestTime2NanosecondsUTC, "psTimeMath() returns the correct ->nsec (PS_TIME_UTC)");
        skip_end();

        psFree(newTime);
        psFree(time);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Attempt to get delta with time1 NULL
    // Following should generate error message for NULL time
    // XXX: We do not test error generation here
    {
        psMemId id = psMemGetId();

        psF64 delta = psTimeDelta(NULL, NULL);
        is_double_tol(delta, 0.0, ERROR_TOL, "psTimeDelta(NULL, NULL) returned 0.0");

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Attempt to get delta with time2 NULL
    // Following should generate error message for NULL time
    // XXX: We do not test error generation here
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);

        psF64 delta = psTimeDelta(time1, NULL);
        is_double_tol(delta, 0.0, ERROR_TOL, "psTimeDelta(time1, NULL) returned 0.0");

        psFree(time1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Attempt to get delta with time1 invalid
    // Following should generate error message for invalid time
    // XXX: We do not test error generation here
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = 0;
        time1->nsec = 2e9;

        psF64 delta = psTimeDelta(time1, time2);
        is_double_tol(delta, 0.0, ERROR_TOL, "psTimeDelta(time1, NULL) returned 0.0 for unallowed time1 arg");

        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Attempt to get delta with time2 invalid
    // Following should generate error message for invalid time
    // XXX: We do not test error generation here
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = 0;
        time2->nsec = 2e9;

        psF64 delta = psTimeDelta(time1, time2);
        is_double_tol(delta, 0.0,  ERROR_TOL, "psTimeDelta(time1, NULL) returned 0.0 for unallowed time2 arg");

        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Set time 2 valid but different type
    // Attempt to get delta with different time types
    // Following should generate error message for incorrect type
    // XXX: We do not test error generation here
    // XXX: this currently fails; probably because specs have changed
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        psTime *time2 = psTimeAlloc(PS_TIME_TAI);
        time2->sec = newTestTime1SecondsUTC;
        time2->nsec = newTestTime1NanosecondsUTC;

        psF64 delta = psTimeDelta(time1, time2);
        is_double_tol(delta, 0.0, ERROR_TOL, "psTimeDelta(time1, NULL) returned 0.0 for different time types");

        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Set time 2 to same as time 1
    // Attempt to get delta with valid times of the same type
    // XXX: this currently fails; probably because specs have changed
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsUTC;
        time1->nsec = testTime1NanosecondsUTC;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = newTestTime1SecondsUTC;
        time2->nsec = newTestTime1NanosecondsUTC;

        psF64 delta = psTimeDelta(time2, time1);
        is_double_tol(delta, deltaTime1, ERROR_TOL, "psTimeDelta(time1, NULL) returned correct delta for times of the same type");

        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Set time 1 and 2 as different times with same type
    // Attempt to get delta with valid times of the same type
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime1SecondsTT;
        time1->nsec = testTime1NanosecondsTT;
        time1->type = PS_TIME_TT;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = newTestTime1SecondsTT;
        time2->nsec = newTestTime1NanosecondsTT;
        time2->type = PS_TIME_TT;

        psF64 delta = psTimeDelta(time2,time1);
        is_double_tol(delta, deltaTime2, ERROR_TOL, "psTimeDelta(time1, NULL) returned correct delta for times of the same type");

        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeDelta()
    // Set time 1 and 2 as different times with same type
    // Attempt to get delta with valid times of the same type
    {
        psMemId id = psMemGetId();
        psTime *time1 = psTimeAlloc(PS_TIME_UTC);
        time1->sec = testTime2SecondsUTC;
        time1->nsec = testTime2NanosecondsUTC;
        time1->type = PS_TIME_UTC;
        psTime *time2 = psTimeAlloc(PS_TIME_UTC);
        time2->sec = newTestTime2SecondsUTC;
        time2->nsec = newTestTime2NanosecondsUTC;
        time2->type = PS_TIME_UTC;
        
        psF64 delta = psTimeDelta(time2,time1);
        is_double_tol(delta, deltaTime3, ERROR_TOL, "psTimeDelta(time1, NULL) returned correct delta for times of the same type");

        psFree(time1);
        psFree(time2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psTimeConvert()
    // XXX: This currently produces lots of errors
    {
        for(psS32 i = 0; i < APPB_TESTS; i++)
        {
            // Initialize time for test time B1
            psTime *time = psTimeAlloc(PS_TIME_TAI);
            time->sec = testTimeBSeconds[i];
            time->nsec = testTimeBNanoseconds[i];

            // Verify TAI ISO string
            char *timeStr = psTimeToISO(time);
            is_str(timeStr, testTimeBStrTAI[i], "TAI ISO string");
            psFree(timeStr);

            psTimeConvert(time, PS_TIME_TT);
            timeStr = psTimeToISO(time);
            is_str(timeStr, testTimeBStrTT[i], "TT ISO string");
            psFree(timeStr);

            // Verify UTC ISO string
            psTimeConvert(time, PS_TIME_UTC);
            time->leapsecond = testTimeBLeapsecond[i];
            timeStr = psTimeToISO(time);
            is_str(timeStr, testTimeBStrUTC[i], "UTC ISO string");
            psFree(timeStr);

            psTimeConvert(time, PS_TIME_UT1);
            timeStr = psTimeToISO(time);
            is_str(timeStr, testTimeBStrUT1[i], "UT1 ISO string");
            psFree(timeStr);

            psFree(time);
        }

    }
}

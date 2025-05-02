/** @file  tst_psTime_04.c
 *
 *  @brief Test driver for psTime functions
 *
 *  This test driver contains the following tests for psTime:
 *      Test A - Initialize time
 *      Test B - Attempt to open non-existant time config file
 *      Test C - Attempt to open non-existant time data files
 *      Test D - Attempt to read incorrect number of files
 *      Test E - Attempt to read incorrect number of from values
 *      Test F - Attempt to read data file with typo in number
 *      Test G - Free data
 *      Test H - Attempt to use all timer functions
 *      Test I - Tidal Corrections to UT1-UTC
 *
 *  @author  Ross Harman, MHPCC
 *  @author  Eric Van Alst, MHPCC
 *  @author  David Robbins, MHPCC
 *
 *  @version $Revision: 1.3 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-06-04 20:25:32 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(19);


    // Test A - Initialize time
    // XXX: Noting is actually verified here
    {
        psMemId id = psMemGetId();
        psLibInit("pslib.config");
        psLibFinalize();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test B - Attempt to open non-existant time config file
    // XXX: Noting is actually verified here
    {
        psMemId id = psMemGetId();
        psLibInit("zzz");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test C - Attempt to open non-existant time data files
    // XXX: Noting is actually verified here
    {
        psMemId id = psMemGetId();
        psLibInit("test.psTime.config1");
        psLibFinalize();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test D - Attempt to read incorrect number of files
    // XXX: Noting is actually verified here
    {
        psMemId id = psMemGetId();
        psLibInit("test.psTime.config2");
        psLibFinalize();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test E - Attempt to read incorrect number of from values
    // XXX: Noting is actually verified here
    {
        psMemId id = psMemGetId();
        psLibInit("test.psTime.config3");
        psLibFinalize();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test F - Attempt to read data file with typo in number
    // XXX: Noting is actually verified here
    {
        psMemId id = psMemGetId();
        psLibInit("test.psTime.config4");
        psLibFinalize();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testTimer1()
    {
        psMemId id = psMemGetId();
        psF64 testTime = 0.0;
        psF64 testTime2 = 0.0;
        psF64 testTime3 = 0.0;

        ok(psTimerStart("newTime"), "psTimerStart successful");
        testTime = psTimerMark("newTime");
        ok(testTime != 0.0, "psTimerMark() successful");
        testTime2 = psTimerClear("newTime");
        ok(testTime2 != 0.0, "psTimerClear() successful");
        psTimerStart("newTime");
        testTime3 = psTimerStop();
        ok(testTime3 != 0.0, "psTimerStop() successful");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testTideUT1Corr()
    // Verify NULL return with NULL input
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psTime *empty = psTime_TideUT1Corr(NULL);
        ok(empty == NULL, "psTime_TideUT1Corr() returned NULL for NULL input time");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testTideUT1Corr()
    // 
    {
        psMemId id = psMemGetId();
        psTime *noTide = psTimeAlloc(PS_TIME_UTC);
        noTide->sec = 1049160600;
        noTide->nsec = 0;
        noTide->leapsecond = false;
        psTime *empty = psTime_TideUT1Corr(noTide);
        is_long(empty->sec, 1049160599, "psTime_TideUT1Corr() returned correct ->sec");
        is_long(empty->nsec, 656981971, "psTime_TideUT1Corr() returned correct ->nsec");
        ok(p_psTimeFinalize(), "p_psTimeFinalize() successful");
        psFree(empty);
        psFree(noTide);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

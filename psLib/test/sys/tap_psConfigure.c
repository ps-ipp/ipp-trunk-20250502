/** @file  tst_psConfigure.c
 *
 *  @brief Test driver for psconfigure functions
 *
 *  This test driver contains the following test points for psConfigure
 *  functions.
 *    1) Return current psLib version
 *
 *  Return:   Number of test points which failed
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.3 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-04-10 21:09:31 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "pslib.h"
#include "tap.h"
#include "pstap.h"
#include "string.h"


psS32 main(psS32 argc, char* argv[])
{
    plan_tests(2);

    // Simple test of psLibVersion()
    // XX: Must somehow verify the output is correct.
    {
        psMemId id = psMemGetId();
        char *stringVal = NULL;
        stringVal = psLibVersion();
        ok(stringVal != NULL && strlen(stringVal), "Version is cool");
        psFree(stringVal);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


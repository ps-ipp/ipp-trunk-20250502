/** @file tst_pmVersion.c
 *
 *  @brief Contains the tests for pmVersion.c:
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-09-18 18:59:00 $
 *
 *  XX: The output version string for psModulesVersionLong() is not verified.
 *  Not sure how to do this since this psModulesVersionLong() produces the
 *  time that the source code, not the test code, was compiled.
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
#include "config.h"
#define ERR_TRACE_LEVEL         0
#define VERBOSE			0

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel(".", 0);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(4);


    // --------------------------------------------------------------------
    // psModulesVersion() tests
    // XX: The output string is not verified
    {
        psMemId id = psMemGetId();
        psString tstStr = NULL;
        psStringAppend(&tstStr, "%s-%s",PACKAGE_NAME,PACKAGE_VERSION);
        psString str = psModulesVersion();
        ok(!strcmp(str, tstStr), "psModulesVersion()");
        psFree(str);
        psFree(tstStr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // psModulesVersionLong() tests
    // XX: The output string is not verified
    {
        psMemId id = psMemGetId();
        psString str = psModulesVersionLong();
        ok(str, "psModulesVersionLong()");
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

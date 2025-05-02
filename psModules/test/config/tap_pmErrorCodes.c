/** @file tst_pmErrorCodes.c
 *
 *  @brief Contains the tests for pmErrorCodes.c:
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-09-18 18:58:58 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
#define ERR_TRACE_LEVEL         0
#define VERBOSE			0

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel(".", 0);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(16);


    // --------------------------------------------------------------------
    // pmErrorRegister() tests
    // Test pmErrorRegister() with currently coded error types
    {
        psMemId id = psMemGetId();
        pmErrorRegister();
        char *errStr;
        errStr = (char *) psErrorCodeString(PM_ERR_BASE);
        ok(!strcmp("First value we use; lower values belong to psLib", errStr),
           "psErrorCodeString(PM_ERR_BASE)");

        errStr = (char *) psErrorCodeString(PM_ERR_UNKNOWN);
        ok(!strcmp("Unknown psModules error code", errStr),
           "psErrorCodeString(PM_ERR_UNKNOWN)");

        errStr = (char *) psErrorCodeString(PM_ERR_PHOTOM);
        ok(!strcmp("Problem in photometry", errStr),
           "psErrorCodeString(PM_ERR_PHOTOM)");

        errStr = (char *) psErrorCodeString(PM_ERR_PSF);
        ok(!strcmp("Problem in PSF", errStr),
           "psErrorCodeString(PM_ERR_PSF)");

        errStr = (char *) psErrorCodeString(PM_ERR_ASTROM);
        ok(!strcmp("Problem in astrometry", errStr),
           "psErrorCodeString(PM_ERR_ASTROM)");

        errStr = (char *) psErrorCodeString(PM_ERR_CAMERA);
        ok(!strcmp("Problem in camera", errStr),
           "psErrorCodeString(PM_ERR_CAMERA)");

        errStr = (char *) psErrorCodeString(PM_ERR_CONCEPTS);
        ok(!strcmp("Problem in concepts", errStr),
           "psErrorCodeString(PM_ERR_CONCEPTS)");

        errStr = (char *) psErrorCodeString(PM_ERR_IMCOMBINE);
        ok(!strcmp("Problem in imcombine", errStr),
           "psErrorCodeString(PM_ERR_IMCOMBINE)");

        errStr = (char *) psErrorCodeString(PM_ERR_OBJECTS);
        ok(!strcmp("Problem in objects", errStr),
           "psErrorCodeString(PM_ERR_OBJECTS)");

        errStr = (char *) psErrorCodeString(PM_ERR_SKY);
        ok(!strcmp("Problem in sky", errStr),
           "psErrorCodeString(PM_ERR_SKY)");

        errStr = (char *) psErrorCodeString(PM_ERR_ARGUMENTS);
        ok(!strcmp("Incorrect arguments", errStr),
           "psErrorCodeString(PM_ERR_ARGUMENTS)");

        errStr = (char *) psErrorCodeString(PM_ERR_SYS);
        ok(!strcmp("System error", errStr),
           "psErrorCodeString(PM_ERR_SYS)");

        errStr = (char *) psErrorCodeString(PM_ERR_CONFIG);
        ok(!strcmp("Problem in configure files", errStr),
           "psErrorCodeString(PM_ERR_CONFIG)");

        errStr = (char *) psErrorCodeString(PM_ERR_PROG);
        ok(!strcmp("Programming error", errStr),
           "psErrorCodeString(PM_ERR_PROG)");

        errStr = (char *) psErrorCodeString(PM_ERR_DATA);
        ok(!strcmp("invalid data", errStr),
           "psErrorCodeString(PM_ERR_DATA)");

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


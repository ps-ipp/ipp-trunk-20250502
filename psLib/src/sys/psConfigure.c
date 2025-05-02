/** @file  psConfigure.c
 *
 *  @brief Contains the declarations for initialization, memory finalization,
 *   and configuration.
 *
 *  These functions initalize psLib data before the beginning of a run and
 *  remove (finalize) the same data after the run is complete. A function is
 *  also provided to return the current psLib version.
 *
 *  @ingroup Configure
 *
 *  @author Ross Harman, MHPCC
 *  @author Robert DeSonia, MHPCC
 *  @author Paul Price, IfA
 *
 *  Copyright 2004-2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fitsio.h>
#include <longnam.h>
#include <fftw3.h>
#include <gsl/gsl_version.h>
#include <gsl/gsl_errno.h>
#ifdef HAVE_PSDB
#include <mysql.h>
#endif

#include "psAbort.h"
#include "psTrace.h"
#include "psString.h"
#include "psTime.h"
#include "psThread.h"
#include "psEarthOrientation.h"
#include "psError.h"
#include "psFFT.h"
#include "psConfigure.h"
#include "psMemory.h"

#include "psVersionDefinitions.h"

static char *memCheckName = NULL;       // Filename to which to write results of mem check
static FILE *memCheckFile = NULL;       // File to which to write results of mem check

#ifndef PSLIB_VERSION
#error "PSLIB_VERSION is not set"
#endif
#ifndef PSLIB_BRANCH
#error "PSLIB_BRANCH is not set"
#endif
#ifndef PSLIB_SOURCE
#error "PSLIB_SOURCE is not set"
#endif

psString psLibVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PSLIB_BRANCH, PSLIB_VERSION);
    return value;
}

psString psLibRevision(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s", PSLIB_VERSION);
    return value;
}

psString psLibSource(void)
{
    return psStringCopy(PSLIB_SOURCE);
}

psString psLibDependencies(void)
{
    psString deps = NULL;               // Dependencies, to return
    float cfitsioVersion;               // CFITSIO version number
    psStringAppend(&deps, "cfitsio-%.3f gsl-%s %s",
                   fits_get_version(&cfitsioVersion), gsl_version, fftwf_version);

#ifdef HAVE_PSDB
    psStringAppend(&deps, " mysql-%s", mysql_get_client_info());
#endif

    return deps;
}

psString psLibVersionLong(void)
{
    psString version = psLibVersion();  // Version, to return
    psString source = psLibSource();    // Source
    psString deps = psLibDependencies();// Dependencies

    psStringPrepend(&version, "psLib ");
    psStringAppend(&version, " from %s, built %s, %s with %s",
                   source, __DATE__, __TIME__, deps);
    psFree(source);
    psFree(deps);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

#ifdef PS_NO_TRACE
    psStringAppend(&version, " without trace");
#else
    psStringAppend(&version, " with trace");
#endif

    return version;
};

// Check the memory; intended for use on exit, but might be used elsewhere
void p_psMemoryCheck(void)
{
    if (!memCheckName || strlen(memCheckName) == 0) {
        return;
    }

    memCheckFile = fopen(memCheckName, "w"); // File to write leaks to
    if (!memCheckFile) {
        psError(PS_ERR_IO, true, "Unable to open leaks file, %s\n", memCheckName);
        return;
    }

    int nLeaks = psMemCheckLeaks(0, NULL, memCheckFile, false); // Number of leaks
    if (nLeaks > 0) {
        psWarning("%d memory leaks found; list written to %s.\n", nLeaks, memCheckName);
    } else {
        psLogMsg(__func__, PS_LOG_INFO, "No memory leaks found.\n");
    }

    int nCorrupted;                     // Number of corrupted memory blocks
    nCorrupted = psMemCheckCorruption(memCheckFile, false);
    if (nCorrupted > 0) {
        psWarning("%d memory blocks corrupted; list written to %s.\n", nCorrupted, memCheckName);
    } else {
        psLogMsg(__func__, PS_LOG_INFO, "No memory corruption found.\n");
    }

    fclose(memCheckFile);

    return;
}


bool psLibInit(const char* timeConfig)
{
    // XXX: Still needs error codes to be set

  gsl_set_error_handler_off();

    if (timeConfig && strlen(timeConfig) > 0) {
        if (!psTimeInit(timeConfig)) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed to initialize %s."), "psTime");
            return false;
        }
    }

    // Does the user want memory checking at exit?
    memCheckName = getenv("PS_ALLOC_CHECK"); // The value of PS_ALLOC_CHECK
    if (memCheckName && strlen(memCheckName) > 0) {
        atexit(&p_psMemoryCheck);
    }

    return true;
}

void psLibFinalize(void)
{
    // Users of persistent memory should free them in this function

    // Stop timers
    psTimerStop();

    // Clean up FFTW threads
    psFFTThreads(0);

    // Clean up threads
    psThreadPoolFinalize();

    // Free the time tables
    if (!p_psTimeFinalize()) {
        psAbort(_("Failed to finalize psTime."));
    }

    // Free the precession tables
    if (!p_psEOCFinalize()) {
        psAbort(_("Failed to finalize psEOC."));
    }

    // Free the trace system
    psTraceReset();

    // Free the error system
    psErrorClear();
}

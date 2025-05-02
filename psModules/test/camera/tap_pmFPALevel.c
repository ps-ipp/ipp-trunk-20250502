#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
    XXX: Add tests for bad input parameters.
*/

#define	ERR_TRACE_LEVEL		0

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(14);

    // ----------------------------------------------------------------------
    // pmFPALevelToName(): tests
    // const char *pmFPALevelToName(pmFPALevel level)
    {
        psMemId id = psMemGetId();
        char *str = (char *) pmFPALevelToName(PM_FPA_LEVEL_NONE);
        ok(!strcmp(str, "NONE"), "pmFPALevelToName(PM_FPA_LEVEL_NONE)");

        str = (char *) pmFPALevelToName(PM_FPA_LEVEL_FPA);
        ok(!strcmp(str, "FPA"), "pmFPALevelToName(PM_FPA_LEVEL_FPA)");

        str = (char *) pmFPALevelToName(PM_FPA_LEVEL_CHIP);
        ok(!strcmp(str, "CHIP"), "pmFPALevelToName(PM_FPA_LEVEL_CHIP)");

        str = (char *) pmFPALevelToName(PM_FPA_LEVEL_CELL);
        ok(!strcmp(str, "CELL"), "pmFPALevelToName(PM_FPA_LEVEL_CELL)");

        str = (char *) pmFPALevelToName(PM_FPA_LEVEL_READOUT);
        ok(!strcmp(str, "READOUT"), "pmFPALevelToName(PM_FPA_LEVEL_READOUT)");

        // XXX: We avoid this because pmFPALevelToName() aborts
        if (0) {
            str = (char *) pmFPALevelToName(-1);
            ok(str == NULL, "pmFPALevelToName(-1)");
	}

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPALevelFromName(): tests
    {
        psMemId id = psMemGetId();
        pmFPALevel lev = pmFPALevelFromName("NONE");
        ok(lev == PM_FPA_LEVEL_NONE, "pmFPALevelToName(NONE)");

        lev = pmFPALevelFromName("FPA");
        ok(lev == PM_FPA_LEVEL_FPA, "pmFPALevelToName(FPA)");

        lev = pmFPALevelFromName("CHIP");
        ok(lev == PM_FPA_LEVEL_CHIP, "pmFPALevelToName(CHIP)");

        lev = pmFPALevelFromName("CELL");
        ok(lev == PM_FPA_LEVEL_CELL, "pmFPALevelToName(CELL)");

        lev = pmFPALevelFromName("READOUT");
        ok(lev == PM_FPA_LEVEL_READOUT, "pmFPALevelToName(READOUT)");

        lev = pmFPALevelFromName("BOGUS");
        ok(lev == PM_FPA_LEVEL_NONE, "pmFPALevelToName(BOGUS)");

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

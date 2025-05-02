/** @file tst_pmConfigMask.c
 *
 *  @brief Contains the tests for pmConfigMask.c:
 *
 * This code will test the pmConfigMask() routine.
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-01-02 20:49:10 $
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
    plan_tests(7);


    // --------------------------------------------------------------------
    // pmConfigMask() tests
    // Test pmConfigMask() with NULL masks input param
    // XXX: I think the memory leak is in pmConfigRead()
    {
        psMemId id = psMemGetId();
        psString str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "dataFiles/SampleIPPConfig";
        psS32 argc = 3;
        pmConfig *config = pmConfigRead(&argc, str, "RecipeName");
        if (!config) {
            str[2] = "../dataFiles/SampleIPPConfig";
            config = pmConfigRead(&argc, str, "RecipeName");
	}
        ok(config != NULL, "pmConfigRead() successful");
        ok(!pmConfigMask(NULL, config), "pmConfigMask() returned NULL with NULL masks input param");
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmConfigMask() with NULL pmConfig input param
    // XXX: This fails on current CVS code because there is no assert in pmConfigMask() for a null pmConfig param
    if (0) {
        psMemId id = psMemGetId();
        char *masks = "Mask0 Mask1 Mask2";
        ok(!pmConfigMask(masks, NULL), "pmConfigMask() returned NULL with NULL pmConfig input param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmConfigMask() with acceptable input params
    // XXX: I think the memory leak is in pmConfigRead()
    {
        psMemId id = psMemGetId();
        // See file ../dataFiles/recipes_masks.config (
        char *masks = "DETECTOR RANGE";
        psMaskType correctMask = 0x02 | 0x04;
        psString str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "dataFiles/SampleIPPConfig";
        psS32 argc = 3;
        pmConfig *config = pmConfigRead(&argc, str, "RecipeName");
        if (!config) {
            str[2] = "../dataFiles/SampleIPPConfig";
            config = pmConfigRead(&argc, str, "RecipeName");
	}
        ok(config != NULL, "pmConfigRead() successful");
        skip_start(config == NULL, 2, "Skipping tests because pmConfigRead() failed");
        psMaskType mask = pmConfigMask(masks, config);
        ok(mask, "pmConfigMask returned non-zero with acceptable input params");
        ok(mask == correctMask, "pmConfigMask() generated the correct output mask (%x).  Should be (%x)", mask, correctMask);
        skip_end();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

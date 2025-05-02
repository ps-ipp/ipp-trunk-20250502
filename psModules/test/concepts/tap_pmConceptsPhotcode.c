#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(25);


    // pmConceptsPhotcodeForView() tests: NULL pmFPAfile input
    // psString pmConceptsPhotcodeForView(pmConfig *config, pmFPAfile *file, const pmFPAview *view)
    {
        psMemId id = psMemGetId();
        pmConfig *config =pmConfigAlloc();
        pmFPAfile *file = pmFPAfileAlloc();
        pmFPAview *view = pmFPAviewAlloc(32);
        ok(NULL == pmConceptsPhotcodeForView(NULL, view),
          "pmConceptsPhotcodeForView(config, NULL, view) returned NULL");
        psFree(config);
        psFree(file);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmConceptsPhotcodeForView() tests: NULL pmFPAview input
    // psString pmConceptsPhotcodeForView(pmConfig *config, pmFPAfile *file, const pmFPAview *view)
    {
        psMemId id = psMemGetId();
        pmConfig *config =pmConfigAlloc();
        pmFPAfile *file = pmFPAfileAlloc();
        pmFPAview *view = pmFPAviewAlloc(32);
        ok(NULL == pmConceptsPhotcodeForView(file, NULL),
          "pmConceptsPhotcodeForView(config, file, NULL) returned NULL");
        psFree(config);
        psFree(file);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmConceptsPhotcodeForView() tests: acceptable inputs
    // psString pmConceptsPhotcodeForView(pmConfig *config, pmFPAfile *file, const pmFPAview *view)
    {
        psMemId id = psMemGetId();
        psString str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
//        str[2] = "../dataFiles/SampleIPPConfig";
        str[2] = "../config/data/SampleIPPConfig";
        psS32 argc = 3;
        pmConfig *config = pmConfigRead(&argc, str, "RecipeName");
        ok(config, "pmConfigRead() returned non-NULL");
        pmFPAfile *file = pmFPAfileAlloc();
        // XXX: Insert code to read a pmFPAfile correctly
        pmFPAview *view = pmFPAviewAlloc(0);

        skip_start(!config, 2, "Skipping tests because pmConfigRead() failed");        
        bool rc;
        psMetadata *recipe  = psMetadataLookupPtr(&rc, config->recipes, "PPIMAGE");
        char *rule = psMetadataLookupStr(&rc, recipe, "PHOTCODE.RULE");
        psString goodPhotcode = pmFPAfileNameFromRule(rule, file, view);

        psString testPhotcode = pmConceptsPhotcodeForView(file, view);
        ok(testPhotcode, "pmConceptsPhotcodeForView(config, file, view) returned non-NULL");
        ok(!strcmp(goodPhotcode, testPhotcode), "pmConceptsPhotcodeForView() produced the correct string");
        skip_end();
        psFree(config);
        psFree(file);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


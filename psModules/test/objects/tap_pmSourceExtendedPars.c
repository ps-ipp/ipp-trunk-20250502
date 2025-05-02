#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
        All functions are tested.
*/

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(35);

    // ----------------------------------------------------------------------
    // pmSourceExtendedParsAlloc() tests
    {
        psMemId id = psMemGetId();
        pmSourceExtendedPars *tmp = pmSourceExtendedParsAlloc();
        ok(tmp && psMemCheckSourceExtendedPars(tmp), "pmSourceExtendedParsAlloc() allocated a pmSourceExtendedPars struct");

        ok(tmp->profile == NULL, "pmSourceExtendedParsAlloc() set the ->profile member to NULL");
        ok(tmp->annuli == NULL, "pmSourceExtendedParsAlloc() set the ->annuli member to NULL");
        ok(tmp->isophot == NULL, "pmSourceExtendedParsAlloc() set the ->isophot member to NULL");
        ok(tmp->petrosian == NULL, "pmSourceExtendedParsAlloc() set the ->petrosian member to NULL");
        ok(tmp->kron == NULL, "pmSourceExtendedParsAlloc() set the ->kron member to NULL");

        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceRadialProfileAlloc() tests
    {
        psMemId id = psMemGetId();
        pmSourceRadialProfile *tmp = pmSourceRadialProfileAlloc();
        ok(tmp && psMemCheckSourceRadialProfile(tmp), "pmSourceRadialProfile() allocated a pmSourceRadialProfilestruct");

        ok(tmp->radius == NULL, "pmSourceRadialProfileAlloc() set the ->radius member to NULL");
        ok(tmp->flux == NULL, "pmSourceRadialProfileAlloc() set the ->flux member to NULL");
        ok(tmp->variance == NULL, "pmSourceRadialProfileAlloc() set the ->variance member to NULL");

        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceIsophotalValuesAlloc() tests
    {
        psMemId id = psMemGetId();
        pmSourceIsophotalValues *tmp = pmSourceIsophotalValuesAlloc();
        ok(tmp && psMemCheckSourceIsophotalValues(tmp), "pmSourceIsophotalValues() allocated a pmSourceIsophotalValuesstruct");

        ok(tmp->mag == 0.0, "pmSourceIsophotalValuesAlloc() set the ->mag member to 0.0");
        ok(tmp->magErr == 0.0, "pmSourceIsophotalValuesAlloc() set the ->magErr member to 0.0");
        ok(tmp->rad == 0.0, "pmSourceIsophotalValuesAlloc() set the ->rad member to 0.0");
        ok(tmp->radErr == 0.0, "pmSourceIsophotalValuesAlloc() set the ->radErr member to 0.0");

        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourcePetrosianValuesAlloc() tests
    {
        psMemId id = psMemGetId();
        pmSourcePetrosianValues *tmp = pmSourcePetrosianValuesAlloc();
        ok(tmp && psMemCheckSourcePetrosianValues(tmp), "pmSourcePetrosianValues() allocated a pmSourcePetrosianValuesstruct");

        ok(tmp->mag == 0.0, "pmSourcePetrosianValuesAlloc() set the ->mag member to 0.0");
        ok(tmp->magErr == 0.0, "pmSourcePetrosianValuesAlloc() set the ->magErr member to 0.0");
        ok(tmp->rad == 0.0, "pmSourcePetrosianValuesAlloc() set the ->rad member to 0.0");
        ok(tmp->radErr == 0.0, "pmSourcePetrosianValuesAlloc() set the ->radErr member to 0.0");

        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceKronValuesAlloc() tests
    {
        psMemId id = psMemGetId();
        pmSourceKronValues *tmp = pmSourceKronValuesAlloc();
        ok(tmp && psMemCheckSourceKronValues(tmp), "pmSourceKronValues() allocated a pmSourceKronValuesstruct");

        ok(tmp->mag == 0.0, "pmSourceKronValuesAlloc() set the ->mag member to 0.0");
        ok(tmp->magErr == 0.0, "pmSourceKronValuesAlloc() set the ->magErr member to 0.0");
        ok(tmp->rad == 0.0, "pmSourceKronValuesAlloc() set the ->rad member to 0.0");
        ok(tmp->radErr == 0.0, "pmSourceKronValuesAlloc() set the ->radErr member to 0.0");

        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceAnnuliAlloc() tests
    {
        psMemId id = psMemGetId();
        pmSourceAnnuli *tmp = pmSourceAnnuliAlloc();
        ok(tmp && psMemCheckSourceAnnuli(tmp), "pmSourceAnnuli() allocated a pmSourceAnnulistruct");

        ok(tmp->flux == NULL, "pmSourceAnnuliAlloc() set the ->flux member to NULL");
        ok(tmp->fluxErr == NULL, "pmSourceAnnuliAlloc() set the ->fluxErr member to NULL");
        ok(tmp->fluxVar == NULL, "pmSourceAnnuliAlloc() set the ->fluxVar member to NULL");

        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

}

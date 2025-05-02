#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(11);

    // Test pmMomentsAlloc()
    {
        psMemId id = psMemGetId();
        pmMoments *tmpMoments = pmMomentsAlloc();
        ok(tmpMoments != NULL, "pmMomentsAlloc() returned a non-NULL pmMoments");
        skip_start(tmpMoments == NULL, 9, "Skipping tests because pmMomentsAlloc() returned NULL");
        ok(tmpMoments->Mx == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->My == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Mxx == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Myy == 0.0, "pmMomentsAlloc set->x correctly");

        ok(tmpMoments->Mxxx == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Mxxy == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Mxyy == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Myyy == 0.0, "pmMomentsAlloc set->x correctly");

        ok(tmpMoments->Mxxxx == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Mxxxy == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Mxxyy == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Mxyyy == 0.0, "pmMomentsAlloc set->x correctly");
        ok(tmpMoments->Myyyy == 0.0, "pmMomentsAlloc set->x correctly");

        ok(tmpMoments->Sum == 0.0, "pmMomentsAlloc set->Sy correctly");
        ok(tmpMoments->Peak == 0.0, "pmMomentsAlloc set->Sxy correctly");
        ok(tmpMoments->Sky == 0.0, "pmMomentsAlloc set->Sum correctly");
        ok(tmpMoments->dSky == 0.0, "pmMomentsAlloc set->Peak correctly");
        ok(tmpMoments->SN == 0.0, "pmMomentsAlloc set->Sky correctly");
        ok(tmpMoments->nPixels == 0, "pmMomentsAlloc set->nPixels correctly");
        psFree(tmpMoments);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
	All functions are tested.
*/

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(11);


    // Test pmFringeRegionsAlloc()
    // pmResiduals *pmResidualsAlloc (int xSize, int ySize, int xBin, int yBin);
    {
        psMemId id = psMemGetId();
        int xSize = 1;
        int ySize = 2;
        int xBin = 3;
        int yBin = 4;
        pmResiduals *resid = pmResidualsAlloc(xSize, ySize, xBin, yBin);
        ok(resid != NULL && psMemCheckResiduals(resid), "pmResidualsAlloc() allocated a pmResiduals struct correctly");
        ok(resid->Ro && psMemCheckImage(resid->Ro), "pmResidualsAlloc() allocated the resid->Ro image");
        ok(resid->Rx && psMemCheckImage(resid->Rx), "pmResidualsAlloc() allocated the resid->Rx image");
        ok(resid->Ry && psMemCheckImage(resid->Ry), "pmResidualsAlloc() allocated the resid->Ry image");
        ok(resid->variance && psMemCheckImage(resid->variance ), "pmResidualsAlloc() allocated the resid->variance image");
        ok(resid->mask && psMemCheckImage(resid->mask), "pmResidualsAlloc() allocated the resid->mask image");

        int nX = xSize * xBin;
        int nY = ySize * yBin;
        nX = (nX % 2) ? nX : nX + 1;
        nY = (nY % 2) ? nY : nY + 1;
        ok(resid->xBin == xBin, "pmResidualsAlloc() set resid->xBin correctly");
        ok(resid->yBin == yBin, "pmResidualsAlloc() set resid->yBin correctly");
        ok(resid->xCenter == 0.5*(nX - 1), "pmResidualsAlloc() set resid->xCenter correctly");
        ok(resid->yCenter == 0.5*(nY - 1), "pmResidualsAlloc() set resid->xCenter correctly");

        psFree(resid);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

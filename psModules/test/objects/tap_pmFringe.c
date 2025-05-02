#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define NUM_ROWS 8
#define NUM_COLS 16

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(35);


    // Test pmFringeRegionsAlloc()
    {
        psMemId id = psMemGetId();
        pmFringeRegions *fringe = pmFringeRegionsAlloc(1, 2, 3, 4, 5);
        ok(fringe != NULL, "pmFringeRegionsAlloc() returned non-NULL");
        ok(fringe->x == NULL, "pmFringeRegionsAlloc() set fringe->x correctly");
        ok(fringe->y == NULL, "pmFringeRegionsAlloc() set fringe->y correctly");
        ok(fringe->mask == NULL, "pmFringeRegionsAlloc() set fringe->mask correctly");
        ok(fringe->nRequested == 1, "pmFringeRegionsAlloc() set fringe->nRequested correctly");
        ok(fringe->nAccepted == 0, "pmFringeRegionsAlloc() set fringe->nAccepted correctly");
        ok(fringe->dX == 2, "pmFringeRegionsAlloc() set fringe->dX correctly");
        ok(fringe->dY == 3, "pmFringeRegionsAlloc() set fringe->dY correctly");
        ok(fringe->nX == 4, "pmFringeRegionsAlloc() set fringe->nX correctly");
        ok(fringe->nY == 5, "pmFringeRegionsAlloc() set fringe->nY correctly");
        psFree(fringe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    #define NUM_FRINGE_PNTS 10
    #define DX 2
    #define DY 2
    #define NX 1
    #define NY 1
    // test pmFringeRegionsCreatePoints(): NULL random number generator,
    // NULL fringe X, y, and mask vectors.
    {
        psMemId id = psMemGetId();
        pmFringeRegions *fringe = pmFringeRegionsAlloc(NUM_FRINGE_PNTS, DX, DY, NX, NY);
        ok(fringe != NULL, "pmFringeRegionsAlloc() returned non-NULL");
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        bool rc = pmFringeRegionsCreatePoints(fringe, img, NULL);
        ok(rc, "pmFringeRegionsCreatePoints() returned TRUE");
        ok(fringe->x != NULL &&
           fringe->x->type.type == PS_TYPE_F32 &&
           fringe->x->n ==NUM_FRINGE_PNTS, "pmFringeRegionsCreatePoints() returned correct fringe->x psVector");
        ok(fringe->y != NULL &&
           fringe->y->type.type == PS_TYPE_F32 &&
           fringe->y->n ==NUM_FRINGE_PNTS, "pmFringeRegionsCreatePoints() returned correct fringe->y psVector");
        ok(fringe->mask != NULL &&
           fringe->mask->type.type == PS_TYPE_MASK &&
           fringe->mask->n ==NUM_FRINGE_PNTS, "pmFringeRegionsCreatePoints() returned correct fringe->mask psVector");
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_FRINGE_PNTS ; i++) {
            if (fringe->mask->data.U8[i] != 0) {
                diag("ERROR: pmFringeRegionsCreatePoints() did not set mask[%d] to 0", i);
                errorFlag = true;
            }
            if (!((fringe->x->data.F32[i] >= DX) &&
                 (fringe->x->data.F32[i] <= NUM_COLS-DX))) {
                diag("ERROR: pmFringeRegionsCreatePoints() did not set x[%d] correctly.  It was %.2f, should be within (%d %d)", i,
                      fringe->x->data.F32[i], DX, NUM_COLS-DX);
                errorFlag = true;
            }
            if (!((fringe->y->data.F32[i] >= DY) &&
                 (fringe->y->data.F32[i] <= NUM_ROWS-DY))) {
                diag("ERROR: pmFringeRegionsCreatePoints() did not set x[%d] correctly.  It was %.2f, should be within (%d %d)", i,
                      fringe->y->data.F32[i], DY, NUM_ROWS-DY);
                errorFlag = true;
            }
        }

        psFree(img);
        psFree(fringe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test pmFringeRegionsCreatePoints(): non-NULL random number generator,
    // non-NULL fringe X, y, and mask vectors.
    {
        psMemId id = psMemGetId();
        pmFringeRegions *fringe = pmFringeRegionsAlloc(NUM_FRINGE_PNTS, DX, DY, NX, NY);
        ok(fringe != NULL, "pmFringeRegionsAlloc() returned non-NULL");
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 10);
        fringe->x = psVectorAlloc(NUM_FRINGE_PNTS/2, PS_TYPE_F32);
        fringe->y = psVectorAlloc(NUM_FRINGE_PNTS/2, PS_TYPE_F32);
        fringe->mask = psVectorAlloc(NUM_FRINGE_PNTS/2, PS_TYPE_MASK);
        bool rc = pmFringeRegionsCreatePoints(fringe, img, NULL);
        ok(rc, "pmFringeRegionsCreatePoints() returned TRUE");
        ok(fringe->x != NULL &&
           fringe->x->type.type == PS_TYPE_F32 &&
           fringe->x->n ==NUM_FRINGE_PNTS, "pmFringeRegionsCreatePoints() returned correct fringe->x psVector");
        ok(fringe->y != NULL &&
           fringe->y->type.type == PS_TYPE_F32 &&
           fringe->y->n ==NUM_FRINGE_PNTS, "pmFringeRegionsCreatePoints() returned correct fringe->y psVector");
        ok(fringe->mask != NULL &&
           fringe->mask->type.type == PS_TYPE_MASK &&
           fringe->mask->n ==NUM_FRINGE_PNTS, "pmFringeRegionsCreatePoints() returned correct fringe->mask psVector");
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_FRINGE_PNTS ; i++) {
            if (fringe->mask->data.U8[i] != 0) {
                diag("ERROR: pmFringeRegionsCreatePoints() did not set mask[%d] to 0", i);
                errorFlag = true;
            }
            if (!((fringe->x->data.F32[i] >= DX) &&
                 (fringe->x->data.F32[i] <= NUM_COLS-DX))) {
                diag("ERROR: pmFringeRegionsCreatePoints() did not set x[%d] correctly.  It was %.2f, should be within (%d %d)", i,
                      fringe->x->data.F32[i], DX, NUM_COLS-DX);
                errorFlag = true;
            }
            if (!((fringe->y->data.F32[i] >= DY) &&
                 (fringe->y->data.F32[i] <= NUM_ROWS-DY))) {
                diag("ERROR: pmFringeRegionsCreatePoints() did not set x[%d] correctly.  It was %.2f, should be within (%d %d)", i,
                      fringe->y->data.F32[i], DY, NUM_ROWS-DY);
                errorFlag = true;
            }
        }
        psFree(rng);
        psFree(img);
        psFree(fringe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

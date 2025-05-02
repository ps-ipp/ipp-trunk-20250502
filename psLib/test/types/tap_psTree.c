#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM 1000000                     // Number of points
#define SPHERICAL_DISTANCE 3.0          // Distance of interest for spherical test
#define SEED 0                          // Random seed

static double distance(double ra1, double dec1, double ra2, double dec2)
{
#if 0
    // Traditional formula
    return acos(sin(dec1) * sin(dec2) + cos(dec1) * cos(dec2) * cos(ra1 - ra2));
#else
    // Haversine formula: used in psTree
    double dphi = dec1 - dec2;
    double sindphi = sin(dphi/2.0);
    double dlambda = ra1 - ra2;
    double sindlambda = sin(dlambda/2.0);
    return 2.0 * asin(sqrt(PS_SQR(sindphi) + cos(dec1) * cos(dec2) * PS_SQR(sindlambda)));
#endif
}

int main(int argc, char *argv[])
{
    psLibInit(NULL);
    plan_tests(13);

    // Euclidean geometry: 6 tests
    {
        psMemId id = psMemGetId();

        psVector *x = psVectorAlloc(NUM, PS_TYPE_F64);
        psVector *y = psVectorAlloc(NUM, PS_TYPE_F64);

        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        for (int i = 0; i < NUM; i++) {
            x->data.F64[i] = 2.0 * psRandomUniform(rng) - 1.0;
            y->data.F64[i] = 2.0 * psRandomUniform(rng) - 1.0;
        }
        psFree(rng);

        psTree *tree = psTreePlant(2, 2, PS_TREE_EUCLIDEAN, x, y);

        ok(tree, "Tree planted");
        skip_start(!tree, 4, "tree died");
        {
            //            psTreePrint(stderr, tree);

            psVector *coords = psVectorAlloc(2, PS_TYPE_F64);
            psVectorInit(coords, 0);

            long closeIndex = psTreeNearest(tree, coords);
            psFree(coords);
            ok(closeIndex >= 0 && closeIndex < tree->numNodes, "found closest point: %ld", closeIndex);

            long bestIndex = -1;
            double bestDist = INFINITY;
            for (int i = 0; i < NUM; i++) {
                double dist = PS_SQR(x->data.F64[i]) + PS_SQR(y->data.F64[i]);
                if (dist < bestDist) {
                    bestIndex = i;
                    bestDist = dist;
                }
            }
            ok(bestIndex == closeIndex, "correct point: %ld vs %ld", closeIndex, bestIndex);

            psVector *closest = psTreeCoords(NULL, tree, closeIndex);
            ok(closest, "got coords: %lf,%lf", closest->data.F64[0], closest->data.F64[1]);
            ok(closest->data.F64[0] == x->data.F64[bestIndex] &&
               closest->data.F64[1] == y->data.F64[bestIndex],
               "correct coords: %lf,%lf(%lf) vs %lf,%lf(%lf)",
               closest->data.F64[0], closest->data.F64[1],
               sqrt(PS_SQR(closest->data.F64[0]) + PS_SQR(closest->data.F64[1])),
               x->data.F64[bestIndex], y->data.F64[bestIndex],
               sqrt(PS_SQR(x->data.F64[bestIndex]) + PS_SQR(y->data.F64[bestIndex])));
            psFree(closest);
        }
        skip_end();

        psFree(tree);
        psFree(x);
        psFree(y);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Spherical geometry: 7 tests
    {
        psMemId id = psMemGetId();

        psVector *ra = psVectorAlloc(NUM, PS_TYPE_F64);
        psVector *dec = psVectorAlloc(NUM, PS_TYPE_F64);

        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, SEED);
        for (int i = 0; i < NUM; i++) {
            // Using http://mathworld.wolfram.com/SpherePointPicking.html
            ra->data.F64[i] = psRandomUniform(rng) * 2.0 * M_PI;
            dec->data.F64[i] = acos(2.0 * psRandomUniform(rng) - 1.0) - M_PI_2;
        }

        psTree *tree = psTreePlant(2, 2, PS_TREE_SPHERICAL, ra, dec);

        ok(tree, "Tree planted");
        skip_start(!tree, 4, "tree died");
        {
            //            psTreePrint(stderr, tree);

            psVector *coords = psVectorAlloc(2, PS_TYPE_F64);
#if 0
            coords->data.F64[0] = psRandomUniform(rng) * 2.0 * M_PI;
            coords->data.F64[1] = acos(2.0 * psRandomUniform(rng) - 1.0) - M_PI_2;
#else
            psVectorInit(coords, 0);
#endif

            psVector *indices = psTreeAllWithin(tree, coords, DEG_TO_RAD(SPHERICAL_DISTANCE));
            ok(indices && indices->type.type == PS_TYPE_S64, "got list of indices (%ld points)", indices->n);
            long closeIndex = psTreeNearest(tree, coords);
            ok(closeIndex >= 0 && closeIndex < tree->numNodes, "found closest point: %ld", closeIndex);

            ok(psVectorSortInPlace(indices), "sorted indices");

#if 0
            for (long i = 0; i < indices->n; i++) {
                long index = indices->data.S64[i];
                double dist = distance(coords->data.F64[0], coords->data.F64[1],
                                       ra->data.F64[index], dec->data.F64[index]);
                diag("%ld (%lf,%lf) is in the list (%lf vs %lf)",
                     index, ra->data.F64[index], dec->data.F64[index], RAD_TO_DEG(dist), SPHERICAL_DISTANCE);
            }
#endif

            bool allgood = true;        // All points in the appropriate place?
            double bestDistance = INFINITY; // Distance to best point
            long bestIndex = -1;        // Index of best point
            long bad = 0;               // Number bad
            for (long i = 0, j = 0; i < NUM; i++) {
                double dist = distance(coords->data.F64[0], coords->data.F64[1],
                                       ra->data.F64[i], dec->data.F64[i]);
                if (dist < bestDistance) {
                    bestDistance = dist;
                    bestIndex = i;
                }
                if (j < indices->n && i == indices->data.S64[j]) {
                    j++;
                    if (dist > DEG_TO_RAD(SPHERICAL_DISTANCE)) {
                        diag("%ld (%lf,%lf) is in the list, but shouldn't be (%lf vs %lf)",
                             i, ra->data.F64[i], dec->data.F64[i], RAD_TO_DEG(dist), SPHERICAL_DISTANCE);
                        allgood = false;
                        bad++;
                    } else {
#if 0
                        diag("%ld (%lf,%lf) correctly identified in the list (%lf vs %lf)",
                             i, ra->data.F64[i], dec->data.F64[i], RAD_TO_DEG(dist), SPHERICAL_DISTANCE);
#endif
                    }
                } else if (dist <= DEG_TO_RAD(SPHERICAL_DISTANCE)) {
                    diag("%ld (%lf,%lf) is not in the list, but should be (%lf vs %lf)",
                         i, ra->data.F64[i], dec->data.F64[i], RAD_TO_DEG(dist), SPHERICAL_DISTANCE);
                    allgood = false;
                    bad++;
                }
            }
            ok(allgood, "list is accurate: %ld bad", bad);
            ok(bestIndex == closeIndex, "correct point: %ld vs %ld", closeIndex, bestIndex);

            psFree(coords);
            psFree(indices);

        }
        skip_end();

        psFree(rng);
        psFree(tree);
        psFree(ra);
        psFree(dec);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    psLibFinalize();
}


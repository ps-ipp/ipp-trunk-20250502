#include <stdio.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define SIZE 1001                       // Size of vector
#define NUM_NONZERO 50                  // Number of non-zero values

// Test that results of psVectorSelect match what you'd get from psVectorSort
// 2 tests
static void testSelect(int size,        // Size of vector
                       int numNonzero   // Number of non-zero values
                       )
{
    psMemId id = psMemGetId();

    psVector *vector = psVectorAlloc(size, PS_TYPE_F32);
    psVectorInit(vector, 0.0);
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 12345);
    for (long i = 0; i < numNonzero; i++) {
        long index = psRandomUniform(rng) * size;
        vector->data.F32[index] = psRandomUniform(rng);
    }
    psFree(rng);

    psVector *sorted = psVectorSort(NULL, vector); // Sorted vector
    psVector *select = NULL;        // Vector for selecting
    bool match = true;              // Everything matches
    for (long i = 0; i < size && match; i++) {
        select = psVectorSelect(select, vector, i);
        if (select->data.F32[i] != sorted->data.F32[i]) {
            match = false;
            diag("Rank %ld doesn't match: %f vs %f", i, select->data.F32[i], sorted->data.F32[i]);
        }
    }
    psFree(sorted);
    psFree(select);
    psFree(vector);

    ok(match, "All selections match, size=%d, number non-zero=%d.", size, numNonzero);
    ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
}

// Test that results of psVectorSelectInPlace match what you'd get from psVectorSort
// 2 tests
static void testSelectInPlace(int size,        // Size of vector
                       int numNonzero   // Number of non-zero values
                       )
{
    psMemId id = psMemGetId();

    psVector *vector = psVectorAlloc(size, PS_TYPE_F32);
    psVectorInit(vector, 0.0);
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 12345);
    for (long i = 0; i < numNonzero; i++) {
        long index = psRandomUniform(rng) * size;
        vector->data.F32[index] = psRandomUniform(rng);
    }
    psFree(rng);

    psVector *sorted = psVectorSort(NULL, vector); // Sorted vector
    bool match = true;              // Everything matches
    for (long i = 0; i < size && match; i++) {
        psVectorSelectInPlace(vector, i);
        if (vector->data.F32[i] != sorted->data.F32[i]) {
            match = false;
            diag("Rank %ld doesn't match: %f vs %f", i, vector->data.F32[i], sorted->data.F32[i]);
        }
    }
    psFree(sorted);
    psFree(vector);

    ok(match, "All selections in-place match, size=%d, number non-zero=%d.", size, numNonzero);
    ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
}


int main(int argc, char *argv[])
{
    plan_tests(6 * 2 + 6 * 2);

    testSelect(100, 0);
    testSelect(100, 1);
    testSelect(100, 10);
    testSelect(100, 100);
    testSelect(137, 13);
    testSelect(137, 137);

    testSelectInPlace(100, 0);
    testSelectInPlace(100, 1);
    testSelectInPlace(100, 10);
    testSelectInPlace(100, 100);
    testSelectInPlace(137, 13);
    testSelectInPlace(137, 137);

    return PS_EXIT_SUCCESS;
}

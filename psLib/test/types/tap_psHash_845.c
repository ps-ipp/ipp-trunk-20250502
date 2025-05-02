#include <stdio.h>
#include "pslib.h"
#include "pstap.h"
#define SIZE 128

int main(int argc, char *argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(5);

    // Very basic test: create a psHash, then add data to it.
    // XXX: Remove this test, or mearge it with another file.
    {
        psMemId id = psMemGetId();
        char *stuff1 = psAlloc(SIZE);       // Stuff to put on hash
        char *stuff2 = psAlloc(SIZE);       // Stuff to put on hash

        psHash *hash = psHashAlloc(16);     // Hash to test
        ok(hash, "Hash allocated");
        bool status1 = psHashAdd(hash, "stuff", stuff1);
        ok(status1, "Added 1 to hash");
        psFree(stuff1);                     // Drop reference
        psMemId last = psMemGetId();        // Last memory I.D.
        bool status2 = psHashAdd(hash, "stuff", stuff2);
        ok(status2, "Added 2 to hash");
        psFree(stuff2);                     // Drop reference
        int numLeaks = psMemCheckLeaks(last, NULL, NULL, false); // Number of leaks
        ok(numLeaks == 0, "No leaks.");
        psFree(hash);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

}

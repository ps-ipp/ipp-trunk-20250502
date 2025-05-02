#include <stdio.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(39 + 1);


    {
        psMemId id = psMemGetId();
        char *testStr = "time    UTC     2005-03-18T16:05:00Z\n";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);
        psTime *time1 = psMetadataLookupTime(NULL, md, "time");
        psTime *time2 = psTimeFromISO("2005-03-18T16:05:00Z", PS_TIME_UTC);
        double delta = psTimeDelta(time1, time2);
        psFree(time2);

        ok(md, "md = %x", md);
        ok(nBad == 0, "number of bad lines = %d", nBad); // One bad line from boolean
        ok(md->list->n == 1, "number of items in metadata = %ld", md->list->n);
        ok1(time1->type == PS_TIME_UTC);
        ok(delta == 0.0, "reported time was off by %fs", delta);

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        char *testStr = "time    UT1     2005-03-18T16:05:00Z\n";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);
        psTime *time1 = psMetadataLookupTime(NULL, md, "time");
        psTime *time2 = psTimeFromISO("2005-03-18T16:05:00Z", PS_TIME_UT1);
        double delta = psTimeDelta(time1, time2);
        psFree(time2);

        ok(md, "md = %x", md);
        ok(nBad == 0, "number of bad lines = %d", nBad);
        ok(md->list->n == 1, "number of items in metadata = %ld", md->list->n);
        ok1(time1->type == PS_TIME_UT1);
        ok(delta == 0.0, "reported time was off by %fs", delta);

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        char *testStr = "time    TAI     2005-03-18T16:05:00Z\n";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);
        psTime *time1 = psMetadataLookupTime(NULL, md, "time");
        psTime *time2 = psTimeFromISO("2005-03-18T16:05:00Z", PS_TIME_TAI);
        double delta = psTimeDelta(time1, time2);
        psFree(time2);

        ok(md, "md = %x", md);
        ok(nBad == 0, "number of bad lines = %d", nBad);
        ok(md->list->n == 1, "number of items in metadata = %ld", md->list->n);
        ok1(time1->type == PS_TIME_TAI);
        ok(delta == 0.0, "reported time was off by %fs", delta);

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        char *testStr = "time    TT      2005-03-18T16:05:00Z";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);
        psTime *time1 = psMetadataLookupTime(NULL, md, "time");
        psTime *time2 = psTimeFromISO("2005-03-18T16:05:00Z", PS_TIME_TT);
        double delta = psTimeDelta(time1, time2);
        psFree(time2);

        ok(md, "md = %x", md);
        ok(nBad == 0, "number of bad lines = %d", nBad);
        ok(md->list->n == 1, "number of items in metadata = %ld", md->list->n);
        ok1(time1->type == PS_TIME_TT);
        ok(delta == 0.0, "reported time was off by %fs", delta);

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        // missing Z
        char *testStr = "broken      UTC     2005-03-18T16:05:00";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);

        ok(md, "returned metdata");
        is_int(nBad, 0, "number of bad lines");
        is_long(psListLength(md->list), 1, "number of items in metadata");

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    {
        psMemId id = psMemGetId();
        // missing Z
        char *testStr = "broken      UT1     2005-03-18T16:05:00";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);

        ok(md, "returned metdata");
        is_int(nBad, 0, "number of bad lines");
        is_long(psListLength(md->list), 1, "number of items in metadata");

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    {
        psMemId id = psMemGetId();
        // missing Z
        char *testStr = "broken      TAI     2005-03-18T16:05:00";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);

        ok(md, "returned metdata");
        is_int(nBad, 0, "number of bad lines");
        is_long(psListLength(md->list), 1, "number of items in metadata");

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        // missing Z
        char *testStr = "broken      TT      2005-03-18T16:05:00";
        psU32 nBad = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nBad, testStr, false);

        ok(md, "returned metdata");
        is_int(nBad, 0, "number of bad lines");
        is_long(psListLength(md->list), 1, "number of items in metadata");

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

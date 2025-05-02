#include <stdio.h>
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"

#define NAME1 "SIMPLE"
#define COMMENT1 "Basic FITS?"
#define FLAG1 0
#define VALUE1 "TRUE"

#define NAME2 "COMMENT"
#define COMMENT2 "This is a    "
#define FLAG2 PS_META_DUPLICATE_OK
#define VALUE2 "   comment."

#define NAME3 "FILENAME"
#define COMMENT3 "File name"
#define FLAG3 0
#define VALUE3 "abcdef123456.fits"

#define NAME4 "COMMENT"
#define COMMENT4 "   this IS a   "
#define FLAG4 PS_META_DUPLICATE_OK
#define VALUE4 "comment    too!"

#define NAME5 "DATE-OBS"
#define COMMENT5 "Observing date, UTC"
#define FLAG5 0
#define VALUE5 "1978-11-29"

#define NAME6 "COMMENT"
#define COMMENT6 "    another"
#define FLAG6 PS_META_DUPLICATE_OK
#define VALUE6 "comment     "

#define NAME7 "COMMENT"
#define COMMENT7 "This is another comment"
#define FLAG7 PS_META_DUPLICATE_OK
#define VALUE7 "Hello world"

#define MULTI_NAME "COMMENT"
#define MULTI_NAME1 NAME2
#define MULTI_NAME2 NAME4
#define MULTI_NAME3 NAME6
#define MULTI_NAME4 NAME7
#define MULTI_COMMENT1 COMMENT2
#define MULTI_COMMENT2 COMMENT4
#define MULTI_COMMENT3 COMMENT6
#define MULTI_COMMENT4 COMMENT7
#define MULTI_VALUE1 VALUE2
#define MULTI_VALUE2 VALUE4
#define MULTI_VALUE3 VALUE6
#define MULTI_VALUE4 VALUE7

// Generate a dummy metadata
static psMetadata *mdGenerate(void)
{
    psMetadata *md = psMetadataAlloc();

    // Something like a FITS header
    psMetadataAddStr(md, PS_LIST_TAIL, NAME1, FLAG1, COMMENT1, VALUE1);
    psMetadataAddStr(md, PS_LIST_TAIL, NAME2, FLAG2, COMMENT2, VALUE2);
    psMetadataAddStr(md, PS_LIST_TAIL, NAME3, FLAG3, COMMENT3, VALUE3);
    psMetadataAddStr(md, PS_LIST_TAIL, NAME4, FLAG4, COMMENT4, VALUE4);
    psMetadataAddStr(md, PS_LIST_TAIL, NAME5, FLAG5, COMMENT5, VALUE5);
    psMetadataAddStr(md, PS_LIST_TAIL, NAME6, FLAG6, COMMENT6, VALUE6);
    psMetadataAddStr(md, PS_LIST_TAIL, NAME7, FLAG7, COMMENT7, VALUE7);

    return md;
}

// Function to perform iteration
typedef psMetadataItem *(*iterFunc)(psMetadataIterator *iter);


// Test the iteration by iterating once and checking the name, comment, and data
static void testIteration(iterFunc function, // Function to use to iterate
                          psMetadataIterator *iterator, // The iterator
                          const char *name, // Name of the item
                          const char *comment, // Comment for the item
                          const char *value // Value for the item
                         )
{
    psMetadataItem *item = function(iterator);
    ok(item, "item = %x", item);
    skip_start(!item, 4, "Skipping 4 tests because iteration failed");
    ok(strcmp(item->name, name) == 0, "item->name = %s", item->name);
    ok(strcmp(item->comment, comment) == 0, "item->comment = %s", item->comment);
    ok(item->type == PS_DATA_STRING, "item->type = %x", item->type);
    skip_start(item->type != PS_DATA_STRING, 1, "Skipping 1 test because the item has incorrect type");
    ok(strcmp(item->data.V, value) == 0, "item->data.V = %s", item->data.V);
    skip_end();
    skip_end();

    return;
}


int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(254);

    // psMetadataIterator tests
    // No regular expressions, forwards
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 35, "Skipping 35 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME1, COMMENT1, VALUE1);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME2, COMMENT2, VALUE2);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME3, COMMENT3, VALUE3);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME4, COMMENT4, VALUE4);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME5, COMMENT5, VALUE5);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME6, COMMENT6, VALUE6);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME7, COMMENT7, VALUE7);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // No regular expressions, backwards
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_TAIL, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 35, "Skipping 35 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME7, COMMENT7, VALUE7);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME6, COMMENT6, VALUE6);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME5, COMMENT5, VALUE5);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME4, COMMENT4, VALUE4);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME3, COMMENT3, VALUE3);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME2, COMMENT2, VALUE2);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME1, COMMENT1, VALUE1);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // No regular expressions, jumping in halfway through (using allocator), forwards
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, 3, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME4, COMMENT4, VALUE4);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME5, COMMENT5, VALUE5);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME6, COMMENT6, VALUE6);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME7, COMMENT7, VALUE7);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //  No regular expressions, jumping in halfway through (using allocator), backwards
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, -4, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 15, "Skipping 20 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME4, COMMENT4, VALUE4);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME3, COMMENT3, VALUE3);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME2, COMMENT2, VALUE2);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME1, COMMENT1, VALUE1);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // No regular expressions, jumping in halfway through (using set), forwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");
        psMetadataIteratorSet(iter, 3);

        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME4, COMMENT4, VALUE4);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME5, COMMENT5, VALUE5);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME6, COMMENT6, VALUE6);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, NAME7, COMMENT7, VALUE7);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // No regular expressions, jumping in halfway through (using set), backwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 15, "Skipping 20 tests because psMetadataIteratorAlloc failed");
        psMetadataIteratorSet(iter, -4);

        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME4, COMMENT4, VALUE4);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME3, COMMENT3, VALUE3);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME2, COMMENT2, VALUE2);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, NAME1, COMMENT1, VALUE1);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // With regular expressions, forwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, "^" MULTI_NAME "$");
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME1, MULTI_COMMENT1, MULTI_VALUE1);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME2, MULTI_COMMENT2, MULTI_VALUE2);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME3, MULTI_COMMENT3, MULTI_VALUE3);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME4, MULTI_COMMENT4, MULTI_VALUE4);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // With regular expressions, backwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_TAIL, "^" MULTI_NAME "$");
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME4, MULTI_COMMENT4, MULTI_VALUE4);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME3, MULTI_COMMENT3, MULTI_VALUE3);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME2, MULTI_COMMENT2, MULTI_VALUE2);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME1, MULTI_COMMENT1, MULTI_VALUE1);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // With regular expressions, jumping in halfway through (using allocator), forwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, 2, "^" MULTI_NAME "$");
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME3, MULTI_COMMENT3, MULTI_VALUE3);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME4, MULTI_COMMENT4, MULTI_VALUE4);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // With regular expressions, jumping in halfway through (using allocator), backwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, -3, "^" MULTI_NAME "$");
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME2, MULTI_COMMENT2, MULTI_VALUE2);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME1, MULTI_COMMENT1, MULTI_VALUE1);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // With regular expressions, jumping in halfway through (using set), forwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, "^" MULTI_NAME "$");
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");
        psMetadataIteratorSet(iter, 2);

        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME3, MULTI_COMMENT3, MULTI_VALUE3);
        testIteration((iterFunc)psMetadataGetAndIncrement, iter, MULTI_NAME4, MULTI_COMMENT4, MULTI_VALUE4);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // With regular expressions, jumping in halfway through (using set), backwards");
    {
        psMemId id = psMemGetId();
        psMetadata *md = mdGenerate();
        psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, "^" MULTI_NAME "$");
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 20, "Skipping 20 tests because psMetadataIteratorAlloc failed");
        psMetadataIteratorSet(iter, -3);

        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME2, MULTI_COMMENT2, MULTI_VALUE2);
        testIteration((iterFunc)psMetadataGetAndDecrement, iter, MULTI_NAME1, MULTI_COMMENT1, MULTI_VALUE1);

        skip_end();
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

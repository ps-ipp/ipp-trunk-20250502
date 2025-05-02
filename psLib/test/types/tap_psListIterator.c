#include <stdio.h>
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"

#define STRING1 "This is string #1"
#define STRING2 "String number 2"
#define STRING3 "3 number string"
#define STRING4 "four number string"
#define STRING5 "number five string"
#define STRING6 "#6 string"
#define STRING7 "#7 $7r!Ng"


// Generate a dummy list
static psList *listGenerate(void)
{
    psList *list = psListAlloc(NULL);

    // Generate the strings
    psString string1 = psStringCopy(STRING1);
    psString string2 = psStringCopy(STRING2);
    psString string3 = psStringCopy(STRING3);
    psString string4 = psStringCopy(STRING4);
    psString string5 = psStringCopy(STRING5);
    psString string6 = psStringCopy(STRING6);
    psString string7 = psStringCopy(STRING7);

    // Add the strings to the list
    psListAdd(list, PS_LIST_TAIL, string1);
    psListAdd(list, PS_LIST_TAIL, string2);
    psListAdd(list, PS_LIST_TAIL, string3);
    psListAdd(list, PS_LIST_TAIL, string4);
    psListAdd(list, PS_LIST_TAIL, string5);
    psListAdd(list, PS_LIST_TAIL, string6);
    psListAdd(list, PS_LIST_TAIL, string7);

    // Drop references
    psFree(string1);
    psFree(string2);
    psFree(string3);
    psFree(string4);
    psFree(string5);
    psFree(string6);
    psFree(string7);

    return list;
}

// Function to perform iteration
typedef psString (*iterFunc)(psListIterator *iter);


// Test the iteration by iterating once and checking the name, comment, and data
static void testIteration(iterFunc function, // Function to use to iterate
                          psListIterator *iterator, // The iterator
                          const char *string // Expected string
                         )
{
    psString output = function(iterator);
    ok(output, "output = %x", output);
    skip_start(!output, 1, "Skipping test because iteration failed");
    ok(strcmp(output, string) == 0, "output = %s", output);
    skip_end();

    return;
}


int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(72);

    note("psListIterator tests");
    // Forwards
    {
        psMemId id = psMemGetId();
        psList *list = listGenerate();
        psListIterator *iter = psListIteratorAlloc(list, PS_LIST_HEAD, false);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 14, "Skipping 14 tests because psMetadataIteratorAlloc failed");

        testIteration((iterFunc)psListGetAndIncrement, iter, STRING1);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING2);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING3);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING4);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING5);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING6);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING7);

        skip_end();
        psFree(iter);
        psFree(list);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Backwards
    {
        psMemId id = psMemGetId();
        psList *list = listGenerate();
        psListIterator *iter = psListIteratorAlloc(list, PS_LIST_TAIL, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 14, "Skipping 14 tests because psListIteratorAlloc failed");

        testIteration((iterFunc)psListGetAndDecrement, iter, STRING7);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING6);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING5);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING4);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING3);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING2);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING1);

        skip_end();
        psFree(iter);
        psFree(list);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Jumping in halfway through (using allocator), forwards
    {
        psMemId id = psMemGetId();
        psList *list = listGenerate();
        psListIterator *iter = psListIteratorAlloc(list, 3, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 8, "Skipping 8 tests because psListIteratorAlloc failed");

        testIteration((iterFunc)psListGetAndIncrement, iter, STRING4);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING5);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING6);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING7);

        skip_end();
        psFree(iter);
        psFree(list);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Jumping in halfway through (using allocator), backwards
    {
        psMemId id = psMemGetId();
        psList *list = listGenerate();
        psListIterator *iter = psListIteratorAlloc(list, -4, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 8, "Skipping 8 tests because psListIteratorAlloc failed");

        testIteration((iterFunc)psListGetAndDecrement, iter, STRING4);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING3);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING2);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING1);

        skip_end();
        psFree(iter);
        psFree(list);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Jumping in halfway through (using set), forwards
    {
        psMemId id = psMemGetId();
        psList *list = listGenerate();
        psListIterator *iter = psListIteratorAlloc(list, PS_LIST_HEAD, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 8, "Skipping 8 tests because psListIteratorAlloc failed");
        psListIteratorSet(iter, 3);

        testIteration((iterFunc)psListGetAndIncrement, iter, STRING4);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING5);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING6);
        testIteration((iterFunc)psListGetAndIncrement, iter, STRING7);

        skip_end();
        psFree(iter);
        psFree(list);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Jumping in halfway through (using set), backwards
    {
        psMemId id = psMemGetId();
        psList *list = listGenerate();
        psListIterator *iter = psListIteratorAlloc(list, PS_LIST_HEAD, NULL);
        ok(iter, "iter = %x", iter);
        skip_start(!iter, 8, "Skipping 8 tests because psListIteratorAlloc failed");
        psListIteratorSet(iter, -4);

        testIteration((iterFunc)psListGetAndDecrement, iter, STRING4);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING3);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING2);
        testIteration((iterFunc)psListGetAndDecrement, iter, STRING1);

        skip_end();
        psFree(iter);
        psFree(list);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

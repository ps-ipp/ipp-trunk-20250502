#include <stdio.h>
#include <unistd.h>
#include <pslib.h>
#include <string.h>

#include "tap.h"
#include "pstap.h"

#define FILENAME "tap_psMetadataConfigRead.config"

#define STRING1 "# This is a comment that should be ignored\n"
#define STRING2 "\t\t\tSomeString   STR    This is a string     # This is the comment    \n"
#define STRING3 "      \t       vAlUes\t\t\t\tMETADATA\t\t\t\n"
#define STRING3a "aFloat\tF32\t1.2345\t#A number with a decimal point\n"
#define STRING3b "                                                anInt S32 98765\n"
#define STRING3c "\t\t\t\t\t\t\t\t\t\t\t\t\tEND\n"
#define STRING4 "Numbers\tMULTI\t\n"
#define STRING5 "Numbers F32 1.23#Number one\n"
#define STRING6 "        \t           \t                                \t              \n"
#define STRING7 "Numbers\t\t\tSTR One point two three   #Number two\n"
#define STRING8 "Numbers                                      BOOL 1#This is true\n"
#define STRING9 "   \tboolean BOOL TRUEsomeTimesButFALSEatOthers#This should fail\n"
#define STRING10 "SomeBoolean BOOL false\n"

#define NAME2 "SomeString"
#define TYPE2 PS_DATA_STRING
#define MIX2 string
#define VALUE2 "This is a string"
#define COMMENT2 "This is the comment"

#define NAME3 "vAlUes"
#define TYPE3 PS_DATA_METADATA
#define VALUE3 NULL                     // Not really, but serves as a placeholder
#define COMMENT3 ""

#define NAME3a "aFloat"
#define TYPE3a PS_TYPE_F32
#define MIX3a real
#define VALUE3a 1.2345
#define COMMENT3a "A number with a decimal point"

#define NAME3b "anInt"
#define TYPE3b PS_TYPE_S32
#define MIX3b integer
#define VALUE3b 98765
#define COMMENT3b ""

#define NAME5 "Numbers"
#define TYPE5 PS_TYPE_F32
#define MIX5 real
#define VALUE5 1.23
#define COMMENT5 "Number one"

#define NAME7 "Numbers"
#define TYPE7 PS_DATA_STRING
#define MIX7 string
#define VALUE7 "One point two three"
#define COMMENT7 "Number two"

#define NAME8 "Numbers"
#define TYPE8 PS_TYPE_BOOL
#define MIX8 boolean
#define VALUE8 1
#define COMMENT8 "This is true"

#define NAME10 "SomeBoolean"
#define TYPE10 PS_TYPE_BOOL
#define MIX10 boolean
#define VALUE10 0
#define COMMENT10 ""

#define MULTI_NAME "Numbers"

#define BAD_NAME "boolean"

typedef union {
    const char *string;
    float real;
    int integer;
    bool boolean;
} mixedType;


// Generate the metadata config file to be parsed
static void generateMDConfig(void)
{
    FILE *fp = fopen(FILENAME, "w");    // Configuration file
    fprintf(fp, "%s", STRING1);
    fprintf(fp, "%s", STRING2);
    fprintf(fp, "%s", STRING3);
    fprintf(fp, "%s", STRING3a);
    fprintf(fp, "%s", STRING3b);
    fprintf(fp, "%s", STRING3c);
    fprintf(fp, "%s", STRING4);
    fprintf(fp, "%s", STRING5);
    fprintf(fp, "%s", STRING6);
    fprintf(fp, "%s", STRING7);
    fprintf(fp, "%s", STRING8);
    fprintf(fp, "%s", STRING9);
    fprintf(fp, "%s", STRING10);
    fclose(fp);
}

static void *testItem(const psMetadataItem *item, // Item to check
                      const char *name, // Name of the item
                      int type, // Type for item
                      const char *comment, // Comment for the item
                      mixedType value // Value for the item
                     )
{
    psMemId id = psMemGetId();
    skip_start(!item, 3, "Skipping 4 tests because !item");
    ok(strcmp(item->name, name) == 0, "item->name = %s", item->name);
    ok(strcmp(item->comment, comment) == 0, "item->comment = %s", item->comment);
    ok(item->type == type, "item->type = %x", item->type);
    skip_end();

    if (item->type == PS_DATA_METADATA || item->type == PS_DATA_METADATA_MULTI) {
        return item->data.V;
    }

    skip_start(item->type != type, 1, "Skipping 1 test because the item has incorrect type");
    switch (item->type) {
    case PS_DATA_STRING: {
            ok(strcmp(item->data.V, value.string) == 0, "item->data.V = %s", item->data.V);
            break;
        }
    case PS_TYPE_F32: {
            ok(item->data.F32 == value.real, "item->data.F32 = %f", item->data.F32);
            break;
        }
    case PS_TYPE_S32: {
            ok(item->data.S32 == value.integer, "item->data.S32 = %d", item->data.S32);
            break;
        }
    case PS_TYPE_BOOL: {
            ok(item->data.B == value.boolean, "item->data.B = %d", item->data.B);
            break;
        }
    default: {
            ok(false, "Unknown item->type = %x", item->type);
        }
    }
    skip_end();
    ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    return NULL;
}


int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(62);

    generateMDConfig();

    unsigned int numBadLines = 0;            // Number of bad lines
    psMetadata *md = psMetadataConfigRead(NULL, &numBadLines, FILENAME, false);
    unlink (FILENAME);

    ok(md, "md = %x", md);
    is_int(numBadLines, 1, "number of bad lines"); // One bad line from boolean
    skip_start(!md || numBadLines != 1, 0,
               "Skipping 0 tests because psMetadataConfigRead failed.");
    ok(psListLength(md->list) == 6, "size = %d", psListLength(md->list));

    {
        // Go through and make sure everything's there
        psMetadataIterator *iterator = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL); // Iterator

        {
            psMetadataItem *item = psMetadataGetAndIncrement(iterator);
            mixedType value;
            value.MIX2 = VALUE2;
            testItem(item, NAME2, TYPE2, COMMENT2, value);
        }

        {
            psMetadataItem *item = psMetadataGetAndIncrement(iterator);
            mixedType value;            // Needed only to check the other stuff
            psMetadata *nested = testItem(item, NAME3, TYPE3, COMMENT3, value);
            ok(psListLength(nested->list) == 2, "size = %d", psListLength(nested->list));
            skip_start(psListLength(nested->list) != 2, 8, "Skipping 8 tests because wrong size");
            psMetadataIterator *nestedIter = psMetadataIteratorAlloc(nested, PS_LIST_HEAD, NULL);

            {
                psMetadataItem *item = psMetadataGetAndIncrement(nestedIter);
                mixedType value;
                value.MIX3a = VALUE3a;
                testItem(item, NAME3a, TYPE3a, COMMENT3a, value);
            }

            {
                psMetadataItem *item = psMetadataGetAndIncrement(nestedIter);
                mixedType value;
                value.MIX3b = VALUE3b;
                testItem(item, NAME3b, TYPE3b, COMMENT3b, value);
            }

            psFree(nestedIter);
            skip_end();
        }

        {
            psMetadataItem *item = psMetadataGetAndIncrement(iterator);
            mixedType value;
            value.MIX5 = VALUE5;
            testItem(item, NAME5, TYPE5, COMMENT5, value);
        }

        {
            psMetadataItem *item = psMetadataGetAndIncrement(iterator);
            mixedType value;
            value.MIX7 = VALUE7;
            testItem(item, NAME7, TYPE7, COMMENT7, value);
        }

        {
            psMetadataItem *item = psMetadataGetAndIncrement(iterator);
            mixedType value;
            value.MIX8 = VALUE8;
            testItem(item, NAME8, TYPE8, COMMENT8, value);
        }

        {
            psMetadataItem *item = psMetadataGetAndIncrement(iterator);
            mixedType value;
            value.MIX10 = VALUE10;
            testItem(item, NAME10, TYPE10, COMMENT10, value);
        }

        psFree(iterator);
    }

    {
        // Check that the correct item is MULTI
        psMetadataItem *item = psMetadataLookup(md, MULTI_NAME);
        mixedType value;                // Needed only so we can test other stuff
        psList *multi = testItem(item, MULTI_NAME, PS_DATA_METADATA_MULTI, "", value);
        ok(psListLength(multi) == 3, "length = %d", psListLength(multi));
        skip_start(psListLength(multi) != 3, 12, "Skipping 12 tests because MULTI failed.");
        psListIterator *multiIter = psListIteratorAlloc(multi, PS_LIST_HEAD, false); // Iterator

        {
            psMetadataItem *item = psListGetAndIncrement(multiIter);
            mixedType value;
            value.MIX5 = VALUE5;
            testItem(item, NAME5, TYPE5, COMMENT5, value);
        }

        {
            psMetadataItem *item = psListGetAndIncrement(multiIter);
            mixedType value;
            value.MIX7 = VALUE7;
            testItem(item, NAME7, TYPE7, COMMENT7, value);
        }

        {
            psMetadataItem *item = psListGetAndIncrement(multiIter);
            mixedType value;
            value.MIX8 = VALUE8;
            testItem(item, NAME8, TYPE8, COMMENT8, value);
        }
        psFree(multiIter);
        skip_end();
    }

    {
        // Check that the bad one isn't there
        psMetadataItem *item = psMetadataLookup(md, BAD_NAME);
        ok(!item, "item = %x", item);
    }

    skip_end();

    psFree(md);

    return exit_status();
}

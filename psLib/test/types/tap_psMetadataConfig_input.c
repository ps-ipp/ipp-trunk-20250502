/**
 * tap_psMetdataConfig_input.c
 *
 * Description:  Tests for psMetadataConfigRead
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 * Author: Joshua Hoblitt, University of Hawaii 2007
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "tap.h"
#include "pstap.h"

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(30);


    // Return NULL for NULL filename input
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigRead(NULL, &nfail, NULL, false);
        ok(md == NULL, "return NULL for NULL filename input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return NULL for invalid filename input
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigRead(NULL, &nfail, ".", false);
        ok(md == NULL, "return NULL for invalid filename input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return NULL for missing file input
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigRead(NULL, &nfail, "table4.dat", false);
        ok(md == NULL, "return NULL for missing file input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return NULL for read-only file with comments, bad syntax, and no good
    // lines
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigRead(NULL, &nfail, "table3.dat", false);
        ok(md == NULL, "return NULL for all bad syntax");
        is_int(nfail, 0, "correct nfail");
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return true for valid inputs - overwrite = false
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigRead(NULL, &nfail, "metaconf.in", false);

        ok(md != NULL, "return metadata for valid input");
        is_int(nfail, 27, "return correct nFail count");

        skip_start(md == NULL, 8,
                     "Skipping 1 tests because metadata container is empty!");

        // look at the first item
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);
        // XXX shouldn't assume item isn't NULL
        is_str(item->name, "item1", "metadataItem name");
        ok(item->type == PS_DATA_BOOL, "metadataItem type");
        is_bool(item->data.B, true, "metdataItem value");
        is_str(item->comment, "I am a boolean", "metadataItem comment.");

        // look at the last item
        item = psMetadataGet(md, PS_LIST_TAIL);
        // XXX shouldn't assume item isn't NULL
        is_str(item->name, "M2", "metadataItem name");
        ok(item->type == PS_DATA_S32, "metdataItem type");
        is_int(item->data.S32, 666, "metdataItem value");
        is_str(item->comment, "this line is \"GOOD\" but parsed in the top level", "metadataItem comment.");
        skip_end();
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return true for valid inputs - overwrite = true
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigRead(NULL, &nfail, "metaconf.in", true);

        ok(md != NULL, "return metadata for valid input");
        is_int(nfail, 23, "return correct nFail count");

        skip_start(md == NULL, 7,
                     "Skipping 1 tests because metadata container is empty!");

        // look at the first item
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);
        // XXX shouldn't assume item isn't NULL
        // note that item1 gets added to the "tail" of the metdata was it's
        // replaced
        is_str(item->name, "item2", "metadataItem name");
        ok(item->type == PS_DATA_S32, "metdataItem type");
        is_bool(item->data.S32, 55, "metadataItem value");
        is_str(item->comment, "I am int", "metadataItem value");

        // look at the last item
        item = psMetadataGet(md, PS_LIST_TAIL);
        // XXX shouldn't assume item isn't NULL
        // meta1 is from "the bad section"
        is_str(item->name, "meta1", "metadataItem name");
        ok(item->type == PS_DATA_METADATA, "metadataItem type");

        is_str(item->comment, "duplicate", "metadataItem comment");
        skip_end();
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

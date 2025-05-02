/**
 * tap_psMetdataConfigParse.c
 *
 * Description:  Tests for psMetadataConfigParse
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 * Author: Joshua Hoblitt, University of Hawaii, 2006
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
    plan_tests(35);


    // Return NULL for NULL string input
    {
        psMemId id = psMemGetId();
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nfail, NULL, false);

        ok(md == NULL, "return NULL for NULL string input.");

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return empty metadata for incorrect string input
    {
        psMemId id = psMemGetId();
        char *str = "-10.05    2        4";
        psMetadata *md = psMetadataConfigParse(NULL, NULL, str, false);

        ok(md == NULL, "return NULL for 1 invalid line");

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return correct metadata for correct string input - S32
    {
        psMemId id = psMemGetId();
        char *str = "new S32       666";
        psMetadata *md = psMetadataConfigParse(NULL, NULL, str, false);
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);

        ok(md != NULL, "return correct metadata for 1 valid line");
        skip_start(item == NULL, 2,
                     "Skipping 1 tests because metadata container is empty!");
        ok(item->type == PS_DATA_S32, "return correct metdataItem type");
        is_int(item->data.S32, 666, "return correct metadataItem data");
        is_str(item->name, "new", "return correct metadataItem name");
        is_str(item->comment, "", "return correct metadataItem comment");
        skip_end();

        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return correct metadata for correct string input - F64
    {
        psMemId id = psMemGetId();
        char *str = "item4            F64       6.28";
        psMetadata *md = psMetadataConfigParse(NULL, NULL, str, false);
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);

        ok(md != NULL, "return correct metadata for valid string input.");
        ok(item != NULL, "return correct metadata for valid string input.");
        skip_start(item == NULL, 2,
                "Skipping 1 tests because metadata container is empty!");
        ok(item->type == PS_DATA_F64, "return correct metadataItem type");
        is_float(6.28, item->data.F64, "return correct metadataItem data");
        is_str(item->name, "item4", "return correct metadataItem name");
        is_str(item->comment, "", "return correct metadataItem comment");
        skip_end();

        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return correct metadata for string with duplicate entry - S8
    {
        psMemId id = psMemGetId();
        char *str = "item8            S8        3\nitem8            S8        5";
        unsigned int nfail = 0;
        psMetadata *md = psMetadataConfigParse(NULL, &nfail, str, false);
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);

        ok(md != NULL, "return metadata for 1 good & 1 duplicate (bad) line");
        ok(item != NULL, "return non-empty metdata");
        is_long(md->list->n, 1, "# of metdataItems");
        is_int(nfail, 1, "number of parse failures");
        skip_start(item == NULL, 2,
                "Skipping 1 tests because metadata container is empty!");
        ok(item->type == PS_DATA_S8, "metdataItem type");
        is_int(item->data.S8, 3, "metdataItem value");
        is_str(item->name, "item8", "return correct metadataItem name");
        is_str(item->comment, "", "return correct metadataItem comment");
        skip_end();
        psFree(md);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return correct metadata for correct string input - Vector
    {
        psMemId id = psMemGetId();
        char *str = "@vector6 U8 1 2 3 4 5   # I am a vector";
        psMetadata *md = psMetadataConfigParse(NULL, NULL, str, true);
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);

        ok(md != NULL, "return metadata for valid string input. (Vector)");
        ok(item != NULL, "non-empty metadata");
        skip_start(item == NULL, 2,
                     "Skipping 1 tests because metadata container is empty!");
        ok(item->type == PS_DATA_VECTOR, "metadataItem type");
        is_int(((psVector*)(item->data.V))->data.U8[0], 1,
                "return correct metadataItem data (Vector)");
        is_str(item->name, "vector6",
                "return correct metadataItem name (Vector)");
        is_str(item->comment, "I am a vector", "return correct metadataItem comment");
        skip_end();

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Return empty metadata for incorrect string input
    {
        psMemId id = psMemGetId();
        char *str = "END\n";
        psMetadata *md = psMetadataConfigParse(NULL, NULL, str, false);

        ok(md == NULL, "return NULL on 1 invalid line");

        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}


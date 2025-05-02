/**
 *  C Implementation: tap_psMetadataConfigFormat.c
 *
 * Description:  Tests for psMetadataConfigFormat
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(18);

    psMetadata *md = NULL;
    psString out = NULL;
    //psString psMetadataConfigFormat(psMetadata *md);
    //Return NULL for NULL metadata input
    {
        psMemId id = psMemGetId();
        out = psMetadataConfigFormat(md);
        ok( !out, "psMetadataConfigFormat:         return NULL for NULL metadata input.");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for empty metadata input
    {
        md = psMetadataAlloc();
        out = psMetadataConfigFormat(md);
        ok( out, "psMetadataConfigFormat:         return valid string for empty metadata input.");
	skip_start (!out, 1, "failed to build valid string");
        ok( *out == 0, "psMetadataConfigFormat:         return empty string for empty metadata input.");
	psFree (out);
	skip_end();
    }


    //Return NULL for metadata with missing hash table
    {
        psMetadataAddS32(md, PS_LIST_HEAD, "new_S32", 0, "", 666);
        psMetadataAddS32(md, PS_LIST_HEAD, "new_S32", PS_META_DUPLICATE_OK, "", 665);
        psHash *temp = md->hash;
        md->hash = NULL;
        out = psMetadataConfigFormat(md);
        ok( out == NULL,
            "psMetadataConfigFormat:         return NULL for metadata with missing "
            "hash table.");
        md->hash = temp;
    }


    //Return NULL for metadata containing a psList
    {
        psList *list = psListAlloc(NULL);
        psMetadataAddList(md, PS_LIST_HEAD, "new_list", 0, "", list);
        out = psMetadataConfigFormat(md);
        ok( out == NULL,
            "psMetadataConfigFormat:         return NULL for metadata containing a psList.");
        psFree(list);
        psFree(md);
    }


    //Return NULL for attempting to format psTime with type = -1
    {
        psMemId id = psMemGetId();
        md = psMetadataAlloc();
        psTime *time = psTimeAlloc(PS_TIME_TT);
        time->type = -1;
        psMetadataAddTime(md, PS_LIST_TAIL, "time", 0, "", time);
        out = psMetadataConfigFormat(md);
        ok( out == NULL,
            "psMetadataConfigFormat:         return NULL for metadata containing a "
            "invalid time type.");
        psFree(time);
        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid metadata for attempting to format NULL psTime metadataItem
    {
        psMemId id = psMemGetId();
        md = psMetadataAlloc();
        psTime *time = psTimeAlloc(PS_TIME_TT);
        psMetadataAddTime(md, PS_LIST_HEAD, "time", 0, "", time);
        psMetadataItem *item = psMetadataGet(md, PS_LIST_HEAD);
        psFree(item->data.V);
        item->data.V = NULL;
        out = psMetadataConfigFormat(md);
        ok( out != NULL,
            "psMetadataConfigFormat:         return valid metadata for metadata "
            "containing a NULL time.");
        char configTest[32];
        strncpy(configTest, "time             TAI       NULL", 31);
        is_strn(configTest, out, 31,
                "psMetadataConfigFormat:         return correct output string.");
        psFree(time);
        psFree(out);
        out = NULL;
        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid metadata for attempting to format NULL psTime metadataItem
    {
        psMemId id = psMemGetId();
        md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "new_S32", 0, NULL, 666);
        psMetadataAddS32(md, PS_LIST_HEAD, "new_S32", PS_META_DUPLICATE_OK, NULL, 665);
        out = psMetadataConfigFormat(md);
        ok( out != NULL,
            "psMetadataConfigFormat:         return valid metadata for metadata "
            "containing a NULL time.");
        char configTest[62];
        strncpy(configTest, "new_S32 MULTI\nnew_S32          S32       666", 61);
        is_strn(configTest, out, strlen(configTest),
                "psMetadataConfigFormat:         return correct output string.");
        psFree(out);
        out = NULL;
        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid metadata for attempting to format NULL psTime metadataItem
    {
        psMemId id = psMemGetId();
        md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "f32", 0, "f32_1", 666);
        psMetadataAddS32(md, PS_LIST_TAIL, "f32", PS_META_DUPLICATE_OK, "f32_3", 666);
        psMetadataAddS32(md, PS_LIST_TAIL, "f32", PS_META_DUPLICATE_OK, "f32_2", 665);
        out = psMetadataConfigFormat(md);
        ok( out != NULL,
            "psMetadataConfigFormat:         return valid metadata for metadata "
            "containing a NULL time.");
        char configTest[201];
        strncpy(configTest,
                "f32 MULTI"
                "\nf32              S32       666              # f32_1"
                "\nf32              S32       666              # f32_3"
                "\nf32              S32       665              # f32_2", 200);
        is_strn(configTest, out, strlen(configTest),
                "psMetadataConfigFormat:         return correct output string.");
        psFree(out);
        out = NULL;
        psFree(md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    done();
}

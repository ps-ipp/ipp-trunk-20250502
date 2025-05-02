/**
 *  C Implementation: tap_psMetadataConfigWrite
 *
 * Description:  Tests for psMetadataConfigWrite
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
    plan_tests(11);


    //Return false for NULL metadata input
    {
        psMemId id = psMemGetId();
        ok( !psMetadataConfigWrite(NULL, "mdcfg.wrt", NULL),
            "return false for NULL metadata input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return false for NULL filename input
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        ok( !psMetadataConfigWrite(md, NULL, NULL),
            "return false for NULL filename input.");
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return false for invalid filename input
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        ok( !psMetadataConfigWrite(md, ".", NULL),
            "return false for invalid filename input.");
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return true for empty metadata input (creates an empty file)
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        ok(psMetadataConfigWrite(md, "mdcfg.wrt", NULL), "return false for empty metadata input.");
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return true for valid inputs
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psMetadataAddBool(md, PS_LIST_TAIL, "item1-1", 0, "I am a boolean", true);
        ok( psMetadataConfigWrite(md, "mdcfg.wrt", NULL),
            "return true for valid inputs.");
        char configTest[61];
        char fileStr[62];
        FILE *mdcfg = fopen("mdcfg.wrt", "r");
        fgets(fileStr, 61, mdcfg);
        strncpy(configTest,
                "item1-1          BOOL      T                # I am a boolean", 60);
        is_strn(configTest, fileStr, 60,
                "return correct output.");
        fclose(mdcfg);
        remove
            ("mdcfg.wrt");
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

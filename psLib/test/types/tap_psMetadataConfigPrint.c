/**
 *  C Implementation: tap_psMetadataConfig_output
 *
 * Description:  Tests for psMetadataConfigPrint 
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
        FILE *openfile = fopen("mdcfg.prt", "w+");

        ok(!psMetadataConfigPrint(openfile, NULL),
           "return false for NULL metadata input.");

        fclose(openfile);
        remove("mdcfg.prt");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return true for empty metadata input (creates an empty file; is not an error)
    {
        psMemId id = psMemGetId();
        FILE *openfile = fopen("mdcfg.prt", "w+");
        psMetadata *md = psMetadataAlloc();

        ok(psMetadataConfigPrint(openfile, md), "return true for empty metadata input.");

        psFree(md);
        fclose(openfile);
        remove("mdcfg.prt");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return false for NULL file input
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "new S32", 0, "", 666);

        ok(!psMetadataConfigPrint(NULL, md),
           "return false for NULL FILE* input.");

        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return false for read-only file input
    {
        psMemId id = psMemGetId();
        // create an empty file to open read only later
        {
            FILE *openfile = fopen("mdcfg.prt", "w");
            fclose(openfile);
        }

        FILE *openfile = fopen("mdcfg.prt", "r");
        psMetadata *md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "new S32", 0, "", 666);

        ok(!psMetadataConfigPrint(openfile, md),
           "return false for read-only FILE* input.");

        psFree(md);
        fclose(openfile);
        remove("mdcfg.prt");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return true for valid inputs
    {
        psMemId id = psMemGetId();
        FILE *openfile = fopen("mdcfg.prt", "w+");
        psMetadata *md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "new S32", 0, "", 666);

        ok(psMetadataConfigPrint(openfile, md),
           "return true for valid inputs.");

        psFree(md);
        fclose(openfile);

        char fileStr2[62];
        memset(fileStr2, '\0', 62);
        FILE *mdcfg = fopen("mdcfg.prt", "r");

        fread(fileStr2, 1, 31, mdcfg);

        is_strn(fileStr2, "\nnew S32          S32       666 ", 30,
                "return correct output.");

        fclose(mdcfg);
        remove("mdcfg.prt");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

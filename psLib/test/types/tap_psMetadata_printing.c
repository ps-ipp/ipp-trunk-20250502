/**
 *  C Implementation: tap_psMetadata_printing
 *
 * Description:  Tests for psMetadataPrint and psMetadataItemPrint.
 *
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <fcntl.h>
#include <unistd.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

static psMetadata *setupMeta(void)
{
    psMetadata *md = NULL;
    psMetadata *newMD = NULL;
    int i = 0;
    md = psMetadataAlloc();
    newMD = psMetadataAlloc();

    psMetadataAddStr(md, PS_LIST_TAIL, "item5", 0, "I am a string", "GNIRTS");
    psMetadataAddBool(md, PS_LIST_TAIL, "item1", 0, "I am a boolean", true);
    psMetadataAddBool(md, PS_LIST_TAIL, "item11", 0, "I am a boolean", false);
    psMetadataAddS8(md, PS_LIST_TAIL, "item6", 0, "I am S8", 6);
    psMetadataAddS16(md, PS_LIST_TAIL, "item7", 0, "I am S16", -666);
    psMetadataAddS32(md, PS_LIST_TAIL, "item2", 0, "I am a integer", 55);
    psMetadataAddS64(md, PS_LIST_TAIL, "helloS64", 0, "I am S64", 666);
    psMetadataAddU8(md, PS_LIST_TAIL, "item8", 0, "I am U8", 6);
    psMetadataAddU16(md, PS_LIST_TAIL, "item9", 0, "I am U16", 666);
    psMetadataAddU32(md, PS_LIST_TAIL, "item10", 0, "I am U32", 666);
    psMetadataAddU64(md, PS_LIST_TAIL, "item12", 0, "I am U64", 666);
    psMetadataAddF32(md, PS_LIST_TAIL, "item3", 0, NULL, 3.14);
    psMetadataAddF64(md, PS_LIST_TAIL, "item4", 0, "", 6.28);

    psMetadataAddS32(newMD, PS_LIST_TAIL, "ITEM01", 0, NULL, 666);
    psMetadata *newestMD = NULL;
    newestMD = psMetadataAlloc();
    psVector *vec = NULL;
    vec = psVectorAlloc(60, PS_DATA_S32);
    for (i = 0; i < 5; i++) {
        vec->data.S32[i] = i+1;
    }
    vec->n = 5;
    psMetadataAddVector(newestMD, PS_LIST_TAIL, "VECTORNEW", 0, "Newest VECTOR", vec);
    psMetadataAddStr(newestMD, PS_LIST_TAIL, "cell", 0, "I am a p-Star string", "pStArRs");
    psMetadataAddMetadata(newMD, PS_LIST_TAIL, "META NEW", 0, "I AM Newest METADATA", newestMD);
    psMetadataAddF32(newMD, PS_LIST_TAIL, "ITEM02", 0, "I AM FLOAT", 666.6);
    psMetadataAddF64(newMD, PS_LIST_TAIL, "ITEM03", 0, "I AM DOUBLE", 666.666);
    psMetadataAddS64(newMD, PS_LIST_TAIL, "item666", 0, "I am S64", 666);

    psMetadataAddMetadata(md, PS_LIST_TAIL, "metadata7", 0, "I am a metadata", newMD);
    psRegion *region = psRegionAlloc(0.0, 2.0, 0.0, 2.0);
    psMetadataAddPtr(md, PS_LIST_TAIL, "region", PS_DATA_REGION, "I am a region", region);
    psList *list = psListAlloc(NULL);
    psMetadataAddList(md, PS_LIST_TAIL, "list01", 0, "I am a list", list);

    psTime *time;
    time = psTimeAlloc(PS_TIME_TAI);
    time->sec = 1000;
    time->nsec = 25;
    time->leapsecond = true;
    psMetadataAddTime(md, PS_LIST_TAIL, "time01", 0, "I am time", time);

    psMetadataAddVector(md, PS_LIST_TAIL, "vector6", 0, "I am a vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_U8);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.U8[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector7", 0, "I am a U8-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_U16);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.U16[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector8", 0, "I am a U16-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_U32);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.U32[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector9", 0, "I am a U32-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_U64);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.U64[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector10", 0, "I am a U64-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_S8);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.S8[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector11", 0, "I am a S8-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_S16);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.S16[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector12", 0, "I am a S16-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_S64);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.S64[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector13", 0, "I am a S64-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_F32);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.F32[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector14", 0, "I am a F32-vector", vec);
    psFree(vec);
    vec = psVectorAlloc(2, PS_DATA_F64);
    vec->n = 2;
    for (i = 0; i < 2; i++) {
        vec->data.F64[i] = i+1;
    }
    psMetadataAddVector(md, PS_LIST_TAIL, "vector15", 0, "I am a F64-vector", vec);

    /*    psImage *image = psImageAlloc(1, 1, PS_TYPE_S32);
        psImageSet(image, 0, 0, 1);
        psMetadataAddImage(md, PS_LIST_TAIL, "image1", 0, "I am an Image", image);

        psHash *hash = psHashAlloc(1);
        psHashAdd(hash, "hash", image);
        psMetadataAddHash(md, PS_LIST_TAIL, "hash1", 0, "I am a Hash", hash);

        psLookupTable *lookup = psLookupTableAlloc("table2.dat", "%s", 0);
        psMetadataAddLookupTable(md, PS_LIST_TAIL, "lookup", 0, "I am a LookupTable", lookup);

        psArray *array = psArrayAlloc(1);
        psArraySet(array, 0, image);
        psMetadataAddArray(md, PS_LIST_TAIL, "array", 0, "I am an Array", array);

        psFree(array);
        psFree(lookup);
        psFree(image);
        psFree(hash);
    */
    psFree(region);
    psFree(list);
    psFree(time);
    psFree(newestMD);
    psFree(vec);
    psFree(newMD);
    return md;
}


int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(30);


    // psMetadataPrint
    {
        psMemId id = psMemGetId();
        FILE *fd = fopen("psMetadataPrint.out", "w+");
        FILE *fd_read = fopen("Makefile", "r");
        psMetadata *md = NULL;
        psMetadata *md2 = NULL;
        md = setupMeta();
        md2 = psMetadataAlloc();
        psSphere *sphere = psSphereAlloc();
        psMetadataAddPtr(md2, PS_LIST_HEAD, "ptr", PS_DATA_SPHERE, "", sphere);
        psMetadataAddUnknown(md2, PS_LIST_TAIL, "unknown", 0, "", sphere);
    
        psImage *image = psImageAlloc(1, 1, PS_TYPE_S32);
        psImageSet(image, 0, 0, 1);
        psMetadataAddImage(md2, PS_LIST_TAIL, "image1", 0, "I am an Image", image);
    
        psHash *hash = psHashAlloc(1);
        psHashAdd(hash, "hash", image);
        psMetadataAddHash(md2, PS_LIST_TAIL, "hash1", 0, "I am a Hash", hash);
    
        psLookupTable *lookup = psLookupTableAlloc("table2.dat", "%s", 0);
        psMetadataAddLookupTable(md2, PS_LIST_TAIL, "lookup", 0, "I am a LookupTable", lookup);
    
        psArray *array = psArrayAlloc(1);
        psArraySet(array, 0, image);
        psMetadataAddArray(md2, PS_LIST_TAIL, "array", 0, "I am an Array", array);
    
        psFree(array);
        psFree(lookup);
        psFree(image);
        psFree(hash);
    
        //Return false for NULL psMetadata input
        {
            ok( !psMetadataPrint(NULL, NULL, 0),
                "psMetadataPrint:            return false for NULL psMetadata input.");
        }
        //Return false for read-only file descriptor
        {
            ok ( !psMetadataPrint(fd_read, md, 0),
                 "psMetadataPrint:            return false for read-only file descriptor.");
        }
        fclose(fd_read);
        //Error and then return true for unprintable datatype (sphere) and DATA_UNKNOWN
        {
            ok ( !psMetadataPrint(NULL, md2, 0),
                 "psMetadataPrint:            return false for metadata with unknown datatype.");
        }
        psFree(md2);
    
        //Valid case where fd is NULL -> stdout
        md2 = psMetadataAlloc();
        psMetadataAddStr(md2, PS_LIST_HEAD,
                         "  >>Hello Wally", 0, "My name is Bubbles", "Pet Me");
        {
            ok ( psMetadataPrint(NULL, md2, 0),
                 "psMetadataPrint:            return true for NULL file pointer.");
        }
        psFree(md2);
        //Valid case where fd is "psMetadataPrint.out"
        {
            ok ( psMetadataPrint(fd, md, 0),
                 "psMetadataPrint:            return true for file='psMetadataPrint.out'.");
        }
    
        //Check for Memory Leaks before exit
        fclose(fd);
        remove("psMetadataPrint.out");
        psFree(sphere);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psMetadataItemPrint
    {
        psMemId id = psMemGetId();
        FILE *fd;
        fd = fopen("psMetadataItemPrint.out", "w");
        psMetadataItem *item1 = psMetadataItemAlloc("bool", PS_DATA_BOOL, "No Comment", true);
        psMetadataItem *item2 = psMetadataItemAlloc("f32", PS_DATA_F32, "No Comment", 0.66601);
        psMetadataItem *item3 = psMetadataItemAlloc("string", PS_DATA_STRING, "", "Can't touch this");
        psMetadataItem *item4 = psMetadataItemAlloc("U16", PS_DATA_U16, "", 1);
        psMetadataItem *item5 = psMetadataItemAlloc("string", PS_DATA_U8, "", "a");
    
        //Return false for NULL file-pointer
        {
            ok( !psMetadataItemPrint(NULL, "%s", item1),
                "psMetadataItemPrint:        return false for NULL FILE*.");
        }
        //Return false for NULL format paramter
        {
            ok( !psMetadataItemPrint(fd, NULL, item1),
                "psMetadataItemPrint:        return false for NULL format.");
        }
        //Return false for NULL metadataItem
        {
            ok( !psMetadataItemPrint(fd, "%s", NULL),
                "psMetadataItemPrint:        return false for NULL psMetadataItem.");
        }
        //Return false for incorrect format parameter
        {
            ok( !psMetadataItemPrint(fd, "sos \n", item1),
                "psMetadataItemPrint:       return false for incorrect format.");
        }
        //Return false for type that doesn't match format (ie, %s for f32 value)
        {
            ok( !psMetadataItemPrint(fd, "%s", item2),
                "psMetadataItemPrint:       return false for item that doesn't match format.");
        }
    
        //Return false for incorrect format parameter
        {
            ok( !psMetadataItemPrint(fd, "%z", item1),
                "psMetadataItemPrint:       return false for incorrect format.");
        }
    
    
        //Return true for format parameter containing additional modifiers (F32)
        {
            ok( psMetadataItemPrint(fd, "%.3f\n", item2),
                "psMetadataItemPrint:       return true for format with .3 modifier.");
        }
        //Return true for valid string input
        {
            ok( psMetadataItemPrint(fd, "%s\n", item3),
                "psMetadataItemPrint:       return true for valid string case w/s.");
        }
        //Return true for valid ptr input
        {
            ok( psMetadataItemPrint(fd, "%p\n", item3),
                "psMetadataItemPrint:       return true for valid ptr case w/p.");
        }
    
        //Return true for format = %d, type = int
        {
            ok( psMetadataItemPrint(fd, "%d\n", item4),
                "psMetadataItemPrint:       return true for valid case w/d.");
        }
        //Return true for format = %i, type = int
        {
            ok( psMetadataItemPrint(fd, "%i\n", item4),
                "psMetadataItemPrint:       return true for valid case w/i.");
        }
        //Return true for format = %c, type = int
        {
            ok( psMetadataItemPrint(fd, "%c\n", item5),
                "psMetadataItemPrint:       return true for valid case w/c.");
        }
        //Return true for format = %o, type = int
        {
            ok( psMetadataItemPrint(fd, "%o\n", item4),
                "psMetadataItemPrint:       return true for valid case w/d.");
        }
        //Return true for format = %u, type = int
        {
            ok( psMetadataItemPrint(fd, "%u\n", item4),
                "psMetadataItemPrint:       return true for valid case w/i.");
        }
        //Return true for format = %x, type = int
        {
            ok( psMetadataItemPrint(fd, "%x\n", item5),
                "psMetadataItemPrint:       return true for valid case w/c.");
        }
        //Return true for format = %X, type = int
        {
            ok( psMetadataItemPrint(fd, "%X\n", item5),
                "psMetadataItemPrint:       return true for valid case w/c.");
        }
    
        //Return true for format = %e, type = int
        {
            ok( psMetadataItemPrint(fd, "%.1e\n", item2),
                "psMetadataItemPrint:       return true for valid case w/e.");
        }
        //Return true for format = %E, type = int
        {
            ok( psMetadataItemPrint(fd, "%.2E\n", item2),
                "psMetadataItemPrint:       return true for valid case w/E.");
        }
        //Return true for format = %F, type = int
        {
            ok( psMetadataItemPrint(fd, "%.3F\n", item2),
                "psMetadataItemPrint:       return true for valid case w/F.");
        }
        //Return true for format = %g, type = int
        {
            ok( psMetadataItemPrint(fd, "%.3g\n", item2),
                "psMetadataItemPrint:       return true for valid case w/g.");
        }
        //Return true for format = %G, type = int
        {
            ok( psMetadataItemPrint(fd, "%.4G\n", item2),
                "psMetadataItemPrint:       return true for valid case w/G.");
        }
        //Return true for format = %a, type = int
        {
            ok( psMetadataItemPrint(fd, "%.4a\n", item2),
                "psMetadataItemPrint:       return true for valid case w/a.");
        }
        //Return true for format = %A, type = int
        {
            ok( psMetadataItemPrint(fd, "%A\n", item2),
                "psMetadataItemPrint:       return true for valid case w/A.");
        }
    
        fclose(fd);
        unlink ("psMetadataItemPrint.out");
        psFree(item1);
        psFree(item2);
        psFree(item3);
        psFree(item4);
        psFree(item5);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}


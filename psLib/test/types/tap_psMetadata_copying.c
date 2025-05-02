/**
 *  C Implementation: tap_psMetadata_copying
 *
 * Description:  Tests for psMetadataCopy, psMetadataItemCopy, & psMetadataItemSupplement.
 *
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
    plan_tests(41);


    // testMetadataCopy()
    {
        psMemId id = psMemGetId();
        psMetadata *in = NULL;
        psMetadata *out = NULL;
        psMetadataItem *tempItem = NULL;
        psMetadataItem *outItem = NULL;
        psMetadataItem *inItem = NULL;
        psMetadata *outTest = NULL;
    
        //Return NULL for NULL psMetadata input
        {
            outTest = psMetadataCopy(out, NULL);
            ok( outTest == NULL,
                "psMetadataCopy:  return NULL for NULL input metadata.");
        }
        //Return allocated out for NULL psMetadata input (with allocated out)
        {
            out = psMetadataAlloc();
            outTest = psMetadataCopy(out, NULL);
            ok( outTest == NULL,
                "psMetadataCopy:  return out for NULL input metadata.");
            psFree(out);
            out = NULL;
        }
        //Return NULL for attempt to copy item of unsupported data type - NULL out metadata
        psMetadataItem *notype = NULL;
        {
            in = psMetadataAlloc();
            psMetadataAddS32(in, PS_LIST_HEAD, "S32", 0, "", 1);
            notype = psMetadataGet(in, PS_LIST_HEAD);
            notype->type = -1;
            outTest = psMetadataCopy(out, in);
            ok( outTest == NULL,
                "psMetadataCopy:  return NULL for metadata with item of bad type.");
            psFree(out);
            out = NULL;
        }
        //Return NULL for attempt to copy item of unsupported data type - allocated out metadata
        {
            out = psMetadataAlloc();
            psMetadataAddS32(out, PS_LIST_TAIL, "S32", 0, "", 2);
            outTest = psMetadataCopy(out, in);
            ok( outTest == NULL,
                "psMetadataCopy:  return NULL for metadata with item of bad type.");
            notype->type = PS_DATA_S32;
            psFree(out);
            out = NULL;
            psFree(in);
            in = NULL;
        }
    
        //Return NULL for NULL psMetadata input->list
        in = psMetadataAlloc();
        if (in->list != NULL) {
            psFree(in->list);
            in->list = NULL;
        }
        {
            outTest = psMetadataCopy(out, in);
            ok( outTest == NULL,
                "psMetadataCopy:  return NULL for NULL metadata input->list.");
        }
        /*    //Return NULL for NULL psMetadata input->list (with allocated out)
            {
                out = psMetadataAlloc();
                out = psMetadataCopy(out, in);
                ok( out == NULL,
                    "psMetadataCopy:  return NULL for NULL metadata input->list.");
            }
        */
        //Return valid (but empty) metadata for NULL out input and empty input metadata.
        in->list = psListAlloc(NULL);
        psFree(in);
        in = psMetadataAlloc();
        if (out != NULL) {
            psFree(out);
            out = NULL;
        }
        outTest = psMetadataCopy(out, in);
        tempItem = psMetadataGet(in, PS_LIST_HEAD);
        {
            ok( outTest != NULL && tempItem == NULL,
                "psMetadataCopy:  return empty copy with no items for empty input metadata.");
            psFree(out);
            psFree(outTest);
            out = NULL;
            outTest = NULL;
        }
    
        //Test case: multiCheckItem->type == PS_DATA_METADATA_MULTI
        psMetadataAddS32(in, PS_LIST_HEAD, "S32", 0, "", 1);
        psMetadataAddS32(in, PS_LIST_TAIL, "S32", PS_META_DUPLICATE_OK, "", 1);
        outTest = psMetadataCopy(out, in);
        tempItem = psMetadataGet(outTest, PS_LIST_HEAD);
        {
            ok( tempItem->type == PS_DATA_S32 && tempItem->data.S32 == 1,
                "psMetadataCopy:  return valid copy for a PS_DATA_METADATA_MULTI entry.");
            psFree(outTest);
            outTest = NULL;
        }
    
        //Test case:  !psMetadataAddItem(out, newItem, PS_LIST_TAIL, flag)
        //XXX: doesn't seem to be easily reachable, certainly by the user...  Skipping.
    
        //Test case: simple valid test
        psFree(in);
        psFree(out);
        out = NULL;
        in = psMetadataAlloc();
        psMetadataAddS32(in, PS_LIST_HEAD, "S32", 0, "", 666);
        outTest = psMetadataCopy(out, in);
        outItem = psMetadataGet(outTest, PS_LIST_HEAD);
        inItem = psMetadataGet(in, PS_LIST_HEAD);
        {
            ok( outItem->type == PS_DATA_S32 && outItem->data.S32 == inItem->data.S32 &&
                (strncmp(inItem->name, outItem->name, 10) == 0),
                "psMetadataCopy:  return valid copy of metadata with 1 S32 entry.");
        }
        psFree(outTest);
        psFree(in);
        psFree(out);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testMetadataItemCopy()
    {
        psMemId id = psMemGetId();
        //Return NULL for NULL input psMetadataItem
        {
            ok( psMetadataItemCopy(NULL) == NULL,
                "psMetadataItemCopy:   return NULL for NULL input metadataItem.");
        }
    
        //Test Simple types:  Bool, S8, S16, S32, S64, U8, U16, U32, U64, F32, F64, STRING
        psMetadataItem *itemBool = psMetadataItemAlloc("itemBool", PS_DATA_BOOL, "", false);
        psMetadataItem *itemS8  =  psMetadataItemAlloc("itemS8", PS_DATA_S8, "", 1);
        psMetadataItem *itemS16 =  psMetadataItemAlloc("itemS16", PS_DATA_S16, "", 2);
        psMetadataItem *itemS32 =  psMetadataItemAlloc("itemS32", PS_DATA_S32, "", 3);
        psMetadataItem *itemS64 =  psMetadataItemAlloc("itemS64", PS_DATA_S64, "", (psS64)4);
        psMetadataItem *itemU8  =  psMetadataItemAlloc("itemU8", PS_DATA_U8, "", 1);
        psMetadataItem *itemU16 =  psMetadataItemAlloc("itemU16", PS_DATA_U16, "", 2);
        psMetadataItem *itemU32 =  psMetadataItemAlloc("itemU32", PS_DATA_U32, "", 3);
        psMetadataItem *itemU64 =  psMetadataItemAlloc("itemU64", PS_DATA_U64, "", 4);
        psMetadataItem *itemF32 =  psMetadataItemAlloc("itemF32", PS_DATA_F32, "", 3.0);
        psMetadataItem *itemF64 =  psMetadataItemAlloc("itemF64", PS_DATA_F64, "", 4.0);
        psMetadataItem *itemString = psMetadataItemAlloc("itemString", PS_DATA_STRING, "", "I M STRiNG");
        psMetadataItem *copy = NULL;
        //Bool case
        {
            copy = psMetadataItemCopy(itemBool);
            ok( (copy->type == PS_DATA_BOOL && (strncmp(copy->name, "itemBool", 10) == 0) &&
                 !copy->data.B && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of BOOL item.");
            psFree(copy);
        }
        //S8 case
        {
            copy = psMetadataItemCopy(itemS8);
            ok( (copy->type == PS_DATA_S8 && (strncmp(copy->name, "itemS8", 10) == 0) &&
                 copy->data.S8 == 1 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of S8 item.");
            psFree(copy);
        }
        //S16 case
        {
            copy = psMetadataItemCopy(itemS16);
            ok( (copy->type == PS_DATA_S16 && (strncmp(copy->name, "itemS16", 10) == 0) &&
                 copy->data.S16 == 2 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of S16 item.");
            psFree(copy);
        }
        //S32 case
        {
            copy = psMetadataItemCopy(itemS32);
            ok( (copy->type == PS_DATA_S32 && (strncmp(copy->name, "itemS32", 10) == 0) &&
                 copy->data.S32 == 3 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of S32 item.");
            psFree(copy);
        }
    
        // S64 case
        {
            psMetadataItem *copy = psMetadataItemCopy(itemS64);
    
            ok(copy->type == PS_DATA_S64, "item type");
            is_str(copy->name, "itemS64", "item name");
            is_long(copy->data.S64, 4, "item data");
            is_str(copy->comment, "", "item comment");
    
            psFree(copy);
        }
    
        //U8 case
        {
            copy = psMetadataItemCopy(itemU8);
            ok( (copy->type == PS_DATA_U8 && (strncmp(copy->name, "itemU8", 10) == 0) &&
                 copy->data.U8 == 1 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of U8 item.");
            psFree(copy);
        }
        //U16 case
        {
            copy = psMetadataItemCopy(itemU16);
            ok( (copy->type == PS_DATA_U16 && (strncmp(copy->name, "itemU16", 10) == 0) &&
                 copy->data.U16 == 2 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of U16 item.");
            psFree(copy);
        }
        //U32 case
        {
            copy = psMetadataItemCopy(itemU32);
            ok( (copy->type == PS_DATA_U32 && (strncmp(copy->name, "itemU32", 10) == 0) &&
                 copy->data.U32 == 3 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of U32 item.");
            psFree(copy);
        }
        //U64 case
        {
            copy = psMetadataItemCopy(itemU64);
            ok( (copy->type == PS_DATA_U64 && (strncmp(copy->name, "itemU64", 10) == 0) &&
                 copy->data.U64 == 4 && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of U64 item.");
            psFree(copy);
        }
        //F32 case
        {
            copy = psMetadataItemCopy(itemF32);
            ok( (copy->type == PS_DATA_F32 && (strncmp(copy->name, "itemF32", 10) == 0) &&
                 abs(copy->data.F32-3.0) < FLT_EPSILON && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of F32 item.");
            psFree(copy);
        }
        //F64 case
        {
            copy = psMetadataItemCopy(itemF64);
            ok( (copy->type == PS_DATA_F64 && (strncmp(copy->name, "itemF64", 10) == 0) &&
                 abs(copy->data.F64-4.0) < DBL_EPSILON && strncmp(copy->comment, "", 2) == 0),
                "psMetadataItemCopy:  return valid copy of F64 item.");
            psFree(copy);
        }
        //STRING case
        {
            copy = psMetadataItemCopy(itemString);
            ok( (copy->type == PS_DATA_STRING && (strncmp(copy->name, "itemString", 12) == 0) &&
                 strncmp(copy->data.str,"I M STRiNG",12) == 0 && strncmp(copy->comment,"",2) == 0),
                "psMetadataItemCopy:  return valid copy of STRING item.");
            psFree(copy);
        }
    
        //Test Remaining types:  Vector, Time, Metadata, Region
        psVector *inV = psVectorAlloc(2, PS_TYPE_S32);
        psVectorSet(inV, 0, 666);
        psVectorSet(inV, 1, 667);
        psMetadataItem *itemVector = psMetadataItemAlloc("itemVector", PS_DATA_VECTOR, "", inV);
        psTime *teaTime = psTimeAlloc(PS_TIME_TAI);
        teaTime->sec = 1;
        teaTime->nsec = 0;
        teaTime->leapsecond = true;
        psMetadataItem *itemTime = psMetadataItemAlloc("itemTime", PS_DATA_TIME, "", teaTime);
        psMetadata *md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "mdS32", 0, "", 666);
        psMetadataItem *itemMD = psMetadataItemAlloc("itemMD", PS_DATA_METADATA, "", md);
        psRegion *reg = psRegionAlloc(0.0, 2.0, 1.0, 3.0);
        psMetadataItem *itemRegion = psMetadataItemAlloc("itemRegion", PS_DATA_REGION, "", reg);
        //Vector case
        {
            copy = psMetadataItemCopy(itemVector);
            int int0 = ((psVector*)(copy->data.V))->data.S32[0];
            int int1 = ((psVector*)(copy->data.V))->data.S32[1];
            ok( (copy->type == PS_DATA_VECTOR && (strncmp(copy->name, "itemVector", 12) == 0)
                 && int0 == 666 && int1 == 667 && strncmp(copy->comment,"",2) == 0),
                "psMetadataItemCopy:  return valid copy of Vector item.");
            psFree(copy);
        }
        //Vector case - invalid vector item
        {
            psFree(itemVector->data.V);
            itemVector->data.V = NULL;
            copy = psMetadataItemCopy(itemVector);
            ok( copy == NULL, "psMetadataItemCopy:  return NULL for invalid vector item");
            //        itemVector->data.V = psVectorAlloc(1, PS_TYPE_S32);
        }
        //Time case
        {
            copy = psMetadataItemCopy(itemTime);
            int int0 = (int)((psTime*)(copy->data.V))->sec;
            int int1 = (int)((psTime*)(copy->data.V))->nsec;
            bool leap = ((psTime*)(copy->data.V))->leapsecond;
            ok( (copy->type == PS_DATA_TIME && (strncmp(copy->name, "itemTime", 12) == 0)
                 && int0 == 1 && int1 == 0 && strncmp(copy->comment,"",2) == 0) && leap,
                "psMetadataItemCopy:  return valid copy of Time item.");
            psFree(copy);
        }
        //Time case - invalid time item
        {
            psTime *t = psTimeAlloc(PS_TIME_TT);
            t->type += 1;
            psMetadataItem *itemTime2 = psMetadataItemAlloc("itemTime", PS_DATA_TIME, "", t);
            copy = psMetadataItemCopy(itemTime2);
            ok( copy != NULL && copy->data.V == NULL,
                "psMetadataItemCopy:  return non-NULL item for invalid input time item");
            psFree(t);
            psFree(copy);
            psFree(itemTime2);
        }
        //Time case - NULL time item
        {
            psFree(itemTime->data.V);
            itemTime->data.V = NULL;
            copy = psMetadataItemCopy(itemTime);
            ok( copy != NULL && copy->data.V == NULL,
                "psMetadataItemCopy:  return non-NULL item for NULL input time item");
            //        (itemTime->data.V) = psTimeAlloc(PS_TIME_TAI);
            psFree(copy);
        }
        //Metadata case
        {
            copy = psMetadataItemCopy(itemMD);
            psMetadataItem *testItem = NULL;
            testItem = psMetadataGet(((psMetadata*)(copy->data.V)), PS_LIST_HEAD);
            if (testItem == NULL)
                printf("\nError in psMetadataGet.\n");
            ok( copy->type == PS_DATA_METADATA && strncmp(copy->name, "itemMD", 8) == 0
                && (strncmp(copy->comment, "",2) == 0) && testItem->type == PS_DATA_S32
                && (strncmp(testItem->name, "mdS32", 10) == 0) && testItem->data.S32 == 666
                && (strncmp(testItem->comment,"",2) == 0),
                "psMetadataItemCopy:  return valid copy of Metadata item.");
            psFree(copy);
        }
        //Region case
        {
            copy = psMetadataItemCopy(itemRegion);
            float x0 = ((psRegion*)(copy->data.V))->x0;
            float x1 = ((psRegion*)(copy->data.V))->x1;
            float y0 = ((psRegion*)(copy->data.V))->y0;
            float y1 = ((psRegion*)(copy->data.V))->y1;
            ok( (copy->type == PS_DATA_REGION && (strncmp(copy->name, "itemRegion", 12) == 0)
                 && abs(x0-0.0) < FLT_EPSILON && abs(x1-2.0) < FLT_EPSILON
                 && abs(y0-1.0) < FLT_EPSILON && abs(y1-3.0) < FLT_EPSILON
                 && strncmp(copy->comment,"",2) == 0),
                "psMetadataItemCopy:  return valid copy of Region item.");
            psFree(copy);
        }
    
        //Test Unsupported type - (sphere for example)
        psSphere *sphere = psSphereAlloc();
        sphere->r = 1.0;
        sphere->d = 6.66;
        psMetadataItem *itemSphere = psMetadataItemAlloc("itemSphere", PS_DATA_SPHERE, "", sphere);
        psFree(sphere);
        {
            copy = psMetadataItemCopy(itemSphere);
            float R = ((psSphere*)(copy->data.V))->r;
            float D = ((psSphere*)(copy->data.V))->d;
            ok( (copy->type == PS_DATA_SPHERE && (strncmp(copy->name, "itemSphere", 12) == 0) &&
                 abs(R - 1.0) < FLT_EPSILON && abs(D - 6.66) < FLT_EPSILON &&
                 strncmp(copy->comment,"",2) == 0),
                "psMetadataItemCopy:  return valid copy of SPHERE item.");
            psFree(copy);
        }
    
        psFree(itemSphere);
        psFree(inV);
        psFree(teaTime);
        psFree(md);
        psFree(reg);
        psFree(itemBool);
        psFree(itemS8);
        psFree(itemS16);
        psFree(itemS32);
        psFree(itemS64);
        psFree(itemU8);
        psFree(itemU16);
        psFree(itemU32);
        psFree(itemU64);
        psFree(itemF32);
        psFree(itemF64);
        psFree(itemString);
        psFree(itemVector);
        psFree(itemTime);
        psFree(itemMD);
        psFree(itemRegion);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testMetadataItemTransfer()
    {
        psMemId id = psMemGetId();
        psMetadata *out = psMetadataAlloc();
        psMetadata *in = psMetadataAlloc();
        psString key = psStringAlloc(30);
        strncpy(key, "key", 4);
    
        //Return false for NULL in input
        {
	  ok( !psMetadataItemSupplement(NULL, out, NULL, key),
                "psMetadataItemSupplement:  return false for NULL in metadata.");
        }
        //Return false for NULL out input
        {
            ok( !psMetadataItemSupplement(NULL, NULL, in, key),
                "psMetadataItemSupplement:  return false for NULL out metadata.");
        }
        //Return false for NULL key input
        {
            ok( !psMetadataItemSupplement(NULL, out, in, NULL),
                "psMetadataItemSupplement:  return false for NULL key string.");
        }
        psMetadataAddS32(in, PS_LIST_HEAD, "key", PS_META_NO_REPLACE, "", 666);
        psMetadataAddF32(out, PS_LIST_HEAD, "out", 0, "", 6.66);
    
        //Return true for valid inputs
        {
            bool transfered = psMetadataItemSupplement(NULL, out, in, key);
            psMetadataItem *item = NULL;
            if (transfered)
                item = psMetadataGet(out, PS_LIST_TAIL);
            ok(transfered && item->type == PS_DATA_S32 && strncmp(item->name, "key", 4) == 0
               && item->data.S32 == 666 && strncmp(item->comment, "", 2) == 0,
               "psMetadataItemSupplement:  return true for valid inputs.");
        }
        /*  Attempt here was to return false inside of ItemTransfer at psMetadataAddItem.
        //Return false for duplicate entry with PS_META_DEFAULT (no duplicates)
        {
            psMetadataRemoveKey(out, key);
            psMetadataItem *item = psMetadataGet(in, PS_LIST_HEAD);
            psMetadataAddItem(out, item, PS_LIST_TAIL, PS_META_NO_REPLACE);
            ok( !psMetadataItemSupplement(out, in, key),
                "psMetadataItemSupplement:  return false for duplicate metadataItem.");
        }
        */
        //Return false for invalid key
        {
            strncpy(key, "not MY key", 15);
            ok( !psMetadataItemSupplement(NULL, out, in, key),
                "psMetadataItemSupplement:  return false for invalid key.");
        }
        //Return false for invalid metadata
        psMetadata *invalid = psMetadataAlloc();
        psFree(invalid->hash);
        invalid->hash = NULL;
        {
            ok( !psMetadataItemSupplement(NULL, invalid, in, "key"),
                "psMetadataItemSupplement:  return false for invalid metadata.");
        }
        psFree(invalid);
    
        psFree(out);
        psFree(in);
        psFree(key);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

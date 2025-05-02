/**
*  C Implementation: tap_psMetadata_creating
*
* Description:  Tests for psMetadataAlloc, psMetadataItemAlloc's (TYPEs),
*               psMemCheckMetadata, psMemCheckMetadataItem, psMetadataAddItem,
*               psMetadataIteratorAlloc
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
    plan_tests(42);
    // psMetadata & psMetadataItem Creation Functions


    // testItemAllocs(void)
    // psMetadataItemAlloc Fxns
    {
        psMemId id = psMemGetId();
        psMetadataItem *itemStr = psMetadataItemAllocStr("itemStr", "", "itemStr");
        psMetadataItem *itemF32 = psMetadataItemAllocF32("itemF32", "", 6.66);
        psMetadataItem *itemF64 = psMetadataItemAllocF64("itemF64", "", 0.666);
        psMetadataItem *itemS8 = psMetadataItemAllocS8("itemS8", "", 0);
        psMetadataItem *itemS16 = psMetadataItemAllocS16("itemS16", "", 1);
        psMetadataItem *itemS32 = psMetadataItemAllocS32("itemS32", "", 2);
        psMetadataItem *itemS64 = psMetadataItemAllocS64("itemS64", "", 3);
        psMetadataItem *itemU8 = psMetadataItemAllocU8("itemU8", "", 0);
        psMetadataItem *itemU16 = psMetadataItemAllocU16("itemU16", "", 1);
        psMetadataItem *itemU32 = psMetadataItemAllocU32("itemU32", "", 2);
        psMetadataItem *itemU64 = psMetadataItemAllocU64("itemU64", "", 3);
        psMetadataItem *itemBool = psMetadataItemAllocBool("itemBool", "", true);
    
        //First try to free a NULL item
        psMetadataItem *nullItem = NULL;
        psFree(nullItem);
        //Verify correct allocation
        //String
        {
            skip_start(itemStr == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocStr() failed");
            ok(itemStr->type == PS_DATA_STRING
               && !strncmp(itemStr->name, "itemStr", 10)
               && !strncmp(itemStr->data.V, "itemStr", 10)
               && !strncmp(itemStr->comment, "", 2),
               "psMetadataItemAllocStr:    create valid string item.");
            skip_end();
        }
        //F32
        {
            skip_start(itemF32 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocF32() failed");
            ok(itemF32->type == PS_DATA_F32
               && !strncmp(itemF32->name, "itemF32", 10)
               && abs(itemF32->data.F32 - 6.66) < FLT_EPSILON
               && !strncmp(itemF32->comment, "", 2),
               "psMetadataItemAllocF32:    create valid F32 item.");
            skip_end();
        }
        //F64
        {
            skip_start(itemF64 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocF64() failed");
            ok(itemF64->type == PS_DATA_F64
               && !strncmp(itemF64->name, "itemF64", 10)
               && abs(itemF64->data.F64 - 0.666) < DBL_EPSILON
               && !strncmp(itemF64->comment, "", 2),
               "psMetadataItemAllocF64:    create valid F64 item.");
            skip_end();
        }
        //S8
        {
            skip_start(itemS8 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocS8() failed");
            ok(itemS8->type == PS_DATA_S8
               && !strncmp(itemS8->name, "itemS8", 10)
               && itemS8->data.S8 == 0
               && !strncmp(itemS8->comment, "", 2),
               "psMetadataItemAllocS8:     create valid S8 item.");
            skip_end();
        }
        //S16
        {
            skip_start(itemS16 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocS16() failed");
            ok(itemS16->type == PS_DATA_S16
               && !strncmp(itemS16->name, "itemS16", 10)
               && itemS16->data.S16 == 1
               && !strncmp(itemS16->comment, "", 2),
               "psMetadataItemAllocS16:    create valid S16 item.");
            skip_end();
        }
        //S32
        {
            skip_start(itemS32 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocS32() failed");
            ok(itemS32->type == PS_DATA_S32
               && !strncmp(itemS32->name, "itemS32", 10)
               && itemS32->data.S32 == 2
               && !strncmp(itemS32->comment, "", 2),
               "psMetadataItemAllocS32:    create valid S32 item.");
            skip_end();
        }
        //S64
        {
            skip_start(itemS64 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocS64() failed");
            ok(itemS64->type == PS_DATA_S64
               && !strncmp(itemS64->name, "itemS64", 10)
               && itemS64->data.S64 == 3
               && !strncmp(itemS64->comment, "", 2),
               "psMetadataItemAllocS64:    create valid S64 item.");
            skip_end();
        }
        //U8
        {
            skip_start(itemU8 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocU8() failed");
            ok(itemU8->type == PS_DATA_U8
               && !strncmp(itemU8->name, "itemU8", 10)
               && itemU8->data.U8 == 0
               && !strncmp(itemU8->comment, "", 2),
               "psMetadataItemAllocU8:     create valid U8 item.");
            skip_end();
        }
        //U16
        {
            skip_start(itemU16 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocU16() failed");
            ok(itemU16->type == PS_DATA_U16
               && !strncmp(itemU16->name, "itemU16", 10)
               && itemU16->data.U16 == 1
               && !strncmp(itemU16->comment, "", 2),
               "psMetadataItemAllocU16:    create valid U16 item.");
            skip_end();
        }
        //U32
        {
            skip_start(itemU32 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocU32() failed");
            ok(itemU32->type == PS_DATA_U32
               && !strncmp(itemU32->name, "itemU32", 10)
               && itemU32->data.U32 == 2
               && !strncmp(itemU32->comment, "", 2),
               "psMetadataItemAllocU32:   create valid U32 item.");
            skip_end();
        }
        //U64
        {
            skip_start(itemU64 == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocU64() failed");
            ok(itemU64->type == PS_DATA_U64
               && !strncmp(itemU64->name, "itemU64", 10)
               && itemU64->data.U64 == 3
               && !strncmp(itemU64->comment, "", 2),
               "psMetadataItemAllocU64:   create valid U64 item.");
            skip_end();
        }
        //Bool
        {
            skip_start(itemBool == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocBool() failed");
            ok(itemBool->type == PS_DATA_BOOL
               && !strncmp(itemBool->name, "itemBool", 10)
               && itemBool->data.B
               && !strncmp(itemBool->comment, "", 2),
               "psMetadataItemAllocBool:  create valid Bool item.");
            skip_end();
        }
    
        //Allocate a pointer.  Example= psSphere
        {
            psSphere *s = psSphereAlloc();
            s->r = 1.1;
            s->d = 2.1;
            psMetadataItem *itemPtr = psMetadataItemAllocPtr("itemPtr", PS_DATA_SPHERE, "", s);
            skip_start(itemPtr == NULL, 1,
                       "Skipping 1 tests because psMetadataItemAllocPtr() failed");
            float r = ((psSphere*)(itemPtr->data.V))->r;
            float d = ((psSphere*)(itemPtr->data.V))->d;
            ok( itemPtr->type == PS_DATA_SPHERE
                && !strncmp(itemPtr->name, "itemPtr", 10)
                && abs(r-1.1) < FLT_EPSILON && abs(d-2.1) < FLT_EPSILON
                && !strncmp(itemPtr->comment, "", 2),
                "psMetadataItemAllocPtr:   create valid ptr (psSphere) item.");
            skip_end();
            psFree(s);
            psFree(itemPtr);
        }
    
        //Try to Allocate an unsupported type with AllocV
        {
            psMetadataItem *itemNULL = NULL;
            psLine *line = psLineAlloc(2);
            itemNULL = psMetadataItemAlloc("itemNULL", PS_DATA_LINE, NULL, line);
            ok( itemNULL == NULL,
                "psMetadataItemAlloc:      return NULL for unsupported type.");
            psFree(line);
        }
    
        //Now Allocate the item with Alloc and test MemCheck for it's type.
        {
            psMetadataItem *itemAllocV = psMetadataItemAlloc("itemAllocV", PS_DATA_S32, NULL, 4);
            skip_start(itemAllocV == NULL, 2,
                       "Skipping 2 tests because psMetadataItemAllocV() failed");
            ok (itemAllocV->type == PS_DATA_S32
                && !strncmp(itemAllocV->name, "itemAllocV", 10)
                && itemAllocV->data.S32 == 4
                && !strncmp(itemAllocV->comment, "", 2),
                "psMetadataItemAlloc:      create valid S32 item.");
            ok ( psMemCheckMetadataItem(itemAllocV),
                 "psMemCheckMetadataItem:   return true for valid MetadataItem.");
            skip_end();
            psFree(itemAllocV);
        }
    
        //Make sure MemCheckItem worked.  Try primitive type.  Expect false.
	// XXX EAM : disabled
        if (0) {
            int j = 2;
            ok( !psMemCheckMetadataItem(&j),
                "psMemCheckMetadataItem:   return false for non-MetadataItem input.");
        }
    
        psFree(itemStr);
        psFree(itemF32);
        psFree(itemF64);
        psFree(itemS8);
        psFree(itemS16);
        psFree(itemS32);
        psFree(itemS64);
        psFree(itemU8);
        psFree(itemU16);
        psFree(itemU32);
        psFree(itemU64);
        psFree(itemBool);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psMetadataAlloc & psMetadataAdd Fxns");
    {
        psMemId id = psMemGetId();
        psMetadata *md = NULL;
        md = psMetadataAlloc();
        psMetadata *md1 = psMetadataAlloc();
        psMetadata *md2 = psMetadataAlloc();
        //test MemCheck for a valid psMetadata
        {
            ok( psMemCheckMetadata(md),
                "psMemCheckMetadata:       return true for valid Metadata.");
        }
        //Make sure MemCheck worked.  Try primitive type.  Expect false.
	// XXX EAM : disabled
        if (0) {
            int j = 2;
            ok( !psMemCheckMetadata(&j),
                "psMemCheckMetadata:       return false for non-Metadata input.");
        }
        //Return false for attempt to add a psMetadataItem with no name.
        {
            ok( !psMetadataAdd(md1, PS_LIST_HEAD, NULL, PS_DATA_S32, "", 1),
                "psMetadataAdd:            return false for adding item with no name.");
        }
        psFree(md1);
        md1 = psMetadataAlloc();
    
        //Return false for NULL metadata input
        psMetadataItem *item = psMetadataItemAllocBool("item", "", true);
        {
            ok( !psMetadataAddItem(NULL, item, PS_LIST_HEAD, PS_META_DEFAULT),
                "psMetadataAddItem:        return false for NULL metadata input.");
        }
        //Return false for metadata with NULL hash
        psFree(md1->hash);
        md1->hash = NULL;
        {
            ok( !psMetadataAddItem(md1, item, PS_LIST_HEAD, PS_META_DEFAULT),
                "psMetadataAddItem:        return false for metadata with no hash.");
        }
        psFree(md1);
        //Return false for metadata with NULL list
        psFree(md2->list);
        md2->list = NULL;
        {
            ok( !psMetadataAddItem(md2, item, PS_LIST_HEAD, PS_META_DEFAULT),
                "psMetadataAddItem:        return false for metadata with no list.");
        }
        psFree(md2);
        //Return false for NULL psMetadataItem input
        {
            ok( !psMetadataAddItem(md, NULL, PS_LIST_HEAD, PS_META_DEFAULT),
                "psMetadataAddItem:        return false for NULL item input.");
        }
        //Return false for psMetadataItem with NULL name
        psFree(item->name);
        item->name = NULL;
        {
            ok( !psMetadataAddItem(md, item, PS_LIST_HEAD, PS_META_DEFAULT),
                "psMetadataAddItem:        return false for item with no name.");
        }
        psFree(item);
    
        psMetadata *itemMD = psMetadataAlloc();
        psMetadataAddS32(itemMD, PS_LIST_HEAD, "s", PS_META_DUPLICATE_OK, "", 2);
        psMetadataItem *item2 = psMetadataItemAllocS32("s", "", 2);
    
        //Return true for addition of multi
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_DUPLICATE_OK),
                "psMetadataAddItem:        return true for addition of PS_DATA_METADATA_MULTI.");
        }
        psFree(item2);
        psFree(itemMD);
    
        //Existing entry with duplicate key found.
        psMetadataAddS32(md, PS_LIST_HEAD, "S32", 0, "", 1);
        item2 = psMetadataItemAllocS32("S32", "", 1);
        //FLAG = PS_META_REPLACE
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_REPLACE),
                "psMetadataAddItem:        return true for PS_META_REPLACE flag.");
        }
        //FLAG = PS_META_DUPLICATE_OK
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_DUPLICATE_OK),
                "psMetadataAddItem:        return true for PS_META_DUPLICATE_OK flag.");
        }
        /*        SEE BELOW.  TEST CASE REDONE b/c item2 in md was already a MULTI
        //FLAG = PS_META_NO_REPLACE
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_NO_REPLACE),
                "psMetadataAddItem:        return true for PS_META_NO_REPLACE flag.");
        }
        */
        //FLAG = PS_META_DEFAULT
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_DEFAULT),
                "psMetadataAddItem:        return false for PS_META_DEFAULT flag.");
        }
    
        //Return false for attempting to add an element twice with flag NO_REPLACE
        {
            psMetadata *norep = psMetadataAlloc();
            psMetadataAddS32(norep, PS_LIST_HEAD, "S32", 0, "", 1);
            psMetadataItem *item666 = psMetadataItemAllocS32("S32", "", 1);
            ok( psMetadataAddItem(norep, item666, PS_LIST_TAIL, PS_META_NO_REPLACE),
                "psMetadataAddItem:        return true for PS_META_NO_REPLACE flag.");
            psFree(item666);
            psFree(norep);
        }
        //Return false for bad list location
        {
            psMetadata *emptymeta = psMetadataAlloc();
            psMetadataItem *item666 = psMetadataItemAllocS32("S32", "", 1);
            ok( !psMetadataAddItem(emptymeta, item666, -100, PS_META_NO_REPLACE),
                "psMetadataAddItem:        return false for invalid list location.");
            psFree(item666);
            psFree(emptymeta);
        }
        //Return false for trying to add to MULTI with broken list element.
        {
            psMetadata *broken_list = psMetadataAlloc();
            psMetadataAddS32(broken_list, PS_LIST_HEAD, "S32", 0, "", 1);
            psMetadataAddS32(broken_list, PS_LIST_TAIL, "S32", PS_META_DUPLICATE_OK, "", 1);
            psMetadataItem *brokenptr = psMetadataLookup(broken_list, "S32");
            if (brokenptr->type != PS_DATA_METADATA_MULTI)
                printf("\nError, Error\n");
            else
            {
                psFree(brokenptr->data.list);
                brokenptr->data.list = NULL;
            }
            psMetadataItem *item666 = psMetadataItemAllocS32("S32", "", 1);
            ok( !psMetadataAddItem(broken_list, item666, PS_LIST_TAIL, PS_META_DUPLICATE_OK),
                "psMetadataAddItem:        return false for MULTI with NULL list.");
            psFree(item666);
            psFree(broken_list);
        }
    
        //Attempt to add a MULTI
        psFree(item2);
        item2 = NULL;
        psMetadata *xxx = psMetadataAlloc();
        psMetadataAddS32(xxx, PS_LIST_TAIL, "new S32", 0, "", 1);
        item2 = psMetadataItemAllocS32("new S32", "", 1);
        psMetadataAddItem(xxx, item2, PS_LIST_TAIL, PS_META_DUPLICATE_OK);
        psFree(item2);
        item2 = psMetadataLookup(xxx, "new S32");
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_DUPLICATE_OK),
                "psMetadataAddItem:        return true for new MULTI item");
        }
        //Attempt to add a 2nd reference of a MULTI to the originating metadata container
        {
            ok( !psMetadataAddItem(xxx, item2, PS_LIST_TAIL, PS_META_NO_REPLACE),
                "psMetadataAddItem:        return false for attempt to add 2nd reference"
                " of a MULTI");
        }
        psFree(xxx);
    
        //No existing entries or duplicates found
        item2 = psMetadataItemAllocS32("new", "", 2);
        {
            ok( psMetadataAddItem(md, item2, PS_LIST_TAIL, PS_META_DUPLICATE_OK),
                "psMetadataAddItem:        return true for new item.");
        }
    
        psFree(item2);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
    

    // psMetadataIteratorAlloc
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psMetadata *md2 = NULL;
        psMetadataAddS32(md, PS_LIST_HEAD, "S32_1", PS_META_NO_REPLACE, "", 1);
        psMetadataAddS32(md, PS_LIST_TAIL, "S32_2", PS_META_DEFAULT, "", 2);
    
        psMetadataIterator *iter = NULL;
        //Return NULL for NULL metadata input
        {
            iter = psMetadataIteratorAlloc(md2, PS_LIST_HEAD, NULL);
            ok( iter == NULL,
                "psMetadataIteratorAlloc:  return NULL for NULL metadata input.");
        }
        //Return NULL for metadata with no List
        md2 = psMetadataAlloc();
        psFree(md2->list);
        md2->list = NULL;
        {
            iter = psMetadataIteratorAlloc(md2, PS_LIST_HEAD, NULL);
            ok( iter == NULL,
                "psMetadataIteratorAlloc:  return NULL for metadata with no list.");
        }
        psFree(md2);
    
        //Return newly allocated MetadataIterator, regex=NULL
        {
            iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
            ok( iter != NULL,
                "psMetadataIteratorAlloc:  return valid iterator for valid inputs, regex=NULL.");
        }
        //Return NULL for attempt to allocate an iterator with unfound regex.
        psFree(iter);
        iter = NULL;
        {
            iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, "IPP.machines.sky");
            ok( iter == NULL,
                "psMetadataIteratorAlloc:  return NULL for not-found regex input");
        }
        //Return NULL for invalid regex input.
        {
            iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, "IPP.machines.sky\\");
            ok( iter == NULL,
                "psMetadataIteratorAlloc:  return NULL for invalid regex input");
        }
        //Return properly allocated iterator for valid non-null regex input.
        {
            iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, "S32_");
            ok( iter != NULL,
                "psMetadataIteratorAlloc:  return valid iterator for valid regex input");
        }
    
        //Check for Memory leaks
        {
            psFree(iter);
            psFree(md);
            checkMem();
        }
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}

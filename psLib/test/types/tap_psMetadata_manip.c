/**
 *  C Implementation: tap_psMetadata_manip
 *
 * Description:  Tests for psMetadataRemove's, psMetadataLookup's (TYPE),
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
    psLogSetLevel(PS_LOG_WARN);
    plan_tests(50);
    // psMetadataLookup, Remove, and Iterator Fxns


    // psMetadataLookup Functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psMetadataAddStr(md, PS_LIST_TAIL, "itemStr", 0, "I am a string", "GNIRTS");
        psMetadataAddBool(md, PS_LIST_TAIL, "itemBool", 0, "I am a boolean", true);
        psMetadataAddS8(md, PS_LIST_TAIL, "itemS8", 0, "I am S8", 6);
        psMetadataAddS16(md, PS_LIST_TAIL, "itemS16", 0, "I am S16", -666);
        psMetadataAddS32(md, PS_LIST_TAIL, "itemS32", 0, "I am a integer", 55);
        psMetadataAddS64(md, PS_LIST_TAIL, "itemS64", 0, "I am S64", 666);
        psMetadataAddU8(md, PS_LIST_TAIL, "itemU8", 0, "I am U8", 6);
        psMetadataAddU16(md, PS_LIST_TAIL, "itemU16", 0, "I am U16", 666);
        psMetadataAddU32(md, PS_LIST_TAIL, "itemU32", 0, "I am U32", 666);
        psMetadataAddU64(md, PS_LIST_TAIL, "itemU64", 0, "I am U64", 666);
        psMetadataAddF32(md, PS_LIST_TAIL, "itemF32", 0, NULL, 3.14);
        psMetadataAddF64(md, PS_LIST_TAIL, "itemF64", 0, "", 6.28);
        psSphere *sphere = psSphereAlloc();
        sphere->r = 6.66;
        sphere->d = 666.666;
        psMetadataAddPtr(md, PS_LIST_HEAD, "ptr", PS_DATA_SPHERE, "", sphere);
        //    psMetadataAddPtr(md, PS_LIST_TAIL, "ptr", PS_DATA_METADATA_MULTI, "", sphere);
        psTime *time;
        time = psTimeAlloc(PS_TIME_TAI);
        time->sec = 1000;
        time->nsec = 25;
        time->leapsecond = true;
        psMetadataAddTime(md, PS_LIST_TAIL, "time01", 0, "I am time", time);
        psMetadata *newMD = psMetadataAlloc();
        psMetadataAddS32(newMD, PS_LIST_TAIL, "1", 0, "", 666);
        psMetadataAddMetadata(md, PS_LIST_TAIL, "metadata7", 0, "I am a metadata", newMD);
    
        //Lookup Standard number-types (valid cases)
        //U8
        {
            bool status = false;
            psU8 u8 = 0;
            u8 = psMetadataLookupU8(&status, md, "itemU8");
            ok( status && u8 == 6,
                "psMetadataLookupU8:     return correct U8 value on lookup.");
        }
        //U16
        {
            bool status = false;
            psU16 u16 = 0;
            u16 = psMetadataLookupU16(&status, md, "itemU16");
            ok( status && u16 == 666,
                "psMetadataLookupU16:    return correct U16 value on lookup.");
        }
        //U32
        {
            bool status = false;
            psU32 u32 = 0;
            u32 = psMetadataLookupU32(&status, md, "itemU32");
            ok( status && u32 == 666,
                "psMetadataLookupU32:    return correct U32 value on lookup.");
        }
        //U64
        {
            bool status = false;
            psU64 u64 = 0;
            u64 = psMetadataLookupU64(&status, md, "itemU64");
            ok( status && u64 == 666,
                "psMetadataLookupU64:    return correct U64 value on lookup.");
        }
        //S8
        {
            bool status = false;
            psS8 s8 = 0;
            s8 = psMetadataLookupS8(&status, md, "itemS8");
            ok( status && s8 == 6,
                "psMetadataLookupS8:     return correct S8 value on lookup.");
        }
        //S16
        {
            bool status = false;
            psS16 s16 = 0;
            s16 = psMetadataLookupS16(&status, md, "itemS16");
            ok( status && s16 == -666,
                "psMetadataLookupS16:    return correct S16 value on lookup.");
        }
        //S32
        {
            bool status = false;
            psS32 s32 = 0;
            s32 = psMetadataLookupS32(&status, md, "itemS32");
            ok( status && s32 == 55,
                "psMetadataLookupS32:    return correct S32 value on lookup.");
        }
        //S64
        {
            bool status = false;
            psS64 s64 = 0;
            s64 = psMetadataLookupS64(&status, md, "itemS64");
            ok( status && s64 == 666,
                "psMetadataLookupS64:    return correct S64 value on lookup.");
        }
        //F32
        {
            bool status = false;
            psF32 f32 = 0;
            f32 = psMetadataLookupF32(&status, md, "itemF32");
            ok( status && abs(f32 - 3.14) < FLT_EPSILON,
                "psMetadataLookupF32:    return correct F32 value on lookup.");
        }
        //F64
        {
            bool status = false;
            psF64 f64 = 0;
            f64 = psMetadataLookupF64(&status, md, "itemF64");
            ok( status && abs(f64 - 6.28) < DBL_EPSILON,
                "psMetadataLookupF64:    return correct F64 value on lookup.");
        }
        //Bool
        {
            bool status = false;
            bool stat = false;
            stat = psMetadataLookupBool(&status, md, "itemBool");
            ok( status && stat,
                "psMetadataLookupBool:   return correct Bool value on lookup.");
        }
        //String
        {
            bool status = false;
            psString str = NULL;
            str = psMetadataLookupStr(&status, md, "itemStr");
            ok( status && !strncmp(str, "GNIRTS", 8),
                "psMetadataLookupStr:   return correct String value on lookup.");
        }
        //Pointer
        {
            bool status = false;
            psSphere *sph = NULL;
            sph = (psSphere*)(psMetadataLookupPtr(&status, md, "ptr"));
            ok( status && sph != NULL && abs(sph->r-6.66) < FLT_EPSILON,
                "psMetadataLookupPtr:   return correct pointer value on lookup.");
        }
        //Time
        {
            bool status = false;
            psTime *t = NULL;
            t = psMetadataLookupTime(&status, md, "time01");
            ok( status && t->sec == 1000,
                "psMetadataLookupTime:  return correct time value on lookup.");
        }
        //Metadata
        {
            bool status = false;
            psMetadata *mdtemp = NULL;
            mdtemp = psMetadataLookupMetadata(&status, md, "metadata7");
            psMetadataItem *itemtemp = psMetadataGet(mdtemp, PS_LIST_HEAD);
            ok( status && itemtemp->type == PS_DATA_S32 && itemtemp->data.S32 == 666,
                "psMetadataLookupMD:    return correct metadata value on lookup.");
        }
    
        //Try Negatives and non-standard lookup cases
        //String  - invalid key name
        {
            bool status = false;
            psString str = NULL;
            str = psMetadataLookupStr(&status, md, "item");
            ok( !status && str == NULL,
                "psMetadataLookupStr:   return NULL for incorrect key.");
            str = NULL;
            str = psMetadataLookupStr(NULL, md, "item");
            ok( str == NULL,
                "psMetadataLookupStr:   return NULL for incorrect key (& NULL status).");
        }
        //String - wrong key->  lookup s8 data
        {
            bool status = false;
            psString str = NULL;
            str = psMetadataLookupStr(&status, md, "itemS8");
            ok( !status && str == NULL,
                "psMetadataLookupStr:   return NULL for incorrect key .");
            str = NULL;
            str = psMetadataLookupStr(NULL, md, "itemS8");
            ok( str == NULL,
                "psMetadataLookupStr:   return NULL for incorrect key (& NULL status).");
        }
        //Pointer - invalid key name
        {
            bool status = false;
            psSphere *sph = NULL;
            sph = psMetadataLookupPtr(&status, md, "item");
            ok( !status && sph == NULL,
                "psMetadataLookupPtr:   return NULL for incorrect key.");
        }
        //Pointer - wrong key -> lookup s8 data
        {
            bool status = false;
            psSphere *sph = NULL;
            sph = psMetadataLookupPtr(&status, md, "itemS8");
            ok( !status && sph == NULL,
                "psMetadataLookupPtr:   return NULL for incorrect key.");
        }
    
        //Pointer**  - lookup a PS_DATA_METADATA_MULTI
        {
            psMetadata *metadata = psMetadataAlloc();
            psMetadataItem *multi = psMetadataItemAlloc("ptr", PS_DATA_METADATA_MULTI, "", NULL);
            psMetadataAddItem(metadata, multi, PS_LIST_HEAD, PS_META_DUPLICATE_OK);
            psMetadataAddPtr(metadata, PS_LIST_TAIL, "ptr",
                             PS_DATA_SPHERE | PS_META_DUPLICATE_OK, "", sphere);
            psSphere *s = psSphereAlloc();
            s->r = 0.666;
            psMetadataAddPtr(metadata, PS_LIST_TAIL, "ptr", PS_DATA_SPHERE | PS_META_DUPLICATE_OK, "", s);
            psFree(s);
            // psMetadataItem *metadataItem = NULL;
            // metadataItem = psMetadataLookup(metadata, "ptr");
            bool status = false;
            psSphere *sph = NULL;
            sph = (psSphere*)(psMetadataLookupPtr(&status, metadata, "ptr"));
            ok( status && sph != NULL && fabs(sph->r - 6.66) < FLT_EPSILON,
                "psMetadataLookupPtr:   return correct value on lookup of MULTI item.");
            psFree(metadata);
            psFree(multi);
        }
    
        //Metadata - invalid key name
        {
            bool status = false;
            psMetadata *met = NULL;
            met = psMetadataLookupMetadata(&status, md, "item");
            ok( !status && met == NULL,
                "psMetadataLookupMD:   return NULL for incorrect key.");
            met = NULL;
            met = psMetadataLookupMetadata(NULL, md, "item");
            ok( met == NULL,
                "psMetadataLookupMD:   return NULL for incorrect key (& NULL status).");
        }
        //Metadata - wrong key -> lookup s8 data
        {
            bool status = false;
            psMetadata *met = NULL;
            met = psMetadataLookupMetadata(&status, md, "itemS8");
            ok( !status && met == NULL,
                "psMetadataLookupMD:   return NULL for incorrect key (& NULL status).");
            met = NULL;
            met = psMetadataLookupMetadata(NULL, md, "itemS8");
            ok( met == NULL,
                "psMetadataLookupMD:   return NULL for incorrect key.");
        }
        //Time - invalid key name
        {
            bool status = false;
            psTime *time = NULL;
            time = psMetadataLookupTime(&status, md, "item");
            ok( !status && time == NULL,
                "psMetadataLookupTime:   return NULL for incorrect key.");
            time = NULL;
            time = psMetadataLookupTime(NULL, md, "item");
            ok( time == NULL,
                "psMetadataLookupTime:   return NULL for incorrect key (& NULL status).");
        }
        //Time - wrong key -> lookup s8 data
        {
            bool status = false;
            psTime *time = NULL;
            time = psMetadataLookupTime(&status, md, "itemS8");
            ok( !status && time == NULL,
                "psMetadataLookupTime:   return NULL for incorrect key.");
            time = NULL;
            time = psMetadataLookupTime(NULL, md, "itemS8");
            ok( time == NULL,
                "psMetadataLookupTime:   return NULL for incorrect key (& NULL status).");
        }
    
        psFree(sphere);
        psFree(time);
        psFree(newMD);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psMetadataRemove Functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psMetadataAddStr(md, PS_LIST_TAIL, "itemStr", 0, "I am a string", "GNIRTS");
        psMetadataAddBool(md, PS_LIST_TAIL, "itemBool", 0, "I am a boolean", true);
        psMetadataAddS8(md, PS_LIST_TAIL, "itemS8", 0, "I am S8", 6);
        psMetadataAddS16(md, PS_LIST_TAIL, "itemS16", 0, "I am S16", -666);
        psMetadataAddS32(md, PS_LIST_TAIL, "itemS32", 0, "I am a integer", 55);
        psMetadataAddS64(md, PS_LIST_TAIL, "itemS64", PS_META_DUPLICATE_OK, "I am S64", 666);
        psMetadataAddU8(md, PS_LIST_TAIL, "itemU8", 0, "I am U8", 6);
    
        //psMetadataRemoveKey
        {
            bool status = psMetadataRemoveKey(md, "itemU8");
            psMetadataItem *temp = psMetadataGet(md, PS_LIST_TAIL);
            ok( status && temp->type == PS_DATA_S64,
                "psMetadataRemoveKey:    return true for valid case.");
        }
        //psMetadataRemoveIndex
        {
            bool status = psMetadataRemoveIndex(md, PS_LIST_HEAD);
            psMetadataItem *temp = psMetadataGet(md, PS_LIST_HEAD);
            ok( status && temp->type == PS_DATA_BOOL,
                "psMetadataRemoveIndex:  return true for valid case.");
        }
        //Attempt to remove an incorrect key - Return false
        {
            ok( !psMetadataRemoveKey(md, "BigDog"),
                "psMetadataRemoveKey:    return false for incorrect key.");
        }
        //Attempt to remove an out-of-range index - Return false
        {
            ok( !psMetadataRemoveIndex(md, 20),
                "psMetadataRemoveIndex:  return false for incorrect index.");
        }
        //Attempt to remove an item with NULL name
        {
            psMetadataItem *temp = psMetadataGet(md, PS_LIST_HEAD);
            psFree(temp->name);
            temp->name = NULL;
            ok( !psMetadataRemoveIndex(md, PS_LIST_HEAD),
                "psMetadataRemoveIndex:  return false for item with no name.");
        }
        //Attempt to remove an item with wrong hash table
        {
            psMetadata *md2 = psMetadataAlloc();
            psMetadataAddBool(md2, PS_LIST_TAIL, "Bool", 0, "I am a boolean", true);
            psMetadataAddS8(md2, PS_LIST_TAIL, "S8", 0, "I am S8", 6);
            psMetadataAddS16(md2, PS_LIST_TAIL, "S16", 0, "I am S16", -666);
            psMetadataAddS32(md2, PS_LIST_TAIL, "S32", 0, "I am a integer", 55);
            psMetadataAddS64(md2, PS_LIST_TAIL, "S64", 0, "I am S64", 666);
    
            psFree(md2->hash);
            md2->hash = NULL;
            md2->hash = md->hash;
            ok( !psMetadataRemoveIndex(md2, PS_LIST_HEAD),
                "psMetadataRemoveIndex:  return false for item with bad table.");
            md2->hash = NULL;
            psFree(md2);
        }
        //Attempt to remove a METADATA_MULTI item by key
        {
            psMetadataAddS64(md, PS_LIST_TAIL, "itemS64", PS_META_DUPLICATE_OK, "I am S64", 667);
            psMetadataAddS64(md, PS_LIST_TAIL, "itemS64", PS_META_DUPLICATE_OK, "I am S64", 668);
            bool status = psMetadataRemoveKey(md, "itemS64");
            psMetadataItem *temp = psMetadataGet(md, PS_LIST_TAIL);
            ok( status && temp->type == PS_DATA_S32,
                "psMetadataRemoveKey:    return true for valid METADATA_MULTI case.");
    
        }
        //Attempt to remove a METADATA_MULTI item by index
        {
            psMetadataAddS64(md, PS_LIST_TAIL, "itemS64", PS_META_DUPLICATE_OK, "I am S64", 667);
            psMetadataAddS64(md, PS_LIST_TAIL, "itemS64", PS_META_DUPLICATE_OK, "I am S64", 668);
            bool status = psMetadataRemoveIndex(md, PS_LIST_TAIL);
            psMetadataItem *temp = psMetadataGet(md, PS_LIST_TAIL);
            ok( status && temp->type == PS_DATA_S64,
                "psMetadataRemoveIndex:  return true for valid METADATA_MULTI case.");
        }
    
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
    

    // psMetadataIterator Functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psMetadataAddS32(md, PS_LIST_HEAD, "S32_1", PS_META_NO_REPLACE, "", 1);
        psMetadataAddS32(md, PS_LIST_TAIL, "S32_2", PS_META_NO_REPLACE, "", 2);
        psMetadataAddS32(md, PS_LIST_TAIL, "S32_3", PS_META_NO_REPLACE, "", 3);
    
        psMetadataIterator *iter = NULL;
    
        //Return NULL in psMetadataGetAnd(In/De)crement for NULL iterator input
        {
            ok( psMetadataGetAndIncrement(NULL) == NULL,
                "psMetadataGetAndIncrement:  return NULL for NULL iterator input.");
            ok( psMetadataGetAndDecrement(NULL) == NULL,
                "psMetadataGetAndDecrement:  return NULL for NULL iterator input.");
        }
        //Return NULL in psMetadataGetAnd(In/De)crement for iterator with no list iterator
        psMetadataIterator *iter2 = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
        psFree(iter2->iter);
        iter2->iter = NULL;
        {
            ok( psMetadataGetAndIncrement(iter2) == NULL,
                "psMetadataGetAndIncrement:  return NULL for iterator with no list iterator.");
            ok( psMetadataGetAndDecrement(iter2) == NULL,
                "psMetadataGetAndDecrement:  return NULL for iterator with no list iterator.");
        }
        psFree(iter2);
    
        //Return valid iterator for valid inputs, regex= NULL
        psMetadataItem *item = NULL;
        {
            iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
            ok( iter != NULL,
                "psMetadataIteratorAlloc:    return valid iterator for valid inputs, regex=NULL.");
            skip_start(iter == NULL, 4,
                       "Skipping 4 tests because psMetadataIteratorAlloc failed");
    
            item = psMetadataGetAndIncrement(iter);
            ok( item != NULL,
                "psMetadataGetAndIncrement:  return valid item for valid iterator input.");
            skip_start(item == NULL, 1,
                       "Skipping 1 tests because psMetadataGetAndIncrement failed");
            ok(item->type == PS_DATA_S32
               && !strncmp(item->name, "S32_1", 7)
               && item->data.S32 == 1
               && !strncmp(item->comment, "", 2),
               "psMetadataGetAndIncrement:  retrieve correct item from valid iterator.");
            skip_end();
    
            item = psMetadataGetAndDecrement(iter);
            ok( item != NULL,
                "psMetadataGetAndDecrement:  return valid item for valid iterator input.");
            skip_start(item == NULL, 1,
                       "Skipping 1 tests because psMetadataGetAndDecrement failed");
            ok(item->type == PS_DATA_S32
               && !strncmp(item->name, "S32_2", 7)
               && item->data.S32 == 2
               && !strncmp(item->comment, "", 2),
               "psMetadataGetAndDecrement:  retrieve correct item from valid iterator.");
            skip_end();
    
            skip_end();
    
        }
    
        psFree(iter);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
    
    
                

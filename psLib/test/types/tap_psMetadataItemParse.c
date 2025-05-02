/**
 *  C Implementation: tap_psMetadataItemParse
 *
 * Description:  Tests for psMetadataItemParse
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
void testItemParse(void);

int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(112);

    // psMetadataItemParse() tests
    {
        psMemId id = psMemGetId();
        psMetadataItem *itemBool = psMetadataItemAlloc("bool", PS_DATA_BOOL, "No Comment", true);
        psMetadataItem *itemF32 = psMetadataItemAlloc("f32", PS_DATA_F32, "No Comment", 0.00);
        psMetadataItem *itemF64 = psMetadataItemAlloc("f64", PS_DATA_F64, "No Comment", 0.50);
        psMetadataItem *itemS8 = psMetadataItemAlloc("s8", PS_DATA_S8, "No Comment", 1);
        psMetadataItem *itemS16 = psMetadataItemAlloc("s16", PS_DATA_S16, "No Comment", 0);
        psMetadataItem *itemS32 = psMetadataItemAlloc("s32", PS_DATA_S32, "No Comment", 1);
        psMetadataItem *itemU8 = psMetadataItemAlloc("u8", PS_DATA_U8, "No Comment", 2);
        psMetadataItem *itemU16 = psMetadataItemAlloc("u16", PS_DATA_U16, "No Comment", 0);
        psMetadataItem *itemU32 = psMetadataItemAlloc("u32", PS_DATA_U32, "No Comment", 1);
        psMetadataItem *itemString1 = psMetadataItemAlloc("String", PS_DATA_STRING, "", "true");
        psMetadataItem *itemString2 = psMetadataItemAlloc("String", PS_DATA_STRING, "", "1.666");
        psMetadataItem *itemString11 = psMetadataItemAlloc("String", PS_DATA_STRING, "", "false");
        psMetadataItem *itemUnsupported = psMetadataItemAlloc("s64", PS_DATA_S64, "", 1);

        //TESTS:  Bool, F32, F64, S8, S16, S32, U8, U16, U32, String
        //BOOL Test
        {
            //Return false for NULL item
            ok ( !psMetadataItemParseBool(NULL),
                 "psMetadataItemParse:      return false in BOOL for NULL psMetadataItem "
                 "input." );
            //Return false for non-supported type S64
            ok ( !psMetadataItemParseBool(itemUnsupported),
                 "psMetadataItemParse:      return false in BOOL for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return true for string with "true"
            ok ( psMetadataItemParseBool(itemString1),
                 "psMetadataItemParse:      return true in BOOL for string-item with "
                 "value 'true'." );
            //Return false for string with "false"
            ok ( !psMetadataItemParseBool(itemString11),
                 "psMetadataItemParse:      return false in BOOL for string-item with "
                 "value 'false'." );
            //Return false for F32 value 0.0
            ok ( !psMetadataItemParseBool(itemF32),
                 "psMetadataItemParse:      return false in BOOL for F32-item with "
                 "value '0.0'." );
            //Return true for F64 value 0.5
            ok ( psMetadataItemParseBool(itemF64),
                 "psMetadataItemParse:      return true in BOOL for F64-item with "
                 "value '0.5'." );
            //Return true for S8 value 1
            ok ( psMetadataItemParseBool(itemS8),
                 "psMetadataItemParse:      return true in BOOL for S8-item with "
                 "value '1'." );
            //Return false for S16 value 0
            ok ( !psMetadataItemParseBool(itemS16),
                 "psMetadataItemParse:      return false in BOOL for S16-item with "
                 "value '0'." );
            //Return true for S32 value 1
            ok ( psMetadataItemParseBool(itemS32),
                 "psMetadataItemParse:      return true in BOOL for S32-item with "
                 "value '1'." );
            //Return true for U8 value 2
            ok ( psMetadataItemParseBool(itemU8),
                 "psMetadataItemParse:     return true in BOOL for U8-item with "
                 "value '2'." );
            //Return false for U16 value 0
            ok ( !psMetadataItemParseBool(itemU16),
                 "psMetadataItemParse:     return false in BOOL for U16-item with "
                 "value '0'." );
            //Return true for U32 value 1
            ok ( psMetadataItemParseBool(itemU32),
                 "psMetadataItemParse:     return true in BOOL for U32-item with "
                 "value '1'." );
        }

        //F32 Test
        {
            //Return NaN for NULL item
            ok ( isnan(psMetadataItemParseF32(NULL)),
                 "psMetadataItemParse:     return NaN for NULL psMetadataItem input." );
            //Return NaN for non-supported type S64
            ok ( isnan(psMetadataItemParseF32(itemUnsupported)),
                 "psMetadataItemParse:     return NaN in F32 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 1.666 for string with "1.666"
            is_float( psMetadataItemParseF32(itemString2), 1.666,
                      "psMetadataItemParse:     return 1.666 in F32 for string-item with "
                      "value '1.666'." );
            //Return 0.0 for F32 value 0.0
            is_float( psMetadataItemParseF32(itemF32), 0.0,
                      "psMetadataItemParse:     return 0.0 in F32 for F32-item with "
                      "value '0.0'." );
            //Return 0.5 for F64 value 0.5
            is_float( psMetadataItemParseF32(itemF64), 0.5,
                      "psMetadataItemParse:     return 0.5 in F32 for F64-item with "
                      "value '0.5'." );
            //Return 1.0 for S8 value 1
            is_float( psMetadataItemParseF32(itemS8), 1.0,
                      "psMetadataItemParse:     return 1.0 in F32 for S8-item with "
                      "value '1'." );
            //Return 0.0 for S16 value 0
            is_float( psMetadataItemParseF32(itemS16), 0.0,
                      "psMetadataItemParse:     return 0.0 in F32 for S16-item with "
                      "value '0'." );
            //Return 1.0 for S32 value 1
            is_float( psMetadataItemParseF32(itemS32), 1.0,
                      "psMetadataItemParse:     return 1.0 in F32 for S32-item with "
                      "value '1'." );
            //Return 2.0 for U8 value 2
            is_float( psMetadataItemParseF32(itemU8), 2.0,
                      "psMetadataItemParse:     return 2.0 in F32 for U8-item with "
                      "value '2'." );
            //Return 0.0 for U16 value 0
            is_float( psMetadataItemParseF32(itemU16), 0.0,
                      "psMetadataItemParse:     return 0.0 in F32 for U16-item with "
                      "value '0'." );
            //Return 1.0 for U32 value 1
            is_float( psMetadataItemParseF32(itemU32), 1.0,
                      "psMetadataItemParse:     return 1.0 in F32 for U32-item with "
                      "value '1'." );
        }

        //F64 Test
        {
            //Return NaN for NULL item
            ok ( isnan(psMetadataItemParseF64(NULL)),
                 "psMetadataItemParse:     return NaN for NULL psMetadataItem input." );
            //Return NaN for non-supported type S64
            ok ( isnan(psMetadataItemParseF64(itemUnsupported)),
                 "psMetadataItemParse:     return NaN in F64 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 1.666 for string with "1.666"
            is_double_tol( psMetadataItemParseF64(itemString2), 1.666, 0.00001,
                           "psMetadataItemParse:     return 1.666 in F64 for string-item with "
                           "value '1.666'." );
            //Return 0.0 for F32 value 0.0
            is_double( psMetadataItemParseF64(itemF32), 0.0,
                       "psMetadataItemParse:     return 0.0 in F64 for F32-item with "
                       "value '0.0'." );
            //Return 0.5 for F64 value 0.5
            is_double( psMetadataItemParseF64(itemF64), 0.5,
                       "psMetadataItemParse:     return 0.5 in F64 for F64-item with "
                       "value '0.5'." );
            //Return 1.0 for S8 value 1
            is_double( psMetadataItemParseF64(itemS8), 1.0,
                       "psMetadataItemParse:     return 1.0 in F64 for S8-item with "
                       "value '1'." );
            //Return 0.0 for S16 value 0
            is_double( psMetadataItemParseF64(itemS16), 0.0,
                       "psMetadataItemParse:     return 0.0 in F64 for S16-item with "
                       "value '0'." );
            //Return 1.0 for S32 value 1
            is_double( psMetadataItemParseF64(itemS32), 1.0,
                       "psMetadataItemParse:     return 1.0 in F64 for S32-item with "
                       "value '1'." );
            //Return 2.0 for U8 value 2
            is_double( psMetadataItemParseF64(itemU8), 2.0,
                       "psMetadataItemParse:     return 2.0 in F64 for U8-item with "
                       "value '2'." );
            //Return 0.0 for U16 value 0
            is_double( psMetadataItemParseF64(itemU16), 0.0,
                       "psMetadataItemParse:     return 0.0 in F64 for U16-item with "
                       "value '0'." );
            //Return 1.0 for U32 value 1
            is_double( psMetadataItemParseF64(itemU32), 1.0,
                       "psMetadataItemParse:     return 1.0 in F64 for U32-item with "
                       "value '1'." );
        }

        //S8 Test
        {
            //Return 0 for NULL item
            ok ( psMetadataItemParseS8(NULL) == 0,
                 "psMetadataItemParse:     return 0 for NULL psMetadataItem input." );
            //Return 0 for non-supported type S64
            ok ( psMetadataItemParseS8(itemUnsupported) == 0,
                 "psMetadataItemParse:     return 0 in S8 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 2 for string with "1.666"
            ok ( psMetadataItemParseS8(itemString2) == 1,
                 "psMetadataItemParse:     return 1 in S8 for string-item with "
                 "value '1.666'." );
            //Return 0 for F32 value 0.0
            ok ( psMetadataItemParseS8(itemF32) == 0,
                 "psMetadataItemParse:     return 0 in S8 for F32-item with "
                 "value '0.0'." );
            //Return 1 for F64 value 0.5
            ok ( psMetadataItemParseS8(itemF64) == 0,
                 "psMetadataItemParse:     return 0 in S8 for F64-item with "
                 "value '0.5'." );
            //Return 1 for S8 value 1
            ok ( psMetadataItemParseS8(itemS8) == 1,
                 "psMetadataItemParse:     return 1 in S8 for S8-item with "
                 "value '1'." );
            //Return 0 for S16 value 0
            ok ( psMetadataItemParseS8(itemS16) == 0,
                 "psMetadataItemParse:     return 0 in S8 for S16-item with "
                 "value '0'." );
            //Return 1 for S32 value 1
            ok ( psMetadataItemParseS8(itemS32) == 1,
                 "psMetadataItemParse:     return 1 in S8 for S32-item with "
                 "value '1'." );
            //Return 2 for U8 value 2
            ok ( psMetadataItemParseS8(itemU8) == 2,
                 "psMetadataItemParse:     return 2 in S8 for U8-item with "
                 "value '2'." );
            //Return 0 for U16 value 0
            ok ( psMetadataItemParseS8(itemU16) == 0,
                 "psMetadataItemParse:     return 0 in S8 for U16-item with "
                 "value '0'." );
            //Return 1 for U32 value 1
            ok ( psMetadataItemParseS8(itemU32) == 1,
                 "psMetadataItemParse:     return 1 in S8 for U32-item with "
                 "value '1'." );
        }

        //S16 Test
        {
            //Return 0 for NULL item
            ok ( psMetadataItemParseS16(NULL) == 0,
                 "psMetadataItemParse:     return 0 for NULL psMetadataItem input." );
            //Return 0 for non-supported type S64
            ok ( psMetadataItemParseS16(itemUnsupported) == 0,
                 "psMetadataItemParse:     return 0 in S16 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 2 for string with "1.666"
            ok ( psMetadataItemParseS16(itemString2) == 1,
                 "psMetadataItemParse:     return 1 in S16 for string-item with "
                 "value '1.666'." );
            //Return 0 for F32 value 0.0
            ok ( psMetadataItemParseS16(itemF32) == 0,
                 "psMetadataItemParse:     return 0 in S16 for F32-item with value '0.0'." );
            //Return 1 for F64 value 0.5
            ok ( psMetadataItemParseS16(itemF64) == 0,
                 "psMetadataItemParse:     return 0 in S16 for F64-item with value '0.5'." );
            //Return 1 for S8 value 1
            ok ( psMetadataItemParseS16(itemS8) == 1,
                 "psMetadataItemParse:     return 1 in S16 for S8-item with value '1'." );
            //Return 0 for S16 value 0
            ok ( psMetadataItemParseS16(itemS16) == 0,
                 "psMetadataItemParse:     return 0 in S16 for S16-item with value '0'." );
            //Return 1 for S32 value 1
            ok ( psMetadataItemParseS16(itemS32) == 1,
                 "psMetadataItemParse:     return 1 in S16 for S32-item with value '1'." );
            //Return 2 for U8 value 2
            ok ( psMetadataItemParseS16(itemU8) == 2,
                 "psMetadataItemParse:     return 2 in S16 for U8-item with value '2'." );
            //Return 0 for U16 value 0
            ok ( psMetadataItemParseS16(itemU16) == 0,
                 "psMetadataItemParse:     return 0 in S16 for U16-item with value '0'." );
            //Return 1 for U32 value 1
            ok ( psMetadataItemParseS16(itemU32) == 1,
                 "psMetadataItemParse:     return 1 in S16 for U32-item with value '1'." );
        }

        //S32 Test
        {
            //Return 0 for NULL item
            ok ( psMetadataItemParseS32(NULL) == 0,
                 "psMetadataItemParse:     return 0 for NULL psMetadataItem input." );
            //Return 0 for non-supported type S64
            ok ( psMetadataItemParseS32(itemUnsupported) == 0,
                 "psMetadataItemParse:     return 0 in S32 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 2 for string with "1.666"
            ok ( psMetadataItemParseS32(itemString2) == 1,
                 "psMetadataItemParse:     return 1 in S32 for string-item with "
                 "value '1.666'." );
            //Return 0 for F32 value 0.0
            ok ( psMetadataItemParseS32(itemF32) == 0,
                 "psMetadataItemParse:     return 0 in S32 for F32-item with value '0.0'." );
            //Return 1 for F64 value 0.5
            ok ( psMetadataItemParseS32(itemF64) == 0,
                 "psMetadataItemParse:     return 0 in S32 for F64-item with value '0.5'." );
            //Return 1 for S8 value 1
            ok ( psMetadataItemParseS32(itemS8) == 1,
                 "psMetadataItemParse:     return 1 in S32 for S8-item with value '1'." );
            //Return 0 for S16 value 0
            ok ( psMetadataItemParseS32(itemS16) == 0,
                 "psMetadataItemParse:     return 0 in S32 for S16-item with value '0'." );
            //Return 1 for S32 value 1
            ok ( psMetadataItemParseS32(itemS32) == 1,
                 "psMetadataItemParse:     return 1 in S32 for S32-item with value '1'." );
            //Return 2 for U8 value 2
            ok ( psMetadataItemParseS32(itemU8) == 2,
                 "psMetadataItemParse:     return 2 in S32 for U8-item with value '2'." );
            //Return 0 for U16 value 0
            ok ( psMetadataItemParseS32(itemU16) == 0,
                 "psMetadataItemParse:     return 0 in S32 for U16-item with value '0'." );
            //Return 1 for U32 value 1
            ok ( psMetadataItemParseS32(itemU32) == 1,
                 "psMetadataItemParse:     return 1 in S32 for U32-item with value '1'." );
        }

        //U8 Test
        {
            //Return 0 for NULL item
            ok ( psMetadataItemParseU8(NULL) == 0,
                 "psMetadataItemParse:     return 0 for NULL psMetadataItem input." );
            //Return 0 for non-supported type S64
            ok ( psMetadataItemParseU8(itemUnsupported) == 0,
                 "psMetadataItemParse:     return 0 in U8 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 2 for string with "1.666"
            ok ( psMetadataItemParseU8(itemString2) == 1,
                 "psMetadataItemParse:     return 1 in U8 for string-item with "
                 "value '1.666'." );
            //Return 0 for F32 value 0.0
            ok ( psMetadataItemParseU8(itemF32) == 0,
                 "psMetadataItemParse:     return 0 in U8 for F32-item with value '0.0'." );
            //Return 1 for F64 value 0.5
            ok ( psMetadataItemParseU8(itemF64) == 0,
                 "psMetadataItemParse:     return 0 in U8 for F64-item with value '0.5'." );
            //Return 1 for S8 value 1
            ok ( psMetadataItemParseU8(itemS8) == 1,
                 "psMetadataItemParse:     return 1 in U8 for S8-item with value '1'." );
            //Return 0 for S16 value 0
            ok ( psMetadataItemParseU8(itemS16) == 0,
                 "psMetadataItemParse:     return 0 in U8 for S16-item with value '0'." );
            //Return 1 for S32 value 1
            ok ( psMetadataItemParseU8(itemS32) == 1,
                 "psMetadataItemParse:     return 1 in U8 for S32-item with value '1'." );
            //Return 2 for U8 value 2
            ok ( psMetadataItemParseU8(itemU8) == 2,
                 "psMetadataItemParse:     return 2 in U8 for U8-item with value '2'." );
            //Return 0 for U16 value 0
            ok ( psMetadataItemParseU8(itemU16) == 0,
                 "psMetadataItemParse:     return 0 in U8 for U16-item with value '0'." );
            //Return 1 for U32 value 1
            ok ( psMetadataItemParseU8(itemU32) == 1,
                 "psMetadataItemParse:     return 1 in U8 for U32-item with value '1'." );
        }

        //U16 Test
        {
            //Return 0 for NULL item
            ok ( psMetadataItemParseU16(NULL) == 0,
                 "psMetadataItemParse:     return 0 for NULL psMetadataItem input." );
            //Return 0 for non-supported type S64
            ok ( psMetadataItemParseU16(itemUnsupported) == 0,
                 "psMetadataItemParse:     return 0 in U16 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 2 for string with "1.666"
            ok ( psMetadataItemParseU16(itemString2) == 1,
                 "psMetadataItemParse:     return 1 in U16 for string-item with "
                 "value '1.666'." );
            //Return 0 for F32 value 0.0
            ok ( psMetadataItemParseU16(itemF32) == 0,
                 "psMetadataItemParse:     return 0 in U16 for F32-item with value '0.0'." );
            //Return 1 for F64 value 0.5
            ok ( psMetadataItemParseU16(itemF64) == 0,
                 "psMetadataItemParse:     return 0 in U16 for F64-item with value '0.5'." );
            //Return 1 for S8 value 1
            ok ( psMetadataItemParseU16(itemS8) == 1,
                 "psMetadataItemParse:     return 1 in U16 for S8-item with value '1'." );
            //Return 0 for S16 value 0
            ok ( psMetadataItemParseU16(itemS16) == 0,
                 "psMetadataItemParse:     return 0 in S8 for S16-item with value '0'." );
            //Return 1 for S32 value 1
            ok ( psMetadataItemParseU16(itemS32) == 1,
                 "psMetadataItemParse:     return 1 in U16 for S32-item with value '1'." );
            //Return 2 for U8 value 2
            ok ( psMetadataItemParseU16(itemU8) == 2,
                 "psMetadataItemParse:     return 2 in U16 for U8-item with value '2'." );
            //Return 0 for U16 value 0
            ok ( psMetadataItemParseU16(itemU16) == 0,
                 "psMetadataItemParse:     return 0 in U16 for U16-item with value '0'." );
            //Return 1 for U32 value 1
            ok ( psMetadataItemParseU16(itemU32) == 1,
                 "psMetadataItemParse:     return 1 in U16 for U32-item with value '1'." );
        }

        //U32 Test
        {
            //Return 0 for NULL item
            ok ( psMetadataItemParseU32(NULL) == 0,
                 "psMetadataItemParse:     return 0 for NULL psMetadataItem input." );
            //Return 0 for non-supported type S64
            ok ( psMetadataItemParseU32(itemUnsupported) == 0,
                 "psMetadataItemParse:     return 0 in U32 for S64 (unsupported) "
                 "psMetadataItem input." );
            //Return 2 for string with "1.666"
            ok ( psMetadataItemParseU32(itemString2) == 1,
                 "psMetadataItemParse:     return 1 in U32 for string-item with "
                 "value '1.666'." );
            //Return 0 for F32 value 0.0
            ok ( psMetadataItemParseU32(itemF32) == 0,
                 "psMetadataItemParse:     return 0 in U32 for F32-item with value '0.0'." );
            //Return 1 for F64 value 0.5
            ok ( psMetadataItemParseU32(itemF64) == 0,
                 "psMetadataItemParse:     return 0 in U32 for F64-item with value '0.5'." );
            //Return 1 for S8 value 1
            ok ( psMetadataItemParseU32(itemS8) == 1,
                 "psMetadataItemParse:     return 1 in U32 for S8-item with value '1'." );
            //Return 0 for S16 value 0
            ok ( psMetadataItemParseU32(itemS16) == 0,
                 "psMetadataItemParse:     return 0 in U32 for S16-item with value '0'." );
            //Return 1 for S32 value 1
            ok ( psMetadataItemParseU32(itemS32) == 1,
                 "psMetadataItemParse:     return 1 in U32 for S32-item with value '1'." );
            //Return 2 for U8 value 2
            ok ( psMetadataItemParseU32(itemU8) == 2,
                 "psMetadataItemParse:     return 2 in U32 for U8-item with value '2'." );
            //Return 0 for U16 value 0
            ok ( psMetadataItemParseU32(itemU16) == 0,
                 "psMetadataItemParse:     return 0 in U32 for U16-item with value '0'." );
            //Return 1 for U32 value 1
            ok ( psMetadataItemParseU32(itemU32) == 1,
                 "psMetadataItemParse:    return 1 in U32 for U32-item with value '1'." );
        }

        //STRING Test
        {
            //Return NULL for NULL item
            ok ( psMetadataItemParseString(NULL) == NULL,
                 "psMetadataItemParse:    return NULL for NULL STRING psMetadataItem "
                 "input." );
            //Return NULL for non-supported type S64
            ok ( psMetadataItemParseString(itemUnsupported) == NULL,
                 "psMetadataItemParse:    return NULL in STRING for S64 (unsupported)"
                 " psMetadataItem input." );
            //Return "true" for STRING of value "true"
            psString tempString = psMetadataItemParseString(itemString1);
            is_strn( tempString, "true", 6,
                     "psMetadataItemParse:    return 'true' in STRING for STRING with "
                     "value 'true'." );
            psFree(tempString);
    
            //Return 0.000000 for F32 of value 0.000000
            tempString = psMetadataItemParseString(itemF32);
            is_strn( tempString, "0.000000", 4,
                     "psMetadataItemParse:    return '0.000000' in STRING for F32 with "
                     "value '0.000000'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 0.500000 for F64 value 0.5
            tempString = psMetadataItemParseString(itemF64);
            is_strn( tempString, "0.500000", 10,
                     "psMetadataItemParse:    return '0.500000' in STRING for F64 with "
                     "value '0.5'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 1 for S8 value 1
            tempString = psMetadataItemParseString(itemS8);
            is_strn( tempString, "1", 3,
                     "psMetadataItemParse:    return '1' in STRING for S8 with "
                     "value '1'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 0 for S16 value 0
            tempString = psMetadataItemParseString(itemS16);
            is_strn( tempString, "0", 3,
                     "psMetadataItemParse:    return '0' in STRING for S8 with "
                     "value '0'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 1 for S32 value 1
            tempString = psMetadataItemParseString(itemS32);
            is_strn( tempString, "1", 3,
                     "psMetadataItemParse:    return '1' in STRING for S8 with "
                     "value '1'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 2 for U8 value 2
            tempString = psMetadataItemParseString(itemU8);
            is_strn( tempString, "2", 3,
                     "psMetadataItemParse:    return '2' in STRING for S8 with "
                     "value '2'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 0 for U16 value 0
            tempString = psMetadataItemParseString(itemU16);
            is_strn( tempString, "0", 3,
                     "psMetadataItemParse:    return '0' in STRING for S8 with "
                     "value '0'.   (actual result=%s)", tempString);
            psFree(tempString);
    
            //Return 1 for U32 value 1
            tempString = psMetadataItemParseString(itemU32);
            is_strn( tempString, "1", 3,
                     "psMetadataItemParse:    return '1' in STRING for S8 with "
                     "value '1'.   (actual result=%s)", tempString);
            psFree(tempString);
        }

        psFree(itemBool);
        psFree(itemF32);
        psFree(itemF64);
        psFree(itemS8);
        psFree(itemS16);
        psFree(itemS32);
        psFree(itemU8);
        psFree(itemU16);
        psFree(itemU32);
        psFree(itemString1);
        psFree(itemString2);
        psFree(itemString11);
        psFree(itemUnsupported);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}


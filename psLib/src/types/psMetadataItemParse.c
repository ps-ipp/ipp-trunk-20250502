#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "psMemory.h"
#include "psAssert.h"
#include "psError.h"
#include "psMetadata.h"
#include "psMetadataItemParse.h"

# define PS_METADATA_ITEM_PARSE_NUMBER(INTYPE,OUTTYPE) \
case PS_DATA_##INTYPE: \
return (ps##OUTTYPE)(item->data.INTYPE); \


// NOTE: This function flows through so that errors may be handled by the "default" case.
#define PS_METADATA_ITEM_PARSE_STRING_FLOAT(OUTTYPE,FUNCTION) \
case PS_DATA_STRING: { \
    char *end = NULL; \
    ps##OUTTYPE number = FUNCTION(item->data.V, &end); \
    if (end != item->data.V) { \
        return number; \
    } \
}

// NOTE: This function flows through so that errors may be handled by the "default" case.
#define PS_METADATA_ITEM_PARSE_STRING_INT(OUTTYPE,FUNCTION) \
case PS_DATA_STRING: { \
    char *end = NULL; \
    ps##OUTTYPE number = FUNCTION(item->data.V, &end, 10); \
    if (end != item->data.V) { \
        return number; \
    } \
}

bool psMetadataItemParseBool(const psMetadataItem *item
                              )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, false);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, BOOL);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, BOOL);
    case PS_DATA_BOOL:
        return item->data.B;
    case PS_DATA_STRING:
        if (strcasecmp(item->data.V, "true") == 0) {
            return true;
        } else if (strcasecmp(item->data.V, "false") == 0) {
            return false;
        }
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of boolean type (%x) --- "
                "treating as false.\n", item->name, item->comment, item->type);
        return false;
    }
}

psF32 psMetadataItemParseF32(const psMetadataItem *item
                            )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, NAN);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, F32);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, F32);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  F32);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, F32);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, F32);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  F32);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, F32);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, F32);
        PS_METADATA_ITEM_PARSE_STRING_FLOAT(F32, strtof);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of floating point type (%x) --- "
                "treating as NaN.\n", item->name, item->comment, item->type);
        return NAN;
    }
}

psF64 psMetadataItemParseF64(const psMetadataItem *item
                            )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, NAN);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, F64);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, F64);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  F64);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, F64);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, F64);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  F64);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, F64);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, F64);
        PS_METADATA_ITEM_PARSE_STRING_FLOAT(F32, strtod);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of double-precision floating "
                "point type (%x) --- treating as NaN.\n", item->name, item->comment, item->type);
        return NAN;
    }
}

psU8 psMetadataItemParseU8(const psMetadataItem *item
                          )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, 0);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, U8);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, U8);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  U8);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, U8);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, U8);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  U8);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, U8);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, U8);
        PS_METADATA_ITEM_PARSE_STRING_INT(U8,strtoul);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of integer type (%x) --- "
                "treating as zero.\n", item->name, item->comment, item->type);
        return 0;
    }
}

psU16 psMetadataItemParseU16(const psMetadataItem *item
                            )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, 0);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, U16);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, U16);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  U16);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, U16);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, U16);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  U16);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, U16);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, U16);
        PS_METADATA_ITEM_PARSE_STRING_INT(U16,strtoul);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of integer type (%x) --- "
                "treating as zero.\n", item->name, item->comment, item->type);
        return 0;
    }
}

psU32 psMetadataItemParseU32(const psMetadataItem *item
                            )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, 0);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, U32);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, U32);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  U32);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, U32);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, U32);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  U32);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, U32);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, U32);
        PS_METADATA_ITEM_PARSE_STRING_INT(U32,strtoul);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of integer type (%x) --- "
                "treating as zero.\n", item->name, item->comment, item->type);
        return 0;
    }
}

psS8 psMetadataItemParseS8(const psMetadataItem *item
                          )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, 0);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, S8);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, S8);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  S8);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, S8);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, S8);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  S8);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, S8);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, S8);
        PS_METADATA_ITEM_PARSE_STRING_INT(S8,strtol);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of integer type (%x) --- "
                "treating as zero.\n", item->name, item->comment, item->type);
        return 0;
    }
}

psS16 psMetadataItemParseS16(const psMetadataItem *item
                            )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, 0);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, S16);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, S16);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  S16);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, S16);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, S16);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  S16);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, S16);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, S16);
        PS_METADATA_ITEM_PARSE_STRING_INT(S16,strtol);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of integer type (%x) --- "
                "treating as zero.\n", item->name, item->comment, item->type);
        return 0;
    }
}

psS32 psMetadataItemParseS32(const psMetadataItem *item
                            )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, 0);

    switch (item->type) {
        PS_METADATA_ITEM_PARSE_NUMBER(F32, S32);
        PS_METADATA_ITEM_PARSE_NUMBER(F64, S32);
        PS_METADATA_ITEM_PARSE_NUMBER(S8,  S32);
        PS_METADATA_ITEM_PARSE_NUMBER(S16, S32);
        PS_METADATA_ITEM_PARSE_NUMBER(S32, S32);
        PS_METADATA_ITEM_PARSE_NUMBER(U8,  S32);
        PS_METADATA_ITEM_PARSE_NUMBER(U16, S32);
        PS_METADATA_ITEM_PARSE_NUMBER(U32, S32);
        PS_METADATA_ITEM_PARSE_STRING_INT(S32,strtol);
        // Flow through
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Item %s (%s) is not of integer type (%x) --- "
                "treating as zero.\n", item->name, item->comment, item->type);
        return 0;
    }
}

# define PS_METADATA_ITEM_PARSE_STRING(TYPE,MODE) \
case PS_DATA_##TYPE: { \
    psString value = NULL; \
    psStringAppend(&value, MODE, item->data.TYPE); \
    return value; } \

psString psMetadataItemParseString(const psMetadataItem *item
                                  )
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, NULL);

    switch (item->type) {
    case PS_DATA_STRING:
        return psMemIncrRefCounter(item->data.V);

        PS_METADATA_ITEM_PARSE_STRING(F32, "%f");
        PS_METADATA_ITEM_PARSE_STRING(F64, "%f");
        PS_METADATA_ITEM_PARSE_STRING(S8,  "%d");
        PS_METADATA_ITEM_PARSE_STRING(S16, "%d");
        PS_METADATA_ITEM_PARSE_STRING(S32, "%d");
        PS_METADATA_ITEM_PARSE_STRING(U8,  "%d");
        PS_METADATA_ITEM_PARSE_STRING(U16, "%d");
        PS_METADATA_ITEM_PARSE_STRING(U32, "%d");
      case PS_DATA_TIME:
        return psTimeToISO(item->data.V);

    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Item %s (%s) is not of string type (%x) --- treating as "
                "undefined.\n", item->name, item->comment, item->type);
        //        return psStringCopy("");
        return NULL;
    }
}


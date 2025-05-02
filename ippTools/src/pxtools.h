/*
 * pxtools.h
 *
 * Copyright (C) 2006  Joshua Hoblitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PXTOOLS_H
#define PXTOOLS_H 1

#include <stdio.h>
#include <string.h>   // for strcmp and strncmp
#include <strings.h>  // for strcasecmp
#include <unistd.h>   // for unlink

// #include <stdlib.h>
// #include <stdint.h>
// #include <inttypes.h>

#include <pslib.h>
#include <psmodules.h>
#include <ippdb.h>

#include "pxconfig.h"
#include "pxtoolsErrorCodes.h"

#include "pxadd.h"
#include "pxcam.h"
#include "pxchip.h"
#include "pxdata.h"
#include "pxfake.h"
#include "pxwarp.h"
#include "pxregister.h"
#include "pxtag.h"
#include "pxtree.h"
#include "pxmagic.h"
#include "pxspace.h"

# define MAX_ROWS 10e9
# define PXTOOL_MODE_NONE 0x0
// we do not revert or update components with this fault value
// This is the same value as PSTAMP_GONE
# define PXTOOL_DO_NOT_REVERT_FAULT 26

bool pxIsValidState(const char *state);
bool pxIsValidCleanedState(const char *state);
psString pxMergeCodeVersions(psString version1, psString version2);
bool pxCoalesceRunStatus(pxConfig *config, const psString dbQFile, psS64 stage_id, psString *software_ver,
                         psS64 *maskfrac_npix, psF32 *maskfrac_static, psF32 *maskfrac_dynamic,
                         psF32 *maskfrac_magic, psF32 *maskfrac_advisory);
bool pxSetRunSoftware(pxConfig *config, const psString tableName, const psString stage_id_name, const psS64 stage_id,
                      psString software_ver);
bool pxSetRunMaskfrac(pxConfig *config, const psString tableName, const psString stage_id_name, const psS64 stage_id,
                      psS64 maskfrac_npix, psF32 maskfrac_static, psF32 maskfrac_dynamic,
                      psF32 maskfrac_magic, psF32 maskfrac_advisory);
bool pxCamSetRunMaskfrac(pxConfig *config, const psString tableName, const psString stage_id_name, const psS64 stage_id,
                         psS64 maskfrac_ref_npix, psF32 maskfrac_ref_static, psF32 maskfrac_ref_dynamic,
                         psF32 maskfrac_ref_magic, psF32 maskfrac_ref_advisory,
                         psS64 maskfrac_max_npix, psF32 maskfrac_max_static, psF32 maskfrac_max_dynamic,
                         psF32 maskfrac_max_magic, psF32 maskfrac_max_advisory);

bool pxSetStateCleaned(const char *tableName, const char *columnName, psArray *rows);
bool pxAddLabelSearchArgs (pxConfig *config, psMetadata *where, char *field, char *name, char *op);

bool pxSetFaultCode(psDB *dbh, const char *tableName, psMetadata *where, psS16 code, psS16 quality);
bool pxExportVersion(pxConfig *config, FILE *f);
bool pxCheckImportVersion(pxConfig *config, psMetadata *md);

psExit pxerrorGetExitStatus(void);

void pxUsage(FILE *stream, int argc, char **argv, const char *modeName, psMetadata *argSet);
bool pxGetOptions(FILE *stream, int argc, char **argv, pxConfig *config, psMetadata *modes, psMetadata *argSets);

bool pxUpdateRun(pxConfig *config, psMetadata *where, psString *pQuery, psString runTable, psString idColumn, psString fileTable, bool has_dist_group, bool has_magicked);

#define PXOPT_ADD_MODE(option, comment, modeval, argset) \
{ \
    if (!psMetadataAddMetadata(argSets, PS_LIST_TAIL, option, 0, comment, argset)) {;\
        psError(PS_ERR_UNKNOWN, false, "failed to add argset for %s", option); \
    } \
    psFree(argset); \
\
    if (!psMetadataAddU32(modes, PS_LIST_TAIL, option, 0, comment, modeval)) {;\
        psError(PS_ERR_UNKNOWN, false, "failed to add argset for %s", option); \
    } \
}

#define PXOPT_LOOKUP_STR(var, md, key, required, ret) \
psString var; \
{ \
    bool status; \
 \
    var = psMetadataLookupStr(&status, md, key); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", key); \
        return ret; \
    } \
 \
    if (required && (!var)) { \
        psError(PS_ERR_UNKNOWN, true, "%s is required", key); \
        return ret; \
    } \
}

#define PXOPT_LOOKUP_F(var, md, key, type, required, ret) \
ps##type var; \
{ \
    bool status; \
 \
    var = psMetadataLookup##type(&status, md, key); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", key); \
        return ret; \
    } \
 \
    if (required && isnan(var)) { \
        psError(PS_ERR_UNKNOWN, true, "%s is required", key); \
        return ret; \
    } \
}

#define PXOPT_LOOKUP_F32(var, md, key, required, ret) \
    PXOPT_LOOKUP_F(var, md, key, F64, required, ret)

#define PXOPT_LOOKUP_F64(var, md, key, required, ret) \
    PXOPT_LOOKUP_F(var, md, key, F64, required, ret)

#define PXOPT_LOOKUP_PRIMITIVE(var, md, key, type, suffix, max, required, ret) \
type var; \
{ \
    psMetadataItem *item = psMetadataLookup(md, key); \
    if (!item) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", key); \
        return ret; \
    } \
    psAssert(item->comment, "metadata item comment should be defined"); \
    if (required && (!psStrcasestr(item->comment, "(found)"))) { \
        psError(PS_ERR_UNKNOWN, true, "%s is required", key); \
        return ret; \
    } \
 \
    var = item->data.suffix; \
 \
}

#define PXOPT_LOOKUP_S16(var, md, key, required, ret) \
    PXOPT_LOOKUP_PRIMITIVE(var, md, key, psS16, S16, INT16_MAX, required, ret)

#define PXOPT_LOOKUP_S32(var, md, key, required, ret) \
    PXOPT_LOOKUP_PRIMITIVE(var, md, key, psS32, S32, INT32_MAX, required, ret)

#define PXOPT_LOOKUP_S64(var, md, key, required, ret) \
    PXOPT_LOOKUP_PRIMITIVE(var, md, key, psS64, S64, INT64_MAX, required, ret)

#define PXOPT_LOOKUP_U16(var, md, key, required, ret) \
    PXOPT_LOOKUP_PRIMITIVE(var, md, key, psU16, U16, UINT16_MAX, required, ret)

#define PXOPT_LOOKUP_U32(var, md, key, required, ret) \
    PXOPT_LOOKUP_PRIMITIVE(var, md, key, psU32, U32, UINT32_MAX, required, ret)

#define PXOPT_LOOKUP_U64(var, md, key, required, ret) \
    PXOPT_LOOKUP_PRIMITIVE(var, md, key, psU64, U64, UINT64_MAX, required, ret)

#define PXOPT_LOOKUP_TIME(var, md, key, required, ret) \
psTime *var; \
{ \
    bool status; \
 \
    var = psMetadataLookupTime(&status, md, key); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", key); \
        return ret; \
    } \
 \
    if (required && (!var)) { \
        psError(PS_ERR_UNKNOWN, true, "%s is required", key); \
        return ret; \
    } \
}

#define PXOPT_LOOKUP_BOOL(var, md, key, ret) \
bool var; \
{ \
    bool status; \
 \
    var = psMetadataLookupBool(&status, md, key); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", key); \
        return ret; \
    } \
}

// XXX the PXOPT_COPY_* macros free 'to' on error

#define PXOPT_COPY_PRIMITIVE(from, to, type, suffix, oldname, newname, newcomment) \
    {									\
      psMetadataItem *item = psMetadataLookup(from, oldname);		\
      if (!item) {							\
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", oldname); \
        return false;							\
      }									\
      psAssert(item->comment, "metadata item comment should be defined"); \
									\
      if (psStrcasestr(item->comment, "(found)")) {			\
        if (!psMetadataAdd##suffix(to, PS_LIST_TAIL, newname, PS_META_DUPLICATE_OK, newcomment, item->data.suffix)) { \
	  psError(PS_ERR_UNKNOWN, false, "failed to add item " newname); \
	  psFree(to);							\
	  return false;							\
        }								\
      }									\
    }

#define PXOPT_COPY_V(from, to, type, suffix, oldname, newname, comment) \
{ \
    bool status = false; \
    type var = psMetadataLookup##suffix(&status, from, oldname); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for " oldname); \
        return false; \
    } \
    if (var) { \
        if (!psMetadataAdd##suffix(to, PS_LIST_TAIL, newname, PS_META_DUPLICATE_OK, comment, var)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " newname); \
            psFree(to); \
            return false; \
        } \
    }\
}

#define PXOPT_COPY_F(from, to, type, oldname, newname, comment) \
{ \
    bool status = false; \
    ps##type var = psMetadataLookup##type(&status, from, oldname); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for " oldname); \
        return false; \
    } \
    if (!isnan(var)) { \
        if (!psMetadataAdd##type(to, PS_LIST_TAIL, newname, PS_META_DUPLICATE_OK, comment, var)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " newname); \
            psFree(to); \
            return false; \
        } \
    } \
}

// convert the supplied value from degrees (external) to radians (internal)
#define PXOPT_COPY_RADEC(from, to, oldname, newname, comment) \
{ \
    bool status = false; \
    psF64 var = psMetadataLookupF64(&status, from, oldname); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for " oldname); \
        return false; \
    } \
    if (!isnan(var)) { \
        if (!psMetadataAddF64(to, PS_LIST_TAIL, newname, PS_META_DUPLICATE_OK, comment, PS_RAD_DEG*var)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " newname); \
            psFree(to); \
            return false; \
        } \
    } \
}

#define PXOPT_COPY_F32(from, to, oldname, newname, comment) \
  PXOPT_COPY_F(from, to, F32, oldname, newname, comment)

#define PXOPT_COPY_F64(from, to, oldname, newname, comment) \
  PXOPT_COPY_F(from, to, F64, oldname, newname, comment)

#define PXOPT_COPY_TIME(from, to, oldname, newname, comment) \
  PXOPT_COPY_V(from, to, psTime *, Time, oldname, newname, comment)

#define PXOPT_COPY_STR(from, to, oldname, newname, comment) \
  PXOPT_COPY_V(from, to, psString, Str, oldname, newname, comment)

#define PXOPT_COPY_S16(from, to, oldname, newname, comment) \
  PXOPT_COPY_PRIMITIVE(from, to, psS16, S16, oldname, newname, comment)

#define PXOPT_COPY_S32(from, to, oldname, newname, comment) \
  PXOPT_COPY_PRIMITIVE(from, to, psS32, S32, oldname, newname, comment)

#define PXOPT_COPY_S64(from, to, oldname, newname, comment) \
  PXOPT_COPY_PRIMITIVE(from, to, psS64, S64, oldname, newname, comment)

#define PXOPT_COPY_U16(from, to, oldname, newname, comment) \
  PXOPT_COPY_PRIMITIVE(from, to, psU16, U16, oldname, newname, comment)

#define PXOPT_COPY_U32(from, to, oldname, newname, comment) \
  PXOPT_COPY_PRIMITIVE(from, to, psU32, U32, oldname, newname, comment)

#define PXOPT_COPY_U64(from, to, oldname, newname, comment) \
  PXOPT_COPY_PRIMITIVE(from, to, psU64, U64, oldname, newname, comment)

#define PXOPT_COPY_BOOL(from, to, oldname, newname, comment) \
  PXOPT_COPY_V(from, to, psBool, Bool, oldname, newname, comment)

/*** these PXOPT_ADD_WHERE macros are used to construct the default sql elements ***/

#define PXOPT_ADD_WHERE_STR(name) \
{ \
    psString str = NULL; \
    bool status = false; \
    if ((str = psMetadataLookupStr(&status, config->args, "-" #name))) { \
        if (!psMetadataAddStr(config->where, PS_LIST_TAIL, #name, 0, "==", str)) {\
            psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_STR_ALIAS(name,realname) \
{ \
    psString str = NULL; \
    bool status = false; \
    if ((str = psMetadataLookupStr(&status, config->args, name))) { \
        if (!psMetadataAddStr(config->where, PS_LIST_TAIL, realname, 0, "==", str)) {\
            psError(PS_ERR_UNKNOWN, false, "failed to add item %s", name); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_S16(name) \
{ \
    psS16 s16 = 0; \
    bool status = false; \
    if ((s16= psMetadataLookupS16(&status, config->args, "-" #name))) { \
        if (!psMetadataAddS16(config->where, PS_LIST_TAIL, #name, 0, "==", s16)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_BOOL(name) \
{ \
    bool value = false;  \
    bool status = false; \
    if ((value = psMetadataLookupBool(&status, config->args, "-" #name))) { \
        if (!psMetadataAddBool(config->where, PS_LIST_TAIL, #name, 0, "==", value)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_S32(name) \
{ \
    psS32 s32 = 0; \
    bool status = false; \
    if ((s32= psMetadataLookupS32(&status, config->args, "-" #name))) { \
        if (!psMetadataAddS32(config->where, PS_LIST_TAIL, #name, 0, "==", s32)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_S64(name) \
{ \
    psS64 s64 = 0; \
    bool status = false; \
    if ((s64= psMetadataLookupS64(&status, config->args, "-" #name))) { \
        if (!psMetadataAddS64(config->where, PS_LIST_TAIL, #name, 0, "==", s64)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_F32(name) \
{ \
    psF32 var = 0; \
    bool status = false; \
    if ((var = psMetadataLookupF32(&status, config->args, "-" #name))) { \
        if (!isnan(var)) { \
            if (!psMetadataAddF32(config->where, PS_LIST_TAIL, #name, 0, "==", var)) { \
                psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
                psFree(config); \
                return NULL; \
            } \
        } \
    } \
}

#define PXOPT_ADD_WHERE_F64(name) \
{ \
    psF64 var = 0; \
    bool status = false; \
    if ((var = psMetadataLookupF64(&status, config->args, "-" #name))) { \
        if (!isnan(var))  { \
            if (!psMetadataAddF64(config->where, PS_LIST_TAIL, #name, 0, "==", var)) { \
                psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
                psFree(config); \
                return NULL; \
            } \
        } \
    } \
}

#define PXOPT_ADD_WHERE_BOOL_ALIAS(flag,name)   \
{ \
    bool value = 0; \
    bool status = false; \
    if ((value = psMetadataLookupBool(&status, config->args, flag))) {  \
        if (!psMetadataAddBOOL(config->where, PS_LIST_TAIL, name, 0, "==", value)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item %s", flag); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_S16_ALIAS(flag,name)    \
{ \
    psS16 s16 = 0; \
    bool status = false; \
    if ((s16= psMetadataLookupS16(&status, config->args, flag))) {      \
        if (!psMetadataAddS16(config->where, PS_LIST_TAIL, name, 0, "==", s16)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item %s", flag); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_S32_ALIAS(flag,name)    \
{ \
    psS32 s32 = 0; \
    bool status = false; \
    if ((s32= psMetadataLookupS32(&status, config->args, flag))) {      \
        if (!psMetadataAddS32(config->where, PS_LIST_TAIL, name, 0, "==", s32)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item %s", flag); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_S64_ALIAS(flag,name)    \
{ \
    psS64 s64 = 0; \
    bool status = false; \
    if ((s64= psMetadataLookupS64(&status, config->args, flag))) {      \
        if (!psMetadataAddS64(config->where, PS_LIST_TAIL, name, 0, "==", s64)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item %s", flag); \
            psFree(config); \
            return NULL; \
        } \
    } \
}

#define PXOPT_ADD_WHERE_F32_ALIAS(flag,name)    \
{ \
    psF32 var = 0; \
    bool status = false; \
    if ((var = psMetadataLookupF32(&status, config->args, flag))) {     \
        if (!isnan(var))  { \
            if (!psMetadataAddF32(config->where, PS_LIST_TAIL, name, 0, "==", var)) { \
                psError(PS_ERR_UNKNOWN, false, "failed to add item %s", flag); \
                psFree(config); \
                return NULL; \
            } \
        } \
    } \
}

#define PXOPT_ADD_WHERE_F64_ALIAS(flag,name)    \
{ \
    psF64 var = 0; \
    bool status = false; \
    if ((var = psMetadataLookupF64(&status, config->args, flag))) {     \
        if (!isnan(var))  { \
            if (!psMetadataAddF64(config->where, PS_LIST_TAIL, name, 0, "==", var)) { \
                psError(PS_ERR_UNKNOWN, false, "failed to add item %s", flag); \
                psFree(config); \
                return NULL; \
            } \
        } \
    } \
}

#define PXOPT_ADD_WHERE_TIME_STR(name) \
{ \
    psString str = NULL; \
    bool status = false; \
    if ((str = psMetadataLookupStr(&status, config->args, "-" #name))) { \
        psTime *time = psTimeFromISO(str, PS_TIME_UTC); \
        if (!time) { \
            psError(PS_ERR_UNKNOWN, false, "failed to convert " #name " into a psTime object"); \
            psFree(config); \
            return NULL; \
        } \
        psMetadataItem *item = psMetadataLookup(config->args, "-" #name); \
        if (item) { \
            str = item->comment; \
        } else { \
            str = NULL; \
        } \
        if (!psMetadataAddTime(config->where, PS_LIST_TAIL, #name, 0, str, time)) {\
            psError(PS_ERR_UNKNOWN, false, "failed to add item " #name); \
            psFree(config); \
            return NULL; \
        } \
        psFree(time); \
    } \
}


/// Add a primary item to the mirror database
///
/// ITEM: Item to add (if it is of the correct type)
/// NAME: Name for table, to match item name
/// ROWTYPE: Type of the row (type returned by PARSEFUNC)
/// PARSEFUNC: Function to parse ITEM and return a ROWTYPE
/// IDENTIFIERS: Vector (of type U64) to store identifiers
/// IDNAME: Element of the row with U64 identifier
/// INSERTFUNC: Function to insert a row into the database
/// DATABASE: Database handle
/// CLEANUP: Operation(s) to perform to cleanup in case of an error
#define PXMIRROR_PRIMARY(ITEM, NAME, ROWTYPE, PARSEFUNC, IDENTIFIERS, IDNAME, INSERTFUNC, DATABASE, CLEANUP) \
    if (strcmp((ITEM)->name, NAME) == 0) { \
        if ((ITEM)->type != PS_DATA_METADATA) { \
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Entry %s is not of type METADATA", (ITEM)->name); \
            CLEANUP; \
            return false; \
        } \
        ROWTYPE *row = PARSEFUNC((ITEM)->data.md); /* Row to insert */ \
        if (!row) { \
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate %s row from metadata", (ITEM)->name); \
            CLEANUP; \
            return false; \
        } \
        if (!INSERTFUNC(DATABASE, row)) { \
            psError(PS_ERR_UNKNOWN, false, "Unable to add %s %" PRIu64, (ITEM)->name, row->IDNAME); \
            psFree(row); \
            CLEANUP; \
            return false; \
        } \
        psVectorAppend((IDENTIFIERS), row->IDNAME); \
        psFree(row); \
    }

/// Add a dependent item to the mirror database
///
/// ITEM: Item to add (if it is of the correct type)
/// NAME: Name for table, to match item name
/// ROWTYPE: Type of the row (type returned by PARSEFUNC)
/// PARSEFUNC: Function to parse ITEM and return a ROWTYPE
/// IDENTIFIERS: Vector (of type U64) to check identifiers
/// IDNAME: Element of the row with U64 identifier
/// INSERTFUNC: Function to insert a row into the database
/// DATABASE: Database handle
/// CLEANUP: Operation(s) to perform to cleanup in case of an error
#define PXMIRROR_OTHER(ITEM, NAME, ROWTYPE, PARSEFUNC, IDENTIFIERS, IDNAME, INSERTFUNC, DATABASE, CLEANUP) \
    if (strcmp((ITEM)->name, NAME) == 0) { \
        if ((ITEM)->type != PS_DATA_METADATA) { \
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Entry %s is not of type METADATA", (ITEM)->name); \
            CLEANUP; \
            return false; \
        } \
        ROWTYPE *row = PARSEFUNC(item->data.md); /* Row to insert */ \
        if (!row) { \
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate %s row from metadata", (ITEM)->name); \
            CLEANUP; \
            return false; \
        } \
        bool found = false;         /* Found the identifier? */ \
        for (int i = 0; i < (IDENTIFIERS)->n && !found; i++) { \
            if (row->IDNAME == (IDENTIFIERS)->data.U64[i]) { \
                found = true; \
                break; \
            } \
        } \
        if (!found) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Identifier not found for %s %" PRIu64, \
                    (ITEM)->name, row->IDNAME); \
            psFree(row); \
            CLEANUP; \
            return false; \
        } \
        if (!INSERTFUNC(DATABASE, row)) { \
            psError(PS_ERR_UNKNOWN, false, "Unable to add %s %" PRIu64, (ITEM)->name, row->IDNAME); \
            psFree(row); \
            CLEANUP; \
            return false; \
        } \
        psFree(row); \
    }

#endif // PXTOOLS_H

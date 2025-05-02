/*
 * pxtool.c
 *
 * Copyright (C) 2008  Joshua Hoblitt
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "pxtools.h"

bool pxIsValidState(const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    if (!strcmp(state, "new")) return true;
    if (!strcmp(state, "reg")) return true;
    if (!strcmp(state, "full")) return true;
    if (!strcmp(state, "drop")) return true;
    if (!strcmp(state, "wait")) return true;
    if (!strcmp(state, "keep")) return true;
    if (!strcmp(state, "ignore")) return true;
    if (!strcmp(state, "goto_cleaned")) return true;
    if (!strcmp(state, "error_cleaned")) return true;
    if (!strcmp(state, "goto_purged")) return true;
    if (!strcmp(state, "error_purged")) return true;
    if (!strcmp(state, "goto_scrubbed")) return true;
    if (!strcmp(state, "error_scrubbed")) return true;
    if (!strcmp(state, "cleaned")) return true;
    if (!strcmp(state, "update")) return true;
    if (!strcmp(state, "purged")) return true;
    if (!strcmp(state, "scrubbed")) return true;
    return false;
}

bool pxIsValidCleanedState(const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    if (!strcmp(state, "cleaned")) return true;
    if (!strcmp(state, "purged")) return true;
    if (!strcmp(state, "scrubbed")) return true;
    return false;
}

psString pxMergeCodeVersions(psString version1, psString version2)
{
    if (!version1 && !version2) {
        return NULL;
    }

    if (!version1) {
        return psStringCopy(version2);
    }
    if (!version2) {
        return psStringCopy(version1);
    }

    bool mod1 = false, mod2 = false;    // Modified versions?
    if (strchr(version1, 'M')) {
        psStringSubstitute(&version1, "M", "");
        mod1 = true;
    }
    if (strchr(version2, 'M')) {
        psStringSubstitute(&version2, "M", "");
        mod2 = true;
    }

    int num1 = strtol(version1, NULL, 10);
    int num2 = strtol(version2, NULL, 10);
    int numO = PS_MAX(num1, num2);

    psString out = NULL;
    psStringAppend(&out, "%" PRId32, numO);
    if (mod1 || mod2) {
        psStringAppend(&out, "M");
    }
    return out;
}

bool pxCoalesceRunStatus(pxConfig *config, const psString dbQFile, psS64 stage_id, psString *software_ver,
                         psS64 *maskfrac_npix, psF32 *maskfrac_static, psF32 *maskfrac_dynamic,
                         psF32 *maskfrac_magic, psF32 *maskfrac_advisory)
{
    psString query = pxDataGet(dbQFile);
    if (!query) {
        psError(psErrorCodeLast(), false, "Unable to read query");
        return false;
    }
    if (!p_psDBRunQueryF(config->dbh, query, stage_id)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    *maskfrac_npix = 0;
    *maskfrac_static = 0.0;
    *maskfrac_dynamic = 0.0;
    *maskfrac_magic = 0.0;
    *maskfrac_advisory = 0.0;

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

        psS32 this_npix = psMetadataLookupS32(NULL, row, "maskfrac_npix");
        psF32 this_static = psMetadataLookupF32(NULL, row, "maskfrac_static");
        psF32 this_dynamic = psMetadataLookupF32(NULL, row, "maskfrac_dynamic");
        psF32 this_magic = psMetadataLookupF32(NULL, row, "maskfrac_magic");
        psF32 this_advisory = psMetadataLookupF32(NULL, row, "maskfrac_advisory");

        psTrace("pxtools", 3, "Mask stats: %d %f %f %f %f\n",
                this_npix, this_static, this_dynamic, this_magic, this_advisory);

        if (this_npix == 0 || this_npix == PS_MAX_S32) {
            continue;
        }
        if (!isfinite(this_static) || !isfinite(this_dynamic) ||
            !isfinite(this_magic) || !isfinite(this_advisory)) {
            continue;
        }

        psString this_version = psMetadataLookupStr(NULL, row, "software_ver");
        *software_ver = pxMergeCodeVersions(*software_ver,this_version);

        *maskfrac_npix += this_npix;
        *maskfrac_static += this_npix * this_static;
        *maskfrac_dynamic += this_npix * this_dynamic;
        *maskfrac_magic += this_npix * this_magic;
        *maskfrac_advisory += this_npix * this_advisory;
    }
    psFree(output);

    if (*maskfrac_npix > 0) {
        *maskfrac_static /= *maskfrac_npix;
        *maskfrac_dynamic /= *maskfrac_npix;
        *maskfrac_magic /= *maskfrac_npix;
        *maskfrac_advisory /= *maskfrac_npix;
    }

    return true;
}

bool pxSetRunSoftware(pxConfig *config, const psString tableName, const psString stage_id_name,
                      const psS64 stage_id, psString software_ver)
{
    char *query = "UPDATE %s SET software_ver = '%s' WHERE %s = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, tableName, software_ver, stage_id_name, stage_id)) {
        psError(psErrorCodeLast(), false,
                "failed to set software version for %s %" PRId64, stage_id_name, stage_id);
        return false;
    }

    return true;
}

bool pxSetRunMaskfrac(pxConfig *config, const psString tableName, const psString stage_id_name,
                      const psS64 stage_id, psS64 maskfrac_npix, psF32 maskfrac_static,
                      psF32 maskfrac_dynamic, psF32 maskfrac_magic, psF32 maskfrac_advisory)
{
    char *query = "UPDATE %s SET maskfrac_npix = %f, maskfrac_static = %f, maskfrac_dynamic = %f, "
        "maskfrac_magic = %f, maskfrac_advisory = %f WHERE %s = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, tableName, (float)maskfrac_npix, maskfrac_static,
                         maskfrac_dynamic, maskfrac_magic, maskfrac_advisory, stage_id_name, stage_id)) {
        psError(psErrorCodeLast(), false,
                "failed to set maskfrac stats for %s %" PRId64,stage_id_name,stage_id);
        return false;
    }



    return true;
}

bool pxCamSetRunMaskfrac(pxConfig *config, const psString tableName,
                         const psString stage_id_name, const psS64 stage_id,
                         psS64 maskfrac_ref_npix, psF32 maskfrac_ref_static, psF32 maskfrac_ref_dynamic,
                         psF32 maskfrac_ref_magic, psF32 maskfrac_ref_advisory,
                         psS64 maskfrac_max_npix, psF32 maskfrac_max_static, psF32 maskfrac_max_dynamic,
                         psF32 maskfrac_max_magic, psF32 maskfrac_max_advisory) {
  char *query = "UPDATE %s SET maskfrac_ref_npix = %f, maskfrac_ref_static = %f, maskfrac_ref_dynamic = %f, "
      "maskfrac_ref_magic = %f, maskfrac_ref_advisory = %f, maskfrac_max_npix = %f, maskfrac_max_static = %f, "
      "maskfrac_max_dynamic = %f, maskfrac_max_magic = %f, maskfrac_max_advisory = %f WHERE %s = %" PRId64;
  if (!p_psDBRunQueryF(config->dbh, query, tableName, (float)maskfrac_ref_npix, maskfrac_ref_static,
                       maskfrac_ref_dynamic,  maskfrac_ref_magic, maskfrac_ref_advisory,
                       (float)maskfrac_max_npix, maskfrac_max_static,
                       maskfrac_max_dynamic,  maskfrac_max_magic, maskfrac_max_advisory,
                       stage_id_name, stage_id)) {
      psError(psErrorCodeLast(), false,
              "failed to set maskfrac stats for %s %" PRId64,stage_id_name,stage_id);
      return false;
  }

  return true;
}

// 'scrubbed' is no longer a virtual state equivalent to cleaned, but allows files to be removed
// even if the config files is missing.  This change was prompted as files that are cleaned can
// be regenerated, but that is not certain after being scrubbed.


// change the value for tableName.columName from 'full' to 'cleaned' if necessary
bool pxSetStateCleaned(const char *tableName, const char *columnName, psArray *rows)
{
    for (long i = 0; i < psArrayLength(rows); i++) {
        psMetadata *row = rows->data[i];
        const char *state = psMetadataLookupStr(NULL, row, columnName);
        if (!state) {
            psError(PS_ERR_PROGRAMMING, false, "%s not found in row %ld of table %s",
                    columnName, i, tableName);
            return false;
        }
        if (!strcmp("full", state)) {
            // change full to cleaned
            psMetadataAddStr(row, PS_LIST_TAIL, columnName, PS_META_REPLACE, "", "cleaned");
        } else if (strcmp("cleaned", state)) {
            // if state isn't cleaned or full we can't set it to cleaned
            psError(PS_ERR_PROGRAMMING, true, "%s with state %s may not be exported cleaned",
                    tableName, state);
            return false;
        }
    }
    return true;
}

// XXX verify data type?
bool pxAddLabelSearchArgs (pxConfig *config, psMetadata *where, char *name, char *field, char *op) {

    psMetadataItem *item = psMetadataLookup(config->args, name);
    if (!item) {
        psError(psErrorCodeLast(), false, "failed to lookup value for %s", name);
        return false;
    }
    psAssert (item->type == PS_DATA_METADATA_MULTI, "%s should be a multi container", name);
    psAssert (item->data.list->n, "%s should at least have a place-holder", name);
    psMetadataItem *entry = (psMetadataItem *)item->data.list->head->data;
    psAssert (entry, "%s should at least have a place-holder", name);
    if (entry->data.str) {
        psListIterator *iter = psListIteratorAlloc (item->data.list, PS_LIST_HEAD, true);
        psMetadataItem *item = NULL;
        while ((item = psListGetAndIncrement(iter))) {
            psMetadataItem *new = psMetadataItemCopy(item);
            // need to change the name and comment
            psFree (new->name);
            new->name = psStringCopy (field);
            psFree (new->comment);
            new->comment = psStringCopy (op);
            if (!psMetadataAddItem(where, new, PS_LIST_TAIL, PS_META_DUPLICATE_OK)) {
                psError(psErrorCodeLast(), false, "failed to add item %s", field);
                psFree(where);
                return false;
            }
        }
        psFree(iter);
    }
    return true;
}

// shared code for updating the various strings for a Run
bool pxUpdateRun(pxConfig *config, psMetadata *where, psString *pQuery, psString runTable, psString idColumn, psString fileTable, bool has_dist_group, bool has_magicked)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(where, false);
    PS_ASSERT_PTR_NON_NULL(pQuery, false);
    PS_ASSERT_PTR_NON_NULL(*pQuery, false);
    PS_ASSERT_PTR_NON_NULL(runTable, false);
    PS_ASSERT_PTR_NON_NULL(idColumn, false);
    PS_ASSERT_PTR_NON_NULL(fileTable, false);

    // make sure that -state is not the only selection parameter
    PXOPT_LOOKUP_STR(where_state, config->args, "-state", false, false);
    if (where_state && (psListLength(where->list) < 2)) {
        psError(PXTOOLS_ERR_CONFIG, true, "selection by -state alone is not allowed");
        return false;
    }

    PXOPT_LOOKUP_STR(state, config->args,       "-set_state", false, false);
    PXOPT_LOOKUP_STR(label, config->args,       "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args,  "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args,        "-set_note", false, false);

#ifdef DISALLOW_CHANGE_TO_UPDATE
    // Back in the days of magic we didn't allow state changes of Run's
    // to update because the interaction with destreaking was problematic
    // With the death of magic we can allow this now.
    if ((state)&&(!strcmp(state, "update"))) {
        fprintf(stderr, "'-updaterun -set_state update' is not supported.");
        if (!strcmp(runTable, "chipRun")) {
            fprintf(stderr, " Use -setimfiletoupdate.\n");
        } else {
            fprintf(stderr, " Use -setskyfiletoupdate.\n");
        }
        exit(1);
    }
#endif
    psString dist_group = NULL;
    if (has_dist_group) {
        PXOPT_LOOKUP_STR(tmp_dist_group, config->args,  "-set_dist_group", false, false);
        dist_group = tmp_dist_group;
    }

    if ((!state) && (!label) && (!data_group) && (has_dist_group && !dist_group) && !(note)) {
        psError(PXTOOLS_ERR_CONFIG, false, "parameters are required");
        return false;
    }

    if (state && ! pxIsValidState(state)) {
        psError(PXTOOLS_ERR_CONFIG, false, "pxIsValidState failed");
        return false;
    }

    // first paramter is added with "SET param = 'value'"
    // subseqent ones with ", param = 'value'"
    char *separator = " SET ";
    char *comma = ",";

#   define addColumn(_tab, _val)                                        \
    do {                                                                \
        if (_val) {                                                     \
            psStringAppend(pQuery, "%s %s.%s = '%s'", separator, _tab, #_val, _val); \
            separator = comma;                                          \
        }                                                               \
    } while (0)

    addColumn(runTable, state);
    addColumn(runTable, data_group);
    if (has_dist_group) {
        addColumn(runTable, dist_group);
    }
    addColumn(runTable, note);
    addColumn(runTable, label);

    psString joinHook = psStringCopy("");
    psString fileWhere = NULL;

#ifdef DISALLOW_CHANGE_TO_UPDATE
    if (state && !strcmp(state, "update")) {
        psStringAppend(&joinHook, "\n JOIN %s USING(%s)", fileTable, idColumn);
        psStringAppend(pQuery, ", %s.data_state = 'update'", fileTable);
        psStringAppend(&fileWhere, "AND %s.data_state = 'cleaned'", fileTable);
    }
#endif

    psString whereClause =  psDBGenerateWhereSQL(where, NULL);
    psStringAppend(pQuery, " %s", whereClause);
    psFree(whereClause);
    if (fileWhere) {
        psStringAppend(pQuery, "%s", fileWhere);
    }
    if (has_magicked) {
        pxmagicAddWhere(config, pQuery, runTable);
    }

    bool mdok;                          // Status of MD lookup
    if (psMetadataLookupBool(&mdok, config->args, "-pretend")) {
        psLogMsg("pxtools", PS_LOG_INFO, "Query to run: %s\n", *pQuery);
        return true;
    }

    if (!p_psDBRunQueryF(config->dbh, *pQuery, joinHook)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}

bool pxLookupVersion(pxConfig *config, psArray **pArray)
{
    const char *query = "SELECT * FROM dbversion";

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psFree(output);
        psError(PXTOOLS_ERR_PROG, true, "no rows in dbversion");
        return false;
    }
    if (psArrayLength(output) > 1) {
        psError(PXTOOLS_ERR_PROG, true, "unexpected number of rows found in dbversion: %ld",
                psArrayLength(output));
        return false;
    }
    *pArray = output;

    return true;
}

psString pxGetDBVersion(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psArray *array = NULL;
    if (!pxLookupVersion(config, &array)) {
        psError(psErrorCodeLast(), false, "pxLookupVersion failed");
        return NULL;
    }
    psMetadata *md = array->data[0];
    if (!md) {
        psError(PXTOOLS_ERR_PROG, true, "output of pxLookupVersion is null");
        return NULL;
    }

    psString version = psMetadataLookupStr(NULL, md, "schema_version");

    return version;
}

bool pxExportVersion(pxConfig *config, FILE *file)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(file, NULL);

    psArray *array = NULL;
    if (!pxLookupVersion(config, &array) || !array) {
        psError(psErrorCodeLast(), false, "pxLookupVersion failed");
        return false;
    }
    if (!ippdbPrintMetadatas(file, array, "dbversion", true)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(array);
        return false;
    }
    return true;
}

bool pxCheckImportVersion(pxConfig *config, psMetadata *input)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(input, NULL);

    // This code was adapted from the way camtool parses the structures.
    // Is this really the way to do it?
    psMetadataItem *multi_item =  psMetadataLookup(input, "dbversion");
    if (!multi_item || (multi_item->type != PS_DATA_METADATA_MULTI)) {
        psError(PXTOOLS_ERR_PROG, true, "dbversion multi not found in input");
        return false;
    }

    psMetadataItem *dbversion = psListGet(multi_item->data.list, 0);
    if (!dbversion) {
        psError(PXTOOLS_ERR_PROG, true, "dbversion not found in input");
        return false;
    }

    if (!strcmp(dbversion->name, "dbversion")) {
        // horray
        psMetadata *md = dbversion->data.md;
        psString schema_version = pxGetDBVersion(config);
        if (!schema_version) {
            psError(psErrorCodeLast(), false, "pxGetDBVersion failed");
            return false;
        }

        psString import_version = psMetadataLookupStr(NULL, md, "schema_version");
        if (import_version && strcmp(import_version, schema_version)) {
            psError(PXTOOLS_ERR_PROG, true, "input file schema_version: %s does not match data base: %s",
                    import_version, schema_version);
            return false;
        } else if (!import_version) {
            psError(PXTOOLS_ERR_PROG, true, "input file schema_version is NULL");
            return false;
        } else {
            // YIPPEE this file is the same version
        }
    } else {
        psError(PXTOOLS_ERR_PROG, true, "Unexpected config dump format");
        return false;
    }

    return true;
}

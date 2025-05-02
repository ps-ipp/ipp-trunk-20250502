/*
 * receivetool.c
 *
 * Copyright (C) 2008
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "pxtools.h"
#include "pxdata.h"
#include "receivetool.h"

static bool definesourceMode(pxConfig *config);
static bool listMode(pxConfig *config);
static bool addfilesetMode(pxConfig *config);
static bool updatelastMode(pxConfig *config);
static bool pendingfilesetMode(pxConfig *config);
static bool updatefilesetMode(pxConfig *config);
static bool addfileMode(pxConfig *config);
static bool pendingfileMode(pxConfig *config);
static bool addresultMode(pxConfig *config);
static bool toadvanceMode(pxConfig *config);
static bool revertMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;


int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = receivetoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(RECEIVETOOL_MODE_DEFINESOURCE, definesourceMode);
        MODECASE(RECEIVETOOL_MODE_LIST, listMode);
        MODECASE(RECEIVETOOL_MODE_ADDFILESET, addfilesetMode);
        MODECASE(RECEIVETOOL_MODE_UPDATELAST, updatelastMode);
        MODECASE(RECEIVETOOL_MODE_PENDINGFILESET, pendingfilesetMode);
        MODECASE(RECEIVETOOL_MODE_UPDATEFILESET, updatefilesetMode);
        MODECASE(RECEIVETOOL_MODE_TOADVANCE, toadvanceMode);
        MODECASE(RECEIVETOOL_MODE_ADDFILE, addfileMode);
        MODECASE(RECEIVETOOL_MODE_PENDINGFILE, pendingfileMode);
        MODECASE(RECEIVETOOL_MODE_ADDRESULT, addresultMode);
        MODECASE(RECEIVETOOL_MODE_REVERT, revertMode);
      default:
        psAbort("invalid option (this should not happen)");
    }

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(EXIT_SUCCESS);

FAIL:
    psErrorStackPrint(stderr, "\n");
    int exit_status = pxerrorGetExitStatus();

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(exit_status);
}

static bool definesourceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(source, config->args, "-source", true, false);
    PXOPT_LOOKUP_STR(product, config->args, "-product",  true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", false, false);

    // optional
    PXOPT_LOOKUP_STR(comment, config->args, "-comment",  false, false);
    PXOPT_LOOKUP_STR(state, config->args, "-state",  false, false);
    PXOPT_LOOKUP_STR(last, config->args, "-last",  false, false);
    PXOPT_LOOKUP_STR(status_product, config->args, "-status_product",  false, false);
    PXOPT_LOOKUP_STR(ds_dbname, config->args, "-ds_dbname",  false, false);
    PXOPT_LOOKUP_STR(ds_dbhost, config->args, "-ds_dbhost",  false, false);

    if (!receiveSourceInsert(config->dbh, 0, source, product, workdir, state, comment, last, status_product, ds_dbname, ds_dbhost)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}

static bool listMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions

    // required
    PXOPT_COPY_STR(config->args, where, "-source", "receiveSource.source", "==");
    PXOPT_COPY_STR(config->args, where, "-product", "receiveSource.product", "==");
    PXOPT_COPY_S64(config->args, where, "-comment", "receiveSource.comment", "LIKE");

    // optional
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("receivetool_list.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("receivetool", PS_LOG_INFO, "No rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "receiveSource", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool addfilesetMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(source_id, config->args, "-src_id", true, false);
    psMetadataItem *filesets = psMetadataLookup(config->args, "-fileset");
    if (!filesets) {
        psError(PS_ERR_UNKNOWN, true, "-fileset is required");
        return false;
    }

    psString query = pxDataGet("receivetool_addfileset.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        return false;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    psString source_id_str = NULL;      // source_id as a string
    psStringAppend(&source_id_str, "%" PRId64, source_id);

    psListIterator *iter = psListIteratorAlloc(filesets->data.list, PS_LIST_HEAD, false); // Iterator
    psMetadataItem *item = NULL;        // Item from iteration
    while ((item = psListGetAndIncrement(iter))) {
        psAssert(item && item->data.V && item->type == PS_DATA_STRING, "Argument is bad");
        const char *fileset = item->data.str; // Fileset name

        if (!p_psDBRunQueryF(config->dbh, query, source_id_str, fileset)) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
            psFree(source_id_str);
            psFree(query);
            psFree(iter);
            if (!psDBTransaction(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "Database error");
            }
            return false;
        }

        psArray *output = p_psDBFetchResult(config->dbh); // Output of query
        if (!output) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
            psFree(source_id_str);
            psFree(query);
            psFree(iter);
            return false;
        }
        if (psArrayLength(output) > 0) {
            psTrace("receivetool", PS_LOG_INFO, "Fileset %s is already present", fileset);
            psFree(output);
            continue;
        }
        psFree(output);

        if (!receiveFilesetInsert(config->dbh, 0, source_id, fileset, "reg", NULL, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add fileset");
            psFree(source_id_str);
            psFree(query);
            psFree(iter);
            if (!psDBTransaction(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "Database error");
            }
            return false;
        }
    }
    psFree(iter);
    psFree(query);
    psFree(source_id_str);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}

static bool updatelastMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(source_id, config->args, "-src_id", true, false);
    PXOPT_LOOKUP_STR(fileset, config->args, "-fileset",  true, false);

    psString query = NULL;              // Query to execute
    psStringAppend(&query, "UPDATE receiveSource SET fileset_last = \'%s\' WHERE source_id = %" PRId64,
                   fileset, source_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}


static bool pendingfilesetMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions

    // required
    PXOPT_COPY_STR(config->args, where, "-source", "receiveSource.source", "==");
    PXOPT_COPY_STR(config->args, where, "-product", "receiveSource.product", "==");
    PXOPT_COPY_STR(config->args, where, "-comment", "receiveSource.comment", "LIKE");

    // optional
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("receivetool_pendingfileset.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // make sure that the data is processed in the order that the filesets
    // were posted
    psStringAppend(&query, "\nORDER BY fileset_id");

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("receivetool", PS_LOG_INFO, "No rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "receiveFileset", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool addfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(fileset_id, config->args, "-fileset_id", true, false);
    PXOPT_LOOKUP_STR(file_list, config->args, "-file_list", true, false);

    unsigned int numBad;
    psMetadata *files = psMetadataConfigRead(NULL, &numBad, file_list, false);
    if (!files) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Unable to cleanly read MDC file with file list.");
        return false;
    }


    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(files, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;
    while ((item = psMetadataGetAndIncrement(iter))) {
        psMetadata *md = item->data.md;
        psString file = psMetadataLookupStr(NULL, md, "file");
        psS64    bytes = psMetadataLookupS64(NULL, md, "bytes");
        psString md5sum = psMetadataLookupStr(NULL, md, "md5sum");
        psString file_type = psMetadataLookupStr(NULL, md, "file_type");
        psString component = psMetadataLookupStr(NULL, md, "component");

        if (!file) {
            psError(PS_ERR_UNKNOWN, false, "failed to find value for file");
            return false;
        }
        if (!component) {
            psError(PS_ERR_UNKNOWN, false, "failed to find value for component");
            return false;
        }

        if (!receiveFileInsert(config->dbh, 0, fileset_id, file, bytes, md5sum, file_type, component)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add file");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "Database error");
                return false;
            }
            psFree(iter);
            return false;
        }
    }

    psFree(iter);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}

static bool pendingfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions

    // required
    PXOPT_COPY_STR(config->args, where, "-source", "receiveSource.source", "==");
    PXOPT_COPY_STR(config->args, where, "-product", "receiveSource.product", "==");
    PXOPT_COPY_STR(config->args, where, "-comment", "receiveSource.comment", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-fileset_id", "receiveFile.fileset_id", "==");
    PXOPT_COPY_S64(config->args, where, "-file_id", "receiveFile.file_id", "==");

    // optional
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("receivetool_pendingfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // make sure that the data is processed in the order that the filesets
    // were posted
    psStringAppend(&query, "\nORDER BY fileset_id");

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("receivetool", PS_LOG_INFO, "No rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "receiveFile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool addresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(file_id, config->args, "-file_id", true, false);

    // optional
    PXOPT_LOOKUP_F32(dtime_copy, config->args, "-dtime_copy", false, false);
    PXOPT_LOOKUP_F32(dtime_extract, config->args, "-dtime_extract", false, false);
    PXOPT_LOOKUP_S32(fault, config->args, "-fault", false, false);

    if (!receiveResultInsert(config->dbh, file_id, dtime_copy, dtime_extract, fault)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}

static bool revertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions

    PXOPT_COPY_S64(config->args, where, "-fileset_id", "receiveResult.fileset_id", "==");
    PXOPT_COPY_S32(config->args, where, "-fault", "receiveResult.fault", "==");
    PXOPT_COPY_STR(config->args, where, "-source", "receiveSource.source", "==");
    PXOPT_COPY_STR(config->args, where, "-product", "receiveSource.product", "==");
    PXOPT_COPY_STR(config->args, where, "-comment", "receiveSource.comment", "LIKE");

    psString query = pxDataGet("receivetool_revert.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}
static bool toadvanceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions

    // optional
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("receivetool_toadvance.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("receivetool", PS_LOG_INFO, "No rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "toadvanceFilesets", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool updatefilesetMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(fileset_id, config->args, "-fileset_id", true, false);

    // to chanage
    PXOPT_LOOKUP_S32(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_STR(destdir, config->args, "-destdir", false, false);
    PXOPT_LOOKUP_STR(dirinfo, config->args, "-dirinfo", false, false);
    PXOPT_LOOKUP_STR(dbinfo, config->args, "-dbinfo", false, false);
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);

    if (!fault && !dirinfo &&!dbinfo && !state) {
        psError(PS_ERR_UNKNOWN, true, "at least one of -fault, -dirinfo, -dbinfo, -set_state are required");
        return false;
    }

    psString query = NULL;              // Query to execute
    psStringAppend(&query, "UPDATE receiveFileset SET ");

    psString sep = "";
    if (fault) {
        psStringAppend(&query, "%s fault = %d", sep, fault);
        sep = ",";
    }
    if (dirinfo) {
        psStringAppend(&query, "%s dirinfo = '%s'", sep, dirinfo);
        sep = ",";
    }
    if (dbinfo) {
        psStringAppend(&query, "%s dbinfo = '%s'", sep, dbinfo);
        sep = ",";
    }
    if (state) {
        psStringAppend(&query, "%s state = '%s'", sep, state);
        sep = ",";
    }

    psStringAppend(&query, " WHERE fileset_id = %" PRId64, fileset_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

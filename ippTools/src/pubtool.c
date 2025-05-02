/*
 * pubtool.c
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
#include "pubtool.h"

static bool defineclientMode(pxConfig *config);
static bool updateclientMode(pxConfig *config);
static bool definerunMode(pxConfig *config);
static bool pendingMode(pxConfig *config);
static bool addMode(pxConfig *config);
static bool revertMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;


int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pubtoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(PUBTOOL_MODE_DEFINECLIENT, defineclientMode);
        MODECASE(PUBTOOL_MODE_UPDATECLIENT, updateclientMode);
        MODECASE(PUBTOOL_MODE_DEFINERUN, definerunMode);
        MODECASE(PUBTOOL_MODE_PENDING, pendingMode);
        MODECASE(PUBTOOL_MODE_ADD, addMode);
        MODECASE(PUBTOOL_MODE_REVERT, revertMode);
        MODECASE(PUBTOOL_MODE_UPDATERUN, updaterunMode);
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

static bool defineclientMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(product, config->args, "-product",  true, false);
    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir",  true, false);

    // optional
    PXOPT_LOOKUP_STR(comment, config->args, "-comment",  false, false);
    PXOPT_LOOKUP_BOOL(unmagicked, config->args, "-unmagicked",  false);
    PXOPT_LOOKUP_STR(name, config->args, "-name",  false, false);
    PXOPT_LOOKUP_S16(output_format, config->args, "-output_format",  false, false);

    if (!publishClientInsert(config->dbh, 0, 0, product, stage, !unmagicked, workdir, comment, name, output_format)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}

static bool updateclientMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions
    PXOPT_COPY_S64(config->args, where, "-client_id", "client_id", "==");
    PXOPT_COPY_STR(config->args, where, "-product", "product", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-comment", "comment", "LIKE");

    PXOPT_LOOKUP_BOOL(active, config->args, "-active",  false);
    PXOPT_LOOKUP_BOOL(inactive, config->args, "-inactive",  false);

    if ((!active && !inactive) || (active && inactive)) {
        psError(PS_ERR_UNKNOWN, false, "Must specify one, and only one, of -active and -inactive.");
        psFree(where);
        return false;
    }

    psString query = NULL;              // Query to run
    psStringAppend(&query, "UPDATE publishClient SET active = %d", active ? 1 : 0);

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\n WHERE %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    long numUpdated = psDBAffectedRows(config->dbh);
    psLogMsg("pubtool", PS_LOG_INFO, "%ld rows updated.", numUpdated);

    return true;
}

static bool definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *diffWhere = psMetadataAlloc(); // WHERE conditions for diffs
    psMetadata *camWhere = psMetadataAlloc(); // WHERE conditions for cams
    psMetadata *diffphotWhere = psMetadataAlloc(); // WHERE conditions for diffphots

    // required

    // optional
    PXOPT_COPY_S64(config->args, diffWhere, "-client_id", "client_id", "==");
    pxAddLabelSearchArgs(config, diffWhere, "-label", "diffRun.label", "==");
    pxAddLabelSearchArgs(config, diffWhere, "-data_group", "diffRun.data_group", "LIKE");
    PXOPT_COPY_TIME(config->args, diffWhere, "-dateobs_begin", "rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, diffWhere, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_STR(config->args, diffWhere, "-filter", "rawExp.filter", "LIKE");
    PXOPT_COPY_STR(config->args, diffWhere, "-obs_mode", "rawExp.obs_mode", "LIKE");
    PXOPT_COPY_STR(config->args, diffWhere, "-comment", "rawExp.comment", "LIKE");

    PXOPT_COPY_S64(config->args, camWhere, "-client_id", "client_id", "==");
    pxAddLabelSearchArgs(config, camWhere, "-label", "camRun.label", "==");
    pxAddLabelSearchArgs(config, camWhere, "-data_group", "camRun.data_group", "LIKE");
    PXOPT_COPY_TIME(config->args, camWhere, "-dateobs_begin", "rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, camWhere, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_STR(config->args, camWhere, "-filter", "rawExp.filter", "LIKE");
    PXOPT_COPY_STR(config->args, camWhere, "-obs_mode", "rawExp.obs_mode", "LIKE");

    PXOPT_COPY_S64(config->args, diffphotWhere, "-client_id", "client_id", "==");
    pxAddLabelSearchArgs(config, diffphotWhere, "-label", "diffPhotRun.label", "==");
    pxAddLabelSearchArgs(config, diffphotWhere, "-data_group", "diffPhotRun.data_group", "LIKE");
    PXOPT_COPY_TIME(config->args, diffphotWhere, "-dateobs_begin", "rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, diffphotWhere, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_STR(config->args, diffphotWhere, "-filter", "rawExp.filter", "LIKE");
    PXOPT_COPY_STR(config->args, diffphotWhere, "-obs_mode", "rawExp.obs_mode", "LIKE");

    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pubtool_definerun.sql"); // Query to run
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(diffWhere);
        psFree(camWhere);
        return false;
    }

    if (!rerun) {
        psStringAppend(&query, "\nWHERE publishRun.client_id IS NULL");
    }

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(diffWhere);
        psFree(camWhere);
        psFree(diffphotWhere);
        return false;
    }

    psString whereDiff = psStringCopy(""); // Additional constraints to add to query
    if (psListLength(diffWhere->list)) {
        psString clause = psDBGenerateWhereConditionSQL(diffWhere, NULL);
        psStringAppend(&whereDiff, "\n AND %s", clause);
        psFree(clause);
    }
    psFree(diffWhere);

    psString whereCam = psStringCopy(""); // Additional constraints to add to query
    if (psListLength(camWhere->list)) {
        psString clause = psDBGenerateWhereConditionSQL(camWhere, NULL);
        psStringAppend(&whereCam, "\n AND %s", clause);
        psFree(clause);
    }
    psFree(camWhere);

    psString whereDiffphot = psStringCopy(""); // Additional constraints to add to query
    if (psListLength(diffphotWhere->list)) {
        psString clause = psDBGenerateWhereConditionSQL(diffphotWhere, NULL);
        psStringAppend(&whereDiffphot, "\n AND %s", clause);
        psFree(clause);
    }
    psFree(diffphotWhere);

    if (!p_psDBRunQueryF(config->dbh, query, whereDiff, whereCam, whereDiffphot)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        psFree(whereDiff);
        psFree(whereCam);
        psFree(whereDiffphot);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
        }
        return false;
    }
    psFree(query);
    psFree(whereDiff);
    psFree(whereCam);
    psFree(whereDiffphot);

    psArray *output = p_psDBFetchResult(config->dbh); // Output of SELECT statement
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("pubtool", PS_LOG_INFO, "No rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        if (!ippdbPrintMetadatas(stdout, output, "publishRun", !simple)) {
            psError(psErrorCodeLast(), false, "Failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    for (int i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row of interest
        psS64 client = psMetadataLookupS64(NULL, row, "client_id"); // Client identifier
        psS64 stage = psMetadataLookupS64(NULL, row, "stage_id");   // Stage identifier
        const char *label = psMetadataLookupStr(NULL, row, "src_label");   // label from correct source

        if (!publishRunInsert(config->dbh, 0, client, stage, set_label ? set_label : label, "new")) {
            psError(PS_ERR_UNKNOWN, false, "Unable to add fileset");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "Database error");
            }
            return false;
        }
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}

static bool pendingMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions

    // required
    PXOPT_COPY_STR(config->args, where, "-client_id", "publishClient.client_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "publishClient.stage", "==");
    PXOPT_COPY_STR(config->args, where, "-comment", "publishClient.comment", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-name", "publishClient.name", "LIKE");
    pxAddLabelSearchArgs(config, where, "-label", "publishRun.label", "==");

    // optional
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psString query = pxDataGet("pubtool_pending.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    psString whereClause = psStringCopy(""); // WHERE conditions to add
    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereClause, "\nAND %s", clause);
        psFree(clause);
    } else {
      if (!all) {
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (try -all)");
        return false;
      }
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereClause, whereClause, whereClause)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(whereClause);
        psFree(query);
        return false;
    }
    psFree(whereClause);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("pubtool", PS_LOG_INFO, "No rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "publishRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool addMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(pub_id, config->args, "-pub_id", true, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

    // optional
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_S32(fault, config->args, "-fault", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    if (!publishDoneInsert(config->dbh, pub_id, path_base, hostname, dtime_script, fault)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add file");
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
        }
        return false;
    }

    if (fault == 0) {
        if (!p_psDBRunQueryF(config->dbh,
                             "UPDATE publishRun SET state = 'full' WHERE pub_id = %" PRId64,
                             pub_id)) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
            if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "Database error");
            }
            return false;
        }
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        return false;
    }

    return true;
}




static bool revertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions
    PXOPT_COPY_S64(config->args, where, "-pub_id", "publishRun.pub_id", "==");
    PXOPT_COPY_S32(config->args, where, "-fault", "publishDone.fault", "==");
    PXOPT_COPY_STR(config->args, where, "-client_id", "publishClient.client_id", "==");
    PXOPT_COPY_STR(config->args, where, "-comment", "publishClient.comment", "LIKE");
    pxAddLabelSearchArgs(config, where, "-label", "publishRun.label", "==");

    psString query = pxDataGet("pubtool_revert.sql");
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

    psLogMsg("pubtool", PS_LOG_INFO, "Deleted %" PRIu64 " rows", psDBAffectedRows(config->dbh));

    return true;
}

static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc(); // WHERE conditions
    PXOPT_COPY_S64(config->args, where, "-pub_id", "pub_id", "==");
    PXOPT_COPY_S64(config->args, where, "-client_id", "client_id", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
    PXOPT_COPY_STR(config->args, where, "-fault", "fault", "==");

    PXOPT_LOOKUP_STR(state, config->args, "-set_state",  true, false);

    psString query = NULL;              // Query to run
    psStringAppend(&query, "UPDATE publishRun LEFT JOIN publishDone using(pub_id) SET state = '%s'", state);

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\n WHERE %s", clause);
        psFree(clause);
    } else {
        psError(PS_ERR_UNKNOWN, false, "select arguments are required");
        return false;
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Database error");
        psFree(query);
        return false;
    }
    psFree(query);

    long numUpdated = psDBAffectedRows(config->dbh);
    psLogMsg("pubtool", PS_LOG_INFO, "%ld rows updated.", numUpdated);

    return true;
}

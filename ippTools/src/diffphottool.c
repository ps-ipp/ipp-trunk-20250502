/*
 * diffphottool.c
 *
 * Copyright (C) 2007-2010  Joshua Hoblitt, Paul Price
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ippdb.h>

#include "pxtools.h"
#include "diffphottool.h"

static bool definerunMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool inputMode(pxConfig *config);
static bool pendingMode(pxConfig *config);
static bool doneMode(pxConfig *config);
static bool advanceMode(pxConfig *config);
static bool revertMode(pxConfig *config);
static bool dataMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = diffphottoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(DIFFPHOTTOOL_MODE_DEFINERUN, definerunMode);
        MODECASE(DIFFPHOTTOOL_MODE_UPDATERUN, updaterunMode);
        MODECASE(DIFFPHOTTOOL_MODE_INPUT,     inputMode);
        MODECASE(DIFFPHOTTOOL_MODE_PENDING,   pendingMode);
        MODECASE(DIFFPHOTTOOL_MODE_DONE,      doneMode);
        MODECASE(DIFFPHOTTOOL_MODE_ADVANCE,   advanceMode);
        MODECASE(DIFFPHOTTOOL_MODE_REVERT,    revertMode);
        MODECASE(DIFFPHOTTOOL_MODE_DATA,      dataMode);

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


static bool definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // Options
    PXOPT_LOOKUP_STR(set_workdir, config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(set_data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(set_reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(set_note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // Selections
    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs(config, where, "-label", "diffRun.label", "LIKE");
    pxAddLabelSearchArgs(config, where, "-data_group", "diffRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-comment", "rawExp.comment", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "rawExp.filter", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "rawExp.dateobs", "<=");


    psString query = pxDataGet("diffphottool_definerun.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (!psDBTransaction(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "Unable to run query: %s", query);
        psFree(where);
        return false;
    }
    psFree(where);

    psArray *results = p_psDBFetchResult(config->dbh); // Results of query
    if (!results) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!psArrayLength(results)) {
        psTrace("diffphottool", 1, "no rows found");
        psFree(results);
        return true;
    }

    if (pretend) {
        if (!ippdbPrintMetadatas(stdout, results, "diffPhotRun", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(results);
            return false;
        }
        psFree(results);
        return true;
    }

    for (int i = 0; i < results->n; i++) {
        psMetadata *row = results->data[i]; // Output row from query
        bool mdok;                          // Status of MD lookup
        psS64 diff_id = psMetadataLookupS64(&mdok, row, "diff_id");
        const char *workdir = psMetadataLookupStr(&mdok, row, "workdir");
        const char *label = psMetadataLookupStr(&mdok, row, "data_group");
        const char *data_group = psMetadataLookupStr(&mdok, row, "data_group");
        const char *reduction = psMetadataLookupStr(&mdok, row, "reduction");
        const char *note = psMetadataLookupStr(&mdok, row, "note");

        diffPhotRunRow *run = diffPhotRunRowAlloc(0, diff_id, "new",
                                                  set_workdir ? set_workdir : workdir,
                                                  set_label ? set_label : label,
                                                  set_data_group ? set_data_group : data_group,
                                                  set_reduction ? set_reduction : reduction,
                                                  registered,
                                                  set_note ? set_note : note, 0);
        if (!diffPhotRunInsertObject(config->dbh, run)) {
            psError(psErrorCodeLast(), false, "database error");
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            psFree(run);
            psFree(results);
            return false;
        }

        run->diff_phot_id = psDBLastInsertID(config->dbh);

        if (!diffPhotRunPrintObject(stdout, run, !simple)) {
            psError(psErrorCodeLast(), false, "failed to print object");
            psFree(run);
            psFree(results);
            return false;
        }
        psFree(run);
    }
    psFree(results);

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}


static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-diff_phot_id", "diff_phot_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-data_group", "data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE diffPhotRun");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "diffPhotRun", "diff_phot_id", "diffPhotSkyfile", true, false);

    psFree(query);
    psFree(where);

    return result;
}


static bool inputMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where,  "-diff_phot_id", "diff_phot_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("diffphottool_input.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
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
    if (!psArrayLength(output)) {
        psTrace("diffphottool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "diffSkyfile", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}


static bool pendingMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_phot_id", "diff_phot_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "diffPhotRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("diffphottool_pending.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
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
    if (!psArrayLength(output)) {
        psTrace("diffphottool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "diffPhotRun", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}


static bool doneMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(diff_phot_id, config->args, "-diff_phot_id", true, false); // required
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", true, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_STR(ver_psphot, config->args, "-ver_psphot", false, false);
    PXOPT_LOOKUP_S64(magicked, config->args, "-magicked", false, false);

    psString version = pxMergeCodeVersions(ver_pslib, ver_psmodules);
    version = pxMergeCodeVersions(version, ver_ppstats);
    version = pxMergeCodeVersions(version, ver_psphot);

    if (!diffPhotSkyfileInsert(config->dbh, diff_phot_id, skycell_id, path_base, dtime_script, hostname,
                               fault, quality, version, magicked)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}

static bool advanceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_phot_id", "diff_phot_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "diffPhotRun.label", "==");

    psString query = pxDataGet("diffphottool_advance.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = psStringCopy("");
    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereClause, "\nAND %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (!psDBTransaction(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereClause)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);
    psFree(whereClause);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    for (int i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row of interest
        psS64 diff_phot_id = psMetadataLookupS64(NULL, row, "diff_phot_id");
        psS64 magicked = psMetadataLookupS64(NULL, row, "magicked");

        const char *query = "UPDATE diffPhotRun SET state = 'full', magicked = %" PRId64
            " WHERE diff_phot_id = %" PRId64;
        if (!p_psDBRunQueryF(config->dbh, query, magicked, diff_phot_id)) {
            psError(psErrorCodeLast(), false,
                    "failed to change state for diff_phot_id %" PRId64, diff_phot_id);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            psFree(output);
            return false;
        }
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}


static bool revertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_phot_id", "diff_phot_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "LIKE");

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    if (!psListLength(where->list) && !all) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        psFree(where);
        return false;
    }

    psString query = pxDataGet("diffphottool_revert.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool dataMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where,  "-diff_phot_id", "diff_phot_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("diffphottool_data.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
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
    if (!psArrayLength(output)) {
        psTrace("diffphottool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "diffPhotSkyfile", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}


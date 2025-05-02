/*
 * fftool.c
 *
 * Copyright (C) 2013 Institute for Astronomy, University of Hawaii
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
#include "pxspace.h"
#include "fftool.h"

static bool definebyqueryMode(pxConfig *config);
static bool defineforstacksMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool todoMode(pxConfig *config);
static bool addresultMode(pxConfig *config);
static bool resultMode(pxConfig *config);
static bool revertMode(pxConfig *config);
static bool updateresultMode(pxConfig *config);
static bool toadvanceMode(pxConfig *config);
static bool addsummaryMode(pxConfig *config);
static bool revertsummaryMode(pxConfig *config);
static bool updatesummaryMode(pxConfig *config);
static bool summaryMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

static bool setfullForceRunState(pxConfig *config, psS64 sky_id, const char *state);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = fftoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(FFTOOL_MODE_DEFINEBYQUERY,     definebyqueryMode);
        MODECASE(FFTOOL_MODE_DEFINEFORSTACKS,   defineforstacksMode);
        MODECASE(FFTOOL_MODE_UPDATERUN,         updaterunMode);
        MODECASE(FFTOOL_MODE_TODO,              todoMode);
        MODECASE(FFTOOL_MODE_ADDRESULT,         addresultMode);
        MODECASE(FFTOOL_MODE_RESULT,            resultMode);
        MODECASE(FFTOOL_MODE_REVERT,            revertMode);
        MODECASE(FFTOOL_MODE_UPDATERESULT,      updateresultMode);
        MODECASE(FFTOOL_MODE_TOADVANCE,         toadvanceMode);
        MODECASE(FFTOOL_MODE_ADDSUMMARY,        addsummaryMode);
        MODECASE(FFTOOL_MODE_REVERTSUMMARY,     revertsummaryMode);
        MODECASE(FFTOOL_MODE_UPDATESUMMARY,     updatesummaryMode);
        MODECASE(FFTOOL_MODE_SUMMARY,           summaryMode);
        MODECASE(FFTOOL_MODE_EXPORTRUN,         exportrunMode);
        MODECASE(FFTOOL_MODE_IMPORTRUN,         importrunMode);
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


static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PXOPT_LOOKUP_STR(workdir,     config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label", true, false);

    // optional
    PXOPT_LOOKUP_STR(data_group,  config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group,  config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(note,        config->args, "-set_note", false, false);
    PXOPT_LOOKUP_STR(reduction,   config->args, "-set_reduction", false, false);

    PXOPT_LOOKUP_STR(sources_path_base,   config->args, "-set_sources_path_base", false, false);

    psMetadata *skycalWhereMD = psMetadataAlloc();
    pxAddLabelSearchArgs(config, skycalWhereMD, "-select_skycal_label",   "skycalRun.label",         "LIKE");
    pxAddLabelSearchArgs(config, skycalWhereMD, "-select_skycal_data_group", "skycalRun.data_group", "LIKE");
    PXOPT_COPY_S64(config->args, skycalWhereMD, "-select_skycal_id",      "skycalRun.skycal_id",     "==");

    if (!psListLength(skycalWhereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "skycal search parameters are required");
        psFree(skycalWhereMD);
        return false;
    }

    PXOPT_COPY_STR(config->args, skycalWhereMD, "-select_skycell_id",    "stackRun.skycell_id",      "LIKE");
    PXOPT_COPY_STR(config->args, skycalWhereMD, "-select_tess_id",       "stackRun.tess_id",         "==");
    pxAddLabelSearchArgs(config, skycalWhereMD, "-select_filter",        "stackRun.filter",          "LIKE");
    if (!pxskycellAddWhere(config, skycalWhereMD)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        psFree(skycalWhereMD);
        return false;
    }

    psMetadata *warpWhereMD = psMetadataAlloc();
    pxAddLabelSearchArgs(config, warpWhereMD, "-select_warp_label",     "warpRun.label",           "LIKE");
    pxAddLabelSearchArgs(config, warpWhereMD, "-select_warp_data_group","warpRun.data_group",      "LIKE");
    PXOPT_COPY_S64(config->args, warpWhereMD, "-select_warp_id",        "warpRun.warp_id",         "==");
    if (!psListLength(warpWhereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "warp search parameters are required");
        psFree(warpWhereMD);
        psFree(skycalWhereMD);
        return false;
    }
    PXOPT_COPY_F32(config->args, warpWhereMD, "-select_good_frac_min",  "warpSkyfile.good_frac",   ">=");
    PXOPT_COPY_STR(config->args, warpWhereMD, "-select_tess_id",       "warpRun.tess_id",         "==");
    pxAddLabelSearchArgs(config, warpWhereMD, "-select_filter",        "rawExp.filter",          "LIKE");

    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    psString select = pxDataGet("fftool_definebyquery.sql");
    if (!select) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(skycalWhereMD);
        psFree(warpWhereMD);
        return false;
    }

    psString where = NULL;
    psString whereClause = psDBGenerateWhereConditionSQL(skycalWhereMD, NULL);
    psStringAppend(&where, "\nAND %s", whereClause);
    psStringAppend(&select, " %s", where);
    psFree(whereClause);
    psFree(skycalWhereMD);

    psString joinHook = NULL;
    if (!rerun) {
        psStringAppend(&joinHook, "\nLEFT JOIN fullForceRun ON fullForceRun.skycal_id = skycalRun.skycal_id");
        psStringAppend(&joinHook, "\n %s\nAND fullForceRun.label = '%s'", where, label);
        psStringAppend(&select, "\nAND ff_id IS NULL");
    }

    if (!p_psDBRunQueryF(config->dbh, select, joinHook)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(select);
        return false;
    }
    psFree(select);
    psFree(joinHook);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }
        psFree(where);
        return false;
    }
    if (!psArrayLength(output)) {
        psWarning("fftool: no rows found");
        psFree(output);
        psFree(where);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "toFullForce", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            psFree(where);
            return false;
        }
        psFree(output);
        psFree(where);
        return true;
    }

    psString warpQueryTemplate = pxDataGet("fftool_definebyquery_select_warps.sql");

    whereClause = psDBGenerateWhereConditionSQL(warpWhereMD, NULL);
    psStringAppend(&warpQueryTemplate, "\nAND %s", whereClause);

    for (long i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row from select
        bool status;

	if (!psDBTransaction(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
	    return false;
	}

        // psS64 warp_id = psMetadataLookupS64(&status, row, "warp_id");
        psS64 skycal_id = psMetadataLookupS64(&status, row, "skycal_id");

        psString path_base = NULL;
        if (sources_path_base) {
            path_base = sources_path_base;
        } else {
            path_base = psMetadataLookupStr(&status, row, "path_base");
	    psAssert(status, "failed to find skycal path_base?");
        }

        psString skycal_data_group = psMetadataLookupStr(&status, row, "data_group");
        psString tess_id = psMetadataLookupStr(&status, row, "tess_id");
        psString skycell_id = psMetadataLookupStr(&status, row, "skycell_id");
        psString filter = psMetadataLookupStr(&status, row, "filter");

        psString query = NULL;
        psStringAppend(&query, warpQueryTemplate, tess_id, skycell_id, filter);

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
        psFree(query);

        // Find the warps for this skycell and filter combination
        psArray *warpOutput = p_psDBFetchResult(config->dbh);
        if (!warpOutput) {
            psErrorCode err = psErrorCodeLast();
            switch (err) {
                case PS_ERR_DB_CLIENT:
                    psError(PXTOOLS_ERR_SYS, false, "database error");
                case PS_ERR_DB_SERVER:
                    psError(PXTOOLS_ERR_PROG, false, "database error");
                default:
                    psError(PXTOOLS_ERR_PROG, false, "unknown error");
            }
            return false;
        }
        if (!psArrayLength(warpOutput)) {
            // no warps for this skycal. Suprise?
            psFree(warpOutput);
            psFree(query);
            continue;
        }

	// create a staticskyRun
	if (!fullForceRunInsert(config->dbh,
				0x0,	     // ff_id
                                skycal_id,
                                path_base,
				"new",	     // state
				workdir,
				label,
				data_group ? data_group : (skycal_data_group ? skycal_data_group : label),
				dist_group,
                                note,
				reduction,
				NULL        // registered
		)
	    ) {
	    if (!psDBRollback(config->dbh)) {
		psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
	    }
	    psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
	    return false;
	}

        psS64 ff_id = psDBLastInsertID(config->dbh);

        for (int j = 0; j < warpOutput->n; j++) {
            psMetadata *warpRow = warpOutput->data[j];
            psS64 warp_id = psMetadataLookupS64(&status, warpRow, "warp_id");

            if (!fullForceInputInsert(config->dbh,
				ff_id,
                                warp_id)
               ) {
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
                }
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(warpOutput);
                psFree(output);
                return false;
            }
        }

	if (!psDBCommit(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(warpOutput);
	    psFree(output);
	    return false;
	}
        psFree(warpOutput);
    }
    psFree(output);

    return true;
}

static bool defineforstacksMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PXOPT_LOOKUP_STR(workdir,     config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label", true, false);

    // optional
    PXOPT_LOOKUP_STR(data_group,  config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group,  config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(note,        config->args, "-set_note", false, false);
    PXOPT_LOOKUP_STR(reduction,   config->args, "-set_reduction", false, false);

    PXOPT_LOOKUP_STR(sources_path_base,   config->args, "-set_sources_path_base", false, false);

    psMetadata *skycalWhereMD = psMetadataAlloc();
    pxAddLabelSearchArgs(config, skycalWhereMD, "-select_skycal_label",   "skycalRun.label",         "LIKE");
    pxAddLabelSearchArgs(config, skycalWhereMD, "-select_skycal_data_group", "skycalRun.data_group", "LIKE");
    PXOPT_COPY_S64(config->args, skycalWhereMD, "-select_skycal_id",      "skycalRun.skycal_id",     "==");

    if (!psListLength(skycalWhereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "skycal search parameters are required");
        psFree(skycalWhereMD);
        return false;
    }

    PXOPT_COPY_STR(config->args, skycalWhereMD, "-select_skycell_id",    "stackRun.skycell_id",      "LIKE");
    PXOPT_COPY_STR(config->args, skycalWhereMD, "-select_tess_id",       "stackRun.tess_id",         "==");
    pxAddLabelSearchArgs(config, skycalWhereMD, "-select_filter",        "stackRun.filter",          "LIKE");
    if (!pxskycellAddWhere(config, skycalWhereMD)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        psFree(skycalWhereMD);
        return false;
    }

    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    psString select = pxDataGet("fftool_defineforstacks.sql");
    if (!select) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(skycalWhereMD);
        return false;
    }

    psString where = NULL;
    psString whereClause = psDBGenerateWhereConditionSQL(skycalWhereMD, NULL);
    psStringAppend(&where, "\nAND %s", whereClause);
    psStringAppend(&select, " %s", where);
    psFree(whereClause);
    psFree(skycalWhereMD);

    psString joinHook = NULL;
    if (!rerun) {
        psStringAppend(&joinHook, "\nLEFT JOIN fullForceRun ON fullForceRun.skycal_id = skycalRun.skycal_id");
        psStringAppend(&joinHook, "\n %s\nAND fullForceRun.label = '%s'", where, label);
        psStringAppend(&select, "\nAND ff_id IS NULL");
    }

    if (!p_psDBRunQueryF(config->dbh, select, joinHook)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(select);
        return false;
    }
    psFree(select);
    psFree(joinHook);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }
        psFree(where);
        return false;
    }
    if (!psArrayLength(output)) {
        psWarning("fftool: no rows found");
        psFree(output);
        psFree(where);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "toFullForce", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            psFree(where);
            return false;
        }
        psFree(output);
        psFree(where);
        return true;
    }

    psString warpQueryTemplate = pxDataGet("fftool_defineforstacks_select_warps.sql");

    for (long i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row from select
        bool status;

	if (!psDBTransaction(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
	    return false;
	}

        psS64 skycal_id = psMetadataLookupS64(&status, row, "skycal_id");

        psString path_base = NULL;
        if (sources_path_base) {
            path_base = sources_path_base;
        } else {
            path_base = psMetadataLookupStr(&status, row, "path_base");
	    psAssert(status, "failed to find skycal path_base?");
        }

        psString skycal_data_group = psMetadataLookupStr(&status, row, "data_group");

        psString query = NULL;
        psStringAppend(&query, warpQueryTemplate, skycal_id);

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
        psFree(query);

        // Find the warps for this skycell and filter combination
        psArray *warpOutput = p_psDBFetchResult(config->dbh);
        if (!warpOutput) {
            psErrorCode err = psErrorCodeLast();
            switch (err) {
                case PS_ERR_DB_CLIENT:
                    psError(PXTOOLS_ERR_SYS, false, "database error");
                case PS_ERR_DB_SERVER:
                    psError(PXTOOLS_ERR_PROG, false, "database error");
                default:
                    psError(PXTOOLS_ERR_PROG, false, "unknown error");
            }
            return false;
        }
        if (!psArrayLength(warpOutput)) {
            // no warps for this skycal. Suprise?
            psFree(warpOutput);
            psFree(query);
            continue;
        }

	// create a staticskyRun
	if (!fullForceRunInsert(config->dbh,
				0x0,	     // ff_id
                                skycal_id,
                                path_base,
				"new",	     // state
				workdir,
				label,
				data_group ? data_group : (skycal_data_group ? skycal_data_group : label),
				dist_group,
                                note,
				reduction,
				NULL        // registered
		)
	    ) {
	    if (!psDBRollback(config->dbh)) {
		psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
	    }
	    psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
	    return false;
	}

        psS64 ff_id = psDBLastInsertID(config->dbh);

        for (int j = 0; j < warpOutput->n; j++) {
            psMetadata *warpRow = warpOutput->data[j];
            psS64 warp_id = psMetadataLookupS64(&status, warpRow, "warp_id");

            if (!fullForceInputInsert(config->dbh,
				ff_id,
                                warp_id)
               ) {
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
                }
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(warpOutput);
                psFree(output);
                return false;
            }
        }

	if (!psDBCommit(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(warpOutput);
	    psFree(output);
	    return false;
	}
        psFree(warpOutput);
    }
    psFree(output);

    return true;
}

static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id",       "ff_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-label",       "fullForceRun.label",    "==");
    PXOPT_COPY_STR(config->args, where, "-data_group",  "fullForceRun.data_group",    "==");
    PXOPT_COPY_STR(config->args, where, "-state",       "fullForceRun.state",    "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",  "stackRun.skycell_id",    "==");
    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE fullForceRun JOIN skycalRun USING(skycal_id) JOIN stackRun USING(stack_id) JOIN skycell USING(tess_id, skycell_id)");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "fullForceRun", "ff_id", "fullForceResult", true, false);
    psFree(query);
    psFree(where);

    return result;
}


static bool todoMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *whereMD = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, whereMD,  "-ff_id", "ff_id", "==");
    pxAddLabelSearchArgs (config, whereMD, "-label", "fullForceRun.label", "==");
    pxskycellAddWhere(config, whereMD);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("fftool_todo.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!psListLength(whereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(whereMD);
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(whereMD, NULL);
    psStringAppend(&query, "\n AND %s", whereClause);
    psFree(whereClause);
    psFree(whereMD);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }

        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("fftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "fullForceRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);
    return true;
}

static bool addresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    // required
    PXOPT_LOOKUP_S64(ff_id, config->args, "-ff_id", true, false);
    PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);

    // optional
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", true, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(software_ver, config->args, "-software_ver", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    // XXX not sure we need a transaction here...
    if (!fullForceResultInsert(config->dbh,
			       ff_id,
                               warp_id,
                               path_base,
                               dtime_script,
                               quality,
                               hostname,
                               software_ver,
                               fault
          )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool resultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id",      "fullForceRun.ff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "fullForceResult.warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
    PXOPT_COPY_S64(config->args, where, "-skycal_id",  "fullForceRun.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",   "stackRun.stack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",      "fullForceRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "fullForceRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "stackRun.tess_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter",      "stackRun.filter", "LIKE");
    PXOPT_COPY_S16(config->args, where, "-fault",      "fullForceResult.fault", "==");
    PXOPT_COPY_S16(config->args, where, "-quality",    "fullForceResult.quality", "==");
    pxskycellAddWhere(config, where);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("fftool_result.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }

    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }

        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("fftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "fullForceResult", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool revertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id", "fullForceRun.ff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id", "fullForceResult.warp_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "fullForceRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fullForceResult.fault", "==");

    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // Delete product
    psString delete = pxDataGet("fftool_revert.sql");
    if (!delete) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&delete, " AND %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQuery(config->dbh, delete)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(delete);
        psFree(where);
        return false;
    }
    psFree(delete);

    int numRows = psDBAffectedRows(config->dbh); // Number of row affected
    psLogMsg("fftool", PS_LOG_INFO, "Deleted %d rows", numRows);

    psFree(where);
    return true;
}

static bool updateresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id",   "ff_id",   "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id",   "==");

    if (!pxSetFaultCode(config->dbh, "fullForceResult", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree (where);
    return true;
}

static bool setfullForceRunState(pxConfig *config, psS64 ff_id, const char *state)
{
    psString query = "UPDATE fullForceRun SET state = 'full' WHERE ff_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, ff_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool toadvanceMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *whereMD = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, whereMD,  "-ff_id", "ff_id", "==");
    pxAddLabelSearchArgs (config, whereMD, "-label", "fullForceRun.label", "==");
    pxskycellAddWhere(config, whereMD);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("fftool_toadvance.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!psListLength(whereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(whereMD);
        return false;
    }

    psString whereClause = NULL;
    psString temp = psDBGenerateWhereConditionSQL(whereMD, NULL);
    psStringAppend(&whereClause, "\n AND %s", temp);
    psFree(temp);
    psFree(whereMD);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    // the where clause is required and is added to the query by the "WHERE hook" format string
    if (!p_psDBRunQueryF(config->dbh, query, whereClause)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereClause);
        psFree(query);
        return false;
    }
    psFree(whereClause);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }

        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("fftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "fullForceRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);
    return true;
}

static bool addsummaryMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);
    // required
    PXOPT_LOOKUP_S64(ff_id, config->args, "-ff_id", true, false);

    // optional
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", true, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(software_ver, config->args, "-software_ver", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!fullForceSummaryInsert(config->dbh,
			       ff_id,
                               path_base,
                               dtime_script,
                               quality,
                               hostname,
                               software_ver,
                               fault
          )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!fault) {
        if (!setfullForceRunState(config, ff_id, "full")) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to change staticskyRun state");
            return false;
        }
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool revertsummaryMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id", "fullForceRun.ff_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "fullForceRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fullForceSummary.fault", "==");

    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // Delete product
    psString delete = pxDataGet("fftool_revertsummary.sql");
    if (!delete) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&delete, " AND %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQuery(config->dbh, delete)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(delete);
        psFree(where);
        return false;
    }
    psFree(delete);

    int numRows = psDBAffectedRows(config->dbh); // Number of row affected
    psLogMsg("fftool", PS_LOG_INFO, "Deleted %d rows", numRows);

    psFree(where);
    return true;
}
static bool updatesummaryMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id",   "ff_id",   "==");


    if (!pxSetFaultCode(config->dbh, "fullForceSummary", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree (where);
    return true;
}
static bool summaryMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id",      "fullForceRun.ff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "fullForceInput.warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
    PXOPT_COPY_S64(config->args, where, "-skycal_id",  "fullForceRun.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",   "stackRun.stack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",      "fullForceRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "fullForceRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "stackRun.tess_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter",     "stackRun.filter", "LIKE");
    PXOPT_COPY_S16(config->args, where, "-fault",      "fullForceSummary.fault", "==");
    pxskycellAddWhere(config, where);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("fftool_summary.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }

    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }

        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("fftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "fullForceSummary", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // PXOPT_LOOKUP_S64(det_id,  config->args, "-warp_id", true,  false);
    PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
    PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);
//    PXOPT_LOOKUP_BOOL(clean,  config->args, "-clean", false);

    FILE *f = fopen (outfile, "w");
    if (f == NULL) {
        psError(PS_ERR_UNKNOWN, false, "failed to open output file");
        return false;
    }

    if (!pxExportVersion(config, f)) {
        psError(PS_ERR_UNKNOWN, false, "failed to write dbversion output file");
        return false;
    }
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ff_id", "ff_id", "==");

    ExportTable tables [] = {
      {"fullForceRun", "fftool_export_run.sql"},
      {"fullForceInput", "fftool_export_input.sql"},
      {"fullForceResult", "fftool_export_result.sql"},
      {"fullForceSummary", "fftool_export_summary.sql"},
    };

    int numTables = sizeof(tables)/sizeof(tables[0]);

    for (int i=0; i < numTables; i++) {
      psString query = pxDataGet(tables[i].sqlFilename);
      if (!query) {
          psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
          return false;
      }

      if (where && psListLength(where->list)) {
          psString whereClause = psDBGenerateWhereSQL(where, NULL);
          psStringAppend(&query, " %s", whereClause);
          psFree(whereClause);
      }

      // treat limit == 0 as "no limit"
      if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
      }

      if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
      }
      psFree(query);

      psArray *output = p_psDBFetchResult(config->dbh);
      if (!output) {
          psError(PS_ERR_UNKNOWN, false, "database error");
          return false;
      }
      if (!psArrayLength(output)) {
        psError(PS_ERR_UNKNOWN, true, "no rows found");
        psFree(output);
        return false;
      }

      // we must write the export table in non-simple (true) format
      if (!ippdbPrintMetadatas(f, output, tables[i].tableName, true)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
      }
      psFree(output);
    }

    fclose (f);

    return true;
}

bool importrunMode(pxConfig *config)
{
  unsigned int nFail;

  int numImportTables = 3;

  char tables[3] [80] = {"fullForceInput", "fullForceResult", "fullForceSummary"};

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);

  psMetadata *input = psMetadataConfigRead (NULL, &nFail, infile, false);

#ifdef notdef
  fprintf (stderr, "---- input ----\n");
  psMetadataPrint (stderr, input, 1);
#endif

  if (!pxCheckImportVersion(config, input)) {
      psError(PS_ERR_UNKNOWN, false, "pxCheckImportVersion failed");
      return false;
  }

  psMetadataItem *item = psMetadataLookup (input, "fullForceRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  fullForceRunRow *fullForceRun = fullForceRunObjectFromMetadata (entry->data.md);
  fullForceRunInsertObject (config->dbh, fullForceRun);

  // fprintf (stdout, "---- warp run ----\n");
  // psMetadataPrint (stderr, entry->data.md, 1);

  for (int i = 0; i < numImportTables; i++) {
    item = psMetadataLookup (input, tables[i]);
    psAssert (item, "entry not in input?");
    psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

    switch (i) {
      case 0:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          fullForceInputRow *fullForceInput = fullForceInputObjectFromMetadata (entry->data.md);
          fullForceInputInsertObject (config->dbh, fullForceInput);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 1:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          fullForceResultRow *fullForceResult = fullForceResultObjectFromMetadata (entry->data.md);
          fullForceResultInsertObject (config->dbh, fullForceResult);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 2:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          fullForceSummaryRow *fullForceSummary = fullForceSummaryObjectFromMetadata (entry->data.md);
          fullForceSummaryInsertObject (config->dbh, fullForceSummary);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;
    }
  }
  return true;
}

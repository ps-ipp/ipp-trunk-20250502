/*
 * dettool_processedexp.c
 *
 * Copyright (C) 2006  Joshua Hoblitt & EAM; IfA
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

#include "dettool.h"

bool toprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_toprocessedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detPendingProcessedExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

bool addprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // det_id, exp_id, recip, -bg, -bg_stdev
    // are required
    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);
    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    // Required if fault == 0
    PXOPT_LOOKUP_STR(recipe, config->args, "-recip", (fault == 0), false);

    // optional
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F64(fringe_0, config->args, "-fringe_0", false, false);
    PXOPT_LOOKUP_F64(fringe_1, config->args, "-fringe_1", false, false);
    PXOPT_LOOKUP_F64(fringe_2, config->args, "-fringe_2", false, false);
    PXOPT_LOOKUP_F64(user_1, config->args, "-user_1", false, false);
    PXOPT_LOOKUP_F64(user_2, config->args, "-user_2", false, false);
    PXOPT_LOOKUP_F64(user_3, config->args, "-user_3", false, false);
    PXOPT_LOOKUP_F64(user_4, config->args, "-user_4", false, false);
    PXOPT_LOOKUP_F64(user_5, config->args, "-user_5", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

    psString query = pxDataGet("dettool_addprocessedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, (long long) det_id, (long long) exp_id)) {
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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    psFree(output);

    // create a new detProcessedImfile object
    detProcessedExpRow *detRow = detProcessedExpRowAlloc(
        det_id,
        exp_id,
        recipe,
        bg,
        bg_stdev,
        bg_mean_stdev,
        fringe_0,
        fringe_1,
        fringe_2,
        user_1,
        user_2,
        user_3,
        user_4,
        user_5,
        path_base,
        "full",
        fault
    );

    // insert the new row into the detProcessedImfile table
    if (!detProcessedExpInsertObject(config->dbh, detRow)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(detRow);
        return false;
    }

    psFree(detRow);

    return true;
}

bool processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM detProcessedExp");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "AND detProcessedExp.fault != 0");
    } else {
        // don't list faulted rows
        psStringAppend(&query, " %s", "AND detProcessedExp.fault = 0");
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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detProcessedExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


bool revertprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",   "fault", "==");
    
    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all-run")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("dettool_revertprocessedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detProcessedExp");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

bool updateprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);
    PXOPT_LOOKUP_STR(data_state, config->args, "-data_state", true, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args,where,"-det_id","det_id","==");
    PXOPT_COPY_S64(config->args,where,"-exp_id","exp_id","==");

    if (!setProcessedExpDataState(config, where, data_state)) {
        return false;
    }
    psFree(where);
    return true;
}

bool updatestateprocessedMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);
  // PXOPT_LOOKUP_S64(det_id,config->args, "-det_id",true,false);
  PXOPT_LOOKUP_STR(data_state,config->args, "-data_state", true, false);

  psMetadata *where = psMetadataAlloc();

  PXOPT_COPY_S64(config->args,where,"-det_id","det_id","==");
  PXOPT_COPY_S64(config->args,where,"-exp_id","exp_id","==");

  if (!setProcessedExpDataState(config,where,data_state)) {
    return(false);
  }
  if (!setProcessedImfileDataState(config,where,data_state)) {
    return(false);
  }
  psFree(where);
  return(true);
}

bool pendingcleanup_processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");

    psString query = pxDataGet("dettool_pendingcleanup_processedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detCleanupProcessedExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// XXX SQL missing
bool donecleanup_processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");

    psString query = pxDataGet("dettool_donecleanup_processedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detDoneCleanupProcessedExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


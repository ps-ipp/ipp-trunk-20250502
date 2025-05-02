/*
 * dettool_normalizedimfile.c
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

// XXX this one is inconsistent - rename 'tonormalizedimfile' and change ipp_serial_detrend.pl and detrend.norm.pro
bool tonormalizeMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_tonormalize.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detPendingNormImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

bool addnormalizedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);
    PXOPT_LOOKUP_S32(iteration, config->args, "-iteration", false, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    // Required if fault == 0
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", (fault == 0), false);

    // optional
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F64(user_1, config->args, "-user_1", false, false);
    PXOPT_LOOKUP_F64(user_2, config->args, "-user_2", false, false);
    PXOPT_LOOKUP_F64(user_3, config->args, "-user_3", false, false);
    PXOPT_LOOKUP_F64(user_4, config->args, "-user_4", false, false);
    PXOPT_LOOKUP_F64(user_5, config->args, "-user_5", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);

    if (!detNormalizedImfileInsert(
            config->dbh,
            det_id,
            iteration,
            class_id,
            uri,
            bg,
            bg_stdev,
            bg_mean_stdev,
            user_1,
            user_2,
            user_3,
            user_4,
            user_5,
            path_base,
            "full",
            fault
            )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

bool normalizedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id",  "class_id", "==");
    PXOPT_COPY_STR(config->args, where, "-recip",     "recipe", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    psString query = pxDataGet("dettool_normalizedimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detNormalizedImfile");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "AND detNormalizedImfile.fault != 0");
    } else {
        // don't list faulted rows
        psStringAppend(&query, " %s", "AND detNormalizedImfile.fault = 0");
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
    if (!ippdbPrintMetadatas(stdout, output, "detNormalizedImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

bool revertnormalizedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id",  "class_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",      "fault", "==");
    
    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all-run")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("dettool_revertnormalizedimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detNormalizedImfile");
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

bool updatenormalizedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);
    PXOPT_LOOKUP_S32(iteration, config->args, "-iteration", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id",  true, false);
    PXOPT_LOOKUP_STR(data_state, config->args, "-data_state", true, false);

    if (!setNormImfileDataState(config, det_id, iteration, class_id, data_state)) {
        return false;
    }
    return true;
}

bool pendingcleanup_normalizedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "detNormalizedImfile.det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "detNormalizedImfile.iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "detNormalizedImfile.det_id", "==");

    psString query = pxDataGet("dettool_pendingcleanup_normalizedimfile.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detCleanupNormImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


bool donecleanup_normalizedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "detNormalizedImfile.det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "detNormalizedImfile.iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "detNormalizedImfile.det_id", "==");

    psString query = pxDataGet("dettool_donecleanup_normalizedimfile.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detDoneCleanup_normalizedimfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


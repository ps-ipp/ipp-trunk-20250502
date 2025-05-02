/*
 * dettool_stack.c
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

static psArray *searchRawImfiles(pxConfig *config);

bool tostackedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_tostacked.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detPendingStackedImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// this is NOT a command-line mode : it is used by 'addstackedMode'
static psArray *searchRawImfiles(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");

    // select exp_ids from detInputExp matching det_id & iteration
    // where query should be pre-generated
    psArray *detInputExp = detInputExpSelectRowObjects(config->dbh, where, 0);
    psFree (where);

    if (!detInputExp) {
        psError(PS_ERR_UNKNOWN, false, "no rawExp rows found");
        return NULL;
    }

    // generate where query with just the exp_ids
    psMetadata *where_exp_ids = psMetadataAlloc();
    for (long i = 0; i < psArrayLength(detInputExp); i++) {
        detInputExpRow *row = detInputExp->data[i];
        if (!psMetadataAddS64(where_exp_ids, PS_LIST_TAIL, "exp_id",
                PS_META_DUPLICATE_OK, "==", row->exp_id)
        ) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
            psFree(detInputExp);
            psFree(where_exp_ids);
            return NULL;
        }
    }
    psFree(detInputExp);

    // select rawImfiles with matching exp_ids
    psArray *rawImfiles =
        rawImfileSelectRowObjects(config->dbh, where_exp_ids, 0);
    psFree(where_exp_ids);
    if (!rawImfiles) {
        psError(PS_ERR_UNKNOWN, false, "no rawImfile rows found");
        return NULL;
    }

    return rawImfiles;
}

bool addstackedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // det_id, iteration, class_id, uri, & recipe are required
    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);
    PXOPT_LOOKUP_S32(iteration, config->args, "-iteration", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    // Required if fault == 0
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", (fault == 0), false);
    PXOPT_LOOKUP_STR(recipe, config->args, "-recip", (fault == 0), false);

    // optional
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F64(user_1, config->args, "-user_1", false, false);
    PXOPT_LOOKUP_F64(user_2, config->args, "-user_2", false, false);
    PXOPT_LOOKUP_F64(user_3, config->args, "-user_3", false, false);
    PXOPT_LOOKUP_F64(user_4, config->args, "-user_4", false, false);
    PXOPT_LOOKUP_F64(user_5, config->args, "-user_5", false, false);

    // correlate the class_id against the input exposure(s)
    // searchRawImfiles defines a where clause to search by det_id and iteration
    psArray *rawImfiles = searchRawImfiles(config);

    if (!rawImfiles) {
        psError(PS_ERR_UNKNOWN, false, "failed to get rawImfiles from db");
        return false;
    }

    bool valid_class_id = false;
    if (rawImfiles) {
        for (long i = 0; i < psArrayLength(rawImfiles); i++) {
            if (!rawImfiles->data[i]) {
                fprintf (stderr, "*");
                continue;
            }
            if (strcmp(class_id, ((rawImfileRow *)rawImfiles->data[i])->class_id) == 0) {
                valid_class_id = true;
                break;
            }
        }
        psFree(rawImfiles);
    }

    if (!valid_class_id) {
        psError(PS_ERR_UNKNOWN, true,
            "class_id can not be correlated with the input exposures");
        return false;
    }

    // create a new detStackedImfile object
    detStackedImfileRow *stackedImfile = detStackedImfileRowAlloc(
            det_id,
            iteration,
            class_id,
            uri,
            recipe,
            bg,
            bg_stdev,
            bg_mean_stdev,
            user_1,
            user_2,
            user_3,
            user_4,
            user_5,
            "full",
            fault
        );

    // insert the new row into the detProcessedImfile table
    if (!detStackedImfileInsertObject(config->dbh, stackedImfile)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(stackedImfile);
        return false;
    }

    psFree(stackedImfile);

    return true;
}

bool stackedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id",  "class_id", "==");
    PXOPT_COPY_STR(config->args, where, "-recip",     "recipe", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_stacked.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detStackedImfile");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "AND detStackedImfile.fault != 0");
    } else {
        // don't list faulted rows
        psStringAppend(&query, " %s", "AND detStackedImfile.fault = 0");
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
    if (!ippdbPrintMetadatas(stdout, output, "rawImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


bool revertstackedMode(pxConfig *config)
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

    psString query = pxDataGet("dettool_revertstacked.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detStackedImfile");
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

bool updatestackedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);
    PXOPT_LOOKUP_S32(iteration, config->args, "-iteration", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
    PXOPT_LOOKUP_STR(data_state, config->args, "-data_state", true, false);

    if (!setStackedImfileDataState(config, det_id, iteration, class_id, data_state)) {
        return false;
    }
    return true;
}

bool pendingcleanup_stackedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "detStackedImfile.det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "detStackedImfile.iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "detStackedImfile.class_id", "==");

    psString query = pxDataGet("dettool_pendingcleanup_stacked.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detCleanupStackedImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// XXX SQL missing
bool donecleanup_stackedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "detStackedImfile.det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "detStackedImfile.iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "detStackedImfile.class_id", "==");

    psString query = pxDataGet("dettool_donecleanup_stacked.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detDoneCleanup_stacked", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


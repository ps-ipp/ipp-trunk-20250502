/*
 * dettool_correction.c
 *
 * Copyright (C) 2006-2007  Joshua Hoblitt
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

bool makecorrectionMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(ref_det_id, config->args, "-det_id", true, false); // required
    PXOPT_LOOKUP_S32(ref_iter, config->args, "-iteration", true, false); // required
    PXOPT_LOOKUP_STR(det_type, config->args, "-det_type", true, false); // required
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false); // optional

    // optional modifications
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);

    // build the needed where
    psMetadata *where = psMetadataAlloc();
    psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", ref_det_id);
    psMetadataAddS32(where, PS_LIST_TAIL, "iteration", 0, "==", ref_iter);

    // select the detRun that matches
    psArray *runs = detRunSelectRowObjects(config->dbh, where, 1);
    psFree (where);

    if (!psArrayLength(runs)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    detRunRow *detRun = runs->data[0];

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    char *use_workdir   = (workdir)   ? workdir   : detRun->workdir;
    char *use_reduction = (reduction) ? reduction : detRun->reduction;
    char *use_label     = (label)     ? label     : detRun->label;

    detRunInsert(config->dbh,
         0,             // det_id
         0,             // iteration
         det_type,
         "correction",  // mode
         "run",         // state
         detRun->filelevel,
         use_workdir,
         detRun->camera,
         detRun->telescope,
         detRun->exp_type,
         use_reduction,
         detRun->filter,
         detRun->airmass_min,
         detRun->airmass_max,
         detRun->exp_time_min,
         detRun->exp_time_max,
         detRun->ccd_temp_min,
         detRun->ccd_temp_max,
         detRun->posang_min,
         detRun->posang_max,
         detRun->registered,
         detRun->time_begin,
         detRun->time_end,
         detRun->use_begin,
         detRun->use_end,
         detRun->solang_min,
         detRun->solang_max,
         use_label,
         detRun->det_id, // ref_det_id
         detRun->iteration // ref_iter
    );
    psFree(runs);

    // print the new detRun
    psS64 new_det_id = psDBLastInsertID(config->dbh);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psMetadata *where_new = psMetadataAlloc();
    psMetadataAddS64(where_new, PS_LIST_TAIL, "det_id", 0, "==", new_det_id);
    psArray *detRuns = psDBSelectRows(config->dbh, "detRun", where_new, 0);
    psFree(where_new);

    if (!detRuns) {
        psError(PS_ERR_UNKNOWN, false, "can't find the detRun we just created");
        return false;
    }
    // sanity check results
    if (psArrayLength(detRuns) != 1) {
        psAbort("found more then one detRun matching det_id %" PRId64 "(this should not happen)", new_det_id);
        return false;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, detRuns, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(detRuns);
        return false;
    }
    psFree(detRuns);

    return true;
}

bool tocorrectimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
    PXOPT_COPY_STR(config->args, where, "-det_type", "det_type", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_tocorrectimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        // NOTE the SQL uses an intermediate table 'det1' for this query
        psString whereClause = psDBGenerateWhereConditionSQL(where, "det1");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree (where);

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
    if (!ippdbPrintMetadatas(stdout, output, "detPendingCorrectImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

bool tocorrectexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_tocorrectexp.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// NOTE : the command-line args are parsed by register_detrend_imfileMode
bool addcorrectimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // insert the row into detRegisterImfile
    if (!register_detrend_imfileMode(config)) {
        return false;
    }

    // automatically stop completed 'correct' detRuns
    psString query = pxDataGet("dettool_stop_completed_correct_runs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

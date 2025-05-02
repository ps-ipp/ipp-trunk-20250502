/*
 * detselect.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "pxtools.h"
#include "detselect.h"

static bool searchMode(pxConfig *config);
static bool selectMode(pxConfig *config);
static bool showMode(pxConfig *config);


# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = detselectConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(DETSELECT_MODE_SEARCH,        searchMode);
        MODECASE(DETSELECT_MODE_SELECT,        selectMode);
	MODECASE(DETSELECT_MODE_SHOW,          showMode);
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

# define PXOPT_COPY_NULLTEST_F32(from, to, oldname, newname, comment) \
{ \
    bool status = false; \
    psF32 var = psMetadataLookupF32(&status, from, oldname); \
    if (!status) { \
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for " oldname); \
        return false; \
    } \
    if (!isnan(var)) { \
        if (!psMetadataAddF32(to, PS_LIST_TAIL, newname, PS_META_DUPLICATE_OK, comment, var)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add item " newname); \
            psFree(to); \
            return false; \
        } \
        if (!psMetadataAddTime(to, PS_LIST_TAIL, newname, PS_META_DUPLICATE_OK, "==", NULL)) { \
            psError(PS_ERR_UNKNOWN, false, "failed to add NULL test " newname); \
            psFree(to); \
            return false; \
        } \
    } \
}

static bool searchMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(unlimit, config->args, "-unlimit", false);

    PXOPT_LOOKUP_TIME(time, config->args, "-time", false, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-inst",      "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where, "-det_type",  "det_type", "==");
    PXOPT_COPY_STR(config->args, where, "-type",      "det_type", "==");
    PXOPT_COPY_STR(config->args, where, "-filter",    "filter", "==");

    // airmass_min  < airmass  < airmass_max
    PXOPT_COPY_NULLTEST_F32(config->args, where, "-airmass", "airmass_min", "<=");
    PXOPT_COPY_NULLTEST_F32(config->args, where, "-airmass", "airmass_max", ">=");

    // exp_time_min < exp_time < exp_time_max
    PXOPT_COPY_NULLTEST_F32(config->args, where, "-exp_time", "exp_time_min", "<=");
    PXOPT_COPY_NULLTEST_F32(config->args, where, "-exp_time", "exp_time_max", ">=");

    // ccd_temp_min < ccd_temp < ccd_temp_max
    PXOPT_COPY_NULLTEST_F32(config->args, where, "-ccd_temp", "ccd_temp_min", "<=");
    PXOPT_COPY_NULLTEST_F32(config->args, where, "-ccd_temp", "ccd_temp_max", ">=");

    PXOPT_COPY_F64(config->args, where, "-posang", "posang_min", "<=");
    PXOPT_COPY_F64(config->args, where, "-posang", "posang_max", ">=");

    // time_begin    < time     < time_end
    // the == NULL tests invokes some psDB magic to make an OR
    // conditional query
    psMetadataAddTime(where, PS_LIST_TAIL, "time_begin", 0, "<=", time);
    psMetadataAddTime(where, PS_LIST_TAIL, "time_begin", PS_META_DUPLICATE_OK, "==", NULL);
    psMetadataAddTime(where, PS_LIST_TAIL, "time_end", 0, ">=", time);
    psMetadataAddTime(where, PS_LIST_TAIL, "time_end", PS_META_DUPLICATE_OK, "==", NULL);

    psString query = pxDataGet("detselect_search.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereConditionalSQL with AND ... because the SQL ends in a WHERE
    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // we choose the single detrend image which matches all criteria and has
    // the latest insertion date

    // unless explicitly specified by the user, list all possible matches
    if (!unlimit) {
        psStringAppend(&query, " ORDER BY registered DESC, iteration DESC LIMIT 1");
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
        // XXX check psError here
        psError(PS_ERR_UNKNOWN, false, "no detrend exposures found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool selectMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id",  "class_id", "==");

    psString query = pxDataGet("detselect_select.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereConditionalSQL with AND ... because the SQL ends in a WHERE
    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
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

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        // XXX check psError here
        psError(PS_ERR_UNKNOWN, false, "no detNormalizedImfile rows found");
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

static bool showMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-inst",      "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where, "-det_type",  "det_type", "==");
    PXOPT_COPY_STR(config->args, where, "-type",      "det_type", "==");

    psString query = pxDataGet("detselect_show.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereConditionalSQL with AND ... because the SQL ends in a WHERE
    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // we choose the single detrend image which matches all criteria and has
    // the latest insertion date

    psStringAppend(&query, " ORDER BY registered DESC, iteration DESC");

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
        // XXX check psError here
        psError(PS_ERR_UNKNOWN, false, "no detRun rows found");
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
    return(true);
}

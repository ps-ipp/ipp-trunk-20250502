/*
 * pstamptool.c
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
#include "pstamptool.h"
#include "pstamp.h"

static bool adddatastoreMode(pxConfig *config);
static bool datastoreMode(pxConfig *config);
static bool moddatastoreMode(pxConfig *config);
static bool addreqMode(pxConfig *config);
static bool completedreqMode(pxConfig *config);
static bool listreqMode(pxConfig *config);
static bool pendingreqMode(pxConfig *config);
static bool updatereqMode(pxConfig *config);
static bool revertreqMode(pxConfig *config);
static bool pendingcleanupMode(pxConfig *config);
static bool addjobMode(pxConfig *config);
static bool listjobMode(pxConfig *config);
static bool pendingjobMode(pxConfig *config);
static bool updatejobMode(pxConfig *config);
static bool stopdependentjobMode(pxConfig *config);
static bool revertjobMode(pxConfig *config);

static bool addprojectMode(pxConfig *config);
static bool projectMode(pxConfig *config);
static bool modprojectMode(pxConfig *config);
static bool getdependentMode(pxConfig *config);
static bool pendingdependentMode(pxConfig *config);
static bool updatedependentMode(pxConfig *config);
static bool revertdependentMode(pxConfig *config);
static bool getwebrequestnumMode(pxConfig *config);
static bool addfileMode(pxConfig *config);
static bool listfileMode(pxConfig *config);
static bool deletefileMode(pxConfig *config);

static bool adddomainMode(pxConfig * config);
static bool updatedomainMode(pxConfig * config);
static bool listdomainMode(pxConfig *config);
static bool adduserMode(pxConfig * config);
static bool updateuserMode(pxConfig * config);
static bool listuserMode(pxConfig * config);
static bool addaccesslevelMode(pxConfig * config);
static bool updateaccesslevelMode(pxConfig * config);
static bool listaccesslevelMode(pxConfig * config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
                goto FAIL; \
            } \
    break;

// XXX make this a configurable parameter
#define PSTAMP_MAX_JOB_FAULTS 5
#define PSTAMP_MAX_DEP_FAULTS 5

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pstamptoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(PSTAMPTOOL_MODE_ADDDATASTORE, adddatastoreMode);
        MODECASE(PSTAMPTOOL_MODE_DATASTORE, datastoreMode);
        MODECASE(PSTAMPTOOL_MODE_MODDATASTORE, moddatastoreMode);
        MODECASE(PSTAMPTOOL_MODE_ADDREQ, addreqMode);
        MODECASE(PSTAMPTOOL_MODE_COMPLETEDREQ, completedreqMode);
        MODECASE(PSTAMPTOOL_MODE_LISTREQ, listreqMode);
        MODECASE(PSTAMPTOOL_MODE_PENDINGREQ, pendingreqMode);
        MODECASE(PSTAMPTOOL_MODE_UPDATEREQ, updatereqMode);
        MODECASE(PSTAMPTOOL_MODE_REVERTREQ, revertreqMode);
        MODECASE(PSTAMPTOOL_MODE_PENDINGCLEANUP, pendingcleanupMode);
        MODECASE(PSTAMPTOOL_MODE_ADDJOB, addjobMode);
        MODECASE(PSTAMPTOOL_MODE_LISTJOB, listjobMode);
        MODECASE(PSTAMPTOOL_MODE_PENDINGJOB, pendingjobMode);
        MODECASE(PSTAMPTOOL_MODE_UPDATEJOB, updatejobMode);
        MODECASE(PSTAMPTOOL_MODE_STOPDEPENDENTJOB, stopdependentjobMode);
        MODECASE(PSTAMPTOOL_MODE_REVERTJOB, revertjobMode);
        MODECASE(PSTAMPTOOL_MODE_ADDPROJECT, addprojectMode);
        MODECASE(PSTAMPTOOL_MODE_MODPROJECT, modprojectMode);
        MODECASE(PSTAMPTOOL_MODE_PROJECT, projectMode);
        MODECASE(PSTAMPTOOL_MODE_GETDEPENDENT, getdependentMode);
        MODECASE(PSTAMPTOOL_MODE_PENDINGDEPENDENT, pendingdependentMode);
        MODECASE(PSTAMPTOOL_MODE_UPDATEDEPENDENT, updatedependentMode);
        MODECASE(PSTAMPTOOL_MODE_REVERTDEPENDENT, revertdependentMode);
        MODECASE(PSTAMPTOOL_MODE_GETWEBREQUESTNUM, getwebrequestnumMode);
        MODECASE(PSTAMPTOOL_MODE_ADDFILE, addfileMode);
        MODECASE(PSTAMPTOOL_MODE_LISTFILE, listfileMode);
        MODECASE(PSTAMPTOOL_MODE_DELETEFILE, deletefileMode);

        MODECASE(PSTAMPTOOL_MODE_ADDDOMAIN, adddomainMode);
        MODECASE(PSTAMPTOOL_MODE_UPDATEDOMAIN, updatedomainMode);
        MODECASE(PSTAMPTOOL_MODE_LISTDOMAIN, listdomainMode);
        MODECASE(PSTAMPTOOL_MODE_ADDUSER, adduserMode);
        MODECASE(PSTAMPTOOL_MODE_UPDATEUSER, updateuserMode);
        MODECASE(PSTAMPTOOL_MODE_LISTUSER, listuserMode);
        MODECASE(PSTAMPTOOL_MODE_ADDACCESSLEVEL, addaccesslevelMode);
        MODECASE(PSTAMPTOOL_MODE_UPDATEACCESSLEVEL, updateaccesslevelMode);
        MODECASE(PSTAMPTOOL_MODE_LISTACCESSLEVEL, listaccesslevelMode);

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

static bool adddatastoreMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(uri,         config->args, "-set_uri",           true, false);
    PXOPT_LOOKUP_STR(outProduct,  config->args, "-set_out_product",   true, false);
    PXOPT_LOOKUP_STR(lastFileset, config->args, "-set_last_fileset", false, false);
    PXOPT_LOOKUP_STR(state,       config->args, "-set_state",         false, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label",         false, false);
    PXOPT_LOOKUP_S32(pollInterval, config->args, "-set_poll_interval",false, false);

    if (!pstampDataStoreInsert(config->dbh,
            0,
            state,
            lastFileset,
            NULL,       // timestamp
            label,      // label
            outProduct,
            uri,
            pollInterval,
            false
        )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool datastoreMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-ds_id", "ds_id", "==");

    PXOPT_LOOKUP_BOOL(ready, config->args, "-ready", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pstamptool_datastore.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);
    if (ready) {
        psStringAppend(&query, " %s", "\nAND TIMESTAMPDIFF(SECOND, timestamp, now()) > pollInterval");
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampDataStore", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool moddatastoreMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(ds_id,       config->args, "-ds_id",         true, false);
    PXOPT_LOOKUP_STR(lastFileset, config->args, "-set_last_fileset",  false, false);
    PXOPT_LOOKUP_STR(uri, config->args,         "-set_uri",  false, false);
    PXOPT_LOOKUP_STR(state,       config->args, "-set_state",         false, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label",         false, false);
    PXOPT_LOOKUP_S32(pollInterval, config->args, "-set_poll_interval",         false, false);
    PXOPT_LOOKUP_BOOL(update_timestamp, config->args, "-update_timestamp", false);

    if (!state && !lastFileset && !pollInterval && !update_timestamp && !label &&!uri) {
        psError(PS_ERR_UNKNOWN, true, "at least one of -last_fileset or -set_state is required");
        return false;
    }

    char *query = psStringCopy ("UPDATE pstampDataStore SET timestamp = CURRENT_TIMESTAMP() ");

    if (lastFileset) {
        psStringAppend(&query, " , lastFileset = '%s'", lastFileset);
    }

    if (uri) {
        psStringAppend(&query, ", uri = '%s'", uri);
    }
    if (state) {
        psStringAppend(&query, ", state = '%s'", state);
    }

    if (label) {
        psStringAppend(&query, ", label = '%s'", label);
    }

    if (pollInterval) {
        psStringAppend(&query, ", pollInterval = %d", pollInterval);
    }

    psStringAppend(&query, " WHERE ds_id = %" PRId64, ds_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    if (affected != 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected one row but %"
                                        PRIu64 " rows were modified", affected);
        return false;
    }

    return true;
}

static bool addreqMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(uri,         config->args, "-uri",   true, false);
    PXOPT_LOOKUP_STR(name,        config->args, "-name",  false, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-label",  false, false);
    PXOPT_LOOKUP_S64(ds_id,       config->args, "-ds_id", false, false);

    PXOPT_LOOKUP_STR(username,    config->args, "-username",  false, false);
    PXOPT_LOOKUP_S64(proj_id,     config->args, "-proj_id", false, false);

    psTime *now = psTimeGetNow(PS_TIME_TAI);

    if (!pstampRequestInsert(config->dbh,
        0,      // req_id
        ds_id,
        "new",  //state
        name,
        NULL,   // reqType
        label,
        NULL,   // outProduct
        uri,
        NULL,   // outdir
        username,
        proj_id,
        now,    // registered
        now,    // timestamp
        0       // fault
        )) {
        psError(PS_ERR_UNKNOWN, false, "failed to insert request");
        return false;
    }

    psS64 req_id = psDBLastInsertID(config->dbh);

    printf("%" PRId64 "\n", req_id);

    return true;
}

static bool pendingreqMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "label", "LIKE");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pstamptool_pendingreq.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "pstampRequest");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, " ORDER BY priority DESC, req_id");

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
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampRequest", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingcleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "label", "LIKE");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pstamptool_pendingcleanup.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "pstampRequest");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, " ORDER BY priority DESC, req_id");

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
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampRequest", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool listreqMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-not_req_id", "req_id", "!=");
    PXOPT_COPY_STR(config->args, where, "-name", "name", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-username", "username", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "LIKE");

    PXOPT_LOOKUP_U64(limit,   config->args, "-limit",  false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!psListLength(where->list)) {
        psError(PS_ERR_UNKNOWN, true, "search paramters are required");
        return false;
    }

    psString query = psStringCopy("SELECT * from pstampRequest");

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
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
        psTrace("pstamptool", PS_LOG_INFO, "request not found");
        // This causes main to exit with PS_EXIT_DATA_ERROR which the pstamp scripts are looking for
        psError(PXTOOLS_ERR_CONFIG, true, "request not found");
        psFree(output);
        // we return false so that the caller can easily determine that a request does not exist
        return false;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampRequest", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool completedreqMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs(config, where, "-label", "label", "LIKE");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pstamptool_completedreq.sql");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, " ORDER BY priority DESC, req_id");

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

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!psArrayLength(output)) {
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampRequest", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool updatereqMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // PXOPT_LOOKUP_S64(req_id,     config->args, "-req_id",   true, false);
    PXOPT_LOOKUP_STR(state,      config->args, "-set_state",      false, false);
    PXOPT_LOOKUP_STR(label,      config->args, "-set_label",      false, false);
    PXOPT_LOOKUP_STR(outProduct, config->args, "-set_outProduct", false, false);
    PXOPT_LOOKUP_S16(fault,      config->args, "-set_fault",      false, false);
    PXOPT_LOOKUP_STR(uri,        config->args, "-set_uri",        false, false);
    PXOPT_LOOKUP_STR(outdir,     config->args, "-set_outdir",     false, false);
    PXOPT_LOOKUP_STR(name,       config->args, "-set_name",       false, false);
    PXOPT_LOOKUP_STR(username,   config->args, "-set_username",   false, false);
    PXOPT_LOOKUP_STR(reqType,    config->args, "-set_reqType",    false, false);
    PXOPT_LOOKUP_BOOL(clearfault,config->args, "-clearfault",     false);

    if (!state && !label && !outProduct && !fault && !uri && !outdir && !name && !username && !reqType && !clearfault) {
        psError(PS_ERR_UNKNOWN, true, "at least one set option is required");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-req_id",     "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id_max", "req_id", "<=");
    PXOPT_COPY_S32(config->args, where, "-fault",      "fault", "==");
    PXOPT_COPY_STR(config->args, where, "-state",      "state", "==");
    PXOPT_COPY_STR(config->args, where, "-reqType",    "reqType", "==");
    PXOPT_COPY_STR(config->args, where, "-name",       "name", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-username",   "username", "==");
    PXOPT_COPY_TIME(config->args, where, "-timestamp_begin", "timestamp", ">=");
    PXOPT_COPY_TIME(config->args, where, "-timestamp_end", "timestamp", "<=");
    pxAddLabelSearchArgs(config, where, "-label",      "pstampRequest.label", "LIKE");
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE pstampRequest SET timestamp = UTC_TIMESTAMP()");

    psString stateCheck = NULL;
    if (state) {
        psStringAppend(&query, ", state = '%s'", state);
        if (!strcmp(state, "goto_cleaned")) {
            psStringAppend(&stateCheck, " AND (state != 'cleaned' AND state != 'goto_cleaned')");
        }
    }
    if (label) {
        psStringAppend(&query, ", label = '%s'", label);
    }
    if (outProduct) {
        psStringAppend(&query, ", outProduct = '%s'", outProduct);
    }
    if (outdir) {
        psStringAppend(&query, ", outdir = '%s'", outdir);
    }
    if (username) {
        psStringAppend(&query, ", username = '%s'", username);
    }
    if (clearfault) {
        if (fault) {
            psError(PXTOOLS_ERR_CONFIG, true, "only one of -fault and -clearfault is allowed");
            return false;
        }
        psStringAppend(&query, ", fault = 0");
    } else if (fault) {
        psStringAppend(&query, ", fault = %d", fault);
    }
    if (uri) {
        psStringAppend(&query, ", uri = '%s'", uri);
    }
    if (name) {
        psStringAppend(&query, ", name = '%s'", name);
    }
    if (reqType) {
        psStringAppend(&query, ", reqType = '%s'", reqType);
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nWHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (stateCheck) {
        psStringAppend(&query, "%s", stateCheck);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    // psU64 affected = psDBAffectedRows(config->dbh);
    // psLogMsg("pstamptool", PS_LOG_INFO, "Updated %" PRIu64 " pstampRequests", affected);

    return true;
}

static bool revertreqMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fault", "pstampRequest.fault", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "pstampRequest.state", "==");
    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "LIKE");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psFree(where);

    // delete any jobs that were queued by requests that didn't complete parsing (pstampRequest.state = 'new'
    // If state =  'run' was supplied this will be a no-op
    psString query = pxDataGet("pstamptool_revertreq_deletejobs.sql");
    psStringAppend(&query, " AND %s", whereClause);
    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);

    // clear fault for requests
    query = pxDataGet("pstamptool_revertreq.sql");
    psStringAppend(&query, " AND %s", whereClause);

    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);

    return true;
}

static bool addjobMode(pxConfig *config)
{
    bool stampJob = false;

    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(req_id,      config->args, "-req_id",     true, false);
    PXOPT_LOOKUP_STR(rownum,      config->args, "-rownum",     true, false);
    PXOPT_LOOKUP_STR(job_type,    config->args, "-job_type",   false, false);
    PXOPT_LOOKUP_STR(outputBase,  config->args, "-outputBase", false,  false);
    PXOPT_LOOKUP_STR(stateString, config->args, "-state",      false, false);
    PXOPT_LOOKUP_S16(fault,       config->args, "-fault",      false, false);
    PXOPT_LOOKUP_S64(exp_id,      config->args, "-exp_id",     false, false);
    PXOPT_LOOKUP_S64(options,     config->args, "-options",    false, false);
    PXOPT_LOOKUP_S64(dep_id,      config->args, "-dep_id",     false, false);
    PXOPT_LOOKUP_S64(parent_id,   config->args, "-parent_id",  false, false);
    PXOPT_LOOKUP_BOOL(is_parent,  config->args, "-is_parent",  false);

    // unless the job is being inserted with stop state require outputBase
    if (strcmp(stateString, "stop") && !outputBase) {
        psError(PS_ERR_UNKNOWN, true, "-outputBase is required");
        return false;
    }

    // default value for job_type is defined in pstamptoolConfig.c
    if (!strcmp(job_type, "get_image") || !strcmp(job_type, "detect_query") || !strcmp(job_type, "none")) {
        stampJob = false;
    } else if (!strcmp(job_type, "child")) {
        // job_type child's only action is to resolve a dependent
        // XXX: IS this necessary?
        if (!dep_id) {
            psError(PS_ERR_UNKNOWN, true, "dep_id required for child job\n");
            return false;
        }
        if (is_parent) {
            psError(PS_ERR_UNKNOWN, true, "job type child can not be a parent job\n");
            return false;
        }
        stampJob = false;
    } else if (!strcmp(job_type, "mosaic")) {
        stampJob = false;
        is_parent = true;
    } else if (!strcmp(job_type, "stamp")) {
        stampJob = true;
    } else {
        psError(PS_ERR_UNKNOWN, false, "unknown value for -job_type: %s", job_type);
        return false;
    }
    if (stampJob) { /* do something?? */ }

    if (!pstampJobInsert(config->dbh,
            0,          // job_id
            req_id,
            rownum,
            stateString,
            job_type,
            fault,
            exp_id,
            outputBase,
            options,
            dep_id,
            0,          // fault_count
            parent_id,
            is_parent
            )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    if (affected != 1) {
        psError(PS_ERR_UNKNOWN, false,
            "should have affected one row but %" PRIu64 " rows were modified",
            affected);
        return false;
    }

    psS64 job_id = psDBLastInsertID(config->dbh);
    printf("%" PRId64 "\n", job_id);

    return true;
}

static bool listjobMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dep_id", "dep_id", "==");
    PXOPT_COPY_STR(config->args, where, "-state",  "state", "==");
    PXOPT_COPY_STR(config->args, where, "-jobType", "jobType", "==");
    PXOPT_COPY_S64(config->args, where, "-fault",  "fault", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!psListLength(where->list)) {
        fprintf(stderr, "search arguments are required\n");
        exit (1);
    }

    psString query = pxDataGet("pstamptool_listjob.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "pstampJob");
        psStringAppend(&query, " WHERE %s", whereClause);
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
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampJob", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingjobMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "LIKE");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pstamptool_pendingjob.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    psString whereStr = NULL;
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereStr, "\n AND %s", whereClause);
        psFree(whereClause);
    } else {
        psStringAppend(&whereStr, "%s", "");
    }
    psFree(where);

    psStringAppend(&query, " ORDER BY priority DESC, req_id, job_id");

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereStr, whereStr)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereStr);
        psFree(query);
        return false;
    }
    psFree(whereStr);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampJob", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool updatejobMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(job_id,    config->args, "-job_id", false, false);
    PXOPT_LOOKUP_S64(req_id,    config->args, "-req_id", false, false);
    PXOPT_LOOKUP_S64(dep_id,    config->args, "-dep_id", false, false);
    PXOPT_LOOKUP_S32(fault_count, config->args, "-fault_count",  false, false);

    if (!job_id && !req_id && !dep_id && !fault_count) {
        psError(PS_ERR_UNKNOWN, true, "at least one of -job_id -req_id -dep_id or -fault_count is required");
        return false;
    }

    PXOPT_LOOKUP_STR(state,     config->args, "-set_state",  false, false);
    PXOPT_LOOKUP_S32(fault,     config->args, "-set_fault",  false, false);
    
    if (!state && !fault) {
        psError(PS_ERR_UNKNOWN, true, "at least one of -set_state and -set_fault is required");
        return false;
    }

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dep_id", "dep_id", "==");
    PXOPT_COPY_S32(config->args, where, "-fault",  "pstampJob.fault", "==");
    PXOPT_COPY_STR(config->args, where, "-state",  "pstampJob.state", "==");
    PXOPT_COPY_S32(config->args, where, "-fault_count", "pstampJob.fault_count", ">=");
    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "LIKE");

    psString query = pxDataGet("pstamptool_updatejob.sql");

    char c = ' ';
    if (state) {
        psStringAppend(&query, "\n %c pstampJob.state = '%s'", c, state);
        c = ',';
    }
    if (fault) {
        psStringAppend(&query, "\n %c pstampJob.fault = %d", c, fault);
        c = ',';
        psStringAppend(&query, ", pstampJob.fault_count = pstampJob.fault_count+ 1");
    }
    
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("pstamptool", PS_LOG_INFO, "Updated %" PRIu64 " pstampJobs", affected);

    return true;
}
// Terminate jobs which have dependents setting both the pstampDependent and pstampJob.fault
static bool stopdependentjobMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S32(fault,  config->args, "-set_fault",  true, false);
    PXOPT_LOOKUP_STR(state,  config->args, "-set_state",  false, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dep_id", "dep_id", "==");

    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-component", "component", "==");

    PXOPT_COPY_S32(config->args, where, "-fault",  "pstampDependent.fault", "==");

    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "LIKE");
    PXOPT_COPY_S32(config->args, where, "-fault_count", "pstampDependent.fault_count", ">=");

    // XXX: How about selecting by pstampRequest.label? No. That is too dangerous by itself.

    psString query = pxDataGet("pstamptool_stopdependentjob.sql");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, fault, fault, state)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("pstamptool", PS_LOG_INFO, "Updated %" PRIu64 " pstampJobs and pstampDependents", affected);

    return true;
}

static bool revertjobMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fault",  "pstampJob.fault", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id_min",  "req_id", ">=");
    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "LIKE");

    PXOPT_LOOKUP_BOOL(clear_fault_count, config->args, "-clear_fault_count", false);

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    // XXX: we don't actually use -limit. It doesn't work for UPDATE
    // it's an allowed arg because add_poll_args adds it
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    if (limit) { /* do something?? */ }

    PXOPT_LOOKUP_S16(fault, config->args, "-fault",  false, false);

    // By default only revert faults < PSTAMP_FIRST_ERROR_CODE which are our "ipp exit codes"
    // codes larger than that are the pstamp request interface.
    // Don't fault those unless -fault was explicitly provided
    psString faultClause = psStringCopy("");
    if (!fault) {
	psStringAppend(&faultClause, " \nAND (pstampJob.fault < %d)", PSTAMP_FIRST_ERROR_CODE);
    }

    psString query = pxDataGet("pstamptool_revertjob.sql");
    if (!psListLength(where->list) && !all) {
	psFree(where);
	psError(PXTOOLS_ERR_CONFIG, false, "search parameters or -all are required");
	return false;
    }
    if (psListLength(where->list)) {
	psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
	psStringAppend(&query, " AND %s", whereClause);
	psFree(whereClause);
    }
    psFree(where);

    // don't keep reverting once the number of faults reaches some value unless 
    // a parameter asking us to clear that count is provided
    psString faultCountClause = NULL;
    if (clear_fault_count) {
	psStringAppend(&faultCountClause, "\n, pstampJob.fault_count = 0");
    } else {
	psStringAppend(&query, " AND pstampJob.fault_count < %d", PSTAMP_MAX_JOB_FAULTS);
    }

    if (!p_psDBRunQueryF(config->dbh, query, faultCountClause, faultClause)) {
	psFree(faultCountClause);
	psFree(faultClause);
	psFree(query);
	psError(PS_ERR_UNKNOWN, false, "database error");
	return false;
    }

    psFree(faultClause);
    psFree(faultCountClause);
    psFree(query);

    return true;
}

# if (1)
// these are unused functions
// XXX: yes they are
static bool addprojectMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(name,  config->args, "-name", true, false);
    PXOPT_LOOKUP_STR(state,  config->args, "-state", false, false);
    PXOPT_LOOKUP_STR(imagedb, config->args, "-imagedb", true, false);
    PXOPT_LOOKUP_STR(dvodb,  config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_BOOL(need_magic, config->args, "-need_magic", false);

    if (!pstampProjectInsert(config->dbh,
			     0,
			     name,
			     state,
			     imagedb,
			     dvodb,
			     camera,
			     telescope,
			     need_magic
	    )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool projectMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-name", "name", "==");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("pstamptool_project.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampProject", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool modprojectMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(proj_id,    config->args, "-proj_id", true, false);
    PXOPT_LOOKUP_STR(state,  config->args, "-state", true, false);

    char *query = psStringCopy ("UPDATE pstampProject SET");

    psStringAppend(&query, " state = '%s'", state);

    psStringAppend(&query, " WHERE proj_id = %" PRId64, proj_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    if (affected != 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected one row but %"
                                        PRIu64 " rows were modified", affected);
        return false;
    }

    return true;
}

static bool getdependentMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(stage,       config->args, "-stage",   true, false);
    PXOPT_LOOKUP_S64(stage_id,    config->args, "-stage_id", true, false);
    PXOPT_LOOKUP_STR(component,   config->args, "-component",  true, false);
    PXOPT_LOOKUP_STR(imagedb,     config->args, "-imagedb",  true, false);
    PXOPT_LOOKUP_STR(rlabel,      config->args, "-rlabel",  true, false);
    PXOPT_LOOKUP_STR(outdir,      config->args, "-outdir",  true, false);
    PXOPT_LOOKUP_BOOL(need_magic, config->args, "-need_magic", false);
    PXOPT_LOOKUP_BOOL(no_create,  config->args, "-no_create", false);
    PXOPT_LOOKUP_BOOL(hold,       config->args, "-hold", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-imagedb", "imagedb", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-component", "component", "==");

    // start a transaction early so it will contain any row level locks
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psString query = pxDataGet("pstamptool_getdependent.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    if (!no_create) {
        // This will lock the row until the transaction is committed
        psStringAppend(&query, " AND %s FOR UPDATE", whereClause);
    } else {
        psStringAppend(&query, " AND %s", whereClause);
    }
    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    if (psArrayLength(output)) {
        psMetadata *dep = output->data[0];
        psS64 dep_id = psMetadataLookupS64(NULL, dep, "dep_id");
        if (!dep_id) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psS32 fault = psMetadataLookupS64(NULL, dep, "fault");
        if (fault > 0) {
            fprintf(stderr, "existing dependent has fault %d\n", fault);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            // return the fault to the client. This is used by the postage stamp parser
            exit (fault);
        }
        bool commit = false;
        // Check the state of the exisiting dependent. By query it is either
        // new or hold
        psString state = psMetadataLookupStr(NULL, dep, "state");
        if (!hold && !strcmp(state, "hold")) {
            // There is a dependent for this component but it's state is hold.
            // This client needs one that will run.
            // Update the state
            psString updateQuery = NULL;
            psStringAppend(&updateQuery, "UPDATE pstampDependent SET state = 'new' "
                "\nWHERE stage = '%s' AND imagedb = '%s' AND stage_id = %"PRId64 " AND component = '%s'", 
                    stage, imagedb, stage_id, component);

            if (!p_psDBRunQuery(config->dbh, updateQuery)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(updateQuery);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            // set flag to commit this change
            commit = true;
            psFree(updateQuery);
        }
        // print the dep_id for the user
        printf("%" PRId64 "\n", dep_id);
        psFree(output);
        // now either commit the change or rollback the transaction which releases the lock
        if (commit) {
            if (!psDBCommit(config->dbh)) {
                // rollback
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
            }
        } else if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }
    if (no_create) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }
    // no existing dependent that matches, insert one
    // Since we have multiple processes running jobs we have a
    // race condition here so that's why we need to lock the table

    if (!pstampDependentInsert(
        config->dbh,
        0,              // dep_id
        hold ? "hold" : "new",          // state
        stage,
        stage_id,
        component,
        imagedb,
        rlabel,
        need_magic,
        outdir,
        0,              // fault
        0               // fault_count
        )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to insert pstampDependent");
        return false;
    }

    // if we try and get this after commit zero is returned
    psS64 dep_id = psDBLastInsertID(config->dbh);
    if (!dep_id) {
        psError(PS_ERR_UNKNOWN, false, "psDBLastInsertID returned NULL");
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    printf("%" PRId64 "\n", dep_id);

    return true;
}

static bool pendingdependentMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-imagedb", "imagedb", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dep_id", "dep_id", "==");
    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_STR(config->args, where, "-rlabel", "rlabel", "==");
    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(includeFaulted, config->args, "-includefaulted", false);

    psString query = pxDataGet("pstamptool_pendingdependent.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!includeFaulted) {
        psStringAppend(&query, "    AND (pstampDependent.fault = 0)\n");
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\n    AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, "\nGROUP BY dep_id ORDER BY priority DESC, MIN(req_id), dep_id");

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
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampDependent", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool updatedependentMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(dep_id,    config->args, "-dep_id", true, false);
    PXOPT_LOOKUP_STR(state,     config->args, "-set_state",  false, false);
    PXOPT_LOOKUP_S16(fault,     config->args, "-set_fault",  false, false);

    if (!fault && !state) {
        psError(PS_ERR_UNKNOWN, true, "at least one of -set_state or fault is required");
        return false;
    }
    psString query = psStringCopy("UPDATE pstampDependent SET");
    bool needComma = false;
    if (state) {
        psStringAppend(&query, " state = '%s'", state);
        needComma = true;
    }
    if (fault) {
        psStringAppend(&query, "%s fault = %d", needComma ? ", " : "", fault);
        needComma = true;
        psStringAppend(&query, ", pstampDependent.fault_count = pstampDependent.fault_count + 1");
    }
    psStringAppend(&query, " WHERE dep_id = %" PRId64, dep_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    if (affected != 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected one row but %"
                                        PRIu64 " rows were modified", affected);
        return false;
    }

    return true;
}
static bool revertdependentMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fault", "pstampDependent.fault", "==");
    PXOPT_COPY_S64(config->args, where, "-dep_id", "dep_id", "==");
    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "pstampRequest.label", "==");
    PXOPT_LOOKUP_BOOL(clear_fault_count, config->args, "-clear_fault_count", false);

    // XXX: we don't actually use -limit. It doesn't work for UPDATE
    // it's an allowed arg because add_poll_args adds it
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    if (limit) { /* do something?? */ }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_S16(fault,      config->args, "-fault",          false, false);

    psString query = pxDataGet("pstamptool_revertdependent.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (!fault) {
        // unless asked to clear a specific fault value, restrict reverts to fault codes
        // that are less than the minimum fault in the API
        psStringAppend(&query, " AND (pstampDependent.fault < %d)", PSTAMP_FIRST_ERROR_CODE);
    }

    // don't keep reverting once the number of faults reaches some value unless 
    // a parameter asking us to clear that count is provided
    psString faultCountClause = NULL;
    if (clear_fault_count) {
        psStringAppend(&faultCountClause, "\n, pstampDependent.fault_count = 0");
    } else {
        psStringAppend(&faultCountClause, " ");
        psStringAppend(&query, " AND pstampDependent.fault_count < %d", PSTAMP_MAX_DEP_FAULTS);
    }

    if (!p_psDBRunQueryF(config->dbh, query, faultCountClause)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    return true;
}

static bool getwebrequestnumMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    if (!pstampWebRequestInsert(config->dbh, 0 )) {
        psError(PS_ERR_UNKNOWN, false, "failed to insert pstampWebRequest");
        return false;
    }

    psS64 req_id = psDBLastInsertID(config->dbh);

    printf("%" PRId64 "\n", req_id);

    return true;
}

static bool addfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(job_id, config->args, "-job_id", true, false);
    PXOPT_LOOKUP_STR(path,   config->args, "-path",   true, false);

    if (!pstampFileInsert(config->dbh,
            0, // file_id
            job_id,
            path
            )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    if (affected != 1) {
        psError(PS_ERR_UNKNOWN, false,
            "should have affected one row but %" PRIu64 " rows were modified",
            affected);
        return false;
    }

    psS64 file_id = psDBLastInsertID(config->dbh);
    printf("%" PRId64 "\n", file_id);

    return true;
}

static bool listfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
    PXOPT_COPY_S64(config->args, where, "-req_id", "req_id", "==");
    PXOPT_COPY_S64(config->args, where, "-file_id", "file_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!psListLength(where->list)) {
        fprintf(stderr, "search arguments are required\n");
        exit (1);
    }

    psString query = pxDataGet("pstamptool_listfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
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
        psTrace("pstamptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampFile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool deletefileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(job_id, config->args, "-job_id", true, false);

    psString query = NULL; 
    psStringAppend(&query, "DELETE FROM pstampFile WHERE job_id = %" PRId64, job_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("pstamptool", PS_LOG_INFO, "Deleted %" PRIu64 " rows from pstampFile", affected);

    return true;
}

static bool adddomainMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(domainName,      config->args, "-set_domain",  true, false);
    PXOPT_LOOKUP_S32(accessLevel,     config->args, "-set_accessLevel", true, false);
    PXOPT_LOOKUP_STR(defaultProduct,  config->args, "-set_defaultProduct",  false, false);
    PXOPT_LOOKUP_STR(defaultLabel,    config->args, "-set_defaultLabel",  false, false);

    if (!pstampUserDomainInsert(config->dbh,
        domainName,
        accessLevel,
        defaultProduct,
        defaultLabel
        )) {
        psError(PS_ERR_UNKNOWN, false, "failed to insert domain");
        return false;
    }

    return true;
}
static bool updatedomainMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(domainName,      config->args, "-domain",  true, false);
    PXOPT_LOOKUP_S32(accessLevel,     config->args, "-set_accessLevel", false, false);
    PXOPT_LOOKUP_STR(defaultProduct,  config->args, "-set_defaultProduct",  false, false);
    PXOPT_LOOKUP_STR(defaultLabel,    config->args, "-set_defaultLabel",  false, false);

    if (!accessLevel && !defaultProduct && !defaultLabel) {
        psError(PS_ERR_UNKNOWN, true, "at least one set option is required");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-domain",   "domainName", "==");
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE pstampUserDomain SET");

    char * sep = "";
    if (accessLevel) {
        psStringAppend(&query, "%s accessLevel = '%d'", sep, accessLevel);
        sep = ", ";
    }
    if (defaultProduct) {
        psStringAppend(&query, "%s  defaultProduct = '%s'", sep, defaultProduct);
        sep = ", ";
    }
    if (defaultLabel) {
        psStringAppend(&query, "%s defaultLabel = '%s'", sep, defaultLabel);
        sep = ", ";
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nWHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    // psU64 affected = psDBAffectedRows(config->dbh);
    // psLogMsg("pstamptool", PS_LOG_INFO, "Updated %" PRIu64 " pstampRequests", affected);

    return true;
}

static bool listdomainMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-domain", "domainName", "=");
    PXOPT_COPY_S32(config->args, where, "-accessLevel", "accessLevel", "=");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = psStringCopy("SELECT * from pstampUserDomain");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
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
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampUserDomain", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool adduserMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(userName,        config->args, "-set_user",  true, false);
    PXOPT_LOOKUP_STR(domainName,      config->args, "-set_domain",  true, false);
    PXOPT_LOOKUP_S32(accessLevel,     config->args, "-set_accessLevel", false, false);
    PXOPT_LOOKUP_STR(defaultProduct,  config->args, "-set_defaultProduct",  false, false);
    PXOPT_LOOKUP_STR(defaultLabel,    config->args, "-set_defaultLabel",  false, false);

    if (!pstampUserInsert(config->dbh,
        userName,
        domainName,
        accessLevel,
        defaultProduct,
        defaultLabel
        )) {
        psError(PS_ERR_UNKNOWN, false, "failed to insert user");
        return false;
    }

    return true;
}
static bool updateuserMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(userName,        config->args, "-user",  true, false);
    PXOPT_LOOKUP_STR(domainName,      config->args, "-domain",  true, false);
    PXOPT_LOOKUP_S32(accessLevel,     config->args, "-set_accessLevel", false, false);
    PXOPT_LOOKUP_STR(defaultProduct,  config->args, "-set_defaultProduct",  false, false);
    PXOPT_LOOKUP_STR(defaultLabel,    config->args, "-set_defaultLabel",  false, false);

    if (!accessLevel && !defaultProduct && !defaultLabel) {
        psError(PS_ERR_UNKNOWN, true, "at least one set option is required");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-user",     "userName", "==");
    PXOPT_COPY_STR(config->args, where, "-domain",   "domainName", "==");
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE pstampUser SET");

    char * sep = "";
    if (accessLevel) {
        psStringAppend(&query, "%s accessLevel = '%d'", sep, accessLevel);
        sep = ", ";
    }
    if (defaultProduct) {
        psStringAppend(&query, "%s  defaultProduct = '%s'", sep, defaultProduct);
        sep = ", ";
    }
    if (defaultLabel) {
        psStringAppend(&query, "%s defaultLabel = '%s'", sep, defaultLabel);
        sep = ", ";
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nWHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    // psU64 affected = psDBAffectedRows(config->dbh);
    // psLogMsg("pstamptool", PS_LOG_INFO, "Updated %" PRIu64 " pstampRequests", affected);

    return true;
}
static bool listuserMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-user", "userName", "=");
    PXOPT_COPY_STR(config->args, where, "-domain", "domainName", "=");
    PXOPT_COPY_S32(config->args, where, "-accessLevel", "pstampUser.accessLevel", "=");
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    if (!psListLength(where->list)) {
        psError(PS_ERR_UNKNOWN, true, "search paramters are required");
        return false;
    }

    psString query = pxDataGet("pstamptool_listuser.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement from pstamptool_listuser.sql");
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
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
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampUser", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool addaccesslevelMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(proj_id,         config->args, "-set_proj_id", false, false);
    PXOPT_LOOKUP_STR(project_name,    config->args, "-set_project_name",  false, false);
    PXOPT_LOOKUP_S32(accessLevel,     config->args, "-set_accessLevel", true, false);
    PXOPT_LOOKUP_F32(mjd_min,         config->args, "-set_mjd_min", false, false);
    PXOPT_LOOKUP_F32(mjd_max,         config->args, "-set_mjd_max", false, false);

    if (proj_id) {
        if (!pstampAccessLevelInsert(config->dbh,
            proj_id,
            accessLevel,
            mjd_min,
            mjd_max
        )) {
            psError(PS_ERR_UNKNOWN, false, "failed to insert accessLevel");
            return false;
        }
    } else if (project_name) {
        // use select insert getting proj_id from pstampProject
        psString query = pxDataGet("pstamptool_addaccesslevel.sql");
        if (!p_psDBRunQueryF(config->dbh, query, accessLevel, mjd_min, mjd_max, project_name)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
        psFree(query);
    } else {
        psError(PS_ERR_UNKNOWN, true, "proj_id or project_name is required");
        return false;
    }

    return true;
}
static bool updateaccesslevelMode(pxConfig *config)
{
    return false;
}
static bool listaccesslevelMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-project_name", "pstampProject.name", "=");
    PXOPT_COPY_S64(config->args, where, "-proj_id", "proj_id", "=");
    PXOPT_COPY_S32(config->args, where, "-accessLevel", "accessLevel", "=");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = psStringCopy("SELECT * from pstampAccessLevel join pstampProject USING(proj_id)");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
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
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pstampAccessLevel", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
# endif

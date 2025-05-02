/*
 * pztool.c
 *
 * Copyright (C) 2006-2008  Joshua Hoblitt
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

#include "pslib.h"
#include "pxtools.h"
#include "pxdata.h"
#include "pztool.h"

static bool adddatastoreMode(pxConfig *config);
static bool datastoreMode(pxConfig *config);
static bool seenMode(pxConfig *config);
static bool pendingExpMode(pxConfig *config);
static bool pendingImfileMode(pxConfig *config);

static bool copydoneMode(pxConfig *config);
static bool copiedMode(pxConfig *config);
static bool updatecopiedMode(pxConfig *config);
static bool revertcopiedMode(pxConfig *config);

static bool clearcommonfaultsMode(pxConfig *config);
static bool toadvanceMode(pxConfig *config);
static bool advanceMode(pxConfig *config);

static bool updatepzexpMode(pxConfig *config);
static bool updatenewexpMode(pxConfig *config);

// XXX EAM : 2021.05.18 : code for updatesummitExp was added but not finished.
// XXX static bool updatesummitExpMode(pxConfig *config);

// static bool copydoneCompleteExp(pxConfig *config);
static psArray *pzGetPendingCameras(pxConfig *config);
static psArray *pzArrayZip(psArray *arraySet, psS64 limit);
static bool pzDownloadExpSetState(pxConfig *config, const psS64 summit_id, const char *state);

// XXX static bool summitExpSetFault(pxConfig *config, const psS64 summit_id, const int fault);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
                goto FAIL; \
            } \
    break;


int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pztoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(PZTOOL_MODE_ADDDATASTORE, adddatastoreMode);
        MODECASE(PZTOOL_MODE_DATASTORE, datastoreMode);
        MODECASE(PZTOOL_MODE_SEEN, seenMode);
        MODECASE(PZTOOL_MODE_PENDINGEXP, pendingExpMode);
        MODECASE(PZTOOL_MODE_PENDINGIMFILE, pendingImfileMode);
        MODECASE(PZTOOL_MODE_COPYDONE, copydoneMode);
        MODECASE(PZTOOL_MODE_COPIED, copiedMode);
        MODECASE(PZTOOL_MODE_UPDATECOPIED, updatecopiedMode);
        MODECASE(PZTOOL_MODE_REVERTCOPIED, revertcopiedMode);
        MODECASE(PZTOOL_MODE_CLEARCOMMONFAULTS, clearcommonfaultsMode);
        MODECASE(PZTOOL_MODE_TOADVANCE, toadvanceMode);
        MODECASE(PZTOOL_MODE_ADVANCE, advanceMode);
	MODECASE(PZTOOL_MODE_UPDATEPZEXP, updatepzexpMode);
        MODECASE(PZTOOL_MODE_UPDATENEWEXP, updatenewexpMode);
	// XXX MODECASE(PZTOOL_MODE_UPDATESUMMITEXP, updatesummitExpMode);
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

    // required
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);
    
    if (!pzDataStoreInsert(config->dbh,
            camera,
            telescope,
            uri,
            NULL,  // epoch
            0      // use_compress
        )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool datastoreMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!p_psDBRunQuery(config->dbh, "SELECT * FROM pzDataStore")) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("pztool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pzDataStore", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool seenMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where,  "-exp_name",     "exp_name", "==");
    PXOPT_COPY_STR(config->args, where,  "-inst",         "camera", "==");
    PXOPT_COPY_STR(config->args, where,  "-telescope",    "telescope", "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_type",     "exp_type", "==");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM summitExp");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "summitExp");
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
        psTrace("pztool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "summitExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingExpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    psMetadata *where2 = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-summit_id",    "summit_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_name",     "exp_name", "==");
    PXOPT_COPY_STR(config->args, where,  "-inst",         "camera", "==");
    PXOPT_COPY_STR(config->args, where,  "-telescope",    "telescope", "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_type",     "exp_type", "==");

    PXOPT_COPY_TIME(config->args, where2, "-dateobs_begin", "dateobs",   ">=");
    PXOPT_COPY_TIME(config->args, where2, "-dateobs_end",   "dateobs",   "<=");
    
    PXOPT_LOOKUP_BOOL(desc, config->args, "-desc", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // XXX leave this query here ?
    psString query = psStringCopy(
            "SELECT"
            "   summitExp.*"
            " FROM summitExp"
            " LEFT JOIN pzDownloadExp"
	    "   USING(summit_id)"
            " WHERE"
	    "   pzDownloadExp.summit_id IS NULL"
            "   AND summitExp.fault = 0"
        );

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "pzDownloadExp");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (psListLength(where2->list)) {
      psString where2Clause = psDBGenerateWhereConditionSQL(where2, "summitExp");
      psStringAppend(&query, " AND %s", where2Clause);
      psFree(where2Clause);
    }
    psFree(where2);
    
    psStringAppend(&query, " ORDER BY summitExp.dateobs");
    if (desc) {
        psStringAppend(&query, " DESC");
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
        psTrace("pztool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pzDownloadExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingImfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-summit_id",     "summit_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_name",      "exp_name",  "==");
    PXOPT_COPY_STR(config->args, where,  "-inst",          "camera",    "==");
    PXOPT_COPY_STR(config->args, where,  "-telescope",     "telescope", "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_type",      "exp_type",  "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "dateobs",   ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "dateobs",   "<=");

    PXOPT_LOOKUP_BOOL(desc, config->args, "-desc", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psArray *cameras = pzGetPendingCameras(config);
    if (!cameras) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to find any cameras");
        return false;
    }

    // array to hold the aggregate results
    psArray *cameraImfiles = psArrayAlloc(0);

    for (long i = 0; i < psArrayLength(cameras); i++) {
        psString query = pxDataGet("pztool_pendingimfile.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(cameraImfiles);
            return false;
        }

        bool status;
        psString camera = psMetadataLookupStr(&status, cameras->data[i], "camera");
        psStringAppend(&query, " WHERE camera = \"%s\"", camera);

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, " AND %s", whereClause);
            psFree(whereClause);
        }

	if (0) {
	    psStringAppend(&query, " ORDER BY dateobs"); 
	    if (desc) { 
		psStringAppend(&query, " DESC"); 
	    } 
	}

        // request the full "limit" from each known camera and throw away any
        // "extra" rows that we may have after merging the results.  This is
        // a lot simplier than a complicated scheme (tried that) to attempt to
        // request on the right number of rows for each camera

        // treat limit == 0 as "no limit"
        if (limit) {
            psString limitString = psDBGenerateLimitSQL(limit);
            psStringAppend(&query, " %s", limitString);
            psFree(limitString);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            psFree(cameraImfiles);
            return false;
        }
        psFree(query);

        psArray *result = p_psDBFetchResult(config->dbh);
        if (!result) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(cameraImfiles);
            return false;
        }
        if (!psArrayLength(result)) {
            psTrace("pztool", PS_LOG_INFO, "no rows found");
            psFree(result);
            continue;
        }

        // add this query into the array of result set
        psArrayAdd(cameraImfiles, 0, result);
        psFree(result);
    }
    psFree(where);

    // stitch the arrays of imfiles together
    psArray *output = pzArrayZip(cameraImfiles, limit);
    psFree(cameraImfiles);

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "pzDownloadImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool copydoneMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(summit_id, config->args, "-summit_id", true, false);
    PXOPT_LOOKUP_STR(exp_name, config->args, "-exp_name", true, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_STR(class, config->args, "-class", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_BOOL(row_lock, config->args, "-row_lock", false);
    PXOPT_LOOKUP_S32(bytes, config->args, "-bytes", false, false);
    PXOPT_LOOKUP_STR(md5sum, config->args, "-md5sum", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    // start a transaction early so it will contain any row level locks
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // query to get an excluse lock on this exposure in
    // pzDownloadExp
    psString lock_query = NULL;
    if (row_lock) {
        lock_query = psStringCopy("SELECT * FROM pzDownloadExp");

        psMetadata *where = psMetadataAlloc();
	PXOPT_COPY_S64(config->args, where,  "-summit_id", "summit_id", "==");
        PXOPT_COPY_STR(config->args, where,  "-exp_name", "exp_name", "==");
        PXOPT_COPY_STR(config->args, where,  "-inst", "camera", "==");
        PXOPT_COPY_STR(config->args, where,  "-telescope", "telescope", "==");

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereSQL(where, NULL);
            psStringAppend(&lock_query, " %s FOR UPDATE", whereClause);
            psFree(whereClause);
        }
        psFree(where);

        // aquire a lock on the pzDownloadExp record
        // lock persists until the transaction is committed
        if (!p_psDBRunQuery(config->dbh, lock_query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(lock_query);
            return false;
        }
        psFree(lock_query);

        // we must fetch the result set from aquiring the row level lock or
        // MySQL will barf all over us.
        psArray *output = p_psDBFetchResult(config->dbh);
        if (!output) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        psFree(output);
    }

    if (!pzDownloadImfileInsert(config->dbh,
            summit_id,			
            exp_name,
            camera,
            telescope,
            class,
            class_id,
            uri,
            fault,
            NULL,    // epoch
            hostname,
            bytes,
            md5sum
    )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool toadvanceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // optional args
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // find all exposures that have had all of their imfiles downloaded but do
    // not appear in newExp
    psString query = pxDataGet("pztool_find_completed_exp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
/*     PXOPT_COPY_STR(config->args, where,  "-summit_id", "summit_id", "=="); */
    PXOPT_COPY_STR(config->args, where,  "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args, where,  "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where,  "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where,  "-label", "label", "==");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);


    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    // find completed exps
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
        psTrace("pztool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "toadvance", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool advanceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(summit_id, config->args, "-summit_id", true, false);
    PXOPT_LOOKUP_STR(exp_name, config->args, "-exp_name", true, false);
    PXOPT_LOOKUP_STR(inst, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false);

    // optional
    PXOPT_LOOKUP_STR(dvodb, config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-end_stage", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);

    // start a transaction so it's all rows or nothing
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!newExpInsert(config->dbh,
            0x0,        // exp_id
	    summit_id,  // summit_id
            exp_name,   // tmp_exp_name
            inst,       // tmp_camera
            telescope,  // tmp_telescope
            "run",      // state
            workdir,    // workdir
            "dirty",    // workdir state
            NULL,       // reduction class
            dvodb,      // dvodb
            tess_id,    // tess_id
            end_stage,  // end_stage
            label,      // label
            NULL        // epoch
            )
        ) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

    psS64 exp_id = psDBLastInsertID(config->dbh);

    // insert newImfiles
    {
      char *query =
                "INSERT INTO newImfile"
                "   SELECT"
                "       %" PRId64 ","               // exp_id
                "       pzDownloadImfile.class_id," // tmp_class_id
                "       pzDownloadImfile.uri,"      // uri
                "       NULL,"                       // epoch
                "       pzDownloadImfile.bytes,"    // bytes
                "       pzDownloadImfile.md5sum"    // md5sum
                "   FROM pzDownloadImfile"
                "   WHERE"
                "       pzDownloadImfile.exp_name = '%s'"
                "       AND pzDownloadImfile.camera = '%s'"
                "       AND pzDownloadImfile.telescope = '%s'";

      if (!p_psDBRunQueryF(config->dbh, query, exp_id, exp_name, inst, telescope)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
	return false;
      }

      // sanity check: we should have inserted at least one row
      psU64 affected = psDBAffectedRows(config->dbh);
      if (psDBAffectedRows(config->dbh) < 1) {
	psError(PS_ERR_UNKNOWN, false, "should have affected at least 1 row but %" PRIu64 " rows were modified", affected);
	return false;
      }
    }

    // set pzDownloadExp.state to 'stop'
    if (!pzDownloadExpSetState(config, summit_id, "stop")) {
        psError(PS_ERR_UNKNOWN, false, "failed to change pzDownloadExp.state for %lld", (long long) summit_id);
        return false;
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }


    return true;
}

static psArray *pzGetPendingCameras(pxConfig *config)
{
    // get a list of cameras we've seen exps for
    if (!p_psDBRunQuery(config->dbh, "SELECT DISTINCT camera FROM pzDownloadExp")) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *cameras = p_psDBFetchResult(config->dbh);
    if (!cameras) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return NULL;
    }
    if (!psArrayLength(cameras)) {
        psTrace("pztool", PS_LOG_INFO, "no rows found");
        psFree(cameras);
        return psArrayAlloc(0);
    }

    return cameras;
}

static psArray *pzArrayZip(psArray *arraySet, psS64 limit)
{
    // figure out the combined size of all arrays in the set
    long setSize = 0;
    for (long i = 0; i < psArrayLength(arraySet); i++) {
        setSize += psArrayLength(arraySet->data[i]);
    }

    // treat 0 as "no limit"
    if (limit == 0) {
        limit = setSize;
    }

    psArray *output = psArrayAllocEmpty(limit);
    // loop over each array in the set forever
    for (
            // init
            long counter = 0,   // the total number of elements zipped so far
            i = 0,              // which array in the set
            index = 0;          // the depth into each array
            // test
            (counter < setSize)
            && (counter < limit)
            && (i < psArrayLength(arraySet));
            // incr
            counter++, ++i,
            i = i % psArrayLength(arraySet),
            i % psArrayLength(arraySet) ? : ++index
        ) {

        psArray *array = arraySet->data[i];
        // make sure that this array has not run out of elements
        if (!(index < psArrayLength(array))) {
            continue;
        }

        psArrayAdd(output, 0, array->data[index]);
    }

    return output;
}


static bool copiedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where,  "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "inst", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where,  "-class", "class", "==");
    PXOPT_COPY_STR(config->args, where,  "-class_id", "class_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    psString query = psStringCopy("SELECT * FROM pzDownloadImfile");

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "WHERE pzDownloadImfile.fault != 0");
    } else {
        // don't list faulted rows
        psStringAppend(&query, " %s", "WHERE pzDownloadImfile.fault = 0");
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "pzDownloadImfile");
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
        psTrace("pztool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipPendingImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool updatecopiedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where,  "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where,  "-class", "class", "==");
    PXOPT_COPY_STR(config->args, where,  "-class_id", "class_id", "==");

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);

    if (!pxSetFaultCode(config->dbh, "pzDownloadImfile", where, fault, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree(where);

    return true;
}


static bool revertcopiedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where,  "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where,  "-class", "class", "==");
    PXOPT_COPY_STR(config->args, where,  "-class_id", "class_id", "==");

    psString query = pxDataGet("pztool_revertcopied.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "pzDownloadImfile");
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

    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected atleast 1 row");
        return false;
    }

    return true;
}


static bool clearcommonfaultsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

{
    psString query = pxDataGet("pztool_revert_downloadimfile_faults.sql");
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
}

{
    psString query = pxDataGet("pztool_revert_fileset_faults.sql");
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
}

    return true;
}


static bool pzDownloadExpSetState(pxConfig *config, const psS64 summit_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!(
            (strncmp(state, "run", 4) == 0)
            || (strncmp(state, "stop", 5) == 0)
            || (strncmp(state, "reg", 4) == 0)
	    || (strncmp(state, "drop", 5) == 0)
        )
    ) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid pzDownloadExp state: %s", state);
        return false;
    }

    char *query = "UPDATE pzDownloadExp SET state = '%s' WHERE summit_id = %ld";
    if (!p_psDBRunQueryF(config->dbh, query, state, summit_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for %lld", (long long) summit_id);
        return false;
    }

    return true;
}

# if (0)
static bool summitExpSetFault(pxConfig *config, const psS64 summit_id, const int fault)
{
    // check that state is a valid string value
  if ((fault < 0) || (fault > 2048)) {
    psError(PS_ERR_UNKNOWN, false,
	    "invalid summitExp fault: %d", fault);
    return false;
  }

    char *query = "UPDATE summitExp SET fault = %d WHERE summit_id = %ld";
    if (!p_psDBRunQueryF(config->dbh, query, fault, summit_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for %lld", (long long) summit_id);
        return false;
    }

    return true;
}

static bool updatesummitExpMode(pxConfig *config)
{
  int fault;

  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_S64(summit_id, config->args, "-summit_id", true, false);
  PXOPT_LOOKUP_S32(state,    config->args, "-set_fault",true, false);

  if (!summitExpSetFault(config,summit_id, fault)) {
    psError(PS_ERR_UNKNOWN, false, "failed to change state for %lld", (long long) summit_id);
    return false;
  }
  return true;
}
# endif  
  
static bool updatepzexpMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_S64(summit_id, config->args, "-summit_id", true, false);
  PXOPT_LOOKUP_STR(state,    config->args, "-set_state",true, false);

  if (!pzDownloadExpSetState(config,summit_id, state)) {
    psError(PS_ERR_UNKNOWN, false, "failed to change state for %lld", (long long) summit_id);
    return false;
  }
  return true;
}
  
  
static bool updatenewexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_name", "tmp_exp_name", "==");

    PXOPT_LOOKUP_STR(new_state, config->args, "-set_state", true, false);

    if (strcmp(new_state, "drop") && strcmp(new_state, "run") && strcmp(new_state, "wait")) {
        psError(PXTOOLS_ERR_ARGUMENTS, true, "%s is not a valid value for -set_state", new_state);
        psFree(where);
        return false;
    }

    if (psListLength(where->list) < 1) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "-exp_name or -exp_id is required");
        return false;
    }

    psString query = NULL;
    psStringAppend(&query, "UPDATE newExp SET state = '%s'", new_state);

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nWHERE %s", whereClause);

    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}




#if 0
static psArray *pzArrayAddArray(psArray *array, psArray *input)
{
    for (long i = 0; i < psArrayLength(input); i++) {
        psArrayAdd(array, psArrayLength(input), input->data[i]);
    }

    return array;
}
#endif

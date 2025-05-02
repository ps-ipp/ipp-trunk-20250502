/*
 * fpcamtool.c
 *
 * Copyright (C) 2022 Eugene Magnier
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
#include <stdint.h>
#include <inttypes.h>

#include "pxtools.h"
#include "pxfpcam.h"
#include "fpcamtool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool pendingexpMode(pxConfig *config);
static bool inputchipsMode(pxConfig *config);
static bool inputastromMode(pxConfig *config);
static bool addprocessedexpMode(pxConfig *config);
static bool processedexpMode(pxConfig *config);
static bool revertprocessedexpMode(pxConfig *config);
static bool updateprocessedexpMode(pxConfig *config);

# define MODECASE(caseName, func) case caseName: if (!func(config)) { goto FAIL; } break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = fpcamtoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(FPCAMTOOL_MODE_DEFINEBYQUERY,        definebyqueryMode);
        MODECASE(FPCAMTOOL_MODE_UPDATERUN,            updaterunMode);
        MODECASE(FPCAMTOOL_MODE_PENDINGEXP,           pendingexpMode);
        MODECASE(FPCAMTOOL_MODE_INPUTCHIPS,           inputchipsMode);
        MODECASE(FPCAMTOOL_MODE_INPUTASTROM,          inputastromMode);
        MODECASE(FPCAMTOOL_MODE_ADDPROCESSEDEXP,      addprocessedexpMode);
        MODECASE(FPCAMTOOL_MODE_PROCESSEDEXP,         processedexpMode);
        MODECASE(FPCAMTOOL_MODE_REVERTPROCESSEDEXP,   revertprocessedexpMode);
        MODECASE(FPCAMTOOL_MODE_UPDATEPROCESSEDEXP,   updateprocessedexpMode);
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
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    
    psMetadata *whereChp = psMetadataAlloc();
    psMetadata *whereCam = psMetadataAlloc();

    // The user can specify the chipRun and camRun independently

    // selections for the relevant chip-stage
    pxfpcamGetSearchArgs (config, whereChp);
    pxAddLabelSearchArgs (config, whereChp, "-chip_label",      "chipRun.label",      "==");
    PXOPT_COPY_S64(config->args,  whereChp, "-chip_id",         "chipRun.chip_id",    "==");
    PXOPT_COPY_STR(config->args,  whereChp, "-chip_reduction",  "chipRun.reduction",  "==");
    PXOPT_COPY_STR(config->args,  whereChp, "-chip_data_group", "chipRun.data_group", "==");
    
    // selections for the relevant cam-stage
    pxfpcamGetSearchArgs (config, whereCam);
    pxAddLabelSearchArgs (config, whereCam, "-cam_label",      "camRun.label",      "==");
    PXOPT_COPY_S64(config->args,  whereCam, "-cam_id",         "camRun.cam_id",     "==");
    PXOPT_COPY_STR(config->args,  whereCam, "-cam_reduction",  "camRun.reduction",  "==");
    PXOPT_COPY_STR(config->args,  whereCam, "-cam_data_group", "camRun.data_group", "==");

    // pxfpcam provides rawExp constraints to select the exposures

    if (!psListLength(whereCam->list)) {
	psFree(whereCam);
	psFree(whereChp);
	psError(PXTOOLS_ERR_CONFIG, false, "search parameters (at least for camRun) are required");
	return false;
    }
    
    PXOPT_LOOKUP_STR(workdir,    config->args, "-set_workdir",    false, false);
    PXOPT_LOOKUP_STR(label,      config->args, "-set_label",      false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction,  config->args, "-set_reduction",  false, false);
    PXOPT_LOOKUP_STR(dvodb,      config->args, "-set_dvodb",      false, false);
    PXOPT_LOOKUP_STR(note,       config->args, "-set_note",       false, false);

    // default
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple,  config->args, "-simple",  false);

    // find the exp_id of all the exposures that we want to queue up.

    if (psListLength(whereChp->list)) {
	// if selections are set for chip (and cam), use query set for both where strings
	psString query = pxDataGet("fpcamtool_find_cam_and_chip_id.sql");
	if (!query) {
	    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
	    psFree(whereChp);
	    psFree(whereCam);
	    return false;
	}
	psString whereChpStr = psDBGenerateWhereConditionSQL(whereChp, NULL);
	psString whereCamStr = psDBGenerateWhereConditionSQL(whereCam, NULL);

	// XXX add limits to the chip & cam queries?
        if (!p_psDBRunQueryF(config->dbh, query, whereChpStr, whereCamStr)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
	    psFree(whereChp);
	    psFree(whereCam);
	    psFree(whereChpStr);
	    psFree(whereCamStr);
            return false;
        }
	psFree(query);
	psFree(whereChp);
	psFree(whereCam);
	psFree(whereChpStr);
	psFree(whereCamStr);
    } else {
	// if we are not selecting the chipRun explicitly, use the camRun selection for both
	psString query = pxDataGet("fpcamtool_find_cam_id.sql");
	if (!query) {
	    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
	    psFree(whereChp);
	    psFree(whereCam);
	    return false;
	}
	psString whereCamStr = psDBGenerateWhereConditionSQL(whereCam, NULL);

	// XXX add limits to the chip & cam queries?
        if (!p_psDBRunQueryF(config->dbh, query, whereCamStr)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
	    psFree(whereChp);
	    psFree(whereCam);
	    psFree(whereCamStr);
            return false;
        }
	psFree(query);
	psFree(whereChp);
	psFree(whereCam);
	psFree(whereCamStr);
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("fpcamtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "fpcamRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    // start a transaction so we don't end up with an exp without any associted
    // imfiles
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // loop over our list of fpcamRun rows
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        fpcamInsertRow *row = fpcamInsertObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into fpcamInsert object");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxfpcamQueueByCamAndChipID(config,
                    row->chip_id,
                    row->cam_id,
                    workdir     ? workdir   : row->workdir,
                    label       ? label     : row->label,
                    data_group  ? data_group: row->data_group,
                    dist_group  ? dist_group: row->dist_group,
                    reduction   ? reduction : row->reduction,
                    dvodb       ? dvodb     : row->dvodb,
                    note
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue cam_id: %lld, chip_id: %lld", (long long) row->cam_id, (long long) row->chip_id);
            psFree(row);
            psFree(output);
            return false;
        }
        psFree(row);
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    pxfpcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where, "-fpcam_id",  "fpcamRun.fpcam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "fpcamRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",   "fpcamRun.chip_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",     "fpcamRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group","fpcamRun.data_group", "==");
    PXOPT_COPY_STR(config->args, where, "-state",     "fpcamRun.state", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "fpcamRun.reduction", "==");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    psString query = psStringCopy("UPDATE fpcamRun JOIN camRun USING(cam_id) JOIN chipRun ON(chipRun.chip_id = camRun.chip_id) JOIN rawExp USING(exp_id)");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "fpcamRun", "fpcam_id", "fpcamProcessedExp", true, false);
    if (!result) {
        psError(psErrorCodeLast(), false, "pxUpdateRun failed");
    }

    psFree(query);
    psFree(where);

    return result;
}

static bool pendingexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psMetadata *where = psMetadataAlloc();
    pxfpcamGetSearchArgs (config, where);
    pxAddLabelSearchArgs (config, where, "-label",    "fpcamRun.label", "==");
    PXOPT_COPY_S64(config->args, where, "-fpcam_id",  "fpcamRun.fpcam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "fpcamRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",   "fpcamRun.chip_id", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "fpcamRun.reduction", "==");

    psString query = pxDataGet("fpcamtool_pendingexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else {
      if (!all) {
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (try -all)");
        return false;
      }
    }
    psFree(where);

    psStringAppend(&query, "\nORDER BY priority DESC, fpcam_id");

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
        psTrace("fpcamtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fpcamPendingExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool inputchipsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(fpcam_id, config->args, "-fpcam_id", true, false);
    PXOPT_LOOKUP_U64(limit, config->args,    "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args,  "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fpcam_id",  "fpcamRun.fpcam_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "fpcamRun.reduction", "==");

    psString query = pxDataGet("fpcamtool_inputchips.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement for %d", (int) fpcam_id);
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (!psListLength(where->list)) {
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (-fpcam_id required)");
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
        psTrace("fpcamtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fpcamInputChips", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool inputastromMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(fpcam_id, config->args, "-fpcam_id", true, false);
    PXOPT_LOOKUP_U64(limit, config->args,    "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args,  "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fpcam_id",  "fpcamRun.fpcam_id", "==");

    psString query = pxDataGet("fpcamtool_inputastrom.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement for %d", (int) fpcam_id);
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (!psListLength(where->list)) {
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (-fpcam_id required)");
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
        psTrace("fpcamtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fpcamInputAstrom", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool addprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(fpcam_id, config->args,      "-fpcam_id", true, false);
    PXOPT_LOOKUP_STR(path_base, config->args,     "-path_base", true, false);

    // optional
    PXOPT_LOOKUP_F32(zpt_obs, config->args,       "-zpt_obs", false, false);
    PXOPT_LOOKUP_F32(zpt_err, config->args,       "-zpt_err", false, false);
    PXOPT_LOOKUP_F32(zpt_uq,  config->args,       "-zpt_uq", false, false);
    PXOPT_LOOKUP_F32(zpt_lq,  config->args,       "-zpt_lq", false, false);

    PXOPT_LOOKUP_F32(dtime_script, config->args,   "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args,       "-hostname", false, false);
    PXOPT_LOOKUP_S32(n_stars, config->args,        "-n_stars", false, false);
 
    PXOPT_LOOKUP_S16(fault, config->args,          "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args,        "-quality", false, false);

    // we store actual detection efficiency by adding in zpt_obs
    PXOPT_LOOKUP_F32(deteff_inst, config->args,    "-deteff_inst", false, false);
    PXOPT_LOOKUP_F32(deteff_inst_lq, config->args, "-deteff_inst_lq", false, false);
    PXOPT_LOOKUP_F32(deteff_inst_uq, config->args, "-deteff_inst_uq", false, false);
    PXOPT_LOOKUP_F32(deteff_err, config->args,     "-deteff_inst_err", false, false);

    psF32 deteff_obs = NAN;
    psF32 deteff_uq = NAN;
    psF32 deteff_lq = NAN;
    if (isfinite(zpt_obs)) {
        if (isfinite(deteff_inst)) {
            deteff_obs = deteff_inst + zpt_obs;
        }
        if (isfinite(deteff_inst_uq)) {
            deteff_uq = deteff_inst_uq + zpt_obs;
        }
        if (isfinite(deteff_inst_lq)) {
            deteff_lq = deteff_inst_lq + zpt_obs;
        }
    }

    PXOPT_LOOKUP_STR(ver_pslib,     config->args, "-ver_pslib",     false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_psphot,    config->args, "-ver_psphot",    false, false);
    PXOPT_LOOKUP_STR(ver_ppstats,   config->args, "-ver_ppstats",   false, false);
    PXOPT_LOOKUP_STR(ver_fpcamera,  config->args, "-ver_fpcamera",  false, false);

    psString software_ver = pxMergeCodeVersions(ver_pslib,ver_psmodules);
    software_ver = pxMergeCodeVersions(software_ver,ver_psphot);
    software_ver = pxMergeCodeVersions(software_ver,ver_ppstats);
    software_ver = pxMergeCodeVersions(software_ver,ver_fpcamera);

    // generate restrictions
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    fpcamProcessedExpRow *row = fpcamProcessedExpRowAlloc(
        fpcam_id,
        path_base,
        zpt_obs,
        zpt_err,
        zpt_lq,
        zpt_uq,
        dtime_script,
        hostname,
        n_stars,
        fault,
	NULL, // XXX : this should be epoch
        software_ver,
        deteff_obs,
        deteff_err,
        deteff_lq,
        deteff_uq,
        quality
        );

    if (!fpcamProcessedExpInsertObject(config->dbh, row)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        return false;
    }

    if (fault) {
        psFree(row);
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }
    // else continue on...

    // since there is only one exp per 'new' set camRun.state = 'full'
    if (!pxfpcamRunSetState(config, row->fpcam_id, "full")) {
        psError(PS_ERR_UNKNOWN, false, "failed to change fpcamRun.state for fpcam_id: %" PRId64, row->fpcam_id);
        psFree(row);
        return false;
    }

    psFree(row);

    // fpcam is a stand-alone stage: it does not trigger another stage to follow

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    return true;
}

static bool processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // some options
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    // generate restrictions
    psMetadata *where = psMetadataAlloc();
    pxfpcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args,  where, "-fpcam_id",    "fpcamRun.fpcam_id", "==");
    PXOPT_COPY_S64(config->args,  where, "-cam_id",      "fpcamRun.cam_id", "==");
    PXOPT_COPY_S64(config->args,  where, "-chip_id",     "fpcamRun.chip_id", "==");
    pxAddLabelSearchArgs (config, where, "-label",       "fpcamRun.label",     "==");
    pxAddLabelSearchArgs (config, where, "-data_group",  "fpcamRun.data_group",     "LIKE");
    PXOPT_COPY_STR(config->args,  where, "-reduction",   "fpcamRun.reduction", "==");
    
    psString where2 = NULL;
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }
    if (!psListLength(where->list) && !where2) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("fpcamtool_find_processedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    }

    // we either add AND (condition) or WHERE (condition):
    if (where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " AND fpcamProcessedExp.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND fpcamProcessedExp.fault = 0");
    }
    if (!where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE fpcamProcessedExp.fault != 0");
    }
    if (!where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE fpcamProcessedExp.fault = 0");
    }
    psFree(where);
    if (where2) {
        psStringAppend(&query, " %s", where2);
        psFree(where2);
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
        psTrace("fpcamtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fpcamProcessedExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool revertprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    pxfpcamGetSearchArgs (config, where);

    PXOPT_COPY_S64(config->args,  where, "-fpcam_id",    "fpcamRun.fpcam_id", "==");
    PXOPT_COPY_S64(config->args,  where, "-cam_id",      "fpcamRun.cam_id", "==");
    PXOPT_COPY_S64(config->args,  where, "-chip_id",     "fpcamRun.chip_id", "==");
    PXOPT_COPY_STR(config->args,  where, "-reduction",   "fpcamRun.reduction", "==");
    PXOPT_COPY_S16(config->args, where,  "-fault",       "fpcamProcessedExp.fault", "==");

    pxAddLabelSearchArgs (config, where, "-label",       "fpcamRun.label",     "==");
    pxAddLabelSearchArgs (config, where, "-data_group",  "fpcamRun.data_group",     "LIKE");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(where);
        return false;
    }

    psString whereClause = NULL;
    {
        psString query = pxDataGet("fpcamtool_revertprocessedexp.sql");
        if (!query) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        // use psDBGenerateWhereConditionalSQL with AND ... because the SQL ends in a WHERE
        if (where && psListLength(where->list)) {
            whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, " AND %s", whereClause);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            psFree(where);
            return false;
        }
        psFree(query);
    }
    psFree(where);

    int numDeleted = psDBAffectedRows(config->dbh);
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psLogMsg("fpcamtool", PS_LOG_INFO, "Deleted %d fpcamProcessedExps", numDeleted);

    {
        psString query = pxDataGet("fpcamtool_revertupdatedexp.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }

        if (whereClause) {
            psStringAppend(&query, "\n AND %s", whereClause);
            psFree(whereClause);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
        psFree(query);
    }
    int numUpdated = psDBAffectedRows(config->dbh);
    psLogMsg("fpcamtool", PS_LOG_INFO, "Updated %d fpcamProcessedExps", numUpdated);

    return true;
}


static bool updateprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fpcam_id", "fpcam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",   "cam_id",   "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",  "chip_id",  "==");

    if (!pxSetFaultCode(config->dbh, "fpcamProcessedExp", where, fault, quality)) {
	psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree (where);
    return true;
}


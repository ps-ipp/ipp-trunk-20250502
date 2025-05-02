/*
 * warptool.c
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ippdb.h>

#include "pxtools.h"
#include "warptool.h"

static psS64 definerunMode(pxConfig *config);
static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool expMode(pxConfig *config);
static bool imfileMode(pxConfig *config);
static bool tooverlapMode(pxConfig *config);
static bool addoverlapMode(pxConfig *config);
static bool revertoverlapMode(pxConfig *config);
static bool scmapMode(pxConfig *config);
static bool towarpedMode(pxConfig *config);
static bool addwarpedMode(pxConfig *config);
static bool advancerunMode(pxConfig *config);
static bool warpedMode(pxConfig *config);
static bool revertwarpedMode(pxConfig *config);
static bool blockMode(pxConfig *config);
static bool maskedMode(pxConfig *config);
static bool unblockMode(pxConfig *config);
static bool tosummaryMode(pxConfig *config);
static bool addsummaryMode(pxConfig *config);
static bool pendingcleanuprunMode(pxConfig *config);
static bool pendingcleanupwarpMode(pxConfig *config);
static bool revertcleanupMode(pxConfig *config);
static bool donecleanupMode(pxConfig *config);
static bool tocleanedskyfileMode(pxConfig *config);
static bool topurgedskyfileMode(pxConfig *config);
static bool toscrubbedskyfileMode(pxConfig *config);
static bool tofullskyfileMode(pxConfig *config);
static bool updateskyfileMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);
static bool runstateMode(pxConfig *config);
static bool listrunMode(pxConfig *config);
static bool setskyfiletoupdateMode(pxConfig *config);

static bool parseAndInsertSkyCellMap(pxConfig *config, const char *mapfile);
static bool isValidMode(pxConfig *config, const char *mode);
bool warpCompletedRuns(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = warptoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(WARPTOOL_MODE_DEFINERUN,          definerunMode);
        MODECASE(WARPTOOL_MODE_DEFINEBYQUERY,      definebyqueryMode);
        MODECASE(WARPTOOL_MODE_UPDATERUN,          updaterunMode);
        MODECASE(WARPTOOL_MODE_EXP,                expMode);
        MODECASE(WARPTOOL_MODE_IMFILE,             imfileMode);
        MODECASE(WARPTOOL_MODE_TOOVERLAP,          tooverlapMode);
        MODECASE(WARPTOOL_MODE_ADDOVERLAP,         addoverlapMode);
        MODECASE(WARPTOOL_MODE_REVERTOVERLAP,      revertoverlapMode);
        MODECASE(WARPTOOL_MODE_SCMAP,              scmapMode);
        MODECASE(WARPTOOL_MODE_TOWARPED,           towarpedMode);
        MODECASE(WARPTOOL_MODE_ADDWARPED,          addwarpedMode);
        MODECASE(WARPTOOL_MODE_ADVANCERUN,         advancerunMode);
        MODECASE(WARPTOOL_MODE_WARPED,             warpedMode);
        MODECASE(WARPTOOL_MODE_REVERTWARPED,       revertwarpedMode);
        MODECASE(WARPTOOL_MODE_BLOCK,              blockMode);
        MODECASE(WARPTOOL_MODE_MASKED,             maskedMode);
        MODECASE(WARPTOOL_MODE_UNBLOCK,            unblockMode);
	MODECASE(WARPTOOL_MODE_TOSUMMARY,          tosummaryMode);
	MODECASE(WARPTOOL_MODE_ADDSUMMARY,         addsummaryMode);
        MODECASE(WARPTOOL_MODE_PENDINGCLEANUPRUN,  pendingcleanuprunMode);
        MODECASE(WARPTOOL_MODE_PENDINGCLEANUPSKYFILE, pendingcleanupwarpMode);
        MODECASE(WARPTOOL_MODE_REVERTCLEANUP,      revertcleanupMode);
        MODECASE(WARPTOOL_MODE_DONECLEANUP,        donecleanupMode);
        MODECASE(WARPTOOL_MODE_TOCLEANEDSKYFILE,   tocleanedskyfileMode);
        MODECASE(WARPTOOL_MODE_TOPURGEDSKYFILE,    topurgedskyfileMode);
        MODECASE(WARPTOOL_MODE_TOSCRUBBEDSKYFILE,  toscrubbedskyfileMode);
        MODECASE(WARPTOOL_MODE_TOFULLSKYFILE,      tofullskyfileMode);
        MODECASE(WARPTOOL_MODE_UPDATESKYFILE,      updateskyfileMode);
        MODECASE(WARPTOOL_MODE_EXPORTRUN,          exportrunMode);
        MODECASE(WARPTOOL_MODE_IMPORTRUN,          importrunMode);
        MODECASE(WARPTOOL_MODE_RUNSTATE,           runstateMode);
        MODECASE(WARPTOOL_MODE_LISTRUN,            listrunMode);
        MODECASE(WARPTOOL_MODE_SETSKYFILETOUPDATE, setskyfiletoupdateMode);

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


static psS64 definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(fake_id, config->args, "-fake_id", true, false); // required
    PXOPT_LOOKUP_STR(mode, config->args, "-mode", true, false); // required
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false); // required
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-dist_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-note", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", true, false); // required (no default TESS)
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false); // required (no default TESS)
    PXOPT_LOOKUP_STR(end_stage, config->args, "-end_stage", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // check mode
    if (mode && !isValidMode(config, mode)) {
        psError(PS_ERR_UNKNOWN, false, "invalid mode");
        return false;
    }

    warpRunRow *warpRun = warpRunRowAlloc(
            0,          // ID
            fake_id,
            mode,
            "new",      // state
            workdir,
            "dirty",    // workdir_state
            label,
            data_group ? data_group : label,
            dist_group,
            dvodb,
            tess_id,
            reduction,
            end_stage,
            registered,
            0,       // magicked
	    NULL,     // software version
	    0,       // mask stat npix
	    NAN,        // static
	    NAN,        // dynamic
	    NAN,        // magic
	    NAN,        // advisory
	    note
    );
    if (!warpRun) {
        psError(PS_ERR_UNKNOWN, false, "failed to alloc warpRun object");
        return true;
    }
    if (!warpRunInsertObject(config->dbh, warpRun)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(warpRun);
        return true;
    }

    // get the assigned warp_id
    psS64 warp_id = psDBLastInsertID(config->dbh);
    warpRun->warp_id = warp_id;

    if (!warpRunPrintObject(stdout, warpRun, !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print object");
            psFree(warpRun);
            return false;
    }

    psFree(warpRun);

    return warp_id;
}


static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args,   where, "-fake_id",            "fakeRun.fake_id",       "==");
    PXOPT_COPY_S64(config->args,   where, "-cam_id",             "camRun.cam_id",         "==");
    PXOPT_COPY_S64(config->args,   where, "-chip_id",            "chipRun.chip_id",       "==");
    PXOPT_COPY_S64(config->args,   where, "-exp_id",             "rawExp.exp_id",         "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_name",           "rawExp.exp_name",       "==");
    PXOPT_COPY_STR(config->args,   where, "-inst",               "rawExp.camera",         "==");
    PXOPT_COPY_STR(config->args,   where, "-telescope",          "rawExp.telescope",      "==");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_begin",     "rawExp.dateobs",        ">=");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_end",       "rawExp.dateobs",        "<=");
    PXOPT_COPY_STR(config->args,   where, "-exp_tag",            "rawExp.exp_tag",        "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_type",           "rawExp.exp_type",       "==");
    PXOPT_COPY_STR(config->args,   where, "-filelevel",          "rawExp.filelevel",      "==");
    PXOPT_COPY_STR(config->args,   where, "-filter",             "rawExp.filter",         "==");
    PXOPT_COPY_F64(config->args,   where, "-airmass_min",        "rawExp.airmass",        ">=");
    PXOPT_COPY_F64(config->args,   where, "-airmass_max",        "rawExp.airmass",        "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min",             "rawExp.ra",             ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max",             "rawExp.ra",             "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min",           "rawExp.decl",           ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max",           "rawExp.decl",           "<");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_min",       "rawExp.exp_time",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_max",       "rawExp.exp_time",       "<");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_min",             "rawExp.bg",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_max",             "rawExp.bg",             "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_min",       "rawExp.bg_stdev",       ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_max",       "rawExp.bg_stdev",       "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_min",  "rawExp.bg_mean_stdev",  ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_max",  "rawExp.bg_mean_stdev",  "<");
    PXOPT_COPY_F64(config->args,   where, "-alt_min",            "rawExp.alt",            ">=");
    PXOPT_COPY_F64(config->args,   where, "-alt_max",            "rawExp.alt",            "<");
    PXOPT_COPY_F64(config->args,   where, "-az_min",             "rawExp.az",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-az_max",             "rawExp.az",             "<");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_min",       "rawExp.ccd_temp",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_max",       "rawExp.ccd_temp",       "<");
    PXOPT_COPY_F64(config->args,   where, "-posang_min",         "rawExp.posang",         ">=");
    PXOPT_COPY_F64(config->args,   where, "-posang_max",         "rawExp.posang",         "<");
    PXOPT_COPY_STR(config->args,   where, "-object",             "rawExp.object",         "==");
    PXOPT_COPY_STR(config->args,   where, "-comment",            "rawExp.comment",        "LIKE");
    PXOPT_COPY_STR(config->args,   where, "-obs_mode",           "rawExp.obs_mode",       "LIKE");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_min",      "rawExp.sun_angle",      ">=");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_max",      "rawExp.sun_angle",      "<");
    PXOPT_COPY_STR(config->args,   where, "-reduction",          "fakeRun.reduction",     "==");
    pxAddLabelSearchArgs (config,  where, "-label",              "fakeRun.label",         "=="); // define using fake label

    if (!psListLength(where->list) &&
        !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(mode, config->args, "-set_mode", true, false); // required
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-set_tess_id", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-set_end_stage", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    // check mode
    // XXX mode is required but is not used?!
    if (mode && !isValidMode(config, mode)) {
        psError(PS_ERR_UNKNOWN, false, "invalid mode");
        return false;
    }

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("warptool_definebyquery.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "AND %s", whereClause);
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "rawExp", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    // would could do this "all in the database" if we didn't want the option
    // of changing the label/reduction/expgroup/dvodb/etc.  So we're pulling the
    // data out so we have the option of changing these values or leaving the
    // old values in place (i.e., passing the values through).

    // loop over our list of fakeRun rows to check the supplied and selected tess_id values:
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        fakeRunRow *row = fakeRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into fakeRun");
            psFree(output);
            return false;
        }

        if (!tess_id  && !row->tess_id) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue warp run without a defined tess id: label: %s, fake_id %" PRId64, row->label, row->fake_id);
            psFree(output);
            return false;
        }

        psFree(row);
    }

    // loop over our list of fakeRun rows
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        fakeRunRow *row = fakeRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into fakeRun");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxwarpQueueByFakeID(config,
                                 row->fake_id,
                                 workdir     ? workdir   : row->workdir,
                                 label       ? label     : row->label,
                                 data_group  ? data_group: row->data_group,
                                 dist_group  ? dist_group: row->dist_group,
                                 dvodb       ? dvodb     : row->dvodb,
                                 tess_id     ? tess_id   : row->tess_id,
                                 reduction   ? reduction : row->reduction,
                                 end_stage   ? end_stage : row->end_stage,
                                 note))
          {
            psError(PS_ERR_UNKNOWN, false, "failed to trying to queue fake_id: %" PRId64, row->fake_id);
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
    PXOPT_COPY_S64(config->args, where, "-warp_id",   "warpRun.warp_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "warpRun.reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-state",     "warpRun.state",     "==");
    PXOPT_COPY_STR(config->args, where, "-data_group","warpRun.data_group","LIKE");
    PXOPT_COPY_STR(config->args, where, "-dist_group","warpRun.dist_group","LIKE");
    pxAddLabelSearchArgs(config,  where, "-label",    "warpRun.label",     "LIKE");

    PXOPT_COPY_TIME(config->args, where, "-registered_begin", "warpRun.registered",  ">=");
    PXOPT_COPY_TIME(config->args, where, "-registered_end",   "warpRun.registered",  "<");

    PXOPT_LOOKUP_BOOL(destreaked, config->args, "-destreaked", false);
    if (destreaked) {
        psMetadataAddS64(where, PS_LIST_TAIL, "warpRun.magicked", PS_META_DUPLICATE_OK, ">", 0);
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE warpRun");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "warpRun", "warp_id", "warpSkyfile", true, true);

    psFree(query);
    psFree(where);

    return result;
}

static bool expMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("warptool_exp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "warpRun");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool imfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("warptool_imfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "warpRun");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpInputImfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool tooverlapMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("warptool_tooverlap.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "warpRun");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else {
      if (!all) {
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (try -all)");
        return false;
      }
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool addoverlapMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(mapfile, config->args, "-mapfile", false, false);
    PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (fault == 0) {
        if (!parseAndInsertSkyCellMap(config, mapfile)) {
            psError(PS_ERR_UNKNOWN, false, "failed to inject mapfile: %s into the database", mapfile);
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
    } else {
        warpSkyCellMapInsert(config->dbh,
            warp_id,
            "faulted",   // skycell_id
            "faulted",   // tess_id
            "faulted",   // class_id
            fault    // fault
        );
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool revertoverlapMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpSkyCellMap.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpSkyCellMap.skycell_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "warpSkyCellMap.tess_id", "==");
    pxAddLabelSearchArgs (config, where, "-label",     "warpRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",      "warpSkyCellMap.fault", "==");

    if (!psListLength(where->list)
        && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    int numDeleted;                     // Number deleted
    {
        psString query = pxDataGet("warptool_revertoverlap.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, " AND %s", whereClause);
            psFree(whereClause);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(query);

        numDeleted = psDBAffectedRows(config->dbh);
    }

    psLogMsg("warptool", PS_LOG_INFO, "Deleted %d warpSkycellMap", numDeleted);

    psFree(where);

    return true;
}



static bool parseAndInsertSkyCellMap(pxConfig *config, const char *mapfile)
{
    unsigned int nFail = 0;
    psMetadata *imfiles = psMetadataAlloc();
    psMetadata *skycells = psMetadataConfigRead(NULL, &nFail, mapfile, false);
    if (!skycells) {
        psError(PS_ERR_UNKNOWN, false, "failed to parse mapfile: %s", mapfile);
        return false;
    }
    if (nFail) {
        psError(PS_ERR_UNKNOWN, false, "there were %d errors parsing mapfile: %s", nFail, mapfile);
        psFree(skycells);
        psFree(imfiles);
        return false;
    }

    psMetadataItem *item = NULL;
    psMetadataIterator *iter = psMetadataIteratorAlloc(skycells, 0, NULL);
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_METADATA) {
            psError(PS_ERR_UNKNOWN, false, "mapfile: %s is in the wrong format", mapfile);
            psFree(iter);
            psFree(skycells);
            psFree(imfiles);
            return false;
        }

        psMetadata *sc = item->data.md;
        // this conversion isn't strictly nessicary but it's an easy way of
        // validating the format
        warpSkyCellMapRow *row = warpSkyCellMapObjectFromMetadata(sc);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert mapfile: %s metdata entry into a warpSkyCellMap object", mapfile);
            psFree(iter);
            psFree(skycells);
            psFree(imfiles);
            return false;
        }
        psMetadataAddS64(imfiles, PS_LIST_TAIL, row->skycell_id, PS_META_REPLACE, "", row->warp_id);

        if (!warpSkyCellMapInsertObject(config->dbh, row)) {
            psErrorCode err = psErrorCodeLast();
            switch (err) {
                case PS_ERR_DB_CLIENT:
                    psError(PXTOOLS_ERR_SYS, false, "database error");
                case PS_ERR_DB_SERVER:
                    psError(PXTOOLS_ERR_PROG, false, "database error");
                default:
                    psError(PXTOOLS_ERR_PROG, false, "unknown error");
            }
            psFree(row);
            psFree(iter);
            psFree(skycells);
            psFree(imfiles);
            return false;
        }

        psFree(row);
    }
    psFree(iter);

    // create warp_skyfile_ids for the output skyfiles
    psString query = "INSERT INTO warpImfile VALUES(%" PRId64 ", '%s', 0)";
    iter = psMetadataIteratorAlloc(imfiles, 0, NULL);
    while ((item = psMetadataGetAndIncrement(iter))) {
        psString skycell_id = item->name;
        psS64 warp_id = item->data.S64;

        if (!p_psDBRunQueryF(config->dbh, query, warp_id, skycell_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }
    psFree(iter);
    psFree(skycells);
    psFree(imfiles);

    return true;
}


static bool scmapMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "class_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("warptool_scmap.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "warpSkyCellMap");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpSkyCellMap", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool towarpedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-warp_id", "warpSkyCellMap.warp_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    // First find Label's with warpRuns in state new in priority order
    psMetadata *labelWhere = psMetadataAlloc();
    pxAddLabelSearchArgs (config, labelWhere, "-label", "warpRun.label", "==");

    psString labelQuery = pxDataGet("warptool_towarped_labels.sql");
    if (!labelQuery) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    psString labelWhereStr = psStringCopy("");
    if (psListLength(labelWhere->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(labelWhere, NULL);
        psStringAppend(&labelWhereStr, "\n AND %s", whereClause);
        psFree(whereClause);
    } else {
      if (!all) {
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (try -all)");
        return false;
      }
    }
    psFree(labelWhere);

    if (!p_psDBRunQueryF(config->dbh, labelQuery, labelWhereStr)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(labelQuery);
        return false;
    }
    psFree(labelQuery);
    psFree(labelWhereStr);

    psArray *labelOutput = p_psDBFetchResult(config->dbh);
    if (psArrayLength(labelOutput) == 0) {
        psTrace("warptool", PS_LOG_INFO, "no labels with pending warps found.");
        return true;
    }

    psString originalQuery = pxDataGet("warptool_towarped.sql");
    if (!originalQuery) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereStr = NULL;
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereStr, "\n AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psArray *allOutput = psArrayAllocEmpty(0);
    // Now loop over the labels and query each individually
    for (long i = 0; i < psArrayLength(labelOutput); i++) {
        psMetadata *labelRow = labelOutput->data[i];

        bool status;
        psString label = psMetadataLookupStr(&status, labelRow, "label");

        psString query = psStringCopy(originalQuery);
	psStringAppend(&query,"\nORDER BY warp_id");
        // treat limit == 0 as "no limit"
        psString limitString = psStringCopy("");
        if (limit) {
            // We apply the limit to both sides of the UNION to avoid slow queries
            // and to the query itself to satisfy the user's requested limit
            psStringAppend(&limitString, "\n%s", psDBGenerateLimitSQL(limit));
            psStringAppend(&query, "%s", limitString);
        }

        psString thisWhere = NULL;
        if (whereStr) {
            psStringAppend(&thisWhere, "\n%s", whereStr);
        }
        psStringAppend(&thisWhere, "\nAND warpRun.label = '%s'", label); 

        if (!p_psDBRunQueryF(config->dbh, query, thisWhere, limitString, thisWhere,  limitString)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
        psFree(limitString);
        psFree(thisWhere);
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
        long outputLength = psArrayLength(output);
        if (outputLength) {
            for (int i = 0; i< outputLength; i++) {
                psPtr ptr = output->data[i];
                allOutput = psArrayAdd(allOutput, outputLength - i + 10, ptr);
                if (!allOutput) {
                    psError(PS_ERR_UNKNOWN, false, "failed to add element to array");
                    return false;
                }
            }
        } else {
            psTrace("warptool", PS_LOG_INFO, "no rows found for %s", label);
            psFree(output);
            continue;
        }
        psFree(output);

        if (limit) {
            limit -= outputLength;
            if (limit <= 0) {
                // All done
                break;
            }
        }
    }
    long allLabelsLength = psArrayLength(allOutput);
    if (allLabelsLength) {
        if (!ippdbPrintMetadatas(stdout, allOutput, "warpPendingSkyCell", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(allOutput);
            return false;
        }
    }
    psFree(allOutput);
    psFree(labelOutput);
    psFree(whereStr);
    psFree(originalQuery);

    return true;
}


static bool addwarpedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", true, false);

    // optional
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F32(dtime_warp, config->args, "-dtime_warp", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_S32(xmin, config->args, "-xmin", false, false);
    PXOPT_LOOKUP_S32(xmax, config->args, "-xmax", false, false);
    PXOPT_LOOKUP_S32(ymin, config->args, "-ymin", false, false);
    PXOPT_LOOKUP_S32(ymax, config->args, "-ymax", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_F32(good_frac, config->args, "-good_frac", false, false);
    PXOPT_LOOKUP_S64(magicked, config->args, "-set_magicked", false, false);

    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_psphot, config->args, "-ver_psphot", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_STR(ver_pswarp, config->args, "-ver_pswarp", false, false);
    PXOPT_LOOKUP_STR(ver_streaks, config->args, "-ver_streaks", false, false);

    PXOPT_LOOKUP_S32(maskfrac_npix, config->args, "-maskfrac_npix", false, false);
    PXOPT_LOOKUP_F32(maskfrac_static, config->args, "-maskfrac_static", false, false);
    PXOPT_LOOKUP_F32(maskfrac_dynamic, config->args, "-maskfrac_dynamic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_magic, config->args, "-maskfrac_magic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_advisory, config->args, "-maskfrac_advisory", false, false);

    PXOPT_LOOKUP_S16(background_model, config->args, "-background_model", false, false);
    
    psTrace("czw.test",1,"Received versions: pslib %s psmodules %s psphot %s ppstats %s pswarp %s streaks %s\n",
	    ver_pslib,ver_psmodules,ver_psphot,ver_ppstats,ver_pswarp,ver_streaks);
    psString ver_code = NULL;
    if ((ver_pslib)&&(ver_psmodules)) {
      ver_code = pxMergeCodeVersions(ver_pslib,ver_psmodules);
    }
    if (ver_psphot) {
      ver_code = pxMergeCodeVersions(ver_code,ver_psphot);
    }
    if (ver_ppstats) {
      ver_code = pxMergeCodeVersions(ver_code,ver_ppstats);
    }
    if (ver_pswarp) {
      ver_code = pxMergeCodeVersions(ver_code,ver_pswarp);
    }
    if (ver_streaks) {
      ver_code = pxMergeCodeVersions(ver_code,ver_streaks);
    }
    
    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);


    
    // we don't want to insert the last skyfile in a run but then not mark the
    // run as 'stop'
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // XXX need to validate that this coresponds to an warpInputImfile
    if (!warpSkyfileInsert(config->dbh,
                           warp_id,
                           skycell_id,
                           tess_id,
                           uri,
                           path_base,
                           "full",      // data_state
                           bg,
                           bg_stdev,
                           dtime_warp,
                           dtime_script,
                           hostname,
                           good_frac,
                           xmin,
                           xmax,
                           ymin,
                           ymax,
                           fault,
                           quality,
			   magicked,
			   ver_code,
			   maskfrac_npix,
			   maskfrac_static,
			   maskfrac_dynamic,
			   maskfrac_magic,
			   maskfrac_advisory,
			   background_model
        )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool advancerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    // PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = pxDataGet("warptool_finished_run_select.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    query = pxDataGet("warptool_finish_run.sql");
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

        bool status;
        psS64 warp_id = psMetadataLookupS64(&status, row, "warp_id");

	psString software_ver = NULL;
	psS64 maskfrac_npix = 0;
	psF32 maskfrac_static = 0;
	psF32 maskfrac_dynamic = 0;
	psF32 maskfrac_magic = 0;
	psF32 maskfrac_advisory = 0;

	// Calculate run level masking and software state
	if (!pxCoalesceRunStatus(config,"warptool_coalesce_run.sql",warp_id,
				 &software_ver,&maskfrac_npix,
				 &maskfrac_static,&maskfrac_dynamic,
				 &maskfrac_magic,&maskfrac_advisory)) {
	  psError(PS_ERR_UNKNOWN, false, "failed to generate run level statistics");
	  psFree(output);
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	  }
	  return(false);
	}
	// Set warpRun.software_ver to the appropriate value	if (
	if (software_ver) {
	  if (!pxSetRunSoftware(config, "warpRun", "warp_id", warp_id, software_ver)) {
	    psError(PS_ERR_UNKNOWN, false, "failed to set warpRun.software_ver for warp_id: %" PRId64,
		    warp_id);
	    psFree(output);
	    if (!psDBRollback(config->dbh)) {
	      psError(PS_ERR_UNKNOWN, false, "database error");
	    }
	    return(false);
	  }
	}
	// Set warpRun.maskfrac* to the appropriate values.
	if (maskfrac_npix) {
	  if (!pxSetRunMaskfrac(config, "warpRun", "warp_id", warp_id, maskfrac_npix, maskfrac_static,
				maskfrac_dynamic, maskfrac_magic, maskfrac_advisory)) {
	    psError(PS_ERR_UNKNOWN, false, "failed to set warpRun.software_ver for warp_id: %" PRId64,
		    warp_id);
	    psFree(output);
	    if (!psDBRollback(config->dbh)) {
	      psError(PS_ERR_UNKNOWN, false, "database error");
	    }
	    return(false);
	  }
	}

	
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to look up value for warp_id");
            psFree(output);
            psFree(query);
            return false;
        }
        psS64 magicked = psMetadataLookupS64(&status, row, "magicked");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to look up value for magicked");
            psFree(output);
            psFree(query);
            return false;
        }
        if (!p_psDBRunQueryF(config->dbh, query, magicked, warp_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
            psFree(query);
            return false;
        }

        psS64 numUpdated = psDBAffectedRows(config->dbh);

        if (numUpdated != 1) {
            psError(PS_ERR_UNKNOWN, false, "should have affected 1 row");
            psFree(query);
            psFree(output);
            return false;
        }
    }
    psFree(output);
    psFree(query);

    return true;
}

bool warpCompletedRuns(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psString query = pxDataGet("warptool_finished_run_select.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
        return false;
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    query = pxDataGet("warptool_finish_run.sql");
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

        bool status;
        psS64 warp_id = psMetadataLookupS64(&status, row, "warp_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to look up value for warp_id");
            psFree(output);
            psFree(query);
            return false;
        }
        psS32 magicked = psMetadataLookupS64(&status, row, "magicked");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to look up value for magicked");
            psFree(output);
            psFree(query);
            return false;
        }
        if (!p_psDBRunQueryF(config->dbh, query, magicked, warp_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
            psFree(query);
            return false;
        }

        psS64 numUpdated = psDBAffectedRows(config->dbh);

        if (numUpdated != 1) {
            psError(PS_ERR_UNKNOWN, false, "should have affected 1 row");
            psFree(query);
            psFree(output);
            return false;
        }
    }
    psFree(output);
    psFree(query);

    return true;
}

static bool warpedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpSkyfile.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpSkyfile.skycell_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_skyfile_id", "warpImfile.warp_skyfile_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpSkyfile.skycell_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "warpSkyfile.tess_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
    PXOPT_COPY_S64(config->args, where, "-fake_id",    "fakeRun.fake_id", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawExp.dateobs",  ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawExp.dateobs",  "<=");
    PXOPT_COPY_STR(config->args, where, "-filter",    "rawExp.filter", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-magicked", "warpSkyfile.magicked", "==");
    PXOPT_COPY_S16(config->args, where, "-background_model", "warpSkyfile.background_model", "==");
    pxAddLabelSearchArgs (config, where, "-label",   "warpRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group",   "warpRun.data_group", "LIKE");

    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("warptool_warped.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // generate where strings for arguments that require extra processing
    // beyond PXOPT_COPY*
    psString where2 = NULL;
    if (!pxmagicAddWhere(config, &where2, "warpSkyfile")) {
        psError(psErrorCodeLast(), false, "pxMagicAddWhere failed");
        return false;
    }
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (!all && !where2) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }

    if (where2) {
        if (psListLength(where->list)) {
            psStringAppend(&query, " %s", where2);
        } else {
            psStringAppend(&query, " WHERE 1 %s", where2);
        }
    }
    psFree(where);

    if (pstamp_order) {
        // put runs in order of exposure id with newest warp Runs first
        // The postage stamp parser depends on this behavior
        psStringAppend(&query, "\nORDER by exp_id, warp_id DESC");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool revertwarpedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpSkyfile.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpSkyfile.skycell_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "warpSkyfile.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction",  "rawExp.reduction", "==");
    pxAddLabelSearchArgs (config, where, "-label",     "warpRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",      "warpSkyfile.fault", "==");

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!psListLength(where->list)
        && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("warptool_revertwarped_delete.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    psString query_updated = pxDataGet("warptool_revertwarped_updated.sql");
    if (!query_updated) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psStringAppend(&query_updated, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!fault) {
        // If fault has not been supplied, don't revert update faults with the magic value
        // We don't do this for new runs because then they would never complete
        // quality should be used to drop bad components
        psStringAppend(&query_updated, " AND warpSkyfile.fault != %d", PXTOOL_DO_NOT_REVERT_FAULT);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    int numDeleted = psDBAffectedRows(config->dbh);

    psLogMsg("warptool", PS_LOG_INFO, "Deleted %d warpSkyfiles", numDeleted);

    // fix any faulted warpSkyfiles in data_state 'update'

    if (!p_psDBRunQuery(config->dbh, query_updated)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query_updated);
        return false;
    }
    psFree(query_updated);

    int numUpdated = psDBAffectedRows(config->dbh);

    psLogMsg("warptool", PS_LOG_INFO, "Updated %d warpSkyfiles", numUpdated);

    return true;
}


static bool blockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    if (!warpMaskInsert(config->dbh, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool maskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM warpMask");

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
        psTrace("warpool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpMask", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool unblockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    char *query = "DELETE FROM warpMask WHERE label = '%s'";

    if (!p_psDBRunQueryF(config->dbh, query, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool tosummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);
  
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpSkyfile.warp_id", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id",    "warpSkyfile.tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-state",      "warpRun.state", "==");
  PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
  PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
  PXOPT_COPY_S64(config->args, where, "-fake_id",    "fakeRun.fake_id", "==");
  PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawExp.dateobs",  ">=");
  PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawExp.dateobs",  "<=");
  PXOPT_COPY_STR(config->args, where, "-filter",    "rawExp.filter", "LIKE");
  PXOPT_COPY_S64(config->args, where, "-magicked", "warpSkyfile.magicked", "==");
  pxAddLabelSearchArgs (config, where, "-label",   "warpRun.label", "LIKE");
  pxAddLabelSearchArgs (config, where, "-data_group",   "warpRun.data_group", "LIKE");

  PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

  // find all rawImfiles matching the default query
  psString query = pxDataGet("warptool_tosummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }

  // generate where strings for arguments that require extra processing
  // beyond PXOPT_COPY*
  psString where2 = NULL;
  if (!pxmagicAddWhere(config, &where2, "warpSkyfile")) {
    psError(psErrorCodeLast(), false, "pxMagicAddWhere failed");
    return false;
  }
  
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
  } else if (!all && !where2) {
    psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
    return false;
  }
  
  if (where2) {
    psStringAppend(&query, " %s", where2);
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
    psTrace("warptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }
  
  if (psArrayLength(output)) {
    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpRun", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }
  
  psFree(output);
  return(true);
}
static bool addsummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);
  PXOPT_LOOKUP_STR(projection_cell, config->args, "-projection_cell", true, false);
  PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

  psString query = pxDataGet("warptool_addsummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  if (!p_psDBRunQueryF(config->dbh, query, warp_id, projection_cell, path_base)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psS64 numUpdated = psDBAffectedRows(config->dbh);
  
  if (numUpdated != 1) {
    psError(PS_ERR_UNKNOWN, false, "should have affected 1 row");
    psFree(query);
    return false;
  }
  
  psFree(query);

  // Print anything here?
  
  return(true);
}

static bool pendingcleanuprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "warpRun.label", "==");

    psString query = pxDataGet("warptool_pendingcleanuprun.sql");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpPendingCleanupRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool pendingcleanupwarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "warpRun.label", "==");

    char * sql_file = all ? "warptool_pendingcleanupskyfile_all.sql" : "warptool_pendingcleanupskyfile.sql";
    psString query = pxDataGet(sql_file);
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpPendingCleanupWarp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool revertcleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpSkyfile.warp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label",     "warpRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group",   "warpRun.data_group", "LIKE");

    PXOPT_LOOKUP_STR(state, config->args, "-state", false, false);

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    if (!state) {
        state = "error_cleaned";
    }
    char *newState = NULL;
    if (!strcmp(state, "error_cleaned")) {
        newState = "goto_cleaned";
    } else if (!strcmp(state, "error_purged")) {
        newState = "goto_purged";
    } else if (!strcmp(state, "error_scrubbed")) {
        newState = "goto_scrubbed";
    } else {
        psError(PXTOOLS_ERR_CONFIG, true, "-state must be either error_cleaned, error_purged, or error_scrubbed");
        return false;
    }

    psString query = pxDataGet("warptool_revertcleanup.sql");
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

    if (!p_psDBRunQueryF(config->dbh, query, newState, state, state)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    int numDeleted = psDBAffectedRows(config->dbh);

    psLogMsg("warptool", PS_LOG_INFO, "Reverted %d warpRuns and warpSkyfiles", numDeleted);

    return true;
}
static bool donecleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    psString query = pxDataGet("warptool_donecleanup.sql");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool isValidMode(pxConfig *config, const char *mode)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(mode, false);

    // check that state is a valid string value
    if (!(
            (strncmp(mode, "warp", 5) == 0)
            || (strncmp(mode, "diff", 5) == 0)
            || (strncmp(mode, "stack", 6) == 0)
            || (strncmp(mode, "magic", 6) == 0)
        )
    ) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid warpRun mode: %s", mode);
        return false;
    }

    return true;
}

// update warpSkyfile.data_state to given value.
// afterwards, if all skfyiles in the run have the new state, update the state for the run as well
// shared code for the modes -tocleanedskyfile -tofullskyfile -topurgedskyfile

static bool change_skyfile_data_state(pxConfig *config, psString data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // warp_id, skycell_id are required
    PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);

    psString query = pxDataGet("warptool_change_skyfile_data_state.sql");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psString set_magicked_skyfile = psStringCopy("");
    psString set_magicked_run = psStringCopy("");
    if (!strcmp(data_state, "full")) {
        // magicked is only an argument for for -tofullskyfile
        PXOPT_LOOKUP_S64(magicked, config->args, "-set_magicked", false, false);
	PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

        if (magicked) {
            psStringAppend(&set_magicked_skyfile, "\n , warpSkyfile.magicked = %" PRId64, magicked);
            psStringAppend(&set_magicked_run, "\n,  warpRun.magicked = %" PRId64, magicked);
        }
	if (quality) {
	  psStringAppend(&set_magicked_skyfile, "\n , warpSkyfile.quality = %"PRId16, quality);
	}
	PXOPT_LOOKUP_S16(background_model, config->args, "-set_background_model", false, false);
	if (background_model) {
	  psStringAppend(&set_magicked_skyfile, "\n , warpSkyfile.background_model = %"PRId16, background_model);
	}
    } else if (!strcmp(data_state, "cleaned") || !strcmp(data_state, "purged")) {
        // if magicked is currently nonzero set it to -1
        // Set warpRun.magicked when the first skyfile is cleaned
        psStringAppend(&set_magicked_skyfile, "\n, warpSkyfile.magicked = IF(warpSkyfile.magicked = 0, 0, -1), warpRun.magicked = IF(warpRun.magicked = 0, 0, -1)");
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state, set_magicked_skyfile, warp_id, skycell_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);
    psFree(set_magicked_skyfile);

    query = pxDataGet("warptool_change_run_state.sql");
    if (!p_psDBRunQueryF(config->dbh, query, data_state, set_magicked_run, warp_id, data_state)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(set_magicked_run);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}
static bool tocleanedskyfileMode(pxConfig *config)
{
    return change_skyfile_data_state(config, "cleaned");
}
static bool tofullskyfileMode(pxConfig *config)
{
    return change_skyfile_data_state(config, "full");
}
static bool topurgedskyfileMode(pxConfig *config)
{
    return change_skyfile_data_state(config, "purged");
}
static bool toscrubbedskyfileMode(pxConfig *config)
{
     return change_skyfile_data_state(config, "scrubbed");
}

static bool updateskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // warp_id, skycell_id, fault are required
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_S16(background_model, config->args, "-set_background_model", false, false);
    if (background_model) {
      // CZW 2012-12-06: I'm unclear why we don't use this form for all updates?
      psMetadata *where = psMetadataAlloc();
      psMetadata *values = psMetadataAlloc();
      PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
      PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");
      PXOPT_COPY_S16(config->args, values, "-set_background_model", "background_model", "==");
      long rows = psDBUpdateRows(config->dbh,"warpSkyfile", where, values);
      psFree(values);
      psFree(where);
      if (!rows) {
	// This maybe should rollback and error if rows != 1
	psError(PS_ERR_UNKNOWN, true, "no rows changed");
	return false;
      }
    }
    else if (!state) {
      psMetadata *where = psMetadataAlloc();
      PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
      PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");
      if (psListLength(where->list) == 0) {
        // this won't happen because warptoolConfig requires these arguments
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        psFree(where);
        return false;
      }
 
      PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
      PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

      if (!pxSetFaultCode(config->dbh, "warpSkyfile", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree(where);
        return false;
      }
      psFree(where);
    }
    else {
      if (strcmp(state,"error_cleaned") == 0) {
        change_skyfile_data_state(config,"error_cleaned");
      }
      else if (strcmp(state, "error_scrubbed") == 0) {
        change_skyfile_data_state(config,"error_scrubbed");
      }
      else if (strcmp(state, "error_purged") == 0) {
        change_skyfile_data_state(config,"error_purged");
      }
      else {
        psError(PS_ERR_UNKNOWN, false, "unhandled state given");
        return(false);
      }
    }

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
    PXOPT_LOOKUP_BOOL(clean,  config->args, "-clean", false);

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
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");

    ExportTable tables [] = {
      {"warpRun", "warptool_export_run.sql"},
      {"warpImfile", "warptool_export_imfile.sql"},
      {"warpSkyfile", "warptool_export_skyfile.sql"},
      {"warpSkyCellMap", "warptool_export_skycell_map.sql"},
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

    if (clean) {
        bool success = true;
        if (!strcmp(tables[i].tableName, "warpRun")) {
            success = pxSetStateCleaned("warpRun", "state", output);
        } else if (!strcmp(tables[i].tableName, "warpSkyfile")) {
            success = pxSetStateCleaned("warpSkyfile", "data_state", output);
        }
        if (!success) {
            psFree(output);
            psError(PS_ERR_UNKNOWN, false, "pxSetStateClean failed for table %s",  tables[i].tableName);
            return false;
        }
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

  char tables[3] [80] = {"warpImfile", "warpSkyfile", "warpSkyCellMap"};

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

  psMetadataItem *item = psMetadataLookup (input, "warpRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  warpRunRow *warpRun = warpRunObjectFromMetadata (entry->data.md);
  warpRunInsertObject (config->dbh, warpRun);

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
          warpImfileRow *warpImfile = warpImfileObjectFromMetadata (entry->data.md);
          warpImfileInsertObject (config->dbh, warpImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 1:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          warpSkyfileRow *warpSkyfile = warpSkyfileObjectFromMetadata (entry->data.md);
          warpSkyfileInsertObject (config->dbh, warpSkyfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 2:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          warpSkyCellMapRow *warpSkyCellMap = warpSkyCellMapObjectFromMetadata (entry->data.md);
          warpSkyCellMapInsertObject (config->dbh, warpSkyCellMap);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;
    }
  }
  return true;
}

static bool runstateMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpRun.warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
    pxAddLabelSearchArgs (config, where, "-label",     "warpRun.label", "LIKE");

    // PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    // PXOPT_LOOKUP_BOOL(no_magic, config->args, "-no_magic", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("warptool_runstate.sql");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpRunState", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool listrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id",    "warpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "warpRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-state",      "warpRun.state", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",    "chipRun.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",     "camRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fake_id",    "fakeRun.fake_id", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawExp.dateobs",  ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawExp.dateobs",  "<=");
    PXOPT_COPY_STR(config->args, where, "-filter",    "rawExp.filter", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-magicked", "warpRun.magicked", "==");
    pxAddLabelSearchArgs (config, where, "-label",   "warpRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group",   "warpRun.data_group", "LIKE");
    pxAddLabelSearchArgs (config, where, "-dist_group",   "warpRun.dist_group", "LIKE");

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("warptool_listrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // generate where strings for arguments that require extra processing
    // beyond PXOPT_COPY*
    psString where2 = NULL;
    if (!pxmagicAddWhere(config, &where2, "warpRun")) {
        psError(psErrorCodeLast(), false, "pxMagicAddWhere failed");
        return false;
    }
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (!all && !where2) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }

    if (where2) {
        if (psListLength(where->list)) {
            psStringAppend(&query, " %s", where2);
        } else {
            psStringAppend(&query, " WHERE 1 %s", where2);
        }
    }
    psFree(where);

    if (pstamp_order) {
        // put runs in order of exposure id with newest warp Runs first
        // The postage stamp parser depends on this behavior
        psStringAppend(&query, "\nORDER by exp_id, warp_id DESC");
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
        psTrace("warptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

// a very specfic function to queue a cleaned warpSkyfile to be updated
static bool setskyfiletoupdateMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);

    psString query = pxDataGet("warptool_setskyfiletoupdate.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString setHook = psStringCopy("");
    if (label) {
        psStringAppend(&setHook, "\n , warpRun.label = '%s'", label);
    }

    if (skycell_id) {
        psStringAppend(&query, " AND (warpSkyfile.skycell_id = '%s')", skycell_id);
    }
    // we do not update components with the magic fault value. They are non-updateable
    // (But can be recovered with "warptool -revertwarped -fault 26" (PXTOOL_DO_NOT_REVERT_FAULT)
    psStringAppend(&query, " AND (warpSkyfile.fault != %d)", PXTOOL_DO_NOT_REVERT_FAULT);

    if (!p_psDBRunQueryF(config->dbh, query, setHook, warp_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(setHook);
    psFree(query);

    if (!skycell_id) {
        // If we are updateing a whole warpRun set skycells with bad quality to 'full'
        query = "UPDATE warpSkyfile SET data_state ='full', fault = 0 WHERE warp_id = %" PRId64 " AND quality != 0 AND (data_state != 'full')";
        if (!p_psDBRunQueryF(config->dbh, query, warp_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }

    return true;
}

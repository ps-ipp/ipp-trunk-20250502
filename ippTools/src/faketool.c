/*
 * faketool.c
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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "pxtools.h"
#include "pxdata.h"

#include "faketool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool pendingexpMode(pxConfig *config);
static bool pendingimfileMode(pxConfig *config);
static bool addprocessedimfileMode(pxConfig *config);
static bool processedimfileMode(pxConfig *config);
static bool revertprocessedimfileMode(pxConfig *config);
static bool updateprocessedimfileMode(pxConfig *config);
static bool advanceexpMode(pxConfig *config);
static bool blockMode(pxConfig *config);
static bool maskedMode(pxConfig *config);
static bool unmaskedMode(pxConfig *config);
static bool unblockMode(pxConfig *config);
static bool pendingcleanuprunMode(pxConfig *config);
static bool pendingcleanupimfileMode(pxConfig *config);
static bool donecleanupMode(pxConfig *config);
static bool tocleanedimfileMode(pxConfig *config);
static bool tofullimfileMode(pxConfig *config);
static bool topurgedimfileMode(pxConfig *config);
static bool toscrubbedimfileMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv) {
    psLibInit(NULL);

    pxConfig *config = faketoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(FAKETOOL_MODE_DEFINEBYQUERY,           definebyqueryMode);
        MODECASE(FAKETOOL_MODE_UPDATERUN,               updaterunMode);
        MODECASE(FAKETOOL_MODE_PENDINGEXP,              pendingexpMode);
        MODECASE(FAKETOOL_MODE_PENDINGIMFILE,           pendingimfileMode);
        MODECASE(FAKETOOL_MODE_ADDPROCESSEDIMFILE,      addprocessedimfileMode);
        MODECASE(FAKETOOL_MODE_PROCESSEDIMFILE,         processedimfileMode);
        MODECASE(FAKETOOL_MODE_REVERTPROCESSEDIMFILE,   revertprocessedimfileMode);
        MODECASE(FAKETOOL_MODE_UPDATEPROCESSEDIMFILE,   updateprocessedimfileMode);
        MODECASE(FAKETOOL_MODE_ADVANCEEXP,              advanceexpMode);
        MODECASE(FAKETOOL_MODE_BLOCK,                   blockMode);
        MODECASE(FAKETOOL_MODE_MASKED,                  maskedMode);
        MODECASE(FAKETOOL_MODE_UNMASKED,                unmaskedMode);
        MODECASE(FAKETOOL_MODE_UNBLOCK,                 unblockMode);
        MODECASE(FAKETOOL_MODE_PENDINGCLEANUPRUN,       pendingcleanuprunMode);
        MODECASE(FAKETOOL_MODE_PENDINGCLEANUPIMFILE,    pendingcleanupimfileMode);
        MODECASE(FAKETOOL_MODE_DONECLEANUP,             donecleanupMode);
        MODECASE(FAKETOOL_MODE_TOCLEANEDIMFILE,         tocleanedimfileMode);
        MODECASE(FAKETOOL_MODE_TOFULLIMFILE,            tofullimfileMode);
        MODECASE(FAKETOOL_MODE_TOPURGEDIMFILE,          topurgedimfileMode);
        MODECASE(FAKETOOL_MODE_TOSCRUBBEDIMFILE,        toscrubbedimfileMode);
        MODECASE(FAKETOOL_MODE_EXPORTRUN,               exportrunMode);
        MODECASE(FAKETOOL_MODE_IMPORTRUN,               importrunMode);

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

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-cam_id", "cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "dateobs", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_tag", "exp_tag", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "=="); // XXX does this choose the right label?
    PXOPT_COPY_STR(config->args, where, "-filelevel", "filelevel", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
    PXOPT_COPY_F64(config->args, where, "-airmass_min", "airmass", ">=");
    PXOPT_COPY_F64(config->args, where, "-airmass_max", "airmass", "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min", "ra", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max", "ra", "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min", "decl", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max", "decl", "<");
    PXOPT_COPY_F32(config->args, where, "-exp_time_min", "exp_time", ">=");
    PXOPT_COPY_F32(config->args, where, "-exp_time_max", "exp_time", "<");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_min", "sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_max", "sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_min", "bt", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_max", "bt", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_min", "bg_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_max", "bg_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_min", "bg_mean_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_max", "bg_mean_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-alt_min", "alt", ">=");
    PXOPT_COPY_F64(config->args, where, "-alt_max", "alt", "<");
    PXOPT_COPY_F64(config->args, where, "-az_min", "az", ">=");
    PXOPT_COPY_F64(config->args, where, "-az_max", "az", "<");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_min", "ccd_temp", ">=");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_max", "ccd_temp", "<");
    PXOPT_COPY_F64(config->args, where, "-posang_min", "posang", ">=");
    PXOPT_COPY_F64(config->args, where, "-posang_max", "posang", "<");
    PXOPT_COPY_STR(config->args, where, "-object", "object", "==");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_min", "sun_angle", ">=");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_max", "sun_angle", "<");
    PXOPT_COPY_STR(config->args, where, "-comment", "comment", "LIKE");

    if (!psListLength(where->list)
        && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(expgroup, config->args, "-set_expgroup", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-set_tess_id", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-set_end_stage", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    // default
    PXOPT_COPY_STR(config->args, where, "-exp_type", "exp_type", "==");
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("faketool_find_camrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }

    psFree(where);

    if (pretend) {
        // then stop before running the query
        fprintf(stderr, "%s\n", query);
        psFree(query);
        return true;
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
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

    // if end_stage is warp (or NULL), check for valid tess_id
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status;
        char *end_stage = psMetadataLookupStr(&status, md, "end_stage");
        if (end_stage && strcasecmp(end_stage, "warp")) continue;

        char *raw_tess_id   = psMetadataLookupStr(&status, md, "tess_id");
        if (raw_tess_id || tess_id) continue;

        char *label  = psMetadataLookupStr(&status, md, "label");
        psS64 exp_id = psMetadataLookupS64(&status, md, "exp_id");

        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue analysis to WARP without a defined tess id: label: %s, exp_id %" PRId64, label, exp_id);
            psFree(output);
            return false;
        }
    }

    // loop over our list of cam_ids
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status;
        psS64 cam_id = psMetadataLookupS64(&status, md, "cam_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for cam_id");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxfakeQueueByCamID(config, cam_id, workdir, label, data_group ? data_group : label, dist_group, reduction, expgroup, dvodb, tess_id, end_stage, note)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue cam_id: %" PRId64, cam_id);
            psFree(output);
            return false;
        }
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
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "dateobs", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_tag", "exp_tag", "==");
    PXOPT_COPY_STR(config->args, where, "-filelevel", "filelevel", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
    PXOPT_COPY_F64(config->args, where, "-airmass_min", "airmass", ">=");
    PXOPT_COPY_F64(config->args, where, "-airmass_max", "airmass", "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min", "ra", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max", "ra", "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min", "decl", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max", "decl", "<");
    PXOPT_COPY_F32(config->args, where, "-exp_time_min", "exp_time", ">=");
    PXOPT_COPY_F32(config->args, where, "-exp_time_max", "exp_time", "<");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_min", "sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_max", "sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_min", "bt", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_max", "bt", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_min", "bg_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_max", "bg_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_min", "bg_mean_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_max", "bg_mean_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-alt_min", "alt", ">=");
    PXOPT_COPY_F64(config->args, where, "-alt_max", "alt", "<");
    PXOPT_COPY_F64(config->args, where, "-az_min", "az", ">=");
    PXOPT_COPY_F64(config->args, where, "-az_max", "az", "<");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_min", "ccd_temp", ">=");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_max", "ccd_temp", "<");
    PXOPT_COPY_F64(config->args, where, "-posang_min", "posang", ">=");
    PXOPT_COPY_F64(config->args, where, "-posang_max", "posang", "<");
    PXOPT_COPY_STR(config->args, where, "-object", "object", "==");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_min", "sun_angle", ">=");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_max", "sun_angle", "<");
    PXOPT_COPY_STR(config->args, where, "-label", "fakeRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "fakeRun.state", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "fakeRun.data_group", "==");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "fakeRun.dist_group", "==");

    if (!psListLength(where->list)) {
        psFree(where);
        where = NULL;
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE fakeRun JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id)");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "fakeRun", "fake_id", "fakeProcessedImfile", true, false);
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
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id", "cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");

    psString query = pxDataGet("faketool_find_pendingexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("camtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakePendingExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool pendingimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "rawImfile.class_id", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "rawExp.telescope", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "rawExp.filter", "==");

    psString query = pxDataGet("faketool_pendingimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "fakeRun");
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
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakePendingImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool addprocessedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // fake_id, ext_tag, class_id are required
    PXOPT_LOOKUP_S64(fake_id, config->args,     "-fake_id", true, false);
    PXOPT_LOOKUP_S64(exp_id, config->args,      "-exp_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args,    "-class_id", true, false);

    // optional
    PXOPT_LOOKUP_STR(uri, config->args,         "-uri", false, false);

    PXOPT_LOOKUP_F32(dtime_fake, config->args,  "-dtime_fake", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args,    "-hostname", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args,   "-path_base", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args,        "-fault", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!fakeProcessedImfileInsert(config->dbh,
                                   fake_id,
                                   exp_id,
                                   class_id,
                                   uri,
                                   dtime_fake,
                                   dtime_script,
                                   hostname,
                                   path_base,
                                   "full",
                                   fault,
                                   NULL         // epoch
            )) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool processedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "rawImfile.class_id", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "rawExp.camera", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "rawExp.filter", "LIKE");

    psString query = pxDataGet("faketool_processedimfile.sql");
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

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "AND fakeProcessedImfile.fault != 0");
    } else {
        // don't list faulted rows
        psStringAppend(&query, " %s", "AND fakeProcessedImfile.fault = 0");
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakeProcessedImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool revertprocessedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "fakeRun.label", "==");
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fakeRun.fake_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "rawExp.exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "rawExp..camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "rawExp.telescope", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "dateobs", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_tag", "exp_tag", "==");
    PXOPT_COPY_STR(config->args, where, "-filelevel", "filelevel", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
    PXOPT_COPY_F64(config->args, where, "-airmass_min", "airmass", ">=");
    PXOPT_COPY_F64(config->args, where, "-airmass_max", "airmass", "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min", "ra", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max", "ra", "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min", "decl", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max", "decl", "<");
    PXOPT_COPY_F32(config->args, where, "-exp_time_min", "exp_time", ">=");
    PXOPT_COPY_F32(config->args, where, "-exp_time_max", "exp_time", "<");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_min", "sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_max", "sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_min", "bt", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_max", "bt", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_min", "bg_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_max", "bg_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_min", "bg_mean_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_max", "bg_mean_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-alt_min", "alt", ">=");
    PXOPT_COPY_F64(config->args, where, "-alt_max", "alt", "<");
    PXOPT_COPY_F64(config->args, where, "-az_min", "az", ">=");
    PXOPT_COPY_F64(config->args, where, "-az_max", "az", "<");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_min", "ccd_temp", ">=");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_max", "ccd_temp", "<");
    PXOPT_COPY_F64(config->args, where, "-posang_min", "posang", ">=");
    PXOPT_COPY_F64(config->args, where, "-posang_max", "posang", "<");
    PXOPT_COPY_STR(config->args, where, "-object", "object", "==");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_min", "sun_angle", ">=");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_max", "sun_angle", "<");

    if (!psListLength(where->list)
        && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("faketool_revertprocessedimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

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

    return true;
}

static bool updateprocessedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "class_id", "==");

    if (!pxSetFaultCode(config->dbh, "fakeProcessedImfile", where, fault, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree(where);
        return false;
    }
    psFree(where);

    return true;
}


static bool blockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    if (!fakeMaskInsert(config->dbh, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool maskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    if (where->list->n < 1) {
        psFree(where);
        where = NULL;
    }

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM fakeMask");

    if (where) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psFree(where);
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakeMask", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// return the list of labels which are NOT blocked
static bool unmaskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    if (where->list->n < 1) {
        psFree(where);
        where = NULL;
    }

    psString query = pxDataGet("faketool_unmasked.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where) {
        psString whereClause = psDBGenerateWhereSQL(where, "fakeUnmask");
        psFree(where);
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakeUnmask", !simple)) {
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

    char *query = "DELETE FROM fakeMask WHERE label = '%s'";

    if (!p_psDBRunQueryF(config->dbh, query, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool pendingcleanuprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "fakeRun.label", "==");

    psString query = pxDataGet("faketool_pendingcleanuprun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakePendingCleanupRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool pendingcleanupimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(fake_id, config->args, "-fake_id", false, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psMetadata *where = psMetadataAlloc();
    if (fake_id) {
        PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    }
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    psString query = pxDataGet("faketool_pendingcleanupimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakePendingCleanupImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool donecleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    psString query = pxDataGet("faketool_donecleanup.sql");
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "fakeDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool advanceexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // look for completed fakePendingExp
    // migrate them to fakeProccessedExp & camPendingExp
    psString query = pxDataGet("faketool_completely_processed_exp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("faketool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

        fakeRunRow *fakeRun = fakeRunObjectFromMetadata(row);
        if (!psDBTransaction(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        // set fakeRun.state to 'full'
        if (!pxfakeRunSetState(config, fakeRun->fake_id, "full")) {
            psError(PS_ERR_UNKNOWN, false, "failed to change fakeRun.state for fake_id: %" PRId64, fakeRun->fake_id);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psFree(fakeRun);
            psFree(output);
            return false;
        }

        // should we stop here or proceed on to the warp stage?
        // NULL for end_stage means go as far as possible
        if ((fakeRun->end_stage && psStrcasestr(fakeRun->end_stage, "fake")) || !fakeRun->tess_id) {
            psFree(fakeRun);
            if (!psDBCommit(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                return false;
            }
            continue;
        }
        // else continue on...

        // pxwarpQueueByFakeID() can only be run after fakeRun.state has been set to stop
        if (!pxwarpQueueByFakeID(config,
                                 fakeRun->fake_id,
                                 fakeRun->workdir,
                                 fakeRun->label,
                                 fakeRun->data_group,
                                 fakeRun->dist_group,
                                 fakeRun->dvodb,
                                 fakeRun->tess_id,
                                 fakeRun->reduction,
                                 fakeRun->end_stage,
                                 NULL // note does not propagate
        )) {
            psError(PS_ERR_UNKNOWN, false, "failed to queue warpRun");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psFree(fakeRun);
            psFree(output);
            return false;
        }
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(fakeRun);
            return false;
        }
        psFree(fakeRun);
    }

    psFree(output);

    return true;
}

// update fakeProcessedImfile.data_state to given value.
// afterwards, if all imfiles in the exposure have the new state, update the state for the exposure as well
// shared code for the modes -tocleanedimfile -tofullimfile -topurgedimfile

static bool change_imfile_data_state(pxConfig *config, psString data_state, psString run_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // fake_id, class_id are required
    PXOPT_LOOKUP_S64(fake_id, config->args, "-fake_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    psString query = pxDataGet("faketool_change_imfile_data_state.sql");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // note only updates if fakeRun.state = run_state
    if (!p_psDBRunQueryF(config->dbh, query, data_state, fake_id, class_id, run_state)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);

    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected atleast 1 row");
        return false;
    }

    query = pxDataGet("faketool_change_exp_state.sql");
    if (!p_psDBRunQueryF(config->dbh, query, data_state, fake_id, data_state)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}
static bool tocleanedimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "cleaned", "goto_cleaned");
}
static bool tofullimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "full", "update");
}
static bool topurgedimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "purged", "goto_purged");
}
static bool toscrubbedimfileMode(pxConfig *config)
{
     return change_imfile_data_state(config, "scrubbed", "goto_scrubbed");
}

bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 2;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // PXOPT_LOOKUP_S64(det_id, config->args, "-fake_id", true,  false);
  PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);
  PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);

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
  PXOPT_COPY_S64(config->args, where, "-fake_id", "fake_id", "==");

  ExportTable tables [] = {
    {"fakeRun", "faketool_export_run.sql"},
    {"fakeProcessedImfile", "faketool_export_processed_imfile.sql"},
  };

  for (int i=0; i < numExportTables; i++) {
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
        if (!strcmp(tables[i].tableName, "fakeRun")) {
            if(!pxSetStateCleaned("fakeRun", "state", output)) {
                psFree(output);
                psError(PS_ERR_UNKNOWN, false, "pxSetStateClean failed for table %s",  tables[i].tableName);
                return false;
            }
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
  psMetadataItem *item = psMetadataLookup (input, "fakeRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  fakeRunRow *fakeRun = fakeRunObjectFromMetadata (entry->data.md);
  fakeRunInsertObject (config->dbh, fakeRun);

  // fprintf (stdout, "---- fake run ----\n");
  // psMetadataPrint (stderr, entry->data.md, 1);

  item = psMetadataLookup (input, "fakeProcessedImfile");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  for (int i = 0; i < item->data.list->n; i++) {
    psMetadataItem *entry = psListGet (item->data.list, i);
    assert (entry);
    assert (entry->type == PS_DATA_METADATA);
    fakeProcessedImfileRow *fakeProcessedImfile = fakeProcessedImfileObjectFromMetadata (entry->data.md);
    fakeProcessedImfileInsertObject (config->dbh, fakeProcessedImfile);

    // fprintf (stdout, "---- row %d ----\n", i);
    // psMetadataPrint (stderr, entry->data.md, 1);
  }

  return true;
}

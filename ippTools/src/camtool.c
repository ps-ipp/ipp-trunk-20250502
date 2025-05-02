/*
 * camtool.c
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
#include <stdint.h>
#include <inttypes.h>

#include "pxtools.h"
#include "pxcam.h"
#include "camtool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool pendingexpMode(pxConfig *config);
static bool pendingimfileMode(pxConfig *config);
static bool addprocessedexpMode(pxConfig *config);
static bool processedexpMode(pxConfig *config);
static bool revertprocessedexpMode(pxConfig *config);
static bool updateprocessedexpMode(pxConfig *config);
static bool blockMode(pxConfig *config);
static bool maskedMode(pxConfig *config);
static bool unblockMode(pxConfig *config);
static bool pendingcleanuprunMode(pxConfig *config);
static bool pendingcleanupexpMode(pxConfig *config);
static bool donecleanupMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = camtoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(CAMTOOL_MODE_DEFINEBYQUERY,        definebyqueryMode);
        MODECASE(CAMTOOL_MODE_UPDATERUN,            updaterunMode);
        MODECASE(CAMTOOL_MODE_PENDINGEXP,           pendingexpMode);
        MODECASE(CAMTOOL_MODE_PENDINGIMFILE,        pendingimfileMode);
        MODECASE(CAMTOOL_MODE_ADDPROCESSEDEXP,      addprocessedexpMode);
        MODECASE(CAMTOOL_MODE_PROCESSEDEXP,         processedexpMode);
        MODECASE(CAMTOOL_MODE_REVERTPROCESSEDEXP,   revertprocessedexpMode);
        MODECASE(CAMTOOL_MODE_UPDATEPROCESSEDEXP,   updateprocessedexpMode);
        MODECASE(CAMTOOL_MODE_BLOCK,                blockMode);
        MODECASE(CAMTOOL_MODE_MASKED,               maskedMode);
        MODECASE(CAMTOOL_MODE_UNBLOCK,              unblockMode);
        MODECASE(CAMTOOL_MODE_PENDINGCLEANUPRUN,    pendingcleanuprunMode);
        MODECASE(CAMTOOL_MODE_PENDINGCLEANUPEXP,    pendingcleanupexpMode);
        MODECASE(CAMTOOL_MODE_DONECLEANUP,          donecleanupMode);
        MODECASE(CAMTOOL_MODE_EXPORTRUN,            exportrunMode);
        MODECASE(CAMTOOL_MODE_IMPORTRUN,            importrunMode);
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
    pxcamGetSearchArgs (config, where);
    pxAddLabelSearchArgs (config, where, "-label", "chipRun.label", "=="); // define using chipRun label
    PXOPT_COPY_STR(config->args, where, "-reduction", "chipRun.reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "chipRun.data_group", "==");
    PXOPT_COPY_STR(config->args, where, "-obs_mode", "rawExp.obs_mode", "==");

    if (!psListLength(where->list) &&
        !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(expgroup, config->args, "-set_expgroup", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-set_tess_id", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-set_end_stage", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    // default
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("camtool_find_chip_id.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
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
        psTrace("camtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "chipRun", !simple)) {
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

    // would could do this "all in the database" if we didn't want the option
    // of changing the label/reduction/expgroup/dvodb/etc.  So we're pulling the
    // data out so we have the option of changing these values or leaving the
    // old values in place (i.e., passing the values through).

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

    // loop over our list of chipRun rows
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        chipRunRow *row = chipRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into chipRun");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxcamQueueByChipID(config,
                    row->chip_id,
                    workdir     ? workdir   : row->workdir,
                    label       ? label     : row->label,
                    data_group  ? data_group: row->data_group,
                    dist_group  ? dist_group: row->dist_group,
                    reduction   ? reduction : row->reduction,
                    expgroup    ? expgroup  : row->expgroup,
                    dvodb       ? dvodb     : row->dvodb,
                    tess_id     ? tess_id   : row->tess_id,
                    end_stage   ? end_stage : row->end_stage,
                    row->magicked,
                    note
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue chip_id: %" PRId64, row->chip_id);
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
    pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",     "camRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group","camRun.data_group", "==");
    PXOPT_COPY_STR(config->args, where, "-state",     "camRun.state", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "camRun.reduction", "==");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    psString query = psStringCopy("UPDATE camRun JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id)");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "camRun", "cam_id", "camProcessedExp", true, true);
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
    pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "camRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "camRun.reduction", "==");

    psString query = pxDataGet("camtool_pendingexp.sql");
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

    psStringAppend(&query, "\nORDER BY priority DESC, cam_id");

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
    if (!ippdbPrintMetadatas(stdout, output, "camPendingExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id",                "==");
    pxAddLabelSearchArgs (config, where, "-label",    "camRun.label",                 "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "camRun.reduction",             "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",   "camRun.chip_id",              "==");
    PXOPT_COPY_STR(config->args, where, "-class_id",  "camProcessedExp.class_id", "==");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psString query = pxDataGet("camtool_find_pendingimfile.sql");
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
    if (!ippdbPrintMetadatas(stdout, output, "camProcessedExp", !simple)) {
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
    PXOPT_LOOKUP_S64(cam_id, config->args, "-cam_id", true, false);
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);

    // optional
    PXOPT_LOOKUP_F32(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F32(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F32(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F32(bias, config->args,           "-bias", false, false);
    PXOPT_LOOKUP_F32(bias_stdev, config->args,     "-bias_stdev", false, false);
    PXOPT_LOOKUP_F32(fringe_0, config->args,       "-fringe_0", false, false);
    PXOPT_LOOKUP_F32(fringe_1, config->args,       "-fringe_1", false, false);
    PXOPT_LOOKUP_F32(fringe_2, config->args,       "-fringe_2", false, false);
    PXOPT_LOOKUP_F32(sigma_ra, config->args, "-sigma_ra", false, false);
    PXOPT_LOOKUP_F32(sigma_dec, config->args, "-sigma_dec", false, false);
    PXOPT_LOOKUP_F32(ap_resid, config->args,       "-ap_resid", false, false);
    PXOPT_LOOKUP_F32(ap_resid_stdev, config->args, "-ap_resid_stdev", false, false);

    PXOPT_LOOKUP_F32(zpt_obs, config->args,       "-zpt_obs", false, false);
    PXOPT_LOOKUP_F32(zpt_err, config->args,       "-zpt_err", false, false);
    PXOPT_LOOKUP_F32(zpt_uq,  config->args,       "-zpt_uq", false, false);
    PXOPT_LOOKUP_F32(zpt_lq,  config->args,       "-zpt_lq", false, false);

    PXOPT_LOOKUP_F32(fwhm_major, config->args,     "-fwhm_major", false, false);
    PXOPT_LOOKUP_F32(fwhm_major_lq, config->args,     "-fwhm_major_lq", false, false);
    PXOPT_LOOKUP_F32(fwhm_major_uq, config->args,     "-fwhm_major_uq", false, false);

    PXOPT_LOOKUP_F32(fwhm_minor,    config->args,     "-fwhm_minor", false, false);
    PXOPT_LOOKUP_F32(fwhm_minor_lq, config->args,     "-fwhm_minor_lq", false, false);
    PXOPT_LOOKUP_F32(fwhm_minor_uq, config->args,     "-fwhm_minor_uq", false, false);

    PXOPT_LOOKUP_F32(iq_fwhm_major,     config->args,      "-iq_fwhm_major",     false, false);
    PXOPT_LOOKUP_F32(iq_fwhm_major_err, config->args,      "-iq_fwhm_major_err",     false, false);
    PXOPT_LOOKUP_F32(iq_fwhm_minor,     config->args,      "-iq_fwhm_minor",     false, false);
    PXOPT_LOOKUP_F32(iq_fwhm_minor_err, config->args,      "-iq_fwhm_minor_err",     false, false);

    PXOPT_LOOKUP_F32(iq_m2,     config->args,      "-iq_m2",     false, false);
    PXOPT_LOOKUP_F32(iq_m2_err, config->args,      "-iq_m2_err", false, false);
    PXOPT_LOOKUP_F32(iq_m2_uq,  config->args,      "-iq_m2_uq",  false, false);
    PXOPT_LOOKUP_F32(iq_m2_lq,  config->args,      "-iq_m2_lq",  false, false);

    PXOPT_LOOKUP_F32(iq_m2c,     config->args,     "-iq_m2c",     false, false);
    PXOPT_LOOKUP_F32(iq_m2c_err, config->args,     "-iq_m2c_err", false, false);
    PXOPT_LOOKUP_F32(iq_m2c_uq,  config->args,     "-iq_m2c_uq",  false, false);
    PXOPT_LOOKUP_F32(iq_m2c_lq,  config->args,     "-iq_m2c_lq",  false, false);

    PXOPT_LOOKUP_F32(iq_m2s,     config->args,     "-iq_m2s",     false, false);
    PXOPT_LOOKUP_F32(iq_m2s_err, config->args,     "-iq_m2s_err", false, false);
    PXOPT_LOOKUP_F32(iq_m2s_uq,  config->args,     "-iq_m2s_uq",  false, false);
    PXOPT_LOOKUP_F32(iq_m2s_lq,  config->args,     "-iq_m2s_lq",  false, false);

    PXOPT_LOOKUP_F32(iq_m3,     config->args,      "-iq_m3",     false, false);
    PXOPT_LOOKUP_F32(iq_m3_err, config->args,      "-iq_m3_err", false, false);
    PXOPT_LOOKUP_F32(iq_m3_uq,  config->args,      "-iq_m3_uq",  false, false);
    PXOPT_LOOKUP_F32(iq_m3_lq,  config->args,      "-iq_m3_lq",  false, false);

    PXOPT_LOOKUP_F32(iq_m4,     config->args,      "-iq_m4",     false, false);
    PXOPT_LOOKUP_F32(iq_m4_err, config->args,      "-iq_m4_err", false, false);
    PXOPT_LOOKUP_F32(iq_m4_uq,  config->args,      "-iq_m4_uq",  false, false);
    PXOPT_LOOKUP_F32(iq_m4_lq,  config->args,      "-iq_m4_lq",  false, false);

    PXOPT_LOOKUP_F32(dtime_script, config->args,   "-dtime_script", false, false);
    PXOPT_LOOKUP_F32(dtime_astrom, config->args,   "-dtime_astrom", false, false);
    PXOPT_LOOKUP_F32(dtime_addstar, config->args,  "-dtime_addstar", false, false);

    PXOPT_LOOKUP_STR(hostname, config->args,       "-hostname", false, false);
    PXOPT_LOOKUP_S32(n_stars, config->args,        "-n_stars", false, false);
    PXOPT_LOOKUP_S32(n_psfstars, config->args,        "-n_psfstars", false, false);
    PXOPT_LOOKUP_S32(n_iqstars, config->args,        "-n_iqstars", false, false);
    PXOPT_LOOKUP_S32(n_extended, config->args,     "-n_extended", false, false);
    PXOPT_LOOKUP_S32(n_cr, config->args,           "-n_cr", false, false);
    PXOPT_LOOKUP_S32(n_astrom, config->args,       "-n_astrom", false, false);

    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);
    PXOPT_LOOKUP_S16(background_model, config->args, "-background_model", false, false);
    PXOPT_LOOKUP_S64(astrom_chips, config->args, "-astrom_chips", false, false);

    
    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_psphot, config->args, "-ver_psphot", false, false);
    PXOPT_LOOKUP_STR(ver_psastro, config->args, "-ver_psastro", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_STR(ver_ppimage, config->args, "-ver_ppimage", false, false);
    PXOPT_LOOKUP_STR(ver_streaks, config->args, "-ver_streaks", false, false);

    PXOPT_LOOKUP_S32(maskfrac_ref_npix, config->args, "-maskfrac_ref_npix", false, false);
    PXOPT_LOOKUP_F32(maskfrac_ref_static, config->args, "-maskfrac_ref_static", false, false);
    PXOPT_LOOKUP_F32(maskfrac_ref_dynamic, config->args, "-maskfrac_ref_dynamic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_ref_magic, config->args, "-maskfrac_ref_magic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_ref_advisory, config->args, "-maskfrac_ref_advisory", false, false);

    PXOPT_LOOKUP_S32(maskfrac_max_npix, config->args, "-maskfrac_max_npix", false, false);
    PXOPT_LOOKUP_F32(maskfrac_max_static, config->args, "-maskfrac_max_static", false, false);
    PXOPT_LOOKUP_F32(maskfrac_max_dynamic, config->args, "-maskfrac_max_dynamic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_max_magic, config->args, "-maskfrac_max_magic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_max_advisory, config->args, "-maskfrac_max_advisory", false, false);
    
    PXOPT_LOOKUP_F32(ast_r0, config->args, "-ast_r0", false, false);
    PXOPT_LOOKUP_F32(ast_d0, config->args, "-ast_d0", false, false);
    PXOPT_LOOKUP_F32(ast_t0, config->args, "-ast_t0", false, false);
    PXOPT_LOOKUP_F32(ast_s0, config->args, "-ast_s0", false, false);
    PXOPT_LOOKUP_F32(ast_rs, config->args, "-ast_rs", false, false);
    PXOPT_LOOKUP_F32(ast_ds, config->args, "-ast_ds", false, false);

    // we store actual detection efficiency by adding in zpt_obs
    PXOPT_LOOKUP_F32(deteff_inst, config->args, "-deteff_inst", false, false);
    PXOPT_LOOKUP_F32(deteff_inst_lq, config->args, "-deteff_inst_lq", false, false);
    PXOPT_LOOKUP_F32(deteff_inst_uq, config->args, "-deteff_inst_uq", false, false);
    // error is dd
    PXOPT_LOOKUP_F32(deteff_err, config->args, "-deteff_inst_err", false, false);
    psF32 deteff = NAN;
    psF32 deteff_uq = NAN;
    psF32 deteff_lq = NAN;
    if (isfinite(zpt_obs)) {
        if (isfinite(deteff_inst)) {
            deteff = deteff_inst + zpt_obs;
        }
        if (isfinite(deteff_inst_uq)) {
            deteff_uq = deteff_inst_uq + zpt_obs;
        }
        if (isfinite(deteff_inst_lq)) {
            deteff_lq = deteff_inst_lq + zpt_obs;
        }
    }

    psString software_ver = NULL;
    if ((ver_pslib)&&(ver_psmodules)) {
      software_ver = pxMergeCodeVersions(ver_pslib,ver_psmodules);
    }
    if (ver_psphot) {
      software_ver = pxMergeCodeVersions(software_ver,ver_psphot);
    }
    if (ver_psastro) {
      software_ver = pxMergeCodeVersions(software_ver,ver_psastro);
    }
    if (ver_ppstats) {
      software_ver = pxMergeCodeVersions(software_ver,ver_ppstats);
    }
    if (ver_ppimage) {
      software_ver = pxMergeCodeVersions(software_ver,ver_ppimage);
    }
    if (ver_streaks) {
      software_ver = pxMergeCodeVersions(software_ver,ver_streaks);
    }

//    Get this from the chipRun
//    PXOPT_LOOKUP_S64(magicked, config->args, "-magicked", false, false);

    // generate restrictions
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-cam_id",   "camRun.cam_id",   "==");

    psString query = pxDataGet("camtool_addprocessedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
        psString whereClaus = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClaus);
        psFree(whereClaus);
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
        psTrace("camtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return false;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    bool status;
    psS64 chip_magicked = psMetadataLookupS64(&status, output->data[0], "chip_magicked");

    camRunRow *pendingRow = camRunObjectFromMetadata(output->data[0]);
    psFree(output);
    camProcessedExpRow *row = camProcessedExpRowAlloc(
        pendingRow->cam_id,
        uri,
        bg,
        bg_stdev,
        bg_mean_stdev,
        bias,
        bias_stdev,
        fringe_0,
        fringe_1,
        fringe_2,
        sigma_ra,
        sigma_dec,
        ap_resid,
        ap_resid_stdev,
        zpt_obs,
        zpt_err,
        zpt_lq,
        zpt_uq,
        fwhm_major,
        fwhm_major_lq,
        fwhm_major_uq,
        fwhm_minor,
        fwhm_minor_lq,
        fwhm_minor_uq,

        iq_fwhm_major,
        iq_fwhm_major_err,
        iq_fwhm_minor,
        iq_fwhm_minor_err,

        iq_m2,
        iq_m2_err,
        iq_m2_lq,
        iq_m2_uq,
        iq_m2c,
        iq_m2c_err,
        iq_m2c_lq,
        iq_m2c_uq,
        iq_m2s,
        iq_m2s_err,
        iq_m2s_lq,
        iq_m2s_uq,
        iq_m3,
        iq_m3_err,
        iq_m3_lq,
        iq_m3_uq,
        iq_m4,
        iq_m4_err,
        iq_m4_lq,
        iq_m4_uq,
        dtime_script,
        dtime_astrom,
        dtime_addstar,
        hostname,
        n_stars,
        n_psfstars,
        n_iqstars,
        n_extended,
        n_cr,
        n_astrom,
        path_base,
        fault,
        software_ver,
        maskfrac_ref_npix,
        maskfrac_ref_static,
        maskfrac_ref_dynamic,
        maskfrac_ref_magic,
        maskfrac_ref_advisory,
        maskfrac_max_npix,
        maskfrac_max_static,
        maskfrac_max_dynamic,
        maskfrac_max_magic,
        maskfrac_max_advisory,
        deteff,
        deteff_err,
        deteff_lq,
        deteff_uq,
        quality,
	background_model,
        astrom_chips,

        ast_r0,
        ast_d0,
        ast_t0,
        ast_s0,
        ast_rs,
        ast_ds
        );

    if (!camProcessedExpInsertObject(config->dbh, row)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        psFree(pendingRow);
        return false;
    }

    if (fault) {
        psFree(row);
        psFree(pendingRow);
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }
    // else continue on...


    // Set camRun.software_ver to the appropriate value
    if (!pxSetRunSoftware(config, "camRun", "cam_id", cam_id, software_ver)) {
      if (!psDBRollback(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PS_ERR_UNKNOWN, false, "failed to set camRun.software_ver for cam_id: %" PRId64,
              cam_id);
      psFree(output);
      return(false);
    }
    // Set chipRun.maskfrac* to the appropriate values.
    if (maskfrac_ref_npix) {
      if (!pxCamSetRunMaskfrac(config, "camRun", "cam_id",cam_id,
                               (float) maskfrac_ref_npix, maskfrac_ref_static,
                               maskfrac_ref_dynamic, maskfrac_ref_magic, maskfrac_ref_advisory,
                               (float) maskfrac_max_npix, maskfrac_max_static,
                               maskfrac_max_dynamic, maskfrac_max_magic, maskfrac_max_advisory)) {
        if (!psDBRollback(config->dbh)) {
          psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to set camRun.maskstats for cam_id: %" PRId64,
                cam_id);
        psFree(output);
        return(false);
      }
    }

    // since there is only one exp per 'new' set camRun.state = 'full'
    // propagate magicked state from chipRun
    if (!pxcamRunSetState(config, row->cam_id, "full", chip_magicked)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change camRun.state for cam_id: %" PRId64, row->cam_id);
        psFree(row);
        psFree(pendingRow);
        return false;
    }

    psFree(row);

    // EAM:  NULL for end_stage means go as far as possible
    // Also, we can run fake even if tess_id is not defined
    // but stop if quality is non-zero.
    if ((quality > 0) || (pendingRow->end_stage && psStrcasestr(pendingRow->end_stage, "cam"))) {
        psFree(pendingRow);
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }

    if (!pxfakeQueueByCamID(config,
            pendingRow->cam_id,
            pendingRow->workdir,
            pendingRow->label,
            pendingRow->data_group,
            pendingRow->dist_group,
            pendingRow->reduction,
            pendingRow->expgroup,
            pendingRow->dvodb,
            pendingRow->tess_id,
            pendingRow->end_stage,
            NULL    // note does not propagate
    )) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to queue new fakeRun");
        psFree(pendingRow);
        return false;
    }

    psFree(pendingRow);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);

    // generate restrictions
    psMetadata *where = psMetadataAlloc();
    pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id",    "==");
    pxAddLabelSearchArgs (config, where, "-label",    "camRun.label",     "==");
    pxAddLabelSearchArgs (config, where, "-data_group", "camRun.data_group",     "LIKE");
    PXOPT_COPY_STR(config->args, where, "-reduction", "camRun.reduction", "==");
    PXOPT_COPY_S16(config->args, where, "-background_model", "camProcessedExp.background_model", "==");
    
    psString where2 = NULL;
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }
    if (!pxmagicAddWhere(config, &where2, "chipRun")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }
    if (!psListLength(where->list) && !where2 &&
        !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters (or -all) are required");
        return false;
    }

    psString query = pxDataGet("camtool_find_processedexp.sql");
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
        psStringAppend(&query, " %s", " AND camProcessedExp.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND camProcessedExp.fault = 0");
    }
    if (!where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE camProcessedExp.fault != 0");
    }
    if (!where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE camProcessedExp.fault = 0");
    }
    psFree(where);
    if (where2) {
        psStringAppend(&query, " %s", where2);
        psFree(where2);
    }


    if (pstamp_order) {
        // put runs in order of exposure id with newest chip Runs first
        // The postage stamp parser depends on this behavior
        psStringAppend(&query, " ORDER BY exp_id, cam_id DESC");
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
        psTrace("camtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "camProcessedExp", !simple)) {
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
    pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id",         "==");
    pxAddLabelSearchArgs (config, where, "-label",    "camRun.label",     "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "camRun.reduction",      "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "camProcessedExp.fault", "==");

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
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
        psString query = pxDataGet("camtool_revertprocessedexp.sql");
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
    psLogMsg("camtool", PS_LOG_INFO, "Deleted %d camProcessedExps", numDeleted);

    {
        psString query = pxDataGet("camtool_revertupdatedexp.sql");
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
    psLogMsg("camtool", PS_LOG_INFO, "Updated %d camProcessedExps", numUpdated);

    return true;
}


static bool updateprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);
    PXOPT_LOOKUP_S16(background_model, config->args, "-set_background_model", false, false);
    if (background_model) {
      // CZW Copied from warptool.c
      psMetadata *where = psMetadataAlloc();
      psMetadata *values = psMetadataAlloc();
      PXOPT_COPY_S64(config->args, where, "-cam_id", "cam_id", "==");
      PXOPT_COPY_S16(config->args, values, "-set_background_model", "background_model", "==");
      long rows = psDBUpdateRows(config->dbh, "camProcessedExp", where, values);
      psFree(values);
      psFree(where);
      if (!rows) {
	psError(PS_ERR_UNKNOWN, true, "no rows changed");
      }
    }
    else {
      psMetadata *where = psMetadataAlloc();
      PXOPT_COPY_S64(config->args, where, "-cam_id",   "cam_id",   "==");
      PXOPT_COPY_S64(config->args, where, "-chip_id",  "chip_id",  "==");
      PXOPT_COPY_STR(config->args, where, "-class",    "class",    "==");
      PXOPT_COPY_STR(config->args, where, "-class_id", "class_id", "==");
      
      if (!pxSetFaultCode(config->dbh, "camProcessedExp", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
      }
      psFree (where);
    }
    return true;
}


static bool blockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    if (!camMaskInsert(config->dbh, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool maskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM camMask");

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

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "camMask", !simple)) {
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

    char *query = "DELETE FROM camMask WHERE label = '%s'";

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
    pxAddLabelSearchArgs (config, where, "-label", "camRun.label", "==");

    psString query = pxDataGet("camtool_pendingcleanuprun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

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

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "camPendingCleanupRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool pendingcleanupexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(cam_id, config->args, "-cam_id", false, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psMetadata *where = psMetadataAlloc();
    if (cam_id) {
        PXOPT_COPY_S64(config->args, where, "-cam_id", "cam_id", "==");
    }
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    psString query = pxDataGet("camtool_pendingcleanupexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

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
        psTrace("chiptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "camPendingCleanupExp", !simple)) {
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

    psString query = pxDataGet("camtool_donecleanup.sql");
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
        psTrace("camtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "camDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 2;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // PXOPT_LOOKUP_S64(det_id, config->args, "-cam_id", true,  false);
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
  PXOPT_COPY_S64(config->args, where, "-cam_id", "cam_id", "==");

  ExportTable tables [] = {
    {"camRun", "camtool_export_run.sql"},
    {"camProcessedExp", "camtool_export_processed_exp.sql"},
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
        if (!strcmp(tables[i].tableName, "camRun")) {
            if (!pxSetStateCleaned("camRun", "state", output)) {
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

  psMetadataItem *item = psMetadataLookup (input, "camRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  camRunRow *camRun = camRunObjectFromMetadata (entry->data.md);
  camRunInsertObject (config->dbh, camRun);

  // fprintf (stdout, "---- cam run ----\n");
  // psMetadataPrint (stderr, entry->data.md, 1);

  item = psMetadataLookup (input, "camProcessedExp");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  for (int i = 0; i < item->data.list->n; i++) {
    psMetadataItem *entry = psListGet (item->data.list, i);
    assert (entry);
    assert (entry->type == PS_DATA_METADATA);
    camProcessedExpRow *camProcessedExp = camProcessedExpObjectFromMetadata (entry->data.md);
    camProcessedExpInsertObject (config->dbh, camProcessedExp);

    // fprintf (stdout, "---- row %d ----\n", i);
    // psMetadataPrint (stderr, entry->data.md, 1);
  }

  return true;
}

/*
 * difftool.c
 *
 * Copyright (C) 2007  Joshua Hoblitt
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
#include "difftool.h"

static bool definerunMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool addinputskyfileMode(pxConfig *config);
static bool inputskyfileMode(pxConfig *config);
static bool todiffskyfileMode(pxConfig *config);
static bool adddiffskyfileMode(pxConfig *config);
static bool advanceMode(pxConfig *config);
static bool diffskyfileMode(pxConfig *config);
static bool revertdiffskyfileMode(pxConfig *config);
static bool definepoprunMode(pxConfig *config);
static bool definewarpstackMode(pxConfig *config);
//static bool definewarpstackOldMethodMode(pxConfig *config);
static bool definewarpwarpMode(pxConfig *config);
static bool definestackstackMode(pxConfig *config);
static bool tosummaryMode(pxConfig *config);
static bool addsummaryMode(pxConfig *config);
static bool pendingcleanuprunMode(pxConfig *config);
static bool pendingcleanupskyfileMode(pxConfig *config);
static bool revertcleanupMode(pxConfig *config);
static bool donecleanupMode(pxConfig *config);
static bool updatediffskyfileMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

static bool setdiffRunState(pxConfig *config, psS64 diff_id, const char *state, psS64 magicked);
static bool change_skyfile_data_state(pxConfig *config, psString data_state, psString run_state);
static bool tocleanedskyfileMode(pxConfig *config);
static bool topurgedskyfileMode(pxConfig *config);
static bool toscrubbedskyfileMode(pxConfig *config);
static bool tofullskyfileMode(pxConfig *config);
static bool listrunMode(pxConfig *config);
static bool listssrunMode(pxConfig *config);
static bool setskyfiletoupdateMode(pxConfig *config);



# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = difftoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(DIFFTOOL_MODE_DEFINERUN,             definerunMode);
        MODECASE(DIFFTOOL_MODE_UPDATERUN,             updaterunMode);
        MODECASE(DIFFTOOL_MODE_ADDINPUTSKYFILE,       addinputskyfileMode);
        MODECASE(DIFFTOOL_MODE_INPUTSKYFILE,          inputskyfileMode);
        MODECASE(DIFFTOOL_MODE_TODIFFSKYFILE,         todiffskyfileMode);
        MODECASE(DIFFTOOL_MODE_ADDDIFFSKYFILE,        adddiffskyfileMode);
        MODECASE(DIFFTOOL_MODE_ADVANCE,               advanceMode);
        MODECASE(DIFFTOOL_MODE_DIFFSKYFILE,           diffskyfileMode);
        MODECASE(DIFFTOOL_MODE_REVERTDIFFSKYFILE,     revertdiffskyfileMode);
        MODECASE(DIFFTOOL_MODE_DEFINEPOPRUN,          definepoprunMode);
        MODECASE(DIFFTOOL_MODE_DEFINEWARPSTACK,       definewarpstackMode);
	//	MODECASE(DIFFTOOL_MODE_DEFINEWARPSTACKOLDMETHOD,       definewarpstackOldMethodMode);
        MODECASE(DIFFTOOL_MODE_DEFINEWARPWARP,        definewarpwarpMode);
        MODECASE(DIFFTOOL_MODE_DEFINESTACKSTACK,      definestackstackMode);
        MODECASE(DIFFTOOL_MODE_TOSUMMARY,             tosummaryMode);
        MODECASE(DIFFTOOL_MODE_ADDSUMMARY,            addsummaryMode);
        MODECASE(DIFFTOOL_MODE_PENDINGCLEANUPRUN,     pendingcleanuprunMode);
        MODECASE(DIFFTOOL_MODE_PENDINGCLEANUPSKYFILE, pendingcleanupskyfileMode);
        MODECASE(DIFFTOOL_MODE_REVERTCLEANUP,         revertcleanupMode);
        MODECASE(DIFFTOOL_MODE_DONECLEANUP,           donecleanupMode);
        MODECASE(DIFFTOOL_MODE_UPDATEDIFFSKYFILE,     updatediffskyfileMode);
        MODECASE(DIFFTOOL_MODE_EXPORTRUN,             exportrunMode);
        MODECASE(DIFFTOOL_MODE_IMPORTRUN,             importrunMode);
        MODECASE(DIFFTOOL_MODE_TOCLEANEDSKYFILE,      tocleanedskyfileMode);
        MODECASE(DIFFTOOL_MODE_TOPURGEDSKYFILE,       topurgedskyfileMode);
        MODECASE(DIFFTOOL_MODE_TOSCRUBBEDSKYFILE,     toscrubbedskyfileMode);
        MODECASE(DIFFTOOL_MODE_TOFULLSKYFILE,         tofullskyfileMode);
        MODECASE(DIFFTOOL_MODE_LISTRUN,               listrunMode);
        MODECASE(DIFFTOOL_MODE_LISTSSRUN,             listssrunMode);
        MODECASE(DIFFTOOL_MODE_SETSKYFILETOUPDATE,    setskyfiletoupdateMode);

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


static bool definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required options
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", true, false);
    PXOPT_LOOKUP_BOOL(bothways, config->args, "-bothways", false);
    PXOPT_LOOKUP_BOOL(exposure, config->args, "-exposure", false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_S16(diff_mode, config->args, "-set_diff_mode", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    // default
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    diffRunRow *run = diffRunRowAlloc(
            0,          // ID
            "reg",      // state
            workdir,
            label,
            data_group ? data_group : label,
            dist_group,
            reduction,
            NULL,       // dvodb
            registered,
            tess_id,
            bothways,
            exposure,
            false,
            NULL, // software version
            0,    // mask stat npix
            NAN,    // static
            NAN,    // dynamic
            NAN,    // magic
            NAN,    // advisory
            diff_mode,
            note
    );
    if (!run) {
        psError(PS_ERR_UNKNOWN, false, "failed to alloc diffRun object");
        return false;
    }
    if (!diffRunInsertObject(config->dbh, run)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(run);
        return false;
    }

    // get the assigned diff_id
    run->diff_id = psDBLastInsertID(config->dbh);

    if (!diffRunPrintObject(stdout, run, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(run);
        return false;
    }

    psFree(run);

    return true;
}


static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-diff_id",  "diff_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-label",     "label",     "LIKE");
    PXOPT_COPY_STR(config->args, where, "-data_group","data_group","LIKE");
    PXOPT_COPY_STR(config->args, where, "-dist_group","dist_group","LIKE");
    PXOPT_COPY_STR(config->args, where, "-state",     "state",     "==");
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE diffRun");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "diffRun", "diff_id", "diffSkyfile", true, true);

    psFree(query);
    psFree(where);

    return result;
}


static bool addinputskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required options
    PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", true, false);

    // optional
    PXOPT_LOOKUP_S64(stack1, config->args, "-stack1", false, false);
    PXOPT_LOOKUP_S64(stack2, config->args, "-stack2", false, false);
    PXOPT_LOOKUP_S64(warp1, config->args, "-warp1", false, false);
    PXOPT_LOOKUP_S64(warp2, config->args, "-warp2", false, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);

    // must provide either stack1 or warp1 but not BOTH
    if (!(stack1 || warp1)) {
        psError(PS_ERR_UNKNOWN, true, "either -stack1 or -warp1 must be specified");
        return false;
    }
    if (stack1 && warp1) {
        psError(PS_ERR_UNKNOWN, true, "either -stack1 or -warp1 must be specified");
        return false;
    }
    // must provide either stack2 or warp2 but not BOTH
    if (!(stack2 || warp2)) {
        psError(PS_ERR_UNKNOWN, true, "either -stack2 or -warp2 must be specified");
        return false;
    }
    if (stack2 && warp2) {
        psError(PS_ERR_UNKNOWN, true, "either -stack2 or -warp2 must be specified");
        return false;
    }

    // if a warp1 was provided we need to lookup the and tess_id from the diffRun
    psString tess_id = NULL;
    if (warp1) {
        if (!p_psDBRunQueryF(config->dbh, "SELECT * from diffRun WHERE diff_id = %" PRId64, diff_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        psArray *output = p_psDBFetchResult(config->dbh);
        if (!output) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        if (!psArrayLength(output)) {
            psError(PS_ERR_UNKNOWN, false, "diff_id %" PRId64 " not found", diff_id);
            psFree(output);
            return false;
        }

        diffRunRow *run = diffRunObjectFromMetadata(output->data[0]);
        tess_id = run->tess_id;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!diffInputSkyfileInsert(config->dbh,
            diff_id,
            skycell_id,
            warp1 ? warp1 : PS_MAX_S64, // defined or NULL
            stack1 ? stack1 : PS_MAX_S64, // defined or NULL
            warp2 ? warp2 : PS_MAX_S64, // defined or NULL
            stack2 ? stack2 : PS_MAX_S64, // defined or NULL
            tess_id,
            0                             // diff_skyfile_id
        )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, "SELECT count(diff_id) FROM diffRun JOIN diffInputSkyfile USING(diff_id) WHERE diff_id = %" PRId64, diff_id)) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "diff_id %" PRId64 " not found", diff_id);
        psFree(output);
        return false;
    }

    bool status;
    psS32 count = psMetadataLookupS32(&status, output->data[0], "count(diff_id)");

    if (count == 2) {
        if (!setdiffRunState(config, diff_id, "new", false)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool inputskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where,  "-diff_id", "diff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "diffInputSkyfile.skycell_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-tess_id", "tess_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(template, config->args, "-template", false);
    PXOPT_LOOKUP_BOOL(input, config->args, "-input", false);

    if (template && input) {
        // User apparently wants both, which is the default behaviour
        template = false;
        input = false;
    }

    // find all rawImfiles matching the default query
    psString query = pxDataGet("difftool_inputskyfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = psStringCopy("");
    if (psListLength(where->list)) {
        whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringPrepend(&whereClause, "\n WHERE ");
    }
    psFree(where);

    // Add condition to get only templates or only inputs
    psString templateClause = psStringCopy("");
    {
        if (template) {
            psStringAppend(&templateClause, "\n WHERE %s", " template != 0");
        } else if (input) {
            psStringAppend(&templateClause, "\n WHERE %s", " template = 0");
        }
    }


    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereClause, whereClause, whereClause, whereClause, templateClause)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(templateClause);
        psFree(whereClause);
        psFree(query);
        return false;
    }
    psFree(templateClause);
    psFree(whereClause);
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffInputSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool todiffskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_id", "diffRun.diff_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "diffRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psString query = pxDataGet("difftool_todiffskyfile.sql");
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
        psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (try -all");
        return false;
      }
    }
    psFree(where);

     psStringAppend(&query, "\nORDER by priority DESC, diff_id, skycell_id");

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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool adddiffskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", true, false); // required
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", (fault == 0), false);

    // optional
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F32(dtime_diff, config->args, "-dtime_diff", false, false);
    PXOPT_LOOKUP_F32(dtime_match, config->args, "-dtime_match", false, false);
    PXOPT_LOOKUP_F32(dtime_phot, config->args, "-dtime_phot", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_S32(stamps_num, config->args, "-stamps_num", false, false);
    PXOPT_LOOKUP_F32(stamps_mean, config->args, "-stamps_mean", false, false);
    PXOPT_LOOKUP_F32(stamps_rms, config->args, "-stamps_rms", false, false);
    PXOPT_LOOKUP_F32(norm, config->args, "-norm", false, false);
    PXOPT_LOOKUP_F32(bg_diff, config->args, "-bg_diff", false, false);
    PXOPT_LOOKUP_F32(kernel_x, config->args, "-kernel_x", false, false);
    PXOPT_LOOKUP_F32(kernel_y, config->args, "-kernel_y", false, false);
    PXOPT_LOOKUP_F32(kernel_xx, config->args, "-kernel_xx", false, false);
    PXOPT_LOOKUP_F32(kernel_xy, config->args, "-kernel_xy", false, false);
    PXOPT_LOOKUP_F32(kernel_yy, config->args, "-kernel_yy", false, false);
    PXOPT_LOOKUP_F32(deconv_max, config->args, "-deconv_max", false, false);
    PXOPT_LOOKUP_S32(sources, config->args, "-sources", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_F32(good_frac, config->args, "-good_frac", false, false);
    PXOPT_LOOKUP_S64(magicked, config->args, "-magicked", false, false);

    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_psphot, config->args, "-ver_psphot", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_STR(ver_ppsub, config->args, "-ver_ppsub", false, false);
    PXOPT_LOOKUP_STR(ver_streaks, config->args, "-ver_streaks", false, false);

    PXOPT_LOOKUP_S32(maskfrac_npix, config->args, "-maskfrac_npix", false, false);
    PXOPT_LOOKUP_F32(maskfrac_static, config->args, "-maskfrac_static", false, false);
    PXOPT_LOOKUP_F32(maskfrac_dynamic, config->args, "-maskfrac_dynamic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_magic, config->args, "-maskfrac_magic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_advisory, config->args, "-maskfrac_advisory", false, false);

    psTrace("czw.test",1,"Received versions: pslib %s psmodules %s psphot %s ppstats %s ppsub %s streaks %s\n",
            ver_pslib,ver_psmodules,ver_psphot,ver_ppstats,ver_ppsub,ver_streaks);
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
    if (ver_ppsub) {
      ver_code = pxMergeCodeVersions(ver_code,ver_ppsub);
    }
    if (ver_streaks) {
      ver_code = pxMergeCodeVersions(ver_code,ver_streaks);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!diffSkyfileInsert(config->dbh,
                           diff_id,
                           skycell_id,
                           path_base,
                           "full",
                           bg,
                           bg_stdev,
                           stamps_num,
                           stamps_mean,
                           stamps_rms,
                           norm,
                           bg_diff,
                           kernel_x,
                           kernel_y,
                           kernel_xx,
                           kernel_xy,
                           kernel_yy,
                           deconv_max,
                           sources,
                           dtime_diff,
                           dtime_match,
                           dtime_phot,
                           dtime_script,
                           hostname,
                           good_frac,
                           fault,
                           quality,
                           magicked,
                           ver_code,
                           maskfrac_npix,
                           maskfrac_static,
                           maskfrac_dynamic,
                           maskfrac_magic,
                           maskfrac_advisory
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

static bool advanceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_id", "diffRun.diff_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "diffRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // look for completed diffRuns
    psString query = pxDataGet("difftool_completed_runs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereString = psStringCopy("");
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereString, "\n AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereString);
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereString)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        psFree(whereString);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);
    psFree(whereString);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

        psS64 diff_id = psMetadataLookupS64(NULL, row, "diff_id");
        psS64 magicked = psMetadataLookupS64(NULL, row, "magicked");

        psString software_ver = NULL;
        psS64 maskfrac_npix = 0;
        psF32 maskfrac_static = 0;
        psF32 maskfrac_dynamic = 0;
        psF32 maskfrac_magic = 0;
        psF32 maskfrac_advisory = 0;

        // Calculate run level masking and software state
        if (!pxCoalesceRunStatus(config,"difftool_coalesce_run.sql",diff_id,
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
        // Set diffRun.software_ver to the appropriate value
        if (software_ver) {
          if (!pxSetRunSoftware(config, "diffRun", "diff_id", diff_id, software_ver)) {
            psError(PS_ERR_UNKNOWN, false, "failed to set diffRun.software_ver for diff_id: %" PRId64,
                    diff_id);
            psFree(output);
            if (!psDBRollback(config->dbh)) {
              psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return(false);
          }
        }
        // Set diffRun.maskfrac* to the appropriate values.
        if (maskfrac_npix) {
          if (!pxSetRunMaskfrac(config, "diffRun", "diff_id", diff_id, maskfrac_npix, maskfrac_static,
                                maskfrac_dynamic, maskfrac_magic, maskfrac_advisory)) {
            psError(PS_ERR_UNKNOWN, false, "failed to set diffRun.software_ver for diff_id: %" PRId64,
                    diff_id);
            psFree(output);
            if (!psDBRollback(config->dbh)) {
              psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return(false);
          }
        }
        // set diffRun.state to 'full'
        if (!setdiffRunState(config, diff_id, "full", magicked)) {
            psError(PS_ERR_UNKNOWN, false, "failed to change diffRun.state for diff_id: %" PRId64,
                diff_id);
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
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


static bool diffskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_id", "diffSkyfile.diff_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "diffInputSkyfile.skycell_id", "==");
    PXOPT_COPY_S64(config->args, where, "-diff_skyfile_id", "diffInputSkyfile.diff_skyfile_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "diffRun.tess_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "diffSkyfile.fault", "==");
    PXOPT_COPY_S64(config->args, where,  "-magicked", "diffSkyfile.magicked", "==");
    pxAddLabelSearchArgs (config, where, "-label", "diffRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "diffRun.data_group", "LIKE");

    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);
    PXOPT_LOOKUP_BOOL(template, config->args, "-template", false);
    if (!template) {
        PXOPT_COPY_S64(config->args, where, "-exp_id", "rawInput.exp_id", "==");
        PXOPT_COPY_STR(config->args, where, "-exp_name", "rawInput.exp_name", "==");
        PXOPT_COPY_STR(config->args, where, "-warp_id", "warpInput.warp_id", "==");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawInput.dateobs",  ">=");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawInput.dateobs",  "<=");
        PXOPT_COPY_STR(config->args, where, "-filter",     "rawInput.filter", "LIKE");
    } else {
        PXOPT_COPY_S64(config->args, where, "-exp_id", "rawTemplate.exp_id", "==");
        PXOPT_COPY_STR(config->args, where, "-exp_name", "rawTemplate.exp_name", "==");
        PXOPT_COPY_STR(config->args, where, "-warp_id", "warpTemplate.warp_id", "==");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawTemplate.dateobs",  ">=");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawTemplate.dateobs",  "<=");
        PXOPT_COPY_STR(config->args, where, "-filter",     "rawTemplate.filter", "LIKE");
    }

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);


    psString where2 = NULL;
    pxmagicAddWhere(config, &where2, "diffSkyfile");
    pxspaceAddWhere(config, &where2, template ? "rawTemplate" : "rawInput");
    psString query = pxDataGet("difftool_skyfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (where2) {
        psStringAppend(&query, " WHERE diffRun.diff_id is not null %s", where2);
    } else if (!all) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }
    psFree(where);

    if (pstamp_order) {
        if (template) {
            psStringAppend(&query, " ORDER BY rawTemplate.exp_id, diff_id DESC");
        } else {
            psStringAppend(&query, " ORDER BY rawInput.exp_id, diff_id DESC");
        }
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool revertdiffskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_id", "diffSkyfile.diff_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-skycell_id", "diffSkyfile.skycell_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "diffRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",     "fault", "==");

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    {
        psString query = pxDataGet("difftool_revertdiffskyfile_delete.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }
        psString query_updated = pxDataGet("difftool_revertdiffskyfile_updated.sql");
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

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
        psFree(query);
        psLogMsg("difftool", PS_LOG_INFO, "Deleted %" PRIu64 " rows", psDBAffectedRows(config->dbh));

        if (!fault) {
            // If fault has not been supplied, don't revert update faults with the magic value
            // We don't do this for new runs because then they would never complete
            // quality should be used to drop bad components
            psStringAppend(&query_updated, " AND (diffSkyfile.fault != %d)", PXTOOL_DO_NOT_REVERT_FAULT);
        }
        if (!p_psDBRunQuery(config->dbh, query_updated)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query_updated);
            return false;
        }
        psFree(query_updated);

        psLogMsg("difftool", PS_LOG_INFO, "Updated %" PRIu64 " rows", psDBAffectedRows(config->dbh));
    }

    psFree(where);


    return true;
}


static bool setdiffRunState(pxConfig *config, psS64 diff_id, const char *state, psS64 magicked)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid diffRun state: %s", state);
        return false;
    }

    if (magicked) {
      char *query = "UPDATE diffRun SET state = '%s', magicked = %" PRId64 " WHERE diff_id = %"PRId64;

      if (!p_psDBRunQueryF(config->dbh, query, state, magicked, diff_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for diff_id %"PRId64, diff_id);
        return false;
      }
    }
    else {
      char *query = "UPDATE diffRun SET state = '%s' WHERE diff_id = %"PRId64;

      if (!p_psDBRunQueryF(config->dbh, query, state, diff_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for diff_id %"PRId64, diff_id);
        return false;
      }
    }

    return true;
}


#ifdef notdef
static bool setdiffRunStateByLabel(pxConfig *config, const char *label, const char *state) {
  PS_ASSERT_PTR_NON_NULL(state,false);

  // check that state is a valid string value
  if (!pxIsValidState(state)) {
    psError(PS_ERR_UNKNOWN, false, "invalid diffRun state: %s", state);
    return false;
  }

  char *query = "UPDATE diffRun SET state = '%s' WHERE label = '%s'";
  if (!p_psDBRunQueryF(config->dbh,query,state,label)) {
    psError(PS_ERR_UNKNOWN, false,
            "failed to change state for label %s", label);
    return(false);
  }

  return true;
}
#endif

// Generate a single populated run
static bool populatedrun(psArray *list, // List of runs, to print
                         const char *workdir, // Working directory
                         const char *skycell_id, // Skycell identifier
                         const char *tess_id, // Tessellation identifier
                         const char *label, // label
                         const char *data_group, // data_group
                         const char *dist_group, // dist_group
                         const char *reduction, // reduction
                         const char *note,      // note
                         psS64 input_warp_id, // Warp identifier for input image, PS_MAX_S64 for none
                         psS64 input_stack_id, // Stack identifier for input image, PS_MAX_S64 for none
                         psS64 template_warp_id, // Warp identifier for template image, PS_MAX_S64 for none
                         psS64 template_stack_id, // Stack identifier for template image, PS_MAX_S64 for none
                         pxConfig *config // Configuration
                         )
{
    PS_ASSERT_STRING_NON_EMPTY(workdir, false);
    PS_ASSERT_STRING_NON_EMPTY(skycell_id, false);
    PS_ASSERT_STRING_NON_EMPTY(tess_id, false);
    if ((input_warp_id == PS_MAX_S64 && input_stack_id == PS_MAX_S64) ||
        (input_warp_id != PS_MAX_S64 && input_stack_id != PS_MAX_S64)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "One, and only one, input must be defined.");
        return false;
    }
    if ((template_warp_id == PS_MAX_S64 && template_stack_id == PS_MAX_S64) ||
        (template_warp_id != PS_MAX_S64 && template_stack_id != PS_MAX_S64)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "One, and only one, template must be defined.");
        return false;
    }
    psS16 diff_mode = IPP_DIFF_MODE_UNDEFINED;
    if ((input_warp_id != PS_MAX_S64) && (template_warp_id != PS_MAX_S64)) {
      diff_mode = IPP_DIFF_MODE_WARP_WARP;
    }
    else if ((input_warp_id != PS_MAX_S64) && (template_stack_id != PS_MAX_S64)) {
      diff_mode = IPP_DIFF_MODE_WARP_STACK;
    }
    else if ((input_stack_id != PS_MAX_S64) && (template_warp_id != PS_MAX_S64)) {
      diff_mode = IPP_DIFF_MODE_STACK_WARP;
    }
    else if ((input_stack_id != PS_MAX_S64) && (template_stack_id != PS_MAX_S64)) {
      diff_mode = IPP_DIFF_MODE_STACK_STACK;
    }


    // default
    // PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);

    diffRunRow *run = diffRunRowAlloc(
            0,          // ID
            "reg",      // state
            workdir,
            label,
            data_group ? data_group : label,
            dist_group,
            reduction,
            NULL,       // dvodb
            registered,
            tess_id,
            false,
            false,
            0,       // magicked
            NULL, // software version
            0,    // mask stat npix
            NAN,    // static
            NAN,    // dynamic
            NAN,    // magic
            NAN,    // advisory
            diff_mode, // diff_mode
            note
    );

    if (!run) {
        psError(PS_ERR_UNKNOWN, false, "failed to alloc diffRun object");
        return true;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!diffRunInsertObject(config->dbh, run)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(run);
        return true;
    }

    // get the assigned diff_id
    run->diff_id = psDBLastInsertID(config->dbh);

    // Template
    if (!diffInputSkyfileInsert(config->dbh,
            run->diff_id,
            skycell_id,
            input_warp_id,
            input_stack_id,
            template_warp_id,
            template_stack_id,
            tess_id,
            0
        )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    char *query = "UPDATE diffRun SET state = 'new' WHERE diff_id = '%" PRId64 "'";
    if (!p_psDBRunQueryF(config->dbh, query, run->diff_id)) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false,
                "failed to set state to run for diff_id %" PRId64, run->diff_id);
        return false;
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (list) {
        psArrayAdd(list, list->n, run);
    }

    psFree(run);

    return true;
}


static bool definepoprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false); // required options
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false); // required options
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", true, false); // required options
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false);
    PXOPT_LOOKUP_S64(template_warp_id, config->args, "-template_warp_id", false, false);
    PXOPT_LOOKUP_S64(template_stack_id, config->args, "-template_stack_id", false, false);
    PXOPT_LOOKUP_S64(input_warp_id, config->args, "-input_warp_id", false, false);
    PXOPT_LOOKUP_S64(input_stack_id, config->args, "-input_stack_id", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    if (template_stack_id && template_warp_id) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Only one template can be defined.");
        return false;
    }
    if (!template_stack_id && !template_warp_id) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "No template has been defined (-template_stack_id or -template_warp_id)");
        return false;
    }

    if (input_stack_id && input_warp_id) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Only one input can be defined.");
        return false;
    }
    if (!input_stack_id && !input_warp_id) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "No input has been defined (-input_stack_id or -input_warp_id)");
        return false;
    }

    psArray *list = psArrayAllocEmpty(16); // List of runs, to print
    // Populated Run will generate the diff_mode value
    if (!populatedrun(list, workdir, skycell_id, tess_id, label, data_group ? data_group : label, dist_group, reduction, note,
                      input_warp_id ? input_warp_id : PS_MAX_S64,
                      input_stack_id ? input_stack_id : PS_MAX_S64,
                      template_warp_id ? template_warp_id : PS_MAX_S64,
                      template_stack_id ? template_stack_id : PS_MAX_S64,
                      config)) {
        psError(PS_ERR_UNKNOWN, false, "failed to create populated diffRun");
        psFree(list);
        return false;
    }

    if (!diffRunPrintObjects(stdout, list, !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print object");
            psFree(list);
            return false;
    }
    psFree(list);

    return true;
}

static bool definewarpstackMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();

  PXOPT_LOOKUP_BOOL(lapQuery, config->args, "-lap_query", false);
  
  PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
  PXOPT_COPY_STR(config->args, where, "-filter", "rawExp.filter", "==");
  PXOPT_COPY_STR(config->args, where, "-comment", "comment", "LIKE");
  PXOPT_COPY_S64(config->args, where, "-warp_id", "warpRun.warp_id", "==");
  PXOPT_COPY_STR(config->args, where, "-warp_label", "warpRun.label", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id", "warpRun.tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-data_group", "warpRun.data_group", "==");
  PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpSkyfile.skycell_id", "==");
  PXOPT_COPY_F32(config->args, where,  "-good_frac", "warpSkyfile.good_frac", ">=");
  PXOPT_COPY_STR(config->args, where, "-stack_label", "stackRun.label", "==");
  PXOPT_COPY_STR(config->args, where, "-stack_data_group", "stackRun.data_group", "==");


  // Add position dependence here.
  if (!pxspaceBoxAddWhere(config,where)) {
    psError(psErrorCodeLast(), false, "pxSpaceBoxAddWhere failed");
    return false;
  }
  
  // PXOPT_LOOKUP_BOOL(available, config->args, "-available", false);
  PXOPT_LOOKUP_BOOL(bothways, config->args, "-bothways", false);
  
  PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false); // required option
  PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false); // option
  PXOPT_LOOKUP_STR(label, config->args, "-set_label", true, false); // option
  PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
  PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
  PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
  PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);
  PXOPT_LOOKUP_STR(warp_data_group, config->args, "-data_group", false, false);
  
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_BOOL(newTemplates, config->args, "-new-templates", false);
  PXOPT_LOOKUP_BOOL(reRun, config->args, "-rerun", false);
  PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);


  if (!data_group) {
    if (warp_data_group) {
      data_group = warp_data_group;
    }
  }
  
  // Get query file
  psString query;
  if (lapQuery) { 
    query = pxDataGet("difftool_definewarpstack.sql");
  }
  else {
    query = pxDataGet("difftool_definewarpstack_old.sql");
  }
  
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }

  psString whereClause = psDBGenerateWhereConditionSQL(where,NULL);
  psString whereClauseString = psStringCopy("");
  if (lapQuery) {
    psString diffWhereClause = psStringCopy("");
    psString joinWhereClause = psStringCopy("");
    
    // Don't queue things that already have diffs.
    if (! (reRun || newTemplates) ) {
      psStringAppend(&joinWhereClause, "\nAND diff_id IS NULL\n");
    }
    // Append diff qualifiers, if we have them
    if (label) {
      psStringAppend(&diffWhereClause, "\nAND ((diffRun.label = '%s') OR (diffRun.label IS NULL))",label);
    }
    if (data_group) {
      psStringAppend(&diffWhereClause, "\nAND ((diffRun.data_group = '%s') OR (diffRun.data_group IS NULL))",data_group);
    }
    if (reduction) {
      psStringAppend(&diffWhereClause, "\nAND ((diffRun.reduction = '%s') OR (diffRun.reduction IS NULL))",reduction);
    }

    psStringAppend(&whereClauseString, " \n AND %s ", whereClause);
    //  fprintf(stderr,query,whereClauseString);

    // This is just a simple query, so we don't need to do a transaction
    if (!p_psDBRunQueryF(config->dbh, query, whereClauseString, diffWhereClause,joinWhereClause)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(query);
      psFree(whereClause);
      return(false);
    }
    
    psFree(diffWhereClause);
    psFree(joinWhereClause);
    
  }
  else {
    // Don't queue things that already have diffs.
    if (! (reRun || newTemplates) ) {
      psStringAppend(&whereClause, "\nAND diff_id IS NULL\n");
    }
    // Append diff qualifiers, if we have them
    if (label) {
      psStringAppend(&whereClause, "\nAND ((diffRun.label = '%s') OR (diffRun.label IS NULL))",label);
    }
    if (data_group) {
      psStringAppend(&whereClause, "\nAND ((diffRun.data_group = '%s') OR (diffRun.data_group IS NULL))",data_group);
    }
    if (reduction) {
      psStringAppend(&whereClause, "\nAND ((diffRun.reduction = '%s') OR (diffRun.reduction IS NULL))",reduction);
    }

    psStringAppend(&whereClauseString, " \n AND %s ", whereClause);
    if (!p_psDBRunQueryF(config->dbh, query, whereClauseString)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(query);
      psFree(whereClause);
      return(false);
    }

  }
    
  psFree(query);
  psFree(whereClause);
  psFree(whereClauseString);
  
  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
      break;
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
      break;
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
      break;
    }
    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("difftool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }
  
  if (pretend) {
    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "diffRun", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
    psFree(output);
    return true;
  }

  psArray *list = psArrayAllocEmpty(16); // List of runs, to print
  long numGood = 0;
  psS64 last_exp_id = 0;
  psS64 diff_id = 0;
  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i];
    bool mdok;

    psS64 exp_id = psMetadataLookupS64(&mdok, row, "exp_id");
    if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "exp_id not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
    }
    psString warpDataGroup = psMetadataLookupStr(&mdok, row, "warpDataGroup");
    if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "warpDataGroup not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
    }
    if (exp_id != last_exp_id) {
      if (diff_id != 0) { // We've added a run already, and are now switching to a new one.
	// Set state to new
	if (!setdiffRunState(config, diff_id, "new", false)) {
	  psError(PS_ERR_UNKNOWN, false, "failed to change diffRun.state for diff_id: %" PRId64, diff_id);
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	  }
	  return false;
	}
	// Commit results
	if (!psDBCommit(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	  psFree(list);
	  return false;
	}
      }
      
      // Begin transaction
      if (!psDBTransaction(config->dbh)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
	return false;
      }
      // Add a new diffRun row
      psString tess_id = psMetadataLookupStr(&mdok, row, "tess_id");
      if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "tess_id not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
      }
      diffRunRow *run = diffRunRowAlloc(
					0,        // ID
					"reg",    // state
					workdir,
					label,
					data_group ? data_group : warpDataGroup,
					dist_group,
					reduction,
					NULL,     // dvodb
					registered,
					tess_id,
					bothways, // bothways (default is false)
					true,     // exposure
					0,        // magicked
					NULL,     // software_version
					0,        // mask stat npix
					NAN,      // static
					NAN,      // dynamic
					NAN,      // magic
					NAN,      // advisory
					IPP_DIFF_MODE_WARP_STACK,
					note
					);
      if (!diffRunInsertObject(config->dbh, run)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
	psFree(run);
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
      }
      diff_id = psDBLastInsertID(config->dbh);
      run->diff_id = diff_id;

      psArrayAdd(list, list->n, run);
      numGood++;
      
      last_exp_id = exp_id;					
    } // End Adding diffRun
    
    if (exp_id == last_exp_id) {
      psString skycell_id = psMetadataLookupStr(&mdok, row, "skycell_id");
      if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "skycell_id not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
      }
      psString tess_id = psMetadataLookupStr(&mdok, row, "tess_id");
      if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "tess_id not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
      }
      psS64 warp_id = psMetadataLookupS64(&mdok, row, "warp_id");
      if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "warp_id not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
      }
      psS64 stack_id = psMetadataLookupS64(&mdok, row, "stack_id");
      if (!mdok) {
	psError(PXTOOLS_ERR_PROG, false, "stack_id not found");
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
      }

      // Add a new skyfile row
      diffInputSkyfileRow *skyfile = diffInputSkyfileRowAlloc(
							      diff_id,   // ID
							      skycell_id,
							      warp_id, // warp1_id 
							      PS_MAX_S64, // stack1 -> NULL
							      PS_MAX_S64, // warp2_id -> NULL
							      stack_id, // stack2
							      tess_id,
							      0 // diff_skyfile_id
							      );
      //      fprintf(stderr,"%"PRId64 " %"PRId64 " %"PRId64 " %s %s\n",diff_id,warp_id,stack_id,skycell_id, tess_id);
      if (!diffInputSkyfileInsertObject(config->dbh, skyfile)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
	psFree(list);
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	return false;
      }
    } // End Adding skyfile
  } // End parsing result set
  
  // Finish the last run's update and close the connection
  if (diff_id != 0) {
    if (!setdiffRunState(config, diff_id, "new", false)) {
      psError(PS_ERR_UNKNOWN, false, "failed to change diffRun.state for diff_id: %" PRId64, diff_id);
      if (!psDBRollback(config->dbh)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
      }
      return false;
    }
    // Commit results
    if (!psDBCommit(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(list);
      return false;
    }
  }
  
  if (numGood && !diffRunPrintObjects(stdout, list, !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print object");
    psFree(list);
    return(false);
  }
  psFree(list);
  // Free things
  return true;
}  

#if (0)
static bool definewarpstackOldMethodMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *expWhere = psMetadataAlloc();
    psMetadata *warp1Where = psMetadataAlloc(); // First set of restrictions on warp
    psMetadata *warp2Where = psMetadataAlloc(); // Second set of restriction on warp
    psMetadata *stackWhere = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, expWhere, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, expWhere, "-filter", "filter", "==");
    PXOPT_COPY_STR(config->args, expWhere, "-comment", "comment", "LIKE");
    
    PXOPT_COPY_S64(config->args, warp1Where, "-warp_id", "warpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, warp1Where, "-warp_label", "warpRun.label", "==");
    PXOPT_COPY_STR(config->args, warp1Where, "-tess_id", "warpRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, warp1Where, "-data_group", "warpRun.data_group", "==");
    PXOPT_COPY_S64(config->args, warp2Where, "-warp_id", "warpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, warp2Where, "-tess_id", "warpRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, warp2Where, "-data_group", "warpRun.data_group", "==");
    PXOPT_COPY_STR(config->args, warp2Where, "-skycell_id", "warpSkyfile.skycell_id", "==");
    PXOPT_COPY_STR(config->args, warp2Where, "-warp_label", "warpRun.label", "==");
    PXOPT_COPY_F32(config->args, warp2Where,  "-good_frac", "warpSkyfile.good_frac", ">=");
    PXOPT_COPY_STR(config->args, stackWhere, "-stack_label", "stackRun.label", "==");
    PXOPT_COPY_STR(config->args, stackWhere, "-stack_data_group", "stackRun.data_group", "==");

    PXOPT_LOOKUP_BOOL(bothways, config->args, "-bothways", false);

    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false); // required option
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false); // option
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false); // option
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(newTemplates, config->args, "-new-templates", false);
    PXOPT_LOOKUP_BOOL(reRun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(available, config->args, "-available", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    // find all things to queue
    psString query = pxDataGet("difftool_definewarpstack_part1.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString warp1Query = NULL;
    psString warp2Query = NULL;
    psString stackQuery = NULL;
    psString expQuery = NULL;

    if (psListLength(expWhere->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(expWhere, NULL);
        psStringAppend(&expQuery, "\n AND %s", whereClause);
        psFree(whereClause);
    } else {
        expQuery = psStringCopy("\n");
    }
    psFree(expWhere);
    if (psListLength(warp1Where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(warp1Where, NULL);
        psStringAppend(&warp1Query, "\n AND %s", whereClause);
        psFree(whereClause);
    } else {
        warp1Query = psStringCopy("\n");
    }
    psFree(warp1Where);
    if (psListLength(warp2Where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(warp2Where, NULL);
        psStringAppend(&warp2Query, "\n AND %s", whereClause);
        psFree(whereClause);
    } else {
        warp2Query = psStringCopy("\n");
    }
    psFree(warp2Where);

    // don't queue for exposures that have already been diff'd unless requested
    psString diffQuery = NULL;
    if (! (reRun || newTemplates) ) {
        psStringAppend(&diffQuery, "\nAND diff_id IS NULL");
    } else {
        diffQuery = psStringCopy("\n");
    }

    if (psListLength(stackWhere->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(stackWhere, NULL);
        psStringAppend(&stackQuery, "\nAND %s", whereClause);
        psFree(whereClause);
    } else {
        stackQuery = psStringCopy("");
    }
    psFree(stackWhere);

    psTrace("difftool", 1, query, warp1Query, warp2Query, diffQuery, expQuery, stackQuery);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, warp1Query, warp2Query, expQuery, diffQuery)) {
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
        psErrorCode err = psErrorCodeLast();
        switch (err) {
          case PS_ERR_DB_CLIENT:
            psError(PXTOOLS_ERR_SYS, false, "database error");
            break;
          case PS_ERR_DB_SERVER:
            psError(PXTOOLS_ERR_PROG, false, "database error");
            break;
          default:
            psError(PXTOOLS_ERR_PROG, false, "unknown error");
            break;
        }
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    // create temporary table
    query = pxDataGet("difftool_definewarpstack_temp_create.sql");
    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(warp1Query);
    psFree(query);
    query = NULL;

    psString skycell_query = pxDataGet("difftool_definewarpstack_part2.sql");

    psArray *list = psArrayAllocEmpty(16); // List of runs, to print
    long numGood = 0;                   // Number of good rows added
    psS64 last_exp_id = 0;
    for (long i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Output row from query
        bool mdok;                      // Status of MD lookup

        // Take the first warp for each exposure.
        // The list is sorted by exposure id and warp_id and the warps are in descending
        // order.
        psS64 exp_id = psMetadataLookupS64(&mdok, row, "exp_id");
        if (!mdok) {
            psError(PXTOOLS_ERR_PROG, false, "exp_id not found");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        if (exp_id == last_exp_id) {
            continue;
        }
        last_exp_id = exp_id;

        // clear temporary table
        if (!p_psDBRunQuery(config->dbh, "DELETE FROM skycellsToDiff")) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psS64 warp_id = psMetadataLookupS64(&mdok, row, "warp_id");
        if (!mdok) {
            psError(PXTOOLS_ERR_PROG, false, "warp_id not found --- ignoring row %ld", i);
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psS64 skycell_count = psMetadataLookupS64(&mdok, row, "skycell_count");
        if (!mdok) {
            psError(PXTOOLS_ERR_PROG, false, "skycell_count not found");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psString tess_id = psMetadataLookupStr(&mdok, row, "tess_id");
        if (!mdok) {
            psError(PXTOOLS_ERR_PROG, false, "tess_id not found");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psString filter = psMetadataLookupStr(&mdok, row, "filter");
        if (!mdok) {
            psError(PXTOOLS_ERR_PROG, false, "filter not found");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psString warp_data_group = psMetadataLookupStr(&mdok, row, "data_group");
        if (!mdok) {
          psError(PXTOOLS_ERR_PROG, false, "warp data_group not found");
          psFree(warp2Query);
          psFree(stackQuery);
          psFree(skycell_query);
          if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
          }
          return false;
        }
        if (!data_group && warp_data_group) {
          data_group = warp_data_group;
        }

        if (!p_psDBRunQueryF(config->dbh, skycell_query, stackQuery, warp_id, filter, warp2Query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psS64 num = psDBAffectedRows(config->dbh);

        if (num == 0) {
            psTrace("difftool", PS_LOG_INFO, "no skycells with stack found for warp_id %" PRId64, warp_id);
            continue;
        }

	// CZW force available off.  If we're using this very-expensive mode, we need to get the one thing it does correctly.
	available = false;;
        if (!available && (num != skycell_count)) {
            psTrace("difftool", PS_LOG_INFO, "%" PRId64 " skyfiles with stack found for warp_id %" PRId64
                    " but need %" PRId64, num, warp_id, skycell_count);
            continue;
        }

        // ok we've got one create the diffRun
        diffRunRow *run = diffRunRowAlloc(
                0,            // ID
                "reg",        // state
                workdir,
                label,
                data_group ? data_group : label,
                dist_group,
                reduction,
                NULL,         // dvodb
                registered,
                tess_id,
                bothways,     // bothways (default is false)
                true,         // exposure
                0,            // magicked
                NULL,         // software version
                0,            // mask stat npix
                NAN,          // static
                NAN,          // dynamic
                NAN,          // magic
                NAN,          // advisory
                IPP_DIFF_MODE_WARP_STACK,
                note
        );

        if (!diffRunInsertObject(config->dbh, run)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(run);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        run->diff_id = psDBLastInsertID(config->dbh);

        psStringAppend(&query, "UPDATE skycellsToDiff SET diff_id = %" PRId64, run->diff_id);
        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            psFree(query);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
            return false;
        }
        psFree(query);
        query = NULL;
        psStringAppend(&query,
                       "INSERT INTO diffInputSkyfile(diff_id, skycell_id, warp1, stack1, warp2, stack2, tess_id) SELECT diff_id, skycell_id, warp1, stack1, warp2, stack2, tess_id from skycellsToDiff");
        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(query);
        query = NULL;

        if (!setdiffRunState(config, run->diff_id, "new", false)) {
            psError(PS_ERR_UNKNOWN, false, "failed to change diffRun.state for diff_id: %" PRId64,
                run->diff_id);
            psFree(warp2Query);
            psFree(stackQuery);
            psFree(skycell_query);
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psArrayAdd(list, list->n, run);
        numGood++;
    }
    psFree(output);
    psFree(warp2Query);
    psFree(stackQuery);
    psFree(skycell_query);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(list);
        return false;
    }

    if (numGood && !diffRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }
    psFree(list);

    return true;
}
#endif

static bool definewarpwarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *selectWhere = psMetadataAlloc();
    psMetadata *insertWhere = psMetadataAlloc();

    // Restrictions for selecting warps
    PXOPT_COPY_S64(config->args, selectWhere, "-warp_id", "inputWarpRun.warp_id", "==");
    PXOPT_COPY_S64(config->args, selectWhere, "-exp_id", "inputRawExp.exp_id", "==");
    PXOPT_COPY_S64(config->args, selectWhere, "-template_exp_id", "templateRawExp.exp_id", "==");
    PXOPT_COPY_S64(config->args, selectWhere, "-template_warp_id", "templateWarpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, selectWhere, "-filter", "inputRawExp.filter", "==");
    PXOPT_COPY_STR(config->args, selectWhere, "-obs_mode", "inputRawExp.obs_mode", "==");
    PXOPT_COPY_STR(config->args, selectWhere, "-obs_mode", "templateRawExp.obs_mode", "==");
    PXOPT_COPY_STR(config->args, selectWhere, "-input_label", "inputWarpRun.label", "==");
    PXOPT_COPY_STR(config->args, selectWhere, "-template_label", "templateWarpRun.label", "==");
    PXOPT_COPY_F32(config->args, selectWhere, "-rotdiff", "ABS(inputRawExp.posang - templateRawExp.posang)", "<=");
    PXOPT_COPY_F32(config->args, selectWhere, "-timediff",
                   "ABS(TIME_TO_SEC(TIMEDIFF(inputRawExp.dateobs, templateRawExp.dateobs)))", "<=");
    PXOPT_COPY_F32(config->args, selectWhere, "-mintimediff",
                   "ABS(TIME_TO_SEC(TIMEDIFF(inputRawExp.dateobs, templateRawExp.dateobs)))", ">=");

    // other where restrictions:
    PXOPT_COPY_TIME(config->args,  selectWhere, "-dateobs_begin",      "inputRawExp.dateobs",        ">=");
    PXOPT_COPY_TIME(config->args,  selectWhere, "-dateobs_end",        "inputRawExp.dateobs",        "<=");
    PXOPT_COPY_RADEC(config->args, selectWhere, "-ra_min",             "inputRawExp.ra",             ">=");
    PXOPT_COPY_RADEC(config->args, selectWhere, "-ra_max",             "inputRawExp.ra",             "<");
    PXOPT_COPY_RADEC(config->args, selectWhere, "-decl_min",           "inputRawExp.decl",           ">=");
    PXOPT_COPY_RADEC(config->args, selectWhere, "-decl_max",           "inputRawExp.decl",           "<");
    PXOPT_COPY_F32(config->args,   selectWhere, "-sat_pixel_frac_min", "inputRawExp.sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args,   selectWhere, "-sat_pixel_frac_max", "inputRawExp.sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-airmass_min",        "inputRawExp.airmass",        ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-airmass_max",        "inputRawExp.airmass",        "<");
    PXOPT_COPY_F32(config->args,   selectWhere, "-exp_time_min",       "inputRawExp.exp_time",       ">=");
    PXOPT_COPY_F32(config->args,   selectWhere, "-exp_time_max",       "inputRawExp.exp_time",       "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-bg_min",             "inputRawExp.bg",             ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-bg_max",             "inputRawExp.bg",             "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-bg_stdev_min",       "inputRawExp.bg_stdev",       ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-bg_stdev_max",       "inputRawExp.bg_stdev",       "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-bg_mean_stdev_min",  "inputRawExp.bg_mean_stdev",  ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-bg_mean_stdev_max",  "inputRawExp.bg_mean_stdev",  "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-alt_min",            "inputRawExp.alt",            ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-alt_max",            "inputRawExp.alt",            "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-az_min",             "inputRawExp.az",             ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-az_max",             "inputRawExp.az",             "<");
    PXOPT_COPY_F32(config->args,   selectWhere, "-ccd_temp_min",       "inputRawExp.ccd_temp",       ">=");
    PXOPT_COPY_F32(config->args,   selectWhere, "-ccd_temp_max",       "inputRawExp.ccd_temp",       "<");
    PXOPT_COPY_F64(config->args,   selectWhere, "-posang_min",         "inputRawExp.posang",         ">=");
    PXOPT_COPY_F64(config->args,   selectWhere, "-posang_max",         "inputRawExp.posang",         "<");
    PXOPT_COPY_F32(config->args,   selectWhere, "-sun_angle_min",      "inputRawExp.sun_angle",      ">=");
    PXOPT_COPY_F32(config->args,   selectWhere, "-sun_angle_max",      "inputRawExp.sun_angle",      "<");
    PXOPT_COPY_STR(config->args,   selectWhere, "-input_comment",      "inputRawExp.comment",        "LIKE");
    PXOPT_COPY_STR(config->args,   selectWhere, "-template_comment",   "templateRawExp.comment",     "LIKE");

    PXOPT_LOOKUP_BOOL(not_bothways, config->args, "-not-bothways", false);

    // Haversine formula for great circle distance
    PXOPT_COPY_F32(config->args, selectWhere, "-distance",
                   "DEGREES(2*ASIN(SQRT(POW(SIN(inputRawExp.decl - templateRawExp.decl),2) + "
                   "COS(inputRawExp.decl)*COS(templateRawExp.decl)*"
                   "POW(SIN(inputRawExp.ra - templateRawExp.ra),2))))", "<=");

    PXOPT_LOOKUP_BOOL(backwards, config->args, "-backwards", false);

    psMetadataAddF32(selectWhere, PS_LIST_TAIL,
                     "TIME_TO_SEC(TIMEDIFF(inputRawExp.dateobs, templateRawExp.dateobs))",
                     PS_META_DUPLICATE_OK, backwards ? "<" : ">", 0.0);

    // Restrictions for inserting skycells
    PXOPT_COPY_F32(config->args, insertWhere, "-good_frac", "inputWarpSkyfile.good_frac", ">=");
    PXOPT_COPY_F32(config->args, insertWhere, "-good_frac", "templateWarpSkyfile.good_frac", ">=");

    // Additional controls
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);

    // Settings to apply to defined run
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false); // required options
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false); // option
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false); // option
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false); // option
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false); // option
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false); // option
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(selectWhere);
        psFree(insertWhere);
        return false;
    }

    if (!rerun) {
        // Need to build table of exposures with diffs
        psString tempCreate = pxDataGet("difftool_definewarpwarp_temp_create.sql"); // Create temp table SQL
        if (!tempCreate) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(selectWhere);
            psFree(insertWhere);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        if (!p_psDBRunQuery(config->dbh, tempCreate)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to create temp table: %s", tempCreate);
            psFree(tempCreate);
            psFree(selectWhere);
            psFree(insertWhere);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(tempCreate);

        psString tempInsert = pxDataGet("difftool_definewarpwarp_temp_insert.sql"); // Insert to temp table
        if (!tempInsert) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(selectWhere);
            psFree(insertWhere);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psString where = psStringCopy(""); // WHERE for insertion
        if (label) {
            psStringAppend(&where, "\nAND diffRun.label = '%s'", label);
        }

        if (!p_psDBRunQueryF(config->dbh, tempInsert, where, where)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to insert into temp table: %s", tempInsert);
            psFree(tempInsert);
            psFree(where);
            psFree(selectWhere);
            psFree(insertWhere);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(where);
        psFree(tempInsert);
    }

    // Get list of warps to diff
    psString select = pxDataGet("difftool_definewarpwarp_select.sql");
    if (!select) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(selectWhere);
        psFree(insertWhere);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }

    psString whereClause = NULL; // The WHERE part of the query

    if (psListLength(selectWhere->list)) {
        psString new = psDBGenerateWhereConditionSQL(selectWhere, NULL);
        psStringAppend(&whereClause, "\nAND %s", new);
        psFree(new);
    }
    psFree(selectWhere);

    if (!rerun) {
        psStringAppend(&whereClause, "\nAND diffs.diff_id IS NULL");
    }

    if (!p_psDBRunQueryF(config->dbh, select,
                         !rerun ? "\n" : "", // Activate LEFT JOIN against diffs?
                         whereClause)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to run query: %s [WITH] %s", select, whereClause);
        psFree(select);
        psFree(whereClause);
        psFree(insertWhere);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(select);
    psFree(whereClause);

    psArray *results = p_psDBFetchResult(config->dbh); // Results of query
    if (!results) {
        psErrorCode err = psErrorCodeLast(); // Code for error
        switch (err) {
          case PS_ERR_DB_CLIENT:
            psError(PXTOOLS_ERR_SYS, false, "database error");
            break;
          case PS_ERR_DB_SERVER:
            psError(PXTOOLS_ERR_PROG, false, "database error");
            break;
          default:
            psError(PXTOOLS_ERR_PROG, false, "unknown error");
            break;
        }
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psFree(insertWhere);
        return false;
    }
    if (!psArrayLength(results)) {
        psTrace("difftool", 1, "no rows found");
        psFree(results);
        psFree(insertWhere);
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, results, "diffRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(results);
            return false;
        }
        psFree(results);
        return true;
    }

    psString insert = pxDataGet("difftool_definewarpwarp_insert.sql"); // Insertion for each new run

    if (psListLength(insertWhere->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(insertWhere, NULL);
        psStringAppend(&insert, "\nAND %s", whereClause);
        psFree(whereClause);
    }
    psFree(insertWhere);

    psArray *list = psArrayAllocEmpty(16); // List of runs defined, to print
    long numGood = 0;                   // Number of good rows added
    for (long i = 0; i < results->n; i++) {
        if (limit && numGood >= limit) {
            break;
        }
        psMetadata *row = results->data[i]; // Result row from query

        psS64 input_id = psMetadataLookupS64(NULL, row, "input_warp_id");
        const char *template = psMetadataLookupStr(NULL, row, "template_warp_id");
        const char *tess_id = psMetadataLookupStr(NULL, row, "tess_id");
        psString input_data_group = psMetadataLookupStr(NULL, row, "input_data_group");
        if (!data_group) {
          data_group = input_data_group;
        }

        if (!input_id || !template || !tess_id) {
            psError(PXTOOLS_ERR_PROG, false, "Identifiers not found");
            psFree(list);
            psFree(insert);
            psFree(results);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        diffRunRow *run = diffRunRowAlloc(0,
                                          "reg",
                                          workdir,
                                          label,
                                          data_group ? data_group : label,
                                          dist_group,
                                          reduction,
                                          NULL,  // dvodb
                                          registered,
                                          tess_id,
                                          !not_bothways,  // bothways (default is true, ie not_bothways is false)
                                          true,  // exposure
                                          false, // magicked
                                          NULL, // software version
                                          0,    // mask stat npix
                                          NAN,    // static
                                          NAN,    // dynamic
                                          NAN,    // magic
                                          NAN,    // advisory
                                          IPP_DIFF_MODE_WARP_WARP,
                                          note); // Run to insert
        if (!diffRunInsertObject(config->dbh, run)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(run);
            psFree(list);
            psFree(insert);
            psFree(results);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        run->diff_id = psDBLastInsertID(config->dbh); // Difference run identifier

        // Convert identifiers to string, for insertion into query
        psString diff = NULL, input = NULL; // String versions of identifiers
        psStringAppend(&diff, "%" PRId64, run->diff_id);
        psStringAppend(&input, "%" PRId64, input_id);

        if (!p_psDBRunQueryF(config->dbh, insert, diff, input, template, input, template)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(input);
            psFree(diff);
            psFree(run);
            psFree(list);
            psFree(insert);
            psFree(results);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psS64 numSkycells = psDBAffectedRows(config->dbh);
        if (numSkycells > 0) {
            if (!setdiffRunState(config, run->diff_id, "new", false)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to change diffRun.state for diff_id: %" PRId64,
                        run->diff_id);
                psFree(input);
                psFree(diff);
                psFree(run);
                psFree(list);
                psFree(insert);
                psFree(results);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            run->state = psStringCopy("new");
        } else {
            // No overlap between the warps. Insert a dummy skycell which will complete this pair of warps
            // XXX: we'd like to use diffInputSkyfileRowAlloc and diffInputSkyfileInsert but there doesn't
            // seem to be a way to pass in NULL for skycell_id, stack1 and stack2
            psString dummyQuery = NULL;
            psStringAppend(&dummyQuery,
              "INSERT INTO diffInputSkyfile VALUES(%" PRId64 ", NULL, %s, NULL, %s, NULL, '%s', 0)",
                    run->diff_id,
                    input,                      // warp1
                    template,                   // warp2
                    tess_id);
            if (!p_psDBRunQuery(config->dbh, dummyQuery)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(dummyQuery);
                psFree(input);
                psFree(diff);
                psFree(run);
                psFree(list);
                psFree(insert);
                psFree(results);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            psFree(dummyQuery);
            psString finishQuery = NULL;
            psStringAppend( &finishQuery,
                "UPDATE diffRun set state ='full', dist_group = NULL, note = 'dummy run - no overlap' WHERE diff_id = %" PRId64,
                run->diff_id);
            if (!p_psDBRunQuery(config->dbh, finishQuery)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(finishQuery);
                psFree(input);
                psFree(diff);
                psFree(run);
                psFree(list);
                psFree(insert);
                psFree(results);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            psFree(finishQuery);
            run->state = psStringCopy("full");
        }
        psFree(input);
        psFree(diff);

        psArrayAdd(list, list->n, run);
        psFree(run);                    // Drop reference

        numGood++;
    }
    psFree(insert);
    psFree(results);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(list);
        return false;
    }

    if (!diffRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }
    psFree(list);

    return true;
}

static bool definestackstackMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *stack1Where = psMetadataAlloc();
    psMetadata *stack2Where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, stack1Where, "-tess_id", "stackRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, stack2Where, "-tess_id", "stackRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, stack1Where, "-filter", "stackRun.filter", "==");
    PXOPT_COPY_STR(config->args, stack2Where, "-filter", "stackRun.filter", "==");
    PXOPT_COPY_STR(config->args, stack1Where, "-skycell_id", "stackRun.skycell_id", "==");
    PXOPT_COPY_STR(config->args, stack2Where, "-skycell_id", "stackRun.skycell_id", "==");
    PXOPT_COPY_STR(config->args, stack1Where, "-input_label", "stackRun.label","==");
    PXOPT_COPY_STR(config->args, stack1Where, "-input_data_group", "stackRun.data_group","==");
    PXOPT_COPY_STR(config->args, stack2Where, "-template_label", "stackRun.label","==");

    PXOPT_COPY_STR(config->args, stack1Where, "-input_stack_id", "stackRun.stack_id","==");
    PXOPT_COPY_STR(config->args, stack2Where, "-template_stack_id", "stackRun.stack_id","==");

    PXOPT_COPY_F32(config->args, stack1Where, "-good_frac", "stackSumSkyfile.good_frac", ">=");
    PXOPT_COPY_F32(config->args, stack2Where, "-good_frac", "stackSumSkyfile.good_frac", ">=");

    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false); // required option
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false); // option
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false); // option
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    PXOPT_LOOKUP_BOOL(bothways, config->args, "-bothways", false);
    PXOPT_LOOKUP_BOOL(relaxedFilters, config->args, "-relaxed-filters", false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(reRun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(newTemplates,config->args,"-new-templates", false);

    // PXOPT_LOOKUP_BOOL(available, config->args, "-available", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    if (!(label)) {
        PXOPT_LOOKUP_STR(label,config->args, "-input_label",true,false);
    }

    // Organize the config information into queries.
    psString stack1Query = NULL;
    psString stack2Query = NULL;

    if (psListLength(stack1Where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(stack1Where, NULL);
        psStringAppend(&stack1Query, "AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(stack1Where);

    if (psListLength(stack2Where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(stack2Where, NULL);
        psStringAppend(&stack2Query, "AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(stack2Where);

    // don't queue for stacks that have already been diffed unless requested
    psString diffQuery0 = NULL;
    psString diffQuery1 = NULL;
    if (! (reRun || newTemplates) ) {
        psStringAppend(&diffQuery0, "AND diffExp.diff_id IS NULL");
        psStringAppend(&diffQuery1, "HAVING n_diff = 0");
    }

    // restrict input and templated to the same filter unless not restricted
    psString filtQuery0 = NULL;
    psString filtQuery1 = NULL;
    if (! relaxedFilters) {
        psStringAppend(&filtQuery0, "AND stackRun.filter = template.filter");
        psStringAppend(&filtQuery1, "AND stackRun.filter = template.filter");
    }

    // find the distinct set of data_groups and filters
    psString query = pxDataGet("difftool_definestackstack_part0.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return(false);
    }
    psTrace("difftool",1,query,stack2Query,stack1Query,diffQuery0,stack1Query);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return(false);
    }

    psStringSubstitute(&query, stack1Query, "@STACK1_QUERY@");
    psStringSubstitute(&query, stack2Query, "@STACK2_QUERY@");
    psStringSubstitute(&query, diffQuery0,  "@DIFF0_QUERY@");
    psStringSubstitute(&query, filtQuery0,  "@FILT0_QUERY@");

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
        psErrorCode err = psErrorCodeLast();
        switch (err) {
          case PS_ERR_DB_CLIENT:
            psError(PXTOOLS_ERR_SYS, false, "database error");
            break;
          case PS_ERR_DB_SERVER:
            psError(PXTOOLS_ERR_PROG, false, "database error");
            break;
          default:
            psError(PXTOOLS_ERR_PROG, false, "unknown error");
            break;
        }
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }


    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffRunTemp", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    psArray *list = psArrayAllocEmpty(16); // List of runs defined, to print
    for (long i = 0; i < output->n; i++) {
      query = pxDataGet("difftool_definestackstack_part1.sql");


        psMetadata *row = output->data[i]; // Output row from query
        bool mdok;                      // Status of MD lookup

        // psMetadataPrint(stderr, row, 1);

        psString tess_id = psMetadataLookupStr(&mdok,row,"INPUT_tess_id");
        psString this_data_group = psMetadataLookupStr(&mdok,row,"INPUT_data_group");
        psString this_dist_group = psMetadataLookupStr(&mdok,row,"INPUT_dist_group");
        psString this_label = psMetadataLookupStr(&mdok,row,"INPUT_label");

        psString this_stack1Query = psStringCopy(stack1Query);

        psString thisWhere = psDBGenerateWhereConditionSQL(row,NULL);
        psStringSubstitute(&thisWhere,"stackRun.","INPUT_");
        psStringAppend(&this_stack1Query,"AND %s", thisWhere);
        psFree(thisWhere);

        psStringSubstitute(&query, this_stack1Query, "@STACK1_QUERY@");
        psStringSubstitute(&query, stack2Query,      "@STACK2_QUERY@");
        psStringSubstitute(&query, diffQuery1,       "@DIFF1_QUERY@");
        psStringSubstitute(&query, filtQuery1,       "@FILT1_QUERY@");

        psTrace("difftool", 1, "%s", query);
        if (!psDBTransaction(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return(false);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(this_stack1Query);

        psArray *diff_id_output = p_psDBFetchResult(config->dbh);
        if (!diff_id_output) {
            psErrorCode err = psErrorCodeLast();
            switch (err) {
              case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
                break;
              case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
                break;
              default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
                break;
            }
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        if (!psArrayLength(diff_id_output)) {
            psTrace("difftool", PS_LOG_INFO, "no rows found");
            psFree(diff_id_output);
            if (!psDBCommit(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                return false;
            }
            return true;
        }

        // ok we've got one create the diffRun
        diffRunRow *run = diffRunRowAlloc(
            0,          // ID
            "reg",      // state
            workdir,
            label ? label : this_label,
            data_group ? data_group : this_data_group,
            dist_group ? dist_group : this_dist_group,
            reduction,
            NULL,       // dvodb
            registered,
            tess_id,
            bothways,               // bothways
            false,                 // exposure
            0,       // magicked
            NULL, // software version
            0,    // mask stat npix
            NAN,    // static
            NAN,    // dynamic
            NAN,    // magic
            NAN,    // advisory
            IPP_DIFF_MODE_STACK_STACK, // diff_mode
            note
            );
        // Commit to database
        if (!diffRunInsertObject(config->dbh, run)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(run);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        // diffRunPrintObject(stdout,run,1);
        run->diff_id = psDBLastInsertID(config->dbh);

        for (long j = 0; j < diff_id_output->n; j++) {
            psMetadata *input_row = diff_id_output->data[j]; // Output row from query
            bool mdok;
            psString in_skycell_id  = psMetadataLookupStr(&mdok,input_row,"skycell_id");
            psS64 in_input_stack_id = psMetadataLookupS64(&mdok,input_row,"stack_id");
            psS64 in_template_stack_id = psMetadataLookupS64(&mdok,input_row,"max_stack_id");
            psString in_tess_id = psMetadataLookupStr(&mdok,input_row,"tess_id");
            psTrace("difftool",1,"%s %" PRId64 " %" PRId64 " %s\n",
                    in_skycell_id, in_input_stack_id, in_template_stack_id, in_tess_id);
            diffInputSkyfileRow *input = diffInputSkyfileRowAlloc(
                run->diff_id,   // ID
                in_skycell_id,
                PS_MAX_S64, // warp1_id -> NULL
                in_input_stack_id, // stack1
                PS_MAX_S64, // warp2_id -> NULL
                in_template_stack_id, // stack2
                in_tess_id,
                0 // diff_skyfile_id
                );

            // Commit to database the input
            if (!diffInputSkyfileInsertObject(config->dbh, input)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(input);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }

/*       diffInputSkyfilePrintObject(stdout,input,1); */
            psFree(input);
        }

        if (!setdiffRunState(config, run->diff_id, "new", false)) {
            psError(PS_ERR_UNKNOWN, false, "failed to change diffRun.state for diff_id: %" PRId64, run->diff_id);
            psFree(stack1Query);
            psFree(stack2Query);
            psFree(diffQuery0);
            psFree(diffQuery1);
            psFree(run);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psArrayAdd(list, list->n, run);
        psFree(run);
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(list);
        return false;
    }

    if (!diffRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }

    psFree(query);
    psFree(stack1Query);
    psFree(stack2Query);
    psFree(diffQuery0);
    psFree(diffQuery1);
    psFree(output);
    psFree(list);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(list);
        return false;
    }

    return(true);
}

static bool tosummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-warp_id",    "diffSkyfile.warp_id", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id",    "diffSkyfile.tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-state",      "diffRun.state", "==");
  PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawExp.dateobs",  ">=");
  PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawExp.dateobs",  "<=");
  PXOPT_COPY_STR(config->args, where, "-filter",    "rawExp.filter", "LIKE");
  PXOPT_COPY_S64(config->args, where, "-magicked", "diffSkyfile.magicked", "==");
  pxAddLabelSearchArgs (config, where, "-label",   "diffRun.label", "LIKE");
  pxAddLabelSearchArgs (config, where, "-data_group",   "diffRun.data_group", "LIKE");

  PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  // find all rawImfiles matching the default query
  psString query = pxDataGet("difftool_tosummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }

  // generate where strings for arguments that require extra processing
  // beyond PXOPT_COPY*
  psString where2 = NULL;
  if (!pxmagicAddWhere(config, &where2, "diffSkyfile")) {
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
    if (psListLength(where->list)) {
      psStringAppend(&query, " %s", where2);
    } else {
      psStringAppend(&query, " AND 1 %s", where2);
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
    psTrace("difftool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "diffRun", !simple)) {
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

  PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", true, false);
  PXOPT_LOOKUP_STR(projection_cell, config->args, "-projection_cell", true, false);
  PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

  psString query = pxDataGet("difftool_addsummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  if (!p_psDBRunQueryF(config->dbh, query, diff_id, projection_cell, path_base)) {
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
    pxAddLabelSearchArgs (config, where, "-label", "diffRun.label", "==");

    psString query = pxDataGet("difftool_pendingcleanuprun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    // pxDataGet/psStringSubstitute workaround
    psString queryCopy = psStringCopy(query);
    psFree(query);
    query = queryCopy;

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereClause, " AND ");
        psStringSubstitute(&query,whereClause,"@INNERCONSTRAINT@");
        psFree(whereClause);
    }
    else {
      psStringSubstitute(&query,NULL,"@INNERCONSTRAINT@");
    }
    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psStringSubstitute(&query,limitString,"@INNERLIMITS@");
        psFree(limitString);

    }
    else {
      psStringSubstitute(&query,NULL,"@INNERLIMITS@");
    }
    //    fprintf(stderr,"%s",query);

    //fprintf(stderr,"%s",query);
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "diffPendingCleanupRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool pendingcleanupskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", false, false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    if (diff_id) {
        PXOPT_COPY_S64(config->args, where, "-diff_id", "diff_id", "==");
    }
    pxAddLabelSearchArgs (config, where, "-label", "diffRun.label", "==");

    char * sql_file  = all ? "difftool_pendingcleanupskyfile_all.sql": "difftool_pendingcleanupskyfile.sql" ;
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "diffPendingCleanupSkyfile", !simple)) {
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
    PXOPT_COPY_S64(config->args, where, "-diff_id",    "diffSkyfile.diff_id", "==");
    pxAddLabelSearchArgs (config, where, "-label",     "diffRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group",   "diffRun.data_group", "LIKE");

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

    psString query = pxDataGet("difftool_revertcleanup.sql");
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

    psLogMsg("difftool", PS_LOG_INFO, "Reverted %d diffRuns and diffSkyfiles", numDeleted);

    return true;
}
static bool donecleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    psString query = pxDataGet("difftool_donecleanup.sql");
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "diffDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);
    return true;
}

static bool updatediffskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);

    if (!state) {
        psMetadata *where = psMetadataAlloc();
        PXOPT_COPY_S64(config->args, where, "-diff_id",   "diff_id",   "==");
        PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id",   "==");

        PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
        PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

        if (!pxSetFaultCode(config->dbh, "diffSkyfile", where, fault, quality)) {
            psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
            psFree (where);
            return false;
        }
        psFree (where);
    } else {
      if (strcmp(state,"error_cleaned") == 0) {
        change_skyfile_data_state(config,"error_cleaned", "goto_cleaned");
      }
      else if (strcmp(state, "error_scrubbed") == 0) {
        change_skyfile_data_state(config,"error_scrubbed", "goto_scrubbed");
      }
      else if (strcmp(state, "error_purged") == 0) {
        change_skyfile_data_state(config,"error_purged", "goto_purged");
      }
      else {
        psError(PS_ERR_UNKNOWN, false, "unhandled state given");
        return(false);
      }
    }

    return true;
}

static bool change_skyfile_data_state(pxConfig *config, psString data_state, psString run_state) {
  PS_ASSERT_PTR_NON_NULL(config, false);

  // diff_id, skycell_id are required
  PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", true, false);
  PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);


  psS64 magicked = 0;
  if (!strcmp(data_state, "full")) {
      PXOPT_LOOKUP_S64(set_magicked, config->args, "-magicked", 0, false);
      magicked = set_magicked;
  }

  psString query = pxDataGet("difftool_change_skyfile_data_state.sql");

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }

  psString set_magicked_skyfile = psStringCopy("");
  psString set_magicked_run = psStringCopy("");
  if (magicked != 0 && !strcmp(data_state, "full")) {
    psStringAppend(&set_magicked_skyfile,  "\n , diffSkyfile.magicked = %" PRId64, magicked);
    psStringAppend(&set_magicked_run,      "\n , diffRun.magicked = %" PRId64, magicked);

  } else if (!strcmp(data_state, "cleaned") || !strcmp(data_state, "purged")) {
    psStringAppend(&set_magicked_skyfile, "\n, diffSkyfile.magicked = IF(diffSkyfile.magicked = 0, 0, -1)");
    psStringAppend(&set_magicked_run, "\n, diffRun.magicked = IF(diffRun.magicked = 0, 0, -1)");
  }

  // Uses the unconstrained (diffRun.state [NEED NOT EQUAL] run_state) version from warptool.c

  if (!p_psDBRunQueryF(config->dbh, query, data_state, set_magicked_skyfile, diff_id, skycell_id)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(set_magicked_skyfile);
    return(false);
  }
  psFree(set_magicked_skyfile);
  psFree(query);

  query = pxDataGet("difftool_change_run_state.sql");
  if (!p_psDBRunQueryF(config->dbh, query, data_state, set_magicked_run, diff_id, data_state)) {
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(set_magicked_run);
    return(false);
  }

  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(set_magicked_run);
    return(false);
  }
  psFree(set_magicked_run);

  return(true);
}

static bool tocleanedskyfileMode(pxConfig *config) {
  return change_skyfile_data_state(config, "cleaned","goto_cleaned");
}
static bool topurgedskyfileMode(pxConfig *config) {
  return change_skyfile_data_state(config, "purged", "goto_purged");
}
static bool toscrubbedskyfileMode(pxConfig *config) {
  return change_skyfile_data_state(config, "scrubbed", "goto_scrubbed");
}
static bool tofullskyfileMode(pxConfig *config) {
  return change_skyfile_data_state(config, "full", "update");
}


bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportFiles = 3;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // PXOPT_LOOKUP_S64(det_id, config->args, "-diff_id", true,  false);
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
  PXOPT_COPY_S64(config->args, where, "-diff_id", "diff_id", "==");

  ExportTable tables [] = {
    {"diffRun", "difftool_export_run.sql"},
    {"diffInputSkyfile", "difftool_export_input_skyfile.sql"},
    {"diffSkyfile", "difftool_export_skyfile.sql"},
  };

  for (int i=0; i < numExportFiles; i++) {
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
      return false;
    }
    if (!psArrayLength(output)) {
      psError(PS_ERR_UNKNOWN, true, "no rows found");
      psFree(output);
      return false;
    }

    if (clean) {
        bool success = true;
        if (!strcmp(tables[i].tableName, "diffRun")) {
            success = pxSetStateCleaned("diffRun", "state", output);
#ifdef notyet
        // diffSkyfile doesn't have dataState yet
        } else if (!strcmp(tables[i].tableName, "diffSkyfile")) {
            success = pxSetStateCleaned("diffSkyfile", "data_state", output);
#endif
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

  int numImportTables = 2;

  char tables[2] [80] = {"diffInputSkyfile", "diffSkyfile"};

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
  psMetadataItem *item = psMetadataLookup (input, "diffRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  diffRunRow *diffRun = diffRunObjectFromMetadata (entry->data.md);
  diffRunInsertObject (config->dbh, diffRun);

  // fprintf (stdout, "---- diff run ----\n");
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
          diffInputSkyfileRow *diffInputSkyfile = diffInputSkyfileObjectFromMetadata (entry->data.md);
          diffInputSkyfileInsertObject (config->dbh, diffInputSkyfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 1:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          diffSkyfileRow *diffSkyfile = diffSkyfileObjectFromMetadata (entry->data.md);
          diffSkyfileInsertObject (config->dbh, diffSkyfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;
    }
  }

  return true;
}

static bool listrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_id",    "diffRun.diff_id",  "==");
    PXOPT_COPY_STR(config->args, where,  "-tess_id",    "diffRun.tess_id",  "LIKE");
    PXOPT_COPY_S64(config->args, where,  "-magicked",   "diffRun.magicked", "==");
    PXOPT_COPY_STR(config->args, where,  "-state",      "diffRun.state",    "==");
    pxAddLabelSearchArgs (config, where, "-label",      "diffRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "diffRun.data_group", "LIKE");
    pxAddLabelSearchArgs (config, where, "-dist_group", "diffRun.dist_group", "LIKE");

    PXOPT_COPY_S64(config->args, where, "-template_exp_id", "rawTemplate.exp_id", "==");

    PXOPT_LOOKUP_BOOL(template, config->args, "-template", false);
    if (!template) {
        PXOPT_COPY_S64(config->args, where, "-exp_id", "rawInput.exp_id", "==");
        PXOPT_COPY_STR(config->args, where, "-exp_name", "rawInput.exp_name", "==");
        PXOPT_COPY_STR(config->args, where, "-warp_id", "warpInput.warp_id", "==");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawInput.dateobs",  ">=");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawInput.dateobs",  "<=");
        PXOPT_COPY_STR(config->args, where, "-filter",     "rawInput.filter", "LIKE");
    } else {
        PXOPT_COPY_S64(config->args, where, "-exp_id", "rawTemplate.exp_id", "==");
        PXOPT_COPY_STR(config->args, where, "-exp_name", "rawTemplate.exp_name", "==");
        PXOPT_COPY_STR(config->args, where, "-warp_id", "warpTemplate.warp_id", "==");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawTemplate.dateobs",  ">=");
        PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "rawTemplate.dateobs",  "<=");
        PXOPT_COPY_STR(config->args, where, "-filter",     "rawTemplate.filter", "LIKE");
    }

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);
    PXOPT_LOOKUP_S16(diff_mode, config->args, "-diff_mode", false, false);
    if (diff_mode) {
      PXOPT_COPY_S16(config->args, where, "-diff_mode", "diffRun.diff_mode", "==");
    }

    psString where2 = NULL;
    pxmagicAddWhere(config, &where2, "diffRun");
    pxspaceAddWhere(config, &where2, template ? "rawTemplate" : "rawInput");
    psString query = pxDataGet("difftool_listrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (where2) {
        psStringAppend(&query, " WHERE diffRun.diff_id is not null %s", where2);
    } else if (!all) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }
    psFree(where);

    if (pstamp_order) {
        if (template) {
            psStringAppend(&query, " ORDER BY rawTemplate.exp_id, diff_id DESC");
        } else {
            psStringAppend(&query, " ORDER BY rawInput.exp_id, diff_id DESC");
        }
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}
static bool listssrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-diff_id",    "diffRun.diff_id",  "==");
    PXOPT_COPY_S64(config->args, where,  "-stack_id",   "stackInput.stack_id",  "==");
    PXOPT_COPY_S64(config->args, where,  "-template_stack_id",  "stackTemplate.stack_id",  "==");
    PXOPT_COPY_STR(config->args, where,  "-tess_id",    "diffRun.tess_id",  "LIKE");
    PXOPT_COPY_STR(config->args, where,  "-state",      "diffRun.state",    "==");
    pxAddLabelSearchArgs (config, where, "-label",      "diffRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "diffRun.data_group", "LIKE");
    pxAddLabelSearchArgs (config, where, "-dist_group", "diffRun.dist_group", "LIKE");

    // lookup these so we don't compare to zero if they are not supplied
    PXOPT_LOOKUP_F64(mjd_obs_begin, config->args, "-mjd_obs_begin", false, false);
    PXOPT_LOOKUP_F64(mjd_obs_end, config->args, "-mjd_obs_end", false, false);

    PXOPT_LOOKUP_BOOL(template, config->args, "-template", false);

    if (!template) {
        if (mjd_obs_begin) {
            PXOPT_COPY_F64(config->args, where, "-mjd_obs_begin", "stackInput.mjd_obs",  ">=");
        }
        if (mjd_obs_end) {
            PXOPT_COPY_F64(config->args, where, "-mjd_obs_end",   "stackInput.mjd_obs",  "<=");
        }
        PXOPT_COPY_STR(config->args, where, "-filter",        "stackInputRun.filter", "LIKE");
    } else {
        if (mjd_obs_begin) {
            PXOPT_COPY_F64(config->args, where, "-mjd_obs_begin", "stackTemplate.mjd_obs",  ">=");
        }
        if (mjd_obs_end) {
            PXOPT_COPY_F64(config->args, where, "-mjd_obs_end",   "stackTemplate.mjd_obs",  "<=");
        }
        PXOPT_COPY_STR(config->args, where, "-filter",        "stackTemplateRun.filter", "LIKE");
    }

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);

    psString where2 = NULL;
    psString query = pxDataGet("difftool_listssrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else if (where2) {
        psStringAppend(&query, " AND %s", where2);
    } else if (!all) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }
    psFree(where);

    if (pstamp_order) {
        if (template) {
            psStringAppend(&query, " ORDER BY stackTemplate.stack_id, diff_id DESC");
        } else {
            psStringAppend(&query, " ORDER BY stackInput.stack_id, diff_id DESC");
        }
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
        psTrace("difftool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffRun", !simple)) {
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

    PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);

    psString query = pxDataGet("difftool_setskyfiletoupdate.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString setHook = psStringCopy("");
    if (label) {
        psStringAppend(&setHook, "\n , diffRun.label = '%s'", label);
    }

    if (skycell_id) {
        psStringAppend(&query, " AND (diffSkyfile.skycell_id = '%s')", skycell_id);
    }
    // we do not update components with the magic fault value. They are non-updateable
    // (But can be recovered with "difftool -revertwarped -fault 26" (PXTOOL_DO_NOT_REVERT_FAULT)
    psStringAppend(&query, " AND (diffSkyfile.fault != %d)", PXTOOL_DO_NOT_REVERT_FAULT);

    if (!p_psDBRunQueryF(config->dbh, query, setHook, diff_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(setHook);
    psFree(query);

    return true;
}

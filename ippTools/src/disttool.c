/*
 * disttool.c
 *
 * Copyright (C) 2008-2009
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
#include "disttool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool revertrunMode(pxConfig *config);
static bool startoverMode(pxConfig *config);
static bool pendingcomponentMode(pxConfig *config);
static bool addprocessedcomponentMode(pxConfig *config);
static bool updateprocessedcomponentMode(pxConfig *config);
static bool revertcomponentMode(pxConfig *config);
static bool revertcomponentMode(pxConfig *config);
static bool processedcomponentMode(pxConfig *config);
static bool toadvanceMode(pxConfig *config);
static bool pendingfilesetMode(pxConfig *config);
static bool addfilesetMode(pxConfig *config);
static bool revertfilesetMode(pxConfig *config);
static bool updatefilesetMode(pxConfig *config);
static bool queuercrunMode(pxConfig *config);
static bool updatercrunMode(pxConfig *config);
static bool revertrcrunMode(pxConfig *config);
static bool pendingdestMode(pxConfig *config);
static bool pendingcleanupMode(pxConfig *config);
static bool listfilesetsMode(pxConfig *config);

static bool definetargetMode(pxConfig *config);
static bool updatetargetMode(pxConfig *config);
static bool listtargetsMode(pxConfig *config);

static bool definedestinationMode(pxConfig *config);
static bool updatedestinationMode(pxConfig *config);

static bool defineinterestMode(pxConfig *config);
static bool updateinterestMode(pxConfig *config);
static bool listinterestsMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
                goto FAIL; \
            } \
    break;


int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = disttoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(DISTTOOL_MODE_DEFINEBYQUERY, definebyqueryMode);
        MODECASE(DISTTOOL_MODE_UPDATERUN, updaterunMode);
        MODECASE(DISTTOOL_MODE_REVERTRUN, revertrunMode);
        MODECASE(DISTTOOL_MODE_STARTOVER, startoverMode);
        MODECASE(DISTTOOL_MODE_PENDINGCOMPONENT, pendingcomponentMode);
        MODECASE(DISTTOOL_MODE_ADDPROCESSEDCOMPONENT, addprocessedcomponentMode);
        MODECASE(DISTTOOL_MODE_UPDATEPROCESSEDCOMPONENT, updateprocessedcomponentMode);
        MODECASE(DISTTOOL_MODE_PROCESSEDCOMPONENT, processedcomponentMode);
        MODECASE(DISTTOOL_MODE_REVERTCOMPONENT, revertcomponentMode);
        MODECASE(DISTTOOL_MODE_TOADVANCE, toadvanceMode);
        MODECASE(DISTTOOL_MODE_PENDINGFILESET, pendingfilesetMode);
        MODECASE(DISTTOOL_MODE_PENDINGCLEANUP, pendingcleanupMode);
        MODECASE(DISTTOOL_MODE_ADDFILESET, addfilesetMode);
        MODECASE(DISTTOOL_MODE_REVERTFILESET, revertfilesetMode);
        MODECASE(DISTTOOL_MODE_LISTFILESETS, listfilesetsMode);
        MODECASE(DISTTOOL_MODE_UPDATEFILESET, updatefilesetMode);
        MODECASE(DISTTOOL_MODE_QUEUERCRUN, queuercrunMode);
        MODECASE(DISTTOOL_MODE_UPDATERCRUN, updatercrunMode);
        MODECASE(DISTTOOL_MODE_REVERTRCRUN, revertrcrunMode);
        MODECASE(DISTTOOL_MODE_PENDINGDEST, pendingdestMode);
        MODECASE(DISTTOOL_MODE_DEFINETARGET, definetargetMode);
        MODECASE(DISTTOOL_MODE_UPDATETARGET, updatetargetMode);
        MODECASE(DISTTOOL_MODE_LISTTARGETS, listtargetsMode);
        MODECASE(DISTTOOL_MODE_DEFINEDESTINATION, definedestinationMode);
        MODECASE(DISTTOOL_MODE_UPDATEDESTINATION, updatedestinationMode);
        MODECASE(DISTTOOL_MODE_DEFINEINTEREST, defineinterestMode);
        MODECASE(DISTTOOL_MODE_UPDATEINTEREST, updateinterestMode);
        MODECASE(DISTTOOL_MODE_LISTINTERESTS, listinterestsMode);
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
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(stage, config->args,     "-stage", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args,   "-workdir", true, false);

    // optional
    PXOPT_LOOKUP_BOOL(use_alternate, config->args, "-use_alternate", false);
    PXOPT_LOOKUP_BOOL(magic, config->args, "-magic", false);
    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(set_data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(set_note, config->args, "-set_note", false, false);

    PXOPT_LOOKUP_S64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    // select arguments
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-target_id", "distTarget.target_id",    "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",    "rawExp.exp_id",           "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",   "chipRun.chip_id",         "==");
    PXOPT_COPY_STR(config->args, where, "-exp_type", "exp_type", "==");;
    PXOPT_COPY_STR(config->args, where, "-dist_group", "distTarget.dist_group", "==");;

    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", false, false);
    PXOPT_LOOKUP_STR(exp_type, config->args, "-exp_type", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-dist_group", false, false);

    PXOPT_LOOKUP_BOOL(single, config->args, "-singlefilter", false);
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);

    if (use_alternate) {
        if (strcmp(stage, "raw")) {
            psError(PXTOOLS_ERR_SYS, true, "alternate inputs only supported for raw stage");
            return false;
        }
        if (!magic) {
            psError(PXTOOLS_ERR_SYS, true, "no_magic forbidden with alternate inputs");
            return false;
        }
    }

    // We want to possible to distribute unmagicked raw exposures for the purpose
    // of distributing detrend inputs
    // To make it less likely that unmagicked raw stage exposures from being distributed
    // accidentally we require that dist_group (to select target and destination with 
    // an interest)
    // and exp_id to be supplied. We could add -exp_type and dateobs cuts to make it easier
    if (!strcmp(stage, "raw") && !magic && !(exp_id && dist_group && exp_type)) {
        psError(PXTOOLS_ERR_SYS, true, "exp_id, exp_type, and dist_group are required for raw stage if no_magic");
        return false;
    }


    psString query = NULL;
    psString magicRunType = NULL;
    psString runJoinStr = NULL;
    if (!strcmp(stage, "raw")) {
        if (! use_alternate ) {
            magicRunType = "rawExp";
            runJoinStr = "rawExp.exp_id";
        } else {
            magicRunType = "camRun";
            runJoinStr = "camRun.exp_id";
        }
        if (magic) {
            query = pxDataGet("disttool_definebyquery_raw.sql");
        } else {
            query = pxDataGet("disttool_definebyquery_raw_no_magic.sql");
        }
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (magic) {
            if (label) {
                psStringAppend(&query, " AND (magicDSRun.label = '%s')", label);
            }
            // for raw stage we select by camRun.label and dist_group because rawExp
            // doesn't have those columns
            if (dist_group) {
                psStringAppend(&query, " AND (camRun.dist_group = '%s')", dist_group);
            }
        }
    } else if (!strcmp(stage, "chip")) {
        magicRunType = "chipRun";
        runJoinStr = "chipRun.chip_id";
        query = pxDataGet("disttool_definebyquery_chip.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (chipRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (chipRun.dist_group = '%s')", dist_group);
        }
    } else if (!strcmp(stage, "chip_bg")) {
        magicRunType = "chipBackgroundRun";
        runJoinStr = "chipBackgroundRun.chip_bg_id";
        query = pxDataGet("disttool_definebyquery_chip_bg.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (chipBackgroundRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (chipBackgroundRun.dist_group = '%s')", dist_group);
        }
    } else if (!strcmp(stage, "camera")) {
        magicRunType = "camRun";    // This is used below to set the magicked business
        runJoinStr = "camRun.cam_id";
        query = pxDataGet("disttool_definebyquery_camera.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (camRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (camRun.dist_group = '%s')", dist_group);
        }
    } else if (!strcmp(stage, "fake")) {
        magicRunType = "fakeRun";
        query = pxDataGet("disttool_definebyquery_fake.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (fakeRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (fakeRun.dist_group = '%s')", dist_group);
        }
        // fake stage doesn't require magic
        magic = false;
    } else if (!strcmp(stage, "warp")) {
        magicRunType = "warpRun";
        runJoinStr = "warpRun.warp_id";
        query = pxDataGet("disttool_definebyquery_warp.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (warpRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (warpRun.dist_group = '%s')", dist_group);
        }

    } else if (!strcmp(stage, "warp_bg")) {
        magicRunType = "warpBackgroundRun";
        runJoinStr = "warpBackgroundRun.warp_bg_id";
        query = pxDataGet("disttool_definebyquery_warp_bg.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (warpBackgroundRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (warpBackgroundRun.dist_group = '%s')", dist_group);
        }

    } else if (!strcmp(stage, "diff")) {
        magicRunType = "diffRun";
        runJoinStr = "diffRun.diff_id";
        query = pxDataGet("disttool_definebyquery_diff.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (diffRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (diffRun.dist_group = '%s')", dist_group);
        }

    } else if (!strcmp(stage, "stack")) {
        magicRunType = "stackRun";
        query = pxDataGet("disttool_definebyquery_stack.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (stackRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (stackRun.dist_group = '%s')", dist_group);
        }
        // stack stage doesn't require magic
        magic = false;
    } else if (!strcmp(stage, "sky")) {
        magicRunType = "staticskyRun";
        if (single) {
            query = pxDataGet("disttool_definebyquery_sky_singlefilter.sql");
        } else {
            query = pxDataGet("disttool_definebyquery_sky.sql");
        }
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (staticskyRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (staticskyRun.dist_group = '%s')", dist_group);
        }
        // (static)sky stage doesn't require magic
        magic = false;
    } else if (!strcmp(stage, "skycal")) {
        query = pxDataGet("disttool_definebyquery_skycal.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }

        if (label) {
            psStringAppend(&query, " AND (skycalRun.label = '%s')", label);
        }
        if (dist_group) {
            psStringAppend(&query, " AND (skycalRun.dist_group = '%s')", dist_group);
        }
        // skycal stage doesn't require magic
        magic = false;
    } else if (!strcmp(stage, "SSdiff")) {
      magicRunType = "diffRun";
      runJoinStr = "diffRun.diff_id";
      query = pxDataGet("disttool_definebyquery_SSdiff.sql");
      if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
        psFree(where);
        return(false);
      }

      if (label) {
        psStringAppend(&query, " AND (diffRun.label = '%s') ", label);
      }
      if (dist_group) {
        psStringAppend(&query, " AND (diffRun.dist_group = '%s') ", dist_group);
      }

      magic = false;
    } else if (!strcmp(stage, "ff")) {
      runJoinStr = "fullForceRun.ff_id";
      query = pxDataGet("disttool_definebyquery_ff.sql");
      if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
        psFree(where);
        return(false);
      }

      if (label) {
        psStringAppend(&query, " AND (fullForceRun.label = '%s') ", label);
      }
      if (dist_group) {
        psStringAppend(&query, " AND (fullForceRun.dist_group = '%s') ", dist_group);
      }

      magic = false;
    } else {
        psError(PS_ERR_UNKNOWN, true, "unknown value for stage: %s", stage);
        psFree(where);
        return false;
    }

    if (0) {
	fprintf (stderr, "runJoinStr: %s\n", runJoinStr);
    }

    if (!strcmp(stage, "raw")) {
        if (magic) {
            psStringAppend(&query, " AND (magicDSRun.re_place = %d)", !use_alternate);
            psStringAppend(&query, " AND (camRun.state = 'full')");
        }
    }
    if (!rerun) {
        psStringAppend(&query, " AND (distRun.dist_id IS NULL)");
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psString joinHook = NULL;

    if (magic) {
        psStringAppend(&query, " AND (%s.magicked > 0)", magicRunType);
    }

    if (!strcmp(stage, "sky")) {
        if (single) {
            psStringAppend(&query, "\nGROUP BY sky_id HAVING count(stack_id) = 1");
        } else {
            psStringAppend(&query, "\nGROUP BY sky_id HAVING count(stack_id) > 1");
        }
    }

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }
    psTrace("disttool",2,query,joinHook ? joinHook : "");
    if (!p_psDBRunQueryF(config->dbh, query, joinHook ? joinHook : "")) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(query);
      return false;
    }
    psFree(query);
    psFree(joinHook);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }


    if (pretend) {
        if (!ippdbPrintMetadatas(stdout, output, "newdistRuns", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *list = psArrayAllocEmpty(limit);
    for (long i=0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
        psString stage = psMetadataLookupStr(NULL, md, "stage");
        psString run_tag = psMetadataLookupStr(NULL, md, "run_tag");
        psS64 stage_id = psMetadataLookupS64(NULL, md, "stage_id");
        psS64 magic_ds_id = psMetadataLookupS64(NULL, md, "magicked");
        psS64 target_id = psMetadataLookupS64(NULL, md, "target_id");
        psString stage_label = psMetadataLookupStr(NULL, md, "label");
        psString stage_data_group = psMetadataLookupStr(NULL, md, "data_group");
        bool clean = psMetadataLookupBool(NULL, md, "clean");

        psString outroot = NULL;
        psStringAppend(&outroot, "%s/%s/%s", workdir, run_tag, stage);

        psString new_label;
        if (set_label != NULL) {
            new_label = set_label;
        } else {
            new_label = stage_label;
        }
        psString new_data_group;
        if (set_data_group != NULL) {
            new_data_group = set_data_group;
        } else {
            new_data_group = stage_data_group;
        }

        distRunRow *row = distRunRowAlloc(
                0,      // dist_id
                target_id,
                stage,
                stage_id,
                magic_ds_id,
                new_label,
                outroot,
                NULL,     // outdir
                clean,
                !magic,
                use_alternate,
                "new",
                NULL,    // time_stamp
                0,       // fault
                new_data_group,
                set_note // note does not propagate
                );

        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to allocate distRunRow");
            psFree(outroot);
            psFree(output);
            return false;
        }
        if (!distRunInsertObject(config->dbh, row)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(outroot);
            psFree(output);
            return false;
        }
        psFree(outroot);
        psS64 dist_id = psDBLastInsertID(config->dbh);
        row->dist_id = dist_id;
        psArrayAdd(list, list->n, row);
        psFree(row);
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!distRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }

    psFree(list);
    psFree(output);

    return true;
}

static bool updaterunMode(pxConfig *config)
{
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");;
    PXOPT_COPY_STR(config->args, where, "-state", "distRun.state", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-data_group", "distRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "distTarget.dist_group", "==");
    PXOPT_COPY_TIME(config->args, where, "-time_stamp_begin", "distRun.time_stamp", ">=");
    PXOPT_COPY_TIME(config->args, where, "-time_stamp_end", "distRun.time_stamp", "<=");
    PXOPT_COPY_S64(config->args, where, "-dist_id_min", "dist_id", ">=");
    PXOPT_COPY_S64(config->args, where, "-dist_id_max", "dist_id", "<=");

    PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
    PXOPT_LOOKUP_BOOL(full, config->args, "-full", false);
    if (clean && full) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, true, "-clean and -full are contradictory parameters");
        return false;
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(outdir, config->args, "-set_outdir", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_STR(set_note, config->args, "-set_note", false, false);

    if ((!state) && (!label) && (!fault) &&(!data_group)) {
        psError(PXTOOLS_ERR_CONFIG, false, "parameters (-fault or -set_state or -set_label -set_data_group) are required");
        psFree(where);
        return false;
    }

    psString extraWhere = NULL;
    psString query = psStringCopy("UPDATE distRun join distTarget using(target_id, stage) SET distRun.time_stamp = UTC_TIMESTAMP()");

    if (outdir) {
        psStringAppend(&query, " , distRun.outdir = '%s'", outdir);
    }
    if (state) {
        psStringAppend(&query, " , distRun.state = '%s'", state);
        if (!strcmp(state, "goto_cleaned")) {
            // don't queue for clean up if run has already already cleaned
            psStringAppend(&extraWhere, " AND (distRun.state != 'cleaned' AND distRun.state != 'goto_cleaned')");
        }
    }

    if (label) {
        psStringAppend(&query, " , distRun.label = '%s'", label);
    }

    if (fault) {
        psStringAppend(&query, " , distRun.fault = %d", fault);
    }

    if (data_group) {
        psStringAppend(&query, " , distRun.data_group = '%s'", data_group);
    }

    if (set_note) {
        psStringAppend(&query, " , distRun.note = '%s'", set_note);
    }


    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (extraWhere) {
        psStringAppend(&query, "%s", extraWhere);
    }
    if (clean) {
        psStringAppend(&query, " AND (distRun.clean)");
    } else if (full) {
        psStringAppend(&query, " AND (!distRun.clean)");
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    return true;
}

static bool startoverMode(pxConfig *config)
{
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "distRun.dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");;
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "distRun.data_group", "LIKE");

    // require data_group or dist_id to be supplied
    PXOPT_LOOKUP_STR(data_group, config->args, "-data_group", false, false);
    PXOPT_LOOKUP_S64(dist_id, config->args, "-dist_id", false, false);
    if (!data_group && !dist_id) {
        psError(PXTOOLS_ERR_CONFIG, true, "data_group or dist_id is required");
        return false;
    }
    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);

    psString label_hook = psStringCopy("");
    if (set_label) {
        psStringAppend(&label_hook, ", label = '%s'", set_label);
    }

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);


    // It might be useful to be able to query by the parameters of the underlying runs

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("disttool_rerun_select.sql");
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
    psFree(where);

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
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (simple) {/* no option? */}

    if (pretend) {
        if (!ippdbPrintMetadatas(stdout, output, "distRunsToUpdate", true)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    query = "DELETE FROM distComponent where dist_id = %" PRId64;
    char *query2 =  "DELETE from rcDSFileset using distRun, rcDSFileset WHERE distRun.dist_id = rcDSFileset.dist_id AND rcDSFileset.state ='cleaned' AND rcDSFileset.dist_id = %" PRId64;
    
    for (long i=0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
        psS64 dist_id = psMetadataLookupS64(NULL, md, "dist_id");

        if (!psDBTransaction(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        // delete any existing distComponents
        if (!p_psDBRunQueryF(config->dbh, query, dist_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        // delete any exisiting fileset
        if (!p_psDBRunQueryF(config->dbh, query2, dist_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        if (!p_psDBRunQueryF(config->dbh,
        "UPDATE distRun SET state = 'new', fault=0 %s WHERE dist_id =%" PRId64,
                label_hook, dist_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        printf("re-running distRun %" PRId64 "\n", dist_id);
    }

    psFree(output);

    return true;
}
static bool revertrunMode(pxConfig *config)
{
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "distRun.dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");;
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    pxAddLabelSearchArgs(config, where, "-label", "distRun.label", "==");

    PXOPT_COPY_S16(config->args, where,  "-fault", "distRun.fault", "==");

    // It might be useful to be able to query by the parameters of the underlying runs

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("disttool_revertrun.sql");
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
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    int numUpdated = psDBAffectedRows(config->dbh);

    psLogMsg("disttool", PS_LOG_INFO, "Updated %d dist runs", numUpdated);

    return true;
}

static bool revertcomponentMode(pxConfig *config)
{
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "distRun.dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");;
    PXOPT_COPY_STR(config->args, where, "-component", "component", "==");;
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    pxAddLabelSearchArgs(config, where, "-label", "distRun.label", "==");

    PXOPT_COPY_S16(config->args, where,  "-fault", "distComponent.fault", "==");

    // It might be useful to be able to query by the parameters of the underlying runs

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("disttool_revertcomponent.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

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

    int numDeleted = psDBAffectedRows(config->dbh);

    psLogMsg("disttool", PS_LOG_INFO, "Deleted %d distComponents", numDeleted);

    return true;
}

static bool pendingcomponentMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "distRun.dist_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "distRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString queryFile = NULL;
    psStringAppend(&queryFile, "disttool_pending_%s.sql", stage);
    psString query = pxDataGet(queryFile);
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement from %s", queryFile);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, "\nORDER BY priority DESC, dist_id");

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
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "pendingcomponent", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool addprocessedcomponentMode(pxConfig *config)
{

    // required values
    PXOPT_LOOKUP_S64(dist_id, config->args, "-dist_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", true, false);

    // unless fault code is set require filename, bytes, and md5sum
    bool require_fileinfo = false;
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    if (!fault) {
        require_fileinfo = true;
    }
    PXOPT_LOOKUP_S32(bytes, config->args, "-bytes", require_fileinfo, false);
    PXOPT_LOOKUP_STR(md5sum, config->args, "-md5sum", require_fileinfo, false);
    PXOPT_LOOKUP_STR(outdir, config->args, "-outdir", require_fileinfo, false);
    PXOPT_LOOKUP_STR(name, config->args, "-name", require_fileinfo, false);

    if (!distComponentInsert(config->dbh, dist_id, component, bytes, md5sum, "full", outdir, name, fault)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}
static bool updateprocessedcomponentMode(pxConfig *config)
{

    // required values
    PXOPT_LOOKUP_S64(dist_id, config->args, "-dist_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", true, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_BOOL(clearfault, config->args, "-clearfault", false);
    PXOPT_LOOKUP_S32(bytes, config->args, "-bytes", false, false);
    PXOPT_LOOKUP_STR(md5sum, config->args, "-md5sum", false, false);
    PXOPT_LOOKUP_STR(outdir, config->args, "-outdir", false, false);
    PXOPT_LOOKUP_STR(name, config->args, "-name", false, false);

    bool setfault = clearfault || fault;

    if (!bytes && !md5sum && !outdir && !name && !setfault && !fault) {
        psError(PS_ERR_UNKNOWN, true, "at least one of bytes md5sum outdir name fault or setfault is required");
        return false;
    }

    char *sep = "";
    psString query = psStringCopy("UPDATE distComponent SET ");
    if (setfault) {
        psStringAppend(&query, "%s fault = %d", sep, fault ? 1 : 0);
        sep = ", ";
    }
    if (bytes) {
        psStringAppend(&query, "%s bytes = %d", sep, bytes);
        sep = ", ";
    }
    if (md5sum) {
        psStringAppend(&query, "%s md5sum = '%s'", sep, md5sum);
        sep = ", ";
    }
    if (outdir) {
        psStringAppend(&query, "%s outdir = '%s'", sep, outdir);
        sep = ", ";
    }
    if (name) {
        psStringAppend(&query, "%s name = '%s'", sep, name);
        sep = ", ";
    }

    psStringAppend(&query, "\nWHERE dist_id = %"PRId64 " AND component = '%s'",
        dist_id, component);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool toadvanceMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("disttool_toadvance.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

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
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "toadvance", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}
static bool processedcomponentMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-component", "component", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("disttool_processedcomponent.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

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
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "processedcomponent", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool pendingfilesetMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "distRun.stage", "==");
    pxAddLabelSearchArgs (config, where, "-label", "distRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("disttool_pendingfileset.sql");
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
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "pendingfileset", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}
static bool addfilesetMode(pxConfig *config)
{

    // required values
    PXOPT_LOOKUP_S64(dist_id, config->args, "-dist_id", true, false);
    PXOPT_LOOKUP_S64(dest_id, config->args, "-dest_id", true, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    // unless fault code is set require name
    PXOPT_LOOKUP_STR(name, config->args, "-name", fault == 0, false);

    if (!rcDSFilesetInsert(config->dbh,
            0,          // fs_id
            dist_id,
            dest_id,
            name,
            "full",
            fault)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool revertfilesetMode(pxConfig *config)
{
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fs_id", "fs_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dist_id", "rcDSFileset.dist_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dest_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");;
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    pxAddLabelSearchArgs(config, where, "-label", "label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "rcDSFileset.fault", "==");

    // It might be useful to be able to query by the parameters of the underlying runs

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("disttool_revertfileset.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
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

    int numUpdated = psDBAffectedRows(config->dbh);

    psLogMsg("disttool", PS_LOG_INFO, "deleted %d rcDSFilesets", numUpdated);

    return true;
}

static bool pendingdestMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dist_id", "==");
//     PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("disttool_pendingdest.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereString = NULL;
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        // where string gets added by hook, not at the end of the query
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

    if (!p_psDBRunQueryF(config->dbh, query, whereString ? whereString : "")) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereString);
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
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "pendingfileset", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool queuercrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-dist_id",  "dist_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id",  "dest_id", "==");
    PXOPT_COPY_S64(config->args, where, "-target_id","target_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fs_id",    "fs_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage",    "stage", "==");;
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",    "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = pxDataGet("disttool_queuercrun.sql");
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

    long numUpdated = psDBAffectedRows(config->dbh);

    psLogMsg("disttool", PS_LOG_INFO, "Inserted %ld rcRuns", numUpdated);

    return true;
}


static bool updatercrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_LOOKUP_S64(rc_id, config->args, "-rc_id", false, false);
    PXOPT_LOOKUP_STR(fs_name, config->args, "-fs_name", false, false);
    PXOPT_LOOKUP_STR(status_fs_name, config->args, "-status_fs_name", false, false);
    PXOPT_LOOKUP_S64(dest_id, config->args, "-dest_id", false, false);

    // We either need rc_id or (dest_id and fs_name) to identifiy the rcRun
    if ((!rc_id) && !(dest_id && fs_name)) {
        psError(PXTOOLS_ERR_CONFIG, true, "either -rc_id or (-fs_name and -dest_id) are required");
        return false;
    }

    // now that we have done the argument checking
    PXOPT_COPY_S64(config->args, where, "-rc_id", "rc_id", "==");
    PXOPT_COPY_STR(config->args, where, "-fs_name", "rcDSFileset.name", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id", "rcRun.dest_id", "==");

    if (!psListLength(where->list)) {
        // this can't happen because we checked above
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }


    PXOPT_LOOKUP_STR(last_fileset, config->args, "-set_last_fileset", false, false);
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!state && (fault < 0)) {
        psError(PXTOOLS_ERR_CONFIG, false, "parameters (-fault or -set_state) are required");
        psFree(where);
        return false;
    }

    psString query = pxDataGet("disttool_updatercrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString setString = psStringCopy("\n");
    psString separator = "";
    if (state) {
        psStringAppend(&setString, " rcRun.state = '%s'", state);
        separator = ",";
    }

    // default for fault is -1, so set it if it's zero or higher. This allows clearing fault
    // without forcing revert first
    if (fault >= 0) {
        psStringAppend(&setString, "%s rcRun.fault = %d", separator, fault);
    }

    if (status_fs_name) {
        psStringAppend(&setString, ", rcRun.status_fs_name = '%s'", status_fs_name);
    }

    if (last_fileset) {
        psStringAppend(&setString, ", rcDestination.last_fileset = '%s'", last_fileset);
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, setString)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(setString);
        psFree(query);
        return false;
    }
    psFree(setString);
    psFree(query);

    long numUpdated = psDBAffectedRows(config->dbh);

    psLogMsg("disttool", PS_LOG_INFO, "Updated %ld rows", numUpdated);

    return true;
}

static bool revertrcrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-rc_id", "rc_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fs_id", "fs_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dest_id", "==");
    PXOPT_COPY_S64(config->args, where, "-fault", "fault", "==");

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("disttool_revertrcrun.sql");
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

    long numUpdated = psDBAffectedRows(config->dbh);

    psLogMsg("disttool", PS_LOG_INFO, "Updated %ld rcRuns", numUpdated);

    return true;
}

static bool definetargetMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(dist_group, config->args, "-dist_group", true, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", true, false);
    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);

    // optional
    PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_STR(comment, config->args, "-comment", false, false);

    distTargetRow *row = distTargetRowAlloc(
            0,          // target_id
            dist_group,
            filter,
            stage,
            clean,
            state ? state : "enabled",
            comment
            );

    if (!row) {
        psError(PS_ERR_UNKNOWN, false, "failed to allocate distTarget object");
        return false;
    }
   if (!distTargetInsertObject(config->dbh, row)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        return false;
    }

    // get the assigned target_id
    row->target_id = psDBLastInsertID(config->dbh);

    if (!distTargetPrintObject(stdout, row, true)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print object");
            psFree(row);
            return false;
    }

    psFree(row);

    return true;
}
static bool updatetargetMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-target_id", "target_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "dist_group", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");

    PXOPT_LOOKUP_STR(state, config->args, "-set_state", true, false);

    psString query = psStringCopy("UPDATE distTarget SET state = '%s'");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        psFree(where);
        psFree(query);
        return false;
    }
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, state)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool listtargetsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-target_id", "target_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "dist_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");

    PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
    PXOPT_LOOKUP_BOOL(full, config->args, "-full", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (clean && full) {
        psError(PS_ERR_UNKNOWN, false, "can't select both -clean and -full");
        return false;
    }

    psString query = psStringCopy("SELECT * FROM distTarget");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
        if (clean) {
            psStringAppend(&query, " AND (clean)");
        } else if (full) {
            psStringAppend(&query, " AND (!clean)");
        }
    } else if (clean) {
        psStringAppend(&query, " WHERE clean");
    } else if (full) {
        psStringAppend(&query, " WHERE !clean");
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
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "distTarget", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool definedestinationMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(name, config->args,         "-name", true, false);
    PXOPT_LOOKUP_STR(dbname, config->args,       "-ds_dbname", true, false);
    PXOPT_LOOKUP_STR(dbhost, config->args,       "-ds_dbhost", true, false);

    // optional
    PXOPT_LOOKUP_STR(status_uri, config->args,   "-status_uri", false, false);
    PXOPT_LOOKUP_STR(comment, config->args,      "-comment", false, false);
    PXOPT_LOOKUP_STR(last_fileset, config->args, "-last_fileset", false, false);
    PXOPT_LOOKUP_STR(state, config->args,        "-set_state", false, false);

    // XXX: should we insure that these names do not contatin any whitespace?

    rcDestinationRow *row = rcDestinationRowAlloc(
            0,          // dest_id
            name,
            status_uri,
            comment,
            last_fileset,
            dbname,
            dbhost,
            state ? state : "enabled"
            );

    if (!row) {
        psError(PS_ERR_UNKNOWN, false, "failed to allocate rcDestination object");
        return false;
    }
   if (!rcDestinationInsertObject(config->dbh, row)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        return false;
    }

    // get the assigned target_id
    row->dest_id = psDBLastInsertID(config->dbh);

    if (!rcDestinationPrintObject(stdout, row, true)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(row);
        return false;
    }

    psFree(row);

    return true;
}

static bool updatedestinationMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dest_id", "==");

    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
#ifdef ALLOW_UPDATE_LAST_FILESET
    PXOPT_LOOKUP_STR(last_fileset, config->args, "-set_last_fileset", false, false);
    if (!(state || last_fileset)) {
        psError(PS_ERR_UNKNOWN, true, "one or more of -set_state or -set_last_fileset is required");
# else
    if (!state) {
#endif
        psFree(where);
        return false;
    }
    psString query = psStringCopy("UPDATE rcDestination SET");
    psString sep = "";
    if (state) {
        psStringAppend(&query, " state = '%s'", state);
        sep = ",";
    }
#ifdef ALLOW_UPDATE_LAST_FILESET
    // last_fileset normally gets set by updatercrunMode
    // Allowing it to be set here might cause problems
    // especially since we are allowing selection by dest_id
    if (last_fileset) {
        psStringAppend(&query, " %s last_fileset = '%s'", sep, last_fileset);
    }
#else
    if (0) { fprintf (stderr, "sep: %s\n", sep); }
#endif

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        psFree(where);
        psFree(query);
        return false;
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

static bool defineinterestMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // one of these is required
    PXOPT_LOOKUP_S64(dest_id, config->args,      "-dest_id", false, false);
    PXOPT_LOOKUP_STR(dest_name, config->args,    "-dest_name", false, false);
    if (!dest_id && !dest_name) {
        psError(PS_ERR_UNKNOWN, true, "either dest_id or dest_name is required");
        return false;
    }

    // either target_id or stage and label are required
    PXOPT_LOOKUP_S64(target_id, config->args,    "-target_id", false, false);
    PXOPT_LOOKUP_STR(stage, config->args,        "-stage", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args,   "-dist_group", false, false);
    PXOPT_LOOKUP_STR(filter, config->args,       "-filter", false, false);
    PXOPT_LOOKUP_BOOL(clean, config->args,       "-clean", false);

    if (!target_id) {
        bool error = false;
        if (!stage) {
            psError(PS_ERR_UNKNOWN, true, "stage is required if target_id is not supplied");
            error = true;
        }
        if (!dist_group) {
            psError(PS_ERR_UNKNOWN, !error, "dist_group is required if target_id is not supplied");
            error = true;
        }
        if (error) {
            return false;
        }
    }

    // optional
    PXOPT_LOOKUP_S64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_STR(state, config->args,  "-set_state", false, false);
    if (state) {
        if (strcmp(state, "enabled") && strcmp(state, "disabled")) {
            psError(PS_ERR_PROGRAMMING, true, "state must be enabled or disabled");
            return false;
        }
    } else {
        // default state
        state = "enabled";
    }

    // now that we've done all of our argument checking, copy the values to where
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dest_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dest_name", "rcDestination.name", "==");
    PXOPT_COPY_S64(config->args, where, "-target_id", "target_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "dist_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "LIKE");
    // if stage is all don't add it to the query (match all stages)
    if (stage && strcmp(stage, "all")) {
        PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    }

    psString query = pxDataGet("disttool_defineinterest.sql");

    if (!psListLength(where->list)) {
        // can't get here
        psError(PS_ERR_PROGRAMMING, true, "search parameters are required");
        psFree(where);
        psFree(query);
        return false;
    }
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
    psFree(where);
    if (clean) {
        psStringAppend(&query, " AND (distTarget.clean)");
    } else {
        psStringAppend(&query, " AND (!distTarget.clean)");
    }
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }
    {
        // psStringSubstitute fails unless the input is a psString which it determines by
        // comparing the memory blocks free function to an expected value.
        // pxDataGet uses psSlurp which leaves a different free function on the memory block.
        // To work around this make a copy of the query before doing the substitution.
        psString queryCopy = psStringCopy(query);
        psFree(query);
        query = queryCopy;
    }
    // change the @STATE@ in the sql file to our state
    if (!psStringSubstitute(&query, state, "@STATE@")) {
        psError(PS_ERR_UNKNOWN, false, "failed to substitute state string");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);
    int numInserted = psDBAffectedRows(config->dbh);
    printf("inserted %d rows into rcInterest\n", numInserted);

    return true;
}

static bool updateinterestMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-int_id",    "int_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id",   "dest_id", "==");
    PXOPT_COPY_S64(config->args, where, "-target_id", "target_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dest_name", "rcDestination.name", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "dist_group", "LIKE");

    PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
    PXOPT_LOOKUP_BOOL(full, config->args, "-full", false);
    if (full && clean) {
        psError(PS_ERR_UNKNOWN, true, "-full and -clean makes no sense, chose one or the other");
        psFree(where);
        return false;
    }


    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    if (!state) {
        psError(PS_ERR_UNKNOWN, true, "-set_state is required");
        psFree(where);
        return false;
    }
    psString query = pxDataGet("disttool_updateinterest.sql");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        psFree(where);
        psFree(query);
        return false;
    }
    psFree(where);

    if (clean) {
        psStringAppend(&query, " AND distTarget.clean");
    } else {
        psStringAppend(&query, " AND NOT distTarget.clean");
    }

    if (!p_psDBRunQueryF(config->dbh, query, state)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psS64 numUpdated = psDBAffectedRows(config->dbh);
    printf("updated %" PRId64 " interests\n", numUpdated);

    return true;
}
static bool listinterestsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-int_id", "int_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dest_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dest_name", "name", "==");
    PXOPT_COPY_S64(config->args, where, "-target_id", "target_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "dist_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-state", "rcInterest.state", "==");

    PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
    PXOPT_LOOKUP_BOOL(full, config->args, "-full", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (clean && full) {
        psError(PS_ERR_UNKNOWN, false, "can't select both -clean and -full");
        return false;
    }

    psString query = pxDataGet("disttool_listinterests.sql");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
        if (clean) {
            psStringAppend(&query, " AND (clean)");
        } else if (full) {
            psStringAppend(&query, " AND (!clean)");
        }
    } else if (clean) {
        psStringAppend(&query, " WHERE clean");
    } else if (full) {
        psStringAppend(&query, " WHERE !clean");
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
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "rcInterest", !simple)) {
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
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    pxAddLabelSearchArgs (config, where, "-label", "distRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    psString query = pxDataGet("disttool_pendingcleanup.sql");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
        psFree(where);
    } else if (!all) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters or -all are required");
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
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "distToCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool listfilesetsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");
    PXOPT_COPY_S64(config->args, where, "-int_id", "int_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dest_id", "dest_id", "==");
    PXOPT_COPY_STR(config->args, where, "-dest_name", "name", "==");
    PXOPT_COPY_S64(config->args, where, "-target_id", "target_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "dist_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");

    // PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
    // PXOPT_LOOKUP_BOOL(full, config->args, "-full", false);

    pxAddLabelSearchArgs (config, where, "-label", "distRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("disttool_listfilesets.sql");

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
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("disttool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "distFilesets", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool updatefilesetMode(pxConfig *config)
{
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-fs_id", "fs_id", "==");
    PXOPT_COPY_S64(config->args, where, "-dist_id", "dist_id", "==");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);

    // We don't use PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false); here
    // because we want -fault 0 to work
    bool gotFault = false;
    psS16 fault = psMetadataLookupS16(&gotFault, config->args, "-fault");

    if ((!state) && (!gotFault)) {
        psError(PXTOOLS_ERR_CONFIG, true, "parameters (-fault or -set_state) is required");
        psFree(where);
        return false;
    }

    psString query = psStringCopy("UPDATE rcDSFileset SET ");

    if (state) {
        psStringAppend(&query, " state = '%s'", state);
    }

    if (gotFault) {
        psStringAppend(&query, "%s fault = %d", state ? ", " : "", fault);
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

    return true;
}

/*
 * magicdstool.c
 *
 * Copyright (C) 2006-2009  IfA
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
#include <ippStages.h>

#include "pxtools.h"
#include "magicdstool.h"

static bool definebyqueryMode(pxConfig *config);
static bool definecopyMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool todestreakMode(pxConfig *config);
static bool adddestreakedfileMode(pxConfig *config);
static bool advancerunMode(pxConfig *config);
static bool revertdestreakedfileMode(pxConfig *config);
static bool updatedestreakedfileMode(pxConfig *config);
static bool clearstatefaultsMode(pxConfig *config);
static bool getskycellsMode(pxConfig *config);
static bool toremoveMode(pxConfig *config);
static bool torevertMode(pxConfig *config);
static bool completedrevertMode(pxConfig *config);
static bool tocleanupMode(pxConfig *config);
static bool tofullfileMode(pxConfig *config);
static bool tocleanedfileMode(pxConfig *config);
static bool setfiletoupdateMode(pxConfig *config);
static bool destreakedfileMode(pxConfig *config);
static bool listrunMode(pxConfig *config);

static bool setmagicDSRunState(pxConfig *config, psS64 magic_id, psString extraSetString, psMetadata *where, const char *state);
static bool validDSRunState(const char *state);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = magicdstoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(MAGICDSTOOL_MODE_DEFINEBYQUERY,       definebyqueryMode);
        MODECASE(MAGICDSTOOL_MODE_DEFINECOPY,          definecopyMode);
        MODECASE(MAGICDSTOOL_MODE_UPDATERUN,           updaterunMode);
        MODECASE(MAGICDSTOOL_MODE_TODESTREAK,          todestreakMode);
        MODECASE(MAGICDSTOOL_MODE_ADDDESTREAKEDFILE,   adddestreakedfileMode);
        MODECASE(MAGICDSTOOL_MODE_ADVANCERUN,          advancerunMode);
        MODECASE(MAGICDSTOOL_MODE_REVERTDESTREAKEDFILE,revertdestreakedfileMode);
        MODECASE(MAGICDSTOOL_MODE_UPDATEDESTREAKEDFILE,updatedestreakedfileMode);
        MODECASE(MAGICDSTOOL_MODE_CLEARSTATEFAULTS,    clearstatefaultsMode);
        MODECASE(MAGICDSTOOL_MODE_GETSKYCELLS,         getskycellsMode);
        MODECASE(MAGICDSTOOL_MODE_TOREMOVE,            toremoveMode);
        MODECASE(MAGICDSTOOL_MODE_TOREVERT,            torevertMode);
        MODECASE(MAGICDSTOOL_MODE_COMPLETEDREVERT,     completedrevertMode);
        MODECASE(MAGICDSTOOL_MODE_TOCLEANUP,           tocleanupMode);
        MODECASE(MAGICDSTOOL_MODE_TOFULLFILE,          tofullfileMode);
        MODECASE(MAGICDSTOOL_MODE_TOCLEANEDFILE,       tocleanedfileMode);
        MODECASE(MAGICDSTOOL_MODE_SETFILETOUPDATE,     setfiletoupdateMode);
        MODECASE(MAGICDSTOOL_MODE_DESTREAKEDFILE,      destreakedfileMode);
        MODECASE(MAGICDSTOOL_MODE_LISTRUN,             listrunMode);
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

    // Required
    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);

    // Optional
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", false, false);
    PXOPT_LOOKUP_STR(recoveryroot, config->args, "-recoveryroot", false, false);
    PXOPT_LOOKUP_BOOL(noreplace, config->args, "-noreplace", false);
    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(set_data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // search args
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-exp_id",  "exp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chip_bg_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",  "cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warp_bg_id", "==");
    PXOPT_COPY_S64(config->args, where, "-diff_id", "diff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id","magicRun.magic_id", "==");
    PXOPT_COPY_S32(config->args, where, "-streaks_max","streaks", "<=");

    pxAddLabelSearchArgs (config, where, "-label", "magicRun.label", "=="); // define using magicRun label

    ippStage stageNum = ippStringToStage(stage);

    psString query = NULL;
    switch (stageNum) {
    case IPP_STAGE_RAW:
        query = pxDataGet("magicdstool_definebyquery_raw.sql");
        break;
    case IPP_STAGE_CHIP:
        query = pxDataGet("magicdstool_definebyquery_chip.sql");
        break;
    case IPP_STAGE_CHIP_BG:
        query = pxDataGet("magicdstool_definebyquery_chip_bg.sql");
        break;
    case IPP_STAGE_CAMERA:
        query = pxDataGet("magicdstool_definebyquery_camera.sql");
        break;
    case IPP_STAGE_WARP:
        query = pxDataGet("magicdstool_definebyquery_warp.sql");
        break;
    case IPP_STAGE_WARP_BG:
        query = pxDataGet("magicdstool_definebyquery_warp_bg.sql");
        break;
    case IPP_STAGE_DIFF:
        query = pxDataGet("magicdstool_definebyquery_diff.sql");
        break;
    case IPP_STAGE_FAKE:
    case IPP_STAGE_STACK:
        psError(PXTOOLS_ERR_CONFIG, true, "%sRuns do not need to be destreaked", stage);
        return false;
    case IPP_STAGE_NONE:
        psError(PXTOOLS_ERR_CONFIG, true, "%s is not a valid stage", stage);
        return false;
    default:
        psError(PXTOOLS_ERR_PROG, true, "ippStageToString returned %d for invalid stage %s",
            stageNum, stage);
        return false;
    }

    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    char *rerun_flag =  rerun ? "\n1 " : "\n0" ;

    if (stageNum != IPP_STAGE_DIFF) {
        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, "\nAND %s", whereClause);
            psFree(whereClause);
        }
        psFree(where);

        // treat limit == 0 as "no limit"
        if (limit) {
            psString limitString = psDBGenerateLimitSQL(limit);
            psStringAppend(&query, " %s", limitString);
            psFree(limitString);
        }
        if (!p_psDBRunQueryF(config->dbh, query, rerun_flag)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
    } else {
        // diff stage query has two types bothways and !bothways
        // so we need to send the rerun flag and the where data twice
        psString whereString = psStringCopy("");
        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&whereString, "\nAND %s", whereClause);
            psFree(whereClause);
        }
        psFree(where);

        // treat limit == 0 as "no limit"
        if (limit) {
            psString limitString = psDBGenerateLimitSQL(limit);
            psStringAppend(&query, " %s", limitString);
            psFree(limitString);
        }

        if (!p_psDBRunQueryF(config->dbh, query, rerun_flag, whereString, rerun_flag, whereString)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(whereString);
            psFree(query);
            return false;
        }
        psFree(whereString);
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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // Parse the list of runs ready to be destreaked

    if (!pretend && !psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *list = psArrayAllocEmpty(16); // List of runs, to print
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i]; // Row of interest
        psS64 stage_id = psMetadataLookupS64(NULL, row, "stage_id");
        psS64 exp_id = psMetadataLookupS64(NULL, row, "exp_id");
        psS64 magic_id = psMetadataLookupS64(NULL, row, "magic_id");
        psS64 inv_magic_id = psMetadataLookupS64(NULL, row, "inv_magic_id");
        psS64 cam_id = psMetadataLookupS64(NULL, row, "cam_id");
        psString magicRunLabel = psMetadataLookupStr(NULL, row, "label");
        psString magicRunDataGroup = psMetadataLookupStr(NULL, row, "data_group");
        psString magicRunWorkdir = psMetadataLookupStr(NULL, row, "workdir");

        // if workdir is not supplied use the magicRun's
        if (!workdir) {
            workdir = magicRunWorkdir;
        }

        psString outroot = NULL;
        // set outroot to workdir/exp_id/stage for example /somewhere/424242/chip
        psStringAppend(&outroot, "%s/%" PRId64 "/%s", workdir, exp_id, stage);

        // create a new magicRun for this group
        magicDSRunRow *run = magicDSRunRowAlloc(
                0, // magic_ds_id
                magic_id,
                inv_magic_id,
                "new",
                stage,
                stage_id,
                cam_id,
                set_label ? set_label : magicRunLabel,
                set_data_group ? set_data_group : magicRunDataGroup,
                outroot,
                recoveryroot,
                noreplace ? 0 :1,   // re_place
                0,      // remove
                0,      // fault
                note);  // remove

        psFree(outroot);
        if (!run) {
            psAbort("failed to alloc magicDSRun object");
        }

        if (!pretend) {
            if (!magicDSRunInsertObject(config->dbh, run)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(run);
                psFree(output);
                psFree(list);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            psS64 magic_ds_id = psDBLastInsertID(config->dbh); // Assigned identifier
            run->magic_ds_id = magic_ds_id;
        }

        psArrayAdd(list, list->n, run);
        psFree(run);
    }

    if (!pretend && !psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(output);

    if (!magicDSRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }

    psFree(list);

    return true;
}


// XXX This currently allows multiple destreak runs to be queued on the same exposure if there are multiple
// magicRuns selected!
static bool definecopyMode(pxConfig *config)
{
    // Required
    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false);
    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", true, false);

    // Optional
    PXOPT_LOOKUP_STR(recoveryroot, config->args, "-recoveryroot", false, false);
    PXOPT_LOOKUP_BOOL(noreplace, config->args, "-noreplace", false);
    PXOPT_LOOKUP_STR(set_data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // search args
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-exp_id",  "chipRun.exp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chipRun.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",  "camRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warpRun.warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-diff_id", "diffRun.diff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");
    PXOPT_COPY_S32(config->args, where, "-streaks_max", "magicMask.streaks", "<=");

    pxAddLabelSearchArgs (config, where, "-magic_label", "magicRun.label", "=="); // define magic label
    psString labelName = NULL;                                                    // Name of label
    if (strcmp(stage, "camera")) {
        psStringAppend(&labelName, "%sRun.label", stage);
    } else {
        psStringAppend(&labelName, "camRun.label");
    }
    pxAddLabelSearchArgs (config, where, "-stage_label", labelName, "=="); // define stageRun label
    psFree(labelName);

    ippStage stageNum = ippStringToStage(stage);

    psString query = NULL;
    switch (stageNum) {
      case IPP_STAGE_RAW:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Raw stage is not appropriate for a copied destreak");
        return false;
      case IPP_STAGE_CHIP:
        query = pxDataGet("magicdstool_definecopy_chip.sql");
        break;
      case IPP_STAGE_WARP:
        query = pxDataGet("magicdstool_definecopy_warp.sql");
        break;
      case IPP_STAGE_CAMERA:
        query = pxDataGet("magicdstool_definecopy_camera.sql");
        break;
      case IPP_STAGE_DIFF:
      case IPP_STAGE_FAKE:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "%s has not been coded.", stage);
        return false;
      case IPP_STAGE_STACK:
        psError(PXTOOLS_ERR_CONFIG, true, "Stacks do not need to be destreaked");
        return false;
      case IPP_STAGE_NONE:
        psError(PXTOOLS_ERR_CONFIG, true, "%s is not a valid stage", stage);
        return false;
      default:
        psError(PXTOOLS_ERR_PROG, true, "ippStageToString returned %d for invalid stage %s",
                stageNum, stage);
        return false;
    }

    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    const char *rerun_flag =  rerun ? "" : "\n"; // String to give query to activate (or not) rerun

    if (stageNum != IPP_STAGE_DIFF) {
        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, "\nAND %s", whereClause);
            psFree(whereClause);
        }
        psFree(where);

        // treat limit == 0 as "no limit"
        if (limit) {
            psString limitString = psDBGenerateLimitSQL(limit);
            psStringAppend(&query, " %s", limitString);
            psFree(limitString);
        }
        if (!p_psDBRunQueryF(config->dbh, query, set_label, rerun_flag)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            return false;
        }
    } else {
        // diff stage query has two types bothways and !bothways
        // so we need to send the rerun flag and the where data twice
        psString whereString = psStringCopy("");
        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&whereString, "\nAND %s", whereClause);
            psFree(whereClause);
        }
        psFree(where);

        // treat limit == 0 as "no limit"
        if (limit) {
            psString limitString = psDBGenerateLimitSQL(limit);
            psStringAppend(&query, " %s", limitString);
            psFree(limitString);
        }

        if (!p_psDBRunQueryF(config->dbh, query, rerun_flag, whereString, rerun_flag, whereString)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(whereString);
            psFree(query);
            return false;
        }
        psFree(whereString);
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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // Parse the list of runs ready to be destreaked

    if (!pretend && !psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *list = psArrayAllocEmpty(16); // List of runs, to print
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i]; // Row of interest
        psS64 stage_id = psMetadataLookupS64(NULL, row, "stage_id");
        psS64 exp_id = psMetadataLookupS64(NULL, row, "exp_id");
        psS64 magic_id = psMetadataLookupS64(NULL, row, "magic_id");
        psS64 inv_magic_id = psMetadataLookupS64(NULL, row, "inv_magic_id");
        psS64 cam_id = psMetadataLookupS64(NULL, row, "cam_id");
        psString magicRunLabel = psMetadataLookupStr(NULL, row, "label");
        psString magicRunDataGroup = psMetadataLookupStr(NULL, row, "data_group");

        psString outroot = NULL;
        // set outroot to workdir/exp_id/stage for example /somewhere/424242/chip
        psStringAppend(&outroot, "%s/%" PRId64 "/%s", workdir, exp_id, stage);

        // create a new magicRun for this group
        magicDSRunRow *run = magicDSRunRowAlloc(
                0, // magic_ds_id
                magic_id,
                inv_magic_id,
                "new",
                stage,
                stage_id,
                cam_id,
                set_label ? set_label : magicRunLabel,
                set_data_group ? set_data_group : magicRunDataGroup,
                outroot,
                recoveryroot,
                noreplace ? 0 :1,   // re_place
                0,      // remove
                0,      // fault
                note);  // remove

        psFree(outroot);
        if (!run) {
            psAbort("failed to alloc magicDSRun object");
        }

        if (!pretend) {
            if (!magicDSRunInsertObject(config->dbh, run)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(run);
                psFree(output);
                psFree(list);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            psS64 magic_ds_id = psDBLastInsertID(config->dbh); // Assigned identifier
            run->magic_ds_id = magic_ds_id;
        }

        psArrayAdd(list, list->n, run);
        psFree(run);
    }

    if (!pretend && !psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(output);

    if (!magicDSRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }

    psFree(list);

    return true;
}


static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", true, false);

    if (!strcmp(state, "update")) {
        fprintf(stderr, "'-updaterun -set_state update' is not supported. Use -setfiletoupdate");
        return false;
    }

    // optional
    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(set_data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(set_note, config->args, "-set_note", false, false);
    psString setString = NULL;
    if (set_label) {
        psStringAppend(&setString, ", label = '%s'", set_label);
    }
    if (set_data_group) {
        psStringAppend(&setString, ", data_group = '%s'", set_data_group);
    }
    if (set_note) {
        psStringAppend(&setString, ", note = '%s'", set_note);
    }

    PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", false, false);
    if (magic_ds_id) {

        return setmagicDSRunState(config, magic_ds_id, setString, NULL, state);

    } else if (!strcmp(state, "full")) {
        psError(PS_ERR_UNKNOWN, true, "magic_ds_id is required to update run state to full");
        return false;
    }
    // we can transition by query as well

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-label", "label", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "data_group", "LIKE");

    if (psListLength(where->list) < 2) {
        psError(PS_ERR_UNKNOWN, true, "at least 2 search arguments are required");
        return false;
    }


    PXOPT_LOOKUP_BOOL(noreplace, config->args, "-noreplace", false);
    if (!noreplace) {
        psMetadataAddS32(where, PS_LIST_TAIL, "re_place", 0, ">", 0);
    }
    bool result = setmagicDSRunState(config, magic_ds_id, setString, where, state);
    psFree(where);

    return result;
}


static bool todestreakMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magicDSRun.magic_ds_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicDSRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString sql_file = NULL;
    psStringAppend(&sql_file, "magicdstool_todestreak_%s.sql", stage);

    psString query = pxDataGet(sql_file);
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement from %s", sql_file);
        psFree(sql_file);
        return false;
    }
    psFree(sql_file);

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, "\nORDER BY priority DESC, magic_ds_id");

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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "todestreak", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


// update the magicked column for the underlying component
// Note: Must be called inside a transaction

static bool
setMagicked(pxConfig *config, psS64 magic_ds_id, psString component, bool clearMagicked)
{
    // first query the magicDSRun to find the stage and the stage_id
    psString query = "SELECT stage, stage_id, magic_id from magicDSRun where magic_ds_id = %" PRId64;

    if (!p_psDBRunQueryF(config->dbh, query, magic_ds_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("magicdstool", PS_LOG_INFO, "magic_ds_id: %" PRId64 " not found", magic_ds_id);
        psFree(output);
        return true;
    }
    if (psArrayLength(output) > 1) {
        psError(PS_ERR_UNKNOWN, true, "unexpected number of rows found %ld for magic_ds_id %" PRId64,
            psArrayLength(output), magic_ds_id);
        psFree(output);
        return false;
    }
    psMetadata *row = output->data[0];

    psString stage= psMetadataLookupStr(NULL, row, "stage");
    psS64 stage_id = psMetadataLookupS64(NULL, row, "stage_id");
    psS64 magic_id = psMetadataLookupS64(NULL, row, "magic_id");
    psFree(output);

    psS64 newMagickedValue;
    if (clearMagicked) {
        newMagickedValue = 0;
    } else {
        newMagickedValue = magic_id;
    }

    ippStage stageNum = ippStringToStage(stage);

    // chose the appropriate query based on the stage
    char *clearRunQuery = NULL;
    switch (stageNum) {
    case IPP_STAGE_RAW:
        query = "UPDATE rawImfile SET magicked = %" PRId64 " where exp_id = %" PRId64 " AND class_id = '%s'";
        clearRunQuery = "UPDATE rawExp SET magicked = 0 where exp_id = %" PRId64;
        break;
    case IPP_STAGE_CHIP:
        query = "UPDATE chipProcessedImfile SET magicked = %" PRId64 " where chip_id = %" PRId64 " AND class_id = '%s'";
        clearRunQuery = "UPDATE chipRun set magicked = 0 where chip_id = %" PRId64;
        break;
    case IPP_STAGE_CHIP_BG:
        query = "UPDATE chipBackgroundImfile SET magicked = %" PRId64 " where chip_bg_id = %" PRId64 " AND class_id = '%s'";
        clearRunQuery = "UPDATE chipBackgroundRun SET magicked = 0 where chip_bg_id = %" PRId64;
        break;
    case IPP_STAGE_CAMERA:
        query = NULL;
        clearRunQuery = "UPDATE camRun SET magicked = 0 where cam_id = %" PRId64;
        break;
    case IPP_STAGE_WARP:
        query = "UPDATE warpSkyfile SET magicked = %" PRId64 " where warp_id = %" PRId64 " AND skycell_id = '%s'";
        clearRunQuery = "UPDATE warpRun SET magicked = 0 where warp_id = %" PRId64;
        break;
    case IPP_STAGE_WARP_BG:
        query = "UPDATE warpBackgroundSkyfile SET magicked = %" PRId64 " where warp_bg_id = %" PRId64 " AND skycell_id = '%s'";
        clearRunQuery = "UPDATE warpBackgroundRun SET magicked = 0 where warp_bg_id = %" PRId64;
        break;
    case IPP_STAGE_DIFF:
        query = "UPDATE diffSkyfile SET magicked = %" PRId64 " where diff_id = %" PRId64 " AND skycell_id = '%s'";
        clearRunQuery = "UPDATE diffRun SET magicked = 0 where diff_id = %" PRId64;
        break;
    default:
        psError(PS_ERR_UNKNOWN, true, "unexpected value for stage: %s found", stage);
        return false;
    }

    if (query) {
        if (!p_psDBRunQueryF(config->dbh, query, newMagickedValue, stage_id, component)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }

    if (clearMagicked && clearRunQuery) {
        if (!p_psDBRunQueryF(config->dbh, clearRunQuery, stage_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }

    return true;
}

static bool
setRunMagicked(pxConfig *config, psS64 magic_ds_id)
{
    // first query the magicDSRun to find the stage and the stage_id
    psString query = "SELECT stage, stage_id, magic_id from magicDSRun where magic_ds_id = %" PRId64;

    if (!p_psDBRunQueryF(config->dbh, query, magic_ds_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psError(PS_ERR_UNKNOWN, true, "magicDSRun not found for magic_ds_id %" PRId64, magic_ds_id);
        psFree(output);
        return false;
    }
    if (psArrayLength(output) > 1) {
        psError(PS_ERR_UNKNOWN, true, "unexpected number of rows found %ld for magic_ds_id %" PRId64,
            psArrayLength(output), magic_ds_id);
        psFree(output);
        return false;
    }
    psMetadata *row = output->data[0];

    psString stage = psMetadataLookupStr(NULL, row, "stage");
    psS64 stage_id = psMetadataLookupS64(NULL, row, "stage_id");
    psS64 magic_id = psMetadataLookupS64(NULL, row, "magic_id");

    ippStage stageNum = ippStringToStage(stage);

    // chose the appropriate query based on the stage
    switch (stageNum) {
    case IPP_STAGE_RAW:
        query = "UPDATE rawExp SET magicked = %" PRId64 " where exp_id = %" PRId64;
        break;
    case IPP_STAGE_CHIP:
        query = "UPDATE chipRun SET magicked = %" PRId64 " where chip_id = %" PRId64;
        break;
    case IPP_STAGE_CHIP_BG:
        query = "UPDATE chipBackgroundRun SET magicked = %" PRId64 " where chip_bg_id = %" PRId64;
        break;
    case IPP_STAGE_CAMERA:
        query = "UPDATE camRun SET magicked = %" PRId64 " where cam_id = %" PRId64;
        break;
    case IPP_STAGE_WARP:
        query = "UPDATE warpRun SET magicked = %" PRId64 " where warp_id = %" PRId64;
        break;
    case IPP_STAGE_WARP_BG:
        query = "UPDATE warpBackgroundRun SET magicked = %" PRId64 " where warp_bg_id = %" PRId64;
        break;
    case IPP_STAGE_DIFF:
        query = "UPDATE diffRun SET magicked = %" PRId64 " where diff_id = %" PRId64;
        break;
    default:
        psError(PS_ERR_UNKNOWN, true, "unexpected value for stage: %s found", stage);
        psFree(output);
        return false;
    }
    if (!p_psDBRunQueryF(config->dbh, query, magic_id, stage_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(output);

    psU64 affected = psDBAffectedRows(config->dbh);
    if (affected != 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected 1 row");
        return false;
    }

    return true;
}

static bool adddestreakedfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required values
    PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", true, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_STR(backup_path_base, config->args, "-backup_path_base", false, false);
    PXOPT_LOOKUP_STR(recovery_path_base, config->args, "-recovery_path_base", false, false);
    PXOPT_LOOKUP_BOOL(setmagicked, config->args, "-setmagicked", false);
    PXOPT_LOOKUP_F32(streak_frac, config->args, "-streak_frac", false, false);
    PXOPT_LOOKUP_F32(nondiff_frac, config->args, "-nondiff_frac", false, false);
    PXOPT_LOOKUP_F32(run_time, config->args, "-run_time", false, false);

    if (setmagicked && (fault != 0)) {
        psError(PS_ERR_UNKNOWN, true, " cannot setmagicked for faulted file");
        return false;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (setmagicked) {
        // set the image file's magicked flag
        if (!setMagicked(config, magic_ds_id, component, false)) {
            psError(PS_ERR_UNKNOWN, false, "setMagicked failed");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
    }

    if (!magicDSFileInsert(config->dbh,
            magic_ds_id,
            component,
            backup_path_base,
            recovery_path_base,
            streak_frac,
            nondiff_frac,
            run_time,
            fault,
            "full"  // data_state
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
static bool updatedestreakedfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required values
    PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", true, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_STR(backup_path_base, config->args, "-backup_path_base", false, false);
    PXOPT_LOOKUP_STR(recovery_path_base, config->args, "-recovery_path_base", false, false);
    PXOPT_LOOKUP_STR(data_state, config->args, "-data_state", false, false);
    PXOPT_LOOKUP_F32(streak_frac, config->args, "-streak_frac", false, false);
    PXOPT_LOOKUP_F32(nondiff_frac, config->args, "-nondiff_frac", false, false);
    PXOPT_LOOKUP_F32(run_time, config->args, "-run_time", false, false);

    psString query = psStringCopy("UPDATE magicDSFile");


    char *initial_separator = " SET";
    char *sep = initial_separator;
    if (fault) {
        psStringAppend(&query, "%s fault = %d", sep, fault);
        sep = ", ";
    }
    if (backup_path_base) {
        psStringAppend(&query, "%s backup_path_base = '%s'", sep, backup_path_base);
        sep = ", ";
    }
    if (recovery_path_base) {
        psStringAppend(&query, "%s recovery_path_base = '%s'", sep, recovery_path_base);
        sep = ", ";
    }
    if (data_state) {
        psStringAppend(&query, "%s data_state = '%s'", sep, data_state);
        sep = ", ";
    }
    if (streak_frac) {
        psStringAppend(&query, "%s streak_frac = '%f'", sep, streak_frac);
        sep = ", ";
    }
    if (nondiff_frac) {
        psStringAppend(&query, "%s nondiff_frac = '%f'", sep, nondiff_frac);
        sep = ", ";
    }
    if (run_time) {
        psStringAppend(&query, "%s run_time = '%f'", sep, run_time);
        sep = ", ";
    }
    if (sep == initial_separator) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, true, "must set at least one value");
        return false;
    }

    psStringAppend(&query, "\nWHERE magic_ds_id = %" PRId64 " AND component = '%s'\n", magic_ds_id, component);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}


static bool advancerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magicDSRun.magic_ds_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicDSRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // look for completed magicDSRuns
    psString query = pxDataGet("magicdstool_completed_runs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

        psS64 magic_ds_id = psMetadataLookupS64(NULL, row, "magic_ds_id");
        psS64 magicked =  psMetadataLookupS64(NULL, row, "magicked");
        if (!psDBTransaction(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        // if re_place, set stageRun.magicked
        bool setmagicked = psMetadataLookupBool(NULL, row, "re_place");
        if (setmagicked) {
            if (magicked <= 0) { 
                if (!setRunMagicked(config, magic_ds_id)) {
                    psError(PS_ERR_UNKNOWN, false, "failed to change stageRun.magicked for magic_ds_id: %" PRId64,
                        magic_ds_id);
                    if (!psDBRollback(config->dbh)) {
                        psError(PS_ERR_UNKNOWN, false, "database error");
                    }
                    return false;
                }
            } else {
                fprintf(stderr, "run is already marked as destreaked for magic_ds_id %" PRId64 "\n", magic_ds_id);
            }
        }

        // set magicDSRun.state to 'full'
        if (!setmagicDSRunState(config, magic_ds_id, NULL, NULL, "full")) {
            psError(PS_ERR_UNKNOWN, false, "failed to change magicDSRun.state for magic_ds_id: %" PRId64,
                magic_ds_id);
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }

    return true;
}


static bool revertdestreakedfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(i_am_sure, config->args, "-i_am_sure", true);
    if (!i_am_sure) {
        psError(PS_ERR_UNKNOWN, true, "Reverting destreaked files must be done carefully. -i_am_sure is required.");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magicDSRun.magic_ds_id", "==");
    PXOPT_COPY_STR(config->args, where, "-component", "component", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", true, false);
    PXOPT_LOOKUP_STR(state, config->args, "-state", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    psString queryFile = NULL;
    bool stateIsUpdate = false;
    bool toRestored = !strcmp(state, "goto_restored");
    if (state) {
        if (! strcmp(state, "new") || toRestored) {
            queryFile = "magicdstool_revertdestreakedfile.sql";
        } else if (!strcmp(state, "update")) {
            queryFile = "magicdstool_revertupdated.sql";
            stateIsUpdate = true;
        } else {
            psError(PXTOOLS_ERR_SYS, true, "%s is not a valid value for state", state);
            return false;
        }
    } else {
        queryFile = "magicdstool_revertdestreakedfile.sql";
    }
    psString query = pxDataGet(queryFile);
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search arguments are required");
        return false;
    }
    psFree(where);

    if (stateIsUpdate && !fault) {
        // If fault has not been supplied, don't revert update faults with
        // the magic "do not update" value
        // We don't do this for new runs because then they would never complete
        // quality should be used to drop bad components
        psStringAppend(&query, " AND magicDSFile.fault != %d", PXTOOL_DO_NOT_REVERT_FAULT);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "failed to revert");
        psFree(query);
        return false;
    }
    psFree(query);
    if (toRestored) {
        // clear the underlying component's magicked value
        if (!setMagicked(config, magic_ds_id, component, true)) {
            psError(PS_ERR_UNKNOWN, false, "setMagicked failed");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
    }
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    return true;
}
static bool clearstatefaultsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    // new state
    PXOPT_LOOKUP_STR(new_state, config->args, "-set_state", false, false);
    // old state (required)
    PXOPT_LOOKUP_STR(state, config->args, "-state", true, false);

    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magicDSRun.magic_ds_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    psString query = pxDataGet("magicdstool_clearstatefaults.sql");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search arguments are required");
        return false;
    }
    psFree(where);

    if (!new_state) {
        if (!strcmp(state, "failed_revert")) {
            new_state = "new";
        } else if (!strcmp(state, "failed_revert_ud")) {
            new_state = "update";
        } else if (!strcmp(state, "failed_cleanup")) {
            new_state = "goto_cleaned";
        } else if (!strcmp(state, "failed_restore")) {
            new_state = "goto_restored";
        } else {
            psError(PS_ERR_UNKNOWN, true, "unexpected value for state: %s", state);
            return false;
        }
    } else {
        if (!validDSRunState(new_state)) {
            psError(PS_ERR_UNKNOWN, true, "unexpected value for new state: %s", new_state);
            return false;
        }
    }
    if (!p_psDBRunQueryF(config->dbh, query, new_state)) {
        psError(PS_ERR_UNKNOWN, false, "failed to clear state faults");
        psFree(query);
        return false;
    }
    psFree(query);
    return true;
}

static bool completedrevertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magicDSRun.magic_ds_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = pxDataGet("magicdstool_completedrevert.sql");
    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    psString whereString = NULL;
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereString, "\nAND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, whereString ? whereString : "")) {
        psFree(whereString);
        psError(PS_ERR_UNKNOWN, false, "failed to revert");
        return false;
    }
    psFree(whereString);
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    for (int i=0; i<psArrayLength(output); i++) {
        psMetadata *row = output->data[i];
        psS64 magic_ds_id = psMetadataLookupS64(NULL, row, "magic_ds_id");
        psString old_state = psMetadataLookupStr(NULL, row, "state");
        psString new_state;
        if (!strcmp(old_state, "goto_censored")) {
            new_state = "censored";
        } else if (!strcmp(old_state, "goto_restored")) {
            new_state = "restored";
        } else {
            psError(PXTOOLS_ERR_PROG, true, "unexpected state found: %s", old_state);
            psFree(output);
            return false;
        }
        char *query2 = "UPDATE magicDSRun SET state = '%s' WHERE magic_ds_id = %" PRId64;
        if (!p_psDBRunQueryF(config->dbh, query2, new_state, magic_ds_id)) {
            psError(PS_ERR_UNKNOWN, false, "failed to set run magicDSRun.state to %s", new_state);
            return false;
        }
    }
    psFree(output);

    return true;
}

static bool getskycellsMode(pxConfig *config)
{
    // required
    // PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", true, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magicDSRun.magic_ds_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id",    "warpSkyCellMap.class_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",  "warpSkyCellMap.skycell_id", "==");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("magicdstool_getskycells.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = NULL;
    if (psListLength(where->list)) {
        whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringPrepend(&whereClause, "\n AND ");
    }
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, whereClause, whereClause)) {
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "magicDiffSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool validDSRunState(const char *state)
{
    if (!((strcmp(state, "new") == 0) ||
          (strcmp(state, "full") == 0) ||
          (strcmp(state, "drop") == 0) ||
          (strcmp(state, "wait") == 0) ||
          (strcmp(state, "failed_restore") == 0) ||
          (strcmp(state, "failed_revert") == 0) ||
          (strcmp(state, "failed_revert_ud") == 0) ||
          (strcmp(state, "error_cleaned") == 0) ||
          (strcmp(state, "restored") == 0) ||
          (strcmp(state, "censored") == 0) ||
          (strcmp(state, "cleaned") == 0) ||
          (strcmp(state, "update") == 0) ||
          (strcmp(state, "goto_restored") == 0) ||
          (strcmp(state, "goto_censored") == 0) ||
          (strcmp(state, "goto_cleaned") == 0))
        ) {
        return false;
    } else {
        return true;
    }
}

static bool setmagicDSRunState(pxConfig *config, psS64 magic_ds_id, psString extraSetStr, psMetadata *where, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    if (!validDSRunState(state)) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid magicDSRun state: %s", state);
        return false;
    }

    psString query = NULL;
    psStringAppend(&query, "UPDATE magicDSRun SET state = '%s' %s\n", state, extraSetStr ? extraSetStr : "");
    if (magic_ds_id) {
        psStringAppend(&query, " WHERE magic_ds_id = %" PRId64, magic_ds_id);
    } else if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search arguments are required");
        return false;
    }

    if (!strcmp(state, "goto_cleaned")) {
        // Don't set state back to goto_cleaned if it is already cleaned
        psStringAppend(&query, " AND (magicDSRun.state != 'cleaned')");

        // don't clean up magicDSRun's where stage is camera
        psStringAppend(&query, " AND (magicDSRun.stage != 'camera')");
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for magic_id %" PRId64, magic_ds_id);
        return false;
    }

    return true;
}

static bool toremoveMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magic_ds_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("magicdstool_toremove.sql");
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "toremove", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool torevertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);

    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magic_ds_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicDSRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString sql_file = NULL;
    psStringAppend(&sql_file, "magicdstool_torevert_%s.sql", stage);

    psString query = pxDataGet(sql_file);
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement from %s", sql_file);
        psFree(sql_file);
        return false;
    }
    psFree(sql_file);

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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "torevert", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool tocleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magic_ds_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicDSRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("magicdstool_tocleanup.sql");
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "tocleanup", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

// update magicDSFile.data_state to given value.
// afterwards, if all files in the magicDSRun have the new state, 
// update the state for it as well
// shared code for the modes -tocleanedfile -tofullfile

static bool change_file_data_state(pxConfig *config, psString data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // magic_id, component
    PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", true, false);

    psString query = pxDataGet("magicdstool_change_file_data_state.sql");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!strcmp(data_state, "full")) {
        // set the component's magicked flag
        
        if (!setMagicked(config, magic_ds_id, component, false)) {
            psError(PS_ERR_UNKNOWN, false, "setMagicked failed");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state, magic_ds_id, component)) {
        psFree(query);
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

    query = pxDataGet("magicdstool_change_run_state.sql");
    if (!p_psDBRunQueryF(config->dbh, query, data_state, magic_ds_id, data_state)) {
        psFree(query);
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}
static bool tocleanedfileMode(pxConfig *config)
{
    return change_file_data_state(config, "cleaned");
}
static bool tofullfileMode(pxConfig *config)
{
    return change_file_data_state(config, "full");
}

// a very specfic function to queue a cleaned magicDSFile to be updated
static bool setfiletoupdateMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(magic_ds_id, config->args, "-magic_ds_id", true, false);
    PXOPT_LOOKUP_STR(component, config->args, "-component", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(recoveryroot, config->args, "-set_recoveryroot", false, false);

    psString query = pxDataGet("magicdstool_setfiletoupdate.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString setHook = psStringCopy("");
    if (label) {
        psStringAppend(&setHook, "\n , magicDSRun.label = '%s'", label);
    }
    if (recoveryroot) {
        psStringAppend(&setHook, "\n , magicDSRun.recoveryroot = '%s'", recoveryroot);
    }

    if (component) {
        psStringAppend(&query, " AND (magicDSFile.component = '%s')", component);
    }

    // we do not update components with the magic fault value. They are non-updateable
    // (But can be recovered with "magicdstool -revertdestreakedfile -fault 26" (PXTOOL_DO_NOT_REVERT_FAULT)
    psStringAppend(&query, " AND (magicDSFile.fault != %d)", PXTOOL_DO_NOT_REVERT_FAULT);

    if (!p_psDBRunQueryF(config->dbh, query, setHook, magic_ds_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(setHook);
    psFree(query);

    return true;
}


static bool destreakedfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magic_ds_id", "==");
    PXOPT_COPY_STR(config->args, where, "-component", "component", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicDSRun.label", "==");
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);


    psString query = pxDataGet("magicdstool_destreakedfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search arguments are required");
        return false;
    }
    psFree(where);

    if (faulted) {
        psStringAppend(&query, " AND magicDSFile.fault > 0");
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "magicDSFile", !simple)) {
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

    PXOPT_COPY_S64(config->args, where, "-magic_ds_id", "magic_ds_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
    PXOPT_COPY_STR(config->args, where, "-stage", "stage", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicDSRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);


    psString query = pxDataGet("magicdstool_listrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search arguments are required");
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
        psTrace("magicdstool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "magicDSRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

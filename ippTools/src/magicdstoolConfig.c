/*
 * magicdstoolConfig.c
 *
 * Copyright (C) 2007  IfA
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

#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "magicdstool.h"

pxConfig *magicdstoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    psTime *now = psTimeGetNow(PS_TIME_TAI);

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-stage",       0, "define stage (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-workdir",     0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-recoveryroot", 0, "define recovery directory", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-noreplace", 0, "do not replace input files with the destreaked versions", false);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",    0, "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);

    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by magicRun.label", NULL);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-exp_id",   0, "search by exp_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-chip_id",  0, "search by chip_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-chip_bg_id",  0, "search by chip_bg_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-cam_id",  0, "search by cam_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-warp_id",  0, "search by warp_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-warp_bg_id",  0, "search by warp_bg_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-diff_id",  0, "search by diff_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic_id", 0);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-streaks_max", 0, "maximum number of streaks", 0);

    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-rerun",   0, "queue a new run even if one already exists", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend", 0, "don't queue runs just display what would be queued", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddU64(definebyqueryArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -definecopy
    psMetadata *definecopyArgs = psMetadataAlloc();
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-stage",       0, "define stage (required)", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-workdir",     0, "define workdir (required)", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-recoveryroot", 0, "define recovery directory", NULL);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-noreplace", 0, "do not replace input files with the destreaked versions", false);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_label",    0, "define label (required)", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data_group", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-stage_label", PS_META_DUPLICATE_OK, "search by stageRun.label", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-magic_label", PS_META_DUPLICATE_OK, "search by magicRun.label", NULL);
    psMetadataAddS64(definecopyArgs, PS_LIST_TAIL, "-exp_id",   0, "search by exp_id", 0);
    psMetadataAddS64(definecopyArgs, PS_LIST_TAIL, "-chip_id",  0, "search by chip_id", 0);
    psMetadataAddS64(definecopyArgs, PS_LIST_TAIL, "-cam_id",  0, "search by cam_id", 0);
    psMetadataAddS64(definecopyArgs, PS_LIST_TAIL, "-warp_id",  0, "search by warp_id", 0);
    psMetadataAddS64(definecopyArgs, PS_LIST_TAIL, "-diff_id",  0, "search by diff_id", 0);
    psMetadataAddS64(definecopyArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic_id", 0);
    psMetadataAddS32(definecopyArgs, PS_LIST_TAIL, "-streaks_max", 0, "maximum number of streaks", 0);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-rerun",   0, "queue a new run even if one already exists", false);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-pretend", 0, "don't queue runs just display what would be queued", false);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddU64(definecopyArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0, "set label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0, "set data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0, "set note", NULL);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magictool ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-stage",     0, "define stage", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",     0, "define state", NULL);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-stage_id", 0, "define stage_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",     0, "define label (LIKE) comparison", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group",     0, "data_group (LIKE) comparision", NULL);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-noreplace", 0, "only update runs with replace not set", false);

    // -addinputskyfile
    psMetadata *addinputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID (required)", 0);
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-diff_id", 0, "define difftool ID (required)", 0);
    psMetadataAddStr(addinputskyfileArgs, PS_LIST_TAIL, "-node", 0, "define symbolic node name (required)", NULL);

    // -todestreak
    psMetadata *todestreakArgs = psMetadataAlloc();
    psMetadataAddStr(todestreakArgs, PS_LIST_TAIL, "-stage", 0, "limit query to stage (required)", NULL);
    psMetadataAddS64(todestreakArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic Destreak ID", 0);
    psMetadataAddS64(todestreakArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddS64(todestreakArgs, PS_LIST_TAIL, "-stage_id", 0, "search by stage ID", 0);
    psMetadataAddStr(todestreakArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddU64(todestreakArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(todestreakArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -adddestreakedfile
    psMetadata *adddestreakedfileArgs = psMetadataAlloc();
    psMetadataAddS64(adddestreakedfileArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(adddestreakedfileArgs, PS_LIST_TAIL, "-component", 0, "define component name (required)", NULL);
    psMetadataAddStr(adddestreakedfileArgs, PS_LIST_TAIL, "-backup_path_base", 0, "define backup URI", NULL);
    psMetadataAddStr(adddestreakedfileArgs, PS_LIST_TAIL, "-recovery_path_base", 0, "define recovery pixels URI", NULL);
    psMetadataAddBool(adddestreakedfileArgs, PS_LIST_TAIL, "-setmagicked", 0, "update the magicked state of the file", false);
    psMetadataAddF32(adddestreakedfileArgs, PS_LIST_TAIL, "-streak_frac", 0, "set fraction of pixels masked by streaks", 0);
    psMetadataAddF32(adddestreakedfileArgs, PS_LIST_TAIL, "-nondiff_frac", 0, "set fraction of pixels masked because nondiffed", 0);
    psMetadataAddF32(adddestreakedfileArgs, PS_LIST_TAIL, "-run_time", 0, "set the streaksremove run time for component ", 0);
    psMetadataAddS16(adddestreakedfileArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -updatedestreakedfile
    psMetadata *updatedestreakedfileArgs = psMetadataAlloc();
    psMetadataAddS64(updatedestreakedfileArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(updatedestreakedfileArgs, PS_LIST_TAIL, "-component", 0, "define component name (required)", NULL);
    psMetadataAddStr(updatedestreakedfileArgs, PS_LIST_TAIL, "-state", 0, "current value for magicDSRun.state", NULL);
    psMetadataAddStr(updatedestreakedfileArgs, PS_LIST_TAIL, "-backup_path_base", 0, "define backup URI", NULL);
    psMetadataAddStr(updatedestreakedfileArgs, PS_LIST_TAIL, "-recovery_path_base", 0, "define recovery pixels URI", NULL);
    psMetadataAddStr(updatedestreakedfileArgs, PS_LIST_TAIL, "-data_state", 0, "change data_state", NULL);
    psMetadataAddBool(updatedestreakedfileArgs, PS_LIST_TAIL, "-setmagicked", 0, "update the magicked state of the file", false);
    psMetadataAddF32(updatedestreakedfileArgs, PS_LIST_TAIL, "-streak_frac", 0, "set fraction of pixels masked by streaks", 0);
    psMetadataAddF32(updatedestreakedfileArgs, PS_LIST_TAIL, "-nondiff_frac", 0, "set fraction of pixels masked because nondiffed", 0);
    psMetadataAddF32(updatedestreakedfileArgs, PS_LIST_TAIL, "-run_time", 0, "set the streaksremove run time for component ", 0);
    psMetadataAddS16(updatedestreakedfileArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -revertdestreakedfile
    psMetadata *revertdestreakedfileArgs = psMetadataAlloc();
    psMetadataAddS64(revertdestreakedfileArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magictool de-streak ID", 0);
    psMetadataAddStr(revertdestreakedfileArgs, PS_LIST_TAIL, "-state", 0, "current value for magicDSRun.state", NULL);
    psMetadataAddStr(revertdestreakedfileArgs, PS_LIST_TAIL, "-component", 0, "search by component", NULL);
    psMetadataAddS16(revertdestreakedfileArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddBool(revertdestreakedfileArgs, PS_LIST_TAIL, "-i_am_sure", 0, "confirm that you know what you are doing", false);
    psMetadataAddStr(revertdestreakedfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);

    // -clearstatefaults
    psMetadata *clearstatefaultsArgs = psMetadataAlloc();
    psMetadataAddStr(clearstatefaultsArgs, PS_LIST_TAIL, "-set_state", 0, "new value for state", NULL);
    psMetadataAddStr(clearstatefaultsArgs, PS_LIST_TAIL, "-state", 0, "search by state (required)", NULL);
    psMetadataAddS64(clearstatefaultsArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magictool de-streak ID", 0);
    psMetadataAddS16(clearstatefaultsArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddStr(clearstatefaultsArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);

    // -getskycells
    psMetadata *getskycellsArgs = psMetadataAlloc();
    psMetadataAddS64(getskycellsArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magic de-streak ID", 0);
    psMetadataAddStr(getskycellsArgs, PS_LIST_TAIL, "-class_id", 0, "define class identifier", NULL);
    psMetadataAddStr(getskycellsArgs, PS_LIST_TAIL, "-skycell_id", 0, "define skycell identifier", NULL);
    psMetadataAddBool(getskycellsArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -toremove
    psMetadata *toremoveArgs = psMetadataAlloc();
    psMetadataAddS64(toremoveArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic Destreak ID", 0);
    psMetadataAddS64(toremoveArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddStr(toremoveArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddU64(toremoveArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(toremoveArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -torevert
    psMetadata *torevertArgs = psMetadataAlloc();
    psMetadataAddS64(torevertArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic Destreak ID", 0);
    psMetadataAddS64(torevertArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddStr(torevertArgs, PS_LIST_TAIL, "-stage", 0, "define output directory (required)", NULL);
    psMetadataAddStr(torevertArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddU64(torevertArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(torevertArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -completedrevert
    psMetadata *completedrevertArgs = psMetadataAlloc();
    psMetadataAddS64(completedrevertArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic Destreak ID", 0);
    psMetadataAddStr(completedrevertArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddU64(completedrevertArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -advancerun
    psMetadata *advancerunArgs = psMetadataAlloc();
    psMetadataAddS64(advancerunArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic Destreak ID", 0);
    psMetadataAddStr(advancerunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddU64(advancerunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -tocleanup
    psMetadata *tocleanupArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanupArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic Destreak ID", 0);
    psMetadataAddS64(tocleanupArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddStr(tocleanupArgs, PS_LIST_TAIL, "-stage", 0, "define output directory", NULL);
    psMetadataAddStr(tocleanupArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddU64(tocleanupArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(tocleanupArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -tocleanedfile
    psMetadata *tocleanedfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedfileArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magic_ds_id", 0);
    psMetadataAddStr(tocleanedfileArgs, PS_LIST_TAIL, "-component", 0, "define component", NULL);

    // -tofullfile
    psMetadata *tofullfileArgs = psMetadataAlloc();
    psMetadataAddS64(tofullfileArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magic_ds_id", 0);
    psMetadataAddStr(tofullfileArgs, PS_LIST_TAIL, "-component", 0, "define component", NULL);
    psMetadataAddBool(tofullfileArgs, PS_LIST_TAIL, "-setmagicked", 0, "update the magicked state of the file", false);

    // -setfiletoupdate
    psMetadata *setfiletoupdateArgs = psMetadataAlloc();
    psMetadataAddS64(setfiletoupdateArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "define magic_ds_id", 0);
    psMetadataAddStr(setfiletoupdateArgs, PS_LIST_TAIL, "-component", 0, "define component", NULL);
    psMetadataAddStr(setfiletoupdateArgs, PS_LIST_TAIL, "-set_label", 0, "set new label", NULL);
    psMetadataAddStr(setfiletoupdateArgs, PS_LIST_TAIL, "-set_recoveryroot", 0, "define new recovery directory", NULL);

    // -destreakedfile
    psMetadata *destreakedfileArgs = psMetadataAlloc();
    psMetadataAddS64(destreakedfileArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic_ds_id", 0);
    psMetadataAddStr(destreakedfileArgs, PS_LIST_TAIL, "-component",   0, "search by component", NULL);
    psMetadataAddStr(destreakedfileArgs, PS_LIST_TAIL, "-stage",       0, "search by stage", NULL);
    psMetadataAddS64(destreakedfileArgs, PS_LIST_TAIL, "-stage_id", 0, "search by magic_id", 0);

    psMetadataAddStr(destreakedfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by magicRun.label", NULL);
    psMetadataAddS64(destreakedfileArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic_id", 0);

    psMetadataAddBool(destreakedfileArgs, PS_LIST_TAIL, "-faulted",  0, "list only faulted components", false);
    psMetadataAddBool(destreakedfileArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddU64(destreakedfileArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -listrun
    psMetadata *listrunArgs = psMetadataAlloc();
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-magic_ds_id", 0, "search by magic_ds_id", 0);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL, "-stage",       0, "define stage", NULL);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-stage_id", 0, "search by magic_id", 0);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic_id", 0);

    psMetadataAddStr(listrunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by magicRun.label", NULL);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-faulted",  0, "list faulted components", false);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddU64(listrunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",       "create magic de-streak runs from magic runs",
                    MAGICDSTOOL_MODE_DEFINEBYQUERY,     definebyqueryArgs);
    PXOPT_ADD_MODE("-definecopy",          "create magic de-streak runs from magic runs for different path",
                    MAGICDSTOOL_MODE_DEFINECOPY,     definecopyArgs);
    PXOPT_ADD_MODE("-updaterun",           "update state of magic de-streak run",
                    MAGICDSTOOL_MODE_UPDATERUN,         updaterunArgs);
    PXOPT_ADD_MODE("-todestreak",          "show pending files",
                    MAGICDSTOOL_MODE_TODESTREAK,       todestreakArgs);
    PXOPT_ADD_MODE("-adddestreakedfile",   "add a de-streaked file",
                    MAGICDSTOOL_MODE_ADDDESTREAKEDFILE, adddestreakedfileArgs);
    PXOPT_ADD_MODE("-advancerun", "change state for runs that have finished destrreaking",
                    MAGICDSTOOL_MODE_ADVANCERUN, advancerunArgs);
    PXOPT_ADD_MODE("-revertdestreakedfile", " revert a faulted de-streaked file",
                    MAGICDSTOOL_MODE_REVERTDESTREAKEDFILE, revertdestreakedfileArgs);
    PXOPT_ADD_MODE("-clearstatefaults", " clear magicDSRuns in fault state",
                    MAGICDSTOOL_MODE_CLEARSTATEFAULTS, clearstatefaultsArgs);
    PXOPT_ADD_MODE("-getskycells", "get skycell files ",
                    MAGICDSTOOL_MODE_GETSKYCELLS, getskycellsArgs);
    PXOPT_ADD_MODE("-toremove", "backup images pending removal",
                    MAGICDSTOOL_MODE_TOREMOVE, toremoveArgs);
    PXOPT_ADD_MODE("-torevert", "images to restore or revert",
                    MAGICDSTOOL_MODE_TOREVERT, torevertArgs);

    PXOPT_ADD_MODE("-completedrevert", "change state for runs that have finished reverting",
                    MAGICDSTOOL_MODE_COMPLETEDREVERT, completedrevertArgs);
    PXOPT_ADD_MODE("-tocleanup", "destreak runs to clean up",
                    MAGICDSTOOL_MODE_TOCLEANUP, tocleanupArgs);

    PXOPT_ADD_MODE("-tofullfile", "set component's data_state to full",
                    MAGICDSTOOL_MODE_TOFULLFILE, tofullfileArgs);
    PXOPT_ADD_MODE("-tocleanedfile", "set component's data_state to cleaned",
                    MAGICDSTOOL_MODE_TOCLEANEDFILE, tocleanedfileArgs);
    PXOPT_ADD_MODE("-setfiletoupdate", "set component's data_state to update",
                    MAGICDSTOOL_MODE_SETFILETOUPDATE, setfiletoupdateArgs);
    PXOPT_ADD_MODE("-updatedestreakedfile", "update parameters of destreaked file",
                    MAGICDSTOOL_MODE_UPDATEDESTREAKEDFILE, updatedestreakedfileArgs);
    PXOPT_ADD_MODE("-destreakedfile", "list destreaked file",
                    MAGICDSTOOL_MODE_DESTREAKEDFILE, destreakedfileArgs);
    PXOPT_ADD_MODE("-listrun", "list parameters of destreak run",
                    MAGICDSTOOL_MODE_LISTRUN, listrunArgs);


    if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
        psError(PS_ERR_UNKNOWN, true, "option parsing failed");
        psFree(argSets);
        psFree(modes);
        psFree(config);
        return NULL;
    }

    psFree(argSets);
    psFree(modes);

    // define Database handle, if used
    // do this last so we don't setup a connection before CLI options are
    // validated
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}

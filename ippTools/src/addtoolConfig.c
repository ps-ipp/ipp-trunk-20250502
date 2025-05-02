/*
 * addtoolConfig.c
 *
 * Copyright (C) 2009  Joshua Hoblitt, Chris Waters
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

#include <math.h>
#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "pxadd.h"
#include "addtool.h"

pxConfig *addtoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-stage",             0, "set the stage (required)", NULL);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-cam_id",             0, "search by cam_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-stack_id",             0, "search by stack_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-sky_id",             0, "search by sky_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-skycal_id",          0, "search by skycal_id", 0);
    pxcamSetSearchArgs(definebyqueryArgs);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-diff_id",            0, "search by diff_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-ff_id",              0, "search by ff_id", 0);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by camRun label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by camRun data_group", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL,  "-filter",        PS_META_DUPLICATE_OK, "search by rawExp filter",    NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-reduction",          0, "search by camRun reduction class", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-destreaked",           0, "queue destreaked runs", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-uncensored",           0, "queue uncensored runs", false);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",        0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",          0, "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",      0, "define reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb",          0, "define DVO db", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-image_only",        0, "addstar image metadata but not detections", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",           0, "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);
    psMetadataAddU64(definebyqueryArgs, PS_LIST_TAIL, "-limit",              0, "limit result set to N items", 0);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-addrand",           0, "set pending add_id randomly processing", false);

    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-set_minidvodb",            0, "use minidvodb", false);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_minidvodb_group", 0,   "define minidvodb_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_minidvodb_name", 0,   "define minidvodb_bname", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_minidvodb_host",  0, "define minidvodb_host",   NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);

    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-set_minra", 0,         "min ra for query", 0);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-set_maxra", 0,         "max ra for query", 360);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-set_mindec", 0,         "min dec for query", -90);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-set_maxdec", 0,         "max dec for query", 90);


    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-stage",             0, "set the stage (required)", NULL);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-add_id",                 0, "search by add_id", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-cam_id",                 0, "search by cam_id", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-stack_id",                 0, "search by stack_id", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-sky_id",                 0, "search by sky_id", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-skycal_id",                 0, "search by skycal_id", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-diff_id",                0, "search by diff_id",0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-ff_id",                  0, "search by _ff_id",0);
    pxcamSetSearchArgs(updaterunArgs);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",                  0, "search by addRun label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",                  0, "search by addRun state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-reduction",              0, "search by addRun reduction class", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dvodb",                  0, "search by dvodb", NULL);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-all",                   0, "allow everything to be queued without search terms", false);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state",              0, "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label",              0, "set label", NULL);
//    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dvodb",              0, "set dvodb", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);

    // -pendingexp
    psMetadata *pendingexpArgs = psMetadataAlloc();
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-stage",             0, "set the stage (required)", NULL);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-add_id",            0, "search by add_id", 0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-cam_id",            0, "search by cam_id", 0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-stack_id",                0, "search by stack_id", 0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-sky_id",                  0, "search by sky_id", 0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-skycal_id",               0, "search by skycal_id", 0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-diff_id",                 0, "search by diff_id",0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-ff_id",                   0, "search by ff_id",0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-stage_extra1",                   0, "search by stage_extra1",0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-stage_id",                   0, "search by stage_id",0);
    pxcamSetSearchArgs(pendingexpArgs);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by addRun label", NULL);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-reduction",         0, "search by addRun reduction class", NULL);
    psMetadataAddU64(pendingexpArgs, PS_LIST_TAIL, "-limit",             0, "limit result set to N items", 0);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-simple",           0, "use the simple output format", false);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-multiadd",           0, "for addstar multi mode, group by stage_id ", false);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-addrand",           0, "set pending add_id randomly processing", false);

    // -addprocessedexp
    psMetadata *addprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-add_id", 0,            "define addtool ID (required)", 0);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", NULL);
     psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-dvodb_path", 0,            "define base output location", NULL);
      psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-minidvodb_name", 0,            "define minidvodb_name", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-minidvodb_host",    0, "define minidvodb_name", NULL);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-dtime_addstar", 0, "define elapsed time for DVO insertion (seconds)", NAN);
    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-stage_extra1",  0,            "set stage_extra1", 0);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-stage_extra1_list",  0, "restrict multiadd ops to a set of stage_extra1 values", 0);
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-stage_id",  0,            "set stage_id", 0);
    psMetadataAddBool(addprocessedexpArgs, PS_LIST_TAIL, "-multiadd",           0, "for addstar multi mode, group by stage_id ", false);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-multiaddlabel", 0,            "label for multiadd", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-stage", 0,            "select stage", NULL);

    // -processedexp
    psMetadata *processedexpArgs = psMetadataAlloc();
    psMetadataAddS64(processedexpArgs, PS_LIST_TAIL, "-add_id",   0,            "search by add_id", 0);
    psMetadataAddS64(processedexpArgs, PS_LIST_TAIL, "-stage_id",   0,            "search by stage_id", 0);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-stage",             0, "set the stage", "cam");
    pxcamSetSearchArgs(processedexpArgs);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by addRun label", NULL);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-reduction",0,            "search by addRun reduction class", NULL);

    psMetadataAddU64(processedexpArgs, PS_LIST_TAIL, "-limit",    0,            "limit result set to N items", 0);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-faulted", 0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-all",     0,            "list everything without restriction", false);

    // -revertprocessedexp
    psMetadata *revertprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-add_id",   0,            "search by add_id", 0);
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-stage_id",   0,            "search by stage_id", 0);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-stage",             0, "set the stage", NULL);
    pxcamSetSearchArgs(revertprocessedexpArgs);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "search by addRun label", NULL);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-reduction",0,            "search by addRun reduction class", NULL);
    psMetadataAddBool(revertprocessedexpArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -updateprocessedexp
    psMetadata *updateprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-add_id", 0,            "search by addtool ID", 0);
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-stage_id",  0,            "search by stage_id", 0);
    psMetadataAddStr(updateprocessedexpArgs, PS_LIST_TAIL, "-stage",             0, "set the stage", NULL);
    psMetadataAddS16(updateprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);

    // -block
    psMetadata *blockArgs = psMetadataAlloc();
    psMetadataAddStr(blockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to mask out (required)", NULL);

    // -masked
    psMetadata *maskedArgs = psMetadataAlloc();
    psMetadataAddBool(maskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -unblock
    psMetadata *unblockArgs = psMetadataAlloc();
    psMetadataAddStr(unblockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to unmask (required)", NULL);

    // -addminidvodbruns
    psMetadata *addminidvodbrunArgs = psMetadataAlloc();
    psMetadataAddStr(addminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_name",        0, "define minidvodb_name", NULL);
    psMetadataAddStr(addminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_group",        0, "define minidvodb_group (required)", NULL);
    psMetadataAddStr(addminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_path",        0, "define path for minidvodb", NULL);
    psMetadataAddStr(addminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_host",  0, "define host for minidvodb (required)", NULL);
    psMetadataAddStr(addminidvodbrunArgs, PS_LIST_TAIL, "-set_state",        0, "define state", NULL);
    psMetadataAddStr(addminidvodbrunArgs, PS_LIST_TAIL, "-set_note",         0, "define note", NULL);

    // -updateminidvodbruns
    psMetadata *updateminidvodbrunArgs = psMetadataAlloc();
    psMetadataAddU64(updateminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id ", 0);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_group",        0, "search by minidvodb_name (LIKE)", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_name",        0, "search by minidvodb_name (LIKE)", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_path",        0, "search by path for minidvodb", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-state",        0, "search by state", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_name",        0, "define minidvodb_name", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_path",        0, "define path for minidvodb", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_group",        0, "define path for the merged dvodb", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-set_minidvodb_host",  0, "define host for the merged dvodb", NULL);
    psMetadataAddStr(updateminidvodbrunArgs, PS_LIST_TAIL, "-set_state",        0, "define state", NULL);
    
    // -listminidvodbrunArgs
    psMetadata *listminidvodbrunArgs = psMetadataAlloc();
    psMetadataAddU64(listminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(listminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_name",        0, "search by minidvodb_name (LIKE)", NULL);
    psMetadataAddStr(listminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_group",     0,    "search by minidvodb.minidvodb_group", NULL);
    psMetadataAddStr(listminidvodbrunArgs, PS_LIST_TAIL, "-state",        0, "search by state", NULL);
    psMetadataAddU64(listminidvodbrunArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(listminidvodbrunArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
    
    //psMetadataAddBool(listminidvodbrunArgs, PS_LIST_TAIL, "-finished_addrun",        0, "limit to minidvodbs with completed addRuns (none in new state)", false);
    psMetadata *flipminidvodbrunArgs = psMetadataAlloc();
    psMetadataAddStr(flipminidvodbrunArgs, PS_LIST_TAIL, "-minidvodb_group",     0,    "for the supplied minidvodb_group (required): flip the current 'new' to 'active', the current 'active' to 'waiting'", NULL);
    
    psMetadata *checkminidvodbrunaddrunArgs = psMetadataAlloc();
    psMetadataAddStr(checkminidvodbrunaddrunArgs, PS_LIST_TAIL, "-minidvodb_group",     0,    "for the supplied minidvodb_group (required): check if the addRun stage is complete (all in addRun.state = full) ", NULL);
    psMetadataAddStr(checkminidvodbrunaddrunArgs, PS_LIST_TAIL, "-minidvodb_name",     0,    "for the supplied minidvodb_name: check if the addRun stage is complete (all in addRun.state = full) ", NULL);
    psMetadataAddStr(checkminidvodbrunaddrunArgs, PS_LIST_TAIL, "-state",     0,    "limit by minidvodbRun state ", NULL);
    psMetadataAddU64(checkminidvodbrunaddrunArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(checkminidvodbrunaddrunArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
    psMetadataAddBool(checkminidvodbrunaddrunArgs, PS_LIST_TAIL, "-all_addrun_states",        0, "list all minidvodbRun.minidvodb_names, not just ones that have complete addRuns", false);

    psMetadata *addminidvodbprocessedArgs = psMetadataAlloc();
    psMetadataAddU64(addminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_id", 0,    "define minidvodb_id (required)", 0);
    psMetadataAddF32(addminidvodbprocessedArgs, PS_LIST_TAIL, "-dtime_relphot",  0,    "define elapsed time for relphot (seconds)", NAN);
    psMetadataAddF32(addminidvodbprocessedArgs, PS_LIST_TAIL, "-dtime_resort", 0,    "define elapsed time for resort (seconds)", NAN);
    psMetadataAddF32(addminidvodbprocessedArgs, PS_LIST_TAIL, "-dtime_script",    0,    "define elapsed time for script (seconds)", NAN);
    psMetadataAddTime(addminidvodbprocessedArgs, PS_LIST_TAIL, "-epoch",         0,    "time merge is finished", NULL);
    psMetadataAddStr(addminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_group",0,    "minidvodb_group", NULL);
    psMetadataAddS16(addminidvodbprocessedArgs, PS_LIST_TAIL, "-fault",          0,    "set fault code", 0);

    psMetadata *listminidvodbprocessedArgs = psMetadataAlloc();
    psMetadataAddU64(listminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(listminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_name",        0, "search by minidvodb_name", NULL);
    psMetadataAddStr(listminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_group",        0, "search by minidvodb.minidvodb_group", NULL);
    psMetadataAddU64(listminidvodbprocessedArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(listminidvodbprocessedArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
    psMetadataAddBool(listminidvodbprocessedArgs, PS_LIST_TAIL, "-faulted",        0, "limit to faulted state", false);

    psMetadata *revertminidvodbprocessedArgs = psMetadataAlloc();
    psMetadataAddU64(revertminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(revertminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_group",        0, "search by addRun.minidvodb_group", NULL);
    psMetadataAddBool(revertminidvodbprocessedArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    psMetadata *updateminidvodbprocessedArgs = psMetadataAlloc();
    psMetadataAddU64(updateminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(updateminidvodbprocessedArgs, PS_LIST_TAIL, "-minidvodb_name",        0, "search by minidvodb_name", NULL);
    psMetadataAddS16(updateminidvodbprocessedArgs, PS_LIST_TAIL, "-set_fault",  0,            "set fault code", 0);
    psMetadataAddF32(updateminidvodbprocessedArgs, PS_LIST_TAIL, "-set_dtime_relphot",  0,    "define elapsed time for relphot (seconds)", 0);
    psMetadataAddF32(updateminidvodbprocessedArgs, PS_LIST_TAIL, "-set_dtime_resort", 0,    "define elapsed time for resort (seconds)", 0);
    psMetadataAddF32(updateminidvodbprocessedArgs, PS_LIST_TAIL, "-set_dtime_script",    0,    "define elapsed time for script (seconds)", 0);
    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from cam/skycal/forcedwarp/diff stage",           ADDTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-updaterun",            "change add run properties",            ADDTOOL_MODE_UPDATERUN,      updaterunArgs);
    PXOPT_ADD_MODE("-pendingexp",           "show pending exps",                    ADDTOOL_MODE_PENDINGEXP,    pendingexpArgs);
    PXOPT_ADD_MODE("-addprocessedexp",      "add a processed exp",                  ADDTOOL_MODE_ADDPROCESSEDEXP, addprocessedexpArgs);
    PXOPT_ADD_MODE("-processedexp",         "show processed exps",                  ADDTOOL_MODE_PROCESSEDEXP,  processedexpArgs);
    PXOPT_ADD_MODE("-revertprocessedexp",   "change procesed exp properties",       ADDTOOL_MODE_REVERTPROCESSEDEXP,  revertprocessedexpArgs);
    PXOPT_ADD_MODE("-updateprocessedexp",   "undo a processed exp",                 ADDTOOL_MODE_UPDATEPROCESSEDEXP,updateprocessedexpArgs);
    PXOPT_ADD_MODE("-block",                "set a label block",                    ADDTOOL_MODE_BLOCK,         blockArgs);
    PXOPT_ADD_MODE("-masked",               "show blocked labels",                  ADDTOOL_MODE_MASKED,        maskedArgs);
    PXOPT_ADD_MODE("-addminidvodbrun",      "create minidvodbs ",                   ADDTOOL_MODE_ADDMINIDVODBRUN, addminidvodbrunArgs);
    PXOPT_ADD_MODE("-updateminidvodbrun",   "change minidvodb properties",          ADDTOOL_MODE_UPDATEMINIDVODBRUN,     updateminidvodbrunArgs);
    PXOPT_ADD_MODE("-listminidvodbrun",     "list minidvodbs",                      ADDTOOL_MODE_LISTMINIDVODBRUN,       listminidvodbrunArgs);
    PXOPT_ADD_MODE("-flipminidvodbrun",     "flip minidvodbs",                      ADDTOOL_MODE_FLIPMINIDVODBRUN,       flipminidvodbrunArgs);
    PXOPT_ADD_MODE("-checkminidvodbrunaddrun", "check minidvodbs to see if addRuns are completed", ADDTOOL_MODE_CHECKMINIDVODBRUNADDRUN,       checkminidvodbrunaddrunArgs);
    PXOPT_ADD_MODE("-addminidvodbprocessed","add a processed minidvodb",            ADDTOOL_MODE_ADDMINIDVODBPROCESSED,  addminidvodbprocessedArgs);
    PXOPT_ADD_MODE("-listminidvodbprocessed","list processed minidvodbs",           ADDTOOL_MODE_LISTMINIDVODBPROCESSED, listminidvodbprocessedArgs);
    PXOPT_ADD_MODE("-revertminidvodbprocessed","revert processed minidvobs",        ADDTOOL_MODE_REVERTMINIDVODBPROCESSED,     revertminidvodbprocessedArgs);
    PXOPT_ADD_MODE("-updateminidvodbprocessed","change processed minidvodb properties",ADDTOOL_MODE_UPDATEMINIDVODBPROCESSED,  updateminidvodbprocessedArgs);
 
 


    if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
        psError(PS_ERR_UNKNOWN, false, "option parsing failed");
        psFree(argSets);
        psFree(modes);
        psFree(config);
        return NULL;
    }

    psFree(argSets);
    psFree(modes);

    // define Database handle, if used
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}

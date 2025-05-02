/*
 * disttoolConfig.c
 *
 * Copyright (C) 2008
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

#include <psmodules.h>

#include "pxtools.h"
#include "disttool.h"

pxConfig *disttoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (! config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration!\n");
        psFree(config);
        return NULL;
    }

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-stage",     0, "define stage for bundle (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-workdir",   0, "define workdir (required)", NULL);

    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-no_magic", 0, "magic is not needed (ignored)", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-magic", 0, "magic is needed", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-use_alternate", 0, "use alternate inputs", false);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",    0, "define label for run", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group",    0, "define data_group for run", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note",    0, "select by dist_group", NULL);

    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend", 0, "don't queue runs just display what would be selected", false);

    // select args
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-target_id",     0, "select by target_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-exp_id",        0, "select by exp_id", 0);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-exp_type",      0, "select by exp_type", NULL);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-chip_id",       0, "select by chip_id", 0);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label",         0, "select by run label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-dist_group",    0, "select by dist_group", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-singlefilter", 0, "select single filter runs (sky stage only)", false);

    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-rerun", 0, "queue a new distRun even if one already exists", false);
    psMetadataAddU64(definebyqueryArgs, PS_LIST_TAIL, "-limit",  0,  "limit result set to N items", 0);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-dist_id",  0, "define dist_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "new value for state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0, "new value for label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0, "new value for label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_outdir", 0, "new value for outdir", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-stage",     0, "value for stage", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",     0, "value for state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",     0, "limit updates to label (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group",     0, "limit updates to data_group (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dist_group",     0, "limit updates to data_group", NULL);
    psMetadataAddTime(updaterunArgs, PS_LIST_TAIL, "-time_stamp_begin", 0, "limit updates by time_stamp (>=)", NULL); 
    psMetadataAddTime(updaterunArgs, PS_LIST_TAIL, "-time_stamp_end", 0, "limit updates by time_stamp (<=)", NULL); 
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-dist_id_min",  0, "limit updates by dist_id (>=)", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-dist_id_max",  0, "limit updates by dist_id (<=)", 0);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL,"-clean",     0, "limit updates to clean distRuns", false);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL,"-full",      0, "limit updates to not clean distRuns", false);
    psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-fault",      0, "define fault code", 0);

    // -startover
    psMetadata *startoverArgs = psMetadataAlloc();
    psMetadataAddStr(startoverArgs, PS_LIST_TAIL, "-data_group",    0, "define data_group (LIKE comparison)", NULL);
    psMetadataAddS64(startoverArgs, PS_LIST_TAIL, "-dist_id", 0, "search by dist_id", 0);
    psMetadataAddStr(startoverArgs, PS_LIST_TAIL, "-stage",    0, "search by stage", NULL);
    psMetadataAddS64(startoverArgs, PS_LIST_TAIL, "-stage_id", 0, "search by stage_id", 0);
    psMetadataAddStr(startoverArgs, PS_LIST_TAIL, "-label",    0, "search by label", NULL);
    psMetadataAddStr(startoverArgs, PS_LIST_TAIL, "-set_label",    0, "define new label", NULL);
    psMetadataAddBool(startoverArgs, PS_LIST_TAIL, "-pretend", 0, "don't queue runs just display what would be selected", false);
    psMetadataAddU64(startoverArgs, PS_LIST_TAIL, "-limit",  0,  "limit result set to N items", 0);
    psMetadataAddBool(startoverArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -revertrun
    psMetadata *revertrunArgs = psMetadataAlloc();
    psMetadataAddS64(revertrunArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(revertrunArgs, PS_LIST_TAIL, "-stage",    0, "define stage", NULL);
    psMetadataAddS64(revertrunArgs, PS_LIST_TAIL, "-stage_id", 0, "define stage_id", 0);
    psMetadataAddStr(revertrunArgs, PS_LIST_TAIL, "-state",    0, "define state", NULL);
    psMetadataAddStr(revertrunArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddS16(revertrunArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);
    psMetadataAddBool(revertrunArgs, PS_LIST_TAIL, "-all",    0, "revert all faulted runs", NULL);

    // -pendingcomponent
    psMetadata *pendingcomponentArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcomponentArgs, PS_LIST_TAIL, "-stage",    0, "limit results to runs for stage (required)", NULL);
    psMetadataAddS64(pendingcomponentArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(pendingcomponentArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "limit results to label", NULL);
    psMetadataAddU64(pendingcomponentArgs, PS_LIST_TAIL, "-limit",  0,  "limit result set to N items", 0);
    psMetadataAddBool(pendingcomponentArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -pendingcleanup
    psMetadata *pendingcleanupArgs = psMetadataAlloc();
    psMetadataAddS64(pendingcleanupArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(pendingcleanupArgs, PS_LIST_TAIL, "-stage",    0, "limit results to runs for stage", NULL);
    psMetadataAddStr(pendingcleanupArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "limit results to label", NULL);
    psMetadataAddU64(pendingcleanupArgs, PS_LIST_TAIL, "-limit",  0,  "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanupArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(pendingcleanupArgs, PS_LIST_TAIL, "-all",    0, "list all runs pending cleanup", NULL);


    // -addprocessedcomponent
    psMetadata *addprocessedcomponentArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedcomponentArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(addprocessedcomponentArgs, PS_LIST_TAIL, "-component", 0, "define component (required)", NULL);
    psMetadataAddStr(addprocessedcomponentArgs, PS_LIST_TAIL, "-outdir", 0, "define output directory", NULL);
    psMetadataAddStr(addprocessedcomponentArgs, PS_LIST_TAIL, "-name", 0, "define file name", NULL);
    psMetadataAddS32(addprocessedcomponentArgs, PS_LIST_TAIL, "-bytes", 0, "define file size", 0);
    psMetadataAddStr(addprocessedcomponentArgs, PS_LIST_TAIL, "-md5sum", 0, "define stage for bundle", NULL);
    psMetadataAddS32(addprocessedcomponentArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);

    // -revertcomponent
    psMetadata *revertcomponentArgs = psMetadataAlloc();
    psMetadataAddS64(revertcomponentArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(revertcomponentArgs, PS_LIST_TAIL, "-component", 0, "define component", NULL);
    psMetadataAddStr(revertcomponentArgs, PS_LIST_TAIL, "-stage",    0, "define stage", NULL);
    psMetadataAddS64(revertcomponentArgs, PS_LIST_TAIL, "-stage_id", 0, "define stage_id", 0);
    psMetadataAddStr(revertcomponentArgs, PS_LIST_TAIL, "-state",    0, "define state", NULL);
    psMetadataAddStr(revertcomponentArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddS16(revertcomponentArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);
    psMetadataAddBool(revertcomponentArgs, PS_LIST_TAIL, "-all",    0, "revert all faulted runs", NULL);

    // -processedcomponent
    psMetadata *processedcomponentArgs = psMetadataAlloc();
    psMetadataAddS64(processedcomponentArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(processedcomponentArgs, PS_LIST_TAIL, "-component", 0, "define component", NULL);
    psMetadataAddU64(processedcomponentArgs, PS_LIST_TAIL, "-limit",  0,  "limit result set to N items", 0);
    psMetadataAddBool(processedcomponentArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -updateprocessedcomponentArgs
    psMetadata *updateprocessedcomponentArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedcomponentArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(updateprocessedcomponentArgs, PS_LIST_TAIL, "-component", 0, "define component (required)", NULL);
    psMetadataAddStr(updateprocessedcomponentArgs, PS_LIST_TAIL, "-outdir", 0, "define output directory", NULL);
    psMetadataAddStr(updateprocessedcomponentArgs, PS_LIST_TAIL, "-name", 0, "define file name", NULL);
    psMetadataAddS32(updateprocessedcomponentArgs, PS_LIST_TAIL, "-bytes", 0, "define file size", 0);
    psMetadataAddStr(updateprocessedcomponentArgs, PS_LIST_TAIL, "-md5sum", 0, "define stage for bundle", NULL);
    psMetadataAddS32(updateprocessedcomponentArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);
    psMetadataAddBool(updateprocessedcomponentArgs, PS_LIST_TAIL, "-clearfault", 0, "set fault to zero", false);

    // -toadvance
    psMetadata *toadvanceArgs = psMetadataAlloc();
    psMetadataAddS64(toadvanceArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-label",   PS_META_DUPLICATE_OK, "limit updates to label", NULL);
    psMetadataAddU64(toadvanceArgs, PS_LIST_TAIL, "-limit",   0,  "limit result set to N items", 0);
    psMetadataAddBool(toadvanceArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -pendingfileset
    psMetadata *pendingfilesetArgs = psMetadataAlloc();
    psMetadataAddS64(pendingfilesetArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-label",   PS_META_DUPLICATE_OK, "limit results to label", NULL);
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-dist_group",   PS_META_DUPLICATE_OK, "limit results to dist_group", NULL);
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-stage",   0, "limit results to runs for stage", NULL);
    psMetadataAddU64(pendingfilesetArgs, PS_LIST_TAIL, "-limit",   0,  "limit result set to N items", 0);
    psMetadataAddBool(pendingfilesetArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -addfileset
    psMetadata *addfilesetArgs = psMetadataAlloc();
    psMetadataAddS64(addfilesetArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddS64(addfilesetArgs, PS_LIST_TAIL, "-dest_id", 0, "define dest_id", 0);
    psMetadataAddStr(addfilesetArgs, PS_LIST_TAIL, "-name",    0, "define file name", NULL);
    psMetadataAddS32(addfilesetArgs, PS_LIST_TAIL, "-fault",   0, "define fault code", 0);

    // -updatefileset
    psMetadata *updatefilesetArgs = psMetadataAlloc();
    psMetadataAddS64(updatefilesetArgs, PS_LIST_TAIL, "-fs_id", 0, "define fs_id", 0);
    psMetadataAddS64(updatefilesetArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddStr(updatefilesetArgs, PS_LIST_TAIL, "-set_state",0, "new value for state", NULL);
    psMetadataAddS32(updatefilesetArgs, PS_LIST_TAIL, "-fault",   0, "define fault code", 0);

    // -pendingdest
    psMetadata *pendingdestArgs = psMetadataAlloc();
    psMetadataAddS64(pendingdestArgs, PS_LIST_TAIL, "-dest_id", 0, "define dest_id", 0);
    psMetadataAddU64(pendingdestArgs, PS_LIST_TAIL, "-limit",   0,  "limit result set to N items", 0);
    psMetadataAddBool(pendingdestArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddStr(pendingdestArgs, PS_LIST_TAIL, "-label",   PS_META_DUPLICATE_OK, "limit results to label", NULL);

    // -revertfileset
    psMetadata *revertfilesetArgs = psMetadataAlloc();
    psMetadataAddS64(revertfilesetArgs, PS_LIST_TAIL, "-fs_id",   0, "define fs_id", 0);
    psMetadataAddS64(revertfilesetArgs, PS_LIST_TAIL, "-dist_id", 0, "define dist_id", 0);
    psMetadataAddS64(revertfilesetArgs, PS_LIST_TAIL, "-dest_id", 0, "define dist_id", 0);
    psMetadataAddStr(revertfilesetArgs, PS_LIST_TAIL, "-stage",   0, "define stage", NULL);
    psMetadataAddS64(revertfilesetArgs, PS_LIST_TAIL, "-stage_id",0, "define stage_id", 0);
    psMetadataAddStr(revertfilesetArgs, PS_LIST_TAIL, "-state",   0, "define state", NULL);
    psMetadataAddStr(revertfilesetArgs, PS_LIST_TAIL, "-label",   PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddS16(revertfilesetArgs, PS_LIST_TAIL, "-fault",   0, "define fault code", 0);
    psMetadataAddBool(revertfilesetArgs, PS_LIST_TAIL, "-all",    0, "revert all faulted runs", NULL);

    // -listfileset
    psMetadata *listfilesetsArgs = psMetadataAlloc();
    psMetadataAddS64(listfilesetsArgs, PS_LIST_TAIL, "-dist_id", 0, "list filesets with dist_id", 0);
    psMetadataAddS64(listfilesetsArgs, PS_LIST_TAIL, "-target_id", 0, "list filesets with target_id", 0);
    psMetadataAddS64(listfilesetsArgs, PS_LIST_TAIL, "-dest_id", 0, "list filesets with dest_id", 0);
    psMetadataAddS64(listfilesetsArgs, PS_LIST_TAIL, "-int_id", 0, "list filesets with int_id", 0);
    psMetadataAddStr(listfilesetsArgs, PS_LIST_TAIL, "-dest_name",  0, "list filesets for destinationn name)", NULL);
    psMetadataAddStr(listfilesetsArgs, PS_LIST_TAIL, "-dist_group",  0, "list filesets for dist_group (LIKE comparison)", NULL);
    psMetadataAddStr(listfilesetsArgs, PS_LIST_TAIL, "-filter",    0, "list filesets by filter (LIKE comparison)", NULL);
    psMetadataAddStr(listfilesetsArgs, PS_LIST_TAIL, "-stage",     0, "list filesets for stage", NULL);
    psMetadataAddBool(listfilesetsArgs, PS_LIST_TAIL,"-clean",     0, "list clean filesets", false);
    psMetadataAddBool(listfilesetsArgs, PS_LIST_TAIL,"-full",      0, "list full filesets", false);
    psMetadataAddStr(listfilesetsArgs, PS_LIST_TAIL, "-state",     0, "list filesets in state", NULL);
    psMetadataAddStr(listfilesetsArgs, PS_LIST_TAIL, "-label",   PS_META_DUPLICATE_OK, "limit results to label", NULL);
    psMetadataAddU64(listfilesetsArgs, PS_LIST_TAIL, "-limit",     0, "limit number of filesets listed to N", 0);
    psMetadataAddBool(listfilesetsArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -queuercrun
    psMetadata *queuercrunArgs = psMetadataAlloc();
    psMetadataAddS64(queuercrunArgs, PS_LIST_TAIL, "-dist_id",   0, "define dist_id", 0);
    psMetadataAddS64(queuercrunArgs, PS_LIST_TAIL, "-dest_id",   0, "define dest_id", 0);
    psMetadataAddS64(queuercrunArgs, PS_LIST_TAIL, "-target_id", 0, "define target_id", 0);
    psMetadataAddS64(queuercrunArgs, PS_LIST_TAIL, "-fs_id",     0, "define fs_id", 0);
    psMetadataAddStr(queuercrunArgs, PS_LIST_TAIL, "-stage",     0, "define stage", NULL);
    psMetadataAddS64(queuercrunArgs, PS_LIST_TAIL, "-stage_id",  0, "define stage_id", 0);
    psMetadataAddStr(queuercrunArgs, PS_LIST_TAIL, "-state",     0, "define state", NULL);
    psMetadataAddStr(queuercrunArgs, PS_LIST_TAIL, "-label",     0, "define label", NULL);
    psMetadataAddU64(queuercrunArgs, PS_LIST_TAIL, "-limit",     0, "limit number of runs queued to N", 0);

    // -updatercrun
    psMetadata *updatercrunArgs = psMetadataAlloc();
    psMetadataAddS64(updatercrunArgs, PS_LIST_TAIL, "-rc_id",    0, "define rc_id", 0);
    psMetadataAddS64(updatercrunArgs, PS_LIST_TAIL, "-dest_id",  0, "define dest_id", 0);
    psMetadataAddStr(updatercrunArgs, PS_LIST_TAIL, "-fs_name",  0, "define fileset name", NULL);
    psMetadataAddStr(updatercrunArgs, PS_LIST_TAIL, "-status_fs_name",  0, "define status fileset name", NULL);
    psMetadataAddStr(updatercrunArgs, PS_LIST_TAIL, "-set_state",0, "new value for state", NULL);
    psMetadataAddStr(updatercrunArgs, PS_LIST_TAIL, "-set_last_fileset",0, "new value for last_fileset", NULL);
    psMetadataAddS16(updatercrunArgs, PS_LIST_TAIL, "-fault",    0, "define fault code", -1);

    // -revertrcrun
    psMetadata *revertrcrunArgs = psMetadataAlloc();
    psMetadataAddS64(revertrcrunArgs, PS_LIST_TAIL,  "-rc_id",   0, "define rc_id", 0);
    psMetadataAddS64(revertrcrunArgs, PS_LIST_TAIL,  "-fs_id",   0, "define fs_id", 0);
    psMetadataAddS64(revertrcrunArgs, PS_LIST_TAIL,  "-dest_id", 0, "define dest_id", 0);
    psMetadataAddS16(revertrcrunArgs, PS_LIST_TAIL,  "-fault",   0, "define fault code", 0);
    psMetadataAddBool(revertrcrunArgs, PS_LIST_TAIL, "-all",     0, "revert all faulted runs", NULL);

    // -definedestination
    psMetadata *definedestinationArgs = psMetadataAlloc();
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-name",        0, "define destination name (required)", NULL);
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-ds_dbname",0, "define data store database name (required)", NULL);
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-ds_dbhost",0, "define data store database host (required)", NULL);
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-status_uri",  0, "define status_uri", NULL);
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-comment",     0, "define comment", NULL);
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-last_fileset",0, "define last_fileset", NULL);
    psMetadataAddStr(definedestinationArgs, PS_LIST_TAIL, "-set_state",   0, "define state", NULL);

    // -updatedestination
    psMetadata *updatedestinationArgs = psMetadataAlloc();
    psMetadataAddS64(updatedestinationArgs, PS_LIST_TAIL, "-dest_id",     0, "define dest_id", 0);
    psMetadataAddStr(updatedestinationArgs, PS_LIST_TAIL, "-name",        0, "define destination name", NULL);
    psMetadataAddStr(updatedestinationArgs, PS_LIST_TAIL, "-status_uri",  0, "define status_uri", NULL);
    psMetadataAddStr(updatedestinationArgs, PS_LIST_TAIL, "-comment",     0, "define comment", NULL);
    psMetadataAddStr(updatedestinationArgs, PS_LIST_TAIL, "-set_state",   0, "define state", NULL);

    // -definetarget
    psMetadata *definetargetArgs = psMetadataAlloc();
    psMetadataAddStr(definetargetArgs, PS_LIST_TAIL, "-dist_group", 0, "define dist_group (required)", NULL);
    psMetadataAddStr(definetargetArgs, PS_LIST_TAIL, "-filter",     0, "define filter (required)", NULL);
    psMetadataAddStr(definetargetArgs, PS_LIST_TAIL, "-stage",     0, "define stage (required)", NULL);
    psMetadataAddBool(definetargetArgs, PS_LIST_TAIL,"-clean",     0, "define clean", false);
    psMetadataAddStr(definetargetArgs, PS_LIST_TAIL, "-set_state", 0, "define state", NULL);
    psMetadataAddStr(definetargetArgs, PS_LIST_TAIL, "-comment",   0, "define comment", NULL);

    // -updatetarget
    psMetadata *updatetargetArgs = psMetadataAlloc();
    psMetadataAddS64(updatetargetArgs, PS_LIST_TAIL, "-target_id", 0, "define target_id", 0);
    psMetadataAddStr(updatetargetArgs, PS_LIST_TAIL, "-stage",     0, "define stage", NULL);
    psMetadataAddStr(updatetargetArgs, PS_LIST_TAIL, "-dist_group", 0, "define dist_group", NULL);
    psMetadataAddStr(updatetargetArgs, PS_LIST_TAIL, "-filter",    0, "define filter", NULL);

    psMetadataAddStr(updatetargetArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);

    // -listtargets
    psMetadata *listtargetsArgs = psMetadataAlloc();
    psMetadataAddS64(listtargetsArgs, PS_LIST_TAIL, "-target_id", 0, "list target with target_id", 0);
    psMetadataAddStr(listtargetsArgs, PS_LIST_TAIL, "-dist_group",  0, "list targets for dist_group", NULL);
    psMetadataAddStr(listtargetsArgs, PS_LIST_TAIL, "-filter",    0, "define filter", NULL);
    psMetadataAddStr(listtargetsArgs, PS_LIST_TAIL, "-stage",     0, "list targets for stage", NULL);
    psMetadataAddBool(listtargetsArgs, PS_LIST_TAIL,"-clean",     0, "list clean targets", false);
    psMetadataAddBool(listtargetsArgs, PS_LIST_TAIL,"-full",      0, "list full targets", false);
    psMetadataAddStr(listtargetsArgs, PS_LIST_TAIL, "-state",     0, "list targets in state", NULL);
    psMetadataAddU64(listtargetsArgs, PS_LIST_TAIL, "-limit",     0, "limit number of targets listed to N", 0);
    psMetadataAddBool(listtargetsArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -defineinterest
    psMetadata *defineinterestArgs = psMetadataAlloc();
    psMetadataAddS64(defineinterestArgs, PS_LIST_TAIL, "-dest_id",   0, "define dest_id", 0);
    psMetadataAddStr(defineinterestArgs, PS_LIST_TAIL, "-dest_name", 0, "define destination name (LIKE comparison)", NULL);
    psMetadataAddS64(defineinterestArgs, PS_LIST_TAIL, "-target_id", 0, "define target_id", 0);
    psMetadataAddStr(defineinterestArgs, PS_LIST_TAIL, "-stage",     0, "define stage", NULL);
    psMetadataAddStr(defineinterestArgs, PS_LIST_TAIL, "-dist_group",     0, "define dist_group", NULL);
    psMetadataAddStr(defineinterestArgs, PS_LIST_TAIL, "-filter",    0, "define filter (LIKE comparison)", NULL);
    psMetadataAddBool(defineinterestArgs, PS_LIST_TAIL,"-clean",     0, "list clean targets", false);
    psMetadataAddStr(defineinterestArgs, PS_LIST_TAIL, "-set_state", 0, "define state", NULL);
    psMetadataAddU64(defineinterestArgs, PS_LIST_TAIL, "-limit",     0, "limit number of targets listed to N", 0);

    // -updateinterest
    psMetadata *updateinterestArgs = psMetadataAlloc();
    psMetadataAddS64(updateinterestArgs, PS_LIST_TAIL, "-int_id",    0, "define int_id", 0);
    psMetadataAddS64(updateinterestArgs, PS_LIST_TAIL, "-dest_id",   0, "define dest_id", 0);
    psMetadataAddS64(updateinterestArgs, PS_LIST_TAIL, "-target_id", 0, "define target_id", 0);
    psMetadataAddStr(updateinterestArgs, PS_LIST_TAIL, "-set_state", 0, "define state (required)", NULL);
    psMetadataAddStr(updateinterestArgs, PS_LIST_TAIL, "-stage",    0, "define stage", NULL);
    psMetadataAddStr(updateinterestArgs, PS_LIST_TAIL, "-filter",    0, "define filter (LIKE comparison)", NULL);
    psMetadataAddStr(updateinterestArgs, PS_LIST_TAIL, "-dest_name",    0, "define destination name", NULL);
    psMetadataAddStr(updateinterestArgs, PS_LIST_TAIL, "-dist_group",    0, "define distribution group", NULL);
    psMetadataAddBool(updateinterestArgs, PS_LIST_TAIL,"-clean",     0, "update clean interests", false);
    psMetadataAddBool(updateinterestArgs, PS_LIST_TAIL,"-full",     0, "update full interests", false);

    // -listinterests
    psMetadata *listinterestsArgs = psMetadataAlloc();
    psMetadataAddS64(listinterestsArgs, PS_LIST_TAIL, "-target_id", 0, "list interests with target_id", 0);
    psMetadataAddS64(listinterestsArgs, PS_LIST_TAIL, "-dest_id", 0, "list interests with dest_id", 0);
    psMetadataAddS64(listinterestsArgs, PS_LIST_TAIL, "-int_id", 0, "list interests with int_id", 0);
    psMetadataAddStr(listinterestsArgs, PS_LIST_TAIL, "-dest_name",  0, "list interests for destinationn name)", NULL);
    psMetadataAddStr(listinterestsArgs, PS_LIST_TAIL, "-dist_group",  0, "list interests for dist_group (LIKE comparison)", NULL);
    psMetadataAddStr(listinterestsArgs, PS_LIST_TAIL, "-filter",    0, "list interests by filter (LIKE comparison)", NULL);
    psMetadataAddStr(listinterestsArgs, PS_LIST_TAIL, "-stage",     0, "list interests for stage", NULL);
    psMetadataAddBool(listinterestsArgs, PS_LIST_TAIL,"-clean",     0, "list clean interests", false);
    psMetadataAddBool(listinterestsArgs, PS_LIST_TAIL,"-full",      0, "list full interests", false);
    psMetadataAddStr(listinterestsArgs, PS_LIST_TAIL, "-state",     0, "list interests in state", NULL);
    psMetadataAddU64(listinterestsArgs, PS_LIST_TAIL, "-limit",     0, "limit number of interests listed to N", 0);
    psMetadataAddBool(listinterestsArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",    "define distribution run", DISTTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-updaterun",    "update distRun", DISTTOOL_MODE_UPDATERUN, updaterunArgs);
    PXOPT_ADD_MODE("-startover",        "reprocess a cleaned distribution run", DISTTOOL_MODE_STARTOVER, startoverArgs);
    PXOPT_ADD_MODE("-revertrun",    "revert distRun.faults", DISTTOOL_MODE_REVERTRUN, revertrunArgs);
    PXOPT_ADD_MODE("-pendingcomponent",   "", DISTTOOL_MODE_PENDINGCOMPONENT,    pendingcomponentArgs);
    PXOPT_ADD_MODE("-addprocessedcomponent", "", DISTTOOL_MODE_ADDPROCESSEDCOMPONENT, addprocessedcomponentArgs);
    PXOPT_ADD_MODE("-updateprocessedcomponent", "", DISTTOOL_MODE_UPDATEPROCESSEDCOMPONENT, updateprocessedcomponentArgs);
    PXOPT_ADD_MODE("-revertcomponent",    "revert faulted components", DISTTOOL_MODE_REVERTCOMPONENT, revertcomponentArgs);
    PXOPT_ADD_MODE("-processedcomponent", "", DISTTOOL_MODE_PROCESSEDCOMPONENT, processedcomponentArgs);
    PXOPT_ADD_MODE("-toadvance",          "", DISTTOOL_MODE_TOADVANCE, toadvanceArgs);
    PXOPT_ADD_MODE("-pendingcleanup",     "", DISTTOOL_MODE_PENDINGCLEANUP, pendingcleanupArgs);
    PXOPT_ADD_MODE("-pendingfileset",     "", DISTTOOL_MODE_PENDINGFILESET, pendingfilesetArgs);
    PXOPT_ADD_MODE("-addfileset",         "", DISTTOOL_MODE_ADDFILESET, addfilesetArgs);
    PXOPT_ADD_MODE("-updatefileset",         "", DISTTOOL_MODE_UPDATEFILESET, updatefilesetArgs);
    PXOPT_ADD_MODE("-listfilesets",        "", DISTTOOL_MODE_LISTFILESETS, listfilesetsArgs);
    PXOPT_ADD_MODE("-revertfileset",      "revert faulted filesets", DISTTOOL_MODE_REVERTFILESET, revertfilesetArgs);
    PXOPT_ADD_MODE("-queuercrun",         "", DISTTOOL_MODE_QUEUERCRUN, queuercrunArgs);
    PXOPT_ADD_MODE("-updatercrun",        "", DISTTOOL_MODE_UPDATERCRUN, updatercrunArgs);
    PXOPT_ADD_MODE("-revertrcrun",        "", DISTTOOL_MODE_REVERTRCRUN, revertrcrunArgs);
    PXOPT_ADD_MODE("-pendingdest",        "", DISTTOOL_MODE_PENDINGDEST, pendingdestArgs);

    PXOPT_ADD_MODE("-definedestination",  "", DISTTOOL_MODE_DEFINEDESTINATION, definedestinationArgs);
    PXOPT_ADD_MODE("-updatedestination",  "", DISTTOOL_MODE_UPDATEDESTINATION, updatedestinationArgs);
//  PXOPT_ADD_MODE("-listdestination",    "", DISTTOOL_MODE_LISTDESTINATION, listdestinationArgs);

    PXOPT_ADD_MODE("-definetarget",       "", DISTTOOL_MODE_DEFINETARGET, definetargetArgs);
    PXOPT_ADD_MODE("-updatetarget",       "", DISTTOOL_MODE_UPDATETARGET, updatetargetArgs);
    PXOPT_ADD_MODE("-listtargets",         "", DISTTOOL_MODE_LISTTARGETS, listtargetsArgs);

    PXOPT_ADD_MODE("-defineinterest",     "", DISTTOOL_MODE_DEFINEINTEREST, defineinterestArgs);
    PXOPT_ADD_MODE("-updateinterest",     "", DISTTOOL_MODE_UPDATEINTEREST, updateinterestArgs);
    PXOPT_ADD_MODE("-listinterests",       "", DISTTOOL_MODE_LISTINTERESTS, listinterestsArgs);

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

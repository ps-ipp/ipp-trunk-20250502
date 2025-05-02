/*
 * pstamptoolConfig.c
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
#include "pstamptool.h"

pxConfig *pstamptoolConfig(pxConfig *config, int argc, char **argv)
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

    // -adddatastore
    psMetadata *adddatastoreArgs = psMetadataAlloc();
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-set_uri",          0, "define storage uri (required)", NULL);
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-set_out_product",  0, "define output product name (required)", NULL);
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-set_last_fileset", 0, "define last fileset seen", NULL);
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-set_state",        0, "define datastore state", "enabled");
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-set_label",        0, "define datastore label", NULL);
    psMetadataAddS32(adddatastoreArgs, PS_LIST_TAIL, "-set_poll_interval", 0, "define datastore poll interval (seconds)", 60);

    // -datastore
    psMetadata *datastoreArgs = psMetadataAlloc();
    psMetadataAddS64(datastoreArgs, PS_LIST_TAIL, "-ds_id", 0,            "define ds_id", 0);
    psMetadataAddBool(datastoreArgs, PS_LIST_TAIL, "-ready", 0,           "list data stores ready to be polled", false);
    psMetadataAddBool(datastoreArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -moddatastore
    psMetadata *moddatastoreArgs = psMetadataAlloc();
    psMetadataAddS64(moddatastoreArgs, PS_LIST_TAIL, "-ds_id", 0,            "define ds_id", 0);
    psMetadataAddStr(moddatastoreArgs, PS_LIST_TAIL, "-set_last_fileset", 0,     "set last_fileset seen", NULL);
    psMetadataAddStr(moddatastoreArgs, PS_LIST_TAIL, "-set_uri", 0,     "set uri for data store", NULL);
    psMetadataAddStr(moddatastoreArgs, PS_LIST_TAIL, "-set_state", 0,            "set state", NULL);
    psMetadataAddStr(moddatastoreArgs, PS_LIST_TAIL, "-set_label", 0,            "set label", NULL);
    psMetadataAddS32(moddatastoreArgs, PS_LIST_TAIL, "-set_poll_interval", 0, "define datastore poll interval (seconds)", 0);
    psMetadataAddBool(moddatastoreArgs, PS_LIST_TAIL, "-update_timestamp", 0, "update the timestamp", false);
    psMetadataAddStr(moddatastoreArgs, PS_LIST_TAIL, "-state", 0,            "search by state", NULL);

    // -addreq
    psMetadata *addreqArgs = psMetadataAlloc();
    psMetadataAddStr(addreqArgs, PS_LIST_TAIL, "-uri", 0,    "define request file uri (required)", NULL);
    psMetadataAddS64(addreqArgs, PS_LIST_TAIL, "-ds_id", 0,  "define request ds_id", 0);
    psMetadataAddS64(addreqArgs, PS_LIST_TAIL, "-proj_id", 0, "define request proj_id", 0);
    psMetadataAddStr(addreqArgs, PS_LIST_TAIL, "-name", 0,   "define request name", NULL);
    psMetadataAddStr(addreqArgs, PS_LIST_TAIL, "-username", 0, "define user name", NULL);
    psMetadataAddStr(addreqArgs, PS_LIST_TAIL, "-label", 0,  "define request label", NULL);

    // -pendingreq
    psMetadata *pendingreqArgs = psMetadataAlloc();
    psMetadataAddS64(pendingreqArgs, PS_LIST_TAIL, "-req_id", 0,            "define req_id", 0);
    psMetadataAddStr(pendingreqArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampRequest label (LIKE comparision)", NULL);
    psMetadataAddU64(pendingreqArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingreqArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -listreq
    psMetadata *listreqArgs = psMetadataAlloc();
    psMetadataAddS64(listreqArgs, PS_LIST_TAIL, "-req_id", 0,            "list by req_id", 0);
    psMetadataAddStr(listreqArgs, PS_LIST_TAIL, "-name", 0,              "list by name", NULL);
    psMetadataAddStr(listreqArgs, PS_LIST_TAIL, "-username", 0,          "list by user name", NULL);
    psMetadataAddStr(listreqArgs, PS_LIST_TAIL, "-state", 0,             "list by state", NULL);
    psMetadataAddStr(listreqArgs, PS_LIST_TAIL, "-label", 0,             "list by label", NULL);
    psMetadataAddS64(listreqArgs, PS_LIST_TAIL, "-not_req_id", 0,        "req_id to not list", 0);
    psMetadataAddU64(listreqArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(listreqArgs, PS_LIST_TAIL, "-simple", 0,           "use the simple output format", false);

    // -completedreq
    psMetadata *completedreqArgs = psMetadataAlloc();
    psMetadataAddStr(completedreqArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampJob label (LIKE comparision)", NULL);
    psMetadataAddU64(completedreqArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(completedreqArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -updatereq
    psMetadata *updatereqArgs = psMetadataAlloc();
    psMetadataAddS64(updatereqArgs, PS_LIST_TAIL, "-req_id", 0,       "req_id for which to update", 0);
    psMetadataAddS64(updatereqArgs, PS_LIST_TAIL, "-req_id_max", 0,   "maximum req_id for which to update", 0);
    psMetadataAddS16(updatereqArgs, PS_LIST_TAIL, "-fault", 0,        "search by fault code", 0);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-state", 0,        "search by state", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-reqType", 0,      "search by reqType", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-name", 0,         "search by reqType (LIKE comparsion)", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-username", 0,     "search by username", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampJob label (LIKE comparision)", NULL);
    psMetadataAddTime(updatereqArgs, PS_LIST_TAIL, "-timestamp_begin", 0, "search by timestamp (>=)", NULL);
    psMetadataAddTime(updatereqArgs, PS_LIST_TAIL, "-timestamp_end", 0, "search by timestamp (<=)", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_state", 0,        "new state", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_label", 0,        "new label", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_outProduct", 0,   "new outProduct", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_outdir", 0,   "new outdir", NULL);
    psMetadataAddS16(updatereqArgs, PS_LIST_TAIL, "-set_fault", 0,        "new fault code", 0);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_uri", 0,          "new uri", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_name", 0,         "new name", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_username", 0,     "new username", NULL);
    psMetadataAddStr(updatereqArgs, PS_LIST_TAIL, "-set_reqType", 0,      "new reqType", NULL);
    psMetadataAddBool(updatereqArgs, PS_LIST_TAIL, "-clearfault", 0,      "set fault to zero", false);

    // -revertreq
    psMetadata *revertreqArgs = psMetadataAlloc();
    psMetadataAddS64(revertreqArgs, PS_LIST_TAIL, "-req_id", 0,     "req_id to revert", 0);
    psMetadataAddS16(revertreqArgs, PS_LIST_TAIL, "-fault",  0,     "fault to revert", 0);
    psMetadataAddStr(revertreqArgs, PS_LIST_TAIL, "-state", 0,      "state to revert", NULL);
    psMetadataAddStr(revertreqArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampRequest label (LIKE comparision)", NULL);
    // this argument is supplied by pantasks so we accept it but don't use it
    psMetadataAddU64(revertreqArgs, PS_LIST_TAIL, "-limit", 0,      "not used", 0);

    // -pendingcleanup
    psMetadata *pendingcleanupArgs = psMetadataAlloc();
    psMetadataAddS64(pendingcleanupArgs, PS_LIST_TAIL, "-req_id", 0,   "define req_id", 0);
    psMetadataAddS64(pendingcleanupArgs, PS_LIST_TAIL, "-ds_id", 0,    "define ds_id", 0);
    psMetadataAddStr(pendingcleanupArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampRequest label (LIKE comparision)", NULL);
    psMetadataAddU64(pendingcleanupArgs, PS_LIST_TAIL, "-limit",  0,   "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanupArgs, PS_LIST_TAIL, "-simple", 0,  "use the simple output format", false);

    // -addjob
    psMetadata *addjobArgs = psMetadataAlloc();
    psMetadataAddS64(addjobArgs, PS_LIST_TAIL, "-req_id", 0,           "define job req_id", 0);
    psMetadataAddStr(addjobArgs, PS_LIST_TAIL, "-rownum", 0,           "define job rownum", NULL);
    psMetadataAddStr(addjobArgs, PS_LIST_TAIL, "-job_type", 0,         "define job job_type", "stamp");
    psMetadataAddStr(addjobArgs, PS_LIST_TAIL, "-outputBase", 0,       "define job outputBase", NULL);
    psMetadataAddStr(addjobArgs, PS_LIST_TAIL, "-state", 0,            "define state", "run");
    psMetadataAddS64(addjobArgs, PS_LIST_TAIL, "-exp_id", 0,           "define exposure id", 0);
    psMetadataAddS64(addjobArgs, PS_LIST_TAIL, "-options", 0,          "define options", 0);
    psMetadataAddS64(addjobArgs, PS_LIST_TAIL, "-dep_id", 0,           "define job dep_id", 0);
    psMetadataAddS64(addjobArgs, PS_LIST_TAIL, "-parent_id", 0,        "define parent's job_id", 0);
    psMetadataAddBool(addjobArgs, PS_LIST_TAIL, "-is_parent", 0,       "define whether job has children", false);
    psMetadataAddS16(addjobArgs, PS_LIST_TAIL, "-fault", 0,            "define job result", 0);

    // -listjob
    psMetadata *listjobArgs = psMetadataAlloc();
    psMetadataAddS64(listjobArgs, PS_LIST_TAIL, "-req_id", 0,          "select by request ID", 0);
    psMetadataAddS64(listjobArgs, PS_LIST_TAIL, "-job_id", 0,          "select by job ID", 0);
    psMetadataAddS64(listjobArgs, PS_LIST_TAIL, "-dep_id", 0,          "select by dependent ID", 0);
    psMetadataAddS16(listjobArgs, PS_LIST_TAIL, "-fault", 0,           "select by fault", 0);
    psMetadataAddStr(listjobArgs, PS_LIST_TAIL, "-jobType", 0,         "select by jobType", 0);
    psMetadataAddStr(listjobArgs, PS_LIST_TAIL, "-state", 0,           "select by job state", 0);
    psMetadataAddU64(listjobArgs, PS_LIST_TAIL, "-limit",  0,          "limit result set to N items", 0);
    psMetadataAddBool(listjobArgs, PS_LIST_TAIL, "-simple", 0,         "use the simple output format", false);

    // -pendingjob
    psMetadata *pendingjobArgs = psMetadataAlloc();
    psMetadataAddS64(pendingjobArgs, PS_LIST_TAIL, "-job_id", 0,            "define job", 0);
    psMetadataAddS64(pendingjobArgs, PS_LIST_TAIL, "-req_id", 0,            "define request", 0);
    psMetadataAddStr(pendingjobArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampJob label (LIKE comparision)", NULL);
    psMetadataAddU64(pendingjobArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingjobArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -updatejob
    psMetadata *updatejobArgs = psMetadataAlloc();
    psMetadataAddS64(updatejobArgs, PS_LIST_TAIL, "-req_id", 0,            "req_id of jobs to update", 0);
    psMetadataAddS64(updatejobArgs, PS_LIST_TAIL, "-job_id", 0,            "job_id of jobs to update", 0);
    psMetadataAddS64(updatejobArgs, PS_LIST_TAIL, "-dep_id", 0,            "dep_id of jobs to update", 0);
    psMetadataAddStr(updatejobArgs, PS_LIST_TAIL, "-state", 0,             "current state of jobs to update", 0);
    psMetadataAddS16(updatejobArgs, PS_LIST_TAIL, "-fault", 0,             "current value for job fault", 0);
    psMetadataAddStr(updatejobArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampJob label (LIKE comparision)", NULL);
    psMetadataAddS32(updatejobArgs, PS_LIST_TAIL, "-fault_count", 0,       "select by fault_count (>=)", 0);
    psMetadataAddStr(updatejobArgs, PS_LIST_TAIL, "-set_state", 0,            "new state", NULL);
    psMetadataAddS16(updatejobArgs, PS_LIST_TAIL, "-set_fault", 0,            "new result", 0);
    psMetadataAddU64(updatejobArgs, PS_LIST_TAIL, "-limit", 0,      "not used", 0);

    // -stopdependentjob
    psMetadata *stopdependentjobArgs = psMetadataAlloc();
    psMetadataAddS64(stopdependentjobArgs, PS_LIST_TAIL,  "-req_id", 0,       "req_id of jobs to update", 0);
    psMetadataAddS64(stopdependentjobArgs, PS_LIST_TAIL,  "-job_id", 0,       "job_id of jobs to update", 0);
    psMetadataAddS64(stopdependentjobArgs, PS_LIST_TAIL,  "-dep_id", 0,       "dep_id of jobs to update", 0);
    psMetadataAddS64(stopdependentjobArgs, PS_LIST_TAIL,  "-stage_id", 0,     "stage_id of jobs to update", 0);
    psMetadataAddStr(stopdependentjobArgs, PS_LIST_TAIL,  "-component", 0,    "component of jobs to update", NULL);
    psMetadataAddS16(stopdependentjobArgs, PS_LIST_TAIL,  "-fault", 0,        "current value for dependent fault", 0);
    psMetadataAddS32(stopdependentjobArgs, PS_LIST_TAIL,  "-fault_count", 0,   "select by fault_count (>=)", 0);
    psMetadataAddS16(stopdependentjobArgs, PS_LIST_TAIL,  "-set_fault", 0,    "new fault value for job and dependent (required)", 0);
    psMetadataAddStr(stopdependentjobArgs, PS_LIST_TAIL, "-set_state", 0,            "new pstampDependent.state", "new");
    psMetadataAddStr(stopdependentjobArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampJob label (LIKE comparision)", NULL);
    psMetadataAddU64(stopdependentjobArgs, PS_LIST_TAIL, "-limit", 0,      "not used", 0);

    // -revertjob
    psMetadata *revertjobArgs = psMetadataAlloc();
    psMetadataAddS64(revertjobArgs, PS_LIST_TAIL, "-req_id", 0,     "req_id to revert", 0);
    psMetadataAddS64(revertjobArgs, PS_LIST_TAIL, "-req_id_min", 0, "minimum req_id to revert", 0);
    psMetadataAddS64(revertjobArgs, PS_LIST_TAIL, "-job_id", 0,     "job_id to revert", 0);
    psMetadataAddS16(revertjobArgs, PS_LIST_TAIL, "-fault",  0,     "fault to revert", 0);
    psMetadataAddStr(revertjobArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by pstampRequest label (LIKE comparision)", NULL);
    psMetadataAddBool(revertjobArgs, PS_LIST_TAIL, "-clear_fault_count", 0,       "clear job fault count", false);
    psMetadataAddBool(revertjobArgs, PS_LIST_TAIL, "-all", 0,       "revert all faulted jobs", false);
    psMetadataAddU64(revertjobArgs, PS_LIST_TAIL, "-limit", 0,      "not used", 0);

    // -getdependent
    psMetadata *getdependentArgs = psMetadataAlloc();
    // arguabely all of these could be -set_ arguments, but since this mode is both a query and
    // a create if doesn't exist I like it this way
    psMetadataAddS64(getdependentArgs, PS_LIST_TAIL, "-stage_id", 0, "define stage id for dependent (required)", 0);
    psMetadataAddStr(getdependentArgs, PS_LIST_TAIL, "-stage", 0,    "define stage for dependent (required)", NULL);
    psMetadataAddStr(getdependentArgs, PS_LIST_TAIL, "-component", 0, "define component for depenent (required)", NULL);
    psMetadataAddStr(getdependentArgs, PS_LIST_TAIL, "-imagedb", 0,  "define imagedb for depenent (required)", NULL);
    psMetadataAddStr(getdependentArgs, PS_LIST_TAIL, "-rlabel", 0,   "define label for dependent ", NULL);
    psMetadataAddBool(getdependentArgs,PS_LIST_TAIL, "-need_magic", 0, "define need_magic", false);
    psMetadataAddStr(getdependentArgs, PS_LIST_TAIL, "-outdir", 0,    "define output directory for dependent (required)", NULL);
    psMetadataAddBool(getdependentArgs,PS_LIST_TAIL, "-hold", 0, "if creating new dependent set it's state to hold", false);
    psMetadataAddBool(getdependentArgs,PS_LIST_TAIL, "-no_create", 0, "if no matching dependent do not create one", false);

    // -updatedependent
    psMetadata *updatedependentArgs = psMetadataAlloc();
    psMetadataAddS64(updatedependentArgs, PS_LIST_TAIL, "-dep_id", 0, "define id for dependent (required)", 0);
    psMetadataAddStr(updatedependentArgs, PS_LIST_TAIL, "-set_state", 0, "new value for state", NULL);
    psMetadataAddS16(updatedependentArgs, PS_LIST_TAIL, "-set_fault",  0,   "new value for fault", 0);

    // -revertdependent
    psMetadata *revertdependentArgs = psMetadataAlloc();
    psMetadataAddS64(revertdependentArgs, PS_LIST_TAIL, "-dep_id", 0, "search by dep_id for dependent", 0);
    psMetadataAddS64(revertdependentArgs, PS_LIST_TAIL, "-job_id", 0, "search by job_ idfor dependent", 0);
    psMetadataAddS64(revertdependentArgs, PS_LIST_TAIL, "-req_id", 0, "search by req_id for dependent", 0);
    psMetadataAddStr(revertdependentArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK,   "define label for dependent ", NULL);
    psMetadataAddS16(revertdependentArgs, PS_LIST_TAIL, "-fault",  0, "search by dependent fault", 0);
    psMetadataAddBool(revertdependentArgs, PS_LIST_TAIL, "-clear_fault_count", 0,       "clear job fault count", false);
    psMetadataAddU64(revertdependentArgs, PS_LIST_TAIL, "-limit", 0,  "limit result set to N items", 0);

    // -pendingdependent
    psMetadata *pendingdependentArgs = psMetadataAlloc();
    psMetadataAddS64(pendingdependentArgs, PS_LIST_TAIL, "-stage_id", 0, "define id for dependent", 0);
    psMetadataAddStr(pendingdependentArgs, PS_LIST_TAIL, "-stage", 0,    "define stage for dependent", NULL);
    psMetadataAddStr(pendingdependentArgs, PS_LIST_TAIL, "-component", 0,    "define component for dependent", NULL);
    psMetadataAddS64(pendingdependentArgs, PS_LIST_TAIL, "-dep_id", 0, "define dep_id for dependent", 0);
    psMetadataAddS64(pendingdependentArgs, PS_LIST_TAIL, "-job_id", 0, "define job_id for dependent", 0);
    psMetadataAddS64(pendingdependentArgs, PS_LIST_TAIL, "-req_id", 0, "define eqp_id for dependent", 0);
    psMetadataAddStr(pendingdependentArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK,   "define label for pstampRequest", NULL);
    psMetadataAddStr(pendingdependentArgs, PS_LIST_TAIL, "-rlabel", 0,   "define label for dependent", NULL);
    psMetadataAddStr(pendingdependentArgs, PS_LIST_TAIL, "-imagedb", 0,  "define imagedb for depenent", NULL);
    psMetadataAddBool(pendingdependentArgs, PS_LIST_TAIL, "-includefaulted", 0,   "include faulted dependents in the output", false);
    psMetadataAddU64(pendingdependentArgs, PS_LIST_TAIL, "-limit",  0,    "limit result set to N items", 0);
    psMetadataAddBool(pendingdependentArgs, PS_LIST_TAIL, "-simple", 0,   "use the simple output format", false);

    // -addproject
    psMetadata *addprojectArgs = psMetadataAlloc();
    psMetadataAddStr(addprojectArgs, PS_LIST_TAIL, "-name",        0, "define project name (required)", NULL);
    psMetadataAddStr(addprojectArgs, PS_LIST_TAIL, "-state",        0, "define state for project (enabled, disabled)", "enabled");
    psMetadataAddStr(addprojectArgs, PS_LIST_TAIL, "-imagedb",      0, "define name of database for project (required)", NULL);
    psMetadataAddStr(addprojectArgs, PS_LIST_TAIL, "-dvodb",        0, "define name of dvo database for project", NULL);
    psMetadataAddStr(addprojectArgs, PS_LIST_TAIL, "-inst",        0, "define name of camera for project (required)", NULL);
    psMetadataAddStr(addprojectArgs, PS_LIST_TAIL, "-telescope",     0, "define name of telescope for project (required)", NULL);
    psMetadataAddBool(addprojectArgs, PS_LIST_TAIL, "-need_magic",   0, "define need_magic for project", false);

    // -modproject
    psMetadata *modprojectArgs = psMetadataAlloc();
    psMetadataAddS64(modprojectArgs, PS_LIST_TAIL, "-proj_id",      0, "define project ID to modify (required)", 0);
    psMetadataAddStr(modprojectArgs, PS_LIST_TAIL, "-imagedb",      0, "define name of database for project", NULL);
    psMetadataAddStr(modprojectArgs, PS_LIST_TAIL, "-state",        0, "define state for project (enabled, disabled)", NULL);
    psMetadataAddStr(modprojectArgs, PS_LIST_TAIL, "-dvodb",        0, "define name of dvo database for project", NULL);
    psMetadataAddStr(modprojectArgs, PS_LIST_TAIL, "-camera",        0, "define name of camera for project", NULL);
    psMetadataAddStr(modprojectArgs, PS_LIST_TAIL, "-telescope",     0, "define name of telescope for project", NULL);
    psMetadataAddBool(modprojectArgs, PS_LIST_TAIL, "-need_magic",   0, "define need_magic for project", false);

    // -project
    psMetadata *projectArgs = psMetadataAlloc();
    psMetadataAddStr(projectArgs, PS_LIST_TAIL, "-name", 0, "define project name to list", NULL);
    psMetadataAddBool(projectArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -getwebrequestnum
    psMetadata *getwebrequestnumArgs = psMetadataAlloc();

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    // -addfile
    psMetadata *addfileArgs = psMetadataAlloc();
    psMetadataAddS64(addfileArgs, PS_LIST_TAIL, "-job_id",       0, "define job ID for file (required)", 0);
    psMetadataAddStr(addfileArgs, PS_LIST_TAIL, "-path",         0, "define path for file (required)", NULL);

    // -listfile
    psMetadata *listfileArgs = psMetadataAlloc();
    psMetadataAddS64(listfileArgs, PS_LIST_TAIL, "-file_id",     0, "select by file ID", 0);
    psMetadataAddS64(listfileArgs, PS_LIST_TAIL, "-job_id",      0, "select by job ID", 0);
    psMetadataAddS64(listfileArgs, PS_LIST_TAIL, "-req_id",      0, "select by request ID", 0);
    psMetadataAddU64(listfileArgs, PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listfileArgs, PS_LIST_TAIL, "-simple",     0, "use the simple output format", false);

    // -deletefile
    psMetadata *deletefileArgs = psMetadataAlloc();
    psMetadataAddS64(deletefileArgs, PS_LIST_TAIL, "-job_id",      0, "select by job ID (required)", 0);

    // Access control tables

    // -adddomain
    psMetadata *adddomainArgs = psMetadataAlloc();
    psMetadataAddStr(adddomainArgs, PS_LIST_TAIL, "-set_domain",   0, "define domain name (required)", NULL);
    psMetadataAddS32(adddomainArgs, PS_LIST_TAIL, "-set_accessLevel", 0, "define access level for this domain (required)", 0);
    psMetadataAddStr(adddomainArgs, PS_LIST_TAIL, "-set_defaultProduct",   0, "define default product", NULL);
    psMetadataAddStr(adddomainArgs, PS_LIST_TAIL, "-set_defaultLabel",   0, "define default Label", NULL);

    // -updatedomain
    psMetadata *updatedomainArgs = psMetadataAlloc();
    psMetadataAddStr(updatedomainArgs, PS_LIST_TAIL, "-domain",   0, "define domain name (required)", NULL);
    psMetadataAddS32(updatedomainArgs, PS_LIST_TAIL, "-set_accessLevel", 0, "define access level for this domain", 0);
    psMetadataAddStr(updatedomainArgs, PS_LIST_TAIL, "-set_defaultProduct",   0, "define default product", NULL);
    psMetadataAddStr(updatedomainArgs, PS_LIST_TAIL, "-set_defaultLabel",   0, "define default Label", NULL);

    // -listdomain
    psMetadata *listdomainArgs = psMetadataAlloc();
    psMetadataAddStr(listdomainArgs, PS_LIST_TAIL, "-domain",   0, "define domain name", NULL);
    psMetadataAddS32(listdomainArgs, PS_LIST_TAIL, "-accessLevel", 0, "select by  access level", 0);
    psMetadataAddU64(listdomainArgs, PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listdomainArgs, PS_LIST_TAIL, "-simple",     0, "use the simple output format", false);

    // -adduser
    psMetadata *adduserArgs = psMetadataAlloc();
    psMetadataAddStr(adduserArgs, PS_LIST_TAIL, "-set_user",   0, "define user name (required)", NULL);
    psMetadataAddStr(adduserArgs, PS_LIST_TAIL, "-set_domain",   0, "define user's domain name (required)", NULL);
    psMetadataAddS32(adduserArgs, PS_LIST_TAIL, "-set_accessLevel", 0, "define maximum access level for this user", 0);
    psMetadataAddStr(adduserArgs, PS_LIST_TAIL, "-set_defaultProduct",      0, "define default data store product for this user", NULL);
    psMetadataAddStr(adduserArgs, PS_LIST_TAIL, "-set_defaultLabel",      0, "define default label for requests from this user", NULL);

    // -updateuser
    psMetadata *updateuserArgs = psMetadataAlloc();
    psMetadataAddStr(updateuserArgs, PS_LIST_TAIL, "-user",   0, "select user to update (required)", NULL);
    psMetadataAddStr(updateuserArgs, PS_LIST_TAIL, "-domain",   0, "select domain of user to update (required)", NULL);
    psMetadataAddS32(updateuserArgs, PS_LIST_TAIL, "-set_accessLevel", 0, "define access level for this domain", 0);
    psMetadataAddStr(updateuserArgs, PS_LIST_TAIL, "-set_defaultProduct",   0, "define default product", NULL);
    psMetadataAddStr(updateuserArgs, PS_LIST_TAIL, "-set_defaultLabel",   0, "define default Label", NULL);

    // -listuser
    psMetadata *listuserArgs = psMetadataAlloc();
    psMetadataAddStr(listuserArgs, PS_LIST_TAIL, "-user",   0, "select by user name", NULL);
    psMetadataAddStr(listuserArgs, PS_LIST_TAIL, "-domain",   0, "select by user's domain name", NULL);
    psMetadataAddS32(listuserArgs, PS_LIST_TAIL, "-accessLevel", 0, "select by maximum access level", 0);
    psMetadataAddU64(listuserArgs, PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listuserArgs, PS_LIST_TAIL, "-simple",     0, "use the simple output format", false);

    // -addaccesslevel
    psMetadata *addaccesslevelArgs = psMetadataAlloc();
    psMetadataAddStr(addaccesslevelArgs, PS_LIST_TAIL, "-set_project_name",   0, "define project name", NULL);
    psMetadataAddS64(addaccesslevelArgs, PS_LIST_TAIL, "-set_proj_id", 0, "define project ID", 0);
    psMetadataAddS32(addaccesslevelArgs, PS_LIST_TAIL, "-set_accessLevel", 0, "define access level (required)", 0);
    psMetadataAddF32(addaccesslevelArgs, PS_LIST_TAIL, "-set_mjd_min", 0, "define mjd_min", 0.0);
    psMetadataAddF32(addaccesslevelArgs, PS_LIST_TAIL, "-set_mjd_max", 0, "define mjd_max", 0.0);

    // -listaccesslevel
    psMetadata *listaccesslevelArgs = psMetadataAlloc();
    psMetadataAddStr(listaccesslevelArgs, PS_LIST_TAIL, "-project_name",   0, "search by project name", NULL);
    psMetadataAddS64(listaccesslevelArgs, PS_LIST_TAIL, "-proj_id", 0, "select by  project ID", 0);
    psMetadataAddS32(listaccesslevelArgs, PS_LIST_TAIL, "-accessLevel", 0, "select by  access level", 0);
    psMetadataAddU64(listaccesslevelArgs, PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listaccesslevelArgs, PS_LIST_TAIL, "-simple",     0, "use the simple output format", false);

    // -updateaccesslevel
    psMetadata *updateaccesslevelArgs = psMetadataAlloc();
    psMetadataAddStr(updateaccesslevelArgs, PS_LIST_TAIL, "-project_name",   0, "select by project name", NULL);
    psMetadataAddS64(updateaccesslevelArgs, PS_LIST_TAIL, "-proj_id", 0, "select by project ID", 0);
    psMetadataAddS32(updateaccesslevelArgs, PS_LIST_TAIL, "-accessLevel", 0, "select by access level (required)", 0);
    psMetadataAddF32(updateaccesslevelArgs, PS_LIST_TAIL, "-set_mjd_min", 0, "define new mjd_min", NAN);
    psMetadataAddF32(updateaccesslevelArgs, PS_LIST_TAIL, "-set_mjd_max", 0, "define new mjd_max", NAN);

    PXOPT_ADD_MODE("-addreq",          "", PSTAMPTOOL_MODE_ADDREQ,       addreqArgs);
    PXOPT_ADD_MODE("-pendingreq",      "", PSTAMPTOOL_MODE_PENDINGREQ,   pendingreqArgs);
    PXOPT_ADD_MODE("-updatereq",       "", PSTAMPTOOL_MODE_UPDATEREQ, updatereqArgs);
    PXOPT_ADD_MODE("-listreq",         "", PSTAMPTOOL_MODE_LISTREQ,      listreqArgs);
    PXOPT_ADD_MODE("-completedreq",    "", PSTAMPTOOL_MODE_COMPLETEDREQ, completedreqArgs);
    PXOPT_ADD_MODE("-revertreq",       "", PSTAMPTOOL_MODE_REVERTREQ,    revertreqArgs);
    PXOPT_ADD_MODE("-pendingcleanup",  "", PSTAMPTOOL_MODE_PENDINGCLEANUP, pendingcleanupArgs);

    PXOPT_ADD_MODE("-addjob",          "", PSTAMPTOOL_MODE_ADDJOB,       addjobArgs);
    PXOPT_ADD_MODE("-listjob",         "", PSTAMPTOOL_MODE_LISTJOB,      listjobArgs);
    PXOPT_ADD_MODE("-pendingjob",      "", PSTAMPTOOL_MODE_PENDINGJOB,   pendingjobArgs);
    PXOPT_ADD_MODE("-updatejob",       "", PSTAMPTOOL_MODE_UPDATEJOB,    updatejobArgs);
    PXOPT_ADD_MODE("-stopdependentjob", "", PSTAMPTOOL_MODE_STOPDEPENDENTJOB,  stopdependentjobArgs);
    PXOPT_ADD_MODE("-revertjob",       "", PSTAMPTOOL_MODE_REVERTJOB,    revertjobArgs);

    PXOPT_ADD_MODE("-adddatastore",    "", PSTAMPTOOL_MODE_ADDDATASTORE, adddatastoreArgs);
    PXOPT_ADD_MODE("-datastore",       "", PSTAMPTOOL_MODE_DATASTORE,    datastoreArgs);
    PXOPT_ADD_MODE("-moddatastore",    "", PSTAMPTOOL_MODE_MODDATASTORE, moddatastoreArgs);

    PXOPT_ADD_MODE("-getdependent",    "", PSTAMPTOOL_MODE_GETDEPENDENT, getdependentArgs);
    PXOPT_ADD_MODE("-updatedependent", "", PSTAMPTOOL_MODE_UPDATEDEPENDENT, updatedependentArgs);
    PXOPT_ADD_MODE("-pendingdependent","", PSTAMPTOOL_MODE_PENDINGDEPENDENT, pendingdependentArgs);
    PXOPT_ADD_MODE("-revertdependent","", PSTAMPTOOL_MODE_REVERTDEPENDENT, revertdependentArgs);

    PXOPT_ADD_MODE("-addproject",      "", PSTAMPTOOL_MODE_ADDPROJECT, addprojectArgs);
    PXOPT_ADD_MODE("-modproject",      "", PSTAMPTOOL_MODE_MODPROJECT, modprojectArgs);
    PXOPT_ADD_MODE("-project",         "", PSTAMPTOOL_MODE_PROJECT,    projectArgs);
    PXOPT_ADD_MODE("-getwebrequestnum","", PSTAMPTOOL_MODE_GETWEBREQUESTNUM,   getwebrequestnumArgs);
    PXOPT_ADD_MODE("-addfile",         "", PSTAMPTOOL_MODE_ADDFILE,   addfileArgs);
    PXOPT_ADD_MODE("-listfile",        "", PSTAMPTOOL_MODE_LISTFILE,   listfileArgs);
    PXOPT_ADD_MODE("-deletefile",      "", PSTAMPTOOL_MODE_DELETEFILE,   deletefileArgs);

    PXOPT_ADD_MODE("-adddomain",       "", PSTAMPTOOL_MODE_ADDDOMAIN, adddomainArgs);
    PXOPT_ADD_MODE("-updatedomain",    "", PSTAMPTOOL_MODE_UPDATEDOMAIN, updatedomainArgs);
    PXOPT_ADD_MODE("-listdomain",      "", PSTAMPTOOL_MODE_LISTDOMAIN, listdomainArgs);
    PXOPT_ADD_MODE("-adduser",         "", PSTAMPTOOL_MODE_ADDUSER, adduserArgs);
    PXOPT_ADD_MODE("-updateuser",      "", PSTAMPTOOL_MODE_UPDATEUSER, updateuserArgs);
    PXOPT_ADD_MODE("-listuser",        "", PSTAMPTOOL_MODE_LISTUSER, listuserArgs);
    PXOPT_ADD_MODE("-addaccesslevel",  "", PSTAMPTOOL_MODE_ADDACCESSLEVEL, addaccesslevelArgs);
    PXOPT_ADD_MODE("-listaccesslevel", "", PSTAMPTOOL_MODE_LISTACCESSLEVEL, listaccesslevelArgs);

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

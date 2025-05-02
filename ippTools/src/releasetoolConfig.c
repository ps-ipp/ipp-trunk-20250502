/*
 * releasetoolConfig.c
 *
 * Copyright (C) 2013 IfA University of Hawaii
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
#include "pxspace.h"
#include "releasetool.h"

pxConfig *releasetoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    config->modules = pmConfigRead(&argc, argv, NULL);
    if (! config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    // -definesurvey
    psMetadata *definesurveyArgs = psMetadataAlloc();
    psMetadataAddS32(definesurveyArgs, PS_LIST_TAIL, "-set_surveyID",   0, "define survey ID (required)", 0);
    psMetadataAddStr(definesurveyArgs, PS_LIST_TAIL, "-set_surveyName", 0, "define survey name (required)", NULL);
    psMetadataAddStr(definesurveyArgs, PS_LIST_TAIL, "-set_description", 0, "define survey description", NULL);

    // -listsurvey
    psMetadata *listsurveyArgs = psMetadataAlloc();
    psMetadataAddStr(listsurveyArgs,  PS_LIST_TAIL, "-surveyName", 0, "select by survey name (LIKE comparision)", NULL);
    psMetadataAddU64(listsurveyArgs,  PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listsurveyArgs, PS_LIST_TAIL, "-simple",      0, "use the simple output format", false);

    // -definerelease
    psMetadata *definereleaseArgs = psMetadataAlloc();
    psMetadataAddS32(definereleaseArgs, PS_LIST_TAIL, "-set_surveyID", 0,      "define survey ID (required)", 0);
    psMetadataAddStr(definereleaseArgs, PS_LIST_TAIL, "-set_release_name", 0,   "define release name (required)", NULL);
    psMetadataAddS32(definereleaseArgs, PS_LIST_TAIL, "-set_dataRelease", 0,   "define dataRelease", -1);
    psMetadataAddS32(definereleaseArgs, PS_LIST_TAIL, "-set_accessLevelMin", 0,   "define accessLevelMin", 0);
    psMetadataAddStr(definereleaseArgs, PS_LIST_TAIL, "-set_release_state", 0, "define release state", NULL);
    psMetadataAddS32(definereleaseArgs, PS_LIST_TAIL, "-set_priority", 0,      "define release priority", 0);
    psMetadataAddStr(definereleaseArgs, PS_LIST_TAIL, "-set_dvodb", 0,         "define dvo db name", NULL);
    psMetadataAddStr(definereleaseArgs, PS_LIST_TAIL, "-set_ubercal_file", 0,  "define ubercal file name", NULL);

    // -updaterelease
    psMetadata *updatereleaseArgs = psMetadataAlloc();

    // release_name and/or rel_id is required. This is handled in updatereleaseMode
    psMetadataAddStr(updatereleaseArgs, PS_LIST_TAIL, "-release_name", 0,        "select by release name", NULL);
    psMetadataAddS32(updatereleaseArgs, PS_LIST_TAIL, "-rel_id", 0,             "select by release ID", 0);

    psMetadataAddStr(updatereleaseArgs, PS_LIST_TAIL, "-set_release_state", 0,  "define new release state", NULL);
    psMetadataAddS32(updatereleaseArgs, PS_LIST_TAIL, "-set_priority", 0,       "define new release priority", 0);
    psMetadataAddS32(updatereleaseArgs, PS_LIST_TAIL, "-set_dataRelease", 0,    "define data release", -1);
    psMetadataAddS32(updatereleaseArgs, PS_LIST_TAIL, "-set_accessLevelMin", 0,   "define accessLevelMin", -1);
    psMetadataAddStr(updatereleaseArgs, PS_LIST_TAIL, "-set_dvodb", 0,         "define new dvo db name", NULL);
    psMetadataAddStr(updatereleaseArgs, PS_LIST_TAIL, "-set_ubercal_file", 0,  "define new ubercal file name", NULL);


    // -listrelease
    psMetadata *listreleaseArgs = psMetadataAlloc();
    psMetadataAddStr(listreleaseArgs,  PS_LIST_TAIL, "-surveyName", 0,  "select by survey name (LIKE comparision)", NULL);
    psMetadataAddStr(listreleaseArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);
    psMetadataAddStr(listreleaseArgs,  PS_LIST_TAIL, "-release_state",  0, "select by release state", NULL);
    psMetadataAddS32(listreleaseArgs, PS_LIST_TAIL,  "-rel_id", 0,      "select by release ID", 0);

    psMetadataAddU64(listreleaseArgs,  PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listreleaseArgs, PS_LIST_TAIL, "-simple",      0, "use the simple output format", false);

    // -definerelexp
    psMetadata *definerelexpArgs = psMetadataAlloc();

        // set the target release
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-release_name", 0, "define release name", NULL);
    psMetadataAddS32(definerelexpArgs, PS_LIST_TAIL,  "-rel_id", 0,      "define release ID", 0);

        // select the processing
    psMetadataAddS64(definerelexpArgs, PS_LIST_TAIL,  "-cam_id", 0,      "select by cam ID", 0);
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-label",  0,      "select by camRun.label", NULL);
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-data_group", 0,  "select by camRun.data_group", NULL);
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-filter", 0,      "select by filter", NULL);
        // select a specific exposure from the "processing"
    psMetadataAddS64(definerelexpArgs, PS_LIST_TAIL,  "-exp_id", 0,      "select by exposure ID", 0);
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-exp_name", 0,    "select by exposure name", NULL);

        // parameters of the relExp
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-set_state", 0,      "define state (required)", NULL);
    psMetadataAddU32(definerelexpArgs,  PS_LIST_TAIL, "-set_flags", 0,      "define flags", 0);
    psMetadataAddF32(definerelexpArgs,  PS_LIST_TAIL, "-set_zpt_obs", 0,    "define zero point", NAN);
    psMetadataAddF32(definerelexpArgs,  PS_LIST_TAIL, "-set_zpt_stdev", 0,  "define zero point stdev", NAN);
    psMetadataAddF32(definerelexpArgs,  PS_LIST_TAIL, "-set_mcal", 0,       "define mcal", NAN);
    psMetadataAddS32(definerelexpArgs,  PS_LIST_TAIL, "-set_ubercal_dist", 0,  "define ubercal dist", 0);
    psMetadataAddStr(definerelexpArgs,  PS_LIST_TAIL, "-set_path_base", 0,  "define state", NULL);
    psMetadataAddS16(definerelexpArgs,  PS_LIST_TAIL, "-set_fault", 0,      "define fault", 0);

    psMetadataAddBool(definerelexpArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(definerelexpArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -updaterelexp
    psMetadata *updaterelexpArgs = psMetadataAlloc();
    psMetadataAddS64(updaterelexpArgs, PS_LIST_TAIL,  "-relexp_id", 0,   "select by released exposure ID", 0);
    psMetadataAddS64(updaterelexpArgs, PS_LIST_TAIL,  "-exp_id", 0,      "select by exposure ID", 0);
    psMetadataAddStr(updaterelexpArgs,  PS_LIST_TAIL, "-exp_name", 0,    "select by exposure name", NULL);
    psMetadataAddS32(updaterelexpArgs, PS_LIST_TAIL,  "-rel_id", 0,      "select by release ID", 0);
    psMetadataAddStr(updaterelexpArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);

    psMetadataAddStr(updaterelexpArgs,  PS_LIST_TAIL, "-set_state", 0,      "define new state)", NULL);
    psMetadataAddU32(updaterelexpArgs,  PS_LIST_TAIL, "-set_flags", 0,      "define new flags", 0);
    psMetadataAddF32(updaterelexpArgs,  PS_LIST_TAIL, "-set_zpt_obs", 0,    "define new zero point", NAN);
    psMetadataAddF32(updaterelexpArgs,  PS_LIST_TAIL, "-set_zpt_stdev", 0,  "define new zero point stdev", NAN);
    psMetadataAddF32(updaterelexpArgs,  PS_LIST_TAIL, "-set_mcal", 0,       "define mcal", NAN);
    psMetadataAddS32(updaterelexpArgs,  PS_LIST_TAIL, "-set_ubercal_dist", 0, "define new ubercal dist", 0);
    psMetadataAddStr(updaterelexpArgs,  PS_LIST_TAIL, "-set_path_base", 0,  "define path_base", NULL);
    psMetadataAddS16(updaterelexpArgs,  PS_LIST_TAIL, "-set_fault", 0,      "define new fault", 0);
    psMetadataAddBool(updaterelexpArgs, PS_LIST_TAIL, "-clearfault",  0,   "set fault to zero", false);

    // -tocalibexp
    psMetadata *tocalibexpArgs = psMetadataAlloc();
    psMetadataAddS64(tocalibexpArgs, PS_LIST_TAIL,  "-relexp_id", 0,   "select by released exposure ID", 0);
    psMetadataAddStr(tocalibexpArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);

    psMetadataAddU64(tocalibexpArgs,  PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(tocalibexpArgs, PS_LIST_TAIL, "-simple",      0, "use the simple output format", false);

    // -listrelexp
    psMetadata *listrelexpArgs = psMetadataAlloc();
    psMetadataAddS64(listrelexpArgs, PS_LIST_TAIL,  "-relexp_id", 0,   "select by released exposure ID", 0);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-release_state", PS_META_DUPLICATE_OK, "select by release state", NULL);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-state", 0,        "select by released exposure state", NULL);

    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-filter", 0,       "select by exposure name (LIKE comparison)", NULL);
    psMetadataAddTime(listrelexpArgs, PS_LIST_TAIL, "-dateobs_begin", 0,"search for exposures by time (>=)", NULL);
    psMetadataAddTime(listrelexpArgs, PS_LIST_TAIL, "-dateobs_end", 0,  "search for exposures by time (<=)", NULL);

    pxspaceAddArguments(listrelexpArgs);

    psMetadataAddF32(listrelexpArgs,  PS_LIST_TAIL, "-fwhm_min", 0, "search by measured seeing (>=)", NAN);
    psMetadataAddF32(listrelexpArgs,  PS_LIST_TAIL, "-fwhm_max", 0, "search by seeing (<=)", NAN);

    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-exp_name", 0, "select by exposure name", NULL);

    psMetadataAddS64(listrelexpArgs,  PS_LIST_TAIL, "-exp_id", 0, "select by exposure ID", 0);
    psMetadataAddS64(listrelexpArgs,  PS_LIST_TAIL, "-chip_id", 0, "select by chip ID", 0);
    psMetadataAddS64(listrelexpArgs,  PS_LIST_TAIL, "-cam_id", 0, "select by cam ID", 0);
    psMetadataAddS64(listrelexpArgs,  PS_LIST_TAIL, "-warp_id", 0, "select by warp ID", 0);

    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-chip_data_group", 0, "chipRun.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-cam_data_group", 0, "camRun.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-warp_data_group", 0, "warpRun.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-skycell_id", 0, "select by skycell", NULL);
    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-tess_id", 0,    "warpRun.tess_id", NULL);

    psMetadataAddStr(listrelexpArgs,  PS_LIST_TAIL, "-surveyName", 0, "select by survey name (LIKE comparision)", NULL);
    psMetadataAddS64(listrelexpArgs,  PS_LIST_TAIL, "-rel_id", 0, "select by release ID", 0);

    psMetadataAddBool(listrelexpArgs, PS_LIST_TAIL, "-priority_order",   0, "order by release priority", false);

    psMetadataAddU64(listrelexpArgs,  PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listrelexpArgs, PS_LIST_TAIL, "-simple",      0, "use the simple output format", false);

    // -deleterelexp
    psMetadata *deleterelexpArgs = psMetadataAlloc();
    psMetadataAddS64(deleterelexpArgs, PS_LIST_TAIL,  "-relexp_id", 0,   "select by released exposure ID", 0);
    psMetadataAddS64(deleterelexpArgs, PS_LIST_TAIL,  "-exp_id", 0,      "select by exposure ID", 0);
    psMetadataAddStr(deleterelexpArgs,  PS_LIST_TAIL, "-exp_name", 0,    "select by exposure name", NULL);
    psMetadataAddS32(deleterelexpArgs, PS_LIST_TAIL,  "-rel_id", 0,      "select by release ID", 0);
    psMetadataAddS32(deleterelexpArgs, PS_LIST_TAIL,  "-cam_id", 0,      "select by cam run ID", 0);
    psMetadataAddStr(deleterelexpArgs,  PS_LIST_TAIL, "-label", 0,       "select by camRun.label", NULL);
    psMetadataAddStr(deleterelexpArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);


    // -definerelstack
    psMetadata *definerelstackArgs = psMetadataAlloc();

        // set the target release
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-release_name", 0, "define release name", NULL);
    psMetadataAddS32(definerelstackArgs, PS_LIST_TAIL,  "-rel_id", 0,      "define release ID", 0);

        // select the processing
    psMetadataAddS64(definerelstackArgs, PS_LIST_TAIL,  "-stack_id", 0,    "select by stack ID", 0);
    psMetadataAddS64(definerelstackArgs, PS_LIST_TAIL,  "-skycal_id", 0,   "select by skycal ID", 0);
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-label",  0,      "select by stackRun.label", NULL);
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-data_group", 0,  "select by stackRun.data_group", NULL);
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-skycal_label",  0, "select by skycalRun.label", NULL);
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-skycal_data_group", 0,  "select by skycalRun.data_group", NULL);

        // parameters of the relStack
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-set_state", 0,      "define state (required)", NULL);
    psMetadataAddU32(definerelstackArgs,  PS_LIST_TAIL, "-set_flags", 0,      "define flags", 0);
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-set_stack_type", 0, "define stack type (required)", NULL);
    psMetadataAddF32(definerelstackArgs,  PS_LIST_TAIL, "-set_zpt_obs", 0,    "define zero point", NAN);
    psMetadataAddF32(definerelstackArgs,  PS_LIST_TAIL, "-set_zpt_stdev", 0,  "define zero point stdev", NAN);
    psMetadataAddF32(definerelstackArgs,  PS_LIST_TAIL, "-set_fwhm_major", 0, "define fwhm_major", NAN);
    psMetadataAddStr(definerelstackArgs,  PS_LIST_TAIL, "-set_path_base", 0,  "define state", NULL);
    psMetadataAddS16(definerelstackArgs,  PS_LIST_TAIL, "-set_fault", 0,      "define fault", 0);

    psMetadataAddBool(definerelstackArgs, PS_LIST_TAIL, "-pretend", 0, "do not actually modify the database", false);
    psMetadataAddBool(definerelstackArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddU64(definerelstackArgs, PS_LIST_TAIL,  "-limit",   0, "limit result set to N items", 0);

    // -listrelstack
    psMetadata *listrelstackArgs = psMetadataAlloc();
    psMetadataAddS64(listrelstackArgs, PS_LIST_TAIL,  "-relstack_id", 0,   "select by released exposure ID", 0);
    psMetadataAddS64(listrelstackArgs, PS_LIST_TAIL,  "-stack_id", 0,   "select by stack ID", 0);
    psMetadataAddS64(listrelstackArgs, PS_LIST_TAIL,  "-skycal_id", 0,   "select by skycal ID", 0);
    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);
    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-release_state", PS_META_DUPLICATE_OK, "select by release state", NULL);
    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-state", 0,        "select by released stack state", NULL);

    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-filter", 0,       "select by filter name (LIKE comparison)", NULL);
    // psMetadataAddTime(listrelstackArgs, PS_LIST_TAIL, "-dateobs_begin", 0,"search for exposures by time (>=)", NULL);
    // psMetadataAddTime(listrelstackArgs, PS_LIST_TAIL, "-dateobs_end", 0,  "search for exposures by time (<=)", NULL);

    pxskycellAddArguments(listrelstackArgs);

    psMetadataAddF32(listrelstackArgs,  PS_LIST_TAIL, "-fwhm_min", 0, "search by measured seeing (>=)", NAN);
    psMetadataAddF32(listrelstackArgs,  PS_LIST_TAIL, "-fwhm_max", 0, "search by seeing (<=)", NAN);

    psMetadataAddF32(listrelstackArgs,  PS_LIST_TAIL, "-mjd_min", 0, "search by MJD seeing (>=)", NAN);
    psMetadataAddF32(listrelstackArgs,  PS_LIST_TAIL, "-mjd_max", 0, "search by MJD (<=)", NAN);

    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-tess_id", 0, "select by tess_id", NULL);
    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-skycell_id", 0, "select by skycell_id (LIKE comparision)", NULL);
    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-stack_type", PS_META_DUPLICATE_OK, "select by stack_type", NULL);

    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-stack_data_group", 0, "select by stackRun.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-skycal_data_group", 0, "select by skycalRun.data_group (LIKE comparison)", NULL);

    psMetadataAddStr(listrelstackArgs,  PS_LIST_TAIL, "-surveyName", 0, "select by survey name (LIKE comparision)", NULL);
    psMetadataAddS64(listrelstackArgs,  PS_LIST_TAIL, "-rel_id", 0, "select by release ID", 0);

    psMetadataAddBool(listrelstackArgs, PS_LIST_TAIL, "-priority_order",   0, "order by release priority", false);

    psMetadataAddU64(listrelstackArgs,  PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(listrelstackArgs, PS_LIST_TAIL, "-simple",      0, "use the simple output format", false);

    psMetadata *calibraterelstackArgs = psMetadataAlloc();

        // set the target release and collection of relStacks (one of these is required)
    psMetadataAddStr(calibraterelstackArgs,  PS_LIST_TAIL, "-release_name", 0, "define release name", NULL);
    psMetadataAddS32(calibraterelstackArgs, PS_LIST_TAIL,  "-rel_id", 0,      "define release ID", 0);

        // select the processing
    psMetadataAddStr(calibraterelstackArgs,  PS_LIST_TAIL, "-skycal_label",  0, "select by skycalRun.label", NULL);
    psMetadataAddStr(calibraterelstackArgs,  PS_LIST_TAIL, "-skycal_data_group", 0,  "select by skycalRun.data_group", NULL);
    psMetadataAddS64(calibraterelstackArgs, PS_LIST_TAIL,  "-skycal_id", 0,      "select by skycalRun.skycal_id", 0);

    psMetadataAddStr(calibraterelstackArgs,  PS_LIST_TAIL, "-tess_id", 0,  "select by tess_id", NULL);
    psMetadataAddStr(calibraterelstackArgs,  PS_LIST_TAIL, "-skycell_id", 0,  "select by skycell_id", NULL);
    pxskycellAddArguments(calibraterelstackArgs);

    psMetadataAddBool(calibraterelstackArgs, PS_LIST_TAIL, "-replace",   0, "replace any existing calibrations", false);
    psMetadataAddU64(calibraterelstackArgs, PS_LIST_TAIL,  "-limit",   0, "limit result set to N items", 0);

    // -deleterelstack
    psMetadata *deleterelstackArgs = psMetadataAlloc();
    psMetadataAddS64(deleterelstackArgs, PS_LIST_TAIL,  "-relstack_id", 0,   "select by released exposure ID", 0);
    psMetadataAddS64(deleterelstackArgs, PS_LIST_TAIL,  "-stack_id", 0,   "select by stack ID", 0);
//    psMetadataAddS64(deleterelstackArgs, PS_LIST_TAIL,  "-skycal_id", 0,   "select by skycal ID", 0);
    psMetadataAddStr(deleterelstackArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);
    psMetadataAddS32(deleterelstackArgs, PS_LIST_TAIL,  "-rel_id", 0,   "select by release ID", 0);

    psMetadataAddStr(deleterelstackArgs,  PS_LIST_TAIL, "-label",  0,      "select by stackRun.label", NULL);
//    psMetadataAddStr(deleterelstackArgs,  PS_LIST_TAIL, "-data_group", 0,  "select by stackRun.data_group", NULL);
//    psMetadataAddStr(deleterelstackArgs,  PS_LIST_TAIL, "-skycal_label",  0, "select by skycalRun.label", NULL);
//    psMetadataAddStr(deleterelstackArgs,  PS_LIST_TAIL, "-skycal_data_group", 0,  "select by skycalRun.data_group", NULL);

    // -summary
    psMetadata *summaryArgs = psMetadataAlloc();
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL,  "-sass_id", 0,   "select by released SASS ID", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL,  "-relstack_id", 0,   "select by released exposure ID", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL,  "-stack_id", 0,   "select by stack ID", 0);
//    psMetadataAddS64(summaryArgs, PS_LIST_TAIL,  "-skycal_id", 0,   "select by skycal ID", 0);
    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-release_name", 0, "select by release name (LIKE comparision)", NULL);
    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-release_state", PS_META_DUPLICATE_OK, "select by release state", NULL);
//    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-state", 0,        "select by released stack state", NULL);

    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-filter", 0,       "select by filter name (LIKE comparison)", NULL);

    pxskycellAddArguments(summaryArgs);

    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-tess_id", 0, "select by tess_id", NULL);
    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-skycell_id", 0, "select by skycell_id (LIKE comparision)", NULL);
    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-projection_cell", 0, "select by projection_cell (LIKE comparision)", NULL);
    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-stack_type", PS_META_DUPLICATE_OK, "select by stack_type", NULL);

    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-stack_data_group", 0, "select by stackRun.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-data_group", 0, "select by stackAsociation.data_group (LIKE comparison)", NULL);

    psMetadataAddStr(summaryArgs,  PS_LIST_TAIL, "-surveyName", 0, "select by survey name (LIKE comparision)", NULL);
    psMetadataAddS64(summaryArgs,  PS_LIST_TAIL, "-rel_id", 0, "select by release ID", 0);

    psMetadataAddBool(summaryArgs, PS_LIST_TAIL, "-priority_order",   0, "order by release priority", false);

    psMetadataAddU64(summaryArgs,  PS_LIST_TAIL, "-limit",       0, "limit result set to N items", 0);
    psMetadataAddBool(summaryArgs, PS_LIST_TAIL, "-simple",      0, "use the simple output format", false);

    // -definerelgroup
    psMetadata *definerelgroupArgs = psMetadataAlloc();

        // set the target release
    psMetadataAddStr(definerelgroupArgs, PS_LIST_TAIL, "-release_name", 0, "define release name", NULL);
    psMetadataAddS32(definerelgroupArgs, PS_LIST_TAIL, "-rel_id", 0,      "define release ID", 0);

        // select the members of the group
    psMetadataAddS64(definerelgroupArgs, PS_LIST_TAIL, "-select_seq_id", 0,  "select by LAP sequence ID", 0);
    psMetadataAddS64(definerelgroupArgs, PS_LIST_TAIL, "-select_lap_id", 0,  "select by LAP ID", 0);
    psMetadataAddStr(definerelgroupArgs, PS_LIST_TAIL, "-select_data_group", PS_META_DUPLICATE_OK,  "select by camRun.data_group", NULL);
    // do we want this?
    // psMetadataAddS64(definerelgroupArgs, PS_LIST_TAIL, "-lap_id", 0,  "select by LAP ID", NULL);

        // parameters of the relGroup
    psMetadataAddStr(definerelgroupArgs, PS_LIST_TAIL, "-set_group_type", 0,  "define group_type (required)", NULL);
    psMetadataAddStr(definerelgroupArgs, PS_LIST_TAIL, "-set_group_name", 0,  "define group_name", NULL);
    psMetadataAddStr(definerelgroupArgs, PS_LIST_TAIL, "-set_label", 0, "define relgroup label (required)", NULL);
    psMetadataAddStr(definerelgroupArgs,  PS_LIST_TAIL, "-set_state", 0,      "define state", NULL);
    psMetadataAddStr(definerelgroupArgs,  PS_LIST_TAIL, "-set_exp_list_path", 0, "define path in exposure list", NULL);
    psMetadataAddS16(definerelgroupArgs,  PS_LIST_TAIL, "-set_fault", 0,      "define fault", 0);

        // options
    psMetadataAddBool(definerelgroupArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(definerelgroupArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -pendingrelgroup
    psMetadata *pendingrelgroupArgs = psMetadataAlloc();

    psMetadataAddStr(pendingrelgroupArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK,  "search by relGroup.label", NULL);
    psMetadataAddS32(pendingrelgroupArgs, PS_LIST_TAIL, "-rel_id", 0,      "select by release ID", 0);
    psMetadataAddS32(pendingrelgroupArgs, PS_LIST_TAIL, "-group_id", 0,    "select by relGroup ID", 0);
    psMetadataAddStr(pendingrelgroupArgs, PS_LIST_TAIL, "-release_name", 0, "select by release name", 0);
    psMetadataAddBool(pendingrelgroupArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddU64(pendingrelgroupArgs, PS_LIST_TAIL,  "-limit",       0, "limit result set to N items", 0);
    
    // -updaterelgroup
    psMetadata *updaterelgroupArgs = psMetadataAlloc();

    psMetadataAddS32(updaterelgroupArgs,  PS_LIST_TAIL, "-group_id", 0,   "select by group ID",  0);
    psMetadataAddStr(updaterelgroupArgs,  PS_LIST_TAIL, "-group_name", 0, "select by group name", 0);

        // parameters of the relGroup
    psMetadataAddStr(updaterelgroupArgs,  PS_LIST_TAIL, "-set_state", 0,  "define state", NULL);
    psMetadataAddStr(updaterelgroupArgs,  PS_LIST_TAIL, "-set_exp_list_path", 0, "define path in exposure list", NULL);
    psMetadataAddStr(updaterelgroupArgs,  PS_LIST_TAIL, "-set_label", 0,  "define relgroup label", NULL);
    psMetadataAddS16(updaterelgroupArgs,  PS_LIST_TAIL, "-set_fault", 0,  "define fault", 0);
    psMetadataAddBool(updaterelgroupArgs, PS_LIST_TAIL, "-clearfault",  0, "set fault to zero", false);

    psMetadata *listrelgroupArgs = psMetadataAlloc();
    psMetadataAddS32(listrelgroupArgs,  PS_LIST_TAIL, "-group_id", 0,   "select by group ID", 0);
    psMetadataAddS32(listrelgroupArgs,  PS_LIST_TAIL, "-rel_id", 0,   "select by release ID", 0);
    psMetadataAddStr(listrelgroupArgs, PS_LIST_TAIL,  "-release_name", 0, "select by release name", 0);

    // ******************************************************************

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definesurvey",     "define a survey",              RELEASETOOL_MODE_DEFINESURVEY,  definesurveyArgs);
    PXOPT_ADD_MODE("-listsurvey",       "list skycell parameters",      RELEASETOOL_MODE_LISTSURVEY,    listsurveyArgs);

    PXOPT_ADD_MODE("-definerelease",    "define a release",             RELEASETOOL_MODE_DEFINERELEASE, definereleaseArgs);
    PXOPT_ADD_MODE("-updaterelease",    "update a release",             RELEASETOOL_MODE_UPDATERELEASE, updatereleaseArgs);
    PXOPT_ADD_MODE("-listrelease",      "list releases",                RELEASETOOL_MODE_LISTRELEASE,   listreleaseArgs);

    PXOPT_ADD_MODE("-definerelexp",     "define a released exposure",   RELEASETOOL_MODE_DEFINERELEXP,  definerelexpArgs);
    PXOPT_ADD_MODE("-updaterelexp",     "update a released exposure",   RELEASETOOL_MODE_UPDATERELEXP,  updaterelexpArgs);
    PXOPT_ADD_MODE("-tocalibexp",       "list relExps pending calibration",  RELEASETOOL_MODE_TOCALIBEXP,  tocalibexpArgs);
    PXOPT_ADD_MODE("-listrelexp",       "list released exposures",      RELEASETOOL_MODE_LISTRELEXP,    listrelexpArgs);
    PXOPT_ADD_MODE("-deleterelexp",     "delete a released exposure",   RELEASETOOL_MODE_DELETERELEXP,  deleterelexpArgs);

    PXOPT_ADD_MODE("-definerelstack",     "define a released stack",    RELEASETOOL_MODE_DEFINERELSTACK,  definerelstackArgs);
    PXOPT_ADD_MODE("-listrelstack",       "list released stacks",      RELEASETOOL_MODE_LISTRELSTACK,    listrelstackArgs);
    PXOPT_ADD_MODE("-setrelstackcalibratedfromskycal", "update parameters of using skycalResults",      RELEASETOOL_MODE_SETRELSTACKCALIBRATEDFROMSKYCAL,    calibraterelstackArgs);
    PXOPT_ADD_MODE("-deleterelstack",     "update parameters of released stacks",      RELEASETOOL_MODE_DELETERELSTACK,  deleterelstackArgs);

    PXOPT_ADD_MODE("-summary",            "list stackSummaryes for released stacks", RELEASETOOL_MODE_SUMMARY,    summaryArgs);

    PXOPT_ADD_MODE("-definerelgroup",     "define a group of exposures", RELEASETOOL_MODE_DEFINERELGROUP,  definerelgroupArgs);
    PXOPT_ADD_MODE("-pendingrelgroup",     "list relGroups pending processing",    RELEASETOOL_MODE_PENDINGRELGROUP,  pendingrelgroupArgs);
    PXOPT_ADD_MODE("-updaterelgroup",     "update a relGroup",          RELEASETOOL_MODE_UPDATERELGROUP,  updaterelgroupArgs);
    PXOPT_ADD_MODE("-listrelgroup",     "define a group of exposures", RELEASETOOL_MODE_LISTRELGROUP,  listrelgroupArgs);



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

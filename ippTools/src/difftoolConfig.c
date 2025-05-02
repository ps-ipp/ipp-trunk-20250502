/*
 * difftoolConfig.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <psmodules.h>

#include "pxtools.h"
#include "difftool.h"

pxConfig *difftoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_workdir", 0,         "define workdir (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-tess_id",  0,            "define tessellation ID (required)", NULL);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-bothways",  0,          "do the subtraction both ways?", false);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-exposure",  0,          "subtraction for entire exposure?", false);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_label",  0,          "define label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_reduction",  0,      "define reduction class", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-set_registered",  0,    "time detrend run was registered", now);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_data_group",  0,     "define data group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_dist_group",  0,     "define dist group", NULL);
    psMetadataAddS16(definerunArgs, PS_LIST_TAIL, "-set_diff_mode", 0,       "specify type of difference (WW=1,WS=2,SW=3,SS=4)", 0);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_note",  0,           "define note", NULL);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-diff_id", 0,          "define diff ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0,            "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0,            "define by label instead of diff ID (LIKE comparison)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0,       "define by data_group instead of diff ID (LIKE comparison)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dist_group", 0,       "define by dist_group instead of diff ID (LIKE comparison)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0,        "define new value for label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0,        "define new state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-set_diff_mode", 0,    "specify type of difference (WW=1,WS=2,SW=3,SS=4)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);
    pxmagicAddArguments(updaterunArgs);

    // -addinputskyfile
    psMetadata *addinputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,            "define diff ID (required)", 0);
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "define stack ID", 0);
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,            "define warp ID", 0);
    psMetadataAddBool(addinputskyfileArgs, PS_LIST_TAIL, "-template",  0,            "this sky cell file is the subtrahend", false);

    // -inputskyfile
    psMetadata *inputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(inputskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,            "search by diff ID", 0);
    psMetadataAddS64(inputskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warp ID", 0);
    psMetadataAddStr(inputskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0,            "search by skycell ID", NULL);
    psMetadataAddStr(inputskyfileArgs, PS_LIST_TAIL, "-tess_id", 0,            "search by tess ID", NULL);
    psMetadataAddBool(inputskyfileArgs, PS_LIST_TAIL, "-template",  0,            "find only subtrahend", false);
    psMetadataAddBool(inputskyfileArgs, PS_LIST_TAIL, "-input", 0, "find only minuend", false);
    psMetadataAddU64(inputskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(inputskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -todiffskyfile
    psMetadata *todiffskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(todiffskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,            "search by diff ID", 0);
    psMetadataAddStr(todiffskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddU64(todiffskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(todiffskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(todiffskyfileArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);

    // -adddiffskyfile
    psMetadata *adddiffskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(adddiffskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,            "define warp ID (required)", 0);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0,       "define skycell of file (required)", 0);
    psMetadataAddS16(adddiffskyfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(adddiffskyfileArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", 0);
    psMetadataAddF64(adddiffskyfileArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(adddiffskyfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-dtime_diff",  0,            "define elapsed processing time", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-dtime_match", 0, "define match processing time", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-dtime_phot", 0, "define photometry processing time", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddS32(adddiffskyfileArgs, PS_LIST_TAIL, "-stamps_num",  0,            "define subtraction stamp number", 0);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-stamps_mean", 0, "define subtraction stamp mean", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-stamps_rms",  0,            "define subtraction stamp rms", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-norm",  0, "define normalisation", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-bg_diff",  0, "define background difference", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-kernel_x",  0, "define kernel x moment", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-kernel_y",  0, "define kernel y moment", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-kernel_xx",  0, "define kernel xx moment", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-kernel_xy",  0, "define kernel xy moment", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-kernel_yy",  0, "define kernel yy moment", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-deconv_max",  0, "define maximum deconvolution fraction", NAN);
    psMetadataAddS32(adddiffskyfileArgs, PS_LIST_TAIL, "-sources",  0,   "define number of sources", 0);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-hostname", 0,   "define hostname", 0);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-good_frac",  0, "define %% of good pixels", NAN);
    psMetadataAddS64(adddiffskyfileArgs, PS_LIST_TAIL, "-magicked",  0, "define magicked state", 0);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-ver_ppsub", 0, "define ppSub version", NULL);
    psMetadataAddStr(adddiffskyfileArgs, PS_LIST_TAIL, "-ver_streaks", 0, "define streaksremove version", NULL);
    psMetadataAddS32(adddiffskyfileArgs, PS_LIST_TAIL, "-maskfrac_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-maskfrac_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-maskfrac_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-maskfrac_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(adddiffskyfileArgs, PS_LIST_TAIL, "-maskfrac_advisory", 0, "define advisory mask fraction", NAN);

    // -advance
    psMetadata *advanceArgs = psMetadataAlloc();
    psMetadataAddS64(advanceArgs, PS_LIST_TAIL, "-diff_id", 0, "select by diff ID", 0);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "select by label", NULL);
    psMetadataAddS32(advanceArgs, PS_LIST_TAIL, "-limit", 0, "limit number of results", 0);

    // -diffskyfile
    psMetadata *diffskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(diffskyfileArgs, PS_LIST_TAIL,  "-diff_id", 0,           "search by diff ID", 0);
    psMetadataAddStr(diffskyfileArgs , PS_LIST_TAIL, "-skycell_id",  0,      "search by skycell ID", NULL);
    psMetadataAddS64(diffskyfileArgs, PS_LIST_TAIL,  "-diff_skyfile_id", 0,   "search by diff_skyfile_id ID", 0);
    psMetadataAddStr(diffskyfileArgs, PS_LIST_TAIL,  "-tess_id",  0,          "search by tessellation ID", NULL);
    psMetadataAddStr(diffskyfileArgs , PS_LIST_TAIL, "-warp_id",  0,         "search by warp_id", NULL);
    psMetadataAddBool(diffskyfileArgs, PS_LIST_TAIL, "-template",  0,        "apply exposure args to template of bothways diff", false);
    psMetadataAddS64(diffskyfileArgs, PS_LIST_TAIL,  "-exp_id",  0,           "search by exposure ID", 0);
    psMetadataAddStr(diffskyfileArgs , PS_LIST_TAIL, "-exp_name",  0,        "search by exposure name", NULL);
    psMetadataAddTime(diffskyfileArgs, PS_LIST_TAIL, "-dateobs_begin", 0,    "search for exposures by time (>=)", NULL);
    psMetadataAddTime(diffskyfileArgs, PS_LIST_TAIL, "-dateobs_end", 0,      "search for exposures by time (<=)", NULL);
    psMetadataAddStr(diffskyfileArgs, PS_LIST_TAIL,  "-filter", 0,           "search for filter", NULL);
    psMetadataAddStr(diffskyfileArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by diffRun label (LIKE comparison)", NULL);
    psMetadataAddStr(diffskyfileArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by diffRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(diffskyfileArgs,  PS_LIST_TAIL, "-dist_group",  PS_META_DUPLICATE_OK, "search by diffRun dist_group (LIKE comparison)", NULL);
    psMetadataAddS16(diffskyfileArgs, PS_LIST_TAIL,  "-fault",  0,           "search by fault code", 0);
    pxmagicAddArguments(diffskyfileArgs);
    pxspaceAddArguments(diffskyfileArgs);

    psMetadataAddBool(diffskyfileArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);
    psMetadataAddBool(diffskyfileArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(diffskyfileArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(diffskyfileArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    // -listrun
    psMetadata *listrunArgs = psMetadataAlloc();
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL,  "-diff_id", 0,           "search by diff ID", 0);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-tess_id",  0,          "search by tessellation ID (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs , PS_LIST_TAIL, "-warp_id",  0,         "search by warp_id", NULL);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-template",  0,        "apply exposure args to template of bothways diff", false);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL,  "-exp_id",  0,           "search by exposure ID", 0);
    psMetadataAddStr(listrunArgs , PS_LIST_TAIL, "-exp_name",  0,        "search by exposure name", NULL);
    psMetadataAddTime(listrunArgs, PS_LIST_TAIL, "-dateobs_begin", 0,    "search for exposures by time (>=)", NULL);
    psMetadataAddTime(listrunArgs, PS_LIST_TAIL, "-dateobs_end", 0,      "search for exposures by time (<=)", NULL);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-filter", 0,           "search for filter", NULL);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL,  "-template_exp_id",  0, "search by exposure ID of template", 0);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by diffRun label (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by diffRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-dist_group",  PS_META_DUPLICATE_OK, "search by diffRun dist_group (LIKE comparison)", NULL);
    psMetadataAddS16(listrunArgs,  PS_LIST_TAIL, "-diff_mode", 0,        "search for diff_mode", 0);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-state",  0,           "search by state", NULL);
    pxmagicAddArguments(listrunArgs);
    pxspaceAddArguments(listrunArgs);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(listrunArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);


    // -listssrun
    psMetadata *listssrunArgs = psMetadataAlloc();
    psMetadataAddS64(listssrunArgs, PS_LIST_TAIL,  "-diff_id", 0,          "search by diff ID", 0);
    psMetadataAddStr(listssrunArgs, PS_LIST_TAIL,  "-tess_id",  0,         "search by tessellation ID (LIKE comparison)", NULL);
    psMetadataAddS64(listssrunArgs , PS_LIST_TAIL, "-stack_id",  0,        "search by input stack_id", 0);
    psMetadataAddS64(listssrunArgs , PS_LIST_TAIL, "-template_stack_id",0, "search by template stack_id", 0);
    psMetadataAddBool(listssrunArgs, PS_LIST_TAIL, "-template",  0,        "apply stack selectors to template", false);
    psMetadataAddF64(listssrunArgs, PS_LIST_TAIL,  "-mjd_obs_begin", 0,    "search by stack MJD-OBS (>=)", 0);
    psMetadataAddF64(listssrunArgs, PS_LIST_TAIL,  "-mjd_obs_end", 0,      "search by stack MJD-OBS(<=)", 0);
    psMetadataAddStr(listssrunArgs, PS_LIST_TAIL,  "-filter", 0,           "search by stack filter", NULL);
    psMetadataAddStr(listssrunArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by diffRun label (LIKE comparison)", NULL);
    psMetadataAddStr(listssrunArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by diffRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listssrunArgs,  PS_LIST_TAIL, "-dist_group",  PS_META_DUPLICATE_OK, "search by diffRun dist_group (LIKE comparison)", NULL);
    psMetadataAddS16(listssrunArgs,  PS_LIST_TAIL, "-diff_mode", 0,        "search for diff_mode", 0);
    psMetadataAddStr(listssrunArgs, PS_LIST_TAIL,  "-state",  0,           "search by state", NULL);
    pxmagicAddArguments(listssrunArgs);
    pxspaceAddArguments(listssrunArgs);

    psMetadataAddBool(listssrunArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);

    psMetadataAddBool(listssrunArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(listssrunArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(listssrunArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);


    // -revertdiffskyfile
    psMetadata *revertdiffskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(revertdiffskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,            "search by diff ID", 0);
    psMetadataAddStr(revertdiffskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", NULL);
    psMetadataAddStr(revertdiffskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS16(revertdiffskyfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertdiffskyfileArgs, PS_LIST_TAIL, "-all",  0, "allow no search terms", 0);

    // -definepoprun
    psMetadata *definepoprunArgs = psMetadataAlloc();
    psMetadataAddStr(definepoprunArgs, PS_LIST_TAIL, "-workdir", 0,            "define workdir (required)", NULL);
    psMetadataAddTime(definepoprunArgs, PS_LIST_TAIL, "-registered",  0,            "time detrend run was registered", now);
    psMetadataAddStr(definepoprunArgs, PS_LIST_TAIL, "-skycell_id",  0,            "define skycell ID (required)", NULL);
    psMetadataAddStr(definepoprunArgs, PS_LIST_TAIL, "-tess_id",  0,            "define tessellation ID (required)", NULL);
    psMetadataAddStr(definepoprunArgs, PS_LIST_TAIL, "-label",  0,            "define label", NULL);
    psMetadataAddStr(definepoprunArgs, PS_LIST_TAIL, "-reduction",  0,            "define reduction class", NULL);
    psMetadataAddBool(definepoprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddS64(definepoprunArgs, PS_LIST_TAIL, "-template_warp_id", 0,            "define warp ID for template", 0);
    psMetadataAddS64(definepoprunArgs, PS_LIST_TAIL, "-template_stack_id", 0,            "define stack ID for template", 0);
    psMetadataAddS64(definepoprunArgs, PS_LIST_TAIL, "-input_warp_id", 0,            "define warp ID for input", 0);
    psMetadataAddS64(definepoprunArgs, PS_LIST_TAIL, "-input_stack_id", 0,            "define stack ID for input", 0);

    // -definewarpstack
    psMetadata *definewarpstackArgs = psMetadataAlloc();
    psMetadataAddS64(definewarpstackArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp ID", 0);
    psMetadataAddS64(definewarpstackArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exposure ID", 0);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID", NULL);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-bothways",  0,          "do the subtraction both ways?", false);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-filter", 0, "search by filter", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-comment", 0, "search by comment (LIKE)", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-stack_label", 0, "search by stack label", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-stack_data_group", 0, "search by stack data_group", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-warp_label", 0, "search by warp label", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group", NULL);
    psMetadataAddF32(definewarpstackArgs, PS_LIST_TAIL, "-good_frac", 0, "minimum good fraction of skycell", NAN);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-set_label",  0, "define label", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-set_reduction",  0, "define reduction class", NULL);
    psMetadataAddTime(definewarpstackArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-set_data_group",  0,     "define data group", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-set_dist_group",  0,     "define dist group", NULL);
    psMetadataAddStr(definewarpstackArgs, PS_LIST_TAIL, "-set_note",  0,           "define note", NULL);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-new-templates", 0, "also search for diffs with new template", false);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-rerun", 0, "define new run even if one exists", false);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-available", 0, "define new run even if no stacks available for some skycells", false);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-pretend", 0, "list results but to not queue", false);
    psMetadataAddBool(definewarpstackArgs, PS_LIST_TAIL, "-lap_query", 0, "Use the updated LAP style SQL code", false);
    
    pxspaceBoxAddArguments(definewarpstackArgs);

#if (0)    
    // -definewarpstackOldMethod
    psMetadata *definewarpstackOldMethodArgs = psMetadataAlloc();
    psMetadataAddS64(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp ID", 0);
    psMetadataAddS64(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exposure ID", 0);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID", NULL);
    psMetadataAddBool(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-bothways",  0,          "do the subtraction both ways?", false);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-filter", 0, "search by filter", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-comment", 0, "search by comment (LIKE)", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-stack_label", 0, "search by stack label", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-stack_data_group", 0, "search by stack data_group", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-warp_label", 0, "search by warp label", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group", NULL);
    psMetadataAddF32(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-good_frac", 0, "minimum good fraction of skycell", NAN);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_label",  0, "define label", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_reduction",  0, "define reduction class", NULL);
    psMetadataAddTime(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_data_group",  0,     "define data group", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_dist_group",  0,     "define dist group", NULL);
    psMetadataAddStr(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-set_note",  0,           "define note", NULL);
    psMetadataAddBool(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-new-templates", 0, "also search for diffs with new template", false);
    psMetadataAddBool(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-rerun", 0, "define new run even if one exists", false);
    psMetadataAddBool(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-available", 0, "define new run even if no stacks available for some skycells", false);
    psMetadataAddBool(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definewarpstackOldMethodArgs, PS_LIST_TAIL, "-pretend", 0, "list results but to not queue", false);

    pxspaceBoxAddArguments(definewarpstackOldMethodArgs);
#endif
    
    // -definewarpwarp
    psMetadata *definewarpwarpArgs = psMetadataAlloc();
    psMetadataAddS64(definewarpwarpArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp ID", 0);
    psMetadataAddS64(definewarpwarpArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exposure ID", 0);
    psMetadataAddBool(definewarpwarpArgs, PS_LIST_TAIL, "-not-bothways",  0, "only do the single-direction subtraction?", false);
    psMetadataAddS64(definewarpwarpArgs, PS_LIST_TAIL,  "-template_exp_id",  0,  "search by template exposure ID", 0);
    psMetadataAddS64(definewarpwarpArgs, PS_LIST_TAIL,  "-template_warp_id",  0,  "search by template warp ID", 0);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-filter", 0, "search by filter", NULL);
    psMetadataAddF32(definewarpwarpArgs, PS_LIST_TAIL, "-distance", 0, "limit distance between input and template (deg)", NAN);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-obs_mode", 0, "search by observation mode", NULL);
    psMetadataAddF32(definewarpwarpArgs, PS_LIST_TAIL, "-timediff", 0, "limit time difference between input and template", NAN);
    psMetadataAddF32(definewarpwarpArgs, PS_LIST_TAIL, "-mintimediff", 0, "limit time difference between input and template to be greater than this", NAN);
    psMetadataAddBool(definewarpwarpArgs, PS_LIST_TAIL, "-backwards", 0, "Template comes after input?", false);
    psMetadataAddF32(definewarpwarpArgs, PS_LIST_TAIL, "-rotdiff", 0, "limit rotator difference between input and template", NAN);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-input_label", 0, "search by warp label for input", NULL);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-template_label", 0, "search by warp label for template", NULL);
    psMetadataAddF32(definewarpwarpArgs, PS_LIST_TAIL, "-good_frac", 0, "minimum good fraction of skycell", NAN);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-set_label",  0, "define label", NULL);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-set_reduction",  0, "define reduction class", NULL);
    psMetadataAddTime(definewarpwarpArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-set_data_group",  0,     "define data group", NULL);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-set_dist_group",  0,     "define dist group", NULL);
    psMetadataAddStr(definewarpwarpArgs, PS_LIST_TAIL, "-set_note",  0,           "define note", NULL);
    psMetadataAddBool(definewarpwarpArgs, PS_LIST_TAIL, "-rerun", 0, "define new run even if one exists", false);
    psMetadataAddBool(definewarpwarpArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definewarpwarpArgs, PS_LIST_TAIL, "-pretend", 0, "list results but to not queue", false);
    psMetadataAddTime(definewarpwarpArgs, PS_LIST_TAIL, "-dateobs_begin",      0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definewarpwarpArgs, PS_LIST_TAIL, "-dateobs_end",        0, "search for exposures by time (<)", NULL);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-ra_min",             0, "search by min RA (degrees) ", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-ra_max",             0, "search by max RA (degrees) ", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-decl_min",           0, "search by min DEC (degrees)", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-decl_max",           0, "search by max DEC (degrees)", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-airmass_min",        0, "search by min airmass", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-airmass_max",        0, "search by max airmass", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-exp_time_min",       0, "search by min exposure time", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-exp_time_max",       0, "search by max exposure time", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-bg_min",             0, "search by min background", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-bg_max",             0, "search by max background", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-bg_stdev_min",       0, "search by min background standard deviation", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-bg_stdev_max",       0, "search by max background standard deviation", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-bg_mean_stdev_min",  0, "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-bg_mean_stdev_max",  0, "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-alt_min",            0, "search by min altitude", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-alt_max",            0, "search by max altitude", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-az_min",             0, "search by min azimuth ", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-az_max",             0, "search by max azimuth ", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-ccd_temp_min",       0, "search by min ccd tempature", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-ccd_temp_max",       0, "search by max ccd tempature", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-posang_min",         0, "search by min rotator position angle", NAN);
    psMetadataAddF64(definewarpwarpArgs,  PS_LIST_TAIL, "-posang_max",         0, "search by max rotator position angle", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-sun_angle_min",      0, "search by min solar angle", NAN);
    psMetadataAddF32(definewarpwarpArgs,  PS_LIST_TAIL, "-sun_angle_max",      0, "search by max solar angle", NAN);
    psMetadataAddStr(definewarpwarpArgs,  PS_LIST_TAIL, "-input_comment",      0, "search by comment field for input exposure(LIKE comparison)", NULL);
    psMetadataAddStr(definewarpwarpArgs,  PS_LIST_TAIL, "-template_comment",   0, "search by comment field for template exposure(LIKE comparison)", NULL);
    psMetadataAddU64(definewarpwarpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -definestackstack
    psMetadata *definestackstackArgs = psMetadataAlloc();
    // stack id and exp id searches seem less useful here
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID", NULL);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-bothways",  0,          "do the subtraction both ways?", false);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-relaxed-filters",  0, "allow mismatched filters between input and template?", false);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-filter", 0, "search by filter", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-input_label", 0, "search by stack label for input", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-template_label", 0, "search by stack label for template", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-input_stack_id", 0, "search by stack_id for input", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-template_stack_id", 0, "search by stack_id for template", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-input_data_group", 0, "search by stack data_group for input", NULL);
    psMetadataAddF32(definestackstackArgs, PS_LIST_TAIL, "-good_frac", 0, "minimum good fraction of skycell", NAN);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction class", NULL);
    psMetadataAddTime(definestackstackArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(definestackstackArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-rerun", 0, "define new run even if one exists", false);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-available", 0, "define new run even if no stacks available for some skycells", false);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-new-templates", 0, "also search for diffs with new template and same template label", false);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definestackstackArgs, PS_LIST_TAIL, "-pretend", 0, "list results but do not queue", false);


    // -pendingcleanuprun
    psMetadata *pendingcleanuprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanuprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list runs for cleanup with specified label", NULL);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanuprunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -tosummary
    psMetadata *tosummaryArgs = psMetadataAlloc();
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL,  "-diff_id", 0,           "search by diff ID", 0);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-tess_id",  0,          "search by tessellation ID", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-state",  0,            "search by state", NULL);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL,  "-exp_id",  0,           "search by exposure ID", 0);
    psMetadataAddStr(tosummaryArgs , PS_LIST_TAIL, "-exp_name",  0,        "search by exposure name", NULL);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL, "-warp_id", 0,         "search by warp ID", 0);
    
    psMetadataAddTime(tosummaryArgs, PS_LIST_TAIL, "-dateobs_begin", 0,    "search for exposures by time (>=)", NULL);
    psMetadataAddTime(tosummaryArgs, PS_LIST_TAIL, "-dateobs_end", 0,      "search for exposures by time (<=)", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-filter", 0,           "search for filter", NULL);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL,  "-magicked", 0,         "search by magic id", 0);
    psMetadataAddStr(tosummaryArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by diffRun label (LIKE comparison)", NULL);
    psMetadataAddStr(tosummaryArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by diffRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(tosummaryArgs,  PS_LIST_TAIL, "-dist_group",  PS_META_DUPLICATE_OK, "search by diffRun dist_group (LIKE comparison)", NULL);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL,  "-destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL,  "-not_destreaked", 0, "search for runs that have not been destreaked", false);
    
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(tosummaryArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);
    
    // -addsummary
    psMetadata *addsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(addsummaryArgs, PS_LIST_TAIL,  "-diff_id", 0,           "search by diff ID", 0);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-projection_cell", 0, "set projection cell", NULL);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-path_base", 0, "set summary path base", NULL);

    // -pendingcleanupskyfile
    psMetadata *pendingcleanupskyfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,          "search by diff ID", 0);
    psMetadataAddBool(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-all",  0,            "list all skycells regardless of data_state", false);
    psMetadataAddBool(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -revertcleanup
    psMetadata *revertcleanupArgs = psMetadataAlloc();
    psMetadataAddS64(revertcleanupArgs, PS_LIST_TAIL, "-diff_id", 0,            "search by difftool ID", 0);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by diffRun label", NULL);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by diffRun data_group", NULL);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-state",  0,            "search by state", NULL);


    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -updatediffskyfile
    psMetadata *updatediffskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(updatediffskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,      "define diff ID (required)", 0);
    psMetadataAddStr(updatediffskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0,   "search by skycell ID", NULL);
    psMetadataAddS16(updatediffskyfileArgs, PS_LIST_TAIL, "-fault", 0,        "set fault code (required)", 0);
    psMetadataAddS16(updatediffskyfileArgs, PS_LIST_TAIL, "-set_quality", 0,  "set quality", 0);
    psMetadataAddStr(updatediffskyfileArgs, PS_LIST_TAIL, "-set_state", 0,    "set data_state state", NULL);

    // -tocleanedskyfile
    psMetadata *tocleanedskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedskyfileArgs, PS_LIST_TAIL, "-diff_id", 0, "difftool ID to update", 0);
    psMetadataAddStr(tocleanedskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);

    // -topurgedskyfile
    psMetadata *topurgedskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(topurgedskyfileArgs, PS_LIST_TAIL, "-diff_id", 0,    "difftool ID to update", 0);
    psMetadataAddStr(topurgedskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);

    // -toscrubbedskyfile
    psMetadata *toscrubbedskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(toscrubbedskyfileArgs, PS_LIST_TAIL, "-diff_id", 0, "difftool ID to update", 0);
    psMetadataAddStr(toscrubbedskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);

    // -tofullskyfile
    psMetadata *tofullskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(tofullskyfileArgs, PS_LIST_TAIL, "-diff_id", 0, "difftool ID to update", 0);
    psMetadataAddStr(tofullskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);
    psMetadataAddS64(tofullskyfileArgs, PS_LIST_TAIL, "-magicked",  0, "define magicked state", 0);

    // -setskyfiletoupdate
    psMetadata *setskyfiletoupdateArgs = psMetadataAlloc();
    psMetadataAddS64(setskyfiletoupdateArgs, PS_LIST_TAIL, "-diff_id", 0,           "search by difftool ID (required)", 0);
    psMetadataAddStr(setskyfiletoupdateArgs, PS_LIST_TAIL, "-skycell_id",  0,       "search by tessellation ID", NULL);
    psMetadataAddStr(setskyfiletoupdateArgs, PS_LIST_TAIL, "-set_label",  0,        "new value for diffRun.label", NULL);



    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-diff_id", 0,          "export this diff ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "mark tables as cleaned", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);


    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definerun",        "", DIFFTOOL_MODE_DEFINERUN,         definerunArgs);
    PXOPT_ADD_MODE("-updaterun",        "", DIFFTOOL_MODE_UPDATERUN,         updaterunArgs);
    PXOPT_ADD_MODE("-addinputskyfile",  "", DIFFTOOL_MODE_ADDINPUTSKYFILE,   addinputskyfileArgs);
    PXOPT_ADD_MODE("-inputskyfile",     "", DIFFTOOL_MODE_INPUTSKYFILE,      inputskyfileArgs);
    PXOPT_ADD_MODE("-todiffskyfile",    "", DIFFTOOL_MODE_TODIFFSKYFILE,     todiffskyfileArgs);
    PXOPT_ADD_MODE("-adddiffskyfile",   "", DIFFTOOL_MODE_ADDDIFFSKYFILE,    adddiffskyfileArgs);
    PXOPT_ADD_MODE("-advance",          "", DIFFTOOL_MODE_ADVANCE,           advanceArgs);
    PXOPT_ADD_MODE("-diffskyfile",      "", DIFFTOOL_MODE_DIFFSKYFILE,       diffskyfileArgs);
    PXOPT_ADD_MODE("-revertdiffskyfile","", DIFFTOOL_MODE_REVERTDIFFSKYFILE, revertdiffskyfileArgs);
    PXOPT_ADD_MODE("-definepoprun",     "", DIFFTOOL_MODE_DEFINEPOPRUN,      definepoprunArgs);
    PXOPT_ADD_MODE("-definewarpstack",  "", DIFFTOOL_MODE_DEFINEWARPSTACK,   definewarpstackArgs);
    //    PXOPT_ADD_MODE("-definewarpstackOldMethod",  "", DIFFTOOL_MODE_DEFINEWARPSTACKOLDMETHOD,   definewarpstackOldMethodArgs);
    PXOPT_ADD_MODE("-definewarpwarp",   "", DIFFTOOL_MODE_DEFINEWARPWARP,    definewarpwarpArgs);
    PXOPT_ADD_MODE("-definestackstack", "", DIFFTOOL_MODE_DEFINESTACKSTACK,  definestackstackArgs);
    PXOPT_ADD_MODE("-listrun",          "list diff runs", DIFFTOOL_MODE_LISTRUN,           listrunArgs);
    PXOPT_ADD_MODE("-listssrun",        "list stack-stack diff runs", DIFFTOOL_MODE_LISTSSRUN,           listssrunArgs);
    PXOPT_ADD_MODE("-pendingcleanuprun",     "show runs that need to be cleaned up", DIFFTOOL_MODE_PENDINGCLEANUPRUN,    pendingcleanuprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupskyfile", "show runs that need to be cleaned up", DIFFTOOL_MODE_PENDINGCLEANUPSKYFILE, pendingcleanupskyfileArgs);
    PXOPT_ADD_MODE("-revertcleanup",           "revert cleanup runs with errors",     DIFFTOOL_MODE_REVERTCLEANUP,          revertcleanupArgs);
    PXOPT_ADD_MODE("-donecleanup",           "show runs that have been cleaned",     DIFFTOOL_MODE_DONECLEANUP,          donecleanupArgs);
    PXOPT_ADD_MODE("-updatediffskyfile",     "update fault code for a diffskyfile",  DIFFTOOL_MODE_UPDATEDIFFSKYFILE,          updatediffskyfileArgs);
    PXOPT_ADD_MODE("-setskyfiletoupdate", "set cleaned skyfile to be updated", DIFFTOOL_MODE_SETSKYFILETOUPDATE, setskyfiletoupdateArgs);

    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", DIFFTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           DIFFTOOL_MODE_IMPORTRUN, importrunArgs);

    PXOPT_ADD_MODE("-tocleanedskyfile", "set skyfile as cleaned", DIFFTOOL_MODE_TOCLEANEDSKYFILE, tocleanedskyfileArgs);
    PXOPT_ADD_MODE("-topurgedskyfile", "set skyfile as purged", DIFFTOOL_MODE_TOPURGEDSKYFILE, topurgedskyfileArgs);
    PXOPT_ADD_MODE("-toscrubbedskyfile", "set skyfile as scrubbed", DIFFTOOL_MODE_TOSCRUBBEDSKYFILE, toscrubbedskyfileArgs);
    PXOPT_ADD_MODE("-tofullskyfile", "set skyfile as full", DIFFTOOL_MODE_TOFULLSKYFILE, tofullskyfileArgs);
    PXOPT_ADD_MODE("-tosummary",            "show runs that can be summarized", DIFFTOOL_MODE_TOSUMMARY, tosummaryArgs);
    PXOPT_ADD_MODE("-addsummary",           "add entry to the summary table", DIFFTOOL_MODE_ADDSUMMARY, addsummaryArgs);

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

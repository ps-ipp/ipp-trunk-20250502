/*
 * warptoolConfig.c
 *
 * Copyright (C) 2006-2007  Joshua Hoblitt
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
#include "warptool.h"

pxConfig *warptoolConfig(pxConfig *config, int argc, char **argv)
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
    // XXX need to allow multiple chip_ids
    // XXX need to allow multiple exp_ids
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-fake_id",            0, "search by fake_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-cam_id",             0, "search by cam_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-chip_id",            0, "search by chip_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-exp_id",             0, "search by exp_id", 0);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-exp_name",           0, "search by exp_name", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-inst",               0, "search for camera", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-telescope",          0, "search for telescope", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-dateobs_begin",     0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-dateobs_end",       0, "search for exposures by time (<)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-exp_tag",            0, "search by exp_tag", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-exp_type",           0, "search by exp_type", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-filelevel",          0, "search by filelevel", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-filter",             0, "search for filter", NULL);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-airmass_min",        0, "search by min airmass", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-airmass_max",        0, "search by max airmass", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-ra_min",             0, "search by min RA (degrees) ", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-ra_max",             0, "search by max RA (degrees) ", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-decl_min",           0, "search by min DEC (degrees)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-decl_max",           0, "search by max DEC (degrees)", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-exp_time_min",       0, "search by min exposure time", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-exp_time_max",       0, "search by max exposure time", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-bg_min",             0, "search by min background", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-bg_max",             0, "search by max background", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-bg_stdev_min",       0, "search by min background standard deviation", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-bg_stdev_max",       0, "search by max background standard deviation", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-bg_mean_stdev_min",  0, "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-bg_mean_stdev_max",  0, "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-alt_min",            0, "search by min altitude", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-alt_max",            0, "search by max altitude", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-az_min",             0, "search by min azimuth ", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-az_max",             0, "search by max azimuth ", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-ccd_temp_min",       0, "search by min ccd tempature", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-ccd_temp_max",       0, "search by max ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-posang_min",         0, "search by min rotator position angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-posang_max",         0, "search by max rotator position angle", NAN);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-object",             0, "search by exposure object", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-comment",            0, "search by comment field (LIKE comparison)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-obs_mode",           0, "search by observation mode", NULL);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-sun_angle_min",      0, "search by min solar angle", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-sun_angle_max",      0, "search by max solar angle", NAN);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-reduction",          0, "search by fakeRun reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search on fakeRun label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_mode",           0, "define mode (warp, diff, stack, magic)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",        0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",          0, "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb",          0, "define DVO db", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_tess_id",        0, "define tess ID", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",      0, "define reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_end_stage",      0, "define end stage", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group",     0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dist_group",     0, "define dist group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note",           0, "define note", NULL);

    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-registered",  0,            "time detrend run was registered", now);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0,            "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddS64(definerunArgs, PS_LIST_TAIL, "-fake_id", 0,            "define camtool ID (required)", 0);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-mode", 0,            "define mode (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-workdir", 0,            "define workdir (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-label", 0,            "define label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-dvodb", 0,            "define dvodb", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-tess_id", 0,            "define tess_id (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-reduction", 0,            "define reduction class", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-end_stage", 0,            "define end stage", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-registered",  0,            "time detrend run was registered", now);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_end_stage",      0, "define end stage", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_data_group",     0, "define data group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_dist_group",     0, "define dist group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_note",           0, "define note", NULL);

    // -updaterun
    // XXX need to allow multiple fake_ids
    // XXX need to allow multiple fake_ids
    // XXX need to allow multiple chip_ids
    // XXX need to allow multiple exp_ids
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-warp_id", 0,    "search by warptool ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-reduction", 0,  "search by warpRun reduction class", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0,      "search by warpRun state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by warpRun label (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0, "search by warpRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dist_group", 0, "search by warpRun dist_group (LIKE comparison)", NULL);
    psMetadataAddTime(updaterunArgs, PS_LIST_TAIL, "-registered_begin", 0, "search for runs by registration time (>=)", NULL);
    psMetadataAddTime(updaterunArgs, PS_LIST_TAIL, "-registered_end", 0, "search for runs by registration time (<)", NULL);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-pretend", 0, "only pretend to run the query", false);

    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0,        "define new state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0,        "define new value for label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);
    pxmagicAddArguments(updaterunArgs);

    // -exp
    psMetadata *expArgs = psMetadataAlloc();
    psMetadataAddS64(expArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddS64(expArgs, PS_LIST_TAIL, "-fake_id", 0,            "search by camtool ID", 0);
    psMetadataAddU64(expArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(expArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -imfile
    psMetadata *imfileArgs = psMetadataAlloc();
    psMetadataAddS64(imfileArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddS64(imfileArgs, PS_LIST_TAIL, "-fake_id", 0,            "search by camtool ID", 0);
    psMetadataAddU64(imfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(imfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -tooverlap
    psMetadata *tooverlapArgs = psMetadataAlloc();
    psMetadataAddS64(tooverlapArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warp ID", 0);
    psMetadataAddStr(tooverlapArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddU64(tooverlapArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tooverlapArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(tooverlapArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -addoverlap
    psMetadata *addoverlapArgs = psMetadataAlloc();
    psMetadataAddStr(addoverlapArgs, PS_LIST_TAIL, "-mapfile", 0,            "path to skycell <-> imfile mapping file", NULL);
    psMetadataAddS64(addoverlapArgs, PS_LIST_TAIL, "-warp_id",  0,            "set warp ID", 0);
    psMetadataAddS16(addoverlapArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);

    // -revertoverlap
    psMetadata *revertoverlapArgs = psMetadataAlloc();
    psMetadataAddS64(revertoverlapArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddStr(revertoverlapArgs, PS_LIST_TAIL, "-skycell_id",  0,            "search by skycell ID", NULL);
    psMetadataAddStr(revertoverlapArgs, PS_LIST_TAIL, "-tess_id",  0,            "search by tessellation ID", NULL);
    psMetadataAddStr(revertoverlapArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddS16(revertoverlapArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertoverlapArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);


    // -scmap
    psMetadata *scmapArgs = psMetadataAlloc();
    psMetadataAddS64(scmapArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddStr(scmapArgs, PS_LIST_TAIL, "-skycell_id", 0,            "search by skycell ID", NULL);
    psMetadataAddStr(scmapArgs, PS_LIST_TAIL, "-tess_id", 0,            "search by tess ID", NULL);
    psMetadataAddStr(scmapArgs, PS_LIST_TAIL, "-class_id", 0,            "search by class ID", NULL);
    psMetadataAddU64(scmapArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(scmapArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -towarped
    psMetadata *towarpedArgs = psMetadataAlloc();
    psMetadataAddS64(towarpedArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddStr(towarpedArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddU64(towarpedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(towarpedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(towarpedArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -addwarped
    psMetadata *addwarpedArgs = psMetadataAlloc();
    psMetadataAddS64(addwarpedArgs, PS_LIST_TAIL, "-warp_id", 0,            "define warptool ID (required)", 0);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-skycell_id",  0,            "define skycell ID (required)", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-tess_id",  0,            "define tessellation ID (required)", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-uri", 0,            "define URI of file", 0);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", 0);
    psMetadataAddF64(addwarpedArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addwarpedArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-dtime_warp",  0,            "define elapsed processing time", NAN);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddS32(addwarpedArgs, PS_LIST_TAIL, "-xmin",  0,            "define minimum x value", INT_MAX);
    psMetadataAddS32(addwarpedArgs, PS_LIST_TAIL, "-xmax",  0,            "define maximum x value", -INT_MAX);
    psMetadataAddS32(addwarpedArgs, PS_LIST_TAIL, "-ymin",  0,            "define minimum y value", INT_MAX);
    psMetadataAddS32(addwarpedArgs, PS_LIST_TAIL, "-ymax",  0,            "define maximum y value", -INT_MAX);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-hostname", 0,            "define hostname", 0);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-good_frac",  0,            "define %% of good pixels", NAN);
    psMetadataAddS64(addwarpedArgs, PS_LIST_TAIL, "-set_magicked",  0, "define if this skycell has been magicked", 0);
    psMetadataAddS16(addwarpedArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(addwarpedArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);

    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-ver_pswarp", 0, "define pswarp version", NULL);
    psMetadataAddStr(addwarpedArgs, PS_LIST_TAIL, "-ver_streaks", 0, "define streaksremove version", NULL);

    psMetadataAddS32(addwarpedArgs, PS_LIST_TAIL, "-maskfrac_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-maskfrac_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-maskfrac_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-maskfrac_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(addwarpedArgs, PS_LIST_TAIL, "-maskfrac_advisory", 0, "define advisory mask fraction", NAN);

    psMetadataAddS16(addwarpedArgs, PS_LIST_TAIL, "-background_model", 0, "define background model version", 0);
    
    // -warped
    psMetadata *warpedArgs = psMetadataAlloc();
    psMetadataAddS64(warpedArgs, PS_LIST_TAIL, "-warp_id", 0,           "search by warptool ID", 0);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL, "-skycell_id",  0,       "search by skycell ID", NULL);
    psMetadataAddS64(warpedArgs, PS_LIST_TAIL, "-warp_skyfile_id", 0,   "search by skyfile ID", 0);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL, "-tess_id",  0,          "search by tessellation ID", NULL);
    psMetadataAddS64(warpedArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure tag", 0);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure tag", 0);
    psMetadataAddS64(warpedArgs, PS_LIST_TAIL, "-fake_id", 0,           "search by phase 3 version of exposure tag", 0);
    psMetadataAddTime(warpedArgs, PS_LIST_TAIL, "-dateobs_begin", 0,    "search for exposures by time (>=)", NULL);
    psMetadataAddTime(warpedArgs, PS_LIST_TAIL, "-dateobs_end", 0,      "search for exposures by time (<=)", NULL);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL,  "-filter", 0,           "search for exposures by filter", NULL);
    psMetadataAddS16(warpedArgs, PS_LIST_TAIL,  "-fault",  0,           "search by fault code", 0);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL,  "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL,  "-data_group",  PS_META_DUPLICATE_OK, "search by warpRun data_group", NULL);
    psMetadataAddStr(warpedArgs, PS_LIST_TAIL,  "-dist_group",  PS_META_DUPLICATE_OK, "search by warpRun dist_group", NULL);
    psMetadataAddS16(warpedArgs, PS_LIST_TAIL,  "-background_model", 0, "search by background model version", 0);
    // add magic related arguments
    pxmagicAddArguments(warpedArgs);
    // add arguments for spatial search
    pxspaceAddArguments(warpedArgs);
    psMetadataAddBool(warpedArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);

    psMetadataAddBool(warpedArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(warpedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(warpedArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    // -listrun
    psMetadata *listrunArgs = psMetadataAlloc();
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-warp_id", 0,           "search by warptool ID", 0);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL, "-tess_id",  0,          "search by tessellation ID", NULL);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL, "-state",  0,            "search by state", NULL);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure id", 0);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure tag", NULL);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-chip_id", 0,           "search by chip id", 0);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-cam_id",  0,           "search by cam id", 0);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-fake_id", 0,           "search by phase 3 version of exposure tag (fake_id)", 0);
    psMetadataAddTime(listrunArgs, PS_LIST_TAIL, "-dateobs_begin", 0,    "search for exposures by time (>=)", NULL);
    psMetadataAddTime(listrunArgs, PS_LIST_TAIL, "-dateobs_end", 0,      "search for exposures by time (<=)", NULL);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-filter", 0,           "search for exposures by filter", NULL);
    psMetadataAddS16(listrunArgs, PS_LIST_TAIL,  "-fault",  0,           "search by fault code", 0);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-data_group",  PS_META_DUPLICATE_OK, "search by warpRun data_group", NULL);
    psMetadataAddStr(listrunArgs, PS_LIST_TAIL,  "-dist_group",  PS_META_DUPLICATE_OK, "search by warpRun dist_group", NULL);

    // add magic related arguments
    pxmagicAddArguments(listrunArgs);
    // add arguments for spatial search
    pxspaceAddArguments(listrunArgs);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(listrunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    // -revertwarped
    // XXX need to allow multiple fake_ids
    // XXX need to allow multiple fake_ids
    // XXX need to allow multiple chip_ids
    // XXX need to allow multiple exp_ids
    psMetadata *revertwarpedArgs = psMetadataAlloc();
    psMetadataAddS64(revertwarpedArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddStr(revertwarpedArgs, PS_LIST_TAIL, "-skycell_id",  0,            "search by skycell ID", NULL);
    psMetadataAddStr(revertwarpedArgs, PS_LIST_TAIL, "-tess_id",  0,            "search by tessellation ID", NULL);
    psMetadataAddStr(revertwarpedArgs, PS_LIST_TAIL, "-reduction",  0,            "search by warpRun reduction class", NULL);
    psMetadataAddStr(revertwarpedArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddS16(revertwarpedArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertwarpedArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -advancerun
    psMetadata *advancerunArgs = psMetadataAlloc();
    psMetadataAddS64(advancerunArgs, PS_LIST_TAIL, "-warp_id", 0,      "search by warp ID", 0);
    psMetadataAddStr(advancerunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label ", NULL);
    psMetadataAddU64(advancerunArgs, PS_LIST_TAIL, "-limit",  0,       "limit exposures to advance to N items", 0);

    // -block
    psMetadata *blockArgs = psMetadataAlloc();
    psMetadataAddStr(blockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to mask out (required)", NULL);

    // -masked
    psMetadata *maskedArgs = psMetadataAlloc();
    psMetadataAddBool(maskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -unblock
    psMetadata *unblockArgs = psMetadataAlloc();
    psMetadataAddStr(unblockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to unmask (required)", NULL);

    // -tosummary
    psMetadata *tosummaryArgs = psMetadataAlloc();
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL, "-warp_id", 0,         "search by warp ID", 0);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-tess_id",  0,          "search by tessellation ID", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-state",  0,            "search by state", NULL);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure tag", 0);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure tag", 0);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL, "-fake_id", 0,           "search by phase 3 version of exposure tag", 0);
    psMetadataAddTime(tosummaryArgs, PS_LIST_TAIL, "-dateobs_begin", 0,    "search for exposures by time (>=)", NULL);
    psMetadataAddTime(tosummaryArgs, PS_LIST_TAIL, "-dateobs_end", 0,      "search for exposures by time (<=)", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-filter", 0,           "search for exposures by filter", NULL);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL,  "-magicked", 0,         "search by magic id", 0);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-data_group",  PS_META_DUPLICATE_OK, "search by warpRun data_group", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL,  "-dist_group",  PS_META_DUPLICATE_OK, "search by warpRun dist_group", NULL);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-not_destreaked", 0, "search for runs that have not been destreaked", false);
    
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(tosummaryArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    // -addsummary
    psMetadata *addsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(addsummaryArgs, PS_LIST_TAIL, "-warp_id", 0,         "set warp ID", 0);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-projection_cell", 0,   "set projection cell", NULL);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-path_base", 0,       "set summary path base", NULL);
    
    // -pendingcleanuprun
    psMetadata *pendingcleanuprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanuprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanuprunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);

    // -pendingcleanupskyfile
    psMetadata *pendingcleanupskyfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,          "search by warp ID (required)", 0);
    psMetadataAddBool(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-all",  0,            "list all skycells regardless of data_state", false);
    psMetadataAddBool(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -revertcleanup
    psMetadata *revertcleanupArgs = psMetadataAlloc();
    psMetadataAddS64(revertcleanupArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warptool ID", 0);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by warpRun data_group", NULL);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-state",  0,            "search by state", NULL);

    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -tocleanedskyfile
    psMetadata *tocleanedskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,    "warptool ID to update", 0);
    psMetadataAddStr(tocleanedskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);

    // -topurgedskyfile
    psMetadata *topurgedskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(topurgedskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,    "warptool ID to update", 0);
    psMetadataAddStr(topurgedskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);

    // -toscrubbedskyfile
    psMetadata *toscrubbedskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(toscrubbedskyfileArgs, PS_LIST_TAIL, "-warp_id", 0, "warptool ID to update", 0);
    psMetadataAddStr(toscrubbedskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);

    // -tofullskyfile
    psMetadata *tofullskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(tofullskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,    "warptool ID to update", 0);
    psMetadataAddStr(tofullskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update", NULL);
    psMetadataAddS64(tofullskyfileArgs, PS_LIST_TAIL, "-set_magicked",  0, "new value for magicked", 0);
    psMetadataAddS16(tofullskyfileArgs, PS_LIST_TAIL, "-set_quality", 0, "new value for quality", 0);
    psMetadataAddS16(tofullskyfileArgs, PS_LIST_TAIL, "-set_background_model", 0, "new value for background_model", 0);
    
    // -updateskyfile
    psMetadata *updateskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(updateskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,    "warptool ID to update (required)", 0);
    psMetadataAddStr(updateskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "skycell ID to update (required)", NULL);
    psMetadataAddS16(updateskyfileArgs, PS_LIST_TAIL, "-fault",  0,      "new fault code", 0);
    psMetadataAddS16(updateskyfileArgs, PS_LIST_TAIL, "-set_quality",  0,"new quality value", 0);
    psMetadataAddStr(updateskyfileArgs, PS_LIST_TAIL, "-set_state", 0,   "set state", 0);
    psMetadataAddS16(updateskyfileArgs, PS_LIST_TAIL, "-set_background_model", 0, "set the background_model value", 0);
    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-warp_id", 0,          "export this warp ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "export run in cleaned state", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);

    // -runstate
    psMetadata *runstateArgs = psMetadataAlloc();
    psMetadataAddS64(runstateArgs, PS_LIST_TAIL, "-warp_id", 0,           "search by warptool ID", 0);
//    psMetadataAddStr(runstateArgs, PS_LIST_TAIL, "-tess_id",  0,          "search by tessellation ID", NULL);
    psMetadataAddS64(runstateArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure tag", 0);
    psMetadataAddStr(runstateArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure tag", 0);
    psMetadataAddStr(runstateArgs, PS_LIST_TAIL,  "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-no_magic",  0,        "magic is not necessary for result", false);

    psMetadataAddU64(runstateArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);
//    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -setskyfiletoupdate
    psMetadata *setskyfiletoupdateArgs = psMetadataAlloc();
    psMetadataAddS64(setskyfiletoupdateArgs, PS_LIST_TAIL, "-warp_id", 0,           "search by warptool ID (required)", 0);
    psMetadataAddStr(setskyfiletoupdateArgs, PS_LIST_TAIL, "-skycell_id",  0,       "search by tessellation ID", NULL);
    psMetadataAddStr(setskyfiletoupdateArgs, PS_LIST_TAIL, "-set_label",  0,        "new value for warpRun.label", NULL);


    psFree(now);
    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",   "", WARPTOOL_MODE_DEFINEBYQUERY,  definebyqueryArgs);
    PXOPT_ADD_MODE("-definerun",       "", WARPTOOL_MODE_DEFINERUN,      definerunArgs);
    PXOPT_ADD_MODE("-updaterun",       "", WARPTOOL_MODE_UPDATERUN,      updaterunArgs);
    PXOPT_ADD_MODE("-exp",             "", WARPTOOL_MODE_EXP,            expArgs);
    PXOPT_ADD_MODE("-imfile",          "", WARPTOOL_MODE_IMFILE,         imfileArgs);
    PXOPT_ADD_MODE("-tooverlap",       "", WARPTOOL_MODE_TOOVERLAP,      tooverlapArgs);
    PXOPT_ADD_MODE("-addoverlap",      "", WARPTOOL_MODE_ADDOVERLAP,     addoverlapArgs);
    PXOPT_ADD_MODE("-revertoverlap",   "", WARPTOOL_MODE_REVERTOVERLAP,  revertoverlapArgs);
    PXOPT_ADD_MODE("-scmap",           "", WARPTOOL_MODE_SCMAP,          scmapArgs);
    PXOPT_ADD_MODE("-towarped",        "", WARPTOOL_MODE_TOWARPED,       towarpedArgs);
    PXOPT_ADD_MODE("-addwarped",       "", WARPTOOL_MODE_ADDWARPED,      addwarpedArgs);
    PXOPT_ADD_MODE("-advancerun",      "", WARPTOOL_MODE_ADVANCERUN,     advancerunArgs);
    PXOPT_ADD_MODE("-warped",          "", WARPTOOL_MODE_WARPED,         warpedArgs);
    PXOPT_ADD_MODE("-listrun",         "", WARPTOOL_MODE_LISTRUN,        listrunArgs);
    PXOPT_ADD_MODE("-runstate",        "", WARPTOOL_MODE_RUNSTATE,       runstateArgs);
    PXOPT_ADD_MODE("-revertwarped",    "", WARPTOOL_MODE_REVERTWARPED,   revertwarpedArgs);
    PXOPT_ADD_MODE("-block",                 "set a label block",                    WARPTOOL_MODE_BLOCK,          blockArgs);
    PXOPT_ADD_MODE("-masked",                "show blocked lables",                  WARPTOOL_MODE_MASKED,         maskedArgs);
    PXOPT_ADD_MODE("-unblock",               "remove a label block",                 WARPTOOL_MODE_UNBLOCK,        unblockArgs);
    PXOPT_ADD_MODE("-pendingcleanuprun",     "show runs that need to be cleaned up", WARPTOOL_MODE_PENDINGCLEANUPRUN,    pendingcleanuprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupskyfile", "show runs that need to be cleaned up", WARPTOOL_MODE_PENDINGCLEANUPSKYFILE, pendingcleanupskyfileArgs);
    PXOPT_ADD_MODE("-revertcleanup",         "revert cleanup runs with errors",     WARPTOOL_MODE_REVERTCLEANUP,          revertcleanupArgs);
    PXOPT_ADD_MODE("-donecleanup",           "show runs that have been cleaned",     WARPTOOL_MODE_DONECLEANUP,          donecleanupArgs);
    PXOPT_ADD_MODE("-tocleanedskyfile", "set skyfile as cleaned", WARPTOOL_MODE_TOCLEANEDSKYFILE, tocleanedskyfileArgs);
    PXOPT_ADD_MODE("-topurgedskyfile", "set skyfile as purged", WARPTOOL_MODE_TOPURGEDSKYFILE, topurgedskyfileArgs);
    PXOPT_ADD_MODE("-toscrubbedskyfile", "set skyfile as scrubbed", WARPTOOL_MODE_TOSCRUBBEDSKYFILE, toscrubbedskyfileArgs);
    PXOPT_ADD_MODE("-tofullskyfile", "set skyfile as full (updated)", WARPTOOL_MODE_TOFULLSKYFILE, tofullskyfileArgs);
    PXOPT_ADD_MODE("-updateskyfile", "update fault code for skyfile", WARPTOOL_MODE_UPDATESKYFILE, updateskyfileArgs);
    PXOPT_ADD_MODE("-setskyfiletoupdate", "set cleaned skyfile to be updated", WARPTOOL_MODE_SETSKYFILETOUPDATE, setskyfiletoupdateArgs);
    PXOPT_ADD_MODE("-tosummary",            "show runs that can be summarized", WARPTOOL_MODE_TOSUMMARY, tosummaryArgs);
    PXOPT_ADD_MODE("-addsummary",           "add entry to the summary table", WARPTOOL_MODE_ADDSUMMARY, addsummaryArgs);

    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", WARPTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           WARPTOOL_MODE_IMPORTRUN, importrunArgs);

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

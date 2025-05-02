/*
 * stacktoolConfig.c
 *
 * Copyright (C) 2007-2008  Joshua Hoblitt
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
#include "stacktool.h"

pxConfig *stacktoolConfig(pxConfig *config, int argc, char **argv)
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
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb", 0, "define dvodb", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_skycell_id", PS_META_DUPLICATE_OK, "search for skycell_id (LIKE comparison, multiple ok)", NULL);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_good_frac_min", 0, "define min good_frac", 0.0);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_exp_type", 0, "search for exp_type", "object");
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_label", PS_META_DUPLICATE_OK, "search by warpRun label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_data_group", PS_META_DUPLICATE_OK , "search by warpRun data_group (LIKE comparison, multiple ok)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_comment", 0, "search for comment (LIKE)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_inst", 0, "search for camera", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_telescope", 0, "search for telescope", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_filter", 0, "search for filter", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_uri", 0, "search for uri", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-select_dateobs_begin", 0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-select_dateobs_end", 0, "search for exposures by time (<)", NULL);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_airmass_min", 0, "define min airmass", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_airmass_max", 0, "define max airmass", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_sat_pixel_frac_max", 0, "define max fraction of saturated pixels", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_exp_time_min", 0, "define min exposure time", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_exp_time_max", 0, "define max exposure time", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_ccd_temp_min", 0, "define min ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_ccd_temp_max", 0, "define max ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_posang_min", 0, "define min rotator position angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_posang_max", 0, "define max rotator position angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_sun_angle_min", 0, "define min solar angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_sun_angle_max", 0, "define max solar angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_fwhm_major_min", 0, "define min fwhm (major axis)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_fwhm_major_max", 0, "define max fwhm (major axis)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_fwhm_minor_min", 0, "define min fwhm (minor axis)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_fwhm_minor_max", 0, "define max fwhm (minor axis)", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_elong_max", 0, "define max elongation from fwhm ", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_elong_max", 0, "define max elongation from iq_fwhm ", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_m2_min", 0, "define min iq_m2", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_m2_max", 0, "define max iq_m2", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_m3_min", 0, "define min iq_m3", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_m3_max", 0, "define max iq_m3", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_m4_min", 0, "define min iq_m4", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_iq_m4_max", 0, "define max iq_m4", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_zpt_obs_min", 0, "define min zero point", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_zpt_obs_max", 0, "define max zero point", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_bg_max", 0, "define max background", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_bg_stdev_max", 0, "define max background stdev", NAN);    
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_astrom", 0, "define max astrometry rms", NAN);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-warp_id", PS_META_DUPLICATE_OK, "include this warp ID (multiple OK)", 0);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-random", 0, "use this number of random elements", 0);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-min_num", 0, "minimum number of inputs", 0);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-max_num", 0, "maximum number of inputs", 0);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-min_new", 0, "minimum number of new inputs", 0);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-min_frac", 0, "minumum fraction of new inputs", NAN);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    pxspaceBoxAddArguments(definebyqueryArgs);

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_workdir", 0,            "define workdir (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_dvodb", 0, "define dvodb", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-set_registered",  0,            "time detrend run was registered", now);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-skycell_id",  0,            "define skycell ID (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-tess_id",  0,            "define tessellation ID (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-filter", 0, "define filter (required)", NULL);
    psMetadataAddS64(definerunArgs, PS_LIST_TAIL, "-warp_id",             PS_META_DUPLICATE_OK,             "include this warp ID (multiple OK, required)", 0);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-stack_id", 0,         "search by stack ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0,            "search by state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0,            "search by label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0,       "search by data_group", 0);
    psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-fault",  0,           "search by fault code", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-sass_id", 0,          "search by stack association ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0,        "define new value for label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0,        "define new state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);

    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-pretend",  0, "show query but do not actually modify the database", false);

    // -addinputskyfile
    psMetadata *addinputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "define stack ID (required)", 0);
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,            "define warp ID (required)", 0);

    // -inputskyfile
    // XXX add additional search terms
    psMetadata *inputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(inputskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "search by stack ID", 0);
    psMetadataAddS64(inputskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warp ID", 0);
    psMetadataAddU64(inputskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(inputskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -tosum
    psMetadata *tosumArgs = psMetadataAlloc();
    psMetadataAddS64(tosumArgs, PS_LIST_TAIL, "-stack_id", 0,            "search by stack ID", 0);
    psMetadataAddStr(tosumArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS64(tosumArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warp ID", 0);
    psMetadataAddU64(tosumArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tosumArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(tosumArgs, PS_LIST_TAIL, "-ignore-warp-state",  0, "generate the stack even if some inputs are not full", false);

    // -tobkg
    psMetadata *tobkgArgs = psMetadataAlloc();
    psMetadataAddS64(tobkgArgs, PS_LIST_TAIL, "-stack_id", 0,            "search by stack ID", 0);
    psMetadataAddStr(tobkgArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddU64(tobkgArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tobkgArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    
    // -addsumskyfile
    psMetadata *addsumskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(addsumskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "define stack ID (required)", 0);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-uri", 0,            "define URI of file", 0);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", 0);
    psMetadataAddF64(addsumskyfileArgs, PS_LIST_TAIL, "-bg",  0,            "define exposue background", NAN);
    psMetadataAddF64(addsumskyfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposue background mean stdev", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_stack",  0,            "define elapsed processing time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_match_mean", 0, "define mean matching time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_match_stdev", 0, "define stdev matching time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_initial", 0, "define initial stack time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_reject", 0, "define rejection time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_final", 0, "define final stack time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_convolve", 0, "define image convolution time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_phot", 0, "define photometry time", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);

    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-match_mean", 0, "define mean matching deviation", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-match_stdev", 0, "define stdev matching deviation", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-match_rms", 0, "define mean rms of deviation", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-stamps_mean", 0, "define mean number of stamps", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-stamps_stdev", 0, "define stdev of number of stamps", NAN);
    psMetadataAddS32(addsumskyfileArgs, PS_LIST_TAIL, "-stamps_min", 0, "define minimum number of stamps", 0);
    psMetadataAddS32(addsumskyfileArgs, PS_LIST_TAIL, "-reject_images", 0, "number of images rejected", 0);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-reject_pix_mean", 0, "mean number of pixels rejected", NAN);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-reject_pix_stdev", 0, "stdev number of pixels rejected", NAN);
    psMetadataAddS32(addsumskyfileArgs, PS_LIST_TAIL, "-sources", 0, "number of sources", 0);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-hostname", 0,            "define hostname", 0);
    psMetadataAddF32(addsumskyfileArgs, PS_LIST_TAIL, "-good_frac",  0,            "define %% of good pixels", NAN);
    psMetadataAddF64(addsumskyfileArgs, PS_LIST_TAIL, "-mjd_obs",  0,            "define mjd_obs (average mjd_obs of inputs)", NAN);
    psMetadataAddS16(addsumskyfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(addsumskyfileArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);

    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-ver_ppstack", 0, "define ppStack version", NULL);
    psMetadataAddStr(addsumskyfileArgs, PS_LIST_TAIL, "-ver_streaks", 0, "define streaksremove version", NULL);

    psMetadataAddS16(addsumskyfileArgs, PS_LIST_TAIL, "-background_model", 0, "define background model version", 0);
    
    // -sumskyfile
    psMetadata *sumskyfileArgs= psMetadataAlloc();
    psMetadataAddS64(sumskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "search by stack ID", 0);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-tess_id", 0,            "search by tess ID (LIKE comparison)", 0);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0,         "search by skycell ID (LIKE comparison)", 0);
#ifdef notdef
    // These don't work so omit (for now) We probably should create a different mode for this type of search.
    psMetadataAddS64(sumskyfileArgs, PS_LIST_TAIL, "-warp_id", 0,            "search by warp ID", 0);
    psMetadataAddS64(sumskyfileArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure ID", 0);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure name", NULL);
#endif
    psMetadataAddS16(sumskyfileArgs, PS_LIST_TAIL, "-background_model", 0, "search by background model version", 0);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK,             "search by stackRun.label", NULL);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK,        "search by stackRun.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-filter", 0,            "search by filter (LIKE comparison)", NULL);
    psMetadataAddF64(sumskyfileArgs, PS_LIST_TAIL, "-mjd_obs_begin",  0,      "search by mjd_obs (average mjd_obs of inputs <=)", NAN);
    psMetadataAddF64(sumskyfileArgs, PS_LIST_TAIL, "-mjd_obs_end",  0,      "search by mjd_obs (average mjd_obs of inputs>=)", NAN);
    psMetadataAddStr(sumskyfileArgs, PS_LIST_TAIL, "-state", 0,             "search by state", NULL);
    psMetadataAddS16(sumskyfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddU64(sumskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(sumskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(sumskyfileArgs, PS_LIST_TAIL, "-all",  0,            "enable search without arguments", false);

    // -sassskyfile
    psMetadata *sassskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(sassskyfileArgs, PS_LIST_TAIL, "-sass_id", 0,           "search by stack association ID", 0);
    psMetadataAddStr(sassskyfileArgs, PS_LIST_TAIL, "-tess_id", 0,            "search by tess ID (LIKE comparison)", 0);
    psMetadataAddStr(sassskyfileArgs, PS_LIST_TAIL, "-projection_cell", 0,         "search by projection cell", 0);

    psMetadataAddStr(sassskyfileArgs, PS_LIST_TAIL, "-data_group", 0,        "search by stackAssociation.data_group (LIKE comparison)", NULL);
    psMetadataAddStr(sassskyfileArgs, PS_LIST_TAIL, "-filter", 0,            "search by filter (LIKE comparison)", NULL);
    psMetadataAddU64(sassskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(sassskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(sassskyfileArgs, PS_LIST_TAIL, "-all",  0,            "enable search without arguments", false);

    // -updatesass
    psMetadata *updatesassArgs = psMetadataAlloc();
    psMetadataAddS64(updatesassArgs, PS_LIST_TAIL, "-sass_id", 0,           "search by stack association id", 0);
    psMetadataAddStr(updatesassArgs, PS_LIST_TAIL, "-data_group", 0,        "search by data group", 0);
    psMetadataAddStr(updatesassArgs, PS_LIST_TAIL, "-projection_cell", 0,   "search by projection cell", 0);
    psMetadataAddStr(updatesassArgs, PS_LIST_TAIL, "-tess_id", 0,           "search by tess_id", 0);
    psMetadataAddStr(updatesassArgs, PS_LIST_TAIL, "-filter", 0,            "search by filter", 0);

    psMetadataAddStr(updatesassArgs, PS_LIST_TAIL, "-set_data_group", 0,    "data_group to assign", 0);

    // -revertsumskyfile
    psMetadata *revertsumskyfileArgs= psMetadataAlloc();
    psMetadataAddS64(revertsumskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "search by stack ID", 0);
    psMetadataAddStr(revertsumskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddS16(revertsumskyfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertsumskyfileArgs, PS_LIST_TAIL, "-all",  0, "allow no search terms", 0);

    // -tosummary
    psMetadata *tosummaryArgs = psMetadataAlloc();
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL, "-stack_id", 0,  "search by stack ID", 0);
    psMetadataAddS64(tosummaryArgs, PS_LIST_TAIL, "-sass_id", 0,  "search by stack association ID", 0);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-tess_id", 0,   "search by tessellation ID", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-state", 0,     "search by state", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-filter", 0,    "search by filter", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by stackRun label (LIKE comparison)", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by stackRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(tosummaryArgs, PS_LIST_TAIL, "-dist_group", PS_META_DUPLICATE_OK, "search by stackRun dist_group (LIKE comparison)", NULL);

    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(tosummaryArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tosummaryArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    // -addsummary
    psMetadata *addsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(addsummaryArgs, PS_LIST_TAIL, "-sass_id", 0,      "set stack Association ID", 0);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-projection_cell", 0, "set projection cell", NULL);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-path_base", 0,     "set summary path base", NULL);

    // -revertsummary
    psMetadata *revertsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(revertsummaryArgs, PS_LIST_TAIL, "-sass_id", 0,   "search by stack association ID", 0);
    
    // -summary
    psMetadata *summaryArgs = psMetadataAlloc();
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-sass_id", 0,  "search by stack association ID", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-stack_id", 0,  "search by stack ID", 0);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-tess_id", 0,   "search by tessellation ID", NULL);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-projection_cell", 0,   "search by projection cell ID", NULL);
//    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-state", 0,     "search by state", NULL);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-filter", 0,    "search by filter (LIKE comparison)", NULL);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by stackRun label (LIKE comparison)", NULL);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by stackAssociation data_group (LIKE comparison)", NULL);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-skycell_id", 0,   "search by skycell ID", NULL);
//     psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-dist_group", PS_META_DUPLICATE_OK, "search by stackRun dist_group (LIKE comparison)", NULL);

//    psMetadataAddBool(summaryArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);
    psMetadataAddU64(summaryArgs, PS_LIST_TAIL,  "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(summaryArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    // -pendingcleanuprun
    psMetadata *pendingcleanuprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanuprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanuprunArgs, PS_LIST_TAIL, "-stack_id", 0,          "search by stack ID", 0);
    psMetadataAddS64(pendingcleanuprunArgs, PS_LIST_TAIL, "-sass_id", 0,          "search by stack association ID", 0);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanuprunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -pendingcleanupskyfile
    psMetadata *pendingcleanupskyfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,          "search by stack ID", 0);
    psMetadataAddBool(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupskyfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    // -updatesumskyfile
    psMetadata *updatesumskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(updatesumskyfileArgs, PS_LIST_TAIL, "-stack_id", 0,            "define stack ID (required)", 0);
    psMetadataAddS16(updatesumskyfileArgs, PS_LIST_TAIL, "-fault", 0,            "set fault code (required)", 0);
    psMetadataAddS16(updatesumskyfileArgs, PS_LIST_TAIL, "-set_quality", 0,            "set quality", 0);
    psMetadataAddS16(updatesumskyfileArgs, PS_LIST_TAIL, "-set_background_model", 0,   "set background model", 0);
    
    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-stack_id", 0,          "export this stack ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "mark tables as cleaned", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);

    // -importexternal
    psMetadata *importexternalArgs = psMetadataAlloc();
    psMetadataAddStr(importexternalArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);
    psMetadataAddS64(importexternalArgs, PS_LIST_TAIL, "-ext_camera_id",  0,   "external stacks come from this camera (required)", 0);
    psMetadataAddStr(importexternalArgs, PS_LIST_TAIL, "-set_filter",  0,      "replace the filter names with this value", NULL);

    // -addexternalcamera
    psMetadata *addexternalArgs = psMetadataAlloc();
    psMetadataAddStr(addexternalArgs, PS_LIST_TAIL, "-ext_camera",  0,      "external camera which can supply stacks (required)", NULL);

    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery", "Define a new stackRun by searching for warp IDs", STACKTOOL_MODE_DEFINEBYQUERY,  definebyqueryArgs);
    PXOPT_ADD_MODE("-definerun",       "", STACKTOOL_MODE_DEFINERUN,      definerunArgs);
    PXOPT_ADD_MODE("-updaterun",       "", STACKTOOL_MODE_UPDATERUN,      updaterunArgs);
    PXOPT_ADD_MODE("-addinputskyfile", "", STACKTOOL_MODE_ADDINPUTSKYFILE, addinputskyfileArgs);
    PXOPT_ADD_MODE("-inputskyfile",    "", STACKTOOL_MODE_INPUTSKYFILE,    inputskyfileArgs);
    PXOPT_ADD_MODE("-tosum",           "", STACKTOOL_MODE_TOSUM,          tosumArgs);
    PXOPT_ADD_MODE("-tobkg",           "", STACKTOOL_MODE_TOBKG,          tobkgArgs);
    PXOPT_ADD_MODE("-addsumskyfile",   "", STACKTOOL_MODE_ADDSUMSKYFILE,   addsumskyfileArgs);
    PXOPT_ADD_MODE("-sumskyfile",      "list results of stackRun", STACKTOOL_MODE_SUMSKYFILE,      sumskyfileArgs);

    PXOPT_ADD_MODE("-revertsumskyfile","", STACKTOOL_MODE_REVERTSUMSKYFILE,      revertsumskyfileArgs);
    PXOPT_ADD_MODE("-updatesumskyfile",      "update fault code for sumskyfile",     STACKTOOL_MODE_UPDATESUMSKYFILE,          updatesumskyfileArgs);

    PXOPT_ADD_MODE("-pendingcleanuprun",     "show runs that need to be cleaned up", STACKTOOL_MODE_PENDINGCLEANUPRUN,    pendingcleanuprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupskyfile", "show runs that need to be cleaned up", STACKTOOL_MODE_PENDINGCLEANUPSKYFILE, pendingcleanupskyfileArgs);
    PXOPT_ADD_MODE("-donecleanup",           "show runs that have been cleaned",     STACKTOOL_MODE_DONECLEANUP,          donecleanupArgs);

    PXOPT_ADD_MODE("-sassskyfile",          "list results of stackAssociation", STACKTOOL_MODE_SASSSKYFILE,      sassskyfileArgs);
    PXOPT_ADD_MODE("-updatesass",           "update data_group of stackAssociation", STACKTOOL_MODE_UPDATESASS,  updatesassArgs);
    PXOPT_ADD_MODE("-tosummary",            "show runs that can be summarized", STACKTOOL_MODE_TOSUMMARY, tosummaryArgs);
    PXOPT_ADD_MODE("-summary",              "show runs that have been summarized", STACKTOOL_MODE_SUMMARY, summaryArgs);
    PXOPT_ADD_MODE("-addsummary",           "add entry to the summary table", STACKTOOL_MODE_ADDSUMMARY, addsummaryArgs);
    PXOPT_ADD_MODE("-revertsummary",        "revert entry in the summary table", STACKTOOL_MODE_REVERTSUMMARY, revertsummaryArgs);
    
    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", STACKTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           STACKTOOL_MODE_IMPORTRUN, importrunArgs);

    PXOPT_ADD_MODE("-importexternal",       "import stacks from external camera from metadata file", STACKTOOL_MODE_IMPORTEXTERNAL, importexternalArgs);
    PXOPT_ADD_MODE("-addexternal",          "add external camera which can supply stacks", STACKTOOL_MODE_ADDEXTERNAL, addexternalArgs);

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

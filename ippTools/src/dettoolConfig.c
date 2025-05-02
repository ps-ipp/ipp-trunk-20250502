/*
 * dettoolConfig.c
 *
 * Copyright (C) 2006  Joshua Hoblitt
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

#include "dettool.h"

pxConfig *dettoolConfig(pxConfig *config, int argc, char **argv)
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

    // -pending
    // XXX EAM : is this used?  does it make sense?
    psMetadata *pendingArgs = psMetadataAlloc();
    psMetadataAddS64(pendingArgs, PS_LIST_TAIL, "-exp_id",  0,         "search by exposure ID", 0);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-exp_type",  0,       "search by exposure type", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-inst",  0,           "search by camera", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-telescope",  0,      "search by telescope", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-filter",  0,         "search by filter", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-uri",  0,            "search by URL", NULL);
    psMetadataAddBool(pendingArgs, PS_LIST_TAIL, "-simple",  0,        "use the simple output format", false);

    // -definebytag
    psMetadata *definebytagArgs = psMetadataAlloc();
    psMetadataAddS64(definebytagArgs, PS_LIST_TAIL, "-exp_id",            PS_META_DUPLICATE_OK,           "include this exposure (multiple OK, required)", 0);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-det_type",  0,            "define the type of detrend run (required)", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-mode",  0,            "define the mode of this detrend run (master, verify)", "master");
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-filelevel",  0,            "define filelevel", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-workdir",  0,            "define workdir (required)", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-inst",  0,            "define camera", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-telescope",  0,            "define telescope", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-exp_type",  0,            "define exposure type", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-filter",  0,            "define filter ", NULL);
    psMetadataAddF32(definebytagArgs, PS_LIST_TAIL, "-airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(definebytagArgs, PS_LIST_TAIL, "-airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF32(definebytagArgs, PS_LIST_TAIL, "-exp_time_min",  0,            "define min exposure time", NAN);
    psMetadataAddF32(definebytagArgs, PS_LIST_TAIL, "-exp_time_max",  0,            "define max exposure time", NAN);
    psMetadataAddF64(definebytagArgs, PS_LIST_TAIL, "-ccd_temp_min",  0,            "define min ccd tempature", NAN);
    psMetadataAddF64(definebytagArgs, PS_LIST_TAIL, "-ccd_temp_max",  0,            "define max ccd tempature", NAN);
    psMetadataAddF64(definebytagArgs, PS_LIST_TAIL, "-posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(definebytagArgs, PS_LIST_TAIL, "-posang_max",  0,            "define max rotator position angle", NAN);
    psMetadataAddF64(definebytagArgs, PS_LIST_TAIL, "-sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF64(definebytagArgs, PS_LIST_TAIL, "-sun_angle_max",  0,            "define max solar angle", NAN);

    psMetadataAddTime(definebytagArgs, PS_LIST_TAIL, "-registered",  0,            "time detrend run was registered", now);
    psMetadataAddTime(definebytagArgs, PS_LIST_TAIL, "-time_begin",  0,            "detrend applies to exposures taken during this period", NULL);
    psMetadataAddTime(definebytagArgs, PS_LIST_TAIL, "-time_end",  0,            "detrend applies to exposures taken during this period", NULL);
    psMetadataAddTime(definebytagArgs, PS_LIST_TAIL, "-use_begin",  0,            "start of detrend run applicable period", NULL);
    psMetadataAddTime(definebytagArgs, PS_LIST_TAIL, "-use_end",  0,            "end of detrend run applicable period", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-reduction",  0,            "define reduction class for processing", NULL);
    psMetadataAddStr(definebytagArgs, PS_LIST_TAIL, "-label",  0,            "define detrun label", NULL);
    psMetadataAddBool(definebytagArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    psMetadataAddS64(definebytagArgs, PS_LIST_TAIL, "-ref_det_id", 0,           "define reference detrend (verify mode only)", 0);
    psMetadataAddS32(definebytagArgs, PS_LIST_TAIL, "-ref_iter", 0,           "define reference detrend (verify mode only)", -1);

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-det_type",  0,            "define the type of detrend run (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-mode",  0,            "define the mode of this detrend run (master, verify)", "master");
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-filelevel",  0,            "define filelevel", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-workdir",  0,            "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-inst",  0,            "define camera (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-telescope",  0,            "define telescope", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-filter",  0,            "define filter ", NULL);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-exp_time_min",  0,            "define min exposure time", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-exp_time_max",  0,            "define max exposure time", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-ccd_temp_min",  0,            "define min ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-ccd_temp_max",  0,            "define max ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-posang_max",  0,            "define max rotator position angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-sun_angle_max",  0,            "define max solar angle", NAN);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-registered",  0,            "time detrend run was registered", now);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-time_begin",  0,            "detrend applies to exposures taken during this period (required)", NULL);
    // i'm requiring time_begin because otherwise if not specified detselect will pick the wrong detrend for old date ranges (it will pick the most recent one with NULL)
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-time_end",  0,            "detrend applies to exposures taken during this period", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-use_begin",  0,            "start of detrend run applicable period", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-use_end",  0,            "end of detrend run applicable period", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_exp_type",  0,            "search for exp_type", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_inst",  0,            "search for camera", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_telescope",  0,            "search for telescope", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_filter",  0,            "search for filter", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_uri",  0,            "search for uri", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-select_dateobs_begin", 0,            "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-select_dateobs_end", 0,            "search for exposures by time (<)", NULL);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_sat_pixel_frac_max",  0,            "define max fraction of saturated pixels", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_sat_pixel_frac_min",  0,            "define min fraction of saturated pixels", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_exp_time_min",  0,            "define min exposure time", NAN);
    psMetadataAddF32(definebyqueryArgs, PS_LIST_TAIL, "-select_exp_time_max",  0,            "define max exposure time", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_ccd_temp_min",  0,            "define min ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_ccd_temp_max",  0,            "define max ccd tempature", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_pon_time_min",  0,            "define min power-on time", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_pon_time_max",  0,            "define max power-on time", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_posang_max",  0,            "define max rotator position angle", NAN);

    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_sun_angle_max",  0,            "define max solar angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_sun_alt_min",  0,            "define min solar altitude", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_sun_alt_max",  0,            "define max solar altitude", NAN);

    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_moon_angle_min",  0,          "define min moon angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_moon_angle_max",  0,          "define max moon angle", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_moon_alt_min",  0,            "define min moon alt", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_moon_alt_max",  0,            "define max moon alt", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_moon_phase_min",  0,          "define min moon phase", NAN);
    psMetadataAddF64(definebyqueryArgs, PS_LIST_TAIL, "-select_moon_phase_max",  0,          "define max moon phase", NAN);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_state",           0,          "select by state (default is 'full')", "full");
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-comment",                0,          "search by comment field (LIKE comparison)", NULL);


    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0,            "print the exposures that would be included in the detrend run and exit", false);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-reduction",  0,            "define reduction class for processing", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label",  0,            "define detrun label", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-random_subset",  0,            "use a random subset of elements", false);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-random_limit",  0,            "use this number of random elements", 20);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-ref_det_id", 0,           "define reference detrend (verify mode only)", 0);
    psMetadataAddS32(definebyqueryArgs, PS_LIST_TAIL, "-ref_iter", 0,           "define reference detrend (verify mode only)", -1);

    // -definebydetrun
    psMetadata *definebydetrunArgs = psMetadataAlloc();
    psMetadataAddS64(definebydetrunArgs, PS_LIST_TAIL, "-det_id",  0,            "det ID to base a new detRun on (required)", 0);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_det_type",  0,            "define the type of detrend run", NULL);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_mode",  0,            "define the mode of this detrend run", "master");
    psMetadataAddS64(definebydetrunArgs, PS_LIST_TAIL, "-ref_det_id",  0,            "reference det ID ('verify' run only)", 0);
    psMetadataAddS32(definebydetrunArgs, PS_LIST_TAIL, "-ref_iter",  0,            "reference iteration ('verify' run only)", -1);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_exp_type",  0,            "define exposure type", NULL);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_filelevel",  0,            "define filelevel", NULL);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_workdir",  0,            "define workdir", NULL);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_filter",  0,            "define filter ", NULL);
    psMetadataAddF32(definebydetrunArgs, PS_LIST_TAIL, "-set_airmass_min",  0,            "define airmass", NAN);
    psMetadataAddF32(definebydetrunArgs, PS_LIST_TAIL, "-set_airmass_max",  0,            "define airmass", NAN);
    psMetadataAddF32(definebydetrunArgs, PS_LIST_TAIL, "-set_exp_time_min",  0,            "define exposure time", NAN);
    psMetadataAddF32(definebydetrunArgs, PS_LIST_TAIL, "-set_exp_time_max",  0,            "define exposure time", NAN);
    psMetadataAddF64(definebydetrunArgs, PS_LIST_TAIL, "-set_ccd_temp_min",  0,            "define ccd tempature", NAN);
    psMetadataAddF64(definebydetrunArgs, PS_LIST_TAIL, "-set_ccd_temp_max",  0,            "define ccd tempature", NAN);
    psMetadataAddF64(definebydetrunArgs, PS_LIST_TAIL, "-set_posang_min",  0,            "define rotator position angle", NAN);
    psMetadataAddF64(definebydetrunArgs, PS_LIST_TAIL, "-set_posang_max",  0,            "define rotator position angle", NAN);
    psMetadataAddF64(definebydetrunArgs, PS_LIST_TAIL, "-set_sun_angle_min",  0,            "define solar angle", NAN);
    psMetadataAddF64(definebydetrunArgs, PS_LIST_TAIL, "-set_sun_angle_max",  0,            "define solar angle", NAN);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_registered",  0,            "time detrend run was registered", now);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_time_begin",  0,            "start of period to apply detrend too", NULL);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_time_end",  0,            "end of period to apply detrend too", NULL);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_use_begin",  0,            "start of detrend run applicable period", NULL);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_use_end",  0,            "end of detrend run applicable period", NULL);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_reduction",  0,            "define reduction class for processing", NULL);
    psMetadataAddStr(definebydetrunArgs, PS_LIST_TAIL, "-set_label",  0,            "define detrun label", NULL);
    psMetadataAddBool(definebydetrunArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_input_begin", 0,     "input exp must be in this period", NULL);
    psMetadataAddTime(definebydetrunArgs, PS_LIST_TAIL, "-set_input_end", 0,       "input exp must be in this period", NULL);
    psMetadataAddBool(definebydetrunArgs, PS_LIST_TAIL, "-only_accepted", 0,       "limit input exp to those accepted in the source detRun", false);
    psMetadataAddS32(definebydetrunArgs, PS_LIST_TAIL, "-iteration",  0,            "det ID to base a new detRun on", -1);

    // *** these are in dettool_correction.c ***
    // -makecorrection
    psMetadata *makecorrectionArgs = psMetadataAlloc();
    psMetadataAddStr(makecorrectionArgs, PS_LIST_TAIL, "-det_type",  0,   "set detrend type of corrected detRun", NULL);
    psMetadataAddS64(makecorrectionArgs, PS_LIST_TAIL, "-det_id",  0,     "detRun det_id to be corrected (required)", 0);
    psMetadataAddS32(makecorrectionArgs, PS_LIST_TAIL, "-iteration",  0,  "detRun iteration to be corrected (required)", -1);;
    psMetadataAddBool(makecorrectionArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    psMetadataAddStr(makecorrectionArgs, PS_LIST_TAIL, "-workdir",  0,   "set workdir", NULL);
    psMetadataAddStr(makecorrectionArgs, PS_LIST_TAIL, "-reduction",  0, "set reduction", NULL);
    psMetadataAddStr(makecorrectionArgs, PS_LIST_TAIL, "-label",  0,     "set label", NULL);

    // -tocorrectexp
    psMetadata *tocorrectexpArgs = psMetadataAlloc();
    psMetadataAddU64(tocorrectexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tocorrectexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -tocorrectimfile
    psMetadata *tocorrectimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocorrectimfileArgs, PS_LIST_TAIL, "-det_id", 0,            "search for detrend ID", 0);
    psMetadataAddStr(tocorrectimfileArgs, PS_LIST_TAIL, "-det_type",  0,   "search by type of corrected detRun", NULL);
    psMetadataAddU64(tocorrectimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tocorrectimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addcorrectimfile
    psMetadata *addcorrectimfileArgs = psMetadataAlloc();
    psMetadataAddS64(addcorrectimfileArgs, PS_LIST_TAIL, "-det_id",  0,        "define detrend ID (required)", 0);
    psMetadataAddStr(addcorrectimfileArgs, PS_LIST_TAIL, "-class_id",  0,      "search for class ID (required)", NULL);
    psMetadataAddStr(addcorrectimfileArgs, PS_LIST_TAIL, "-uri",  0,           "define resid file URI", NULL);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addcorrectimfileArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addcorrectimfileArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);

    // -runs
    psMetadata *runsArgs = psMetadataAlloc();
    psMetadataAddStr(runsArgs, PS_LIST_TAIL, "-det_type",  0,            "search for type of detrend run", NULL);
    psMetadataAddS64(runsArgs, PS_LIST_TAIL, "-det_id", 0,            "search for detrend ID", 0);
    psMetadataAddStr(runsArgs, PS_LIST_TAIL, "-inst", 0,            "search for camera (instrument)", NULL);
    psMetadataAddStr(runsArgs, PS_LIST_TAIL, "-telescope", 0,            "search for telescope", NULL);
    psMetadataAddStr(runsArgs, PS_LIST_TAIL, "-filter", 0,            "search for filter", NULL);
    psMetadataAddF32(runsArgs, PS_LIST_TAIL, "-airmass", 0,            "match airmass", NAN);
    psMetadataAddF32(runsArgs, PS_LIST_TAIL, "-exp_time", 0,            "match exp time", NAN);
    psMetadataAddF32(runsArgs, PS_LIST_TAIL, "-ccd_temp", 0,            "match ccd temp", NAN);
    psMetadataAddF64(runsArgs, PS_LIST_TAIL, "-posang", 0,            "match posang", NAN);
    psMetadataAddStr(runsArgs, PS_LIST_TAIL, "-mode", 0,            "search for det run by mode", NULL);
    psMetadataAddBool(runsArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(runsArgs, PS_LIST_TAIL, "-active",  0,            "only return active detRuns", false);
    psMetadataAddU64(runsArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -childlessrun
    psMetadata *childlessrunArgs = psMetadataAlloc();
    psMetadataAddStr(childlessrunArgs, PS_LIST_TAIL, "-det_type",  0,            "search for type of detrend run", NULL);
    psMetadataAddU64(childlessrunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(childlessrunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -input
    psMetadata *inputArgs = psMetadataAlloc();
    psMetadataAddS64(inputArgs, PS_LIST_TAIL, "-det_id", 0,            "search for detrend ID", 0);
    psMetadataAddS32(inputArgs, PS_LIST_TAIL, "-iteration",  0,            "search for iteration number (default 0)", 0);
    psMetadataAddS64(inputArgs, PS_LIST_TAIL, "-exp_id",  0,            "search for exp ID", 0);
    psMetadataAddBool(inputArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -raw
    psMetadata *rawArgs = psMetadataAlloc();
    psMetadataAddBool(rawArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -toprocessedimfile
    psMetadata *toprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(toprocessedimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS64(toprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search for exp ID", 0);
    psMetadataAddStr(toprocessedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search for class ID", NULL);
    psMetadataAddU64(toprocessedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(toprocessedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addprocessedimfile
    psMetadata *addprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "define exp ID (required)", 0);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "define class ID (required)", NULL);
    psMetadataAddS16(addprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-uri",  0,            "define URI", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-recip",  0,            "define recipe", NULL);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-fringe_0",  0,            "define fringe slope (0th term)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-fringe_1",  0,            "define fringe slope (1st term)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-fringe_2",  0,            "define fringe slope (2nd term)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addprocessedimfileArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);

    // -processedimfile
    psMetadata *processedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(processedimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS64(processedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search for exp ID", 0);
    psMetadataAddStr(processedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search for class ID", NULL);
    psMetadataAddStr(processedimfileArgs, PS_LIST_TAIL, "-select_state",  0,            "search for state", NULL);
    psMetadataAddStr(processedimfileArgs, PS_LIST_TAIL, "-select_mode",  0,            "search for mode", NULL);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-included",  0,            "restrict results to exposures 'includeded' in the current iteration", false);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddU64(processedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertprocessedimfile
    psMetadata *revertprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(revertprocessedimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS64(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddS16(revertprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertprocessedimfileArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);
    // -updateprocessedimfile
    psMetadata *updateprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(updateprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(updateprocessedimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);
    psMetadataAddStr(updateprocessedimfileArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_processedimfile
    psMetadata *pendingcleanup_processedimfileArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_processedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_processedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_processedimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(pendingcleanup_processedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddStr(pendingcleanup_processedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);

    // -donecleanup_processedimfile
    psMetadata *donecleanup_processedimfileArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_processedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_processedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_processedimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(donecleanup_processedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddStr(donecleanup_processedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);

    // -toprocessedexp
    psMetadata *toprocessedexpArgs = psMetadataAlloc();
    psMetadataAddU64(toprocessedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(toprocessedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addproccessedexp
    psMetadata *addprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-recip",  0,            "define recipe", NULL);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-fringe_0",  0,            "define fringe slope (0th term)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-fringe_1",  0,            "define fringe slope (1st term)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-fringe_2",  0,            "define fringe slope (2nd term)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);
    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);

    // -proccessedexp
    psMetadata *processedexpArgs = psMetadataAlloc();
    psMetadataAddS64(processedexpArgs, PS_LIST_TAIL, "-det_id",  0,            "search by detrend ID", 0);
    psMetadataAddS64(processedexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exp ID", 0);
    psMetadataAddU64(processedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertprocessedexp
    psMetadata *revertprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-det_id",  0,            "search by detrend ID", 0);
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddS16(revertprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertprocessedexpArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);
    // -updateprocessedexp
    psMetadata *updateprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(updateprocessedexpArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_processedexp
    psMetadata *pendingcleanup_processedexpArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_processedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_processedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_processedexpArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(pendingcleanup_processedexpArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);

    // -donecleanup_processedexp
    psMetadata *donecleanup_processedexpArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_processedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_processedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_processedexpArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(donecleanup_processedexpArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);

    // -updatestate_processed
    psMetadata *updatestateprocessedArgs = psMetadataAlloc();
    psMetadataAddS64(updatestateprocessedArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(updatestateprocessedArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(updatestateprocessedArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -tostacked
    psMetadata *tostackedArgs = psMetadataAlloc();
    psMetadataAddU64(tostackedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tostackedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addstacked
    psMetadata *addstackedArgs = psMetadataAlloc();
    psMetadataAddS64(addstackedArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS32(addstackedArgs, PS_LIST_TAIL, "-iteration",  0,            "define iteration number (required)", 0);
    psMetadataAddStr(addstackedArgs, PS_LIST_TAIL, "-class_id",  0,            "define class ID (required)", NULL);
    psMetadataAddS16(addstackedArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddStr(addstackedArgs, PS_LIST_TAIL, "-uri",  0,            "define URI", NULL);
    psMetadataAddStr(addstackedArgs, PS_LIST_TAIL, "-recip",  0,            "define recipe", NULL);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addstackedArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);

    // -stacked
    psMetadata *stackedArgs = psMetadataAlloc();
    psMetadataAddS64(stackedArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS32(stackedArgs, PS_LIST_TAIL, "-iteration",  0,            "search for iteration number", 0);
    psMetadataAddStr(stackedArgs, PS_LIST_TAIL, "-class_id",  0,            "search for class ID", NULL);
    psMetadataAddStr(stackedArgs, PS_LIST_TAIL, "-recip",  0,            "search for recipe", NULL);
    psMetadataAddU64(stackedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(stackedArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(stackedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertstacked
    psMetadata *revertstackedArgs= psMetadataAlloc();
    psMetadataAddS64(revertstackedArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS32(revertstackedArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(revertstackedArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddS16(revertstackedArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertstackedArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);

    // -updatestacked
    psMetadata *updatestackedArgs = psMetadataAlloc();
    psMetadataAddS64(updatestackedArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(updatestackedArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(updatestackedArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddStr(updatestackedArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_stacked
    psMetadata *pendingcleanup_stackedArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_stackedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_stackedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_stackedArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(pendingcleanup_stackedArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(pendingcleanup_stackedArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);

    // -donecleanup_stacked
    psMetadata *donecleanup_stackedArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_stackedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_stackedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_stackedArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(donecleanup_stackedArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(donecleanup_stackedArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);

    // -tonormalizedstat
    psMetadata *tonormalizedstatArgs = psMetadataAlloc();
    psMetadataAddU64(tonormalizedstatArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tonormalizedstatArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addnormalizedstat
    psMetadata *addnormstatArgs = psMetadataAlloc();
    psMetadataAddS64(addnormstatArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS32(addnormstatArgs, PS_LIST_TAIL, "-iteration",  0,            "define iteration number", 0);
    psMetadataAddStr(addnormstatArgs, PS_LIST_TAIL, "-class_id",  0,            "define class ID (required)", NULL);
    psMetadataAddF32(addnormstatArgs, PS_LIST_TAIL, "-norm",  0,            "define normal value (required)", NAN);
    psMetadataAddS16(addnormstatArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);

    // -normalizedstat
    psMetadata *normalizedstatArgs = psMetadataAlloc();
    psMetadataAddS64(normalizedstatArgs, PS_LIST_TAIL, "-det_id",  0,            "search by detrend ID", 0);
    psMetadataAddS32(normalizedstatArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(normalizedstatArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddU64(normalizedstatArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(normalizedstatArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(normalizedstatArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertnormalizedstat
    psMetadata *revertnormalizedstatArgs= psMetadataAlloc();
    psMetadataAddS64(revertnormalizedstatArgs, PS_LIST_TAIL, "-det_id",  0,            "search by detrend ID", 0);
    psMetadataAddS32(revertnormalizedstatArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(revertnormalizedstatArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddS16(revertnormalizedstatArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertnormalizedstatArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);
    // -updatenormalizedstat
    psMetadata *updatenormalizedstatArgs = psMetadataAlloc();
    psMetadataAddS64(updatenormalizedstatArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(updatenormalizedstatArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(updatenormalizedstatArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);
    psMetadataAddStr(updatenormalizedstatArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_normalizedstat
    psMetadata *pendingcleanup_normalizedstatArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_normalizedstatArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_normalizedstatArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_normalizedstatArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(pendingcleanup_normalizedstatArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(pendingcleanup_normalizedstatArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);

    // -donecleanup_normalizedstat
    psMetadata *donecleanup_normalizedstatArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_normalizedstatArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_normalizedstatArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_normalizedstatArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(donecleanup_normalizedstatArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(donecleanup_normalizedstatArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);

    // -tonormalize
    psMetadata *tonormalizeArgs = psMetadataAlloc();
    psMetadataAddU64(tonormalizeArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tonormalizeArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addnormalizedimfile
    psMetadata *addnormalizedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(addnormalizedimfileArgs, PS_LIST_TAIL, "-det_id", 0,            "define detrend ID (required)", 0);
    psMetadataAddS32(addnormalizedimfileArgs, PS_LIST_TAIL, "-iteration", 0,            "define iteration number", 0);
    psMetadataAddStr(addnormalizedimfileArgs, PS_LIST_TAIL, "-class_id", 0,            "define class ID (required)", NULL);
    psMetadataAddS16(addnormalizedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddStr(addnormalizedimfileArgs, PS_LIST_TAIL, "-uri", 0,            "define URI", NULL);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-bg", 0,            "define exposure background", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-bg_stdev", 0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev", 0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addnormalizedimfileArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addnormalizedimfileArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", NULL);

    // -normalizedimfile
    psMetadata *normalizedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(normalizedimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS32(normalizedimfileArgs, PS_LIST_TAIL, "-iteration",  0,            "search for iteration number", 0);
    psMetadataAddStr(normalizedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search for class ID", NULL);
    psMetadataAddStr(normalizedimfileArgs, PS_LIST_TAIL, "-recip",  0,            "search for recipe", NULL);
    psMetadataAddU64(normalizedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(normalizedimfileArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(normalizedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertnormalizedimfile
    psMetadata *revertnormalizedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(revertnormalizedimfileArgs, PS_LIST_TAIL, "-det_id", 0,            "search by detrend ID", 0);
    psMetadataAddS32(revertnormalizedimfileArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddStr(revertnormalizedimfileArgs, PS_LIST_TAIL, "-class_id", 0,            "search by class ID", NULL);
    psMetadataAddS16(revertnormalizedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertnormalizedimfileArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);
    // -updatenormalizedimfile
    psMetadata *updatenormalizedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(updatenormalizedimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(updatenormalizedimfileArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddStr(updatenormalizedimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);
    psMetadataAddStr(updatenormalizedimfileArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_normalizedimfile
    psMetadata *pendingcleanup_normalizedimfileArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_normalizedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_normalizedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_normalizedimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(pendingcleanup_normalizedimfileArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddStr(pendingcleanup_normalizedimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);

    // -donecleanup_normalizedimfile
    psMetadata *donecleanup_normalizedimfileArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_normalizedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_normalizedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_normalizedimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(donecleanup_normalizedimfileArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddStr(donecleanup_normalizedimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);

    // -tonormalizedexp
    psMetadata *tonormalizedexpArgs = psMetadataAlloc();
    psMetadataAddU64(tonormalizedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(tonormalizedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addnormalizedexp
    psMetadata *addnormalizedexpArgs = psMetadataAlloc();
    psMetadataAddS64(addnormalizedexpArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS32(addnormalizedexpArgs, PS_LIST_TAIL, "-iteration",  0,            "define iteration number", 0);
    psMetadataAddS16(addnormalizedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddStr(addnormalizedexpArgs, PS_LIST_TAIL, "-recip",  0,            "search for recipe", NULL);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addnormalizedexpArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addnormalizedexpArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);

    // -normalizedexp
    psMetadata *normalizedexpArgs = psMetadataAlloc();
    psMetadataAddS64(normalizedexpArgs, PS_LIST_TAIL, "-det_id",  0,            "search by detrend ID", 0);
    psMetadataAddS32(normalizedexpArgs, PS_LIST_TAIL, "-iteration",  0,            "search by iteration number", 0);
    psMetadataAddStr(normalizedexpArgs, PS_LIST_TAIL, "-recip",  0,            "search for recipe", NULL);
    psMetadataAddU64(normalizedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(normalizedexpArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(normalizedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertnormalizedexp
    psMetadata *revertnormalizedexpArgs = psMetadataAlloc();
    psMetadataAddS64(revertnormalizedexpArgs, PS_LIST_TAIL, "-det_id", 0,            "search by detrend ID", 0);
    psMetadataAddS32(revertnormalizedexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddS16(revertnormalizedexpArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertnormalizedexpArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);

    // -updatenormalizedexp
    psMetadata *updatenormalizedexpArgs = psMetadataAlloc();
    psMetadataAddS64(updatenormalizedexpArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(updatenormalizedexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddStr(updatenormalizedexpArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_normalizedexp
    psMetadata *pendingcleanup_normalizedexpArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_normalizedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_normalizedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_normalizedexpArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(pendingcleanup_normalizedexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);

    // -donecleanup_normalizedexp
    psMetadata *donecleanup_normalizedexpArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_normalizedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_normalizedexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_normalizedexpArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(donecleanup_normalizedexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);

    // -toresidimfile
    psMetadata *toresidimfileArgs = psMetadataAlloc();
    psMetadataAddU64(toresidimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(toresidimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addresidimfile
    // XXX does a default value of 0 mean the entry is not instatiated? what about a supplied value of 0?
    psMetadata *addresidimfileArgs = psMetadataAlloc();
    psMetadataAddS64(addresidimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS32(addresidimfileArgs, PS_LIST_TAIL, "-iteration",  0,         "define iteration number", 0);
    psMetadataAddS64(addresidimfileArgs, PS_LIST_TAIL, "-ref_det_id",  0,        "define reference detrend ID (required)", 0);
    psMetadataAddS32(addresidimfileArgs, PS_LIST_TAIL, "-ref_iter",  0,          "define reference iteration number (required)", -1);
    psMetadataAddS64(addresidimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddStr(addresidimfileArgs, PS_LIST_TAIL, "-class_id",  0,          "define class ID (required)", NULL);
    psMetadataAddS16(addresidimfileArgs, PS_LIST_TAIL, "-fault",  0,              "set fault code", 0);
    psMetadataAddStr(addresidimfileArgs, PS_LIST_TAIL, "-uri",  0,               "define resid file URI", NULL);
    psMetadataAddStr(addresidimfileArgs, PS_LIST_TAIL, "-recip",  0,             "define recipe", NULL);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-bg",  0,                "define exposure background", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-bg_skewness",  0,            "define exposure background skewness", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-bg_kurtosis",  0,            "define exposure background kurtosis", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-bin_stdev",  0,            "define exposure background binned stdev", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-fringe_0",  0,            "define fringe slope (0th term)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-fringe_1",  0,            "define fringe slope (1st term)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-fringe_2",  0,            "define fringe slope (2nd term)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-fringe_resid_0",  0,            "define fringe residual (0th term)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-fringe_resid_1",  0,            "define fringe residual (1st term)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-fringe_resid_2",  0,            "define fringe residual (2nd term)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addresidimfileArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addresidimfileArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);

    // -residimfile
    psMetadata *residimfileArgs = psMetadataAlloc();
    psMetadataAddS64(residimfileArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS32(residimfileArgs, PS_LIST_TAIL, "-iteration",  0,            "search for iteration number", 0);
    psMetadataAddS64(residimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by detrend ID", 0);
    psMetadataAddStr(residimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search for class ID", NULL);
    psMetadataAddStr(residimfileArgs, PS_LIST_TAIL, "-recip",  0,            "search for recipe", NULL);
    psMetadataAddU64(residimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(residimfileArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(residimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(residimfileArgs, PS_LIST_TAIL, "-included",  0,            "use only files included in ths detrun", false);
    // XXX is this option used?
    psMetadataAddStr(residimfileArgs, PS_LIST_TAIL, "-select_state",  0,            "search for state", NULL);

    // -revertresidimfile
    psMetadata *revertresidimfileArgs =  psMetadataAlloc();
    psMetadataAddS64(revertresidimfileArgs, PS_LIST_TAIL, "-det_id", 0,          "search by detrend ID", 0);
    psMetadataAddS32(revertresidimfileArgs, PS_LIST_TAIL, "-iteration", 0,       "search by iteration number", 0);
    psMetadataAddS64(revertresidimfileArgs, PS_LIST_TAIL, "-exp_id",  0,         "search by detrend ID", 0);
    psMetadataAddStr(revertresidimfileArgs, PS_LIST_TAIL, "-class_id",  0,       "search for class ID", NULL);
    psMetadataAddS16(revertresidimfileArgs, PS_LIST_TAIL, "-fault",  0,           "search by fault code", 0);
    psMetadataAddBool(revertresidimfileArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);

    // -updateresidimfile
    psMetadata *updateresidimfileArgs = psMetadataAlloc();
    psMetadataAddS64(updateresidimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(updateresidimfileArgs, PS_LIST_TAIL, "-iteration", 0,       "search by iteration number", 0);
    psMetadataAddS64(updateresidimfileArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(updateresidimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);
    psMetadataAddStr(updateresidimfileArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_residimfile
    psMetadata *pendingcleanup_residimfileArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_residimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_residimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_residimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(pendingcleanup_residimfileArgs, PS_LIST_TAIL, "-iteration", 0,       "search by iteration number", 0);
    psMetadataAddS64(pendingcleanup_residimfileArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(pendingcleanup_residimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);

    // -donecleanup_residimfile
    psMetadata *donecleanup_residimfileArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_residimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_residimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_residimfileArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS32(donecleanup_residimfileArgs, PS_LIST_TAIL, "-iteration", 0,       "search by iteration number", 0);
    psMetadataAddS64(donecleanup_residimfileArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(donecleanup_residimfileArgs, PS_LIST_TAIL, "-class_id",             0,            "search by exp_name", NULL);

    // -toresidexp
    psMetadata *toresidexpArgs = psMetadataAlloc();
    psMetadataAddU64(toresidexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(toresidexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -addresidexp
    psMetadata *addresidexpArgs = psMetadataAlloc();
    psMetadataAddS64(addresidexpArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS32(addresidexpArgs, PS_LIST_TAIL, "-iteration",  0,            "define iteration number", 0);
    psMetadataAddS64(addresidexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS16(addresidexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddStr(addresidexpArgs, PS_LIST_TAIL, "-recip",  0,            "define recipe", NULL);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-bg_skewness",  0,            "define exposure background skewness", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-bg_kurtosis",  0,            "define exposure background kurtosis", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-bin_stdev",  0,            "define exposure background binned stdev", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-fringe_0",  0,            "define fringe slope (0th term)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-fringe_1",  0,            "define fringe slope (1st term)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-fringe_2",  0,            "define fringe slope (2nd term)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-fringe_resid_0",  0,            "define fringe residual (0th term)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-fringe_resid_1",  0,            "define fringe residual (1st term)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-fringe_resid_2",  0,            "define fringe residual (2nd term)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(addresidexpArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(addresidexpArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);
    psMetadataAddBool(addresidexpArgs, PS_LIST_TAIL, "-reject",  0,            "exposure is not to be stacked in the next iteration", false);

    // -residexp
    psMetadata *residexpArgs = psMetadataAlloc();
    psMetadataAddS64(residexpArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS32(residexpArgs, PS_LIST_TAIL, "-iteration",  0,            "search for iteration number", 0);
    psMetadataAddS64(residexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search for exp ID", 0);
    psMetadataAddStr(residexpArgs, PS_LIST_TAIL, "-recip",  0,            "search for recipe", NULL);
    psMetadataAddBool(residexpArgs, PS_LIST_TAIL, "-reject",  0,            "search for acceptable residuals", false);
    psMetadataAddU64(residexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(residexpArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(residexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -revertresidexp
    psMetadata *revertresidexpArgs = psMetadataAlloc();
    psMetadataAddS64(revertresidexpArgs, PS_LIST_TAIL, "-det_id", 0,            "search by detrend ID", 0);
    psMetadataAddS32(revertresidexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddS64(revertresidexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by detrend ID", 0);
    psMetadataAddS16(revertresidexpArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertresidexpArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);
    // -updateresidexp
    psMetadata *updateresidexpArgs = psMetadataAlloc();
    psMetadataAddS64(updateresidexpArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID", 0);
    psMetadataAddS32(updateresidexpArgs, PS_LIST_TAIL, "-iteration",  0,            "define iteration number", 0);
    psMetadataAddS64(updateresidexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "define exp ID", 0);
    psMetadataAddStr(updateresidexpArgs, PS_LIST_TAIL, "-recip",  0,            "define recipe", NULL);
    psMetadataAddF64(updateresidexpArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(updateresidexpArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(updateresidexpArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddStr(updateresidexpArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);
    psMetadataAddBool(updateresidexpArgs, PS_LIST_TAIL, "-reject",  0,            "exposure is not to be stacked in the next iteration", false);
    psMetadataAddStr(updateresidexpArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -pendingcleanup_residexp
    psMetadata *pendingcleanup_residexpArgs = psMetadataAlloc();
    psMetadataAddBool(pendingcleanup_residexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanup_residexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(pendingcleanup_residexpArgs, PS_LIST_TAIL, "-det_id", 0,            "search by detrend ID (required)", 0);
    psMetadataAddS32(pendingcleanup_residexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddS64(pendingcleanup_residexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by detrend ID", 0);

    // -donecleanup_residexp
    psMetadata *donecleanup_residexpArgs = psMetadataAlloc();
    psMetadataAddBool(donecleanup_residexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanup_residexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddS64(donecleanup_residexpArgs, PS_LIST_TAIL, "-det_id", 0,            "search by detrend ID (required)", 0);
    psMetadataAddS32(donecleanup_residexpArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddS64(donecleanup_residexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by detrend ID", 0);

    // -updatestate_resid
    psMetadata *updatestateresidArgs = psMetadataAlloc();
    psMetadataAddS64(updatestateresidArgs, PS_LIST_TAIL, "-det_id",               0,            "search by det ID", 0);
    psMetadataAddS64(updatestateresidArgs, PS_LIST_TAIL, "-exp_id",               0,            "search by exp_id", 0);
    psMetadataAddStr(updatestateresidArgs, PS_LIST_TAIL, "-data_state",           0,            "set state", NULL);

    // -todetrunsummary
    psMetadata *todetrunsummaryArgs = psMetadataAlloc();
    psMetadataAddBool(todetrunsummaryArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(todetrunsummaryArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -adddetrunsummary
    psMetadata *adddetrunsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(adddetrunsummaryArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddS32(adddetrunsummaryArgs, PS_LIST_TAIL, "-iteration",  0,            "define iteration number", 0);
    psMetadataAddF64(adddetrunsummaryArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(adddetrunsummaryArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(adddetrunsummaryArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddS16(adddetrunsummaryArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddBool(adddetrunsummaryArgs, PS_LIST_TAIL, "-accept",  0,            "declare that this detrun iteration is accepted as a master", false);
    psMetadataAddBool(adddetrunsummaryArgs, PS_LIST_TAIL, "-again",  0,            "start a new iteration of this detrend run", false);

    // -detrunsummary
    psMetadata *detrunsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(detrunsummaryArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend ID", 0);
    psMetadataAddS32(detrunsummaryArgs, PS_LIST_TAIL, "-iteration",  0,            "search for iteration number", 0);
    psMetadataAddU64(detrunsummaryArgs, PS_LIST_TAIL, "-limit",  0,                     "limit result set to N items", 0);
    psMetadataAddBool(detrunsummaryArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(detrunsummaryArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(detrunsummaryArgs, PS_LIST_TAIL, "-reject",  0,            "search for acceptable residuals", false);

    // -revertdetrunsummary
    psMetadata *revertdetrunsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(revertdetrunsummaryArgs, PS_LIST_TAIL, "-det_id", 0,            "search by detrend ID", 0);
    psMetadataAddS32(revertdetrunsummaryArgs, PS_LIST_TAIL, "-iteration", 0,            "search by iteration number", 0);
    psMetadataAddS16(revertdetrunsummaryArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
    psMetadataAddBool(revertdetrunsummaryArgs, PS_LIST_TAIL, "-all-run",  0,            "revert all in state 'run'", false);

    // -updatedetrunsummary
    psMetadata *updatedetrunsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(updatedetrunsummaryArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend master for detrend ID (required)", 0);
    psMetadataAddBool(updatedetrunsummaryArgs, PS_LIST_TAIL, "-accept",  0,            "declare that this detrun iteration is accepted as a master", false);
    psMetadataAddBool(updatedetrunsummaryArgs, PS_LIST_TAIL, "-reject",  0,            "reject this detrun iteration as a master", false);
    psMetadataAddStr(updatedetrunsummaryArgs, PS_LIST_TAIL, "-data_state", 0,          "set the data state", NULL);
    psMetadataAddS32(updatedetrunsummaryArgs, PS_LIST_TAIL, "-iteration", 0,           "search by iteration number", 0);

    // -pendingcleanup_detrunsummary
    psMetadata *pendingcleanup_detrunsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(pendingcleanup_detrunsummaryArgs, PS_LIST_TAIL, "-det_id", 0,     "search by detrend ID (required)", 0);
    psMetadataAddS32(pendingcleanup_detrunsummaryArgs, PS_LIST_TAIL, "-iteration", 0,  "search by iteration number", 0);
    psMetadataAddU64(pendingcleanup_detrunsummaryArgs, PS_LIST_TAIL, "-limit",  0,     "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanup_detrunsummaryArgs, PS_LIST_TAIL, "-simple", 0,    "use the simple output format", false);

    // -updatedetrun
    psMetadata *updatedetrunArgs = psMetadataAlloc();
    psMetadataAddS64(updatedetrunArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend master for detrend ID (required)", 0);
    psMetadataAddBool(updatedetrunArgs, PS_LIST_TAIL, "-again",  0,            "start a new iteration of this detrend run", false);
    psMetadataAddStr(updatedetrunArgs, PS_LIST_TAIL, "-state",  0,            "set the state of this detrend run", false);
    psMetadataAddStr(updatedetrunArgs, PS_LIST_TAIL, "-set_det_type", 0,          "set the det_type", NULL);
    psMetadataAddTime(updatedetrunArgs, PS_LIST_TAIL, "-set_time_begin",  0,            "start of period to apply detrend too", NULL);
    psMetadataAddTime(updatedetrunArgs, PS_LIST_TAIL, "-set_time_end",  0,            "end of period to apply detrend too", NULL);
    psMetadataAddTime(updatedetrunArgs, PS_LIST_TAIL, "-set_use_begin",  0,            "start of detrend run applicable period", NULL);
    psMetadataAddTime(updatedetrunArgs, PS_LIST_TAIL, "-set_use_end",  0,            "end of detrend run applicable period", NULL);

    // -rerun
    psMetadata *rerunArgs = psMetadataAlloc();
    psMetadataAddS64(rerunArgs, PS_LIST_TAIL, "-det_id",  0,            "search for detrend master for detrend ID (required)", 0);
    psMetadataAddS64(rerunArgs, PS_LIST_TAIL, "-exp_id",  PS_META_DUPLICATE_OK,            "include this exposure (multiple OK, required)", 0);

    // -register_detrend
    psMetadata *register_detrendArgs = psMetadataAlloc();
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-det_type",  0,            "define the type of detrend run (required)", NULL);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-filelevel",  0,            "define filelevel (required)", NULL);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-workdir",  0,            "define workdir", NULL);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-inst",  0,            "define camera", NULL);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-telescope",  0,            "define telescope", NULL);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-exp_type",  0,            "define exposure type", NULL);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-filter",  0,            "define filter ", NULL);
    psMetadataAddF32(register_detrendArgs, PS_LIST_TAIL, "-airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(register_detrendArgs, PS_LIST_TAIL, "-airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF32(register_detrendArgs, PS_LIST_TAIL, "-exp_time_min",  0,            "define min exposure time", NAN);
    psMetadataAddF32(register_detrendArgs, PS_LIST_TAIL, "-exp_time_max",  0,            "define max exposure time", NAN);
    psMetadataAddF32(register_detrendArgs, PS_LIST_TAIL, "-ccd_temp_min",  0,            "define min ccd tempature", NAN);
    psMetadataAddF32(register_detrendArgs, PS_LIST_TAIL, "-ccd_temp_max",  0,            "define max ccd tempature", NAN);
    psMetadataAddF64(register_detrendArgs, PS_LIST_TAIL, "-posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(register_detrendArgs, PS_LIST_TAIL, "-posang_max",  0,            "define max rotator position angle", NAN);
    psMetadataAddF64(register_detrendArgs, PS_LIST_TAIL, "-sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF64(register_detrendArgs, PS_LIST_TAIL, "-sun_angle_max",  0,            "define max solar angle", NAN);
    psMetadataAddTime(register_detrendArgs, PS_LIST_TAIL, "-registered",  0,            "time detrend run was registered", now);
    psMetadataAddTime(register_detrendArgs, PS_LIST_TAIL, "-time_begin",  0,            "detrend applies to exposures taken during this period", NULL);
    psMetadataAddTime(register_detrendArgs, PS_LIST_TAIL, "-time_end",  0,            "detrend applies to exposures taken during this period", NULL);
    psMetadataAddTime(register_detrendArgs, PS_LIST_TAIL, "-use_begin",  0,            "start of detrend run applicable period", NULL);
    psMetadataAddTime(register_detrendArgs, PS_LIST_TAIL, "-use_end",  0,            "end of detrend run applicable period", NULL);
    psMetadataAddS64(register_detrendArgs, PS_LIST_TAIL, "-ref_det_id",  0,            "define reference det_id", 0);
    psMetadataAddS32(register_detrendArgs, PS_LIST_TAIL, "-ref_iter",  0,            "define reference iteration", -1);
    psMetadataAddStr(register_detrendArgs, PS_LIST_TAIL, "-label",  0,            "define detrun label", NULL);
    psMetadataAddBool(register_detrendArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -register_detrend_imfile
    psMetadata *register_detrend_imfileArgs = psMetadataAlloc();
    psMetadataAddS64(register_detrend_imfileArgs, PS_LIST_TAIL, "-det_id",  0,            "define detrend ID (required)", 0);
    psMetadataAddStr(register_detrend_imfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search for class ID (required)", NULL);
    psMetadataAddStr(register_detrend_imfileArgs, PS_LIST_TAIL, "-uri",  0,            "define resid file URI (required)", NULL);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-user_1",  0,            "define user statistic (1)", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-user_2",  0,            "define user statistic (2)", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-user_3",  0,            "define user statistic (3)", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-user_4",  0,            "define user statistic (4)", NAN);
    psMetadataAddF64(register_detrend_imfileArgs, PS_LIST_TAIL, "-user_5",  0,            "define user statistic (5)", NAN);
    psMetadataAddStr(register_detrend_imfileArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-det_id", 0,          "export this detrend ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);


    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-pending",         "list active detruns", DETTOOL_MODE_PENDING,       pendingArgs);
    PXOPT_ADD_MODE("-definebytag",     "", DETTOOL_MODE_DEFINEBYTAG,   definebytagArgs);
    PXOPT_ADD_MODE("-definebyquery",   "", DETTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-definebydetrun",  "", DETTOOL_MODE_DEFINEBYDETRUN, definebydetrunArgs);
    PXOPT_ADD_MODE("-makecorrection",  "", DETTOOL_MODE_MAKECORRECTION, makecorrectionArgs);
    PXOPT_ADD_MODE("-tocorrectexp",    "", DETTOOL_MODE_TOCORRECTEXP, tocorrectexpArgs);
    PXOPT_ADD_MODE("-tocorrectimfile", "", DETTOOL_MODE_TOCORRECTIMFILE, tocorrectimfileArgs);
    PXOPT_ADD_MODE("-addcorrectimfile", "", DETTOOL_MODE_ADDCORRECTIMFILE, addcorrectimfileArgs);
    PXOPT_ADD_MODE("-raw",             "", DETTOOL_MODE_RAW,           rawArgs);
    PXOPT_ADD_MODE("-runs",            "", DETTOOL_MODE_RUNS,          runsArgs);
    PXOPT_ADD_MODE("-childlessrun",    "", DETTOOL_MODE_CHILDLESSRUN,  childlessrunArgs);
    PXOPT_ADD_MODE("-input",           "", DETTOOL_MODE_INPUT,         inputArgs);

    PXOPT_ADD_MODE("-toprocessedimfile", "",              DETTOOL_MODE_TOPROCESSEDIMFILE, toprocessedimfileArgs);
    PXOPT_ADD_MODE("-addprocessedimfile", "",             DETTOOL_MODE_ADDPROCESSEDIMFILE,  addprocessedimfileArgs);
    PXOPT_ADD_MODE("-processedimfile", "",                DETTOOL_MODE_PROCESSEDIMFILE, processedimfileArgs);
    PXOPT_ADD_MODE("-revertprocessedimfile", "",          DETTOOL_MODE_REVERTPROCESSEDIMFILE, revertprocessedimfileArgs);
    PXOPT_ADD_MODE("-updateprocessedimfile", "",          DETTOOL_MODE_UPDATEPROCESSEDIMFILE, updateprocessedimfileArgs);
    PXOPT_ADD_MODE("-pendingcleanup_processedimfile", "", DETTOOL_MODE_PENDINGCLEANUP_PROCESSEDIMFILE, pendingcleanup_processedimfileArgs);
    PXOPT_ADD_MODE("-donecleanup_processedimfile", "",    DETTOOL_MODE_DONECLEANUP_PROCESSEDIMFILE, donecleanup_processedimfileArgs);

    PXOPT_ADD_MODE("-toprocessedexp",  "", DETTOOL_MODE_TOPROCESSEDEXP,  toprocessedexpArgs);
    PXOPT_ADD_MODE("-addprocessedexp", "", DETTOOL_MODE_ADDPROCESSEDEXP, addprocessedexpArgs);
    PXOPT_ADD_MODE("-processedexp",    "", DETTOOL_MODE_PROCESSEDEXP, processedexpArgs);
    PXOPT_ADD_MODE("-revertprocessedexp", "", DETTOOL_MODE_REVERTPROCESSEDEXP, revertprocessedexpArgs);
    PXOPT_ADD_MODE("-updateprocessedexp", "", DETTOOL_MODE_UPDATEPROCESSEDEXP, updateprocessedexpArgs);
    PXOPT_ADD_MODE("-pendingcleanup_processedexp", "", DETTOOL_MODE_PENDINGCLEANUP_PROCESSEDEXP, pendingcleanup_processedexpArgs);
    PXOPT_ADD_MODE("-donecleanup_processedexp", "",    DETTOOL_MODE_DONECLEANUP_PROCESSEDEXP, donecleanup_processedexpArgs);
    PXOPT_ADD_MODE("-updatestate_processed", "", DETTOOL_MODE_UPDATESTATE_PROCESSED, updatestateprocessedArgs);

    PXOPT_ADD_MODE("-tostacked",       "", DETTOOL_MODE_TOSTACKED,     tostackedArgs);
    PXOPT_ADD_MODE("-addstacked",      "", DETTOOL_MODE_ADDSTACKED,    addstackedArgs);
    PXOPT_ADD_MODE("-stacked",         "", DETTOOL_MODE_STACKED,       stackedArgs);
    PXOPT_ADD_MODE("-revertstacked",   "", DETTOOL_MODE_REVERTSTACKED,  revertstackedArgs);
    PXOPT_ADD_MODE("-updatestacked",   "", DETTOOL_MODE_UPDATESTACKED,  updatestackedArgs);
    PXOPT_ADD_MODE("-pendingcleanup_stacked", "", DETTOOL_MODE_PENDINGCLEANUP_STACKED, pendingcleanup_stackedArgs);
    PXOPT_ADD_MODE("-donecleanup_stacked", "",    DETTOOL_MODE_DONECLEANUP_STACKED, donecleanup_stackedArgs);

    PXOPT_ADD_MODE("-tonormalizedstat",      "", DETTOOL_MODE_TONORMALIZEDSTAT,    tonormalizedstatArgs);
    PXOPT_ADD_MODE("-addnormalizedstat",     "", DETTOOL_MODE_ADDNORMALIZEDSTAT,   addnormstatArgs);
    PXOPT_ADD_MODE("-normalizedstat",        "", DETTOOL_MODE_NORMALIZEDSTAT,   normalizedstatArgs);
    PXOPT_ADD_MODE("-revertnormalizedstat",  "", DETTOOL_MODE_REVERTNORMALIZEDSTAT,   revertnormalizedstatArgs);
    PXOPT_ADD_MODE("-updatenormalizedstat",  "", DETTOOL_MODE_UPDATENORMALIZEDSTAT,   updatenormalizedstatArgs);
    PXOPT_ADD_MODE("-pendingcleanup_normalizedstat", "", DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDSTAT, pendingcleanup_normalizedstatArgs);
    PXOPT_ADD_MODE("-donecleanup_normalizedstat", "",    DETTOOL_MODE_DONECLEANUP_NORMALIZEDSTAT, donecleanup_normalizedstatArgs);

    PXOPT_ADD_MODE("-tonormalize",     "", DETTOOL_MODE_TONORMALIZE,   tonormalizeArgs);
    PXOPT_ADD_MODE("-addnormalizedimfile", "", DETTOOL_MODE_ADDNORMALIZEDIMFILE,addnormalizedimfileArgs);
    PXOPT_ADD_MODE("-normalizedimfile","", DETTOOL_MODE_NORMALIZEDIMFILE, normalizedimfileArgs);
    PXOPT_ADD_MODE("-revertnormalizedimfile","", DETTOOL_MODE_REVERTNORMALIZEDIMFILE, revertnormalizedimfileArgs);
    PXOPT_ADD_MODE("-updatenormalizedimfile","", DETTOOL_MODE_UPDATENORMALIZEDIMFILE, updatenormalizedimfileArgs);
    PXOPT_ADD_MODE("-pendingcleanup_normalizedimfile", "", DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDIMFILE, pendingcleanup_normalizedimfileArgs);
    PXOPT_ADD_MODE("-donecleanup_normalizedimfile", "",    DETTOOL_MODE_DONECLEANUP_NORMALIZEDIMFILE, donecleanup_normalizedimfileArgs);

    PXOPT_ADD_MODE("-tonormalizedexp", "", DETTOOL_MODE_TONORMALIZEDEXP, tonormalizedexpArgs);
    PXOPT_ADD_MODE("-addnormalizedexp", "", DETTOOL_MODE_ADDNORMALIZEDEXP, addnormalizedexpArgs);
    PXOPT_ADD_MODE("-normalizedexp",   "", DETTOOL_MODE_NORMALIZEDEXP, normalizedexpArgs);
    PXOPT_ADD_MODE("-revertnormalizedexp","", DETTOOL_MODE_REVERTNORMALIZEDEXP, revertnormalizedexpArgs);
    PXOPT_ADD_MODE("-updatenormalizedexp","", DETTOOL_MODE_UPDATENORMALIZEDEXP, updatenormalizedexpArgs);
    PXOPT_ADD_MODE("-pendingcleanup_normalizedexp", "", DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDEXP, pendingcleanup_normalizedexpArgs);
    PXOPT_ADD_MODE("-donecleanup_normalizedexp", "",    DETTOOL_MODE_DONECLEANUP_NORMALIZEDEXP, donecleanup_normalizedexpArgs);

    PXOPT_ADD_MODE("-toresidimfile",   "", DETTOOL_MODE_TORESIDIMFILE, toresidimfileArgs);
    PXOPT_ADD_MODE("-addresidimfile",  "", DETTOOL_MODE_ADDRESIDIMFILE, addresidimfileArgs);
    PXOPT_ADD_MODE("-residimfile",     "", DETTOOL_MODE_RESIDIMFILE, residimfileArgs);
    PXOPT_ADD_MODE("-revertresidimfile", "", DETTOOL_MODE_REVERTRESIDIMFILE, revertresidimfileArgs);
    PXOPT_ADD_MODE("-updateresidimfile", "", DETTOOL_MODE_UPDATERESIDIMFILE, updateresidimfileArgs);
    PXOPT_ADD_MODE("-pendingcleanup_residimfile", "", DETTOOL_MODE_PENDINGCLEANUP_RESIDIMFILE, pendingcleanup_residimfileArgs);
    PXOPT_ADD_MODE("-donecleanup_residimfile", "",    DETTOOL_MODE_DONECLEANUP_RESIDIMFILE, donecleanup_residimfileArgs);

    PXOPT_ADD_MODE("-toresidexp",      "", DETTOOL_MODE_TORESIDEXP,    toresidexpArgs);
    PXOPT_ADD_MODE("-addresidexp",     "", DETTOOL_MODE_ADDRESIDEXP,  addresidexpArgs);
    PXOPT_ADD_MODE("-residexp",        "", DETTOOL_MODE_RESIDEXP,     residexpArgs);
    PXOPT_ADD_MODE("-revertresidexp",  "", DETTOOL_MODE_REVERTRESIDEXP, revertresidexpArgs);
    PXOPT_ADD_MODE("-updateresidexp",  "", DETTOOL_MODE_UPDATERESIDEXP, updateresidexpArgs);
    PXOPT_ADD_MODE("-pendingcleanup_residexp", "", DETTOOL_MODE_PENDINGCLEANUP_RESIDEXP, pendingcleanup_residexpArgs);
    PXOPT_ADD_MODE("-donecleanup_residexp", "",    DETTOOL_MODE_DONECLEANUP_RESIDEXP, donecleanup_residexpArgs);
    PXOPT_ADD_MODE("-updatestate_resid", "", DETTOOL_MODE_UPDATESTATE_RESID, updatestateresidArgs);

    PXOPT_ADD_MODE("-todetrunsummary",  "", DETTOOL_MODE_TODETRUNSUMMARY,  todetrunsummaryArgs);
    PXOPT_ADD_MODE("-adddetrunsummary", "", DETTOOL_MODE_ADDDETRUNSUMMARY,adddetrunsummaryArgs);
    PXOPT_ADD_MODE("-detrunsummary", "", DETTOOL_MODE_DETRUNSUMMARY,detrunsummaryArgs);
    PXOPT_ADD_MODE("-revertdetrunsummary", "", DETTOOL_MODE_REVERTDETRUNSUMMARY, revertdetrunsummaryArgs);
    PXOPT_ADD_MODE("-updatedetrunsummary", "", DETTOOL_MODE_UPDATEDETRUNSUMMARY, updatedetrunsummaryArgs);

    PXOPT_ADD_MODE("-pendingcleanup_detrunsummary", "", DETTOOL_MODE_PENDINGCLEANUP_DETRUNSUMMARY, pendingcleanup_detrunsummaryArgs);

    PXOPT_ADD_MODE("-updatedetrun", "", DETTOOL_MODE_UPDATEDETRUN, updatedetrunArgs);
    PXOPT_ADD_MODE("-rerun",           "", DETTOOL_MODE_RERUN,         rerunArgs);
    PXOPT_ADD_MODE("-register_detrend", "", DETTOOL_MODE_REGISTER_DETREND, register_detrendArgs);
    PXOPT_ADD_MODE("-register_detrend_imfile", "", DETTOOL_MODE_REGISTER_DETREND_IMFILE, register_detrend_imfileArgs);
    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", DETTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           DETTOOL_MODE_IMPORTRUN, importrunArgs);

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

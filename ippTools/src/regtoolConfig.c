/*
 * regtoolConfig.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "regtool.h"

#define ADD_OPT(TYPE,TARG,NAME,COMMENT,DEFAULT) psMetadataAdd##TYPE(TARG, PS_LIST_TAIL, NAME, 0, COMMENT, DEFAULT)

pxConfig *regtoolConfig(pxConfig *config, int argc, char **argv)
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

    // -pendingimfile
    psMetadata *pendingimfileArgs = psMetadataAlloc();
    ADD_OPT(U64,  pendingimfileArgs, "-limit",     "limit result set to N items",  0);
    ADD_OPT(Bool, pendingimfileArgs, "-simple",    "use the simple output format", false);

    // -checkburntoolimfile
    psMetadata *checkburntoolimfileArgs = psMetadataAlloc();
    ADD_OPT(Str,  checkburntoolimfileArgs, "-exp_name",       "define the exp_name (required)",         NULL);
    ADD_OPT(Str,  checkburntoolimfileArgs, "-class_id",       "define class ID (required)",         NULL);
    ADD_OPT(Str,  checkburntoolimfileArgs, "-dateobs_begin",  "set the earliest summit dateobs to consider (required)", NULL);
    ADD_OPT(Str,  checkburntoolimfileArgs, "-dateobs_end",    "set the latest summit dateobs to consider (required)", NULL);
    ADD_OPT(S32,  checkburntoolimfileArgs, "-valid_burntool", "define the good burntool value (required)", 0);
    ADD_OPT(Str,  checkburntoolimfileArgs, "-inst",           "define the camera name",     NULL);
    ADD_OPT(Str,  checkburntoolimfileArgs, "-telescope",      "define the telescope name",     NULL);
    ADD_OPT(Bool, checkburntoolimfileArgs, "-simple",    "use the simple output format",          false);

    // -pendingburntoolimfile
    psMetadata *pendingburntoolimfileArgs = psMetadataAlloc();
    ADD_OPT(Str,  pendingburntoolimfileArgs, "-dateobs_begin",  "set the earliest summit dateobs to consider (required)", NULL);
    ADD_OPT(Str,  pendingburntoolimfileArgs, "-dateobs_end",    "set the latest summit dateobs to consider (required)", NULL);
    ADD_OPT(S32,  pendingburntoolimfileArgs, "-valid_burntool", "define the good burntool value (required)", 0);
    ADD_OPT(Bool, pendingburntoolimfileArgs, "-simple",    "use the simple output format",          false);
    ADD_OPT(Bool, pendingburntoolimfileArgs, "-ignore_state",   "ignore the data_state when deciding what to work on",  false);
    ADD_OPT(U64,  pendingburntoolimfileArgs, "-limit",     "limit result set to N items",  0);

    // -addprocessedimfile
    psMetadata *addprocessedimfileArgs = psMetadataAlloc();
    ADD_OPT(S64,  addprocessedimfileArgs, "-exp_id",         "define exposure ID (required)",        0);
    ADD_OPT(Str,  addprocessedimfileArgs, "-exp_name",       "define the exp_name (required)",         NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-inst",           "define the camera name (required)",     NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-telescope",      "define the telescope name (required)",     NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-tmp_class_id",   "define temp. class ID (required)",     NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-class_id",       "define class ID (required)",         NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-uri",            "define URI (required)",             NULL);

    ADD_OPT(F32,  addprocessedimfileArgs, "-longitude",      "specify the observatory longitude (NOTE: not saved in db)", NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-latitude",       "specify the observatory latitude (NOTE: not saved in db)", NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-elevation",       "specify the elevation (NOTE: not saved in db)", NAN);

    ADD_OPT(Str,  addprocessedimfileArgs, "-exp_type",       "define exposure type",             NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-filelevel",      "define filelevel",             NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-filter",         "define filter ",                 NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-comment",        "define comment ",             NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-obs_mode",       "define observing mode (data usage goal)",             NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-obs_group",      "define observing group (set of associated observations)",   NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-data_state",     "define the data_state",       "full");
    // Note: RA & DEC are supplied here in radians, but the query options use degrees.  This
    // can be justified by the fact that this option (addprocessedimfile) is a software
    // interface, but the query mechanisms are human user interfaces.
    ADD_OPT(F64,  addprocessedimfileArgs, "-ra",             "define RA (NOTE: radians)", NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-decl",           "define DEC (NOTE: radians)", NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-airmass",        "define airmass",                 NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-exp_time",       "define exposure time",             NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-sat_pixel_frac", "define fraction of saturated pixels",     NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-bg",             "define exposue background",          NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-bg_stdev",       "define exposue background stdev",     NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-bg_mean_stdev",  "define exposue background mean stdev",     NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-alt",            "define altitute",                  NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-az",             "define azimuth",                      NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-ccd_temp",       "define ccd tempature",                NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-posang",         "define rotator position angle",         NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m1_x",           "define M1 X position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m1_y",           "define M1 Y position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m1_z",           "define M1 Z position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m1_tip",         "define M1 TIP position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m1_tilt",        "define M1 TILT position",            NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m2_x",           "define M2 X position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m2_y",           "define M2 Y position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m2_z",           "define M2 Z position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m2_tip",         "define M2 TIP position",                NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-m2_tilt",        "define M2 TILT position",         NAN);

    ADD_OPT(F32,  addprocessedimfileArgs, "-env_temperature","define Environmental Temperature",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-env_humidity",   "define Environmental Humidity",         NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-env_wind_speed", "define Environmental Wind Speed",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-env_wind_dir",   "define Environmental Wind Direction",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-teltemp_m1",     "define Telescope Temperature : M1",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-teltemp_m1cell", "define Telescope Temperature : M1 Cell",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-teltemp_m2",     "define Telescope Temperature : M2",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-teltemp_spider", "define Telescope Temperature : Spider",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-teltemp_truss",  "define Telescope Temperature : Truss",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-teltemp_extra",  "define Telescope Temperature : Extra",     NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-pon_time",       "define time to last Power On",         NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-user_1",         "define user statistic (1)",         NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-user_2",         "define user statistic (2)",         NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-user_3",         "define user statistic (3)",         NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-user_4",         "define user statistic (4)",         NAN);
    ADD_OPT(F64,  addprocessedimfileArgs, "-user_5",         "define user statistic (5)",         NAN);
    ADD_OPT(Str,  addprocessedimfileArgs, "-object",         "define exposure object",             NULL);
    ADD_OPT(F32,  addprocessedimfileArgs, "-sun_angle",      "define angle to sun",             NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-sun_alt",        "define sun altitude (neg = below horizon)", NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-moon_angle",     "define angle to moon",             NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-moon_alt",       "define moon altitude (neg = below horizon)", NAN);
    ADD_OPT(F32,  addprocessedimfileArgs, "-moon_phase",     "define moon phase (0.0 = new)",   NAN);
    ADD_OPT(Bool, addprocessedimfileArgs, "-ignore",         "ignore this imfile?", false);
    ADD_OPT(Time, addprocessedimfileArgs, "-dateobs",        "define observation time",         NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-hostname",       "define host name",                NULL);
    ADD_OPT(Str,  addprocessedimfileArgs, "-md5sum",         "define md5sum",                NULL);
    ADD_OPT(S32,  addprocessedimfileArgs, "-bytes",          "define bytes",                0);
    ADD_OPT(S16,  addprocessedimfileArgs, "-burntool_state",        "set burntool state", 0);
    ADD_OPT(Bool, addprocessedimfileArgs, "-video_cells",    "define if chip has video cells", false);
    ADD_OPT(S16,  addprocessedimfileArgs, "-fault",           "set fault code",                  0);
    ADD_OPT(S16,  addprocessedimfileArgs, "-quality",        "set quality flag", 0);

    // -processedimfile
    psMetadata *processedimfileArgs = psMetadataAlloc();
    ADD_OPT(S64,  processedimfileArgs, "-exp_id",    "search by exposure ID",                 0);
    ADD_OPT(Str,  processedimfileArgs, "-exp_name",  "search by exposure name",               NULL);
    ADD_OPT(Str,  processedimfileArgs, "-class_id",  "search by class ID",                    NULL);
    ADD_OPT(Str,  processedimfileArgs, "-filter",  "search by filter",                        NULL);
    ADD_OPT(Str,  processedimfileArgs, "-obs_mode",  "search by obs_mod",                        NULL);
    ADD_OPT(Str,  processedimfileArgs, "-exp_type",  "search by exposure type",                        NULL);
    ADD_OPT(Str,  processedimfileArgs, "-data_state", "search by data_state",                 NULL);
    ADD_OPT(Time, processedimfileArgs, "-dateobs_begin", "search for exposures by time (>=)", NULL);
    ADD_OPT(Time, processedimfileArgs, "-dateobs_end", "search for exposures by time (<)", NULL);
    ADD_OPT(U64,  processedimfileArgs, "-limit",     "limit result set to N items",           0);
    ADD_OPT(Bool, processedimfileArgs, "-faulted",   "only return imfiles with a fault status set", false);
    ADD_OPT(Bool, processedimfileArgs, "-allfiles",   "return imfiles regardless of fault status", false);
    ADD_OPT(Bool, processedimfileArgs, "-all",   "list without search arguments", false);
    ADD_OPT(Bool, processedimfileArgs, "-simple",    "use the simple output format",          false);
    ADD_OPT(Bool, processedimfileArgs, "-ordered_by_date", "force output to be in DATE order", false);
    ADD_OPT(Bool, processedimfileArgs, "-video_cells",    "limit result to chips that have video cells", false);
    pxspaceAddArguments(processedimfileArgs);
    pxmagicAddArguments(processedimfileArgs);

    // -revertprocessedimfile
    psMetadata *revertprocessedimfileArgs = psMetadataAlloc();
    ADD_OPT(S64, revertprocessedimfileArgs, "-exp_id",        "search by exposure ID", 0);
    ADD_OPT(Str, revertprocessedimfileArgs, "-tmp_class_id",  "searcy by temp. class ID", NULL);
    ADD_OPT(Str, revertprocessedimfileArgs, "-class_id",      "search by class ID", NULL);
    ADD_OPT(S16, revertprocessedimfileArgs, "-fault",          "search by fault code", 0);
    ADD_OPT(S64, revertprocessedimfileArgs, "-exp_id_begin",  "search by exposure ID", 0);
    ADD_OPT(S64, revertprocessedimfileArgs, "-exp_id_end",    "search by exposure ID", 0);
    // This argument is not used but is needed because the task uses add.poll.args
    ADD_OPT(U64, revertprocessedimfileArgs, "-limit",     "for compatability not used", 0);

    // -updateprocessedimfile
    psMetadata *updateprocessedimfileArgs = psMetadataAlloc();
    ADD_OPT(S64, updateprocessedimfileArgs, "-exp_id",        "search by exposure ID", 0);
    ADD_OPT(Str, updateprocessedimfileArgs, "-class_id",      "search by class ID", NULL);
    ADD_OPT(S16, updateprocessedimfileArgs, "-burntool_state",        "set burntool state", INT16_MAX);
    ADD_OPT(S16, updateprocessedimfileArgs, "-fault",          "set fault code", INT16_MAX);
    ADD_OPT(Str, updateprocessedimfileArgs, "-hostname",       "set host name",                NULL);
    ADD_OPT(S32, updateprocessedimfileArgs, "-set_bytes",      "set bytes", INT32_MAX);
    ADD_OPT(Str, updateprocessedimfileArgs, "-set_md5sum",     "set md5sum", NULL);
    ADD_OPT(Str, updateprocessedimfileArgs, "-set_state",      "set data state", NULL);
    psMetadataAddBool(updateprocessedimfileArgs, PS_LIST_TAIL, "-set_ignored",  0,        "set imfile to be ignored for processing", false);
    psMetadataAddBool(updateprocessedimfileArgs, PS_LIST_TAIL, "-clear_ignored",  0,        "set imfile to not be ignored for processing", false);

    
    // -pendingexp
    psMetadata *pendingexpArgs = psMetadataAlloc();
    psMetadataAddU64(pendingexpArgs, PS_LIST_TAIL, "-limit",    0,        "limit result set to N items", 0);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-simple",  0,        "use the simple output format", false);

    // -addprocessedexp
    psMetadata *addprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-exp_id",           0,        "exp_id to operate on (required)", 0);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-exp_name",         0,        "define the exp_name (required)", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-inst",             0,        "define the camera name (required)", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-telescope",        0,        "define the telescope name (required)", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-exp_tag",          0,        "define the external exposure tag name (required)", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-filelevel",        0,        "define the data partitioning level of this file (required)", NULL);

    ADD_OPT(F32,  addprocessedexpArgs, "-longitude",        "specify the observatory longitude (NOTE: not saved in db)", 0.0);
    ADD_OPT(F32,  addprocessedexpArgs, "-latitude",         "specify the observatory latitude (NOTE: not saved in db)", 0.0);

    ADD_OPT(Time, addprocessedexpArgs, "-dateobs",          "define observation time", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-exp_type",         "define exposure type", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-workdir",          "define the \"default\" workdir for this exposure", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-state",            "define the state for this exposure", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-dvodb",            "define the dvodb for the next processing step", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-tess_id",          "define the tess_id for the next processing step", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-end_stage",        "define the end goal processing step", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-reduction",        "define the \"default\" reduction class for this exposure", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-filter",           "define filter ", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-comment",          "define comment ", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-obs_mode",         "define observing mode (data usage goal)",             NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-obs_group",        "define observing group (set of associated observations)",             NULL);

    // Note: RA & DEC are supplied here in radians, but the query options use degrees.  This
    // can be justified by the fact that this option (addprocessedexp) is a software
    // interface, but the query mechanisms are human user interfaces.
    ADD_OPT(F64,  addprocessedexpArgs, "-ra",               "define RA (NOTE: radians)", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-decl",             "define DEC (NOTE: radians)", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-airmass",          "define airmass", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-exp_time",         "define exposure time", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-sat_pixel_frac",   "define fraction of saturated pixels", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-bg",               "define exposue background", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-bg_stdev",         "define exposue background stdev", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-bg_mean_stdev",    "define exposue background mean stdev", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-alt",              "define altitute", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-az",               "define azimuth", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-ccd_temp",         "define ccd tempature", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-posang",           "define rotator position angle", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m1_x",             "define M1 X position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m1_y",             "define M1 Y position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m1_z",             "define M1 Z position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m1_tip",           "define M1 TIP position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m1_tilt",          "define M1 TILT position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m2_x",             "define M2 X position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m2_y",             "define M2 Y position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m2_z",             "define M2 Z position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m2_tip",           "define M2 TIP position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-m2_tilt",          "define M2 TILT position", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-env_temperature",  "define Environmental Temperature", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-env_humidity",     "define Environmental Humidity", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-env_wind_speed",   "define Environmental Wind Speed", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-env_wind_dir",     "define Environmental Wind Direction", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-teltemp_m1",       "define Telescope Temperature : M1", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-teltemp_m1cell",   "define Telescope Temperature : M1 Cell", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-teltemp_m2",       "define Telescope Temperature : M2", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-teltemp_spider",   "define Telescope Temperature : Spider", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-teltemp_truss",    "define Telescope Temperature : Truss", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-teltemp_extra",    "define Telescope Temperature : Extra", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-pon_time",         "define time to last Power On", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-user_1",           "define user statistic (1)", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-user_2",           "define user statistic (2)", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-user_3",           "define user statistic (3)", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-user_4",           "define user statistic (4)", NAN);
    ADD_OPT(F64,  addprocessedexpArgs, "-user_5",           "define user statistic (5)", NAN);
    ADD_OPT(Str,  addprocessedexpArgs, "-object",           "define exposure object", NULL);
    ADD_OPT(F32,  addprocessedexpArgs, "-sun_angle",        "define angle to sun",             NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-sun_alt",          "define sun altitude (neg = below horizon)", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-moon_angle",       "define angle to moon",             NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-moon_alt",         "define moon altitude (neg = below horizon)", NAN);
    ADD_OPT(F32,  addprocessedexpArgs, "-moon_phase",       "define moon phase (0.0 = new)",   NAN);
    ADD_OPT(Str,  addprocessedexpArgs, "-label",            "define label for chip stage (non-detrend data only)", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-data_group",       "define data_group for chip stage (non-detrend data only)", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-dist_group",       "define dist_group for chip stage (non-detrend data only)", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-chip_workdir",     "define workdir for chip stage (non-detrend data only)", NULL);
    ADD_OPT(Str,  addprocessedexpArgs, "-hostname",         "define host name", NULL);
    ADD_OPT(S16,  addprocessedexpArgs, "-fault",             "set fault code", 0);

    // -processedexp
    psMetadata *processedexpArgs = psMetadataAlloc();
    psMetadataAddS64(processedexpArgs,  PS_LIST_TAIL, "-exp_id",        0,            "search by exposure ID", 0);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-exp_name",      0,            "search by exp_name", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-inst",          0,            "search for camera", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-telescope",     0,            "search for telescope", NULL);
    psMetadataAddTime(processedexpArgs, PS_LIST_TAIL, "-dateobs_begin", 0,            "search for exposures by time (>=)", NULL);
    psMetadataAddTime(processedexpArgs, PS_LIST_TAIL, "-dateobs_end",   0,            "search for exposures by time (<)", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-state",         0,            "search by exposure state", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-exp_tag",       0,            "search by exp_tag", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-exp_type",      0,            "search by exp_type", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-filelevel",     0,            "search by filelevel", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-reduction",     0,            "search by reduction class", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-filter",        0,            "search for filter", NULL);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-airmass_min",   0,            "search by min airmass", NAN);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-airmass_max",   0,            "search by max airmass", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-ra_min",        0,            "search by min RA (degrees) ", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-ra_max",        0,            "search by max RA (degrees) ", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-decl_min",      0,            "search by min DEC (degrees)", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-decl_max",      0,            "search by max DEC (degrees)", NAN);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-exp_time_min",  0,            "search by min exposure time", NAN);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-exp_time_max",  0,            "search by max exposure time", NAN);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-sat_pixel_frac_min",  0,      "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-sat_pixel_frac_max",  0,      "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-bg_min",        0,            "search by min background", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-bg_max",        0,            "search by max background", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-bg_stdev_min",  0,            "search by min background standard deviation", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-bg_stdev_max",  0,            "search by max background standard deviation", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-bg_mean_stdev_min",  0,       "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-bg_mean_stdev_max",  0,       "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-alt_min",       0,            "search by min altitude", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-alt_max",       0,            "search by max altitude", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-az_min",        0,            "search by min azimuth ", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-az_max",        0,            "search by max azimuth ", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-ccd_temp_min",  0,            "search by min ccd tempature", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-ccd_temp_max",  0,            "search by max ccd tempature", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-posang_min",    0,            "search by min rotator position angle", NAN);
    psMetadataAddF64(processedexpArgs,  PS_LIST_TAIL, "-posang_max",    0,            "search by max rotator position angle", NAN);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-object",        0,            "search by exposure object", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-obs_mode",      0,            "search by exposure obs_mode", NULL);
    psMetadataAddStr(processedexpArgs,  PS_LIST_TAIL, "-comment",       0,            "search by exposure comment", NULL);
    
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-sun_angle_min",    0,         "define min solar angle", NAN);
    psMetadataAddF32(processedexpArgs,  PS_LIST_TAIL, "-sun_angle_max",    0,         "define max solar angle", NAN);
    pxspaceAddArguments(processedexpArgs);
    pxmagicAddArguments(processedexpArgs);

    psMetadataAddU64(processedexpArgs,  PS_LIST_TAIL, "-limit",         0,            "limit result set to N items", 0);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-faulted",       0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-simple",        0,            "use the simple output format", false);

    // -revertprocessedexp
    psMetadata *revertprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-exp_id",   0,            "search by exposure ID", 0);
    psMetadataAddS16(revertprocessedexpArgs, PS_LIST_TAIL, "-fault",     0,            "search by fault code", 0);
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-exp_id_begin",   0,      "search by exposure ID", 0);
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-exp_id_end",   0,      "search by exposure ID", 0);

    // -updatedprocessedexp
    psMetadata *updatedprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(updatedprocessedexpArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddS16(updatedprocessedexpArgs, PS_LIST_TAIL, "-fault",    0,            "set fault code (required)", INT16_MAX);
    psMetadataAddStr(updatedprocessedexpArgs, PS_LIST_TAIL, "-set_state", 0,           "set state", NULL);
    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-exp_id", 0,          "export this exposure ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);

    // -cleardupexp
    psMetadata *cleardupexpArgs = psMetadataAlloc();

    // -updatebyquery
    psMetadata *updatebyqueryArgs = psMetadataAlloc();
    
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-set_state",     0,            "set the state", NULL);
		     
    psMetadataAddS64(updatebyqueryArgs,  PS_LIST_TAIL, "-exp_id",        0,            "search by exposure ID", 0);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-exp_name",      0,            "search by exp_name", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-inst",          0,            "search for camera", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-telescope",     0,            "search for telescope", NULL);
    psMetadataAddTime(updatebyqueryArgs, PS_LIST_TAIL, "-dateobs_begin", 0,            "search for exposures by time (>=)", NULL);
    psMetadataAddTime(updatebyqueryArgs, PS_LIST_TAIL, "-dateobs_end",   0,            "search for exposures by time (<)", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-exp_tag",       0,            "search by exp_tag", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-exp_type",      0,            "search by exp_type", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-filelevel",     0,            "search by filelevel", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-state",         0,            "search by rawExp.state", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-reduction",     0,            "search by reduction class", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-filter",        0,            "search for filter", NULL);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-airmass_min",   0,            "search by min airmass", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-airmass_max",   0,            "search by max airmass", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-ra_min",        0,            "search by min RA (degrees) ", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-ra_max",        0,            "search by max RA (degrees) ", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-decl_min",      0,            "search by min DEC (degrees)", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-decl_max",      0,            "search by max DEC (degrees)", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-exp_time_min",  0,            "search by min exposure time", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-exp_time_max",  0,            "search by max exposure time", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-sat_pixel_frac_min",  0,      "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-sat_pixel_frac_max",  0,      "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-bg_min",        0,            "search by min background", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-bg_max",        0,            "search by max background", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-bg_stdev_min",  0,            "search by min background standard deviation", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-bg_stdev_max",  0,            "search by max background standard deviation", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-bg_mean_stdev_min",  0,       "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-bg_mean_stdev_max",  0,       "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-alt_min",       0,            "search by min altitude", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-alt_max",       0,            "search by max altitude", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-az_min",        0,            "search by min azimuth ", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-az_max",        0,            "search by max azimuth ", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-ccd_temp_min",  0,            "search by min ccd tempature", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-ccd_temp_max",  0,            "search by max ccd tempature", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-posang_min",    0,            "search by min rotator position angle", NAN);
    psMetadataAddF64(updatebyqueryArgs,  PS_LIST_TAIL, "-posang_max",    0,            "search by max rotator position angle", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-sun_angle_min",    0,         "search by min solar angle", NAN);
    psMetadataAddF32(updatebyqueryArgs,  PS_LIST_TAIL, "-sun_angle_max",    0,         "search by max solar angle", NAN);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-object",        0,            "search by exposure object", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-comment",        0,            "search by comment", NULL);
    psMetadataAddStr(updatebyqueryArgs,  PS_LIST_TAIL, "-obs_mode",       0,            "search by obs_mode", NULL);
    
    psMetadataAddU64(updatebyqueryArgs,  PS_LIST_TAIL, "-limit",         0,            "limit result set to N items", 0);
    psMetadataAddBool(updatebyqueryArgs, PS_LIST_TAIL, "-simple",        0,            "use the simple output format", false);

    
    // -pendingcompressimfile
    psMetadata *pendingcompressimfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingcompressimfileArgs, PS_LIST_TAIL, "-exp_id", 0,      "search by exp_id", 0);
    psMetadataAddStr(pendingcompressimfileArgs, PS_LIST_TAIL, "-class_id", 0,    "search by class ID",                    NULL);
    psMetadataAddBool(pendingcompressimfileArgs, PS_LIST_TAIL, "-simple", 0,     "use the simple output format", false);
    psMetadataAddU64(pendingcompressimfileArgs, PS_LIST_TAIL, "-limit",   0,     "limit result set to N items", 0);
    psMetadataAddBool(pendingcompressimfileArgs, PS_LIST_TAIL, "-compress", 0,   "search only for files to compress", 0);
    psMetadataAddBool(pendingcompressimfileArgs, PS_LIST_TAIL, "-clean",  0,     "search only for files to clean originals", 0);
    
    // -finishcompressexp
    psMetadata *finishcompressexpArgs = psMetadataAlloc();
    psMetadataAddS64(finishcompressexpArgs, PS_LIST_TAIL, "-exp_id", 0,       "search by exp_id", 0);
    psMetadataAddBool(finishcompressexpArgs, PS_LIST_TAIL, "-simple", 0,   "use the simple output format", false);
    psMetadataAddU64(finishcompressexpArgs, PS_LIST_TAIL, "-limit",   0,   "limit result set to N items", 0);

    // -checkstatus
    psMetadata *checkstatusArgs = psMetadataAlloc();
    ADD_OPT(Str, checkstatusArgs, "-dateobs_begin", "set the earlist summit dateobs to consider", NULL);
    ADD_OPT(Str, checkstatusArgs, "-dateobs_end", "set the latest summit dateobs to consider", NULL);
    ADD_OPT(Str, checkstatusArgs, "-date", "use default observing extent and look over entire night", NULL);
    ADD_OPT(Str, checkstatusArgs, "-class_id",    "define class ID (required)", NULL);
    ADD_OPT(Bool, checkstatusArgs, "-simple",    "use the simple output format", false);
    
    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-pendingimfile",           "", REGTOOL_MODE_PENDINGIMFILE, pendingimfileArgs);
    PXOPT_ADD_MODE("-checkburntoolimfile",     "", REGTOOL_MODE_CHECKBURNTOOLIMFILE, checkburntoolimfileArgs);
    PXOPT_ADD_MODE("-pendingburntoolimfile",   "", REGTOOL_MODE_PENDINGBURNTOOLIMFILE, pendingburntoolimfileArgs);
    PXOPT_ADD_MODE("-addprocessedimfile",      "", REGTOOL_MODE_ADDPROCESSEDIMFILE, addprocessedimfileArgs);
    PXOPT_ADD_MODE("-processedimfile",         "", REGTOOL_MODE_PROCESSEDIMFILE, processedimfileArgs);
    PXOPT_ADD_MODE("-revertprocessedimfile",   "", REGTOOL_MODE_REVERTPROCESSEDIMFILE, revertprocessedimfileArgs);
    PXOPT_ADD_MODE("-updateprocessedimfile",   "", REGTOOL_MODE_UPDATEPROCESSEDIMFILE, updateprocessedimfileArgs);
    PXOPT_ADD_MODE("-pendingexp",              "", REGTOOL_MODE_PENDINGEXP,pendingexpArgs);
    PXOPT_ADD_MODE("-addprocessedexp",         "", REGTOOL_MODE_ADDPROCESSEDEXP, addprocessedexpArgs);
    PXOPT_ADD_MODE("-processedexp",            "", REGTOOL_MODE_PROCESSEDEXP, processedexpArgs);
    PXOPT_ADD_MODE("-revertprocessedexp",      "", REGTOOL_MODE_REVERTPROCESSEDEXP, revertprocessedexpArgs);
    PXOPT_ADD_MODE("-updateprocessedexp",      "", REGTOOL_MODE_UPDATEPROCESSEDEXP,      updatedprocessedexpArgs);
    PXOPT_ADD_MODE("-cleardupexp",             "", REGTOOL_MODE_CLEARDUPEXP,      cleardupexpArgs);
    PXOPT_ADD_MODE("-updatebyquery",           "", REGTOOL_MODE_UPDATEBYQUERY, updatebyqueryArgs);
    PXOPT_ADD_MODE("-pendingcompressimfile",   "", REGTOOL_MODE_PENDINGCOMPRESSIMFILE, pendingcompressimfileArgs);
    PXOPT_ADD_MODE("-finishcompressexp",       "", REGTOOL_MODE_FINISHCOMPRESSEXP, finishcompressexpArgs);
    PXOPT_ADD_MODE("-checkstatus",             "", REGTOOL_MODE_CHECKSTATUS, checkstatusArgs);
    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", REGTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           REGTOOL_MODE_IMPORTRUN, importrunArgs);

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
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PXTOOLS_ERR_SYS, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}

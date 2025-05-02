/*
 * bgtoolConfig.c
 *
 * Copyright (C) 2006-2010  Joshua Hoblitt, Paul Price
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
#include "bgtool.h"

pxConfig *bgtoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definechip
    psMetadata *definechipArgs = psMetadataAlloc();
    psMetadataAddS64(definechipArgs, PS_LIST_TAIL, "-chip_id", 0, "search by chip_id", 0);
    psMetadataAddS64(definechipArgs, PS_LIST_TAIL, "-cam_id", 0, "choose cam_id", 0);
    psMetadataAddS64(definechipArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exp_id", 0);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-exp_name", 0, "search by exp_name", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-inst", 0, "search for camera", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-telescope", 0, "search for telescope", NULL);
    psMetadataAddTime(definechipArgs, PS_LIST_TAIL, "-dateobs_begin", 0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definechipArgs, PS_LIST_TAIL, "-dateobs_end", 0, "search for exposures by time (<)", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-exp_tag", 0, "search by exp_tag", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-exp_type", 0, "search by exp_type", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-filelevel", 0, "search by filelevel", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-filter", 0, "search for filter", NULL);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-airmass_min", 0, "search by min airmass", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-airmass_max", 0, "search by max airmass", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-ra_min", 0, "search by min RA (degrees) ", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-ra_max", 0, "search by max RA (degrees) ", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-decl_min", 0, "search by min DEC (degrees)", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-decl_max", 0, "search by max DEC (degrees)", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-exp_time_min", 0, "search by min exposure time", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-exp_time_max", 0, "search by max exposure time", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-bg_min", 0, "search by min background", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-bg_max", 0, "search by max background", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-bg_stdev_min", 0, "search by min background standard deviation", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-bg_stdev_max", 0, "search by max background standard deviation", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-bg_mean_stdev_min", 0, "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-bg_mean_stdev_max", 0, "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-alt_min", 0, "search by min altitude", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-alt_max", 0, "search by max altitude", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-az_min", 0, "search by min azimuth ", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-az_max", 0, "search by max azimuth ", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-ccd_temp_min", 0, "search by min ccd tempature", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-ccd_temp_max", 0, "search by max ccd tempature", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-posang_min", 0, "search by min rotator position angle", NAN);
    psMetadataAddF64(definechipArgs, PS_LIST_TAIL, "-posang_max", 0, "search by max rotator position angle", NAN);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-object", 0, "search by exposure object", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-comment", 0, "search by comment field (LIKE comparison)", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-obs_mode", 0, "search by observation mode", NULL);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-sun_angle_min", 0, "search by min solar angle", NAN);
    psMetadataAddF32(definechipArgs, PS_LIST_TAIL, "-sun_angle_max", 0, "search by max solar angle", NAN);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-label", 0, "search on chipRun label", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-cam_label", 0, "search on camRun label", NULL);
    psMetadataAddBool(definechipArgs, PS_LIST_TAIL, "-destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddBool(definechipArgs, PS_LIST_TAIL, "-rerun", 0, "re-run data?", false);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction class", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(definechipArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);

    psMetadataAddTime(definechipArgs, PS_LIST_TAIL, "-registered", 0, "time detrend run was registered", now);
    psMetadataAddBool(definechipArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definechipArgs, PS_LIST_TAIL, "-pretend", 0, "do not actually modify the database", false);

    // -updatechip
    psMetadata *updatechipArgs = psMetadataAlloc();
    psMetadataAddS64(updatechipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label (LIKE comparison)", NULL);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", NULL);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-dist_group", 0, "search by dist_group (LIKE comparison)", NULL);
    psMetadataAddTime(updatechipArgs, PS_LIST_TAIL, "-registered_begin", 0, "search by registration time (>=)", NULL);
    psMetadataAddTime(updatechipArgs, PS_LIST_TAIL, "-registered_end", 0, "search by registration time (<)", NULL);
    psMetadataAddBool(updatechipArgs, PS_LIST_TAIL, "-destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddS64(updatechipArgs, PS_LIST_TAIL, "-magicked",  0,        "define magicked status", 0); 
    psMetadataAddS64(updatechipArgs, PS_LIST_TAIL, "-not_destreaked",  0,        "define magicked status", 0); 
    psMetadataAddBool(updatechipArgs, PS_LIST_TAIL, "-pretend", 0, "only pretend to run the query", false);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-set_label", 0, "define new value for label", 0);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-set_data_group", 0, "define new data_group", NULL);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define new dist_group", NULL);
    psMetadataAddStr(updatechipArgs, PS_LIST_TAIL, "-set_note", 0, "define new note", NULL);

    // -tochip
    psMetadata *tochipArgs = psMetadataAlloc();
    psMetadataAddS64(tochipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddStr(tochipArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddU64(tochipArgs, PS_LIST_TAIL, "-limit",  0, "limit result set to N items", 0);
    psMetadataAddBool(tochipArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -chipinputs
    psMetadata *chipinputsArgs = psMetadataAlloc();
    psMetadataAddS64(chipinputsArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddStr(chipinputsArgs, PS_LIST_TAIL, "-class_id", 0, "search by class_id", 0);
    psMetadataAddU64(chipinputsArgs, PS_LIST_TAIL, "-limit",  0, "limit result set to N items", 0);
    psMetadataAddBool(chipinputsArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -addchip
    psMetadata *addchipArgs = psMetadataAlloc();
    psMetadataAddS64(addchipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "define chip_bg_id (required)", 0);
    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-class_id", 0, "define class_id (required)", NULL);

    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location (required)", NULL);
    psMetadataAddS64(addchipArgs, PS_LIST_TAIL, "-set_magicked",  0, "define if this skycell has been magicked", 0);
    psMetadataAddF32(addchipArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", NULL);
    psMetadataAddS16(addchipArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddS16(addchipArgs, PS_LIST_TAIL, "-fault",  0, "set fault code", 0);
    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-ver_ppbackground", 0, "define ppBackground version", NULL);
    psMetadataAddStr(addchipArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddF64(addchipArgs, PS_LIST_TAIL, "-bg", 0, "define exposure background", NAN);
    psMetadataAddF64(addchipArgs, PS_LIST_TAIL, "-bg_stdev", 0, "define exposure background stdev", NAN);
    psMetadataAddS32(addchipArgs, PS_LIST_TAIL, "-maskfrac_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(addchipArgs, PS_LIST_TAIL, "-maskfrac_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(addchipArgs, PS_LIST_TAIL, "-maskfrac_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(addchipArgs, PS_LIST_TAIL, "-maskfrac_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(addchipArgs, PS_LIST_TAIL, "-maskfrac_advisory", 0, "define advisory mask fraction", NAN);

    // -chip
    psMetadata *chipArgs = psMetadataAlloc();
    psMetadataAddS64(chipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddS64(chipArgs, PS_LIST_TAIL, "-chip_id", 0, "search by chip_id", 0);
    psMetadataAddStr(chipArgs, PS_LIST_TAIL, "-class_id", 0, "search by class_id", NULL);
    psMetadataAddS64(chipArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exp", 0);
    psMetadataAddStr(chipArgs, PS_LIST_TAIL, "-exp_name", 0, "search by exposure name", NULL);
    psMetadataAddS16(chipArgs, PS_LIST_TAIL, "-fault",  0, "search by fault code", 0);
    psMetadataAddStr(chipArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddStr(chipArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by data_group", NULL);
    psMetadataAddStr(chipArgs, PS_LIST_TAIL, "-dist_group", PS_META_DUPLICATE_OK, "search by dist_group", NULL);
    pxmagicAddArguments(chipArgs);
    pxspaceAddArguments(chipArgs);
    psMetadataAddBool(chipArgs, PS_LIST_TAIL, "-all", 0, "search without arguments", false);
    psMetadataAddU64(chipArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(chipArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -advancechip
    psMetadata *advancechipArgs = psMetadataAlloc();
    psMetadataAddS64(advancechipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddStr(advancechipArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label ", NULL);
    psMetadataAddU64(advancechipArgs, PS_LIST_TAIL, "-limit", 0, "search limit", 0);

    // -revertchip
    psMetadata *revertchipArgs = psMetadataAlloc();
    psMetadataAddS64(revertchipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddStr(revertchipArgs, PS_LIST_TAIL, "-class_id",  0, "search by class_id", NULL);
    psMetadataAddStr(revertchipArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS16(revertchipArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddBool(revertchipArgs, PS_LIST_TAIL, "-all", 0, "allow everything to be queued without search terms", false);

    // -listchip
    psMetadata *listchipArgs = psMetadataAlloc();
    psMetadataAddS64(listchipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id (required)", 0);
    psMetadataAddStr(listchipArgs, PS_LIST_TAIL, "-class_id",  0, "search by class_id", NULL);
    psMetadataAddBool(listchipArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(listchipArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -pendingcleanupchiprun
    psMetadata *pendingcleanupchiprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupchiprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list runs for specified label", NULL);
    psMetadataAddStr(pendingcleanupchiprunArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "list runs for specified data_group", NULL);
    psMetadataAddBool(pendingcleanupchiprunArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(pendingcleanupchiprunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -pendingcleanupchipimfile
    psMetadata *pendingcleanupchipimfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingcleanupchipimfileArgs, PS_LIST_TAIL, "-chip_bg_id", 0,          "search by chip_bg_id (required)", 0);
    psMetadataAddBool(pendingcleanupchipimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupchipimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -tocleanedchipimfile
    psMetadata *tocleanedchipimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedchipimfileArgs, PS_LIST_TAIL, "-chip_bg_id", 0,          "search by chip ID (required)", 0);
    psMetadataAddStr(tocleanedchipimfileArgs, PS_LIST_TAIL, "-class_id",  0, "search by class_id", NULL);
    psMetadataAddBool(tocleanedchipimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(tocleanedchipimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -updatechipimfile
    psMetadata *updatechipimfileArgs = psMetadataAlloc();
    psMetadataAddS64(updatechipimfileArgs, PS_LIST_TAIL, "-chip_bg_id", 0,          "search by chip ID (required)", 0);
    psMetadataAddStr(updatechipimfileArgs, PS_LIST_TAIL, "-class_id",  0, "search by class_id (required)", NULL);
    psMetadataAddStr(updatechipimfileArgs, PS_LIST_TAIL, "-set_data_state",  0, "new value for data_state", NULL);
    psMetadataAddS16(updatechipimfileArgs, PS_LIST_TAIL, "-set_fault",  0, "set fault code", 0);

    // -exportchip
    psMetadata *exportchipArgs = psMetadataAlloc();
    psMetadataAddS64(exportchipArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "export this chip_bg_id (required)", 0);
    psMetadataAddStr(exportchipArgs, PS_LIST_TAIL, "-outfile", 0, "export to this file (required)", NULL);
    psMetadataAddBool(exportchipArgs, PS_LIST_TAIL, "-clean", 0, "export run in cleaned state", false);

    // -importchip
    psMetadata *importchipArgs = psMetadataAlloc();
    psMetadataAddStr(importchipArgs, PS_LIST_TAIL, "-infile",  0, "import from this file (required)", NULL);



    // -definewarp
    psMetadata *definewarpArgs = psMetadataAlloc();
    psMetadataAddS64(definewarpArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp_id", 0);
    psMetadataAddS64(definewarpArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exp_id", 0);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-exp_name", 0, "search by exp_name", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-inst", 0, "search for camera", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-telescope", 0, "search for telescope", NULL);
    psMetadataAddTime(definewarpArgs, PS_LIST_TAIL, "-dateobs_begin", 0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definewarpArgs, PS_LIST_TAIL, "-dateobs_end", 0, "search for exposures by time (<)", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-exp_tag", 0, "search by exp_tag", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-exp_type", 0, "search by exp_type", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-filelevel", 0, "search by filelevel", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-filter", 0, "search for filter", NULL);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-airmass_min", 0, "search by min airmass", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-airmass_max", 0, "search by max airmass", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-ra_min", 0, "search by min RA (degrees) ", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-ra_max", 0, "search by max RA (degrees) ", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-decl_min", 0, "search by min DEC (degrees)", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-decl_max", 0, "search by max DEC (degrees)", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-exp_time_min", 0, "search by min exposure time", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-exp_time_max", 0, "search by max exposure time", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-bg_min", 0, "search by min background", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-bg_max", 0, "search by max background", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-bg_stdev_min", 0, "search by min background standard deviation", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-bg_stdev_max", 0, "search by max background standard deviation", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-bg_mean_stdev_min", 0, "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-bg_mean_stdev_max", 0, "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-alt_min", 0, "search by min altitude", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-alt_max", 0, "search by max altitude", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-az_min", 0, "search by min azimuth ", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-az_max", 0, "search by max azimuth ", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-ccd_temp_min", 0, "search by min ccd tempature", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-ccd_temp_max", 0, "search by max ccd tempature", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-posang_min", 0, "search by min rotator position angle", NAN);
    psMetadataAddF64(definewarpArgs, PS_LIST_TAIL, "-posang_max", 0, "search by max rotator position angle", NAN);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-object", 0, "search by exposure object", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-comment", 0, "search by comment field (LIKE comparison)", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-obs_mode", 0, "search by observation mode", NULL);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-sun_angle_min", 0, "search by min solar angle", NAN);
    psMetadataAddF32(definewarpArgs, PS_LIST_TAIL, "-sun_angle_max", 0, "search by max solar angle", NAN);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-warp_label", PS_META_DUPLICATE_OK, "search on warpRun label", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-chip_label", PS_META_DUPLICATE_OK, "search on chipRun label", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-chip_bg_label", PS_META_DUPLICATE_OK, "search on chipBackgroundRun label", NULL);
    psMetadataAddBool(definewarpArgs, PS_LIST_TAIL, "-destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddBool(definewarpArgs, PS_LIST_TAIL, "-rerun", 0, "rerun data?", false);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction class", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(definewarpArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);

    psMetadataAddTime(definewarpArgs, PS_LIST_TAIL, "-registered", 0, "time detrend run was registered", now);
    psMetadataAddBool(definewarpArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definewarpArgs, PS_LIST_TAIL, "-pretend", 0, "do not actually modify the database", false);
    psMetadataAddBool(definewarpArgs, PS_LIST_TAIL, "-all", 0, "allow everything to be queued without search terms", false);

    // -updatewarp
    psMetadata *updatewarpArgs = psMetadataAlloc();
    psMetadataAddS64(updatewarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id", 0);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label (LIKE comparison)", NULL);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", NULL);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-dist_group", 0, "search by dist_group (LIKE comparison)", NULL);
    psMetadataAddTime(updatewarpArgs, PS_LIST_TAIL, "-registered_begin", 0, "search by registration time (>=)", NULL);
    psMetadataAddTime(updatewarpArgs, PS_LIST_TAIL, "-registered_end", 0, "search by registration time (<)", NULL);
    psMetadataAddBool(updatewarpArgs, PS_LIST_TAIL, "-destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddBool(updatewarpArgs, PS_LIST_TAIL, "-not_destreaked", 0, "search for runs that have been destreaked", false);
    psMetadataAddS64(updatewarpArgs, PS_LIST_TAIL, "-magicked",  0,        "define magicked status", 0); 
    psMetadataAddBool(updatewarpArgs, PS_LIST_TAIL, "-pretend", 0, "only pretend to run the query", false);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-set_label", 0, "define new value for label", 0);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-set_data_group", 0, "define new data_group", NULL);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define new dist_group", NULL);
    psMetadataAddStr(updatewarpArgs, PS_LIST_TAIL, "-set_note", 0, "define new note", NULL);

    // -towarp
    psMetadata *towarpArgs = psMetadataAlloc();
    psMetadataAddS64(towarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id", 0);
    psMetadataAddStr(towarpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddU64(towarpArgs, PS_LIST_TAIL, "-limit",  0, "limit result set to N items", 0);
    psMetadataAddBool(towarpArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -warpinputs
    psMetadata *warpinputsArgs = psMetadataAlloc();
    psMetadataAddS64(warpinputsArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id", 0);
    psMetadataAddStr(warpinputsArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", 0);
    psMetadataAddU64(warpinputsArgs, PS_LIST_TAIL, "-limit",  0, "limit result set to N items", 0);
    psMetadataAddBool(warpinputsArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -addwarp
    psMetadata *addwarpArgs = psMetadataAlloc();
    psMetadataAddS64(addwarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "define warp_bg_id (required)", 0);
    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-skycell_id", 0, "define skycell_id (required)", NULL);

    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location", 0);
    psMetadataAddS64(addwarpArgs, PS_LIST_TAIL, "-set_magicked",  0, "define if this skycell has been magicked", 0);
    psMetadataAddF32(addwarpArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", 0);
    psMetadataAddS16(addwarpArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddS16(addwarpArgs, PS_LIST_TAIL, "-fault",  0, "set fault code", 0);
    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-ver_pswarp", 0, "define pswarp version", NULL);
    psMetadataAddStr(addwarpArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddF64(addwarpArgs, PS_LIST_TAIL, "-bg", 0, "define exposure background", NAN);
    psMetadataAddF64(addwarpArgs, PS_LIST_TAIL, "-bg_stdev", 0, "define exposure background stdev", NAN);
    psMetadataAddS32(addwarpArgs, PS_LIST_TAIL, "-maskfrac_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(addwarpArgs, PS_LIST_TAIL, "-maskfrac_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(addwarpArgs, PS_LIST_TAIL, "-maskfrac_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(addwarpArgs, PS_LIST_TAIL, "-maskfrac_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(addwarpArgs, PS_LIST_TAIL, "-maskfrac_advisory", 0, "define advisory mask fraction", NAN);

    // -warp
    psMetadata *warpArgs = psMetadataAlloc();
    psMetadataAddS64(warpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id", 0);
    psMetadataAddS64(warpArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp_id", 0);
    psMetadataAddS64(warpArgs, PS_LIST_TAIL, "-chip_bg_id", 0, "search by chip_bg_id", 0);
    psMetadataAddStr(warpArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", NULL);
    psMetadataAddS16(warpArgs, PS_LIST_TAIL, "-fault",  0, "search by fault code", 0);
    psMetadataAddStr(warpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddStr(warpArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by data_group", NULL);
    psMetadataAddStr(warpArgs, PS_LIST_TAIL, "-dist_group", PS_META_DUPLICATE_OK, "search by dist_group", NULL);
    pxmagicAddArguments(warpArgs);
    pxspaceAddArguments(warpArgs);
    psMetadataAddBool(warpArgs, PS_LIST_TAIL, "-all", 0, "search without arguments", false);
    psMetadataAddU64(warpArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(warpArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -advancewarp
    psMetadata *advancewarpArgs = psMetadataAlloc();
    psMetadataAddS64(advancewarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id", 0);
    psMetadataAddStr(advancewarpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label ", NULL);
    psMetadataAddU64(advancewarpArgs, PS_LIST_TAIL, "-limit", 0, "search limit", 0);

    // -revertwarp
    psMetadata *revertwarpArgs = psMetadataAlloc();
    psMetadataAddS64(revertwarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id", 0);
    psMetadataAddStr(revertwarpArgs, PS_LIST_TAIL, "-skycell_id",  0, "search by skycell_id", NULL);
    psMetadataAddStr(revertwarpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS16(revertwarpArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddBool(revertwarpArgs, PS_LIST_TAIL, "-all", 0, "allow everything to be queued without search terms", false);

    // -listwarp
    psMetadata *listwarpArgs = psMetadataAlloc();
    psMetadataAddS64(listwarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id (required)", 0);
    psMetadataAddStr(listwarpArgs, PS_LIST_TAIL, "-skycell_id",  0, "search by skycell_id", NULL);
    psMetadataAddBool(listwarpArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(listwarpArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -pendingcleanupwarprun
    psMetadata *pendingcleanupwarprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupwarprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddStr(pendingcleanupwarprunArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "list blocks for specified data_group", NULL);
    psMetadataAddBool(pendingcleanupwarprunArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(pendingcleanupwarprunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -pendingcleanupwarpskyfile
    psMetadata *pendingcleanupwarpskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingcleanupwarpskyfileArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id (required)", 0);
    psMetadataAddBool(pendingcleanupwarpskyfileArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(pendingcleanupwarpskyfileArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    
    // -tocleanwarpskyfile
    psMetadata *tocleanedwarpskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedwarpskyfileArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "search by warp_bg_id (required)", 0);
    psMetadataAddStr(tocleanedwarpskyfileArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id (required)", 0);
    psMetadataAddStr(tocleanedwarpskyfileArgs, PS_LIST_TAIL, "-state", 0, "cleaned state to set", NULL);

    // -updatewarpskyfile
    psMetadata *updatewarpskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(updatewarpskyfileArgs, PS_LIST_TAIL, "-warp_bg_id", 0,          "search by chip ID (required)", 0);
    psMetadataAddStr(updatewarpskyfileArgs, PS_LIST_TAIL, "-skycell_id",  0, "search by skycell_id (required)", NULL);
    psMetadataAddStr(updatewarpskyfileArgs, PS_LIST_TAIL, "-set_state",  0, "new value for data_state", NULL);
    psMetadataAddS16(updatewarpskyfileArgs, PS_LIST_TAIL, "-fault",  0, "set fault code", 0);

    // -exportwarp
    psMetadata *exportwarpArgs = psMetadataAlloc();
    psMetadataAddS64(exportwarpArgs, PS_LIST_TAIL, "-warp_bg_id", 0, "export this warp_bg_id (required)", 0);
    psMetadataAddStr(exportwarpArgs, PS_LIST_TAIL, "-outfile", 0, "export to this file (required)", NULL);
    psMetadataAddBool(exportwarpArgs, PS_LIST_TAIL, "-clean", 0, "export run in cleaned state", false);

    // -importwarp
    psMetadata *importwarpArgs = psMetadataAlloc();
    psMetadataAddStr(importwarpArgs, PS_LIST_TAIL, "-infile",  0, "import from this file (required)", NULL);



    psFree(now);
    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definechip",  "", BGTOOL_MODE_DEFINECHIP,  definechipArgs);
    PXOPT_ADD_MODE("-updatechip",  "", BGTOOL_MODE_UPDATECHIP,  updatechipArgs);
    PXOPT_ADD_MODE("-tochip",      "", BGTOOL_MODE_TOCHIP,      tochipArgs);
    PXOPT_ADD_MODE("-chipinputs",  "", BGTOOL_MODE_CHIPINPUTS,  chipinputsArgs);
    PXOPT_ADD_MODE("-addchip",     "", BGTOOL_MODE_ADDCHIP,     addchipArgs);
    PXOPT_ADD_MODE("-chip",        "", BGTOOL_MODE_CHIP,        chipArgs);
    PXOPT_ADD_MODE("-advancechip", "", BGTOOL_MODE_ADVANCECHIP, advancechipArgs);
    PXOPT_ADD_MODE("-revertchip",  "", BGTOOL_MODE_REVERTCHIP,  revertchipArgs);
    PXOPT_ADD_MODE("-listchip",    "", BGTOOL_MODE_LISTCHIP,    listchipArgs);
    PXOPT_ADD_MODE("-pendingcleanupchiprun", "", BGTOOL_MODE_PENDINGCLEANUPCHIPRUN, pendingcleanupchiprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupchipimfile", "", BGTOOL_MODE_PENDINGCLEANUPCHIPIMFILE, pendingcleanupchipimfileArgs);
    PXOPT_ADD_MODE("-tocleanedchipimfile", "", BGTOOL_MODE_TOCLEANEDCHIPIMFILE, tocleanedchipimfileArgs);
    PXOPT_ADD_MODE("-updatechipimfile", "", BGTOOL_MODE_UPDATECHIPIMFILE, updatechipimfileArgs);
    PXOPT_ADD_MODE("-exportchip",  "", BGTOOL_MODE_EXPORTCHIP,  exportchipArgs);
    PXOPT_ADD_MODE("-importchip",  "", BGTOOL_MODE_IMPORTCHIP,  importchipArgs);

    PXOPT_ADD_MODE("-definewarp",  "", BGTOOL_MODE_DEFINEWARP,  definewarpArgs);
    PXOPT_ADD_MODE("-updatewarp",  "", BGTOOL_MODE_UPDATEWARP,  updatewarpArgs);
    PXOPT_ADD_MODE("-towarp",      "", BGTOOL_MODE_TOWARP,      towarpArgs);
    PXOPT_ADD_MODE("-warpinputs",  "", BGTOOL_MODE_WARPINPUTS,  warpinputsArgs);
    PXOPT_ADD_MODE("-addwarp",     "", BGTOOL_MODE_ADDWARP,     addwarpArgs);
    PXOPT_ADD_MODE("-warp",        "", BGTOOL_MODE_WARP,        warpArgs);
    PXOPT_ADD_MODE("-advancewarp", "", BGTOOL_MODE_ADVANCEWARP, advancewarpArgs);
    PXOPT_ADD_MODE("-revertwarp",  "", BGTOOL_MODE_REVERTWARP,  revertwarpArgs);
    PXOPT_ADD_MODE("-listwarp",    "", BGTOOL_MODE_LISTWARP,    listwarpArgs);
    PXOPT_ADD_MODE("-pendingcleanupwarprun", "", BGTOOL_MODE_PENDINGCLEANUPWARPRUN, pendingcleanupwarprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupwarpskyfile", "", BGTOOL_MODE_PENDINGCLEANUPWARPSKYFILE, pendingcleanupwarpskyfileArgs);
    PXOPT_ADD_MODE("-tocleanedwarpskyfile", "", BGTOOL_MODE_TOCLEANEDWARPSKYFILE, tocleanedwarpskyfileArgs);
    PXOPT_ADD_MODE("-updatewarpskyfile", "", BGTOOL_MODE_UPDATEWARPSKYFILE, updatewarpskyfileArgs);
    PXOPT_ADD_MODE("-exportwarp",  "", BGTOOL_MODE_EXPORTWARP,  exportwarpArgs);
    PXOPT_ADD_MODE("-importwarp",  "", BGTOOL_MODE_IMPORTWARP,  importwarpArgs);

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

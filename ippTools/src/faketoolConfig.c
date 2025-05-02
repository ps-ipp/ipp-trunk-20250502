/*
 * faketoolConfig.c
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

#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "faketool.h"

pxConfig *faketoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definebyquery
    psMetadata *queueArgs = psMetadataAlloc();
    // XXX need to allow multiple exp_ids
    psMetadataAddS64(queueArgs, PS_LIST_TAIL, "-cam_id",             0, "search by cam_id", 0);
    psMetadataAddS64(queueArgs, PS_LIST_TAIL, "-chip_id",            0, "search by chip_id", 0);
    psMetadataAddS64(queueArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exp_id", 0);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-exp_name",  0,            "search by exp_name", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-inst",  0,            "search for camera", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-telescope",  0,            "search for telescope", NULL);
    psMetadataAddTime(queueArgs, PS_LIST_TAIL, "-dateobs_begin", 0,            "search for exposures by time (>=)", NULL);
    psMetadataAddTime(queueArgs, PS_LIST_TAIL, "-dateobs_end", 0,            "search for exposures by time (<)", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-exp_tag",  0,            "search by exp_tag", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-exp_type",  0,            "search by exp_type", "object");
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-filelevel",  0,            "search by filelevel", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-reduction",  0,            "search by reduction class", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-filter",  0,            "search for filter", NULL);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-ra_min",  0,            "define min RA (degrees)", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-ra_max",  0,            "define max RA (degrees)", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-decl_min",  0,            "define min DEC (degrees)", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-decl_max",  0,            "define max DEC (degrees)", NAN);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-exp_time_min",  0,            "define min", NAN);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-exp_time_max",  0,            "define max", NAN);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-sat_pixel_frac_min",  0,            "define max fraction of saturated pixels", NAN);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-sat_pixel_frac_max",  0,            "define min fraction of saturated pixels", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-bg_min",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-bg_max",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-bg_stdev_min",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-bg_stdev_max",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-bg_mean_stdev_min",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-bg_mean_stdev_max",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-alt_min",  0,            "define min", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-alt_max",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-az_min",  0,            "define min", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-az_max",  0,            "define max", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-ccd_temp_min",  0,            "define min ccd tempature", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-ccd_temp_max",  0,            "define max ccd tempature", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(queueArgs, PS_LIST_TAIL, "-posang_max",  0,            "define max rotator position angle", NAN);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-object",  0,            "search by exposure object", NULL);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF32(queueArgs, PS_LIST_TAIL, "-sun_angle_max",  0,            "define max solar angle", NAN);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_workdir",  0,            "define workdir", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_label",  0,            "define label", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_reduction",  0,            "define reduction class", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_expgroup",  0,            "define exposure group", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_dvodb",  0,            "define DVO db", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_tess_id",  0,            "define tessellation identifier", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_end_stage",  0,            "define end stage", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_dist_group",  0,           "define dist group", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_data_group",  0,           "define data group", NULL);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-set_note",  0,                 "define note", NULL);
    psMetadataAddBool(queueArgs, PS_LIST_TAIL, "-pretend",  0,            "do not actually modify the database", false);
    psMetadataAddBool(queueArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);
    psMetadataAddStr(queueArgs, PS_LIST_TAIL, "-comment",     0,        "search by comment field (LIKE comparison)", NULL);


    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-fake_id", 0,            "search by fake ID", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exp_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-exp_name",  0,            "search by exp_name", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-inst",  0,            "search for camera", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-telescope",  0,            "search for telescope", NULL);
    psMetadataAddTime(updaterunArgs, PS_LIST_TAIL, "-dateobs_begin", 0,            "search for exposures by time (>=)", NULL);
    psMetadataAddTime(updaterunArgs, PS_LIST_TAIL, "-dateobs_end", 0,            "search for exposures by time (<)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-exp_tag",  0,            "search by exp_tag", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-exp_type",  0,            "search by exp_type", "object");
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-filelevel",  0,            "search by filelevel", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-reduction",  0,            "search by reduction class", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-filter",  0,            "search for filter", NULL);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-ra_min",  0,            "define min RA (degrees)", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-ra_max",  0,            "define max RA (degrees)", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-decl_min",  0,            "define min DEC (degrees)", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-decl_max",  0,            "define max DEC (degrees)", NAN);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-exp_time_min",  0,            "define min", NAN);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-exp_time_max",  0,            "define max", NAN);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-sat_pixel_frac_min",  0,            "define max fraction of saturated pixels", NAN);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-sat_pixel_frac_max",  0,            "define min fraction of saturated pixels", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-bg_min",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-bg_max",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-bg_stdev_min",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-bg_stdev_max",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-bg_mean_stdev_min",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-bg_mean_stdev_max",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-alt_min",  0,            "define min", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-alt_max",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-az_min",  0,            "define min", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-az_max",  0,            "define max", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-ccd_temp_min",  0,            "define min ccd tempature", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-ccd_temp_max",  0,            "define max ccd tempature", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(updaterunArgs, PS_LIST_TAIL, "-posang_max",  0,            "define max rotator position angle", NAN);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-object",  0,            "search by exposure object", NULL);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-sun_angle_max",  0,            "define max solar angle", NAN);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0, "search by label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dist_group", 0, "search by data_group", NULL);

    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0,            "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0,            "set label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);

    // -pendingexp
    psMetadata *pendingexpArgs = psMetadataAlloc();
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-fake_id", 0,            "search by fake ID", 0);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-cam_id", 0,            "search by camtool ID", 0);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-chip_id", 0,            "search by chiptool ID", 0);
    psMetadataAddU64(pendingexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -pendingimfile
    psMetadata *pendingimfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingimfileArgs, PS_LIST_TAIL, "-fake_id",  0,            "search by fake ID", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS64(pendingimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-inst",  0,            "search by camera of interest", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-filter",  0,            "search by filter of interest", NULL);
    psMetadataAddU64(pendingimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -addprocessedimfile
    psMetadata *addprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-fake_id",  0,            "define fake ID (required)", 0);
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "define exposure ID (required)", 0);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "define class ID (required)", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-uri",  0,            "define URL", NULL);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-dtime_fake",  0,            "define elapsed time for detrend (seconds)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-hostname",  0,            "define hostname", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);
    psMetadataAddS16(addprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);

    // -processedimfile
    psMetadata *processedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(processedimfileArgs, PS_LIST_TAIL, "-fake_id",  0,            "define fake ID", 0);
    psMetadataAddS64(processedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "define exposure ID", 0);
    psMetadataAddStr(processedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "define class ID", NULL);
    psMetadataAddStr(processedimfileArgs, PS_LIST_TAIL, "-inst",  0,            "define camera of interest", NULL);
    psMetadataAddStr(processedimfileArgs, PS_LIST_TAIL, "-filter",  0,            "define filter of interest", NULL);
    psMetadataAddU64(processedimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -updateprocessedimfile
    psMetadata *updateprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedimfileArgs, PS_LIST_TAIL, "-fake_id",  0,            "search by fake ID", 0);
    psMetadataAddS64(updateprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exposure ID", 0);
    psMetadataAddStr(updateprocessedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddS16(updateprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code (required)", 0);

    // -revertprocessedimfile
    psMetadata *revertprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(revertprocessedimfileArgs, PS_LIST_TAIL, "-fake_id", 0,            "search by fake ID", 0);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS64(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "search by exp_id", 0);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_name",  0,            "search by exp_name", NULL);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-inst",  0,            "search for camera", NULL);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-telescope",  0,            "search for telescope", NULL);
    psMetadataAddTime(revertprocessedimfileArgs, PS_LIST_TAIL, "-dateobs_begin", 0,            "search for exposures by time (>=)", NULL);
    psMetadataAddTime(revertprocessedimfileArgs, PS_LIST_TAIL, "-dateobs_end", 0,            "search for exposures by time (<)", NULL);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_tag",  0,            "search by exp_tag", NULL);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_type",  0,            "search by exp_type", "object");
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-filelevel",  0,            "search by filelevel", NULL);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-reduction",  0,            "search by reduction class", NULL);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-filter",  0,            "search for filter", NULL);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-airmass_min",  0,            "define min airmass", NAN);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-airmass_max",  0,            "define max airmass", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-ra_min",  0,            "define min RA (degrees)", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-ra_max",  0,            "define max RA (degrees)", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-decl_min",  0,            "define min DEC (degrees)", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-decl_max",  0,            "define max DEC (degrees)", NAN);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_time_min",  0,            "define min", NAN);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-exp_time_max",  0,            "define max", NAN);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-sat_pixel_frac_min",  0,            "define max fraction of saturated pixels", NAN);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-sat_pixel_frac_max",  0,            "define min fraction of saturated pixels", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-bg_min",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-bg_max",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-bg_stdev_min",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-bg_stdev_max",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev_min",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev_max",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-alt_min",  0,            "define min", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-alt_max",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-az_min",  0,            "define min", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-az_max",  0,            "define max", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-ccd_temp_min",0,            "define min ccd tempature", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-ccd_temp_max",0,            "define max ccd tempature", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-posang_min",  0,            "define min rotator position angle", NAN);
    psMetadataAddF64(revertprocessedimfileArgs, PS_LIST_TAIL, "-posang_max",  0,            "define max rotator position angle", NAN);
    psMetadataAddStr(revertprocessedimfileArgs, PS_LIST_TAIL, "-object",  0,            "search by exposure object", NULL);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-sun_angle_min",  0,            "define min solar angle", NAN);
    psMetadataAddF32(revertprocessedimfileArgs, PS_LIST_TAIL, "-sun_angle_max",  0,            "define max solar angle", NAN);

    psMetadataAddBool(revertprocessedimfileArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);
    psMetadataAddS16(revertprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);


    // -advanceexp
    psMetadata *advanceexpArgs = psMetadataAlloc();
    psMetadataAddS64(advanceexpArgs, PS_LIST_TAIL, "-fake_id", 0,      "search by fake ID", 0);
    psMetadataAddStr(advanceexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label ", NULL);
    psMetadataAddU64(advanceexpArgs, PS_LIST_TAIL, "-limit",  0,       "limit exposures to advance to N items", 0);

    // -block
    psMetadata *blockArgs = psMetadataAlloc();
    psMetadataAddStr(blockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to mask out (required)", NULL);

    // -masked
    psMetadata *maskedArgs = psMetadataAlloc();
    psMetadataAddStr(maskedArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(maskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(maskedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -unmasked
    psMetadata *unmaskedArgs = psMetadataAlloc();
    psMetadataAddStr(unmaskedArgs, PS_LIST_TAIL, "-label",  0,            "restrict to specified label", NULL);
    psMetadataAddBool(unmaskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(unmaskedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -unblock
    psMetadata *unblockArgs = psMetadataAlloc();
    psMetadataAddStr(unblockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to unmask (required)", NULL);

    // -pendingcleanuprun
    psMetadata *pendingcleanuprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanuprunArgs,  PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanuprunArgs,  PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -pendingcleanupimfile
    psMetadata *pendingcleanupimfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupimfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupimfileArgs, PS_LIST_TAIL, "-fake_id", 0,          "search by chip ID", 0);
    psMetadataAddBool(pendingcleanupimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    // -tocleanedimfile
    psMetadata *tocleanedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedimfileArgs, PS_LIST_TAIL, "-fake_id", 0,          "fake ID to update", 0);
    psMetadataAddStr(tocleanedimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -tofullimfile
    psMetadata *tofullimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tofullimfileArgs, PS_LIST_TAIL, "-fake_id", 0,          "fake ID to update", 0);
    psMetadataAddStr(tofullimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -topurgedimfile
    psMetadata *topurgedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(topurgedimfileArgs, PS_LIST_TAIL, "-fake_id", 0,          "fake ID to update", 0);
    psMetadataAddStr(topurgedimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -toscrubbedimfile
    psMetadata *toscrubbedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(toscrubbedimfileArgs, PS_LIST_TAIL, "-fake_id", 0,         "fake ID to update", 0);
    psMetadataAddStr(toscrubbedimfileArgs, PS_LIST_TAIL, "-class_id", 0,        "class ID to update", NULL);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-fake_id", 0,          "export this fake ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "mark tables as cleaned", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);




    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",         "create runs from raw exposures",       FAKETOOL_MODE_DEFINEBYQUERY,                    queueArgs);
    PXOPT_ADD_MODE("-updaterun",             "change fake run properties",           FAKETOOL_MODE_UPDATERUN,                updaterunArgs);
    PXOPT_ADD_MODE("-pendingexp",            "show pending exposures",               FAKETOOL_MODE_PENDINGEXP,               pendingexpArgs);
    PXOPT_ADD_MODE("-pendingimfile",         "show pending imfiles",                 FAKETOOL_MODE_PENDINGIMFILE,            pendingimfileArgs);
    PXOPT_ADD_MODE("-addprocessedimfile",    "add a processed imfile",               FAKETOOL_MODE_ADDPROCESSEDIMFILE,       addprocessedimfileArgs);
    PXOPT_ADD_MODE("-processedimfile",       "show processed imfiles",               FAKETOOL_MODE_PROCESSEDIMFILE,          processedimfileArgs);
    PXOPT_ADD_MODE("-updateprocessedimfile","change procesed imfile properties",     FAKETOOL_MODE_UPDATEPROCESSEDIMFILE,    updateprocessedimfileArgs);
    PXOPT_ADD_MODE("-revertprocessedimfile", "undo a processed imfile",              FAKETOOL_MODE_REVERTPROCESSEDIMFILE,    revertprocessedimfileArgs);
    PXOPT_ADD_MODE("-advanceexp",            "advance completed exposoures",         FAKETOOL_MODE_ADVANCEEXP,          advanceexpArgs);
    PXOPT_ADD_MODE("-block",                 "set a label block",                    FAKETOOL_MODE_BLOCK,          blockArgs);
    PXOPT_ADD_MODE("-masked",                "show blocked labels",                  FAKETOOL_MODE_MASKED,         maskedArgs);
    PXOPT_ADD_MODE("-unmasked",              "",                                     FAKETOOL_MODE_UNMASKED,       unmaskedArgs);
    PXOPT_ADD_MODE("-unblock",               "remove a label block",                 FAKETOOL_MODE_UNBLOCK,        unblockArgs);
    PXOPT_ADD_MODE("-pendingcleanuprun",     "show runs that need to be cleaned up", FAKETOOL_MODE_PENDINGCLEANUPRUN,    pendingcleanuprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupimfile",  "show runs that need to be cleaned up", FAKETOOL_MODE_PENDINGCLEANUPIMFILE, pendingcleanupimfileArgs);
    PXOPT_ADD_MODE("-donecleanup",           "show runs that have been cleaned",     FAKETOOL_MODE_DONECLEANUP,          donecleanupArgs);
    PXOPT_ADD_MODE("-tocleanedimfile",      "set imfile state to cleaned",           FAKETOOL_MODE_TOCLEANEDIMFILE,      tocleanedimfileArgs);
    PXOPT_ADD_MODE("-tofullimfile",        "set imfile state to full",               FAKETOOL_MODE_TOFULLIMFILE,         tofullimfileArgs);
    PXOPT_ADD_MODE("-topurgedimfile",      "set imfile state to purged",             FAKETOOL_MODE_TOPURGEDIMFILE,       topurgedimfileArgs);
    PXOPT_ADD_MODE("-toscrubbedimfile",    "set imfile state to scrubbed",           FAKETOOL_MODE_TOSCRUBBEDIMFILE,     toscrubbedimfileArgs);
    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", FAKETOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           FAKETOOL_MODE_IMPORTRUN, importrunArgs);


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
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}

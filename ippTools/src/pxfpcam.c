/*
 * pxfpcam.c
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

#include <stdlib.h>
#include <ippdb.h>
#include <string.h>

#include "pxtools.h"
#include "pxfpcam.h"

bool pxfpcamSetSearchArgs (psMetadata *md) {

    // standard query options for fpcamera
    psMetadataAddS64(md,  PS_LIST_TAIL, "-chip_id",            0, "search by chip_id", 0);
    psMetadataAddS64(md,  PS_LIST_TAIL, "-cam_id",             0, "search by cam_id", 0);
    psMetadataAddS64(md,  PS_LIST_TAIL, "-exp_id",             0, "search by exp_id", 0);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_name",           0, "search by exp_name", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-inst",               0, "search for camera", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-telescope",          0, "search for telescope", NULL);
    psMetadataAddTime(md, PS_LIST_TAIL, "-dateobs_begin",      0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(md, PS_LIST_TAIL, "-dateobs_end",        0, "search for exposures by time (<)", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_tag",            0, "search by exp_tag", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_type",           0, "search by exp_type", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-obs_mode",           0, "search by obs_mode", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-comment",            0, "search by comment", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-filelevel",          0, "search by filelevel", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-filter",             0, "search for filter", NULL);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-airmass_min",        0, "define min airmass", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-airmass_max",        0, "define max airmass", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_min",             0, "define min RA (degrees) ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_max",             0, "define max RA (degrees) ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-decl_min",           0, "define min DEC (degrees)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-decl_max",           0, "define max DEC (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-exp_time_min",       0, "define min exposure time", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-exp_time_max",       0, "define max exposure time", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "define max fraction of saturated pixels", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "define max fraction of saturated pixels", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_min",             0, "define min background", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_max",             0, "define max background", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_stdev_min",       0, "define min background standard deviation", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_stdev_max",       0, "define max background standard deviation", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_mean_stdev_min",  0, "define min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_mean_stdev_max",  0, "define max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-alt_min",            0, "define min altitude", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-alt_max",            0, "define max altitude", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-az_min",             0, "define min azimuth ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-az_max",             0, "define max azimuth ", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-ccd_temp_min",       0, "define min ccd tempature", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-ccd_temp_max",       0, "define max ccd tempature", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-posang_min",         0, "define min rotator position angle", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-posang_max",         0, "define max rotator position angle", NAN);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-object",             0, "search by exposure object", NULL);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sun_angle_min",      0, "define min solar angle", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sun_angle_max",      0, "define max solar angle", NAN);

    return true;
}

bool pxfpcamGetSearchArgs (pxConfig *config, psMetadata *where) {

    // only add the rawExp constraints to the where MD
    // chip and cam constraints are added independently to two different where MDs
    PXOPT_COPY_S64(config->args,   where, "-exp_id",             "rawExp.exp_id",         "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_name",           "rawExp.exp_name",       "==");
    PXOPT_COPY_STR(config->args,   where, "-inst",               "rawExp.camera",         "==");
    PXOPT_COPY_STR(config->args,   where, "-telescope",          "rawExp.telescope",      "==");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_begin",      "rawExp.dateobs",        ">=");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_end",        "rawExp.dateobs",        "<=");
    PXOPT_COPY_STR(config->args,   where, "-exp_tag",            "rawExp.exp_tag",        "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_type",           "rawExp.exp_type",       "==");
    PXOPT_COPY_STR(config->args,   where, "-obs_mode",           "rawExp.obs_mode",       "==");
    PXOPT_COPY_STR(config->args,   where, "-comment",            "rawExp.comment",        "LIKE");
    PXOPT_COPY_STR(config->args,   where, "-filelevel",          "rawExp.filelevel",      "==");
    PXOPT_COPY_STR(config->args,   where, "-filter",             "rawExp.filter",         "LIKE");
    PXOPT_COPY_F64(config->args,   where, "-airmass_min",        "rawExp.airmass",        ">=");
    PXOPT_COPY_F64(config->args,   where, "-airmass_max",        "rawExp.airmass",        "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min",             "rawExp.ra",             ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max",             "rawExp.ra",             "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min",           "rawExp.decl",           ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max",           "rawExp.decl",           "<");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_min",       "rawExp.exp_time",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_max",       "rawExp.exp_time",       "<");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_min",             "rawExp.bg",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_max",             "rawExp.bg",             "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_min",       "rawExp.bg_stdev",       ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_max",       "rawExp.bg_stdev",       "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_min",  "rawExp.bg_mean_stdev",  ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_max",  "rawExp.bg_mean_stdev",  "<");
    PXOPT_COPY_F64(config->args,   where, "-alt_min",            "rawExp.alt",            ">=");
    PXOPT_COPY_F64(config->args,   where, "-alt_max",            "rawExp.alt",            "<");
    PXOPT_COPY_F64(config->args,   where, "-az_min",             "rawExp.az",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-az_max",             "rawExp.az",             "<");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_min",       "rawExp.ccd_temp",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_max",       "rawExp.ccd_temp",       "<");
    PXOPT_COPY_F64(config->args,   where, "-posang_min",         "rawExp.posang",         ">=");
    PXOPT_COPY_F64(config->args,   where, "-posang_max",         "rawExp.posang",         "<");
    PXOPT_COPY_STR(config->args,   where, "-object",             "rawExp.object",         "==");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_min",      "rawExp.sun_angle",      ">=");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_max",      "rawExp.sun_angle",      "<");

    return true;
}


bool pxfpcamRunSetState(pxConfig *config, psS64 fpcam_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid fpcamRun state: %s", state);
        return false;
    }

    char *query = "UPDATE fpcamRun SET state = '%s' WHERE fpcam_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, fpcam_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for fpcam_id %" PRId64, fpcam_id);
        return false;
    }

    return true;
}

bool pxfpcamQueueByCamAndChipID(pxConfig *config,
                    psS64 chip_id,
                    psS64 cam_id,
                    char *workdir,
                    char *label,
                    char *data_group,
                    char *dist_group,
                    char *reduction,
                    char *dvodb,
                    char *note)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // load the SQL to enqueue our exp_ids from disk once
    static psString query = NULL;
    if (!query) {
        query = pxDataGet("fpcamtool_queue_cam_id.sql");
        psMemSetPersistent(query, true);
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }
    }

    // queue the exposure
    if (!p_psDBRunQueryF(config->dbh, query,
			 (long long) chip_id,
			 (long long) cam_id,
                         "new",     // state
                         workdir    ? workdir  : "NULL",
                         "dirty",   // workdir_state
                         label      ? label    : "NULL",
                         data_group ? data_group : "NULL",
                         dist_group ? dist_group : "NULL",
                         reduction  ? reduction   : "NULL",
                         dvodb      ? dvodb    : "NULL",
			 note       ? note     : "NULL"
    )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // just to be safe, we should have changed at least one row
    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false,
                "no rows affected - should have changed at least one row");
        return false;
    }

    return true;
}

/** fpcamInsertRow tools **/

static void fpcamInsertRowFree(fpcamInsertRow *object)
{
    psFree(object->state);
    psFree(object->workdir);
    psFree(object->workdir_state);
    psFree(object->label);
    psFree(object->data_group);
    psFree(object->dist_group);
    psFree(object->reduction);
    psFree(object->dvodb);
    psFree(object->software_ver);
    psFree(object->note);
}

static fpcamInsertRow *fpcamInsertRowAlloc(psS64 chip_id, psS64 cam_id, 
				    const char *state, const char *workdir, const char *workdir_state,
				    const char *label, const char *data_group, const char *dist_group,
				    const char *reduction, const char *dvodb, const char *software_ver, const char *note)
{
    fpcamInsertRow     *_object;

    _object = psAlloc(sizeof(fpcamInsertRow));
    psMemSetDeallocator(_object, (psFreeFunc)fpcamInsertRowFree);

    _object->chip_id = chip_id;
    _object->cam_id = cam_id;
    _object->state = psStringCopy(state);
    _object->workdir = psStringCopy(workdir);
    _object->workdir_state = psStringCopy(workdir_state);
    _object->label = psStringCopy(label);
    _object->data_group = psStringCopy(data_group);
    _object->dist_group = psStringCopy(dist_group);
    _object->reduction = psStringCopy(reduction);
    _object->dvodb = psStringCopy(dvodb);
    _object->software_ver = psStringCopy(software_ver);
    _object->note = psStringCopy(note);

    return _object;
}

fpcamInsertRow *fpcamInsertObjectFromMetadata(psMetadata *md)
{

bool status = false;
    psS64 chip_id = psMetadataLookupS64(&status, md, "chip_id");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item chip_id");
        return false;
    }
    psS64 cam_id = psMetadataLookupS64(&status, md, "cam_id");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item cam_id");
        return false;
    }
    char* state = psMetadataLookupPtr(&status, md, "state");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item state");
        return false;
    }
    char* workdir = psMetadataLookupPtr(&status, md, "workdir");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item workdir");
        return false;
    }
    char* workdir_state = psMetadataLookupPtr(&status, md, "workdir_state");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item workdir_state");
        return false;
    }
    char* label = psMetadataLookupPtr(&status, md, "label");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item label");
        return false;
    }
    char* data_group = psMetadataLookupPtr(&status, md, "data_group");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item data_group");
        return false;
    }
    char* dist_group = psMetadataLookupPtr(&status, md, "dist_group");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item dist_group");
        return false;
    }
    char* reduction = psMetadataLookupPtr(&status, md, "reduction");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item reduction");
        return false;
    }
    char* dvodb = psMetadataLookupPtr(&status, md, "dvodb");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item dvodb");
        return false;
    }
    char* software_ver = psMetadataLookupPtr(&status, md, "software_ver");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item software_ver");
        return false;
    }
    char* note = psMetadataLookupPtr(&status, md, "note");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item note");
        return false;
    }

    return fpcamInsertRowAlloc(chip_id, cam_id, state, workdir, workdir_state, label, data_group, dist_group, reduction, dvodb, software_ver, note);
}

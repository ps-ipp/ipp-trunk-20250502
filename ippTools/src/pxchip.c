/*
 * pxchip.c
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

#include <stdlib.h>
#include <ippdb.h>
#include <string.h>

#include "pxtools.h"
#include "pxchip.h"

bool pxchipSetSearchArgs (psMetadata *md) {

    // psMetadataAddStr(md,  PS_LIST_TAIL, "-class_id",           0, "search by class ID", NULL);
    // psMetadataAddStr(md,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);

    // XXX need to allow multiple exp_ids
    psMetadataAddS64(md,  PS_LIST_TAIL, "-exp_id",             0, "search by exp_id", 0);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_name",           0, "search by exp_name", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-inst",               0, "search for camera", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-telescope",          0, "search for telescope", NULL);
    psMetadataAddTime(md, PS_LIST_TAIL, "-dateobs_begin",      0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(md, PS_LIST_TAIL, "-dateobs_end",        0, "search for exposures by time (<=)", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_tag",            0, "search by exp_tag", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_type",           0, "search by exp_type", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-data_state",         0, "search by data_state", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-filelevel",          0, "search by filelevel", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-filter",             0, "search for filter", NULL);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-airmass_min",        0, "search by min airmass", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-airmass_max",        0, "search by max airmass", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_min",             0, "search by min RA (degrees) ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_max",             0, "search by max RA (degrees) ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-decl_min",           0, "search by min DEC (degrees)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-decl_max",           0, "search by max DEC (degrees)", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-exp_time_min",       0, "search by min exposure time", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-exp_time_max",       0, "search by max exposure time", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "search by min fraction of saturated pixels", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "search by max fraction of saturated pixels", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_min",             0, "search by min background", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_max",             0, "search by max background", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_stdev_min",       0, "search by min background standard deviation", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_stdev_max",       0, "search by max background standard deviation", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_mean_stdev_min",  0, "search by min background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_mean_stdev_max",  0, "search by max background mean standard deviation (across imfiles)", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-alt_min",            0, "search by min altitude", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-alt_max",            0, "search by max altitude", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-az_min",             0, "search by min azimuth ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-az_max",             0, "search by max azimuth ", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ccd_temp_min",       0, "search by min ccd tempature", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-ccd_temp_max",       0, "search by max ccd tempature", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-posang_min",         0, "search by min rotator position angle", NAN);
    psMetadataAddF64(md,  PS_LIST_TAIL, "-posang_max",         0, "search by max rotator position angle", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sun_angle_min",      0, "search by min solar angle", NAN);
    psMetadataAddF32(md,  PS_LIST_TAIL, "-sun_angle_max",      0, "search by max solar angle", NAN);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-object",             0, "search by exposure object (LIKE comparison)", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-comment",            0, "search by comment field (LIKE comparison)", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-comment-skip",       0, "exclude by comment field (NOT LIKE comparison)", NULL);
    psMetadataAddStr(md,  PS_LIST_TAIL, "-obs_mode",           0, "search by observation mode", NULL);
    return true;
}

// XXX explicit refs to rawExp and chipRun?
// XXX careful with entries that could match rawExp rawImfile, chipRun, or chipProcessedImfile (eg, bg)
bool pxchipGetSearchArgs (pxConfig *config, psMetadata *where) {

    // definebyquery : rawExp only
    // updaterun : rawExp, chipRun
    // pendingimfile : rawExp, chipRun
    // processedimfile : rawExp, chipRun, chipProcessedImfile
    // revertprocessedimfile : rawExp, chipProcessedImfile
    // updateprocessedimfile : chipProcessedImfile
    PXOPT_COPY_S64(config->args, where, "-exp_id", "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "rawExp.exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "rawExp.camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "rawExp.telescope", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_tag", "rawExp.exp_tag", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_type", "rawExp.exp_type", "==");
    PXOPT_COPY_STR(config->args, where, "-data_state", "rawExp.state", "==");
    PXOPT_COPY_STR(config->args, where, "-filelevel", "rawExp.filelevel", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "rawExp.filter", "LIKE");
    PXOPT_COPY_F64(config->args, where, "-airmass_min", "rawExp.airmass", ">=");
    PXOPT_COPY_F64(config->args, where, "-airmass_max", "rawExp.airmass", "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min", "rawExp.ra", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max", "rawExp.ra", "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min", "rawExp.decl", ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max", "rawExp.decl", "<");
    PXOPT_COPY_F32(config->args, where, "-exp_time_min", "rawExp.exp_time", ">=");
    PXOPT_COPY_F32(config->args, where, "-exp_time_max", "rawExp.exp_time", "<");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args, where, "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_min", "rawExp.bg", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_max", "rawExp.bg", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_min", "rawExp.bg_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_stdev_max", "rawExp.bg_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_min", "rawExp.bg_mean_stdev", ">=");
    PXOPT_COPY_F64(config->args, where, "-bg_mean_stdev_max", "rawExp.bg_mean_stdev", "<");
    PXOPT_COPY_F64(config->args, where, "-alt_min", "rawExp.alt", ">=");
    PXOPT_COPY_F64(config->args, where, "-alt_max", "rawExp.alt", "<");
    PXOPT_COPY_F64(config->args, where, "-az_min", "rawExp.az", ">=");
    PXOPT_COPY_F64(config->args, where, "-az_max", "rawExp.az", "<");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_min", "rawExp.ccd_temp", ">=");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp_max", "rawExp.ccd_temp", "<");
    PXOPT_COPY_F64(config->args, where, "-posang_min", "rawExp.posang", ">=");
    PXOPT_COPY_F64(config->args, where, "-posang_max", "rawExp.posang", "<");
    PXOPT_COPY_STR(config->args, where, "-object", "rawExp.object", "LIKE");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_min", "rawExp.sun_angle", ">=");
    PXOPT_COPY_F32(config->args, where, "-sun_angle_max", "rawExp.sun_angle", "<");
    PXOPT_COPY_STR(config->args, where, "-comment", "rawExp.comment", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-comment-skip", "rawExp.comment", "NOTLKE");
    PXOPT_COPY_STR(config->args, where, "-obs_mode", "rawExp.obs_mode", "LIKE");
    return true;
}

bool pxchipRunSetState(pxConfig *config, psS64 chip_id, const char *state, const psS64 magicked)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid chipRun state: %s", state);
        return false;
    }

    char *query = "UPDATE chipRun SET state = '%s', magicked = %" PRId64 " WHERE chip_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, magicked, chip_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for chip_id %" PRId64, chip_id);
        return false;
    }

    return true;
}


bool pxchipRunSetStateByQuery(pxConfig *config, psMetadata *where, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid chipRun state: %s", state);
        return false;
    }

    if (!strcmp(state, "full")) {
        // There are states that need to be met for a run to be set to full that we don't
        // check here.
        // for example all of the run's Imfiles must have chipProcessedImfile.data_state == "full"
        // chipRun.magicked = (SUM(!chipProcessedImfile.magicked) = 0)
        // so don't do allow setting the state to full
        psError(PS_ERR_UNKNOWN, true, "cannot use -updaterun so set chipRun state to full");
        return false;
    }


    psString query = psStringCopy("UPDATE chipRun JOIN rawExp USING(exp_id) SET state = '%s'");

    if (!strcmp(state, "cleaned") || !strcmp(state, "purged")) {
        // if magicked is non-zero set it to -1
        psStringAppend(&query, ", chipRun.magicked = IF(chipRun.magicked = 0, 0, -1)");
    }

    if (where && psListLength(where->list) > 0) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQueryF(config->dbh, query, state)) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(query);

    return true;
}

bool pxchipProcessedImfileSetStateByQuery(pxConfig *config, psMetadata *where, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid chipProcessedImfile state: %s", state);
        return false;
    }

/*     if (!strcmp(state, "full")) { */
/*         // There are states that need to be met for a run to be set to full that we don't */
/*         // check here. */
/*         // for example all of the run's Imfiles must have chipProcessedImfile.data_state == "full" */
/*         // chipRun.magicked = (SUM(!chipProcessedImfile.magicked) = 0) */
/*         // so don't do allow setting the state to full */
/*         psError(PS_ERR_UNKNOWN, true, "cannot use -updaterun so set chipRun state to full"); */
/*         return false; */
/*     } */

    psString query = psStringCopy("UPDATE chipProcessedImfile JOIN chipRun USING(chip_id) JOIN rawExp ON chipRun.exp_id = rawExp.exp_id SET data_state = '%s'");

    if (where && psListLength(where->list) > 0) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, state)) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(query);

    return true;
}

psS64 pxchipQueueByExpTag(pxConfig *config,
                         psS64 exp_id,
                         const char *workdir,
                         const char *label,
                         const char *data_group,
                         const char *dist_group,
                         const char *reduction,
                         const char *expgroup,
                         const char *dvodb,
                         const char *tess_id,
                         const char *end_stage,
                         const char *note)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }


    // create a chipRun
    if (!chipRunInsert(config->dbh,
            0x0, // chip_id
            exp_id,
            "new",      // state
            workdir,
            "dirty",    // workdir_state
            label,
            data_group,
            dist_group,
            reduction,
            expgroup,
            dvodb,
            tess_id,
            end_stage,
            0,          // magicked
		       NULL,
		       0,
		       NAN,
		       NAN,
		       NAN,
		       NAN,
            0,         // update_mode
            note
            )
    ) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return 0;
    }

    psS64 chip_id =  psDBLastInsertID(config->dbh);

    // Create rows in chipImfile table for each input exposure.
    // This creates the chip_image_id values
    psString query = "INSERT INTO chipImfile "
                     "SELECT %" PRId64 ", class_id, 0 FROM rawImfile WHERE exp_id = %" PRId64;

    if (!p_psDBRunQueryF(config->dbh, query, chip_id, exp_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return 0;
    }
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return 0;
    }

    return chip_id;
}


/*
 * flatcorr.c
 *
 * Copyright (C) 2006-2007  Joshua Hoblitt
 * Copyright (C) 2008  Eugene Magnier
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <ippdb.h>

#include "pxtools.h"
#include "pxchip.h"
#include "pxcam.h"
#include "pxadd.h"
#include "flatcorr.h"

static bool definebyqueryMode(pxConfig *config);
static bool definerunMode(pxConfig *config);
static bool addchipMode(pxConfig *config);
static bool dropchipMode(pxConfig *config);
static bool addcameraMode(pxConfig *config);
static bool dropcameraMode(pxConfig *config);
static bool advancecameraMode(pxConfig *config);
static bool advanceaddstarMode(pxConfig *config);
static bool pendingprocessMode(pxConfig *config);
static bool addprocessMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool inputexpMode(pxConfig *config);
static bool inputimfileMode(pxConfig *config);

static bool setflatcorrRunState(pxConfig *config, psS64 corr_id, const char *state);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = flatcorrConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(FLATCORR_MODE_DEFINEBYQUERY,  definebyqueryMode);
        MODECASE(FLATCORR_MODE_DEFINERUN,      definerunMode);
        MODECASE(FLATCORR_MODE_ADDCHIP,        addchipMode);
        MODECASE(FLATCORR_MODE_DROPCHIP,       dropchipMode);
        MODECASE(FLATCORR_MODE_ADDCAMERA,      addcameraMode);
        MODECASE(FLATCORR_MODE_DROPCAMERA,     dropcameraMode);
        MODECASE(FLATCORR_MODE_ADVANCECAMERA,  advancecameraMode);
        MODECASE(FLATCORR_MODE_ADVANCEADDSTAR, advanceaddstarMode);
        MODECASE(FLATCORR_MODE_PENDINGPROCESS, pendingprocessMode);
        MODECASE(FLATCORR_MODE_ADDPROCESS,     addprocessMode);
        MODECASE(FLATCORR_MODE_UPDATERUN,      updaterunMode);
        MODECASE(FLATCORR_MODE_INPUTEXP,       inputexpMode);
        MODECASE(FLATCORR_MODE_INPUTIMFILE,    inputimfileMode);
        default:
            psAbort("invalid option (this should not happen)");
    }

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(EXIT_SUCCESS);

FAIL:
    psErrorStackPrint(stderr, "\n");
    int exit_status = pxerrorGetExitStatus();

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(exit_status);
}


static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    pxchipGetSearchArgs (config, where);

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // require the camera to be defined: this analysis does not make sense
    // across multiple cameras
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(det_type, config->args, "-det_type", true, false);

    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", false, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(expgroup, config->args, "-set_expgroup", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-set_filter", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-set_tess_id", false, false);
    PXOPT_LOOKUP_STR(region, config->args, "-set_region", false, false);
    PXOPT_LOOKUP_BOOL(make_correction, config->args, "-make_correction", false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("chiptool_find_rawexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("flatcorr", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        for (long i = 0; i < psArrayLength(output); i++) {
            // negative simple so the default is true
            if (!ippdbPrintMetadataRaw(stdout, output->data[i], !simple)) {
                psError(PS_ERR_UNKNOWN, false, "failed to print array");
                psFree(output);
                return false;
            }
        }
        psFree(output);
        return true;
    }

    // start a transaction so we don't end up with an exp without any associted
    // imfiles
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // create a new flatcorrRun
    if (!flatcorrRunInsert(
            config->dbh,
            0,      // corr_id
            det_type,
            dvodb,
            camera,
            telescope,
            NULL,
            filter,
            "reg",  // state
            make_correction,
            workdir,
            label,
            reduction,
            region,
            NULL,
            0
        )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // figure out the ID of the flatcorrRun we just created
    psS64 corr_id = psDBLastInsertID(config->dbh);

    // loop over our list of exp_ids
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status;
        psS64 exp_id = psMetadataLookupS64(&status, md, "exp_id");
        if (!status) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for exp_id");
            psFree(output);
            return false;
        }

        // queue the exp : force this to stop at the chip stage
        psS64 chip_id = pxchipQueueByExpTag(config, exp_id, workdir, label, label, NULL, reduction, expgroup, dvodb, tess_id, "chip", note);
        if (!chip_id) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue exp_id: %" PRId64, exp_id);
            psFree(output);
            return false;
        }

        // add a flatcorrChipLink to the flatcorr Run we just created (initial state has include = TRUE)
        if (!flatcorrChipLinkInsert(config->dbh, corr_id, chip_id, 1)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

    }
    psFree(output);

    // set the flatcorrRun to a state of 'new'
    if (!setflatcorrRunState(config, corr_id, "new")) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to set run state");
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // require the camera to be defined: this analysis does not make sense across multiple
    // cameras
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(det_type, config->args, "-det_type", true, false);

    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", false, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(expgroup, config->args, "-set_expgroup", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-set_filter", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-set_tess_id", false, false);
    PXOPT_LOOKUP_STR(region, config->args, "-set_region", false, false);
    PXOPT_LOOKUP_BOOL(make_correction, config->args, "-make_correction", false);
    // XXX probably should make the region in -set_region match ra_min, ra_max, etc

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // start a transaction so we don't end up with an exp without any associted
    // imfiles
    // if (!psDBTransaction(config->dbh)) {
    //     psError(PS_ERR_UNKNOWN, false, "database error");
    //     return false;
    // }

    // create a new flatcorrRun
    flatcorrRunRow *row = flatcorrRunRowAlloc(
        0,      // corr_id
        det_type,
        dvodb,
        camera,
        telescope,
        NULL,
        filter,
        "reg",  // state
        make_correction,
        workdir,
        label,
        reduction,
        region,
        NULL, // hostname
        0 // fault
        );

    // create a new flatcorrRun
    if (!flatcorrRunInsertObject(config->dbh,row)) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // figure out the ID of the flatcorrRun we just created
    psS64 corr_id = psDBLastInsertID(config->dbh);
    row->corr_id = corr_id;

    // if (!psDBCommit(config->dbh)) {
    //     psError(PS_ERR_UNKNOWN, false, "database error");
    //     return false;
    // }

    flatcorrRunPrintObject (stdout, row, !simple);
    return true;
}

static bool addchipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", true, false);

    // add a flatcorrChipLink (initial state has include = TRUE)
    if (!flatcorrChipLinkInsert(config->dbh, corr_id, chip_id, 1)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool dropchipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", true, false);

    // UPDATE flatcorrChipLink set include = 0 where corr_id = %lld AND chip_id = %lld
    psString query = pxDataGet("flatcorr_dropchip.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, (long long) corr_id, (long long) chip_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    return true;
}

static bool addcameraMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", true, false);
    PXOPT_LOOKUP_S64(cam_id, config->args, "-cam_id", true, false);

    // add a flatcorrCamLink (initial state has include = TRUE)
    // XXX should add checks that the chip_id and corr_id are in ChipLink
    if (!flatcorrCamLinkInsert(config->dbh, corr_id, chip_id, cam_id, 1)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool dropcameraMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_S64(cam_id, config->args, "-cam_id", true, false);

    // UPDATE flatcorrCamLink set include = 0 where corr_id = %lld AND cam_id = %lld
    psString query = pxDataGet("flatcorr_dropcamera.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, (long long) corr_id, (long long) cam_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    return true;
}

// select the flatcorr chip runs that have completed and for which there is no camera entry
// queue a new camera run for them
static bool advancecameraMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple,  config->args, "-simple",  false);
    PXOPT_LOOKUP_BOOL(limit,   config->args, "-limit",   false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "flatcorrRun.label", "==");

    psString query = pxDataGet("flatcorr_chiprundone.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("flatcorr", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "flatcorr_addcamera", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    // start a transaction so we don't end up with an exp without any associted
    // imfiles
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // loop over our list of chipRun rows
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status;
        psS64 corr_id = psMetadataLookupS64(&status, md, "corr_id");
        if (!status) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for corr_id");
            psFree(output);
            return false;
        }

        chipRunRow *row = chipRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into chipRun");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxcamQueueByChipID(
                config,
                row->chip_id,
                row->workdir,
                row->label,
                row->data_group,
                row->dist_group,
                row->reduction,
                row->expgroup,
                row->dvodb,
                row->tess_id,
                "camera",
                row->magicked,
                NULL // note does not propragate
                )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue chip_id: %" PRId64, row->chip_id);
            psFree(row);
            psFree(output);
            return false;
        }

        // figure out the ID of the flatcorrRun we just created
        psS64 cam_id = psDBLastInsertID(config->dbh);

        // add the camRun entry to the flatcorrCamLink table (include is TRUE)
        if (!flatcorrCamLinkInsert(config->dbh, corr_id, row->chip_id, cam_id, 1)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        psFree(row);
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return false;
}

// Select the flatcorr camera runs that have completed and for which there is no addstar
// entry.  Queue a new addstar run for them
static bool advanceaddstarMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple,  config->args, "-simple",  false);
    PXOPT_LOOKUP_BOOL(limit,   config->args, "-limit",   false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "flatcorrRun.label", "==");

    psString query = pxDataGet("flatcorr_camerarundone.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("flatcorr", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "flatcorr_addcamera", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    // start a transaction so we don't end up with an exp without any associted
    // imfiles
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // loop over our list of chipRun rows
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status;
        psS64 corr_id = psMetadataLookupS64(&status, md, "corr_id");
        if (!status) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for corr_id");
            psFree(output);
            return false;
        }

        camRunRow *row = camRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into chipRun");
            psFree(output);
            return false;
        }

        // queue the exp : force image_only to be false (flatcorr is meaningless with just image info)
        if (!pxaddQueueByCamID(
                config,
		"cam",
                row->cam_id,
		0,
                row->workdir,
                row->reduction,
                row->label,
                row->data_group,
                row->dvodb,
                NULL,       // note is not propagated
                0,
		0,  //The minidvodb stuff is off
		NULL,
		NULL,
		NULL)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue cam_id: %" PRId64, row->cam_id);
            psFree(row);
            psFree(output);
            return false;
        }

        // figure out the ID of the flatcorrRun we just created
        psS64 add_id = psDBLastInsertID(config->dbh);

        // add the addRun entry to the flatcorrAddstarLink table (include is TRUE)
        if (!flatcorrAddstarLinkInsert(config->dbh, corr_id, row->cam_id, add_id, 1)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        psFree(row);
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return false;
}

// select the flatcorr chip runs that have completed and for which there is no camera entry
// queue a new camera run for them
static bool pendingprocessMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple,  config->args, "-simple",  false);
    PXOPT_LOOKUP_BOOL(limit,   config->args, "-limit",   false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    psString query = pxDataGet("flatcorr_pendingprocess.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("flatcorr", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "flatcorrPending", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    return true;
}

// XXX need a fault state
static bool addprocessMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_S16(code, config->args, "-code", false, false);

    char *query = "UPDATE flatcorrRun SET state = 'full', hostname = '%s', fault = '%hd' WHERE corr_id = %" PRId64;

    if (!p_psDBRunQueryF(config->dbh, query, hostname, code, corr_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for corr_id %" PRId64, corr_id);
        return false;
    }

    return true;
}

static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_STR(state, config->args, "-state", true, false);

    if (!setflatcorrRunState(config, corr_id, state)) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to set run state");
        return false;
    }

    return true;
}

// select the flatcorr chip runs that have completed and for which there is no camera entry
// queue a new camera run for them
static bool inputexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(corr_id, config->args, "-corr_id", true, false);
    PXOPT_LOOKUP_BOOL(simple,  config->args, "-simple",  false);
    PXOPT_LOOKUP_BOOL(limit,   config->args, "-limit",   false);

    char *query = psStringCopy ("SELECT * FROM flatcorrChipLink WHERE corr_id = %" PRId64);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, corr_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("flatcorr", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "flatcorrPending", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    return true;
}

// select the flatcorr chip runs that have completed and for which there is no camera entry
// queue a new camera run for them
static bool inputimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", true, false);
    PXOPT_LOOKUP_BOOL(simple,  config->args, "-simple",  false);
    PXOPT_LOOKUP_BOOL(limit,   config->args, "-limit",   false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chipProcessedImfile.chip_id", "==");

    psString query = pxDataGet("flatcorr_inputimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, chip_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("flatcorr", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "flatcorrPending", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    return true;
}

static bool setflatcorrRunState(pxConfig *config, psS64 corr_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!strcmp(state, "reg") &&
        !strcmp(state, "new") &&
        !strcmp(state, "full"))
    {
        psError(PS_ERR_UNKNOWN, false, "invalid state: %s", state);
        return false;
    }

    char *query = "UPDATE flatcorrRun SET state = '%s' WHERE corr_id = %" PRId64;

    if (!p_psDBRunQueryF(config->dbh, query, state, corr_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for corr_id %" PRId64, corr_id);
        return false;
    }

    return true;
}

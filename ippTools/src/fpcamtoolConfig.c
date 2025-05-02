/*
 * fpcamtoolConfig.c
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
#include "pxfpcam.h"
#include "fpcamtool.h"

pxConfig *fpcamtoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    pxfpcamSetSearchArgs(definebyqueryArgs);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-chip_label", PS_META_DUPLICATE_OK, "search by chipRun label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-chip_reduction",          0, "search by chipRun reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-chip_data_group",         0, "search by chipRun data_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-cam_label", PS_META_DUPLICATE_OK, "search by camRun label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-cam_reduction",          0, "search by camRun reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-cam_data_group",         0, "search by camRun data_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",        0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",          0, "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group",     0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dist_group",     0, "define dist group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",      0, "define reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb",          0, "define DVO db", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note",           0, "define note", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",           0, "do not actual modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    pxfpcamSetSearchArgs(updaterunArgs);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-fpcam_id",           0, "search by fpcam_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",              0, "search by fpcamRun state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",              0, "search by fpcamRun label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group",         0, "search by fpcamRun data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dist_group",         0, "search by fpcamRun dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-reduction",          0, "search by fpcamRun reduction class", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state",          0, "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label",          0, "set label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group",     0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group",     0,   "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note",           0,         "define new note", NULL);

    // -pendingexp
    psMetadata *pendingexpArgs = psMetadataAlloc();
    pxfpcamSetSearchArgs(pendingexpArgs);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-fpcam_id",            0, "search by fpcam_id", 0);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by fpcamRun label", NULL);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-reduction",         0, "search by fpcamRun reduction class", NULL);
    psMetadataAddU64(pendingexpArgs, PS_LIST_TAIL, "-limit",             0, "limit result set to N items", 0);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-simple",           0, "use the simple output format", false);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-all",               0, "allow everything to be queued without search terms", false);

    // -inputchips
    psMetadata *inputchipsArgs = psMetadataAlloc();
    psMetadataAddS64(inputchipsArgs, PS_LIST_TAIL, "-fpcam_id",          0, "search by fpcam_id (required)", 0);
    psMetadataAddStr(inputchipsArgs, PS_LIST_TAIL, "-class_id",          0, "limit by class_id", NULL);
    psMetadataAddU64(inputchipsArgs, PS_LIST_TAIL, "-limit",             0, "limit result set to N items", 0);
    psMetadataAddBool(inputchipsArgs, PS_LIST_TAIL, "-simple",           0, "use the simple output format", false);

    // -inputastrom
    psMetadata *inputastromArgs = psMetadataAlloc();
    psMetadataAddS64(inputastromArgs, PS_LIST_TAIL, "-fpcam_id",            0, "search by fpcam_id", 0);
    psMetadataAddU64(inputastromArgs, PS_LIST_TAIL, "-limit",             0, "limit result set to N items", 0);
    psMetadataAddBool(inputastromArgs, PS_LIST_TAIL, "-simple",           0, "use the simple output format", false);

    // -addprocessedexp
    psMetadata *addprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-fpcam_id", 0,        "define fpcamtool ID (required)", 0);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", NULL);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_obs", 0,   "define observed zero point", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_err", 0,   "define observed zero point error", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_uq", 0,   "define observed zero point upper quartile", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_lq", 0,   "define observed zero point lower quartile", 0);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-hostname", 0,            "define hostname", NULL);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_stars", 0,            "define number of stars", 0);

    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst", 0, "define deteff", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst_err", 0, "define deteff_err", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst_lq", 0, "define deteff_lq", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst_uq", 0, "define deteff_uq", NAN);

    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_fpcamera", 0, "define ppImage version", NULL);

    // -processedexp
    psMetadata *processedexpArgs = psMetadataAlloc();
    pxfpcamSetSearchArgs(processedexpArgs);
    psMetadataAddS64(processedexpArgs, PS_LIST_TAIL, "-fpcam_id",   0,            "search by fpcam_id", 0);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by fpcamRun label", NULL);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by fpcamRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-reduction",0,            "search by fpcamRun reduction class", NULL);
    pxspaceAddArguments(processedexpArgs);

    psMetadataAddU64(processedexpArgs, PS_LIST_TAIL, "-limit",    0,            "limit result set to N items", 0);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-faulted", 0,            "only return imfiles with a fault status set", false);

    // -revertprocessedexp
    psMetadata *revertprocessedexpArgs = psMetadataAlloc();
    pxfpcamSetSearchArgs(revertprocessedexpArgs);
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-fpcam_id",   0,            "search by cam_id", 0);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "search by fpcamRun label", NULL);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-reduction",0,            "search by fpcamRun reduction class", NULL);
    psMetadataAddS16(revertprocessedexpArgs, PS_LIST_TAIL, "-code",     0,            "search by fault code", 0);

    psMetadataAddBool(revertprocessedexpArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);
    psMetadataAddS16(revertprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -updateprocessedexp
    psMetadata *updateprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-fpcam_id", 0,            "search by fpcamRun ID", 0);
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-cam_id", 0,            "search by camRun ID", 0);
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-chip_id",  0,            "search by chipRun ID", 0);
    psMetadataAddS16(updateprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code (required)", INT16_MAX);
    psMetadataAddS16(updateprocessedexpArgs, PS_LIST_TAIL, "-set_quality",  0,            "set quality", 0);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from chip stage",          FPCAMTOOL_MODE_DEFINEBYQUERY,      definebyqueryArgs);
    PXOPT_ADD_MODE("-updaterun",            "change cam run properties",            FPCAMTOOL_MODE_UPDATERUN,          updaterunArgs);
    PXOPT_ADD_MODE("-pendingexp",           "show pending exposures",               FPCAMTOOL_MODE_PENDINGEXP,         pendingexpArgs);
    PXOPT_ADD_MODE("-inputchips",           "list input chips",                     FPCAMTOOL_MODE_INPUTCHIPS,         inputchipsArgs);
    PXOPT_ADD_MODE("-inputastrom",          "list input astrometry file",           FPCAMTOOL_MODE_INPUTASTROM,        inputastromArgs);
    PXOPT_ADD_MODE("-addprocessedexp",      "add a processed exposure",             FPCAMTOOL_MODE_ADDPROCESSEDEXP,    addprocessedexpArgs);
    PXOPT_ADD_MODE("-processedexp",         "show processed exposures",             FPCAMTOOL_MODE_PROCESSEDEXP,       processedexpArgs);
    PXOPT_ADD_MODE("-revertprocessedexp",   "undo a processed exposure",            FPCAMTOOL_MODE_REVERTPROCESSEDEXP, revertprocessedexpArgs);
    PXOPT_ADD_MODE("-updateprocessedexp",   "changed processed exp properties",     FPCAMTOOL_MODE_UPDATEPROCESSEDEXP, updateprocessedexpArgs);

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
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}

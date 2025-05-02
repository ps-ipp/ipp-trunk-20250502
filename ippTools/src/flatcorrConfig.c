/*
 * flatcorrConfig.c
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

#include <psmodules.h>

#include "pxtools.h"
#include "flatcorr.h"

pxConfig *flatcorrConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(PS_ERR_UNKNOWN, false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    psTime *now = psTimeGetNow(PS_TIME_TAI);

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    // XXX need to allow multiple exp_ids
    pxchipSetSearchArgs (definebyqueryArgs);

    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-det_type",     0,            "define detrend type to be generated", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",  0,            "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",    0,            "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",0,            "define reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_expgroup", 0,            "define exposure group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb",    0,            "define DVO db", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_filter",   0,            "define filter", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_tess_id",  0,            "define tessalation", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_region",   0,            "define region", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-make_correction",  0,       "generate a correction image and add to detrend database", false);

    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0,            "print the exposures that would be included in the detrend run and exit", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-inst", 0, "define camera", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-telescope", 0, "define telescope", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-det_type",     0,            "define detrend type to be generated", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_workdir",  0,            "define workdir", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_label",  0,            "define label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_reduction",  0,            "define reduction class", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_expgroup",  0,            "define exposure group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_dvodb",  0,            "define DVO db", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_filter",  0,            "define filter", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_tess_id",  0,            "define tessalation", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_region",  0,            "define region", NULL);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-make_correction",  0,       "generate a correction image and add to detrend database", false);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -addchip
    psMetadata *addchipArgs = psMetadataAlloc();
    psMetadataAddS64(addchipArgs, PS_LIST_TAIL, "-corr_id", 0,            "define Flat Correction ID (required)", 0);
    psMetadataAddS64(addchipArgs, PS_LIST_TAIL, "-chip_id", 0,            "define Chip ID (required)", 0);

    // -dropchip
    psMetadata *dropchipArgs = psMetadataAlloc();
    psMetadataAddS64(dropchipArgs, PS_LIST_TAIL, "-corr_id", 0,            "define Flat Correction ID (required)", 0);
    psMetadataAddS64(dropchipArgs, PS_LIST_TAIL, "-chip_id", 0,            "define Chip ID (required)", 0);

    // -addcamera
    psMetadata *addcameraArgs = psMetadataAlloc();
    psMetadataAddS64(addcameraArgs, PS_LIST_TAIL, "-corr_id", 0,            "define Flat Correction ID (required)", 0);
    psMetadataAddS64(addcameraArgs, PS_LIST_TAIL, "-chip_id", 0,            "define Chip ID (required)", 0);
    psMetadataAddS64(addcameraArgs, PS_LIST_TAIL, "-cam_id", 0,             "define Camera ID (required)", 0);

    // -dropcamera
    psMetadata *dropcameraArgs = psMetadataAlloc();
    psMetadataAddS64(dropcameraArgs, PS_LIST_TAIL, "-corr_id", 0,      "define Flat Correction ID (required)", 0);
    psMetadataAddS64(dropcameraArgs, PS_LIST_TAIL, "-cam_id", 0,       "define Camera ID (required)", 0);

    // -advancecamera
    psMetadata *advancecameraArgs = psMetadataAlloc();
    psMetadataAddU64 (advancecameraArgs, PS_LIST_TAIL, "-limit",   0, "limit result set to N items", 0);
    psMetadataAddBool(advancecameraArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddBool(advancecameraArgs, PS_LIST_TAIL, "-pretend", 0, "use the simple output format", false);
    psMetadataAddStr(advancecameraArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by label", NULL);

    // -advanceaddstar
    psMetadata *advanceaddstarArgs = psMetadataAlloc();
    psMetadataAddU64 (advanceaddstarArgs, PS_LIST_TAIL, "-limit",   0, "limit result set to N items", 0);
    psMetadataAddBool(advanceaddstarArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);
    psMetadataAddBool(advanceaddstarArgs, PS_LIST_TAIL, "-pretend", 0, "use the simple output format", false);
    psMetadataAddStr(advanceaddstarArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by label", NULL);

    // -pendingprocess
    psMetadata *pendingprocessArgs = psMetadataAlloc();
    psMetadataAddU64 (pendingprocessArgs, PS_LIST_TAIL, "-limit",  0, "limit result set to N items", 0);
    psMetadataAddBool(pendingprocessArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddStr(pendingprocessArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by label", NULL);

    // -addprocess (XXX need to add fault and stats)
    psMetadata *addprocessArgs = psMetadataAlloc();
    psMetadataAddS64 (addprocessArgs, PS_LIST_TAIL, "-corr_id",  0, "add complete run for specified corr_id (required)", 0);
    psMetadataAddStr (addprocessArgs, PS_LIST_TAIL, "-hostname", 0, "set hostname", NULL);
    psMetadataAddS16 (addprocessArgs, PS_LIST_TAIL, "-code",     0, "set fault code", 0);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-corr_id", 0, "define correction id (required)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",   0, "set state (required)", NULL);
    // XXX add mechanism to change this value: psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-set_correction",  0, "generate a correction image and add to detrend database", false);

    // -inputexp
    psMetadata *inputexpArgs = psMetadataAlloc();
    psMetadataAddS64(inputexpArgs, PS_LIST_TAIL, "-corr_id", 0, "define correction id (required)", 0);
    psMetadataAddBool(inputexpArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(inputexpArgs, PS_LIST_TAIL, "-limit",   0, "limit result set to N items", 0);

    // -inputimfile
    psMetadata *inputimfileArgs = psMetadataAlloc();
    psMetadataAddS64(inputimfileArgs, PS_LIST_TAIL, "-chip_id", 0, "define chip id (required)", 0);
    psMetadataAddBool(inputimfileArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(inputimfileArgs, PS_LIST_TAIL, "-limit",   0, "limit result set to N items", 0);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-corr_id", 0,          "export this correction ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);


    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",  "create a new, populated flat correction run",         FLATCORR_MODE_DEFINEBYQUERY,  definebyqueryArgs);
    PXOPT_ADD_MODE("-definerun",      "create a new, empty flat correction run",             FLATCORR_MODE_DEFINERUN,      definerunArgs);
    PXOPT_ADD_MODE("-addchip",        "add an existing chip run to a flat correction run",   FLATCORR_MODE_ADDCHIP,        addchipArgs);
    PXOPT_ADD_MODE("-addcamera",      "add an existing camera run to a flat correction run", FLATCORR_MODE_ADDCAMERA,      addcameraArgs);
    PXOPT_ADD_MODE("-advancecamera",  "migrate completed chips to camera stage analysis",    FLATCORR_MODE_ADVANCECAMERA,  advancecameraArgs);
    PXOPT_ADD_MODE("-advanceaddstar", "migrate completed exposures to addstar processing",   FLATCORR_MODE_ADVANCEADDSTAR,  advanceaddstarArgs);
    PXOPT_ADD_MODE("-dropchip",       "drop a chip from a flat correction run",              FLATCORR_MODE_DROPCHIP,       dropchipArgs);
    PXOPT_ADD_MODE("-dropcamera",     "drop an exposure (camera stage analysis)",            FLATCORR_MODE_DROPCAMERA,     dropcameraArgs);
    PXOPT_ADD_MODE("-pendingprocess", "show flat correction runs needing to be processed",   FLATCORR_MODE_PENDINGPROCESS, pendingprocessArgs);
    PXOPT_ADD_MODE("-addprocess",     "report completed flat correction analysis",           FLATCORR_MODE_ADDPROCESS,     addprocessArgs);
    PXOPT_ADD_MODE("-updaterun",      "change a flat calibration run's state",               FLATCORR_MODE_UPDATERUN,      updaterunArgs);
    PXOPT_ADD_MODE("-inputexp",       "list exposures for a correction run",                 FLATCORR_MODE_INPUTEXP,       inputexpArgs);
    PXOPT_ADD_MODE("-inputimfile",    "list imfiles for a chip run",                         FLATCORR_MODE_INPUTIMFILE,    inputimfileArgs);

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

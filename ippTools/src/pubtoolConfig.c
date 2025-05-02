/*
 * pubtoolConfig.c
 *
 * Copyright (C) 2008
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
#include "pubtool.h"

pxConfig *pubtoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration!");
        psFree(config);
        return NULL;
    }

    // -defineclient
    psMetadata *defineclientArgs = psMetadataAlloc();
    psMetadataAddStr(defineclientArgs, PS_LIST_TAIL, "-stage", 0, "define stage (required)", NULL);
    psMetadataAddStr(defineclientArgs, PS_LIST_TAIL, "-product", 0, "define product (required)", NULL);
    psMetadataAddStr(defineclientArgs, PS_LIST_TAIL, "-workdir", 0, "define workdir (required)", NULL);
    psMetadataAddS16(defineclientArgs, PS_LIST_TAIL, "-output_format", 0, "define output format", 2);
    psMetadataAddStr(defineclientArgs, PS_LIST_TAIL, "-comment", 0, "define comment", NULL);
    psMetadataAddStr(defineclientArgs, PS_LIST_TAIL, "-name", 0, "define name", NULL);
    psMetadataAddBool(defineclientArgs, PS_LIST_TAIL, "-unmagicked", 0, "allow unmagicked data?", false);

    // -updateclient
    psMetadata *updateclientArgs = psMetadataAlloc();
    psMetadataAddS64(updateclientArgs, PS_LIST_TAIL, "-client_id", 0, "search by client_id", 0);
    psMetadataAddStr(updateclientArgs, PS_LIST_TAIL, "-stage", 0, "search by stage", NULL);
    psMetadataAddStr(updateclientArgs, PS_LIST_TAIL, "-product", 0, "search by product", NULL);
    psMetadataAddStr(updateclientArgs, PS_LIST_TAIL, "-comment", 0, "search by comment (LIKE)", NULL);
    psMetadataAddBool(updateclientArgs, PS_LIST_TAIL, "-active", 0, "set to active state", NULL);
    psMetadataAddBool(updateclientArgs, PS_LIST_TAIL, "-inactive", 0, "set to inactive state", NULL);

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_label", 0, "set label", NULL);
    psMetadataAddS64(definerunArgs, PS_LIST_TAIL, "-client_id", 0, "search by client_id", 0);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "set and search by label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by data_group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-comment", 0, "search by comment", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-dateobs_begin", 0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-dateobs_end", 0, "search for exposures by time (<=)", NULL);
    psMetadataAddStr(definerunArgs,  PS_LIST_TAIL, "-filter", 0, "search for filter", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-obs_mode", 0, "search by observation mode", NULL);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-rerun", 0, "Re-run publish?", false);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-pretend", 0, "Pretend to define?", false);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple",  0, "use simple output format?", false);
    psMetadataAddU64(definerunArgs, PS_LIST_TAIL, "-limit",  0, "limit result set", 0);

    // -pending
    psMetadata *pendingArgs = psMetadataAlloc();
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-client_id", 0, "search on client_id", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-stage", 0, "search on source", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-comment", 0, "search on comment (LIKE)", NULL);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search on label", NULL);
    psMetadataAddBool(pendingArgs, PS_LIST_TAIL, "-simple",  0, "use simple output format?", false);
    psMetadataAddU64(pendingArgs, PS_LIST_TAIL, "-limit",  0, "limit result set", 0);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-name", 0, "search on client name", NULL);
    psMetadataAddBool(pendingArgs, PS_LIST_TAIL, "-all",  0,             "search without arguments", false);

    // -add
    psMetadata *addArgs = psMetadataAlloc();
    psMetadataAddS64(addArgs, PS_LIST_TAIL, "-pub_id", 0, "define pub_id (required)", 0);
    psMetadataAddStr(addArgs, PS_LIST_TAIL, "-path_base", 0, "define path_base (required)", NULL);
    psMetadataAddStr(addArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", NULL);
    psMetadataAddF32(addArgs, PS_LIST_TAIL, "-dtime_script", 0, "define time for script", NAN);
    psMetadataAddS32(addArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);

    // -revert
    psMetadata *revertArgs = psMetadataAlloc();
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-pub_id", 0, "search on pub_id", 0);
    psMetadataAddS32(revertArgs, PS_LIST_TAIL, "-fault", 0, "search on fault code", 0);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-client_id", 0, "search on client_id", NULL);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-comment", 0, "search on comment (LIKE)", NULL);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search on label", NULL);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "new value for state (required)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-pub_id", 0, "search on pub_id", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-client_id", 0, "search on client_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0, "search by label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-fault", 0, "search by fault", NULL);


    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-defineclient", "", PUBTOOL_MODE_DEFINECLIENT, defineclientArgs);
    PXOPT_ADD_MODE("-updateclient", "", PUBTOOL_MODE_UPDATECLIENT, updateclientArgs);
    PXOPT_ADD_MODE("-definerun", "", PUBTOOL_MODE_DEFINERUN, definerunArgs);
    PXOPT_ADD_MODE("-pending", "", PUBTOOL_MODE_PENDING, pendingArgs);
    PXOPT_ADD_MODE("-add", "", PUBTOOL_MODE_ADD, addArgs);
    PXOPT_ADD_MODE("-revert", "", PUBTOOL_MODE_REVERT, revertArgs);
    PXOPT_ADD_MODE("-updaterun", "", PUBTOOL_MODE_UPDATERUN, updaterunArgs);

    if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
        psError(PS_ERR_UNKNOWN, true, "option parsing failed");
        psFree(argSets);
        psFree(modes);
        psFree(config);
        return NULL;
    }

    psFree(argSets);
    psFree(modes);

    // define database handle, if used
    // do this last so we don't setup a connection before CLI options are validated
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}

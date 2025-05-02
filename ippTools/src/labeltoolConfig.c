/*
 * labeltoolConfig.c
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

#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "labeltool.h"

pxConfig *labeltoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definelabel
    psMetadata *definelabelArgs = psMetadataAlloc();
    psMetadataAddStr(definelabelArgs, PS_LIST_TAIL, "-set_label",   0, "define label (required)", NULL);
    psMetadataAddS64(definelabelArgs, PS_LIST_TAIL, "-set_priority", 0, "define priority (required)", 0);
    psMetadataAddBool(definelabelArgs, PS_LIST_TAIL, "-set_inactive", 0, "set label inactive", false);
    psMetadataAddStr(definelabelArgs, PS_LIST_TAIL, "-set_comment",  0, "define label comment", NULL);
    psMetadataAddBool(definelabelArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);

    // -updatelabel
    psMetadata *updatelabelArgs = psMetadataAlloc();
    psMetadataAddStr(updatelabelArgs, PS_LIST_TAIL, "-label", 0, "search by label (LIKE comparison) (required)", NULL);
    psMetadataAddS64(updatelabelArgs, PS_LIST_TAIL, "-set_priority", 0, "define new priority", 0);
    psMetadataAddBool(updatelabelArgs, PS_LIST_TAIL, "-set_active", 0, "set label active", false);
    psMetadataAddBool(updatelabelArgs, PS_LIST_TAIL, "-set_inactive", 0, "set label inactive", false);
    psMetadataAddStr(updatelabelArgs, PS_LIST_TAIL, "-set_comment",  0, "define new label comment", NULL);

    // -deletelabel
    psMetadata *deletelabelArgs = psMetadataAlloc();
    psMetadataAddStr(deletelabelArgs, PS_LIST_TAIL, "-label", 0, "label to delete (required)", NULL);

    // -listlabel
    psMetadata *listlabelArgs = psMetadataAlloc();
    psMetadataAddStr(listlabelArgs, PS_LIST_TAIL, "-label", 0, "search by label (LIKE comparison)", NULL);
    psMetadataAddBool(listlabelArgs, PS_LIST_TAIL, "-hightolow", 0, "order by priority high to low", false);
    psMetadataAddBool(listlabelArgs, PS_LIST_TAIL, "-lowtohigh", 0, "order by priority low to high", false);
    psMetadataAddBool(listlabelArgs, PS_LIST_TAIL, "-active", 0, "list active labels", false);
    psMetadataAddBool(listlabelArgs, PS_LIST_TAIL, "-inactive", 0, "list inactive labels", false);
    psMetadataAddU64(listlabelArgs, PS_LIST_TAIL, "-limit",  0,  "limit result set to N items", 0);
    psMetadataAddBool(listlabelArgs, PS_LIST_TAIL, "-simple",  0, "use the simple output format", false);



    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definelabel",   "", LABELTOOL_MODE_DEFINELABEL, definelabelArgs);
    PXOPT_ADD_MODE("-updatelabel",   "", LABELTOOL_MODE_UPDATELABEL, updatelabelArgs);
    PXOPT_ADD_MODE("-deletelabel",   "", LABELTOOL_MODE_DELETELABEL, deletelabelArgs);
    PXOPT_ADD_MODE("-listlabel",     "", LABELTOOL_MODE_LISTLABEL,   listlabelArgs);

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

/*
 * sctoolConfig.c
 *
 * Copyright (C) 2012 IfA University of Hawaii
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
#include "pxspace.h"
#include "sctool.h"

pxConfig *sctoolConfig(pxConfig *config, int argc, char **argv)
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

    // -defineskycells
    psMetadata *defineskycellsArgs = psMetadataAlloc();
    psMetadataAddStr(defineskycellsArgs, PS_LIST_TAIL, "-input", 0, "file containing skycell data (required)", NULL);

    // -list
    psMetadata *listArgs = psMetadataAlloc();
    psMetadataAddStr(listArgs,  PS_LIST_TAIL, "-skycell_id",        0, "search by skycell_id (LIKE comparision)", NULL);
    psMetadataAddStr(listArgs,  PS_LIST_TAIL, "-tess_id",           0, "search by tess_id (LIKE comparision)", NULL);
    // add skycell coordinate search arguments
    pxskycellAddArguments(listArgs);
    psMetadataAddU64(listArgs,  PS_LIST_TAIL, "-limit",             0, "limit result set to N items", 0);
    psMetadataAddBool(listArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-defineskycells",        "insert skycells from mdc file",  SCTOOL_MODE_DEFINESKYCELLS,        defineskycellsArgs);
    PXOPT_ADD_MODE("-list",                   "list skycell parameters",       SCTOOL_MODE_LIST,        listArgs);

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

/*
 * caltoolConfig.c
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
#include "caltool.h"

pxConfig *caltoolConfig(pxConfig *config, int argc, char **argv)
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

    psTime *time = psTimeGetNow(PS_TIME_TAI);
    psString now = psTimeToISO(time);
    psFree(time);

    // -adddb
    psMetadata *adddbArgs = psMetadataAlloc();
    psMetadataAddStr(adddbArgs, PS_LIST_TAIL, "-dvodb",  0,            "define DVO db (required)", NULL);

    // -dbs
    psMetadata *dbsArgs = psMetadataAlloc();
    psMetadataAddU64(dbsArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(dbsArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -addrun
    psMetadata *addrunArgs = psMetadataAlloc();
    psMetadataAddStr(addrunArgs, PS_LIST_TAIL, "-cal_id",  0,            "define calibration DB ID (required)", NULL);
    psMetadataAddStr(addrunArgs, PS_LIST_TAIL, "-region",  0,            "define region (required)", NULL);
    psMetadataAddStr(addrunArgs, PS_LIST_TAIL, "-last_step",  0,            "define last step (required)", NULL);
    psMetadataAddStr(addrunArgs, PS_LIST_TAIL, "-state",  0,            "define state (required)", NULL);

    // -runs
    psMetadata *runsArgs = psMetadataAlloc();
    psMetadataAddStr(runsArgs, PS_LIST_TAIL, "-cal_id",  0,            "search for calibration ID", NULL);
    psMetadataAddU64(runsArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(runsArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddBool(runsArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-adddb",   "add a DVO calibration DB",        CALTOOL_MODE_ADDDB,     adddbArgs);
    PXOPT_ADD_MODE("-dbs",     "list DVO calibration DBs",        CALTOOL_MODE_DBS,       dbsArgs);
    PXOPT_ADD_MODE("-addrun",  "add the results of a calibration run",        CALTOOL_MODE_ADDRUN,    addrunArgs);
    PXOPT_ADD_MODE("-runs",    "list the results of calibration runs",        CALTOOL_MODE_RUNS,      runsArgs);

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

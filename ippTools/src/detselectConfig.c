/*
 * detselectConfig.c
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

#include <psmodules.h>
#include <math.h>

#include "pxtools.h"
#include "detselect.h"

pxConfig *detselectConfig(pxConfig *config, int argc, char **argv)
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

    // -search
    psMetadata *searchArgs = psMetadataAlloc();
    psMetadataAddStr(searchArgs, PS_LIST_TAIL, "-inst", 0,                     "search by camera", NULL);
    psMetadataAddStr(searchArgs, PS_LIST_TAIL, "-telescope", 0,                     "search by telescope", NULL);
    psMetadataAddStr(searchArgs, PS_LIST_TAIL, "-det_type", 0,                     "search by detrend type", NULL);
    psMetadataAddStr(searchArgs, PS_LIST_TAIL, "-type", 0,                     "search by detrend type (alias for -det_type)", NULL);
    psMetadataAddStr(searchArgs, PS_LIST_TAIL, "-filter", 0,                     "search by filter", NULL);
    psMetadataAddF32(searchArgs, PS_LIST_TAIL, "-airmass", 0,                     "define airmass", NAN);
    psMetadataAddF32(searchArgs, PS_LIST_TAIL, "-exp_time", 0,                     "search by exposure time", NAN);
    psMetadataAddF32(searchArgs, PS_LIST_TAIL, "-ccd_temp", 0,                     "search by ccd tempature", NAN);
    psMetadataAddF64(searchArgs, PS_LIST_TAIL, "-posang", 0,                     "search by rotator position angle", NAN);
    psMetadataAddTime(searchArgs, PS_LIST_TAIL, "-time", 0,                      "define time for desired detrend data", NULL);
    psMetadataAddBool(searchArgs, PS_LIST_TAIL, "-simple",  0,                      "use the simple output format", false);
    psMetadataAddBool(searchArgs, PS_LIST_TAIL, "-unlimit",  0,                      "list all possible detruns, not just the best match", false);
 
    // -select
    psMetadata *selectArgs = psMetadataAlloc();
    psMetadataAddStr(selectArgs, PS_LIST_TAIL, "-det_id", 0,                     "search by detrend ID", NULL);
    psMetadataAddS32(selectArgs, PS_LIST_TAIL, "-iteration", 0,                     "search by iteration number", 0);
    psMetadataAddStr(selectArgs, PS_LIST_TAIL, "-class_id", 0,                     "search by class ID", NULL);
    psMetadataAddBool(selectArgs, PS_LIST_TAIL, "-simple",  0,                      "use the simple output format", false);

    // -show
    psMetadata *showArgs = psMetadataAlloc();
    psMetadataAddStr(showArgs, PS_LIST_TAIL, "-inst",       0,       "search by camera", NULL);
    psMetadataAddStr(showArgs, PS_LIST_TAIL, "-telescope",  0,       "search by telescope", NULL);
    psMetadataAddStr(showArgs, PS_LIST_TAIL, "-det_type",   0,       "search by detrend type", NULL);
    psMetadataAddStr(showArgs, PS_LIST_TAIL, "-type",       0,       "search by detrend type (alias for -det_type)", NULL);
    psMetadataAddBool(showArgs, PS_LIST_TAIL, "-simple",     0,       "use the simple output format", false);
    
    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-search",  "search for an appropriate detrend", DETSELECT_MODE_SEARCH, searchArgs);
    PXOPT_ADD_MODE("-select",  "retreive detrend information", DETSELECT_MODE_SELECT, selectArgs);
    PXOPT_ADD_MODE("-show",    "show all active detrends of this type with constraints", DETSELECT_MODE_SHOW, showArgs);

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

/*
 * pzgetexpConfig.c
 *
 * Copyright (C) 2006-2008  Joshua Hoblitt
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

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "pxtools.h"

pxConfig *pzgetexpConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    config->modules = pmConfigRead(&argc, argv, NULL);
    if (! config->modules) {
        psError(PS_ERR_UNKNOWN, false, "Can't find site configuration!\n");
        psFree(config);
        return NULL;
    }

    psMetadata *args = psMetadataAlloc();
    psMetadataAddStr(args , PS_LIST_TAIL, "-uri", 0,
        "DataStore product URI (required)", "");
    psMetadataAddStr(args , PS_LIST_TAIL, "-inst", 0,
        "camera name (required)", "");
    psMetadataAddStr(args , PS_LIST_TAIL, "-telescope",  0,
        "telescope name (required)", "");
    psMetadataAddS32(args, PS_LIST_TAIL, "-timeout",  0,
        "HTTP timeout", 0);
    psMetadataAddBool(args, PS_LIST_TAIL, "-all",  0,
        "download ALL filesets", 0);
    psMetadataAddBool(args, PS_LIST_TAIL, "-ignore-errors",  0,
        "do not fail if one of the entries raises a database error", 0);


    bool status = false;
    if (!psArgumentParse(args, &argc, argv)
        || argc != 1
        || strcmp(psMetadataLookupStr(&status, args, "-uri"), "") == 0
        || strcmp(psMetadataLookupStr(&status, args, "-inst"), "") == 0
        || strcmp(psMetadataLookupStr(&status, args, "-telescope"), "") == 0
    ) {
        fprintf(stderr, "error parsing arguments\n");
        printf("\nPan-STARRS Phase Z Get Exposures Tool\n");
        printf("Usage: %s -uri <uri> -inst <camera> -telescope <telescope> [-timeout <n>]\n\n",
            argv[0]);
        psArgumentHelp(args);
        psFree(config);
        return NULL;
    }

    config->args = args;
    // don't free args here as it's silly to increment the ref count then
    // "free" it

    // define Database handle, if used
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if(!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't connect to db\n");
        psFree(config);
        return NULL;
    }

    return config;
}

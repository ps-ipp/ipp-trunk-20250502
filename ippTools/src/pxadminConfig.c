/*
 * pxadminConfig.c
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

#include "pxtools.h"
#include "pxadmin.h"

static void pxAdminShowDB (const pxConfig *config, const char *program)
{
    fprintf (stderr, "\nPan-STARRS DataBase Admin Tool\n\n");
    fprintf (stderr, "Usage: %s [mode]\n", program);
    fprintf (stderr, " [mode] : -create | -create-mirror | -delete\n\n");

    psMetadataItem *server = pmConfigUserSite(config->modules, "DBSERVER",   PS_DATA_STRING);
    psMetadataItem *user   = pmConfigUserSite(config->modules, "DBUSER",     PS_DATA_STRING);
    psMetadataItem *name   = pmConfigUserSite(config->modules, "DBNAME",     PS_DATA_STRING);
    psMetadataItem *port   = pmConfigUserSite(config->modules, "DBPORT",     PS_TYPE_S32);

    if (!server || !user || !name) {
        psErrorClear();
        psWarning("Unable to determine database connection details.");
        return;
    }
    if (!port) {
        psErrorClear();
    }

    fprintf (stderr, "connecting to %s as %s (port %d)\n", server->data.str, user->data.str,
             port ? port->data.S32 : 0);
    fprintf (stderr, "using database %s\n\n", name->data.str);

    return;
}

pxConfig *pxAdminConfig(pxConfig *config, int argc, char **argv)
{
    if (!psArgumentGet(argc, argv, "-dbname")) {
        psError(PS_ERR_UNEXPECTED_NULL, true,
                "Command-line argument '-dbname XXX' is required, for database safety");
        return NULL;
    }

    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(PS_ERR_UNKNOWN, false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    // Parse other command-line arguments
    psMetadata *arguments = psMetadataAlloc(); // The arguments, with default values

    int N;
    config->mode = PXADMIN_MODE_NONE;
    if ((N = psArgumentGet(argc, argv, "-create"))) {
        psArgumentRemove(N, &argc, argv);
        if (config->mode) {
            psAbort("only one mode selection is allowed");
        }
        config->mode = PXADMIN_MODE_CREATE;
    }
    if ((N = psArgumentGet(argc, argv, "-create-mirror"))) {
        psArgumentRemove(N, &argc, argv);
        if (config->mode) {
            psAbort("only one mode selection is allowed");
        }
        config->mode = PXADMIN_MODE_CREATE_MIRROR;
    }
    if ((N = psArgumentGet(argc, argv, "-delete"))) {
        psArgumentRemove(N, &argc, argv);
        if (config->mode) {
            psAbort("only one mode selection is allowed");
        }
        config->mode = PXADMIN_MODE_DELETE;
    }
    if ((N = psArgumentGet(argc, argv, "-recreate"))) {
        psArgumentRemove(N, &argc, argv);
        if (config->mode) {
            psAbort("only one mode selection is allowed");
        }
        config->mode = PXADMIN_MODE_RECREATE;
    }

    // paul's argument parsing convention requires: -key value
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-create", 0,
            "create all IPP tables", "");
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-create-mirror", 0,
            "mirror all IPP tables", "");
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-delete", 0,
            "delete all IPP tables", "");
    psMetadataAddStr(arguments, PS_LIST_TAIL, "-recreate", 0,
            "delete and recreate all tables", "");

    if (config->mode == PXADMIN_MODE_NONE) {
    pxAdminShowDB (config, argv[0]);
        fprintf (stderr, "admin mode not specified\n");

        psArgumentHelp(arguments);
        psFree(arguments);
        psFree(config);
        return NULL;
    }

    if ((N = psArgumentGet (argc, argv, "-help"))) {
    pxAdminShowDB (config, argv[0]);
        psArgumentHelp(arguments);
        psFree(arguments);
        psFree(config);
        return NULL;
    }

    if (! psArgumentParse(arguments, &argc, argv) || argc != 1) {
    pxAdminShowDB (config, argv[0]);
        psArgumentHelp(arguments);
        psFree(arguments);
        psFree(config);
        return NULL;
    }

   psFree(arguments);

    // define Database handle, if used
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    // save argv/argc
    config->argv = argv;
    config->argc = argc;

    return config;
}

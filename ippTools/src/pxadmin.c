/*
 * pxadmin.c
 *
 * Copyright (C) 2006-2009  Joshua Hoblitt
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

#include <stdlib.h>
#include <string.h>

#include "pxtools.h"
#include "pxadmin.h"

bool createMode(pxConfig *config);
bool createMirrorMode(pxConfig *config);
bool deleteMode(pxConfig *config);
static bool insert_dbversion(pxConfig * config, const char *versionString);
static bool runMultipleStatments(pxConfig *config, const char *query);

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pxAdminConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        case PXADMIN_MODE_RECREATE:
            if (!deleteMode(config)) {
                goto FAIL;
            }
            // fall through
        case PXADMIN_MODE_CREATE:
            if (!createMode(config)) {
                goto FAIL;
            }
            break;
        case PXADMIN_MODE_CREATE_MIRROR:
            if (!createMirrorMode(config)) {
                goto FAIL;
            }
            break;
        case PXADMIN_MODE_DELETE:
            if (!deleteMode(config)) {
                goto FAIL;
            }
            break;
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


bool createMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psString query = pxDataGet("pxadmin_create_tables.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // BEGIN
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!runMultipleStatments(config, query)) {
        if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    if (!insert_dbversion(config, IPPDB_VERSION)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set database version");
        return false;
    }

    // COMMIT
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


bool createMirrorMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psString query = pxDataGet("pxadmin_create_mirror_tables.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // BEGIN
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!runMultipleStatments(config, query)) {
        if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    if (!insert_dbversion(config, IPPDB_VERSION)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set database version");
        return false;
    }

    // COMMIT
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


bool deleteMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    {
        char line[128], answer[128];

        psMetadataItem *name = pmConfigUserSite(config->modules, "DBNAME", PS_DATA_STRING);
        if (!name) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine database name.");
            return false;
        }
        psString dbName = name->data.str;

	// if name is e.g., gpc1, strcasecmp returns FALSE
	bool allowDelete = true;
	allowDelete = allowDelete && strcasecmp(dbName, "gpc1");  
	allowDelete = allowDelete && strcasecmp(dbName, "gpc2");  
	allowDelete = allowDelete && strcasecmp(dbName, "nebulous");  
	allowDelete = allowDelete && strcasecmp(dbName, "isp");  
	allowDelete = allowDelete && strcasecmp(dbName, "ssp");  
	allowDelete = allowDelete && strcasecmp(dbName, "uic");  
	allowDelete = allowDelete && strcasecmp(dbName, "hsc_v1");  
	allowDelete = allowDelete && strcasecmp(dbName, "megacam_v1");  

	if (!allowDelete) {
            fprintf (stdout, "**** WARNING: not allowed to delete database %s.\n", dbName);
            return false;
        }

        fprintf(stdout, "*** delete the tables from database %s? ***\n", dbName);
        fprintf(stdout, "*** to delete the tables, answer YES, and give password ***\n");
        fprintf(stdout, "*** WARNING: this action is permanent ***\n\n");

        fprintf(stdout, "delete the tables (YES/[n]): ");
        if (!fgets(line, 128, stdin)) {
            psError(PS_ERR_IO, true, "Unable to read response.");
            return false;
        }
        sscanf(line, "%s", answer);
        if (strcmp (answer, "YES"))  {
            psError(PS_ERR_UNKNOWN, true, "tables NOT deleleted");
            return false;
        }

        fprintf(stdout, "enter dbh connection password: ");
        if (!fgets(line, 128, stdin)) {
            psError(PS_ERR_IO, true, "Unable to read response.");
            return false;
        }
        sscanf(line, "%s", answer);

        psMetadataItem *pass = pmConfigUserSite(config->modules, "DBPASSWORD", PS_DATA_STRING);
        if (!pass) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine database password.");
            return false;
        }
        psString dbPassword = pass->data.str;
        if (strcmp (answer, dbPassword)) {
            psError(PS_ERR_UNKNOWN, true, "invalid passwd - tables NOT deleleted");
            return false;
        }
    }

    psString query = pxDataGet("pxadmin_drop_tables.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // BEGIN
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!runMultipleStatments(config, query)) {
        if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    // COMMIT
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool runMultipleStatments(pxConfig *config, const char *query)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(query, false);

    // loop over all statements in string
    psList *statements = psStringSplit(query, ";", false);

    psString q = NULL;
    psListIterator *iter = psListIteratorAlloc(statements, PS_LIST_HEAD, false);
    while ((q = psListGetAndIncrement(iter))) {
        if (!p_psDBRunQuery(config->dbh, q)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(iter);
            psFree(statements);
            return false;
        }
    }
    psFree(iter);
    psFree(statements);

    return true;
}

#ifdef notdef
static bool update_dbversion(pxConfig * config, const char *versionString)
{
    psString query = pxDataGet("pxadmin_update_version.sql");
    if (!query) {
        psError(PS_ERR_UNKNOWN, false, "failed to retrieve SQL statement");
        psFree(query);
        return false;
    }
    if (!p_psDBRunQueryF(config->dbh, query, versionString)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    return true;
}
#endif
static bool insert_dbversion(pxConfig * config, const char *versionString)
{
    psString query = "INSERT INTO dbversion VALUES('%s', CURRENT_TIMESTAMP())";
    if (!query) {
        psError(PS_ERR_UNKNOWN, false, "failed to retrieve SQL statement");
        psFree(query);
        return false;
    }
    if (!p_psDBRunQueryF(config->dbh, query, versionString)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    return true;
}

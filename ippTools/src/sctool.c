/*
 * sctool.c
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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "pxtools.h"
#include "pxdata.h"
#include "pxchip.h"
#include "pxspace.h"

#include "sctool.h"

static bool defineskycellsMode(pxConfig *config);
static bool listMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv) {
    psLibInit(NULL);

    pxConfig *config = sctoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(SCTOOL_MODE_DEFINESKYCELLS,    defineskycellsMode);
        MODECASE(SCTOOL_MODE_LIST,              listMode);
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


static bool defineskycellsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_STR(inputFile, config->args, "-input", true, false);

    unsigned int numBad = 0;
    psMetadata *skycells = psMetadataConfigRead(NULL, &numBad, inputFile, false);
    if (!skycells || numBad > 0) {
        psError(PXTOOLS_ERR_UNKNOWN, false, "failed to read %s", inputFile);
        return false;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(skycells, PS_LIST_HEAD, NULL);

    psMetadataItem *item;
    int numSkycells = 0;
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_METADATA) {
            psError(PS_ERR_UNKNOWN, true, "unexpected item in metadata type: %d", item->type);
            return false;
        }
        psMetadata *md = item->data.md;
        psString tess_id = psMetadataLookupStr(NULL, md, "tess_id");
        psString skycell_id = psMetadataLookupStr(NULL, md, "skycell_id");

        skycellRow *skycell = skycellObjectFromMetadata(md);
        if (!skycell) {
            psError(PS_ERR_UNKNOWN, false, "failed to create skycellRow from Metadata");
            return false;
        }

        numSkycells++;
        if (!skycellInsertObject(config->dbh, skycell)) {
            psError(PS_ERR_UNKNOWN, false, "failed to insert skycell %s %s", tess_id, skycell_id);
            return false;
        }
    } 
    psFree(iter);
    psFree(skycells);

   if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psLogMsg("sctool", PS_LOG_INFO, "%d skycells added", numSkycells);

    return true;
}
static bool listMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell.skycell_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "skycell.tess_id", "LIKE");
    pxskycellAddWhere(config, where);

    psString query = pxDataGet("sctool_list.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!psListLength(where->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search paramters are required\n");
        psFree(where);
        return false;
    }
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!psArrayLength(output)) {
        psTrace("sctool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "skycell", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);


    return true;
}

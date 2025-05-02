/*
 * magictool.c
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ippdb.h>

#include "pxtools.h"
#include "labeltool.h"

static bool definelabelMode(pxConfig *config);
static bool updatelabelMode(pxConfig *config);
static bool deletelabelMode(pxConfig *config);
static bool listlabelMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = labeltoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(LABELTOOL_MODE_DEFINELABEL,         definelabelMode);
        MODECASE(LABELTOOL_MODE_UPDATELABEL,         updatelabelMode);
        MODECASE(LABELTOOL_MODE_DELETELABEL,         deletelabelMode);
        MODECASE(LABELTOOL_MODE_LISTLABEL,           listlabelMode);
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

static bool definelabelMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(label,    config->args, "-set_label", true, false);
    // XXX: perhaps we should have a default priority?
    PXOPT_LOOKUP_S32(priority, config->args, "-set_priority", true, false);

    // optional
    PXOPT_LOOKUP_BOOL(inactive, config->args, "-set_inactive", false);
    PXOPT_LOOKUP_STR(comment, config->args, "-set_comment", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    LabelRow *row = LabelRowAlloc(
            label,
            priority,
            inactive ? false : true,
            comment
    );

    if (!row) {
        psError(PS_ERR_UNKNOWN, false, "failed to allocate Label object");
        return false;
    }
    if (!LabelInsertObject(config->dbh, row)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        return false;
    }

    if (!LabelPrintObject(stdout, row, !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print object");
            psFree(row);
            return false;
    }

    psFree(row);

    return true;
}


static bool updatelabelMode(pxConfig *config)
{
    PXOPT_LOOKUP_STR(label,    config->args, "-label", true, false);


    // optional (at least one is required)
    PXOPT_LOOKUP_S32(priority, config->args, "-set_priority", false, false);
    PXOPT_LOOKUP_BOOL(inactive, config->args, "-set_inactive", false);
    PXOPT_LOOKUP_BOOL(active, config->args, "-set_active", false);
    PXOPT_LOOKUP_STR(comment, config->args, "-set_comment", false, false);

    if (! (priority || active || inactive || comment) ) {
        psError(PS_ERR_UNKNOWN, true, "at least one -set option is required\n");
        return false;
    }
    if (active && inactive) {
        psError(PS_ERR_UNKNOWN, true, "only one of -active and -inactive may be supplied");
        return false;
    }

    psString query = psStringCopy("UPDATE Label SET");
    char sep = ' ';

    if (priority) {
        psStringAppend(&query, "%c priority = %d", sep, priority);
        sep = ',';
    }
    if (active) {
        psStringAppend(&query, "%c active = %d", sep, 1);
        sep = ',';
    }
    if (inactive) {
        psStringAppend(&query, "%c active = %d", sep, 0);
        sep = ',';
    }
    if (comment) {
        psStringAppend(&query, "%c comment = '%s'", sep, comment);
        sep = ',';
    }

    psStringAppend(&query, " WHERE label = '%s'", label);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool deletelabelMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label,    config->args, "-label", true, false);

    psString query = NULL;
    
    psStringAppend(&query, "DELETE FROM Label WHERE label = '%s'", label);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool listlabelMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-label", "label", "LIKE");

    PXOPT_LOOKUP_BOOL(active, config->args,   "-active", false);
    PXOPT_LOOKUP_BOOL(inactive, config->args, "-inactive", false);
    if (active && inactive) {
        psError(PS_ERR_UNKNOWN, true, "only one of -active and inactive may be supplied");
        return false;
    }
    PXOPT_LOOKUP_BOOL(lowtohigh, config->args, "-lowtohigh", false);
    PXOPT_LOOKUP_BOOL(hightolow, config->args, "-hightolow", false);
    if (lowtohigh && hightolow) {
        psError(PS_ERR_UNKNOWN, true, "only one of -lowtohigh and -hightolow may be supplied");
        return false;
    }

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = psStringCopy("SELECT * FROM Label\n");

    char *sep = " WHERE ";
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
        sep = " AND ";
    }
    psFree(where);

    if (active) {
        psStringAppend(&query, "%s active\n", sep);
        sep = " AND ";
    } else if (inactive) {
        psStringAppend(&query, "%s NOT active\n", sep);
        sep = " AND ";
    }

    if (lowtohigh || hightolow) {
        char *order = lowtohigh ? "" : "DESC";
        psStringAppend(&query, "\nORDER BY priority %s\n", order);
    }

    // treat limit == 0 as "no limit"
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
        psErrorCode err = psErrorCodeLast();
        switch (err) {
            case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
        }

        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("labeltool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "Label", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

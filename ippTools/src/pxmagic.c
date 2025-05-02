/*
 * pxmagic.c
 *
 * Copyright (C) 2009 IfA
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
#include <ippdb.h>
#include <string.h>

#include "pxtools.h"
#include "pxmagic.h"

bool pxmagicRestoreStage(pxConfig *config, psString stage, psString whereClause, psString newState)
{
    psString queryFile = NULL;
    psStringAppend(&queryFile, "magictool_restore_%s.sql", stage);
    psString query_temp = pxDataGet(queryFile);
    if (!query_temp) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement from %s", queryFile);
        psFree(queryFile);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(queryFile);

    // psStringSubstittute fails on strings created by pxDataGet()
    psString query = psStringCopy(query_temp);
    // change the @NEW_STATE@ in the sql file to our state
    if (!psStringSubstitute(&query, newState, "@NEW_STATE@")) {
        psError(PS_ERR_UNKNOWN, false, "failed to substitute state string");
        return false;
    }

    psStringAppend(&query, " AND %s",  whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }
    psFree(query);

    return true;
}

bool pxmagicAddWhere(pxConfig *config, psString *out, psString table)
{
    PXOPT_LOOKUP_U64(magicked, config->args, "-magicked", false, false);
    PXOPT_LOOKUP_BOOL(destreaked, config->args,     "-destreaked", false);
    PXOPT_LOOKUP_BOOL(not_destreaked, config->args, "-not_destreaked", false);

    if (not_destreaked) {
        if (destreaked) {
            psError(PXTOOLS_ERR_CONFIG, true, "providing -not_destreaked and -destreaked makes no sense");
            return false;
        }
        if (magicked) {
            psError(PXTOOLS_ERR_CONFIG, true, "providing -not_destreaked and -magicked makes no sense");
            return false;
        }
        psStringAppend(out, " AND %s.magicked = 0", table);
    }
    if (destreaked) {
        psStringAppend(out, " AND %s.magicked != 0", table);
    }
    // Note -magicked is  handled by the caller. XXX: Why?
    return true;
}

void pxmagicAddArguments(psMetadata *md)
{
    psMetadataAddBool(md, PS_LIST_TAIL, "-destreaked",  0,      "search for destreaked images", false);
    psMetadataAddBool(md, PS_LIST_TAIL, "-not_destreaked",  0,  "search for images that have not been destreaked", false);
    psMetadataAddS64(md, PS_LIST_TAIL,  "-magicked", 0,         "search by magicked value", 0);
}

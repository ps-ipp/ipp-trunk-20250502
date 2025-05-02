/*
 * pxtag.c
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>

#include "pxtools.h"
#include "pxtag.h"

psString pxGenExpTag(pxConfig *config, const char *exp_name)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // start a transaction so we don't increment the expTag counter unless we
    // can successfully retreive it's value
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return NULL;
    }

    if (!p_psDBRunQuery(config->dbh,
        "UPDATE expTagCounter SET counter = LAST_INSERT_ID(counter + 1)")) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return NULL;
    }

    // psDBLastInsertID() can't be used here as it called mysql_insert_id()
    // which doesn't work with this trick.  See:
    // http://dev.mysql.com/doc/refman/4.1/en/information-functions.html
    if (!p_psDBRunQuery(config->dbh, "SELECT LAST_INSERT_ID() as counter")) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return NULL;
    }

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return NULL;
    }
    // sanity check that we only got one row
    if (psArrayLength(output) != 1) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "should have gotten 1 row but %lu rows were returned", psArrayLength(output));
        psFree(output);
        return NULL;
    }

    // point of no return 
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return NULL;
    }

    psMetadata *row = output->data[0];
    bool status = false;
    psU64 counter = psMetadataLookupU64(&status, row, "counter");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for counter");
        psFree(output);
        return NULL;
    }
    psString exp_id = NULL;
    psStringAppend(&exp_id, "%s.%" PRIu64, exp_name, counter);
    psFree(output);

    return exp_id;
}

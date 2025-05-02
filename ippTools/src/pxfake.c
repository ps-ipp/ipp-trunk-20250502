/*
 * pxfake.c
 *
 * Copyright (C) 2007-2008  Joshua Hoblitt
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
#include "pxcam.h"

bool pxfakeRunSetState(pxConfig *config, psS64 fake_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid fakeRun state: %s", state);
        return false;
    }

    char *query = "UPDATE fakeRun SET state = '%s' WHERE fake_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, fake_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for fake_id %" PRId64, fake_id);
        return false;
    }

    return true;
}

psS64 pxfakeQueueByCamID(pxConfig *config,
                    psS64 cam_id,
                    char *workdir,
                    char *label,
                    char *data_group,
                    char *dist_group,
                    char *reduction,
                    char *expgroup,
                    char *dvodb,
                    char *tess_id,
                    char *end_stage,
                    char *note)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psTrace("pxtool", PS_LOG_INFO, "attemping to queue cam_id: %"PRId64, cam_id);

    // load the SQL to enqueue our exp_ids from disk once
    static psString query = NULL;
    if (!query) {
        query = pxDataGet("faketool_queue_cam_id.sql");
        psMemSetPersistent(query, true);
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }
    }

    // queue the exp
    // XXX chip_id is being cast here work around psS64 have a different type
    // different on 32/64
    if (!p_psDBRunQueryF(config->dbh, query,
                "new", // state
                workdir  ? workdir  : "NULL",
                label    ? label    : "NULL",
                data_group ? data_group : "NULL",
                dist_group ? dist_group : "NULL",
                reduction? reduction: "NULL",
                expgroup ? expgroup : "NULL",
                dvodb    ? dvodb    : "NULL",
                tess_id  ? tess_id  : "NULL",
                end_stage ? end_stage : "NULL",
                note     ? note     : "NULL",
                (long long)cam_id
    )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // just to be safe, we should have changed at least one row
    if (psDBAffectedRows(config->dbh) != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "should have changed just one row");
        return false;
    }

    return psDBLastInsertID(config->dbh);
}

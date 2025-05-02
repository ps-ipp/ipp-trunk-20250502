/*
 * register.c
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

#include <stdlib.h>
#include <ippdb.h>
#include <string.h>

#include "pxtools.h"
#include "pxregister.h"

bool pxnewExpSetState(pxConfig *config, psS64 exp_id, const char *state)
{
    if (!exp_id) {
        psError(PS_ERR_UNKNOWN, true, "0 is not a valid exp_id");
        return false;
    }
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!(
            (strncmp(state, "run", 4) == 0)
            || (strncmp(state, "stop", 5) == 0)
            || (strncmp(state, "reg", 4) == 0)
        )
    ) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid newExp state: %s", state);
        return false;
    }

    char *query = "UPDATE newExp SET state = '%s' WHERE exp_id = %"PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, exp_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for exp_id %"PRId64, exp_id);
        return false;
    }

    return true;
}

bool pxrawExpSetState(pxConfig *config, psS64 exp_id, const char *state)
{
    if (!exp_id) {
        psError(PS_ERR_UNKNOWN, true, "0 is not a valid exp_id");
        return false;
    }
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!(
            (strncmp(state, "full", 5) == 0)
            || (strncmp(state, "new", 4) == 0)
	    || (strncmp(state, "goto_compressed", 14) == 0)
	    || (strncmp(state, "compressed", 11) == 0)
	    || (strncmp(state, "goto_lossy", 11) == 0)
            || (strncmp(state, "lossy", 6) == 0)
        )
    ) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid rawExp state: %s", state);
        return false;
    }

    char *query = "UPDATE rawExp SET state = '%s' WHERE exp_id = %"PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, exp_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for exp_id %"PRId64, exp_id);
        return false;
    }

    return true;
}
bool pxrawImfileSetState(pxConfig *config, psS64 exp_id, const char *class_id, const char *state)
{
    if (!exp_id) {
        psError(PS_ERR_UNKNOWN, true, "0 is not a valid exp_id");
        return false;
    }
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!(
            (strncmp(state, "full", 5) == 0)
            || (strncmp(state, "new", 4) == 0)
	    || (strncmp(state, "goto_compressed", 14) == 0)
	    || (strncmp(state, "compressed", 11) == 0)
	    || (strncmp(state, "goto_lossy", 11) == 0)
            || (strncmp(state, "lossy", 6) == 0)
        )
    ) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid rawImfile data_state: %s", state);
        return false;
    }

    char *query = "UPDATE rawImfile SET data_state = '%s' WHERE class_id = %s AND exp_id = %"PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, class_id, exp_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for exp_id %"PRId64, exp_id);
        return false;
    }

    return true;
}

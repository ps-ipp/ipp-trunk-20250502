/*
 * pxwarp.c
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
#include "pxwarp.h"

bool pxwarpQueueByFakeID(pxConfig *config,
                         psS64 fake_id,
                         const char *workdir,
                         const char *label,
                         const char *data_group,
                         const char *dist_group,
                         const char *dvodb,
                         const char *tess_id,
                         const char *reduction,
                         const char *end_stage,
                         const char *note)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // depend on the f-keys to make sure we have a valid cam_id
    if (!warpRunInsert(config->dbh,
        0,          // ID
        fake_id,
        "warp",     // mode
        "new",      // state
        workdir,
        "dirty",    // workdir_state
        label,
        data_group,
        dist_group,
        dvodb,
        tess_id,
        reduction,
        end_stage,
        NULL,      // registered
        0,         // magicked set to zero when created may get updated when warpRun goes to 'full'
		       NULL, // version
		       0,   // maskfrac_npix
		       NAN, // static
		       NAN, // dynamic
		       NAN, // magic
		       NAN, // advisory		       
        NULL        // note
    )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return true;
    }

    return true;
}

/*
 * pxadd.c
 *
 * Copyright (C) 2007  Joshua Hoblitt
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
#include "pxminidvodb.h"

bool pxaddQueueByMinidvodbID(pxConfig *config,
                       psS64 minidvodb_id,
                       char *destination_host,
                       char *minidvodb_rsync_path
  )
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // load the SQL to enqueue our exp_ids from disk once
    static psString query = NULL;
    if (!query) {
        query = pxDataGet("minidvodbtool_queue_minidvodb_id.sql");
        psMemSetPersistent(query, true);
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }
    }

    // queue the exp
    // Note: cam_id is being cast here work around psS64 have a different type different
    // on 32/64
    //if (!p_psDBRunQueryF(config->dbh, query,
    //			 minidvodb_rsync_path   ? minidvodb_rsync_path     : "NULL",
    //			 destination_host  ? destination_host   : "NULL",
    //                  	 0,  //fault
			   //			 "new", // state
    //			 "NULL",
						   //                      0, //dtime
                       
									     //                    (long long) minidvodb_id
    //    )) 
    if (!p_psDBRunQueryF(config->dbh,"INSERT INTO minidvodbCopy SELECT         0,   minidvodb_id, '%s', '%s', 0, 'new', NULL, 0 FROM minidvodbRun     WHERE    minidvodbRun.minidvodb_id = %lld and minidvodbRun.state = 'merged'", minidvodb_rsync_path, destination_host, (long long) minidvodb_id))

    {
      psError(PS_ERR_UNKNOWN, false, "database error %s", query);
        return false;
    }

    // just to be safe, we should have changed at least one row
    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false,
                "no rows affected - should have changed at least one row \n%s", query);
        return false;
    }

    return true;
}

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
#include "pxadd.h"

bool pxaddRunSetState(pxConfig *config, psS64 add_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid stageRun state: %s", state);
        return false;
    }

    char *query = "UPDATE addRun SET state = '%s' WHERE add_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, add_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for add_id %" PRId64, add_id);
        return false;
    }

    return true;
}


bool pxaddRunSetStateByQuery(pxConfig *config, psMetadata *where, const char *stage, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid stageRun state: %s", state);
        return false;
    }
    psString query = NULL;
    if (strcmp(stage, "cam") == 0) {
      query = psStringCopy("UPDATE addRun JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) SET addRun.state = '%s'");
    } 
    if (strcmp(stage, "stack") == 0) {
      ///xxx this needs to be fixed
     
      /// psString query = psStringCopy("UPDATE addRun JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) SET addRun.state = '%s'");
    } 
    if (where) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQueryF(config->dbh, query, state)) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(query);

    return true;
}


bool pxaddRunSetLabel(pxConfig *config, psS64 add_id, const char *label)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    // note label == NULL should be explicitly allowed

    char *query = "UPDATE addRun SET addRun.label = '%s' WHERE add_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, label, add_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for add_id %" PRId64, add_id);
        return false;
    }

    return true;
}

bool pxaddRunSetLabelByQuery(pxConfig *config, psMetadata *where, const char *stage, const char *label)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    // note label == NULL should be explicitly allowed
    //xxx fix for stack
    psString query = psStringCopy("UPDATE addRun JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) SET addRun.label = '%s'");

    if (where) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQueryF(config->dbh, query, label)) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(query);

    return true;
}

bool pxaddQueueByCamID(pxConfig *config,
		       char *stage,
                       psS64 stage_id,
		       psS32 stage_extra1,
                       char *workdir,
                       char *reduction,
                       char *label,
                       char *data_group,
                       char *dvodb,
                       char *note,
                       bool image_only,
		       bool minidvodb,
		       char *minidvodb_group,
		       char *minidvodb_name,
		       char *minidvodb_host)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // load the SQL to enqueue our exp_ids from disk once
    static psString query = NULL;
    if (!query) {
      if (strcmp( stage , "cam") == 0) {
        query = pxDataGet("addtool_queue_cam_id.sql");
        psMemSetPersistent(query, true);
      } 
      else if (strcmp(stage,"stack") == 0) {
	query = pxDataGet("addtool_queue_stack_id.sql");
	psMemSetPersistent(query, true);
      }
      else if (strcmp(stage,"staticsky") == 0) {
	query = pxDataGet("addtool_queue_sky_id_multi.sql");
        psMemSetPersistent(query, true);
      } 
      else if (strcmp(stage,"skycal") == 0) {
	query = pxDataGet("addtool_queue_skycal_id.sql");
        psMemSetPersistent(query, true);
      }
      else if (strcmp(stage,"diff") == 0) {
	query = pxDataGet("addtool_queue_diff_id.sql");
        psMemSetPersistent(query, true);
      }
      else if (strcmp(stage,"fullforce") == 0) {
	query = pxDataGet("addtool_queue_ff_id.sql");
        psMemSetPersistent(query, true);
      }
      else if (strcmp(stage,"fullforce_summary") == 0) {
	query = pxDataGet("addtool_queue_ffsummary.sql");
	psMemSetPersistent(query,true);
      }


      else 
	{
	  psError(PXTOOLS_ERR_SYS, false, "uknown stage %s for pxadd.c", stage);
	  return false;
	}

      if (!query) {
	psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
	return false;
      }
    }

    psTrace("addtool.c", PS_LOG_INFO, "pxadd query \n%s\n",query);
    // queue the exp
    // Note: cam_id is being cast here work around psS64 have a different type different
    // on 32/64
    if (!p_psDBRunQueryF(config->dbh, query,
			 stage_extra1 ? stage_extra1 : 0,
			 "new", // state
                         workdir  ? workdir   : "NULL",
			  "dirty", //workdir_state
                         reduction? reduction : "NULL",
                         label    ? label     : "NULL",
                         data_group ? data_group     : "NULL",
                         dvodb    ? dvodb     : "NULL",
                         note     ? note     : "NULL",
                         image_only,
			 minidvodb,
			 minidvodb_group,
			 minidvodb_name,
			 minidvodb_host,
                         (long long) stage_id
    )) {
      psError(PS_ERR_UNKNOWN, false, "database error %s", query);
        return false;
    }

    // just to be safe, we should have changed at least one row
    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false,
                "no rows affected - should have changed at least one row");
        return false;
    }

    return true;
}

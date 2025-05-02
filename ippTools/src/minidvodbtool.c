/*
 * addtool.c
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
#include <stdint.h>
#include <inttypes.h>

#include "pxtools.h"
#include "pxadd.h"
#include "pxminidvodb.h"
#include "minidvodbtool.h"

static bool definebyqueryMode(pxConfig *config);
static bool listminidvodbcopyMode(pxConfig *config);
static bool revertminidvodbcopyMode(pxConfig *config);
static bool updateminidvodbcopyMode(pxConfig *config);



# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = addtoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(ADDTOOL_MODE_DEFINEBYQUERY,        definebyqueryMode);
        MODECASE(ADDTOOL_MODE_LISTMINIDVODBCOPY, listminidvodbcopyMode);
        MODECASE(ADDTOOL_MODE_REVERTMINIDVODBCOPY, revertminidvodbcopyMode);
        MODECASE(ADDTOOL_MODE_UPDATEMINIDVODBCOPY, updateminidvodbcopyMode);

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


static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where,  "-minidvodb_id",    "minidvodbRun.minidvodb_id", "==");
    pxAddLabelSearchArgs (config, where, "-minidvodb_group",     "minidvodbRun.minidvodb_group", "=="); // define using camRun label
   


    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(destination_host,     config->args, "-set_destination_host", false, false);
    PXOPT_LOOKUP_STR(minidvodb_rsync_path, config->args, "-set_minidvodb_rsync_path", false, false);
    PXOPT_LOOKUP_BOOL(pretend,    config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple,     config->args, "-simple", false);

    // prevent queueing an addRun if a given exposure has already been added to
    // the given dvo database
    psString dvodb_string = NULL;
    psString bare_query = NULL;
    if (destination_host) {
      psTrace("minidvodbtool.c", PS_LOG_INFO, "destination_host argument found (%s) using minidvodbtool_find_minidvodb_id_dvo.sql\n", destination_host);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("minidvodbtool_find_minidvodb_id_dvo.sql");
	// user supplied dvodb
	psStringAppend(&dvodb_string, "minidvodbCopy.destination_host = '%s'", destination_host);
    } else {
      psError(PS_ERR_UNKNOWN, false, "cannot queue minidvodbcopy run without a defined destination_host");
       
            return false;
         }
    if (!bare_query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
	psFree(where);
	return false;
    }
    // Take the bare query and add the dvodb selector
    psString query = NULL;
    psStringAppend(&query, bare_query, dvodb_string);
    psFree(dvodb_string);
    psFree(bare_query);

    // use psDBGenerateWhereConditionSQL because the SQL ends in a WHERE
    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        return false;
    }
    psFree(where);

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
        psTrace("addtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "minidvodbCopy", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    // loop over our list of camRun rows to check the supplied and selected dvodb and workdir values:
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        minidvodbRunRow *row = minidvodbRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into minidvodbRun");
            psFree(output);
            return false;
        }

        if (!minidvodb_rsync_path) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue minidvodbcopy run without a defined minidvodb_rsync_path: minidvodb_id %" PRId64, row->minidvodb_id);
            psFree(output);
            return false;
        }
        if (!destination_host) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue minidvodbcopy run without a defined destination_host: minidvodb_id %" PRId64, row->minidvodb_id);
            psFree(output);
            return false;
        }

        psFree(row);
    }

    // start a transaction so we don't end up with an exp without any associted
    // imfiles
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // would could do this "all in the database" if we didn't want the option
    // of changing the label/reduction/expgroup/dvodb/etc.  So we're pulling the
    // data out so we have the option of changing these values or leaving the
    // old values in place (i.e., passing the values through).

    // loop over our list of camRun rows
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        minidvodbRunRow *row = minidvodbRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxaddQueueByMinidvodbID(config,
                               row->minidvodb_id,
				     destination_host ? destination_host : "NULL",
				     minidvodb_rsync_path ?minidvodb_rsync_path : "NULL"
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error sfg");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue minidvodb_id: %" PRId64, row->minidvodb_id);
            psFree(row);
            psFree(output);
            return false;
        }
        psFree(row);
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}




static bool listminidvodbcopyMode(pxConfig *config) {
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id", "minidvodbCopy.minidvodb_id", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodbcopy_id", "minidvodbCopy.minidvodbcopy_id", "==");
  PXOPT_COPY_STR(config->args, where, "-destination_host", "minidvodbCopy.destination_host", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_group", "minidvodbRun.minidvodb_group", "==");
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_BOOL(pending, config->args, "-pending", false);
  PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
  if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

  psString query = pxDataGet("minidvodbtool_find_minidvodbcopy.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    }

// we either add AND (condition) or WHERE (condition):
    if (where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " AND minidvodbCopy.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND minidvodbCopy.fault = 0");
    }
    if (!where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE minidvodbCopy.fault != 0");
    }
    if (!where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE minidvodbCopy.fault = 0");
    }
    psFree(where);

    if (pending) {
       //add the cuts for pending (state new, no faults)
      psStringAppend(&query, " %s", " AND minidvodbCopy.state = 'new' AND minidvodbCopy.fault = 0 AND minidvodbCopy.destination_host IS NOT NULL AND minidvodbCopy.minidvodb_rsync_path IS NOT NULL");
    }


    // order by epoch
    psStringAppend(&query, " ORDER BY minidvodbcopy_id");

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
      psError(PS_ERR_UNKNOWN, false, "database error %s ", query);
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
        psTrace("addtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "minidvodbCopy", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

return true;
}



static bool revertminidvodbcopyMode(pxConfig *config) {
  psMetadata *where = psMetadataAlloc();
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_COPY_S64(config->args, where, "-minidvodbcopy_id", "minidvodbCopy.minidvodbcopy_id", "==");
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id", "minidvodbCopy.minidvodb_id", "==");
  PXOPT_COPY_STR(config->args, where, "-destination_host", "destination_host", "==");
  PXOPT_COPY_S16(config->args, where, "-fault", "minidvodbCopy.fault", "==");

  if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  if (!psDBTransaction(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(where);
      return false;
  }

  {
    psString query = pxDataGet("minidvodbtool_revertminidvodbcopy.sql");
    if (!query) {
      // rollback
      if (!psDBRollback(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
      psFree(where);
      return false;
    }

    // use psDBGenerateWhereConditionalSQL with AND ... because the SQL ends in a WHERE
    if (where && psListLength(where->list)) {
      psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
      psStringAppend(&query, " AND %s", whereClause);
      psFree(whereClause);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
      // rollback
      if (!psDBRollback(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error %s", query);
      }
      psError(PS_ERR_UNKNOWN, false, "database error %s", query );
      psFree(query);
      psFree(where);
            return false;
    }
    psFree(query);
  }
  psFree(where);

  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  return true;
}



static bool updateminidvodbcopyMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);
  psMetadata *where = psMetadataAlloc();

  // PXOPT_LOOKUP_U64(minidvodbcopy_id,  config->args, "-minidvodbcopy_id", false, false);
  // PXOPT_LOOKUP_U64(minidvodb_id,  config->args, "-minidvodb_id", false, false);
  PXOPT_LOOKUP_STR(state,  config->args, "-state", false, false);
  PXOPT_LOOKUP_STR(host,  config->args, "-host", false, false);
  PXOPT_LOOKUP_STR(rsync_path,  config->args, "-minidvodb_rsync_path", false, false);
  // PXOPT_LOOKUP_S16(fault,  config->args, "-fault", false, false);
  
  PXOPT_LOOKUP_S16(set_fault,  config->args, "-set_fault", false, false);
  PXOPT_LOOKUP_STR(set_rsync_path,  config->args, "-set_minidvodb_rsync_path", false, false);
  PXOPT_LOOKUP_STR(set_host,  config->args, "-set_destination_host", false, false);
  PXOPT_LOOKUP_STR(set_state,  config->args, "-set_state", false, false);
  PXOPT_LOOKUP_F32(dtime,  config->args, "-set_dtime", false, false);
  PXOPT_COPY_S64(config->args, where, "-minidvodbcopy_id",     "minidvodbCopy.minidvodbcopy_id", "==");
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id",     "minidvodbCopy.minidvodb_id", "==");
  
  PXOPT_COPY_STR(config->args, where, "-state",     "minidvodbCopy.state", "==");
  PXOPT_COPY_STR(config->args, where, "-host",     "minidvodbCopy.destination_host", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_rsync_path",     "minidvodbCopy.minidvodb_rsync_path", "==");


  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  psString query = psStringCopy("UPDATE minidvodbCopy JOIN minidvodbRun USING (minidvodb_id) SET ");
  int cnt = 0;
  psString comma = ",";
  if (set_fault) {
    psStringAppend(&query, " minidvodbCopy.fault = %d", set_fault);
  cnt++;
  }

  if (set_rsync_path) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }

    psStringAppend(&query, " minidvodbCopy.minidvodb_rsync_path = '%s'" , set_rsync_path);
    cnt++;
  }

  if (set_host) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }

    psStringAppend(&query, " minidvodbCopy.destination_host = '%s'" , set_host);
    cnt++;
  }


  if (set_state) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }

    psStringAppend(&query, " minidvodbCopy.state = '%s'" , set_state);
    cnt++;
  }

  if (dtime) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " minidvodbCopy.dtime = %f", dtime);
    cnt++;
  }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query, " WHERE %s", whereClause);

  
  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error %s", query);
   psFree(query);
   return false;
  }

  psFree(query);
  psFree(where);

  return true;
}



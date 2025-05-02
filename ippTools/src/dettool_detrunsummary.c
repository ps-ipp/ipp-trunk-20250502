/*
 * dettool_detrunsummary.c
 *
 * Copyright (C) 2006  Joshua Hoblitt & EAM; IfA
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

#include "dettool.h"

static detRunSummaryRow *mdToDetRunSummary(pxConfig *config, psMetadata *row);

/* The SQL returns a list of detrend runs (with detrend id, iteration and detrend type) which
 * have completed all residexps.
 */

bool todetrunsummaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_todetrunsummary.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
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
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detRejectExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// this function is used to generate the detrunSummary entries used below
// XXX why is this a separate function?  roll it back into adddetrunsummary?
static detRunSummaryRow *mdToDetRunSummary(pxConfig *config, psMetadata *row)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    bool status = false;
    // from row
    psS64 det_id = psMetadataLookupS64(&status, row, "det_id");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for det_id");
        return false;
    }
    psS32 iteration = psMetadataLookupS32(&status, row, "iteration");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to lookup value for iteration");
        return false;
    }

    // optional
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_BOOL(accept, config->args, "-accept", false);

    return detRunSummaryRowAlloc(
            det_id,
            iteration,
            "full",
            bg,
            bg_stdev,
            bg_mean_stdev,
            accept,
            fault
        );
}

// XXX data_state should be set to 'full' here
bool adddetrunsummaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // build a query to search by det_id, iteration, exp_id
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");

    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false); // required
    PXOPT_LOOKUP_BOOL(again, config->args, "-again", false);

    // The values supplied as arguments on the command (eg, -bg) are parsed
    // by mdToDetRunSummary below.
    // XXX why is there ever more than one?

    psString query = pxDataGet("dettool_find_completed_runs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // start a transaction so it's all rows or nothing
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // XXX why is there ever more than one result here?
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];
        // convert metadata into a detResidExp object
        detRunSummaryRow *runSummary = mdToDetRunSummary(config, row);
        if (!runSummary) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata to detResidExp");
            psFree(output);
            return false;
        }
        // insert detResidExp object into the database
        if (!detRunSummaryInsertObject(config->dbh, runSummary)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(runSummary);
            psFree(output);
            return false;
        }
        psFree(runSummary);
    }

    psFree(output);

    // XXX this logic does not deal with the case of -fault being set
    // XXX it should be an error for -again and -fault to both be set
    if (again) {
        if (!startNewIteration(config, det_id)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to start new iteration");
            return false;
        }
    } else {
        // set detRun.state to stop
        if (!setDetRunState(config, det_id, "stop")) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to set detRun.state");
            return false;
        }
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    return true;
}

bool detrunsummaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    psString query = pxDataGet("dettool_detrunsummary.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "NULL");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "AND detRunSummary.fault != 0");
    } else {
        // don't list faulted rows
        psStringAppend(&query, " %s", "AND detRunSummary.fault = 0");
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
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "rawDetrendImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


bool revertdetrunsummaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",      "fault", "==");
    
    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all-run")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("dettool_revertdetrunsummary.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detRunSummary");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

bool pendingcleanup_detrunsummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");
  PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");

  psString query = pxDataGet("dettool_pendingcleanup_detrunsummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }

  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, "detRunSummary");
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
  }
  psFree(where);

  // treat limit == 0 as "no limit"
  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s", limitString);
    psFree(limitString);
  }
  //  fprintf(stderr,">>>%s<<<\n",query);
  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    psTrace("dettool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return(true);
  }

  // negative simple so the default is true
  if (!ippdbPrintMetadatas(stdout, output, "pendingCleanupDetRun", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return(false);
  }
  psFree(output);

  return(true);
}


// preliminary code now.
bool updatedetrunsummaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false); // required
    PXOPT_LOOKUP_BOOL(accept, config->args, "-accept", false);
    PXOPT_LOOKUP_BOOL(reject, config->args, "-reject", false);
    PXOPT_LOOKUP_STR(data_state, config->args, "-data_state", ((accept == 0)&&(reject == 0)), false);
    PXOPT_LOOKUP_S32(iteration, config->args, "-iteration", ((accept == 0)&&(reject == 0)), false);


    if (accept && reject) {
        psError(PS_ERR_UNKNOWN, true, "-accept and -reject are exclusive");
        return false;
    }

    if (!(accept || reject || (data_state != NULL))) {
        psError(PS_ERR_UNKNOWN, true, "either -accept or -reject is required if -data_state is not supplied");
        return false;
    }

    if (accept || reject) {
      char *query = "UPDATE detRunSummary SET accept = %d WHERE det_id = %"PRId64;
      if (!p_psDBRunQueryF(config->dbh, query, accept, det_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
      }
    }
    else {
      PS_ASSERT_PTR_NON_NULL(data_state,false);

/*       if (!isValidDataState(data_state)) return false; */



      char *query_detRunSummary = "UPDATE detRunSummary SET data_state = '%s'"
        " WHERE det_id = %" PRId64
        " AND iteration = %" PRId32;
      if (!p_psDBRunQueryF(config->dbh, query_detRunSummary, data_state, det_id,iteration)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
        return false;
      }

      /* This check allows the one update to flag everything for cleanup.  The check for full is only temporary while I test for bugs. */
      if ((!strncmp(data_state,"goto_",5)
           //      || (!strcmp(data_state,"full"))
           //      || (!strcmp(data_state,"cleaned"))
           ))   {
        char *query_detProcessedImfile = "UPDATE detProcessedImfile SET data_state = '%s'"
          " WHERE det_id = %" PRId64;
        if (!p_psDBRunQueryF(config->dbh, query_detProcessedImfile,data_state,det_id)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detProcessedImfile det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detProcessedExp = "UPDATE detProcessedExp SET data_state = '%s'"
          " WHERE det_id = %" PRId64;
        if (!p_psDBRunQueryF(config->dbh, query_detProcessedExp,data_state,det_id)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detProcessedExp det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detNormalizedImfile = "UPDATE detNormalizedImfile SET data_state = '%s'"
          " WHERE det_id = %" PRId64
          " AND iteration = %" PRId32;
        if (!p_psDBRunQueryF(config->dbh, query_detNormalizedImfile,data_state,det_id,iteration)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detNormalizedImfile det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detNormalizedStatImfile = "UPDATE detNormalizedStatImfile SET data_state = '%s'"
          " WHERE det_id = %" PRId64
          " AND iteration = %" PRId32;
        if (!p_psDBRunQueryF(config->dbh, query_detNormalizedStatImfile,data_state,det_id,iteration)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detNormalizedStatImfile det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detNormalizedExp = "UPDATE detNormalizedExp SET data_state = '%s'"
          " WHERE det_id = %" PRId64
          " AND iteration = %" PRId32;
        if (!p_psDBRunQueryF(config->dbh, query_detNormalizedExp,data_state,det_id,iteration)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detNormalizedExp det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detResidImfile = "UPDATE detResidImfile SET data_state = '%s'"
          " WHERE det_id = %" PRId64
          " AND iteration = %" PRId32;
        if (!p_psDBRunQueryF(config->dbh, query_detResidImfile,data_state,det_id,iteration)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detResidImfile det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detResidExp = "UPDATE detResidExp SET data_state = '%s'"
          " WHERE det_id = %" PRId64
          " AND iteration = %" PRId32;
        if (!p_psDBRunQueryF(config->dbh, query_detResidExp,data_state,det_id,iteration)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detResidExp det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }

        char *query_detStackedImfile = "UPDATE detStackedImfile SET data_state = '%s'"
          " WHERE det_id = %" PRId64
          " AND iteration = %" PRId32;
        if (!p_psDBRunQueryF(config->dbh, query_detStackedImfile,data_state,det_id,iteration)) {
          psError(PS_ERR_UNKNOWN, false,
                  "failed to change state for detStackedImfile det_id %" PRId64 ", iteration %" PRId32, det_id,iteration);
          return(false);
        }
      }
      /* End if */

    }

    return true;
}



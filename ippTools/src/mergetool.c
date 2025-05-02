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
#include "pxmerge.h"
#include "mergetool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool pendingmergeMode(pxConfig *config);
static bool addmergedMode(pxConfig *config);
static bool listmergedMode(pxConfig *config);
static bool revertmergedMode(pxConfig *config);
static bool updatemergedMode(pxConfig *config);
static bool definebyquerymergecopyMode(pxConfig *config);
static bool listmergedvodbcopyMode(pxConfig *config);
static bool revertmergedvodbcopyMode(pxConfig *config);
static bool updatemergedvodbcopyMode(pxConfig *config);



# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = mergetoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(MERGETOOL_MODE_DEFINEBYQUERY,        definebyqueryMode);
	MODECASE(MERGETOOL_MODE_UPDATERUN, updaterunMode);
        MODECASE(MERGETOOL_MODE_PENDINGMERGE, pendingmergeMode);
        MODECASE(MERGETOOL_MODE_ADDMERGED, addmergedMode);
        MODECASE(MERGETOOL_MODE_LISTMERGED, listmergedMode);
        MODECASE(MERGETOOL_MODE_REVERTMERGED, revertmergedMode);
        MODECASE(MERGETOOL_MODE_UPDATEMERGED, updatemergedMode);
	MODECASE(MERGETOOL_MODE_DEFINEBYQUERYMERGECOPY,        definebyquerymergecopyMode);
        MODECASE(MERGETOOL_MODE_LISTMERGEDVODBCOPY, listmergedvodbcopyMode);
        MODECASE(MERGETOOL_MODE_REVERTMERGEDVODBCOPY, revertmergedvodbcopyMode);
        MODECASE(MERGETOOL_MODE_UPDATEMERGEDVODBCOPY, updatemergedvodbcopyMode);



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
    PXOPT_COPY_S64(config->args, where, "-minidvodb_id", "minidvodbRun.minidvodb_id", "==");
    PXOPT_COPY_STR(config->args, where, "-minidvodb_group", "minidvodbRun.minidvodb_group", "==");
    //   PXOPT_COPY_STR(config->args, where, "-mergedvodb", "mergedvodbRun.mergedvodb", "==");



    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_STR(mergedvodb_path, config->args, "-set_mergedvodb_path", false, false);
    PXOPT_LOOKUP_STR(mergedvodb, config->args, "-mergedvodb", true, false);

    PXOPT_LOOKUP_BOOL(pretend,    config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple,     config->args, "-simple", false);

    //find the items to queue (uniq minidvodb_id)
 psString dvodb_string = NULL;
    psString bare_query = NULL;

bare_query = pxDataGet("mergetool_find_minidvodb_id.sql");
 psStringAppend(&dvodb_string, "mergedvodbRun.mergedvodb = '%s'", mergedvodb);

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


    psStringAppend(&query, " GROUP BY minidvodb_id");


    psTrace("mergetool.c", PS_LOG_INFO,"query: \n\n%s\n\n",query);

if (!p_psDBRunQuery(config->dbh, query)) {
      psError(PS_ERR_UNKNOWN, false, "database error, \n%s\n", query);
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
        if (!ippdbPrintMetadatas(stdout, output, "addRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        minidvodbRunRow *row = minidvodbRunObjectFromMetadata(md);
        
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into minidvodbRun");
            psFree(output);
            return false;
        }

        if (!mergedvodb) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue merge run without a defined mergedvodb:  minidvodb_id %" PRId64,row->minidvodb_id);
            psFree(output);
            return false;
        }
        if (!mergedvodb_path) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue merge run without a defined mergedvodb_path: minidvodb_id %" PRId64,row->minidvodb_id);
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
for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
        psS64 minidvodb_id =0; 
        
                  minidvodbRunRow *row = minidvodbRunObjectFromMetadata(md);
          minidvodb_id = row->minidvodb_id;
        
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into minidvodbRun");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxmergeQueueByMinidvodbID(config,
				       minidvodb_id,
				       mergedvodb,
				       mergedvodb_path
                              
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error sfg");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue mergedvodb %" PRId64, minidvodb_id);
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

static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-merge_id",    "mergedvodbRun.merge_id", "==");
    PXOPT_COPY_STR(config->args, where, "-mergedvodb", "mergedvodbRun.mergedvodb", "==");
    
    if (!psListLength(where->list)) {
	  psFree(where);
	  psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
	  return false;
	}
	
	PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
	PXOPT_LOOKUP_STR(mergedvodb_path, config->args, "-set_mergedvodb_path", false, false);
	psString query = NULL;

	query = psStringCopy("UPDATE mergedvodbRun SET");
	
	int cnt = 0;
	psString comma = ",";
	if (state) {
	  psStringAppend(&query, " state = '%s'", state);
	  cnt++;
	}

	if (mergedvodb_path) {
	  if (cnt) {
	    psStringAppend(&query, "%s", comma);
	  }
	  psStringAppend(&query, " mergedvodb_path = '%s'", mergedvodb_path);
	  cnt++;
	}


	psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
	psStringAppend(&query, " WHERE %s", whereClause);

	if (!p_psDBRunQuery(config->dbh, query)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	  psFree(query);
	  return false;
	}

	psFree(query);
	psFree(where);
	
  
	return true;
}

static bool pendingmergeMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-minidvodb_id",    "mergedvodbRun.minidvodb_id", "==");
    PXOPT_COPY_S64(config->args, where, "-merge_id",    "mergedvodbRun.merge_id", "==");
    PXOPT_COPY_STR(config->args, where, "-mergedvodb",    "mergedvodbRun.mergedvodb", "==");



    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
   
    psString query = NULL;
    query = pxDataGet("mergetool_find_pending_merge.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
      return false;
    }

    // use psDBGenerateWhereSQL because the SQL yields an intermediate table
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
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

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "mergedvodbPendingMerge", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);
    
    return true;
}

static bool addmergedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

 PXOPT_LOOKUP_U64(merge_id, config->args, "-merge_id", true, false);
 PXOPT_LOOKUP_STR(mergedvodb, config->args, "-mergedvodb", true, false);

   // optional

//PXOPT_LOOKUP_U64(merge_order,     config->args, "-merge_order", false, false);
 PXOPT_LOOKUP_F32(dtime_merge, config->args, "-dtime_merge", false, false);
 PXOPT_LOOKUP_F32(dtime_verify, config->args, "-dtime_verify", false, false);
 PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
  
 PXOPT_LOOKUP_TIME(epoch, config->args, "-epoch", false, false);
 PXOPT_LOOKUP_S16(fault,         config->args, "-fault", false, false);

 PXOPT_COPY_S64(config->args, where, "-merge_id",   "mergedvodbRun.merge_id",   "==");
 PXOPT_COPY_STR(config->args, where, "-mergedvodb",   "mergedvodbRun.mergedvodb",   "==");



  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
 psString query = pxDataGet("mergetool_find_pendingmergeprocess.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }
  // use psDBGenerateWhereSQL because the SQL yields an intermediate table
  if (psListLength(where->list)) {
    psString whereClaus = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClaus);
    psFree(whereClaus);
  }
  psFree(where);
  psTrace("mergetool.c", PS_LOG_INFO, "query is:\n%s", query);
  
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
    psTrace("mergetool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  mergedvodbRunRow *pendingRow = mergedvodbRunObjectFromMetadata(output->data[0]);
  psFree(output);
  mergedvodbProcessedRow *row = mergedvodbProcessedRowAlloc(
               pendingRow->merge_id,
               0,
	       dtime_verify ? dtime_verify : 0,
               dtime_merge ? dtime_merge : 0,
               dtime_script ? dtime_script : 0,
               epoch,
	       fault
               );

  if (!mergedvodbProcessedInsertObject(config->dbh, row)) {
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(row);
    psFree(pendingRow);
    return false;
  }

//this finds the # of merged things (for the merge order)
  psString query3 = NULL;
  psStringAppend(&query3, "select count(*) from mergedvodbRun join mergedvodbProcessed using (merge_id) where state = 'full' and mergedvodb = '%s';", mergedvodb);

  if (!p_psDBRunQuery(config->dbh, query3)) {
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query3);
    return false;
  }
  psArray *output2 = p_psDBFetchResult(config->dbh);
  if (!output2) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  if (!psArrayLength(output2)) {
    psTrace("mergetool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }
  bool status;
  psS64 m_order = psMetadataLookupS64(&status, output2->data[0], "count(*)");
  if (!status) {

    psAbort("failed to lookup value for count column");
    return false;
  }
  psString final = NULL;
  psStringAppend(&final, "%" PRIu64, m_order);
    //return false;
  psFree(query3);
  psFree(output2);



  //update the merge_order

  psString query4 = NULL;
  psStringAppend(&query4, "update mergedvodbProcessed set merge_order = %"PRIu64,m_order);
  psStringAppend(&query4," where merge_id = %" PRIu64,  merge_id);
  //printf("%s", query4);
  if (!p_psDBRunQuery(config->dbh, query4)) {
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query4);
    return false;
  }
  //
psFree(query4);

// since there is only one exp per 'new' set mergedvodbRun.state = 'full'

  psString query2 = NULL ;

  if (!fault) {
    psStringAppend(&query2, "UPDATE mergedvodbRun SET state = 'full' WHERE merge_id = %'" PRIu64, row->merge_id);
  
    if (!p_psDBRunQuery(config->dbh, query2)) {
      // rollback
      if (!psDBRollback(config->dbh)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(query2);
      return false;
    }
    
  }






  psFree(row);
  psFree(pendingRow);





  //commit the changes
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }




 //print the merge_order (why not!)
  printf("%s", final);
  psFree(final);











    return true;
}


static bool listmergedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-minidvodb_id",    "mergedvodbRun.minidvodb_id",    "==");
    PXOPT_COPY_S64(config->args, where, "-merge_id",    "mergedvodbRun.merge_id",    "==");
    PXOPT_COPY_STR(config->args, where, "-mergedvodb",    "mergedvodbRun.mergedvodb",    "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    psString query = NULL;
    query = pxDataGet("mergetool_find_processed.sql");

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
    if (psListLength(where->list) && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " AND fault != 0");
    }
      if (psListLength(where->list) && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND fault = 0");
    }
	if (!psListLength(where->list) && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE fault != 0");
    }
	if (!psListLength(where->list) && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE fault = 0");
    }

psFree(where);

    // order by merge_id
    psStringAppend(&query, " ORDER BY merge_id");
    

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
        psTrace("addtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

 // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "addProcessedExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);



    return true;
}


static bool revertmergedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-merge_id",    "mergedvodbRun.merge_id",         "==");
    PXOPT_COPY_S16(config->args, where, "-fault",    "mergedvodbRun.fault",         "==");
    PXOPT_COPY_STR(config->args, where, "-mergedvodb",    "mergedvodbRun.mergedvodb",         "==");
    
    if (!psListLength(where->list)) {
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
      psString query = NULL;
      query = pxDataGet("mergetool_revertmergedvodbprocessed.sql");

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
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
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

static bool updatemergedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    psMetadata *where = psMetadataAlloc();
    // PXOPT_LOOKUP_U64(minidvodb_id,  config->args, "-merge_id", true, false);
    PXOPT_COPY_S64(config->args, where, "-merge_id",     "mergedvodbProcessed.merge_id", "==");
    PXOPT_LOOKUP_F32(dtime_merge,  config->args, "-set_dtime_merge", false, false);
    PXOPT_LOOKUP_F32(dtime_verify,  config->args, "-set_dtime_verify", false, false);
    PXOPT_LOOKUP_F32(dtime_script,  config->args, "-set_dtime_script", false, false);
    PXOPT_LOOKUP_S16(fault,  config->args, "-set_fault", false, false);
    
    PXOPT_LOOKUP_STR(state,  config->args, "-set_state", false, false);

 if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }
 psString query = psStringCopy("UPDATE mergedvodbProcessed JOIN mergedvodbRun USING (merge_id) SET ");
int cnt = 0;
  psString comma = ",";
  if (fault) {
    psStringAppend(&query, " fault = %d", fault);
  cnt++;
  }



  if (dtime_verify) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " dtime_verify = %f", dtime_verify);
    cnt++;
  }

  if (dtime_merge) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " dtime_merge = %f", dtime_merge);
    cnt++;
 }

if (dtime_script) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " dtime_script = %f", dtime_script);
    cnt++;
 }
if (state) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " state = '%s'", state);
    cnt++;
 }




  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query, " WHERE %s", whereClause);

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error \n%s\n",query );
   psFree(query);
   return false;
  }

  psFree(query);
  psFree(where);

    return true;
}

static bool definebyquerymergecopyMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-merge_id",    "mergedvodbRun.merge_id", "==");
    pxAddLabelSearchArgs (config, where, "-mergedvodb",     "mergedvodbRun.mergedvodb", "=="); // define using camRun label
   


    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(destination_host,     config->args, "-set_destination_host", false, false);
    PXOPT_LOOKUP_STR(mergedvodb_rsync_path, config->args, "-set_mergedvodb_rsync_path", false, false);
    PXOPT_LOOKUP_BOOL(pretend,    config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple,     config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(last_merged, config->args, "-last_merged", false);
    // prevent queueing an addRun if a given exposure has already been added to
    // the given dvo database
    psString dvodb_string = NULL;
    psString bare_query = NULL;
    if (destination_host) {
      psTrace("mergedvodbtool.c", PS_LOG_INFO, "destination_host argument found (%s) using mergedvodbtool_find_merge_id_dvo.sql\n", destination_host);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("mergedvodbtool_find_merge_id_dvo.sql");
        // user supplied dvodb
        psStringAppend(&dvodb_string, "mergedvodbCopy.destination_host = '%s'", destination_host);
    } else {
      psError(PS_ERR_UNKNOWN, false, "cannot queue mergedvodbcopy run without a defined destination_host");
       
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
    if (last_merged) {
      psStringAppend(&query, " ORDER BY merge_order DESC LIMIT 1");
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
        if (!ippdbPrintMetadatas(stdout, output, "mergedvodbCopy", !simple)) {
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

        mergedvodbRunRow *row = mergedvodbRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into mergedvodbRun");
            psFree(output);
            return false;
        }

        if (!mergedvodb_rsync_path) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue mergedvodbcopy run without a defined mergedvodb_rsync_path: merge_id %" PRId64, row->merge_id);
            psFree(output);
            return false;
        }
        if (!destination_host) {
            psError(PS_ERR_UNKNOWN, false, "cannot queue mergedvodbcopy run without a defined destination_host: merge_id %" PRId64, row->merge_id);
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

  for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        mergedvodbRunRow *row = mergedvodbRunObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
            psFree(output);
            return false;
        }

        // queue the exp
        if (!pxaddQueueByMergeID(config,
                               row->merge_id,
                                     destination_host ? destination_host : "NULL",
                                     mergedvodb_rsync_path ?mergedvodb_rsync_path : "NULL"
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error sfg");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue merge_id: %" PRId64, row->merge_id);
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


static bool listmergedvodbcopyMode(pxConfig *config) {
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-merge_id", "mergedvodbCopy.merge_id", "==");
  PXOPT_COPY_STR(config->args, where, "-mergedvodbcopy_id", "mergedvodbCopy.mergedvodbcopy_id", "==");
  PXOPT_COPY_STR(config->args, where, "-destination_host", "mergedvodbCopy.destination_host", "==");
  PXOPT_COPY_STR(config->args, where, "-mergedvodb", "mergedvodbRun.mergedvodb", "==");
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_BOOL(pending, config->args, "-pending", false);
  PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
  if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

  psString query = pxDataGet("mergedvodbtool_find_mergedvodbcopy.sql");
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
        psStringAppend(&query, " %s", " AND mergedvodbCopy.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND mergedvodbCopy.fault = 0");
    }
    if (!where->list && faulted) {

       // list only faulted rows
        psStringAppend(&query, " %s", " AND mergedvodbCopy.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND mergedvodbCopy.fault = 0");
    }
    if (!where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE mergedvodbCopy.fault != 0");
    }
    if (!where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE mergedvodbCopy.fault = 0");
    }
    psFree(where);

    if (pending) {
       //add the cuts for pending (state new, no faults)
      psStringAppend(&query, " %s", " AND mergedvodbCopy.state = 'new' AND mergedvodbCopy.fault = 0 AND mergedvodbCopy.destination_host IS NOT NULL AND mergedvodbCopy.mergedvodb_rsync_path IS NOT NULL");
    }


    // order by epoch
    psStringAppend(&query, " ORDER BY mergedvodbcopy_id");

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
    if (!ippdbPrintMetadatas(stdout, output, "mergedvodbCopy", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

return true;
}

static bool revertmergedvodbcopyMode(pxConfig *config) {
  psMetadata *where = psMetadataAlloc();
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_COPY_S64(config->args, where, "-mergedvodbcopy_id", "mergedvodbCopy.mergedvodbcopy_id", "==");
  PXOPT_COPY_S64(config->args, where, "-merge_id", "mergedvodbCopy.merge_id", "==");
  PXOPT_COPY_STR(config->args, where, "-destination_host", "destination_host", "==");
  PXOPT_COPY_S16(config->args, where, "-fault", "mergedvodbCopy.fault", "==");

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
    psString query = pxDataGet("mergedvodbtool_revertmergedvodbcopy.sql");
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

static bool updatemergedvodbcopyMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);
  psMetadata *where = psMetadataAlloc();

  // require? PXOPT_LOOKUP_U64(mergedvodbcopy_id,  config->args, "-mergedvodbcopy_id", false, false);
  // require? PXOPT_LOOKUP_U64(merge_id,  config->args, "-merge_id", false, false);
  PXOPT_LOOKUP_STR(state,  config->args, "-state", false, false);
  PXOPT_LOOKUP_STR(host,  config->args, "-host", false, false);
  PXOPT_LOOKUP_STR(rsync_path,  config->args, "-mergedvodb_rsync_path", false, false);
  // PXOPT_LOOKUP_S16(fault,  config->args, "-fault", false, false);
  
  PXOPT_LOOKUP_S16(set_fault,  config->args, "-set_fault", false, false);
  PXOPT_LOOKUP_STR(set_rsync_path,  config->args, "-set_mergedvodb_rsync_path", false, false);
  PXOPT_LOOKUP_STR(set_host,  config->args, "-set_destination_host", false, false);
  PXOPT_LOOKUP_STR(set_state,  config->args, "-set_state", false, false);
  PXOPT_LOOKUP_F32(dtime,  config->args, "-set_dtime", false, false);
  PXOPT_COPY_S64(config->args, where, "-mergedvodbcopy_id",     "mergedvodbCopy.mergedvodbcopy_id", "==");
  PXOPT_COPY_S64(config->args, where, "-merge_id",     "mergedvodbCopy.merge_id", "==");
  
  PXOPT_COPY_STR(config->args, where, "-state",     "mergedvodbCopy.state", "==");
  PXOPT_COPY_STR(config->args, where, "-host",     "mergedvodbCopy.destination_host", "==");
  PXOPT_COPY_STR(config->args, where, "-mergedvodb_rsync_path",     "mergedvodbCopy.mergedvodb_rsync_path", "==");


  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  psString query = psStringCopy("UPDATE mergedvodbCopy JOIN mergedvodbRun USING (merge_id) SET ");
  int cnt = 0;
  psString comma = ",";
  if (set_fault) {
    psStringAppend(&query, " mergedvodbCopy.fault = %d", set_fault);
  cnt++;
  }
  if (set_rsync_path) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }

    psStringAppend(&query, " mergedvodbCopy.mergedvodb_rsync_path = '%s'" , set_rsync_path);
    cnt++;
  }

  if (set_host) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }

    psStringAppend(&query, " mergedvodbCopy.destination_host = '%s'" , set_host);
    cnt++;
  }


  if (set_state) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }

    psStringAppend(&query, " mergedvodbCopy.state = '%s'" , set_state);
    cnt++;
  }

  if (dtime) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " mergedvodbCopy.dtime = %f", dtime);
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




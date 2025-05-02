#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "pxtools.h"
#include "pxdqstats.h"
#include "dqstatstool.h"

static bool definebyqueryMode(pxConfig *config);
static bool createbundleMode(pxConfig *config);
//static bool cleanbundleMode(pxConfig *config);
static bool pendingbundleMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool revertrunMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv) {
  psLibInit(NULL);

  pxConfig *config = dqstatstoolConfig(NULL, argc, argv);
  if (!config) {
    psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
    goto FAIL;
  }

  switch (config->mode) {
    MODECASE(DQSTATS_MODE_DEFINEBYQUERY, definebyqueryMode);
    MODECASE(DQSTATS_MODE_PENDINGBUNDLE, pendingbundleMode);
    MODECASE(DQSTATS_MODE_CREATEBUNDLE, createbundleMode);
    //    MODECASE(DQSTATS_MODE_CLEANBUNDLE, cleanbundleMode);
    MODECASE(DQSTATS_MODE_UPDATERUN, updaterunMode);
    MODECASE(DQSTATS_MODE_REVERTRUN, revertrunMode);
  default:
    psAbort("invalid option (this should not happen)");
  }

  //psFree(config);
  pmConfigDone();
  psLibFinalize();
  exit(EXIT_SUCCESS);

 FAIL:
  psErrorStackPrint(stderr, "\n");
  int exit_status = pxerrorGetExitStatus();

  //    psFree(config);
  pmConfigDone();
  psLibFinalize();
  exit(exit_status);
}

static bool definebyqueryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config,NULL);

  // optional
  PXOPT_LOOKUP_STR(label,       config->args, "-label", false, false);
  PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);
  PXOPT_LOOKUP_STR(set_label,   config->args, "-set_label", false, false);
  PXOPT_LOOKUP_BOOL(pretend,    config->args, "-pretend", false);
  PXOPT_LOOKUP_BOOL(simple,     config->args, "-simple", false);
  PXOPT_LOOKUP_BOOL(force,      config->args, "-force", false);
  PXOPT_LOOKUP_U64(limit,       config->args, "-limit", false, 0);

  // properties of the raw exposures
  psMetadata *where = psMetadataAlloc();

  pxdqstatsGetSearchArgs(config, where);

  psString query = pxDataGet("dqstatstool_definebyquery.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    psFree(where);
    return(false);
  }

  if (label) {
    pxAddLabelSearchArgs (config, where, "-label", "camRun.label", "LIKE"); // define using cam label
  }
  // use psDBGenerateWhereConditionSQL because the SQL ends in a WHERE
  if (force) {
    psStringAppend(&query, " AND (invalid IS NULL OR invalid = 1 OR invalid = 0) ");
  }
  else {
    psStringAppend(&query, " AND ((warpRun.warp_id IS NOT NULL OR camProcessedExp.quality > 0) AND invalid IS NULL) ");
  }

  if (where && psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
  }

  psFree(where);
  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s", limitString);
    psFree(limitString);
  }
  psTrace("debug",3,">> %s",query);
  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }
  //  printf("%s\n",query);
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("dqstatstool", PS_LOG_INFO, "no rows found");
    psWarning("dqstatstool: no rows found");
    psFree(output);
    return true;
  }

  if (pretend) {
    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "dqstatsContent", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
    psFree(output);
    return true;
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(where);
    return false;
  }

  // Create the run entry and grab the dqstats_id
  dqstatsRunRow *run = dqstatsRunRowAlloc(
                                          0,        // ID
                                          "new",    // state
                                          registered,
                                          set_label,
                                          0         // fault
                                          );

  if (!dqstatsRunInsertObject(config->dbh, run)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(output);
    psFree(run);
    return(false);
  }

  psS64 dqstats_id = psDBLastInsertID(config->dbh);

  psArray *list = psArrayAllocEmpty(16); // List of runs to print
  psArrayAdd(list, list->n, run);
  if (!dqstatsRunPrintObjects(stdout, list, !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print object");
    psFree(list);
    return(false);
  }
  psFree(list);

  // Create the content entry.
  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i]; // Row from select
    bool status;

    psS64 exp_id = psMetadataLookupS64(&status, row, "exp_id");
    psS64 chip_id= psMetadataLookupS64(&status, row, "chip_id");
    psS64 cam_id = psMetadataLookupS64(&status, row, "cam_id");
    psS64 warp_id= psMetadataLookupS64(&status, row, "warp_id");

    if (!dqstatsContentInsert(config->dbh,
                              dqstats_id,
                              exp_id,
                              chip_id,
                              cam_id,
                              warp_id,
                              0 // do not create automatically invalid contents
                              )) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(output);
      if (!psDBRollback(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
      }
      return(false);
    }
  }

  psFree(output);

  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }

  return(true);
}

static bool pendingbundleMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config,false);

  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);


  psMetadata *where = psMetadataAlloc();
  //  pxdqstatsGetSearchArgs (config, where);
  PXOPT_COPY_S64(config->args, where, "-dqstats_id", "dqstatsRun.dqstats_id", "==");
  PXOPT_COPY_STR(config->args, where, "-label", "dqstatsRun.label", "==");
  PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

  psString query = pxDataGet("dqstatstool_get_run.sql");           // query
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "Failed to retrieve sql statement (dqstatstool_get_run.sql)");
    return(false);
  }

  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
  } else {
    if (!all) {
      psError(PXTOOLS_ERR_SYS, false, "unrestricted query not allowed (try -all)");
      return false;
    }
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
    return(false);
  }
  psFree(query);                                                  // where whereClause

  psArray *output = p_psDBFetchResult(config->dbh);               // where whereClause output
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    psTrace("dqstatstool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  // negative simple so the default is true
  if (!ippdbPrintMetadatas(stdout, output, "dqstatsPendingBundle", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return false;
  }

  psFree(output);

  return(true);
}
//static bool cleanbundleMode(pxConfig *config) {
//  return(true);
//}


static bool createbundleMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config,false);

  bool status = true;
  PXOPT_LOOKUP_S64(dqstats_id,       config->args, "-dqstats_id", true, 0);
  PXOPT_LOOKUP_STR(bundleUri,        config->args, "-uri",        true, 0);

  // Determine the information about this run.
  psString runQuery = pxDataGet("dqstatstool_get_run.sql");
  if (!runQuery) {
    psError(PXTOOLS_ERR_SYS, false, "Failed to retrieve sql statement (dqstatstool_get_run.sql)");
    return(false);
  }

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-dqstats_id",      "dqstats_id", "==");
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);  // Save for content query
  psStringAppend(&runQuery, " AND %s", whereClause);
  psFree(where);

  if (!p_psDBRunQuery(config->dbh, runQuery)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(runQuery);
    psFree(whereClause);
    return(false);
  }
  psFree(runQuery);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(whereClause);
    return(false);
  }

  if (psArrayLength(output) != 1) {
    psError(PS_ERR_UNKNOWN, false, "dqstatstool: single return row not found.");
    psFree(output);
    psFree(whereClause);
    return(false);
  }

  psMetadata *run = output->data[0];
  if (strcmp(psMetadataLookupStr(&status,run,"state"),"new") != 0) {
    psError(PS_ERR_UNKNOWN, false, "dqstatstool: bundle already created for this run (should not happen).");
    psFree(output);
    psFree(whereClause);
    return(false);
  }
  psFree(output);   // This held the information about the Run.

  // Read column list from config file
  psMetadata *recipe = psMetadataLookupPtr(&status, config->modules->recipes, "DQSTATS");
  psMetadata *columns = psMetadataLookupMetadata(&status,recipe,"STATS.COLUMNS");
  if (!columns) {
    psError(PS_ERR_UNKNOWN, false, "Could not read STATS.COLUMN metadata");
    psFree(whereClause);
    return(false);
  }

  psMetadataIterator *iter = psMetadataIteratorAlloc(columns, PS_LIST_HEAD,NULL);

  psMetadataItem *colItem = NULL;
  psString columnlist = NULL;

  psMetadata *colNames = psMetadataAlloc();
  psMetadata *colTypes = psMetadataAlloc();
  while ((colItem = psMetadataGetAndIncrement(iter))) {
    if (strcmp(colItem->name, "COLUMN")) continue;
    if (colItem->type != PS_DATA_METADATA) {
      psError(PS_ERR_UNKNOWN, false, "dqstatstool COLUMN not of type METADATA\n");
      return(NULL);
    }
    
    bool status;
    psMetadata *coldef = colItem->data.md;

    if (columnlist) { // Stick intervening commas
      psStringAppend(&columnlist, ", ");
    }

    psStringAppend(&columnlist, "%s.%s",
                   psMetadataLookupStr(&status,coldef,"TABLE"),
                   psMetadataLookupStr(&status,coldef,"ENTRY"));

    psMetadataAddStr(colNames,PS_LIST_TAIL,
                     psMetadataLookupStr(&status,coldef,"ENTRY"),0,"",
                     psMetadataLookupStr(&status,coldef,"COLNAME"));
    psMetadataAddStr(colTypes,PS_LIST_TAIL,
                     psMetadataLookupStr(&status,coldef,"ENTRY"),0,"",
                     psMetadataLookupStr(&status,coldef,"DATATYPE"));
  }
  psFree(iter);

  if (!columnlist) {
    psError(PS_ERR_UNKNOWN, false, "Failed to find a columnlist");
    psFree(whereClause);
    psFree(colNames);
    psFree(colTypes);
    return(false);
  }

  // Read decision engine settings from config file.
  psMetadata *engine = psMetadataLookupMetadata(&status,recipe,"DECISION.ENGINE");
  if (!engine) {
    psError(PS_ERR_UNKNOWN, false, "Could not read DECISION.ENGINE metadata");
    psFree(whereClause);
    return(false);
  }
  psArray *decisionRules = psArrayAllocEmpty(4); // list of screened and verified rules.
  psMetadataIterator *iterEngine = psMetadataIteratorAlloc(engine, PS_LIST_HEAD,NULL);
  psMetadataItem *engineItem = NULL;

  while ((engineItem = psMetadataGetAndIncrement(iterEngine))) {
    if (strcmp(engineItem->name, "RULE")) continue;
    if (engineItem->type != PS_DATA_METADATA) {
      psError(PS_ERR_UNKNOWN, false, "dqstatstool RULE not of type METADATA\n");
      return(NULL);
    }
    
    bool status;
    psMetadata *enginedef = engineItem->data.md;

    if (psMetadataLookupStr(&status,enginedef,"COLNAME")&&
	(strcmp(psMetadataLookupStr(&status,enginedef,"RULETYPE"),"STRICT") == 0)&&
	(isfinite(psMetadataLookupF32(&status,enginedef,"MINIMUM"))&&
	 isfinite(psMetadataLookupF32(&status,enginedef,"MAXIMUM")))) {
      psArrayAdd(decisionRules,decisionRules->n,enginedef);
    }
    else if (psMetadataLookupStr(&status,enginedef,"COLNAME")&&
	     (strcmp(psMetadataLookupStr(&status,enginedef,"RULETYPE"),"CDF") == 0)&&
	     (isfinite(psMetadataLookupF32(&status,enginedef,"CDF00"))&&
	      isfinite(psMetadataLookupF32(&status,enginedef,"CDF50"))&&
	      isfinite(psMetadataLookupF32(&status,enginedef,"CDF100")))) {
      psArrayAdd(decisionRules,decisionRules->n, enginedef);
    }
    else if (strcmp(psMetadataLookupStr(&status,enginedef,"RULETYPE"),"UNUSED") == 0) {
    }
    else {
      psError(PS_ERR_UNKNOWN, false, "Rule invalid.");
    }
  }
  psFree(iterEngine);

  if (decisionRules->n < 1) {
    psError(PS_ERR_UNKNOWN, false, "Failed to generate a decision engine");
    psFree(decisionRules);
    psFree(engine);
    return(false);
  }
  psFree(engine);

  // Find the contents that comprise this run
  psString contentQuery = pxDataGet("dqstatstool_get_contents.sql");
  if (!contentQuery) {
    psError(PXTOOLS_ERR_SYS, false, "Failed to retrieve sql statement (dqstatstool_get_contents.sql)");
    psFree(whereClause);
    psFree(colNames);
    psFree(colTypes);
    psFree(columnlist);
    return(false);
  }

  psStringAppend(&contentQuery, " AND %s", whereClause);
  psFree(whereClause);

  if (!p_psDBRunQuery(config->dbh, contentQuery)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(colNames);
    psFree(colTypes);
    psFree(columnlist);
    psFree(contentQuery);
    return(false);
  }
  psFree(contentQuery);

  psArray *contents = p_psDBFetchResult(config->dbh);
  if (!contents) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(colNames);
    psFree(colTypes);
    psFree(columnlist);
    return(false);
  }
  if (!psArrayLength(contents)) {
    psWarning("dqstatstool No rows found");
    psFree(colNames);
    psFree(colTypes);
    psFree(columnlist);
    psFree(contents);
    return(false);
  }

  // Grab data from the database for a row of the bundle.
  psString query = pxDataGet("dqstatstool_createbundle.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "Failed to retrieve sql statement");
    psFree(colNames);
    psFree(colTypes);
    psFree(columnlist);
    psFree(contents);
    return(false);
  }
  // From disttool.c:
  // "psStringSubstitute fails unless the input is a psString which it determines by
  //  comparing the memory blocks free function to an expected value.
  //  pxDataGet uses psSlurp which leaves a different free function on the memory block.
  //  To work around this make a copy of the query before doing the substitution."
  psString queryCopy = psStringCopy(query);
  psFree(query);
  query = queryCopy;
  psStringSubstitute(&query,columnlist, "@COLLIST@");
  psFree(columnlist);

  psArray *outTable = psArrayAllocEmpty(contents->n);
  for (long i = 0; i < contents->n; i++) {   // Loop over content entries.
    psMetadata *row = contents->data[i];

    // Limit it just to the stuff we care about for this row.
    bool status;
    psString MYquery = psStringCopy(query);
    if (!MYquery) {
      psTrace("debug",4,"Failed to copy, trying again.");
      MYquery = psStringCopy(query);
    }

    psMetadata *where = psMetadataAlloc();

    psMetadataAddS64(where,PS_LIST_TAIL, "rawExp.exp_id", 0, "==", psMetadataLookupS64(&status,row,"exp_id"));
    psMetadataAddS64(where,PS_LIST_TAIL, "chipRun.chip_id", 0, "==", psMetadataLookupS64(&status,row,"chip_id"));
    psMetadataAddS64(where,PS_LIST_TAIL, "camRun.cam_id", 0, "==", psMetadataLookupS64(&status,row,"cam_id"));
    // This may not be real, so we need to check first. Is there a better way to check for a null here?
    if (psMetadataLookupS64(&status,row,"warp_id") && (psMetadataLookupS64(&status,row,"warp_id") != PS_MAX_S64)) {
      psMetadataAddS64(where,PS_LIST_TAIL, "warpRun.warp_id", 0, "==", psMetadataLookupS64(&status,row,"warp_id"));
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    if (!whereClause) {
      psError(PS_ERR_UNKNOWN, false, "Unable to generate Where Condition");
      return(false);
    }
    psStringAppend(&MYquery, " WHERE %s", whereClause);

/*     psTrace("debug",2,"%ld %ld %ld %ld %s\n",psMetadataLookupS64(&status,row,"exp_id"), */
/* 	    psMetadataLookupS64(&status,row,"chip_id"), */
/* 	    psMetadataLookupS64(&status,row,"cam_id"), */
/* 	    psMetadataLookupS64(&status,row,"warp_id"), */
/* 	    whereClause); */
	    
/*      psFree(whereClause); */
/*     psFree(where); */
    //    printf("%s\n",MYquery);

    if (!p_psDBRunQuery(config->dbh, MYquery)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(MYquery);
      psFree(query);
      psFree(colNames);
      psFree(colTypes);
      psFree(contents);
      psFree(outTable);
      return(false);
    }

    psArray *statsoutput = p_psDBFetchResult(config->dbh);
    if (!statsoutput) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(colNames);
      psFree(colTypes);
      psFree(contents);
      psFree(query);
      psFree(outTable);
      return(false);
    }
    if (!psArrayLength(statsoutput)) {
      psWarning("dqstatstool No stats rows found: %ld %s %s",i,MYquery,query);
      psFree(colNames);
      psFree(colTypes);
      psFree(contents);
      psFree(query);
      psFree(outTable);
      psFree(statsoutput);
      return(false);
    }
    psFree(MYquery);
    psFree(whereClause);
    psFree(where);

    // Magic happens and the fits table is filled.
    psMetadata *statsrow = statsoutput->data[0]; // Because there should only be one row.

    psMetadataIterator *statIter = psMetadataIteratorAlloc(statsrow,PS_LIST_HEAD,NULL);

    psMetadataItem *rowItem = NULL;
    psMetadata *tableRow = psMetadataAlloc();
    // set args -dbname gpc1 -createbundle -dqstats_id 2 -uri /tmp/czwtest.fits
    while ((rowItem = psMetadataGetAndIncrement(statIter))) {
      bool success = false;

      psString colType = psMetadataLookupStr(&status,colTypes,rowItem->name);
      if (!colType) {
        psError(PS_ERR_UNKNOWN,false, "Could not find type for %s",rowItem->name);
        return(false);
      }
      psString colName = psMetadataLookupStr(&status,colNames,rowItem->name);
      if (!colName) {
        psError(PS_ERR_UNKNOWN,false, "Could not find name for %s",rowItem->name);
        return(false);
      }

      if (!strcmp(colType,"PS_DATA_BOOL")) {
        success = psMetadataAddBool(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.B);
      }
      else if (!strcmp(colType,"PS_DATA_S8")) {
        success = psMetadataAddS8(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.S8);
      }
      else if (!strcmp(colType,"PS_DATA_S16")) {
        success = psMetadataAddS16(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.S16);
      }
      else if (!strcmp(colType,"PS_DATA_S32")) {
        success = psMetadataAddS32(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.S32);
      }
      else if (!strcmp(colType,"PS_DATA_S64")) {
        success = psMetadataAddS64(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.S64);
      }
      else if (!strcmp(colType,"PS_DATA_U8")) {
        success = psMetadataAddU8(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.U8);
      }
      else if (!strcmp(colType,"PS_DATA_U16")) {
        success = psMetadataAddU16(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.U16);
      }
      else if (!strcmp(colType,"PS_DATA_U32")) {
        success = psMetadataAddU32(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.U32);
      }
      else if (!strcmp(colType,"PS_DATA_U64")) {
        success = psMetadataAddU64(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.U64);
      }
      else if (!strcmp(colType,"PS_DATA_F32")) {
        success = psMetadataAddF32(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.F32);
      }
      else if (!strcmp(colType,"PS_DATA_F64")) {
        success = psMetadataAddF64(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.F64);
      }
      else if (!strcmp(colType,"PS_DATA_STRING")) {
        if (rowItem->data.str == NULL) {
            // psFits blows up on a string with a null value.
            // Arguably I should fix psLib.db should handle this
            rowItem->data.str = psStringCopy("NULL");
        }
        success = psMetadataAddStr(tableRow,PS_LIST_TAIL,colName,0,"",rowItem->data.str);
      }
      else if (!strcmp(colType,"PS_DATA_TIME")) {
	psString timeString = psTimeToISO(rowItem->data.V);
	success = psMetadataAddStr(tableRow,PS_LIST_TAIL,colName,0,"",timeString);
	psFree(timeString);
      }
      
      if (!success) {
        psError(PS_ERR_UNKNOWN,false, "Error constructing fits table row. %s %s",colType,colName);
        psFree(colNames);
        psFree(colTypes);
        psFree(contents);
        psFree(query);
        psFree(outTable);
        psFree(statsoutput);
        psFree(statIter);
        psFree(tableRow);
        psFree(colType);
        psFree(colName);
        return(false);
      }
    }
    // Calculate if this exposure was "good" or not.
    bool accept = true;
    psF32 quality = 1.0;
    for (long j = 0; j < decisionRules->n; j++) {
      psMetadata *rule = decisionRules->data[j];
      bool status = true;
      psString colname = psMetadataLookupStr(&status,rule,"COLNAME");
      if (!status) {
	psError(PS_EXIT_CONFIG_ERROR, false, "Unable to find column name");
	return(false);
      }
      // Not happy with this being set to a F32. Can this ever be something else?
      psF32 value      = psMetadataLookupF32(&status,tableRow,colname);
      if (!status) {
	psError(PS_EXIT_CONFIG_ERROR, false, "Unable to find column %s (%f) in compare with rule.",
		colname,value);
	psFree(colNames);
	psFree(colTypes);
	psFree(contents);
	psFree(query);
	psFree(outTable);
	psFree(tableRow);
	return(false);
      }
      psString filter = psMetadataLookupStr(&status,rule,"FILTER");
      if (!status) {
	status = true;
      }
      if (filter) {
	psString imfilter = psMetadataLookupStr(&status,tableRow,"FILTER");
	if (strcmp(filter,imfilter) != 0) {
	  continue;
	}
      }
      
      if (strcmp(psMetadataLookupStr(&status,rule,"RULETYPE"),"STRICT") == 0) {
	psF32 min_value  = psMetadataLookupF32(&status,rule,"MINIMUM");
	if (!status) {
	  psError(PS_EXIT_CONFIG_ERROR, false, "Unable to find min value for %s",colname);
	  return(false);
	}      
	psF32 max_value  = psMetadataLookupF32(&status,rule,"MAXIMUM");
	if (!status) {
	  psError(PS_EXIT_CONFIG_ERROR, false, "Unable to find max value for %s",colname);
	  return(false);
	}

	if ((value > max_value)||(value < min_value)) {
	  accept = false;
	}
      } // End STRICT min/max rule parsing
      else if (strcmp(psMetadataLookupStr(&status,rule,"RULETYPE"),"CDF") == 0) {
	// I'm just going to implement a quick linear interpolation over the points that exist.
	psF32 min_value,max_value = 99;
	psF32 min_cdf = 0.0,max_cdf = 1.0;
	psF32 cdf;
	psF32 try[22] = {0.00,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,
			 0.55,0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,0.99,1.00};
	int i;
	int done = 0;
	status = true;
	min_value = psMetadataLookupF32(&status,rule,"CDF00");
	min_cdf   = 0.0;
	if (!isfinite(value)) {
	  quality = 0.0;
	  continue;
	}
	if (!status) {
	  status = true;
	  psError(PS_EXIT_CONFIG_ERROR, false, "Unable to find CDF00 value for %s",colname);
	}
	if (value < min_value) {
	  if (quality > min_cdf) {
	    quality = 0.0;
	    continue;
	  }
	}

	for (i = 1; i < 22; i++) {
	  psString label = psStringCopy("");
	  psStringAppend(&label,"CDF%02d",(int) (100 * try[i]));
	  
	  max_value = psMetadataLookupF32(&status,rule,label);
	  max_cdf   = try[i];
	  if (!status) {
	    status = true;
	    psError(PS_EXIT_CONFIG_ERROR, false, "Unable to find %s value for %s",label,colname);
	  }
	  if ((value < max_value)&&(value > min_value)) {
	    cdf = ((value - min_value) * max_cdf + (max_value - value) * min_cdf) / (max_value - min_value);
	    //	    fprintf(stderr,"value: %f %f %f %f %f %f %d %d %f\n",value,quality,min_cdf,min_value,max_cdf,max_value, value < max_value, value > min_value,cdf);
	    if (quality > cdf) {
	      quality = cdf;
	      done = 1;
	      i = 25;
	    }
	  }
	  min_value = max_value;
	  min_cdf = max_cdf;
	}
	if (done) {
	  continue;
	}
	// Reached end of array without finding the value;
	if (value > min_value) {
	  if (quality > min_cdf) {
	    quality = 1.0;
	  }
	}


      } // End CDF rule parsing
    }
    bool success;
    success = psMetadataAddBool(tableRow,PS_LIST_TAIL,"ACCEPT",0,"",accept);
    if (!success) {
      psError(PS_ERR_UNKNOWN,false, "Error adding exposure quality to fits table row.");
      psFree(colNames);
      psFree(colTypes);
      psFree(contents);
      psFree(query);
      psFree(outTable);
      psFree(statsoutput);
      psFree(statIter);
      psFree(tableRow);
      return(false);
    }
    success = psMetadataAddF32(tableRow,PS_LIST_TAIL,"QUALITY",0,"",quality);
    if (!success) {
      psError(PS_ERR_UNKNOWN,false, "Error adding exposure quality to fits table row.");
      psFree(colNames);
      psFree(colTypes);
      psFree(contents);
      psFree(query);
      psFree(outTable);
      psFree(statsoutput);
      psFree(statIter);
      psFree(tableRow);
      return(false);
    }

    // Add the row to the table array.
    psArrayAdd(outTable,0,tableRow);
    psFree(tableRow);
  }
  psFree(colNames);
  psFree(colTypes);
  psFree(contents);
  psFree(query);
  // Define the fits table here.

  psFits *Table = psFitsOpen(bundleUri,"w");
  if (!Table) {
    psError(PS_ERR_UNKNOWN, false, "Could not open the fits table for writing.");
    psFree(outTable);
    return(false);
  }

  psMetadata *header = psMetadataAlloc();
  psMetadataAddS64(header, PS_LIST_TAIL, "DQSTATS_ID", 0, "ID number of DQSTATS run", dqstats_id);

  if (!psFitsWriteTable(Table,header,outTable,"DQSTATS")) {
    psError(PS_ERR_UNKNOWN, false, "Unable to write table to fits file.");
    psFree(outTable);
    psFree(Table);
    psFree(header);
    return(false);
  }
  psFree(outTable);
  psFree(header);

  // Tear down, cleanup.
  psFitsClose(Table);
  return(true);
}


static bool updaterunMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config,false);
  // PXOPT_LOOKUP_S64(dqstats_id, config->args, "-dqstats_id",     true, false);
  PXOPT_LOOKUP_STR(state,      config->args, "-set_state",      true, false);
  PXOPT_LOOKUP_S16(fault,      config->args, "-fault",          false, false);

  if (state && ! pxIsValidState(state)) {
    psError(PXTOOLS_ERR_CONFIG, false, "pxIsValidState failed");
    return(false);
  }

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-dqstats_id", "dqstats_id", "==");
  PXOPT_COPY_S64(config->args, where, "-state", "state", "==");

  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return(false);
  }

  psString query = psStringCopy("UPDATE dqstatsRun ");
  psStringAppend(&query," SET state = '%s' ", state);
  if (fault) {
    psStringAppend(&query, ", fault = %d ", fault);
  }
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query," WHERE %s ",whereClause);

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }

  psFree(query);
  psFree(where);
  psFree(whereClause);

  return(true);
}

static bool revertrunMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config,false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-dqstats_id", "dqstats_id", "==");
  PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return(false);
  }

  psString query = psStringCopy("UPDATE dqstatsRun SET fault = 0 ");
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query," WHERE fault != 0 AND %s ",whereClause);

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }

  psFree(query);
  psFree(whereClause);
  psFree(where);

  return(true);
}








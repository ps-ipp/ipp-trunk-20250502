/*
 * laptool.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "pxtools.h"
#include "pxdata.h"
#include "laptool.h"


// Sequence level
static bool definesequenceMode(pxConfig *config);
static bool listsequenceMode(pxConfig *config);

// Run level
static bool definerunMode(pxConfig *config);
static bool definerunbyreleaseMode(pxConfig *config);
static bool pendingrunMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool revertsassMode(pxConfig *config);

// Exposure level
static bool pendingexpMode(pxConfig *config);
static bool exposuresMode(pxConfig *config);
static bool stacksMode(pxConfig *config);
static bool updateexpMode(pxConfig *config);

static bool diffcheckMode(pxConfig *config);

static bool inactiveexpMode(pxConfig *config);

// Groups
static bool definegroupMode(pxConfig *config);
static bool pendinggroupMode(pxConfig *config);
static bool filtersforgroupMode(pxConfig *config);
static bool updategroupMode(pxConfig *config);
static bool revertgroupMode(pxConfig *config);
static bool listgroupMode(pxConfig *config);

# define MODECASE(caseName, func) \
  case caseName: \
  if (!func(config)) { \
  goto FAIL; \
  } \
  break;

int main(int argc, char **argv)
{
  psLibInit(NULL);

  pxConfig *config = laptoolConfig(NULL, argc, argv);
  if (!config) {
    psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
    goto FAIL;
  }

  switch (config->mode) {
    MODECASE(LAPTOOL_MODE_DEFINESEQUENCE, definesequenceMode);
    MODECASE(LAPTOOL_MODE_LISTSEQUENCE,   listsequenceMode);
    
    MODECASE(LAPTOOL_MODE_DEFINERUN,     definerunMode);
    MODECASE(LAPTOOL_MODE_DEFINERUNBYRELEASE, definerunbyreleaseMode);
    MODECASE(LAPTOOL_MODE_PENDINGRUN,    pendingrunMode);
    MODECASE(LAPTOOL_MODE_UPDATERUN,     updaterunMode);
    MODECASE(LAPTOOL_MODE_REVERTSASS,    revertsassMode);
    
    MODECASE(LAPTOOL_MODE_PENDINGEXP,    pendingexpMode);
    MODECASE(LAPTOOL_MODE_EXPOSURES,     exposuresMode);
    MODECASE(LAPTOOL_MODE_STACKS,        stacksMode);
    MODECASE(LAPTOOL_MODE_UPDATEEXP,     updateexpMode);

    MODECASE(LAPTOOL_MODE_DIFFCHECK,     diffcheckMode);
    
    MODECASE(LAPTOOL_MODE_INACTIVEEXP,   inactiveexpMode);

    MODECASE(LAPTOOL_MODE_DEFINEGROUP,   definegroupMode);
    MODECASE(LAPTOOL_MODE_PENDINGGROUP,  pendinggroupMode);
    MODECASE(LAPTOOL_MODE_FILTERSFORGROUP, filtersforgroupMode);
    MODECASE(LAPTOOL_MODE_UPDATEGROUP,   updategroupMode);
    MODECASE(LAPTOOL_MODE_REVERTGROUP,   revertgroupMode);
    MODECASE(LAPTOOL_MODE_LISTGROUP,     listgroupMode);

  default:
    psAbort("invalid option (this should not happen)");
  }
  psTrace("laptool", 9, "Attempting to free config\n");
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

// Sequence level

static bool definesequenceMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_STR(name,        config->args, "-name",        true, false);
  PXOPT_LOOKUP_STR(description, config->args, "-description", true, false);
  PXOPT_LOOKUP_BOOL(simple,     config->args, "-simple",     false);

  lapSequenceRow *run = lapSequenceRowAlloc(0, // seq_id
					    name,
					    description);
  if (!run) {
    psError(PS_ERR_UNKNOWN, false, "failed to alloc lapSequence object");
    return(true);
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  if (!lapSequenceInsertObject(config->dbh, run)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(run);
    return(true);
  }

  // point of no return
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  if (!lapSequencePrintObject(stdout, run, !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print object");
    psFree(run);
    return false;
  }

  psFree(run);

  return true;  
}

static bool listsequenceMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",  false, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  PXOPT_COPY_STR(config->args, where, "-seq_name",   "name",   "LIKE");

  psString query = pxDataGet("laptool_listsequence.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
  }

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
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapSequence", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);

  return true;
}
  

// Run level
static bool definerunMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_S64(seq_id,          config->args, "-seq_id",          true, false);
  PXOPT_LOOKUP_STR(projection_cell, config->args, "-projection_cell", true, false);
  PXOPT_LOOKUP_STR(tess_id,         config->args, "-tess_id",         true, false);
  PXOPT_LOOKUP_F64(ra,              config->args, "-ra",              true, false);
  PXOPT_LOOKUP_F64(decl,            config->args, "-decl",            true, false);
  PXOPT_LOOKUP_F32(radius,          config->args, "-radius",          true, false);
  PXOPT_LOOKUP_STR(filter,          config->args, "-filter",          true, false);
  PXOPT_LOOKUP_STR(label,           config->args, "-label",           false, false);
  PXOPT_LOOKUP_STR(dist_group,      config->args, "-dist_group",      false, false);
  PXOPT_LOOKUP_BOOL(all_obsmode,    config->args, "-all_obsmode",     false);
  PXOPT_LOOKUP_BOOL(simple,         config->args, "-simple",          false);

  lapRunRow *run = lapRunRowAlloc(0, // lap_id
				  seq_id,
				  tess_id,
				  projection_cell,
				  filter,
				  "new", // state
				  label,
				  dist_group,
				  NULL, // registered
				  0,    // fault
				  INT64_MAX,    // quick_sass_id
				  INT64_MAX     // final_sass_id
				  );
  if (!run) {
    psError(PS_ERR_UNKNOWN, false, "failed to alloc lapRun object");
    return(true);
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  if (!lapRunInsertObject(config->dbh, run)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(run);
    return(true);
  }


  if (!lapRunPrintObject(stdout, run, !simple)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "failed to print object");
    psFree(run);
    return false;
  }

  psS64 lap_id = psDBLastInsertID(config->dbh);

  // Find the input exposures
  psString query = pxDataGet("laptool_definerun.sql");
  if (!query) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }

  // Add constraints to the exposure search:
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_STR(config->args, where, "-filter", "rawExp.filter", "==");

  psMetadata *chipWhere = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, chipWhere, "-seq_id", "lapRun.seq_id", "==");
  
  // This seems unnecessarily clunky.
  if (!all_obsmode) {
    //    fprintf(stderr, "Not doing all obsmodes!\n");
    if (!psMetadataLookupStr(NULL,config->args,"-obsmode")) {
      //      fprintf(stderr, "Not doing all obsmodes and none specified!\n");
      psMetadataAddStr(where,PS_LIST_TAIL,"rawExp.obs_mode",0,"==","3PI");
    }
    pxAddLabelSearchArgs (config, where, "-obsmode", "rawExp.obs_mode", "==");
  }
  
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  if (!pxspaceAddWhere(config, &whereClause, "rawExp")) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
    return(false);
  }

  psString chipWhereClause = psDBGenerateWhereConditionSQL(chipWhere,NULL);
  if (whereClause) {
    psStringSubstitute(&query,whereClause,"@WHERE@");
  }
  if (chipWhereClause) {
    psStringSubstitute(&query,chipWhereClause,"@CHIPWHERE@");
  }
  psFree(where);
  psFree(chipWhere);

  // Fetch exposures
  if (!p_psDBRunQuery(config->dbh, query)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return(true);
  }

  // Insert the exposure data
  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i]; // Row from select
    // Add default values from this run:
    psMetadataAddS64(row, PS_LIST_TAIL, "lap_id", 0, "", lap_id);
    psMetadataAddS64(row, PS_LIST_TAIL, "pair_id", 0, "", INT64_MAX);
    psMetadataAddStr(row, PS_LIST_TAIL, "data_state", 0, "", "new");
    lapExpRow *lapExp = lapExpObjectFromMetadata(row);
    lapExp->lap_id = lap_id;

    if (!lapExpInsertObject(config->dbh,lapExp)) {
      if (!lapExpPrintObject(stdout, lapExp, !simple)) {
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	psError(PS_ERR_UNKNOWN, false, "failed to print object");
	psFree(run);
	return false;
      }
      
      if (!psDBRollback(config->dbh)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(output);
      psFree(lapExp);
      return(false);
    }
  }

  // point of no return
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  
  psFree(output);
  return(true);  
}

static bool definerunbyreleaseMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_S64(seq_id,          config->args, "-seq_id",          true, false);
  PXOPT_LOOKUP_STR(projection_cell, config->args, "-projection_cell", true, false);
  PXOPT_LOOKUP_STR(tess_id,         config->args, "-tess_id",         true, false);
  PXOPT_LOOKUP_STR(filter,          config->args, "-filter",          true, false);
  PXOPT_LOOKUP_STR(label,           config->args, "-label",           false, false);
  PXOPT_LOOKUP_STR(dist_group,      config->args, "-dist_group",      false, false);
  PXOPT_LOOKUP_S64(rel_id,          config->args, "-rel_id",          false, false);
  PXOPT_LOOKUP_STR(rel_name,        config->args, "-release_name",    false, false);
  PXOPT_LOOKUP_BOOL(simple,         config->args, "-simple",          false);

  // Validate config
  if ((!rel_name)&&(!rel_id)) {
    // Die here.
  }
  
  lapRunRow *run = lapRunRowAlloc(0, // lap_id
				  seq_id,
				  tess_id,
				  projection_cell,
				  filter,
				  "registered", // state
				  label,
				  dist_group,
				  NULL, // registered
				  0,    // fault
				  INT64_MAX,    // quick_sass_id
				  INT64_MAX     // final_sass_id
				  );
  if (!run) {
    psError(PS_ERR_UNKNOWN, false, "failed to alloc lapRun object");
    return(true);
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
    
  if (!lapRunInsertObject(config->dbh, run)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(run);
    return(false);
  }

  if (!lapRunPrintObject(stdout, run, !simple)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "failed to print object");
    psFree(run);
    return(false);
  }

  psS64 lap_id = psDBLastInsertID(config->dbh);

  psString query = pxDataGet("laptool_definerunbyrelease.sql");
  if (!query) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }
  
  // Add constraints
  psMetadata *relWhere = psMetadataAlloc();
  PXOPT_COPY_STR(config->args, relWhere, "-filter",       "rawExp.filter", "==");
  PXOPT_COPY_STR(config->args, relWhere, "-release_name", "ippRelease.release_name", "==");
  PXOPT_COPY_S64(config->args, relWhere, "-rel_id",       "ippRelease.rel_id", "==");
  PXOPT_COPY_TIME(config->args, relWhere, "-dateobs_begin", "rawExp.dateobs", ">=");
  PXOPT_COPY_TIME(config->args, relWhere, "-dateobs_end", "rawExp.dateobs", "<=");
  PXOPT_COPY_RADEC(config->args, relWhere, "-ra_min", "rawExp.ra", ">=");
  PXOPT_COPY_RADEC(config->args, relWhere, "-ra_max", "rawExp.ra", "<");
  PXOPT_COPY_RADEC(config->args, relWhere, "-decl_min", "rawExp.decl", ">=");
  PXOPT_COPY_RADEC(config->args, relWhere, "-decl_max", "rawExp.decl", "<");

  psMetadata *lapWhere = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, lapWhere, "-seq_id",       "lapRun.seq_id", "==");
  PXOPT_COPY_STR(config->args, lapWhere, "-filter",       "lapRun.filter", "==");

  psString relWhereClause = psDBGenerateWhereConditionSQL(relWhere,NULL);
  psString lapWhereClause = psDBGenerateWhereConditionSQL(lapWhere,NULL);

  if (relWhereClause) {
    psStringSubstitute(&query,relWhereClause,"@RELWHERE@");
  }
  if (lapWhereClause) {
    psStringSubstitute(&query,lapWhereClause,"@LAPWHERE@");
  }
  psFree(relWhere);
  psFree(lapWhere);

  // Fetch exposures
  if (!p_psDBRunQuery(config->dbh, query)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }

    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return(true);
  }

  // Insert the exposure data
  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i]; // Row from select
    // Add default values from this run:
    psMetadataAddS64(row, PS_LIST_TAIL, "lap_id", 0, "", lap_id);
    psMetadataAddS64(row, PS_LIST_TAIL, "pair_id", 0, "", INT64_MAX);
    psMetadataAddStr(row, PS_LIST_TAIL, "data_state", 0, "", "new");
    lapExpRow *lapExp = lapExpObjectFromMetadata(row);
    lapExp->lap_id = lap_id;

    if (!lapExpInsertObject(config->dbh,lapExp)) {
      if (!lapExpPrintObject(stdout, lapExp, !simple)) {
	if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
	}
	psError(PS_ERR_UNKNOWN, false, "failed to print object");
	psFree(run);
	return false;
      }
      
      if (!psDBRollback(config->dbh)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(output);
      psFree(lapExp);
      return(false);
    }
  }

  // point of no return
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  
  psFree(output);
  return(true);  
}


  
  
		   
static bool pendingrunMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",  false, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");
  PXOPT_COPY_STR(config->args, where, "-projection_cell", "projection_cell", "==");
  PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
  //  PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
  PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
  PXOPT_COPY_STR(config->args, where, "-fault", "fault", "==");

  pxAddLabelSearchArgs(config, where, "-label", "lapRun.label", "==");
  
  psString query = pxDataGet("laptool_pendingrun.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
  }
  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, "\n %s", limitString);
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
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapRun", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);

  return true;
}

static bool updaterunMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();

  // require? PXOPT_LOOKUP_S64(lap_id, config->args, "-lap_id", true, false);
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");

  psMetadata *values = psMetadataAlloc();
  PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
  PXOPT_COPY_STR(config->args, values, "-set_state", "state", "==");
  PXOPT_COPY_S16(config->args, values, "-fault",     "fault", "==");
  PXOPT_COPY_STR(config->args, values, "-set_label", "label", "==");
  PXOPT_COPY_S64(config->args, values, "-set_quick_sass_id", "quick_sass_id", "==");
  PXOPT_COPY_S64(config->args, values, "-set_final_sass_id", "final_sass_id", "==");

  long rows = psDBUpdateRows(config->dbh, "lapRun", where, values);
  psFree(values);
  
  if (rows) {
    // We're done with these exposures now, so mark them as inactive.
    if (state) {
      if ((strcmp(state,"drop") == 0)||
	  (strcmp(state,"full") == 0)) {
	values = psMetadataAlloc();
	psMetadataAddBool(values, PS_LIST_TAIL, "active", 0, "", false);
	long exps = psDBUpdateRows(config->dbh, "lapExp", where, values);
	
	if (exps) {
	  return(true);
	}
	else {
	  return(true); // We shouldn't really fail if we didn't change anything. Maybe there's nothing to change.
	}
      }
    }
  }

  return(true);
}

static bool revertsassMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");
  PXOPT_COPY_S64(config->args, where, "-sass_id", "final_sass_id", "==");
  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  psString query = pxDataGet("laptool_revertsass.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query, "\n WHERE %s", whereClause);
  psFree(whereClause);

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  
  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }
  psFree(query);

  long numUpdated = psDBAffectedRows(config->dbh);
  if (numUpdated != 1) {
    psError(PS_ERR_UNKNOWN, false, "should have updated only one row. updated %ld", numUpdated);
    return false;
  }

  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  
  return true;
}
// Exposure level

static bool pendingexpMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",  false, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");

  psString query = pxDataGet("laptool_pendingexp.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\n AND %s", whereClause);
    psFree(whereClause);
  }
  psStringAppend(&query, " ORDER BY rawExp.dateobs ");
  
  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, "\n %s", limitString);
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
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapExp", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);

  return true;
}


static bool exposuresMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");
  PXOPT_COPY_S64(config->args, where, "-exp_id", "lapExp.exp_id", "==");
  
  psString query = pxDataGet("laptool_exposures.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  if (whereClause) {
    psStringSubstitute(&query,whereClause,"@WHERE@");
  }
  
  psString limitString = NULL;
  if (limit) {
    limitString = psDBGenerateLimitSQL(limit);
    psStringPrepend(&limitString, "\n");
  }

  if (!p_psDBRunQueryF(config->dbh, query, whereClause, limitString ? limitString : "")) {
    psError(PXTOOLS_ERR_PROG, false, "database error");
    psFree(limitString);
    psFree(query);
    psFree(whereClause);
    return(false);
  }
  psFree(limitString);
  psFree(query);
  psFree(whereClause);
  
  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return(true);
  }
  
  if (!ippdbPrintMetadatas(stdout, output, "lapExp", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return(false);
  }

  psFree(output);
  return(true);
}

static bool stacksMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  // require? PXOPT_LOOKUP_S64(lap_id, config->args, "-lap_id", true, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lapRun.lap_id", "==");
  
  psString query = pxDataGet("laptool_stacks.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }
  
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringPrepend(&whereClause, "\n AND ");
    psStringSubstitute(&query,whereClause,"@WHERE@");
    psFree(whereClause);
  }

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
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error %d",err);
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapRunStacks", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);
  return(true);
}

static bool updateexpMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();

  // test for required opts?
  // PXOPT_LOOKUP_S64(lap_id,  config->args, "-lap_id",  true, false);
  // PXOPT_LOOKUP_S64(exp_id,  config->args, "-exp_id",  true, false);
  // PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", false, false);

  PXOPT_LOOKUP_S64(set_chip_id, config->args, "-set_chip_id", false, false);
  PXOPT_LOOKUP_S64(set_pair_id, config->args, "-set_pair_id", false, false);
  PXOPT_LOOKUP_STR(set_data_state, config->args, "-set_data_state", false, false);
  PXOPT_LOOKUP_BOOL(private,    config->args, "-private",     false);
  PXOPT_LOOKUP_BOOL(public,     config->args, "-public",      false);
  PXOPT_LOOKUP_BOOL(pairwise,   config->args, "-pairwise",    false);
  PXOPT_LOOKUP_BOOL(nopairwise, config->args, "-nopairwise",  false);
  PXOPT_LOOKUP_BOOL(active,     config->args, "-active",      false);
  PXOPT_LOOKUP_BOOL(inactive,   config->args, "-inactive",    false);


  if (private && public) {
    psError(PS_ERR_UNKNOWN, false, "only one of -private and -public may be selected");
  }
  if (active && inactive) {
    psError(PS_ERR_UNKNOWN, false, "only one of -active and -inactive may be selected");
  }
  if (pairwise && nopairwise) {
    psError(PS_ERR_UNKNOWN, false, "only one of -pairwise and -nopairwise may be selected");
  }

  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");
  PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
  PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");


  psMetadata *values = psMetadataAlloc();
  if (set_chip_id) {
    PXOPT_COPY_S64(config->args, values, "-set_chip_id", "chip_id", "==");
  }
  if (set_pair_id) {
    PXOPT_COPY_S64(config->args, values, "-set_pair_id", "pair_id", "==");
  }
  if (private) {
    psMetadataAddBool(values, PS_LIST_TAIL, "private", 0, "==", true);
  }
  else if (public) {
    psMetadataAddBool(values, PS_LIST_TAIL, "private", 0, "==", false);
  }
  if (active) {
    psMetadataAddBool(values, PS_LIST_TAIL, "active", 0, "==", true);
  }
  else if (inactive) {
    psMetadataAddBool(values, PS_LIST_TAIL, "active", 0, "==", false);
  }
  if (pairwise) {
    psMetadataAddBool(values, PS_LIST_TAIL, "pairwise", 0, "==", true);
  }
  else if (nopairwise) {
    psMetadataAddBool(values, PS_LIST_TAIL, "pairwise", 0, "==", false);
  }
  if (set_data_state) {
    PXOPT_COPY_STR(config->args, values, "-set_data_state", "data_state", "==");
  }
  long rows = psDBUpdateRows(config->dbh,"lapExp",where,values);
  if (rows) {
    return(true);
  }
/*   else { */
/*     return(false); */
/*   }   */
  return(true);
}

static bool diffcheckMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lapRun.lap_id", "==");
  PXOPT_COPY_S64(config->args, where, "-exp_id",  "exp_id", "==");
  PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");
  PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
  PXOPT_COPY_S64(config->args, where, "-seq_id",  "seq_id", "==");
  PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");

  psString query = pxDataGet("laptool_WSdiff_check.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }
  
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringPrepend(&whereClause, "\n AND ");
    psStringSubstitute(&query,whereClause,"@WHERE@");
    psFree(whereClause);
  }

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error %d",err);
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapRunWarpStackSkycells", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);
  return(true);
}    

static bool inactiveexpMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  // PXOPT_LOOKUP_S64(lap_id,          config->args, "-lap_id",          true, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-lap_id", "lap_id", "==");
  
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  
  psString query = pxDataGet("laptool_inactiveexp.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return(false);
  }
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  if (whereClause) {
    psStringSubstitute(&query,whereClause,"@WHERE@");
  }
  
  psString limitString = NULL;
  if (limit) {
    limitString = psDBGenerateLimitSQL(limit);
    psStringPrepend(&limitString, "\n");
  }

  if (!p_psDBRunQueryF(config->dbh, query, whereClause, limitString ? limitString : "")) {
    psError(PXTOOLS_ERR_PROG, false, "database error");
    psFree(limitString);
    psFree(query);
    psFree(whereClause);
    return(false);
  }
  psFree(limitString);
  psFree(query);
  psFree(whereClause);
  
  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return(true);
  }
  
  if (!ippdbPrintMetadatas(stdout, output, "lapExp", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return(false);
  }

  psFree(output);
  return(true);
}
// ---------------------------
// Group level (a collection of completed lapRuns for a given lapSequence projection cell and collection of filters

static bool definegroupMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_S64(seq_id,          config->args, "-seq_id",    true, false);
  PXOPT_LOOKUP_STR(label,           config->args, "-set_label", true, false);

  PXOPT_LOOKUP_BOOL(pretend,        config->args, "-pretend",    false);
  PXOPT_LOOKUP_BOOL(simple,         config->args, "-simple",    false);
  PXOPT_LOOKUP_U64(limit,           config->args, "-limit",     false, false);

  // the following insures that the -filter argument has been set up properly (adapted from pxAddLabelSearchArgs)
  psMetadataItem *item = psMetadataLookup(config->args, "-filter");
  psAssert (item, "-filter argument not found in config->args");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "%s should be a multi container", "filter");
  psAssert (item->data.list->n, "%s should at least have a place-holder", "filter");
  psMetadataItem *entry = (psMetadataItem *)item->data.list->head->data;
  psAssert (entry, "%s should at least have a place-holder", "filter");
  // end of checking

  // Now if the ony entry is the place-holder then the user supplied no -filter arguments
  // which are required
  if (!entry->data.str) {
    psError(PXTOOLS_ERR_ARGUMENTS, true, "at least one -filter is required");
    return false;
  }

  int nFilters = item->data.list->n;

  psString query = psStringCopy("SELECT seq_id, tess_id, projection_cell,\n");

  // We construct a query joining completed lap runs with the same lapSequence.seq_id,
  // tess_id, and projection_cell 

  // select the lap_ids for the various filters. These are only used for debugging.
  psStringAppend(&query, "lap_id_%d", 0);
  for (int i = 1; i < nFilters; i++) {
    psStringAppend(&query, ", lap_id_%d", i);
  }


  // sub query for each supplied filter
  char * lapRunForFilter = "(\nSELECT seq_id, tess_id, projection_cell, lap_id as 'lap_id_%d'\n"
    "FROM lapRun\n"
    "WHERE filter LIKE '%s'\n"
    "   AND lapRun.seq_id = %"PRId64"\n"
    "   AND (lapRun.state = 'done' or lapRun.state = 'full')\n"
    " ) as lap_%d\n";

  // loop over supplied filters and flesh out the query using the format above
  psListIterator *iter = psListIteratorAlloc (item->data.list, PS_LIST_HEAD, true);
  psMetadataItem *filterItem = NULL;
  int i = -1;
  while ((filterItem = psListGetAndIncrement(iter))) {
    ++i;
    if (i == 0) {
      psStringAppend(&query, "\nFROM\n");
    } else {
      psStringAppend(&query, "\nJOIN\n");
    }

    psString filter = filterItem->data.str;
    psStringAppend(&query, lapRunForFilter, i, filter, seq_id, i);

    if (i != 0) {
      psStringAppend(&query, "USING (seq_id, tess_id, projection_cell)\n");
    }
  }
  // now join to lapGroup
  psStringAppend(&query, "\nLEFT JOIN lapGroup USING(seq_id, tess_id, projection_cell)\n");

  // we only want projection cells which do not already of an entry
  psStringAppend(&query, "\nWHERE lapGroup.projection_cell IS NULL\n");
  // Projection cell's don't have a convienient order. Queue runs in the order of lap_id
  // psStringAppend(&query, "\nORDER by projection_cell\n");
  psStringAppend(&query, "\nORDER by lap_id_0\n");

  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, "\n %s", limitString);
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
      psTrace("laptool", PS_LOG_INFO, "no rows found");
      psFree(output);
      return true;
  }

  if (pretend) {
    if (!ippdbPrintMetadatas(stdout, output, "new_lapGroups", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);
    return true;
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  psTime *now = psTimeGetNow(PS_TIME_TAI);

  psString last_projection_cell = NULL;
  for (long i = 0; i < psArrayLength(output); i++) {
    psMetadata *row = output->data[i];

    psString tess_id = psMetadataLookupStr(NULL, row, "tess_id");
    psString projection_cell = psMetadataLookupStr(NULL, row, "projection_cell");
    if (last_projection_cell && !strcmp(last_projection_cell, projection_cell)) {
        // duplicate lap runs for a filter will generate multiple rows
        // Since we care about projection_cells not lapRuns per se this is not problem.
        // Skip any duplicates.
        continue;
    }
    last_projection_cell = projection_cell;

    lapGroupRow *group = lapGroupRowAlloc(
                                  seq_id,
				  tess_id,
				  projection_cell,
				  "new",  // state
                                  label,
                                  now,    // registered
				  0       // fault
				  );
    if (!group) {
      psError(PS_ERR_UNKNOWN, false, "failed to alloc lapGroup object");
      psFree(output);
      psFree(now);
      return(false);
    }


    if (!lapGroupInsertObject(config->dbh, group)) {
      if (!psDBRollback(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(output);
      return(true);
    }
  }

  // point of no return
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  psFree(now);
  psFree(output);

  return(true);  
}

static bool pendinggroupMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",  false, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  pxAddLabelSearchArgs(config, where, "-label", "lapRun.label", "==");
  
  psString query = pxDataGet("laptool_pendinggroup.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }

  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
  }
  psStringAppend(&query, "\nGROUP by projection_cell ORDER by MIN(lap_id)");
  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, "\n %s", limitString);
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
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapGroup", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);

  return true;
}
static bool filtersforgroupMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id", "tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-projection_cell", "projection_cell", "==");
  
  psString query = pxDataGet("laptool_filtersforgroup.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }

  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query, " WHERE %s", whereClause);
  psFree(whereClause);

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "filters", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);

  return true;
}

static bool updategroupMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  // check that all of the keys that define a lapGroup are supplied
  PXOPT_LOOKUP_S64(seq_id, config->args, "-seq_id", true, false);
  PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", true, false);
  PXOPT_LOOKUP_STR(projection_cell, config->args, "-projection_cell", true, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id", "tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-projection_cell", "projection_cell", "==");

  psMetadata *values = psMetadataAlloc();
  PXOPT_COPY_STR(config->args, values, "-set_state", "state", "==");
  PXOPT_COPY_STR(config->args, values, "-set_label", "label", "==");
  PXOPT_COPY_S16(config->args, values, "-set_fault", "fault", "==");

  if (!psListLength(values->list)) {
    psFree(values);
    psFree(where);
    psError(PXTOOLS_ERR_ARGUMENTS, true, "must set at least one column");
    return false;
  }

  long rows = psDBUpdateRows(config->dbh, "lapGroup", where, values);
  if (rows < 1) {
    psFree(values);
    psError(PXTOOLS_ERR_SYS, true, "failed to update lapGroup for %" PRId64 " %s %s", seq_id, tess_id, projection_cell);
    return false;
  }
  psFree(values);

  return(true);
}

static bool revertgroupMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  pxAddLabelSearchArgs (config, where, "-label", "label", "==");
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id", "tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-projection_cell", "projection_cell", "==");

  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search arguments are required");
    return false;
  }

  psString query = pxDataGet("laptool_revertgroup.sql");
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query, " AND %s", whereClause);
  psFree(whereClause);
  psFree(where);
  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }
  psFree(query);

  psU64 affected = psDBAffectedRows(config->dbh);
  psLogMsg("laptool", PS_LOG_INFO, "Updated %" PRIu64 " lapGroups", affected);

  return true;
}

static bool listgroupMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psError(PXTOOLS_ERR_SYS, true, "not yet implemented");
  return false;

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",  false, false);
  
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-seq_id", "seq_id", "==");
  PXOPT_COPY_STR(config->args, where, "-seq_name",   "name",   "LIKE");

  psString query = pxDataGet("laptool_listsequence.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
  }

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
    psErrorCode err = psErrorCodeLast();
    switch (err) {
    case PS_ERR_DB_CLIENT:
      psError(PXTOOLS_ERR_SYS, false, "database error");
    case PS_ERR_DB_SERVER:
      psError(PXTOOLS_ERR_PROG, false, "database error");
    default:
      psError(PXTOOLS_ERR_PROG, false, "unknown error");
    }

    return false;
  }
  if (!psArrayLength(output)) {
    psTrace("laptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    if (!ippdbPrintMetadatas(stdout, output, "lapSequence", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);

  return true;
}

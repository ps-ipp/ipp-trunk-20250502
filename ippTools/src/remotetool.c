/*
 * remotetool.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "pxtools.h"
#include "pxdata.h"
#include "remotetool.h"

// Create
static bool definebyqueryMode(pxConfig *config);

// List
static bool listrunMode(pxConfig *config);
static bool listcomponentMode(pxConfig *config);

// list remoteRuns with all remoteComponents in state 'prep_done' or 'prep_fail'
static bool doneprepMode(pxConfig *config);

// Update
static bool dropcomponentMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool updatepollMode(pxConfig *config);
static bool updatecomponentMode(pxConfig *config);
static bool revertrunMode(pxConfig *config);
static bool revertauthMode(pxConfig *config);
static bool revertcomponentMode(pxConfig *config);

# define MODECASE(caseName, func) \
  case caseName: \
  if (!func(config)) { \
  goto FAIL; \
  } \
  break;

int main(int argc, char **argv)
{
  psLibInit(NULL);

  pxConfig *config = remotetoolConfig(NULL, argc, argv);
  if (!config) {
    psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
    goto FAIL;
  }

  switch (config->mode) {
    MODECASE(REMOTETOOL_MODE_DEFINEBYQUERY,   definebyqueryMode);
    MODECASE(REMOTETOOL_MODE_LISTRUN,         listrunMode);
    MODECASE(REMOTETOOL_MODE_DONEPREP,         doneprepMode);
    MODECASE(REMOTETOOL_MODE_LISTCOMPONENT,   listcomponentMode);
    MODECASE(REMOTETOOL_MODE_DROPCOMPONENT,   dropcomponentMode);
    MODECASE(REMOTETOOL_MODE_UPDATERUN,       updaterunMode);
    MODECASE(REMOTETOOL_MODE_UPDATEPOLL,      updatepollMode);
    MODECASE(REMOTETOOL_MODE_UPDATECOMPONENT, updatecomponentMode);
    MODECASE(REMOTETOOL_MODE_REVERTRUN,       revertrunMode);
    MODECASE(REMOTETOOL_MODE_REVERTAUTH,      revertauthMode);
    MODECASE(REMOTETOOL_MODE_REVERTCOMPONENT, revertcomponentMode);

  default:
    psAbort("invalid option (this should not happen)");
  }
  psTrace("remotetool", 9, "Attempting to free config\n");
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

// Create

static bool definebyqueryMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  // These are required
  PXOPT_LOOKUP_STR(label,      config->args, "-label",        true, false);
  PXOPT_LOOKUP_STR(stage,      config->args, "-stage",        true, false);
  PXOPT_LOOKUP_STR(path_base,  config->args, "-path_base",    true, false);

  // These are not
  PXOPT_LOOKUP_STR(set_label,  config->args, "-set_label",    false, false);
  PXOPT_LOOKUP_S16(limit,      config->args, "-limit",        false, false);

  PXOPT_LOOKUP_BOOL(simple,    config->args, "-simple",       false);
  PXOPT_LOOKUP_BOOL(pretend,   config->args, "-pretend",      false);

  set_label = set_label ? set_label : label;

  // Get a list of the things we can insert into a new component
  // CZW: these labels no longer need to be linked to the table, as I've split stageRun and remoteRun into sub-queries.
  psString query = NULL;
  psString whereOption = psStringCopy("");
  if (!strcmp(stage,"chip")) {
    query = pxDataGet("remotetool_definebyquery_chip.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else if (!strcmp(stage,"camera")) {
    query = pxDataGet("remotetool_definebyquery_cam.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else if (!strcmp(stage,"warp")) {
    query = pxDataGet("remotetool_definebyquery_warp.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      // Because warp has to check to see if something other than the state is set correctly, I have to use this wonky way to pass the label in.
      //      psStringAppend(&query, " AND (warpRun.label = '%s')", label);
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else if (!strcmp(stage,"stack")) {
    query = pxDataGet("remotetool_definebyquery_stack.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else if (!strcmp(stage,"staticsky")) {
    query = pxDataGet("remotetool_definebyquery_staticsky.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else if (!strcmp(stage,"diff")) {
    query = pxDataGet("remotetool_definebyquery_diff.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else if (!strcmp(stage,"ff")) {
    query = pxDataGet("remotetool_definebyquery_ff.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
    }

    if (label) {
      psStringAppend(&whereOption, "\n AND (label = '%s')", label);
    }
  }
  else {
    psError(PS_ERR_UNKNOWN, true, "unknown value for stage: %s", stage);
    return false;
  }

  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s", limitString);
    psFree(limitString);
  }
  if (!p_psDBRunQueryF(config->dbh, query,whereOption,whereOption)) { // This needs to be here twice because we need to restrict on label twice.
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
    psTrace("remotetool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (pretend) {
    if (!ippdbPrintMetadatas(stdout, output, "newremoteComponents", !simple)) {
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


  // Insert the top level run object
  remoteRunRow *run = remoteRunRowAlloc(0, // remote_id
                                        "new", // state
                                        stage,
                                        set_label,
                                        path_base,
                                        -1, // job_id
                                        NULL, // last_poll
                                        0   // fault
                                          );
  if (!run) {
    psError(PS_ERR_UNKNOWN, false, "failed to alloc remoteRun object");
    return(true);
  }

  if (!remoteRunInsertObject(config->dbh, run)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(run);
    return(true);
  }
  psS64 remote_id = psDBLastInsertID(config->dbh);

  psArray *list = psArrayAllocEmpty(limit);
  for (long i=0; i < psArrayLength(output); i++) {
    psMetadata *md = output->data[i];
    psS64 stage_id = psMetadataLookupS64(NULL, md, "stage_id");

    remoteComponentRow *comp = remoteComponentRowAlloc(remote_id, stage_id, 0, "new", NULL);
    if (!comp) {
      psError(PS_ERR_UNKNOWN, false, "failed to alloc remoteComponent object");
      return(true);
    }

    if (!remoteComponentInsertObject(config->dbh, comp)) {
      if (!psDBRollback(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
      }
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(run);
      return(true);
    }

    psArrayAdd(list, list->n, comp);
    psFree(comp);

  }
  psFree(run);
  // point of no return
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  if (!remoteComponentPrintObjects(stdout, list, !simple)) {
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "failed to print object");
    psFree(run);
    return false;
  }

  psFree(list);
  psFree(output);

  return true;
}

// List
static bool listrunMode(pxConfig *config)
{
  psMetadata *where = psMetadataAlloc();

  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_STR(config->args, where, "-state", "state", "==");
  PXOPT_COPY_STR(config->args, where, "-stage",  "stage",  "==");
  PXOPT_COPY_STR(config->args, where, "-label",  "label",  "==");

  PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");
  PXOPT_COPY_TIME(config->args, where, "-poll_begin", "last_poll", ">=");
  PXOPT_COPY_TIME(config->args, where, "-poll_end", "last_poll", "<=");

  PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");

  PXOPT_LOOKUP_S16(limit,   config->args, "-limit",  false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  psArray *remRuns = NULL;
  remRuns = psDBSelectRows(config->dbh, "remoteRun", where, limit);

  if (!ippdbPrintMetadatas(stdout, remRuns, "remoteRun", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(remRuns);
    return false;
  }
  psFree(remRuns);

  return true;
}

static bool listcomponentMode(pxConfig *config)
{
  psMetadata *where = psMetadataAlloc();

  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_S64(config->args, where, "-stage_id",  "stage_id",  "==");
  PXOPT_COPY_S64(config->args, where, "-jobs",      "jobs",      "==");

  PXOPT_COPY_STR(config->args, where, "-state", "remoteComponent.state", "==");
  PXOPT_COPY_STR(config->args, where, "-label", "remoteRun.label", "==");

  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_S16(limit,   config->args, "-limit",  false, false);

  psString query = pxDataGet("remotetool_listcomponent.sql");
  if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
  }

  if (psListLength(where->list)) {
      psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
      psStringAppend(&query, " WHERE %s", whereClause);
      psFree(whereClause);
  }
  psFree(where);

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
    psTrace("remotetool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (!ippdbPrintMetadatas(stdout, output, "remoteComponent", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return false;
  }
  psFree(output);

  return true;
}

// List
static bool doneprepMode(pxConfig *config)
{
  psMetadata *where = psMetadataAlloc();

  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_S64(config->args, where, "-job_id", "job_id", "==");

  PXOPT_COPY_STR(config->args, where, "-stage",  "stage",  "==");
  PXOPT_COPY_STR(config->args, where, "-state", "remoteRun.state", "==");
  PXOPT_COPY_STR(config->args, where, "-label",  "label",  "==");

  PXOPT_LOOKUP_S16(limit,   config->args, "-limit",  false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  psString query = pxDataGet("remotetool_doneprep.sql");
  if (!query) {
      psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
      return(false);
  }

  psString whereDone = NULL;
  psString whereJobs = NULL;
  if (psListLength(where->list)) {
      psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
      psStringAppend(&whereDone, " \n AND %s", whereClause);
      psStringAppend(&whereJobs, " \n WHERE %s", whereClause);
      psFree(whereClause);
  }

  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s", limitString);
    psFree(limitString);
  }

  // fprintf (stderr, "query: %s\n", query);
  // fprintf (stderr, "whereDone: %s\n", whereDone);
  // fprintf (stderr, "whereJobs: %s\n", whereJobs);

  if (!p_psDBRunQueryF(config->dbh, query, whereDone, whereJobs)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(whereDone);
      psFree(whereJobs);
      psFree(query);
      return false;
  }
  psFree(whereDone);
  psFree(whereJobs);
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      return false;
  }
  if (!psArrayLength(output)) {
    psTrace("remotetool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (!ippdbPrintMetadatas(stdout, output, "remoteRun", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return false;
  }
  psFree(output);

  return true;
}

// Update

static bool dropcomponentMode(pxConfig *config)
{
  fprintf (stderr, "do we really want to use -dropcomponent?\n");
  exit (1);

  // Assert that we have a unique component to operate on.
  PXOPT_LOOKUP_S64(remote_id,config->args, "-remote_id",true, false);
  psAssert(remote_id,"This must exist for this mode to be safe.");
  PXOPT_LOOKUP_S64(stage_id, config->args, "-stage_id", true, false);
  psAssert(stage_id,"This must exist for this mode to be safe.");

  psString query = pxDataGet("remotetool_dropcomponent.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "Failed to retreive SQL statement");
    return false;
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  if (!p_psDBRunQueryF(config->dbh, query, remote_id, stage_id)) {
    psError(PS_ERR_UNKNOWN, false, "Database error");
    psFree(query);
    return false;
  }
  psFree(query);

  // point of no return
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  return true;
}

static bool updaterunMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_S64(remote_id,  config->args, "-remote_id",    true, false);
  psAssert(remote_id,"This should have failed before this point.");

  psMetadata *where = psMetadataAlloc();
  psMetadata *values = psMetadataAlloc();

  // Wheres
  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");

  // Values to set
  PXOPT_COPY_STR(config->args, values, "-set_label", "label", "==");
  PXOPT_COPY_STR(config->args, values, "-set_state", "state", "==");
  PXOPT_COPY_S16(config->args, values, "-fault",     "fault", "==");
  PXOPT_COPY_S64(config->args, values, "-job_id",    "job_id", "==");

  long rows = psDBUpdateRows(config->dbh, "remoteRun", where, values);
  psFree(values);

  if (rows < 1) fprintf (stderr, "no rows changed (run may already be in desired state)\n");
  return(true);
}

static bool updatepollMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_S64(remote_id,  config->args, "-remote_id",    true, false);
  psAssert(remote_id,"This should have failed before this point.");

  psMetadata *where = psMetadataAlloc();
  // Wheres
  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psFree(where);

  psString query = NULL;
  query = pxDataGet("remotetool_updatepoll.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS,false, "failed to retreive SQL statement");
    psFree(whereClause);
    return(false);
  }

  if (!p_psDBRunQueryF(config->dbh, query,whereClause)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    psFree(whereClause);
    return false;
  }
  psFree(query);
  psFree(whereClause);
  return(true);
}

static bool updatecomponentMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_LOOKUP_S64(remote_id,  config->args, "-remote_id",    true, false);
  psAssert(remote_id,"This should have failed before this point.");

  psMetadata *where = psMetadataAlloc();
  psMetadata *values = psMetadataAlloc();

  // Wheres
  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_S64(config->args, where, "-stage_id",  "stage_id",  "==");
  PXOPT_COPY_STR(config->args, where, "-state",     "state",     "==");

  // Values to set
  PXOPT_COPY_S32(config->args, values, "-set_jobs",      "jobs",      "==");
  PXOPT_COPY_STR(config->args, values, "-set_path_base", "path_base", "==");
  PXOPT_COPY_STR(config->args, values, "-set_state", "state", "==");

  long rows = psDBUpdateRows(config->dbh, "remoteComponent", where, values);
  psFree(values);

  if (rows < 1) fprintf (stderr, "no rows changed (component may already be in desired state)\n");
  return true;
}

static bool revertrunMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  psMetadata *values = psMetadataAlloc();

  // required
  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
  PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");
  // Value to set
  psMetadataAddS16(values,PS_LIST_TAIL,"fault",0, "==", 0);
  
  long rows = psDBUpdateRows(config->dbh, "remoteRun", where, values);
  psFree(values);
  psFree(where);
  if (rows < 1) {
    psError(PS_ERR_UNKNOWN,false, "no rows affected");
    return(false);
  }

  return(true);
}

static bool revertcomponentMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config,false);
  psMetadata *where = psMetadataAlloc();
  psMetadata *values = psMetadataAlloc();

  // required
  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_S64(config->args, where, "-stage_id", "stage_id", "==");
  psMetadataAddStr(where,PS_LIST_TAIL,"state",0, "LIKE", "%fail");
  
  // value to set
  psMetadataAddStr(values, PS_LIST_TAIL, "state",0, "==", "retry");
  

  long rows = psDBUpdateRows(config->dbh, "remoteComponent", where, values);
  psFree(values);
  psFree(where);
  if (rows < 1) {
    psError(PS_ERR_UNKNOWN,false, "no rows affected");
    return(false);
  }

  return(true);
}
  
  

static bool revertauthMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  psMetadata *values = psMetadataAlloc();

  // required
  PXOPT_COPY_S64(config->args, where, "-remote_id", "remote_id", "==");
  PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

  psMetadataAddStr(where, PS_LIST_TAIL, "state", 0, "==", "auth");

  // Value to set
  psMetadataAddStr(values, PS_LIST_TAIL, "state", 0, "==", "run");

  long rows = psDBUpdateRows(config->dbh, "remoteRun", where, values);
  psFree(values);
  psFree(where);
  if (rows < 1) {
    psError(PS_ERR_UNKNOWN,false, "no rows affected");
    return(false);
  }

  return(true);
}


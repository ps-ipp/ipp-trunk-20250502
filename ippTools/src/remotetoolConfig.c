/*
 * remotetoolConfig.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>
#include <psmodules.h>
#include "pxtools.h"
#include "remotetool.h"

#define ADD_OPT(TYPE,TARG,NAME,COMMENT,DEFAULT) psMetadataAdd##TYPE(TARG, PS_LIST_TAIL, NAME, 0, COMMENT, DEFAULT)

pxConfig *remotetoolConfig(pxConfig *config, int argc, char **argv)
{
  if (!config) {
    config = pxConfigAlloc();
  }

  pmConfigReadParamsSet(false);

  // setup site config
  config->modules = pmConfigRead(&argc, argv, NULL);
  if (!config->modules) {
    psError(psErrorCodeLast(), false, "Can't find site configuration");
    psFree(config);
    return NULL;
  }

  // -definebyquery
  psMetadata *definebyqueryArgs = psMetadataAlloc();
  ADD_OPT(Str, definebyqueryArgs, "-label",          "stage label to query", NULL);
  ADD_OPT(Str, definebyqueryArgs, "-set_label",      "remRun label to use", NULL);
  ADD_OPT(Str, definebyqueryArgs, "-stage",          "stage to query", NULL);
  ADD_OPT(Str, definebyqueryArgs, "-path_base",      "define base output location", NULL);
  ADD_OPT(S16, definebyqueryArgs, "-limit",          "limit to number of jobs to bundle",0);
  ADD_OPT(Bool, definebyqueryArgs, "-pretend",       "pretend to do this.", 0);
  ADD_OPT(Bool, definebyqueryArgs,"-simple",         "simple print format",0);

  // -listrun
  psMetadata *listrunArgs = psMetadataAlloc();
  ADD_OPT(S64, listrunArgs, "-remote_id",             "run to list", 0);
  ADD_OPT(Str, listrunArgs, "-state",              "run state", NULL);
  ADD_OPT(Str, listrunArgs, "-stage",              "stage to return", NULL);
  ADD_OPT(Str, listrunArgs, "-label",              "remRun label to use", NULL);

  ADD_OPT(S64, listrunArgs, "-job_id",             "job_id", 0);
  //  ADD_OPT(Str, listrunArgs, "-poll_begin",         "last polled time begin", NULL);
  //  ADD_OPT(Str, listrunArgs, "-poll_end",           "last polled time begin", NULL);
  psMetadataAddTime(listrunArgs, PS_LIST_TAIL, "-poll_begin", 0, "last polled time begin", NULL);
  psMetadataAddTime(listrunArgs, PS_LIST_TAIL, "-poll_end", 0, "last polled time end", NULL);

  ADD_OPT(S16, listrunArgs, "-fault",              "fault code", 0);

  ADD_OPT(Bool, listrunArgs, "-simple",            "simple print format", false);
  ADD_OPT(S16, listrunArgs,  "-limit",             "limit to number of runs to display", 0);

  // -doneprep
  psMetadata *doneprepArgs = psMetadataAlloc();
  ADD_OPT(S64, doneprepArgs, "-remote_id",          "run to list", 0);
  ADD_OPT(S64, doneprepArgs, "-job_id",             "job_id", 0);
  ADD_OPT(Str, doneprepArgs, "-stage",              "stage to return", NULL);
  ADD_OPT(Str, doneprepArgs, "-state",              "state to return", NULL);
  ADD_OPT(Str, doneprepArgs, "-label",              "remRun label to use", NULL);

  ADD_OPT(Bool, doneprepArgs, "-simple",            "simple print format", false);
  ADD_OPT(S16, doneprepArgs,  "-limit",             "limit to number of runs to display", 0);

  // -revertcomponent
  psMetadata   *revertcomponentArgs = psMetadataAlloc();
  ADD_OPT(S64,  revertcomponentArgs, "-remote_id",       "run to revert components from", 0);
  ADD_OPT(S64,  revertcomponentArgs, "-stage_id",        "component stage_id to revert", 0);
  
  
  // -listcomponents
  psMetadata   *listcomponentArgs = psMetadataAlloc();
  ADD_OPT(S64,  listcomponentArgs, "-remote_id",         "run to list", 0);
  ADD_OPT(S64,  listcomponentArgs, "-stage_id",          "run to list", 0);
  ADD_OPT(S32,  listcomponentArgs, "-jobs",              "run to list", 0);
  ADD_OPT(Str,  listcomponentArgs, "-state",             "run to list", NULL);
  ADD_OPT(S16,  listcomponentArgs, "-limit",             "limit to number of runs to display", 0);
  ADD_OPT(Bool, listcomponentArgs, "-simple",            "simple print format", false);
  ADD_OPT(Str,  listcomponentArgs, "-label",             "label to use", NULL);

  // -dropcomponent
  psMetadata *dropcomponentArgs = psMetadataAlloc();
  ADD_OPT(S64, dropcomponentArgs, "-remote_id",         "run to update", 0);
  ADD_OPT(S64, dropcomponentArgs, "-stage_id",          "stage_id to remote", 0);

  // -updaterun
  psMetadata *updaterunArgs = psMetadataAlloc();
  ADD_OPT(S64, updaterunArgs, "-remote_id",             "run to update", 0);
  //  ADD_OPT(Str, updaterunArgs, "-label",                  "label to update", NULL);
  ADD_OPT(Str, updaterunArgs, "-set_label",          "remRun label to use", NULL);
  ADD_OPT(Str, updaterunArgs, "-set_state",          "remoteRun state to assign", NULL);
  ADD_OPT(S64, updaterunArgs, "-job_id",             "job_id to set", 0);
  ADD_OPT(S16, updaterunArgs, "-fault",              "fault code", 0);
  //  ADD_OPT(Bool, updaterunArgs, "-poll",              "set poll date", NULL);

  // -updatepoll
  psMetadata *updatepollArgs = psMetadataAlloc();
  ADD_OPT(S64, updatepollArgs, "-remote_id",            "run to update", 0);

  // -updatecomponent
  psMetadata  *updatecomponentArgs = psMetadataAlloc();
  ADD_OPT(S64, updatecomponentArgs, "-remote_id",     "remote_id to update", 0);
  ADD_OPT(S64, updatecomponentArgs, "-stage_id",      "stage_id to update", 0);
  ADD_OPT(Str, updatecomponentArgs, "-state",         "limit to this remoteComponent state", NULL);
  ADD_OPT(Str, updatecomponentArgs, "-set_state",     "remoteComponent state to assign", NULL);
  ADD_OPT(Str, updatecomponentArgs, "-set_path_base", "remoteRun path_base to assign", NULL);
  ADD_OPT(S32, updatecomponentArgs, "-set_jobs",      "number of jobs for this stage_id", 0);

  // -revertrun
  psMetadata *revertrunArgs = psMetadataAlloc();
  ADD_OPT(S64, revertrunArgs, "-remote_id",             "run to revert", 0);
  ADD_OPT(Str, revertrunArgs, "-label",      "remRun label to use", NULL);
  ADD_OPT(S16, revertrunArgs, "-fault",              "fault code", 0);

  // -revertauth
  psMetadata *revertauthArgs = psMetadataAlloc();
  ADD_OPT(S64, revertauthArgs, "-remote_id",             "run to revert", 0);
  ADD_OPT(Str, revertauthArgs, "-label",      "remRun label to use", NULL);

  psMetadata *argSets = psMetadataAlloc();
  psMetadata *modes   = psMetadataAlloc();

  PXOPT_ADD_MODE("-definebyquery",     "", REMOTETOOL_MODE_DEFINEBYQUERY,   definebyqueryArgs);
  PXOPT_ADD_MODE("-listrun",           "", REMOTETOOL_MODE_LISTRUN,         listrunArgs);
  PXOPT_ADD_MODE("-doneprep",          "", REMOTETOOL_MODE_DONEPREP,        doneprepArgs);
  PXOPT_ADD_MODE("-listcomponent",     "", REMOTETOOL_MODE_LISTCOMPONENT,   listcomponentArgs);
  PXOPT_ADD_MODE("-revertcomponent",   "", REMOTETOOL_MODE_REVERTCOMPONENT, revertcomponentArgs);
  PXOPT_ADD_MODE("-dropcomponent",     "", REMOTETOOL_MODE_DROPCOMPONENT,   dropcomponentArgs);
  PXOPT_ADD_MODE("-updaterun",         "", REMOTETOOL_MODE_UPDATERUN,       updaterunArgs);
  PXOPT_ADD_MODE("-updatepoll",        "", REMOTETOOL_MODE_UPDATEPOLL,      updatepollArgs);
  PXOPT_ADD_MODE("-updatecomponent",   "", REMOTETOOL_MODE_UPDATECOMPONENT, updatecomponentArgs);
  PXOPT_ADD_MODE("-revertrun",         "", REMOTETOOL_MODE_REVERTRUN,       revertrunArgs);
  PXOPT_ADD_MODE("-revertauth",        "", REMOTETOOL_MODE_REVERTAUTH,      revertauthArgs);


  if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
    psError(PS_ERR_UNKNOWN, true, "option parsing failed");
    psFree(argSets);
    psFree(modes);
    psFree(config);
    return NULL;
  }

  psFree(argSets);
  psFree(modes);

  // define Database handle, if used
  config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
  if (!config->dbh) {
    psError(PXTOOLS_ERR_SYS, false, "Can't configure database");
    psFree(config);
    return NULL;
  }

  return config;
}


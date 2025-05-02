#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>
#include <psmodules.h>
#include "pxtools.h"
#include "pxdqstats.h"
#include "dqstatstool.h"

pxConfig *dqstatstoolConfig(pxConfig *config, int argc, char **argv) {

  if (!config) {
    config = pxConfigAlloc();
  }

  pmConfigReadParamsSet(false);

  config->modules = pmConfigRead(&argc, argv, "DQSTATS");
  if (!config->modules) {
      psError(psErrorCodeLast(), false, "Can't find site configuration");
      psFree(config);
      return(NULL);
  }
  psTime *now = psTimeGetNow(PS_TIME_TAI);
  // -definebyquery
  psMetadata *definebyqueryArgs = psMetadataAlloc();
  psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-label",          PS_META_DUPLICATE_OK, "search by camRun label", NULL);
  psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-set_registered",  0,                   "time detrend run was registered", now);
  psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_label",       0,                   "set run label", NULL);

  pxdqstatsSetSearchArgs(definebyqueryArgs);

  psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",       0, "do not actually modify the database", false);
  psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",        0, "use the simple output format", false);
  psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-force",         0, "invalidate old entries that match", false);
  psMetadataAddU64(definebyqueryArgs, PS_LIST_TAIL, "-limit",          0, "limit bundle size to N rows", 0);

  // -createbundle
  psMetadata *createbundleArgs = psMetadataAlloc();
  psMetadataAddS64(createbundleArgs, PS_LIST_TAIL, "-dqstats_id",      0, "search by dqstats_id", 0);
  psMetadataAddStr(createbundleArgs, PS_LIST_TAIL, "-uri",             0, "output table URI", 0);

  // -updaterun
  psMetadata *updaterunArgs = psMetadataAlloc();
  psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-dqstats_id",         0, "search by dqstats_id", 0);
  psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",              0, "search by dqstatsRun state", NULL);
  psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label",          0, "set label", NULL);
  psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state",          0, "set dqstatsRun stats", NULL);
  psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-fault",              0, "set fault code", 0);

  // -pendingbundle
  psMetadata *pendingbundleArgs = psMetadataAlloc();
  psMetadataAddS64(pendingbundleArgs, PS_LIST_TAIL, "-dqstats_id",         0, "search by dqstats_id", 0);
  psMetadataAddU64(pendingbundleArgs, PS_LIST_TAIL, "-limit",              0, "limit number of bundles to N", 0);
  psMetadataAddBool(pendingbundleArgs, PS_LIST_TAIL, "-simple",        0, "use the simple output format", false);
  psMetadataAddBool(pendingbundleArgs, PS_LIST_TAIL, "-all",           0, "list everything without restriction", false);
  psMetadataAddStr(pendingbundleArgs, PS_LIST_TAIL, "-label",          0, "search by dqstatsRun.label", NULL);

  // -revertrun
  psMetadata *revertrunArgs = psMetadataAlloc();
  psMetadataAddStr(revertrunArgs, PS_LIST_TAIL, "-label",              0, "search by dqstatsRun.label", NULL);
  psMetadataAddS64(revertrunArgs, PS_LIST_TAIL, "-dqstats_id",         0, "search by dqstats_id", 0);

  psMetadata *argSets = psMetadataAlloc();
  psMetadata *modes = psMetadataAlloc();

  PXOPT_ADD_MODE("-definebyquery",   "create runs from cam stage",       DQSTATS_MODE_DEFINEBYQUERY, definebyqueryArgs);
  PXOPT_ADD_MODE("-updaterun",       "change dqstats run properties",    DQSTATS_MODE_UPDATERUN,     updaterunArgs);
  PXOPT_ADD_MODE("-createbundle",    "create the bundle for a run",      DQSTATS_MODE_CREATEBUNDLE,  createbundleArgs);
  PXOPT_ADD_MODE("-pendingbundle",   "list bundles needing creation",    DQSTATS_MODE_PENDINGBUNDLE, pendingbundleArgs);
  PXOPT_ADD_MODE("-revertrun",       "revert runs with errors",          DQSTATS_MODE_REVERTRUN,     revertrunArgs);

  if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
    psError(PS_ERR_UNKNOWN, false, "option parsing failed");
    psFree(argSets);
    psFree(modes);
    psFree(config);
    return(NULL);
  }

  psFree(argSets);
  psFree(modes);

  // define Database handle, if used
  config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
  if (!config->dbh) {
    psError(PS_ERR_UNKNOWN, false, "Can't configure database");
    psFree(config);
    return(NULL);
  }

  //  config = psMetadataConfigRead(config,1,"dqstatsTool.config",1);

  return config;
}





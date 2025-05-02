/*
 * laptoolConfig.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>

#include <psmodules.h>
#include "pxtools.h"
#include "laptool.h"

#define ADD_OPT(TYPE,TARG,NAME,COMMENT,DEFAULT) psMetadataAdd##TYPE(TARG, PS_LIST_TAIL, NAME, 0, COMMENT, DEFAULT)

pxConfig *laptoolConfig(pxConfig *config, int argc, char **argv)
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

  // -definesequence
  psMetadata *definesequenceArgs = psMetadataAlloc();
  ADD_OPT(Str, definesequenceArgs, "-name",                   "short name for this LAP sequence (required)", NULL);
  ADD_OPT(Str, definesequenceArgs, "-description",            "define the description for this LAP sequence (required)", NULL);
  ADD_OPT(Bool,definesequenceArgs, "-simple",                 "use the simple output format", false);
  
  // -listsequence
  psMetadata *listsequenceArgs = psMetadataAlloc();
  ADD_OPT(S64, listsequenceArgs, "-seq_id",                   "search by LAP sequence ID", 0);
  ADD_OPT(Str, listsequenceArgs, "-seq_name",                 "search by LAP sequence name", 0);
  ADD_OPT(Bool,listsequenceArgs, "-simple",                   "use the simple output format", false);
  ADD_OPT(U64, listsequenceArgs, "-limit",                    "limit result set to N items", 0);

  
  // -definerun
  psMetadata *definerunArgs = psMetadataAlloc();
  ADD_OPT(S64, definerunArgs, "-seq_id",                      "define the LAP sequence for this run (required)", 0);
  ADD_OPT(Str, definerunArgs, "-projection_cell",             "define the projection cell for this run (required)", NULL);
  ADD_OPT(Str, definerunArgs, "-tess_id",                     "define the tessellation used (required)", NULL);
  ADD_OPT(F64, definerunArgs, "-ra",                          "define RA center (required)", NAN);
  ADD_OPT(F64, definerunArgs, "-decl",                        "define DEC center (required)", NAN);
  ADD_OPT(F32, definerunArgs, "-radius",                      "define radius from center to consider (required)", NAN);
  ADD_OPT(Str, definerunArgs, "-filter",                      "define the filter used (required)", NULL);
  ADD_OPT(Str, definerunArgs, "-label",                       "define the label used", NULL);
  ADD_OPT(Str, definerunArgs, "-dist_group",                  "define the distribution group for this data", NULL);
  
  psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-obsmode", PS_META_DUPLICATE_OK, "search by obsmode", NULL);
  ADD_OPT(Bool,definerunArgs, "-all_obsmode",                 "use all science obsmodes", false);
  
  ADD_OPT(Bool,definerunArgs, "-simple",                      "use the simple output format", false);

  // -definerunbyrelease
  psMetadata *definerunbyreleaseArgs = psMetadataAlloc();
  ADD_OPT(S64, definerunbyreleaseArgs, "-seq_id",             "define the LAP sequence for this run (required)", 0);
  ADD_OPT(Str, definerunbyreleaseArgs, "-projection_cell",    "define the projection cell for this run (required)", NULL);
  ADD_OPT(Str, definerunbyreleaseArgs, "-tess_id",            "define the tessellation used (required)", NULL);
  ADD_OPT(Str, definerunbyreleaseArgs, "-filter",             "define the filter used (required)", NULL);
  ADD_OPT(Str, definerunbyreleaseArgs, "-label",              "define the label used", NULL);
  ADD_OPT(Str, definerunbyreleaseArgs, "-dist_group",         "define the distribution group for this data", NULL);
  ADD_OPT(S64, definerunbyreleaseArgs, "-rel_id",             "define the release to copy", 0);
  ADD_OPT(Str, definerunbyreleaseArgs, "-release_name",       "define the release to copy", NULL);
  
  ADD_OPT(Bool, definerunbyreleaseArgs, "-simple",            "use the simple output format", false);
  psMetadataAddTime(definerunbyreleaseArgs, PS_LIST_TAIL, "-dateobs_begin",      0, "search for exposures by time (>=)", NULL);
  psMetadataAddTime(definerunbyreleaseArgs, PS_LIST_TAIL, "-dateobs_end",        0, "search for exposures by time (<=)", NULL);
  psMetadataAddF64(definerunbyreleaseArgs,  PS_LIST_TAIL, "-ra_min",             0, "search by min RA (degrees) ", NAN);
  psMetadataAddF64(definerunbyreleaseArgs,  PS_LIST_TAIL, "-ra_max",             0, "search by max RA (degrees) ", NAN);
  psMetadataAddF64(definerunbyreleaseArgs,  PS_LIST_TAIL, "-decl_min",           0, "search by min DEC (degrees)", NAN);
  psMetadataAddF64(definerunbyreleaseArgs,  PS_LIST_TAIL, "-decl_max",           0, "search by max DEC (degrees)", NAN);
  
  // -pendingrun
  psMetadata *pendingrunArgs = psMetadataAlloc();
  ADD_OPT(S64, pendingrunArgs, "-seq_id",                     "search by LAP sequence ID", 0);
  ADD_OPT(S64, pendingrunArgs, "-lap_id",                     "search by LAP run ID", 0);
  ADD_OPT(Str, pendingrunArgs, "-projection_cell",            "search by projection cell", NULL);
  ADD_OPT(Str, pendingrunArgs, "-filter",                     "search by filter", NULL);
  //  ADD_OPT(Str, pendingrunArgs, "-label",                      "search by LAP run label", NULL);
  psMetadataAddStr(pendingrunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by LAP run label", NULL);
  ADD_OPT(Str, pendingrunArgs, "-state",                      "search by LAP run state", NULL);
  ADD_OPT(Str, pendingrunArgs, "-fault",                      "search by LAP run fault", NULL);
  ADD_OPT(Bool,pendingrunArgs, "-simple",                     "use the simple output format", false);
  ADD_OPT(U64, pendingrunArgs, "-limit",                      "limit result set to N items", 0);

  // -listrun
  psMetadata *listrunArgs = psMetadataAlloc();
  ADD_OPT(S64, listrunArgs, "-seq_id",                     "search by LAP sequence ID", 0);
  ADD_OPT(S64, listrunArgs, "-lap_id",                     "search by LAP run ID", 0);
  ADD_OPT(Str, listrunArgs, "-projection_cell",            "search by projection cell", NULL);
  ADD_OPT(Str, listrunArgs, "-filter",                     "search by filter", NULL);
  //  ADD_OPT(Str, listrunArgs, "-label",                      "search by LAP run label", NULL);
  psMetadataAddStr(listrunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by LAP run label", NULL);
  ADD_OPT(Str, listrunArgs, "-state",                      "search by LAP run state", NULL);
  ADD_OPT(Str, listrunArgs, "-fault",                      "search by LAP run fault", NULL);
  ADD_OPT(Bool,listrunArgs, "-simple",                     "use the simple output format", false);
  ADD_OPT(U64, listrunArgs, "-limit",                      "limit result set to N items", 0);

  // -updaterun
  psMetadata *updaterunArgs = psMetadataAlloc();
  ADD_OPT(S64, updaterunArgs, "-lap_id",                      "search by lap run ID", 0);
  ADD_OPT(Str, updaterunArgs, "-set_state",                   "set state", NULL);
  ADD_OPT(S16, updaterunArgs, "-fault",                       "set fault code", INT16_MAX);
  ADD_OPT(Str, updaterunArgs, "-set_label",                   "set label", NULL);
  ADD_OPT(S64, updaterunArgs, "-set_quick_sass_id",           "set quick stack sass_id", 0);
  ADD_OPT(S64, updaterunArgs, "-set_final_sass_id",           "set final stack sass_id", 0);
  
  // -revertsass
  psMetadata *revertsassArgs = psMetadataAlloc();
  ADD_OPT(S64, revertsassArgs, "-lap_id",                     "search by lap run id", 0);
  ADD_OPT(S64, revertsassArgs, "-sass_id",                    "sass id that is being removed", 0);

  // -pendingexp
  psMetadata *pendingexpArgs = psMetadataAlloc();
  ADD_OPT(S64, pendingexpArgs, "-lap_id",                     "lap run ID", 0);
  ADD_OPT(Str, pendingexpArgs, "-projection_cell",            "projection cell to consider", NULL);
  ADD_OPT(U64, pendingexpArgs, "-limit",                      "limit result set to N items", 0);
  ADD_OPT(Bool,pendingexpArgs, "-simple",                     "use the simple output format", false);

  // -exposures
  psMetadata *exposuresArgs = psMetadataAlloc();
  ADD_OPT(S64, exposuresArgs, "-lap_id",                      "search by lap run ID", 0);
  ADD_OPT(S64, exposuresArgs, "-exp_id",                      "search by exp_id", 0);
  ADD_OPT(Bool,exposuresArgs, "-simple",                      "use the simple output format", false);
  ADD_OPT(U64, exposuresArgs, "-limit",                       "limit result set to N items", 0);

  // -stacks
  psMetadata *stacksArgs = psMetadataAlloc();
  ADD_OPT(S64, stacksArgs, "-lap_id",                         "search by lap run ID", 0);
  ADD_OPT(Bool,stacksArgs, "-simple",                         "use the simple output format", false);
  ADD_OPT(U64, stacksArgs, "-limit",                          "limit result set to N items", 0);
	  
  // -updateexp
  psMetadata *updateexpArgs = psMetadataAlloc();
  ADD_OPT(S64, updateexpArgs, "-lap_id",                      "search by lap run ID", 0);
  ADD_OPT(S64, updateexpArgs, "-exp_id",                      "search by exposure ID", 0);
  ADD_OPT(S64, updateexpArgs, "-chip_id",                     "search by chip ID", 0);
  ADD_OPT(S64, updateexpArgs, "-set_chip_id",                 "set the chip ID", 0);
  ADD_OPT(S64, updateexpArgs, "-set_pair_id",                 "set the pair ID", 0);
  ADD_OPT(Str, updateexpArgs, "-set_data_state",              "set the lapExp data_state", NULL);
  ADD_OPT(Bool,updateexpArgs, "-private",                     "set this exposure as private", 0);
  ADD_OPT(Bool,updateexpArgs, "-public",                      "set this exposure as public", 0);
  ADD_OPT(Bool,updateexpArgs, "-pairwise",                    "set this exposure to be pairwise", 0);
  ADD_OPT(Bool,updateexpArgs, "-nopairwise",                  "set this exposure to not be pairwise", 0);
  ADD_OPT(Bool,updateexpArgs, "-active",                      "set this exposure to active", 0);
  ADD_OPT(Bool,updateexpArgs, "-inactive",                    "set this exposure to active", 0);

  // -diffcheck
  psMetadata *diffcheckArgs = psMetadataAlloc();
  ADD_OPT(S64, diffcheckArgs, "-lap_id",                      "search by lap run ID", 0);
  ADD_OPT(S64, diffcheckArgs, "-exp_id",                      "search by exposure ID", 0);
  ADD_OPT(S64, diffcheckArgs, "-chip_id",                     "search by chip ID", 0);
  ADD_OPT(S64, diffcheckArgs, "-warp_id",                     "search by warp ID", 0);
  ADD_OPT(S64, diffcheckArgs, "-seq_id",                      "search by lap sequence ID", 0);
  ADD_OPT(Str, diffcheckArgs, "-skycell_id",                  "search by skycell ID", 0);
  ADD_OPT(Bool, diffcheckArgs, "-simple",                     "use simple output format", 0);
  
  // -inactiveexp
  psMetadata *inactiveexpArgs = psMetadataAlloc();
  ADD_OPT(S64, inactiveexpArgs, "-lap_id",                    "search by lap run ID", 0);
  ADD_OPT(Bool,inactiveexpArgs, "-simple",                    "use the simple output format", false);
  ADD_OPT(U64, inactiveexpArgs, "-limit",                     "limit result set to N items", 0);
  
  // -definegroup
  psMetadata *definegroupArgs = psMetadataAlloc();
  ADD_OPT(S64, definegroupArgs, "-seq_id",                    "search by lap sequence ID (required)", 0);
  ADD_OPT(Str, definegroupArgs, "-set_label",                 "set label (required)", 0);

  psMetadataAddStr(definegroupArgs,  PS_LIST_TAIL, 
                                "-filter", PS_META_DUPLICATE_OK, 
                                                              "search by filter (LIKE comparison, multiple OK)", NULL);
  ADD_OPT(Bool,definegroupArgs, "-pretend",                   "do not actuallym modify the database", false);
  ADD_OPT(Bool,definegroupArgs, "-simple",                    "use the simple output format", false);
  ADD_OPT(U64, definegroupArgs, "-limit",                     "limit result set to N items", 0);

  // -pendinggroup
  psMetadata *pendinggroupArgs = psMetadataAlloc();
  psMetadataAddStr(pendinggroupArgs,  PS_LIST_TAIL, 
                                "-label", PS_META_DUPLICATE_OK, 
                                                              "search by label (multiple OK)", NULL);
  ADD_OPT(S64, pendinggroupArgs, "-seq_id",                    "search by lap sequence ID (required)", 0);

  ADD_OPT(Bool,pendinggroupArgs, "-simple",                    "use the simple output format", false);
  ADD_OPT(U64, pendinggroupArgs, "-limit",                     "limit result set to N items", 0);

  // -updategroup
  psMetadata *updategroupArgs = psMetadataAlloc();
  ADD_OPT(S64, updategroupArgs, "-seq_id",                    "search by lap sequence ID (required)", 0);
  ADD_OPT(Str, updategroupArgs, "-tess_id",                   "search by tess_id (required)", 0);
  ADD_OPT(Str, updategroupArgs, "-projection_cell",           "search by projection_cell (required)", 0);
  ADD_OPT(Str, updategroupArgs, "-set_state",                 "set state", 0);
  ADD_OPT(S16, updategroupArgs, "-set_fault",                 "set fault code", INT16_MAX);
  ADD_OPT(Str, updategroupArgs, "-set_label",                 "set label", 0);

  // -revertgroup
  psMetadata *revertgroupArgs = psMetadataAlloc();
  psMetadataAddStr(revertgroupArgs,  PS_LIST_TAIL, 
                                "-label", PS_META_DUPLICATE_OK, 
                                                              "search by label (multiple OK)", NULL);
  ADD_OPT(S64, revertgroupArgs, "-seq_id",                    "search by lap sequence ID", 0);
  ADD_OPT(Str, revertgroupArgs, "-tess_id",                   "search by tess_id", 0);
  ADD_OPT(Str, revertgroupArgs, "-projection_cell",           "search by projection_cell", 0);
  ADD_OPT(S16, revertgroupArgs, "-fault",                     "fault code", INT16_MAX);

  // -filtersforgroup
  psMetadata *filtersforgroupArgs = psMetadataAlloc();
  ADD_OPT(S64, filtersforgroupArgs, "-seq_id",                    "search by lap sequence ID (required)", 0);
  ADD_OPT(Str, filtersforgroupArgs, "-tess_id",                   "search by tess_id (required)", 0);
  ADD_OPT(Str, filtersforgroupArgs, "-projection_cell",           "search by projection_cell (required)", 0);
  ADD_OPT(Bool,filtersforgroupArgs, "-simple",                    "use the simple output format", false);

  
  psMetadata *argSets = psMetadataAlloc();
  psMetadata *modes = psMetadataAlloc();
  
  PXOPT_ADD_MODE("-definesequence",          "", LAPTOOL_MODE_DEFINESEQUENCE,   definesequenceArgs);
  PXOPT_ADD_MODE("-listsequence",            "", LAPTOOL_MODE_LISTSEQUENCE,     listsequenceArgs);
  PXOPT_ADD_MODE("-definerun",               "", LAPTOOL_MODE_DEFINERUN,        definerunArgs);
  PXOPT_ADD_MODE("-definerunbyrelease",      "", LAPTOOL_MODE_DEFINERUNBYRELEASE, definerunbyreleaseArgs);
  PXOPT_ADD_MODE("-pendingrun",              "", LAPTOOL_MODE_PENDINGRUN,       pendingrunArgs);
  PXOPT_ADD_MODE("-listrun",                 "", LAPTOOL_MODE_PENDINGRUN,       listrunArgs);
  PXOPT_ADD_MODE("-updaterun",               "", LAPTOOL_MODE_UPDATERUN,        updaterunArgs);
  PXOPT_ADD_MODE("-revertsass",              "mode to remove the sass_id", LAPTOOL_MODE_REVERTSASS,   revertsassArgs);
  PXOPT_ADD_MODE("-pendingexp",              "", LAPTOOL_MODE_PENDINGEXP,       pendingexpArgs);
  PXOPT_ADD_MODE("-exposures",               "", LAPTOOL_MODE_EXPOSURES,        exposuresArgs);
  PXOPT_ADD_MODE("-stacks",                  "", LAPTOOL_MODE_STACKS,           stacksArgs);
  PXOPT_ADD_MODE("-updateexp",               "", LAPTOOL_MODE_UPDATEEXP,        updateexpArgs);
  PXOPT_ADD_MODE("-diffcheck",               "", LAPTOOL_MODE_DIFFCHECK,        diffcheckArgs);
  PXOPT_ADD_MODE("-inactiveexp",             "", LAPTOOL_MODE_INACTIVEEXP,      inactiveexpArgs);
  PXOPT_ADD_MODE("-definegroup",             "", LAPTOOL_MODE_DEFINEGROUP,      definegroupArgs);
  PXOPT_ADD_MODE("-pendinggroup",            "", LAPTOOL_MODE_PENDINGGROUP,     pendinggroupArgs);
  PXOPT_ADD_MODE("-filtersforgroup",         "", LAPTOOL_MODE_FILTERSFORGROUP,  filtersforgroupArgs);
  PXOPT_ADD_MODE("-updategroup",             "", LAPTOOL_MODE_UPDATEGROUP,      updategroupArgs);
  PXOPT_ADD_MODE("-revertgroup",             "", LAPTOOL_MODE_REVERTGROUP,      revertgroupArgs);
  
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


  

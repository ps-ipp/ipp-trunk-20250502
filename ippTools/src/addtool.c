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
#include "addtool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool pendingexpMode(pxConfig *config);
static bool addprocessedexpMode(pxConfig *config);
static bool processedexpMode(pxConfig *config);
static bool revertprocessedexpMode(pxConfig *config);
static bool updateprocessedexpMode(pxConfig *config);
static bool blockMode(pxConfig *config);
static bool maskedMode(pxConfig *config);
static bool unblockMode(pxConfig *config);
static bool addminidvodbrunMode(pxConfig *config);
static bool updateminidvodbrunMode(pxConfig *config);
static bool listminidvodbrunMode(pxConfig *config);
static bool flipminidvodbrunMode(pxConfig *config);
static bool checkminidvodbrunaddrunMode(pxConfig *config);
static bool addminidvodbprocessedMode(pxConfig *config);
static bool listminidvodbprocessedMode(pxConfig *config);
static bool revertminidvodbprocessedMode(pxConfig *config);
static bool updateminidvodbprocessedMode(pxConfig *config);



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
        MODECASE(ADDTOOL_MODE_UPDATERUN,            updaterunMode);
        MODECASE(ADDTOOL_MODE_PENDINGEXP,           pendingexpMode);
        MODECASE(ADDTOOL_MODE_ADDPROCESSEDEXP,      addprocessedexpMode);
        MODECASE(ADDTOOL_MODE_PROCESSEDEXP,         processedexpMode);
        MODECASE(ADDTOOL_MODE_REVERTPROCESSEDEXP,   revertprocessedexpMode);
        MODECASE(ADDTOOL_MODE_UPDATEPROCESSEDEXP,   updateprocessedexpMode);
        MODECASE(ADDTOOL_MODE_BLOCK,                blockMode);
        MODECASE(ADDTOOL_MODE_MASKED,               maskedMode);
        MODECASE(ADDTOOL_MODE_UNBLOCK,              unblockMode);
        MODECASE(ADDTOOL_MODE_ADDMINIDVODBRUN,      addminidvodbrunMode);
        MODECASE(ADDTOOL_MODE_UPDATEMINIDVODBRUN,   updateminidvodbrunMode);
        MODECASE(ADDTOOL_MODE_LISTMINIDVODBRUN,     listminidvodbrunMode);
        MODECASE(ADDTOOL_MODE_FLIPMINIDVODBRUN,     flipminidvodbrunMode);
        MODECASE(ADDTOOL_MODE_CHECKMINIDVODBRUNADDRUN, checkminidvodbrunaddrunMode);
        MODECASE(ADDTOOL_MODE_ADDMINIDVODBPROCESSED,addminidvodbprocessedMode);
        MODECASE(ADDTOOL_MODE_LISTMINIDVODBPROCESSED,listminidvodbprocessedMode);
        MODECASE(ADDTOOL_MODE_REVERTMINIDVODBPROCESSED,revertminidvodbprocessedMode);
        MODECASE(ADDTOOL_MODE_UPDATEMINIDVODBPROCESSED,updateminidvodbprocessedMode);

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
    //  pxcamGetSearchArgs (config, where);
    PXOPT_COPY_S64(config->args, where,  "-cam_id",    "camRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where,  "-stack_id",    "stackRun.stack_id", "==");
    PXOPT_COPY_S64(config->args, where,  "-sky_id",    "staticskyRun.sky_id", "==");
    PXOPT_COPY_S64(config->args, where,  "-skycal_id",  "skycalRun.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where,  "-diff_id",  "diffRun.diff_id", "==");
    PXOPT_COPY_S64(config->args, where,  "-ff_id",  "fullForceRun.ff_id", "==");
        
    // PXOPT_LOOKUP_STR(stage,       config->args, "-stage", false, false);
    PXOPT_LOOKUP_STR(workdir,     config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(dvodb,       config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group,  config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(reduction,   config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(note,        config->args, "-set_note", false, false);
    PXOPT_LOOKUP_STR(minidvodb_name,  config->args, "-set_minidvodb_name", false, false);
    PXOPT_LOOKUP_STR(minidvodb_group, config->args, "-set_minidvodb_group", false, false);
    PXOPT_LOOKUP_STR(minidvodb_host,  config->args, "-set_minidvodb_host",  false, false);
    PXOPT_LOOKUP_BOOL(image_only, config->args, "-image_only", false);
    PXOPT_LOOKUP_BOOL(minidvodb,  config->args, "-set_minidvodb", false);
    PXOPT_LOOKUP_BOOL(destreaked, config->args, "-destreaked", false);
    PXOPT_LOOKUP_BOOL(uncensored, config->args, "-uncensored", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(addrand, config->args,  "-addrand", false);

    PXOPT_LOOKUP_BOOL(pretend,    config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple,     config->args, "-simple", false);
   
    PXOPT_LOOKUP_F32(minra,     config->args, "-set_minra", false, false);
    PXOPT_LOOKUP_F32(maxra,     config->args, "-set_maxra", false, false);
    PXOPT_LOOKUP_F32(mindec,     config->args, "-set_mindec", false, false);
    PXOPT_LOOKUP_F32(maxdec,     config->args, "-set_maxdec", false, false);

    // Handle stages
    PXOPT_LOOKUP_STR(stage,       config->args, "-stage", false, false);
    if (strcmp(stage, "cam") == 0) {
      pxcamGetSearchArgs (config, where);
      pxAddLabelSearchArgs (config, where, "-label",     "camRun.label", "=="); // define using camRun label
      pxAddLabelSearchArgs (config, where, "-data_group","camRun.data_group", "=="); // define using camRun label
      pxAddLabelSearchArgs (config, where,     "-filter",   "rawExp.filter", "LIKE"); // define using camRun label
      PXOPT_COPY_STR(config->args, where,  "-reduction", "camRun.reduction", "==");
    }
    else if (strcmp(stage, "stack") == 0) {
      pxAddLabelSearchArgs (config, where, "-label",     "stackRun.label", "=="); // define using camRun label
      pxAddLabelSearchArgs (config, where, "-data_group","stackRun.data_group", "=="); // define using camRun label
      PXOPT_COPY_STR(config->args, where,  "-reduction", "stackRun.reduction", "==");
      pxAddLabelSearchArgs (config, where, "-filter",     "stackRun.filter", "=="); // define using camRun label
    }
    else if (strcmp(stage, "staticsky") == 0) {
      pxAddLabelSearchArgs (config, where, "-label",     "staticskyRun.label", "=="); // define using camRun label
      pxAddLabelSearchArgs (config, where, "-data_group","staticskyRun.data_group", "=="); // define using camRun label
      PXOPT_COPY_STR(config->args, where,  "-reduction", "staticskyRun.reduction", "==");
      //no filter here
    }
    else if (strcmp(stage, "skycal") == 0) {
      pxAddLabelSearchArgs (config, where, "-label",     "skycalRun.label", "=="); //define using skycalRun label
      pxAddLabelSearchArgs (config, where, "-data_group","skycalRun.data_group", "==");
      PXOPT_COPY_STR(config->args, where,  "-reduction", "skycalRun.reduction",  "==");
    }
    else if (strcmp(stage, "diff") == 0) {
      pxAddLabelSearchArgs (config, where, "-label",     "diffRun.label", "=="); //define using diffRun label
      pxAddLabelSearchArgs (config, where, "-data_group","diffRun.data_group", "==");
      PXOPT_COPY_STR(config->args, where,  "-reduction", "diffRun.reduction",  "==");
    }
    else if (strcmp(stage, "fullforce") == 0) {
      pxAddLabelSearchArgs (config, where, "-label",     "fullForceRun.label", "=="); //define using fullForceRun label
      pxAddLabelSearchArgs (config, where, "-data_group","fullForceRun.data_group", "==");
      PXOPT_COPY_STR(config->args, where,  "-reduction", "fullForceRun.reduction",  "==");
    }
    else if (strcmp(stage, "fullforce_summary")==0) {
      //should be nearly identical to fullforce (uses the same tables)
      pxAddLabelSearchArgs (config, where, "-label",     "fullForceRun.label", "=="); 
      pxAddLabelSearchArgs (config, where, "-data_group","fullForceRun.data_group", "==");
      PXOPT_COPY_STR(config->args, where,  "-reduction", "fullForceRun.reduction",  "==");
    }
 
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
   
    // Select either destreaked or uncensored data.  CZW: Should this be defaulted to uncensored?
    if (!(destreaked && uncensored) && (destreaked || uncensored)) {
      //if one of destreaked and uncensored is true, but not both
      if (destreaked) {
	psTrace("addtool.c", PS_LOG_INFO, "using -destreaked\n");
      } else {
	psTrace("addtool.c", PS_LOG_INFO, "using -unscensored\n");
      }
    } else {
      if (destreaked && uncensored) {
	psError(PS_ERR_UNKNOWN, false, "Both -destreaked and -uncensored are selected. Please use one or the other, not both, \n");
	return false;
      }
      if (!destreaked && !uncensored) {
	psError(PS_ERR_UNKNOWN, false, "Neither -destreaked or -uncensored are selected. Please use one.\n");
	return false;
      }
    }
    if ((strcmp(stage,"diff") == 0) ||
	(strcmp(stage,"fullforce") == 0) ||
	(strcmp(stage,"fullforce_summary") == 0)) {
      if (! (dvodb)) {
	psError(PS_ERR_UNKNOWN, false, "The SQL is not correctly written to handle this case (-set_dvodb not specified), so stopping here.");
	return(false);
      }
    }
    
    
    // prevent queueing an addRun if a given exposure has already been added to
    // the given dvo database
    psString dvodb_string = NULL;
    psString bare_query = NULL;
    
    if (strcmp(stage,"cam") == 0 ) {
      if (dvodb) {
	psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_cam_id_dvo.sql\n%s\n", dvodb,stage);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_cam_id_dvo.sql");
	// user supplied dvodb
	psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
	psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_find_cam_id.sql\n%s\n",stage);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_cam_id.sql");
        // inherit dvodb from camRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(camRun.dvodb IS NOT NULL AND previous_dvodb = camRun.dvodb)");
      }
    }

    else if (strcmp(stage,"stack") == 0) {
      if (dvodb) {
	psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_stack_id_dvo.sql\n%s\n", dvodb,stage);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_stack_id_dvo.sql");
	// user supplied dvodb
	psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
	psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_find_stack_id.sql\n%s\n",stage);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_stack_id.sql");
        // inherit dvodb from camRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(stackRun.dvodb IS NOT NULL AND previous_dvodb = stackRun.dvodb)");
      }
    }

    else if (strcmp(stage,"staticsky") == 0) {
      if (dvodb ) {
	psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_sky_id_multi_dvo.sql\n%s\n", dvodb,stage);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_sky_id_multi_dvo.sql");
	// user supplied dvodb
	psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
	psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_find_sky_id_multi.sql\n%s\n",stage);
        // find the cam_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_sky_id_multi.sql");
        // inherit dvodb from camRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(staticskyRun.dvodb IS NOT NULL AND previous_dvodb = staticskyRun.dvodb)");
      }
    }

    else if (strcmp(stage,"skycal") == 0) {
      if (dvodb) {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_skycal_id_dvo.sql\n%s\n", dvodb,stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_skycal_id_dvo.sql");
        // user supplied dvodb
        psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_find_skycal_id.sql\n%s\n",stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_skycal_id.sql");
        // inherit dvodb from skycalRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(skycalRun.dvodb IS NOT NULL AND previous_dvodb = skycalRun.dvodb)");
	// this is silly, there is no dvodb in skycalRun...?
      }
    }

    else if (strcmp(stage,"diff") == 0) {
      if (dvodb) {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_diff_id_dvo.sql\n%s\n", dvodb,stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_diff_id_dvo.sql");
        // user supplied dvodb
        psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_find_diff_id.sql\n%s\n",stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_diff_id.sql");
        // inherit dvodb from skycalRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(diffRun.dvodb IS NOT NULL AND previous_dvodb = diffRun.dvodb)");
	// this is silly, there is no dvodb in skycalRun...?
      }
    }

    else if (strcmp(stage,"fullforce") == 0) {
      if (dvodb) {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_ff_id_dvo.sql\n%s\n", dvodb,stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_ff_id_dvo.sql");
        // user supplied dvodb
        psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_ff_id.sql\n%s\n",stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_ff_id.sql");
        // inherit dvodb from skycalRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(fullForceRun.dvodb IS NOT NULL AND previous_dvodb = fullForceRun.dvodb)");
	// this is silly, there is no dvodb in skycalRun...?
      }
    }
    else if (strcmp(stage,"fullforce_summary") == 0) {
      if (dvodb) {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument found (%s) using addtool_find_ffsummary_id_dvo.sql\n%s\n", dvodb,stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_ffsummary_id_dvo.sql");
        // user supplied dvodb
        psStringAppend(&dvodb_string, "addRun.dvodb = '%s'", dvodb);
      } else {
        psTrace("addtool.c", PS_LOG_INFO, "dvodb argument not found using addtool_ffsummary_id.sql\n%s\n",stage);
        // find the skycal_id of all the exposures that we want to queue up.
        bare_query = pxDataGet("addtool_find_ffsummary_id.sql");
        // inherit dvodb from skycalRun, avoid matching NULL
        psStringAppend(&dvodb_string, "(fullForceRun.dvodb IS NOT NULL AND previous_dvodb = fullForceRun.dvodb)");
	// this is silly, there is no dvodb in skycalRun...?
      }
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

	if ((strcmp(stage,"diff") == 0)||
	    (strcmp(stage,"fullforce") == 0)||
	    (strcmp(stage,"fullforce_summary") == 0)) {
	  //diff needs the ra /deg stuff
	  //ff needs the ra /deg stuff
	  //ff summary needs the ra /deg stuff
	  psStringAppend(&query, " AND radeg >= %f", minra);
	  psStringAppend(&query, " AND radeg <= %f", maxra);
	  psStringAppend(&query, " AND decdeg >= %f", mindec);
	  psStringAppend(&query, " AND decdeg <= %f", maxdec);
	}

        psFree(whereClause);
    } else {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        return false;
    }
    psFree(where);

    if (destreaked) {
      //This picks the magicked/censored ones
      if ((strcmp(stage,"cam") == 0)||
	  (strcmp(stage,"stack") == 0)) { 
        psStringAppend(&query, " AND (camRun.magicked > 0)");
      }
      // staticSky/skycal have no magicked column.
    } else {
      //This picks only the unmagicked/uncensored ones
      //if (strcmp(stage,"cam") == 0) {
	//we can now properly handle the magicked case in ippScripts, so we queue camRuns in any magic state now if -uncensored.
        //psStringAppend(&query, " AND (camRun.magicked = 0)");
	//}
      if (strcmp(stage,"stack") == 0) {
	psStringAppend(&query, " AND (stackRun.magicked = 0)");
      }
    }

    // if we grab a group of camRun/stackRuns that have multiple camRun/stackRun pointing to the same exp_id/stack_id, and that exp_id/stack_id has never before been added to addRun, we need to group by exp_id/stack_id to ensure that only 1 of the camRun/stackRun for that exp_id is added to the addRun stage.
    
    if (strcmp(stage,"cam") == 0) {
      psStringAppend(&query, " GROUP BY exp_id"); 
    }
    else if (strcmp(stage,"stack") == 0) {
      psStringAppend(&query, " GROUP BY stack_id");
    }
    else if (strcmp(stage,"staticsky") == 0) {
      psStringAppend(&query, " GROUP BY sky_id, stack_id");  //some reason it needs this
    }
    else if (strcmp(stage,"skycal") == 0) {
      psStringAppend(&query, " GROUP BY skycal_id, sky_id, stack_id ");  //this needs checking, but I think it shoul be fine? it groups by lots of stuff (including stack - we only want one of each stack in there
    }
    //needs to be checked HAF xxx
    else if (strcmp(stage,"diff") == 0) {
      psStringAppend(&query, " GROUP BY diff_id, diff_skyfile_id ");  //this needs checking, but I think it shoul be fine? it groups by lots of stuff (including stack - we only want one of each stack in there
    }
    //needs to be checked HAF xxx
    else if (strcmp(stage,"fullforce") == 0) {
      psStringAppend(&query, " GROUP BY ff_id, warp_id ");  //this needs checking, but I think it shoul be fine? it groups by lots of stuff (including stack - we only want one of each stack in there
    }
    else if (strcmp(stage,"fullforce_summary") == 0) {
      psStringAppend(&query, " GROUP BY ff_id ");  //needs to be checked, but should be fine: want 1 ff summary cmf.
    }

    // random order of pending files
    if (addrand) {
      psStringAppend(&query, " order by rand()");
    }

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    psError(PS_ERR_UNKNOWN, false, "query: %s\n", query);

    psTrace("addtool.c", PS_LOG_INFO,"query: \n\n%s\n\n",query);

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
    
    // loop over our list of camRun rows to check the supplied and selected dvodb and workdir values:
    if (strcmp(stage,"cam") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	
	camRunRow *row = camRunObjectFromMetadata(md);
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
	  psFree(output);
	  return false;
        }
	
        if (!dvodb && !row->dvodb) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, cam_id %" PRId64, row->label, row->cam_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, cam_id %" PRId64, row->label, row->cam_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
    }
    else if (strcmp(stage,"stack") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	
	stackRunRow *row = stackRunObjectFromMetadata(md);
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
	  psFree(output);
	  return false;
        }
	
        if (!dvodb && !row->dvodb) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, stack_id %" PRId64, row->label, row->stack_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, stack_id %" PRId64, row->label, row->stack_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
    }
    else if (strcmp(stage,"staticsky") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	bool status = false;
	psS32 num_inputs = psMetadataLookupS32(&status, md, "num_inputs");
	if (!status) {
	  psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item num_inputs");
	  return false;
	}
	if (num_inputs < 0) {
	  psError(PS_ERR_UNKNOWN, true, "invalid value for num_inputs");
	  return false;
	}
	
	staticskyRunRow *row = staticskyRunObjectFromMetadata(md);
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
	  psFree(output);
	  return false;
        }
	
        if (!dvodb) {  //there's no staticsky.dvodb
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, sky_id %" PRId64, row->label, row->sky_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, sky_id %" PRId64, row->label, row->sky_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
    }
    else if (strcmp(stage,"skycal") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	
        skycalRunRow *row = skycalRunObjectFromMetadata(md);

        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into skycalRun");
	  psFree(output);
	  return false;
        }
	
	if (!dvodb) {  // there's no skycalRun.dvodb
 	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, skycal_id %" PRId64, row->label, row->skycal_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, skycal_id %" PRId64, row->label, row->skycal_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
    }
    else if (strcmp(stage,"diff") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	
        diffRunRow *row = diffRunObjectFromMetadata(md);
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into diffRun");
	  psFree(output);
	  return false;
        }
	
	if (!dvodb) {  // there's no skycalRun.dvodb
 	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, diff_id %" PRId64, row->label, row->diff_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, diff_id %" PRId64, row->label, row->diff_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
    }
    else if (strcmp(stage,"fullforce") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	
        fullForceRunRow *row = fullForceRunObjectFromMetadata(md);
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into fullforceRun");
	  psFree(output);
	  return false;
        }

	if (!dvodb) {  // there's no skycalRun.dvodb
 	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, ff_id %" PRId64, row->label, row->ff_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, ff_id %" PRId64, row->label, row->ff_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
    }
    else if (strcmp(stage,"fullforce_summary") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	//i believe this is correct for ff_summary as well:
        fullForceRunRow *row = fullForceRunObjectFromMetadata(md);
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into fullforceRun");
	  psFree(output);
	  return false;
        }
	
	if (!dvodb) {  // there's no skycalRun.dvodb
 	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined dvodb: label: %s, ff_id %" PRId64, row->label, row->ff_id);
	  psFree(output);
	  return false;
        }
        if (!workdir && !row->workdir) {
	  psError(PS_ERR_UNKNOWN, false, "cannot queue addstar run without a defined workdir: label: %s, ff_id %" PRId64, row->label, row->ff_id);
	  psFree(output);
	  return false;
        }
	
        psFree(row);
      }
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

    
    if (strcmp(stage,"cam") == 0) {
      // loop over our list of camRun rows
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	
	camRunRow *row = camRunObjectFromMetadata(md);
	stage_id = row->cam_id;
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
	  psFree(output);
	  return false;
        }
	
        // queue the exp
        if (!pxaddQueueByCamID(config,
			       stage,
                               stage_id,
			       0,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : row->dvodb,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
        }
        psFree(row);
      }
    }
    else if (strcmp(stage,"stack") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	
	stackRunRow *row = stackRunObjectFromMetadata(md);
	stage_id = row->stack_id;
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
	  psFree(output);
	  return false;
        }
	
        // queue the exp
        if (!pxaddQueueByCamID(config,
			       stage,
                               stage_id,
			       0,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : row->dvodb,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
        }
        psFree(row);
      }
    }
    else if (strcmp(stage,"staticsky") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	bool status = false;
	//psS32 num_inputs = psMetadataLookupS32(&status, md, "num_inputs");
	psS32 stack_id = psMetadataLookupS32(&status, md, "stack_id");
	
	if (!status) {
	  psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item stack_id");
	  return false;
	}
	staticskyRunRow *row = staticskyRunObjectFromMetadata(md);
	stage_id = row->sky_id;
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into camRun");
	  psFree(output);
	  return false;
        }
        // queue the exp
      	//for (int filter_id = 0; filter_id < num_inputs; filter_id++) {  
	if (!pxaddQueueByCamID(config,
			       stage,
			       stage_id,
			       stack_id,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : NULL,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
	}
	//}
        psFree(row);
      }
    }
    else if (strcmp(stage,"skycal") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	
	skycalRunRow *row = skycalRunObjectFromMetadata(md);
	stage_id = row->skycal_id;
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into skycalRun");
	  psFree(output);
	  return false;
        }
	
        // queue the exp
        if (!pxaddQueueByCamID(config,
			       stage,
                               stage_id,
			       0,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : NULL,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
        }
        psFree(row);
      }
    }
    else if (strcmp(stage,"diff") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	//	psS64 stage_extra1 = 0;
	diffRunRow *row = diffRunObjectFromMetadata(md);
	stage_id = row->diff_id;
	bool status = false;
	psS32 stage_extra1 = psMetadataLookupS32(&status, md, "diff_skyfile_id");
	if (!status) {
	  psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item diff_skyfile_id");
	  return false;
	}
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into skycalRun");
	  psFree(output);
	  return false;
        }
	
        // queue the exp
        if (!pxaddQueueByCamID(config,
			       stage,
                               stage_id,
			       stage_extra1,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : NULL,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
        }
        psFree(row);
      }
    }
    else if (strcmp(stage,"fullforce") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	
	fullForceRunRow *row = fullForceRunObjectFromMetadata(md);
	stage_id = row->ff_id;
	bool status = false;
	psS32 stage_extra1 = psMetadataLookupS32(&status, md, "warp_id");
	if (!status) {
	  psError(PS_ERR_UNKNOWN, true, "failed to lookup value for item warp_id");
	  return false;
	}
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into skycalRun");
	  psFree(output);
	  return false;
        }
	
        // queue the exp
        if (!pxaddQueueByCamID(config,
			       stage,
                               stage_id,
			       stage_extra1,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : NULL,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
        }
        psFree(row);
      }
    }
    else if (strcmp(stage,"fullforce_summary") == 0) {
      for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];
	psS64 stage_id =0; 
	
	fullForceRunRow *row = fullForceRunObjectFromMetadata(md);
	stage_id = row->ff_id;
	
        if (!row) {
	  psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into fullforceRun");
	  psFree(output);
	  return false;
        }
	
        // queue the exp
        if (!pxaddQueueByCamID(config,
			       stage,
                               stage_id,
			       0,
                               workdir     ? workdir   : row->workdir,
                               reduction   ? reduction : row->reduction,
                               label       ? label     : row->label,
                               data_group  ? data_group : (row->data_group ? row->data_group :  (label ? label : row->label)),
                               dvodb       ? dvodb     : NULL,
                               note        ? note      : NULL,
                               image_only,
                               minidvodb,
                               minidvodb_group,
                               minidvodb_name,
                               minidvodb_host
			       )) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error sfg");
	  }
	  psError(PS_ERR_UNKNOWN, false,
		  "failed to trying to queue stage %s %" PRId64,stage, stage_id);
	  psFree(row);
	  psFree(output);
	  return false;
        }
        psFree(row);
      }
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
    PXOPT_COPY_S64(config->args, where, "-add_id",    "addRun.add_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",    "stackRun.stack_id", "==");
    PXOPT_COPY_S64(config->args, where, "-sky_id",    "staticskyRun.sky_id", "==");
    PXOPT_COPY_S64(config->args, where, "-skycal_id", "skycalRun.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-diff_id", "diffRun.diff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-ff_id", "fullForceRun.ff_id", "==");

    PXOPT_LOOKUP_STR(stage,       config->args, "-stage", false, false);
    pxcamGetSearchArgs (config, where); // most search arguments based on camera
    PXOPT_COPY_STR(config->args, where, "-label",     "addRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-state",     "addRun.state", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "addRun.reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-dvodb", "addRun.dvodb", "==");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    psString query = NULL;
    if (strcmp(stage, "cam") == 0) {
      query = psStringCopy("UPDATE addRun JOIN camRun on (cam_id = stage_id and stage = 'cam')  JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id)");
    }
    else if (strcmp(stage, "stack") == 0) {
      query = psStringCopy("UPDATE addRun JOIN stackRun on (stack_id = stage_id and stage = 'stack')");
    }
    else if (strcmp(stage, "staticsky") == 0) {
      query = psStringCopy("UPDATE addRun JOIN staticskyRun on (sky_id = stage_id and stage = 'staticsky')");
    }
    else if (strcmp(stage, "skycal") == 0) {
      query = psStringCopy("UPDATE addRun JOIN skycalRun on (skycal_id = stage_id and stage = 'skycal')");
    }
    else if (strcmp(stage, "diff") == 0) {
      query = psStringCopy("UPDATE addRun JOIN diffInputSkyfile on (diff_id = stage_id and diff_skyfile_id = stage_extra1 and stage = 'diff') JOIN diffSkyfile on (diff_id, skycell_id) JOIN diffRun using (diff_id)");
    }
    else if (strcmp(stage, "fullforce") == 0) {
      query = psStringCopy("UPDATE addRun JOIN fullForceResult on (ff_id = stage_id and warp_id = stage_extra1 and stage = 'fullforce') JOIN fullForceRun on (ff_id)");
    }
    else if (strcmp(stage, "fullforce_summary") == 0) {
      query = psStringCopy("UPDATE addRun JOIN fullForceSummary on (ff_id = stage_id and stage = 'fullforce_summary') JOIN fullForceRun on (ff_id)");
    }


    // pxUpdateRun gets parameters from config->args and runs the update query
    bool result = pxUpdateRun(config, where, &query, "addRun", "add_id",
        "addProcessedExp", false, false);

    psFree(query);
    psFree(where);

    return result;
}


static bool pendingexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-add_id",    "addRun.add_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",    "camRun.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",    "stackRun.stack_id", "==");
    PXOPT_COPY_S64(config->args, where, "-sky_id",    "staticskyRun.sky_id", "==");
    PXOPT_COPY_S64(config->args, where, "-skycal_id", "skycalRun.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-ff_id",    "fullForceRun.ff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-diff_id",    "diffRun.diff_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_extra1",    "addRun.stage_extra1", "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id",    "addRun.stage_id", "==");
    PXOPT_LOOKUP_STR(stage,       config->args, "-stage", false, false);
    pxcamGetSearchArgs (config, where);
    pxAddLabelSearchArgs (config, where, "-label", "addRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "addRun.reduction", "==");
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(multiadd, config->args, "-multiadd", false);
    PXOPT_LOOKUP_BOOL(addrand, config->args,  "-addrand", false);
    
    psString query = NULL;
    
    if (strcmp(stage, "cam") == 0) { 
      query = pxDataGet("addtool_find_pendingexp_cam.sql");
    }
    else if (strcmp(stage, "stack") == 0) { 
      query = pxDataGet("addtool_find_pendingexp_stack.sql");
    }
    else if (strcmp(stage, "staticsky") == 0) { 
      query = pxDataGet("addtool_find_pendingexp_staticsky_multi.sql");
    }
    else if (strcmp(stage, "skycal") == 0) {
      query = pxDataGet("addtool_find_pendingexp_skycal.sql");
    }
    else if (strcmp(stage, "diff") == 0) {
      query = pxDataGet("addtool_find_pendingexp_diff.sql");
    }
    else if (strcmp(stage, "fullforce") == 0) {
      query = pxDataGet("addtool_find_pendingexp_ff.sql");
    }
    else if (strcmp(stage, "fullforce_summary") == 0) {
      query = pxDataGet("addtool_find_pendingexp_ffsummary.sql");
    }


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
    // random order of pending files
    if (addrand) {
      psStringAppend(&query, " ORDER BY RAND()");
    }
    psFree(where);
    if (strcmp(stage, "stack") == 0) {
      //this group by is needed to join against all the warps (to get camera)
      psStringAppend(&query, " GROUP BY %s", "stack_id");
    }
    else if (strcmp(stage, "skycal") == 0) {
      //this group by is needed to join against all the warps (to get camera)
      psStringAppend(&query, " GROUP BY %s", "skycal_id");
    }
    else if (strcmp(stage, "staticsky") == 0) {
      //this group by is needed to join against all the warps (to get camera)
      psStringAppend(&query, " GROUP BY %s", "sky_id, stage_extra1");
    }
    else if (strcmp(stage, "diff") == 0) {
      //this group by is needed to join against all the warps (to get camera)
      if (multiadd) {
	psStringAppend(&query, " GROUP BY %s", "diff_id");
      } else {
	psStringAppend(&query, " GROUP BY %s", "diff_id, stage_extra1");
      }
    }
    else if (strcmp(stage, "fullforce") == 0) {
      //this group by is needed to join against all the warps (to get camera)
      if (multiadd) {
	psStringAppend(&query, "GROUP BY %s", "ff_id");
      } else {
	psStringAppend(&query, " GROUP BY %s", "ff_id, stage_extra1");
      }
    }
    else if (strcmp(stage, "fullforce_summary") == 0) {
      //this group by is needed to join against all the warps (to get camera)
      psStringAppend(&query, "GROUP BY %s", "ff_id");
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
    //    printf ("%s\n", query);
    //psTrace("addtool.c", PS_LOG_INFO, "used this query:\n %s\n",query); 
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
    if (!ippdbPrintMetadatas(stdout, output, "addPendingExp", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
    
    psFree(output);
    
    return true;
}

static bool addprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
  
    // required
    // PXOPT_LOOKUP_S64(add_id, config->args, "-add_id", true, false);
    
    // optional
    PXOPT_LOOKUP_STR(path_base,     config->args, "-path_base", false, false);
    PXOPT_LOOKUP_F32(dtime_addstar, config->args, "-dtime_addstar", false, false);
    PXOPT_LOOKUP_STR(dvodb_path, config->args, "-dvodb_path", false, false);
    PXOPT_LOOKUP_STR(minidvodb_name, config->args, "-minidvodb_name", false, false);
    PXOPT_LOOKUP_STR(minidvodb_host, config->args, "-minidvodb_host", false, false);
    PXOPT_LOOKUP_S16(fault,         config->args, "-fault", false, false);
    PXOPT_LOOKUP_S64(stage_extra1, config->args, "-stage_extra1", false, false);
    
    PXOPT_LOOKUP_BOOL(multiadd, config->args, "-multiadd", false);
    // generate restrictions
    psMetadata *where = psMetadataAlloc();
    if (multiadd) {
      // NOTE: these tests are not needed because the PXOPT_COPY_* calls below return FALSE on failure to find the item
      // required, check that they are there
      // PXOPT_LOOKUP_STR(multiaddlabel, config->args, "-multiaddlabel", true, false);
      // PXOPT_LOOKUP_STR(stage, config->args, "-stage", true, false);
      // PXOPT_LOOKUP_S64(stage_id, config->args, "-stage_id", true, false);

      // do the where
      PXOPT_COPY_STR(config->args, where, "-multiaddlabel",   "addRun.label",   "==");
      PXOPT_COPY_STR(config->args, where, "-stage", "addRun.stage","==");
      PXOPT_COPY_S64(config->args, where, "-stage_id", "addRun.stage_id", "==");

      //PXOPT_LOOKUP_S64(stage_id, config->args, "-stage_id", false, false);
    } else {
      //if not multiadd mode, then do it by add_id
      // required, check for add_id
      // PXOPT_LOOKUP_S64(add_id, config->args, "-add_id",true,false);
      PXOPT_COPY_S64(config->args, where, "-add_id",   "addRun.add_id",   "==");
      
    }
    psString query = pxDataGet("addtool_find_pendingexp.sql");
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
    
    if (multiadd) {
      // optional -stage_extra1_list extra1,extra1,extra1 to restrict
      PXOPT_LOOKUP_STR(stage_extra1_list, config->args, "-stage_extra1_list", false, false);
      if (stage_extra1_list) {
	psStringAppend(&query, " AND stage_extra1 IN (%s)", stage_extra1_list);
      }
    }


    //if (multiadd) {
    //  psStringAppend(&query, "group by  %s", "stage_extra1");
    //}
    
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
      return false;
    }
    
    if (!psDBTransaction(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(output);
      return false;
    }
    
    for (long i =0; i<psArrayLength(output);i++) {
      addRunRow *pendingRow = addRunObjectFromMetadata(output->data[i]);
      
      addProcessedExpRow *row = addProcessedExpRowAlloc(
							pendingRow->add_id,
							dtime_addstar,
							path_base,
							dvodb_path,
							fault
							);
      
      if (!addProcessedExpInsertObject(config->dbh, row)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        psFree(pendingRow);
        return false;
      }
      
      //if there is a minidvodb_name, set it in addRun (it's not known until it is processed)
      if (minidvodb_name) {
	psString setName = NULL;
	psStringAppend (&setName, "UPDATE addRun set minidvodb_name = '%s' where add_id = %" PRId64, minidvodb_name, row->add_id);
	if (!p_psDBRunQuery(config->dbh, setName)) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	  }
	  psError(PS_ERR_UNKNOWN, false, "database error");
	  
	  return false;
	}
      }

      //if there is a minidvodb_host, set it in addRun (it's not known until it is processed)
      if (minidvodb_host) {
        psString setName = NULL;
        psStringAppend (&setName, "UPDATE addRun set minidvodb_host = '%s' where add_id = %" PRId64, minidvodb_host, row->add_id);
        if (!p_psDBRunQuery(config->dbh, setName)) {
          if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
          }
          psError(PS_ERR_UNKNOWN, false, "database error");
         
          return false;
        }
      }

      //if there is a stage_extra1, set it in addRun (it's not known until it is processed)
      if (stage_extra1) {
	psString setExtra = NULL;
	psStringAppend (&setExtra, "UPDATE addRun set stage_extra1 = %" PRId64, stage_extra1);
	psStringAppend (&setExtra, " where add_id = %" PRId64, row->add_id);
	
	if (!p_psDBRunQuery(config->dbh, setExtra)) {
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	  }
	  psError(PS_ERR_UNKNOWN, false, "database error");
	  
	  return false;
	}
      }
      
      // since there is only one exp per 'new' set addRun.state = 'full'
      // but check to make sure there are no faults
      
      if (!fault) {
	if (!pxaddRunSetState(config, row->add_id, "full")) {
	  psError(PS_ERR_UNKNOWN, false, "failed to change addRun.state for add_id: %" PRId64, row->add_id);
	  psFree(row);
	  psFree(pendingRow);
	  return false;
	}
      }
      psFree(row);
      psFree(pendingRow);
      
      if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
      }
    }
    
    return true;
}


static bool processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // generate restrictions
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-add_id",    "addRun.add_id",    "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id",    "addRun.stage_id",    "==");
    PXOPT_LOOKUP_STR(stage,       config->args, "-stage", false, false);
    pxcamGetSearchArgs (config, where);
    pxAddLabelSearchArgs (config, where, "-label",    "addRun.label",     "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "addRun.reduction", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    if (!psListLength(where->list) &&
        !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters (or -all) are required");
        return false;
    }
    psString query = NULL;

    if (strcmp (stage,"cam") == 0) {
      query = pxDataGet("addtool_find_processedexp_cam.sql");
    } else if (strcmp (stage,"stack") == 0) {
      query = pxDataGet("addtool_find_processedexp_stack.sql");
    } else if (strcmp (stage,"staticsky") == 0) {
      query = pxDataGet("addtool_find_processedexp_staticsky_multi.sql");
    } else if (strcmp (stage,"skycal") == 0) {
      query = pxDataGet("addtool_find_processedexp_skycal.sql");
    } else if (strcmp (stage,"diff") == 0) {
      query = pxDataGet("addtool_find_processedexp_diff.sql");
    } else if (strcmp (stage,"fullforce") == 0) {
      query = pxDataGet("addtool_find_processedexp_ff.sql");
    } else if (strcmp (stage,"fullforce_summary") == 0) {
      query = pxDataGet("addtool_find_processedexp_ffsummary.sql");
    } else {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "can't find sql file for stage %s", stage);
        return false;
    }

   
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
        psStringAppend(&query, " %s", " AND addProcessedExp.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND addProcessedExp.fault = 0");
    }
    if (!where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE addProcessedExp.fault != 0");
    }
    if (!where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE addProcessedExp.fault = 0");
    }
    psStringAppend(&query, " AND stage = '%s'", stage);

    psFree(where);

    // order by add_id so that the postage stamp parser can easliy find the 'latest' astrometry
    psStringAppend(&query, " ORDER BY add_id");

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


static bool revertprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-add_id",    "addRun.add_id",         "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id",    "addRun.stage_id",         "==");
    PXOPT_LOOKUP_STR(stage,       config->args, "-stage", true, false);
    pxcamGetSearchArgs (config, where);
    pxAddLabelSearchArgs (config, where, "-label",    "addRun.label",     "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "addRun.reduction",      "==");
    
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
      psString query = NULL;
      if (strcmp(stage, "cam") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_cam.sql");
      }
      else if (strcmp(stage, "stack") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_stack.sql");
      }
      else if (strcmp(stage, "staticsky") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_staticsky_multi.sql");
      }
      else if (strcmp(stage, "skycal") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_skycal.sql");
      }
      else if (strcmp(stage, "diff") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_diff.sql");
      }
      else if (strcmp(stage, "fullforce") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_ff.sql");
      }
      else if (strcmp(stage, "fullforce_summary") == 0) {
	query = pxDataGet("addtool_revertprocessedexp_ffsummary.sql");
      }
     
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


static bool updateprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-add_id",   "add_id",   "==");
    PXOPT_COPY_S64(config->args, where, "-stage_id",  "stage_id",  "==");
    PXOPT_LOOKUP_STR(stage,       config->args, "-stage", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);

    if (!pxSetFaultCode(config->dbh, "addProcessedExp", where, fault, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree (where);

    return true;
}


static bool blockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    if (!addMaskInsert(config->dbh, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool maskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM addMask");

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

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "addMask", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool unblockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    char *query = "DELETE FROM addMask WHERE label = '%s'";

    if (!p_psDBRunQueryF(config->dbh, query, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool addminidvodbrunMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // required
  PXOPT_LOOKUP_STR(minidvodb_group, config->args, "-set_minidvodb_group", true, false);
  PXOPT_LOOKUP_STR(minidvodb_path, config->args, "-set_minidvodb_path", false, false);
  PXOPT_LOOKUP_STR(minidvodb_host, config->args, "-set_minidvodb_host", false, false);
  //optional
  PXOPT_LOOKUP_STR(minidvodb_name, config->args, "-set_minidvodb_name", false, false);
  PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
  PXOPT_LOOKUP_STR(note,            config->args, "-set_note",            false, false);

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

//  psString minidvodbpath = "NULL";

  // I don't know how to get around the complaints of minidvodb_path can't be null. this 'fixes' it, but someone smarter can fix it properly.
//  if (minidvodb_path) {
//    minidvodbpath = minidvodb_path;
//  } else {
//    psError(PS_ERR_UNKNOWN, false, "require minidvodb_path");
//    return false;
//  }
  
  if (!minidvodbRunInsert(config->dbh,
			  0, // job_id
			  minidvodb_name,
			  minidvodb_group,
			  minidvodb_path,
			  minidvodb_host,
			  "new",
			  note,
			  0
			  )) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  
  psU64 affected = psDBAffectedRows(config->dbh);
  if (affected != 1) {
    psError(PS_ERR_UNKNOWN, false,
            "should have affected one row but %" PRIu64 " rows were modified",
            affected);
    return false;
  }
  
  psS64 minidvodb_id = psDBLastInsertID(config->dbh);
  printf("%" PRId64 "\n", minidvodb_id);

  
  if (!minidvodb_name) {
    psStringAppend(&minidvodb_name, "%s.%" PRIu64,minidvodb_group,minidvodb_id);
  }
  if (minidvodb_path) {
    psStringAppend(&minidvodb_path,"/%s",minidvodb_name);
  }

  psString query = NULL;

  psStringAppend(&query, "UPDATE minidvodbRun SET minidvodb_path = '%s', minidvodb_name = '%s' where minidvodb_id = %" PRIu64";", minidvodb_path, minidvodb_name, minidvodb_id);

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }

  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  return true;
}

static bool updateminidvodbrunMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-minidvodb_id",     "minidvodb_id", "==");
    PXOPT_COPY_STR(config->args, where, "-minidvodb_name",     "minidvodb_name", "==");
    PXOPT_COPY_STR(config->args, where, "-state",     "state", "==");
    PXOPT_COPY_STR(config->args, where, "-minidvodb_path", "minidvodb_path", "==");
    PXOPT_COPY_STR(config->args, where, "-minidvodb_group",     "minidvodb_group", "==");

    PXOPT_LOOKUP_STR(minidvodb_name,  config->args, "-set_minidvodb_name", false, false);
    PXOPT_LOOKUP_STR(minidvodb_path,  config->args, "-set_minidvodb_path", false, false);
    PXOPT_LOOKUP_STR(minidvodb_host,  config->args, "-set_minidvodb_host", false, false);
    PXOPT_LOOKUP_STR(state,  config->args, "-set_state", false, false);
    PXOPT_LOOKUP_STR(minidvodb_group,  config->args, "-set_minidvodb_group", false, false);


    if (!psListLength(where->list)) {
      psFree(where);
      psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
      return false;
    }

    psString query = psStringCopy("UPDATE minidvodbRun SET ");
    int cnt = 0;
    psString comma = ",";
    if (minidvodb_name) {
      psStringAppend(&query, " minidvodb_name = '%s'", minidvodb_name);
      cnt++;
    }

    if (minidvodb_path) {
      if (cnt) {
	psStringAppend(&query, "%s", comma);
      }
      
      psStringAppend(&query, " minidvodb_path = '%s'", minidvodb_path);
      cnt++;
    }

    if (minidvodb_host) {
      if (cnt) {
        psStringAppend(&query, "%s", comma);
      }
     
      psStringAppend(&query, " minidvodb_host = '%s'", minidvodb_host);
      cnt++;
    }

    if (state) {
      if (cnt) {
        psStringAppend(&query, "%s", comma);
      }
      psStringAppend(&query, " state = '%s'", state);
      cnt++;
    }

    if (minidvodb_group) {
      if (cnt) {
        psStringAppend(&query, "%s", comma);
      }
      psStringAppend(&query, " minidvodb_group = '%s'", minidvodb_group);
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

static bool flipminidvodbrunMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_LOOKUP_STR(minidvodb_group,  config->args, "-minidvodb_group",true, false);
  PXOPT_COPY_STR(config->args, where, "-minidvodb_group",     "minidvodb_group", "==");

//this flips the new - > active
// and the active - > waiting in one action


// the first query looks to find things that are new and where all the fields are filled (ie, ready to be flipped to active)

// start a transaction eraly so it will contain any row level locks
  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  psString firstquery = psStringCopy("SELECT * from  minidvodbRun where state = 'new' and minidvodb_name is NOT NULL and minidvodb_group IS NOT NULL and minidvodb_path IS NOT NULL");

  psString firstwhereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&firstquery, " AND %s", firstwhereClause);



  if (!p_psDBRunQuery(config->dbh, firstquery)) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    return false;
  }
  psFree(firstquery);

 //we don't care what the stuff is that is found, just that there is stuff. This is a check to see that there is something in the 'new' state, before flipping (so that if there is nothing in new, it won't flip the active to waiting.  the flipminidvo is just to make it easy to flip from new -> active.

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


  //ok, there's something, so flip active -> waiting

  psString query = psStringCopy("UPDATE minidvodbRun SET state = 'waiting' WHERE state = 'active' ");

  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringAppend(&query, " AND %s", whereClause);


  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    return false;
  }


  //now flip new -> active
  psString query2 = psStringCopy("UPDATE minidvodbRun SET state = 'active' WHERE state = 'new' AND minidvodb_name is NOT NULL and minidvodb_group IS NOT NULL and minidvodb_path IS NOT NULL ");
  psStringAppend(&query2, " AND minidvodb_group = '%s' limit 1;", minidvodb_group);
  if (!p_psDBRunQuery(config->dbh, query2)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psFree(query);
    return false;
  }


  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psFree(query);
    return false;
  }
  psFree(query2);
  psFree(where);
  
  return true;
 
}


static bool checkminidvodbrunaddrunMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);
  psMetadata *where = psMetadataAlloc();

  //this checks to see if a minidvod_group/name is has completed addRun processing
  PXOPT_COPY_STR(config->args, where, "-minidvodb_group", "minidvodbRun.minidvodb_group", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_name", "minidvodbRun.minidvodb_name", "==");
  PXOPT_COPY_STR(config->args, where, "-state", "minidvodbRun.state", "==");
  PXOPT_LOOKUP_STR(minidvodb_group,  config->args, "-minidvodb_group", false, false);
  
  PXOPT_LOOKUP_U64(limit,      config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(all_addrun_states, config->args, "-all_addrun_states", false);
  //this doesn't care what state the addRun is in (useful for counting addRuns)
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  if (!minidvodb_group) {
    psError(PXTOOLS_ERR_CONFIG, false, "minidvodb_group  required");
    return false;
  }


  psString bare_query = pxDataGet("addtool_checkminidvodbaddrun.sql");
  
  if (!bare_query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }

  psString where_string = NULL;
  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&where_string, " WHERE %s", whereClause);
    psFree(whereClause);
    //search params are defined above
  }

  psString query = NULL;
  psStringAppend(&query, bare_query, minidvodb_group, minidvodb_group);
  psStringAppend(&query," %s ", where_string);

  if (!all_addrun_states) {
    psStringAppend(&query, " AND (cnt2 = cnt) ");
  }

  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s", limitString);
    psFree(limitString);
  }


  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error,\n\n%s", query);
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

  if (!ippdbPrintMetadatas(stdout, output, "minidvodbRun", !simple)) {
    psError(PS_ERR_UNKNOWN, false, "failed to print array");
    psFree(output);
    return false;
  }

  psFree(output);

  return true;
}



static bool listminidvodbrunMode(pxConfig *config) {
    PS_ASSERT_PTR_NON_NULL(config, false);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-minidvodb_name", "minidvodbRun.minidvodb_name", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-minidvodb_group", "minidvodbRun.minidvodb_group", "==");
    PXOPT_COPY_S64(config->args, where, "-minidvodb_id", "minidvodbRun.minidvodb_id", "==");

    PXOPT_COPY_STR(config->args, where, "-state", "minidvodbRun.state", "==");
    PXOPT_LOOKUP_U64(limit,      config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);


    if (!psListLength(where->list)) {
      psFree(where);
      psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
      return false;
    }

    psString query = pxDataGet("addtool_find_minidvodbrun.sql");

    if (!query) {
      psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
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
      psError(PS_ERR_UNKNOWN, false, "database error");
      return false;
    }
    if (!psArrayLength(output)) {
      psTrace("addtool", PS_LOG_INFO, "no rows found");
      psFree(output);
      return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "minidvodbRun",  !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }

    psFree(output);

return true;
}

static bool addminidvodbprocessedMode(pxConfig *config) {

  PS_ASSERT_PTR_NON_NULL(config, false);

  // required
  // PXOPT_LOOKUP_U64(minidvodb_id, config->args, "-minidvodb_id", true, false);
  PXOPT_LOOKUP_STR(minidvodb_group, config->args, "-minidvodb_group", true, false);

  // optional
  PXOPT_LOOKUP_F32(dtime_relphot, config->args, "-dtime_relphot", false, false);
  PXOPT_LOOKUP_F32(dtime_resort, config->args, "-dtime_resort", false, false);
  PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
  
  PXOPT_LOOKUP_TIME(epoch, config->args, "-epoch", false, false);
  PXOPT_LOOKUP_S16(fault,         config->args, "-fault", false, false);

  //generate restrictions
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id",   "minidvodbRun.minidvodb_id",   "==");


  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }


  psString query = pxDataGet("addtool_find_pendingmergeprocess.sql");
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

  minidvodbRunRow *pendingRow = minidvodbRunObjectFromMetadata(output->data[0]);
  psFree(output);
  minidvodbProcessedRow *row = minidvodbProcessedRowAlloc(
							  pendingRow->minidvodb_id,
							  dtime_resort,
							  dtime_relphot,
							  dtime_script,
							  epoch,
							  fault
							  );
  
  if (!minidvodbProcessedInsertObject(config->dbh, row)) {
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(row);
    psFree(pendingRow);
    return false;
  }

  psString query2 = NULL ;
  psStringAppend(&query2, "UPDATE minidvodbRun SET state = 'merged' WHERE minidvodb_id = %'" PRIu64, row->minidvodb_id);

  if (!p_psDBRunQuery(config->dbh, query2)) {
    // rollback
    if (!psDBRollback(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
    }
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query2);
    return false;
  }
  psFree(row);
  psFree(pendingRow);

  //commit the changes
  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  return true;
}



static bool listminidvodbprocessedMode(pxConfig *config) {
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id", "minidvodbProcessed.minidvodb_id", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_name", "minidvodbRun.minidvodb_name", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_group", "minidvodbRun.minidvodb_group", "==");
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
  if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

  psString query = pxDataGet("addtool_find_minidvodbprocessed.sql");
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
        psStringAppend(&query, " %s", " AND minidvodbProcessed.fault != 0");
    }
    if (where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " AND minidvodbProcessed.fault = 0");
    }
    if (!where->list && faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", " WHERE minidvodbProcessed.fault != 0");
    }
    if (!where->list && !faulted) {
        // don't list faulted rows
        psStringAppend(&query, " %s", " WHERE minidvodbProcessed.fault = 0");
    }
    psFree(where);

    // order by epoch
    psStringAppend(&query, " ORDER BY minidvodb_id");

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
      psError(PS_ERR_UNKNOWN, false, "database error ");
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
    if (!ippdbPrintMetadatas(stdout, output, "minidvodbProcessed", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

return true;
}



static bool revertminidvodbprocessedMode(pxConfig *config) {
  psMetadata *where = psMetadataAlloc();
  PS_ASSERT_PTR_NON_NULL(config, false);
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id", "minidvodbProcessed.minidvodb_id", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_group", "addRun.minidvodb_group", "==");

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
    psString query = pxDataGet("addtool_revertminidvodbprocessed.sql");
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



static bool updateminidvodbprocessedMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);
  psMetadata *where = psMetadataAlloc();

  // PXOPT_LOOKUP_U64(minidvodb_id,  config->args, "-minidvodb_id", true, false);
  PXOPT_LOOKUP_S16(fault,  config->args, "-set_fault", false, false);
  PXOPT_LOOKUP_F32(dtime_relphot,  config->args, "-set_dtime_relphot", false, false);
  PXOPT_LOOKUP_F32(dtime_resort,  config->args, "-set_dtime_resort", false, false);
  PXOPT_LOOKUP_F32(dtime_script,  config->args, "-set_dtime_script", false, false);
  PXOPT_COPY_S64(config->args, where, "-minidvodb_id",     "minidvodbProcessed.minidvodb_id", "==");
  PXOPT_COPY_STR(config->args, where, "-minidvodb_name",     "minidvodbRun.minidvodb_name", "==");


  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  psString query = psStringCopy("UPDATE minidvodbProcessed JOIN minidvodbRun USING (minidvodb_id) SET ");
  int cnt = 0;
  psString comma = ",";
  if (fault) {
    psStringAppend(&query, " fault = %d", fault);
  cnt++;
  }

  if (dtime_relphot) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " dtime_relphot = %f", dtime_relphot);
    cnt++;
  }

  if (dtime_resort) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " dtime_resort = %f", dtime_resort);
    cnt++;
  }

  if (dtime_script) {
    if (cnt) {
      psStringAppend(&query, "%s", comma);
    }
    psStringAppend(&query, " dtime_script = %f", dtime_script);
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



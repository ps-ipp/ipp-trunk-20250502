/*
 * staticskytool.c
 *
 * Copyright (C) 2007-2010  Joshua Hoblitt, Paul Price
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ippdb.h>

#include "pxtools.h"
#include "pxspace.h"
#include "staticskytool.h"

static bool definebyqueryMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool inputsMode(pxConfig *config);
static bool todoMode(pxConfig *config);
static bool addresultMode(pxConfig *config);
static bool resultMode(pxConfig *config);
static bool revertMode(pxConfig *config);
static bool updateresultMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

static bool defineskycalrunMode(pxConfig *config);
static bool updateskycalrunMode(pxConfig *config);
static bool pendingskycalrunMode(pxConfig *config);
static bool addskycalresultMode(pxConfig *config);
static bool skycalresultMode(pxConfig *config);
static bool revertskycalresultMode(pxConfig *config);
static bool updateskycalresultMode(pxConfig *config);
static bool exportskycalrunMode(pxConfig *config);
static bool importskycalrunMode(pxConfig *config);

static bool setstaticskyRunState(pxConfig *config, psS64 sky_id, const char *state);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = staticskytoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(STATICSKYTOOL_MODE_DEFINEBYQUERY,     definebyqueryMode);
        MODECASE(STATICSKYTOOL_MODE_UPDATERUN,         updaterunMode);
        MODECASE(STATICSKYTOOL_MODE_INPUTS,            inputsMode);
        MODECASE(STATICSKYTOOL_MODE_TODO,              todoMode);
        MODECASE(STATICSKYTOOL_MODE_ADDRESULT,         addresultMode);
        MODECASE(STATICSKYTOOL_MODE_RESULT,            resultMode);
        MODECASE(STATICSKYTOOL_MODE_REVERT,            revertMode);
        MODECASE(STATICSKYTOOL_MODE_UPDATERESULT,      updateresultMode);
        MODECASE(STATICSKYTOOL_MODE_EXPORTRUN,         exportrunMode);
        MODECASE(STATICSKYTOOL_MODE_IMPORTRUN,         importrunMode);
        MODECASE(STATICSKYTOOL_MODE_DEFINESKYCALRUN,   defineskycalrunMode);
        MODECASE(STATICSKYTOOL_MODE_UPDATESKYCALRUN,   updateskycalrunMode);
        MODECASE(STATICSKYTOOL_MODE_PENDINGSKYCALRUN,  pendingskycalrunMode);
        MODECASE(STATICSKYTOOL_MODE_ADDSKYCALRESULT,   addskycalresultMode);
        MODECASE(STATICSKYTOOL_MODE_UPDATESKYCALRESULT,updateskycalresultMode);
        MODECASE(STATICSKYTOOL_MODE_REVERTSKYCALRESULT,revertskycalresultMode);
        MODECASE(STATICSKYTOOL_MODE_SKYCALRESULT,      skycalresultMode);
        MODECASE(STATICSKYTOOL_MODE_EXPORTSKYCALRUN,   exportskycalrunMode);
        MODECASE(STATICSKYTOOL_MODE_IMPORTSKYCALRUN,   importskycalrunMode);
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
    PS_ASSERT_PTR_NON_NULL(config, false);
    PXOPT_LOOKUP_STR(workdir,     config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label", true, false);

    // optional
    PXOPT_LOOKUP_STR(data_group,  config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group,  config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction,   config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(note,        config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);

    psMetadata *whereMD = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, whereMD, "-select_stack_id",      "stackRun.stack_id",         "==");
    PXOPT_COPY_STR(config->args, whereMD, "-select_skycell_id",    "stackRun.skycell_id",       "LIKE");
    PXOPT_COPY_STR(config->args, whereMD, "-select_tess_id",       "stackRun.tess_id",          "==");
    PXOPT_COPY_F32(config->args, whereMD, "-select_good_frac_min", "stackSumSkyfile.good_frac", ">=");
    pxAddLabelSearchArgs(config, whereMD, "-select_label",         "stackRun.label",            "LIKE");
    pxAddLabelSearchArgs(config, whereMD, "-select_data_group",    "stackRun.data_group",       "LIKE");
    pxAddLabelSearchArgs(config, whereMD, "-select_filter",        "stackRun.filter",           "LIKE");
    if (!pxskycellAddWhere(config, whereMD)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    // find the number of requested filters:
    psMetadataItem *filters = psMetadataLookup(config->args, "-select_filter");
    psAssert (filters, "-select_filter must exist");
    psAssert (filters->type == PS_DATA_METADATA_MULTI, "-select_filter should be a multi container");
    psAssert (filters->data.list->n, "-select_filter should at least have a place-holder");
    int num_filter = filters->data.list->n;

    PXOPT_LOOKUP_BOOL(group_by_data_group, config->args, "-group_by_data_group", false);
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(check_inputs, config->args, "-check_inputs", false);

    psString select;
    if (group_by_data_group) {
        select = pxDataGet("staticskytool_definebyquery_select_by_dg.sql");
    } else {
        select = pxDataGet("staticskytool_definebyquery_select.sql");
    }
    if (!select) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(whereMD);
        return false;
    }

    if (!psListLength(whereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(whereMD);
        return false;
    }

    // this 'where' is used for both staticskytool_definebyquery_select.sql and staticskytool_definebyquery_inputs.sql 
    psString where = NULL;
    psString whereClause = psDBGenerateWhereConditionSQL(whereMD, NULL);
    psStringAppend(&where, "\nAND %s", whereClause);
    psFree(whereClause);
    psFree(whereMD);

    psString where2 = NULL;
    psString make_unique = NULL;
    if (!rerun) {
        psStringAppend(&where2, "\n %s\nAND staticskyRun.label = '%s'", where, label);
        psStringAppend(&make_unique, "\nAND sky_id IS NULL");
    }

    if (!p_psDBRunQueryF(config->dbh, select, where, where2 ? where2 : where, num_filter, make_unique ? make_unique : "")) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(select);
        return false;
    }
    psFree(select);
    psFree(where2);
    psFree(make_unique);

    // we now have a list of (tess_id, skycell_id) that (potentially) meet out needs
    // we now need to loop over all of these and for each pair, select the best set of
    // inputs

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
        psFree(where);
        return false;
    }
    if (!psArrayLength(output)) {
        psWarning("staticskytool: no rows found");
        psFree(output);
        psFree(where);
        return true;
    }
    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "staticskyInput", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            psFree(where);
            return false;
        }
        psFree(output);
        psFree(where);
        return true;
    }

    psString inputsSQL = pxDataGet("staticskytool_definebyquery_inputs.sql");
    if (!inputsSQL) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    for (long i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row from select
        bool status;

        // pull out the skycell_id, tess_id, filter
        psString skycell_id = psMetadataLookupStr(&status, row, "skycell_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup skycell_id");
            psFree(output);
            psFree(inputsSQL);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psString tess_id = psMetadataLookupStr(&status, row, "tess_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup tess_id");
            psFree(output);
            psFree(inputsSQL);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psString by_data_group = NULL;
        psString stack_data_group = NULL;
        if (group_by_data_group) {
            stack_data_group = psMetadataLookupStr(&status, row, "data_group");
            if (!status) {
                psError(PS_ERR_UNKNOWN, false, "failed to lookup data_group");
                psFree(output);
                psFree(inputsSQL);
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
            psStringAppend(&by_data_group, "\nAND stackRun.data_group = '%s'", stack_data_group);
        }

	// query for the inputs
	if (!p_psDBRunQueryF(config->dbh, inputsSQL, tess_id, skycell_id, where, 
                                                        by_data_group ? by_data_group : "")) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
	    psFree(inputsSQL);
	    return false;
	}
	
	psArray *inputs = p_psDBFetchResult(config->dbh);
	if (!inputs) {
	    psErrorCode err = psErrorCodeLast();
	    switch (err) {
	      case PS_ERR_DB_CLIENT:
                psError(PXTOOLS_ERR_SYS, false, "database error");
	      case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
	      default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
	    }
	    psFree(inputs);
            psFree(output);
            psFree(inputsSQL);
	    return false;
	}
	if (!psArrayLength(inputs)) {
	    psWarning("staticskytool ERROR: no rows found for known tess_id, skycell_id?");
	    continue;
	}

	if (check_inputs) {
	  // negative simple so the default is true
	  if (!ippdbPrintMetadatas(stdout, inputs, "staticskyInput", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            psFree(where);
            return false;
	  }
	  psFree(inputs);
	  continue;
	}

	// XXX if we are unable to guarantee that all selected inputs have all and only the
	// requested filters, see the code at the bottom of this file (ifdef'ed out for now)

	// insert a new staticsky entry and find its new id
	if (!psDBTransaction(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	    psFree(inputs);
            psFree(output);
            psFree(inputsSQL);
	    return false;
	}

	// create a staticskyRun
	if (!staticskyRunInsert(config->dbh,
				0x0,	     // sky_id
				"new",	     // state
				workdir,
				label,
				data_group ? data_group : (stack_data_group ? stack_data_group : label),
				dist_group,
				reduction,
				registered,
				note
		)
	    ) {
	    if (!psDBRollback(config->dbh)) {
		psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
	    }
	    psError(PS_ERR_UNKNOWN, false, "database error");
	    psFree(inputs);
            psFree(output);
            psFree(inputsSQL);
	    return false;
	}

	psS64 sky_id =  psDBLastInsertID(config->dbh);

	// loop over the possible inputs and record only the valid ones
	for (int j = 0; j < inputs->n; j++) {
	    psMetadata *inputRow = inputs->data[j]; // Row from select
	    
	    // pull out the skycell_id, tess_id, filter
	    psS64 stack_id = psMetadataLookupS64(&status, inputRow, "stack_id");
	    psAssert(status, "failed to find stack_id?");
	    
	    // add a staticskyInput entry
	    if (!staticskyInputInsert(config->dbh, sky_id, stack_id)) {
		if (!psDBRollback(config->dbh)) {
		    psError(PS_ERR_UNKNOWN, false, "database error failed to rollback transaction");
		}
		psError(PS_ERR_UNKNOWN, false, "database error");
		psFree(inputs);
		psFree(output);
		psFree(inputsSQL);
		return false;
	    }
	}
	psFree (inputs);
	
	if (!psDBCommit(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	    psFree(output);
	    psFree(inputsSQL);
	    return false;
	}
    }
    psFree(inputsSQL);
    psFree(output);
    return true;
}

static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sky_id",      "sky_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-label",       "staticskyRun.label",    "==");
    PXOPT_COPY_STR(config->args, where, "-data_group",  "staticskyRun.data_group",    "==");
    PXOPT_COPY_STR(config->args, where, "-state",       "staticskyRun.state",    "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id",     "stackRun.tess_id",    "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",  "stackRun.skycell_id",    "==");
    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE staticskyRun JOIN staticskyInput USING(sky_id) JOIN stackRun using(stack_id) JOIN skycell USING(tess_id, skycell_id)");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "staticskyRun", "sky_id", "staticskyResult", true, false);
    psFree(query);
    psFree(where);

    return result;
}


static bool inputsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // XXX require at least a sky id (add better search options)
    // PXOPT_LOOKUP_S64(sky_id, config->args, "-sky_id", true, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sky_id", "sky_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("staticskytool_inputs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "staticskyInput");
        psStringAppend(&query, " WHERE %s", whereClause);
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
        psTrace("staticskytool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "staticskyInput", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool todoMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *whereMD = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, whereMD, "-sky_id", "sky_id", "==");
    pxAddLabelSearchArgs (config, whereMD, "-label", "staticskyRun.label", "==");
    pxskycellAddWhere(config, whereMD);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("staticskytool_todo.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!psListLength(whereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(whereMD);
        return false;
    }

    psString where = NULL;
    psString whereClause = psDBGenerateWhereConditionSQL(whereMD, NULL);
    psStringAppend(&where, "\n AND %s", whereClause);
    psFree(whereClause);
    psFree(whereMD);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    // the where clause is required and matches the WHERE hook format string
    if (!p_psDBRunQueryF(config->dbh, query, where)) {
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
        psTrace("staticskytool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "staticskyResult", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);
    return true;
}

static bool addresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(sky_id, config->args, "-sky_id", true, false);

    // optional
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);
    PXOPT_LOOKUP_F32(dtime_phot, config->args, "-dtime_phot", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_S32(sources, config->args, "-sources", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_F32(good_frac, config->args, "-good_frac", false, false);
    PXOPT_LOOKUP_S32(num_inputs, config->args, "-num_inputs", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    // XXX not sure we need a transaction here...
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!staticskyResultInsert(config->dbh,
			       sky_id,
                               path_base,
                               dtime_phot,
                               dtime_script,
                               sources,
			       num_inputs,
                               hostname,
                               good_frac,
                               fault,
                               quality
          )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!fault) {
        if (!setstaticskyRunState(config, sky_id, "full")) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to change staticskyRun state");
            return false;
        }
    }

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

// XXX what is this used by?  what filters are needed?
static bool resultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sky_id",     "staticskyRun.sky_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",   "staticskyInput.stack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",      "staticskyRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "staticskyRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "stackRun.tess_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "LIKE");
    PXOPT_COPY_S16(config->args, where, "-fault",      "staticskyResult.fault", "==");
    pxskycellAddWhere(config, where);
    PXOPT_LOOKUP_S32(num_filters, config->args, "-num_filters", false, false);

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("staticskytool_result.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (!all) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }

    psFree(where);

    psStringAppend(&query, "\nGROUP BY sky_id");
    if (num_filters) {
        psStringAppend(&query, "\nHAVING COUNT(filter) = %d", num_filters);
    }
        

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
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
        psTrace("staticskytool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "staticskyResult", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool revertMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sky_id", "staticskyResult.sky_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "staticskyRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "staticskyResult.fault", "==");

    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString delete = pxDataGet("staticskytool_revert.sql");
    if (!delete) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    psString update = pxDataGet("staticskytool_revert_update.sql");
    if (!update) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&delete, " AND %s", whereClause);
        psStringAppend(&update, " AND %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQuery(config->dbh, delete)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(delete);
        psFree(where);
        return false;
    }
    psFree(delete);

    int numRows = psDBAffectedRows(config->dbh);
    psLogMsg("staticskytool", PS_LOG_INFO, "Deleted %d rows", numRows);

    if (!p_psDBRunQuery(config->dbh, update)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(delete);
        psFree(where);
        return false;
    }
    psFree(update);

    numRows = psDBAffectedRows(config->dbh);
    psLogMsg("staticskytool", PS_LOG_INFO, "Updated %d rows", numRows);

    psFree(where);
    return true;
}

static bool updateresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sky_id",   "sky_id",   "==");

    if (!pxSetFaultCode(config->dbh, "staticskyResult", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree (where);
    return true;
}

bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 3;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // XXX unused PXOPT_LOOKUP_S64(det_id, config->args, "-sky_id", true,  false);
  PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
  PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);

  FILE *f = fopen (outfile, "w");
  if (f == NULL) {
      psError(PS_ERR_UNKNOWN, false, "failed to open output file");
      return false;
  }

  if (!pxExportVersion(config, f)) {
    psError(PS_ERR_UNKNOWN, false, "failed to write dbversion output file");
    return false;
  }
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-sky_id", "sky_id", "==");

  ExportTable tables [] = {
    {"staticskyRun", "staticskytool_export_run.sql"},
    {"staticskyInput", "staticskytool_export_input.sql"},
    {"staticskyResult", "staticskytool_export_result.sql"},
  };

  for (int i=0; i < numExportTables; i++) {
    psString query = pxDataGet(tables[i].sqlFilename);
    if (!query) {
      psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
      return false;
    }

    if (where && psListLength(where->list)) {
      psString whereClause = psDBGenerateWhereSQL(where, NULL);
      psStringAppend(&query, " %s", whereClause);
      psFree(whereClause);
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
      psError(PS_ERR_UNKNOWN, true, "no rows found");
      psFree(output);
      return false;
    }

    if (clean) {
        if (!strcmp(tables[i].tableName, "staticskyRun")) {
            if (!pxSetStateCleaned("staticskyRun", "state", output)) {
                psFree(output);
                psError(PS_ERR_UNKNOWN, false, "pxSetStateClean failed for table %s",  tables[i].tableName);
                return false;
            }
        }
    }

      // we must write the export table in non-simple (true) format
    if (!ippdbPrintMetadatas(f, output, tables[i].tableName, true)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);
  }

    fclose (f);

    return true;
}

bool importrunMode(pxConfig *config)
{
  return false;
}

bool exportskycalrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 2;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // XXX unused PXOPT_LOOKUP_S64(det_id, config->args, "-sky_id", true,  false);
  PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
  PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);

  FILE *f = fopen (outfile, "w");
  if (f == NULL) {
      psError(PS_ERR_UNKNOWN, false, "failed to open output file");
      return false;
  }

  if (!pxExportVersion(config, f)) {
    psError(PS_ERR_UNKNOWN, false, "failed to write dbversion output file");
    return false;
  }
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-skycal_id", "skycal_id", "==");

  ExportTable tables [] = {
    {"skycalRun", "staticskytool_export_skycalrun.sql"},
    {"skycalResult", "staticskytool_export_skycalresult.sql"},
  };

  for (int i=0; i < numExportTables; i++) {
    psString query = pxDataGet(tables[i].sqlFilename);
    if (!query) {
      psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
      return false;
    }

    if (where && psListLength(where->list)) {
      psString whereClause = psDBGenerateWhereSQL(where, NULL);
      psStringAppend(&query, " %s", whereClause);
      psFree(whereClause);
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
      psError(PS_ERR_UNKNOWN, true, "no rows found");
      psFree(output);
      return false;
    }

    if (clean) {
        if (!strcmp(tables[i].tableName, "skycalRun")) {
            if (!pxSetStateCleaned("skycalRun", "state", output)) {
                psFree(output);
                psError(PS_ERR_UNKNOWN, false, "pxSetStateClean failed for table %s",  tables[i].tableName);
                return false;
            }
        }
    }

      // we must write the export table in non-simple (true) format
    if (!ippdbPrintMetadatas(f, output, tables[i].tableName, true)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);
  }

    fclose (f);

    return true;
}

bool importskycalrunMode(pxConfig *config)
{
  return false;
}

static bool setstaticskyRunState(pxConfig *config, psS64 sky_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid staticskyRun state: %s", state);
        return false;
    }

    char *query = "UPDATE staticskyRun SET state = '%s' WHERE sky_id = %"PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, sky_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for sky_id %"PRId64, sky_id);
        return false;
    }
    return true;

}

static bool defineskycalrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required options
    // none required. We get workdir, etc from staticskyRun if not provided

    // optional
    PXOPT_LOOKUP_STR(label,       config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(workdir,     config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(data_group,  config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group,  config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction,   config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(note,        config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);

    psMetadata *whereMD = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, whereMD, "-select_sky_id",        "staticskyRun.sky_id",       "==");
    PXOPT_COPY_S64(config->args, whereMD, "-select_stack_id",      "stackRun.stack_id",         "==");
    PXOPT_COPY_STR(config->args, whereMD, "-select_skycell_id",    "stackRun.skycell_id",       "==");
    PXOPT_COPY_STR(config->args, whereMD, "-select_tess_id",       "stackRun.tess_id",          "==");
    pxAddLabelSearchArgs(config, whereMD, "-select_filter",        "stackRun.filter",           "LIKE");
    PXOPT_COPY_F32(config->args, whereMD, "-select_good_frac_min", "stackSumSkyfile.good_frac", ">=");
    pxAddLabelSearchArgs(config, whereMD, "-select_label",         "staticskyRun.label",            "LIKE");
    pxAddLabelSearchArgs(config, whereMD, "-select_data_group",    "staticskyRun.data_group",       "LIKE");
    if (!pxskycellAddWhere(config, whereMD)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = pxDataGet("staticskytool_defineskycalrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(whereMD);
        return false;
    }

    if (!psListLength(whereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(whereMD);
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(whereMD, NULL);
    psStringAppend(&query, "\nAND %s", whereClause);
    psFree(whereClause);
    psFree(whereMD);

    psString labelHook = psStringCopy("");
    if (!rerun) {
        if (label)  {
            psStringAppend(&labelHook, "\nAND skycalRun.label = '%s'", label);
        }
        psStringAppend(&query, "\nAND skycal_id IS NULL");
    }

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, labelHook)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(labelHook);
        psFree(query);
        return false;
    }
    psFree(labelHook);
    psFree(query);

    // we now have a list of (tess_id, skycell_id) that (potentially) meet out needs
    // we now need to loop over all of these and for each pair, select the best set of
    // inputs

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
        psWarning("staticskytool: no rows found");
        psFree(output);
        return true;
    }
    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "skycalRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    for (long i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row from select
        bool status;

        psS64 sky_id = psMetadataLookupS64(&status, row, "sky_id");
        psS64 stack_id = psMetadataLookupS64(&status, row, "stack_id");
        psString sky_workdir = psMetadataLookupStr(&status, row, "workdir");
        psString sky_label =  psMetadataLookupStr(&status, row, "label");
        psString sky_data_group =  psMetadataLookupStr(&status, row, "data_group");
        psString sky_dist_group =  psMetadataLookupStr(&status, row, "dist_group");

	// create a staticskyRun
	if (!skycalRunInsert(config->dbh,
				0x0,	     // skycal_id
                                sky_id,
                                stack_id,
				"new",	     // state
				workdir ? workdir : sky_workdir,
				label ? label : sky_label,
				data_group ? data_group : sky_data_group,
				dist_group ? dist_group : sky_dist_group,
				reduction,
				registered,
				note
		)
	    ) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
	    return false;
	}

    }
    psFree(output);
    return true;
}

static bool updateskycalrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-skycal_id", "skycal_id",   "==");
    PXOPT_COPY_S64(config->args, where, "-sky_id",   "sky_id",   "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stack_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",  "stackRun.skycell_id",    "==");
    PXOPT_COPY_STR(config->args, where, "-label",   "skycalRun.label",    "==");
    PXOPT_COPY_STR(config->args, where, "-state",   "skycalRun.state",    "==");
    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE skycalRun JOIN stackRun using(stack_id) join skycell using(tess_id, skycell_id)");

    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "skycalRun", "skycal_id", "skycalResult", true, false);
    psFree(query);
    psFree(where);

    return result;
}

static bool pendingskycalrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *whereMD = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, whereMD, "-skycal_id", "skycal_id", "==");
    PXOPT_COPY_S64(config->args, whereMD, "-sky_id", "sky_id", "==");
    pxAddLabelSearchArgs (config, whereMD, "-label", "skycalRun.label", "==");
    PXOPT_COPY_STR(config->args, whereMD, "-filter", "stackRun.filter", "LIKE");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("staticskytool_pendingskycalrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!psListLength(whereMD->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(whereMD);
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(whereMD, NULL);
    psStringAppend(&query, "\n AND %s", whereClause);
    psFree(whereClause);
    psFree(whereMD);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, "\n%s", limitString);
        psFree(limitString);
    }

    // the where clause is required and matches the WHERE hook format string
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
        psTrace("staticskytool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "pendingskycalRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);
    return true;
}

static bool addskycalresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(skycal_id, config->args, "-skycal_id", true, false);

    // optional
    PXOPT_LOOKUP_F32(sigma_ra, config->args,  "-sigma_ra", false, false);
    PXOPT_LOOKUP_F32(sigma_dec, config->args, "-sigma_dec", false, false);

    PXOPT_LOOKUP_F32(zpt_obs, config->args,   "-zpt_obs", false, false);
    PXOPT_LOOKUP_F32(zpt_stdev, config->args, "-zpt_stdev", false, false);
    PXOPT_LOOKUP_F32(fwhm_major, config->args, "-fwhm_major", false, false);
    PXOPT_LOOKUP_F32(fwhm_minor, config->args, "-fwhm_minor", false, false);


    PXOPT_LOOKUP_F32(dtime_script, config->args,   "-dtime_script", false, false);
    PXOPT_LOOKUP_F32(dtime_astrom, config->args,   "-dtime_astrom", false, false);

    PXOPT_LOOKUP_STR(hostname, config->args,    "-hostname", false, false);
    PXOPT_LOOKUP_S32(n_astrom, config->args,    "-n_astrom", false, false);
    PXOPT_LOOKUP_S32(n_detections, config->args,"-n_detections", false, false);
    PXOPT_LOOKUP_S32(n_extended, config->args,  "-n_extended", false, false);
    PXOPT_LOOKUP_S32(n_forced, config->args,    "-n_forced", false, false);

    PXOPT_LOOKUP_STR(path_base, config->args,   "-path_base", false, false);
    PXOPT_LOOKUP_S16(fault, config->args,       "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args,     "-quality", false, false);

    PXOPT_LOOKUP_STR(ver_pslib, config->args,   "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_psphot, config->args,  "-ver_psphot", false, false);
    PXOPT_LOOKUP_STR(ver_psastro, config->args, "-ver_psastro", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);

    psString software_ver = NULL;
    if ((ver_pslib)&&(ver_psmodules)) {
      software_ver = pxMergeCodeVersions(ver_pslib,ver_psmodules);
    }
    if (ver_psphot) {
      software_ver = pxMergeCodeVersions(software_ver,ver_psphot);
    }
    if (ver_psastro) {
      software_ver = pxMergeCodeVersions(software_ver,ver_psastro);
    }
    if (ver_ppstats) {
      software_ver = pxMergeCodeVersions(software_ver,ver_ppstats);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    skycalResultRow *row = skycalResultRowAlloc(
        skycal_id,
        path_base,
        dtime_script,
        dtime_astrom,
        sigma_ra,
        sigma_dec,
        n_astrom,
        n_detections,
        n_extended,
        n_forced,
        zpt_obs,
        zpt_stdev,
        fwhm_major,
        fwhm_minor,
        quality,
        software_ver,
        hostname,
        fault
        );

    if (!skycalResultInsertObject(config->dbh, row)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(row);
        return false;
    }

    psFree(row);

    if (fault) {
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        return true;
    }

    psString query = "UPDATE skycalRun SET state = 'full' WHERE skycal_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, skycal_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}
static bool updateskycalresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-set_fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-skycal_id",   "skycal_id",   "==");

    if (!pxSetFaultCode(config->dbh, "skycalResult", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
    }
    psFree (where);
    return true;
}
static bool skycalresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-skycal_id",  "skycalRun.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-sky_id",     "skycalRun.sky_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",   "stackRun.stack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id",    "stackRun.tess_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter",     "stackRun.filter", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-label",      "skycalRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "skycalRun.data_group", "LIKE");
    PXOPT_COPY_S16(config->args, where, "-quality",     "skycalResult.quality", "==");
    PXOPT_COPY_S16(config->args, where, "-fault",      "skycalResult.fault", "==");

    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("staticskytool_skycalresult.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!psListLength(where->list)) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters are required");
        return false;
    }
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);
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
        psTrace("staticskytool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "staticskyResult", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}
static bool revertskycalresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-skycal_id", "skycalResult.skycal_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "skycalRun.label", "==");
    pxAddLabelSearchArgs(config, where, "-data_group", "skycalRun.data_group", "==");
    pxAddLabelSearchArgs(config, where, "-filter", "stackRun.filter", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "skycalResult.fault", "==");
    if (!pxskycellAddWhere(config, where)) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to add skycell search arguments");
        return false;
    }

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // Delete product
    psString delete = pxDataGet("staticskytool_revertskycal.sql");
    if (!delete) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&delete, " AND %s", whereClause);
        psFree(whereClause);
    }

    if (!p_psDBRunQuery(config->dbh, delete)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(delete);
        psFree(where);
        return false;
    }
    psFree(delete);

    int numRows = psDBAffectedRows(config->dbh); // Number of row affected
    psLogMsg("staticskytool", PS_LOG_INFO, "Deleted %d rows", numRows);

    psFree(where);
    return true;
}

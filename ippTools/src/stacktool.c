/*
 * stacktool.c
 *
 * Copyright (C) 2007-2008  Joshua Hoblitt
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
#include "stacktool.h"

static bool definebyqueryMode(pxConfig *config);
static bool definerunMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool addinputskyfileMode(pxConfig *config);
static bool inputskyfileMode(pxConfig *config);
static bool tosumMode(pxConfig *config);
static bool tobkgMode(pxConfig *config);
static bool addsumskyfileMode(pxConfig *config);
static bool sumskyfileMode(pxConfig *config);
static bool sassskyfileMode(pxConfig *config);
static bool updatesassMode(pxConfig *config);
static bool revertsumskyfileMode(pxConfig *config);
static bool tosummaryMode(pxConfig *config);
static bool addsummaryMode(pxConfig *config);
static bool revertsummaryMode(pxConfig *config);
static bool summaryMode(pxConfig *config);
static bool pendingcleanuprunMode(pxConfig *config);
static bool pendingcleanupskyfileMode(pxConfig *config);
static bool donecleanupMode(pxConfig *config);
static bool updatesumskyfileMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);
static bool importexternalMode(pxConfig *config);
static bool addexternalMode(pxConfig *config);

static bool setstackRunState(pxConfig *config, psS64 stack_id, const char *state);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = stacktoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(STACKTOOL_MODE_DEFINEBYQUERY,         definebyqueryMode);
        MODECASE(STACKTOOL_MODE_DEFINERUN,             definerunMode);
        MODECASE(STACKTOOL_MODE_UPDATERUN,             updaterunMode);
        MODECASE(STACKTOOL_MODE_ADDINPUTSKYFILE,       addinputskyfileMode);
        MODECASE(STACKTOOL_MODE_INPUTSKYFILE,          inputskyfileMode);
        MODECASE(STACKTOOL_MODE_TOSUM,                 tosumMode);
	MODECASE(STACKTOOL_MODE_TOBKG,                 tobkgMode);
        MODECASE(STACKTOOL_MODE_ADDSUMSKYFILE,         addsumskyfileMode);
        MODECASE(STACKTOOL_MODE_SUMSKYFILE,            sumskyfileMode);
        MODECASE(STACKTOOL_MODE_SASSSKYFILE,           sassskyfileMode);
	MODECASE(STACKTOOL_MODE_UPDATESASS,            updatesassMode);
        MODECASE(STACKTOOL_MODE_REVERTSUMSKYFILE,      revertsumskyfileMode);
        MODECASE(STACKTOOL_MODE_TOSUMMARY,             tosummaryMode);
        MODECASE(STACKTOOL_MODE_ADDSUMMARY,            addsummaryMode);
	MODECASE(STACKTOOL_MODE_REVERTSUMMARY,         revertsummaryMode);
        MODECASE(STACKTOOL_MODE_SUMMARY,               summaryMode);
        MODECASE(STACKTOOL_MODE_PENDINGCLEANUPRUN,     pendingcleanuprunMode);
        MODECASE(STACKTOOL_MODE_PENDINGCLEANUPSKYFILE, pendingcleanupskyfileMode);
        MODECASE(STACKTOOL_MODE_DONECLEANUP,           donecleanupMode);
        MODECASE(STACKTOOL_MODE_UPDATESUMSKYFILE,      updatesumskyfileMode);
        MODECASE(STACKTOOL_MODE_EXPORTRUN,             exportrunMode);
        MODECASE(STACKTOOL_MODE_IMPORTRUN,             importrunMode);
        MODECASE(STACKTOOL_MODE_IMPORTEXTERNAL,        importexternalMode);
        MODECASE(STACKTOOL_MODE_ADDEXTERNAL,           addexternalMode);
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
//stackAssociationRow *association = pxStackAssociationDefine(data_group,tess_id,filter,skycell_id);
stackAssociationRow *pxStackAssociationDefine(pxConfig *config, psS64 stack_id) {
  psString select = pxDataGet("stacktool_associationdefine_select.sql");
  if (!select) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }

  psString idString = NULL;
  psStringAppend(&idString, "%" PRId64, stack_id);
  // Copy string to get around the issue with psStringSubstitute not believing
  // that select is a psString.
  psString rep = psStringCopy(select);
  psFree(select);
  select = rep;
  psStringSubstitute(&select, idString, "@STACK_ID@");
  psFree(idString);

  if (!p_psDBRunQuery(config->dbh, select)) {
    psError(PS_ERR_UNKNOWN,false, "database error");
    psFree(select);
    return(NULL);
  }
  psFree(select);
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
    return(NULL);
  }
  if (psArrayLength(output) != 1) {
    psWarning("stacktool: incorrect number of rows found");
    psFree(output);
    return(NULL);
  }
  psMetadata *outrow = psMetadataAlloc();
  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i];

/*     printf("%" PRId64 " %s %s %s %s\n",psMetadataLookupS64(NULL,row,"sass_id"), */
/*            psMetadataLookupStr(NULL,row,"data_group"), */
/*            psMetadataLookupStr(NULL,row,"tess_id"), */
/*            psMetadataLookupStr(NULL,row,"filter"), */
/*            psMetadataLookupStr(NULL,row,"projection_cell")); */

    if (psMetadataLookupS64(NULL,row,"sass_id") == PS_MAX_S64) {
      psMetadataAddS64(outrow,PS_LIST_TAIL,"sass_id",PS_META_REPLACE,"",0);
    }
    else {
      psMetadataAddS64(outrow,PS_LIST_TAIL,"sass_id",PS_META_REPLACE,"",psMetadataLookupS64(NULL,row,"sass_id"));
    }
    psMetadataAddStr(outrow,PS_LIST_TAIL,"data_group",PS_META_REPLACE,"",psMetadataLookupStr(NULL,row,"data_group"));
    psMetadataAddStr(outrow,PS_LIST_TAIL,"tess_id",PS_META_REPLACE,"",psMetadataLookupStr(NULL,row,"tess_id"));
    psMetadataAddStr(outrow,PS_LIST_TAIL,"filter",PS_META_REPLACE,"",psMetadataLookupStr(NULL,row,"filter"));
    psMetadataAddStr(outrow,PS_LIST_TAIL,"projection_cell",PS_META_REPLACE,"",
                     psMetadataLookupStr(NULL,row,"projection_cell"));
  }
/*   printf("%" PRId64 " %s %s %s %s\n",psMetadataLookupS64(NULL,outrow,"sass_id"), */
/*          psMetadataLookupStr(NULL,outrow,"data_group"), */
/*          psMetadataLookupStr(NULL,outrow,"tess_id"), */
/*          psMetadataLookupStr(NULL,outrow,"filter"), */
/*          psMetadataLookupStr(NULL,outrow,"projection_cell")); */


  psFree(output);
  stackAssociationRow *sassRow = stackAssociationObjectFromMetadata(outrow);
  psFree(outrow);
  return(sassRow);
}






static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required options
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false);

    // optional
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);

    psMetadata *where = psMetadataAlloc();
    psMetadata *having = psMetadataAlloc(); // HAVING clause

    // select based on properties of the raw exposures
    PXOPT_COPY_STR(config->args,  where, "-select_comment",            "rawExp.comment", "LIKE");
    PXOPT_COPY_STR(config->args,  where, "-select_inst",               "rawExp.camera", "==");
    PXOPT_COPY_STR(config->args,  where, "-select_telescope",          "rawExp.telescope", "==");
    PXOPT_COPY_STR(config->args,  where, "-select_filter",             "rawExp.filter", "==");
    PXOPT_COPY_STR(config->args,  where, "-select_uri",                "rawExp.uri", "==");
    PXOPT_COPY_TIME(config->args, where, "-select_dateobs_begin",      "rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-select_dateobs_end",        "rawExp.dateobs", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_airmass_min",        "rawExp.airmass", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_airmass_max",        "rawExp.airmass", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_exp_time_min",       "rawExp.exp_time", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_exp_time_max",       "rawExp.exp_time", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_ccd_temp_min",       "rawExp.ccd_temp", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_ccd_temp_max",       "rawExp.ccd_temp", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_posang_min",         "rawExp.posang", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_posang_max",         "rawExp.posang", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_sun_angle_min",      "rawExp.sun_angle", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_sun_angle_max",      "rawExp.sun_angle", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_fwhm_major_min",     "camProcessedExp.fwhm_major", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_fwhm_major_max",     "camProcessedExp.fwhm_major", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_fwhm_minor_min",     "camProcessedExp.fwhm_minor", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_fwhm_minor_max",     "camProcessedExp.fwhm_minor", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_elong_max",          "(camProcessedExp.fwhm_major / camProcessedExp.fwhm_minor)", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_iq_elong_max",       "(camProcessedExp.iq_fwhm_major / camProcessedExp.iq_fwhm_minor)", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_iq_m2_max",          "camProcessedExp.iq_m2", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_iq_m2_min",          "camProcessedExp.iq_m2", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_iq_m3_max",          "camProcessedExp.iq_m3", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_iq_m4_min",          "camProcessedExp.iq_m4", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_iq_m4_max",          "camProcessedExp.iq_m4", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_zpt_obs_min",        "camProcessedExp.zpt_obs", ">=");
    PXOPT_COPY_F32(config->args,  where, "-select_zpt_obs_max",        "camProcessedExp.zpt_obs", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_bg_max",             "camProcessedExp.bg", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_bg_stdev_max",       "camProcessedExp.bg_stdev", "<=");
    PXOPT_COPY_F32(config->args,  where, "-select_astrom",             "sqrt(POWER(camProcessedExp.sigma_ra, 2) + POWER(camProcessedExp.sigma_dec, 2))", "<=");

    PXOPT_COPY_STR(config->args,  where, "-select_exp_type",           "rawExp.exp_type", "==");
    PXOPT_COPY_F32(config->args,  where, "-select_good_frac_min",      "warpSkyfile.good_frac", ">=");
    pxAddLabelSearchArgs(config,  where, "-select_skycell_id",         "warpSkyfile.skycell_id", "LIKE");
    pxAddLabelSearchArgs(config,  where, "-select_data_group",         "warpRun.data_group", "LIKE");
    pxAddLabelSearchArgs (config, where, "-select_label",              "warpRun.label", "LIKE"); // define using warp label
    pxAddLabelSearchArgs (config, where, "-warp_id",                   "warpRun.warp_id", "==");
    
    // Add position dependence here.
    if (!pxspaceBoxAddWhere(config, where)) {
      psError(psErrorCodeLast(), false, "pxSpaceBoxAddWhere failed");
      return false;
    }
  
    // these are used to build the HAVING restriction
    PXOPT_COPY_S32(config->args, having, "-min_num", "num_warp", ">=");
    PXOPT_COPY_S32(config->args, having, "-max_num", "num_warp", "<=");
    
    PXOPT_LOOKUP_S32(min_num,     config->args, "-min_num",  false, false);
    if (min_num < 2) {
        psError(PXTOOLS_ERR_CONFIG, true, "Require at least two inputs for a stack, but min_num = %d",
                min_num);
        psFree(where);
        psFree(having);
        return false;
    }

    // other options applied outside of the WHERE
    PXOPT_LOOKUP_S32(randomLimit, config->args, "-random",   false, false);
    PXOPT_LOOKUP_S32(min_new,     config->args, "-min_new",  false, false);
    PXOPT_LOOKUP_F32(min_frac,    config->args, "-min_frac", false, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    if (!psListLength(where->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        psFree(where);
        psFree(having);
        return false;
    }

    psString select = pxDataGet("stacktool_definebyquery_select.sql");
    if (!select) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        psFree(having);
        return false;
    }

    psString where1 = psStringCopy("");
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&where1, "\nAND %s", whereClause);
        psFree(whereClause);
    }

    psString where2 = psStringCopy("");
    if (label) {
        psStringAppend(&where2, "\nWHERE stackRun.label = '%s'", label);
    }

    // Restriction on aggregated quantities using HAVING
    {
        psString havingClause = NULL;   // HAVING string
        if (psListLength(having->list)) {
            havingClause = psDBGenerateWhereConditionSQL(having, NULL);
        }

        if (min_new > 0) {
            if (havingClause) {
                psStringAppend(&havingClause, " AND");
            }
            psStringAppend(&havingClause,
                           " (num_warp - num_stack >= %d OR (num_warp >= %d AND num_stack IS NULL))",
                           min_new, min_new);
        }
        if (isfinite(min_frac)) {
            if (havingClause) {
                psStringAppend(&havingClause, " AND");
            }
            // Avoiding division by zero
            psStringAppend(&havingClause, " (num_warp >= %f * num_stack OR num_stack IS NULL)",
                           (double)min_frac);
        }
        if (havingClause) {
            psStringAppend(&select, " HAVING %s", havingClause);
            psFree(havingClause);
        }
    }
    psFree(having);

    if (!p_psDBRunQueryF(config->dbh, select, where1, where2)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(select);
        psFree(where1);
        psFree(where2);
        return false;
    }
    psFree(select);
    psFree(where1);
    psFree(where2);

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
        psWarning("stacktool: no rows found");
        psFree(output);
        psFree(where);
        return true;
    }
    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "stackSkycells", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            psFree(where);
            return false;
        }
        psFree(output);
        psFree(where);
        return true;
    }

    psString insert = NULL;             // Insertion query
    if (randomLimit > 0) {
        insert = pxDataGet("stacktool_definebyquery_insert_random_part1.sql");
    } else {
        insert = pxDataGet("stacktool_definebyquery_insert.sql");
    }
    if (!insert) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&insert, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (randomLimit > 0) {
        psString part2 = pxDataGet("stacktool_definebyquery_insert_random_part2.sql");
        if (!part2) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(insert);
            return false;
        }
        psStringAppend(&insert, "%s", part2);
        psFree(part2);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(where);
        return false;
    }

    psArray *list = psArrayAllocEmpty(16); // List of runs, to print
    for (long i = 0; i < output->n; i++) {
        psMetadata *row = output->data[i]; // Row from select
        bool status;

        // pull out the skycell_id, tess_id, filter
        psString skycell_id = psMetadataLookupStr(&status, row, "skycell_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup skycell_id");
            psFree(output);
            psFree(insert);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psString tess_id = psMetadataLookupStr(&status, row, "tess_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup tess_id");
            psFree(output);
            psFree(insert);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psString filter = psMetadataLookupStr(&status, row, "filter");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup filter");
            psFree(output);
            psFree(insert);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        // create a new stackRun for this stack
        stackRunRow *run = stackRunRowAlloc(
            0,                          // ID
            "new",                      // state
            workdir,
            label,
            data_group ? data_group : label,
            dist_group,
            reduction,
            dvodb,
            registered,
            skycell_id,
            tess_id,
            filter,
            NULL, // software_ver
            note);

        if (!stackRunInsertObject(config->dbh, run)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
            psFree(run);
            psFree(insert);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        // figure out the new stack_id
        psS64 stack_id = psDBLastInsertID(config->dbh);
        run->stack_id = stack_id;

        psArrayAdd(list, list->n, run);
        psFree(run);

        //CZW Add an association entry here.
        // Define the requested association, and insert it if it doesn't already exist
        stackAssociationRow *association = pxStackAssociationDefine(config,stack_id);
        psS64 sass_id=-1;
        if (!association->sass_id) {
          psTrace("stacktool.association",2,"No required Association found. Adding.");

          if (!stackAssociationInsertObject(config->dbh,association)) {
            if (!psDBRollback(config->dbh)) {
              psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
            psFree(insert);
            psFree(list);
            psFree(association);
            if (!psDBRollback(config->dbh)) {
              psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return(false);
          }
          sass_id = psDBLastInsertID(config->dbh);
          association->sass_id = sass_id;
        }
	else {
	  sass_id = association->sass_id;
	}
        // Insert the map entry for this row.
        stackAssociationMapRow *maprow = stackAssociationMapRowAlloc(sass_id,stack_id);
        if (!stackAssociationMapInsertObject(config->dbh,maprow)) {
          if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
          }
          psError(PS_ERR_UNKNOWN, false, "database error");
          psFree(output);
          psFree(insert);
          psFree(list);
          psFree(association);
          if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
          }
          return(false);
        }


        // Create a suitable insertion query for this run
        psString thisInsert = psStringCopy(insert);
        psString idString = NULL;
        psStringAppend(&idString, "%" PRId64, stack_id);
        psStringSubstitute(&thisInsert, idString, "@STACK_ID@");
        psFree(idString);

        // replace @FILTER@, @SKYCELL_ID@, @RANDOM_LIMIT@
        psStringSubstitute(&thisInsert, filter, "@FILTER@");
        psStringSubstitute(&thisInsert, skycell_id, "@SKYCELL_ID@");

        if (randomLimit > 0) {
          psString limString = NULL;
          psStringAppend(&limString, "%d", randomLimit);
          psStringSubstitute(&thisInsert, limString, "@RANDOM_LIMIT@");
          psFree(limString);
        }

        // XXX this insert uses a select to generate the list of warp_ids for the stack,
        // we have applied a set of criteria above (WHERE) to select the relevant warps
        // this insert needs to use exactly the same restrictions (race condition is probably not critical)
        // the insert below seems to only restrict matches to the skycell, tess, and filter
        if (!p_psDBRunQuery(config->dbh, thisInsert)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(thisInsert);
            psFree(insert);
            psFree(output);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(thisInsert);

# if (0)
        {
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
            psWarning("stacktool (definebyquery, insert): no rows found");
            psFree(output);
            psFree(where);
            return true;
          }
          // negative simple so the default is true
          if (!ippdbPrintMetadatas(stdout, output, "stackSkycells", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            psFree(where);
            return false;
          }
          psFree(output);
          psFree(where);
          return true;
        }
# endif

    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(list);
        return false;
    }

    if (!stackRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }
    psFree(list);
    return true;
}


static bool definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required options
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", true, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", true, false);

    // default
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);

    // options
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);

    // we have to support multipe exp_ids
    psMetadataItem *warp_ids = psMetadataLookup(config->args, "-warp_id");
    if (!warp_ids) {
        // this shouldn't actually happen when using psArgs
        psError(PS_ERR_UNKNOWN, true, "-warp_id is required");
        return false;
    }

    stackRunRow *run = stackRunRowAlloc(
        0,                              // ID
        "new",                          // state
        workdir,
        label,
        data_group ? data_group : label,
        dist_group,
        reduction,
        dvodb,
        registered,
        skycell_id,
        tess_id,
        filter,
        NULL, // software_ver
        note);

    if (!run) {
        psError(PS_ERR_UNKNOWN, false, "failed to alloc stackRun object");
        return true;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!stackRunInsertObject(config->dbh, run)) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(run);
        return true;
    }

    // get the assigned warp_id
    run->stack_id = psDBLastInsertID(config->dbh);

    //CZW Add an association entry here.

    // insert the stackInputSkyfile rows
    psListIterator *iter = psListIteratorAlloc(warp_ids->data.list, 0, false);
    psMetadataItem *item = NULL;
    while ((item = psListGetAndIncrement(iter))) {
        // if the value is NULL this is probably the first pass through the
        // loop and -warp_id was not specified at all
        if (!item->data.V) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for -warp_id");
            return false;
        }
        if (!stackInputSkyfileInsert(config->dbh, run->stack_id, item->data.S64)) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to insert stackInputSkyfile rows");
            return false;
        }
    }
    psFree(iter);

    // point of no return
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!stackRunPrintObject(stdout, run, !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print object");
            psFree(run);
            return false;
    }

    psFree(run);

    return true;
}


static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

#ifdef notdef
    PXOPT_LOOKUP_S64(stack_id, config->args, "-stack_id", false, false);
    PXOPT_LOOKUP_STR(state, config->args, "-state", true, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
#endif
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-stack_id",  "stackRun.stack_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-label",     "stackRun.label",     "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "stackRun.data_group", "==");
    PXOPT_COPY_STR(config->args, where, "-state",     "stackRun.state",     "==");

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S64(sass_id, config->args, "-sass_id", false, false);

    if (sass_id) {
      PXOPT_COPY_S64(config->args, where, "-sass_id",   "stackAssociationMap.sass_id",  "==");
    }
    
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    //CZW join against stackAssociationMap
    psString query = psStringCopy("UPDATE stackRun");

    if (fault) {
        psStringAppend(&query, " JOIN stackSumSkyfile USING(stack_id)");
        PXOPT_COPY_S16(config->args, where, "-fault", "stackSumSkyfile.fault", "==");
    }
    if (sass_id) {
      psStringAppend(&query, " JOIN stackAssociationMap USING(stack_id)");
    }
    
    // pxUpdateRun gets parameters from config->args and updates
    bool result = pxUpdateRun(config, where, &query, "stackRun", "stack_id", "stackSumSkyfile", true, false);

    psFree(query);
    psFree(where);

    return result;

#ifdef notdef
    // Hack-y work around to make stacktool more like the other tools, without breaking other stuff (hopefully).

    if ((state)&&(stack_id)) {
        // set detRun.state to state
        return setstackRunState(config, stack_id, state);
    }

    if ((state)&&(label)) {
      return setstackRunStateByLabel(config, label, state);
    }

    psError(PS_ERR_UNKNOWN, false, "Required options not found.");
    return false;
#endif
}


static bool addinputskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(stack_id, config->args, "-stack_id", true, false);
    PXOPT_LOOKUP_S64(warp_id, config->args, "-warp_id", true, false);

    // XXX need to validate the warp_id here
    // XXX instead of validiting it here we should just use forgein key
    // constrants
    if (!stackInputSkyfileInsert(config->dbh,
            stack_id,
            warp_id
        )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool inputskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // XXX require at least a stack id (add better search options)
    // PXOPT_LOOKUP_S64(stack_id, config->args, "-stack_id", true, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stack_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("stacktool_inputskyfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "stackInputSkyfile");
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "stackInputSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool tosumMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_id", "warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stack_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "stackRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(ignore_warp_state, config->args, "-ignore-warp-state", false);

    psString query = ignore_warp_state ? pxDataGet("stacktool_tosum_allstates.sql") : pxDataGet("stacktool_tosum.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = psStringCopy(""); // WHERE conditions to add
    if (psListLength(where->list)) {
        psString new = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereClause, "\nAND %s", new);
        psFree(new);
    }
    psFree(where);

    psStringAppend(&query, "\nORDER by priority DESC, stack_id");

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereClause)) {
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "stackSumSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool tobkgMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stack_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "stackRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("stacktool_tobkg.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = psStringCopy(""); // WHERE conditions to add
    if (psListLength(where->list)) {
        psString new = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereClause, "\nAND %s", new);
        psFree(new);
    }
    psFree(where);

    psStringAppend(&query, "\nORDER by priority DESC, stack_id");

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereClause)) {
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "stackBkgSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool addsumskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(stack_id, config->args, "-stack_id", true, false);

    // optional
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F32(dtime_stack, config->args, "-dtime_stack", false, false);
    PXOPT_LOOKUP_F32(dtime_match_mean, config->args, "-dtime_match_mean", false, false);
    PXOPT_LOOKUP_F32(dtime_match_stdev, config->args, "-dtime_match_stdev", false, false);
    PXOPT_LOOKUP_F32(dtime_initial, config->args, "-dtime_initial", false, false);
    PXOPT_LOOKUP_F32(dtime_reject, config->args, "-dtime_reject", false, false);
    PXOPT_LOOKUP_F32(dtime_final, config->args, "-dtime_final", false, false);
    PXOPT_LOOKUP_F32(dtime_convolve, config->args, "-dtime_convolve", false, false);
    PXOPT_LOOKUP_F32(dtime_phot, config->args, "-dtime_phot", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_F32(match_mean, config->args, "-match_mean", false, false);
    PXOPT_LOOKUP_F32(match_stdev, config->args, "-match_stdev", false, false);
    PXOPT_LOOKUP_F32(match_rms, config->args, "-match_rms", false, false);
    PXOPT_LOOKUP_F32(stamps_mean, config->args, "-stamps_mean", false, false);
    PXOPT_LOOKUP_F32(stamps_stdev, config->args, "-stamps_stdev", false, false);
    PXOPT_LOOKUP_S32(stamps_min, config->args, "-stamps_min", false, false);
    PXOPT_LOOKUP_S32(reject_images, config->args, "-reject_images", false, false);
    PXOPT_LOOKUP_F32(reject_pix_mean, config->args, "-reject_pix_mean", false, false);
    PXOPT_LOOKUP_F32(reject_pix_stdev, config->args, "-reject_pix_stdev", false, false);
    PXOPT_LOOKUP_S32(sources, config->args, "-sources", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_F32(good_frac, config->args, "-good_frac", false, false);
    PXOPT_LOOKUP_F64(mjd_obs, config->args, "-mjd_obs", false, false);

    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_psphot, config->args, "-ver_psphot", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_STR(ver_ppstack, config->args, "-ver_ppstack", false, false);
    PXOPT_LOOKUP_STR(ver_streaks, config->args, "-ver_streaks", false, false);

    PXOPT_LOOKUP_S16(background_model, config->args, "-background_model", false, false);
    
    psTrace("czw.test",1,"Received versions: pslib %s psmodules %s psphot %s ppstats %s ppstack %s streaks %s\n",
            ver_pslib,ver_psmodules,ver_psphot,ver_ppstats,ver_ppstack,ver_streaks);
    psString software_ver = NULL;
    if ((ver_pslib)&&(ver_psmodules)) {
      software_ver = pxMergeCodeVersions(ver_pslib,ver_psmodules);
    }
    if (ver_psphot) {
      software_ver = pxMergeCodeVersions(software_ver,ver_psphot);
    }
    if (ver_ppstats) {
      software_ver = pxMergeCodeVersions(software_ver,ver_ppstats);
    }
    if (ver_ppstack) {
      software_ver = pxMergeCodeVersions(software_ver,ver_ppstack);
    }
    if (ver_streaks) {
      software_ver = pxMergeCodeVersions(software_ver,ver_streaks);
    }

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // XXX need to validate the stack_id here
    // XXX instead of validiting it here we should just use forgein key
    // constrants
    if (!stackSumSkyfileInsert(config->dbh,
                               stack_id,
                               uri,
                               path_base,
                               bg,
                               bg_stdev,
                               dtime_stack,
                               dtime_match_mean,
                               dtime_match_stdev,
			       dtime_convolve,
                               dtime_initial,
                               dtime_reject,
                               dtime_final,
                               dtime_phot,
                               dtime_script,
                               match_mean,
                               match_stdev,
                               match_rms,
                               stamps_mean,
                               stamps_stdev,
                               stamps_min,
                               reject_images,
                               reject_pix_mean,
                               reject_pix_stdev,
                               sources,
                               hostname,
                               good_frac,
                               mjd_obs,
                               fault,
                               software_ver,
			       background_model,
                               quality
          )) {
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (fault == 0) {
        // Set stackRun software if we are finished.
        if (!pxSetRunSoftware(config, "stackRun", "stack_id", stack_id, software_ver)) {
          if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
          }
          psError(PS_ERR_UNKNOWN, false, "failed to set stackRun.software_ver for stack_id: %" PRId64,
                  stack_id);
          return(false);
        }

        if (!setstackRunState(config, stack_id, "full")) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to change stackRun's state");
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


static bool sumskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stackSumSkyfile.stack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "stackRun.tess_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter", "stackRun.filter", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-state", "stackRun.state", "==");
    pxAddLabelSearchArgs(config, where, "-label", "stackRun.label", "LIKE");
    pxAddLabelSearchArgs(config, where, "-data_group", "stackRun.data_group", "LIKE");
    PXOPT_COPY_S16(config->args, where, "-fault", "stackSumSkyfile.fault", "==");
    PXOPT_COPY_F64(config->args, where, "-mjd_obs_begin", "stackSumSkyfile.mjd_obs", ">=");
    PXOPT_COPY_F64(config->args, where, "-mjd_obs_end", "stackSumSkyfile.mjd_obs", "<=");
    PXOPT_COPY_S16(config->args, where, "-background_model","stackSumSkyfile.background_model", "==");

//  The following three selectors are incompatible with the sql so omit them
//    PXOPT_COPY_S64(config->args, where, "-warp_id", "warpRun.warp_id", "==");
//     PXOPT_COPY_S64(config->args, where, "-exp_id", "rawExp.exp_id", "==");
//    PXOPT_COPY_STR(config->args, where, "-exp_name", "rawExp.exp_name", "==");

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("stacktool_sumskyfile.sql");
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "stackSumSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool sassskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sass_id", "stackAssociation.sass_id", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "stackAssociation.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-projection_cell", "stackAssociation.projection_cell", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "stackAssociation.filter", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-data_group", "stackAssociation.data_group", "LIKE");

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("stacktool_sassskyfile.sql");
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

    // This needs to be sorted by skycell_id, or the ppSkycell calls to mosaic things
    // don't work properly.
    psStringAppend(&query, " ORDER BY stackRun.skycell_id ");
    
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "stackSumSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool updatesassMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-sass_id", "sass_id", "==");
  PXOPT_COPY_STR(config->args, where, "-data_group", "data_group", "==");
  PXOPT_COPY_STR(config->args, where, "-projection_cell", "projection_cell", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id", "tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");
  if (!psListLength(where->list)) {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }

  psMetadata *values = psMetadataAlloc();
  PXOPT_COPY_STR(config->args, values, "-set_data_group", "data_group", "==");
  if (!psListLength(values->list)) {
    psFree(values);
    psError(PXTOOLS_ERR_CONFIG, false, "data_group to assign not found");
    return false;
  }

  if (!psDBTransaction(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }
  
  long rows = psDBUpdateRows(config->dbh, "stackAssociation", where, values);
  psFree(values);
  psFree(where);

  if (rows != 1) {
    psError(PS_ERR_UNKNOWN, false, "should have updated only one row. updated %ld", rows);
    return false;
  }

  if (!psDBCommit(config->dbh)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return false;
  }

  return true;
}

static bool revertsumskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stackSumSkyfile.stack_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "stackRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "stackSumSkyfile.fault", "==");

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // Delete product
    psString delete = pxDataGet("stacktool_revertsumskyfile_delete.sql");
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
    psLogMsg("stacktool", PS_LOG_INFO, "Deleted %d rows", numRows);

    psFree(where);

    return true;
}


static bool setstackRunState(pxConfig *config, psS64 stack_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid stackRun state: %s", state);
        return false;
    }

    char *query = "UPDATE stackRun SET state = '%s' WHERE stack_id = %"PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, stack_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for stack_id %"PRId64, stack_id);
        return false;
    }

    return true;
}

#ifdef notdef
static bool setstackRunStateByLabel(pxConfig *config, const char *label, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!pxIsValidState(state)) {
        psError(PS_ERR_UNKNOWN, false, "invalid stackRun state: %s", state);
        return false;
    }

    char *query = "UPDATE stackRun SET state = '%s' WHERE label = '%s'";
    if (!p_psDBRunQueryF(config->dbh, query, state, label)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for label %s", label);
        return false;
    }

    return true;
}
#endif
static bool tosummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-stack_id",    "stackSumSkyfile.warp_id", "==");
  PXOPT_COPY_S64(config->args, where, "-sass_id",     "stackAssociationMap.sass_id", "==");
  PXOPT_COPY_STR(config->args, where, "-tess_id",    "stackSumSkyfile.tess_id", "==");
  PXOPT_COPY_STR(config->args, where, "-state",      "stackRun.state", "==");
  PXOPT_COPY_STR(config->args, where, "-filter",    "stackRun.filter", "LIKE");
  pxAddLabelSearchArgs (config, where, "-label",   "stackRun.label", "LIKE");
  pxAddLabelSearchArgs (config, where, "-data_group",   "stackRun.data_group", "LIKE");

  PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  // find all rawImfiles matching the default query
  psString query = pxDataGet("stacktool_tosummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return false;
  }

  // generate where strings for arguments that require extra processing
  // beyond PXOPT_COPY*
  psString whereClause;
  if (psListLength(where->list)) {
    whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringPrepend(&whereClause, "\n AND ");
  } else if (!all) {
    psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
    psFree(whereClause);
    return false;
  }

  psFree(where);

  // treat limit == 0 as "no limit"
  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s", limitString);
    psFree(limitString);
  }

  if (!p_psDBRunQueryF(config->dbh, query, whereClause)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(whereClause);
    psFree(query);
    return false;
  }
  psFree(whereClause);
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
    psTrace("stacktool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  if (psArrayLength(output)) {
    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "stackRun", !simple)) {
      psError(PS_ERR_UNKNOWN, false, "failed to print array");
      psFree(output);
      return false;
    }
  }

  psFree(output);
  return(true);
}
static bool addsummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_S64(sass_id, config->args, "-sass_id", true, false);
  PXOPT_LOOKUP_STR(projection_cell, config->args, "-projection_cell", true, false);
  PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

  psString query = pxDataGet("stacktool_addsummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  if (!p_psDBRunQueryF(config->dbh, query, sass_id, projection_cell, path_base)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psS64 numUpdated = psDBAffectedRows(config->dbh);

  if (numUpdated != 1) {
    psError(PS_ERR_UNKNOWN, false, "should have affected 1 row");
    psFree(query);
    return false;
  }

  psFree(query);

  // Print anything here?

  return(true);
}

static bool revertsummaryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, NULL);
  PXOPT_LOOKUP_S64(sass_id, config->args, "-sass_id", true, false);

  psString query = pxDataGet("stacktool_revertsummary.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  if (!p_psDBRunQueryF(config->dbh, query, sass_id)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psS64 numUpdated = psDBAffectedRows(config->dbh);

  if (numUpdated != 1) {
    psError(PS_ERR_UNKNOWN, false, "should have affected 1 row");
    psFree(query);
    return(false);
  }
  psLogMsg("stacktool", PS_LOG_INFO, "Deleted %ld rows", numUpdated);
  
  psFree(query);

  return(true);
}

static bool summaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-sass_id", "stackSummary.sass_id", "==");
    PXOPT_COPY_STR(config->args, where, "-projection_cell", "stackAssociation.projection_cell", "==");
    PXOPT_COPY_STR(config->args, where, "-tess_id", "stackAssociation.tess_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-filter", "stackAssociation.filter", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-stack_id", "stackSumSkyfile.stack_id", "==");
    pxAddLabelSearchArgs(config, where, "-data_group", "stackAssociation.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "LIKE");
//    PXOPT_COPY_STR(config->args, where, "-state", "stackRun.state", "==");
    pxAddLabelSearchArgs(config, where, "-label", "stackRun.label", "LIKE");
//    PXOPT_COPY_S16(config->args, where, "-fault", "stackSumSkyfile.fault", "==");
//    PXOPT_COPY_F64(config->args, where, "-mjd_obs_begin", "stackSumSkyfile.mjd_obs", ">=");
//    PXOPT_COPY_F64(config->args, where, "-mjd_obs_end", "stackSumSkyfile.mjd_obs", "<=");
//    PXOPT_COPY_S16(config->args, where, "-background_model","stackSumSkyfile.background_model", "==");

//    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("stacktool_summary.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters are required");
        return false;
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "stackSummary", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool pendingcleanuprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
/*     PXOPT_COPY_S64(config->args, where, "-stack_id", "stackRun.stack_id", "=="); */
/*     PXOPT_COPY_S64(config->args, where, "-sass_id", "stackAssociationMap.sass_id", "=="); */

    pxAddLabelSearchArgs (config, where, "-label", "stackRun.label", "==");

    psString query = pxDataGet("stacktool_pendingcleanuprun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "stackPendingCleanupRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingcleanupskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(stack_id, config->args, "-stack_id", false, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    if (stack_id) {
        PXOPT_COPY_S64(config->args, where, "-stack_id", "stack_id", "==");
    }
    pxAddLabelSearchArgs (config, where, "-label", "stackRun.label", "==");

    psString query = pxDataGet("stacktool_pendingcleanupskyfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "stackPendingCleanupSkyfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool donecleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");
    PXOPT_COPY_STR(config->args, where, "-sass_id", "sass_id", "==");

    psString query = pxDataGet("stacktool_donecleanup.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
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
        psTrace("stacktool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "stackDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool updatesumskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);
    PXOPT_LOOKUP_S16(background_model, config->args, "-set_background_model", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-stack_id",   "stack_id",   "==");

    if (background_model) {
      psMetadata *values = psMetadataAlloc();
      PXOPT_COPY_S16(config->args, values, "-set_background_model", "background_model", "==");
      long rows = psDBUpdateRows(config->dbh,"stackSumSkyfile", where, values);
      psFree(values);
      if (!rows) {
	// This maybe should rollback and error if rows != 1
	psError(PS_ERR_UNKNOWN, true, "no rows changed");
	return false;
      }
    }
    else {    
      if (!pxSetFaultCode(config->dbh, "stackSumSkyfile", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        psFree (where);
        return false;
      }
    }
    psFree (where);

    return true;
}

//CZW I have not added sass information to the export/import run modes yet.
bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 3;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // PXOPT_LOOKUP_S64(det_id, config->args, "-stack_id", true,  false);
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
  PXOPT_COPY_S64(config->args, where, "-stack_id", "stack_id", "==");

  ExportTable tables [] = {
    {"stackRun", "stacktool_export_run.sql"},
    {"stackInputSkyfile", "stacktool_export_input_skyfile.sql"},
    {"stackSumSkyfile", "stacktool_export_sum_skyfile.sql"},
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
        if (!strcmp(tables[i].tableName, "stackRun")) {
            if (!pxSetStateCleaned("stackRun", "state", output)) {
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
  unsigned int nFail;

  int numImportTables = 2;

  char tables[2] [80] = {"stackInputSkyfile", "stackSumSkyfile"};

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);

  psMetadata *input = psMetadataConfigRead (NULL, &nFail, infile, false);

#ifdef notdef
  fprintf (stderr, "---- input ----\n");
  psMetadataPrint (stderr, input, 1);
#endif

  if (!pxCheckImportVersion(config, input)) {
      psError(PS_ERR_UNKNOWN, false, "pxCheckImportVersion failed");
      return false;
  }
  psMetadataItem *item = psMetadataLookup (input, "stackRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  stackRunRow *stackRun = stackRunObjectFromMetadata (entry->data.md);
  stackRunInsertObject (config->dbh, stackRun);

  // fprintf (stdout, "---- stack run ----\n");
  // psMetadataPrint (stderr, entry->data.md, 1);

  for (int i = 0; i < numImportTables; i++) {
    psMetadataItem *item = psMetadataLookup (input, tables[i]);
    psAssert (item, "entry not in input?");
    psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

    switch (i) {
      case 0:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          stackInputSkyfileRow *stackInputSkyfile = stackInputSkyfileObjectFromMetadata (entry->data.md);
          stackInputSkyfileInsertObject (config->dbh, stackInputSkyfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 1:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          stackSumSkyfileRow *stackSumSkyfile = stackSumSkyfileObjectFromMetadata (entry->data.md);
          stackSumSkyfileInsertObject (config->dbh, stackSumSkyfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;
    }
  }

  return true;
}

// in this mode, the supplied stacks are inserted into the db and the
// StackExternalStack table is updated with the IDs of the inserted stack
bool importexternalMode(pxConfig *config)
{
    unsigned int nFail;

    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);
    PXOPT_LOOKUP_S64(ext_camera_id, config->args, "-ext_camera_id", true, false);

    // optionally let the user change the filter:
    PXOPT_LOOKUP_STR(new_filter, config->args, "-set_filter", false, false);

    psMetadata *input = psMetadataConfigRead (NULL, &nFail, infile, false);

    // XXX for TEST: psMetadataPrint (stderr, input, 1);

    psMetadataItem *itemRun = psMetadataLookup (input, "stackRun");
    psAssert (itemRun, "entry not in input?");
    psAssert (itemRun->type == PS_DATA_METADATA_MULTI, "entry not multi?");

    psMetadataItem *entry = psListGet (itemRun->data.list, 0);
    assert (entry);
    assert (entry->type == PS_DATA_METADATA);

    // grab the data for the stackRun & format as stackRunRow:
    stackRunRow *stackRun = stackRunObjectFromMetadata (entry->data.md);

    // save the original stack_id and insert with stack_id = 0 to force a
    // new iD.  we then need to retrieve that ID and use it below
    // the new stackRun should 

    psS64 ext_stack_id = stackRun->stack_id;
    stackRun->stack_id = 0;

    // update filter as well..
    if (new_filter) {
      stackRun->filter = psMemIncrRefCounter(new_filter);
    }

    // start the transaction so we can ensure stackRun, stackSumSkyfile, stackExternalStack table entries are consistent
    if (!psDBTransaction(config->dbh)) {
	psError(PS_ERR_UNKNOWN, false, "database error");
	return false;
    }

    // now insert the new stackRun object:
    stackRunInsertObject (config->dbh, stackRun);

    // get the new stack_id
    psS64 stack_id = psDBLastInsertID(config->dbh);

    // add an entry in the stackExternalStack table:
    psString query = pxDataGet("stacktool_insertexternal_link.sql");
    if (!query) {
	psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
	return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, stack_id, ext_stack_id, ext_camera_id)) {
	psError(PS_ERR_UNKNOWN,false, "database error");
	return(NULL);
    }

    // now insert the stackSumSkyfile data
    psMetadataItem *itemSum = psMetadataLookup (input, "stackSumSkyfile");

    psAssert (itemSum, "entry not in input?");
    psAssert (itemSum->type == PS_DATA_METADATA_MULTI, "entry not multi?");

    for (int i = 0; i < itemSum->data.list->n; i++) {
	entry = psListGet (itemSum->data.list, i);
	assert (entry);
	assert (entry->type == PS_DATA_METADATA);
	stackSumSkyfileRow *stackSumSkyfile = stackSumSkyfileObjectFromMetadata (entry->data.md);

	// need to update stack_id
	stackSumSkyfile->stack_id = stack_id;
	stackSumSkyfileInsertObject (config->dbh, stackSumSkyfile);

	// fprintf (stdout, "---- row %d ----\n", i);
	// psMetadataPrint (stderr, entry->data.md, 1);
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

bool addexternalMode(pxConfig *config) {

    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_STR(ext_camera, config->args, "-ext_camera", true,  false);

    psString query = pxDataGet("stacktool_addexternal.sql");
    if (!query) {
	psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
	return false;
    }

    psStringSubstitute(&query, ext_camera, "@EXT_CAMERA@");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
	psError(PS_ERR_UNKNOWN,false, "database error");
	return(NULL);
    }

    int ext_camera_id = psDBLastInsertID(config->dbh);
    psLogMsg("stacktool", PS_LOG_INFO, "Added new external camera %s (ID = %d)", ext_camera, ext_camera_id);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

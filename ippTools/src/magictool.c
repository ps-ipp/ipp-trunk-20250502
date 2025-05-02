/*
 * magictool.c
 *
 * Copyright (C) 2006-2007  Joshua Hoblitt
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
#include "magictool.h"

static bool definebyqueryMode(pxConfig *config);
static psS64 definerunMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool addinputskyfileMode(pxConfig *config);
static bool inputskyfileMode(pxConfig *config);
static bool totreeMode(pxConfig *config);
static bool inputtreeMode(pxConfig *config);
static bool reverttreeMode(pxConfig *config);
static bool toprocessMode(pxConfig *config);
static bool addresultMode(pxConfig *config);
static bool revertnodeMode(pxConfig *config);
static bool inputsMode(pxConfig *config);
static bool tomaskMode(pxConfig *config);
static bool addmaskMode(pxConfig *config);
static bool revertmaskMode(pxConfig *config);
static bool maskMode(pxConfig *config);
static bool censorrunMode(pxConfig *config);
static bool exposureMode(pxConfig *config);
static bool setgotocleanedMode(pxConfig *config);
static bool tocleanupMode(pxConfig *config);
static bool setworkdirstateMode(pxConfig *config);

static bool setmagicRunState(pxConfig *config, psS64 magic_id, const char *state, psString setString);
static bool parseAndInsertNodeDeps(pxConfig *config, psS64 magic_id, const char *filename);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = magictoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(MAGICTOOL_MODE_DEFINEBYQUERY,       definebyqueryMode);
        MODECASE(MAGICTOOL_MODE_DEFINERUN,           definerunMode);
        MODECASE(MAGICTOOL_MODE_UPDATERUN,           updaterunMode);
        MODECASE(MAGICTOOL_MODE_ADDINPUTSKYFILE,     addinputskyfileMode);
        MODECASE(MAGICTOOL_MODE_INPUTSKYFILE,        inputskyfileMode);
        MODECASE(MAGICTOOL_MODE_TOTREE,              totreeMode);
        MODECASE(MAGICTOOL_MODE_INPUTTREE,           inputtreeMode);
        MODECASE(MAGICTOOL_MODE_REVERTTREE,          reverttreeMode);
        MODECASE(MAGICTOOL_MODE_TOPROCESS,           toprocessMode);
        MODECASE(MAGICTOOL_MODE_ADDRESULT,           addresultMode);
        MODECASE(MAGICTOOL_MODE_REVERTNODE,          revertnodeMode);
        MODECASE(MAGICTOOL_MODE_INPUTS,              inputsMode);
        MODECASE(MAGICTOOL_MODE_TOMASK,              tomaskMode);
        MODECASE(MAGICTOOL_MODE_ADDMASK,             addmaskMode);
        MODECASE(MAGICTOOL_MODE_REVERTMASK,          revertmaskMode);
        MODECASE(MAGICTOOL_MODE_MASK,                maskMode);
        MODECASE(MAGICTOOL_MODE_CENSORRUN,           censorrunMode);
        MODECASE(MAGICTOOL_MODE_EXPOSURE,            exposureMode);
        MODECASE(MAGICTOOL_MODE_SETGOTOCLEANED,      setgotocleanedMode);
        MODECASE(MAGICTOOL_MODE_TOCLEANUP,           tocleanupMode);
        MODECASE(MAGICTOOL_MODE_SETWORKDIRSTATE,     setworkdirstateMode);
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

    // Required
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    // Optional
    PXOPT_LOOKUP_STR(data_group, config->args, "-data_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-note", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(multidiff, config->args, "-multidiff", false);
    psMetadata *diffWhere = psMetadataAlloc(); // WHERE conditions for diffRuns
    PXOPT_COPY_STR(config->args, diffWhere, "-diff_label", "diffRun.label", "==");
    PXOPT_COPY_S64(config->args, diffWhere, "-diff_id", "diff_id", "==");

    psMetadata *queryWhere = psMetadataAlloc(); // WHERE conditions for everything else
    PXOPT_COPY_S64(config->args, queryWhere, "-exp_id", "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, queryWhere, "-select_filter", "rawExp.filter", "==");

    // Get list of exposures ready to magic
    {
        psString query;
        if (multidiff) {
	  query = pxDataGet("magictool_definebyquery_select_multidiff.sql");
        }
        else {
	  query = pxDataGet("magictool_definebyquery_select.sql");
        }
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }

        // "available" means queue magic run even though the diffRun has skycells that did not complete
        // "rerun" means we want a new run even if there's already a magic run defined for the exposure
        PXOPT_LOOKUP_BOOL(available, config->args, "-available", false);
        PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);

        psString diffWhereStr = NULL;   // WHERE conditions for diffRuns
        psString queryWhereStr = NULL;  // WHERE conditions for entire query
        if (!available) {
            psStringAppend(&diffWhereStr, "\nAND diffRun.state = 'full'");
        }
        // what if no skycells for the diff run completed?

        psStringAppend(&queryWhereStr, "\n%s magic_id IS NULL", queryWhereStr ? "AND" : "WHERE");
        psString rerunWhereStr = NULL;
        if (rerun) {
            psStringAppend(&rerunWhereStr, "\n WHERE magicRun.label = '%s'", label);
        }

        // now add the user specified qualifiers
        if (psListLength(diffWhere->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(diffWhere, NULL);
            psStringAppend(&diffWhereStr, "\nAND %s", whereClause);
            psFree(whereClause);
        }
        if (psListLength(queryWhere->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(queryWhere, NULL);
            psStringAppend(&queryWhereStr, "\n%s %s", queryWhereStr ? "AND" : "WHERE", whereClause);
            psFree(whereClause);
        }
        psFree(diffWhere);
        psFree(queryWhere);

        // Ensure the WHERE strings have something
        if (!diffWhereStr) {
            diffWhereStr = psStringCopy("");
        }
        if (!queryWhereStr) {
            queryWhereStr = psStringCopy("");
        }
        if (!rerunWhereStr) {
            rerunWhereStr = psStringCopy("");
        }

        if (!p_psDBRunQueryF(config->dbh, query, diffWhereStr, diffWhereStr, rerunWhereStr, queryWhereStr)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(diffWhereStr);
            psFree(queryWhereStr);
            psFree(rerunWhereStr);
            psFree(query);
            return false;
        }
        psFree(diffWhereStr);
        psFree(queryWhereStr);
        psFree(rerunWhereStr);
        psFree(query);
    }

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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }


    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "diffRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }


    // Parse the list of exposures ready to magic
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psString insert = pxDataGet("magictool_definebyquery_insert.sql"); // Insert query

    psArray *list = psArrayAllocEmpty(16); // List of runs, to print
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i]; // Row of interest
        psS64 exp_id = psMetadataLookupS64(NULL, row, "exp_id"); // Exposure identifier
        psS64 diff_id = psMetadataLookupS64(NULL, row, "diff_id"); // difference identifier
        bool inverse = psMetadataLookupU64(NULL, row, "inverse"); // Inverse subtraction? Note types!
        psString diff_data_group = psMetadataLookupStr(NULL, row, "diff_data_group");

        // create a new magicRun for this group
        magicRunRow *run = magicRunRowAlloc(0,
                                            exp_id,
                                            diff_id,
                                            inverse,
                                            "new",      // state
                                            workdir,
                                            "dirty",    // workdir_state
                                            label,
                                            data_group ? data_group : (diff_data_group ? diff_data_group : label),
                                            dvodb,
                                            registered,
                                            0,          // fault
                                            note);
        if (!run) {
            psAbort("failed to alloc magicRun object");
        }

        if (!magicRunInsertObject(config->dbh, run)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(run);
            psFree(insert);
            psFree(output);
            psFree(list);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psS64 magic_id = psDBLastInsertID(config->dbh); // Assigned identifier
        run->magic_id = magic_id;

        psArrayAdd(list, list->n, run);
        psFree(run);

        // Create a suitable insertion query for this run
        psString thisInsert = psStringCopy(insert);
        {
            psString idString = NULL;
            psStringAppend(&idString, "%" PRId64, magic_id);
            psStringSubstitute(&thisInsert, idString, "@MAGIC_ID@");
            psFree(idString);
        }
        {
            psString idString = NULL;
            psStringAppend(&idString, "%" PRId64, diff_id);
            psStringSubstitute(&thisInsert, idString, "@DIFF_ID@");
            psFree(idString);
        }

        if (!p_psDBRunQueryF(config->dbh, thisInsert, magic_id, exp_id)) {
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
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(output);

    if (!magicRunPrintObjects(stdout, list, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print object");
        psFree(list);
        return false;
    }

    psFree(list);

    return true;
}

static psS64 definerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false);
    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);
    PXOPT_LOOKUP_S64(diff_id, config->args, "-diff_id", false, false);

    // optional
    PXOPT_LOOKUP_BOOL(inverse, config->args, "-inverse", false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    magicRunRow *run = magicRunRowAlloc(
            0,          // ID
            exp_id,
            diff_id ? diff_id : PS_MAX_S64,
            inverse,
            "reg",      // state
            workdir,
            "dirty",    // workdir_state
            label,
            NULL,       // data_group
            dvodb,
            registered,
            0,          // fault
            NULL
    );

    if (!run) {
        psError(PS_ERR_UNKNOWN, false, "failed to alloc magicRun object");
        return false;
    }
    if (!magicRunInsertObject(config->dbh, run)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(run);
        return false;
    }

    // get the assigned magic_id
    psS64 magic_id = psDBLastInsertID(config->dbh);
    run->magic_id = magic_id;

    if (!magicRunPrintObject(stdout, run, !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print object");
            psFree(run);
            return false;
    }

    psFree(run);

    return magic_id;
}


static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", true, false);
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", true, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-set_fault", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_BOOL(clearfault, config->args, "-clearfault", false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    
    psString setString = NULL;
    if (fault || clearfault) {
        psStringAppend(&setString, ", fault = %d", fault);
    }
    if (note) {
        psStringAppend(&setString, ", note = '%s'", note);
    }
    if (label) {
        psStringAppend(&setString, ", label = '%s'", label);
    }

    if (state) {
        // set detRun.state to state
        return setmagicRunState(config, magic_id, state, setString);
    }

    return true;
}


static bool addinputskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", true, false);
    PXOPT_LOOKUP_STR(node, config->args, "-node", true, false);

    magicInputSkyfileInsert(
            config->dbh,
            magic_id,
            node
    );

    return true;
}


static bool inputskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    PXOPT_COPY_S64(config->args, where, "-diff_id", "diff_id", "==");
    PXOPT_COPY_STR(config->args, where, "-node", "node", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("magictool_inputskyfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "magicInputSkyfile");
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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "magicInputSkyfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool totreeMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("magictool_totree.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, "\nORDER BY priority DESC, magic_id");

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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "totree", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool inputtreeMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", true, false);

    // Optional values
    PXOPT_LOOKUP_STR(dep_file, config->args, "-dep_file", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (fault > 0) {
        char *query = "UPDATE magicRun SET fault = %d WHERE magic_id = %" PRId64;
        if (!p_psDBRunQueryF(config->dbh, query, fault, magic_id)) {
            psError(PS_ERR_UNKNOWN, false,
                    "failed to set fault for magic_id %" PRId64, magic_id);
            return false;
        }
        return true;
    }

    if (!parseAndInsertNodeDeps(config, magic_id, dep_file)) {
        psError(PS_ERR_UNKNOWN, false, "failed to parse file");
        return false;
    }

    return true;
}

static bool reverttreeMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicRun.label", "==");

    psString query = psStringCopy("UPDATE magicRun SET fault = 0 WHERE state = 'new' AND fault != 0");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "failed to revert");
        return false;
    }
    psS32 numUpdated = psDBAffectedRows(config->dbh);
    psLogMsg("magictool", PS_LOG_INFO, "Reverted %d magic runs", numUpdated);

    return true;
}


static bool inputsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // Regrettably, there are multiple WHERE hooks which call the same things different names
    psMetadata *templatesWhere = psMetadataAlloc(); // WHERE for selecting template
    psMetadata *magicWhere = psMetadataAlloc();     // WHERE for selecting magic runs

    PXOPT_COPY_S64(config->args, templatesWhere, "-magic_id", "magicRun.magic_id", "==");
    PXOPT_COPY_STR(config->args, templatesWhere, "-node", "diffInputSkyfile.skycell_id", "==");

    PXOPT_COPY_S64(config->args, magicWhere, "-magic_id", "magicRun.magic_id", "==");
    PXOPT_COPY_STR(config->args, magicWhere, "-node", "magicTree.node", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("magictool_inputs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString templatesWhereStr = psStringCopy(""); // WHERE for selecting template
    psString magicWhereStr = psStringCopy("");     // WHERE for selecting magic runs

    if (psListLength(templatesWhere->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(templatesWhere, NULL);
        psStringAppend(&templatesWhereStr, "\nAND %s", whereClause);
        psFree(whereClause);
    }
    psFree(templatesWhere);

    if (psListLength(magicWhere->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(magicWhere, NULL);
        psStringAppend(&magicWhereStr, "\nWHERE %s", whereClause);
        psFree(whereClause);
    }
    psFree(magicWhere);

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query,
                         templatesWhereStr, templatesWhereStr,
                         magicWhereStr, magicWhereStr)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(templatesWhereStr);
        psFree(magicWhereStr);
        psFree(query);
        return false;
    }
    psFree(templatesWhereStr);
    psFree(magicWhereStr);
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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "magicNode", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

bool findBaseNodes(void *arg, pxNode *node)
{
    bool status = false;
    psS64 done = psMetadataLookupS64(&status, node->data, "done");
    if (!status) {
        psAbort("failed to lookup value for done column");
    }

    if ((!pxNodeHasChildren(node)) && (!done)) {
        // if this node has no child and it's not 'done', then push it's data
        // onto the void *array
        psArrayAdd((psArray *)arg, 0, node->data);
        return false;
    }

    return true;
}


bool findReadyNodes(void *arg, pxNode *node)
{
    if (!node) {
        // It's not there --- must have failed
        return false;
    }

    if (!node->data) {
        // It's a leaf node, not done
        return false;
    }

    if (psMetadataLookupBool(NULL, node->data, "done")) {
        // It's already done
        return true;
    }

    if (pxNodeHasChildren(node)) {
        psListIterator *iter = psListIteratorAlloc(node->children, 0, false);
        psMetadata *work = psMetadataCopy(NULL, node->data);
        psMetadataRemoveKey(work, "dep");
        psMetadataRemoveKey(work, "done");
        pxNode *child = NULL;
        while ((child = psListGetAndIncrement(iter))) {
            psMetadata *data = child->data;
            if (!data) {
                // Child is a leaf node, not done
                psFree(iter);
                psFree(work);
                return false;
            }

            bool status = false;
            psS64 done = psMetadataLookupS64(&status, data, "done");
            if (!status) {
                psAbort("failed to lookup value for done column");
            }
            psS64 bad = psMetadataLookupS64(&status, data, "bad");
            if (!status) {
                psAbort("failed to lookup value for bad column");
            }

            if (!done || bad) {
                // if a child isn't "done", give up on this node and continue
                // to crawl the tree
                psFree(iter);
                psFree(work);
                return true;
            }
        }
        psFree(iter);
        // if all this nodes children are done, then push it's data onto the
        // void *array
        psArrayAdd((psArray *)arg, 0, work);
        psFree(work);
        return false;
    }

    return true;
}


static bool toprocessMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicRun.label", "==");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString whereString = NULL;
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereString, "\nAND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    // First look for branch nodes that need to be processed.
    // These get priority over skycells because they are from runs
    // that are already in progress and there are fewer of them.
    // When we looked for skycells first we got starved.

    // first find incomplete magicRuns
    psString query = pxDataGet("magictool_toprocess_runs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // Find outstanding magicRuns in new state.
    // We limit the query, but this is problematic. In practice do
    // we need to?
    // XXX: If the first 1000 magicRuns have no branch nodes ready
    // but higher runs do we they won't be noticed.
    // Perhaps have this limit be an argument.
    {
        psString limitString = psDBGenerateLimitSQL( 1000 );
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereString ? whereString :  "")) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereString);
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *magicRuns = p_psDBFetchResult(config->dbh);
    if (!magicRuns) {
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

    if (!psArrayLength(magicRuns)) {
        // no pending magicRuns nothing to do
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        return true;
    }
    psArray *output = psArrayAllocEmpty(100);

    {
        query = pxDataGet("magictool_toprocess_tree.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }

        for (psS64 index = 0; index < psArrayLength(magicRuns); index++) {
            if (limit && (psArrayLength(output) >= limit)) {
                break;
            }
            bool status;
            psS64 magic_id = psMetadataLookupS64(&status, magicRuns->data[index], "magic_id");
            if (!status) {
                psAbort("failed to lookup value for magic_id column");
            }

            psString whereString2 = NULL;
            psStringAppend(&whereString2, "\nAND (magic_id = %" PRId64 ")", magic_id);
            if (!p_psDBRunQueryF(config->dbh, query, whereString2 )) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(whereString2);
                psFree(query);
                return false;
            }
            psFree(whereString2);
            psArray *magicTree = p_psDBFetchResult(config->dbh);
            if (!magicTree) {
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
            psS64 length = psArrayLength(magicTree);
            if (!length) {
                psTrace("magictool", PS_LOG_INFO, "no rows found for magic_id %" PRId64, magic_id);
                psFree(magicTree);
                continue;
            }

            psHash *forest = psHashAlloc(length);

            // convert the array of metadata into a pxTree structure
            for (long i = 0; i < length; i++) {
                bool status;
                psString node = psMetadataLookupStr(&status, magicTree->data[i], "node");
                if (!status) {
                    psAbort("failed to lookup value for node column");
                }

                psString dep = psMetadataLookupStr(&status, magicTree->data[i], "dep");
                if (!status) {
                    psAbort("failed to lookup value for dep column");
                }

                pxTreeBuilder(forest, node, dep, magicTree->data[i]);

            }

            // find the root of the tree
            pxNode *root = psMemIncrRefCounter(psHashLookup(forest, "root"));
            psFree(forest);
            //    pxTreePrint(stdout, root);

            // crawl through the tree and looking for nodes with children that are all
            // "done"
            pxTreeCrawl(root, findReadyNodes, output);
            psFree(root);
            psFree(magicTree);
        }

        int len = psArrayLength(output);
        if (len) {
            if (limit) {
                if (limit < psArrayLength(output)) {
                    // truncate the array
                    long arrayLength = psArrayLength(output);
                    for (long i = arrayLength - 1; i >= limit; i--) {
                        psArrayRemoveIndex(output, i);
                    }
                    // negative simple so the default is true
                    if (!ippdbPrintMetadatas(stdout, output, "magicMe", !simple)) {
                        psError(PS_ERR_UNKNOWN, false, "failed to print array");
                        psFree(output);
                        return false;
                    }
                    psFree(output);
                    return true;
                } else {
                    limit -= len;
                }
            }
        }
    }

    // look for "inputs" (skycells) that need to processed
    query = pxDataGet("magictool_toprocess_inputs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psStringAppend(&query, "\nORDER BY priority DESC, magic_id");

    // treat limit == 0 as "no limit"
    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereString, whereString)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereString);
        psFree(query);
        return false;
    }
    psFree(whereString);
    psFree(query);

    psArray *skycellOutput = p_psDBFetchResult(config->dbh);
    if (!skycellOutput) {
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
    // merge the arrays so that we can print them all at once
    if (psArrayLength(skycellOutput)) {
        int len = psArrayLength(skycellOutput);
        for (int i=0; i < len; i++) {
            psArrayAdd(output, 0, skycellOutput->data[i]);
        }
    }
    psFree(skycellOutput);
    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "magicMe", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    } else {
        psTrace("magictool", PS_LOG_INFO, "no rows found");
    }
    psFree(output);
    return true;
}


static bool addresultMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", true, false);
    PXOPT_LOOKUP_STR(node, config->args, "-node", true, false);

    // optional
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!magicNodeResultInsert(config->dbh,
                               magic_id,
                               node,
                               path_base,
                               fault
        )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (fault == 0 && !strcmp(node, "root")) {
        // Set the magicRun state
        psString query = pxDataGet("magictool_setfull.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        // manually add constraint
        psStringAppend(&query, " AND magic_id = %" PRId64, magic_id);

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(query);
    }
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool revertnodeMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    PXOPT_COPY_STR(config->args, where, "-node", "node", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "magicNodeResult.fault", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicRun.label", "==");

    if (!psListLength(where->list)) {
        psError(PS_ERR_UNKNOWN, false, "search parameters are required");
        psFree(where);
        return false;
    }
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psFree(where);

    // don't need a transaction because it is ok if the first query succeeds
    // but the second query does not
    psString query = pxDataGet("magictool_deletemask.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    psStringAppend(&query, " AND %s", whereClause);
    if (!p_psDBRunQuery(config->dbh, query)) {
        psFree(whereClause);
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "failed to delete faulted masks");
        return false;
    }
    psFree(query);

    query = pxDataGet("magictool_revertnode.sql");
    if (!query) {
        psFree(whereClause);
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "failed to revert");
        return false;
    }
    psFree(query);

    psS32 numUpdated = psDBAffectedRows(config->dbh);
    psLogMsg("magictool", PS_LOG_INFO, "Reverted %d magic nodes", numUpdated);

    return true;
}


static bool tomaskMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // look for "inputs" that need to processed
    psString query = pxDataGet("magictool_tomask.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "tomask", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool addmaskMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", true, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

    // optional
    PXOPT_LOOKUP_S32(streaks, config->args, "-streaks", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!magicMaskInsert(config->dbh,
                         magic_id,
                         NULL,
                         path_base,
                         streaks,
                         fault
        )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }

    return true;
}

static bool revertmaskMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "fault", "==");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // Set to "new"
    {
        psString query = psStringCopy("UPDATE magicRun JOIN magicMask USING(magic_id) "
                                      "SET magicRun.state = 'new' WHERE magicMask.fault != 0");

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, "magicMask");
            psStringAppend(&query, " AND %s", whereClause);
            psFree(whereClause);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "failed to revert");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psFree(query);
    }

    // Delete failed attempt at mask
    {
        psString query = psStringCopy("DELETE FROM magicMask WHERE fault != 0");

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, "magicMask");
            psStringAppend(&query, " AND %s", whereClause);
            psFree(whereClause);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "failed to revert");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }

        psFree(query);
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(where);

    return true;
}


static bool maskMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("magictool_mask.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "magicMask", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool setmagicRunState(pxConfig *config, psS64 magic_id, const char *state, psString setString)
{
    PS_ASSERT_PTR_NON_NULL(state, false);

    // check that state is a valid string value
    if (!(
            (strncmp(state, "new", 3) == 0)
            || (strncmp(state, "full", 4) == 0)
            || (strncmp(state, "drop", 4) == 0)
            || (strncmp(state, "reg", 3) == 0)
        )
    ) {
        psError(PS_ERR_UNKNOWN, false,
                "invalid magicRun state: %s", state);
        return false;
    }
    psString query = NULL;
    psStringAppend(&query, "UPDATE magicRun SET state = '%s'", state);
    if (setString) {
        psStringAppend(&query, "%s", setString);
    }
    psStringAppend(&query, " WHERE magic_id = %" PRId64, magic_id);;

//    char *query = "UPDATE magicRun SET state = '%s' WHERE magic_id = %" PRId64;
    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for magic_id %" PRId64, magic_id);
        return false;
    }

    return true;
}

static bool parseAndInsertNodeDeps(pxConfig *config, psS64 magic_id, const char *filename)
{
    unsigned int nFail = 0;
    psMetadata *deps = psMetadataConfigRead(NULL, &nFail, filename, false);
    if (!deps) {
        psError(PS_ERR_UNKNOWN, false, "failed to parse file: %s", filename);
        return false;
    }
    if (nFail) {
        psError(PS_ERR_UNKNOWN, false, "there were %d errors parsing file: %s", nFail, filename);
        psFree(deps);
        return false;
    }

    psMetadataItem *item = NULL;
    psMetadataIterator *iter = psMetadataIteratorAlloc(deps, 0, NULL);
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (item->type != PS_DATA_STRING) {
            psError(PS_ERR_UNKNOWN, false, "file: %s is in the wrong format", filename);
            psFree(iter);
            psFree(deps);
            return false;
        }

        char *name = item->name;
        char *dependsOn = item->data.str;

        if (!magicTreeInsert(
                config->dbh,
                magic_id,
                name,
                dependsOn
            )) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(iter);
            psFree(deps);
            return false;
        }
    }
    psFree(iter);
    psFree(deps);

    return true;
}

static bool censorrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", false, false);
    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);

    if (!magic_id) {
        if (!exp_id && !label) {
            psError(PS_ERR_UNKNOWN, true, "either -magic_id or exp_id and label is required");
            return false;
        }
    }

    // at least one of these required
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magic_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    if (!psListLength(where->list)) {
        psError(PS_ERR_UNKNOWN, true, "either -exp_id or -magic_id is required");
        psFree(where);
        return false;
    }

    psString query = psStringCopy("UPDATE magicRun SET state = 'censored'");

    psString whereClause = psDBGenerateWhereConditionSQL(where, "magicRun");
    psFree(where);
    psStringAppend(&query, " WHERE %s", whereClause);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(whereClause);
        psFree(query);
        return false;
    }
    psFree(query);

    psS32 numUpdated = psDBAffectedRows(config->dbh);
    if (numUpdated == 0) {
        psError(PS_ERR_UNKNOWN, false, "failed to censor magicRun");
        psFree(whereClause);
        return false;
    }

    // Now queue any destreaked files to be re-verted

    // note: on failure pxmagicRestoreStage issues the rollback
    if (!pxmagicRestoreStage(config, "raw", whereClause, "goto_censored")) {
        psFree(whereClause);
        return false;
    }
    if (!pxmagicRestoreStage(config, "chip", whereClause, "goto_censored")) {
        psFree(whereClause);
        return false;
    }
    if (!pxmagicRestoreStage(config, "camera", whereClause, "goto_censored")) {
        psFree(whereClause);
        return false;
    }
    if (!pxmagicRestoreStage(config, "warp", whereClause, "goto_censored")) {
        psFree(whereClause);
        return false;
    }
    if (!pxmagicRestoreStage(config, "diff", whereClause, "goto_censored")) {
        psFree(whereClause);
        return false;
    }

    psFree(whereClause);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool exposureMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");
    PXOPT_COPY_BOOL(config->args, where, "-inverse", "magicRun.inverse", "==");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("magictool_exposure.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = psStringCopy(""); // WHERE restrictions
    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereClause, "\nWHERE %s", clause);
        psFree(clause);
    }
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, whereClause, whereClause)) {
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
                break;
            case PS_ERR_DB_SERVER:
                psError(PXTOOLS_ERR_PROG, false, "database error");
                break;
            default:
                psError(PXTOOLS_ERR_PROG, false, "unknown error");
                break;
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "magicMask", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}
static bool setgotocleanedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "magicRun.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "magicRun.label", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-data_group", "magicRun.data_group", "LIKE");

    PXOPT_LOOKUP_STR(set_label, config->args, "-set_label", false, false);

    psString query = psStringCopy("UPDATE magicRun SET workdir_state = 'goto_cleaned'\n");
    if (set_label) {
        psStringAppend(&query, ", label = '%s'", set_label);
    }
    // This mode doubles as a revert function for cleanup errors
    PXOPT_LOOKUP_BOOL(clearfault, config->args, "-clearfault", false);
    if (!clearfault) {
        psStringAppend(&query, "WHERE workdir_state = 'dirty'");
    } else {
        psStringAppend(&query, "WHERE workdir_state = 'error_cleaned'");
    }
    psStringAppend(&query, "\nAND (magicRun.state = 'full' OR magicRun.state = 'drop')");

    // Require search parameters unless we're just clearing faults
    if (psListLength(where->list)) {
        psString clause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", clause);
        psFree(clause);
        psFree(where);
    } else if ( !clearfault) {
        psError(PS_ERR_UNKNOWN, false, "search parameters are required");
        psFree(where);
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool setworkdirstateMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(magic_id, config->args, "-magic_id", true, false);
    PXOPT_LOOKUP_STR(workdir_state, config->args, "-set_workdir_state", true, false);

    if (strcmp(workdir_state, "cleaned") && strcmp(workdir_state, "error_cleaned")) {
        psError(PS_ERR_UNKNOWN, true, "%s is not a valid value for workdir_state", workdir_state);
        return false;
    }
    
    psString query = NULL;
    psStringAppend(&query, "UPDATE magicRun SET workdir_state = '%s' WHERE magic_id = %" PRId64, workdir_state, magic_id);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}
static bool tocleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-magic_id", "magicRun.magic_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "magicRun.label", "==");
    pxAddLabelSearchArgs (config, where, "-data_group", "magicRun.data_group", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("magictool_tocleanup.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    psStringAppend(&query, "\nORDER BY priority DESC, magic_id");

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
        psTrace("magictool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "tocleanup", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

/*
 * vptool.c
 *
 * Copyright (C) 2011 IfA University of Hawaii
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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "pxtools.h"
#include "pxdata.h"
#include "pxchip.h"

#include "vptool.h"

static bool definebyqueryMode(pxConfig *config);
static bool pendingrunMode(pxConfig *config);
static bool updaterunMode(pxConfig *config);
static bool revertrunMode(pxConfig *config);
static bool listrunMode(pxConfig *config);
static bool pendingimfileMode(pxConfig *config);
static bool addprocessedcellMode(pxConfig *config);
static bool processedcellMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv) {
    psLibInit(NULL);

    pxConfig *config = vptoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(VPTOOL_MODE_DEFINEBYQUERY,         definebyqueryMode);
        MODECASE(VPTOOL_MODE_PENDINGRUN,            pendingrunMode);
        MODECASE(VPTOOL_MODE_UPDATERUN,             updaterunMode);
        MODECASE(VPTOOL_MODE_REVERTRUN,             revertrunMode);
        MODECASE(VPTOOL_MODE_LISTRUN,               listrunMode);
        MODECASE(VPTOOL_MODE_PENDINGIMFILE,         pendingimfileMode);
        MODECASE(VPTOOL_MODE_ADDPROCESSEDCELL,      addprocessedcellMode);
        MODECASE(VPTOOL_MODE_PROCESSEDCELL,         processedcellMode);
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

// Queue exposures for videophot processing
static bool queue_exposures(pxConfig *config,  // Configuration
                            const psArray *exps, // Exposures information
                            const char *label,   // Label, or NULL to inherit
                            const char *data_group, // Data group, or NULL to inherit
                            const char *workdir, // Working directory, or NULL to inherit
                            const char *note,       // Note
                            const psS64 dest_id
    )
{
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }


# define GET_VALUE(PTYPE,CTYPE,VALUE,NAME)                              \
    PTYPE VALUE;                                                        \
    {                                                                   \
        bool status;                                                    \
        VALUE = psMetadataLookup##CTYPE(&status, md, NAME);             \
        if (!status) {                                                  \
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for %s", NAME); \
            return false;                                               \
        }                                                               \
    }

    // loop over our list of exp_ids
    for (long i = 0; i < psArrayLength(exps); i++) {
        psMetadata *md = exps->data[i];

        rawExpRow *row = rawExpObjectFromMetadata(md);
        if (!row) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert metadata into chipRun");
            return false;
        }

        GET_VALUE (psS64,    S64, exp_id,        "exp_id");
        GET_VALUE (psString, Str, raw_workdir,   "workdir");
        GET_VALUE (psString, Str, raw_label,     "label");
        // GET_VALUE (psString, Str, raw_reduction, "reduction");
        // GET_VALUE (psString, Str, raw_expgroup,  "expgroup");
        // GET_VALUE (psString, Str, raw_dvodb,     "dvodb");
        // GET_VALUE (psString, Str, raw_tess_id,   "tess_id");
        // GET_VALUE (psString, Str, raw_end_stage, "end_stage");

        if (!row->exp_id) {
            psError(PS_ERR_UNKNOWN, false, "failed to find value for exp_id");
            return false;
        }

        // queue the exp
        if (!vpRunInsert(config->dbh,
                                 0,  //vp_id
                                 exp_id,
                                 "new",
                                 label       ? label     : raw_label,
                                 data_group  ? data_group : (label ? label : raw_label),
                                 workdir     ? workdir   : raw_workdir,
                                 note,
                                 dest_id,
                                 NULL,      // outroot
                                 0,         // dtime_script
                                 NULL,      // hostname
                                 0          // fault
                                 )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to queue exp_id: %" PRId64, exp_id);
            return false;
        }
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", false, false);

    PXOPT_LOOKUP_STR(label, config->args, "-set_label", true, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_S64(dest_id, config->args, "-set_dest_id", false, false);

    // default
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    pxchipGetSearchArgs (config, where); // rawExp only
    pxAddLabelSearchArgs (config, where, "-label", "newExp.label", "="); // define using newExp label

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("vptool_find_rawexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);

    psFree(whereClause);
    psFree(where);

    if (!rerun) {
      psStringAppend(&query, "\n  AND vp_id IS NULL\n  AND (vpRun.label = '%s' OR vpRun.label IS NULL)", label);
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "rawExp", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    if (!queue_exposures(config, output, label, data_group, workdir, note, dest_id)) {
        psError(psErrorCodeLast(), false, "Unable to queue exposures for videophot processing.");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool pendingrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    // Do we need pxchipArgs?
    pxchipGetSearchArgs (config, where); //chipRun, rawExp
    PXOPT_COPY_S64(config->args, where, "-vp_id", "vpRun.vp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "vpRun.label", "==");

    psString query = pxDataGet("vptool_pendingrun.sql");
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

    psStringAppend(&query, "\nORDER BY priority DESC, vp_id");

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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "vpPendingRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

#ifdef notdef
static bool definecopyMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", false, false);

    psMetadata *where = psMetadataAlloc();
    pxchipGetSearchArgs (config, where); // rawExp only
    pxAddLabelSearchArgs (config, where, "-label", "chipOld.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "chipOld.data_group", "LIKE");

    // psListLength(where->list) is at least 1 because exp_type defaults to "object"
    // so we require a list longer than 1 entry
    if ((psListLength(where->list) <= 1) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", true, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", true, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(expgroup, config->args, "-set_expgroup", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-set_tess_id", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-set_end_stage", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);

    // default
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("vptool_definecopy.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);

    psFree(whereClause);
    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, label)) {
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "rawExp", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    if (!queue_exposures(config, output, workdir, label, data_group, dist_group,
                         note)) {
        psError(psErrorCodeLast(), false, "Unable to queue exposures for chip.");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}
#endif


static bool updaterunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
//     pxchipGetSearchArgs (config, where); // rawExp, vpRun
    PXOPT_COPY_S64(config->args,  where, "-vp_id", "vpRun.vp_id", "==");
    PXOPT_COPY_STR(config->args,  where, "-label",   "vpRun.label",   "LIKE");
    PXOPT_COPY_STR(config->args,  where, "-state",   "vpRun.state",   "==");
    PXOPT_COPY_STR(config->args,  where, "-data_group", "vpRun.data_group",   "LIKE");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-set_hostname", false, false);
    PXOPT_LOOKUP_STR(outroot, config->args, "-set_outroot", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-set_fault", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-set_dtime_script", false, false);

    if (!state && !label && !data_group && !note && !hostname && !fault) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, true, "must supply at least one set parameter");
        return false;
    }

    psString query = psStringCopy("UPDATE vpRun");

    char *separator = " SET ";
    char * comma = ",";

#   define addColumn(_tab, _fmt, _val)                                        \
        do {                                                                \
            if (_val) {                                                     \
                psStringAppend(&query, "%s %s.%s =" _fmt, separator, _tab, #_val, _val); \
                separator = comma;                                          \
            }                                                               \
        } while (0)

    addColumn("vpRun", "'\%s'", state);
    addColumn("vpRun", "'\%s'", label);
    addColumn("vpRun", "'\%s'", data_group);
    addColumn("vpRun", "'\%s'", note);
    addColumn("vpRun", "'\%s'", hostname);
    addColumn("vpRun", "'\%s'", outroot);
    addColumn("vpRun", "%d", fault);
    addColumn("vpRun", "%f", dtime_script);

    psString  whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psFree(where);
    psStringAppend(&query, "\nWHERE %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psFree(query);

    return true;
}


static bool pendingimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    // Do we need pxchipArgs?
    pxchipGetSearchArgs (config, where); //chipRun, rawExp
    PXOPT_COPY_S64(config->args, where, "-vp_id", "vpRun.vp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "vpRun.label", "==");

    psString query = pxDataGet("vptool_pendingimfile.sql");
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "vpPendingImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool addprocessedcellMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // vp_id, class_id, and cell_id are required
    PXOPT_LOOKUP_S64(vp_id, config->args, "-vp_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
    PXOPT_LOOKUP_STR(cell_id, config->args, "-cell_id", true, false);

    // optional
    PXOPT_LOOKUP_F32(dtime_photom,  config->args,  "-dtime_photom",  false, false);
    PXOPT_LOOKUP_STR(path_base, config->args,      "-path_base", false, false);

    // default values
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!vpProcessedCellInsert(config->dbh,
                                   vp_id,
                                   class_id,
                                   cell_id,
                                   dtime_photom,
                                   quality,
                                   path_base,
                                   fault
            )) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool processedcellMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    pxchipGetSearchArgs (config, where); // chipRun, chipProcessedImfile, rawExp
    PXOPT_COPY_S64(config->args, where, "-vp_id", "vp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "class_id", "==");
    PXOPT_COPY_STR(config->args, where, "-cell_id", "cell_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "vpRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "vpRun.data_group", "LIKE");

    psString query = pxDataGet("vptool_processedcell.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "vpProcessedCell", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool revertrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-vp_id", "vpRun.vp_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "vpRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "vpProcessedCell.fault", "==");

    // PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    // find vpRuns with faulted cells
    psString query = pxDataGet("vptool_revertrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }


    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }

    psFree(where);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);
    int numUpdated = psDBAffectedRows(config->dbh);
    psLogMsg("vptool", PS_LOG_INFO, "Reverted %d vpRuns", numUpdated);

    return true;
}

#ifdef NOTDEF
static bool revertcleanupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chipRun.chip_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "chipRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "chipRun.data_group", "LIKE");
    PXOPT_LOOKUP_STR(state, config->args, "-state", false, false);

    char* newState = NULL;
    if (!state) {
        state = "error_cleaned";
    }
    if (!strcmp(state, "error_cleaned")) {
        newState = "goto_cleaned";
    } else if (!strcmp(state, "error_purged")) {
        newState = "goto_purged";
    } else if (!strcmp(state, "error_scrubbed")) {
        newState = "goto_scrubbed";
    } else {
        psError(PXTOOLS_ERR_CONFIG, true, "-state must be either error_cleaned, error_purged, or error_scrubbed");
        return false;
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("vptool_revertcleanup.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }

    psFree(where);

    if (!p_psDBRunQueryF(config->dbh, query, newState, state, state)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool updateprocessedcellMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "class_id", "==");
    PXOPT_LOOKUP_STR(state, config->args, "-set_state", false, NULL);
    if (psListLength(where->list) == 0) {
        psError(PS_ERR_UNKNOWN, true, "search parameters are required");
        return false;
    }

    if (!state) {
      PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);
      PXOPT_LOOKUP_S16(quality, config->args, "-set_quality", false, false);

      if (!pxSetFaultCode(config->dbh, "chipProcessedImfile", where, fault, quality)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
        return false;
      }
      psFree(where);
    }
    else {
      if (!pxchipProcessedImfileSetStateByQuery(config,where,state)) {
        psError(PS_ERR_UNKNOWN, false, "failed to set chipProcessedImfile state");
        return(false);
      }
    }


    return true;
}


static bool blockMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(label, config->args, "-label", true, false);

    if (!chipMaskInsert(config->dbh, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool maskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    if (where->list->n < 1) {
        psFree(where);
        where = NULL;
    }

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = psStringCopy("SELECT * FROM chipMask");

    if (where) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psFree(where);
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipMask", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

// return the list of labels which are NOT blocked
static bool unmaskedMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-label", "label", "==");

    if (where->list->n < 1) {
        psFree(where);
        where = NULL;
    }

    psString query = pxDataGet("vptool_unmasked.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where) {
        psString whereClause = psDBGenerateWhereSQL(where, "chipUnmask");
        psFree(where);
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipUnmask", !simple)) {
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

    char *query = "DELETE FROM chipMask WHERE label = '%s'";

    if (!p_psDBRunQueryF(config->dbh, query, label)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool pendingcleanuprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    psString query = pxDataGet("vptool_pendingcleanuprun.sql");
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipPendingCleanupRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool pendingcleanupimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", false, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    if (chip_id) {
        PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");
    }
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    psString query = pxDataGet("vptool_pendingcleanupimfile.sql");
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipPendingCleanupImfile", !simple)) {
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

    psString query = pxDataGet("vptool_donecleanup.sql");
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool runMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_STR(state, config->args, "-state", true, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // make sure that the state string is valid
    if (!pxIsValidState(state)) {
        psError(PXTOOLS_ERR_CONFIG, false, "%s is not a valid state", state);
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    pxchipGetSearchArgs (config, where); // chipRun, chipProcessedImfile, rawExp
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chipRun.chip_id", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "chipRun.reduction", "==");
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "state", "==");

    psString query = pxDataGet("vptool_run.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipDoneCleanup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool advanceexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs (config, where, "-label", "label", "==");

    // look for completed chipPendingExp
    // migrate them to chipProccessedExp & camPendingExp
    psString query = pxDataGet("vptool_completely_processed_exp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, NULL);
        psStringAppend(&query, " %s", whereClause);
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];

	psS64 chip_id = psMetadataLookupS64(NULL,row,"chip_id");
	
	psString software_ver = NULL;
	psS64 maskfrac_npix = 0;
	psF32 maskfrac_static = 0;
	psF32 maskfrac_dynamic = 0;
	psF32 maskfrac_magic = 0;
	psF32 maskfrac_advisory = 0;

	// Calculate run level masking and software state
	if (!pxCoalesceRunStatus(config,"vptool_coalesce_run.sql",chip_id,
				 &software_ver,&maskfrac_npix,
				 &maskfrac_static,&maskfrac_dynamic,
				 &maskfrac_magic,&maskfrac_advisory)) {
	  psError(PS_ERR_UNKNOWN, false, "failed to generate run level statistics");
	  psFree(output);
	  if (!psDBRollback(config->dbh)) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	  }
	  return(false);
	}
	psMetadataAddStr(row, PS_LIST_TAIL, "software_ver",  0, "Software version", software_ver);
	psMetadataAddS32(row, PS_LIST_TAIL, "maskfrac_npix",  0, "Number of pixels used for mask stats", (psS32) maskfrac_npix);
	psMetadataAddF32(row, PS_LIST_TAIL, "maskfrac_static",  0, "Fraction of static masked pixels", maskfrac_static);
	psMetadataAddF32(row, PS_LIST_TAIL, "maskfrac_dynamic",  0, "Fraction of dynamic masked pixels", maskfrac_dynamic);
	psMetadataAddF32(row, PS_LIST_TAIL, "maskfrac_magic",  0, "Fraction of magic masked pixels", maskfrac_magic);
	psMetadataAddF32(row, PS_LIST_TAIL, "maskfrac_advisory",  0, "Fraction of advisory pixels", maskfrac_advisory);
/* 	psWarning("ADVANCE %ld %s %d %f %f %f %f\n",chip_id,software_ver,maskfrac_npix,maskfrac_static,maskfrac_dynamic,maskfrac_magic,maskfrac_advisory); */
        chipRunRow *chipRun = chipRunObjectFromMetadata(row);
        if (!psDBTransaction(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

	// Set chipRun.software_ver to the appropriate value
	if (software_ver) {
	  if (!pxSetRunSoftware(config, "chipRun", "chip_id", chip_id, software_ver)) {
	    psError(PS_ERR_UNKNOWN, false, "failed to set chipRun.software_ver for chip_id: %" PRId64,
		    chip_id);
	    psFree(output);
	    if (!psDBRollback(config->dbh)) {
	      psError(PS_ERR_UNKNOWN, false, "database error");
	    }
	    return(false);
	  }
	}
	// Set chipRun.maskfrac* to the appropriate values.
	if (maskfrac_npix) {
	  if (!pxSetRunMaskfrac(config, "chipRun", "chip_id",chip_id, maskfrac_npix, maskfrac_static,
				maskfrac_dynamic, maskfrac_magic, maskfrac_advisory)) {
	    psError(PS_ERR_UNKNOWN, false, "failed to set chipRun.software_ver for chip_id: %" PRId64,
		    chip_id);
	    psFree(output);
	    if (!psDBRollback(config->dbh)) {
	      psError(PS_ERR_UNKNOWN, false, "database error");
	    }
	    return(false);
	  }
	}
        // set chipRun.state to 'stop' and update the magicked state
        if (!pxchipRunSetState(config, chipRun->chip_id, "full", chipRun->magicked)) {
            psError(PS_ERR_UNKNOWN, false, "failed to change chipRun.state for chip_id: %" PRId64, chipRun->chip_id);
            psFree(chipRun);
            psFree(output);
            return false;
        }

        // should we stop here or proceed on to the cam stage?
        // NULL for end_stage means go as far as possible
        if (chipRun->end_stage && psStrcasestr(chipRun->end_stage, "chip")) {
            if (!psDBCommit(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                return false;
            }

            psFree(chipRun);
            continue;
        }
        // else continue on...

        // camQueueChipID() can only be run after chipRun.state has been set to
        // stop
        if (!pxcamQueueByChipID(config,
				chipRun->chip_id,
				chipRun->workdir,
				chipRun->label,
				chipRun->data_group,
				chipRun->dist_group,
				chipRun->reduction,
				chipRun->expgroup,
				chipRun->dvodb,
				chipRun->tess_id,
				chipRun->end_stage,
				chipRun->magicked,
				NULL    // note does not propagate
        )) {
           if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to queue camPendingExp");
            psFree(chipRun);
            psFree(output);
            return false;
        }
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        psFree(chipRun);
    }

    psFree(output);

    return true;
}

// update chipProcessedImfile.data_state to given value.
// afterwards, if all imfiles in the exposure have the new state, update the state for the exposure as well
// shared code for the modes -tocleanedimfile -tofullimfile -topurgedimfile

static bool change_imfile_data_state(pxConfig *config, psString data_state, psString run_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // chip_id, class_id are required
    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    psString query = pxDataGet("vptool_change_imfile_data_state.sql");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psString set_magicked_imfile = psStringCopy("");
    psString set_magicked_run = psStringCopy("");
    if (!strcmp(data_state, "full")) {
        // if (chipProcessedImfile.magicked < 0 and rawImfile.magicked = 0) leave magicked unchanged. This will
        // block warp processing until destreaking has been done
        // otherwise copy magicked from the rawImfile
        // Same thing for chipRun/rawExp
        psStringAppend(&set_magicked_imfile, "\n , chipProcessedImfile.magicked = IF((chipProcessedImfile.magicked < 0"
                                      " AND rawImfile.magicked = 0), chipProcessedImfile.magicked, rawImfile.magicked)");
        psStringAppend(&set_magicked_run, "\n , chipRun.magicked = IF((chipRun.magicked < 0 AND rawExp.magicked = 0), "
                                      " chipRun.magicked, rawExp.magicked)");

    } else if (!strcmp(data_state, "cleaned") || !strcmp(data_state, "purged")) {
        // if magicked is non-zero set it to -1
        // Once one imfile has been cleaned, the chipRun is no longer 'magicked'
        psStringAppend(&set_magicked_imfile, "\n, chipProcessedImfile.magicked = IF(chipProcessedImfile.magicked = 0, 0, -1),"
                                             " chipRun.magicked = IF(chipRun.magicked = 0, 0, -1)");
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state, set_magicked_imfile, chip_id, class_id)) {
        psFree(query);
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);
    psFree(set_magicked_imfile);
    psFree(set_magicked_run);
    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected atleast 1 row");
        return false;
    }

    query = pxDataGet("vptool_change_exp_state.sql");
    if (!p_psDBRunQueryF(config->dbh, query, data_state, set_magicked_run, chip_id, data_state)) {
        psFree(query);
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    psFree(query);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}
static bool tocleanedimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "cleaned", "goto_cleaned");
}
static bool tofullimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "full", "update");
}
static bool topurgedimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "purged", "goto_purged");
}
static bool toscrubbedimfileMode(pxConfig *config)
{
  return change_imfile_data_state(config, "scrubbed", "goto_scrubbed");
}
bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 3;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_S64(dummy, config->args, "-chip_id", true,  false);
  PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);
  PXOPT_LOOKUP_BOOL(clean, config->args, "-clean", false);


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
  PXOPT_COPY_S64(config->args, where, "-chip_id", "chip_id", "==");

  ExportTable tables [] = {
    {"chipRun", "vptool_export_run.sql"},
    {"chipImfile", "vptool_export_imfile.sql"},
    {"chipProcessedImfile", "vptool_export_processed_imfile.sql"},
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
      psTrace("vptool", PS_LOG_INFO, "no rows found");
      psFree(output);
      return false;
    }

    if (clean) {
        bool success = true;
        if (!strcmp(tables[i].tableName, "chipRun")) {
            success = pxSetStateCleaned("chipRun", "state", output);
        } else if (!strcmp(tables[i].tableName, "chipProcessedImfile")) {
            success = pxSetStateCleaned("chipProcessedImfile", "data_state", output);
        }
        if (!success) {
            psFree(output);
            psError(PS_ERR_UNKNOWN, false, "pxSetStateClean failed for table %s",  tables[i].tableName);
            return false;
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
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_STR(infile, config->args, "-infile", true, false);
    unsigned int nFail = 0;               // Number of failed lines
    psMetadata *input = psMetadataConfigRead(NULL, &nFail, infile, false);
    if (nFail) {
        psError(PS_ERR_IO, false, "%d failed lines in input", nFail);
        psFree(input);
        return false;
    }

    psVector *identifiers = psVectorAllocEmpty(16, PS_TYPE_U64); // Identifiers inserted

    psMetadataIterator *iter = psMetadataIteratorAlloc(input, PS_LIST_HEAD, NULL);       // Iterator

    if (!pxCheckImportVersion(config, input)) {
        psError(PS_ERR_UNKNOWN, false, "pxCheckImportVersion failed");
        return false;
    }
    // first item is the dbversion, skip it
    psMetadataItem *dbversion =  psMetadataGetAndIncrement(iter);
    (void) dbversion;

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        PXMIRROR_PRIMARY(item, "chipRun", chipRunRow, chipRunObjectFromMetadata, identifiers, chip_id,
                         chipRunInsertObject, config->dbh,
                         { psFree(iter); psFree(identifiers); psFree(input); });

        PXMIRROR_OTHER(item, "chipImfile", chipImfileRow, chipImfileObjectFromMetadata, identifiers, chip_id,
                       chipImfileInsertObject, config->dbh,
                       { psFree(iter); psFree(identifiers); psFree(input); });

        PXMIRROR_OTHER(item, "chipProcessedImfile", chipProcessedImfileRow,
                       chipProcessedImfileObjectFromMetadata, identifiers, chip_id,
                       chipProcessedImfileInsertObject, config->dbh,
                       { psFree(iter); psFree(identifiers); psFree(input); });
    }
    psFree(iter);
    psFree(input);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psLogMsg("vptool", PS_LOG_INFO, "%ld chipRuns added", identifiers->n);
    psFree(identifiers);

    return true;
}

static bool runstateMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_id",    "chipRun.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");
    pxAddLabelSearchArgs (config, where, "-label",     "chipRun.label", "LIKE");

//    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_BOOL(no_magic, config->args, "-no_magic", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("vptool_runstate.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "chipRunState", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

// a very specfic function to queue a cleaned chipProcessedImfile to be updated
static bool setimfiletoupdateMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_S64(chip_id, config->args, "-chip_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);

    psString query = pxDataGet("vptool_setimfiletoupdate.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString setHook = psStringCopy("");
    if (label) {
        psStringAppend(&setHook, "\n , chipRun.label = '%s'", label);
    }

    if (class_id) {
        psStringAppend(&query, " AND (chipProcessedImfile.class_id = '%s')", class_id);
    }

    // we do not update components with the magic fault value. They are non-updateable
    psStringAppend(&query, " AND (chipProcessedImfile.fault != %d)", PXTOOL_DO_NOT_REVERT_FAULT);

    if (!p_psDBRunQueryF(config->dbh, query, setHook, chip_id)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psFree(setHook);
    psFree(query);

    return true;
}
#endif // NOTDEF
static bool listrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pstamp_order, config->args, "-pstamp_order", false);

    psMetadata *where = psMetadataAlloc();
    pxchipGetSearchArgs (config, where); // chipRun, chipProcessedImfile, rawExp
    PXOPT_COPY_S64(config->args, where, "-chip_id", "chipRun.chip_id", "==");
    PXOPT_COPY_STR(config->args, where, "-reduction", "chipRun.reduction", "==");
    PXOPT_COPY_STR(config->args, where, "-state", "chipRun.state", "==");
    pxAddLabelSearchArgs (config, where, "-label", "chipRun.label", "LIKE");
    pxAddLabelSearchArgs (config, where, "-data_group", "chipRun.data_group", "LIKE");
    pxAddLabelSearchArgs (config, where, "-dist_group", "chipRun.dist_group", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-magicked", "chipRun.magicked", "==");

    psString where2 = NULL;
    pxmagicAddWhere(config, &where2, "chipRun");
    // add cuts on ra and decl if supplied
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }

    psString query = pxDataGet("vptool_listrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s %s", whereClause, where2 ? where2 : "");
        psFree(whereClause);
    } else if (psMetadataLookupBool(NULL, config->args, "-all") || where2) {
        psStringAppend(&query, " WHERE chipRun.chip_id IS NOT NULL %s", where2 ? where2 : "");
    } else {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters (or -all) are required");
        return false;
    }
    psFree(where);

    if (pstamp_order) {
        // put runs in order of exposure id with newest chip Runs first
        // The postage stamp parser depends on this behavior
        psStringAppend(&query, "\nORDER by exp_id, chip_id DESC");
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
        psTrace("vptool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

/*
 * bgtool.c
 *
 * Copyright (C) 2006-2010  Joshua Hoblitt, Paul Price
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
#include "bgtool.h"

static bool definechipMode(pxConfig *config);
static bool updatechipMode(pxConfig *config);
static bool tochipMode(pxConfig *config);
static bool chipinputsMode(pxConfig *config);
static bool addchipMode(pxConfig *config);
static bool chipMode(pxConfig *config);
static bool advancechipMode(pxConfig *config);
static bool revertchipMode(pxConfig *config);
static bool listchipMode(pxConfig *config);
static bool definewarpMode(pxConfig *config);
static bool updatewarpMode(pxConfig *config);
static bool towarpMode(pxConfig *config);
static bool warpinputsMode(pxConfig *config);
static bool addwarpMode(pxConfig *config);
static bool warpMode(pxConfig *config);
static bool advancewarpMode(pxConfig *config);
static bool revertwarpMode(pxConfig *config);
static bool listwarpMode(pxConfig *config);

static bool pendingcleanupchiprunMode(pxConfig *config);
static bool pendingcleanupchipimfileMode(pxConfig *config);
static bool tocleanedchipimfileMode(pxConfig *config);
static bool updatechipimfileMode(pxConfig *config);

static bool pendingcleanupwarprunMode(pxConfig *config);
static bool pendingcleanupwarpskyfileMode(pxConfig *config);
static bool tocleanedwarpskyfileMode(pxConfig *config);

static bool exportchipMode(pxConfig *config);
static bool importchipMode(pxConfig *config);
static bool exportwarpMode(pxConfig *config);
static bool importwarpMode(pxConfig *config);

static bool validDataState(psString data_state);

// Tables to import/export
typedef struct {
    const char *name;                   // Table name
    void* (*parse)();                   // Parsing function
    bool (*insert)();                   // Insertion function
} tableData;
static const tableData chipTables[] = {
    { "chipBackgroundRun", (void*)&chipBackgroundRunObjectFromMetadata, &chipBackgroundRunInsertObject },
    { "chipBackgroundImfile", (void*)&chipBackgroundImfileObjectFromMetadata, &chipBackgroundImfileInsertObject },
    { NULL, NULL, NULL }
};
static const tableData warpTables[] = {
    { "warpBackgroundRun", (void*)&warpBackgroundRunObjectFromMetadata, &warpBackgroundRunInsertObject },
    { "warpBackgroundSkyfile", (void*)&warpBackgroundSkyfileObjectFromMetadata, &warpBackgroundSkyfileInsertObject },
    { NULL, NULL, NULL }
};


# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = bgtoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(BGTOOL_MODE_DEFINECHIP,  definechipMode);
        MODECASE(BGTOOL_MODE_UPDATECHIP,  updatechipMode);
        MODECASE(BGTOOL_MODE_TOCHIP,      tochipMode);
        MODECASE(BGTOOL_MODE_CHIPINPUTS,  chipinputsMode);
        MODECASE(BGTOOL_MODE_ADDCHIP,     addchipMode);
        MODECASE(BGTOOL_MODE_CHIP,        chipMode);
        MODECASE(BGTOOL_MODE_ADVANCECHIP, advancechipMode);
        MODECASE(BGTOOL_MODE_REVERTCHIP,  revertchipMode);
        MODECASE(BGTOOL_MODE_LISTCHIP,    listchipMode);
        MODECASE(BGTOOL_MODE_DEFINEWARP,  definewarpMode);
        MODECASE(BGTOOL_MODE_UPDATEWARP,  updatewarpMode);
        MODECASE(BGTOOL_MODE_TOWARP,      towarpMode);
        MODECASE(BGTOOL_MODE_WARPINPUTS,  warpinputsMode);
        MODECASE(BGTOOL_MODE_ADDWARP,     addwarpMode);
        MODECASE(BGTOOL_MODE_WARP,        warpMode);
        MODECASE(BGTOOL_MODE_ADVANCEWARP, advancewarpMode);
        MODECASE(BGTOOL_MODE_REVERTWARP,  revertwarpMode);
        MODECASE(BGTOOL_MODE_LISTWARP,    listwarpMode);
        MODECASE(BGTOOL_MODE_PENDINGCLEANUPCHIPRUN, pendingcleanupchiprunMode);
        MODECASE(BGTOOL_MODE_PENDINGCLEANUPCHIPIMFILE, pendingcleanupchipimfileMode);
        MODECASE(BGTOOL_MODE_TOCLEANEDCHIPIMFILE, tocleanedchipimfileMode);
        MODECASE(BGTOOL_MODE_UPDATECHIPIMFILE, updatechipimfileMode);
        MODECASE(BGTOOL_MODE_PENDINGCLEANUPWARPRUN, pendingcleanupwarprunMode);
        MODECASE(BGTOOL_MODE_PENDINGCLEANUPWARPSKYFILE, pendingcleanupwarpskyfileMode);
        MODECASE(BGTOOL_MODE_TOCLEANEDWARPSKYFILE, tocleanedwarpskyfileMode);
        MODECASE(BGTOOL_MODE_EXPORTCHIP,  exportchipMode);
        MODECASE(BGTOOL_MODE_IMPORTCHIP,  importchipMode);
        MODECASE(BGTOOL_MODE_EXPORTWARP,  exportwarpMode);
        MODECASE(BGTOOL_MODE_IMPORTWARP,  importwarpMode);

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool exportTables(pxConfig *config, // Configuration (with DB handle)
                         const char *filename, // Filename to which to write
                         const tableData tables[], // Tables to export (NULL terminated)
                         const psMetadata *where, // WHERE restrictions
                         bool clean               // Write as cleaned?
                         )
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    FILE *file = fopen(filename, "w");
    if (!file) {
        psError(PXTOOLS_ERR_SYS, true, "failed to open output file %s", filename);
        return false;
    }

    if (!pxExportVersion(config, file)) {
        psError(psErrorCodeLast(), false, "failed to write dbversion to output file %s", filename);
        return false;
    }

    for (int i = 0; tables[i].name; i++) {
        const char *name = tables[i].name; // Name of table
        psString query = NULL;
        psStringAppend(&query, "SELECT * FROM %s", name);

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereSQL(where, NULL);
            psStringAppend(&query, " %s", whereClause);
            psFree(whereClause);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(psErrorCodeLast(), false, "database error");
            psFree(query);
            return false;
        }
        psFree(query);

        psArray *output = p_psDBFetchResult(config->dbh);
        if (!output) {
            psError(psErrorCodeLast(), false, "database error");
            return false;
        }
        if (!psArrayLength(output)) {
            psError(PXTOOLS_ERR_CONFIG, true, "no rows found");
            psFree(output);
            return false;
        }

        if (clean &&
            (strcmp(name, "chipBackgroundRun") == 0 ||
             strcmp(name, "warpBackgroundRun") == 0) &&
            !pxSetStateCleaned(name, "state", output)) {
            psFree(output);
            psError(psErrorCodeLast(), false, "pxSetStateClean failed for table %s", name);
            return false;
        }

        if (!ippdbPrintMetadatas(file, output, name, true)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
    }
    fclose(file);

    return true;
}

static bool importTables(pxConfig *config, // Configuration
                        const char *filename, //
                        const tableData tables[] // Tables to read in
                        )
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    unsigned int badLines = 0;          // Number of bad lines
    psMetadata *input = psMetadataConfigRead(NULL, &badLines, filename, false); // Input file contents
    if (!input) {
        psError(psErrorCodeLast(), false, "Unable to parse input file %s", filename);
        return false;
    }
    if (badLines > 0) {
        psWarning("%d bad lines encountered when parsing %s", badLines, filename);
    }

    if (!pxCheckImportVersion(config, input)) {
        psError(psErrorCodeLast(), false, "pxCheckImportVersion failed");
        return false;
    }

    // Import primary table
    for (int i = 0; tables[i].name; i++) {
        const char *name = tables[i].name; // Name of table
        psMetadataItem *item = psMetadataLookup(input, name); // Item from input
        psAssert(item, "%s not in input", name);
        psAssert(item->type == PS_DATA_METADATA_MULTI, "%s not MULTI type", name);
        psAssert(psListLength(item->data.list) == 1, "%s has multiple entries", name);
        psMetadataItem *entry = psListGet(item->data.list, PS_LIST_HEAD); // Entry of interest
        void *data = tables[i].parse(entry);                             // Parsed entry
        if (!data) {
            psError(PXTOOLS_ERR_CONFIG, false, "Unable to parse entry %s", name);
            psFree(input);
            return false;
        }
        if (!tables[0].insert(config->dbh, data)) {
            psError(psErrorCodeLast(), false, "Unable to insert entry %s", name);
            psFree(input);
            return false;
        }
    }

    psFree(input);

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for chip stage
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool definechipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args,   where, "-chip_id",            "chipRun.chip_id",       "==");
    PXOPT_COPY_S64(config->args,   where, "-cam_id",             "camRun.cam_id",         "==");
    PXOPT_COPY_S64(config->args,   where, "-exp_id",             "rawExp.exp_id",         "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_name",           "rawExp.exp_name",       "==");
    PXOPT_COPY_STR(config->args,   where, "-inst",               "rawExp.camera",         "==");
    PXOPT_COPY_STR(config->args,   where, "-telescope",          "rawExp.telescope",      "==");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_begin",      "rawExp.dateobs",        ">=");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_end",        "rawExp.dateobs",        "<=");
    PXOPT_COPY_STR(config->args,   where, "-exp_tag",            "rawExp.exp_tag",        "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_type",           "rawExp.exp_type",       "==");
    PXOPT_COPY_STR(config->args,   where, "-filelevel",          "rawExp.filelevel",      "==");
    PXOPT_COPY_STR(config->args,   where, "-filter",             "rawExp.filter",         "==");
    PXOPT_COPY_F64(config->args,   where, "-airmass_min",        "rawExp.airmass",        ">=");
    PXOPT_COPY_F64(config->args,   where, "-airmass_max",        "rawExp.airmass",        "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min",             "rawExp.ra",             ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max",             "rawExp.ra",             "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min",           "rawExp.decl",           ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max",           "rawExp.decl",           "<");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_min",       "rawExp.exp_time",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_max",       "rawExp.exp_time",       "<");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_min",             "rawExp.bg",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_max",             "rawExp.bg",             "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_min",       "rawExp.bg_stdev",       ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_max",       "rawExp.bg_stdev",       "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_min",  "rawExp.bg_mean_stdev",  ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_max",  "rawExp.bg_mean_stdev",  "<");
    PXOPT_COPY_F64(config->args,   where, "-alt_min",            "rawExp.alt",            ">=");
    PXOPT_COPY_F64(config->args,   where, "-alt_max",            "rawExp.alt",            "<");
    PXOPT_COPY_F64(config->args,   where, "-az_min",             "rawExp.az",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-az_max",             "rawExp.az",             "<");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_min",       "rawExp.ccd_temp",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_max",       "rawExp.ccd_temp",       "<");
    PXOPT_COPY_F64(config->args,   where, "-posang_min",         "rawExp.posang",         ">=");
    PXOPT_COPY_F64(config->args,   where, "-posang_max",         "rawExp.posang",         "<");
    PXOPT_COPY_STR(config->args,   where, "-object",             "rawExp.object",         "==");
    PXOPT_COPY_STR(config->args,   where, "-comment",            "rawExp.comment",        "LIKE");
    PXOPT_COPY_STR(config->args,   where, "-obs_mode",           "rawExp.obs_mode",       "LIKE");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_min",      "rawExp.sun_angle",      ">=");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_max",      "rawExp.sun_angle",      "<");
    PXOPT_COPY_STR(config->args,   where, "-label",              "chipRun.label",         "==");
    PXOPT_COPY_STR(config->args,   where, "-cam_label",          "camRun.label",          "==");

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(destreaked, config->args, "-destreaked", false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_STR(cam_label, config->args, "-cam_label", false, false);
    PXOPT_LOOKUP_S64(cam_id, config->args, "-cam_id", false, false);
    if (!cam_id && !cam_label) {
        psError(PXTOOLS_ERR_CONFIG, true, "either cam_id or cam_label is required");
        return false;
    }

    // Get chip runs to promote to chipBackgroundRun

    psString query = pxDataGet("bgtool_definechip.sql"); // Query to execute
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (destreaked) {
        psStringAppend(&query, " AND chipRun.magicked > 0");
    }

    psString labelHook = psStringCopy("");
    if (!rerun) {
        if (label) {
            // check for run with the newly specified label
            psStringAppend(&labelHook, "\nAND (chipBackgroundRun.label = '%s')", label);
        }
        psStringAppend(&query, "\nAND chip_bg_id IS NULL");
    }

    if (!psDBTransaction(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, labelHook)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        psFree(labelHook);
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    psFree(query);
    psFree(labelHook);

    psArray *output = p_psDBFetchResult(config->dbh); // Matching rows
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "chipBackgroundRun", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        chipBackgroundRunRow *row = chipBackgroundRunObjectFromMetadata(md);
        if (!row) {
            psError(psErrorCodeLast(), false, "failed to convert metadata into fakeRun");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }

        if (!chipBackgroundRunInsert(config->dbh, 0, row->chip_id, row->cam_id, "new",
                                     workdir     ? workdir    : row->workdir,
                                     label       ? label      : row->label,
                                     data_group  ? data_group : row->data_group,
                                     dist_group  ? dist_group : row->dist_group,
                                     reduction   ? reduction  : row->reduction,
                                     note        ? note       : row->note,
                                     NULL, 0)) {
            psError(psErrorCodeLast(), false, "database error");
            psFree(row);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }
        psFree(row);
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
        return false;
    }

    return true;
}

static bool updatechipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-state",      "chipBackgroundRun.state",     "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "chipBackgroundRun.data_group","LIKE");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "chipBackgroundRun.dist_group","LIKE");
    pxAddLabelSearchArgs(config,  where, "-label",     "chipBackgroundRun.label",     "LIKE");
    PXOPT_COPY_TIME(config->args, where, "-registered_begin", "chipBackgroundRun.registered",  ">=");
    PXOPT_COPY_TIME(config->args, where, "-registered_end",   "chipBackgroundRun.registered",  "<");

    PXOPT_LOOKUP_BOOL(destreaked, config->args, "-destreaked", false);
    if (destreaked) {
        psMetadataAddS64(where, PS_LIST_TAIL, "chipBackgroundRun.magicked", PS_META_DUPLICATE_OK, ">", 0);
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE chipBackgroundRun");
    bool result = pxUpdateRun(config, where, &query, "chipBackgroundRun", "chip_bg_id",
                              "chipBackgroundImfile", true, true);

    psFree(query);
    psFree(where);

    return result;
}

static bool tochipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "chipBackgroundRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("bgtool_tochip.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString whereStr = psStringCopy("");
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereStr, "\n AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereStr)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(whereStr);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to fetch result of query %s", query);
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "pendingchipBackgroundRun", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool chipinputsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "chipProcessedImfile.class_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("bgtool_chipinputs.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "pendingchipBackgroundImfile", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool addchipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(chip_bg_id, config->args, "-chip_bg_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

    // optional

    PXOPT_LOOKUP_S64(magicked, config->args, "-set_magicked", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_ppbackground, config->args, "-ver_ppbackground", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_S32(maskfrac_npix, config->args, "-maskfrac_npix", false, false);
    PXOPT_LOOKUP_F32(maskfrac_static, config->args, "-maskfrac_static", false, false);
    PXOPT_LOOKUP_F32(maskfrac_dynamic, config->args, "-maskfrac_dynamic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_magic, config->args, "-maskfrac_magic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_advisory, config->args, "-maskfrac_advisory", false, false);

    psString ver_code = pxMergeCodeVersions(ver_pslib, ver_psmodules);
    ver_code = pxMergeCodeVersions(ver_code, ver_ppbackground);
    ver_code = pxMergeCodeVersions(ver_code, ver_ppstats);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!chipBackgroundImfileInsert(config->dbh, chip_bg_id, class_id, path_base, "full", magicked, dtime_script,
                                    hostname, quality, fault, ver_code, bg, bg_stdev, maskfrac_npix,
                                    maskfrac_static, maskfrac_dynamic, maskfrac_magic, maskfrac_advisory)) {
        psError(psErrorCodeLast(), false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}

static bool chipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id",    "chipBackgroundRun.chip_bg_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",    "chipBackgroundRun.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",    "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "rawExp.exp_name", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "chipBackgroundImfile.class_id", "==");
    pxAddLabelSearchArgs(config, where, "-label",   "chipBackgroundRun.label", "LIKE");
    pxAddLabelSearchArgs(config, where, "-data_group",   "chipBackgroundRun.data_group", "LIKE");
    pxAddLabelSearchArgs(config, where, "-dist_group",   "chipBackgroundRun.data_group", "LIKE");

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("bgtool_chip.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString magicWhere = NULL;
    if (!pxmagicAddWhere(config, &magicWhere, "chipBackgroundImfile")) {
        psError(psErrorCodeLast(), false, "pxMagicAddWhere failed");
        return false;
    }
    if (!pxspaceAddWhere(config, &magicWhere, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (!all && !magicWhere) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }
    if (magicWhere) {
        psStringAppend(&query, "%s %s", psListLength(where->list) ? "AND" : "WHERE", magicWhere);
    }
    psFree(magicWhere);
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "chipBackgroundImfile", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
    }
    psFree(output);

    return true;
}


static bool advancechipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "chipBackgroundRun.label", "==");
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString select = pxDataGet("bgtool_advancechip.sql");
    if (!select) {
        psError(psErrorCodeLast(), false, "failed to retrieve SQL statement");
        return false;
    }

    psString selectWhere = psStringCopy("");
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&selectWhere, "\n AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&select, " %s", limitString);
        psFree(limitString);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, select, selectWhere)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(select);
        psFree(selectWhere);
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    psFree(select);
    psFree(selectWhere);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];
        bool status = true;             // Status of MD lookup
        psS64 chip_bg_id = psMetadataLookupS64(&status, row, "chip_bg_id");
        if (!status) {
            psError(PXTOOLS_ERR_PROG, true, "failed to look up value for chip_bg_id");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }
        psS64 magicked = psMetadataLookupS64(&status, row, "magicked");
        if (!status) {
            psError(PXTOOLS_ERR_PROG, true, "failed to look up value for magicked");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }

        if (!p_psDBRunQueryF(config->dbh,
                             "UPDATE chipBackgroundRun "
                             "SET state = 'full', magicked = %" PRId64 " "
                             " WHERE chip_bg_id = %" PRId64,
                             magicked, chip_bg_id)) {
            psError(psErrorCodeLast(), false, "database error");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }

        psS64 numUpdated = psDBAffectedRows(config->dbh);
        if (numUpdated != 1) {
            psError(PXTOOLS_ERR_PROG, true, "should have affected 1 row");
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            psFree(output);
            return false;
        }
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}

static bool revertchipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "chipBackgroundImfile.class_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "chipBackgroundRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "chipBackgroundImfile.fault", "==");

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("bgtool_revertchip.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
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

    int numDeleted = psDBAffectedRows(config->dbh);
    psLogMsg("bgtool", PS_LOG_INFO, "Deleted %d chipBackgroundImfiles", numDeleted);

    return true;
}

static bool listchipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-class_id", "chipProcessedImfile.class_id", "==");

    // chip_bg_id is required
    // PXOPT_LOOKUP_S64(chip_bg_id, config->args, "-chip_bg_id", true, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("bgtool_listchip.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString whereStr = psStringCopy("");
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nWHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(whereStr);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to fetch result of query %s", query);
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "chipProcessedImfile", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool pendingcleanupchiprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs(config, where, "-label", "chipBackgroundRun.label", "==");
    pxAddLabelSearchArgs(config, where, "-data_group", "chipBackgroundRun.data_group", "==");

    psString query = pxDataGet("bgtool_pendingcleanupchiprun.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipBackgroundRun", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}
static bool pendingcleanupchipimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // PXOPT_LOOKUP_S64(chip_bg_id, config->args, "-chip_bg_id", true, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundRun.chip_bg_id", "==");

    psString query = pxDataGet("bgtool_pendingcleanupchipimfile.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "chipBackgroundImfile", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool change_imfile_data_state(pxConfig *config, psString data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // chip_id, class_id are required
    PXOPT_LOOKUP_S64(chip_bg_id, config->args, "-chip_bg_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    psString query = pxDataGet("bgtool_change_imfile_data_state.sql");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state, chip_bg_id, class_id)) {
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
    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected atleast 1 row");
        return false;
    }

    query = pxDataGet("bgtool_change_chiprun_state.sql");
    if (!p_psDBRunQueryF(config->dbh, query, data_state, chip_bg_id, data_state)) {
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

static bool tocleanedchipimfileMode(pxConfig *config)
{
    return change_imfile_data_state(config, "cleaned");
}

static bool updatechipimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    // PXOPT_LOOKUP_S64(chip_bg_id, config->args, "-chip_bg_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chipBackgroundImfile.chip_bg_id", "==");
    PXOPT_COPY_S64(config->args, where, "-class_id", "chipBackgroundImfile.class_id", "==");

    // optional

    PXOPT_LOOKUP_STR(data_state, config->args, "-set_data_state", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-set_fault", false, false);
    if (!data_state && !fault) {
        psError(PS_ERR_UNKNOWN, true, "must supply either -set_fault or -set_data_state");
        return false;
    }
    char *sep = "";
    psString query = psStringCopy("UPDATE chipBackgroundImfile SET ");
    if (data_state) {
        if (!validDataState(data_state)) {
            return false;
        }
        psStringAppend(&query, "%s data_state = '%s'", sep, data_state);
        sep = ", ";
    }
    if (fault) {
        psStringAppend(&query, "%s fault = %d", sep, fault);
        sep = ", ";
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
    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    return true;
}

static bool exportchipMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // PXOPT_LOOKUP_S64(chip_bg_id, config->args, "-chip_bg_id", true,  false);
    PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
    PXOPT_LOOKUP_BOOL(clean,  config->args, "-clean", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id", "chip_bg_id", "==");

    bool status = exportTables(config, outfile, chipTables, where, clean);

    psFree(where);
    return status;
}

static bool importchipMode(pxConfig *config)
{
    PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);

    return importTables(config, infile, chipTables);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for warp stage
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool definewarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args,   where, "-warp_id",            "warpRun.warp_id",       "==");
    PXOPT_COPY_S64(config->args,   where, "-exp_id",             "rawExp.exp_id",         "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_name",           "rawExp.exp_name",       "==");
    PXOPT_COPY_STR(config->args,   where, "-inst",               "rawExp.camera",         "==");
    PXOPT_COPY_STR(config->args,   where, "-telescope",          "rawExp.telescope",      "==");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_begin",      "rawExp.dateobs",        ">=");
    PXOPT_COPY_TIME(config->args,  where, "-dateobs_end",        "rawExp.dateobs",        "<=");
    PXOPT_COPY_STR(config->args,   where, "-exp_tag",            "rawExp.exp_tag",        "==");
    PXOPT_COPY_STR(config->args,   where, "-exp_type",           "rawExp.exp_type",       "==");
    PXOPT_COPY_STR(config->args,   where, "-filelevel",          "rawExp.filelevel",      "==");
    PXOPT_COPY_STR(config->args,   where, "-filter",             "rawExp.filter",         "==");
    PXOPT_COPY_F64(config->args,   where, "-airmass_min",        "rawExp.airmass",        ">=");
    PXOPT_COPY_F64(config->args,   where, "-airmass_max",        "rawExp.airmass",        "<");
    PXOPT_COPY_RADEC(config->args, where, "-ra_min",             "rawExp.ra",             ">=");
    PXOPT_COPY_RADEC(config->args, where, "-ra_max",             "rawExp.ra",             "<");
    PXOPT_COPY_RADEC(config->args, where, "-decl_min",           "rawExp.decl",           ">=");
    PXOPT_COPY_RADEC(config->args, where, "-decl_max",           "rawExp.decl",           "<");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_min",       "rawExp.exp_time",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-exp_time_max",       "rawExp.exp_time",       "<");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_min",             "rawExp.bg",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_max",             "rawExp.bg",             "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_min",       "rawExp.bg_stdev",       ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_stdev_max",       "rawExp.bg_stdev",       "<");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_min",  "rawExp.bg_mean_stdev",  ">=");
    PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_max",  "rawExp.bg_mean_stdev",  "<");
    PXOPT_COPY_F64(config->args,   where, "-alt_min",            "rawExp.alt",            ">=");
    PXOPT_COPY_F64(config->args,   where, "-alt_max",            "rawExp.alt",            "<");
    PXOPT_COPY_F64(config->args,   where, "-az_min",             "rawExp.az",             ">=");
    PXOPT_COPY_F64(config->args,   where, "-az_max",             "rawExp.az",             "<");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_min",       "rawExp.ccd_temp",       ">=");
    PXOPT_COPY_F32(config->args,   where, "-ccd_temp_max",       "rawExp.ccd_temp",       "<");
    PXOPT_COPY_F64(config->args,   where, "-posang_min",         "rawExp.posang",         ">=");
    PXOPT_COPY_F64(config->args,   where, "-posang_max",         "rawExp.posang",         "<");
    PXOPT_COPY_STR(config->args,   where, "-object",             "rawExp.object",         "==");
    PXOPT_COPY_STR(config->args,   where, "-comment",            "rawExp.comment",        "LIKE");
    PXOPT_COPY_STR(config->args,   where, "-obs_mode",           "rawExp.obs_mode",       "LIKE");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_min",      "rawExp.sun_angle",      ">=");
    PXOPT_COPY_F32(config->args,   where, "-sun_angle_max",      "rawExp.sun_angle",      "<");
    pxAddLabelSearchArgs(config,   where, "-warp_label",         "warpRun.label",         "==");
    pxAddLabelSearchArgs(config,   where, "-chip_bg_label",      "chipBackgroundRun.label", "==");
    pxAddLabelSearchArgs(config,   where, "-chip_label",         "chipRun.label",         "==");

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters are required");
        return false;
    }

    PXOPT_LOOKUP_BOOL(destreaked, config->args, "-destreaked", false);
    PXOPT_LOOKUP_BOOL(rerun, config->args, "-rerun", false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_STR(data_group, config->args, "-set_data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group, config->args, "-set_dist_group", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(note, config->args, "-set_note", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);

    // Get warp runs to promote to warpBackgroundRun

    psString query = pxDataGet("bgtool_definewarp.sql"); // Query to execute
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (destreaked) {
        psStringAppend(&query, " AND warpRun.magicked > 0");
    }

    psString labelHook = psStringCopy("");
    if (!rerun) {
        if (label) {
            // check for run with the newly specified label
            psStringAppend(&labelHook, "\nAND (warpBackgroundRun.label = '%s')", label);
        }
        psStringAppend(&query, "\nAND warp_bg_id IS NULL");
    }

    if (!psDBTransaction(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, labelHook)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(labelHook);
        psFree(query);
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    psFree(labelHook);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh); // Matching rows
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "warpBackgroundRun", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        psS64 chip_bg_id = psMetadataLookupS64(NULL, md, "chip_bg_id");

        
        warpBackgroundRunRow *row = warpBackgroundRunObjectFromMetadata(md);
        if (!row) {
            psError(psErrorCodeLast(), false, "failed to convert metadata into warpRun");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }

        if (!warpBackgroundRunInsert(config->dbh, 0, row->warp_id, chip_bg_id, "new",
                                     workdir     ? workdir    : row->workdir,
                                     label       ? label      : row->label,
                                     data_group  ? data_group : row->data_group,
                                     dist_group  ? dist_group : row->dist_group,
                                     reduction   ? reduction  : row->reduction,
                                     note        ? note       : row->note,
                                     NULL, 0)) {
            psError(psErrorCodeLast(), false, "database error");
            psFree(row);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }
        psFree(row);
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
        return false;
    }

    return true;
}

static bool updatewarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warpBackgroundRun.warp_bg_id",   "==");
    PXOPT_COPY_STR(config->args, where, "-state",      "warpBackgroundRun.state",     "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "warpBackgroundRun.data_group","LIKE");
    PXOPT_COPY_STR(config->args, where, "-dist_group", "warpBackgroundRun.dist_group","LIKE");
    pxAddLabelSearchArgs(config,  where, "-label",     "warpBackgroundRun.label",     "LIKE");
    PXOPT_COPY_TIME(config->args, where, "-registered_begin", "warpBackgroundRun.registered",  ">=");
    PXOPT_COPY_TIME(config->args, where, "-registered_end",   "warpBackgroundRun.registered",  "<");

    PXOPT_LOOKUP_BOOL(destreaked, config->args, "-destreaked", false);
    if (destreaked) {
        psMetadataAddS64(where, PS_LIST_TAIL, "warpBackgroundRun.magicked", PS_META_DUPLICATE_OK, ">", 0);
    }

    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }

    psString query = psStringCopy("UPDATE warpBackgroundRun");
    bool result = pxUpdateRun(config, where, &query, "warpBackgroundRun", "warp_bg_id",
                              "warpBackgroundSkyfile", true, true);

    psFree(query);
    psFree(where);

    return result;
}

static bool towarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warpBackgroundRun.warp_bg_id", "==");
    pxAddLabelSearchArgs (config, where, "-label", "warpBackgroundRun.label", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("bgtool_towarp.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString whereStr = psStringCopy("");
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&whereStr, "\n AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, whereStr)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(whereStr);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to fetch result of query %s", query);
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "warpBackgroundRun", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool warpinputsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warpBackgroundRun.warp_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "skycell_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("bgtool_warpinputs.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    if (!ippdbPrintMetadatas(stdout, output, "warpBackgroundSkyfile", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool addwarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(warp_bg_id, config->args, "-warp_bg_id", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", true, false);

    // optional

    PXOPT_LOOKUP_S64(magicked, config->args, "-set_magicked", false, false);
    PXOPT_LOOKUP_F32(dtime_script, config->args, "-dtime_script", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_STR(ver_pslib, config->args, "-ver_pslib", false, false);
    PXOPT_LOOKUP_STR(ver_psmodules, config->args, "-ver_psmodules", false, false);
    PXOPT_LOOKUP_STR(ver_pswarp, config->args, "-ver_pswarp", false, false);
    PXOPT_LOOKUP_STR(ver_ppstats, config->args, "-ver_ppstats", false, false);
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_S32(maskfrac_npix, config->args, "-maskfrac_npix", false, false);
    PXOPT_LOOKUP_F32(maskfrac_static, config->args, "-maskfrac_static", false, false);
    PXOPT_LOOKUP_F32(maskfrac_dynamic, config->args, "-maskfrac_dynamic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_magic, config->args, "-maskfrac_magic", false, false);
    PXOPT_LOOKUP_F32(maskfrac_advisory, config->args, "-maskfrac_advisory", false, false);

    psString ver_code = pxMergeCodeVersions(ver_pslib, ver_psmodules);
    ver_code = pxMergeCodeVersions(ver_code, ver_pswarp);
    ver_code = pxMergeCodeVersions(ver_code, ver_ppstats);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!warpBackgroundSkyfileInsert(config->dbh, warp_bg_id, skycell_id, path_base, "full", magicked, dtime_script,
                                    hostname, quality, fault, ver_code, bg, bg_stdev, maskfrac_npix,
                                    maskfrac_static, maskfrac_dynamic, maskfrac_magic, maskfrac_advisory)) {
        psError(psErrorCodeLast(), false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}

static bool warpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id",   "warpBackgroundRun.warp_bg_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id",      "warpBackgroundRun.warp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_bg_id",   "warpBackgroundRun.chip_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",   "warpBackgroundSkyfile.skycell_id", "==");
    pxAddLabelSearchArgs(config, where, "-label",        "warpBackgroundRun.label", "LIKE");
    pxAddLabelSearchArgs(config, where, "-data_group",   "warpBackgroundRun.data_group", "LIKE");
    pxAddLabelSearchArgs(config, where, "-dist_group",   "warpBackgroundRun.data_group", "LIKE");

    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find all rawImfiles matching the default query
    psString query = pxDataGet("bgtool_warp.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString magicWhere = NULL;
    if (!pxmagicAddWhere(config, &magicWhere, "warpBackgroundSkyfile")) {
        psError(psErrorCodeLast(), false, "pxMagicAddWhere failed");
        return false;
    }
    if (!pxspaceAddWhere(config, &magicWhere, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else if (!all && !magicWhere) {
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters or -all are required");
        return false;
    }
    if (magicWhere) {
        psStringAppend(&query, "%s %s", psListLength(where->list) ? "AND" : "WHERE", magicWhere);
    }
    psFree(magicWhere);
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }
    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "warpBackgroundSkyfile", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
    }
    psFree(output);

    return true;
}


static bool advancewarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warp_bg_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "label", "==");
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString select = pxDataGet("bgtool_advancewarp.sql");
    if (!select) {
        psError(psErrorCodeLast(), false, "failed to retrieve SQL statement");
        return false;
    }

    psString selectWhere = psStringCopy("");
    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&selectWhere, "\n AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&select, " %s", limitString);
        psFree(limitString);
    }

    if (!psDBTransaction(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, select, selectWhere)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(select);
        psFree(selectWhere);
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    psFree(select);
    psFree(selectWhere);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        if (!psDBRollback(config->dbh)) {
            psError(psErrorCodeLast(), false, "database error");
        }
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];
        bool status = true;             // Status of MD lookup
        psS64 warp_bg_id = psMetadataLookupS64(&status, row, "warp_bg_id");
        if (!status) {
            psError(PXTOOLS_ERR_PROG, true, "failed to look up value for warp_bg_id");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }
        psS64 magicked = psMetadataLookupS64(&status, row, "magicked");
        if (!status) {
            psError(PXTOOLS_ERR_PROG, true, "failed to look up value for magicked");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }

        if (!p_psDBRunQueryF(config->dbh,
                             "UPDATE warpBackgroundRun "
                             "SET state = 'full', magicked = %" PRId64 " "
                             " WHERE warp_bg_id = %" PRId64,
                             magicked, warp_bg_id)) {
            psError(psErrorCodeLast(), false, "database error");
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            return false;
        }

        psS64 numUpdated = psDBAffectedRows(config->dbh);
        if (numUpdated != 1) {
            psError(PXTOOLS_ERR_PROG, true, "should have affected 1 row");
            if (!psDBRollback(config->dbh)) {
                psError(psErrorCodeLast(), false, "database error");
            }
            psFree(output);
            return false;
        }
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }

    return true;
}

static bool revertwarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warpBackgroundRun.warp_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpBackgroundSkyfile.skycell_id", "==");
    pxAddLabelSearchArgs(config, where, "-label", "warpBackgroundRun.label", "==");
    PXOPT_COPY_S16(config->args, where, "-fault", "warpBackgroundSkyfile.fault", "==");

    if (!psListLength(where->list) && !psMetadataLookupBool(NULL, config->args, "-all")) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, true, "search parameters are required");
        return false;
    }

    psString query = pxDataGet("bgtool_revertwarp.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
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

    int numDeleted = psDBAffectedRows(config->dbh);
    psLogMsg("bgtool", PS_LOG_INFO, "Deleted %d warpBackgroundSkyfiles", numDeleted);

    return true;
}
static bool listwarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warpBackgroundRun.warp_bg_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "warpSkyfile.skycell_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("bgtool_listwarp.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    psString whereStr = psStringCopy("");
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nWHERE %s", whereClause);
    psFree(whereClause);
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(whereStr);
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to fetch result of query %s", query);
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        if (!ippdbPrintMetadatas(stdout, output, "warpSkyfile", !simple)) {
            psError(psErrorCodeLast(), false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}

static bool pendingcleanupwarprunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    pxAddLabelSearchArgs(config, where, "-label", "warpBackgroundRun.label", "==");

    psString query = pxDataGet("bgtool_pendingcleanupwarprun.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpBackgroundRun", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}
static bool pendingcleanupwarpskyfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // PXOPT_LOOKUP_S64(warp_bg_id, config->args, "-warp_bg_id", true, false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warpBackgroundRun.warp_bg_id", "==");

    psString query = pxDataGet("bgtool_pendingcleanupwarpskyfile.sql");
    if (!query) {
        psError(psErrorCodeLast(), false, "failed to retreive SQL statement");
        return false;
    }

    if (where && psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(psErrorCodeLast(), false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(psErrorCodeLast(), false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("bgtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "warpBackgroundSkyfile", !simple)) {
        psError(psErrorCodeLast(), false, "failed to print array");
        psFree(output);
        return false;
    }
    psFree(output);

    return true;
}

static bool change_skyfile_data_state(pxConfig *config, psString data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // warp_id, skycell_id are required
    PXOPT_LOOKUP_S64(warp_bg_id, config->args, "-warp_bg_id", true, false);
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", true, false);

    psString query = pxDataGet("bgtool_change_skyfile_data_state.sql");

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state, warp_bg_id, skycell_id)) {
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
    if (psDBAffectedRows(config->dbh) < 1) {
        psError(PS_ERR_UNKNOWN, false, "should have affected atleast 1 row");
        return false;
    }

    query = pxDataGet("bgtool_change_warprun_state.sql");
    if (!p_psDBRunQueryF(config->dbh, query, data_state, warp_bg_id, data_state)) {
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

static bool tocleanedwarpskyfileMode(pxConfig *config)
{
    return change_skyfile_data_state(config, "cleaned");
}

static bool exportwarpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // PXOPT_LOOKUP_S64(warp_bg_id, config->args, "-warp_bg_id", true,  false);
    PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
    PXOPT_LOOKUP_BOOL(clean,  config->args, "-clean", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-warp_bg_id", "warp_bg_id", "==");

    bool status = exportTables(config, outfile, warpTables, where, clean);

    psFree(where);
    return status;
}

static bool importwarpMode(pxConfig *config)
{
    PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);

    return importTables(config, infile, warpTables);
}

static bool validDataState(psString data_state)
{
    // NOTE: can't use pxIsValidState because
    // update, scrubbed and purged are not supported for the backround stages (yet?)
    if (!strcmp(data_state, "new") ||
        !strcmp(data_state, "full") ||
        !strcmp(data_state, "drop") ||
        !strcmp(data_state, "cleaned") ||
        !strcmp(data_state, "goto_cleaned") ||
        !strcmp(data_state, "error_cleaned")) {
        return true;
    } else {
        psError(PS_ERR_UNKNOWN, true, "%s is not a valid value data_state", data_state);
        return false;
    }
}

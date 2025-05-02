/*
 * dettool.c
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

#include "dettool.h"

static bool pendingMode(pxConfig *config);
static bool definebytagMode(pxConfig *config);
static bool definebyqueryMode(pxConfig *config);
static bool definebydetrunMode(pxConfig *config);
static bool runsMode(pxConfig *config);
static bool childlessrunMode(pxConfig *config);
static bool inputMode(pxConfig *config);
static bool rawMode(pxConfig *config);
static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

// run
static bool updatedetrunMode(pxConfig *config);
static bool rerunMode(pxConfig *config);
// register
static bool register_detrendMode(pxConfig *config);

//static psArray *validDetInputClassIds(pxConfig *config, const char *det_id);
//static psArray *searchInputImfiles(pxConfig *config, const char *det_id);
static detInputExpRow *rawDetrenTodetInputExpRow(rawExpRow *rawExp, psS64 det_id, psS32 iteration);
static psS32 incrementIteration(pxConfig *config, psS64 det_id);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;
/*
typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
} ExportTable;
*/
int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = dettoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(DETTOOL_MODE_PENDING,          pendingMode);
        MODECASE(DETTOOL_MODE_DEFINEBYTAG,      definebytagMode);
        MODECASE(DETTOOL_MODE_DEFINEBYQUERY,    definebyqueryMode);
        MODECASE(DETTOOL_MODE_DEFINEBYDETRUN,   definebydetrunMode);
        MODECASE(DETTOOL_MODE_RUNS,             runsMode);
        MODECASE(DETTOOL_MODE_CHILDLESSRUN,     childlessrunMode);
        MODECASE(DETTOOL_MODE_INPUT,            inputMode);
        MODECASE(DETTOOL_MODE_RAW,              rawMode);
        // correction
        MODECASE(DETTOOL_MODE_MAKECORRECTION,    makecorrectionMode);
        MODECASE(DETTOOL_MODE_TOCORRECTEXP,      tocorrectexpMode);
        MODECASE(DETTOOL_MODE_TOCORRECTIMFILE,   tocorrectimfileMode);
        MODECASE(DETTOOL_MODE_ADDCORRECTIMFILE,  addcorrectimfileMode);
        // imfile
        MODECASE(DETTOOL_MODE_TOPROCESSEDIMFILE,toprocessedimfileMode);
        MODECASE(DETTOOL_MODE_ADDPROCESSEDIMFILE,addprocessedimfileMode);
        MODECASE(DETTOOL_MODE_PROCESSEDIMFILE,  processedimfileMode);
        MODECASE(DETTOOL_MODE_REVERTPROCESSEDIMFILE, revertprocessedimfileMode);
        MODECASE(DETTOOL_MODE_UPDATEPROCESSEDIMFILE, updateprocessedimfileMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_PROCESSEDIMFILE, pendingcleanup_processedimfileMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_PROCESSEDIMFILE, donecleanup_processedimfileMode);
        // exp
        MODECASE(DETTOOL_MODE_TOPROCESSEDEXP,   toprocessedexpMode);
        MODECASE(DETTOOL_MODE_ADDPROCESSEDEXP,  addprocessedexpMode);
        MODECASE(DETTOOL_MODE_PROCESSEDEXP,     processedexpMode);
        MODECASE(DETTOOL_MODE_REVERTPROCESSEDEXP, revertprocessedexpMode);
        MODECASE(DETTOOL_MODE_UPDATEPROCESSEDEXP, updateprocessedexpMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_PROCESSEDEXP, pendingcleanup_processedexpMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_PROCESSEDEXP, donecleanup_processedexpMode);
        MODECASE(DETTOOL_MODE_UPDATESTATE_PROCESSED, updatestateprocessedMode);
        // stacked
        MODECASE(DETTOOL_MODE_TOSTACKED,        tostackedMode);
        MODECASE(DETTOOL_MODE_ADDSTACKED,       addstackedMode);
        MODECASE(DETTOOL_MODE_STACKED,          stackedMode);
        MODECASE(DETTOOL_MODE_REVERTSTACKED,    revertstackedMode);
        MODECASE(DETTOOL_MODE_UPDATESTACKED,    updatestackedMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_STACKED, pendingcleanup_stackedMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_STACKED, donecleanup_stackedMode);
        // normalizedstat
        MODECASE(DETTOOL_MODE_TONORMALIZEDSTAT, tonormalizedstatMode);
        MODECASE(DETTOOL_MODE_ADDNORMALIZEDSTAT,addnormalizedstatMode);
        MODECASE(DETTOOL_MODE_NORMALIZEDSTAT,   normalizedstatMode);
        MODECASE(DETTOOL_MODE_REVERTNORMALIZEDSTAT, revertnormalizedstatMode);
        MODECASE(DETTOOL_MODE_UPDATENORMALIZEDSTAT, updatenormalizedstatMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDSTAT, pendingcleanup_normalizedstatMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_NORMALIZEDSTAT, donecleanup_normalizedstatMode);
        // normalizedimfile
        MODECASE(DETTOOL_MODE_TONORMALIZE,      tonormalizeMode);
        MODECASE(DETTOOL_MODE_ADDNORMALIZEDIMFILE,addnormalizedimfileMode);
        MODECASE(DETTOOL_MODE_NORMALIZEDIMFILE, normalizedimfileMode);
        MODECASE(DETTOOL_MODE_REVERTNORMALIZEDIMFILE, revertnormalizedimfileMode);
        MODECASE(DETTOOL_MODE_UPDATENORMALIZEDIMFILE, updatenormalizedimfileMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDIMFILE, pendingcleanup_normalizedimfileMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_NORMALIZEDIMFILE, donecleanup_normalizedimfileMode);
        // normalizedexp
        MODECASE(DETTOOL_MODE_TONORMALIZEDEXP,  tonormalizedexpMode);
        MODECASE(DETTOOL_MODE_ADDNORMALIZEDEXP, addnormalizedexpMode);
        MODECASE(DETTOOL_MODE_NORMALIZEDEXP,    normalizedexpMode);
        MODECASE(DETTOOL_MODE_REVERTNORMALIZEDEXP, revertnormalizedexpMode);
        MODECASE(DETTOOL_MODE_UPDATENORMALIZEDEXP, updatenormalizedexpMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_NORMALIZEDEXP, pendingcleanup_normalizedexpMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_NORMALIZEDEXP, donecleanup_normalizedexpMode);
        // residimfile
        MODECASE(DETTOOL_MODE_TORESIDIMFILE,    toresidimfileMode);
        MODECASE(DETTOOL_MODE_ADDRESIDIMFILE,   addresidimfileMode);
        MODECASE(DETTOOL_MODE_RESIDIMFILE,      residimfileMode);
        MODECASE(DETTOOL_MODE_REVERTRESIDIMFILE,revertresidimfileMode);
        MODECASE(DETTOOL_MODE_UPDATERESIDIMFILE, updateresidimfileMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_RESIDIMFILE, pendingcleanup_residimfileMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_RESIDIMFILE, donecleanup_residimfileMode);
        // residexp
        MODECASE(DETTOOL_MODE_TORESIDEXP,       toresidexpMode);
        MODECASE(DETTOOL_MODE_ADDRESIDEXP,      addresidexpMode);
        MODECASE(DETTOOL_MODE_RESIDEXP,         residexpMode);
        MODECASE(DETTOOL_MODE_REVERTRESIDEXP,   revertresidexpMode);
        MODECASE(DETTOOL_MODE_UPDATERESIDEXP,   updateresidexpMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_RESIDEXP, pendingcleanup_residexpMode);
        MODECASE(DETTOOL_MODE_DONECLEANUP_RESIDEXP, donecleanup_residexpMode);
        MODECASE(DETTOOL_MODE_UPDATESTATE_RESID, updatestateresidMode);
        // detrunsummary
        MODECASE(DETTOOL_MODE_TODETRUNSUMMARY,  todetrunsummaryMode);
        MODECASE(DETTOOL_MODE_ADDDETRUNSUMMARY, adddetrunsummaryMode);
        MODECASE(DETTOOL_MODE_DETRUNSUMMARY,    detrunsummaryMode);
        MODECASE(DETTOOL_MODE_REVERTDETRUNSUMMARY, revertdetrunsummaryMode);
        MODECASE(DETTOOL_MODE_UPDATEDETRUNSUMMARY, updatedetrunsummaryMode);
        MODECASE(DETTOOL_MODE_UPDATEDETRUN,     updatedetrunMode);
        MODECASE(DETTOOL_MODE_RERUN,            rerunMode);
        MODECASE(DETTOOL_MODE_PENDINGCLEANUP_DETRUNSUMMARY, pendingcleanup_detrunsummaryMode);
        // register
        MODECASE(DETTOOL_MODE_REGISTER_DETREND, register_detrendMode);
        MODECASE(DETTOOL_MODE_REGISTER_DETREND_IMFILE, register_detrend_imfileMode);
        MODECASE(DETTOOL_MODE_EXPORTRUN,               exportrunMode);
        MODECASE(DETTOOL_MODE_IMPORTRUN,               importrunMode);
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

static bool pendingMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("dettool_pending.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_type", "exp_type", "==");
    PXOPT_COPY_STR(config->args, where, "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "rawExp");
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

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "rawExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool definebytagMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // what type of detRun is this?
    // required
    PXOPT_LOOKUP_STR(det_type, config->args, "-det_type", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false);

    // optional
    PXOPT_LOOKUP_STR(filelevel, config->args, "-filelevel", false, false);

    // check mode
    PXOPT_LOOKUP_STR(mode, config->args, "-mode", true, false); // default ('master') is supplied
    if (!isValidMode(config, mode)) {
        psError(PS_ERR_UNKNOWN, false, "invalid mode");
        return false;
    }

    // get -ref_det_id and -ref_iter : required for 'verify' mode / disallowed otherwise
    PXOPT_LOOKUP_S64(ref_det_id, config->args, "-ref_det_id", false, false);
    PXOPT_LOOKUP_S32(ref_iter, config->args, "-ref_iter", false, false);
    if (!strcmp(mode, "verify") && ((ref_det_id == 0) || (ref_iter == -1))) {
        psError(PS_ERR_UNKNOWN, false, "verify mode requires both -ref_det_id and -ref_iter");
        return false;
    }
    if (strcmp(mode, "verify") && ((ref_det_id != 0) || (ref_iter != -1))) {
        psError(PS_ERR_UNKNOWN, false, "master mode cannot have -ref_det_id or -ref_iter set");
        return false;
    }

    PXOPT_LOOKUP_STR(camera, config->args, "-inst", false, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", false, false);
    PXOPT_LOOKUP_STR(exp_type, config->args, "-exp_type", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", false, false);

    PXOPT_LOOKUP_F32(airmass_min, config->args, "-airmass_min", false, false);
    PXOPT_LOOKUP_F32(airmass_max, config->args, "-airmass_max", false, false);
    PXOPT_LOOKUP_F32(exp_time_min, config->args, "-exp_time_min", false, false);
    PXOPT_LOOKUP_F32(exp_time_max, config->args, "-exp_time_max", false, false);
    PXOPT_LOOKUP_F32(ccd_temp_min, config->args, "-ccd_temp_min", false, false);
    PXOPT_LOOKUP_F32(ccd_temp_max, config->args, "-ccd_temp_max", false, false);
    PXOPT_LOOKUP_F64(posang_min, config->args, "-posang_min", false, false);
    PXOPT_LOOKUP_F64(posang_max, config->args, "-posang_max", false, false);
    PXOPT_LOOKUP_F64(solang_min, config->args, "-sun_angle_min", false, false);
    PXOPT_LOOKUP_F64(solang_max, config->args, "-sun_angle_max", false, false);
    PXOPT_LOOKUP_TIME(time_begin, config->args, "-time_begin", false, false);
    PXOPT_LOOKUP_TIME(time_end, config->args, "-time_end", false, false);
    PXOPT_LOOKUP_TIME(use_begin, config->args, "-use_begin", false, false);
    PXOPT_LOOKUP_TIME(use_end, config->args, "-use_end", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);

    // default
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);

    // we have to support multiple exp_ids
    psMetadataItem *item = psMetadataLookup(config->args, "-exp_id");
    if (!item) {
        // this shouldn't actually happen when using psArgs
        psError(PS_ERR_UNKNOWN, true, "at least one -exp_id is required");
        return false;
    }
    psMetadata *where = psMetadataAlloc();

    if ((item->type != PS_DATA_METADATA_MULTI) && (item->type != PS_DATA_S64)) {
        psAbort("-exp_id was not parsed correctly (this should not happen");
    }

    // make sure that -exp_id was parsed correctly
    if (item->type == PS_DATA_METADATA_MULTI) {
        psListIterator *iter = psListIteratorAlloc(item->data.list, 0, false);
        psMetadataItem *mItem = NULL;
        while ((mItem = psListGetAndIncrement(iter))) {
            psS64 exp_id = mItem->data.S64;
            if (!psMetadataAddS64(where, PS_LIST_TAIL, "exp_id", PS_META_DUPLICATE_OK, "==", exp_id)) {
                psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
                psFree(iter);
                psFree(where);
                return false;
            }
        }
        psFree(iter);
    }
    if (item->type == PS_DATA_S64) {
        psS64 exp_id = item->data.S64;
        if (!psMetadataAddS64(where, PS_LIST_TAIL, "exp_id", PS_META_DUPLICATE_OK, "==", exp_id)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
            psFree(where);
            return false;
        }
    }

    if (psListLength(where->list) < 1) {
        psFree(where);
        where = NULL;
    }

    // check that the specified exp_ids actually exist
    psArray *detrendExps = rawExpSelectRowObjects(config->dbh, where, 0);
    psFree(where);
    if (!detrendExps) {
        psError(PS_ERR_UNKNOWN, false, "no rawExp rows found");
        return false;
    }

    // we should have one rawExp row per exp_id specified
    if (psListLength(item->data.list) != psArrayLength(detrendExps)) {
        psAbort(    "an -exp_id matched more then one rawExp (this should not happen");

    }

    // check to see if -filelevel was set on the command line
    if (!filelevel) {
        filelevel = psStringCopy(((rawExpRow *)detrendExps->data[0])->filelevel);
    }

    // start a transaction so we don't end up with childlessed det_ids
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(detrendExps);
        return false;
    }

    // the first iteration is always 0
    // XXX the camera name is set from the first inputExp
    // XXX det_id
    detRunInsert(config->dbh,
                 0,
                 0,
                 det_type,
                 mode,
                 "run",
                 filelevel,
                 workdir,
                 camera,
                 telescope,
                 exp_type,
                 reduction,
                 filter,
                 airmass_min,
                 airmass_max,
                 exp_time_min,
                 exp_time_max,
                 ccd_temp_min,
                 ccd_temp_max,
                 posang_min,
                 posang_max,
                 registered,
                 time_begin,
                 time_end,
                 use_begin,
                 use_end,
                 solang_min,
                 solang_max,
                 label,
                 ref_det_id,
                 ref_iter
        );
    psS64 det_id = psDBLastInsertID(config->dbh);

    // create new detInputExp row(s) from the rawExp row(s)
    psArray *inputExps = psArrayAllocEmpty(psArrayLength(detrendExps));
    for (long i = 0; i < psArrayLength(detrendExps); i++) {
        detInputExpRow *inputExp = rawDetrenTodetInputExpRow(
            detrendExps->data[i],
            det_id,
            0 // the first iteration is explicitly 0
        );
        psArrayAdd(inputExps, 0, inputExp);
        psFree(inputExp);
    }

    psFree(detrendExps);

    // insert detInputExp objects into the database
    if (!detInputExpInsertObjects(config->dbh, inputExps)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psFree(inputExps);
        return false;
    }
    psFree(inputExps);

    // point of no return for det_id creation
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // print the new det_id
    psArray *detRuns = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", det_id);
        detRuns = psDBSelectRows(config->dbh, "detRun", where, 0);
        psFree(where);
    }
    if (!detRuns) {
        psError(PS_ERR_UNKNOWN, false, "can't find the detRun we just created");
        return false;
    }
    // sanity check results
    if (psArrayLength(detRuns) != 1) {
        psAbort("found more then one detRun matching det_id %" PRId64 " (this should not happen)", det_id);
        return false;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, detRuns, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(detRuns);
        return false;
    }
    psFree(detRuns);

    return true;
}

#if 0
// This function is used to convert the det_id from an int, as it is used
// internally, to be a string for external use.  The rationale being that we may
// want to change how det_id is generated in the future and don't want to
// external programs to become depending on this value being an int.
static bool convertDetIdToStr(psArray *mds)
{
    PS_ASSERT_PTR_NON_NULL(mds, false);

    for (long i = 0; i < psArrayLength(mds); i++) {
        psMetadata *md = mds->data[i];
        bool status = false;
        psS32 det_id = psMetadataLookupS32(&status, md, "det_id");
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for det_id");
            return false;
        }
        psMetadataRemoveKey(md, "det_id");
        psString det_idStr = psDBIntToString((psU64)det_id);
        psMetadataAddStr(mds->data[i], PS_LIST_HEAD, "det_id", 0, NULL, det_idStr);
        psFree(det_idStr);
    }

    return true;
}
#endif

static bool definebyqueryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_STR(det_type, config->args, "-det_type", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);

    // check mode
    PXOPT_LOOKUP_STR(mode, config->args, "-mode", true, false); // default ('master') is supplied
    if (!isValidMode(config, mode)) {
        psError(PS_ERR_UNKNOWN, false, "invalid mode");
        return false;
    }

    // get -ref_det_id and -ref_iter : required for 'verify' mode / disallowed otherwise
    PXOPT_LOOKUP_S64(ref_det_id, config->args, "-ref_det_id", false, false);
    PXOPT_LOOKUP_S32(ref_iter, config->args, "-ref_iter", false, false);
    if (!strcmp(mode, "verify") && ((ref_det_id == 0) || (ref_iter == -1))) {
        psError(PS_ERR_UNKNOWN, false, "verify mode requires both -ref_det_id and -ref_iter");
        return false;
    }
    if (strcmp(mode, "verify") && ((ref_det_id != 0) || (ref_iter != -1))) {
        psError(PS_ERR_UNKNOWN, false, "master mode cannot have -ref_det_id or -ref_iter set");
        return false;
    }

    PXOPT_LOOKUP_STR(filelevel, config->args, "-filelevel", false, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", false, false);
    PXOPT_LOOKUP_F32(airmass_min, config->args, "-airmass_min", false, false);
    PXOPT_LOOKUP_F32(airmass_max, config->args, "-airmass_max", false, false);
    PXOPT_LOOKUP_F32(exp_time_min, config->args, "-exp_time_min", false, false);
    PXOPT_LOOKUP_F32(exp_time_max, config->args, "-exp_time_max", false, false);
    PXOPT_LOOKUP_F32(ccd_temp_min, config->args, "-ccd_temp_min", false, false);
    PXOPT_LOOKUP_F32(ccd_temp_max, config->args, "-ccd_temp_max", false, false);
    PXOPT_LOOKUP_F64(posang_min, config->args, "-posang_min", false, false);
    PXOPT_LOOKUP_F64(posang_max, config->args, "-posang_max", false, false);
    PXOPT_LOOKUP_F64(solang_min, config->args, "-sun_angle_min", false, false);
    PXOPT_LOOKUP_F64(solang_max, config->args, "-sun_angle_max", false, false);

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    PXOPT_LOOKUP_BOOL(randomSubset, config->args, "-random_subset", false);
    PXOPT_LOOKUP_S32(randomLimit, config->args, "-random_limit", false, false);

    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_TIME(time_begin, config->args, "-time_begin", false, false);
    PXOPT_LOOKUP_TIME(time_end, config->args, "-time_end", false, false);
    PXOPT_LOOKUP_TIME(use_begin, config->args, "-use_begin", false, false);
    PXOPT_LOOKUP_TIME(use_end, config->args, "-use_end", false, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-select_exp_type", "exp_type", "==");
    PXOPT_COPY_STR(config->args, where, "-select_inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-select_telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where, "-select_filter", "filter", "==");
    PXOPT_COPY_STR(config->args, where, "-select_uri", "uri", "==");
    PXOPT_COPY_TIME(config->args, where, "-select_dateobs_begin", "dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-select_dateobs_end", "dateobs", "<=");
    PXOPT_COPY_F32(config->args, where, "-select_airmass_min", "airmass", ">=");
    PXOPT_COPY_F32(config->args, where, "-select_airmass_max", "airmass", "<=");
    PXOPT_COPY_F32(config->args, where, "-select_sat_pixel_frac_max", "sat_pixel_frac", "<=");
    PXOPT_COPY_F32(config->args, where, "-select_sat_pixel_frac_min", "sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args, where, "-select_exp_time_min", "exp_time", ">=");
    PXOPT_COPY_F32(config->args, where, "-select_exp_time_max", "exp_time", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_ccd_temp_min", "ccd_temp", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_ccd_temp_max", "ccd_temp", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_pon_time_min", "pon_time", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_pon_time_max", "pon_time", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_posang_min", "posang", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_posang_max", "posang", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_sun_angle_min", "sun_angle", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_sun_angle_max", "sun_angle", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_sun_alt_min", "sun_alt", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_sun_alt_max", "sun_alt", "<=");

    PXOPT_COPY_F64(config->args, where, "-select_moon_angle_min", "moon_angle", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_moon_angle_max", "moon_angle", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_moon_alt_min", "moon_alt", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_moon_alt_max", "moon_alt", "<=");
    PXOPT_COPY_F64(config->args, where, "-select_moon_phase_min", "moon_phase", ">=");
    PXOPT_COPY_F64(config->args, where, "-select_moon_phase_max", "moon_phase", "<=");

    PXOPT_COPY_STR(config->args, where, "-select_state", "state", "=");
    PXOPT_COPY_STR(config->args, where, "-comment", "comment", "LIKE");

    if (!psListLength(where->list)) {
        psFree(where);
        where = NULL;
    }

    // there is some namespace overlap between the names of the fields we'd
    // like to search by to setup a detrun and the names of the fields we'd
    // like to assign values to so I've separated them but prepending set- to
    // the assigned values

    // search for rawExps with the specified options
    psArray *detrendExps = rawExpSelectRowObjects(config->dbh, where, 0);
    psFree(where);
    // make sure that we found at least one rawExp
    if (!detrendExps) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(detrendExps)) {
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(detrendExps);
        return true;
    }

    if (randomSubset && (randomLimit < detrendExps->n)) {
      // generate a random-valued vector, return an index sorted by the random values
      psVector *randomVector = psVectorAlloc(detrendExps->n, PS_TYPE_F32); // random values
      /*
       * change due to PAP work on random number generator?
       * psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS, 0);
       */
      psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
      for (int i = 0; i < randomVector->n; i++) {
        randomVector->data.F32[i] = psRandomUniform(rng);
      }
      psVector *indexVector = psVectorSortIndex(NULL, randomVector);

      // accept for first n of the sequence
      psArray *subset = psArrayAlloc (randomLimit);
      for (int i = 0; i < randomLimit; i++ ){
        int j = indexVector->data.S32[i];
        subset->data[i] = psMemIncrRefCounter (detrendExps->data[j]);
      }
      psFree (detrendExps);
      detrendExps = subset;
    }

    // check to see if -filelevel was set on the command line
    if (!filelevel) {
        filelevel = psStringCopy(((rawExpRow *)detrendExps->data[0])->filelevel);
    }

    if (pretend) {
        // negative simple so the default is true
        if (!rawExpPrintObjects(stdout, detrendExps, !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(detrendExps);
            return false;
        }
        psFree(detrendExps);
        return true;
    }

    // start a transaction so we don't end up with childlessed det_ids
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(detrendExps);
        return false;
    }

    // the first iteration is always 0
    // XXX det_id
    detRunInsert(config->dbh,
                 0,      // det_id
                 0,      // iteration
                 det_type,
                 mode,
                 "run",  // state
                 filelevel,
                 workdir,
                 camera,
                 telescope,
                 "NA",
                 reduction,
                 filter,
                 airmass_min,
                 airmass_max,
                 exp_time_min,
                 exp_time_max,
                 ccd_temp_min,
                 ccd_temp_max,
                 posang_min,
                 posang_max,
                 registered,
                 time_begin,
                 time_end,
                 use_begin,
                 use_end,
                 solang_min,
                 solang_max,
                 label,
                 ref_det_id,
                 ref_iter
        );
    psS64 det_id = psDBLastInsertID(config->dbh);

    // create new detInputExp row(s) from the rawExp row(s)
    psArray *inputExps = psArrayAllocEmpty(psArrayLength(detrendExps));
    for (long i = 0; i < psArrayLength(detrendExps); i++) {
        detInputExpRow *inputExp = rawDetrenTodetInputExpRow(
            detrendExps->data[i],
            det_id,
            0 // the first iteration is explicitly 0
        );
        psArrayAdd(inputExps, 0, inputExp);
        psFree(inputExp);
    }

    psFree(detrendExps);

    // insert detInputExp objects into the database
    if (!detInputExpInsertObjects(config->dbh, inputExps)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psFree(inputExps);
        return false;
    }
    psFree(inputExps);

    // point of no return for det_id creation
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }


    // print the new det_id
    psArray *detRuns = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", det_id);
        detRuns = psDBSelectRows(config->dbh, "detRun", where, 0);
        psFree(where);
    }
    if (!detRuns) {
        psError(PS_ERR_UNKNOWN, false, "can't find the detRun we just created");
        return false;
    }
    // sanity check results
    if (psArrayLength(detRuns) != 1) {
        psAbort("found more then one detRun matching det_id %" PRId64 " (this should not happen)", det_id);
        return false;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, detRuns, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(detRuns);
        return false;
    }
    psFree(detRuns);

    return true;
}

static bool definebydetrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // XXX this mode is not well-tested: probably need to specify iteration here
    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false); // required
    PXOPT_LOOKUP_STR(det_type, config->args, "-set_det_type", false, false); // optional

    PXOPT_LOOKUP_STR(mode, config->args, "-set_mode", false, false); // optional
    if (!isValidMode(config, mode)) {
        psError(PS_ERR_UNKNOWN, false, "invalid mode");
        return false;
    }

    // get -ref_det_id and -ref_iter : required for 'verify' mode / disallowed otherwise
    PXOPT_LOOKUP_S64(ref_det_id, config->args, "-ref_det_id", false, false);
    PXOPT_LOOKUP_S32(ref_iter, config->args, "-ref_iter", false, false);
    if (!strcmp(mode, "verify") && ((ref_det_id == 0) || (ref_iter == -1))) {
        psError(PS_ERR_UNKNOWN, false, "verify mode requires both -ref_det_id and -ref_iter");
        return false;
    }
    if (strcmp(mode, "verify") && ((ref_det_id != 0) || (ref_iter != -1))) {
        psError(PS_ERR_UNKNOWN, false, "master mode cannot have -ref_det_id or -ref_iter set");
        return false;
    }

    // the new detRun may have different values for these limits:
    PXOPT_LOOKUP_STR(exp_type, config->args, "-set_exp_type", false, false);
    PXOPT_LOOKUP_STR(filelevel, config->args, "-set_filelevel", false, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-set_workdir", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-set_filter", false, false);
    PXOPT_LOOKUP_F32(airmass_min, config->args, "-set_airmass_min", false, false);
    PXOPT_LOOKUP_F32(airmass_max, config->args, "-set_airmass_max", false, false);
    PXOPT_LOOKUP_F32(exp_time_min, config->args, "-set_exp_time_min", false, false);
    PXOPT_LOOKUP_F32(exp_time_max, config->args, "-set_exp_time_max", false, false);
    PXOPT_LOOKUP_F64(ccd_temp_min, config->args, "-set_ccd_temp_min", false, false);
    PXOPT_LOOKUP_F64(ccd_temp_max, config->args, "-set_ccd_temp_max", false, false);
    PXOPT_LOOKUP_F64(posang_min, config->args, "-set_posang_min", false, false);
    PXOPT_LOOKUP_F64(posang_max, config->args, "-set_posang_max", false, false);
    PXOPT_LOOKUP_F64(solang_min, config->args, "-set_sun_angle_min", false, false);
    PXOPT_LOOKUP_F64(solang_max, config->args, "-set_sun_angle_max", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-set_registered", false, false);
    PXOPT_LOOKUP_TIME(time_begin, config->args, "-set_time_begin", false, false);
    PXOPT_LOOKUP_TIME(time_end, config->args, "-set_time_end", false, false);
    PXOPT_LOOKUP_TIME(use_begin, config->args, "-set_use_begin", false, false);
    PXOPT_LOOKUP_TIME(use_end, config->args, "-set_use_end", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-set_reduction", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-set_label", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false); // optional

    // reference iteration to use (otherwise last iteration)
    PXOPT_LOOKUP_S32(iteration, config->args, "-iteration", false, false); // for specifying input exp restrictions

    // lookup the detRun that we will be basing this one on
    psArray *detRuns = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", det_id);
        detRuns = detRunSelectRowObjects(config->dbh, where, 0);
        psFree(where);
    }
    if (!detRuns) {
        psError(PS_ERR_UNKNOWN, false, "database error: no matching det_id found");
        return false;
    }

    // sanity check the result... we should have only found one det_id
    if (psArrayLength(detRuns) != 1) {
        psAbort("found more then one detRun matching det_id %" PRId64 " (this should not happen)", det_id);
    }

    // pull the detRun object out the result array
    detRunRow *detRun = psMemIncrRefCounter(detRuns->data[0]);

    // discard the resultarray
    psFree(detRuns);

    if (iteration < 0) {
        iteration = detRun->iteration;
    }

    detRun->det_id = 0; // set the det_id to 0/NULL so the database can assign it
    detRun->iteration = 0; // reset the iteration to 0

    detRun->ref_det_id = ref_det_id; // not inherited: may only be set for 'verify' run
    detRun->ref_iter = ref_iter;  // not inherited: may only be set for 'verify' run

    // reset the state to "run"
    psFree(detRun->state);
    detRun->state = psStringCopy("run");

    // walk through the optional values and update the detRun as required.  Otherwise, these
    // value are inherited from the specified detRun
    if (det_type) {
        psFree(detRun->det_type);
        detRun->det_type = psStringCopy(det_type);
    }

    if (mode) {
        psFree(detRun->mode);
        detRun->mode = psStringCopy(mode);
    }

    if (exp_type) {
        psFree(detRun->exp_type);
        detRun->exp_type = psStringCopy(exp_type);
    }

    if (filelevel) {
        psFree(detRun->filelevel);
        detRun->filelevel = psStringCopy(filelevel);
    }

    if (workdir) {
        psFree(detRun->workdir);
        detRun->workdir = psStringCopy(workdir);
    }

    if (filter) {
        psFree(detRun->filter);
        detRun->filter = psStringCopy(filter);
    }

    if (!isnan(airmass_min)) {
        detRun->airmass_min = airmass_min;
    }

    if (!isnan(airmass_max)) {
        detRun->airmass_max = airmass_max;
    }

    if (!isnan(exp_time_min)) {
        detRun->exp_time_min = exp_time_min;
    }

    if (!isnan(exp_time_max)) {
        detRun->exp_time_max = exp_time_max;
    }

    if (!isnan(ccd_temp_min)) {
        detRun->ccd_temp_min = ccd_temp_min;
    }

    if (!isnan(ccd_temp_max)) {
        detRun->ccd_temp_max = ccd_temp_max;
    }

    if (!isnan(posang_min)) {
        detRun->posang_min = posang_min;
    }

    if (!isnan(posang_max)) {
        detRun->posang_max = posang_max;
    }

    if (!isnan(solang_min)) {
        detRun->solang_min = solang_min;
    }

    if (!isnan(solang_max)) {
        detRun->solang_max = solang_max;
    }

    if (label) {
        psFree(detRun->label);
        detRun->label = label;
    }

    if (registered) {
        psFree(detRun->registered);
        detRun->registered = psMemIncrRefCounter(registered);
    }

    if (time_begin) {
        psFree(detRun->time_begin);
        detRun->time_begin = psMemIncrRefCounter(time_begin);
    }

    if (time_end) {
        psFree(detRun->time_end);
        detRun->time_end = psMemIncrRefCounter(time_end);
    }

    if (use_begin) {
        psFree(detRun->use_begin);
        detRun->use_begin = psMemIncrRefCounter(use_begin);
    }

    if (use_end) {
        psFree(detRun->use_end);
        detRun->use_end = psMemIncrRefCounter(use_end);
    }

    if (reduction) {
        psFree(detRun->reduction);
        detRun->reduction = psStringCopy(reduction);
    }

    // create a metadata to restrict detInputExp's be in in the specified range

    psMetadata *input_filter = psMetadataAlloc();

    // additional restriction on the detInputExp's to be selected
    PXOPT_LOOKUP_TIME(input_begin, config->args, "-set_input_begin", false, false);
    if (input_begin) {
      PXOPT_COPY_TIME(config->args, input_filter, "-set_input_begin", "dateobs", ">=");
    }
    PXOPT_LOOKUP_TIME(input_end, config->args, "-set_input_end", false, false);
    if (input_end) {
      PXOPT_COPY_TIME(config->args, input_filter, "-set_input_end", "dateobs", "<");
    }
    PXOPT_LOOKUP_BOOL(only_accepted, config->args, "-only_accepted", false); // optional

    // start a transaction so we don't end up with childlessed det_ids
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(input_filter);
        psFree(detRun);
        return false;
    }

    if (!detRunInsertObject(config->dbh, detRun)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psFree(input_filter);
        psFree(detRun);
        return false;
    }
    psFree(detRun);

    // get the det_id
    psS64 newDet_id = psDBLastInsertID(config->dbh);

    psString query = pxDataGet("dettool_definebydetrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (input_filter->list->n) {
        psString whereClause = psDBGenerateWhereConditionSQL(input_filter, "rawExp");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree(input_filter);

    if (only_accepted) {
        psStringAppend(&query, " AND accept = 1");
    }

    if (!p_psDBRunQueryF(config->dbh, query, (long long) newDet_id, (long long) det_id, iteration)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);

    // point of no return for det_id creation
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // print the new det_id
    psArray *newDetRuns = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", newDet_id);
        newDetRuns = psDBSelectRows(config->dbh, "detRun", where, 0);
        psFree(where);
    }
    if (!newDetRuns) {
        psError(PS_ERR_UNKNOWN, false, "can't find the detRun we just created");
        return false;
    }
    // sanity check the result... we should have only found one det_id
    if (psArrayLength(newDetRuns) != 1) {
        psAbort("found more then one detRun matching det_id %" PRId64 " (this should not happen)", newDet_id);
        return false;                   // unreachable
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, newDetRuns, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(newDetRuns);
        return false;
    }
    psFree(newDetRuns);

    return true;
}

static bool runsMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-det_type", "det_type", "==");
    PXOPT_COPY_S64(config->args, where, "-det_id",   "det_id", "==");

    PXOPT_COPY_STR(config->args, where, "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args, where, "-mode", "mode", "==");
    PXOPT_COPY_STR(config->args, where, "-telescope", "telescope", "==");
    PXOPT_COPY_STR(config->args, where, "-filter", "filter", "==");

    // airmass_min  < airmass  < airmass_max
    PXOPT_COPY_F32(config->args, where, "-airmass", "airmass_min", "<=");
    PXOPT_COPY_F32(config->args, where, "-airmass", "airmass_max", ">=");

    // exp_time_min < exp_time < exp_time_max
    PXOPT_COPY_F32(config->args, where, "-exp_time", "exp_time_min", "<=");
    PXOPT_COPY_F32(config->args, where, "-exp_time", "exp_time_max", ">=");

    // ccd_temp_min < ccd_temp < ccd_temp_max
    PXOPT_COPY_F32(config->args, where, "-ccd_temp", "ccd_temp_min", "<=");
    PXOPT_COPY_F32(config->args, where, "-ccd_temp", "ccd_temp_max", ">=");

    PXOPT_COPY_F64(config->args, where, "-posang", "posang_min", "<=");
    PXOPT_COPY_F64(config->args, where, "-posang", "posang_max", ">=");

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(active, config->args, "-active", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    psString query = pxDataGet("dettool_runs.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list) && active) {
        psString whereClause = psDBGenerateWhereSQL(where, "detRun");
        psStringAppend(&query, " %s AND (detRun.state = 'run' OR detRun.state = 'stop' OR detRun.state = 'register')", whereClause);
        psFree(whereClause);
    }
    if (psListLength(where->list) && !active) {
        psString whereClause = psDBGenerateWhereSQL(where, "detRun");
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }
    if (!psListLength(where->list) && active) {
        psStringAppend(&query, " WHERE (detRun.state = 'run' OR detRun.state = 'stop' OR detRun.state = 'register')");
    }
    psFree (where);

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

    psArray *runs = p_psDBFetchResult(config->dbh);
    if (!runs) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!psArrayLength(runs)) {
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(runs);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, runs, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(runs);
        return false;
    }

    psFree(runs);

    return true;
}

static bool childlessrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_STR(config->args, where, "-det_type", "det_type", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // find the exp_id of all the exposures that we want to queue up.
    psString query = pxDataGet("dettool_childlessrun.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "detRun");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree (where);

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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static detInputExpRow *rawDetrenTodetInputExpRow(rawExpRow *rawExp, psS64 det_id, psS32 iteration)
{
    PS_ASSERT_PTR_NON_NULL(rawExp, NULL);

    return detInputExpRowAlloc(
        det_id,
        iteration,
        rawExp->exp_id,
        true            // use
    );
}

static bool inputMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where, "-det_id",    "det_id", "==");
    PXOPT_COPY_S32(config->args, where, "-iteration", "iteration", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",    "exp_id", "==");

    psString query = pxDataGet("dettool_input.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereSQL(where, "detInputExp");
        psStringAppend(&query, " %s", whereClause);
        psFree(whereClause);
    }
    psFree (where);

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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "detInputExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool rawMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psMetadata *where = psMetadataAlloc();

    psString query = pxDataGet("dettool_raw.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "rawImfile");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    }
    psFree (where);

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
        psTrace("dettool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "rawImfile", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool updatedetrunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    bool updating = false;
    long rows = 0;
    psMetadata *where = psMetadataAlloc();
    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false); // required

    PXOPT_LOOKUP_BOOL(again, config->args, "-again", false);
    PXOPT_LOOKUP_STR(state, config->args, "-state", false, false);

    PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");

    psMetadata *values = psMetadataAlloc();
    PXOPT_LOOKUP_TIME(time_begin, config->args, "-set_time_begin", false, false);
    if (time_begin) {
      updating = true;
      PXOPT_COPY_TIME(config->args, values,       "-set_time_begin", "time_begin", "==");
    }
    PXOPT_LOOKUP_TIME(time_end,   config->args, "-set_time_end",   false, false);
    if (time_end) {
      updating = true;
      PXOPT_COPY_TIME(config->args,   values,       "-set_time_end",   "time_end", "==");
    }    
    PXOPT_LOOKUP_TIME(use_begin,  config->args, "-set_use_begin",  false, false);
    if (use_begin) {
      updating = true;
      PXOPT_COPY_TIME(config->args,  values,       "-set_use_begin",  "use_begin", "==");
    }
    PXOPT_LOOKUP_TIME(use_end,    config->args, "-set_use_end",    false, false);
    if (use_end) {
      updating = true;
      PXOPT_COPY_TIME(config->args,    values,       "-set_use_end",    "use_end", "==");
    }
    if (state) {
      updating = true;
      if (!isValidDetRunState (state)) return false;
      PXOPT_COPY_STR(config->args,     values,       "-state",        "state", "==");
    }

    PXOPT_LOOKUP_STR(det_type, config->args, "-set_det_type", false, false);
    if (det_type) {
      updating = true;
      PXOPT_COPY_STR(config->args,     values,       "-set_det_type", "det_type", "==");
    }


    // either -rerun or -state must be specified
    if (!(again || updating)) {
        psError(PS_ERR_UNKNOWN, true, "either -again or update parameters must be specified");
        return false;
    }
    if (again && updating) {
        psError(PS_ERR_UNKNOWN, true, "-again and update parameters are exclusive");
        return false;
    }

    if (updating) {
      rows = psDBUpdateRows(config->dbh,"detRun",where,values);
      if (rows) {
	return(true);
      }
      else {
	return(false);
      }
    }

    // else
    // -again
    if (!startNewIteration(config, det_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to start new iteration");
        return false;
    }

    return true;
}

// used by updatedetrunMode
bool startNewIteration(pxConfig *config, psS64 det_id)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psString query = pxDataGet("dettool_start_new_iteration.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // restrict the update only to the specified det_id
    psStringAppend(&query, " WHERE det_id = %" PRId64 , det_id);

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
        psError(PS_ERR_UNKNOWN, false, "det_id %" PRId64 " not found", det_id);
        psFree(output);
        return false;
    }

    // start a transaction so we don't end up with an incremented iteration
    // count but no detInputExps
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // up the detRuns iteration count for for the single det_id we are
    // operating on
    // XXX this will have to changed in order to support multiple det_ids with
    // a single invocation of this functions
    psS32 newIteration = incrementIteration(config, det_id);
    if (!newIteration) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psFree(output);
        return false;
    }

    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *row = output->data[i];
        bool status = false;
        psS64 exp_id = psMetadataLookupS64(&status, row, "exp_id");
        if (!status) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for exp_id");
            psFree(output);
            return false;
        }
        bool accept = psMetadataLookupBool(&status, row, "accept");
        if (!status) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to lookup value for accept");
            psFree(output);
            return false;
        }

        // detResidExp.include is used to set detInputExp.include
        if (!detInputExpInsert(
                    config->dbh,
                    det_id,
                    newIteration,
                    exp_id,
                    accept
                )
        ) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    // point of no return for det_id creation
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool rerunMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // det_id is required
    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false);

    // we have to support multipe exp_ids
    psMetadataItem *item = psMetadataLookup(config->args, "-exp_id");
    if (!item) {
        // this shouldn't actually happen when using psArgs
        psError(PS_ERR_UNKNOWN, true, "-exp_id is required");
        return false;
    }

    psList *exp_id_list = item->data.list;
    psMetadata *where = psMetadataAlloc();

    // make sure that -exp_id was parsed correctly
    if (item->type == PS_DATA_METADATA_MULTI) {
        psListIterator *iter = psListIteratorAlloc(item->data.list, 0, false);
        psMetadataItem *mItem = NULL;
        while ((mItem = psListGetAndIncrement(iter))) {
            psS64 exp_id = mItem->data.S64;
            if (!psMetadataAddS64(where, PS_LIST_TAIL, "exp_id", PS_META_DUPLICATE_OK, "==", exp_id)) {
                psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
                psFree(iter);
                psFree(where);
                return false;
            }
        }
        psFree(iter);
    }
    if (item->type == PS_DATA_S64) {
        psS64 exp_id = item->data.S64;
        if (!psMetadataAddS64(where, PS_LIST_TAIL, "exp_id", PS_META_DUPLICATE_OK, "==", exp_id)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
            psFree(where);
            return false;
        }
    }

# if (0)
    // make sure that -exp_id was parsed correctly
    // XXX this can be removed someday
    if (item->type == PS_DATA_METADATA_MULTI) {
        psListIterator *iter = psListIteratorAlloc(item->data.list, 0, false);
        psMetadataItem *mItem = NULL;
        while ((mItem = psListGetAndIncrement(iter))) {
            psString exp_id = mItem->data.V;
            // if exp_id is NULL then it means that -exp_id has not been
            // specified
            if (!exp_id) {
                psError(PS_ERR_UNKNOWN, true,
                        "at least one -exp_id is required");
                psFree(where);
                return false;
            }

            if (!psMetadataAddStr(where, PS_LIST_TAIL, "exp_id",
                        PS_META_DUPLICATE_OK, "==", exp_id)) {
                psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
                psFree(iter);
                psFree(where);
                return false;
            }
        }
        psFree(iter);
    } else {
        psAbort("-exp_id was not parsed correctly (this should not happen");
    }
# endif

    // check that the specified exp_ids actually exist in the iteration zero
    // detInputExp set

    // add the det_id & iteration == 0 to the where clause
    if (!psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", det_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to add item det_id");
        psFree(where);
        return false;
    }
    if (!psMetadataAddS32(where, PS_LIST_TAIL, "iteration", 0, "==", 0)) {
        psError(PS_ERR_UNKNOWN, false, "failed to add item iteration");
        psFree(where);
        return false;
    }

    psArray *detrendExps = detInputExpSelectRowObjects(config->dbh, where, 0);
    psFree(where);
    if (!detrendExps) {
        psError(PS_ERR_UNKNOWN, false, "no rawExp rows found");
        psFree(where);
        return false;
    }

    // build a hash for the valid exp_ids
    psHash *valid_exp_ids = psHashAlloc(psArrayLength(detrendExps));
    for (long i = 0; i < psArrayLength(detrendExps); i++) {
        psString exp_idStr = psDBIntToString(((detInputExpRow *)detrendExps->data[i])->exp_id);
        psHashAdd(valid_exp_ids, exp_idStr, detrendExps->data[i]);
        psFree(exp_idStr);
    }
    psFree(detrendExps);

    // start a transaction so we don't end up with an incremented iteration
    // count but no detInputExps
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(valid_exp_ids);
        return false;
    }

    // up the detRuns iteration count
    psS32 newIteration = incrementIteration(config, det_id);
    if (!newIteration) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psFree(valid_exp_ids);
        return false;
    }

    // check exp_ids and build up an array of new detInputExp rows at the same
    // time
    psListIterator *iter = psListIteratorAlloc(exp_id_list, 0, false);
    psMetadataItem *mItem = NULL;
    psArray *newInputExps = psArrayAllocEmpty(psListLength(exp_id_list));
    while ((mItem = psListGetAndIncrement(iter))) {
      psString exp_idStr = psDBIntToString(mItem->data.S64);
      detInputExpRow *inputExp = psHashLookup(valid_exp_ids, exp_idStr);
      if (!inputExp) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            // invalid exp_id
            psError(PS_ERR_UNKNOWN, false, "exp_id %" PRId64 " is invalid for det_id %" PRId64, mItem->data.S64, det_id);
            psFree(iter);
            psFree(valid_exp_ids);
            return false;
        }
        detInputExpRow *newInputExp = detInputExpRowAlloc(
            det_id,
            newIteration,
            inputExp->exp_id,
            true   // use
        );
        psArrayAdd(newInputExps, 0, newInputExp);
        psFree(newInputExp);
    }
    psFree(iter);
    psFree(valid_exp_ids);

    for (long i = 0; i < psArrayLength(newInputExps); i++) {
        if (!detInputExpInsertObject(config->dbh, newInputExps->data[i])) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psFree(newInputExps);
            return false;
        }
    }
    psFree(newInputExps);

    // point of no return for det_id creation
    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool register_detrendMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(det_type, config->args, "-det_type", true, false); // required
    PXOPT_LOOKUP_STR(filelevel, config->args, "-filelevel", true, false); // required
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", false, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", false, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", false, false);
    PXOPT_LOOKUP_STR(exp_type, config->args, "-exp_type", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", false, false);
    PXOPT_LOOKUP_F32(airmass_min, config->args, "-airmass_min", false, false);
    PXOPT_LOOKUP_F32(airmass_max, config->args, "-airmass_max", false, false);
    PXOPT_LOOKUP_F32(exp_time_min, config->args, "-exp_time_min", false, false);
    PXOPT_LOOKUP_F32(exp_time_max, config->args, "-exp_time_max", false, false);
    PXOPT_LOOKUP_F32(ccd_temp_min, config->args, "-ccd_temp_min", false, false);
    PXOPT_LOOKUP_F32(ccd_temp_max, config->args, "-ccd_temp_max", false, false);
    PXOPT_LOOKUP_F64(posang_min, config->args, "-posang_min", false, false);
    PXOPT_LOOKUP_F64(posang_max, config->args, "-posang_max", false, false);
    PXOPT_LOOKUP_F64(solang_min, config->args, "-sun_angle_min", false, false);
    PXOPT_LOOKUP_F64(solang_max, config->args, "-sun_angle_max", false, false);
    PXOPT_LOOKUP_TIME(registered, config->args, "-registered", false, false);
    PXOPT_LOOKUP_TIME(time_begin, config->args, "-time_begin", false, false);
    PXOPT_LOOKUP_TIME(time_end, config->args, "-time_end", false, false);
    PXOPT_LOOKUP_TIME(use_begin, config->args, "-use_begin", false, false);
    PXOPT_LOOKUP_TIME(use_end, config->args, "-use_end", false, false);
    PXOPT_LOOKUP_S64(ref_det_id, config->args, "-ref_det_id", false, false);
    PXOPT_LOOKUP_S32(ref_iter, config->args, "-ref_iter", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!detRunInsert(config->dbh,
                      0,            // det_id
                      0,            // the iteration is fixed at 0
                      det_type,
                      "register",   // mode is required to be "register"
                      "register",   // state
                      filelevel,
                      workdir,
                      camera,
                      telescope,
                      exp_type,
                      NULL,
                      filter,
                      airmass_min,
                      airmass_max,
                      exp_time_min,
                      exp_time_max,
                      ccd_temp_min,
                      ccd_temp_max,
                      posang_min,
                      posang_max,
                      registered,
                      time_begin,
                      time_end,
                      use_begin,
                      use_end,
                      solang_min,
                      solang_max,
                      label,
                      ref_det_id,
                      ref_iter
            )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        return false;
    }

    // print the new detRun
    psS64 det_id = psDBLastInsertID(config->dbh);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psArray *detRuns = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", det_id);
        detRuns = psDBSelectRows(config->dbh, "detRun", where, 0);
        psFree(where);
    }
    if (!detRuns) {
        psError(PS_ERR_UNKNOWN, false, "can't find the detRun we just created");
        return false;
    }
    // sanity check results
    if (psArrayLength(detRuns) != 1) {
        psAbort("found more then one detRun matching det_id %" PRId64 "(this should not happen)", det_id);
        return false;
    }

    // negative simple so the default is true
    if (!ippdbPrintMetadatas(stdout, detRuns, "detRun", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(detRuns);
        return false;
    }
    psFree(detRuns);

    return true;
}

// NOTE : this function is also used by addcorrectimfileMode
bool register_detrend_imfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true, false); // required
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false); // required
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false); // required
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F64(user_1, config->args, "-user_1", false, false);
    PXOPT_LOOKUP_F64(user_2, config->args, "-user_2", false, false);
    PXOPT_LOOKUP_F64(user_3, config->args, "-user_3", false, false);
    PXOPT_LOOKUP_F64(user_4, config->args, "-user_4", false, false);
    PXOPT_LOOKUP_F64(user_5, config->args, "-user_5", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-path_base", false, false);

    if (!detRegisteredImfileInsert(config->dbh,
                                   det_id,
                                   0,  // the iteration is fixed at 0
                                   class_id,
                                   uri,
                                   bg,
                                   bg_stdev,
                                   bg_mean_stdev,
                                   user_1,
                                   user_2,
                                   user_3,
                                   user_4,
                                   user_5,
                                   path_base,
                                   "full",
                                   0       // fault code
            )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static psS32 incrementIteration(pxConfig *config, psS64 det_id)
{
    // this function returns zero on error
    PS_ASSERT_PTR_NON_NULL(config, 0);

    char *query = "UPDATE detRun SET iteration = iteration + 1 WHERE det_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, det_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to increment iteration for det_id %" PRId64, det_id);
        return 0;
    }

    psMetadata *where = psMetadataAlloc();
    if (!psMetadataAddS64(where, PS_LIST_TAIL, "det_id", 0, "==", det_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to add item det_id");
        psFree(where);
        return 0;
    }

    psArray *detRuns = detRunSelectRowObjects(config->dbh, where, 0);
    psFree(where);
    if (!detRuns) {
        psError(PS_ERR_UNKNOWN, false, "no detRun rows found");
        return 0;
    }

    // sanity check the database
    if (psArrayLength(detRuns) != 1) {
        // this should no happen
        psAbort(                "database query return too many rows (this should not happen");
    }

    psS32 newIteration = ((detRunRow *)detRuns->data[0])->iteration;
    psFree(detRuns);

    return newIteration;
}

bool setDetRunState(pxConfig *config, psS64 det_id, const char *state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(state, false);

    if (!isValidDetRunState (state)) return false;

    char *query = "UPDATE detRun SET state = '%s' WHERE det_id = %" PRId64;
    if (!p_psDBRunQueryF(config->dbh, query, state, det_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to change state for det_id %" PRId64, det_id);
        return false;
    }

    return true;
}

// the detRun states are a superset of the data states below
bool isValidDetRunState (const char *state) {

    // check that state is a valid string value
    if (!strcmp(state, "run")) return true;
    if (!strcmp(state, "stop")) return true;
    if (!strcmp(state, "drop")) return true;
    if (!strcmp(state, "wait")) return true;
    if (!strcmp(state, "test")) return true;
    if (!strcmp(state, "ignore")) return true;
    if (!strcmp(state, "register")) return true;

    psError(PS_ERR_UNKNOWN, true, "invalid detRun state: %s", state);
    return false;
}

bool isValidDataState (const char *data_state) {

    // check that state is a valid string value
    if (!strcmp(data_state, "run")) return true;
    if (!strcmp(data_state, "stop")) return true;
    if (!strcmp(data_state, "drop")) return true;
    if (!strcmp(data_state, "register")) return true;
    // These are valid data states, and are necessary for the cleanup to work correctly.
    if (!strcmp(data_state, "full")) return true;
    if (!strcmp(data_state, "goto_cleaned")) return true;
    if (!strcmp(data_state, "goto_scrubbed")) return true;
    if (!strcmp(data_state, "goto_purged")) return true;
    if (!strcmp(data_state, "cleaned")) return true;
    if (!strcmp(data_state, "scrubbed")) return true;
    if (!strcmp(data_state, "purged")) return true;
    if (!strcmp(data_state, "error_cleaned")) return true;
    if (!strcmp(data_state, "error_scrubbed")) return true;
    if (!strcmp(data_state, "error_purged")) return true;

    psError(PS_ERR_UNKNOWN, true, "invalid data state: %s", data_state);
    return false;
}

bool isValidMode(pxConfig *config, const char *mode)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(mode, false);

    // check that state is a valid string value
    if (!strcmp(mode, "master")) return true;
    if (!strcmp(mode, "verify")) return true;

    psError(PS_ERR_UNKNOWN, false, "invalid detRun mode: %s", mode);
    return false;
}


bool setProcessedImfileDataState(pxConfig *config, psMetadata *where, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(where, false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    psString query = psStringCopy("UPDATE detProcessedImfile SET data_state = '%s'");
    if (where && psListLength(where->list) > 0) {
      psString whereClause = psDBGenerateWhereSQL(where,NULL);
      psStringAppend(&query," %s",whereClause);
      psFree(whereClause);
    } else {
      psError(PS_ERR_UNKNOWN, true, "search parameters are required");
      return(false);
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state)) {
      psFree(query);
      psError(PS_ERR_UNKNOWN, false, "database error");
      return(false);
    }
    psFree(query);
    return(true);
}

bool setProcessedExpDataState(pxConfig *config, psMetadata *where, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(where, false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    psString query = psStringCopy("UPDATE detProcessedExp SET data_state = '%s'");
    if (where && psListLength(where->list) > 0) {
      psString whereClause = psDBGenerateWhereSQL(where,NULL);
      psStringAppend(&query," %s",whereClause);
      psFree(whereClause);
    } else {
      psError(PS_ERR_UNKNOWN, true, "search parameters are required");
      return(false);
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state)) {
      psFree(query);
      psError(PS_ERR_UNKNOWN, false, "database error");
      return(false);
    }
    psFree(query);
    return(true);
}

bool setResidImfileDataState(pxConfig *config, psMetadata *where, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(where,false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    psString query = psStringCopy("UPDATE detResidImfile SET data_state = '%s'");
    if (where && psListLength(where->list) > 0) {
      psString whereClause = psDBGenerateWhereSQL(where,NULL);
      psStringAppend(&query," %s",whereClause);
      psFree(whereClause);
    } else {
      psError(PS_ERR_UNKNOWN, true, "search parameters are required");
      return(false);
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state)) {
      psFree(query);
      psError(PS_ERR_UNKNOWN, false, "database error");
      return(false);
    }
    psFree(query);
    return(true);
}
// This function apparently not used anymore.
bool setResidExpDataState(pxConfig *config, psMetadata *where, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(where,false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    psString query = psStringCopy("UPDATE detResidExp SET data_state = '%s'");
    if (where && psListLength(where->list) > 0) {
      psString whereClause = psDBGenerateWhereSQL(where,NULL);
      psStringAppend(&query," %s",whereClause);
      psFree(whereClause);
    } else {
      psError(PS_ERR_UNKNOWN, true, "search parameters are required");
      return(false);
    }

    if (!p_psDBRunQueryF(config->dbh, query, data_state)) {
      psFree(query);
      psError(PS_ERR_UNKNOWN, false, "database error");
      return(false);
    }
    psFree(query);
    return(true);
}

// Not yet updated for cleaning
bool setStackedImfileDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *class_id, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(class_id, false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    char *query = "UPDATE detStackedImfile SET data_state = '%s'"
        " WHERE det_id = %" PRId64
        " AND iteration = %" PRId32
        " AND class_id = '%s'";
    if (!p_psDBRunQueryF(config->dbh, query, data_state, det_id, iteration, class_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for det_id %" PRId64 ", iteration %" PRId32 "class_id %s",
                det_id, iteration, class_id);
        return false;
    }

    return true;
}

bool setNormStatImfileDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *class_id, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(class_id, false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    char *query = "UPDATE detNormalizedStatImfile SET data_state = '%s'"
        " WHERE det_id = %" PRId64
        " AND iteration = %" PRId32
        " AND class_id = '%s'";
/*     fprintf(stderr,"DETTOOL SAYS: %s\n",query); */
    if (!p_psDBRunQueryF(config->dbh, query, data_state, det_id, iteration,class_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for det_id %" PRId64 ", iteration %" PRId32,
                det_id, iteration);
        return false;
    }

    return true;
}

bool setNormImfileDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *class_id, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(class_id, false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    char *query = "UPDATE detNormalizedImfile SET data_state = '%s'"
        " WHERE det_id = %" PRId64
        " AND iteration = %" PRId32
        " AND class_id = '%s'";
    if (!p_psDBRunQueryF(config->dbh, query, data_state, det_id, iteration, class_id)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for det_id %" PRId64 ", iteration %" PRId32 " class %s",
                det_id, iteration, class_id);
        return false;
    }

    return true;
}

bool setNormExpDataState(pxConfig *config, psS64 det_id, psS32 iteration, const char *data_state)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(data_state, false);

    if (!isValidDataState (data_state)) return false;

    char *query = "UPDATE detNormalizedExp SET data_state = '%s'"
        " WHERE det_id = %" PRId64
        " AND iteration = %" PRId32;
    if (!p_psDBRunQueryF(config->dbh, query, data_state, det_id, iteration)) {
        psError(PS_ERR_UNKNOWN, false,
                "failed to change state for det_id %" PRId64 ", iteration %" PRId32,
                det_id, iteration);
        return false;
    }

    return true;
}
// End not yet modified for cleanup.

bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 12;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // replaced below PXOPT_LOOKUP_S64(det_id, config->args, "-det_id", true,  false);
  PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);

  FILE *f = fopen (outfile, "w");
  if (f == NULL) {
    psError(PS_ERR_UNKNOWN, false, "failed to open output file");
    return false;
  }

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-det_id", "det_id", "==");

  ExportTable tables [] = {
    {"detRun", "dettool_export_run.sql"},
    {"detInputExp", "dettool_export_input_exp.sql"},
    {"detNormalizedExp", "dettool_export_normalized_exp.sql"},
    {"detNormalizedImfile", "dettool_export_normalized_imfile.sql"},
    {"detNormalizedStatImfile", "dettool_export_normalized_stat_imfile.sql"},
    {"detProcessedExp", "dettool_export_processed_exp.sql"},
    {"detProcessedImfile", "dettool_export_processed_imfile.sql"},
    {"detRegisteredImfile", "dettool_export_registered_imfile.sql"},
    {"detResidExp", "dettool_export_resid_exp.sql"},
    {"detResidImfile", "dettool_export_resid_imfile.sql"},
    {"detRunSummary", "dettool_export_run_summary.sql"},
    {"detStackedImfile", "dettool_export_stacked_imfile.sql"},
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
      psTrace("chiptool", PS_LOG_INFO, "no rows found");
      psFree(output);
      return true;
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

  int numImportTables = 11;

  char tables[11] [80] = {"detInputExp", "detNormalizedExp",
    "detNormalizedImfile", "detNormalizedStatImfile", "detProcessedExp",
    "detProcessedImfile", "detRegisteredImfile", "detResidExp",
    "detResidImfile", "detRunSummary", "detStackedImfile"};

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);

  psMetadata *input = psMetadataConfigRead (NULL, &nFail, infile, false);

  fprintf (stdout, "---- input ----\n");
  psMetadataPrint (stderr, input, 1);

  psMetadataItem *item = psMetadataLookup (input, "detRun");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  detRunRow *detRun = detRunObjectFromMetadata (entry->data.md);
  detRunInsertObject (config->dbh, detRun);

  // fprintf (stdout, "---- det run ----\n");
  // psMetadataPrint (stderr, entry->data.md, 1);

  for (int i = 0; i < numImportTables; i++) {
    item = psMetadataLookup (input, tables[i]);
    psAssert (item, "entry not in input?");
    psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

    switch (i) {
      case 0:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detInputExpRow *detInputExp = detInputExpObjectFromMetadata (entry->data.md);
          detInputExpInsertObject (config->dbh, detInputExp);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 1:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detNormalizedExpRow *detNormalizedExp = detNormalizedExpObjectFromMetadata (entry->data.md);
          detNormalizedExpInsertObject (config->dbh, detNormalizedExp);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 2:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detNormalizedStatImfileRow *detNormalizedStatImfile = detNormalizedStatImfileObjectFromMetadata (entry->data.md);
          detNormalizedStatImfileInsertObject (config->dbh, detNormalizedStatImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 3:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detProcessedExpRow *detProcessedExp =  detProcessedExpObjectFromMetadata (entry->data.md);
          detProcessedExpInsertObject (config->dbh, detProcessedExp);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 4:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detProcessedImfileRow *detProcessedImfile =  detProcessedImfileObjectFromMetadata (entry->data.md);
          detProcessedImfileInsertObject (config->dbh, detProcessedImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 5:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detRegisteredImfileRow *detRegisteredImfile = detRegisteredImfileObjectFromMetadata (entry->data.md);
          detRegisteredImfileInsertObject (config->dbh, detRegisteredImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 6:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detResidExpRow *detResidExp = detResidExpObjectFromMetadata (entry->data.md);
          detResidExpInsertObject (config->dbh, detResidExp);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 7:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detResidImfileRow *detResidImfile = detResidImfileObjectFromMetadata (entry->data.md);
          detResidImfileInsertObject (config->dbh, detResidImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 8:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detNormalizedImfileRow *detNormalizedImfile = detNormalizedImfileObjectFromMetadata (entry->data.md);
          detNormalizedImfileInsertObject (config->dbh, detNormalizedImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 9:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detRunSummaryRow *detRunSummary = detRunSummaryObjectFromMetadata (entry->data.md);
          detRunSummaryInsertObject (config->dbh, detRunSummary);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;

      case 10:
        for (int i = 0; i < item->data.list->n; i++) {
          entry = psListGet (item->data.list, i);
          assert (entry);
          assert (entry->type == PS_DATA_METADATA);
          detStackedImfileRow *detStackedImfile = detStackedImfileObjectFromMetadata (entry->data.md);
          detStackedImfileInsertObject (config->dbh, detStackedImfile);

          // fprintf (stdout, "---- row %d ----\n", i);
          // psMetadataPrint (stderr, entry->data.md, 1);
        }
        break;
    }
  }

  return true;
}

#if 0
// XXX this function was left in commented as this method may be useful in the
// future
static psArray *validDetInputClassIds(pxConfig *config, const char *det_id)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    // det_id is input as a string because the fact that it is an integer
    // is just a database impliementation detail.
    PS_ASSERT_PTR_NON_NULL(det_id, NULL);

    psArray *rawImfiles = searchInputImfiles(config, det_id);
    if (!rawImfiles) {
        return NULL;
    }

    psArray *valid_class_ids = NULL;
    {
        // All this jumping through hoops is so that we end up with unique
        // values. PP thinks that making multiple passes through this array and
        // deleting matched elements would end up being more expensive then a
        // double sort and stagger scheme. JH thinks it would be cheaper to
        // just do a unqiue sort and delete.  So this is really just a cheap
        // hack to avoid implimenting a unique sort function but at least it's
        // stable.
        psHash *hash = psHashAlloc(psArrayLength(rawImfiles));
        for (long i = 0; i < psArrayLength(rawImfiles); i++) {
            psHashAdd(hash,
                ((rawImfileRow *)rawImfiles->data[i])->class_id,
                ((rawImfileRow *)rawImfiles->data[i])->class_id
            );
        }
        valid_class_ids = psHashToArray(hash);
        psFree(hash);
    }
    psFree(rawImfiles);

    return valid_class_ids;
}

static psArray *searchInputImfiles(pxConfig *config, const char *det_id)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    // det_id is input as a string because the fact that it is an integer
    // is just a database impliementation detail.
    PS_ASSERT_PTR_NON_NULL(det_id, NULL);

    psArray *inputExps = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        if (!psMetadataAddS32(where, PS_LIST_TAIL, "det_id", 0, "==",
                (psS32)atoi(det_id))) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item det_id");
            psFree(where);
            return NULL;
        }
        inputExps = detInputExpSelectRowObjects(config->dbh, where, 0);
        psFree(where);
    }
    if (!inputExps) {
        psError(PS_ERR_UNKNOWN, false, "no detInputExp rows found");
        return NULL;
    }

    // find rawImfiles associated with detInputExps
    psArray *rawImfiles = NULL;
    {
        psMetadata *where = psMetadataAlloc();
        for (long i = 0; i < psArrayLength(inputExps); i++) {
	    detInputExpRow *row = (detInputExpRow *row) inputExps->data[i];
	    if (!psMetadataAddS64(where, PS_LIST_TAIL, "exp_id", PS_META_DUPLICATE_OK, "==", row->exp_id)) {
                psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
                psFree(inputExps);
                psFree(where);
                return NULL;
            }
        }
        psFree(inputExps);
        rawImfiles = rawImfileSelectRowObjects(config->dbh, where, 0);
        // XXX this really should be sorted for uniqueness
        psFree(where);
    }
    if (!rawImfiles) {
        psError(PS_ERR_UNKNOWN, false, "no rawImfile rows found");
        return NULL;
    }

    return rawImfiles;
}
#endif


/*
 * releasetool.c
 *
 * Copyright (C) 2013 IfA University of Hawaii
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
#include "pxspace.h"

#include "releasetool.h"

static bool definesurveyMode(pxConfig *config);
static bool listsurveyMode(pxConfig *config);
static bool definereleaseMode(pxConfig *config);
static bool updatereleaseMode(pxConfig *config);
static bool listreleaseMode(pxConfig *config);
static bool definerelexpMode(pxConfig *config);
static bool tocalibexpMode(pxConfig *config);
static bool updaterelexpMode(pxConfig *config);
static bool listrelexpMode(pxConfig *config);
static bool deleterelexpMode(pxConfig *config);
static bool definerelstackMode(pxConfig *config);
static bool updaterelstackMode(pxConfig *config);
static bool setrelstackcalibratedfromskycalMode(pxConfig *config);
static bool listrelstackMode(pxConfig *config);
static bool deleterelstackMode(pxConfig *config);
static bool summaryMode(pxConfig *config);
static bool definerelgroupMode(pxConfig *config);
static bool pendingrelgroupMode(pxConfig *config);
static bool updaterelgroupMode(pxConfig *config);
static bool listrelgroupMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv) {
    psLibInit(NULL);

    pxConfig *config = releasetoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(RELEASETOOL_MODE_DEFINESURVEY,     definesurveyMode);
        MODECASE(RELEASETOOL_MODE_LISTSURVEY,       listsurveyMode);

        MODECASE(RELEASETOOL_MODE_DEFINERELEASE,    definereleaseMode);
        MODECASE(RELEASETOOL_MODE_UPDATERELEASE,    updatereleaseMode);
        MODECASE(RELEASETOOL_MODE_TOCALIBEXP,       tocalibexpMode);
        MODECASE(RELEASETOOL_MODE_LISTRELEASE,      listreleaseMode);

        MODECASE(RELEASETOOL_MODE_DEFINERELEXP,     definerelexpMode);
        MODECASE(RELEASETOOL_MODE_UPDATERELEXP,     updaterelexpMode);
        MODECASE(RELEASETOOL_MODE_LISTRELEXP,       listrelexpMode);
        MODECASE(RELEASETOOL_MODE_DELETERELEXP,     deleterelexpMode);

        MODECASE(RELEASETOOL_MODE_DEFINERELSTACK,   definerelstackMode);
        MODECASE(RELEASETOOL_MODE_UPDATERELSTACK,   updaterelstackMode);
        MODECASE(RELEASETOOL_MODE_SETRELSTACKCALIBRATEDFROMSKYCAL, setrelstackcalibratedfromskycalMode);
        MODECASE(RELEASETOOL_MODE_LISTRELSTACK,     listrelstackMode);
        MODECASE(RELEASETOOL_MODE_DELETERELSTACK,   deleterelstackMode);
        MODECASE(RELEASETOOL_MODE_SUMMARY,          summaryMode);

        MODECASE(RELEASETOOL_MODE_DEFINERELGROUP,   definerelgroupMode);
        MODECASE(RELEASETOOL_MODE_UPDATERELGROUP,   updaterelgroupMode);
        MODECASE(RELEASETOOL_MODE_PENDINGRELGROUP,  pendingrelgroupMode);
        MODECASE(RELEASETOOL_MODE_LISTRELGROUP,     listrelgroupMode);
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

static bool definesurveyMode(pxConfig *config)
{
    psError(PS_ERR_UNKNOWN, true, "not yet implemented");
    return false;
}

static bool listsurveyMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-surveyName", "surveyName", "LIKE");

    psString query = pxDataGet("releasetool_listsurvey.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, " WHERE %s", whereClause);
        psFree(whereClause);
    } else {
        // carry on The list of surveys is short there is no need to require parameters
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "survey", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool definereleaseMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S32(surveyID,    config->args,   "-set_surveyID", true, false);
    PXOPT_LOOKUP_STR(release_name, config->args,  "-set_release_name", true, false);
    PXOPT_LOOKUP_STR(release_state, config->args, "-set_release_state",  true, false);
    PXOPT_LOOKUP_S32(dataRelease, config->args,   "-set_dataRelease", false, false);
    PXOPT_LOOKUP_S32(priority,    config->args,   "-set_priority", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args,         "-set_dvodb",  false, false);
    PXOPT_LOOKUP_STR(ubercal_file, config->args,  "-set_ubercal_file",  false, false);
    PXOPT_LOOKUP_S32(accessLevelMin,    config->args,   "-set_accessLevelMin", false, false);

    if (!ippReleaseInsert(config->dbh,
        0,      // rel_id (auto_increment)
        surveyID,
        release_name,
        release_state,
        dataRelease,
        priority,
        dvodb,
        ubercal_file,
        accessLevelMin
    )) {
        psError(PS_ERR_UNKNOWN, false, "failed to insert ippRelease");
        return false;
    }

    return true;
}

static bool updatereleaseMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
    PXOPT_LOOKUP_S32(rel_id, config->args,      "-rel_id", false, false);

    if (!release_name && !rel_id) {
        psError(PXTOOLS_ERR_CONFIG, true, "either -release_name or -rel_id is required\n");
        return false;
    }
    PXOPT_LOOKUP_STR(release_state,    config->args, "-set_release_state",  false, false);
    PXOPT_LOOKUP_S32(priority,         config->args, "-set_priority",  false, false);
    PXOPT_LOOKUP_S32(dataRelease,      config->args, "-set_dataRelease",  false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-set_dvodb",  false, false);
    PXOPT_LOOKUP_STR(ubercal_file, config->args, "-set_ubercal_file",  false, false);
    PXOPT_LOOKUP_S32(accessLevelMin,    config->args,   "-set_accessLevelMin", false, false);
    if (!release_state && priority == 0 && dataRelease == -1 && !dvodb && !ubercal_file && accessLevelMin < 0) {
        psError(PXTOOLS_ERR_CONFIG, true, "at least one of -set_release_state -set_priority -set_dvodb -set_ubercal_file and -set_dataRelease is required\n");
        return false;
    }

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S32(config->args, where, "-rel_id",  "rel_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "release_name", "==");

    psString sep = "";
    psString comma = ", ";
    psString query = psStringCopy("UPDATE ippRelease SET");
    if (release_state) {
        psStringAppend(&query, "%s release_state = '%s'", sep, release_state);
        sep = comma;
    }
    if (priority != 0) {
        psStringAppend(&query, "%s priority = %d", sep, priority);
        sep = comma;
    }
    if (dataRelease != -1) {
        psStringAppend(&query, "%s dataRelease = %d", sep, dataRelease);
        sep = comma;
    }
    if (accessLevelMin != -1) {
        psStringAppend(&query, "%s accessLevelMin = %d", sep, accessLevelMin);
        sep = comma;
    }
    if (dvodb) {
        psStringAppend(&query, "%s dvodb = '%s'", sep, dvodb);
        sep = comma;
    }
    if (ubercal_file) {
        psStringAppend(&query, "%s ubercal_file = '%s'", sep, ubercal_file);
        sep = comma;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("releasetool", PS_LOG_INFO, "Updated %" PRIu64 " ippReleases", affected);


    psFree(query);

    return true;
}

static bool listreleaseMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-surveyName",  "surveyName", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-release_name", "release_name", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-release_state", "release_state", "==");
    PXOPT_COPY_S32(config->args, where, "-rel_id",  "rel_id", "==");

        psString query = pxDataGet("releasetool_listrelease.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            return false;
        }

        if (psListLength(where->list)) {
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, " WHERE %s", whereClause);
            psFree(whereClause);
        } else {
            // The list of releases is short there is no need to require parameters
        #ifdef notyet
            psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
            psFree(where);
            return false;
        #endif
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
            psTrace("releasetool", PS_LOG_INFO, "no rows found");
            psFree(output);
            return true;
        }

        if (!ippdbPrintMetadatas(stdout, output, "ippRelease", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }

        psFree(output);

        return true;
    }

static bool definerelexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-label",      "camRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "camRun.data_group", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",     "camRun.cam_id", "==");

    // Insure that at least one of the camRun selectors is supplied
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "at least one of -label, -data_group, or -cam_id is required");
        return false;
    }
    PXOPT_COPY_STR(config->args, where, "-filter",     "rawExp.filter", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",     "rawExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name",   "rawExp.exp_name", "==");

    // insure that at least one of these is supplied to select the release
    PXOPT_LOOKUP_S64(rel_id, config->args,      "-rel_id", false, false);
    PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
    if (!rel_id && !release_name) {
        psError(PXTOOLS_ERR_CONFIG, true, "either -rel_id or -release_name is required");
        return false;
    }

    // limit query to the target release. (Note: this will select nothing if release_name and rel_id don't match)
    // note: we add these after above to insure that search args are supplied
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "==");
    PXOPT_COPY_S32(config->args, where, "-rel_id", "ippRelease.rel_id", "==");


    PXOPT_LOOKUP_STR(state, config->args,     "-set_state", true, false);
    PXOPT_LOOKUP_U32(flags, config->args,     "-set_flags", false, false);
    PXOPT_LOOKUP_F32(user_zpt_obs, config->args,   "-set_zpt_obs", false, false);
    PXOPT_LOOKUP_F32(user_zpt_stdev, config->args, "-set_zpt_stdev", false, false);
    PXOPT_LOOKUP_F32(mcal,     config->args, "-set_mcal",  false, false);
    PXOPT_LOOKUP_S32(ubercal_dist, config->args, "-set_ubercal_dist", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-set_path_base", false, false);
    PXOPT_LOOKUP_S16(fault, config->args,     "-set_fault", false, false);

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args,  "-simple", false);

    // find the parameters of all the exposures that we want to add to the release
    psString query = pxDataGet("releasetool_definerelexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);

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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "chipRun", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    psTime *now = psTimeGetNow(PS_TIME_TAI);
    // loop over our list of  exposures
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status = true;
        psS32 rel_id = psMetadataLookupS32(&status, md, "rel_id");
        psS64 exp_id = psMetadataLookupS64(&status, md, "exp_id");
        psS64 chip_id = psMetadataLookupS64(&status, md, "chip_id");
        psS64 cam_id = psMetadataLookupS64(&status, md, "cam_id");

        // if zpt_obs or zpt_stdev were not supplied use the results from the camRun
        psF32 zpt_obs, zpt_stdev;
        if (!isfinite(user_zpt_obs)) {
            zpt_obs = psMetadataLookupF32(&status, md, "zpt_obs");
        } else {
            zpt_obs = user_zpt_obs;
        }
        if (!isfinite(user_zpt_stdev)) {
            zpt_stdev = psMetadataLookupF32(&status, md, "zpt_stdev");
        } else {
            zpt_stdev = user_zpt_stdev;
        }

        if (!relExpInsert(config->dbh,
            0,          // relexp_id (auto increment)
            rel_id,
            exp_id,
            chip_id,
            cam_id,
            0,          // group_id
            state,
            flags,
            zpt_obs,
            zpt_stdev,
            mcal,
            ubercal_dist,
            path_base,
            fault,
            now,       // registered
            now        // time_stamp
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to insert relExp for cam_id %" PRId64, cam_id);
            psFree(output);
            psFree(now);
            return false;
        }
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool updaterelexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
    PXOPT_LOOKUP_S32(rel_id, config->args,      "-rel_id", false, false);
    PXOPT_LOOKUP_S32(relexp_id, config->args,   "-relexp_id", false, false);

    if (!relexp_id && !release_name && !rel_id) {
        psError(PXTOOLS_ERR_CONFIG, true, "at least one of -relexp_id, -release_name, and -rel_id is required\n");
        return false;
    }

    PXOPT_LOOKUP_STR(state,    config->args, "-set_state",  false, false);
    PXOPT_LOOKUP_U32(flags,    config->args, "-set_flags",  false, false);
    PXOPT_LOOKUP_STR(path_base,config->args, "-set_path_base", false, false);
    PXOPT_LOOKUP_F32(zpt_obs,  config->args, "-set_zpt_obs",  false, false);
    PXOPT_LOOKUP_F32(zpt_stdev,config->args, "-set_zpt_stdev",  false, false);
    PXOPT_LOOKUP_F32(mcal,     config->args, "-set_mcal",  false, false);
    PXOPT_LOOKUP_S16(fault,    config->args, "-set_fault",  false, false);
    PXOPT_LOOKUP_S32(ubercal_dist, config->args, "-set_ubercal_dist", false, false);
    PXOPT_LOOKUP_BOOL(clearfault, config->args, "-clearfault",  false);
    if (!state && !flags && !path_base && !isfinite(zpt_obs) && !isfinite(zpt_stdev) &&!isfinite(mcal)
        && !fault && !clearfault && !ubercal_dist) {
        psError(PXTOOLS_ERR_CONFIG, true, "must set at least one column\n");
        return false;
    }

    if (fault && clearfault) {
        psError(PXTOOLS_ERR_CONFIG, true, "cannot set and clear fault at same time\n");
        return false;
    }

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-exp_id", "relExp.exp_id", "==");
    PXOPT_COPY_S32(config->args, where, "-relexp_id", "relExp.relexp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "rawExp.exp_name", "==");
    // make sure that we have enough parameters to identify the relExp to change
    if (!psListLength(where->list)){
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    PXOPT_COPY_S32(config->args, where, "-rel_id",  "rel_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "release_name", "==");

    psString sep = "";
    psString comma = ", ";
    psString query = psStringCopy("UPDATE relExp JOIN ippRelease USING(rel_id) JOIN rawExp USING(exp_id) SET");
    if (state) {
        psStringAppend(&query, "%s relExp.state = '%s'", sep, state);
        sep = comma;
    }
    if (path_base) {
        psStringAppend(&query, "%s relExp.path_base = '%s'", sep, path_base);
        sep = comma;
    }
    if (flags) {
        psStringAppend(&query, "%s relExp.flags = %d", sep, flags);
        sep = comma;
    }
    if (isfinite(zpt_obs)) {
        psStringAppend(&query, "%s relExp.zpt_obs = %f", sep, zpt_obs);
        sep = comma;
    }
    if (isfinite(zpt_stdev)) {
        psStringAppend(&query, "%s relExp.zpt_stdev = %f", sep, zpt_stdev);
        sep = comma;
    }
    if (fault) {
        psStringAppend(&query, "%s relExp.fault = %d", sep, fault);
        sep = comma;
    }
    if (clearfault) {
        psStringAppend(&query, "%s relExp.fault = 0", sep);
        sep = comma;
    }
    if (isfinite(mcal)) {
        psStringAppend(&query, "%s relExp.mcal = %f", sep, mcal);
        sep = comma;
    }
    if (ubercal_dist) {
        psStringAppend(&query, "%s relExp.ubercal_dist = %d", sep, ubercal_dist);
        sep = comma;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("releasetool", PS_LOG_INFO, "Updated %" PRIu64 " relExp rows", affected);


    psFree(query);

    return true;
}

static bool tocalibexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-relexp_id",   "relExp.relexp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "LIKE");
#ifdef notdef
    PXOPT_COPY_S32(config->args, where, "-rel_id",      "relExp.rel_id", "==");
    pxAddLabelSearchArgs(config, where, "-release_state","ippRelease.state", "==");
    PXOPT_COPY_STR(config->args, where, "-state",       "relExp.state", "==");
    PXOPT_COPY_STR(config->args, where, "-filter",      "rawExp.filter", "LIKE");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin","rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_min",    "camProcessedExp.fwhm_major", ">=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_max",    "camProcessedExp.fwhm_major", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_name",    "rawExp.exp_name", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",      "relExp.exp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",     "relExp.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",      "relExp.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id",     "warpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-chip_data_group", "chipRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-cam_data_group",  "camRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-warp_data_group", "warpRun.data_group", "LIKE");

    PXOPT_COPY_STR(config->args, where, "-surveyName",  "survey.surveyName", "LIKE");

    PXOPT_LOOKUP_BOOL(priority_order, config->args, "-priority_order", false);
#endif

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("releasetool_tocalibexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString where2 = NULL;
#ifdef notdef
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxspaceAddWhere failed");
        return false;
    }
#endif

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", whereClause);
        psFree(whereClause);
#ifdef notdef
    } else if (where2) {
        psStringAppend(&query, "\nAND ");
#endif
    } else {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    if (where2) {
        psStringAppend(&query, "\n%s", where2);
        psFree(where2);
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "relExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool listrelexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-relexp_id",   "relExp.relexp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "LIKE");
    pxAddLabelSearchArgs(config, where, "-release_state","ippRelease.state", "==");
    PXOPT_COPY_STR(config->args, where, "-state",       "relExp.state", "==");
    PXOPT_COPY_STR(config->args, where, "-filter",      "rawExp.filter", "LIKE");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin","rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_min",    "camProcessedExp.fwhm_major", ">=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_max",    "camProcessedExp.fwhm_major", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_name",    "rawExp.exp_name", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",      "relExp.exp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",     "relExp.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",      "relExp.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id",     "warpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-chip_data_group", "chipRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-cam_data_group",  "camRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-warp_data_group", "warpRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-tess_id",      "warpRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",   "warpSkyfile.skycell_id", "==");
    
    PXOPT_LOOKUP_STR(skycell_id, config->args, "-skycell_id", false, false);

    PXOPT_COPY_STR(config->args, where, "-surveyName",  "survey.surveyName", "LIKE");
    PXOPT_COPY_S32(config->args, where, "-rel_id",      "relExp.rel_id", "==");

    PXOPT_LOOKUP_BOOL(priority_order, config->args, "-priority_order", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("releasetool_listrelexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (skycell_id) {
        psStringAppend(&query, "\nJOIN warpSkyfile ON warpRun.warp_id = warpSkyfile.warp_id AND warpRun.tess_id = warpSkyfile.tess_id");
    }

    psString where2 = NULL;
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxspaceAddWhere failed");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", whereClause);
        if (where2) {
            psStringAppend(&query, " %s", where2);
        }
        psStringAppend(&query, " AND warpRun.state != 'drop'");
        psFree(whereClause);
    } else if (where2) {
        psStringAppend(&query, "\nWHERE ");
    } else {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    if (where2) {
        psStringAppend(&query, "\n%s", where2);
        psFree(where2);
    }

    if (priority_order) {
        psStringAppend(&query, "\nAND priority > 0 ORDER BY exp_id, priority DESC");
    } else {
        psStringAppend(&query, "\nORDER BY exp_id, relexp_id DESC");
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "relExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool deleterelexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(relexp_id, config->args,   "-relexp_id", false, false);
    // If relexp_id is supplied that is all that we need
    if (!relexp_id) {
        // Otherwise we need to identify the release ...
        PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
        PXOPT_LOOKUP_S32(rel_id, config->args,      "-rel_id", false, false);

        if (!release_name && !rel_id) {
            psError(PXTOOLS_ERR_CONFIG, true, "at least one of -relexp_id, -release_name, or -rel_id is required\n");
            return false;
        }
        // ... and the exposure
        PXOPT_LOOKUP_STR(label, config->args,       "-label", false, false);
        PXOPT_LOOKUP_S64(cam_id, config->args,      "-cam_id", false, false);
        PXOPT_LOOKUP_S64(exp_id, config->args,      "-exp_id", false, false);
        PXOPT_LOOKUP_STR(exp_name, config->args,    "-exp_name", false, false);
        if (!label && !cam_id && !exp_id && !exp_name) {
            psError(PXTOOLS_ERR_CONFIG, true, "at least one of -label, -cam_id, -exp_id, -exp_name is required\n");
            return false;
        }
    }

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S32(config->args, where, "-relexp_id", "relExp.relexp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label",  "camRun.label", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id", "relExp.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id", "relExp.exp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-exp_name", "rawExp.exp_name", "==");

    PXOPT_COPY_S32(config->args, where, "-rel_id",  "relExp.rel_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "==");

    psString query = pxDataGet("releasetool_deleterelexp.sql");

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\n    AND %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("releasetool", PS_LOG_INFO, "Deleted %" PRIu64 " relExp rows", affected);


    psFree(query);

    return true;
}
static bool definerelstackMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    bool includeSkycal = false;

    PXOPT_COPY_STR(config->args, where, "-skycal_label", "skycalRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-skycal_data_group", "skycalRun.data_group", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-skycal_id",  "skycalRun.skycal_id", "==");
    if (psListLength(where->list)) {
        includeSkycal = true;
    }

    PXOPT_COPY_STR(config->args, where, "-label",      "stackRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-data_group", "stackRun.data_group", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-stack_id",   "stackRun.stack_id", "==");

    // Insure that at least one of the skycalRun selectors is supplied
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "at least one of -label, -data_group, or -skycal_id is required");
        return false;
    }

    // PXOPT_COPY_STR(config->args, where, "-tess_id",    "stackRun.tess_id", "==");
    // PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "==");

    // insure that at least one of these is supplied to select the release
    PXOPT_LOOKUP_S64(rel_id, config->args,      "-rel_id", false, false);
    PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
    if (!rel_id && !release_name) {
        psError(PXTOOLS_ERR_CONFIG, true, "either -rel_id or -release_name is required");
        return false;
    }

    // limit query to the target release. (Note: this will select nothing if release_name and rel_id don't match)
    // note: we add these after above to insure that search args are supplied
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "==");
    PXOPT_COPY_S32(config->args, where, "-rel_id", "ippRelease.rel_id", "==");


    PXOPT_LOOKUP_STR(state, config->args,     "-set_state", true, false);
    PXOPT_LOOKUP_STR(stack_type, config->args,"-set_stack_type", true, false);
    PXOPT_LOOKUP_U32(flags, config->args,     "-set_flags", false, false);
    PXOPT_LOOKUP_F32(zpt_obs, config->args,   "-set_zpt_obs", false, false);
    PXOPT_LOOKUP_F32(zpt_stdev, config->args, "-set_zpt_stdev", false, false);
    PXOPT_LOOKUP_STR(path_base, config->args, "-set_path_base", false, false);
    PXOPT_LOOKUP_S16(fault, config->args,     "-set_fault", false, false);

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args,  "-simple", false);
    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

    // find the parameters of all the exposures that we want to add to the release
    psString query;
    
    if (includeSkycal) {
        query = pxDataGet("releasetool_definerelstack_with_skycal.sql");
    } else {
        query = pxDataGet("releasetool_definerelstack.sql");
    }
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    psString joinConditions = psStringCopy("");
    // Finish up the selectors for the JOIN to previousRelStack
    // join to rows with our stack_type
    psStringAppend(&joinConditions, "\nAND previousRelStack.stack_type = '%s'", stack_type);

    if (!strcmp(stack_type, "nightly")) {
        // detect nightly stack entries that already exist for this day
        psStringAppend(&joinConditions, "\nAND previousRelStack.mjd_obs = floor(stackSumSkyfile.mjd_obs)");
    }

    // Add in the where conditions
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\nAND %s", whereClause);
    psFree(whereClause);

    if (!strcmp(stack_type, "nightly")) {
        // avoid stacks with NAN mjd_obs. These are old skycells that were lost due to system failure
        // prior to the time that the mjd_obs was extracted from the headers and added to stackSumSkyfile
        psStringAppend(&query, "\nAND stackSumSkyfile.mjd_obs IS NOT NULL");
    }

    psFree(where);

    if (limit) {
        psString limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    if (!p_psDBRunQueryF(config->dbh, query, joinConditions)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        psFree(joinConditions);
        return false;
    }
    psFree(query);
    psFree(joinConditions);

    psArray *output = p_psDBFetchResult(config->dbh);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }
    if (!psArrayLength(output)) {
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "relstack", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    psTime *now = psTimeGetNow(PS_TIME_TAI);
    // loop over our list of  exposures
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status = true;
        psS32 rel_id = psMetadataLookupS32(&status, md, "rel_id");
        psS64 stack_id = psMetadataLookupS64(&status, md, "stack_id");
        psS64 skycal_id = psMetadataLookupS64(&status, md, "skycal_id");
        psString skycell_id = psMetadataLookupStr(&status, md, "skycell_id");
        psString tess_id = psMetadataLookupStr(&status, md, "tess_id");
        psString filter = psMetadataLookupStr(&status, md, "filter");

        // use skycal zero point measurement if not supplied with arguments
        psF32 sc_zpt_obs = psMetadataLookupF32(&status, md, "zpt_obs");
        psF32 sc_zpt_stdev = psMetadataLookupF32(&status, md, "zpt_stdev");
        psF32 stack_mjd_obs = psMetadataLookupF32(&status, md, "mjd_obs");

        psU32 mjd_obs = 0;
        if (!strcmp(stack_type, "nightly")) {
            // only for nightly stacks do we set mjd_obs to a non zero value
            // We use integer day because this is part of the key that is used
            // to make sure entries are unique
            //      KEY (rel_id, tess_id, skycell_id, filter, mjd_obs),
            // 1 entry per release, skycell, filter and mjd_obs. 
            // mjd_obs is zero for deep and reference stacks
            mjd_obs = (psU32) stack_mjd_obs;
        }

        if (!relStackInsert(config->dbh,
            0,          // relstack_id (auto increment)
            rel_id,
            stack_id,
            skycal_id,
            skycell_id,
            tess_id,
            filter,
            state,
            flags,
            stack_type,
            isfinite(zpt_obs)   ? zpt_obs   : sc_zpt_obs,
            isfinite(zpt_stdev) ? zpt_stdev : sc_zpt_stdev,
            mjd_obs,
            path_base,
            fault,
            now,       // registered
            now        // time_stamp
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false,
                    "failed to trying to insert relStack for stack_id %" PRId64, stack_id);
            psFree(output);
            psFree(now);
            return false;
        }
    }
    psFree(output);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool updaterelstackMode(pxConfig *config)
{
    psError(PS_ERR_UNKNOWN, true, "not yet implemented");
    return false;
}
static bool setrelstackcalibratedfromskycalMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-skycal_label", "skycalRun.label", "==");
    PXOPT_COPY_STR(config->args, where, "-skycal_data_group", "skycalRun.data_group", "LIKE");
    PXOPT_COPY_S64(config->args, where, "-skycal_id",  "skycalRun.skycal_id", "==");
    if (!psListLength(where->list)) {
        psError(PXTOOLS_ERR_CONFIG, false, "at least one of -skycal_label, -skycal_data_group, or -skycal_id is required");
        return false;
    }

    PXOPT_COPY_STR(config->args, where, "-tess_id",    "stackRun.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id", "stackRun.skycell_id", "==");
    pxskycellAddWhere(config, where);

    // insure that at least one of these is supplied to select the release
    PXOPT_LOOKUP_S64(rel_id, config->args,      "-rel_id", false, false);
    PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
    if (!rel_id && !release_name) {
        psError(PXTOOLS_ERR_CONFIG, true, "either -rel_id or -release_name is required");
        return false;
    }

    // limit query to the target release. (Note: this will select nothing if release_name and rel_id don't match)
    // note: we add these after above to insure that search args are supplied
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "==");
    PXOPT_COPY_S32(config->args, where, "-rel_id", "ippRelease.rel_id", "==");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(replace, config->args, "-replace", false);

    psString query = pxDataGet("releasetool_setrelstackcalibratedfromskycal.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (replace) {
        // don't cut on relStack.state
        psStringAppend(&query, "\nWHERE ");;
    } else {
        // normal path only update rows in state processed
        psStringAppend(&query, "\nWHERE relStack.state = 'processed' and skycalResult.quality = 0\nAND ");;
    }
        
    // Add in the where conditions
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "%s", whereClause);
    psFree(whereClause);

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

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("releasetool", PS_LOG_INFO, "Updated %" PRIu64 " relStack rows", affected);

    return true;
}

static bool deleterelstackMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(relstack_id, config->args,   "-relstack_id", false, false);
    // If relexp_id is supplied that is all that we need
    if (!relstack_id) {
        // Otherwise we need to identify the release ...
        PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
        PXOPT_LOOKUP_S32(rel_id, config->args,      "-rel_id", false, false);

        if (!release_name && !rel_id) {
            psError(PXTOOLS_ERR_CONFIG, true, "at least one of -relstack_id, -release_name, or -rel_id is required\n");
            return false;
        }
        // ... and the Stack
        PXOPT_LOOKUP_STR(label, config->args,       "-label", false, false);
        PXOPT_LOOKUP_S64(stack_id, config->args,    "-stack_id", false, false);
        if (!label && !stack_id) {
            psError(PXTOOLS_ERR_CONFIG, true, "at least one of -label and  -stack_id is required\n");
            return false;
        }
    }

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S32(config->args, where, "-relstack_id", "relStack.relstack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-label", "stackRun.label", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id", "relStack.stack_id", "==");

    PXOPT_COPY_S32(config->args, where, "-rel_id", "relStack.rel_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "==");

    psString query = pxDataGet("releasetool_deleterelstack.sql");

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, "\n    AND %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("releasetool", PS_LOG_INFO, "Deleted %" PRIu64 " relStack rows", affected);


    psFree(query);

    return true;
}

static bool listrelstackMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-relstack_id", "relStack.relstack_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",    "relStack.stack_id", "==");
    PXOPT_COPY_S64(config->args, where, "-skycal_id",   "relStack.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-relstack_id", "relStack.relstack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "LIKE");
    pxAddLabelSearchArgs(config, where, "-release_state","ippRelease.state", "==");
    PXOPT_COPY_STR(config->args, where, "-state",       "relStack.state", "==");
    PXOPT_COPY_STR(config->args, where, "-filter",      "relStack.filter", "LIKE");
    PXOPT_COPY_F32(config->args, where, "-mjd_min",    "stackSumSkyfile.mjd_obs", ">=");
    PXOPT_COPY_F32(config->args, where, "-mjd_max",    "stackSumSkyfile.mjd_obs", "<=");
    PXOPT_COPY_STR(config->args, where, "-tess_id",     "relStack.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",  "relStack.skycell_id", "LIKE");
//    PXOPT_COPY_STR(config->args, where, "-stack_type",  "relStack.stack_type", "==");
    PXOPT_COPY_STR(config->args, where, "-stack_data_group",  "stackRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-skycal_data_group", "skycalRun.data_group", "LIKE");

    PXOPT_COPY_STR(config->args, where, "-surveyName",  "survey.surveyName", "LIKE");
    PXOPT_COPY_S32(config->args, where, "-rel_id",      "relStack.rel_id", "==");
    pxskycellAddWhere(config, where);

    PXOPT_COPY_F32(config->args, where, "-fwhm_min",    "IFNULL(skycalResult.fwhm_major, 999)", ">=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_max",    "IFNULL(skycalResult.fwhm_major, 0)", "<=");

    PXOPT_LOOKUP_BOOL(priority_order, config->args, "-priority_order", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    pxAddLabelSearchArgs (config, where, "-stack_type", "relStack.stack_type", "==");

    psString query = pxDataGet("releasetool_listrelstack.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    if (priority_order) {
        psStringAppend(&query, "\nAND priority > 0 order by priority DESC, stack_id");
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "relStack", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}
static bool summaryMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-sass_id",     "stackAssociation.sass_id", "==");
    PXOPT_COPY_S64(config->args, where, "-relstack_id", "relStack.relstack_id", "==");
    PXOPT_COPY_S64(config->args, where, "-stack_id",    "relStack.stack_id", "==");
//    PXOPT_COPY_S64(config->args, where, "-skycal_id",   "relStack.skycal_id", "==");
    PXOPT_COPY_S64(config->args, where, "-relstack_id", "relStack.relstack_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "LIKE");
    pxAddLabelSearchArgs(config, where, "-release_state","ippRelease.state", "==");
//    PXOPT_COPY_STR(config->args, where, "-state",       "relStack.state", "==");
    PXOPT_COPY_STR(config->args, where, "-filter",      "relStack.filter", "LIKE");
//    PXOPT_COPY_F32(config->args, where, "-mjd_min",    "stackSumSkyfile.mjd_obs", ">=");
//    PXOPT_COPY_F32(config->args, where, "-mjd_max",    "stackSumSkyfile.mjd_obs", "<=");
    PXOPT_COPY_STR(config->args, where, "-tess_id",     "relStack.tess_id", "==");
    PXOPT_COPY_STR(config->args, where, "-skycell_id",  "relStack.skycell_id", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-projection_cell",  "stackAssociation.projection_cell", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-stack_data_group",  "stackRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-data_group",  "stackAssociation.data_group", "LIKE");
//    PXOPT_COPY_STR(config->args, where, "-skycal_data_group", "skycalRun.data_group", "LIKE");

    PXOPT_COPY_STR(config->args, where, "-surveyName",  "survey.surveyName", "LIKE");
    PXOPT_COPY_S32(config->args, where, "-rel_id",      "relExp.rel_id", "==");
    pxskycellAddWhere(config, where);

//    PXOPT_COPY_F32(config->args, where, "-fwhm_min",    "IFNULL(skycalResult.fwhm_major, 999)", ">=");
//    PXOPT_COPY_F32(config->args, where, "-fwhm_max",    "IFNULL(skycalResult.fwhm_major, 0)", "<=");

    PXOPT_LOOKUP_BOOL(priority_order, config->args, "-priority_order", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    pxAddLabelSearchArgs (config, where, "-stack_type", "relStack.stack_type", "==");

    psString query = pxDataGet("releasetool_summary.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    if (priority_order) {
        psStringAppend(&query, "\nAND priority > 0 order by priority DESC, stack_id");
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "stackSummary", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool definerelgroupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    PXOPT_LOOKUP_STR(group_type, config->args, "-set_group_type", true, false);
    PXOPT_LOOKUP_STR(group_name, config->args, "-set_group_name", false, false);
    PXOPT_LOOKUP_STR(label, config->args,     "-set_label", true, false);

    PXOPT_LOOKUP_STR(state, config->args,     "-set_state", false, false);
    PXOPT_LOOKUP_STR(exp_list_path, config->args, "-set_exp_list_path", false, false);
    PXOPT_LOOKUP_S16(fault, config->args,     "-set_fault", false, false);

    psString query = NULL;

    psMetadata *where = psMetadataAlloc();

    // select sql and paramters based on group type
    bool type_lap = false;
    if (!strcmp(group_type, "lap")) {
        type_lap = true;
        if (group_name) {
            psError(PXTOOLS_ERR_CONFIG, true, "group_name is not allowed with group_type lap");
            psFree(where);
            return false;
        }
        PXOPT_COPY_S64(config->args, where, "-select_seq_id", "lapRun.seq_id", "==");
        PXOPT_COPY_S64(config->args, where, "-select_lap_id", "lapRun.lap_id", "==");
        query = pxDataGet("releasetool_definerelgroup_select_lap.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }
    } else {
        if (!strcmp(group_type, "date")) {
            // group_type ok
        } else if (!strcmp(group_type, "data_group")) {
            // group_type ok
        } else {
            psError(PXTOOLS_ERR_CONFIG, true, "%s is not valid group_type", group_type);
            return false;
        }
        if (!group_name) {
            psError(PXTOOLS_ERR_CONFIG, true, "group_name is required allowed with group_type %s", group_type);
            psFree(where);
            return false;
        }
        pxAddLabelSearchArgs (config, where, "-select_data_group", "camRun.data_group", "==");
        query = pxDataGet("releasetool_definerelgroup_select_data_group.sql");
        if (!query) {
            psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
            psFree(where);
            return false;
        }
    }

    // Insure that at least one of the exposure selectors is supplied
    if (!psListLength(where->list)) {
        psFree(where);
        psError(PXTOOLS_ERR_CONFIG, false, "search arguments are required");
        return false;
    }

    // insure that at least one of these is supplied to select the release
    PXOPT_LOOKUP_S64(rel_id, config->args,      "-rel_id", false, false);
    PXOPT_LOOKUP_STR(release_name, config->args, "-release_name", false, false);
    if (!rel_id && !release_name) {
        psError(PXTOOLS_ERR_CONFIG, true, "either -rel_id or -release_name is required");
        return false;
    }

    // limit query to the target release. (Note: this will select nothing if release_name and rel_id don't match)
    // note: we add these after above to insure that search args are supplied
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "==");
    PXOPT_COPY_S32(config->args, where, "-rel_id", "ippRelease.rel_id", "==");

    PXOPT_LOOKUP_BOOL(pretend, config->args, "-pretend", false);
    PXOPT_LOOKUP_BOOL(simple, config->args,  "-simple", false);

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);

    psFree(where);

    if (!type_lap) {
        psStringAppend(&query, "\nHAVING COUNT(exp_id) > 0");
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (pretend) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "relGroupToQueue", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
        return true;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    psTime *now = psTimeGetNow(PS_TIME_TAI);
    // loop over our list of  exposures
    for (long i = 0; i < psArrayLength(output); i++) {
        psMetadata *md = output->data[i];

        bool status = true;
        psS32 rel_id = psMetadataLookupS32(&status, md, "rel_id");
        psS64 lap_id = 0;
        if (type_lap) {
            lap_id = psMetadataLookupS64(&status, md, "lap_id");
        }

        if (!relGroupInsert(config->dbh,
            0,          // group_id (auto increment)
            rel_id,
            group_type,
            lap_id,
            group_name,
            "reg",      // state
            label,
            exp_list_path,
            fault,
            now        // registered
        )) {
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "failed to trying to insert relGroup");
            psFree(output);
            psFree(now);
            return false;
        }
        // Here's our new group_id
        psS32 group_id = psDBLastInsertID(config->dbh);

        psString query;
        if (type_lap) {
            query = pxDataGet("releasetool_definerelgroup_select_exp_lap.sql");
            if (!query) {
                psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
                psFree(where);
                return false;
            }
            psStringAppend(&query, " AND relExp.rel_id = %d AND lap_id = %" PRId64, rel_id, lap_id);
        } else {
            query = pxDataGet("releasetool_definerelgroup_select_exp_data_group.sql");
            if (!query) {
                psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
                psFree(where);
                return false;
            }
            psStringAppend(&query, " AND relExp.rel_id = %d", rel_id);

            psMetadata *where = psMetadataAlloc();
            pxAddLabelSearchArgs (config, where, "-select_data_group", "camRun.data_group", "==");
            psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
            psStringAppend(&query, " AND %s", whereClause);
            psFree(whereClause);
            psFree(where);
        }

        if (!p_psDBRunQuery(config->dbh, query)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(query);
            psFree(output);
            psFree(now);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        psFree(query);

        psArray *exposures = p_psDBFetchResult(config->dbh);
        if (!exposures) {
            psFree(now);
            psFree(output);
            psError(PS_ERR_UNKNOWN, false, "database error");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
        if (!psArrayLength(exposures)) {
            fprintf(stderr, "no exposures found for lap_id %" PRId64 "\n", lap_id);
#ifdef notdef
            continue;
            psFree(now);
            psFree(output);
            psFree(exposures);
            psError(PS_ERR_UNKNOWN, false, "no exposures found error");
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
#endif
        }

        // assign the relExps to this new group
        char *updateQuery = "UPDATE relExp SET group_id = %d WHERE relexp_id = %" PRId64;
        for (int j = 0; j < psArrayLength(exposures); j++) {
            psMetadata *md = exposures->data[j];

            bool status = true;
            psS32 relexp_id = psMetadataLookupS32(&status, md, "relexp_id");
            if (!p_psDBRunQueryF(config->dbh, updateQuery, group_id, relexp_id)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
                psFree(now);
                psFree(output);
                psFree(exposures);

                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                return false;
            }
        }
        psFree(exposures);

        // update the state of relGroup to 'new'
        if (!p_psDBRunQueryF(config->dbh, "UPDATE relGroup set state = 'new' WHERE group_id = %d", group_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(now);
            psFree(output);
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            return false;
        }
    }
    psFree(output);
    psFree(now);

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static bool updaterelgroupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S32(group_id, config->args,      "-group_id", false, false);
    PXOPT_LOOKUP_STR(group_name, config->args,    "-group_name",  false, false);
    if (!group_id && !group_name) {
        psError(PXTOOLS_ERR_CONFIG, true, "either group_id or group_name is required\n");
        return false;
    }

    PXOPT_LOOKUP_STR(state,    config->args,    "-set_state",  false, false);
    PXOPT_LOOKUP_STR(exp_list_path, config->args, "-set_exp_list_path", false, false);
    PXOPT_LOOKUP_STR(new_label, config->args,   "-set_label", false, false);
    PXOPT_LOOKUP_S16(fault,    config->args,    "-set_fault",  false, false);
    PXOPT_LOOKUP_BOOL(clearfault, config->args, "-clearfault",  false);

    if (!state && !exp_list_path && !new_label && !fault && !clearfault) {
        psError(PXTOOLS_ERR_CONFIG, true, "must set at least one column\n");
        return false;
    }

    if (fault && clearfault) {
        psError(PXTOOLS_ERR_CONFIG, true, "cannot set and clear fault at same time\n");
        return false;
    }

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_S64(config->args, where, "-group_id", "relGroup.group_id", "==");
    PXOPT_COPY_STR(config->args, where, "-group_name", "relGroup.group_name", "==");
    // XXX TODO if label is enabled (for changing label or state for a block) we should
    // disallow setting some parameters such as exp_list_path
    //    PXOPT_COPY_S64(config->args, where, "-label", "relGroup.label", "==");
    // make sure that we have enough parameters to identify the relGroup to change
    if (!psListLength(where->list)){
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    psString sep = "";
    psString comma = ", ";
    psString query = psStringCopy("UPDATE relGroup SET");
    if (state) {
        psStringAppend(&query, "%s relGroup.state = '%s'", sep, state);
        sep = comma;
    }
    if (exp_list_path) {
        psStringAppend(&query, "%s relGroup.exp_list_path = '%s'", sep, exp_list_path);
        sep = comma;
    }
    if (new_label) {
        psStringAppend(&query, "%s relGroup.label = '%s'", sep, new_label);
        sep = comma;
    }
    if (fault) {
        psStringAppend(&query, "%s relGroup.fault = %d", sep, fault);
        sep = comma;
    }
    if (clearfault) {
        psStringAppend(&query, "%s relGroup.fault = 0", sep);
        sep = comma;
    }

    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " WHERE %s", whereClause);
    psFree(whereClause);

    if (!p_psDBRunQuery(config->dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }

    psU64 affected = psDBAffectedRows(config->dbh);
    psLogMsg("releasetool", PS_LOG_INFO, "Updated %" PRIu64 " ippReleases", affected);


    psFree(query);

    return true;
}

static bool pendingrelgroupMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    pxAddLabelSearchArgs (config, where, "-label",      "relGroup.label", "==");
    PXOPT_COPY_S32(config->args, where, "-group_id",    "relGroup.group_id", "==");
    PXOPT_COPY_S32(config->args, where, "-rel_id",      "relExp.rel_id", "==");
    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "LIKE");

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("releasetool_pendingrelgroup.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nAND %s", whereClause);
        psFree(whereClause);
    } else {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "pending_relGroup", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

static bool listrelgroupMode(pxConfig *config)
{
    return false;
#ifdef notdef
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psMetadata *where = psMetadataAlloc();

    PXOPT_COPY_STR(config->args, where, "-release_name", "ippRelease.release_name", "LIKE");
    pxAddLabelSearchArgs(config, where, "-release_state","ippRelease.state", "==");
    PXOPT_COPY_STR(config->args, where, "-state",       "relExp.state", "==");
    PXOPT_COPY_STR(config->args, where, "-filter",      "rawExp.filter", "LIKE");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin","rawExp.dateobs", ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end", "rawExp.dateobs", "<=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_min",    "skycalResult.fwhm_major", ">=");
    PXOPT_COPY_F32(config->args, where, "-fwhm_max",    "skycalResult.fwhm_major", "<=");
    PXOPT_COPY_STR(config->args, where, "-exp_name",    "rawExp.exp_ame", "==");
    PXOPT_COPY_S64(config->args, where, "-exp_id",      "relExp.exp_id", "==");
    PXOPT_COPY_S64(config->args, where, "-chip_id",     "relExp.chip_id", "==");
    PXOPT_COPY_S64(config->args, where, "-cam_id",      "relExp.cam_id", "==");
    PXOPT_COPY_S64(config->args, where, "-warp_id",     "warpRun.warp_id", "==");
    PXOPT_COPY_STR(config->args, where, "-chip_data_group", "chipRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-cam_data_group",  "camRun.data_group", "LIKE");
    PXOPT_COPY_STR(config->args, where, "-warp_data_group", "warpRun.data_group", "LIKE");

    PXOPT_COPY_STR(config->args, where, "-surveyName",  "survey.surveyName", "LIKE");
    PXOPT_COPY_S32(config->args, where, "-rel_id",      "relExp.rel_id", "==");

    PXOPT_LOOKUP_BOOL(priority_order, config->args, "-priority_order", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    psString query = pxDataGet("releasetool_listrelexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString where2 = NULL;
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxspaceAddWhere failed");
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psStringAppend(&query, "\nWHERE %s", whereClause);
        psFree(whereClause);
    } else if (where2) {
        psStringAppend(&query, "\nWHERE ");
    } else {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required\n");
        psFree(where);
        return false;
    }

    if (where2) {
        psStringAppend(&query, "\n%s", where2);
        psFree(where2);
    }

    if (priority_order) {
        psStringAppend(&query, "\nAND priority > 0 order by exp_id, priority");
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
        psTrace("releasetool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (!ippdbPrintMetadatas(stdout, output, "relExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
#endif //notdef
}


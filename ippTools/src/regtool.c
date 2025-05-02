/*
 * regtool.c
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
#include <math.h>

#include "pxtools.h"
#include "pxdata.h"
#include "regtool.h"
#include "chiptool.h"

// imfile
static bool pendingimfileMode(pxConfig *config);
static bool addprocessedimfileMode(pxConfig *config);
static bool checkburntoolimfileMode(pxConfig *config);
static bool pendingburntoolimfileMode(pxConfig *config);
static bool processedimfileMode(pxConfig *config);
static bool revertprocessedimfileMode(pxConfig *config);
static bool updateprocessedimfileMode(pxConfig *config);
static bool pendingcompressimfileMode(pxConfig *config);
// exp
static bool pendingexpMode(pxConfig *config);
static bool addprocessedexpMode(pxConfig *config);
static bool processedexpMode(pxConfig *config);
static bool revertprocessedexpMode(pxConfig *config);
static bool updateprocessedexpMode(pxConfig *config);
static bool cleardupexpMode(pxConfig *config);
static bool finishcompressexpMode(pxConfig *config);

static bool updatebyqueryMode(pxConfig *config);

static bool checkstatusMode(pxConfig *config);

static bool exportrunMode(pxConfig *config);
static bool importrunMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = regtoolConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        // imfile
        MODECASE(REGTOOL_MODE_PENDINGIMFILE,         pendingimfileMode);
        MODECASE(REGTOOL_MODE_CHECKBURNTOOLIMFILE,   checkburntoolimfileMode);
        MODECASE(REGTOOL_MODE_PENDINGBURNTOOLIMFILE, pendingburntoolimfileMode);
        MODECASE(REGTOOL_MODE_ADDPROCESSEDIMFILE,    addprocessedimfileMode);
        MODECASE(REGTOOL_MODE_PROCESSEDIMFILE,       processedimfileMode);
        MODECASE(REGTOOL_MODE_REVERTPROCESSEDIMFILE, revertprocessedimfileMode);
        MODECASE(REGTOOL_MODE_UPDATEPROCESSEDIMFILE, updateprocessedimfileMode);
        // exp
        MODECASE(REGTOOL_MODE_PENDINGEXP,            pendingexpMode);
        MODECASE(REGTOOL_MODE_ADDPROCESSEDEXP,       addprocessedexpMode);
        MODECASE(REGTOOL_MODE_PROCESSEDEXP,          processedexpMode);
        MODECASE(REGTOOL_MODE_REVERTPROCESSEDEXP,    revertprocessedexpMode);
        MODECASE(REGTOOL_MODE_UPDATEPROCESSEDEXP,    updateprocessedexpMode);
        MODECASE(REGTOOL_MODE_UPDATEBYQUERY,         updatebyqueryMode);
        MODECASE(REGTOOL_MODE_PENDINGCOMPRESSIMFILE, pendingcompressimfileMode);
        MODECASE(REGTOOL_MODE_FINISHCOMPRESSEXP,     finishcompressexpMode);
        MODECASE(REGTOOL_MODE_CLEARDUPEXP,           cleardupexpMode);
	MODECASE(REGTOOL_MODE_CHECKSTATUS,           checkstatusMode);
        MODECASE(REGTOOL_MODE_EXPORTRUN,             exportrunMode);
        MODECASE(REGTOOL_MODE_IMPORTRUN,             importrunMode);
        default:
            psAbort("invalid option (this should not happen)");
    }
    psTrace("regtool",9,"Attempting to free config\n");
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


static bool pendingimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // select newImfiles that:
    // exp_id is in newExp
    // don't have their exp_id in rawExp
    // XXX having the same exp_id in newExp and raw*Exp is probably an error
    // that should be checked for

    psString query = pxDataGet("regtool_pendingimfile.sql");
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
        // XXX PS_EXIT_PROG_ERROR (incorrect SQL) or SYS_ERROR (database comms)
        psError(PXTOOLS_ERR_PROG, false, "database error");
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
        psTrace("regtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "regPendingImfile", !simple)) {
        psError(PXTOOLS_ERR_PROG, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}

  
  

static bool checkburntoolimfileMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config,false);

  // required
  PXOPT_LOOKUP_STR(exp_name, config->args, "-exp_name", true, false);
  PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
  PXOPT_LOOKUP_STR(dateobs_begin,     config->args, "-dateobs_begin", true, false);
  PXOPT_LOOKUP_STR(dateobs_end,     config->args, "-dateobs_end", true, false);
  PXOPT_LOOKUP_S32(valid_burntool, config->args, "-valid_burntool", true, false);
  // optional
  PXOPT_LOOKUP_STR(camera, config->args, "-inst", false, false);
  PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  psString query = pxDataGet("regtool_checkburntoolimfile.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  psString rep = psStringCopy(query);
  psFree(query);
  query = rep;

  // convert regular class_id format to summitImfile.class_id format
  rep = psStringCopy(class_id);

  class_id = rep;
  
  psStringSubstitute(&class_id,"ota","XY");
  
  psStringSubstitute(&query,exp_name,"@EXP_NAME@");
  psStringSubstitute(&query,class_id,"@CLASS_ID@");
  psStringSubstitute(&query,dateobs_begin,"@DATEOBS_BEGIN@");
  psStringSubstitute(&query,dateobs_end,"@DATEOBS_END@");

  // fprintf(stderr,"%s",query);

  if (!p_psDBRunQuery(config->dbh, query)) {
    // XXX PS_EXIT_PROG_ERROR (incorrect SQL) or SYS_ERROR (database comms)
    psError(PXTOOLS_ERR_PROG, false, "database error");
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
    psTrace("regtool", PS_LOG_INFO, "no rows found");
    return true;
  }

  psString previous_uri = NULL;
  bool ok_to_burn = true;
  bool already_burned = true;
  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i];

    if (strcasecmp(psMetadataLookupStr(NULL,row,"download_state"),"drop") == 0) {
      continue;
    }
    char *is_ccim = strchr(psMetadataLookupStr(NULL,row,"exp_name"),'c');
    if (is_ccim) { // Is a camera commanded exposure
      //      fprintf(stderr,"IN camera commanded!\n");
      if ((strcasecmp(psMetadataLookupStr(NULL,row,"exp_type"),"DOMEFLAT") == 0)||
	  (strcasecmp(psMetadataLookupStr(NULL,row,"exp_type"),"DARK") == 0) ||
	  (strcasecmp(psMetadataLookupStr(NULL,row,"exp_type"),"BIAS") == 0)) {
	continue;
      }
    }
    char *is_stareim = strchr(psMetadataLookupStr(NULL,row,"exp_name"),'a');
    if (is_stareim) {
      continue;
    }
    
    if ((psMetadataLookupS32(NULL,row,"is_downloaded") != 1)||
        (psMetadataLookupS32(NULL,row,"is_registered") != 1)) {
      ok_to_burn = false;
    }
    if (already_burned == false) {
      ok_to_burn = false;
    }
    if (abs(psMetadataLookupS32(NULL,row,"burntool_state")) == valid_burntool) {
      already_burned = true;
    }
    else {
      already_burned = false;
    }

    if (previous_uri) {
      psMetadataAddStr(row,PS_LIST_TAIL,"previous_uri",PS_META_REPLACE,"",previous_uri);
      psFree(previous_uri);
    }
    psMetadataAddBool(row,PS_LIST_TAIL,"burnable",PS_META_REPLACE,"",ok_to_burn);
    psMetadataAddBool(row,PS_LIST_TAIL,"already_burned",PS_META_REPLACE,"",already_burned);
    previous_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri"));
  }

  // negate simple so the default is true
  if (!ippdbPrintMetadatas(stdout, output, "regBurntoolImfile", !simple)) {
    psError(PXTOOLS_ERR_PROG, false, "failed to print array");
    psFree(output);
    return false;
  }

  psFree(output);


  return(true);

}


// CZW: 2013-12-11 A bad exposure caused this code to fail to return any rows.
//      I don't clearly see any logic flaws, and so I fixed the issue by
//      retricting dateobs_begin to exclude this exposure.  Problem for
//      another day.
static bool pendingburntoolimfileMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config,false);

  // required
  PXOPT_LOOKUP_STR(dateobs_begin,   config->args, "-dateobs_begin",  true, false);
  PXOPT_LOOKUP_STR(dateobs_end,     config->args, "-dateobs_end",    true, false);
  PXOPT_LOOKUP_S32(valid_burntool,  config->args, "-valid_burntool", true, false);
  // optional
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
  PXOPT_LOOKUP_BOOL(ignore_state, config->args, "-ignore_state", false);
  psString query = pxDataGet("regtool_pendingburntoolimfile.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  psString rep = psStringCopy(query);
  psFree(query);
  query = rep;

  // This matches against summitExp.dateobs
  psStringSubstitute(&query,dateobs_begin,"@DATEOBS_BEGIN@");
  psStringSubstitute(&query,dateobs_end,"@DATEOBS_END@");

  // fprintf(stderr,"%s",query);

  if (!p_psDBRunQuery(config->dbh, query)) {
    // XXX PS_EXIT_PROG_ERROR (incorrect SQL) or SYS_ERROR (database comms)
    psError(PXTOOLS_ERR_PROG, false, "database error");
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

  // fprintf (stderr, "found %ld rows\n", output->n);
  if (!psArrayLength(output)) {
    psTrace("regtool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  psString previous_uri = NULL;
  psString this_uri = NULL;
  bool ok_to_burn = true;
  bool already_burned = true;
  psString previous_class_id = NULL;
  psString this_class_id = NULL;
  psArray *results = psArrayAllocEmpty(60); // List of suggested imfiles to burntool.

  for (long i = 0; i < output->n; i++) {
    psMetadata *row = output->data[i];

    if (strcasecmp(psMetadataLookupStr(NULL,row,"download_state"),"drop") == 0) {
      continue;
    }

    char *is_ccim = strchr(psMetadataLookupStr(NULL,row,"exp_name"),'c');
    if (is_ccim) { // Is a camera commanded exposure
      //      fprintf(stderr,"IN camera commanded!\n");
      if ((strcasecmp(psMetadataLookupStr(NULL,row,"exp_type"),"DOMEFLAT") == 0)||
	  (strcasecmp(psMetadataLookupStr(NULL,row,"exp_type"),"DARK") == 0) ||
	  (strcasecmp(psMetadataLookupStr(NULL,row,"exp_type"),"BIAS") == 0)) {
	continue;
      }
    }
    char *is_stareim = strchr(psMetadataLookupStr(NULL,row,"exp_name"),'a');
    if (is_stareim) {
      continue;
    }


    bool status = false;
    char *tmp_id = psMetadataLookupStr(&status,row,"summit_class_id");
    if (!status) {
            fprintf (stderr, "incomplete on %s\n", psMetadataLookupStr(NULL,row,"exp_name"));
        continue;
    }

    // Add the information about this row and the previous, if it exists.
    // Write the class_id stuff for debugging.
    // this_class_id = psStringCopy(psMetadataLookupStr(NULL,row,"tmp_class_id"));
    this_class_id = psStringCopy(tmp_id);

    psMetadataAddStr(row,PS_LIST_TAIL,"this_class_id",PS_META_REPLACE,"",this_class_id);

    if (previous_class_id) {
      psMetadataAddStr(row,PS_LIST_TAIL,"previous_class_id",PS_META_REPLACE,"",previous_class_id);
    }
    // class_id = NULL sorts to the top of the list, so skip those (incomplete downloads)
    if (!this_class_id) {
      continue;
    }

    
    if (0 && !strcmp(this_class_id, "ota44")) {
        printf("STAT 1: %s (%d %d) %d %d %d\n",
               this_class_id,
               ok_to_burn, already_burned,
               psMetadataLookupS32(NULL,row,"burntool_state"),
               psMetadataLookupS32(NULL,row,"is_registered"),
               psMetadataLookupS32(NULL,row,"is_downloaded"));
    }

    // Determine if we've crossed a class_id boundary, as this resets the bits.
    if (previous_class_id) {
      if (strcmp(this_class_id,previous_class_id) != 0) {
        ok_to_burn = true;
        already_burned = true;
        previous_uri = NULL;
	//	fprintf (stderr, "crossed boundary: %s : %s to %s\n", psMetadataLookupStr(NULL,row,"exp_name"), previous_class_id, this_class_id);
      }
    }

    // Write the URIs as well.
    this_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri")); // Duplicate, but helpful for my debugging.
    psMetadataAddStr(row,PS_LIST_TAIL,"this_uri",PS_META_REPLACE,"",this_uri);
    if (previous_uri) {
      psMetadataAddStr(row,PS_LIST_TAIL,"previous_uri",PS_META_REPLACE,"",previous_uri);
    }

    // Check the two status variables
    // Convert bits of the SQL query into booleans describing the data state
    if ((psMetadataLookupS32(NULL,row,"is_downloaded") != 1)||
        (psMetadataLookupS32(NULL,row,"is_registered") != 1)) {
      // printf("I claim this isn't downloaded or registered? %s %s\n",this_uri,this_class_id);
        ok_to_burn = false;
    }
    if (already_burned == false) {
        // printf("already_burned looks false %s %s\n",this_uri,this_class_id);
        ok_to_burn = false;
    }
    if (abs(psMetadataLookupS32(NULL,row,"burntool_state")) == valid_burntool) {
      already_burned = true;
    }
    else {
      already_burned = false;
    }
    psMetadataAddBool(row,PS_LIST_TAIL,"burnable",PS_META_REPLACE,"",ok_to_burn);
    psMetadataAddBool(row,PS_LIST_TAIL,"already_burned",PS_META_REPLACE,"",already_burned);

    // Check the uri for this exposure
    if (!this_uri) {
        ok_to_burn = false;
        already_burned = false;

	fprintf (stderr, "missing uri: %s %s\n", psMetadataLookupStr(NULL,row,"exp_name"), this_class_id);

        // Save this round for next round.
        psFree(previous_class_id);
        psFree(previous_uri);
        previous_class_id = psStringCopy(psMetadataLookupStr(NULL,row,"summit_class_id"));
        psStringSubstitute(&previous_class_id,"ota","XY");
        previous_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri")); // Save for next round.
        continue;
    }

    if (0 && !strcmp(this_class_id, "ota44")) {
        printf("STATUS: %s %s %s %s (%d %d) %d %d %d\n",this_uri,previous_uri,this_class_id,previous_class_id,ok_to_burn,already_burned,psMetadataLookupS32(NULL,row,"burntool_state"),psMetadataLookupS32(NULL,row,"is_registered"),psMetadataLookupS32(NULL,row,"is_downloaded"));
    }
    // If the state of this imfile is not "pending_burntool" then we can't burn it.
    if (!ignore_state) {
      psString imfile_state = psMetadataLookupStr(NULL,row,"imfile_state");

      if (!imfile_state) { // imfile state is NULL, so we probably aren't registered.
        ok_to_burn = false;

        // fprintf (stderr, "missing imfile_state: %s %s\n", psMetadataLookupStr(NULL,row,"exp_name"), this_class_id);

        // Save this round for next round.
        psFree(previous_class_id);
        psFree(previous_uri);
        previous_class_id = psStringCopy(psMetadataLookupStr(NULL,row,"summit_class_id"));
        psStringSubstitute(&previous_class_id,"ota","XY");
        previous_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri")); // Save for next round.
        continue;
      }
      if (strcmp("pending_burntool",imfile_state) != 0) { // Probably the state is full, do not twiddle states

          if (0 && strcmp(imfile_state, "full")) {
              fprintf (stderr, "not pending nor full: %s %s %s\n", psMetadataLookupStr(NULL,row,"exp_name"), this_class_id, imfile_state);
          }

        // Save this round for next round.
        psFree(previous_class_id);
        psFree(previous_uri);
        previous_class_id = psStringCopy(psMetadataLookupStr(NULL,row,"summit_class_id"));
        psStringSubstitute(&previous_class_id,"ota","XY");
        previous_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri")); // Save for next round.
        continue;
      }
    }

    // Determine if we've already suggested an entry for this ota, and if not, copy this
    // suggestion into our output result list.
    if (!ok_to_burn || already_burned) {
      // Save this round for next round.
      psFree(previous_class_id);
      psFree(previous_uri);
      previous_class_id = psStringCopy(psMetadataLookupStr(NULL,row,"summit_class_id"));
      psStringSubstitute(&previous_class_id,"ota","XY");
      previous_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri")); // Save for next round.
      continue;
    }
    // If we're here, then we think we could potentially burntool this file.
    psArrayAdd(results,results->n,row);

    // Save this round for next round.
    psFree(previous_class_id);
    psFree(previous_uri);
    previous_class_id = psStringCopy(psMetadataLookupStr(NULL,row,"summit_class_id"));
    psStringSubstitute(&previous_class_id,"ota","XY");
    previous_uri = psStringCopy(psMetadataLookupStr(NULL,row,"uri")); // Save for next round.
  }

  // negate simple so the default is true
  if (!ippdbPrintMetadatas(stdout, results, "regPendingBurntoolImfile", !simple)) {
    psError(PXTOOLS_ERR_PROG, false, "failed to print array");
    psFree(output);
    psFree(results);
    return false;
  }

  psFree(output);
  psFree(results);


  return(true);

}



static bool addprocessedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // required
    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);
    PXOPT_LOOKUP_STR(exp_name, config->args, "-exp_name", true, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_STR(tmp_class_id, config->args, "-tmp_class_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);

    // optional
    PXOPT_LOOKUP_STR(exp_type, config->args, "-exp_type", false, false);
    PXOPT_LOOKUP_STR(filelevel, config->args, "-filelevel", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", false, false);
    PXOPT_LOOKUP_STR(comment, config->args, "-comment", false, false);
    PXOPT_LOOKUP_STR(obs_mode, config->args, "-obs_mode", false, false);
    PXOPT_LOOKUP_STR(obs_group, config->args, "-obs_group", false, false);
    PXOPT_LOOKUP_STR(data_state, config->args, "-data_state", false, false);

    PXOPT_LOOKUP_F32(airmass, config->args, "-airmass", false, false);
    PXOPT_LOOKUP_F64(ra, config->args, "-ra", false, false);
    PXOPT_LOOKUP_F64(decl, config->args, "-decl", false, false);
    PXOPT_LOOKUP_F32(exp_time, config->args, "-exp_time", false, false);
    PXOPT_LOOKUP_F32(sat_pixel_frac, config->args, "-sat_pixel_frac", false, false);
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F64(alt, config->args, "-alt", false, false);
    PXOPT_LOOKUP_F64(az, config->args, "-az", false, false);
    PXOPT_LOOKUP_F32(ccd_temp, config->args, "-ccd_temp", false, false);
    PXOPT_LOOKUP_F64(posang, config->args, "-posang", false, false);
    PXOPT_LOOKUP_F32(m1_x, config->args, "-m1_x", false, false);
    PXOPT_LOOKUP_F32(m1_y, config->args, "-m1_y", false, false);
    PXOPT_LOOKUP_F32(m1_z, config->args, "-m1_z", false, false);
    PXOPT_LOOKUP_F32(m1_tip, config->args, "-m1_tip", false, false);
    PXOPT_LOOKUP_F32(m1_tilt, config->args, "-m1_tilt", false, false);
    PXOPT_LOOKUP_F32(m2_x, config->args, "-m2_x", false, false);
    PXOPT_LOOKUP_F32(m2_y, config->args, "-m2_y", false, false);
    PXOPT_LOOKUP_F32(m2_z, config->args, "-m2_z", false, false);
    PXOPT_LOOKUP_F32(m2_tip, config->args, "-m2_tip", false, false);
    PXOPT_LOOKUP_F32(m2_tilt, config->args, "-m2_tilt", false, false);

    PXOPT_LOOKUP_F32(env_temp, config->args, "-env_temperature", false, false);
    PXOPT_LOOKUP_F32(env_humid, config->args, "-env_humidity", false, false);
    PXOPT_LOOKUP_F32(env_wind, config->args, "-env_wind_speed", false, false);
    PXOPT_LOOKUP_F32(env_dir, config->args, "-env_wind_dir", false, false);
    PXOPT_LOOKUP_F32(teltemp_m1, config->args, "-teltemp_m1", false, false);
    PXOPT_LOOKUP_F32(teltemp_m1cell, config->args, "-teltemp_m1cell", false, false);
    PXOPT_LOOKUP_F32(teltemp_m2, config->args, "-teltemp_m2", false, false);
    PXOPT_LOOKUP_F32(teltemp_spider, config->args, "-teltemp_spider", false, false);
    PXOPT_LOOKUP_F32(teltemp_truss, config->args, "-teltemp_truss", false, false);
    PXOPT_LOOKUP_F32(teltemp_extra, config->args, "-teltemp_extra", false, false);
    PXOPT_LOOKUP_F32(pon_time, config->args, "-pon_time", false, false);
    // PXOPT_LOOKUP_S16(burntool_state, config->args, "-burntool_state", false, false);
    PXOPT_LOOKUP_F64(user_1, config->args, "-user_1", false, false);
    PXOPT_LOOKUP_F64(user_2, config->args, "-user_2", false, false);
    PXOPT_LOOKUP_F64(user_3, config->args, "-user_3", false, false);
    PXOPT_LOOKUP_F64(user_4, config->args, "-user_4", false, false);
    PXOPT_LOOKUP_F64(user_5, config->args, "-user_5", false, false);
    PXOPT_LOOKUP_STR(object, config->args, "-object", false, false);
    PXOPT_LOOKUP_F32(sun_angle,  config->args, "-sun_angle", false, false);
    PXOPT_LOOKUP_F32(sun_alt,    config->args, "-sun_alt",   false, false);
    PXOPT_LOOKUP_F32(moon_angle, config->args, "-moon_angle", false, false);
    PXOPT_LOOKUP_F32(moon_alt,   config->args, "-moon_alt",   false, false);
    PXOPT_LOOKUP_F32(moon_phase, config->args, "-moon_phase", false, false);
    PXOPT_LOOKUP_BOOL(ignored, config->args, "-ignore", false);
    PXOPT_LOOKUP_TIME(dateobs, config->args, "-dateobs", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    PXOPT_LOOKUP_S32(bytes, config->args,  "-bytes", false, false);
    PXOPT_LOOKUP_STR(md5sum, config->args, "-md5sum", false, false);
    PXOPT_LOOKUP_BOOL(video_cells, config->args, "-video_cells", false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);
    PXOPT_LOOKUP_S16(quality, config->args, "-quality", false, false);

    if (!rawImfileInsert(
        config->dbh,
        exp_id,
        exp_name,
        camera,
        telescope,
        dateobs,
        tmp_class_id,
        class_id,
        uri,
        data_state,
        exp_type,
        filelevel,
        filter,
        comment,
        obs_mode,
        obs_group,
        airmass,
        ra,
        decl,
        exp_time,
        sat_pixel_frac,
        bg,
        bg_stdev,
        bg_mean_stdev,
        alt,
        az,
        ccd_temp,
        posang,
        m1_x,
        m1_y,
        m1_z,
        m1_tip,
        m1_tilt,
        m2_x,
        m2_y,
        m2_z,
        m2_tip,
        m2_tilt,
        env_temp,
        env_humid,
        env_wind,
        env_dir,
        teltemp_m1,
        teltemp_m1cell,
        teltemp_m2,
        teltemp_spider,
        teltemp_truss,
        teltemp_extra,
        pon_time,
        user_1,
        user_2,
        user_3,
        user_4,
        user_5,
        object,
        sun_angle,
        sun_alt,
        moon_angle,
        moon_alt,
        moon_phase,
        ignored,
        hostname,
        fault,
        quality,
        NULL,
        0,
        bytes,
        md5sum,
        0,   // burntool_state
        video_cells
    )) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool processedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-exp_id",        "exp_id",   "==");
    PXOPT_COPY_STR(config->args, where,  "-exp_name",      "exp_name", "==");
    PXOPT_COPY_STR(config->args, where,  "-class_id",      "class_id", "==");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_begin", "dateobs",  ">=");
    PXOPT_COPY_TIME(config->args, where, "-dateobs_end",   "dateobs",  "<=");
    PXOPT_COPY_STR(config->args, where,  "-filter",        "filter", "LIKE");
    PXOPT_COPY_STR(config->args, where,  "-exp_type",      "exp_type", "==");
    PXOPT_COPY_STR(config->args, where,  "-obs_mode",      "obs_mode", "==");
    PXOPT_COPY_STR(config->args, where,  "-data_state",    "data_state", "==");
    PXOPT_LOOKUP_BOOL(video_cells, config->args, "-video_cells", false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);
    PXOPT_LOOKUP_BOOL(allfiles, config->args, "-allfiles", false);
    if (allfiles) {
        faulted = false;
    }
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_BOOL(ordered_by_date, config->args, "-ordered_by_date", false);

    // build where string for some parameters that don't fit PXOPT_COPY*
    psString where2 = NULL;
    if (!pxmagicAddWhere(config, &where2, "rawImfile")) {
        psError(psErrorCodeLast(), false, "pxmagicAddWhere failed");
        return false;
    }
    if (!pxspaceAddWhere(config, &where2, "rawImfile")) {
        psError(psErrorCodeLast(), false, "pxspaceAddWhere failed");
        return false;
    }

    psString query = pxDataGet("regtool_processedimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "rawImfile");
        psStringAppend(&query, " AND %s", whereClause);
        psFree(whereClause);
    } else if (!all && !where2) {
        psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
        return false;
    }
    psFree(where);
    if (where2) {
        psStringAppend(&query, " %s", where2);
        psFree(where2);
    }

    if (faulted) {
        // list only faulted rows
        psStringAppend(&query, " %s", "AND rawImfile.fault != 0");
    } else if (!allfiles) {
        // don't list faulted rows
        psStringAppend(&query, " %s", "AND rawImfile.fault = 0");
    }
    if (video_cells) {
        psStringAppend(&query, " %s", "AND rawImfile.video_cells");
    }

    // add the ORDER BY statement if desired
    if (ordered_by_date) {
        psStringAppend(&query, " ORDER BY dateobs");
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
        psTrace("regtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    if (psArrayLength(output)) {
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "rawImfile", !simple)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
    }

    psFree(output);

    return true;
}


static bool revertprocessedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-exp_id",       "exp_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-tmp_class_id", "tmp_class_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-class_id",     "class_id", "==");
    PXOPT_COPY_S16(config->args, where,  "-fault",         "fault", "==");
    PXOPT_COPY_S64(config->args, where,  "-exp_id_begin", "exp_id", ">=");
    PXOPT_COPY_S64(config->args, where,  "-exp_id_end", "exp_id", "<=");

    psString query = pxDataGet("regtool_revertprocessedimfile.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        psFree(where);
        return false;
    }

    if (psListLength(where->list)) {
        psString whereClause = psDBGenerateWhereConditionSQL(where, "rawImfile");
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

    psLogMsg("regtool", PS_LOG_INFO, "Updated %d rawImfile", numUpdated);

    return true;
}


static bool updateprocessedimfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);
    PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);

    PXOPT_LOOKUP_S16(fault, config->args, "-fault",   false, false);
    PXOPT_LOOKUP_S16(burntool_state, config->args, "-burntool_state", false, false);
    PXOPT_LOOKUP_S32(set_bytes, config->args, "-set_bytes", false, false);
    PXOPT_LOOKUP_STR(set_md5sum, config->args, "-set_md5sum", false, false);
    PXOPT_LOOKUP_STR(set_state, config->args, "-set_state", false, false);
    PXOPT_LOOKUP_BOOL(set_ignored, config->args, "-set_ignored", false);
    PXOPT_LOOKUP_BOOL(clear_ignored, config->args, "-clear_ignored", false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);
    
    if ((fault == INT16_MAX) && (burntool_state == INT16_MAX) && !set_state && !set_ignored && !clear_ignored) {
        psError(PS_ERR_UNKNOWN, false, "at least one of -fault or -burntool_state -set_ignored -clear_ignored or -set_state must be selected");
        return false;
    }
    if ((fault != INT16_MAX) && (burntool_state != INT16_MAX)) {
        psError(PS_ERR_UNKNOWN, false, "only one of -fault or -burntool_state must be selected");
        return false;
    }
    if (set_ignored && clear_ignored) {
        psError(PS_ERR_UNKNOWN, true, "only one of -set_ignored or -clear_ignored may be selected");
        return false;
    }

    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args, where,  "-exp_id",       "exp_id", "==");
    PXOPT_COPY_STR(config->args, where,  "-class_id",     "class_id", "==");

    if (fault != INT16_MAX) {
        // this is fairly dangerous : can set all if the where is not set... but since exp_id and class_id
        // were required above this is safe
        if (!pxSetFaultCode(config->dbh, "rawImfile", where, fault, 0)) {
            psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
            psFree (where);
            return false;
        }
        psFree (where);
        return(true);
    }
    psFree (where);

    psString setvalues = NULL;
    if (burntool_state != INT16_MAX) {
      psStringAppend(&setvalues,"rawImfile.burntool_state = %d",burntool_state);
    }
    if (set_bytes != INT32_MAX) {
      if (setvalues) {
        psStringAppend(&setvalues,",");
      }
      psStringAppend(&setvalues,"rawImfile.bytes = %d",set_bytes);
    }
    if (set_md5sum) {
      if (setvalues) {
        psStringAppend(&setvalues,",");
      }
      psStringAppend(&setvalues,"rawImfile.md5sum = '%s'",set_md5sum);
    }
    if (set_state) {
      if (setvalues) {
        psStringAppend(&setvalues,",");
      }
      psStringAppend(&setvalues,"rawImfile.data_state = '%s'",set_state);
    }
    if (set_ignored) {
      if (setvalues) {
        psStringAppend(&setvalues,",");
      }
      psStringAppend(&setvalues,"rawImfile.ignored = 1");
    }
    if (clear_ignored) {
      if (setvalues) {
        psStringAppend(&setvalues,",");
      }
      psStringAppend(&setvalues,"rawImfile.ignored = 0");
    }
    if (hostname) {
      if (setvalues) {
	psStringAppend(&setvalues,",");
      }
      psStringAppend(&setvalues,"rawImfile.hostname = '%s'", hostname);
    }
    psString query = pxDataGet("regtool_updateprocessedimfile.sql");
    if (!query) {
      psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
      return false;
    }

    //    printf(query,setvalues,exp_id,class_id);
    if (!p_psDBRunQueryF(config->dbh, query, setvalues, exp_id,class_id)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      psFree(query);
      psFree(setvalues);
      return false;
    }
    psFree(setvalues);
    psFree(query);

    return true;
}

static bool pendingcompressimfileMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");
  PXOPT_COPY_STR(config->args, where, "-class_id", "class_id", "==");

  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  if (whereClause) {
    psStringPrepend(&whereClause, "\n AND ");
  }
  PXOPT_LOOKUP_BOOL(compress, config->args, "-compress", false);
  PXOPT_LOOKUP_BOOL(clean,    config->args, "-clean", false);

  if ((compress && clean) || (!compress && !clean)) {
      psStringAppend(&whereClause, "\n AND ((data_state = 'goto_compressed' AND state = 'goto_compressed')\n  OR (data_state = 'goto_lossy' AND state = 'goto_lossy')) ");
  }
  else if (compress) {
    psStringAppend(&whereClause, "\n AND ((data_state = 'goto_compressed' AND state = 'goto_compressed')) ");
  }
  else if (clean) {
    psStringAppend(&whereClause, "\n AND ((data_state = 'goto_lossy' AND state = 'goto_lossy')) ");
  }
  else {
    psStringAppend(&whereClause, "\n AND ((data_state = 'goto_compressed' AND state = 'goto_compressed')\n  OR (data_state = 'goto_lossy' AND state = 'goto_lossy')) ");
  }

  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  psString query = pxDataGet("regtool_pendingcompressimfile.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  psString limitString = NULL;
  if (limit) {
    limitString = psDBGenerateLimitSQL(limit);
    psStringPrepend(&limitString, "\n");
  }
  //  printf(query,whereClause,limitString);
  if (!p_psDBRunQueryF(config->dbh, query, whereClause, limitString ? limitString : "")) {
    psError(PXTOOLS_ERR_PROG, false, "database error");
    psFree(limitString);
    psFree(query);
    psFree(whereClause);
    psFree(where);
    return(false);
  }
  psFree(limitString);
  psFree(query);
  psFree(whereClause);
  psFree(where);

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
    psTrace("regtool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return true;
  }

  // negate simple so the default is true
  if (!ippdbPrintMetadatas(stdout, output, "regPendingCompressImfile", !simple)) {
    psError(PXTOOLS_ERR_PROG, false, "failed to print array");
    psFree(output);
    return false;
  }

  psFree(output);

  return true;
}


static bool pendingexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    // return only exps that:
    // are not in rawExp
    // have ALL of their imfiles in rawImfile (by count)
    // and have no associated imfiles left in newImfile
    psString query = pxDataGet("regtool_pendingexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // treat limit == 0 as "no limit"
    psString limitString = NULL;
    if (limit) {
        limitString = psDBGenerateLimitSQL(limit);
        // skip past the "hook" comment
        psStringPrepend(&limitString, "\n");
    }

    // 1st arg: where hook, 2nd arg: limit hook
    if (!p_psDBRunQueryF(config->dbh, query, "", limitString ? limitString : "")) {
        // XXX PS_EXIT_PROG_ERROR (incorrect SQL) or SYS_ERROR (database comms)
        psError(PXTOOLS_ERR_PROG, false, "database error");
        psFree(limitString);
        psFree(query);
        return false;
    }
    psFree(limitString);
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
        psTrace("regtool", PS_LOG_INFO, "no rows found");
        psFree(output);
        return true;
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadatas(stdout, output, "regPendingExp", !simple)) {
        psError(PXTOOLS_ERR_PROG, false, "failed to print array");
        psFree(output);
        return false;
    }

    psFree(output);

    return true;
}


static bool addprocessedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    // make sure that the exp_id(s) are ready to be updated based on:
    // exp_id is not in rawExp
    // exp_id is not in newImfile
    // that the correct count of imfiles is in rawImfile

    // required
    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);
    PXOPT_LOOKUP_STR(exp_name, config->args, "-exp_name", true, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_STR(exp_tag, config->args, "-exp_tag", true, false);
    PXOPT_LOOKUP_STR(filelevel, config->args, "-filelevel", true, false);

    // optional
    PXOPT_LOOKUP_TIME(dateobs, config->args, "-dateobs", false, false);
    PXOPT_LOOKUP_STR(exp_type, config->args, "-exp_type", false, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", false, false);
    PXOPT_LOOKUP_STR(state, config->args, "-state", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-end_stage", false, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false);
    PXOPT_LOOKUP_STR(filter, config->args, "-filter", false, false);
    PXOPT_LOOKUP_STR(comment, config->args, "-comment", false, false);
    PXOPT_LOOKUP_STR(obs_mode, config->args, "-obs_mode", false, false);
    PXOPT_LOOKUP_STR(obs_group, config->args, "-obs_group", false, false);
    PXOPT_LOOKUP_F32(airmass, config->args, "-airmass", false, false);
    PXOPT_LOOKUP_F64(ra, config->args, "-ra", false, false);
    PXOPT_LOOKUP_F64(decl, config->args, "-decl", false, false);
    PXOPT_LOOKUP_F32(exp_time, config->args, "-exp_time", false, false);
    PXOPT_LOOKUP_F32(sat_pixel_frac, config->args, "-sat_pixel_frac", false, false);
    PXOPT_LOOKUP_F64(bg, config->args, "-bg", false, false);
    PXOPT_LOOKUP_F64(bg_stdev, config->args, "-bg_stdev", false, false);
    PXOPT_LOOKUP_F64(bg_mean_stdev, config->args, "-bg_mean_stdev", false, false);
    PXOPT_LOOKUP_F64(alt, config->args, "-alt", false, false);
    PXOPT_LOOKUP_F64(az, config->args, "-az", false, false);
    PXOPT_LOOKUP_F32(ccd_temp, config->args, "-ccd_temp", false, false);
    PXOPT_LOOKUP_F64(posang, config->args, "-posang", false, false);
    PXOPT_LOOKUP_F32(m1_x, config->args, "-m1_x", false, false);
    PXOPT_LOOKUP_F32(m1_y, config->args, "-m1_y", false, false);
    PXOPT_LOOKUP_F32(m1_z, config->args, "-m1_z", false, false);
    PXOPT_LOOKUP_F32(m1_tip, config->args, "-m1_tip", false, false);
    PXOPT_LOOKUP_F32(m1_tilt, config->args, "-m1_tilt", false, false);
    PXOPT_LOOKUP_F32(m2_x, config->args, "-m2_x", false, false);
    PXOPT_LOOKUP_F32(m2_y, config->args, "-m2_y", false, false);
    PXOPT_LOOKUP_F32(m2_z, config->args, "-m2_z", false, false);
    PXOPT_LOOKUP_F32(m2_tip, config->args, "-m2_tip", false, false);
    PXOPT_LOOKUP_F32(m2_tilt, config->args, "-m2_tilt", false, false);

    PXOPT_LOOKUP_F32(env_temp, config->args, "-env_temperature", false, false);
    PXOPT_LOOKUP_F32(env_humid, config->args, "-env_humidity", false, false);
    PXOPT_LOOKUP_F32(env_wind, config->args, "-env_wind_speed", false, false);
    PXOPT_LOOKUP_F32(env_dir, config->args, "-env_wind_dir", false, false);
    PXOPT_LOOKUP_F32(teltemp_m1, config->args, "-teltemp_m1", false, false);
    PXOPT_LOOKUP_F32(teltemp_m1cell, config->args, "-teltemp_m1cell", false, false);
    PXOPT_LOOKUP_F32(teltemp_m2, config->args, "-teltemp_m2", false, false);
    PXOPT_LOOKUP_F32(teltemp_spider, config->args, "-teltemp_spider", false, false);
    PXOPT_LOOKUP_F32(teltemp_truss, config->args, "-teltemp_truss", false, false);
    PXOPT_LOOKUP_F32(teltemp_extra, config->args, "-teltemp_extra", false, false);
    PXOPT_LOOKUP_F32(pon_time, config->args, "-pon_time", false, false);
    PXOPT_LOOKUP_F64(user_1, config->args, "-user_1", false, false);
    PXOPT_LOOKUP_F64(user_2, config->args, "-user_2", false, false);
    PXOPT_LOOKUP_F64(user_3, config->args, "-user_3", false, false);
    PXOPT_LOOKUP_F64(user_4, config->args, "-user_4", false, false);
    PXOPT_LOOKUP_F64(user_5, config->args, "-user_5", false, false);
    PXOPT_LOOKUP_STR(object, config->args, "-object", false, false);
    PXOPT_LOOKUP_F32(sun_angle,  config->args, "-sun_angle", false, false);
    PXOPT_LOOKUP_F32(sun_alt,    config->args, "-sun_alt",   false, false);
    PXOPT_LOOKUP_F32(moon_angle, config->args, "-moon_angle", false, false);
    PXOPT_LOOKUP_F32(moon_alt,   config->args, "-moon_alt",   false, false);
    PXOPT_LOOKUP_F32(moon_phase, config->args, "-moon_phase", false, false);
    PXOPT_LOOKUP_STR(label,  config->args, "-label", false, false);
    PXOPT_LOOKUP_STR(data_group,  config->args, "-data_group", false, false);
    PXOPT_LOOKUP_STR(dist_group,  config->args, "-dist_group", false, false);
    PXOPT_LOOKUP_STR(chip_workdir,config->args, "-chip_workdir", false, false);
    PXOPT_LOOKUP_STR(hostname, config->args, "-hostname", false, false);

    // default
    PXOPT_LOOKUP_S16(fault, config->args, "-fault", false, false);

    psString query = pxDataGet("regtool_pendingexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    psString whereClause = NULL;
    {
        // build a query to search by exp_id
        psMetadata *where = psMetadataAlloc();
        if (!psMetadataAddS64(where, PS_LIST_TAIL, "newExp.exp_id", 0, "==", exp_id)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
            psFree(where);
            psFree(query);
            return false;
        }

        whereClause = psDBGenerateWhereConditionSQL(where, NULL);
        psFree(where);
        if (whereClause) {
            // skip past comment "hook"
            psStringPrepend(&whereClause, "\n AND ");
        }
    }

    // 1st arg: where hook, 2nd arg: limit hook
    if (!p_psDBRunQueryF(config->dbh, query, whereClause ? whereClause : "", "")) {
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
        psError(PS_ERR_UNKNOWN, false, "no pending newExp rows found");
        psFree(output);
        return false;
    }
    // sanity check that we only got one row
    if (psArrayLength(output) != 1) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "should have gotten 1 row but %lu rows were returned", psArrayLength(output));
        psFree(output);
        return NULL;
    }

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(output);
        return false;
    }

    // insert the exp into rawExp
    psMetadata *row = output->data[0];
    // convert metadata into a newExp object
    psMetadataConfigPrint(stdout, row);
    newExpRow *newExp = newExpObjectFromMetadata(row);
    psFree(output);
    if (!newExp) {
        psError(PS_ERR_UNKNOWN, false, "this should not happen");
        return false;
    }

    // carry through these values
    // new CLI options overwrite existing values
    workdir   = workdir   ? workdir   : newExp->workdir;
    reduction = reduction ? reduction : newExp->reduction;
    dvodb     = dvodb     ? dvodb     : newExp->dvodb;
    tess_id   = tess_id   ? tess_id   : newExp->tess_id;
    end_stage = end_stage ? end_stage : newExp->end_stage;
    label     = label     ? label     : newExp->label;
    // don't free newExp until just before we return, or these refs will break

    if (!rawExpInsert(config->dbh,
        exp_id,
        exp_name,
        camera,
        telescope,
        dateobs,
        exp_tag,
        exp_type,
        filelevel,
        workdir,
        state,
        reduction,
        dvodb,
        tess_id,
        end_stage,
        filter,
        comment,
        obs_mode,
        obs_group,
        airmass,
        ra,
        decl,
        exp_time,
        sat_pixel_frac,
        bg,
        bg_stdev,
        bg_mean_stdev,
        alt,
        az,
        ccd_temp,
        posang,
        m1_x,
        m1_y,
        m1_z,
        m1_tip,
        m1_tilt,
        m2_x,
        m2_y,
        m2_z,
        m2_tip,
        m2_tilt,
        env_temp,
        env_humid,
        env_wind,
        env_dir,
        teltemp_m1,
        teltemp_m1cell,
        teltemp_m2,
        teltemp_spider,
        teltemp_truss,
        teltemp_extra,
        pon_time,
        user_1,
        user_2,
        user_3,
        user_4,
        user_5,
        object,
        sun_angle,
        sun_alt,
        moon_angle,
        moon_alt,
        moon_phase,
        hostname,
        fault,
        NULL,
        0
    )) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(newExp);
        return false;
    }

    // set the state for the newExp to stop
    // but only if we didn't encounter a fault
    if (fault == 0) {
      if (!pxnewExpSetState(config, exp_id, "stop")) {
        // rollback
        if (!psDBRollback(config->dbh)) {
	  psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to change newExp.state for exp_id: %"PRId64, exp_id);
	psFree(newExp);
        return false;
      }
    }

    // should we stop here and proceed on to the chip stage?
    // NULL for end_stage means go as far as possible
    if (end_stage && psStrcasestr(end_stage, "reg")) {
        // then we are done here
        if (!psDBCommit(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

    psFree(newExp);
        return true;
    }
    // else continue on...

    chip_workdir = chip_workdir ? chip_workdir : workdir;
    // insert an entry into the chipPendingExp table
    // this can only be run as the newExp's state has been set to stop
    if (!pxchipQueueByExpTag(config,
                exp_id,
                             chip_workdir,
                label,
                             data_group,
                             dist_group,
                reduction,
                NULL,       // expgroup
                dvodb,
                tess_id,
                end_stage,
                NULL        // note
    )) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "failed to queue chipPendingExp");
    psFree(newExp);
        return false;
    }

    if (!psDBCommit(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(newExp);
        return false;
    }

    psFree(newExp);
    return true;
}


static bool processedexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    psString query = pxDataGet("regtool_processedexp.sql");
    if (!query) {
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    // XX test this out; need to make this consistent with the list in regtoolConfig.c
    psMetadata *where = psMetadataAlloc();
    PXOPT_COPY_S64(config->args,   where,  "-exp_id", "exp_id", "==");
    PXOPT_COPY_STR(config->args,   where,  "-exp_name", "exp_name", "==");
    PXOPT_COPY_STR(config->args,   where,  "-inst", "camera", "==");
    PXOPT_COPY_STR(config->args,   where,  "-telescope", "telescope", "==");
    PXOPT_COPY_TIME(config->args,  where,  "-dateobs_begin", "dateobs", ">=");
    PXOPT_COPY_TIME(config->args,  where,  "-dateobs_end", "dateobs", "<=");
    PXOPT_COPY_STR(config->args,   where,  "-state", "state", "==");
    PXOPT_COPY_STR(config->args,   where,  "-exp_tag", "exp_tag", "==");
    PXOPT_COPY_STR(config->args,   where,  "-exp_type", "exp_type", "==");
    PXOPT_COPY_STR(config->args,   where,  "-filelevel", "filelevel", "==");
    PXOPT_COPY_STR(config->args,   where,  "-reduction", "reduction", "==");
    PXOPT_COPY_STR(config->args,   where,  "-filter", "filter", "LIKE");
    PXOPT_COPY_F32(config->args,   where,  "-airmass_min", "airmass", ">=");
    PXOPT_COPY_F32(config->args,   where,  "-airmass_max", "airmass", "<");
    PXOPT_COPY_RADEC(config->args, where,  "-ra_min", "ra", ">=");
    PXOPT_COPY_RADEC(config->args, where,  "-ra_max", "ra", "<");
    PXOPT_COPY_RADEC(config->args, where,  "-decl_min", "decl", ">=");
    PXOPT_COPY_RADEC(config->args, where,  "-decl_max", "decl", "<");
    PXOPT_COPY_F32(config->args,   where,  "-exp_time_min", "exp_time", ">=");
    PXOPT_COPY_F32(config->args,   where,  "-exp_time_max", "exp_time", "<");
    PXOPT_COPY_F32(config->args,   where,  "-sat_pixel_frac_min", "sat_pixel_frac", ">=");
    PXOPT_COPY_F32(config->args,   where,  "-sat_pixel_frac_max", "sat_pixel_frac", "<");
    PXOPT_COPY_F64(config->args,   where,  "-bg_min", "bg", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-bg_max", "bg", "<");
    PXOPT_COPY_F64(config->args,   where,  "-bg_stdev_min", "bg_stdev", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-bg_stdev_max", "bg_stdev", "<");
    PXOPT_COPY_F64(config->args,   where,  "-bg_mean_stdev_min", "bg_mean_stdev", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-bg_mean_stdev_max", "bg_mean_stdev", "<");
    PXOPT_COPY_F64(config->args,   where,  "-alt_min", "alt", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-alt_max", "alt", "<");
    PXOPT_COPY_F64(config->args,   where,  "-az_min", "az", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-az_max", "az", "<");
    PXOPT_COPY_F64(config->args,   where,  "-ccd_temp_min", "ccd_temp", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-ccd_temp_max", "ccd_temp", "<");
    PXOPT_COPY_F64(config->args,   where,  "-posang_min", "posang", ">=");
    PXOPT_COPY_F64(config->args,   where,  "-posang_max", "posang", "<");
    PXOPT_COPY_STR(config->args,   where,  "-object", "object", "LIKE");
    PXOPT_COPY_STR(config->args,   where,  "-obs_mode", "obs_mode", "LIKE");
    PXOPT_COPY_STR(config->args,   where,  "-comment", "comment", "LIKE");
    PXOPT_COPY_F32(config->args,   where,  "-sun_angle_min", "sun_angle", ">=");
    PXOPT_COPY_F32(config->args,   where,  "-sun_angle_max", "sun_angle", "<");

    psString where2 = NULL;
    if (!pxmagicAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxmagicAddWhere failed");
        return false;
    }
    if (!pxspaceAddWhere(config, &where2, "rawExp")) {
        psError(psErrorCodeLast(), false, "pxSpaceAddWhere failed");
        return false;
    }

    PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);
    PXOPT_LOOKUP_BOOL(faulted, config->args, "-faulted", false);

    if (psListLength(where->list)) {
      psString whereClause = psDBGenerateWhereConditionSQL(where, "rawExp");
      psStringAppend(&query, " AND %s", whereClause);
      psFree(whereClause);
    }
    psFree(where);

    if (where2) {
        psStringAppend(&query, " %s ", where2);
        psFree(where2);
    }

    if (faulted) {
      // list only faulted rows
      psStringAppend(&query, " %s", "AND rawExp.fault != 0");
    } else {
      // don't list faulted rows
      psStringAppend(&query, " %s", "AND rawExp.fault = 0");
    }

    // treat limit == 0 as "no limit"
    if (limit) {
      psString limitString = psDBGenerateLimitSQL(limit);
      psStringAppend(&query, " %s", limitString);
      psFree(limitString);
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
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
      psTrace("regtool", PS_LOG_INFO, "no rows found");
      psFree(output);
      return true;
    }

    if (psArrayLength(output)) {
      // negative simple so the default is true
      if (!ippdbPrintMetadatas(stdout, output, "rawExp", !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(output);
        return false;
      }
    }

    psFree(output);

    return true;
}

static bool revertprocessedexpMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where,  "-exp_id",       "exp_id", "==");
  PXOPT_COPY_S16(config->args, where,  "-fault",         "fault", "==");
  PXOPT_COPY_S64(config->args, where,  "-exp_id_begin", "exp_id", ">=");
  PXOPT_COPY_S64(config->args, where,  "-exp_id_end", "exp_id", "<=");

  psString query = pxDataGet("regtool_revertprocessedexp.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    psFree(where);
    return false;
  }

  if (psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, "rawExp");
    psStringAppend(&query, " AND %s", whereClause);
    psFree(whereClause);
  } else {
    psFree(where);
    psError(PXTOOLS_ERR_CONFIG, false, "search parameters are required");
    return false;
  }
  psFree(where);

  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return false;
  }
  psFree(query);

  int numUpdated = psDBAffectedRows(config->dbh);

  psLogMsg("regtool", PS_LOG_INFO, "Updated %d rawExp", numUpdated);

  return true;
}


static bool updateprocessedexpMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where,  "-exp_id",       "exp_id", "==");

  PXOPT_LOOKUP_S16(fault, config->args, "-fault", true, false);

  if (!pxSetFaultCode(config->dbh, "rawExp", where, fault, 0)) {
    psError(PS_ERR_UNKNOWN, false, "failed to set set fault flag");
    psFree(where);
    return false;
  }
  psFree(where);

  return true;
}


static bool finishcompressexpMode(pxConfig *config) {

  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");

  // XXX unused PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  psString query = pxDataGet("regtool_finishcompressexp.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  if (where && psListLength(where->list)) {
    psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
    psStringAppend(&query, " %s ", whereClause);
    psFree(whereClause);
  }
  psFree(where);

  if (limit) {
    psString limitString = psDBGenerateLimitSQL(limit);
    psStringAppend(&query, " %s ", limitString);
    psFree(limitString);
  }
  //  printf(query);
  //  return(false);
  if (!p_psDBRunQuery(config->dbh, query)) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    psFree(query);
    return(false);
  }
  psFree(query);

  psArray *output = p_psDBFetchResult(config->dbh);
  if (!output) {
    psError(PS_ERR_UNKNOWN, false, "database error");
    return(false);
  }
  if (!psArrayLength(output)) {
    psTrace("chiptool", PS_LOG_INFO, "no rows found");
    psFree(output);
    return(true);
  }

  for (long i = 0; i < psArrayLength(output); i++) {
    psMetadata *md = output->data[i];

    rawExpRow *row = rawExpObjectFromMetadata(md);
    if (!psDBTransaction(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      return(false);
    }

    if (strcmp(row->state,"goto_compressed") == 0) {
      if (!pxrawExpSetState(config,row->exp_id, "compressed")) {
        psError(PS_ERR_UNKNOWN, false, "failed to set rawExp.state for exp_id: %" PRId64, row->exp_id);
        psFree(row);
        psFree(output);
        return(false);
      }
    }
    else if (strcmp(row->state,"goto_lossy") == 0) {
      if (!pxrawExpSetState(config,row->exp_id, "lossy")) {
        psError(PS_ERR_UNKNOWN, false, "failed to set rawExp.state for exp_id: %" PRId64, row->exp_id);
        psFree(row);
        psFree(output);
        return(false);
      }
    }
    if (!psDBCommit(config->dbh)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      return false;
    }
    psFree(row);
  }

  psFree(output);
  return(true);
}

static bool updatebyqueryMode(pxConfig *config) {
  PS_ASSERT_PTR_NON_NULL(config, false);

  PXOPT_LOOKUP_STR(set_state, config->args, "-set_state", true, false);

  // XX test this out; need to make this consistent with the list in regtoolConfig.c
  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args,   where,  "-exp_id", "rawExp.exp_id", "==");
  PXOPT_COPY_STR(config->args,   where,  "-exp_name", "rawExp.exp_name", "==");
  PXOPT_COPY_STR(config->args,   where,  "-inst", "rawExp.camera", "==");
  PXOPT_COPY_STR(config->args,   where,  "-telescope", "rawExp.telescope", "==");
  PXOPT_COPY_TIME(config->args,  where, "-dateobs_begin", "rawExp.dateobs", ">=");
  PXOPT_COPY_TIME(config->args,  where, "-dateobs_end", "rawExp.dateobs", "<=");
  PXOPT_COPY_STR(config->args,   where,  "-exp_tag", "rawExp.exp_tag", "==");
  PXOPT_COPY_STR(config->args,   where,  "-exp_type", "rawExp.exp_type", "==");
  PXOPT_COPY_STR(config->args,   where,  "-filelevel", "rawExp.filelevel", "==");
  PXOPT_COPY_STR(config->args,   where,  "-state",     "rawExp.state", "==");
  PXOPT_COPY_STR(config->args,   where,  "-reduction", "rawExp.reduction", "==");
  PXOPT_COPY_STR(config->args,   where,  "-filter", "rawExp.filter", "==");
  PXOPT_COPY_F32(config->args,   where,  "-airmass_min", "rawExp.airmass", ">=");
  PXOPT_COPY_F32(config->args,   where,  "-airmass_max", "rawExp.airmass", "<");
  PXOPT_COPY_RADEC(config->args, where,  "-ra_min", "rawExp.ra", ">=");
  PXOPT_COPY_RADEC(config->args, where,  "-ra_max", "rawExp.ra", "<");
  PXOPT_COPY_RADEC(config->args, where,  "-decl_min", "rawExp.decl", ">=");
  PXOPT_COPY_RADEC(config->args, where,  "-decl_max", "rawExp.decl", "<");
  PXOPT_COPY_F32(config->args,   where,  "-exp_time_min", "rawExp.exp_time", ">=");
  PXOPT_COPY_F32(config->args,   where,  "-exp_time_max", "rawExp.exp_time", "<");
  PXOPT_COPY_F32(config->args,   where,  "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
  PXOPT_COPY_F32(config->args,   where,  "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
  PXOPT_COPY_F64(config->args,   where,  "-bg_min", "rawExp.bg", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-bg_max", "rawExp.bg", "<");
  PXOPT_COPY_F64(config->args,   where,  "-bg_stdev_min", "rawExp.bg_stdev", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-bg_stdev_max", "rawExp.bg_stdev", "<");
  PXOPT_COPY_F64(config->args,   where,  "-bg_mean_stdev_min", "rawExp.bg_mean_stdev", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-bg_mean_stdev_max", "rawExp.bg_mean_stdev", "<");
  PXOPT_COPY_F64(config->args,   where,  "-alt_min", "rawExp.alt", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-alt_max", "rawExp.alt", "<");
  PXOPT_COPY_F64(config->args,   where,  "-az_min", "rawExp.az", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-az_max", "rawExp.az", "<");
  PXOPT_COPY_F64(config->args,   where,  "-ccd_temp_min", "rawExp.ccd_temp", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-ccd_temp_max", "rawExp.ccd_temp", "<");
  PXOPT_COPY_F64(config->args,   where,  "-posang_min", "rawExp.posang", ">=");
  PXOPT_COPY_F64(config->args,   where,  "-posang_max", "rawExp.posang", "<");
  PXOPT_COPY_F32(config->args,   where,  "-sun_angle_min", "rawExp.sun_angle", ">=");
  PXOPT_COPY_F32(config->args,   where,  "-sun_angle_max", "rawExp.sun_angle", "<");
  PXOPT_COPY_STR(config->args,   where,  "-object", "rawExp.object", "==");
  PXOPT_COPY_STR(config->args,   where,  "-comment", "rawExp.comment", "LIKE");
  PXOPT_COPY_STR(config->args,   where,  "-obs_mode", "rawExp.obs_mode", "LIKE");


  psString whereClause = psDBGenerateWhereConditionSQL(where, NULL);
  psStringPrepend(&whereClause,"\n AND ");
  PXOPT_LOOKUP_U64(limit, config->args, "-limit", false, false);
  // PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

  psString limitString = NULL;
  if (limit) {
    limitString = psDBGenerateLimitSQL(limit);
    psStringPrepend(&limitString, "\n");
  }
  // Update the imfiles first, because if you select by state, you'll clobber things
  psString query = pxDataGet("regtool_updatebyqueryimfile.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }

  //  printf(query,set_state,whereClause,limitString);
  if (!p_psDBRunQueryF(config->dbh, query, set_state, whereClause, limitString ? limitString : "")) {
    psError(PXTOOLS_ERR_PROG, false, "database error");
    psFree(limitString);
    psFree(query);
    psFree(whereClause);
    psFree(where);
    return(false);
  }

  psFree(query);
  // Now up date the exposure.
  query = pxDataGet("regtool_updatebyquery.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
    return(false);
  }
  if (!p_psDBRunQueryF(config->dbh, query, set_state,whereClause, limitString ? limitString : "")) {
    psError(PXTOOLS_ERR_PROG, false, "database error");
    psFree(limitString);
    psFree(query);
    psFree(whereClause);
    psFree(where);
    return(false);
  }

  psFree(limitString);
  psFree(query);
  psFree(whereClause);
  psFree(where);

  return true;
}



static bool cleardupexpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

// create temp table of exp_ids to be removed
{
    psString query = pxDataGet("regtool_create_dup_table.sql");
    if (!query) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);
}

// populate that table
// XXX note that this query currently doesn't not correctly handle the case
// where there is more than one duplicate
{
    psString query = pxDataGet("regtool_populate_dup_table.sql");
    if (!query) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PXTOOLS_ERR_SYS, false, "failed to retreive SQL statement");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, query)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(query);
        return false;
    }
    psFree(query);
}

    if (!p_psDBRunQuery(config->dbh, "DELETE FROM rawImfile USING rawImfile, duplicate WHERE duplicate.exp_id = rawImfile.exp_id")) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, "DELETE FROM rawExp USING rawExp, duplicate WHERE duplicate.exp_id = rawExp.exp_id")) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, "DELETE FROM newImfile USING newImfile, duplicate WHERE duplicate.exp_id = newImfile.exp_id")) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    if (!p_psDBRunQuery(config->dbh, "DELETE FROM newExp USING newExp, duplicate WHERE duplicate.exp_id = newExp.exp_id")) {
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

static bool checkstatusMode(pxConfig *config)
{
  PS_ASSERT_PTR_NON_NULL(config,false);

  // required
  PXOPT_LOOKUP_STR(class_id, config->args, "-class_id", true, false);
  // Conditionally required
  PXOPT_LOOKUP_STR(dateobs_begin, config->args, "-dateobs_begin", false, false);
  PXOPT_LOOKUP_STR(dateobs_end,   config->args, "-dateobs_end",   false, false);
  PXOPT_LOOKUP_STR(date,          config->args, "-date",          false, false);
  PXOPT_LOOKUP_BOOL(simple,       config->args, "-simple",        false);

  psString query = pxDataGet("regtool_checkstatus.sql");
  if (!query) {
    psError(PXTOOLS_ERR_SYS, false, "failed to retrieve SQL statement");
    return false;
  }
  psString rep = psStringCopy(query);
  psFree(query);
  query = rep;
  psStringSubstitute(&query,class_id,"@CLASS_ID@");
  psStringSubstitute(&query,"ota","XY");
  
  if (!date) {
    if (!dateobs_begin || !dateobs_end) {
      psError(PXTOOLS_ERR_CONFIG, false, "Either -date or -dateobs_begin -dateobs_end is required");
      psFree(query);
      return false;
    }
    psStringAppend(&query," AND summitExp.dateobs >= '%s' AND summitExp.dateobs <= '%s' ",
		   dateobs_begin,dateobs_end);
  }
  else {
    psStringAppend(&query," AND summitExp.dateobs >= '%sT00:00:00' AND summitExp.dateobs <= '%sT23:59:59' ",
		   date,date);
  }

  psStringAppend(&query," ORDER BY summitExp.dateobs ");
  
  if (!p_psDBRunQuery(config->dbh, query)) {
    // XXX PS_EXIT_PROG_ERROR (incorrect SQL) or SYS_ERROR (database comms)
    psError(PXTOOLS_ERR_PROG, false, "database error");
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
    psTrace("regtool", PS_LOG_INFO, "no rows found");
    return true;
  }

  if (!ippdbPrintMetadatas(stdout, output, "regCheckStatus", !simple)) {
    psError(PXTOOLS_ERR_PROG, false, "failed to print array");
    psFree(output);
    return false;
  }

  psFree(output);

  return(true);
}

bool exportrunMode(pxConfig *config)
{
  typedef struct ExportTable {
    char tableName[80];
    char sqlFilename[80];
  } ExportTable;

  int numExportTables = 2;

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  // unused and wrong! PXOPT_LOOKUP_S64(det_id, config->args, "-exp_id", true,  false);
  PXOPT_LOOKUP_STR(outfile, config->args, "-outfile", true,  false);
  PXOPT_LOOKUP_U64(limit,   config->args, "-limit",   false, false);

  FILE *f = fopen (outfile, "w");
  if (f == NULL) {
    psError(PS_ERR_UNKNOWN, false, "failed to open output file");
    return false;
  }

  if (!pxExportVersion(config, f)) {
    psError(PS_ERR_UNKNOWN, false, "failed to write dbversion to output file");
    return false;
  }

  psMetadata *where = psMetadataAlloc();
  PXOPT_COPY_S64(config->args, where, "-exp_id", "exp_id", "==");

  ExportTable tables [] = {
    {"rawExp", "regtool_export_exp.sql"},
    {"rawImfile", "regtool_export_imfile.sql"},
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
      psTrace("regtool", PS_LOG_INFO, "no rows found");
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

  PS_ASSERT_PTR_NON_NULL(config, NULL);

  PXOPT_LOOKUP_STR(infile, config->args, "-infile", true,  false);

  psMetadata *input = psMetadataConfigRead (NULL, &nFail, infile, false);

  if (!pxCheckImportVersion(config, input)) {
    psError(PS_ERR_UNKNOWN, false, "pxCheckImportVersion failed");
    return false;
  }


  fprintf (stdout, "---- input ----\n");
  psMetadataPrint (stderr, input, 1);

  psMetadataItem *item = psMetadataLookup (input, "rawExp");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  psMetadataItem *entry = psListGet (item->data.list, 0);
  assert (entry);
  assert (entry->type == PS_DATA_METADATA);
  rawExpRow *rawExp = rawExpObjectFromMetadata (entry->data.md);
  rawExpInsertObject (config->dbh, rawExp);

  // fprintf (stdout, "---- raw exp ----\n");
  // psMetadataPrint (stderr, entry->data.md, 1);

  item = psMetadataLookup (input, "rawImfile");
  psAssert (item, "entry not in input?");
  psAssert (item->type == PS_DATA_METADATA_MULTI, "entry not multi?");

  for (int i = 0; i < item->data.list->n; i++) {
    psMetadataItem *entry = psListGet (item->data.list, i);
    assert (entry);
    assert (entry->type == PS_DATA_METADATA);
    rawImfileRow *rawImfile = rawImfileObjectFromMetadata (entry->data.md);
    rawImfileInsertObject (config->dbh, rawImfile);

    // fprintf (stdout, "---- row %d ----\n", i);
    // psMetadataPrint (stderr, entry->data.md, 1);
  }

  return true;
}


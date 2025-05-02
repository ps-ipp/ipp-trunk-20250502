#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmConfig.h"
#include "pmConfigCommand.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmConfigCamera.h"
#include "pmDetrendDB.h"
#include "psPipe.h"
#include "psIOBuffer.h"

// ************* detrend select functions **************

//
static void pmDetrendSelectOptionsFree(pmDetrendSelectOptions *options)
{

    if (!options) {
        return;
    }

    psFree(options->camera);
    psFree(options->filter);
    psFree(options->dettype);
    psFree(options->version);

    return;
}

// define basic options for a new detrend database query
pmDetrendSelectOptions *pmDetrendSelectOptionsAlloc(const char *camera, psTime time, pmDetrendType type)
{
    pmDetrendSelectOptions *options = psAlloc(sizeof(pmDetrendSelectOptions));
    psMemSetDeallocator(options, (psFreeFunc) pmDetrendSelectOptionsFree);

    // basic options required by every query
    options->camera = psStringCopy (camera);
    options->time   = time;
    options->type   = type;

    // these other options depend on the type of detrend data
    options->filter   = NULL;
    options->version  = NULL;
    options->dettype  = NULL;
    options->exptime  = 0.0;
    options->airmass  = 0.0;
    options->dettemp  = 0.0;
    options->twilight = 0.0;

    options->exptimeSet  = false; // not selected
    options->airmassSet  = false; // not selected
    options->dettempSet  = false; // not selected
    options->twilightSet = false; // not selected

    return options;
}

static void pmDetrendSelectResultsFree(pmDetrendSelectResults *results)
{

    if (!results) {
        return;
    }

    psFree(results->detID);
    psFree(results->level);

    return;
}

pmDetrendSelectResults *pmDetrendSelectResultsAlloc(void)
{

    pmDetrendSelectResults *results = psAlloc(sizeof(pmDetrendSelectResults));
    psMemSetDeallocator(results, (psFreeFunc) pmDetrendSelectResultsFree);

    results->detID = NULL;
    results->level = NULL;

    return results;
}

psString pmDetrendTypeToString(pmDetrendType type)
{

#define DETREND_STRING_CASE(TYPE) \
  case PM_DETREND_TYPE_##TYPE: \
    return psStringCopy(#TYPE);

    switch (type) {
        DETREND_STRING_CASE(NONE);
        DETREND_STRING_CASE(MASK);
        DETREND_STRING_CASE(BIAS);
        DETREND_STRING_CASE(DARK);
        DETREND_STRING_CASE(FLAT);
        DETREND_STRING_CASE(FLATCORR);
        DETREND_STRING_CASE(SHUTTER);
        DETREND_STRING_CASE(FRINGE);
        DETREND_STRING_CASE(ASTROM);
        DETREND_STRING_CASE(NOISEMAP);
	DETREND_STRING_CASE(VIDEOMASK);
	DETREND_STRING_CASE(VIDEODARK);
	DETREND_STRING_CASE(LINEARITY);
	DETREND_STRING_CASE(NEWNONLIN);
	DETREND_STRING_CASE(AUXMASK);
	DETREND_STRING_CASE(KH_CORRECT);
	DETREND_STRING_CASE(PATTERN_ROW_AMP);
	DETREND_STRING_CASE(PATTERN_DEAD_CELLS);
    default:
        return NULL;
    }
    return NULL;
}

// detselect -camera (camera) -time (time) -type (type) [others]
// returns: (type) (class) (exp_flag) DONE
pmDetrendSelectResults *pmDetrendSelect(const pmDetrendSelectOptions *options, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(options, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psIOBuffer *buffer = NULL;
    psPipe *pipe = NULL;
    psMetadata *answer = NULL;

    int status, exit_status;
    psString line = NULL;
    psString time = psTimeToISO (&options->time);

    char *type = NULL;
    if (options->dettype) {
        type = psMemIncrRefCounter (options->dettype);
    } else {
        type = pmDetrendTypeToString (options->type);
	if (!type) {
	  psError (PM_ERR_CONFIG, false, "unknown detrend type %d", options->type);
	  goto failure_v1; // this goto must NOT free 'results'
	}
    }

    pmDetrendSelectResults *results = pmDetrendSelectResultsAlloc();
    psString realCamera = pmConfigCameraRootName (options->camera);
    psStringAppend(&line, "detselect -search -inst %s -det_type %s -time %s", realCamera, type, time);
    psFree (realCamera);

    // require a filter for certain types of detrends:
    if ((options->type == PM_DETREND_TYPE_FLAT) && !options->filter) {
        psError (PM_ERR_CONFIG, false, "requesting a FLAT-class of detrend without a filter");
        goto failure;
    }
    if ((options->type == PM_DETREND_TYPE_FLATCORR) && !options->filter) {
        psError (PM_ERR_CONFIG, false, "requesting a FLATCORR-class of detrend without a filter");
        goto failure;
    }
    if ((options->type == PM_DETREND_TYPE_FRINGE) && !options->filter) {
        psError (PM_ERR_CONFIG, false, "requesting a FRINGE-class of detrend without a filter");
        goto failure;
    }

    // add the restrictions
    if (options->filter) {
        psStringAppend(&line, " -filter %s", options->filter);
    }
    if (options->version) {
        psStringAppend(&line, " -version %s", options->version);
    }
    if (options->exptimeSet) {
        psStringAppend(&line, " -exp_time %f", options->exptime);
    }
    if (options->airmassSet) {
        psStringAppend(&line, " -airmass %f", options->airmass);
    }
    if (options->dettempSet) {
        psStringAppend(&line, " -airmass %f", options->dettemp);
    }
    if (options->twilightSet) {
        psStringAppend(&line, " -airmass %f", options->twilight);
    }

    if (!pmConfigDatabaseCommand(&line, config)) {
        psError (PS_ERR_IO, false, "error building detrend command %s", line);
        goto failure;
    }

    if (!pmConfigTraceCommand(&line)) {
        psError (PS_ERR_IO, false, "error building detrend command %s", line);
        goto failure;
    }

    psTrace("psModules.detrend", 5, "running %s", line);

    // use psPipe to exec the command, wait for response
    buffer = psIOBufferAlloc (512);
    pipe = psPipeOpen (line);
    if (!pipe) {
        psError (PS_ERR_IO, false, "error calling command %s", line);
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect command:\n %s\n", line);
        goto failure;
    }

    status = psIOBufferReadEmpty (buffer, 2000, pipe->fd_stdout);
    if (!status) {
        psError (PS_ERR_IO, false, "detselect is not responding");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect command:\n %s\n", line);
        goto failure;
    }
    exit_status = psPipeClose (pipe);
    if (exit_status) {
        psError (PS_ERR_IO, false, "error running detselect");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect command:\n %s\n", line);
        goto failure;
    }

    if (!buffer->data || strlen(buffer->data) == 0) {
        psError(PS_ERR_IO, true, "Unable to find suitable detrend");
        psLogMsg("psModules.detrend", PS_LOG_ERROR, "detselect command:\n %s\n", line);
        goto failure;
    }

    psTrace("psModules.detrend", 5, "got answer: %s\n", buffer->data);

    unsigned int nFail = 0;
    answer = psMetadataConfigParse (NULL, &nFail, buffer->data, false);
    if (!answer) {
        psError(PS_ERR_IO, false, "failed to parse response from detselect\n");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response (%d bytes):\n %s\n", buffer->n, buffer->data);
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect command:\n %s\n", line);
        goto failure;
    }

    psMetadata *md = psMetadataLookupPtr (NULL, answer, "detExp");
    if (!md) {
        psError(PS_ERR_IO, false, "detselect response is missing 'detExp' Metadata\n");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response:\n %s\n", buffer->data);
        goto failure;
    }

    bool mdstatus;
    int detID = psMetadataLookupS32 (&mdstatus, md, "det_id");
    if (!mdstatus) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find det_id in output from detselect.");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response:\n %s\n", buffer->data);
        goto failure;
    }
    int iteration  = psMetadataLookupS32 (&mdstatus, md, "iteration");
    if (!mdstatus) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find iteration in output from detselect.");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response:\n %s\n", buffer->data);
        goto failure;
    }
    psString fileLevel = psMetadataLookupStr(&mdstatus, md, "filelevel");
    if (!mdstatus || !fileLevel || strlen(fileLevel) == 0) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find filelevel in output from detselect.");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response:\n %s\n", buffer->data);
        goto failure;
    }

    results->detID = NULL; // it should be NULL already from the Alloc above
    psStringAppend (&results->detID, " -det_id %d -iteration %d ", detID, iteration);
    results->level = psMemIncrRefCounter(fileLevel);

    psTrace("psModules.detrend", 5, "generated detID %s\n", results->detID);

    psFree (answer);
    psFree (buffer);
    psFree (pipe);
    psFree (line);
    psFree (time);
    psFree (type);
    return results;

 failure:
    psFree (results);
 failure_v1:
    psFree (answer);
    psFree (pipe);
    psFree (buffer);
    psFree (line);
    psFree (type);
    psFree (time);
    return NULL;
}

// ************* detrend file functions **************

// detselect -select -detID (detID) -classID (classID)
// returns: (detID) (classID) (filename) DONE
char *pmDetrendFile (const char *detID, const char *classID, const pmConfig *config)
{
    unsigned int nFail;

    PS_ASSERT_PTR_NON_NULL(detID, NULL);

    bool status;
    psString line = NULL;
    psArray *array = NULL;

    // generate the detselect command
    psStringAppend (&line, "detselect -select %s", detID);
    if (classID && strlen(classID) > 0) {
        psStringAppend(&line, " -class_id %s", classID);
    }
    pmConfigDatabaseCommand(&line, config);
    pmConfigTraceCommand(&line);
    psTrace("psModules.detrend", 5, "running %s", line);

    // use psPipe to exec the command, wait for response
    psIOBuffer *buffer = psIOBufferAlloc (512);
    psPipe *pipe = psPipeOpen (line);
    if (!pipe) {
        psError (PS_ERR_IO, false, "error calling command %s", line);
        goto failure;
    }

    // timeout somewhat longer than 2sec.  this could still be too short....
    status = psIOBufferReadEmpty (buffer, 2000, pipe->fd_stdout);
    if (!status) {
        psError (PS_ERR_IO, false, "detselect is not responding");
        goto failure;
    }
    status = psPipeClose (pipe);
    if (status) {
        psError (PS_ERR_IO, false, "error running detselect");
        goto failure;
    }

    psTrace("psModules.detrend", 5, "got answer: %s\n", buffer->data);

    if (!buffer->n) {
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response (%d bytes):\n %s\n", buffer->n, buffer->data);
        psError(PS_ERR_IO, true, "no matching detrend data in database\n");
        goto failure;
    }

    psMetadata *answer = psMetadataConfigParse (NULL, &nFail, buffer->data, false);
    if (!answer) {
        psError(PS_ERR_IO, false, "failed to parse response from detselect\n");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response (%d bytes):\n %s\n", buffer->n, buffer->data);
        goto failure;
    }
    psMetadataItem *item = psMetadataLookup (answer, "detNormalizedImfile");
    if ((item->type == PS_DATA_METADATA_MULTI) && (item->data.list->n > 1)) {
        psError(PS_ERR_IO, false, "detselect returned too many files\n");
        goto failure;
    }

    psMetadata *md = psMetadataLookupPtr (NULL, answer, "detNormalizedImfile");
    if (!md) {
        psError(PS_ERR_IO, false, "detselect response is missing 'detNormalizedImfile' Metadata\n");
        psLogMsg ("psModules.detrend", PS_LOG_ERROR, "detselect response:\n %s\n", buffer->data);
        goto failure;
    }

    char *result = psStringCopy (psMetadataLookupStr (NULL, md, "uri"));
    psTrace("psModules.detrend", 5, "detrend file: %s\n", result);

    // XXX: A somewhat hacked bit of code to force the analysis to use a specific version of the detrend file
    char *is_nebulous = strstr(result,"neb://");
    if (is_nebulous) { // This file matches the nebulous string
      psString truncated = psStringCopy(is_nebulous + 6);
      //      printf("A: %s %s\n",result,truncated);

      psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes,"PPIMAGE");
      psMetadata *detloc = psMetadataLookupMetadata(&status, recipe, "DETREND.LOCATIONS");

      if (detloc) { // This exists, so we have the information.
	psString location = psMetadataLookupStr(&status, detloc, classID);
	//	printf("B: %s %s\n", classID,location);
	if (location) { // Found a location
	  psString ext_key = strstr(truncated,"/"); // Find the first '/'
	  //	  printf("C: %s\n", ext_key);
	  if (ext_key) {
	    psString copyBecauseReasons = psStringCopy(ext_key);
	    psStringPrepend(&copyBecauseReasons,"neb://%s",location);
	    //	    printf("D: %s\n",copyBecauseReasons);
	    psFree(result); // Yes, because we are going to reallocate a clean copy of cBR to this object.
	    result = psStringCopy(copyBecauseReasons);
	    //	    psFree(ext_key); // No, because this is just a pointer into some other memory space.
	    psTrace("psModules.detrend", 5, "altered detrend file: %s\n", result);
	    psFree(copyBecauseReasons); // Yes, because we copied this to the output object, and so we don't need this any more.
	  }
	  //	  psFree(location);  // Don't free this because the string lookup isn't a copy, so it deletes it from the config structure entirely.
	}
      }
      //      psFree(is_nebulous);  // No, because this is just a pointer into some other memory space.
      psFree(truncated);
    }
	  
    
    psFree (answer);
    psFree (pipe);
    psFree (buffer);
    psFree (line);
    return result;

failure:
    psFree (array);
    psFree (pipe);
    psFree (buffer);
    psFree (line);
    return NULL;
}

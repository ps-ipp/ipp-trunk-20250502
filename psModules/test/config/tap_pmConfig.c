/** @file tst_pmConfig.c
 *
 *  @brief Contains the tests for pmConfig.c:
 *
 * This code will test the pmConfig() routine.
*       pmConfigReadParamsSet
*       pmConfigAlloc		
*       pmConfigSet
*       pmConfigDone
*       pmConfigFileRead
*        pmConfigValidateCameraFormat
*        pmConfigCameraFormatFromHeader
*        pmConfigCameraByName
?        pmConfigDB
*        pmConfigConformHeader
-        pmConfigFileSets
-        pmConfigFileSetsMD
*        pmConfigConvertFilename
*        pmConfigRead
 * XXXX: Must determine what to do with NULL arguments, then test it.
 *
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-06-10 20:58:28 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
#define ERR_TRACE_LEVEL         10
#define	VERBOSE			0

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel(".", 0);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(181);
    
    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Tests for pmConfigReadParamsSet()
    {
        psMemId id = psMemGetId();
        bool oldReadCameraConfig = pmConfigReadParamsSet(false);
        ok(oldReadCameraConfig == true, "pmConfigReadParamsSet() returned old value correctly");
        oldReadCameraConfig = pmConfigReadParamsSet(true);
        ok(oldReadCameraConfig == false, "pmConfigReadParamsSet() returned old value correctly");
        oldReadCameraConfig = pmConfigReadParamsSet(true);
        ok(oldReadCameraConfig == true, "pmConfigReadParamsSet() returned new value correctly");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    
    
    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Tests for pmConfigAlloc()
    // XXX: Add a MemCheckConfig() function to verify the return type
    {
        psMemId id = psMemGetId();

        pmConfig *myConfig = pmConfigAlloc();
        ok(myConfig != NULL, "pmConfigAlloc() returned non-NULL");
        skip_start(myConfig == NULL, 10, "skipping tests because pmConfigAlloc() returned NULL");
        ok(myConfig->site == NULL, "pmConfigAlloc() initialized pmConfig->site properly");
        ok(myConfig->camera == NULL, "pmConfigAlloc() initialized pmConfig->camera properly");
        ok(myConfig->cameraName == NULL, "pmConfigAlloc() initialized pmConfig->cameraName properly");
        ok(myConfig->format == NULL, "pmConfigAlloc() initialized pmConfig->format properly");
        ok(myConfig->formatName == NULL, "pmConfigAlloc() initialized pmConfig->formatName properly");
        ok(myConfig->recipes != NULL && psMemCheckMetadata(myConfig->recipes),
            "pmConfigAlloc() initialized pmConfig->recipes properly");
        ok(myConfig->recipesRead == PM_RECIPE_SOURCE_NONE, "pmConfigAlloc() initialized pmConfig->recipesRead properly");
        ok(myConfig->recipeSymbols != NULL && psMemCheckMetadata(myConfig->recipeSymbols),
            "pmConfigAlloc() initialized pmConfig->recipesSymbols properly");
        ok(myConfig->arguments != NULL && psMemCheckMetadata(myConfig->arguments),
            "pmConfigAlloc() initialized pmConfig->arguments properly");
        ok(myConfig->database == NULL, "pmConfigAlloc() initialized pmConfig->database properly");
        ok(myConfig->defaultRecipe == NULL, "pmConfigAlloc() initialized pmConfig->defaultRecipe properly");

        skip_end();
        psFree(myConfig);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    
    
    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigSet()
    // Test with NULL input
    // XX: Test with empty string?
    {
        psMemId id = psMemGetId();
        pmConfigSet(NULL);
        ok(true, "called pmConfigSet() with NULL input pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigSet(NULL) created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    
    
    // Test with acceptable config file name, with a simple path
    {
        pmConfigDone();
        psMemId id = psMemGetId();
        psMetadata *config = NULL;
        bool rc = pmConfigFileRead(&config, "SampleIPPConfig2", "DESCRIPTION");
        ok(rc == false, "pmConfigFileRead() returned FALSE before calling pmConfigSet()");
        pmConfigSet("../dataFiles/path2:dataFiles/path2");
        rc = pmConfigFileRead(&config, "SampleIPPConfig2", "DESCRIPTION");
        ok(rc == true, "pmConfigFileRead() returned TRUE after calling pmConfigSet() (test 1)");
        psFree(config);
        pmConfigDone();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with acceptable config file name, with a compound path
    {
        pmConfigDone();
        psMemId id = psMemGetId();
        psMetadata *config = NULL;
        bool rc = pmConfigFileRead(&config, "SampleIPPConfig2", "DESCRIPTION");
        ok(rc == false, "pmConfigFileRead() returned FALSE before calling pmConfigSet()");
        pmConfigSet("junk:../dataFiles/path2:dataFiles/path2:data/path5");
        rc = pmConfigFileRead(&config, "SampleIPPConfig2", "DESCRIPTION");
        ok(rc == true, "pmConfigFileRead() returned TRUE after calling pmConfigSet() (test 2)");
        psFree(config);
        pmConfigDone();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigDone(): ensure it frees memory allocated by pmConfigSet();
    // This also ensures that pmConfigSet() psFrees memory between calls
    {
        psMemId id = psMemGetId();
        pmConfigSet("junk01");
        pmConfigSet("junk:../dataFiles/path2:dataFiles/path2:data/path5");
        pmConfigSet("junk02");
        pmConfigDone();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks: pmConfigDone()");
    }



    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigFileRead()
    // Test with NULL config pointer
    // XXX: There's a memory leak if the first argument points to data that's
    // already allocated (pmConfigFileRead() frees the first argument)
    {
        psMemId id = psMemGetId();
        psString name = "foo1";
        psString desc = "foo2";
        bool rc = pmConfigFileRead(NULL, name, desc);
        ok(rc == false, "pmConfigFileRead() returned FALSE with NULL config pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigFileRead() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL name pointer
    // XXX: Test with empty name string?
    {
        psMemId id = psMemGetId();
        psMetadata *config = psMetadataAlloc();
        psString desc = "foo2";
        bool rc = pmConfigFileRead(&config, NULL, desc);
        ok(rc == false, "pmConfigFileRead() returned FALSE with NULL name pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileRead() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL desc pointer
    // XXX: Test with empty desc string and otherwise acceptable inputs.
    // That generated errors elsewhere.
    {
        psMemId id = psMemGetId();
        psMetadata *config = psMetadataAlloc();
        psString name = "foo1";
        bool rc = pmConfigFileRead(&config, name, NULL);
        ok(rc == false, "pmConfigFileRead() returned FALSE with NULL desc pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileRead() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test with acceptable config file name, no paths
    {
        psMemId id = psMemGetId();
        psMetadata *config = NULL;
        psString name = "../dataFiles/SampleIPPConfig";
        psString name2 = "dataFiles/SampleIPPConfig";
        psString desc = "DESCRIPTION";
        // First try to read data from ../dataFiles, then try dataFiles.
        bool rc = pmConfigFileRead(&config, name, desc);
        if (!rc) {
            rc = pmConfigFileRead(&config, name2, desc);
	}
        ok(rc == true, "pmConfigFileRead() returned TRUE with acceptable inputs");

        // Test the several arbitrary config file strings.
        psS32 tmpInt = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_S32");
        ok(rc == true && tmpInt == 20, "pmConfigFileRead() properly set metadata (S32)");
        psF32 tmpFloat = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F32");
        ok(rc == true && tmpFloat == 21.0, "pmConfigFileRead() properly set metadata (F32)");
        psF64 tmpDub = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F64");
        ok(rc == true && tmpDub == 22.0, "pmConfigFileRead() properly set metadata (F64)");
        psBool tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_T");
        ok(rc == true && tmpBool == true, "pmConfigFileRead() properly set metadata (bool)");
        tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_F");
        ok(rc == true && tmpBool == false, "pmConfigFileRead() properly set metadata (bool)");
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with acceptable config file name, no paths
    {
        psMemId id = psMemGetId();
        psMetadata *config = NULL;
        psString name = "../dataFiles/SampleIPPConfig";
        psString name2 = "dataFiles/SampleIPPConfig";
        psString desc = "DESCRIPTION";
        bool rc = pmConfigFileRead(&config, name, desc);
        if (!rc) {
            rc = pmConfigFileRead(&config, name2, desc);
	}
        ok(rc == true, "pmConfigFileRead() returned TRUE with acceptable inputs");

        // Test the several arbitrary config file strings.
        psS32 tmpInt = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_S32");
        ok(rc == true && tmpInt == 20, "pmConfigFileRead() properly set metadata (S32)");
        psF32 tmpFloat = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F32");
        ok(rc == true && tmpFloat == 21.0, "pmConfigFileRead() properly set metadata (F32)");
        psF64 tmpDub = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F64");
        ok(rc == true && tmpDub == 22.0, "pmConfigFileRead() properly set metadata (F64)");
        psBool tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_T");
        ok(rc == true && tmpBool == true, "pmConfigFileRead() properly set metadata (bool)");
        tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_F");
        ok(rc == true && tmpBool == false, "pmConfigFileRead() properly set metadata (bool)");
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with acceptable config file name, no paths
    {
        psMemId id = psMemGetId();
        psMetadata *config = NULL;
        psString name = "../dataFiles/SampleIPPConfig";
        psString name2 = "dataFiles/SampleIPPConfig";
        psString desc = "DESCRIPTION";
        bool rc = pmConfigFileRead(&config, name, desc);
        if (!rc) {
            rc = pmConfigFileRead(&config, name2, desc);
	}
        ok(rc == true, "pmConfigFileRead() returned TRUE with acceptable inputs");

        // Test the several arbitrary config file strings.
        psS32 tmpInt = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_S32");
        ok(rc == true && tmpInt == 20, "pmConfigFileRead() properly set metadata (S32)");
        psF32 tmpFloat = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F32");
        ok(rc == true && tmpFloat == 21.0, "pmConfigFileRead() properly set metadata (F32)");
        psF64 tmpDub = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F64");
        ok(rc == true && tmpDub == 22.0, "pmConfigFileRead() properly set metadata (F64)");
        psBool tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_T");
        ok(rc == true && tmpBool == true, "pmConfigFileRead() properly set metadata (bool)");
        tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_F");
        ok(rc == true && tmpBool == false, "pmConfigFileRead() properly set metadata (bool)");
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with acceptable config file name, with path
    // Note: This also tests the pmConfigSet() function.
    {
        psMemId id = psMemGetId();
        psMetadata *config = NULL;
        psString name = "SampleIPPConfig2";
        psString desc = "DESCRIPTION";
        pmConfigSet("../dataFiles/path2:dataFiles/path2");
        bool rc = pmConfigFileRead(&config, name, desc);
        ok(rc == true, "pmConfigFileRead() returned TRUE using paths");
        // Test the several arbitrary config file strings.
        psS32 tmpInt = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_S32");
        ok(rc == true && tmpInt == 20, "pmConfigFileRead() properly set metadata (S32)");
        psF32 tmpFloat = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F32");
        ok(rc == true && tmpFloat == 21.0, "pmConfigFileRead() properly set metadata (F32)");
        psF64 tmpDub = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_F64");
        ok(rc == true && tmpDub == 22.0, "pmConfigFileRead() properly set metadata (F64)");
        psBool tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_T");
        ok(rc == true && tmpBool == true, "pmConfigFileRead() properly set metadata (bool)");
        tmpBool = psMetadataLookupS32(&rc, config, "ARBITRARY_STRING_BOOL_F");
        ok(rc == true && tmpBool == false, "pmConfigFileRead() properly set metadata (bool)");
        psFree(config);
        pmConfigDone();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigValidateCameraFormat()
    // Test with NULL valid pointer
    // XXX: This is commented out because of a seg-fault
    if (0) {
        psMemId id = psMemGetId();
        psMetadata *cameraFormat = psMetadataAlloc();
        psMetadata *header = psMetadataAlloc();
        bool rc = pmConfigValidateCameraFormat(NULL, cameraFormat, header);
        ok(rc == false, "pmConfigValidateCameraFormat() returned FALSE with NULL valid pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(cameraFormat);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL cameraFormat pointer
    {
        psMemId id = psMemGetId();
        bool valid;
        psMetadata *cameraFormat = psMetadataAlloc();
        psMetadata *header = psMetadataAlloc();
        bool rc = pmConfigValidateCameraFormat(&valid, NULL, header);
        ok(rc == false, "pmConfigValidateCameraFormat() returned FALSE with NULL valid pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(cameraFormat);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL header pointer
    {
        psMemId id = psMemGetId();
        bool valid;
        psMetadata *cameraFormat = psMetadataAlloc();
        psMetadata *header = psMetadataAlloc();
        bool rc = pmConfigValidateCameraFormat(&valid, cameraFormat, NULL);
        ok(rc == false, "pmConfigValidateCameraFormat() returned FALSE with NULL valid pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(cameraFormat);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test with acceptable input
    // psTraceSetLevel("psModules.config", 10);
    {
        psMemId id = psMemGetId();
        bool valid;
        psMetadata *header = psMetadataAlloc();
        psMetadata *camera = psMetadataAlloc();
        // Test with unitialized camera metadata
        bool rc = pmConfigValidateCameraFormat(&valid, camera, header);
        ok(rc == false && valid == false, "pmConfigValidateCameraFormat() returned FALSE with uninitialized psMetadata");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_UNKNOWN == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_UNKNOWN error");
        psFree(tmpErr);
        psErrorClear();
        psFree(camera);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with acceptable input
    // psTraceSetLevel("psModules.config", 10);
    // Add a test that sets the metadata value incorrectly
    {
        psMemId id = psMemGetId();
        bool valid;
        psMetadata *header = psMetadataAlloc();
        psMetadata *camera = NULL;
        bool rc = pmConfigFileRead(&camera, "../dataFiles/camera0/format0.config", "Camera 0 Config File");
        if (!rc) {
            rc = pmConfigFileRead(&camera, "dataFiles/camera0/format0.config", "Camera 0 Config File");
	}
        ok(rc == true, "pmConfigFileRead() read ../dataFiles/camera0/format0.config");
        psMetadataAddStr(header, PS_LIST_TAIL, "F0_KEY0", 0, "", "string20");
        rc = pmConfigValidateCameraFormat(&valid, camera, header);
        ok(rc == true, "pmConfigValidateCameraFormat() returned FALSE with acceptable input, wrong values");
        ok(valid == false, "pmConfigValidateCameraFormat() rejected header correctly");
        psMetadataAddStr(header, PS_LIST_TAIL, "F0_KEY1", 0, "", "string21");
        rc = pmConfigValidateCameraFormat(&valid, camera, header);
        ok(rc == true, "pmConfigValidateCameraFormat() returned FALSE with acceptable input, wrong values");
        ok(valid == false, "pmConfigValidateCameraFormat() rejected header correctly");
        psMetadataAddS32(header, PS_LIST_TAIL, "F0_KEY2", 0, "", 20);
        rc = pmConfigValidateCameraFormat(&valid, camera, header);
        ok(rc == true, "pmConfigValidateCameraFormat() returned TRUE with acceptable input, correct values");
        ok(valid == true, "pmConfigValidateCameraFormat() accepted header correctly");
        psFree(camera);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigCameraFormatFromHeader()
    //
    // Given a FITS header, check it against all known cameras (unless we
    // already know which camera, from pmConfigRead) and all known formats
    // for those cameras in order to identify which is appropriate.
    //
    // psMetadata *pmConfigCameraFormatFromHeader(
    //    pmConfig *config,
    //    const psMetadata *header,
    //    bool readRecipes)
    // Test with NULL config pointer
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        psMetadata *header = psMetadataAlloc();
        bool readRecipes = false;
        psMetadata *camera = pmConfigCameraFormatFromHeader(NULL, header, readRecipes);
        ok(camera == NULL, "pmConfigCameraFormatFromHeader() returned NULL with NULL config pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigCameraFormatFromHeader() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL header pointer
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        psMetadata *header = psMetadataAlloc();
        bool readRecipes = false;
        psMetadata *camera = pmConfigCameraFormatFromHeader(config, NULL, readRecipes);
        ok(camera == NULL, "pmConfigCameraFormatFromHeader() returned NULL with NULL header pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigCameraFormatFromHeader() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test with acceptable inputs
    if (1) {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        psMetadata *header = psMetadataAlloc();
        bool readRecipes = false;
        // Load a sample config file
        bool rc = pmConfigFileRead(&config->site, "../dataFiles/SampleIPPConfig", "DESCRIPTION");
        if (!rc) {
            rc = pmConfigFileRead(&config->site, "dataFiles/SampleIPPConfig", "DESCRIPTION");
	}
        ok(rc == true && !config, "pmConfigFileRead() was successful");
        // Set metadata tags for a simulated FITS header
        psMetadataAddStr(header, PS_LIST_TAIL, "F0_KEY0", 0, "", "string20");
        psMetadataAddStr(header, PS_LIST_TAIL, "F0_KEY1", 0, "", "string21");
        psMetadataAddS32(header, PS_LIST_TAIL, "F0_KEY2", 0, "", 20);
        psMetadata *camera = pmConfigCameraFormatFromHeader(config, header, readRecipes);

        ok(camera != NULL,
          "pmConfigCameraFormatFromHeader() returned non-NULL with acceptable inputs");
        ok(camera != NULL && psMemCheckMetadata(camera),
          "pmConfigCameraFormatFromHeader() returned non-NULL and correct type with acceptable inputs");
        psFree(config);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigCameraByName()
    //    psMetadata *pmConfigCameraByName(pmConfig *config, const char *cameraName)
    // Test with NULL config pointer
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        psString cameraName = "CameraName";
        psMetadata *camera = pmConfigCameraByName(NULL, cameraName);
        ok(camera == NULL, "pmConfigCameraByName() returned NULL with NULL config pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigCameraByName() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL camera Name
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        // psString cameraName = "CameraName";
        psMetadata *camera = pmConfigCameraByName(config, NULL);
        ok(camera == NULL, "pmConfigCameraByName() returned NULL with NULL camera name");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigCameraByName() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psTraceSetLevel("err", 10);
    // Test with acceptable data
    if (1) {
        psMemId id = psMemGetId();
        pmConfigSet("data");
        pmConfig *config = pmConfigAlloc();
        bool rc = pmConfigFileRead(&config->site, "../dataFiles/SampleIPPConfig", "DESCRIPTION");
        if (!rc) {
            rc = pmConfigFileRead(&config->site, "dataFiles/SampleIPPConfig", "DESCRIPTION");
	}
        ok(rc == true, "pmConfigFileRead() was successful");
        psString cameraName = "CAMERA0";
        psMetadata *camera = pmConfigCameraByName(config, cameraName);
        ok(camera != NULL && psMemCheckMetadata(camera),
          "pmConfigCameraByName() returned non-NULL with acceptable");
        char *tmpStr = psMetadataLookupStr(&rc, camera, "ID");
        ok(tmpStr != NULL && !strcmp(tmpStr, "CAMERA0"), "pmConfigCameraByName() returned non-NULL(CAMERA0)");
        pmConfigDone();
        psFree(config);
        psFree(tmpStr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigDB()
    // psDB *pmConfigDB(pmConfig *config)
    // Test with NULL config pointer
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        psDB *db = pmConfigDB(NULL);
        ok(db == NULL, "pmConfigDB() returned NULL with NULL config pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigDB()() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL config->site pointer
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        config->site = NULL;
        psDB *db = pmConfigDB(config);
        ok(db == NULL, "pmConfigDB() returned NULL with NULL config->site pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigDB()() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test with acceptable data
    // XXX: Not tested.
    if (1) {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        config->site = psMetadataAlloc();
        bool rc = pmConfigFileRead(&config->site, "../dataFiles/SampleIPPConfig", "DESCRIPTION");
        if (!rc) {
            rc = pmConfigFileRead(&config->site, "dataFiles/SampleIPPConfig", "DESCRIPTION");
	}
        ok(rc == true, "pmConfigFileRead() was successful");
        skip_start(rc == false, 1, "Skipping pmConfigDB() tests because we could not read config file");
        psDB *db = pmConfigDB(config);
        ok(db != NULL, "pmConfigDB() returned non NULL with acceptable data");
        psFree(db);
        skip_end();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigConformHeader()
    // Test with NULL header pointer
    {
        psMemId id = psMemGetId();
        psMetadata *header = psMetadataAlloc();
        psMetadata *format = psMetadataAlloc();
        bool rc = pmConfigConformHeader(NULL, (const psMetadata *) format);
        ok(rc == false, "pmConfigConformHeader() returned FALSE with NULL header pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigConformHeader() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(header);
        psFree(format);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL format pointer
    {
        psMemId id = psMemGetId();
        psMetadata *header = psMetadataAlloc();
        psMetadata *format = psMetadataAlloc();
        bool rc = pmConfigConformHeader(header, NULL);
        ok(rc == false, "pmConfigConformHeader() returned FALSE with NULL header pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigConformHeader() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(header);
        psFree(format);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test with acceptable data
    {
        psMemId id = psMemGetId();
        bool valid;
        psMetadata *header = psMetadataAlloc();
        psMetadata *format = psMetadataAlloc();
        bool rc = pmConfigFileRead(&format, "../dataFiles/camera0/format0.config", "Camera 0 Config File");
        if (!rc) {
            rc = pmConfigFileRead(&format, "dataFiles/camera0/format0.config", "Camera 0 Config File");
	}
        ok(rc == true, "pmConfigFileRead() read camera format correctly");

        // First ensure that the header is not accepted
        rc = pmConfigValidateCameraFormat(&valid, format, header);
        ok(rc == true && valid == false, "pmConfigValidateCameraFormat() rejected header with uninitialized psMetadata");

        // Now call pmConfigConformHeader() and ensure that the header is accepted
        rc = pmConfigConformHeader(header, format);
        ok(rc == true, "pmConfigConformHeader() returned TRUE");
        rc = pmConfigValidateCameraFormat(&valid, format, header);
        ok(rc == true, "pmConfigValidateCameraFormat() returned TRUE");
        ok(valid == true, "pmConfigValidateCameraFormat() validated camera format");
        psFree(header);
        psFree(format);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigFileSets()
    // test with NULL argc pointer
    {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        char *filename = "FILENAME";
        char *list = "LIST";
        psArray *array = pmConfigFileSets(NULL, str, filename, list);
        if (!array) {
            str[2] = "dataFiles/SampleIPPConfig";
            array = pmConfigFileSets(NULL, str, filename, list);
	}
        ok(array == NULL, "pmConfigFileSets() returned NULL with NULL argc pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigFileSets() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with negative argc pointer
    {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = -1;
        char *filename = "FILENAME";
        char *list = "LIST";
        psArray *array = pmConfigFileSets(&myArgc, str, filename, list);
        if (!array) {
            str[2] = "dataFiles/SampleIPPConfig";
            array = pmConfigFileSets(&myArgc, str, filename, list);
	}
        ok(array == NULL, "pmConfigFileSets() returned NULL with negative argc pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileSets() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL argv pointer
    {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *filename = "FILENAME";
        char *list = "LIST";
        psArray *array = pmConfigFileSets(&myArgc, NULL, filename, list);
        ok(array == NULL, "pmConfigFileSets() returned NULL with NULL argv pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigFileSets() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL filename pointer
    {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *list = "LIST";
        psArray *array = pmConfigFileSets(&myArgc, str, NULL, list);
        if (!array) {
            str[2] = "dataFiles/SampleIPPConfig";
            array = pmConfigFileSets(&myArgc, str, NULL, list);
	}
        ok(array == NULL, "pmConfigFileSets() returned NULL with NULL filename pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileSets() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL list pointer
    {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *filename = "FILENAME";
        psArray *array = pmConfigFileSets(&myArgc, str, filename, NULL);
        if (!array) {
            str[2] = "dataFiles/SampleIPPConfig";
            array = pmConfigFileSets(&myArgc, str, filename, NULL);
	}
        ok(array == NULL, "pmConfigFileSets() returned NULL with NULL list pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileSets() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with acceptable data
    if (0) {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *filename = "FILENAME";
        char *list = "LIST";
        psArray *array = pmConfigFileSets(&myArgc, str, filename, list);
        if (!array) {
            str[2] = "dataFiles/SampleIPPConfig";
            array = pmConfigFileSets(&myArgc, str, filename, list);
	}
        ok(array != NULL && psMemCheckArray(array),
          "pmConfigFileSets() returned non-NULL with acceptable input data");
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigFileSetsMD()
    // bool pmConfigFileSetsMD(psMetadata *metadata, int *argc, char **argv, const char *name,
    //                         const char *file, const char *list)
    // XX: Should we test/check for bad argv/argc params?
    // test with NULL metadata pointer
    {
        psMemId id = psMemGetId();
        psMetadata *metadata = psMetadataAlloc();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *name = "NAME";
        char *file = "FILENAME";
        char *list = "LIST";
        bool rc = pmConfigFileSetsMD(NULL, &myArgc, str, name, file, list);
        if (!rc) {
            str[2] = "dataFiles/SampleIPPConfig";
            rc = pmConfigFileSetsMD(NULL, &myArgc, str, name, file, list);
	}
        ok(rc == false, "pmConfigFileSetsMD() returned FALSE with NULL metadata pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigFileSetsMD() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psFree(metadata);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL name pointer
    {
        psMemId id = psMemGetId();
        psMetadata *metadata = psMetadataAlloc();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *file = "FILENAME";
        char *list = "LIST";
        bool rc = pmConfigFileSetsMD(metadata, &myArgc, str, NULL, file, list);
        if (!rc) {
            str[2] = "dataFiles/SampleIPPConfig";
            rc = pmConfigFileSetsMD(metadata, &myArgc, str, NULL, file, list);
	}
        ok(rc == false, "pmConfigFileSetsMD() returned FALSE with NULL name pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileSetsMD() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psFree(metadata);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL file pointer
    {
        psMemId id = psMemGetId();
        psMetadata *metadata = psMetadataAlloc();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *name = "NAME";
        char *list = "LIST";
        bool rc = pmConfigFileSetsMD(metadata, &myArgc, str, name, NULL, list);
        if (!rc) {
            str[2] = "dataFiles/SampleIPPConfig";
            rc = pmConfigFileSetsMD(metadata, &myArgc, str, name, NULL, list);
	}
        ok(rc == false, "pmConfigFileSetsMD() returned FALSE with NULL file pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileSetsMD() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psFree(metadata);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL list pointer
    {
        psMemId id = psMemGetId();
        psMetadata *metadata = psMetadataAlloc();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *name = "NAME";
        char *file = "FILENAME";
        bool rc = pmConfigFileSetsMD(metadata, &myArgc, str, name, file, NULL);
        if (!rc) {
            str[2] = "dataFiles/SampleIPPConfig";
            rc = pmConfigFileSetsMD(metadata, &myArgc, str, name, file, NULL);
	}
        ok(rc == false, "pmConfigFileSetsMD() returned FALSE with NULL list pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigFileSetsMD() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psFree(metadata);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with acceptable input data
    if (0) {
        psMemId id = psMemGetId();
        psMetadata *metadata = psMetadataAlloc();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int myArgc = 3;
        char *name = "NAME";
        char *file = "FILENAME";
        char *list = "LIST";
        bool rc = pmConfigFileSetsMD(metadata, &myArgc, str, name, file, list);
        if (!rc) {
            str[2] = "dataFiles/SampleIPPConfig";
            rc = pmConfigFileSetsMD(metadata, &myArgc, str, name, file, list);
	}
        ok(rc == true, "pmConfigFileSetsMD() returned TRUE with acceptable input data");
        psFree(metadata);
        psErrorClear();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigConvertFilename()
    // convert the supplied name, create a new output psString
    // psString pmConfigConvertFilename(const char *filename, const pmConfig *config,
    //                                  bool create)
    // test with NULL filename pointer
    {
        psMemId id = psMemGetId();
        pmConfig *config = pmConfigAlloc();
        bool create = false;
        bool rc = pmConfigConvertFilename(NULL, (const pmConfig *) config, create, false);
        ok(rc == false, "pmConfigConvertFilename() returned FALSE with NULL filename pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_VALUE == tmpErr->code,
          "pmConfigConvertFilename() created the PS_ERR_BAD_PARAMETER_VALUE error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with NULL config pointer
    {
        psMemId id = psMemGetId();
        char *name = "NAME";
        pmConfig *config = pmConfigAlloc();
        bool create = false;
        bool rc = pmConfigConvertFilename(name, NULL, create, false);
        ok(rc == false, "pmConfigConvertFilename() returned FALSE with NULL config pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigConvertFilename() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test with acceptable input data
    // First call with create==FALSE, and a filename that does not exist.
    // Should return NULL.  Then set create==TRUE, and call; should return
    // "JUNK".  Then call again with create==FALSE; should return "JUNK"
    if (1) {
        psMemId id = psMemGetId();
        char *filename = "file:///////JUNK";
        pmConfig *config = pmConfigAlloc();
        bool rc = pmConfigFileRead(&config->site, "../dataFiles/SampleIPPConfig", "DESCRIPTION");
        if (!rc) {
            rc = pmConfigFileRead(&config->site, "dataFiles/SampleIPPConfig", "DESCRIPTION");
	}
        ok(rc == true, "pmConfigFileRead() was successful");
        bool create = false;
        psString tmpStr = pmConfigConvertFilename(filename, (const pmConfig *) config, create, false);
        ok(NULL == tmpStr, "pmConfigConvertFilename() returned NULL with create==FALSE");
        create = true;
        tmpStr = pmConfigConvertFilename(filename, (const pmConfig *) config, create, false);
        ok(tmpStr != NULL, "pmConfigConvertFilename() returned non-NULL with create==TRUE");
        ok(!strcmp(tmpStr, "/JUNK"), "pmConfigConvertFilename() returned correct filename (%s)", tmpStr);
        psFree(tmpStr);
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // --------------------------------------------------------------------
    // --------------------------------------------------------------------
    // Test pmConfigRead()
    // Test with NULL argc pointer
    {
        psMemId id = psMemGetId();
        psString str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        pmConfig *myConfig = pmConfigRead(NULL, str, "RecipeName");
        if (!myConfig) {
            str[2] = "dataFiles/SampleIPPConfig";
            myConfig = pmConfigRead(NULL, str, "RecipeName");
	}
        ok(myConfig == NULL, "pmConfigRead() returned NULL with NULL argc pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(myConfig);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL argv pointer
    {
        psMemId id = psMemGetId();
        pmConfig *myConfig = pmConfigRead(&argc, NULL, "RecipeName");
        ok(myConfig == NULL, "pmConfigRead() returned NULL with NULL argv pointer");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(myConfig);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test with NULL recipe string (should not cause error)
    // XXX: Not working, debug
    if (0) {
        psMemId id = psMemGetId();
        char *str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        int numArgs = 3;
        pmConfig *myConfig = pmConfigRead(&numArgs, str, NULL);
        if (!myConfig) {
            str[2] = "dataFiles/SampleIPPConfig";
            myConfig = pmConfigRead(&numArgs, str, NULL);
	}
        ok(myConfig != NULL, "pmConfigRead() returned non-NULL with NULL recipe");
        psErr *tmpErr = psErrorLast();
        ok(PS_ERR_BAD_PARAMETER_NULL == tmpErr->code,
          "pmConfigValidateCameraFormat() created the PS_ERR_BAD_PARAMETER_NULL error");
        psFree(tmpErr);
        psErrorClear();
        psFree(myConfig);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmConfigRead() with acceptable data.
    if (1) {
        psMemId id = psMemGetId();
        bool testStatus = true;
        psMetadata *site = psMetadataAlloc();
        psMetadata *camera = psMetadataAlloc();
        psMetadata *recipe = psMetadataAlloc();
        psString str[3];
        str[0] = "ARGS:";
        str[1] = "-site";
        str[2] = "../dataFiles/SampleIPPConfig";
        psS32 argc = 3;
        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with acceptable arguments.\n");
        bool rc;
        pmConfig *myConfig = pmConfigRead(&argc, str, "RecipeName");
        if (!myConfig) {
            str[2] = "dataFiles/SampleIPPConfig";
            myConfig = pmConfigRead(&argc, str, "RecipeName");
	}
        ok(myConfig, "pmConfigRead() returned non-NULL");
        if (myConfig == NULL) {
            diag("TEST ERROR: pmConfigRead() returned NULL\n");
            testStatus = false;
        } else {
            //
            // Ensure that the various trace and logging values are set properly
            //
            if (1 != psTraceGetLevel("dummyTraceFunc01")) {
                diag("TEST ERROR: failed to properly set tracelevel for dummyTraceFunc01\n");
                diag("    tracelevel was %d, should be 1.\n", psTraceGetLevel("dummyTraceFunc01"));
                testStatus = false;
            }

            if (2 != psTraceGetLevel("dummyTraceFunc02")) {
                diag("TEST ERROR: failed to properly set tracelevel for dummyTraceFunc02\n");
                diag("    tracelevel was %d, should be 2.\n", psTraceGetLevel("dummyTraceFunc02"));
                testStatus = false;
            }

            if (1 != psLogGetDestination()) {
                diag("TEST ERROR: failed to properly set log destination.\n");
                diag("    it was %d, should be 1\n", psLogGetDestination());
                testStatus = false;
            }

            if (3 != psLogGetLevel()) {
                diag("TEST ERROR: failed to properly set log level.\n");
                diag("    it was %d, should be 3\n", psLogGetLevel());
                testStatus = false;
            }

            //
            // Test the several arbitrary config file strings.
            //
            psS32 tmpInt = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_S32");
            if ((rc == false) || (tmpInt != 20)) {
                diag("TEST ERROR: failed to properly set metadata integer ARBITRARY_STRING_S32.\n");
                testStatus = false;
            }

            psF32 tmpFloat = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_F32");
            if ((rc == false) || (tmpFloat != 21.0)) {
                diag("TEST ERROR: failed to properly set metadata float ARBITRARY_STRING_F32.\n");
                testStatus = false;
            }

            psF64 tmpDub = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_F64");
            if ((rc == false) || (tmpDub != 22.0)) {
                diag("TEST ERROR: failed to properly set metadata double ARBITRARY_STRING_F64.\n");
                testStatus = false;
            }

            psBool tmpBool;
            tmpBool = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_BOOL_T");
            if ((rc == false) || (tmpBool != true)) {
                diag("TEST ERROR: failed to properly set metadata double ARBITRARY_STRING_BOOL_T.\n");
                testStatus = false;
            }
            tmpBool = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_BOOL_F");
            if ((rc == false) || (tmpBool != false)) {
                diag("TEST ERROR: failed to properly set metadata double ARBITRARY_STRING_BOOL_F.\n");
                testStatus = false;
            }

            //
            // Test the database camera metadata keywords.
            //
            psMetadata *tmpMeta = psMetadataLookupMetadata(&rc, site, "CAMERAS");
            if (rc == false) {
                diag("TEST ERROR: failed to properly set metadata metadata CAMERAS.\n");
                testStatus = false;
            } else {
                psString tmpStr;
                tmpStr = psMetadataLookupStr(&rc, tmpMeta, "MEGACAM_RAW");
                if ((rc == false) || (0 != strcmp(tmpStr, "megacam_raw.config"))) {
                    diag("TEST ERROR: failed to properly set metadata metadata CAMERAS->MEGACAM_RAW.\n");
                    testStatus = false;
                }

                tmpStr = psMetadataLookupStr(&rc, tmpMeta, "MEGACAM_SPLICE");
                if ((rc == false) || (0 != strcmp(tmpStr, "megacam_splice.config"))) {
                    diag("TEST ERROR: failed to properly set metadata metadata CAMERAS->MEGACAM_SPLICE.\n");
                    testStatus = false;
                }

                tmpStr = psMetadataLookupStr(&rc, tmpMeta, "GPC1_RAW");
                if ((rc == false) || (0 != strcmp(tmpStr, "gpc1_raw.config"))) {
                    diag("TEST ERROR: failed to properly set metadata metadata CAMERAS->GPC1_RAW.\n");
                    testStatus = false;
                }

                tmpStr = psMetadataLookupStr(&rc, tmpMeta, "LRIS_BLUE");
                if ((rc == false) || (0 != strcmp(tmpStr, "lris_blue.config"))) {
                    diag("TEST ERROR: failed to properly set metadata metadata CAMERAS->LRIS_BLUE.\n");
                    testStatus = false;
                }

                tmpStr = psMetadataLookupStr(&rc, tmpMeta, "LRIS_RED");
                if ((rc == false) || (0 != strcmp(tmpStr, "lris_red.config"))) {
                    diag("TEST ERROR: failed to properly set metadata metadata CAMERAS->LRIS_RED.\n");
                    testStatus = false;
                }
            }

            //
            // Test the database metadata keywords.  This is somewhat redundant since
            // we already test metadat strings.
            //
            psString tmpStr;
            tmpStr = psMetadataLookupStr(&rc, site, "DBSERVER");
            if ((rc == false) || (0 != strcmp(tmpStr, "ippdb.ifa.hawaii.edu"))) {
                diag("TEST ERROR: failed to properly set metadata string DBSERVER.\n");
                testStatus = false;
            }

            tmpStr = psMetadataLookupStr(&rc, site, "DBUSER");
            if ((rc == false) || (0 != strcmp(tmpStr, "ipp"))) {
                diag("TEST ERROR: failed to properly set metadata string DBUSER.\n");
                testStatus = false;
            }

            tmpStr = psMetadataLookupStr(&rc, site, "DBPASSWORD");
            if ((rc == false) || (0 != strcmp(tmpStr, "password"))) {
                diag("TEST ERROR: failed to properly set metadata string DBPASSWORD.\n");
                testStatus = false;
            }
        }
        psFree(site);
        psFree(camera);
        psFree(recipe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

}


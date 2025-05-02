/** @file tst_pmConfig.c
 *
 *  @brief Contains the tests for pmConfig.c:
 *
 * test00: This code will test the pmConfig() routine.
 *
 *  @author GLG, MHPCC
 *
 * XXX: Untested:
 * pmConfigValidateCamera()
 * pmConfigCameraFromHeader()
 * pmConfigRecipeFromCamera()
 * pmConfigDB()
 *
 * XXXX: Must determine what to do with NULL arguments, then test it.
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-10-13 21:15:45 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include "psTest.h"
#include "pslib.h"
#include "pmConfig.h"

static int test00(void);
testDescription tests[] = {
                              {test00, 000, "pmConfig:", true, false},
                              {NULL}
                          };

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    //
    // We include the function names here in psTraceSetLevel() commands for
    // debugging convenience.  There is no guarantee that this list of functions
    // is complete.
    //
    psTraceSetLevel(".", 0);
    psTraceSetLevel("readConfig", 0);
    psTraceSetLevel("pmConfigRead", 0);
    psTraceSetLevel("pmConfigValidateCamera", 0);
    psTraceSetLevel("pmConfigCameraFromHeader", 0);
    psTraceSetLevel("pmConfigRecipeFromCamera", 0);
    psTraceSetLevel("pmConfigDB", 0);
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}

/******************************************************************************
test00():
XXX: untested:
    TIME:
    LOGFORMAT: tested implicitly via the stdout files.
    Command line arguments for logging shall override config file defaults.
 *****************************************************************************/
int test00( void )
{
    psTraceSetLevel(".", 0);
    bool testStatus = true;
    psBool rc = 0;
    psString str[3];
    str[0] = "ARGS:";
    str[1] = "-site";
    str[2] = "SampleIPPConfig";

    psMetadata *site = psMetadataAlloc();
    psMetadata *camera = psMetadataAlloc();
    psMetadata *recipe = psMetadataAlloc();
    psS32 argc = 3;


    psMetadata *nullMD = NULL;
    if (0) {
        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL site pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(NULL, &camera, &recipe, &argc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL *site pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(&nullMD, &camera, &recipe, &argc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL camera pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(&site, NULL, &recipe, &argc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL *camera pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(&site, &nullMD, &recipe, &argc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL recipe pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(&site, &camera, NULL, &argc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL *recipe pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(&site, &camera, &nullMD, &argc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with NULL argv pointer.  Should generate ERROR, return false.\n");
        rc = pmConfigRead(&site, &camera, &recipe, &argc, NULL, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }

        printf("----------------------------------------------------------------\n");
        printf("Calling pmConfigRead() with negative argc.  Should generate ERROR, return false.\n");
        psS32 tmpArgc = -1;
        rc = pmConfigRead(&site, &camera, &recipe, &tmpArgc, str, "RecipeName");
        if (rc == true) {
            printf("TEST ERROR: pmConfigRead() returned true\n");
            testStatus = false;
        }
    }

    printf("----------------------------------------------------------------\n");
    printf("Calling pmConfigRead() with acceptable arguments.\n");

    rc = pmConfigRead(&site, &camera, &recipe, &argc, str, "RecipeName");
    if (rc == false) {
        printf("TEST ERROR: pmConfigRead() returned False\n");
        testStatus = false;
    } else {
        //
        // Ensure that the various trace and logging values are set properly
        //
        if (1 != psTraceGetLevel("dummyTraceFunc01")) {
            printf("TEST ERROR: failed to properly set tracelevel for dummyTraceFunc01\n");
            printf("    tracelevel was %d, should be 1.\n", psTraceGetLevel("dummyTraceFunc01"));
            testStatus = false;
        }

        if (2 != psTraceGetLevel("dummyTraceFunc02")) {
            printf("TEST ERROR: failed to properly set tracelevel for dummyTraceFunc02\n");
            printf("    tracelevel was %d, should be 2.\n", psTraceGetLevel("dummyTraceFunc02"));
            testStatus = false;
        }

        if (1 != psLogGetDestination()) {
            printf("TEST ERROR: failed to properly set log destination.\n");
            printf("    it was %d, should be 1\n", psLogGetDestination());
            testStatus = false;
        }

        if (3 != psLogGetLevel()) {
            printf("TEST ERROR: failed to properly set log level.\n");
            printf("    it was %d, should be 3\n", psLogGetLevel());
            testStatus = false;
        }

        //
        // Test the several arbitrary config file strings.
        //
        psS32 tmpInt = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_S32");
        if ((rc == false) || (tmpInt != 20)) {
            printf("TEST ERROR: failed to properly set metadata integer ARBITRARY_STRING_S32.\n");
            testStatus = false;
        }

        psF32 tmpFloat = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_F32");
        if ((rc == false) || (tmpFloat != 21.0)) {
            printf("TEST ERROR: failed to properly set metadata float ARBITRARY_STRING_F32.\n");
            testStatus = false;
        }

        psF64 tmpDub = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_F64");
        if ((rc == false) || (tmpDub != 22.0)) {
            printf("TEST ERROR: failed to properly set metadata double ARBITRARY_STRING_F64.\n");
            testStatus = false;
        }

        psBool tmpBool;
        tmpBool = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_BOOL_T");
        if ((rc == false) || (tmpBool != true)) {
            printf("TEST ERROR: failed to properly set metadata double ARBITRARY_STRING_BOOL_T.\n");
            testStatus = false;
        }
        tmpBool = psMetadataLookupS32(&rc, site, "ARBITRARY_STRING_BOOL_F");
        if ((rc == false) || (tmpBool != false)) {
            printf("TEST ERROR: failed to properly set metadata double ARBITRARY_STRING_BOOL_F.\n");
            testStatus = false;
        }

        //
        // Test the database camera metadata keywords.
        //
        psMetadata *tmpMeta = psMetadataLookupMetadata(&rc, site, "CAMERAS");
        if (rc == false) {
            printf("TEST ERROR: failed to properly set metadata metadata CAMERAS.\n");
            testStatus = false;
        } else {
            psString tmpStr;
            tmpStr = psMetadataLookupStr(&rc, tmpMeta, "MEGACAM_RAW");
            if ((rc == false) || (0 != strcmp(tmpStr, "megacam_raw.config"))) {
                printf("TEST ERROR: failed to properly set metadata metadata CAMERAS->MEGACAM_RAW.\n");
                testStatus = false;
            }

            tmpStr = psMetadataLookupStr(&rc, tmpMeta, "MEGACAM_SPLICE");
            if ((rc == false) || (0 != strcmp(tmpStr, "megacam_splice.config"))) {
                printf("TEST ERROR: failed to properly set metadata metadata CAMERAS->MEGACAM_SPLICE.\n");
                testStatus = false;
            }

            tmpStr = psMetadataLookupStr(&rc, tmpMeta, "GPC1_RAW");
            if ((rc == false) || (0 != strcmp(tmpStr, "gpc1_raw.config"))) {
                printf("TEST ERROR: failed to properly set metadata metadata CAMERAS->GPC1_RAW.\n");
                testStatus = false;
            }

            tmpStr = psMetadataLookupStr(&rc, tmpMeta, "LRIS_BLUE");
            if ((rc == false) || (0 != strcmp(tmpStr, "lris_blue.config"))) {
                printf("TEST ERROR: failed to properly set metadata metadata CAMERAS->LRIS_BLUE.\n");
                testStatus = false;
            }

            tmpStr = psMetadataLookupStr(&rc, tmpMeta, "LRIS_RED");
            if ((rc == false) || (0 != strcmp(tmpStr, "lris_red.config"))) {
                printf("TEST ERROR: failed to properly set metadata metadata CAMERAS->LRIS_RED.\n");
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
            printf("TEST ERROR: failed to properly set metadata string DBSERVER.\n");
            testStatus = false;
        }

        tmpStr = psMetadataLookupStr(&rc, site, "DBUSER");
        if ((rc == false) || (0 != strcmp(tmpStr, "ipp"))) {
            printf("TEST ERROR: failed to properly set metadata string DBUSER.\n");
            testStatus = false;
        }

        tmpStr = psMetadataLookupStr(&rc, site, "DBPASSWORD");
        if ((rc == false) || (0 != strcmp(tmpStr, "password"))) {
            printf("TEST ERROR: failed to properly set metadata string DBPASSWORD.\n");
            testStatus = false;
        }
    }

    psFree(site);
    psFree(camera);
    psFree(recipe);
    return(testStatus);
}

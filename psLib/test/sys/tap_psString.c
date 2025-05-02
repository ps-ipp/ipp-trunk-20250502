/** @file  tst_psString.c
 *
 * -*- mode: C; c-basic-indent: 4; tab-width: 8; indent-tabs-mode: nil -*-
 * vim: set cindent ts=8 sw=4 expandtab:
 *
 *  @brief Test driver for psString functions
 *
 *  This test driver contains the following test points for psStringCopy
 *  and psStringNCopy, and related string functions.
 *    1) Verify string copy - psStringCopy
 *    2) Verify empty string copy - psStringCopy
 *    3) Verify string copy with length - psStringNCopy
 *    4) Verify empty string copy with length - psStringNCopy
 *    5) Copy string to larger string - psStringNCopy
 *    6) Copy string with negative size - psStringNCopy
 *    7) Verifiy creation of string literal - PS_STRING
 *
 *  Return:   Number of test points which failed
 *
 *  @author  Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.11 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2008-05-05 00:09:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <string.h>
#include "pslib.h"
#include "tap.h"
#include "pstap.h"

#define STR_0 "binky had a leeeetle lamb"


psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(65);


    //testStringCopy00()
    {
        psMemId id = psMemGetId();
        char  stringval[20] = "E R R O R";
        psS32   result = 0;
        psS32   result1 = 0;
        char  *strResult;

        // Test point #1 Verify string copy - psStringCopy
        strResult = psStringCopy(stringval);
        // Perform string compare
        result = strcmp(strResult, stringval);
        // Modify original string
        stringval[0]='G';
        result1 = strcmp(strResult, stringval);
        stringval[0]='E';
        ok(( result == 0 ) && ( result1 != 0),
             "Failed test point #1 strcmp result = %d expected 0",result);
        // Free memory allocated
        psFree(strResult);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStringCopy01()
    {
        psMemId id = psMemGetId();
        char  *emptyval = "";
        psS32   result = 0;
        char  *strResult;

        // Test point #2 Verify empty string copy - psStringCopy
        strResult = psStringCopy(emptyval);
        // Perform string compare
        result = strcmp(strResult, emptyval);
        ok(result == 0,
             "test point #2 strcmp result = %d expected 0",result);
        // Free memory allocated
        psFree(strResult);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStringCopy02()
    {
        psMemId id = psMemGetId();
        psS32   result = 0;
        psS32   result1 = 0;
        char  *strResult;
        char  stringval1[20] = "e r r o r";
        psS32   substringlen = 5;
        char  *substringval = "e r r";

        // Test point #3 Verify string copy with length - psStringNCopy
        strResult = psStringNCopy(stringval1, substringlen);
        // Perform string compare and get string length
        result = strncmp(strResult, substringval, substringlen);
        // Change original string
        stringval1[0] = 'g';
        result1 = strncmp(strResult, substringval, substringlen);
        ok(( result == 0 ) && ( result1 == 0 ),
             "Failed test point #3 strcmp result = %d expected 0",result);
        psFree(strResult);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStringCopy03()
    {
        psMemId id = psMemGetId();
        char  *stringvalnocopy = "F A I L";

        // Test point #4 Verify empty string copy with length - psStringNCopy
        char *strResult = psStringNCopy(stringvalnocopy, 0);

        // Perform string compare and get string length
        int result = strcmp(strResult, stringvalnocopy);
        ok(result != 0, "test point #4 strcmp result = %d, expected %d", result, 4);

        int resultLen = strlen(strResult);
        ok(resultLen == 0, "test point #4 strcmp result = %d, expected %d", resultLen, 0);

        psFree(strResult);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStringCopy04()
    {
        psMemId id = psMemGetId();
        psS32   result = 0;
        psS32   result1 = 0;
        char  *strResult;
        char  stringval[20] = "E R R O R";
        psS32   increaseSize = 5;

        // Test point #5 Copy string to larger string - psStringNCopy
        strResult = psStringNCopy(stringval, (strlen(stringval) + increaseSize));
        // Perform string compare and get string length
        result = strcmp(strResult, stringval);
        result1 = strlen(strResult);
        // The strings should still compare
        ok(result == 0 && result1 == strlen(stringval),
             "test point #5 strcmp result = %d expected %d",result,0);
        psFree(strResult);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // XXX This test needs to be modified to check for maximum size
    // This will require a mod to psStringNCopy source to check for maximum size

    // psS32 testStringCopy05()
    skip_start (1, 6, "Skipping psSTringNCopy() because of failure to test max value");
    {
      char  *strResult;
      char  stringval[20] = "E R R O R";
      psS32   negativeSize = -5;

      // Test point #6 Copy string with negative size - psStringNCopy
      strResult = psStringNCopy(stringval, negativeSize);
      if ( strResult != NULL ) {
        fprintf(stderr,"Failed test point #6 return value = %p expected NULL\n",
                strResult);
        return 1;
      }
      // Memory should not have been allocated
    }
    skip_end();

    // testStringCopy06()
    {
        psMemId id = psMemGetId();
        char  *strResult;
        char  stringval[20] = "E R R O R";
        psS32   result = 0;

        // Test point #7 Verify creation of string literal - PS_STRING
        strResult = PS_STRING(E R R O R);
        result = strcmp(strResult, stringval);
        ok(result == 0,
             "test point #7 strcmp result = %d expected %d",result,0);
        // Memory should not have been allocated
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStrAppend00()
    {
        psMemId id = psMemGetId();
        char *str = psStringCopy("3.14159");
        psStringAppend(&str, "%d%s", 2653589, "79323846");
        // Test point: Verify string append
        int result = strcmp(str, "3.14159265358979323846");
        ok(result == 0, "Failed test point");
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testStrAppend01()
    {
        psMemId id = psMemGetId();
        char *str = NULL;
        // test nonsensical invocations ...
        ssize_t sz = psStringAppend(NULL, NULL);
        ok(!sz, "append NULL string to NULL string");
        sz = psStringAppend(&str, NULL);
        ok(!sz, "append NULL string to NULL string");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testStrAppend02()
    {
        psMemId id = psMemGetId();
        char *str=NULL;
        // test string creation
        psStringAppend(&str, "%s", "fubar");
        int result = strcmp(str, "fubar");
        ok(result == 0, "Failed test point");
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStrAppend03()
    {
        psMemId id = psMemGetId();
        char *str =psStringCopy(STR_0);
        // test null-op
        psStringAppend(&str, "%s", "");
        is_str(str, STR_0, "Failed test point");
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStrPrepend00()
    {
        psMemId id = psMemGetId();
        char *str = psStringCopy("79323846");
        psStringPrepend(&str, "%s%d","3.14159", 2653589 );
        // Test point: Verify string append
        int result = strcmp(str, "3.14159265358979323846");
        ok(result == 0, "Failed test point");
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testStrPrepend01()
    {
        psMemId id = psMemGetId();
        char *str=NULL;
        // test nonsensical invocations ...
        ssize_t sz = psStringPrepend(NULL, NULL);
        ok(sz == 0, "Failed test point");
        sz = psStringPrepend(&str, NULL);
        ok(sz == 0, "Failed test point");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testStrPrepend02()
    {
        psMemId id = psMemGetId();
        char *str=NULL;
        // test string creation
        psStringPrepend(&str, "%s", "fubar");
        int result = strcmp(str, "fubar");
        ok(result == 0, "Failed test point");
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStrPrepend03()
    {
        // test null-op
        psMemId id = psMemGetId();
        char *str =  psStringCopy(STR_0);
        psStringPrepend(&str, "%s", "");
        int result = strcmp(str, STR_0);
        ok(result == 0, "test point str=[%s]", str);
        psFree(str);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStrSplit00()
    {
        psMemId id = psMemGetId();
        psList *strList = NULL;
        char str[35];
        char split[5];
        strncpy(str, "This is, a, test case, to check", 35);
        strncpy(split, ",", 2);
        psString psStr;
        psString psSplit;
        psStr = psStringCopy(str);
        psSplit = psStringCopy(split);

        //Return NULL for NULL inputs
        strList = psStringSplit(NULL, NULL, true);
        ok(!strList, "psStringSplit" );
        psFree(strList);

        strList = NULL;
        //Return empty list for NULL string input
        strList = psStringSplit(NULL, split, true);
        ok(!psListLength(strList), "psListLength()" );
        psFree(strList);

        strList = NULL;
        //Return NULL for NULL splitter input
        strList = psStringSplit(str, NULL, true);
        ok(!strList, "psStringSplit" );
        psFree(strList);

        strList = NULL;
        //Return a psList* of psStrings
        strList = psStringSplit(str, split, true);
        ok(strList->n == 4,
            "psStringSplit to return the correct number of strings");
        ok(!strncmp((psString)(strList->head->data), "This is", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->data), " a", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->data), " test case",10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->next->data), " to check", 10),
             "psStringSplit failed to return expected strings");

        psFree(strList);
        //Return correct psList when using (psString, char*)
        strList = psStringSplit(psStr, split, true);
        ok(strList->n == 4,
            "psStringSplit to return the correct number of strings");
        ok(!strncmp((psString)(strList->head->data), "This is", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->data), " a", 10),
             "psStringSplit failed to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->data), " test case",10),
             "psStringSplit failed to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->next->data), " to check", 10),
             "psStringSplit to return expected strings");

        psFree(strList);
        //Return correct psList when using (char*, psString)
        strList = psStringSplit(str, psSplit, true);
        ok(strList->n == 4,
            "psStringSplit to return the correct number of strings");
        ok(!strncmp((psString)(strList->head->data), "This is", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->data), " a", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->data), " test case",10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->next->data), " to check", 10),
             "psStringSplit to return expected strings");

        psFree(strList);
        //Return correct psList when using (psString, psString)
        strList = psStringSplit(psStr, psSplit, true);
        ok(strList->n == 4,
            "psStringSplit to return the correct number of strings");
        ok(!strncmp((psString)(strList->head->data), "This is", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->data), " a", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->data), " test case",10),
             "psStringSplit  to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->next->data), " to check", 10),
             "psStringSplit to return expected strings");

        psFree(strList);
        //Return correct psList output for string of zero length case
        strncpy(str, "This is,, a,, test case,, to check", 35);
        strList = psStringSplit(str, split, false);
        ok(strList->n == 4,
            "psStringSplit to return the correct number of strings");
        ok(!strncmp((psString)(strList->head->data), "This is", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->data), " a", 10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->data), " test case",10),
             "psStringSplit to return expected strings");
        ok(!strncmp((psString)(strList->head->next->next->next->data), " to check", 10),
             "psStringSplit to return expected strings");
        psFree(strList);
        psFree(psStr);
        psFree(psSplit);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testNULLStrings()
    {
        psMemId id = psMemGetId();
        psString nullTest = NULL;
        psString output = NULL;
        ssize_t outSize = 0;
        char** nullDest = NULL;
        char** test = NULL;
        //psStringCopy should return NULL for NULL input string
        // Following should generate error message
        output = psStringCopy(nullTest);
        ok(output == NULL, "psStringCopy to return NULL for NULL input string");

        //psStringNCopy should return NULL for NULL input string
        output = psStringNCopy(nullTest, 100);
        ok(output == NULL, "psStringNCopy to return NULL for NULL input string");

        //psStringAppend should return 0 for NULL input destination
        outSize = psStringAppend(nullDest, "%s", "");
        ok(outSize == 0, "psStringAppend to return 0 for NULL input destination");

        //psStringAppend should return 0 for NULL input format
	// note that only a string literal is allowed for the NULL due to gcc change
        outSize = psStringAppend(test, NULL);
        ok(outSize == 0, "psStringAppend to return 0 for NULL input format");

        //psStringPrepend should return 0 for NULL input destination
        outSize = psStringPrepend(nullDest, " ");
        ok(outSize == 0, "psStringPrepend to return 0 for NULL input destination");

        //psStringPrepend should return 0 for NULL input format
	// note that only a string literal is allowed for the NULL due to gcc change
        outSize = psStringPrepend(test, NULL);
        ok(outSize == 0, "psStringPrepend to return 0 for NULL input format");

        //psStringSplit should return empty list for NULL input string
        psList *nullList = NULL;
        nullList = psStringSplit(nullTest, ",", true);
        ok(!psListLength(nullList), "psStringSplit to return NULL for NULL input string");
        psFree(nullList);

        nullList = NULL;
        //psStringSplit should return NULL for NULL input splitter
        nullList = psStringSplit("Hello World", nullTest, true);
        ok(!nullList, "psStringSplit to return NULL for NULL input splitter");
        psFree(nullList);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testStrCheck()
    {
        psMemId id = psMemGetId();
        psString str = psStringAlloc(10);
        strcpy(str, "Hello");
        ok(psMemCheckString(str), "psString allocated!");
        ok(psMemCheckType(PS_DATA_STRING, str), "psString allocated");
        psFree(str);

	// XXX EAM this function raises an abort since we are trying to test a non-psLib memory block
        // char charStr[10];
        // ok(!psMemCheckType(PS_DATA_STRING, charStr), "Input string is a psDataType");

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

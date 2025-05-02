/** @file  tst_psError.c
 *
 *  @brief Test driver for psError function
 *
 *  @author  Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.7 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2008-05-05 00:09:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "pslib.h"
#include "tap.h"
#include "pstap.h"

// XXX in this file, several operations are only validated by printing an output to the screen.   
// we should define an output file and compare the contents of the output file to expectations.
// I've commented out these features for now

# if (0)
// Function used in testError02 to verify the psErrorStackPrintV function
static void myErrorStackPrint(
    FILE *fd,
    const char *fmt,
    ...)
{
    va_list ap;

    // Test whether psErrorStackPrintV() accept a va_list for output variables
    va_start(ap, fmt);
    psErrorStackPrintV(fd, fmt, ap);
    va_end(ap);
}
# endif

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(60);


    //psError() tests
    {
        psS32  intval=1;
        psS64 longval = 2;
        float floatval = 3.01;
        char  charval = 'E';
        char *stringval = "E R R O R";

        // Multiple type values placed in the error string
        // XX: The output is never verified
        {
            psMemId id = psMemGetId();
            psError(PS_ERR_UNKNOWN, true,
                    "ALL TYPES intval = %d longval = %"PRId64 " floatval = %f charval = %c strval = %s",
                    intval,longval,floatval,charval,stringval);
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
    

        // String values in error message
        // XX: The output is never verified
        {
            psMemId id = psMemGetId();
            psError(PS_ERR_UNKNOWN, true, "NO VALUES");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Empty strings in error message
        // XX: The output is never verified
        {
            psMemId id = psMemGetId();
            psError(PS_ERR_UNKNOWN, true, " ");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Verify the return value of psErrorMsg is the psErrorCode passed
        {
            psMemId id = psMemGetId();
            psErrorCode code=PS_ERR_BAD_PARAMETER_VALUE;
            ok(psError(code, true, "Error code = %d", code) == code, "Failed return value verify.");
            // psErrorStackPrint(stderr,"ERROR STACK PRINT Test1A");
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // test1B empty string in for name argument
        {
            psMemId id = psMemGetId();
            psErrorCode code=PS_ERR_BAD_PARAMETER_VALUE;
            ok(psError(code+1, true, "Error code = %d", code+1) == code+1,
                 "Failed return with empty string.");
            // psErrorStackPrint(stderr,"ERROR STACK PRINT Test1B");
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // test1D undefined code
        {
            psMemId id = psMemGetId();
            ok(psError(-1, true, "Error code = %d", -1) == -1,
                 "Failed return with undefined code.");
            // psErrorStackPrint(stderr,"ERROR STACK PRINT Test1D");
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // test1E set psErrorMsg argument to false
        {
            psMemId id = psMemGetId();
            psErrorCode code=PS_ERR_BAD_PARAMETER_VALUE;
            ok(psError(code, false, "Error code = %d", code) == code,
                "Failed return with false new arg.");
            // psErrorStackPrint(stderr,"ERROR STACK PRINT Test1E");
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // test1F psErrorMsg with a error code less then PS_ERR_BASE(256)
        {
            psMemId id = psMemGetId();
            ok(psError(9, true, "Errno code = %d", 9) == 9, "Error Code" );
            // psErrorStackPrint(stderr,"ERROR STACK PRINT Test1F");
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }
    }


    //testError02()
    {
        // Generate error message and verify return value
        {
            psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
            psMemId id = psMemGetId();
            ok(psError(code, true, "Error code = %d", code) == code,
                "Failed return value verify.");
            // myErrorStackPrint(stderr,"ERROR STACK PRINT Test%dA",2);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }
    }


    //psErrorGet(), psErrorLast()
    {
        // Attempt to get last error message with an empty stack verify 
        // psErr with code PS_ERR_NONE
        {
            psMemId id = psMemGetId();
            psErrorClear();
            psErr *last = psErrorLast();
            ok(last != NULL, "psErrorLast() returned non-NULL");
            skip_start(last == NULL, 1, "Skipping tests because psErrorLast() returned NULL");
            ok(last->code == PS_ERR_NONE,
              "psErrorLast did return PS_ERR_NONE for empty stack(%d)", last->code);
            skip_end();
            psFree(last);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }

        // Attempt to get specific error message with empty stack verify
        // psErr with code PS_ERR_NONE
        {
            psMemId id = psMemGetId();
            psErr *getErr= psErrorGet(2);
            ok(getErr != NULL, "psErrorGet(2) returned non-NULL");
            skip_start(getErr == NULL, 1, "Skipping tests because psErrorGet() returned NULL");
            ok(getErr->code == PS_ERR_NONE,
               "psErrorGet(2) did return PS_ERR_NONE for empty stack(%d)", getErr->code);
            skip_end();
            psFree(getErr);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to get error message with invalid index and an empty stack
        {
            psMemId id = psMemGetId();
            psErr *getErr= psErrorGet(-1);
            ok(getErr != NULL, "psErrorGet(-1) returned non-NULL");
            skip_start(getErr == NULL, 1, "Skipping tests because psErrorGet(-1) returned NULL");
            ok(getErr->code == PS_ERR_NONE, "psErrorGet with invalid index/empty stack");
            skip_end();
            psFree(getErr);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // Generate three error messages, ensure they are correctly returned by psError()
        {
            psMemId id = psMemGetId();
            psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
            ok(psError(code, true, "Error code = %d", code) ==  code,
                "psError() returned correct error code (example 1)");
            ok(psError((code+1), false, "Error code = %d", (code+1)) == (code+1),
                "psError() returned correct error code (example 2)");
            ok(psError((code+2), false, "Error code = %d", (code+2)) == (code+2),
                "psError() returned correct error code (example 3)");
    
            psErr *last = psErrorLast();
            psErr *getErr= psErrorGet(0);
    
            // Check that last and get with 0 index are equal
            ok(last == getErr, "psErrorGet(0) equal to psErrorLast()");
            psFree(last);

            // Verify the last error message was returned
            ok(getErr->code == (code+2), "psErrorLast() did retrieve last error");
            psFree(getErr);
            psErrorClear();
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // Verify the middle error message can be retrieved
        {
            psMemId id = psMemGetId();
            psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
            psError(code, true, "Error code = %d", code);
            psError((code+1), false, "Error code = %d", (code+1));
            psError((code+2), false, "Error code = %d", (code+2));
            psErr *getErr= psErrorGet(1);
            ok(getErr->code == (code+1), "psErrorGet() did not retrieve proper error");
            psFree(getErr);
            psErrorClear();
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }

        // Verify the psErrorGet returns non-NULL PS_ERR_NONE if an invalid index
        // is given with non-empty error stack
        {
            psMemId id = psMemGetId();
            psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
            psError(code, true, "Error code = %d", code);
            psError((code+1), false, "Error code = %d", (code+1));
            psError((code+2), false, "Error code = %d", (code+2));
            psErr *getErr= psErrorGet(-1);
            ok(getErr->code == PS_ERR_NONE, "psErrorGet() did not return PS_ERR_NONE w/ invalid arg");
            psFree(getErr);
            psErrorClear();
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }
    }


    // psErrorClear()
    {
        // With an attemp error stack call psErrorClear
        {
            psMemId id = psMemGetId();
            psErrorClear();
            // Get the last error message and verify PS_ERR_NONE (empty stack)
            psErr *lastAfterClear = psErrorLast();
            ok(lastAfterClear->code == PS_ERR_NONE, "psErrorLast return expected.");
            psFree(lastAfterClear);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }


        // Generate three error messages to have messages on error stack
        {
            psMemId id = psMemGetId();
            psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
            ok(psError(code, true, "Error code = %d", code) == code,
                "Failed return value verify.");
            ok(psError((code+1), false, "Error code = %d", (code+1)) == (code+1),
                "Failed return value verify.");
            ok(psError((code+2), false, "Error code = %d", (code+2)) == (code+2),
                "Failed return value verify.");
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }

        // Get the last error message and verify it has the expected code
        {
            psMemId id = psMemGetId();
            psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
            psErr *last = psErrorLast();
            ok(last->code == (code+2), "psErrorLast return expected.");
            psFree(last);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }

        // Clear the error stack
        {
            psMemId id = psMemGetId();
            psErrorClear();
            // Get the last error message after clear and verify is has PS_ERR_NONE code
            psErr *lastAfterClear  = psErrorLast();
            ok(lastAfterClear->code == PS_ERR_NONE, "psErrorLast return expected.");
            psFree(lastAfterClear);
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        }
    }


    // psErrorCodeString()
    {
        // Verify the return value of psErrorCodeString
        // psErrorCode code = PS_ERR_BAD_PARAMETER_VALUE;
        // Verify the return value of psErrorCodeString if code is negative
        ok( psErrorCodeString(-1) == NULL, "error string with neg. code");
    }


    //testErrorRegister()
    {
        psS32 numErr = 4;
        psErrorDescription errDesc[] = { {PS_ERR_N_ERR_CLASSES+1,"first"},
                                         {PS_ERR_N_ERR_CLASSES+2,"second"},
                                         {PS_ERR_N_ERR_CLASSES+3,"third"},
                                         {PS_ERR_N_ERR_CLASSES+4,"fourth"} };
        /*
            1. invoke psErrorRegister with a n>1 array of psErrorDescriptions. Verify that:
                a. Each error description given is retrievable with psErrorCodeString.
        */
        psErrorRegister(errDesc,numErr);

        for (psS32 i = 0; i < numErr; i++) {
            const char *desc = psErrorCodeString(PS_ERR_N_ERR_CLASSES+1+i);
            ok(desc, "psErrorCode found registered error code.");
            ok(!strcmp(desc,errDesc[i].description), "psErrorCode returned the proper description.  Got '%s', expected '%s'.", desc, errDesc[i].description);
        }

        /* 2. invoke psErrorCodeString with a static/builtin psLib error code. Verify:
                a. the result is correct.
        */
        const char *desc = psErrorCodeString(PS_ERR_N_ERR_CLASSES);
        ok(desc, "psErrorCode found static error code.");
        ok(!strcmp(desc, "error classes end marker"), "psErrorCode returned the proper description.  Got '%s', expected '%s'.", desc, "error classes end marker");

        desc = psErrorCodeString(PS_ERR_NONE);
        ok(desc, "psErrorCode found static error code.");
        ok(!strcmp(desc,"not an error"), "psErrorCode returned the proper description.  Got '%s', expected '%s'.", desc, "not an error");
    
        /* 3. invoke psErrorCodeString with an invalid code. Verify a NULL is returned. */
        desc = psErrorCodeString(PS_ERR_N_ERR_CLASSES+numErr+1);
        ok(!desc, "psErrorCode returns a NULL with a bogus input code.");

        /* 4. invoke psErrorRegister with a NULL psErrorDescription. Verify that:
                a. the execution does not cease.
                b. an appropriate error is generated.
        */
        // Following should be an error
        psErrorClear();
        psErrorRegister(NULL,1);
        psErr* err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL,
        "psErrorCode generated proper error code for NULL input.");

        psFree(err);

        /*
            5. invoke psErrorRegister with nerror=0. Verify that no error occurs.
        */
        psErrorClear();
        psErrorRegister(errDesc,0);
        err = psErrorLast();
        ok(err->code == PS_ERR_NONE,
            "psErrorCode did not generate an error for nErrors = 0.");
        psFree(err);
    }
}

/**
 *  C Implementation: tap_psArguments_all
 *
 * Description:  Tests for psArgumentVerbosity, psArgumentGet, psArgumentRemove,
 *               psArgumentParse, psArgumentHelp, psLogArguments, psTraceArguments
 *
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

// tests which send output to the screen are silent unless DEBUG = 1
#define DEBUG 0

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(30);

    // test the failure cases for psArgumentGet, psArgumentRemove
    {
        psMemId id = psMemGetId();

	// define a simple fake argument list : this is not modified
        char *argv[5];
        argv[0] = "./program";
        argv[1] = "-string";
        argv[2] = "new";
        argv[3] = "-float";
        argv[4] = "6.66";
        int argc = 5;

	// define a simple fake argument list : this is not modified
	char *argvBad[5];
	argvBad[0] = "./program";
	argvBad[1] = "-string";
	argvBad[2] = "new";
	argvBad[3] = "-test";
	argvBad[4] = "6.66";
	int argcBad = 5;

        // simple argument definitions for comparison
        psMetadata *args = psMetadataAlloc();
        psMetadataAdd(args, PS_LIST_TAIL, "-string", PS_DATA_STRING, "Test String", "SomeString");
        psMetadataAdd(args, PS_LIST_TAIL, "-float", PS_DATA_F32, "Test Float", 0.666);
        psMetadataAdd(args, PS_LIST_TAIL, "-double", PS_DATA_F64, "Test Double", 0.666);

	ok( psArgumentGet(argc, argv, "-float") == 3, "psArgumentGet: return correct location of input argument.");
	ok( psArgumentGet(argc, NULL, "-float") == 0, "psArgumentGet: return 0 for a NULL input argument.");

	ok( psArgumentGet(argc, argv, NULL) == 0, "psArgumentGet: return 0 for a NULL input string.");
	ok( psArgumentGet(argc, argv, "")   == 0, "psArgumentGet: return 0 for an empty input string.");

	ok( psArgumentGet(argc, argv, "-xxx") == 0, "psArgumentGet: return 0 for an unmatched input string.");

	ok( !psArgumentRemove(0, &argc, argv), "psArgumentRemove: return false for argnum = 0.");
	ok( !psArgumentRemove(0, NULL, argv), "psArgumentRemove: return false for NULL argc.");

        // psArgumentParse tests
	ok( !psArgumentParse(NULL, &argc, argv), "psArgumentParse: return false for NULL argument metadata input.");
	ok( !psArgumentParse(args, NULL, argv),  "psArgumentParse: return false for NULL argc input.");
	ok( !psArgumentParse(args, &argc, NULL), "psArgumentParse: return false for NULL argv input.");

	int tempc = 0;
	ok( !psArgumentParse(args, &tempc, argv), "psArgumentParse: return false for argc = 0 input.");
	ok( !psArgumentParse(args, &argcBad, argvBad), "psArgumentParse: return false for argv containing unspecified input.");

	psFree(args);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // define a valid argument input set and definition set
    // Return true for valid case
    {
        psMemId id = psMemGetId();

	// define the sample arguments
	char *argv[20];
	argv[0] = "./program";
	argv[1] = "-string";
	argv[2] = "new";
	argv[3] = "-float";
	argv[4] = "6.66";
	argv[5] = "-double";
	argv[6] = "0.666";
	argv[7] = "-int";
	argv[8] = "8";
	argv[9] = "16";
	argv[10] = "32";
	argv[11] = "64";
	argv[12] = "-U8";
	argv[13] = "1";
	argv[14] = "-U16";
	argv[15] = "666";
	argv[16] = "-U32";
	argv[17] = "666";
	argv[18] = "-Bool";
	argv[19] = "true";
	int argc = 20;

	// setup the argument definition
	psMetadata *args = psMetadataAlloc();

	// three simple types
	psMetadataAdd(args, PS_LIST_TAIL, "-string", PS_DATA_STRING, "Test String", "SomeString");
	psMetadataAdd(args, PS_LIST_TAIL, "-float", PS_DATA_F32, "Test Float", 0.666);
	psMetadataAdd(args, PS_LIST_TAIL, "-double", PS_DATA_F64, "Test Double", 0.666);

	// a multiple argument option : -int 1 2 3 4
	psMetadata *ints = psMetadataAlloc();
	psMetadataAdd(ints, PS_LIST_TAIL, "int1", PS_DATA_S8, "Int1", 8);
	psMetadataAdd(ints, PS_LIST_TAIL, "int2", PS_DATA_S16, "Int2", 16);
	psMetadataAdd(ints, PS_LIST_TAIL, "int3", PS_DATA_S32, "Int3", 32);
	psMetadataAdd(ints, PS_LIST_TAIL, "int4", PS_DATA_S64, "Int4", 64);
	psMetadataAdd(args, PS_LIST_TAIL, "-int", PS_DATA_METADATA, "Integers", ints);
	psFree(ints);                  // Drop reference

	// a multiple option example : -multi 1 -multi 2
	psMetadataAddS32(args, PS_LIST_TAIL, "-multi", 0, "666", 2);
	psMetadataAddS32(args, PS_LIST_TAIL, "-multi", PS_META_DUPLICATE_OK, "hello kitty", 1);

	// simple psLib types
	psMetadataAddU8(args, PS_LIST_TAIL, "-U8", 0, "U8", 1);
	psMetadataAddU16(args, PS_LIST_TAIL, "-U16", 0, "U16", 666);
	psMetadataAddU32(args, PS_LIST_TAIL, "-U32", 0, "U32", 666);
	psMetadataAddU64(args, PS_LIST_TAIL, "-U64", 0, "U64", 666);
	psMetadataAddBool(args, PS_LIST_TAIL, "-Bool", 0, "boolean", true);

	// attempt to parse the above arguments using the above argument definition
	ok( psArgumentParse(args, &argc, argv), "psArgumentParse:      return true for valid inputs.");

	// XXX does not check the results...

        // Check for Memory leaks
	psFree(args);
	checkMem();

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");

    }

    // other basic tests
    { 
        psMemId id = psMemGetId();

	// Return false for inconsistent argc in metadata
	{
	    psMetadata *tempArg = psMetadataAlloc();
	    psMetadata *mdtemp = psMetadataAlloc();
	    psMetadataAdd(mdtemp, PS_LIST_TAIL, "int1", PS_DATA_S8, "Int1", 8);
	    psMetadataAdd(mdtemp, PS_LIST_TAIL, "int2", PS_DATA_S16, "Int2", 16);
	    psMetadataAdd(mdtemp, PS_LIST_TAIL, "int3", PS_DATA_S32, "Int3", 32);
	    psMetadataAdd(tempArg, PS_LIST_TAIL, "-int", PS_DATA_METADATA, "Integers", mdtemp);
	    psFree(mdtemp);

	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-string";
	    ok( !psArgumentParse(tempArg, &argc, argv), "psArgumentParse: return false for inconsistent argc.");
	    psFree(tempArg);
	    ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
	}

	// setup the argument definition
	psMetadata *args = psMetadataAlloc();

	// three simple types
	psMetadataAdd(args, PS_LIST_TAIL, "-string", PS_DATA_STRING, "Test String", "SomeString");
	psMetadataAddS32(args, PS_LIST_TAIL, "-multi", 0, "666", 2);
	psMetadataAddS32(args, PS_LIST_TAIL, "-multi", PS_META_DUPLICATE_OK, "hello kitty", 1);

	// Return false for argc = 1
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-string";
	    ok( !psArgumentParse(args, &argc, argv), "psArgumentParse: return false for incomplete string syntax.");
	}

	// Return true for an unfound MULTI match
	{
	    int argc = 3;
	    char *argv[3];
	    argv[0] = "./program";
	    argv[1] = "-multi";
	    argv[2] = "2";
	    ok( psArgumentParse(args, &argc, argv), "psArgumentParse: return true for MULTI.");
	}

	// Check for Memory leaks
	psFree(args);
	ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // test psArgumentVerbosity()
    {
	psMemId id = psMemGetId();
	// Return 2 (default) for NULL input argument
	{
	    int argc = 1;
	    ok( psArgumentVerbosity(&argc, NULL) == 2, "psArgumentVerbosity: return 2 for NULL argument input.");
	}

	// Return 2 for NULL argc input
	{
	    char *argv[1];
	    argv[0] = "./program";
	    ok( psArgumentVerbosity(NULL, argv) == 2, "psArgumentVerbosity: return 2 for NULL argument input.");
	}

	// Return 3 for "-v" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-v";
	    ok( psArgumentVerbosity(&argc, argv) == 3, "psArgumentVerbosity:  return 3 for '-v' argument input.");
	}

	// Return 4 for "-vv" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-vv";
	    ok( psArgumentVerbosity(&argc, argv) == 4, "psArgumentVerbosity:  return 4 for '-vv' argument input.");
	}

	// Return 5 for "-vvv" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-vvv";
	    ok( psArgumentVerbosity(&argc, argv) == 5, "psArgumentVerbosity:  return 5 for '-vvv' argument input.");
	}

	// These routines should return 5 since that was the last level set above with -vvv
	// Return 5 for "-logfmt" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-logfmt";
	    int rc = psArgumentVerbosity(&argc, argv);
	    ok(rc == 5, "psArgumentVerbosity:  return 5 for '-logfmt' argument input (was %d)", rc);
	}

	// Return 5 for "-logfmt" option with "H"
	{
	    int argc = 3;
	    char *argv[3];
	    argv[0] = "./program";
	    argv[1] = "-logfmt";
	    argv[2] = "H";
	    int rc = psArgumentVerbosity(&argc, argv);
	    ok(rc == 5, "psArgumentVerbosity:  return 5 for '-logfmt H' argument inputs (was %d)", rc);
	}

	// Return 5 for "-trace" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-trace";
	    int rc = psArgumentVerbosity(&argc, argv);
	    ok(rc == 5, "psArgumentVerbosity:  return 5 for '-trace' argument input (was %d)", rc);
	}

	// Return 5 for "-trace 1 2" option
	{
	    int argc = 4;
	    char *argv[4];
	    argv[0] = "./program";
	    argv[1] = "-trace";
	    argv[2] = "1";
	    argv[3] = "2";
	    // XXX EAM : we are getting unneeded output because of the verbosity set above.
	    int rc = psArgumentVerbosity(&argc, argv);
	    ok(rc == 5, "psArgumentVerbosity:  return 5 for '-trace 1 2' argument inputs (was %d", rc);
	}

	// Return 5 for "-trace-levels" option
	{
	    // this function sends output to the screen or /dev/null (i
# if (!DEBUG)
	    FILE *f = fopen ("/dev/null", "w");
	    int fd = fileno(f);
	    psTraceSetDestination (fd);
# endif

	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-trace-levels";
	    int rc = psArgumentVerbosity(&argc, argv);
	    ok(rc == 5, "psArgumentVerbosity:  return 5 for '-trace-levels' argument input (was %d)", rc);
	}
	ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    /** EAM 2019.11.09 this section is commented out because these functions do not exist
    void testLogTraceArguments(void)
    {
	note("  >>>Test 3:  psLogArguments & psTraceArguments Fxns");
 
	// Return 2 (default) for NULL arguments input
	{
	    int argc = 4;
	    ok( psLogArguments(&argc, NULL) == 2, "psLogArguments: return 2 for NULL argument input.");
	}

	// Return 2 (default) for NULL argc input
	{
	    char *argv[1];
	    argv[0] = "./program";
	    ok( psLogArguments(NULL, argv) == 2, "psLogArguments: return 2 for NULL argc input.");
	}

	// Return 3 for "-v" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-v";
	    ok( psLogArguments(&argc, argv) == 3, "psLogArguments: return 3 for '-v' argument input.");
	}

	// Return 4 for "-vv" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-vv";
	    ok( psLogArguments(&argc, argv) == 4, "psLogArguments: return 4 for '-vv' argument input.");
	}

	// Return 5 for "-vvv" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-vvv";
	    ok( psLogArguments(&argc, argv) == 5, "psLogArguments: return 5 for '-vvv' argument input.");
	}

	// Return 2 for "-logfmt" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-logfmt";
	    ok( psLogArguments(&argc, argv) == 2, "psLogArguments: return 2 for '-logfmt' argument input.");
	}

	// Return 2 for "-logfmt" option with "H"
	{
	    int argc = 3;
	    char *argv[3];
	    argv[0] = "./program";
	    argv[1] = "-logfmt";
	    argv[2] = "H";
	    ok( psLogArguments(&argc, argv) == 2, "psLogArguments: return 2 for '-logfmt H' argument inputs.");
	}
 
	// psTraceArguments Tests
	// Return 0 (default) for NULL arguments input
	{
	    int argc = 4;
	    ok( psTraceArguments(&argc, NULL) == 0, "psTraceArguments: return 0 for NULL argument input.");
	}

	// Return 0 (default) for NULL argc input
	{
	    char *argv[1];
	    argv[0] = "./program";
	    ok( psTraceArguments(NULL, argv) == 0, "psTraceArguments: return 0 for NULL argc input.");
	}

	// Return 0 for "-trace" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-trace";
	    ok( psTraceArguments(&argc, argv) == 0, "psTraceArguments: return 2 for '-trace' argument input.");
	}

	// Return 2 for "-trace 1 2" option
	{
	    int argc = 4;
	    char *argv[4];
	    argv[0] = "./program";
	    argv[1] = "-trace";
	    argv[2] = "1";
	    argv[3] = "2";
	    ok( psTraceArguments(&argc, argv) == 1, "psTraceArguments: return 2 for '-trace 1 2' argument inputs.");
	}

	// Return 2 for "-trace-levels" option
	{
	    int argc = 2;
	    char *argv[2];
	    argv[0] = "./program";
	    argv[1] = "-trace-levels";
	    ok( psTraceArguments(&argc, argv) == 0, "psTraceArguments: return 2 for '-trace-levels' argument input.");
	}
 
	// Check for Memory leaks
	ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
    **/
}

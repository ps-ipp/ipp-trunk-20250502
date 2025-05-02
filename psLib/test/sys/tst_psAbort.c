/** @file  atst_psAbort_01.c
 *
 *  @brief Test driver for psAbort function
 *
 *  This test drivers contains the following test points for psAbort
 *     1) Multiple type values in abort message
 *
 *  @author  Eric Van Alst, MHPCC
 *
 *  @version $Revision: 1.1 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2005-07-13 02:47:01 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include "pslib_strict.h"
#include "psTest.h"

static psS32 testAbort00(void);
static psS32 testAbort01(void);
static psS32 testAbort02(void);

testDescription tests[] = {
                              {testAbort00, 0, "Multiple type values in abort message", -6, false},
                              {testAbort01, 1, "String values in abort message", -6, false},
                              {testAbort02, 2, "Empty strings in abort message", -6, false},
                              {NULL}
                          };

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetLevel( PS_LOG_INFO );

    return ( ! runTestSuite( stderr, "psAbort", tests, argc, argv ) );
}

static psS32 testAbort00(void)
{
    psS32   intval = 1;
    psS64  longval = 2;
    float floatval = 3.01;
    char  charval = 'E';
    char  *stringval = "E R R O R";

    // Test point #1 Multiple type values placed in the error string
    psAbort(__func__,
            "ALL TYPES intval = %d longval = %lld floatval = %f charval = %c strval = %s",
            intval, longval, floatval, charval, stringval );

    // Program execution should have ended before this statement but if it
    // does not return a zero since the expected return value of this test
    // is a non-zero value
    return 0;
}

static psS32 testAbort01(void)
{
    // Test point #2 String values in abort message
    psAbort(PS_STRING(__LINE__), "NO_VALUES");

    // Program execution should have ended before this statement but if it
    // does not return a zero since the expected return value of this test
    // is a non-zero value
    return 0;
}

static psS32 testAbort02(void)
{
    // Test point #2 String values in abort message
    psAbort("","");

    // Program execution should have ended before this statement but if it
    // does not return a zero since the expected return value of this test
    // is a non-zero value
    return 0;
}

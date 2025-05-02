/*****************************************************************************
    This code will test whether trace levels can be set successfully.
 
  XXX : various tests result in text sent to stdout -- these should go to a file 
        or buffer and be validated against a truth set.

  XXX : some of the error messages have the wrong sense (they report a negative result even if the test is successful)
*****************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "pslib.h"
#include "tap.h"
#include "pstap.h"


psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(54);

# define DEBUG 1
# if (!DEBUG)
    FILE *output = fopen ("/dev/null", "w");
    int outFD = fileno (output);
# else
    int outFD = 2;
# endif

    // testTrace00()
    {
        psMemId id = psMemGetId();
        psTraceSetDestination(outFD);
        for (int i = 0; i < 10; i++) {
            psTraceSetLevel(".", i);
            int lev = psTraceGetLevel(".");
            ok (lev == i, "trace level was %d, actual was %d", i, lev);
        }

        psTraceSetLevel(".", 3);
        for (int i = 5; i < 10;i++) {
            psTraceSetLevel(".NODE00", i);
            int lev1 = psTraceGetLevel(".NODE00");
            ok (lev1 == i,"(.NODE00) expected trace level was %d, actual was %d", i, lev1);
    
            int lev2 = psTraceGetLevel(".");
            ok (lev2 == 3, "expected trace level was %d, actual was %d", 3, lev2);
        }
    
        psTraceSetLevel(".NODE00.NODE01", 4);
        for (int i = 0; i < 10; i++) {
            psTraceSetLevel(".NODE00.NODE01", i);
            int lev = psTraceGetLevel(".NODE00.NODE01");
            ok (lev == i, "(.NODE00.NODE01) expected trace level was %d, actual was %d", i, lev);
        }
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testTrace01()
    // XXX need to check the output on these as part of the test
    {
        psMemId id = psMemGetId();
        psTraceSetDestination(outFD);
        psTraceSetLevel(".A.B.C.D.E", 5);
        psTrace(".A.C.D.C",     1, "You should not see this");
        psTrace(".A.B.C.D.E",   2, "You should see this");
        psTrace(".A.B.C.D.E.F", 3, "You should see this too");
        psTracePrintLevels();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // testTrace02()
    {
        psMemId id = psMemGetId();
        psTraceReset();
        psTraceSetDestination(outFD);
        psTraceSetLevel(".A.B", 2);
        psTraceSetLevel(".A.B.C.D.E", 5);
        psTracePrintLevels();
        psTraceSetLevel(".A.B", 10);
        psTracePrintLevels();

        ok (10 == psTraceGetLevel(".A.B.C"),     ".A.B.C did not dynamically inherit a trace level (%d)", psTraceGetLevel(".A.B.C"));
        ok (10 == psTraceGetLevel(".A.B.C.D"),   ".A.B.C.D did not dynamically inherit a trace level (%d)", psTraceGetLevel(".A.B.C.D"));
        ok ( 5 == psTraceGetLevel(".A.B.C.D.E"), ".A.B.C.D.E did dynamically inherit a trace level (%d)", psTraceGetLevel(".A.B.C.D.E"));
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // testTrace03()
    {
        psMemId id = psMemGetId();
        psTraceSetDestination(outFD);

        for (int i = 0; i < 10; i++) {
            psTraceSetLevel(".", i);
            psTraceReset();
            int lev = psTraceGetLevel(".");
            ok (lev == PS_UNKNOWN_TRACE_LEVEL, "expected trace level was %d, actual was %d", PS_UNKNOWN_TRACE_LEVEL, lev);
        }

        psTraceSetLevel(".", 5);
        psTraceSetLevel(".a", 4);
        psTraceSetLevel(".a.b", 3);
        psTraceSetLevel(".a.b.c", 2);
        ok (5 == psTraceGetLevel("."), "level 0");
	ok (4 == psTraceGetLevel(".a"), "level 1");
	ok (3 == psTraceGetLevel(".a.b"), "level 2");
	ok (2 == psTraceGetLevel(".a.b.c"), "level 3");

        psTraceReset();
        ok (PS_UNKNOWN_TRACE_LEVEL == psTraceGetLevel("."), "trace levels were not reset properly");
	ok (PS_UNKNOWN_TRACE_LEVEL == psTraceGetLevel(".a"), "trace levels were not reset properly");
	ok (PS_UNKNOWN_TRACE_LEVEL == psTraceGetLevel(".a.b"), "trace levels were not reset properly");
	ok (PS_UNKNOWN_TRACE_LEVEL == psTraceGetLevel(".a.b.c"), "trace levels were not reset properly");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // testTrace04()
    {
        psMemId id = psMemGetId();
        int FD = creat("tst_psTrace02_OUT", 0666);
        for (int nb = 0; nb < 4; nb++) {
            if (nb == 0) psTraceSetDestination(((outFD == 2) ? 1 : outFD));
            if (nb == 1) psTraceSetDestination(((outFD == 2) ? 2 : outFD));
            if (nb == 2) psTraceSetDestination(((outFD == 2) ? 0 : outFD));
            if (nb == 3) psTraceSetDestination(FD);

            psTraceSetLevel(".", 4);
            psTrace(".", 5, "(0) This message should not be displayed (%x)", 0xbeefface);
            psTraceSetLevel(".", 7);
            psTrace(".", 5, "(0) This message should be displayed (%x)", 0xbeefface);

            psTraceSetLevel(".a", 4);
            psTrace(".a", 5, "(1) This message should not be displayed (%x)", 0xbeefface);
            psTraceSetLevel(".a", 7);
            psTrace(".a", 5, "(1) This message should be displayed (%x)", 0xbeefface);

            psTraceSetLevel(".a.b", 4);
            psTrace(".a.b", 5, "(2) This message should not be displayed (%x)", 0xbeefface);
            psTraceSetLevel(".a.b", 7);
            psTrace(".a.b", 5, "(2) This message should be displayed (%x)", 0xbeefface);

            psTraceSetLevel(".a.b.c", 12);
            psTrace(".a.b.c", 11, "(3) This message should be displayed (%x)", 0xbeefface);
        }
        close(FD);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testTrace05()
    {
        psMemId id = psMemGetId();
        psTraceSetDestination(outFD);
        psTraceSetLevel(".", 9);
        psTraceSetLevel(".a", 8);
        psTraceSetLevel(".b", 7);
        psTraceSetLevel(".c", 5);
        psTraceSetLevel(".a.a", 4);
        psTraceSetLevel(".a.b", 3);
        psTraceSetLevel(".b.a", 2);
        psTraceSetLevel(".b.b", 1);
        psTraceSetLevel(".c.a", 0);
        psTraceSetLevel(".c.b", 3);
        psTraceSetLevel(".c.c", 5);
        psTracePrintLevels();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testTrace05a()
    {
        psMemId id = psMemGetId();
        psTraceSetLevel(".", 9);
        psTraceSetLevel("a", 8);
        psTraceSetLevel("b", 7);
        psTraceSetLevel("c", 5);
        psTraceSetLevel("a.a", 4);
        psTraceSetLevel("a.b", 3);
        psTraceSetLevel("b.a", 2);
        psTraceSetLevel("b.b", 1);
        psTraceSetLevel("c.a", 0);
        psTraceSetLevel("c.b", 3);
        psTraceSetLevel("c.c", 5);
        psTracePrintLevels();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testTrace06()
    {
        psMemId id = psMemGetId();
        psTraceSetDestination(outFD);
        psTraceSetLevel(".", 9);
        psTraceSetLevel(".a", 8);
        psTraceSetLevel(".b", 7);
        psTraceSetLevel(".c", 5);
        psTraceSetLevel(".a.a", 4);
        psTraceSetLevel(".a.b", 3);
        psTraceSetLevel(".b.a", 2);
        psTraceSetLevel(".b.b", 1);
        psTraceSetLevel(".c.a", 0);
        psTraceSetLevel(".c.b", 3);
        psTraceSetLevel(".c.c", 5);
        psTraceReset();

	// the reset should clear all of the levels above
	ok (psTraceGetLevel(".")    == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".a")   == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".b")   == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".c")   == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".a.a") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".a.b") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".b.a") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".b.b") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".c.a") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".c.b") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");
	ok (psTraceGetLevel(".c.c") == PS_UNKNOWN_TRACE_LEVEL, "valid trace level");

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Ensure that the leading dot in the component names are optional.
    // testTrace08()
    {
        psMemId id = psMemGetId();
        psTraceReset();
        psTraceSetLevel(".", 9);
        psTraceSetLevel(".a", 8);
        psTraceSetLevel(".b", 7);
        psTraceSetLevel(".c", 5);
        psTraceSetLevel(".a.a", 4);
        psTraceSetLevel(".a.b", 3);
        psTraceSetLevel(".b.a", 2);
        psTraceSetLevel(".b.b", 1);
        psTraceSetLevel(".c.a", 0);
        psTraceSetLevel(".c.b", 3);
        psTraceSetLevel(".c.c", 5);
        psTracePrintLevels();

        ok(psTraceGetLevel(".")   == 9, "level is valid without first dot");
        ok(psTraceGetLevel("a")   == 8, "level is valid without first dot");
        ok(psTraceGetLevel("b")   == 7, "level is valid without first dot");
        ok(psTraceGetLevel("c")   == 5, "level is valid without first dot");
        ok(psTraceGetLevel("a.a") == 4, "level is valid without first dot");
        ok(psTraceGetLevel("a.b") == 3, "level is valid without first dot");
        ok(psTraceGetLevel("b.a") == 2, "level is valid without first dot");
        ok(psTraceGetLevel("b.b") == 1, "level is valid without first dot");
        ok(psTraceGetLevel("c.a") == 0, "level is valid without first dot");
        ok(psTraceGetLevel("c.b") == 3, "level is valid without first dot");
        ok(psTraceGetLevel("c.c") == 5, "level is valid without first dot");

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

# if (DEBUG)
    close (outFD);
# endif

}

/** @file  tst_psLine.c
 *
 *  @brief Test driver for psLine functions
 *
 *  @author  dRob, MHPCC
 *
 *  @version $Revision: 1.4 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-04-10 21:09:31 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "pslib.h"
#include "tap.h"
#include "pstap.h"
#include "string.h"

psS32 main( psS32 argc, char* argv[] )
{
    plan_tests(16);
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);

    // testLineAlloc()
    {
        psMemId id = psMemGetId();
        psLine *lineline = NULL;
        lineline = psLineAlloc(20);
        ok(lineline->NLINE == 20, "psLine set NLINE parameter during Allocation");
        ok(lineline->Nline ==  0, "psLine set Nline parameter during Allocation");
        strncpy(lineline->line, "Hello World", 20);
        ok(!strncmp(lineline->line, "Hello World", 20), "psLine was stored a simple string!");
        psFree(lineline);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testLineInit()
    {
        psMemId id = psMemGetId();
        psLine *line = NULL;
        // Return false for NULL input
        int okay = !psLineInit(line);
        ok(okay, "psLineInit.  Expected false for NULL psLine input");
        // Allocate a line and return true on Init
        line = psLineAlloc(1);
        okay = psLineInit(line);
        ok(okay, "psLineInit.  Expected true for valid psLine input");
        ok(line->NLINE == 1 && line->Nline == 0, "psLineInit returned line parameters.");
        psFree(line);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testLineAdd()
    {
        psMemId id = psMemGetId();
        psLine *line = NULL;
        //Return false for NULL input
        ok(!psLineAdd(line, "Hello World"),
            "psLineAdd.  Expected false for NULL psLine input.");
        //Allocate and return true for valid input.
        line = psLineAlloc(20);
        int okay = psLineAdd(line, "Hello %s", "World");
        ok( okay, "psLineAdd.  Expected true for valid psLine input");
        ok(line->NLINE == 20 && line->Nline == 11, "psLineAdd failed to return the correct line parameters");
        ok(!strncmp(line->line, "Hello World", 20), "psLineAdd failed to store the correct line string.");
        psFree(line);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testLineChk()
    {
        psMemId id = psMemGetId();
        psLine *line = NULL;
        //Return false for Null input line
        ok(!psMemCheckLine(line), "psMemCheckLine return false for NULL line input");
        line = psLineAlloc(1);
        ok(psMemCheckLine(line), "psMemCheckLine return true for valid line input");
        psFree(line);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

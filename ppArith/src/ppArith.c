/** @file ppArith.c
 *
 *  @brief
 *
 *  @ingroup ppArith
 *
 *  @author IfA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 19:45:30 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppArith.h"

int main(int argc, char *argv[])
{
    psExit exitValue = PS_EXIT_SUCCESS; // Exit value
    psTimerStart("ppArith");
    psLibInit(NULL);

    pmConfig *config = pmConfigRead(&argc, argv, PPARITH_RECIPE); // Configuration
    if (!config) {
        psErrorStackPrint(stderr, "Error reading configuration.");
        exitValue = PS_EXIT_CONFIG_ERROR;
        goto die;
    }

    ppArithVersionPrint();

    if (!ppArithArguments(argc, argv, config)) {
        psErrorStackPrint(stderr, "Error reading arguments.\n");
        exitValue = PS_EXIT_CONFIG_ERROR;
        goto die;
    }

    if (!ppArithLoop(config)) {
        psErrorStackPrint(stderr, "Error performing arithmetic.\n");
        exitValue = PS_EXIT_PROG_ERROR;
        goto die;
    }

 die:
    psTrace("ppArith", 1, "Finished at %f sec\n", psTimerMark("ppArith"));
    psTimerStop();

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(exitValue);
}

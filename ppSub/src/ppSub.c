/** @file ppSub.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

int main(int argc, char *argv[])
{
    psLibInit(NULL);
    psTimerStart("ppSub");

    pmErrorRegister();
    ppSubErrorRegister();
    psphotErrorRegister();

    ppSubData *data = NULL;             // Processing data
    pmConfig *config = pmConfigRead(&argc, argv, PPSUB_RECIPE); // Configuration
    if (!config) {
        goto die;
    }

    ppSubVersionPrint();

    if (!pmModelClassInit()) {
        psError(PPSUB_ERR_PROG, false, "Unable to initialise model classes.");
        psFree(config);
        goto die;
    }

    if (!psphotInit()) {
        psError(PPSUB_ERR_PROG, false, "Error initialising psphot.");
        psFree(config);
        goto die;
    }

    data = ppSubDataAlloc(config);

    if (!ppSubArguments(argc, argv, data)) {
        psError(psErrorCodeLast(), false, "Error reading arguments.");
        goto die;
    }

    if (!ppSubCamera(data)) {
        goto die;
    }

    if (!ppSubLoop(data)) {
        goto die;
    }

 die:
    {
        psExit exitValue = ppSubExitCode(PS_EXIT_SUCCESS); // Exit code

        if (data && data->stats && data->statsFile) {
  	    psMetadataAddF32(data->stats, PS_LIST_TAIL, "TIME_DIFF", 0, "Total time (sec)", psTimerMark("ppSub"));
            psString stats = psMetadataConfigFormat(data->stats); // Statistics to output
            if (!stats || strlen(stats) == 0) {
                psError(PPSUB_ERR_IO, false, "Unable to format statistics file");
            } else if (fprintf(data->statsFile, "%s", stats) != strlen(stats)) {
                psError(PPSUB_ERR_IO, true, "Unable to write statistics file");
            }
            psFree(stats);
            if (fclose(data->statsFile) == EOF) {
                psError(PPSUB_ERR_IO, true, "Unable to close statistics file");
            }
            data->statsFile = NULL;
            pmConfigRunFilenameAddWrite(data->config, "STATS", data->statsName);
            exitValue = ppSubExitCode(exitValue);
        }

        if (config && !ppSubFilesIterateUp(config, PPSUB_FILES_ALL)) {
            psError(psErrorCodeLast(), false, "Unable to close files.");
            exitValue = ppSubExitCode(exitValue);
            pmFPAfileFreeSetStrict(false);
        }

        if (data) {
            psString dump_file = psMetadataLookupStr(NULL, data->config->arguments, "DUMP_CONFIG");
            if (dump_file) {
                if (!pmConfigDump(data->config, dump_file)) {
                    psError(PPSUB_ERR_IO, false, "Unable to dump configuration.");
                    exitValue = ppSubExitCode(exitValue);
                }
            }
            psFree(data);
        }

        psTrace("ppSub", 1, "Finished at %f sec\n", psTimerMark("ppSub"));
	psLogMsg("ppSub", PS_LOG_INFO, "Complete ppSub run: %f sec\n", psTimerMark("ppSub"));
        psTimerStop();

        pmVisualClose(); //close plot windows, if -visual is set
        pmModelClassCleanup();
        pmConfigDone();
	pmVisualCleanup ();
        psLibFinalize();

	fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stdout, false), "ppSub");
        exitValue = ppSubExitCode(exitValue);
        exit(exitValue);
    }
}

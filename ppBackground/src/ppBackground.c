#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppBackground.h"

int main(int argc, char *argv[])
{
    ppBackgroundVersionPrint();

    pmErrorRegister();
    psphotErrorRegister();
    ppBackgroundErrorRegister();

    printf("Initializing data\n");
    ppBackgroundData *data = ppBackgroundDataInit(&argc, argv);
    if (!data) {
        goto DIE;
    }
    printf("Reading Arguments\n");
    if (!ppBackgroundArguments(data, argc, argv)) {
        goto DIE;
    }
    printf("Setting up Camera\n");
    if (!ppBackgroundCamera(data)) {
        goto DIE;
    }

    printf("Looping over data!\n");
    if (!ppBackgroundLoop(data)) {
        goto DIE;
    }

 DIE:
    ; // Empty statement to satisy compiler
    psExit exitValue = ppBackgroundExitCode(PS_EXIT_SUCCESS); // Exit code

    if (data && data->stats && data->statsFile) {
        psString stats = psMetadataConfigFormat(data->stats); // Statistics to output
        if (!stats || strlen(stats) == 0) {
            psError(PPBACKGROUND_ERR_IO, false, "Unable to format statistics file");
        } else if (fprintf(data->statsFile, "%s", stats) != strlen(stats)) {
            psError(PPBACKGROUND_ERR_IO, true, "Unable to write statistics file");
        }
        psFree(stats);
        if (fclose(data->statsFile) == EOF) {
            psError(PPBACKGROUND_ERR_IO, true, "Unable to close statistics file");
        }
        data->statsFile = NULL;
        exitValue = ppBackgroundExitCode(exitValue);
    }

    if (data) {
        psString dump_file = psMetadataLookupStr(NULL, data->config->arguments, "-dumpconfig");
        if (dump_file) {
            if (!pmConfigDump(data->config, dump_file)) {
                psError(psErrorCodeLast(), false, "Unable to dump configuration.");
                exitValue = ppBackgroundExitCode(exitValue);
            }
        }
        psFree(data);
    }

    pmConfigDone();
    psLibFinalize();

    return ppBackgroundExitCode(exitValue);
}


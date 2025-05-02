#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoMakeCorr.h"

int main(int argc, char **argv) {

    psLibInit(NULL);

    psTimerStart(TIMERNAME);

    // USAGE: dvoMakeCorr (mosaic.fits) (output)

    // Parse the configuration and arguments
    pmConfig *config = dvoMakeCorrArguments (argc, argv);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // define recipe options
    // define the active I/O files
    dvoMakeCorrOptions *options = dvoMakeCorrParseCamera(config);
    if (options == NULL) {
        psErrorStackPrint(stderr, "Unable to parse camera.\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Image Loop
    if (!dvoMakeCorrLoop(config, options)) {
        psErrorStackPrint(stderr, "Unable to loop over input\n");
        exit(PS_EXIT_SYS_ERROR);
    }

    psLogMsg ("ppImage", 3, "Complete dvoMakeCorr run: %f sec\n", psTimerMark(TIMERNAME));

    // Cleaning up
    dvoMakeCorrCleanup(config, options);

    return PS_EXIT_SUCCESS;
}


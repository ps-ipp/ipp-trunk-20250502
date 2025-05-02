#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoApplyCorr.h"

int main(int argc, char **argv) {

    psLibInit(NULL);

    psTimerStart(TIMERNAME);

    // USAGE: dvoApplyCorr -file INPUT OUTPUT

    // Parse the configuration and arguments
    pmConfig *config = dvoApplyCorrArguments (argc, argv);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // define recipe options
    // define the active I/O files
    dvoApplyCorrOptions *options = dvoApplyCorrParseCamera(config);
    if (options == NULL) {
        psErrorStackPrint(stderr, "Unable to parse camera.");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Image Loop
    if (!dvoApplyCorrLoop(config, options)) {
        psErrorStackPrint(stderr, "Unable to loop over input");
        exit(PS_EXIT_SYS_ERROR);
    }

    psLogMsg ("ppImage", 3, "Complete ppImage run: %f sec\n", psTimerMark(TIMERNAME));

    // Cleaning up
    dvoApplyCorrCleanup(config, options);

    return PS_EXIT_SUCCESS;
}


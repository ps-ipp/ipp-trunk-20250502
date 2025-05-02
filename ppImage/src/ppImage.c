#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

int main(int argc, char **argv) {

    psLibInit(NULL);

    psTimerStart(TIMER_TOTAL);

    // Parse the configuration and arguments
    // Open the input image(s)
    // Determine camera, format from header if not already defined
    // Construct camera in preparation for reading
    pmConfig *config = ppImageArguments(argc, argv);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.");
        ppImageCleanup(config, NULL);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    ppImageVersionPrint();

    // define recipe options
    // define the active I/O files
    ppImageOptions *options = ppImageParseCamera(config);
    if (options == NULL) {
        psErrorStackPrint(stderr, "Unable to parse camera.");
        ppImageCleanup(config, options);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Image Arithmetic Loop
    if (!ppImageLoop(config, options)) {
        psErrorStackPrint(stderr, "Unable to loop over input");
        ppImageCleanup(config, options);
        exit(PS_EXIT_SYS_ERROR);
    }

    psLogMsg("ppImage", PS_LOG_INFO, "Complete ppImage run: %f sec\n", psTimerMark(TIMER_TOTAL));

    // Cleaning up
    ppImageCleanup(config, options);

    return PS_EXIT_SUCCESS;
}

#include "ppSmooth.h"

int main(int argc, char **argv) {

    psLibInit(NULL);

    psTimerStart(TIMER_TOTAL);

    // Parse the configuration and arguments
    // Open the input image(s)
    // Determine camera, format from header if not already defined
    // Construct camera in preparation for reading
    pmConfig *config = ppSmoothArguments(argc, argv);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.");
        ppSmoothCleanup(config);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    ppSmoothVersionPrint();

    // define recipe options
    // define the active I/O files
    if (!ppSmoothParseCamera(config)) {
        psErrorStackPrint(stderr, "Unable to parse camera.");
        ppSmoothCleanup(config);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Image Arithmetic Loop
    if (!ppSmoothLoop(config)) {
        psErrorStackPrint(stderr, "Unable to loop over input");
        ppSmoothCleanup(config);
        exit(PS_EXIT_SYS_ERROR);
    }

    psLogMsg("ppSmooth", PS_LOG_INFO, "Complete ppSmooth run: %f sec\n", psTimerMark(TIMER_TOTAL));

    // Cleaning up
    ppSmoothCleanup(config);

    return PS_EXIT_SUCCESS;
}

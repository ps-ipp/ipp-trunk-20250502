#include "ppNoiseMap.h"

int main(int argc, char **argv) {

    psLibInit(NULL);

    psTimerStart(TIMER_TOTAL);

    // Parse the configuration and arguments
    // Open the input image
    // Determine camera, format from header if not already defined
    // Construct camera in preparation for reading
    pmConfig *config = ppNoiseMapArguments(argc, argv);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.");
        ppNoiseMapCleanup(config);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    ppNoiseMapVersionPrint();

    // define the active I/O files
    if (!ppNoiseMapParseCamera(config)) {
        psErrorStackPrint(stderr, "Unable to parse camera.");
        ppNoiseMapCleanup(config);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Image Arithmetic Loop
    if (!ppNoiseMapLoop(config)) {
        psErrorStackPrint(stderr, "Unable to loop over input");
        ppNoiseMapCleanup(config);
        exit(PS_EXIT_SYS_ERROR);
    }

    psLogMsg("ppNoiseMap", PS_LOG_INFO, "Complete ppNoiseMap run: %f sec\n", psTimerMark(TIMER_TOTAL));

    // Cleaning up
    ppNoiseMapCleanup(config);

    return PS_EXIT_SUCCESS;
}

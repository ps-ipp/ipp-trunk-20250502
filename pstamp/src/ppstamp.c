#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppstamp.h"

int main(int argc, char **argv)
{
    ppstampOptions *options = NULL;
    int exitCode;

    psTimerStart(TIMERNAME);
    psLibInit(NULL);
    pstampErrorRegister();

    pmConfig *config = ppstampArguments(argc, argv, &options);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.");
        ppstampCleanup(config, options);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // define the active I/O files
    if (!ppstampParseCamera(config, options)) {
        psErrorStackPrint(stderr, "Unable to parse camera.");
        ppstampCleanup(config, options);
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // find the pixels that we need to copy, setup the output image
    exitCode = ppstampMakeStamp(config, options);

    psLogMsg ("ppstamp", 3, "Complete ppstamp run: %f sec\n", psTimerMark(TIMERNAME));

    // Cleaning up
    ppstampCleanup(config, options);

    return exitCode;
}

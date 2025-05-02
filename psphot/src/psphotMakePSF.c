# include "psphotStandAlone.h"

int main (int argc, char **argv) {

    psTimerStart ("complete");
    pmErrorRegister();                  // register psModule's error codes/messages
    psphotInit();

    // load command-line arguments, options, and system config data
    pmConfig *config = psphotMakePSFArguments (argc, argv);
    assert(config);

    psphotVersionPrint();

    // load input data (config and images (signal, noise, mask)
    if (!psphotParseCamera (config)) {
        psErrorStackPrint(stderr, "Error setting up the camera\n");
        exit (psphotGetExitStatus());
    }

    // call psphot for each readout
    if (!psphotImageLoop (config, PSPHOT_MAKE_PSF)) {
        psErrorStackPrint(stderr, "Error in the psphot image loop\n");
        exit (psphotGetExitStatus());
    }

    psLogMsg ("psphot", 3, "complete psphot run: %f sec\n", psTimerMark ("complete"));

    psErrorCode exit_status = psphotGetExitStatus();
    psphotCleanup (config);
    exit (exit_status);
}

// all functions which return to this level must raise one of the top-level error codes if they
// exit with an error.  these error codes are used to specify the program exit status

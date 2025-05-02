# include "psphotStandAlone.h"

int main (int argc, char **argv) {

    // Set flag to tell certain library functions that we are in the psphotStack program. 
    // (This is defined in psphotDefineFiles.c)
    psphotINpsphotStack = true;

    // uncomment to turn on memory dumps (move this to an option)
    // psMemDumpSetState(true);

    psTimerStart ("complete");
    pmErrorRegister();                  // register psModule's error codes/messages
    psphotInit();

    // load command-line arguments, options, and system config data
    pmConfig *config = psphotStackArguments (argc, argv);
    assert(config);

    psphotVersionPrint();

    psMemDump("start");

    // load input data (config and images (signal, noise, mask)
    if (!psphotStackParseCamera (config)) {
        psErrorStackPrint(stderr, "Error setting up the camera\n");
        exit (psphotGetExitStatus());
    }

    // call psphot for each readout
    if (!psphotStackImageLoop (config)) {
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

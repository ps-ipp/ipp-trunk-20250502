#include "ppStack.h"

int main(int argc, char *argv[])
{
    psLibInit(NULL);
    psTimerStart("PPSTACK");
    psTimerStart("PPSTACK_STEPS");

    pmErrorRegister();
    ppStackErrorRegister();
    psphotErrorRegister();

    ppStackOptions *options = NULL;                               // Options for stacking

    pmConfig *config = pmConfigRead(&argc, argv, PPSTACK_RECIPE); // Configuration
    if (!config) {
	ppStackCleanup(config, options);
    }

    ppStackVersionPrint();

    if (!pmModelClassInit()) {
        psError(PPSTACK_ERR_PROG, false, "Unable to initialise model classes.");
	ppStackCleanup(config, options);
    }

    if (!psphotInit()) {
        psError(PPSTACK_ERR_PROG, false, "Error initialising psphot.");
	ppStackCleanup(config, options);
    }

    if (!ppStackArgumentsSetup(argc, argv, config)) {
	ppStackCleanup(config, options);
    }

    if (!ppStackCamera(config)) {
	ppStackCleanup(config, options);
    }

    if (!ppStackArgumentsParse(config)) {
	ppStackCleanup(config, options);
    }

    options = ppStackOptionsAlloc();
    if (!ppStackLoop(config, options)) {
	ppStackCleanup(config, options);
    }

    ppStackCleanup(config, options);
}


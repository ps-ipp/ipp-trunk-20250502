# include "ppSim.h"

int failure (pmConfig *config, psExit status, char *message) { 
    ppSimRandomGaussianFree ();
    psErrorStackPrint(stderr, "%s", message);
    psFree(config);
    pmModelClassCleanup();
    psLibFinalize();
    exit (status);
}

int main(int argc, char *argv[])
{
    psLibInit(NULL);
    if (!pmModelClassInit ()) abort();

    pmConfig *config = pmConfigRead(&argc, argv, PPSIM_RECIPE); // Configuration
    if (!config) {
	failure (config, PS_EXIT_CONFIG_ERROR, "Unable to read configurations.");
    }

    if (!ppSimArguments(argc, argv, config)) {
	failure (config, PS_EXIT_CONFIG_ERROR, "Error parsing command-line arguments");
    }

    if (!ppSimCreate(config)) {
	failure (config, PS_EXIT_CONFIG_ERROR, "Unable to create output file.");
    }

    if (!ppSimLoop(config)) {
	failure (config, PS_EXIT_SYS_ERROR, "Unable to generate data.");
    }

    ppSimRandomGaussianFree ();
    psFree(config);
    pmModelClassCleanup();
    pmConfigDone();
    pmConceptsDone();
    psLibFinalize();

    exit (PS_EXIT_SUCCESS);
}

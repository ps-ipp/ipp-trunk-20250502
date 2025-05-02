# include "ppNoiseMap.h"

void ppNoiseMapCleanup (pmConfig *config)
{
    // Free memory used by ppNoiseMap
    psFree(config);

    // Free memory used by psModules
    pmSourceFitSetDone ();
    pmConceptsDone();
    pmConfigDone();
    pmModelClassCleanup();

    // Free memory used by psLib
    psLibFinalize();

    fprintf(stderr, "Found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stdout, false), "ppNoiseMap");

    return;
}

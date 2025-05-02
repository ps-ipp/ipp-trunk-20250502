# include "ppSmooth.h"

void ppSmoothCleanup (pmConfig *config)
{
    // Free memory used by ppSmooth
    psFree(config);

    // Free memory used by psModules
    pmSourceFitSetDone ();
    pmConceptsDone();
    pmConfigDone();
    pmModelClassCleanup();

    // Free memory used by psLib
    psLibFinalize();

    // fprintf(stderr, "Found %d leaks at %s\n", psMemCheckLeaks (0, NULL, NULL, false), "ppSmooth");
    fprintf(stderr, "Found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stdout, false), "ppSmooth");

    return;
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "dvoMakeCorr.h"

void dvoMakeCorrCleanup (pmConfig *config, dvoMakeCorrOptions *options)
{
    // Free memory used by dvoMakeCorrCleanup
    psFree(options);
    psFree(config);

    // Free memory used by psModules
    pmConceptsDone();
    pmConfigDone();

    // Free memory used by psLib
    psLibFinalize();
    fprintf (stderr, "Found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stderr, false), "dvoMakeCorrCleanup");

    return;
}

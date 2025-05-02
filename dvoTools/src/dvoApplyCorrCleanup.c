#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "dvoApplyCorr.h"

void dvoApplyCorrCleanup (pmConfig *config, dvoApplyCorrOptions *options)
{
    // Free memory used by dvoApplyCorrCleanup
    psFree(options);
    psFree(config);

    // Free memory used by psModules
    pmConceptsDone();
    pmConfigDone();

    // Free memory used by psLib
    psLibFinalize();
    fprintf (stderr, "Found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stderr, false), "dvoApplyCorrCleanup");

    return;
}

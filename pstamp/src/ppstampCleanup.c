#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppstamp.h"

void ppstampCleanup (pmConfig *config, ppstampOptions *options)
{
    // Free memory used by ppstamp
    psFree(config);
    psFree(options);

    // Free memory used by psModules
    pmConceptsDone();
    pmConfigDone();
    pmModelClassCleanup();

    // Free memory used by psLib
    psLibFinalize();

    // psMemCheckLeaks (0, NULL, stderr, false);
    // fprintf (stderr, "Found %d leaks in %s\n", psMemCheckLeaks (0, NULL, NULL, false), "ppstamp");

    return;
}
